/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef BOOT_CONTROL_H_
#define BOOT_CONTROL_H_

#define EXBC_DEBUG 0
#include <stdint.h>
#include <stdbool.h>

using android::fs_mgr::Fstab;
using android::fs_mgr::ReadDefaultFstab;

typedef struct ExynosSlotInfo {
  uint8_t magic[4];

  uint8_t bootable;
  uint8_t is_active;
  uint8_t boot_successful;
  uint8_t tries_remaining;

  uint8_t reserved[8];
} ExynosSlotInfo;


class BootControl {
 private:
  ExynosSlotInfo m_slot_info[2];
  int m_slotinfo_fd;
  const int m_slot_size = 2*sizeof(ExynosSlotInfo);

  // If magic code is currupted, SlotInfo will be reset like below.
  // +======+=======+==========+===========+=================+=================+
  // | Slot | Magic | bootable | is_active | boot_successful | tries_ramaining |
  // +======+=======+==========+===========+=================+=================+
  // | A    | EXBC  | True     | True      | False           | 7               |
  // +------+-------+----------+-----------+-----------------+-----------------+
  // | B    | EXBC  | True     | False     | False           | 7               |
  // +======+=======+==========+===========+=================+=================+
  void reset();
  void reset_if_invalid();

  // It reads ExynosSlotInfo from slotinfo partiton,
  // and copy it to m_slot_info.
  bool init();
  bool slotinfo_open();
  void print_slot_info(const char* msg);

 public:
  BootControl();
  ~BootControl();
  // It saves m_slot_info to slotinfo_partition
  bool save();

  // Mark this slot as valid
  // +==========+=================+=================+
  // | Slot     | boot_successful | tries_ramaining |
  // +==========+=================+=================+
  // | Required | True            | 0               |
  // +==========+=================+=================+
  void mark_boot_successful(int slot);

  // +===========+==========+===========+=================+=================+
  // | Slot      | bootable | is_active | boot_successful | tries_ramaining |
  // +===========+==========+===========+=================+=================+
  // | Required  | True     | True      | False           | 7               |
  // +-----------+----------+-----------+-----------------+-----------------+
  // | The Other |          | False     |                 |                 |
  // +===========+==========+===========+=================+=================+
  void set_active_boot_slot(int slot);

  // set requested slot as unbootable.
  // +==========+==========+===========+=================+=================+
  // | Slot     | bootable | is_active | boot_successful | tries_ramaining |
  // +==========+==========+===========+=================+=================+
  // | Required | False    | False     | False           | 0               |
  // +==========+==========+===========+=================+=================+
  // If the other slot is bootable, it will be setted active.
  void set_slot_as_unbootable(int slot);

  // Getter of slot infos
  int is_slot_bootable(int slot);
  int is_slot_boot_successful(int slot);
};

#if (__STDC_VERSION__ >= 201112L) || defined(__cplusplus)
static_assert(sizeof(ExynosSlotInfo) == 16,
    "struct bootloader_message size changes, which may break A/B devices");
#endif

#endif  // BOOT_CONTROL_H

