#include "webserver.hpp"

#include <hardware/watchdog.h>
#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/tcp.h>
#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config_store.hpp"

extern "C" {
#include "dhserver.h"
}

// ── HTML pages ────────────────────────────────────────────────────────────────

static const char *HTML_FORM_TEMPLATE =
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
    "<!DOCTYPE html><html><head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Button Box Setup</title>"
    "<style>"
    "body{font-family:sans-serif;max-width:400px;margin:40px auto;padding:16px}"
    "h1{color:#333}label{display:block;margin-top:12px;font-weight:bold}"
    "input{width:100%;padding:8px;margin:4px 0;box-sizing:border-box;font-size:1em}"
    "button{margin-top:16px;padding:12px;width:100%;background:#4CAF50;"
    "color:#fff;border:none;font-size:1em;cursor:pointer}"
    "</style></head><body>"
    "<h1>Button Box Setup</h1>"
    "<form method='POST' action='/'>"
    "<label>WiFi SSID</label>"
    "<input type='text' name='ssid' maxlength='63' required value='%s'>"
    "<label>WiFi Password</label>"
    "<input type='password' name='psk' maxlength='63' value='%s'>"
    "<label>Device UUID</label>"
    "<input type='text' name='uuid' maxlength='63' value='%s'>"
    "<button type='submit'>Save &amp; Reboot</button>"
    "</form></body></html>";

static const char *HTML_SAVED =
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
    "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;padding:40px'>"
    "<h1>Saved!</h1><p>Device is rebooting&hellip;</p></body></html>";

static const char *HTML_BAD =
    "HTTP/1.0 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nSSID required\n";

// ── Config context shared with TCP callbacks ──────────────────────────────────

struct WebCtx {
  volatile bool saved;
  char ssid[64];
  char psk[64];
  char uuid[64];
};

static WebCtx web_ctx;

// ── URL decode ────────────────────────────────────────────────────────────────

static void urlDecode(char *str) {
  char *src = str, *dst = str;
  while (*src) {
    if (*src == '+') {
      *dst++ = ' '; src++;
    } else if (*src == '%' && src[1] && src[2]) {
      char hex[3] = {src[1], src[2], 0};
      *dst++ = (char)strtol(hex, nullptr, 16);
      src += 3;
    } else {
      *dst++ = *src++;
    }
  }
  *dst = 0;
}

// Parse "ssid=foo&psk=bar&uuid=baz" into the WebCtx
static void parseFormBody(const char *body) {
  char tmp[1024];
  strncpy(tmp, body, sizeof(tmp) - 1);
  tmp[sizeof(tmp) - 1] = 0;

  char *save;
  char *pair = strtok_r(tmp, "&", &save);
  while (pair) {
    char *eq = strchr(pair, '=');
    if (eq) {
      *eq = 0;
      char *v = eq + 1;
      urlDecode(v);
      if (!strcmp(pair, "ssid"))      { strncpy(web_ctx.ssid, v, 63); web_ctx.ssid[63] = 0; }
      else if (!strcmp(pair, "psk"))  { strncpy(web_ctx.psk,  v, 63); web_ctx.psk[63]  = 0; }
      else if (!strcmp(pair, "uuid")) { strncpy(web_ctx.uuid, v, 63); web_ctx.uuid[63] = 0; }
    }
    pair = strtok_r(nullptr, "&", &save);
  }
}

// ── TCP callbacks ─────────────────────────────────────────────────────────────

static err_t tcpRecv(void *, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
  if (!p) { tcp_close(pcb); return ERR_OK; }
  if (err != ERR_OK) { pbuf_free(p); return err; }

  uint16_t len = p->tot_len > 2047 ? 2047 : p->tot_len;
  char buf[2048];
  pbuf_copy_partial(p, buf, len, 0);
  buf[len] = 0;
  tcp_recved(pcb, p->tot_len);  // acknowledge received bytes to reopen receive window
  pbuf_free(p);

  char method[8] = "", path[64] = "";
  char *eol = strstr(buf, "\r\n");
  if (eol) {
    char line[128];
    size_t ll = (size_t)(eol - buf);
    if (ll >= sizeof(line)) ll = sizeof(line) - 1;
    memcpy(line, buf, ll); line[ll] = 0;
    sscanf(line, "%7s %63s", method, path);
  }

  char *body_start = strstr(buf, "\r\n\r\n");
  const char *body = body_start ? body_start + 4 : "";

  const char *response = nullptr;
  size_t response_len = 0;
  char html[2048];

  if (!strcmp(method, "POST") && !strcmp(path, "/")) {
    web_ctx.ssid[0] = web_ctx.psk[0] = web_ctx.uuid[0] = 0;
    parseFormBody(body);

    if (web_ctx.ssid[0]) {
      response = HTML_SAVED;
      response_len = strlen(HTML_SAVED);
      web_ctx.saved = true;
    } else {
      response = HTML_BAD;
      response_len = strlen(HTML_BAD);
    }
  } else {
    // GET — serve the form pre-filled with current values
    snprintf(html, sizeof(html), HTML_FORM_TEMPLATE,
             web_ctx.ssid, web_ctx.psk, web_ctx.uuid);
    response = html;
    response_len = strlen(html);
  }

  err_t wr = tcp_write(pcb, response, response_len, TCP_WRITE_FLAG_COPY);
  if (wr == ERR_OK) {
    tcp_output(pcb);
  }
  tcp_close(pcb);

  return ERR_OK;
}

static err_t tcpAccept(void *, struct tcp_pcb *newpcb, err_t err) {
  if (err != ERR_OK) return err;
  tcp_recv(newpcb, tcpRecv);
  return ERR_OK;
}

// ── Public entry point ────────────────────────────────────────────────────────

void webserverRun(const char *current_ssid, const char *current_psk,
                  const char *current_uuid) {
  printf("=== CONFIG MODE ===\n");

  // Pre-fill form with existing values
  strncpy(web_ctx.ssid, current_ssid, 63); web_ctx.ssid[63] = 0;
  strncpy(web_ctx.psk,  current_psk,  63); web_ctx.psk[63]  = 0;
  strncpy(web_ctx.uuid, current_uuid, 63); web_ctx.uuid[63] = 0;
  web_ctx.saved = false;

  cyw43_arch_init();

  // Set channel 6 before enabling AP
  cyw43_wifi_ap_set_channel(&cyw43_state, 6);
  cyw43_arch_enable_ap_mode("ButtonBox-Config", NULL, CYW43_AUTH_OPEN);

  // Reduce power-save aggressiveness (matches reference project)
  cyw43_wifi_pm(&cyw43_state, 0xa11140);

  // Configure AP interface address
  cyw43_arch_lwip_begin();

  struct netif *ap = &cyw43_state.netif[CYW43_ITF_AP];
  ip4_addr_t ip, mask, gw;
  IP4_ADDR(&gw,   192, 168, 4, 1);
  IP4_ADDR(&ip,   192, 168, 4, 1);
  IP4_ADDR(&mask, 255, 255, 255, 0);
  netif_set_addr(ap, &ip, &mask, &gw);

  // ── TinyUSB dhserver (proven working, used by pico-examples) ─────────────
  static dhcp_entry_t dhcp_entries[] = {
    { {0}, IPADDR4_INIT_BYTES(192, 168, 4, 2), 24 * 60 * 60 },
    { {0}, IPADDR4_INIT_BYTES(192, 168, 4, 3), 24 * 60 * 60 },
  };
  static const dhcp_config_t dhcp_config = {
    .router   = IPADDR4_INIT_BYTES(192, 168, 4, 1),
    .port     = 67,
    .dns      = IPADDR4_INIT_BYTES(192, 168, 4, 1),
    .domain   = "",
    .num_entry = 2,
    .entries  = dhcp_entries,
  };
  while (dhserv_init(&dhcp_config) != ERR_OK) {
    printf("dhserv_init failed, retrying...\n");
    sleep_ms(100);
  }
  printf("DHCP server started\n");

  // ── TCP HTTP server on port 80 ─────────────────────────────────────────────
  struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
  tcp_bind(pcb, IP_ADDR_ANY, 80);
  pcb = tcp_listen_with_backlog(pcb, 4);
  tcp_accept(pcb, tcpAccept);
  printf("HTTP server on http://192.168.4.1\n");

  cyw43_arch_lwip_end();

  // Main loop — poll until config is saved
  while (!web_ctx.saved) {
    cyw43_arch_poll();
    sleep_ms(10);
  }

  printf("Saving config and rebooting...\n");
  configSave(web_ctx.ssid, web_ctx.psk, web_ctx.uuid);

  sleep_ms(500);
  watchdog_enable(100, 1);
  while (1);
}
