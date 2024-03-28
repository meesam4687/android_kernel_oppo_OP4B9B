ifeq ($(CONFIG_OPPO_FINGERPRINT_QCOM),y)
PROJECT_NAME = $(subst MSM_,,$(OPPO_TARGET_DEVICE))
else ifeq ($(CONFIG_OPPO_FINGERPRINT_MTK),y)
PROJECT_NAME = $(shell echo $(TARGET_PRODUCT) | sed -e 's/full_oppo.*_//g')
endif

DEFS = -DCONFIG_OPPO_FINGERPRINT_PROJCT=$(PROJECT_NAME)
ccflags-y += $(DEFS)

ifeq ($(TARGET_PRODUCT),sdm660_64)
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=660
ifeq ($(PROJECT_NAME), 18323)
DEFS_IRQ = -DCONFIG_OPTICAL_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
else ifeq ($(PROJECT_NAME), 18321)
DEFS_IRQ = -DCONFIG_OPTICAL_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
else ifeq ($(PROJECT_NAME), 18005)
DEFS_IRQ = -DCONFIG_OPTICAL_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
endif
else ifeq ($(TARGET_PRODUCT),msm8953_64)
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=8953
else ifeq ($(TARGET_PRODUCT),msm8952_64)
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=8976
else ifeq ($(shell echo $(TARGET_PRODUCT) | sed -e 's/_1.*//g'),full_oppo6763)
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=6763
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/asf_inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/asf_export_inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/module
else ifeq ($(shell echo $(TARGET_PRODUCT) | sed -e 's/_1.*//g'),full_oppo6771)
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=6771
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/asf_inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/asf_export_inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/module
ifneq ($(filter 19531 19151 19350, $(PROJECT_NAME)),)
DEFS_IRQ = -DCONFIG_OPTICAL_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
endif
else ifeq ($(shell echo $(TARGET_PRODUCT) | sed -e 's/_1.*//g'),full_oppo6779)
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=6779
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/asf_inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/asf_export_inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/module
ifeq ($(PROJECT_NAME), 18073)
DEFS_IRQ = -DCONFIG_OPTICAL_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
else ifeq ($(PROJECT_NAME), 18593)
DEFS_IRQ = -DCONFIG_OPTICAL_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
else ifeq ($(PROJECT_NAME), 19011)
DEFS_IRQ = -DCONFIG_OPTICAL_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
else ifeq ($(PROJECT_NAME), 19301)
DEFS_IRQ = -DCONFIG_OPTICAL_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
endif
else ifeq ($(TARGET_PRODUCT),sdm710)
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=710
DEFS_IRQ = -DCONFIG_OPTICAL_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
else ifeq ($(TARGET_PRODUCT),msmnile)
ifneq ($(filter 19071 19081 19053 19371, $(PROJECT_NAME)),)
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=855
DEFS_IRQ = -DCONFIG_OPTICAL_GF_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
else
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=855
DEFS_IRQ = -DCONFIG_OPTICAL_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
endif
else ifeq ($(TARGET_PRODUCT),trinket)
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=6125
ccflags-y += $(DEFS_IRQ)
else ifeq ($(TARGET_PRODUCT),sm6150)
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=7150
DEFS_IRQ = -DCONFIG_OPTICAL_GF_IRQ_HANDLER
ccflags-y += $(DEFS_IRQ)
else
DEFS_PLATFORM = -DCONFIG_OPPO_FINGERPRINT_PLATFORM=0
endif

ccflags-y += $(DEFS_PLATFORM)

obj-y     +=  oppo_fp_common.o
