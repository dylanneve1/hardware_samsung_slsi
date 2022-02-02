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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <fstab/fstab.h>
#include <hardware/hardware.h>
#include <hardware/boot_control.h>
#include <bootloader.h>
#include <android-base/properties.h>
#include <android-base/logging.h>

#include "boot_control.h"

void BootControl::reset() {
  LOG(ERROR) << "Warning: Slot info is reset";
  memcpy(m_slot_info[0].magic, "EXBC", 4);
  m_slot_info[0].bootable = true;
  m_slot_info[0].is_active = true;
  m_slot_info[0].boot_successful = false;
  m_slot_info[0].tries_remaining = 7;

  memcpy(m_slot_info[1].magic, "EXBC", 4);
  m_slot_info[1].bootable = true;
  m_slot_info[1].is_active = false;
  m_slot_info[1].boot_successful = false;
  m_slot_info[1].tries_remaining = 7;
}

void BootControl::print_slot_info(const char* msg) {
  LOG(INFO) << msg;
  LOG(INFO) << "Slot[A].bootable = " << (int) m_slot_info[0].bootable;
  LOG(INFO) << "Slot[A].is_active = " << (int) m_slot_info[0].is_active;
  LOG(INFO) << "Slot[A].boot_successful = " << (int) m_slot_info[0].boot_successful;
  LOG(INFO) << "Slot[A].tries_remaining = " << (int) m_slot_info[0].tries_remaining;

  LOG(INFO) << "Slot[B].bootable = " << (int) m_slot_info[1].bootable;
  LOG(INFO) << "Slot[B].is_active = " << (int) m_slot_info[1].is_active;
  LOG(INFO) << "Slot[B].boot_successful = " << (int) m_slot_info[1].boot_successful;
  LOG(INFO) << "Slot[B].tries_remaining = " << (int) m_slot_info[1].tries_remaining;
}

void BootControl::reset_if_invalid() {
  bool a_is_valid = memcmp(m_slot_info[0].magic, "EXBC", 4) == 0;
  bool b_is_valid = memcmp(m_slot_info[1].magic, "EXBC", 4) == 0;

  // just return if magics in both of slots are valid.
  if (!a_is_valid || !b_is_valid) {
    LOG(ERROR) << "Warning: Slot info is invalid";
    reset();
  }
}

#define BOOTINFO_OFFSET offsetof(struct bootloader_message_ab, slot_suffix)

BootControl::BootControl() {
  if(!init())
    reset();
  reset_if_invalid();
}

BootControl::~BootControl() {
  close(m_slotinfo_fd);
}

bool BootControl::slotinfo_open() {
  // Initialize fstab
  Fstab fstab;
  if (!ReadDefaultFstab(&fstab)) {
    LOG(ERROR) << "ERROR: Cannot open fatab";
    return false;
  }

  for (const auto& entry : fstab) {
    if (entry.mount_point == "/slotinfo") {
      if (entry.blk_device.empty()) {
        LOG(ERROR) << "ERROR: The path of slotinfo device is empty";
        return false;
      }
      m_slotinfo_fd = open(entry.blk_device.c_str(), O_RDWR);
      if (m_slotinfo_fd == -1) {
        LOG(ERROR) << "ERROR: cannot open slotinfo fd";
        return false;
      } else {
        return true;
      }
    }
  }
  LOG(ERROR) << "ERROR: Cannot get slotinfo from fatab";
  return false;
}

bool BootControl::init() {
  if (!slotinfo_open()) {
    LOG(ERROR) << "ERROR: fail within BootControl::init";
    return false;
  }

  int seeked_byte = lseek(m_slotinfo_fd, BOOTINFO_OFFSET, SEEK_SET);
  if (seeked_byte != BOOTINFO_OFFSET) {
    LOG(ERROR) << "ERROR: cannot seek slotinfo file: " << seeked_byte;
    return false;
  }

  ssize_t num_read;
  do {
    num_read = read(m_slotinfo_fd, (void*) m_slot_info, m_slot_size);
  } while (num_read == -1 && errno == EINTR);

  if (num_read != m_slot_size) {
    LOG(ERROR) << "ERROR: read bytes length is now matched with slot size: " << num_read << " != " << m_slot_size;
    return false;
  }
#if EXBC_DEBUG
  print_slot_info("Read in BootControl::init()");
#endif

  return true;
}

bool BootControl::save() {
  int seeked_byte = lseek(m_slotinfo_fd, BOOTINFO_OFFSET, SEEK_SET);
  if (seeked_byte != BOOTINFO_OFFSET) {
    LOG(ERROR) << "ERROR: cannot seek slotinfo file: " << seeked_byte;
    return false;
  }

  ssize_t num_written;
  do {
    num_written = write(m_slotinfo_fd, (void*) m_slot_info, m_slot_size);
  } while (num_written == -1 && errno == EINTR);

  if (num_written != m_slot_size) {
    PLOG(ERROR) << "ERROR: written bytes length is now matched with slot size: " << num_written << " != " << m_slot_size;
    return false;
  }
#if EXBC_DEBUG
  print_slot_info("Write in BootControl::save()");
#endif

  return true;
}

void BootControl::mark_boot_successful(int slot) {
  m_slot_info[slot].boot_successful = true;
  /* m_slot_info[slot].tries_remaining = 0; */
}

void BootControl::set_active_boot_slot(int slot) {
  m_slot_info[slot].bootable = true;
  m_slot_info[slot].is_active = true;
  m_slot_info[slot].boot_successful = false;
  m_slot_info[slot].tries_remaining = 7;

  m_slot_info[1-slot].is_active = false;
}

void BootControl::set_slot_as_unbootable(int slot) {
  m_slot_info[slot].bootable = false;
  m_slot_info[slot].is_active = false;
  m_slot_info[slot].boot_successful = false;
  m_slot_info[slot].tries_remaining = 0;

  if (m_slot_info[1-slot].bootable)
    m_slot_info[1-slot].is_active = true;
}

int BootControl::is_slot_bootable(int slot) {
  return m_slot_info[slot].bootable;
}

int BootControl::is_slot_boot_successful(int slot) {
  return m_slot_info[slot].boot_successful;
}

void boot_control_init(struct boot_control_module *module)
{
  android::base::InitLogging(nullptr, &android::base::KernelLogger);
  android::base::SetDefaultTag(std::string("ExynosBootControl"));
  return;
}

unsigned get_number_slots(struct boot_control_module *module)
{
  return 2;
}

unsigned get_current_slot(struct boot_control_module *module)
{
  const char* suffix[2] = {"_a", "_b"};
  std::string suffix_prop = android::base::GetProperty("ro.boot.slot_suffix", "");
  for (int i = 0; i < 2; i++) {
    if (strncmp(suffix_prop.c_str(), suffix[i], 2) == 0)
      return i;
  }
  LOG(ERROR) << "WARNING: androidboot.slot_suffix is invalid";
  return 0;
}

int mark_boot_successful(struct boot_control_module *module)
{
  BootControl boot_control = BootControl();
  int current_slot = get_current_slot(module);
  boot_control.mark_boot_successful(current_slot);

  if (!boot_control.save()) {
    LOG(ERROR) << "WARNING: Error saving boot-info.";
    return -errno;
  }
  return 0;
}

int set_active_boot_slot(struct boot_control_module *module, unsigned slot)
{
  if (slot >= 2) {
    LOG(ERROR) << "Error: Slot number sholud be 0 or 1";
    return -1;
  }
  BootControl boot_control = BootControl();
  boot_control.set_active_boot_slot(slot);
  if (!boot_control.save()) {
    LOG(ERROR) << "WARNING: Error saving boot-info.";
    return -errno;
  }
  return 0;
}

int set_slot_as_unbootable(struct boot_control_module *module, unsigned slot)
{
  if (slot >= 2) {
    LOG(ERROR) << "Error: Slot number sholud be 0 or 1";
    return -1;
  }
  BootControl boot_control = BootControl();
  boot_control.set_slot_as_unbootable(slot);
  if (!boot_control.save()) {
    LOG(ERROR) << "WARNING: Error saving boot-info.";
    return -errno;
  }
  return 0;
}

int is_slot_bootable(struct boot_control_module *module, unsigned slot)
{
  if (slot >= 2) {
    LOG(ERROR) << "Error: Slot number sholud be 0 or 1";
    return -1;
  }
  BootControl boot_control = BootControl();
  return boot_control.is_slot_bootable(slot);
}

int is_slot_boot_successful(struct boot_control_module *module, unsigned slot)
{
  if (slot >= 2) {
    LOG(ERROR) << "Error: Slot number sholud be 0 or 1";
    return -1;
  }
  BootControl boot_control = BootControl();
  return boot_control.is_slot_boot_successful(slot);
}

const char* get_suffix(struct boot_control_module *module, unsigned slot)
{
  static const char* suffix[2] = {"_a", "_b"};

  if (slot >= 2) {
    LOG(ERROR) << "Error: Slot number sholud be 0 or 1";
    return NULL;
  }

  return suffix[slot];
}

static int bootcontrol_open(const hw_module_t* module __unused, const char* id __unused,
    hw_device_t** device __unused) {
  /* Nothing to do currently. */
  return 0;
}

static struct hw_module_methods_t boot_control_module_methods = {
  .open = bootcontrol_open,
};

struct boot_control_module HAL_MODULE_INFO_SYM = {
  .common = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = BOOT_CONTROL_MODULE_API_VERSION_0_1,
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = BOOT_CONTROL_HARDWARE_MODULE_ID,
    .name = "Exynos Boot Control HAL",
    .author = "Exynos",
    .methods = &boot_control_module_methods,
  },

  .init = boot_control_init,
  .getNumberSlots = get_number_slots,
  .getCurrentSlot = get_current_slot,
  .markBootSuccessful = mark_boot_successful,
  .setActiveBootSlot = set_active_boot_slot,
  .setSlotAsUnbootable = set_slot_as_unbootable,
  .isSlotBootable = is_slot_bootable,
  .getSuffix = get_suffix,
  .isSlotMarkedSuccessful = is_slot_boot_successful,
};
