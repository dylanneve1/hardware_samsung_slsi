/*
**
** Copyright 2017, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_PIPE_GSC_H
#define EXYNOS_CAMERA_PIPE_GSC_H

#include "ExynosCameraSWPipe.h"
#include "csc.h"

#define CSC_HW_PROPERTY_DEFAULT ((CSC_HW_PROPERTY_TYPE)2) /* Not fixed mode */
#define CSC_MEMORY_TYPE         CSC_MEMORY_DMABUF /* (CSC_MEMORY_USERPTR) */

namespace android {

class ExynosCameraPipeGSC : protected virtual ExynosCameraSWPipe {
public:
    ExynosCameraPipeGSC()
    {
        m_init(NULL);
    }

    ExynosCameraPipeGSC(
        int cameraId,
        ExynosCameraConfigurations *configurations,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums) : ExynosCameraSWPipe(cameraId, configurations, obj_param, isReprocessing, nodeNums)
    {
        m_init(nodeNums);
    }

    virtual status_t        create(int32_t *sensorIds = NULL);

protected:
    virtual status_t        m_destroy(void);
    virtual status_t        m_run(void);

private:
    void                    m_init(int32_t *nodeNums);

private:
    int                     m_gscNum;
    void                   *m_csc;
    CSC_HW_PROPERTY_TYPE    m_property;
};

}; /* namespace android */

#endif
