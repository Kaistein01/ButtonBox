#include "config_store.hpp"

#include <hardware/flash.h>
#include <pico/critical_section.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

// ── Flash storage layout ──────────────────────────────────────────────────────

// Device config stored in the last sector of flash
// Magic number to validate the config
#define CONFIG_MAGIC 0xCAFEBABE

// Critical section for flash operations
static critical_section_t flash_cs;

struct DeviceConfig {
  uint32_t magic;
  char ssid[64];
  char psk[64];
  char uuid[64];
};

// Compile-time check: DeviceConfig must fit in one flash sector (4096 bytes)
static_assert(sizeof(DeviceConfig) <= FLASH_SECTOR_SIZE,
              "DeviceConfig must fit in one flash sector");

// Flash offset: last sector of the flash (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
// This is a logical offset within the XIP region
static constexpr uint32_t CONFIG_OFFSET =
    PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;

// XIP base address (where flash is memory-mapped)
static constexpr uint32_t CONFIG_ADDRESS = XIP_BASE + CONFIG_OFFSET;

// ── Public API ────────────────────────────────────────────────────────────────

bool configLoad(char *ssid, char *psk, char *uuid) {
  const DeviceConfig *cfg = (const DeviceConfig *)CONFIG_ADDRESS;

  // Check magic number
  if (cfg->magic != CONFIG_MAGIC) {
    return false;
  }

  // Copy fields
  strncpy(ssid, cfg->ssid, 63);
  ssid[63] = '\0';
  strncpy(psk, cfg->psk, 63);
  psk[63] = '\0';
  strncpy(uuid, cfg->uuid, 63);
  uuid[63] = '\0';

  return true;
}

void configSave(const char *ssid, const char *psk, const char *uuid) {
  DeviceConfig cfg;
  cfg.magic = CONFIG_MAGIC;
  strncpy(cfg.ssid, ssid, 63);
  cfg.ssid[63] = '\0';
  strncpy(cfg.psk, psk, 63);
  cfg.psk[63] = '\0';
  strncpy(cfg.uuid, uuid, 63);
  cfg.uuid[63] = '\0';

  // Flash write must not be interrupted
  critical_section_init(&flash_cs);
  critical_section_enter_blocking(&flash_cs);

  // Erase the config sector
  flash_range_erase(CONFIG_OFFSET, FLASH_SECTOR_SIZE);

  // Write the config struct (padded to sector boundary)
  // flash_range_program expects data aligned to 256 bytes and padded
  uint8_t buffer[FLASH_SECTOR_SIZE];
  memset(buffer, 0xFF, FLASH_SECTOR_SIZE);
  memcpy(buffer, &cfg, sizeof(DeviceConfig));

  flash_range_program(CONFIG_OFFSET, buffer, FLASH_SECTOR_SIZE);

  critical_section_exit(&flash_cs);

  printf("Config saved to flash\n");
}
