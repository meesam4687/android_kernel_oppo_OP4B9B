/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/*
 * MAIN2 AF voice coil motor driver
 *
 *
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#ifdef VENDOR_EDIT
/*Henry.Chang@Camera.Drv add for main2AF 20190927*/
#include<soc/oppo/oppo_project.h>
#include <linux/regulator/consumer.h>
#endif
/* OIS/EIS Timer & Workqueue */
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
/* ------------------------- */

#include "lens_info.h"
#include "lens_list.h"

#define AF_DRVNAME "MAIN2AF"

#if defined(CONFIG_MTK_LEGACY)
#define I2C_CONFIG_SETTING 1
#elif defined(CONFIG_OF)
#define I2C_CONFIG_SETTING 2 /* device tree */
#else

#define I2C_CONFIG_SETTING 1
#endif


#if I2C_CONFIG_SETTING == 1
#ifdef VENDOR_EDIT
/*Henry.Chang@Camera.Drv add for main2AF 20190927*/
#define LENS_I2C_BUSNUM 2
#else
#define LENS_I2C_BUSNUM 0
#endif
#define I2C_REGISTER_ID            0x28
#endif

#define PLATFORM_DRIVER_NAME "lens_actuator_main2_af"
#define AF_DRIVER_CLASS_NAME "actuatordrv_main2_af"


#if I2C_CONFIG_SETTING == 1
static struct i2c_board_info kd_lens_dev __initdata = {
	I2C_BOARD_INFO(AF_DRVNAME, I2C_REGISTER_ID)
};
#endif

#define AF_DEBUG
#ifdef AF_DEBUG
#define LOG_INF(format, args...) pr_info(AF_DRVNAME " [%s] " format, __func__, ##args)
#else
#define LOG_INF(format, args...)
#endif

/* OIS/EIS Timer & Workqueue */
static struct workqueue_struct *ois_workqueue;
static struct work_struct ois_work;
static struct hrtimer ois_timer;

static DEFINE_MUTEX(ois_mutex);
static int g_EnableTimer;
static int g_GetOisInfoCnt;
static int g_OisPosIdx;
static struct stAF_OisPosInfo OisPosInfo;
/* ------------------------- */

static struct stAF_DrvList g_stAF_DrvList[MAX_NUM_OF_LENS] = {
	#ifdef VENDOR_EDIT
	/*Henry.Chang@Camera.Drv add for main2AF 20190927*/
	{1, AFDRV_DW9718TAF, DW9718TAF_SetI2Cclient, DW9718TAF_Ioctl, DW9718TAF_Release, NULL},
	#endif
	{1, AFDRV_LC898212XDAF_F, LC898212XDAF_F_SetI2Cclient, LC898212XDAF_F_Ioctl, LC898212XDAF_F_Release, NULL},
	{1, AFDRV_LC898217AF, LC898217AF_SetI2Cclient, LC898217AF_Ioctl, LC898217AF_Release, NULL},
	{1, AFDRV_LC898217AFA, LC898217AFA_SetI2Cclient, LC898217AF_Ioctl, LC898217AF_Release, NULL},
	{1, AFDRV_LC898217AFB, LC898217AFB_SetI2Cclient, LC898217AF_Ioctl, LC898217AF_Release, NULL},
	{1, AFDRV_LC898217AFC, LC898217AFC_SetI2Cclient, LC898217AF_Ioctl, LC898217AF_Release, NULL},
	#ifdef CONFIG_MTK_LENS_DW9800WAF_SUPPORT
	{1, AFDRV_DW9800WAF, DW9800WAF_SetI2Cclient, DW9800WAF_Ioctl, DW9800WAF_Release, NULL},
	#endif
	{1, AFDRV_AK7371AF, AK7371AF_SetI2Cclient, AK7371AF_Ioctl, AK7371AF_Release, NULL},
	{1, AFDRV_BU64748AF, bu64748af_SetI2Cclient_Main2, bu64748af_Ioctl_Main2, bu64748af_Release_Main2, NULL},
};

static struct stAF_DrvList *g_pstAF_CurDrv;

static spinlock_t g_AF_SpinLock;
static spinlock_t g_afupdown_SpinLock;

static int g_s4AF_Opened;

static struct i2c_client *g_pstAF_I2Cclient;

static dev_t g_AF_devno;
static struct cdev *g_pAF_CharDrv;
static struct class *actuator_class;
static struct device *lens_device;

#ifdef VENDOR_EDIT
/*Henry.Chang@Camera.Drv add for main2AF 20190927*/
static struct regulator *regVCAMAF_Main2;
static int g_regVCAMAF_Main2En;
int AF_UpDown = 0;
static void AFRegulatorCtrl_Main2(int Stage)
{
	LOG_INF("AFIOC_S_SETPOWERCTRL regulator_put %p %d\n", regVCAMAF_Main2, Stage);

	if (Stage == 0) {
		if (regVCAMAF_Main2 == NULL) {
			struct device_node *node, *kd_node;

			/* check if customer camera node defined */
			node = of_find_compatible_node(NULL, NULL, "mediatek,CAMERA_MAIN_TWO_AF");

			if (node) {
				kd_node = lens_device->of_node;
				lens_device->of_node = node;
				regVCAMAF_Main2 = regulator_get(lens_device, "vldo28");
				LOG_INF("[Init] regulator_get %p\n", regVCAMAF_Main2);

				lens_device->of_node = kd_node;
			}
		}
	} else if (Stage == 1) {
		if (regVCAMAF_Main2 != NULL && g_regVCAMAF_Main2En == 0) {
			int Status = regulator_is_enabled(regVCAMAF_Main2);

			LOG_INF("regulator_is_enabled %d\n", Status);
			LOG_INF("MAIN2_AF UP AF_updown=%d\n",AF_UpDown);
			if (!Status) {
				if(AF_UpDown < 1){
					Status = regulator_set_voltage(regVCAMAF_Main2, 2800000, 2800000);

					LOG_INF("regulator_set_voltage %d\n", Status);

					if (Status != 0)
						LOG_INF("regulator_set_voltage fail\n");

					Status = regulator_enable(regVCAMAF_Main2);
					LOG_INF("regulator_enable %d\n", Status);

					if (Status != 0)
						LOG_INF("regulator_enable fail\n");
				}


					g_regVCAMAF_Main2En = 1;
					usleep_range(5000, 5500);
			} else {
				LOG_INF("AF Power on\n");
				if(g_regVCAMAF_Main2En == 0)
					g_regVCAMAF_Main2En = 1;
			}
			spin_lock(&g_afupdown_SpinLock);
			AF_UpDown = AF_UpDown + 1;
			spin_unlock(&g_afupdown_SpinLock);
		}
	} else {
		if (regVCAMAF_Main2 != NULL && g_regVCAMAF_Main2En == 1) {
			int Status = regulator_is_enabled(regVCAMAF_Main2);

			LOG_INF("regulator_is_enabled %d\n", Status);

			if (Status) {
				LOG_INF("Camera Power enable\n");
				spin_lock(&g_afupdown_SpinLock);
				AF_UpDown = AF_UpDown - 1;
				spin_unlock(&g_afupdown_SpinLock);
				LOG_INF("MAIN2_AF DOWN AF_updown=%d\n",AF_UpDown);

				if(AF_UpDown < 1){
					Status = regulator_disable(regVCAMAF_Main2);
					LOG_INF("regulator_disable %d\n", Status);
					if (Status != 0)
					LOG_INF("Fail to regulator_disable\n");
					AF_UpDown = 0;
				}
			}
			/* regulator_put(regVCAMAF_Main2); */
			LOG_INF("AFIOC_S_SETPOWERCTRL regulator_put %p\n", regVCAMAF_Main2);
			/* regVCAMAF_Main2 = NULL; */
			g_regVCAMAF_Main2En = 0;
		}
	}
}
#endif

void MAIN2AF_PowerDown(void)
{
	if (g_pstAF_I2Cclient != NULL) {
		LOG_INF("CONFIG_MTK_PLATFORM : %s\n", CONFIG_MTK_PLATFORM);
		#ifdef VENDOR_EDIT
		/*Henry.Chang@Camera.Drv add for main2AF 20190927*/
		#ifdef CONFIG_MTK_LENS_DW9718TAF_SUPPORT
		DW9718TAF_SetI2Cclient(g_pstAF_I2Cclient, &g_AF_SpinLock, &g_s4AF_Opened);
		#endif
		#endif

		#if defined(CONFIG_MACH_MT6775)
		LC898217AF_SetI2Cclient(g_pstAF_I2Cclient, &g_AF_SpinLock, &g_s4AF_Opened);
		LC898217AF_PowerDown();
		#endif

		#if defined(CONFIG_MACH_MT6771)
		if (strncmp(CONFIG_ARCH_MTK_PROJECT, "k71v1_64_bsp_ducam", 18) == 0) {
			LC898217AF_SetI2Cclient(g_pstAF_I2Cclient, &g_AF_SpinLock, &g_s4AF_Opened);
			LC898217AF_PowerDown();
		}
		#endif

		#ifdef CONFIG_MACH_MT6758
		AK7371AF_SetI2Cclient(g_pstAF_I2Cclient, &g_AF_SpinLock, &g_s4AF_Opened);
		AK7371AF_PowerDown();
		#endif

		#ifdef CONFIG_MTK_LENS_DW9800WAF_SUPPORT
		DW9800WAF_SetI2Cclient(g_pstAF_I2Cclient, &g_AF_SpinLock, &g_s4AF_Opened);
		#endif
	}
}

static long AF_SetMotorName(__user struct stAF_MotorName *pstMotorName)
{
	long i4RetValue = -1;
	int i;
	struct stAF_MotorName stMotorName;

	if (copy_from_user(&stMotorName, pstMotorName, sizeof(struct stAF_MotorName)))
		LOG_INF("copy to user failed when getting motor information\n");

	for (i = 0; i < MAX_NUM_OF_LENS; i++) {
		if (g_stAF_DrvList[i].uEnable != 1)
			break;

		/* LOG_INF("Search Motor Name : %s\n", g_stAF_DrvList[i].uDrvName); */
		if (strcmp(stMotorName.uMotorName, g_stAF_DrvList[i].uDrvName) == 0) {
			LOG_INF("Set Motor Name : %s (%d)\n", stMotorName.uMotorName, i);
			g_pstAF_CurDrv = &g_stAF_DrvList[i];
			i4RetValue = g_pstAF_CurDrv->pAF_SetI2Cclient(g_pstAF_I2Cclient,
								&g_AF_SpinLock, &g_s4AF_Opened);
			break;
		}
	}
	return i4RetValue;
}

static inline int64_t getCurNS(void)
{
	int64_t ns;
	struct timespec time;

	time.tv_sec = time.tv_nsec = 0;
	get_monotonic_boottime(&time);
	ns = time.tv_sec * 1000000000LL + time.tv_nsec;

	return ns;
}

/* OIS/EIS Timer & Workqueue */
static void ois_pos_polling(struct work_struct *data)
{
	mutex_lock(&ois_mutex);
	if (g_pstAF_CurDrv) {
		if (g_pstAF_CurDrv->pAF_OisGetHallPos) {
			int PosX = 0, PosY = 0;

			g_pstAF_CurDrv->pAF_OisGetHallPos(&PosX, &PosY);
			OisPosInfo.TimeStamp[g_OisPosIdx] = getCurNS();
			OisPosInfo.i4OISHallPosX[g_OisPosIdx] = PosX;
			OisPosInfo.i4OISHallPosY[g_OisPosIdx] = PosY;
			g_OisPosIdx++;
			g_OisPosIdx &= OIS_DATA_MASK;
		}
	}
	mutex_unlock(&ois_mutex);
}

static enum hrtimer_restart ois_timer_func(struct hrtimer *timer)
{
	g_GetOisInfoCnt--;

	if (ois_workqueue != NULL && g_GetOisInfoCnt > 11)
		queue_work(ois_workqueue, &ois_work);

	if (g_GetOisInfoCnt < 10) {
		g_EnableTimer = 0;
		return HRTIMER_NORESTART;
	}

	hrtimer_forward_now(timer, ktime_set(0, 5000000));
	return HRTIMER_RESTART;
}
/* ------------------------- */

/* ////////////////////////////////////////////////////////////// */
static long AF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command, unsigned long a_u4Param)
{
	long i4RetValue = 0;

	switch (a_u4Command) {
	case AFIOC_S_SETDRVNAME:
		i4RetValue = AF_SetMotorName((__user struct stAF_MotorName *)(a_u4Param));
		break;
	#ifdef VENDOR_EDIT
	/*Henry.Chang@Camera.Drv add for main2AF 20190927*/
	case AFIOC_S_SETPOWERDOWN:
		MAIN2AF_PowerDown();
		i4RetValue = 1;
		break;

	case AFIOC_S_SETPOWERCTRL:
		AFRegulatorCtrl_Main2(0);

		if (a_u4Param > 0)
			AFRegulatorCtrl_Main2(1);
		break;
	#else
	#if !defined(CONFIG_MTK_LEGACY)
	case AFIOC_S_SETPOWERCTRL:
		AFRegulatorCtrl(0);

		if (a_u4Param > 0)
			AFRegulatorCtrl(1);
		break;
	#endif
	#endif

	case AFIOC_G_OISPOSINFO:
		if (g_pstAF_CurDrv) {
			if (g_pstAF_CurDrv->pAF_OisGetHallPos) {
				__user struct stAF_OisPosInfo *pstOisPosInfo =
				(__user struct stAF_OisPosInfo *)a_u4Param;

				mutex_lock(&ois_mutex);

				if (copy_to_user(pstOisPosInfo, &OisPosInfo, sizeof(struct stAF_OisPosInfo)))
					LOG_INF("copy to user failed when getting motor information\n");

				g_OisPosIdx = 0;
				g_GetOisInfoCnt = 100;
				memset(&OisPosInfo, 0, sizeof(OisPosInfo));
				mutex_unlock(&ois_mutex);

				if (g_EnableTimer == 0) {
					/* Start Timer */
					hrtimer_start(&ois_timer, ktime_set(0, 50000000), HRTIMER_MODE_REL);
					g_EnableTimer = 1;
				}

			}
		}
		break;

	default:
		if (g_pstAF_CurDrv)
			i4RetValue = g_pstAF_CurDrv->pAF_Ioctl(a_pstFile, a_u4Command, a_u4Param);
		break;
	}

	return i4RetValue;
}

#ifdef CONFIG_COMPAT
static long AF_Ioctl_Compat(struct file *a_pstFile, unsigned int a_u4Command, unsigned long a_u4Param)
{
	long i4RetValue = 0;

	i4RetValue = AF_Ioctl(a_pstFile, a_u4Command, (unsigned long)compat_ptr(a_u4Param));

	return i4RetValue;
}
#endif

/* Main jobs: */
/* 1.check for device-specified errors, device not ready. */
/* 2.Initialize the device if it is opened for the first time. */
/* 3.Update f_op pointer. */
/* 4.Fill data structures into private_data */
/* CAM_RESET */
static int AF_Open(struct inode *a_pstInode, struct file *a_pstFile)
{
	LOG_INF("Start\n");

	if (g_s4AF_Opened) {
		LOG_INF("The device is opened\n");
		return -EBUSY;
	}

	spin_lock(&g_AF_SpinLock);
	g_s4AF_Opened = 1;
	spin_unlock(&g_AF_SpinLock);
#ifdef VENDOR_EDIT
/*Henry.Chang@Camera.Drv add for main2AF 20190927*/
	AFRegulatorCtrl_Main2(1);
#else
#if !defined(CONFIG_MTK_LEGACY)
	AFRegulatorCtrl(1);
#endif
#endif

	/* OIS/EIS Timer & Workqueue */
	/* init work queue */
	INIT_WORK(&ois_work, ois_pos_polling);

	/* init timer */
	hrtimer_init(&ois_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ois_timer.function = ois_timer_func;

	g_EnableTimer = 0;
	/* ------------------------- */

	LOG_INF("End\n");

	return 0;
}

/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
static int AF_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	LOG_INF("Start\n");

	if (g_pstAF_CurDrv) {
		g_pstAF_CurDrv->pAF_Release(a_pstInode, a_pstFile);
		g_pstAF_CurDrv = NULL;
	} else {
		spin_lock(&g_AF_SpinLock);
		g_s4AF_Opened = 0;
		spin_unlock(&g_AF_SpinLock);
	}

#ifdef VENDOR_EDIT
/*Henry.Chang@Camera.Drv add for main2AF 20190927*/
	AFRegulatorCtrl_Main2(2);
#else
#if !defined(CONFIG_MTK_LEGACY)
	AFRegulatorCtrl(2);
#endif
#endif

	/* OIS/EIS Timer & Workqueue */
	/* Cancel Timer */
	hrtimer_cancel(&ois_timer);

	/* flush work queue */
	flush_work(&ois_work);

	if (ois_workqueue) {
		flush_workqueue(ois_workqueue);
		destroy_workqueue(ois_workqueue);
		ois_workqueue = NULL;
	}
	/* ------------------------- */

	LOG_INF("End\n");

	return 0;
}

static const struct file_operations g_stAF_fops = {
	.owner = THIS_MODULE,
	.open = AF_Open,
	.release = AF_Release,
	.unlocked_ioctl = AF_Ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = AF_Ioctl_Compat,
#endif
};

static inline int Register_AF_CharDrv(void)
{
	LOG_INF("Start\n");

	/* Allocate char driver no. */
	if (alloc_chrdev_region(&g_AF_devno, 0, 1, AF_DRVNAME)) {
		LOG_INF("Allocate device no failed\n");

		return -EAGAIN;
	}
	/* Allocate driver */
	g_pAF_CharDrv = cdev_alloc();

	if (g_pAF_CharDrv == NULL) {
		unregister_chrdev_region(g_AF_devno, 1);

		LOG_INF("Allocate mem for kobject failed\n");

		return -ENOMEM;
	}
	/* Attatch file operation. */
	cdev_init(g_pAF_CharDrv, &g_stAF_fops);

	g_pAF_CharDrv->owner = THIS_MODULE;

	/* Add to system */
	if (cdev_add(g_pAF_CharDrv, g_AF_devno, 1)) {
		LOG_INF("Attatch file operation failed\n");

		unregister_chrdev_region(g_AF_devno, 1);

		return -EAGAIN;
	}

	actuator_class = class_create(THIS_MODULE, AF_DRIVER_CLASS_NAME);
	if (IS_ERR(actuator_class)) {
		int ret = PTR_ERR(actuator_class);

		LOG_INF("Unable to create class, err = %d\n", ret);
		return ret;
	}

	lens_device = device_create(actuator_class, NULL, g_AF_devno, NULL, AF_DRVNAME);

	if (lens_device == NULL)
		return -EIO;

	LOG_INF("End\n");
	return 0;
}

static inline void Unregister_AF_CharDrv(void)
{
	LOG_INF("Start\n");

	/* Release char driver */
	cdev_del(g_pAF_CharDrv);

	unregister_chrdev_region(g_AF_devno, 1);

	device_destroy(actuator_class, g_AF_devno);

	class_destroy(actuator_class);

	LOG_INF("End\n");
}

/* //////////////////////////////////////////////////////////////////// */

static int AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int AF_i2c_remove(struct i2c_client *client);
static const struct i2c_device_id AF_i2c_id[] = { {AF_DRVNAME, 0}, {} };

/* Compatible name must be the same with that defined in codegen.dws and cust_i2c.dtsi */
/* TOOL : kernel-3.10\tools\dct */
/* PATH : vendor\mediatek\proprietary\custom\#project#\kernel\dct\dct */
#if I2C_CONFIG_SETTING == 2
static const struct of_device_id MAINAF_of_match[] = {
	{.compatible = "mediatek,CAMERA_MAIN_TWO_AF"},
	{},
};
#endif

static struct i2c_driver AF_i2c_driver = {
	.probe = AF_i2c_probe,
	.remove = AF_i2c_remove,
	.driver.name = AF_DRVNAME,
#if I2C_CONFIG_SETTING == 2
	.driver.of_match_table = MAINAF_of_match,
#endif
	.id_table = AF_i2c_id,
};

static int AF_i2c_remove(struct i2c_client *client)
{
	return 0;
}

/* Kirby: add new-style driver {*/
static int AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i4RetValue = 0;

	LOG_INF("Start\n");

	/* Kirby: add new-style driver { */
	g_pstAF_I2Cclient = client;

	/* Register char driver */
	i4RetValue = Register_AF_CharDrv();

	if (i4RetValue) {

		LOG_INF(" register char device failed!\n");

		return i4RetValue;
	}

	spin_lock_init(&g_AF_SpinLock);
	spin_lock_init(&g_afupdown_SpinLock);

#ifdef VENDOR_EDIT
/*Henry.Chang@Camera.Drv add for main2AF 20190927*/
	AFRegulatorCtrl_Main2(0);
#else
#if !defined(CONFIG_MTK_LEGACY)
	AFRegulatorCtrl(0);
#endif
#endif

	LOG_INF("Attached!!\n");

	return 0;
}

static int AF_probe(struct platform_device *pdev)
{
	return i2c_add_driver(&AF_i2c_driver);
}

static int AF_remove(struct platform_device *pdev)
{
	i2c_del_driver(&AF_i2c_driver);
	return 0;
}

static int AF_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int AF_resume(struct platform_device *pdev)
{
	return 0;
}

/* platform structure */
static struct platform_driver g_stAF_Driver = {
	.probe = AF_probe,
	.remove = AF_remove,
	.suspend = AF_suspend,
	.resume = AF_resume,
	.driver = {
		   .name = PLATFORM_DRIVER_NAME,
		   .owner = THIS_MODULE,
		   }
};

static struct platform_device g_stAF_device = {
	.name = PLATFORM_DRIVER_NAME,
	.id = 0,
	.dev = {}
};
#ifdef VENDOR_EDIT
/*Henry.Chang@Camera.Drv add for main2AF 20190927*/
static int __init MAIN2AF_i2C_init(void)
{
	#if I2C_CONFIG_SETTING == 1
	i2c_register_board_info(LENS_I2C_BUSNUM, &kd_lens_dev, 1);
	#endif

	if (platform_device_register(&g_stAF_device)) {
		LOG_INF("failed to register AF driver\n");
		return -ENODEV;
	}

	if (platform_driver_register(&g_stAF_Driver)) {
		LOG_INF("Failed to register AF driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit MAIN2AF_i2C_exit(void)
{
	platform_driver_unregister(&g_stAF_Driver);
}
module_init(MAIN2AF_i2C_init);
module_exit(MAIN2AF_i2C_exit);
MODULE_DESCRIPTION("MAIN2AF lens module driver");
MODULE_AUTHOR("KY Chen <ky.chen@Mediatek.com>");
MODULE_LICENSE("GPL");
#else
static int __init MAINAF_i2C_init(void)
{
	#if I2C_CONFIG_SETTING == 1
	i2c_register_board_info(LENS_I2C_BUSNUM, &kd_lens_dev, 1);
	#endif

	if (platform_device_register(&g_stAF_device)) {
		LOG_INF("failed to register AF driver\n");
		return -ENODEV;
	}

	if (platform_driver_register(&g_stAF_Driver)) {
		LOG_INF("Failed to register AF driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit MAINAF_i2C_exit(void)
{
	platform_driver_unregister(&g_stAF_Driver);
}
module_init(MAINAF_i2C_init);
module_exit(MAINAF_i2C_exit);
MODULE_DESCRIPTION("MAINAF lens module driver");
MODULE_AUTHOR("KY Chen <ky.chen@Mediatek.com>");
MODULE_LICENSE("GPL");
#endif