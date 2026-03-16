#include "api_client.hpp"

#include <lwip/tcp.h>
#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ── Configuration ─────────────────────────────────────────────────────────────

static char s_host[64] = "";
static uint16_t s_port = 3000;

void ApiClient::init(const char *host, uint16_t port) {
  strncpy(s_host, host, 63);
  s_host[63] = '\0';
  s_port = port;
}

// ── Minimal blocking HTTP GET over lwIP raw TCP ───────────────────────────────

enum ConnState { CONN_PENDING, CONN_DONE, CONN_ERROR };

struct HttpConn {
  volatile ConnState state;
  struct tcp_pcb *pcb; // set to nullptr once the pcb is closed by a callback
  char request[512];
  uint16_t req_len;
};

static HttpConn s_conn;

static err_t cb_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
  HttpConn *c = (HttpConn *)arg;
  if (!p) {
    // Server closed the connection — request delivered
    c->state = CONN_DONE;
    c->pcb = nullptr;
    tcp_close(pcb);
  } else {
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
  }
  return ERR_OK;
}

static void cb_err(void *arg, err_t err) {
  HttpConn *c = (HttpConn *)arg;
  c->state = CONN_ERROR;
  c->pcb = nullptr; // lwIP already freed the pcb on error
}

static err_t cb_connected(void *arg, struct tcp_pcb *pcb, err_t err) {
  HttpConn *c = (HttpConn *)arg;
  if (err != ERR_OK) {
    c->state = CONN_ERROR;
    c->pcb = nullptr;
    return err;
  }
  err_t r = tcp_write(pcb, c->request, c->req_len, TCP_WRITE_FLAG_COPY);
  if (r != ERR_OK) {
    c->state = CONN_ERROR;
    c->pcb = nullptr;
    tcp_close(pcb);
    return r;
  }
  tcp_output(pcb);
  return ERR_OK;
}

// ── Public API ────────────────────────────────────────────────────────────────

bool ApiClient::send(const char *category, const char *const *ctr_names,
                     const uint8_t *ctr_values, int n) {
  if (s_host[0] == '\0') {
    printf("[ApiClient] No host configured\n");
    return false;
  }

  // Build query: /log?category=<cat>&<name>=<val>&...
  char query[256];
  int qlen = snprintf(query, sizeof(query), "/log?category=%s", category);
  for (int i = 0; i < n && qlen < (int)sizeof(query) - 1; i++) {
    qlen += snprintf(query + qlen, sizeof(query) - qlen,
                     "&%s=%d", ctr_names[i], ctr_values[i]);
  }

  // Build HTTP GET request
  s_conn.req_len = (uint16_t)snprintf(
      s_conn.request, sizeof(s_conn.request),
      "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
      query, s_host);
  s_conn.state = CONN_PENDING;

  // Parse IP address (host must be a dotted-decimal IPv4 string)
  ip_addr_t addr;
  if (!ipaddr_aton(s_host, &addr)) {
    printf("[ApiClient] Invalid IP: %s\n", s_host);
    return false;
  }

  // Open TCP connection
  cyw43_arch_lwip_begin();
  struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
  if (!pcb) {
    cyw43_arch_lwip_end();
    printf("[ApiClient] tcp_new failed\n");
    return false;
  }
  s_conn.pcb = pcb;
  tcp_arg(pcb, &s_conn);
  tcp_recv(pcb, cb_recv);
  tcp_err(pcb, cb_err);
  err_t err = tcp_connect(pcb, &addr, s_port, cb_connected);
  cyw43_arch_lwip_end();

  if (err != ERR_OK) {
    printf("[ApiClient] tcp_connect error %d\n", (int)err);
    return false;
  }

  // Block (with polling) until done or 5-second timeout
  uint32_t t0 = to_ms_since_boot(get_absolute_time());
  while (s_conn.state == CONN_PENDING) {
    cyw43_arch_poll();
    sleep_ms(1);
    if (to_ms_since_boot(get_absolute_time()) - t0 > 5000) {
      printf("[ApiClient] timeout\n");
      if (s_conn.pcb) {
        cyw43_arch_lwip_begin();
        tcp_abort(s_conn.pcb);
        cyw43_arch_lwip_end();
        s_conn.pcb = nullptr;
      }
      return false;
    }
  }

  printf("[ApiClient] %s  GET http://%s:%d%s\n",
         s_conn.state == CONN_DONE ? "OK " : "ERR",
         s_host, s_port, query);
  return s_conn.state == CONN_DONE;
}
