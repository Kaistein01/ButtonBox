#pragma once

#include <stddef.h>

/**
 * Device configuration storage in flash.
 * Stores WiFi SSID, PSK, and UUID in the last flash sector.
 */

/**
 * Load device config from flash.
 * @param ssid      Output buffer (min 64 bytes)
 * @param psk       Output buffer (min 64 bytes)
 * @param api_host  Output buffer (min 64 bytes) — API server IP address
 * @param api_port  Output buffer (min 8 bytes)  — API server port as string
 * @return true if config was found and valid, false if uninitialized (use defaults)
 */
bool configLoad(char *ssid, char *psk, char *api_host, char *api_port);

/**
 * Save device config to flash.
 * Erases the config sector and writes the new config.
 * @param ssid      WiFi SSID (null-terminated, max 63 chars)
 * @param psk       WiFi password (null-terminated, max 63 chars)
 * @param api_host  API server IP address (null-terminated, max 63 chars)
 * @param api_port  API server port as string (null-terminated, max 7 chars)
 */
void configSave(const char *ssid, const char *psk, const char *api_host,
                const char *api_port);
