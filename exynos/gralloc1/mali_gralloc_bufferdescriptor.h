/*
 * Copyright (C) 2016 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef MALI_GRALLOC_BUFFERDESCRIPTOR_H_
#define MALI_GRALLOC_BUFFERDESCRIPTOR_H_

#include "gralloc_helper.h"

typedef struct buffer_descriptor
{
	uint32_t width;
	uint32_t height;
	uint32_t layerCount;
	int32_t hal_format;
	uint64_t producer_usage;
	uint64_t consumer_usage;
} buffer_descriptor_t;

int mali_gralloc_create_descriptor_internal(gralloc1_buffer_descriptor_t* outDescriptor);
int mali_gralloc_destroy_descriptor_internal(gralloc1_buffer_descriptor_t descriptor);
int mali_gralloc_set_dimensions_internal(gralloc1_buffer_descriptor_t descriptor, uint32_t width, uint32_t height);
int mali_gralloc_set_format_internal(gralloc1_buffer_descriptor_t descriptor, int32_t format);
int mali_gralloc_set_producerusage_internal(gralloc1_buffer_descriptor_t descriptor, uint64_t usage);
int mali_gralloc_set_consumerusage_internal(gralloc1_buffer_descriptor_t descriptor, uint64_t usage);

int mali_gralloc_get_backing_store_internal(buffer_handle_t buffer, gralloc1_backing_store_t* outStore);
int mali_gralloc_get_consumer_usage_internal(buffer_handle_t buffer, uint64_t* outUsage);
int mali_gralloc_get_dimensions_internal(buffer_handle_t buffer, uint32_t* outWidth, uint32_t* outHeight);
int mali_gralloc_get_format_internal(buffer_handle_t buffer, int32_t* outFormat);
int mali_gralloc_get_producer_usage_internal(buffer_handle_t buffer, uint64_t* outUsage);
int mali_gralloc_set_layer_count_internal(gralloc1_buffer_descriptor_t descriptor, uint32_t layerCount);
int mali_gralloc_get_layer_count_internal(buffer_handle_t buffer, uint32_t *outLayerCount);

int mali_gralloc_query_getstride(buffer_handle_t handle, int *pixelStride);

#endif /* MALI_GRALLOC_BUFFERDESCRIPTOR_H_ */
