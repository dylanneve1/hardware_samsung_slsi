/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file       Exynos_OMX_ABasecomponent.c
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             Yunji Kim (yunji.kim@samsung.com)
 * @version    2.0.0
 * @history
 *    2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Exynos_OSAL_Event.h"
#include "Exynos_OSAL_Thread.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OMX_ABaseport.h"
#include "Exynos_OMX_ABasecomponent.h"
#include "Exynos_OMX_Resourcemanager.h"
#include "Exynos_OMX_Macros.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_ABASE_COMP"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

static OMX_ERRORTYPE Exynos_SetPortFlush(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam);
static OMX_ERRORTYPE Exynos_SetPortEnable(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam);
static OMX_ERRORTYPE Exynos_SetPortDisable(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam);

/* Change CHECK_SIZE_VERSION Macro */
OMX_ERRORTYPE Exynos_OMX_Check_SizeVersion(OMX_PTR header, OMX_U32 size)
{
    OMX_ERRORTYPE        ret        = OMX_ErrorNone;
    OMX_VERSIONTYPE     *version    = NULL;

    if (header == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    version = (OMX_VERSIONTYPE*)((char*)header + sizeof(OMX_U32));
    if (*((OMX_U32*)header) != size) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_FUNC_TRACE, "[%s] nVersionMajor:%d, nVersionMinor:%d", __FUNCTION__, version->s.nVersionMajor, version->s.nVersionMinor);

    if ((version->s.nVersionMajor != VERSIONMAJOR_NUMBER) ||
        (version->s.nVersionMinor > VERSIONMINOR_NUMBER)) {
        ret = OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetComponentVersion(
    OMX_IN  OMX_HANDLETYPE   hComponent,
    OMX_OUT OMX_STRING       pComponentName,
    OMX_OUT OMX_VERSIONTYPE *pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE *pSpecVersion,
    OMX_OUT OMX_UUIDTYPE    *pComponentUUID)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    unsigned long             compUUID[3];

    FunctionIn();

    /* check parameters */
    if (hComponent     == NULL ||
        pComponentName == NULL || pComponentVersion == NULL ||
        pSpecVersion   == NULL || pComponentUUID    == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    Exynos_OSAL_Strcpy(pComponentName, pExynosComponent->componentName);
    Exynos_OSAL_Memcpy(pComponentVersion, &(pExynosComponent->componentVersion), sizeof(OMX_VERSIONTYPE));
    Exynos_OSAL_Memcpy(pSpecVersion, &(pExynosComponent->specVersion), sizeof(OMX_VERSIONTYPE));

    /* Fill UUID with handle address, PID and UID.
     * This should guarantee uiniqness */
    compUUID[0] = (unsigned long)pOMXComponent;
    compUUID[1] = (unsigned long)getpid();
    compUUID[2] = (unsigned long)getuid();
    Exynos_OSAL_Memcpy(*pComponentUUID, compUUID, 3 * sizeof(*compUUID));

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetState (
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_OUT OMX_STATETYPE *pState)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pState == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    *pState = pExynosComponent->currentState;
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentStateSet(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 messageParam)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_MESSAGE       *message;
    OMX_STATETYPE             destState = messageParam;
    OMX_STATETYPE             currentState = pExynosComponent->currentState;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    unsigned int              i = 0, j = 0;
    int                       k = 0;

    FunctionIn();

    /* check parameters */
    if (currentState == destState) {
         ret = OMX_ErrorSameState;
            goto EXIT;
    }
    if (currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_INFO, "[%p][%s] current:(%s) dest:(%s)", pExynosComponent, __FUNCTION__, stateString(currentState), stateString(destState));
    switch (destState) {
    case OMX_StateInvalid:
        switch (currentState) {
        case OMX_StateWaitForResources:
            Exynos_OMX_Out_WaitForResource(pOMXComponent);
        case OMX_StateIdle:
        case OMX_StateExecuting:
        case OMX_StatePause:
        case OMX_StateLoaded:
            pExynosComponent->currentState = OMX_StateInvalid;
            ret = pExynosComponent->exynos_BufferProcessTerminate(pOMXComponent);

            for (i = 0; i < ALL_PORT_NUM; i++) {
                Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].dataBuffer.bufferMutex);
                pExynosComponent->pExynosPort[i].dataBuffer.bufferMutex = NULL;

                Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].hPortMutex);
                pExynosComponent->pExynosPort[i].hPortMutex = NULL;
            }

            Exynos_OSAL_SignalTerminate(pExynosComponent->pauseEvent);
            pExynosComponent->pauseEvent = NULL;

            for (i = 0; i < ALL_PORT_NUM; i++) {
                Exynos_OSAL_SemaphoreTerminate(pExynosComponent->pExynosPort[i].bufferSemID);
                pExynosComponent->pExynosPort[i].bufferSemID = NULL;
            }

            if (currentState != OMX_StateLoaded)
                pExynosComponent->exynos_codec_componentTerminate(pOMXComponent);

            Exynos_OSAL_SignalSet(pExynosComponent->abendStateEvent);

            ret = OMX_ErrorInvalidState;
            break;
        default:
            ret = OMX_ErrorInvalidState;
            break;
        }
        break;
    case OMX_StateLoaded:
        switch (currentState) {
        case OMX_StateIdle:
            for(i = 0; i < pExynosComponent->portParam.nPorts; i++)
                pExynosComponent->pExynosPort[i].portState = EXYNOS_OMX_PortStateDisabling;

            ret = pExynosComponent->exynos_BufferProcessTerminate(pOMXComponent);

            for (i = 0; i < ALL_PORT_NUM; i++) {
                Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].dataBuffer.bufferMutex);
                pExynosComponent->pExynosPort[i].dataBuffer.bufferMutex = NULL;

                Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].hPortMutex);
                pExynosComponent->pExynosPort[i].hPortMutex = NULL;
            }

            Exynos_OSAL_SignalTerminate(pExynosComponent->pauseEvent);
            pExynosComponent->pauseEvent = NULL;

            for (i = 0; i < ALL_PORT_NUM; i++) {
                Exynos_OSAL_SemaphoreTerminate(pExynosComponent->pExynosPort[i].bufferSemID);
                pExynosComponent->pExynosPort[i].bufferSemID = NULL;
            }

            pExynosComponent->exynos_codec_componentTerminate(pOMXComponent);

            for (i = 0; i < (pExynosComponent->portParam.nPorts); i++) {
                pExynosPort = (pExynosComponent->pExynosPort + i);
#ifdef TUNNELING_SUPPORT
                if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    while (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) > 0) {
                        message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
                        if (message != NULL)
                            Exynos_OSAL_Free(message);
                    }
                    ret = pExynosComponent->exynos_FreeTunnelBuffer(pExynosPort, i);
                    if (OMX_ErrorNone != ret) {
                        goto EXIT;
                    }
                } else
#endif
                {
                    if (CHECK_PORT_ENABLED(pExynosPort)) {
                        if (pExynosPort->assignedBufferNum > 0) {
                            Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] %s port is waiting for unloadedResource",
                                            pExynosComponent, __FUNCTION__,
                                            (i == INPUT_PORT_INDEX)? "input":"output");
                            Exynos_OSAL_SemaphoreWait(pExynosPort->unloadedResource);
                            Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] unloadedResource about %s port is posted",
                                            pExynosComponent, __FUNCTION__,
                                            (i == INPUT_PORT_INDEX)? "input":"output");
                        }

                        while (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) > 0) {
                            message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
                            if (message != NULL)
                                Exynos_OSAL_Free(message);
                        }

                        Exynos_OSAL_Set_SemaphoreCount(pExynosPort->unloadedResource, 0);
                    }
                }

                if (pExynosComponent->abendState != OMX_TRUE)
                    pExynosPort->portState = EXYNOS_OMX_PortStateLoaded;
            }

            /*  this signal will be handled by invalid state handling
            if (pExynosComponent->abendState == OMX_TRUE) {
                Exynos_OSAL_SignalSet(pExynosComponent->abendStateEvent);
                goto EXIT;
            }
            */

            pExynosComponent->transientState = EXYNOS_OMX_TransStateMax;
            pExynosComponent->currentState = OMX_StateLoaded;
            break;
        case OMX_StateWaitForResources:
            ret = Exynos_OMX_Out_WaitForResource(pOMXComponent);
            pExynosComponent->currentState = OMX_StateLoaded;
            break;
        case OMX_StateExecuting:
        case OMX_StatePause:
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StateIdle:
        switch (currentState) {
        case OMX_StateLoaded:
            for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
                pExynosPort = &(pExynosComponent->pExynosPort[i]);

                pExynosPort->portState = EXYNOS_OMX_PortStateEnabling;

#ifdef TUNNELING_SUPPORT
                if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    if (CHECK_PORT_ENABLED(pExynosPort)) {
                        ret = pExynosComponent->exynos_AllocateTunnelBuffer(pExynosPort, i);
                        if (ret!=OMX_ErrorNone)
                            goto EXIT;
                    }
                } else
#endif
                {
                    if (CHECK_PORT_ENABLED(pExynosPort)) {
                        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] %s port is waiting for loadedResource",
                                        pExynosComponent, __FUNCTION__,
                                        (i == INPUT_PORT_INDEX)? "input":"output");
                        Exynos_OSAL_SemaphoreWait(pExynosComponent->pExynosPort[i].loadedResource);
                        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] loadedResource about %s port is posted",
                                        pExynosComponent, __FUNCTION__,
                                        (i == INPUT_PORT_INDEX)? "input":"output");

                        Exynos_OSAL_Set_SemaphoreCount(pExynosPort->loadedResource, 0);
                    }
                }

                if (pExynosComponent->abendState != OMX_TRUE)
                    pExynosPort->portState = EXYNOS_OMX_PortStateIdle;
            }

            if (pExynosComponent->abendState == OMX_TRUE) {
                Exynos_OSAL_SignalSet(pExynosComponent->abendStateEvent);
                goto EXIT;
            }

            Exynos_OSAL_Get_Log_Property(); // For debuging, Function called when GetHandle function is success
            ret = pExynosComponent->exynos_codec_componentInit(pOMXComponent);
            if (ret != OMX_ErrorNone) {
#ifdef TUNNELING_SUPPORT
                /*
                 * if (CHECK_PORT_TUNNELED == OMX_TRUE) thenTunnel Buffer Free
                 */
#endif
                Exynos_OSAL_SignalSet(pExynosComponent->abendStateEvent);
                goto EXIT;
            }

            Exynos_OSAL_SignalCreate(&pExynosComponent->pauseEvent);

            for (i = 0; i < ALL_PORT_NUM; i++) {
                ret = Exynos_OSAL_SemaphoreCreate(&pExynosComponent->pExynosPort[i].bufferSemID);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s] Failed to SemaphoreCreate (0x%x)", pExynosComponent, __FUNCTION__, ret);
                    goto EXIT;
                }
            }
            for (i = 0; i < ALL_PORT_NUM; i++) {
                ret = Exynos_OSAL_MutexCreate(&pExynosComponent->pExynosPort[i].dataBuffer.bufferMutex);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s] Failed to MutexCreate (0x%x) Line:%d", pExynosComponent, __FUNCTION__, ret, __LINE__);
                    goto EXIT;
                }

                ret = Exynos_OSAL_MutexCreate(&pExynosComponent->pExynosPort[i].hPortMutex);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s] Failed to MutexCreate (0x%x) Line:%d", pExynosComponent, __FUNCTION__, ret, __LINE__);
                    goto EXIT;
                }
            }

            ret = pExynosComponent->exynos_BufferProcessCreate(pOMXComponent);
            if (ret != OMX_ErrorNone) {
#ifdef TUNNELING_SUPPORT
                /*
                 * if (CHECK_PORT_TUNNELED == OMX_TRUE) thenTunnel Buffer Free
                 */
#endif
                Exynos_OSAL_SignalTerminate(pExynosComponent->pauseEvent);
                pExynosComponent->pauseEvent = NULL;

                for (i = 0; i < ALL_PORT_NUM; i++) {
                    Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].dataBuffer.bufferMutex);
                    pExynosComponent->pExynosPort[i].dataBuffer.bufferMutex = NULL;

                    Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].hPortMutex);
                    pExynosComponent->pExynosPort[i].hPortMutex = NULL;
                }
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    Exynos_OSAL_SemaphoreTerminate(pExynosComponent->pExynosPort[i].bufferSemID);
                    pExynosComponent->pExynosPort[i].bufferSemID = NULL;
                }

                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            pExynosComponent->transientState = EXYNOS_OMX_TransStateMax;
            pExynosComponent->currentState = OMX_StateIdle;
            break;
        case OMX_StateExecuting:
            Exynos_SetPortFlush(pExynosComponent, ALL_PORT_INDEX);
            Exynos_OMX_BufferFlushProcess(pOMXComponent, ALL_PORT_INDEX, OMX_FALSE);
            pExynosComponent->transientState = EXYNOS_OMX_TransStateMax;
            pExynosComponent->currentState = OMX_StateIdle;
            break;
        case OMX_StatePause:
            Exynos_SetPortFlush(pExynosComponent, ALL_PORT_INDEX);
            Exynos_OMX_BufferFlushProcess(pOMXComponent, ALL_PORT_INDEX, OMX_FALSE);
            pExynosComponent->currentState = OMX_StateIdle;
            break;
        case OMX_StateWaitForResources:
            pExynosComponent->currentState = OMX_StateIdle;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StateExecuting:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        case OMX_StateIdle:
            for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
                pExynosPort = &pExynosComponent->pExynosPort[i];
                if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort) && CHECK_PORT_ENABLED(pExynosPort)) {
                    for (j = 0; j < pExynosPort->tunnelBufferNum; j++) {
                        Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[i].bufferSemID);
                    }
                }
            }

            pExynosComponent->transientState = EXYNOS_OMX_TransStateMax;
            pExynosComponent->currentState = OMX_StateExecuting;

            Exynos_OSAL_SignalSet(pExynosComponent->pauseEvent);
            break;
        case OMX_StatePause:
            for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
                pExynosPort = &pExynosComponent->pExynosPort[i];
                if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort) && CHECK_PORT_ENABLED(pExynosPort)) {
                    OMX_S32 semaValue = 0, cnt = 0;
                    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[i].bufferSemID, &semaValue);
                    if (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) > semaValue) {
                        cnt = Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) - semaValue;
                        for (k = 0; k < cnt; k++) {
                            Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[i].bufferSemID);
                        }
                    }
                }
            }

            pExynosComponent->currentState = OMX_StateExecuting;

            Exynos_OSAL_SignalSet(pExynosComponent->pauseEvent);
            break;
        case OMX_StateWaitForResources:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StatePause:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        case OMX_StateIdle:
            pExynosComponent->currentState = OMX_StatePause;
            break;
        case OMX_StateExecuting:
            pExynosComponent->currentState = OMX_StatePause;
            break;
        case OMX_StateWaitForResources:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StateWaitForResources:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = Exynos_OMX_In_WaitForResource(pOMXComponent);
            pExynosComponent->currentState = OMX_StateWaitForResources;
            break;
        case OMX_StateIdle:
        case OMX_StateExecuting:
        case OMX_StatePause:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    default:
        ret = OMX_ErrorIncorrectStateTransition;
        break;
    }

EXIT:
    if (ret == OMX_ErrorNone) {
        if (pExynosComponent->pCallbacks != NULL) {
            Exynos_OSAL_Log(EXYNOS_LOG_INFO, "[%p][%s] OMX_EventCmdComplete", pExynosComponent, __FUNCTION__);
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
            pExynosComponent->callbackData,
            OMX_EventCmdComplete, OMX_CommandStateSet,
            destState, NULL);
        }
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s] (0x%x)", pExynosComponent, __FUNCTION__, ret);
        if (pExynosComponent->pCallbacks != NULL) {
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
            pExynosComponent->callbackData,
            OMX_EventError, ret, 0, NULL);
        }
    }
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_MessageHandlerThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;
    OMX_U32                   messageType = 0, portIndex = 0;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    while (pExynosComponent->bExitMessageHandlerThread == OMX_FALSE) {
        Exynos_OSAL_SemaphoreWait(pExynosComponent->msgSemaphoreHandle);
        message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosComponent->messageQ);
        if (message != NULL) {
            messageType = message->messageType;
            switch (messageType) {
            case OMX_CommandStateSet:
                ret = Exynos_OMX_ComponentStateSet(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandFlush:
                Exynos_SetPortFlush(pExynosComponent, message->messageParam);
                ret = Exynos_OMX_BufferFlushProcess(pOMXComponent, message->messageParam, OMX_TRUE);
                break;
            case OMX_CommandPortDisable:
                Exynos_SetPortDisable(pExynosComponent, message->messageParam);
                ret = Exynos_OMX_PortDisableProcess(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandPortEnable:
                Exynos_SetPortEnable(pExynosComponent, message->messageParam);
                ret = Exynos_OMX_PortEnableProcess(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandMarkBuffer:
                portIndex = message->messageParam;
                pExynosComponent->pExynosPort[portIndex].markType.hMarkTargetComponent = ((OMX_MARKTYPE *)message->pCmdData)->hMarkTargetComponent;
                pExynosComponent->pExynosPort[portIndex].markType.pMarkData            = ((OMX_MARKTYPE *)message->pCmdData)->pMarkData;
                break;
            case (OMX_COMMANDTYPE)EXYNOS_OMX_CommandComponentDeInit:
                pExynosComponent->bExitMessageHandlerThread = OMX_TRUE;
                break;
            default:
                break;
            }
            Exynos_OSAL_Free(message);
            message = NULL;
        }
    }

    Exynos_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_SetPortFlush(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE        ret         = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT *pExynosPort = NULL;
    OMX_S32              nPortIndex  = nParam;

    OMX_U16 i = 0, cnt = 0, index = 0;


    if ((pExynosComponent->currentState == OMX_StateExecuting) ||
        (pExynosComponent->currentState == OMX_StatePause)) {
        if ((nPortIndex != ALL_PORT_INDEX) &&
           ((OMX_S32)nPortIndex >= (OMX_S32)pExynosComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if (nPortIndex == ALL_PORT_INDEX) {
            for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
                pExynosPort = &pExynosComponent->pExynosPort[i];
                if (!CHECK_PORT_ENABLED(pExynosPort)) {
                    ret = OMX_ErrorIncorrectStateOperation;
                    goto EXIT;
                }

                pExynosPort->portState = EXYNOS_OMX_PortStateFlushing;
            }
        } else {
            pExynosPort = &pExynosComponent->pExynosPort[nPortIndex];
            if (!CHECK_PORT_ENABLED(pExynosPort)) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }

            pExynosPort->portState = EXYNOS_OMX_PortStateFlushing;
        }
    } else {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    return ret;
}

static OMX_ERRORTYPE Exynos_SetPortEnable(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE        ret            = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT *pExynosPort    = NULL;
    OMX_S32              nPortIndex     = nParam;

    OMX_U16 i = 0;

    FunctionIn();

    if ((nPortIndex != ALL_PORT_INDEX) &&
        ((OMX_S32)nPortIndex >= (OMX_S32)pExynosComponent->portParam.nPorts)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if (nPortIndex == ALL_PORT_INDEX) {
        for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
            pExynosPort = &pExynosComponent->pExynosPort[i];
            if (CHECK_PORT_ENABLED(pExynosPort)) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }

            pExynosPort->portState = EXYNOS_OMX_PortStateEnabling;
        }
    } else {
        pExynosPort = &pExynosComponent->pExynosPort[nPortIndex];
        if (CHECK_PORT_ENABLED(pExynosPort)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        pExynosPort->portState = EXYNOS_OMX_PortStateEnabling;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;

}

static OMX_ERRORTYPE Exynos_SetPortDisable(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE        ret            = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT *pExynosPort    = NULL;
    OMX_S32              nPortIndex     = nParam;

    OMX_U16 i = 0;

    FunctionIn();

    if ((nPortIndex != ALL_PORT_INDEX) &&
        ((OMX_S32)nPortIndex >= (OMX_S32)pExynosComponent->portParam.nPorts)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if (nPortIndex == ALL_PORT_INDEX) {
        for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
            pExynosPort = &pExynosComponent->pExynosPort[i];
            if (!CHECK_PORT_ENABLED(pExynosPort)) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }

            pExynosPort->portState = EXYNOS_OMX_PortStateDisabling;
        }
    } else {
        pExynosPort = &pExynosComponent->pExynosPort[nPortIndex];
        if (!CHECK_PORT_ENABLED(pExynosPort)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        pExynosPort->portState = EXYNOS_OMX_PortStateDisabling;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_SetMarkBuffer(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (nParam >= pExynosComponent->portParam.nPorts) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((pExynosComponent->currentState == OMX_StateExecuting) ||
        (pExynosComponent->currentState == OMX_StatePause)) {
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorIncorrectStateOperation;
    }

EXIT:
    return ret;
}

static OMX_ERRORTYPE Exynos_StateSet(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam)
{
    OMX_U32 destState = nParam;
    OMX_U32 i = 0;

    if ((destState == OMX_StateIdle) && (pExynosComponent->currentState == OMX_StateLoaded)) {
        pExynosComponent->transientState = EXYNOS_OMX_TransStateLoadedToIdle;

        for(i = 0; i < pExynosComponent->portParam.nPorts; i++) {
            pExynosComponent->pExynosPort[i].portState = EXYNOS_OMX_PortStateEnabling;
        }

        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] OMX_StateLoaded to OMX_StateIdle", pExynosComponent, __FUNCTION__);
    } else if ((destState == OMX_StateLoaded) && (pExynosComponent->currentState == OMX_StateIdle)) {
        pExynosComponent->transientState = EXYNOS_OMX_TransStateIdleToLoaded;

        for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
            pExynosComponent->pExynosPort[i].portState = EXYNOS_OMX_PortStateDisabling;
        }

        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] OMX_StateIdle to OMX_StateLoaded", pExynosComponent, __FUNCTION__);
    } else if ((destState == OMX_StateIdle) && (pExynosComponent->currentState == OMX_StateExecuting)) {
        EXYNOS_OMX_BASEPORT *pExynosPort = NULL;

        pExynosComponent->transientState = EXYNOS_OMX_TransStateExecutingToIdle;

        pExynosPort = &(pExynosComponent->pExynosPort[INPUT_PORT_INDEX]);
        if ((pExynosPort->portDefinition.bEnabled == OMX_FALSE) &&
            (pExynosPort->portState == EXYNOS_OMX_PortStateEnabling)) {
            pExynosPort->exceptionFlag = INVALID_STATE;
            Exynos_OSAL_SemaphorePost(pExynosPort->loadedResource);
        }

        pExynosPort = &(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX]);
        if ((pExynosPort->portDefinition.bEnabled == OMX_FALSE) &&
            (pExynosPort->portState == EXYNOS_OMX_PortStateEnabling)) {
            pExynosPort->exceptionFlag = INVALID_STATE;
            Exynos_OSAL_SemaphorePost(pExynosPort->loadedResource);
        }

        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] OMX_StateExecuting to OMX_StateIdle", pExynosComponent, __FUNCTION__);
    } else if ((destState == OMX_StateIdle) && (pExynosComponent->currentState == OMX_StatePause)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] OMX_StatePause to OMX_StateIdle", pExynosComponent, __FUNCTION__);
    } else if ((destState == OMX_StateExecuting) && (pExynosComponent->currentState == OMX_StateIdle)) {
        pExynosComponent->transientState = EXYNOS_OMX_TransStateIdleToExecuting;
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] OMX_StateIdle to OMX_StateExecuting", pExynosComponent, __FUNCTION__);
    } else if (destState == OMX_StateInvalid) {
        for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
            pExynosComponent->pExynosPort[i].portState = EXYNOS_OMX_PortStateInvalid;
        }
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE Exynos_OMX_CommandQueue(
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent,
    OMX_COMMANDTYPE        Cmd,
    OMX_U32                nParam,
    OMX_PTR                pCmdData)
{
    OMX_ERRORTYPE    ret = OMX_ErrorNone;
    EXYNOS_OMX_MESSAGE *command = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_MESSAGE));

    if (command == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    command->messageType  = (OMX_U32)Cmd;
    command->messageParam = nParam;
    command->pCmdData     = pCmdData;

    ret = Exynos_OSAL_Queue(&pExynosComponent->messageQ, (void *)command);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    ret = Exynos_OSAL_SemaphorePost(pExynosComponent->msgSemaphoreHandle);

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OMX_SendCommand(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_COMMANDTYPE Cmd,
    OMX_IN OMX_U32         nParam,
    OMX_IN OMX_PTR         pCmdData)
{
    OMX_ERRORTYPE             ret               = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent     = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent  = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (Cmd) {
    case OMX_CommandStateSet :
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] Command: OMX_CommandStateSet", pExynosComponent, __FUNCTION__);
        Exynos_StateSet(pExynosComponent, nParam);
        break;
    case OMX_CommandFlush :
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] Command: OMX_CommandFlush", pExynosComponent, __FUNCTION__);
        ret = Exynos_SetPortFlush(pExynosComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandPortDisable :
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] Command: OMX_CommandPortDisable", pExynosComponent, __FUNCTION__);
        ret = Exynos_SetPortDisable(pExynosComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandPortEnable :
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] Command: OMX_CommandPortEnable", pExynosComponent, __FUNCTION__);
        ret = Exynos_SetPortEnable(pExynosComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandMarkBuffer :
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] Command: OMX_CommandMarkBuffer", pExynosComponent, __FUNCTION__);
        ret = Exynos_SetMarkBuffer(pExynosComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    default:
        break;
    }

    ret = Exynos_OMX_CommandQueue(pExynosComponent, Cmd, nParam, pCmdData);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nParamIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }
        portParam->nPorts               = 0;
        portParam->nStartPortNumber     = 0;
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex      = portDefinition->nPortIndex;
        EXYNOS_OMX_BASEPORT          *pExynosPort    = NULL;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = Exynos_OMX_Check_SizeVersion(portDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        Exynos_OSAL_Memcpy(((char *)portDefinition) + nOffset,
                           ((char *)&pExynosPort->portDefinition) + nOffset,
                           portDefinition->nSize - nOffset);
    }
        break;
    case OMX_IndexParamPriorityMgmt:
    {
        OMX_PRIORITYMGMTTYPE *compPriority = (OMX_PRIORITYMGMTTYPE *)ComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(compPriority, sizeof(OMX_PRIORITYMGMTTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        compPriority->nGroupID       = pExynosComponent->compPriority.nGroupID;
        compPriority->nGroupPriority = pExynosComponent->compPriority.nGroupPriority;
    }
        break;

    case OMX_IndexParamCompBufferSupplier:
    {
        OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplier = (OMX_PARAM_BUFFERSUPPLIERTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = bufferSupplier->nPortIndex;
        EXYNOS_OMX_BASEPORT          *pExynosPort;

        if ((pExynosComponent->currentState == OMX_StateLoaded) ||
            (pExynosComponent->currentState == OMX_StateWaitForResources)) {
            if (portIndex >= pExynosComponent->portParam.nPorts) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            }
            ret = Exynos_OMX_Check_SizeVersion(bufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }

            pExynosPort = &pExynosComponent->pExynosPort[portIndex];


            if (pExynosPort->portDefinition.eDir == OMX_DirInput) {
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyInput;
                } else if (CHECK_PORT_TUNNELED(pExynosPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyOutput;
                } else {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyUnspecified;
                }
            } else {
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyOutput;
                } else if (CHECK_PORT_TUNNELED(pExynosPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyInput;
                } else {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyUnspecified;
                }
            }
        }
        else
        {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
    }
        break;
    default:
    {
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

    ret = OMX_ErrorNone;

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pExynosComponent->currentState != OMX_StateLoaded) &&
            (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        /* ret = OMX_ErrorUndefined; */
        /* Exynos_OSAL_Memcpy(&pExynosComponent->portParam, portParam, sizeof(OMX_PORT_PARAM_TYPE)); */
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = portDefinition->nPortIndex;
        EXYNOS_OMX_BASEPORT          *pExynosPort = NULL;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = Exynos_OMX_Check_SizeVersion(portDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];

        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            if (pExynosPort->portDefinition.bEnabled == OMX_TRUE) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
        }
        if (portDefinition->nBufferCountActual < pExynosPort->portDefinition.nBufferCountMin) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        Exynos_OSAL_Memcpy(((char *)&pExynosPort->portDefinition) + nOffset,
                           ((char *)portDefinition) + nOffset,
                           portDefinition->nSize - nOffset);
    }
        break;
    case OMX_IndexParamPriorityMgmt:
    {
        OMX_PRIORITYMGMTTYPE *compPriority = (OMX_PRIORITYMGMTTYPE *)ComponentParameterStructure;

        if ((pExynosComponent->currentState != OMX_StateLoaded) &&
            (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        ret = Exynos_OMX_Check_SizeVersion(compPriority, sizeof(OMX_PRIORITYMGMTTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pExynosComponent->compPriority.nGroupID = compPriority->nGroupID;
        pExynosComponent->compPriority.nGroupPriority = compPriority->nGroupPriority;
    }
        break;
    case OMX_IndexParamCompBufferSupplier:
    {
        OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplier = (OMX_PARAM_BUFFERSUPPLIERTYPE *)ComponentParameterStructure;
        OMX_U32           portIndex = bufferSupplier->nPortIndex;
        EXYNOS_OMX_BASEPORT *pExynosPort = NULL;


        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = Exynos_OMX_Check_SizeVersion(bufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            if (pExynosPort->portDefinition.bEnabled == OMX_TRUE) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
        }

        if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyUnspecified) {
            ret = OMX_ErrorNone;
            goto EXIT;
        }
        if (CHECK_PORT_TUNNELED(pExynosPort) == 0) {
            ret = OMX_ErrorNone; /*OMX_ErrorNone ?????*/
            goto EXIT;
        }

        if (pExynosPort->portDefinition.eDir == OMX_DirInput) {
            if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyInput) {
                /*
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    ret = OMX_ErrorNone;
                }
                */
                pExynosPort->tunnelFlags |= EXYNOS_TUNNEL_IS_SUPPLIER;
                bufferSupplier->nPortIndex = pExynosPort->tunneledPort;
                ret = OMX_SetParameter(pExynosPort->tunneledComponent, OMX_IndexParamCompBufferSupplier, bufferSupplier);
                goto EXIT;
            } else if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyOutput) {
                ret = OMX_ErrorNone;
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    pExynosPort->tunnelFlags &= ~EXYNOS_TUNNEL_IS_SUPPLIER;
                    bufferSupplier->nPortIndex = pExynosPort->tunneledPort;
                    ret = OMX_SetParameter(pExynosPort->tunneledComponent, OMX_IndexParamCompBufferSupplier, bufferSupplier);
                }
                goto EXIT;
            }
        } else if (pExynosPort->portDefinition.eDir == OMX_DirOutput) {
            if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyInput) {
                ret = OMX_ErrorNone;
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    pExynosPort->tunnelFlags &= ~EXYNOS_TUNNEL_IS_SUPPLIER;
                    ret = OMX_ErrorNone;
                }
                goto EXIT;
            } else if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyOutput) {
                /*
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    ret = OMX_ErrorNone;
                }
                */
                pExynosPort->tunnelFlags |= EXYNOS_TUNNEL_IS_SUPPLIER;
                ret = OMX_ErrorNone;
                goto EXIT;
            }
        }
    }
        break;
    default:
    {
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

    ret = OMX_ErrorNone;

EXIT:

    FunctionOut();

    return ret;
}

OMX_PTR Exynos_OMX_MakeDynamicConfigCMD(
    OMX_INDEXTYPE   nIndex,
    OMX_PTR         pComponentConfigStructure)
{
    OMX_PTR ret                  = NULL;
    OMX_S32 nConfigStructureSize = 0;

    switch ((int)nIndex) {
    case OMX_IndexConfigVideoIntraPeriod:
    {
        nConfigStructureSize = sizeof(OMX_U32);
        ret = Exynos_OSAL_Malloc(sizeof(OMX_U32) + nConfigStructureSize);
    }
        break;
    case OMX_IndexConfigVideoRoiInfo:
    {
        EXYNOS_OMX_VIDEO_CONFIG_ROIINFO *pRoiInfo = (EXYNOS_OMX_VIDEO_CONFIG_ROIINFO *)pComponentConfigStructure;
        OMX_S32 nRoiMBInfoSize = 0;
        nConfigStructureSize = *(OMX_U32 *)pComponentConfigStructure;
        if (pRoiInfo->bUseRoiInfo == OMX_TRUE)
            nRoiMBInfoSize = pRoiInfo->nRoiMBInfoSize;
        ret = Exynos_OSAL_Malloc(sizeof(OMX_U32) + nConfigStructureSize + nRoiMBInfoSize);
        if (ret != NULL)
            Exynos_OSAL_Memcpy((OMX_PTR)((OMX_U8 *)ret + sizeof(OMX_U32) + nConfigStructureSize), pRoiInfo->pRoiMBInfo, nRoiMBInfoSize);
    }
        break;
    default:
        nConfigStructureSize = *(OMX_U32 *)pComponentConfigStructure;
        ret = Exynos_OSAL_Malloc(sizeof(OMX_U32) + nConfigStructureSize);
        break;
    }

    if (ret != NULL) {
        *((OMX_S32 *)ret) = (OMX_S32)nIndex;
        Exynos_OSAL_Memcpy((OMX_PTR)((OMX_U8 *)ret + sizeof(OMX_U32)), pComponentConfigStructure, nConfigStructureSize);
    }

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_INOUT OMX_PTR     pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    ret = OMX_ErrorBadParameter;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_SetCallbacks (
    OMX_IN OMX_HANDLETYPE    hComponent,
    OMX_IN OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN OMX_PTR           pAppData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pCallbacks == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }
    if (pExynosComponent->currentState != OMX_StateLoaded) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pExynosComponent->pCallbacks = pCallbacks;
    pExynosComponent->callbackData = pAppData;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

#ifdef EGL_IMAGE_SUPPORT
OMX_ERRORTYPE Exynos_OMX_UseEGLImage(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN void                     *eglImage)
{
    return OMX_ErrorNotImplemented;
}
#endif

void Exynos_OMX_Component_abnormalTermination(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort      = NULL;

    int i = 0;

    FunctionIn();

    if (hComponent == NULL)
        goto EXIT;
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;

    if (pOMXComponent->pComponentPrivate == NULL)
        goto EXIT;
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    /* clear a msg command piled on queue */
    while(Exynos_OSAL_GetElemNum(&pExynosComponent->messageQ) > 0)
        Exynos_OSAL_Free(Exynos_OSAL_Dequeue(&pExynosComponent->messageQ));

    pExynosComponent->abendState = OMX_TRUE;

    /* post semaphore for msg handler thread, if it is waiting */
    for (i = 0; i < ALL_PORT_NUM; i++) {
        pExynosPort = &pExynosComponent->pExynosPort[i];

        if ((pExynosComponent->transientState == EXYNOS_OMX_TransStateLoadedToIdle) ||
            (pExynosPort->portState == EXYNOS_OMX_PortStateInvalid) ||
            (pExynosPort->portState == EXYNOS_OMX_PortStateEnabling)) { // enabling exception
            pExynosComponent->abendState = OMX_TRUE;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] post loadedResource about %s port",
                                                    pExynosComponent, __FUNCTION__,
                                                    (i == INPUT_PORT_INDEX)? "input":"output");
            Exynos_OSAL_SemaphorePost(pExynosPort->loadedResource);
        }

        if ((pExynosComponent->transientState == EXYNOS_OMX_TransStateIdleToLoaded) ||
            (pExynosPort->portState == EXYNOS_OMX_PortStateInvalid) ||
            (pExynosPort->portState == EXYNOS_OMX_PortStateFlushingForDisable) ||
            (pExynosPort->portState == EXYNOS_OMX_PortStateDisabling)) { // disabling exception
            pExynosComponent->abendState = OMX_TRUE;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] post unloadedResource about %s port",
                                                    pExynosComponent, __FUNCTION__,
                                                    (i == INPUT_PORT_INDEX)? "input":"output");
            Exynos_OSAL_SemaphorePost(pExynosPort->unloadedResource);
        }
    }

    /* change to invalid state except for Loaded state */
    if (pExynosComponent->transientState != EXYNOS_OMX_TransStateLoadedToIdle)
        Exynos_OMX_SendCommand(hComponent, OMX_CommandStateSet, OMX_StateInvalid, NULL);

    /* wait for state change or LoaedToIdle handling */
    Exynos_OSAL_SignalWait(pExynosComponent->abendStateEvent, 1000);
    Exynos_OSAL_SignalReset(pExynosComponent->abendStateEvent);

EXIT:
    FunctionOut();

    return;
}

OMX_ERRORTYPE Exynos_OMX_BaseComponent_Constructor(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%s] lib version is %s", __FUNCTION__, IS_64BIT_OS? "64bit":"32bit");

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%s] OMX_ErrorBadParameter (0x%x)", __FUNCTION__, ret);
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pExynosComponent = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_BASECOMPONENT));
    if (pExynosComponent == NULL) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%s] Failed to Malloc (0x%x)", __FUNCTION__, ret);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosComponent, 0, sizeof(EXYNOS_OMX_BASECOMPONENT));
    pOMXComponent->pComponentPrivate = (OMX_PTR)pExynosComponent;

    ret = Exynos_OSAL_SemaphoreCreate(&pExynosComponent->msgSemaphoreHandle);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s] Failed to SemaphoreCreate (0x%x)", pExynosComponent, __FUNCTION__, ret);
        goto EXIT;
    }
    ret = Exynos_OSAL_MutexCreate(&pExynosComponent->compMutex);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s] Failed to MutexCreate (0x%x)", pExynosComponent, __FUNCTION__, ret);
        goto EXIT;
    }
    ret = Exynos_OSAL_SignalCreate(&pExynosComponent->abendStateEvent);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s] Failed to SignalCreate (0x%x)", pExynosComponent, __FUNCTION__, ret);
        goto EXIT;
    }

    pExynosComponent->bExitMessageHandlerThread = OMX_FALSE;
    Exynos_OSAL_QueueCreate(&pExynosComponent->messageQ, MAX_QUEUE_ELEMENTS);
    ret = Exynos_OSAL_ThreadCreate(&pExynosComponent->hMessageHandler, Exynos_OMX_MessageHandlerThread, pOMXComponent);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s] Failed to ThreadCreate (0x%x)", pExynosComponent, __FUNCTION__, ret);
        goto EXIT;
    }

    pOMXComponent->GetComponentVersion = &Exynos_OMX_GetComponentVersion;
    pOMXComponent->SendCommand         = &Exynos_OMX_SendCommand;
    pOMXComponent->GetState            = &Exynos_OMX_GetState;
    pOMXComponent->SetCallbacks        = &Exynos_OMX_SetCallbacks;

#ifdef EGL_IMAGE_SUPPORT
    pOMXComponent->UseEGLImage         = &Exynos_OMX_UseEGLImage;
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BaseComponent_Destructor(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    OMX_S32                   semaValue = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    Exynos_OMX_CommandQueue(pExynosComponent, (OMX_COMMANDTYPE)EXYNOS_OMX_CommandComponentDeInit, 0, NULL);
    Exynos_OSAL_SleepMillisec(0);
    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->msgSemaphoreHandle, &semaValue);
    if (semaValue == 0)
        Exynos_OSAL_SemaphorePost(pExynosComponent->msgSemaphoreHandle);
    Exynos_OSAL_SemaphorePost(pExynosComponent->msgSemaphoreHandle);

    Exynos_OSAL_ThreadTerminate(pExynosComponent->hMessageHandler);
    pExynosComponent->hMessageHandler = NULL;

    Exynos_OSAL_SignalTerminate(pExynosComponent->abendStateEvent);
    pExynosComponent->abendStateEvent = NULL;
    Exynos_OSAL_MutexTerminate(pExynosComponent->compMutex);
    pExynosComponent->compMutex = NULL;
    Exynos_OSAL_SemaphoreTerminate(pExynosComponent->msgSemaphoreHandle);
    pExynosComponent->msgSemaphoreHandle = NULL;
    Exynos_OSAL_QueueTerminate(&pExynosComponent->messageQ);

    Exynos_OSAL_Free(pExynosComponent);
    pExynosComponent = NULL;

    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}


