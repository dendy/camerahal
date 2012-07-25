ifeq ($(TARGET_BOARD_PLATFORM),omap4)
ifeq ($(TI_OMAP4_CAMERAHAL_VARIANT),)

LOCAL_PATH:= $(call my-dir)

#OMAP4_CAMERA_HAL_USES:= OMX
#OMAP4_CAMERA_HAL_USES:= USB
OMAP4_CAMERA_HAL_USES:= ALL

ifdef TI_CAMERAHAL_DEBUG_ENABLED
    # Enable CameraHAL debug logs
    CAMERAHAL_CFLAGS += -DCAMERAHAL_DEBUG
endif

ifdef TI_CAMERAHAL_VERBOSE_DEBUG_ENABLED
    # Enable CameraHAL verbose debug logs
    CAMERAHAL_CFLAGS += -DCAMERAHAL_DEBUG_VERBOSE
endif

ifdef TI_CAMERAHAL_DEBUG_FUNCTION_NAMES
    # Enable CameraHAL function enter/exit logging
    CAMERAHAL_CFLAGS += -DTI_UTILS_FUNCTION_LOGGER_ENABLE
endif

ifdef TI_CAMERAHAL_DEBUG_TIMESTAMPS
    # Enable timestamp logging
    CAMERAHAL_CFLAGS += -DTI_UTILS_DEBUG_USE_TIMESTAMPS
endif

ifndef TI_CAMERAHAL_DONT_USE_RAW_IMAGE_SAVING
    # Enabled saving RAW images to file
    CAMERAHAL_CFLAGS += -DCAMERAHAL_USE_RAW_IMAGE_SAVING
endif

ifdef TI_CAMERAHAL_PROFILING
    # Enable OMX Camera component profiling
    CAMERAHAL_CFLAGS += -DCAMERAHAL_OMX_PROFILING
endif

CAMERAHAL_CFLAGS += -DLOG_TAG=\"CameraHal\"

TI_CAMERAHAL_COMMON_INCLUDES := \
    hardware/ti/omap4xxx/tiler \
    hardware/ti/omap4xxx/hwc \
    external/jpeg \
    external/jhead \
    $(LOCAL_PATH)/../libtiutils \
    $(LOCAL_PATH)/inc

TI_CAMERAHAL_COMMON_INCLUDES += \
    frameworks/base/include/media/stagefright \
    hardware/ti/omap4xxx/include

TI_CAMERAHAL_COMMON_SRC := \
    CameraHal_Module.cpp \
    CameraHal.cpp \
    CameraHalUtilClasses.cpp \
    AppCallbackNotifier.cpp \
    ANativeWindowDisplayAdapter.cpp \
    BufferSourceAdapter.cpp \
    CameraProperties.cpp \
    BaseCameraAdapter.cpp \
    MemoryManager.cpp \
    Encoder_libjpeg.cpp \
    SensorListener.cpp  \
    NV12_resize.cpp \
    CameraParameters.cpp \
    TICameraParameters.cpp \
    CameraHalCommon.cpp

TI_CAMERAHAL_OMX_SRC := \
    OMXCameraAdapter/OMX3A.cpp \
    OMXCameraAdapter/OMXAlgo.cpp \
    OMXCameraAdapter/OMXCameraAdapter.cpp \
    OMXCameraAdapter/OMXCapabilities.cpp \
    OMXCameraAdapter/OMXCapture.cpp \
    OMXCameraAdapter/OMXReprocess.cpp \
    OMXCameraAdapter/OMXDefaults.cpp \
    OMXCameraAdapter/OMXExif.cpp \
    OMXCameraAdapter/OMXFD.cpp \
    OMXCameraAdapter/OMXFocus.cpp \
    OMXCameraAdapter/OMXMetadata.cpp \
    OMXCameraAdapter/OMXZoom.cpp \
    OMXCameraAdapter/OMXDccDataSave.cpp

TI_CAMERAHAL_USB_SRC := \
    V4LCameraAdapter/V4LCameraAdapter.cpp \
    V4LCameraAdapter/V4LCapabilities.cpp

TI_CAMERAHAL_COMMON_SHARED_LIBRARIES := \
    libui \
    libbinder \
    libutils \
    libcutils \
    libtiutils \
    libcamera_client \
    libgui \
    libion \
    libjpeg \
    libexif


# ====================
#  OMX Camera Adapter
# --------------------

ifeq ($(OMAP4_CAMERA_HAL_USES),OMX)

include $(CLEAR_VARS)

CAMERAHAL_CFLAGS += -DOMX_CAMERA_ADAPTER

LOCAL_SRC_FILES:= \
    $(TI_CAMERAHAL_COMMON_SRC) \
    $(TI_CAMERAHAL_OMX_SRC)

LOCAL_C_INCLUDES += \
    $(TI_CAMERAHAL_COMMON_INCLUDES) \
    $(DOMX_PATH)/omx_core/inc \
    $(DOMX_PATH)/mm_osal/inc \
    $(LOCAL_PATH)/inc/OMXCameraAdapter

LOCAL_SHARED_LIBRARIES:= \
    $(TI_CAMERAHAL_COMMON_SHARED_LIBRARIES) \
    libmm_osal \
    libOMX_Core \
    libdomx

LOCAL_CFLAGS := -fno-short-enums -DCOPY_IMAGE_BUFFER $(CAMERAHAL_CFLAGS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS:= optional

include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)

else
ifeq ($(OMAP4_CAMERA_HAL_USES),USB)


# ====================
#  USB Camera Adapter
# --------------------

include $(CLEAR_VARS)

CAMERAHAL_CFLAGS += -DV4L_CAMERA_ADAPTER

LOCAL_SRC_FILES:= \
    $(TI_CAMERAHAL_COMMON_SRC) \
    $(TI_CAMERAHAL_USB_SRC)

LOCAL_C_INCLUDES += \
    $(TI_CAMERAHAL_COMMON_INCLUDES) \
    $(LOCAL_PATH)/inc/V4LCameraAdapter

LOCAL_SHARED_LIBRARIES:= \
    $(TI_CAMERAHAL_COMMON_SHARED_LIBRARIES)

LOCAL_CFLAGS := -fno-short-enums -DCOPY_IMAGE_BUFFER $(CAMERAHAL_CFLAGS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS:= optional

include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)

else
ifeq ($(OMAP4_CAMERA_HAL_USES),ALL)


# =====================
#  ALL Camera Adapters
# ---------------------

include $(CLEAR_VARS)

CAMERAHAL_CFLAGS += -DOMX_CAMERA_ADAPTER -DV4L_CAMERA_ADAPTER

LOCAL_SRC_FILES:= \
    $(TI_CAMERAHAL_COMMON_SRC) \
    $(TI_CAMERAHAL_OMX_SRC) \
    $(TI_CAMERAHAL_USB_SRC)

LOCAL_C_INCLUDES += \
    $(TI_CAMERAHAL_COMMON_INCLUDES) \
    $(DOMX_PATH)/omx_core/inc \
    $(DOMX_PATH)/mm_osal/inc \
    $(LOCAL_PATH)/inc/OMXCameraAdapter \
    $(LOCAL_PATH)/inc/V4LCameraAdapter

LOCAL_SHARED_LIBRARIES:= \
    $(TI_CAMERAHAL_COMMON_SHARED_LIBRARIES) \
    libmm_osal \
    libOMX_Core \
    libdomx

LOCAL_CFLAGS := -fno-short-enums -DCOPY_IMAGE_BUFFER $(CAMERAHAL_CFLAGS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS:= optional

include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)

endif
endif
endif
endif
endif
