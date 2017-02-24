NVDM_SRC = middleware/MTK/nvdm

C_FILES += $(NVDM_SRC)/src/nvdm_port.c

ifeq ($(PRODUCT_VERSION),7687)
C_FILES += $(NVDM_SRC)/src/nvdm_cli.c
endif
ifeq ($(PRODUCT_VERSION),7697)
C_FILES += $(NVDM_SRC)/src/nvdm_cli.c
endif

 
  
  
#################################################################################
#include path
CFLAGS  += -I$(SOURCE_DIR)/$(NVDM_SRC)/inc
CFLAGS  += -I$(SOURCE_DIR)/$(NVDM_SRC)/inc_core
CFLAGS  += -I$(SOURCE_DIR)/middleware/util/include 
CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/minicli/include 
CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/include
CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source/include 
CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source/include/portable/GCC/ARM_CM4F
CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source
CFLAGS  += -I$(SOURCE_DIR)/middleware/mlog/include
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/mt7687/include
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/inc
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/nvdm/src_core/),)
include $(SOURCE_DIR)/middleware/MTK/nvdm/src_core/GCC/module.mk
else
LIBS += $(SOURCE_DIR)/middleware/MTK/nvdm/lib/libnvdm_CM4_GCC.a
endif

#################################################################################
#Enable the feature by configuring
CFLAGS += -DMTK_NVDM_ENABLE