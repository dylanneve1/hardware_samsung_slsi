/*
**
** Copyright 2015, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef EXYNOS_CAMERA_CLASS_COMMON_DEFINE
#define EXYNOS_CAMERA_CLASS_COMMON_DEFINE

#include <utils/threads.h>
#include <utils/RefBase.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <hardware/camera.h>
#include <hardware/camera3.h>
#include <hardware/gralloc.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <camera/CameraMetadata.h>
#include <media/hardware/MetadataBufferType.h>
#include <system/camera_metadata.h>

#include <fcntl.h>
#include <sys/mman.h>
#include "csc.h"

#include "ExynosCameraParameters.h"

#ifdef USE_CAMERA2_API_SUPPORT
#include "ExynosCamera3FrameFactory.h"
#else
#include "ExynosCameraFrameFactory.h"
#include "ExynosCameraFrameFactoryPreview.h"
#include "ExynosCameraFrameReprocessingFactory.h"
#include "ExynosCameraFrameFactoryVision.h"
#endif

#include "ExynosCameraMemory.h"
#include "ExynosCameraBufferManager.h"
#include "ExynosCameraBufferLocker.h"
#include "ExynosCameraActivityControl.h"
#include "ExynosCameraScalableSensor.h"
#include "ExynosCameraFrameSelector.h"
#ifdef CAMERA_VENDOR_TURNKEY_FEATURE
#include "Sfl/ExynosCameraSFLMgr.h"
#endif

namespace android {

typedef struct ExynosCameraJpegCallbackBuffer {
    ExynosCameraBuffer buffer;
    int callbackNumber;
} jpeg_callback_buffer_t;

typedef ExynosCameraList<ExynosCameraFrame *> frame_queue_t;
typedef ExynosCameraList<ExynosCameraBuffer*> buffer_queue_t;

typedef ExynosCameraList<uint32_t> worker_queue_t;
typedef ExynosCameraList<jpeg_callback_buffer_t> jpeg_callback_queue_t;
typedef ExynosCameraList<ExynosCameraBuffer> yuv_callback_queue_t;
typedef ExynosCameraList<ExynosCameraBuffer> postview_callback_queue_t;
typedef ExynosCameraList<ExynosCameraBuffer> thumbnail_callback_queue_t;
typedef ExynosCameraList<ExynosCameraFrame *> capture_queue_t;
#ifdef GED_DNG
typedef ExynosCameraList<ExynosCameraBuffer> dng_capture_queue_t;
typedef ExynosCameraList<ExynosCameraBuffer> bayer_release_queue_t;
#endif

typedef enum buffer_direction_type {
    SRC_BUFFER_DIRECTION        = 0,
    DST_BUFFER_DIRECTION        = 1,
    INVALID_BUFFER_DIRECTION,
} buffer_direction_type_t;

enum jpeg_save_thread {
    JPEG_SAVE_THREAD0       = 0,
    JPEG_SAVE_THREAD1       = 1,
    JPEG_SAVE_THREAD2,
    JPEG_SAVE_THREAD_MAX_COUNT,
};

/*
typedef struct {
    uint32_t frameNumber;
    camera3_stream_buffer streamBuffer;
} result_buffer_info_t;
*/

enum FRAME_FACTORY_TYPE {
    FRAME_FACTORY_TYPE_CAPTURE_PREVIEW = 0,
    FRAME_FACTORY_TYPE_RECORDING_PREVIEW,
    FRAME_FACTORY_TYPE_DUAL_PREVIEW,
    FRAME_FACTORY_TYPE_REPROCESSING,
    FRAME_FACTORY_TYPE_VISION,
    FRAME_FACTORY_TYPE_MAX,
};

enum EXYNOS_CAMERA_STREAM_CHARACTERISTICS_ID {
    HAL_STREAM_ID_RAW           = 0,
    HAL_STREAM_ID_PREVIEW       = 1,
    HAL_STREAM_ID_VIDEO         = 2,
    HAL_STREAM_ID_JPEG          = 3,
    HAL_STREAM_ID_CALLBACK      = 4,
    HAL_STREAM_ID_ZSL_INPUT     = 5,
    HAL_STREAM_ID_ZSL_OUTPUT    = 6,
    HAL_STREAM_ID_MAX           = 7,
};
}
#endif
