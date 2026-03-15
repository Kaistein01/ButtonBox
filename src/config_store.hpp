#pragma once

#include <stddef.h>

/**
 * Device configuration storage in flash.
 * Stores WiFi SSID, PSK, and UUID in the last flash sector.
 */

/**
 * Load device config from flash.
 * @param ssid    Output buffer (min 64 bytes)
 * @param psk     Output buffer (min 64 bytes)
 * @param uuid    Output buffer (min 64 bytes)
 * @return true if config was found and valid, false if uninitialized (use defaults)
 */
bool configLoad(char *ssid, char *psk, char *uuid);

/**
 * Save device config to flash.
 * Erases the config sector and writes the new config.
 * @param ssid  WiFi SSID (null-terminated, max 63 chars)
 * @param psk   WiFi password (null-terminated, max 63 chars)
 * @param uuid  Device UUID (null-terminated, max 63 chars)
 */
void configSave(const char *ssid, const char *psk, const char *uuid);
