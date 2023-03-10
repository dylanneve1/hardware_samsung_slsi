/*
 * Copyright (C) 2017, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EXYNOS_CAMERA_PLUGIN_CONVERTER_ARCHSOFT_FUSION_H__
#define EXYNOS_CAMERA_PLUGIN_CONVERTER_ARCHSOFT_FUSION_H__

#include "ExynosCameraPlugInConverter.h"

#include "ExynosCameraParameters.h"
#include "ExynosCameraConfigurations.h"

namespace android {

class ExynosCameraPlugInConverterArcsoftFusion : public virtual ExynosCameraPlugInConverter {
public:
    ExynosCameraPlugInConverterArcsoftFusion() : ExynosCameraPlugInConverter()
    {
        m_init();
    }

    ExynosCameraPlugInConverterArcsoftFusion(int cameraId, int pipeId) : ExynosCameraPlugInConverter(cameraId, pipeId)
    {
        m_init();
    }

    virtual ~ExynosCameraPlugInConverterArcsoftFusion() { ALOGD("%s", __FUNCTION__); };

protected:
    // inherit this function.
    virtual status_t m_init(void);
    //virtual status_t m_deinit(void);
    //virtual status_t m_create(Map_t *map);
    //virtual status_t m_setup(Map_t *map);
    //virtual status_t m_make(Map_t *map);

protected:
    // help function.

private:
    // for default converting to send the plugIn

public:
    static void getZoomRatioList(float *list, int maxZoom, float maxZoomRatio);
    static int  getOtherDualCameraId(int cameraId);
    static int  frameType2SyncType(frame_type_t frameType);
    static enum DUAL_OPERATION_MODE frameType2DualOperationMode(frame_type_t frameType);
    static void getMetaData(ExynosCameraFrameSP_sptr_t frame, const struct camera2_shot_ext *metaData[2]);
};
}; /* namespace android */
#endif
