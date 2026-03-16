#pragma once

/**
 * Configuration webserver for AP mode.
 *
 * Starts the device in Access Point mode ("ButtonBox-Config") and serves
 * an HTTP form for configuring WiFi credentials, UUID, and API server.
 *
 * This function blocks until configuration is saved and the device reboots.
 * Does not return.
 */
void webserverRun(const char *current_ssid, const char *current_psk,
                  const char *current_api_host, const char *current_api_port);
