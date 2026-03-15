#include "tft_display.hpp"
#include "hardware/spi.h"
#include "hw_config.h"
#include "pico/stdlib.h"
#include <stdio.h>

// SPI note: Hardware SPI0 used — PIO not needed (pins are PL022-capable).

// ── ILI9341 commands ─────────────────────────────────────────────────────────
#define ILI_SWRESET 0x01
#define ILI_SLPOUT 0x11
#define ILI_COLMOD 0x3A
#define ILI_MADCTL 0x36
#define ILI_CASET 0x2A
#define ILI_PASET 0x2B
#define ILI_RAMWR 0x2C
#define ILI_DISPON 0x29
#define ILI_INVOFF 0x20

// ── Colours (RGB565) — dark purple theme
// ──────────────────────────────────────────────────────────
#define COL_BG 0x0000u        // black (dark mode background)
#define COL_HEADER_BG 0x2004u // very dark purple
#define COL_HDR_FG 0xFFFFu    // white
#define COL_DIV 0x3806u       // dark purple divider
#define COL_CAT_IDLE 0x1002u  // very dark purple (inactive cell)
#define COL_CAT_ON 0x07E0u    // bright green (active + blink ON)
#define COL_CAT_DIM 0x0320u   // dark green  (active + blink OFF)
#define COL_CAT_TXT_I 0xFFFFu // white text on idle cell
#define COL_CAT_TXT_A 0x0000u // black text on active cell
#define COL_CTR_BG 0x0801u    // very dark purple counter panel
#define COL_CTR_NAME 0x7BEFu  // grey counter label
#define COL_CTR_VAL_Z 0x3806u // purple-grey (value = 0)
#define COL_CTR_VAL 0xFFFFu   // white (value > 0)
#define COL_CTR_DIV 0x2004u   // dark purple panel divider

// ── Layout (portrait 240×320)
// ───────────────────────────────────────────────── Header: y=0..31 (32px)
// Body:   y=32..319 (288px)
//   Left (categories):  x=0..171  (172px),  4 cols × 40px + 3×4px gap = 172
//   Divider:            x=172..173 (2px)
//   Right (counters):   x=174..239 (66px),  3 panels × 96px = 288
static constexpr uint16_t HEAD_H = 32;
static constexpr uint16_t BODY_Y = 32;
static constexpr uint16_t BODY_H = 288; // 320 - 32
static constexpr uint16_t CATCELL_W = 40;
static constexpr uint16_t CATCELL_H = 93; // 3*93 + 2*4 = 287 ≈ 288
static constexpr uint16_t CATCELL_G = 4;  // gap between cells
static constexpr uint16_t DIV_X = 172;
static constexpr uint16_t CTR_X = 174;
static constexpr uint16_t CTR_W = 66;
static constexpr uint16_t CTR_H = 96; // 3*96 = 288 ✓

// ── 5×7 bitmap font (ASCII 0x20–0x7E)
// ───────────────────────────────────────── Column bytes, bit0 = top pixel.
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '\''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // ')'
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // '}'
    {0x10, 0x08, 0x08, 0x10, 0x08}, // '~'
};

// ── SPI
// ───────────────────────────────────────────────────────────────────────
void TftDisplay::csLow() { gpio_put(PIN_TFT_CS, 0); }
void TftDisplay::csHigh() { gpio_put(PIN_TFT_CS, 1); }

void TftDisplay::writeCmd(uint8_t cmd) {
  gpio_put(PIN_TFT_DC, 0);
  csLow();
  spi_write_blocking(SPI_PORT, &cmd, 1);
  csHigh();
}
void TftDisplay::writeByte(uint8_t d) {
  gpio_put(PIN_TFT_DC, 1);
  csLow();
  spi_write_blocking(SPI_PORT, &d, 1);
  csHigh();
}
void TftDisplay::writeWord(uint16_t d) {
  uint8_t b[2] = {(uint8_t)(d >> 8), (uint8_t)(d & 0xFF)};
  gpio_put(PIN_TFT_DC, 1);
  csLow();
  spi_write_blocking(SPI_PORT, b, 2);
  csHigh();
}

// ── Drawing
// ───────────────────────────────────────────────────────────────────
void TftDisplay::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  writeCmd(ILI_CASET);
  writeByte(x0 >> 8);
  writeByte(x0 & 0xFF);
  writeByte(x1 >> 8);
  writeByte(x1 & 0xFF);
  writeCmd(ILI_PASET);
  writeByte(y0 >> 8);
  writeByte(y0 & 0xFF);
  writeByte(y1 >> 8);
  writeByte(y1 & 0xFF);
  writeCmd(ILI_RAMWR);
  gpio_put(PIN_TFT_DC, 1);
}
void TftDisplay::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                          uint16_t col) {
  if (!w || !h)
    return;
  setWindow(x, y, x + w - 1, y + h - 1);
  uint8_t b[2] = {(uint8_t)(col >> 8), (uint8_t)(col & 0xFF)};
  csLow();
  for (uint32_t i = 0; i < (uint32_t)w * h; i++)
    spi_write_blocking(SPI_PORT, b, 2);
  csHigh();
}
void TftDisplay::fillScreen(uint16_t col) {
  fillRect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, col);
}

// ── Text
// ──────────────────────────────────────────────────────────────────────
void TftDisplay::drawChar(int16_t x, int16_t y, char c, uint16_t fg,
                          uint16_t bg, uint8_t s) {
  if (c < 0x20 || c > 0x7E)
    c = '?';
  const uint8_t *g = font5x7[c - 0x20];
  for (int col = 0; col < 5; col++) {
    uint8_t cd = g[col];
    for (int row = 0; row < 7; row++)
      fillRect(x + col * s, y + row * s, s, s, (cd & (1u << row)) ? fg : bg);
  }
  fillRect(x + 5 * s, y, s, 7 * s, bg); // inter-character gap
}
void TftDisplay::drawString(int16_t x, int16_t y, const char *str, uint16_t fg,
                            uint16_t bg, uint8_t s) {
  while (*str) {
    drawChar(x, y, *str++, fg, bg, s);
    x += 6 * s;
  }
}
int TftDisplay::strPixelW(const char *str, uint8_t s) {
  int n = 0;
  while (*str++)
    n++;
  return n * 6 * s;
}

// ── ILI9341 init ─────────────────────────────────────────────────────────────
void TftDisplay::init() {
  gpio_init(PIN_TFT_CS);
  gpio_set_dir(PIN_TFT_CS, GPIO_OUT);
  gpio_put(PIN_TFT_CS, 1);
  gpio_init(PIN_TFT_DC);
  gpio_set_dir(PIN_TFT_DC, GPIO_OUT);
  gpio_put(PIN_TFT_DC, 1);
  gpio_init(PIN_TFT_RST);
  gpio_set_dir(PIN_TFT_RST, GPIO_OUT);
  gpio_put(PIN_TFT_RST, 1);

  spi_init(SPI_PORT, 40 * 1000 * 1000);
  gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SPI_TX, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SPI_RX, GPIO_FUNC_SPI);

  gpio_put(PIN_TFT_RST, 0);
  sleep_ms(15);
  gpio_put(PIN_TFT_RST, 1);
  sleep_ms(120);

  writeCmd(0x01);
  sleep_ms(150); // SW reset
  writeCmd(0x11);
  sleep_ms(120); // Sleep out
  writeCmd(0xCF);
  writeByte(0x00);
  writeByte(0xC1);
  writeByte(0x30);
  writeCmd(0xED);
  writeByte(0x64);
  writeByte(0x03);
  writeByte(0x12);
  writeByte(0x81);
  writeCmd(0xE8);
  writeByte(0x85);
  writeByte(0x00);
  writeByte(0x78);
  writeCmd(0xCB);
  writeByte(0x39);
  writeByte(0x2C);
  writeByte(0x00);
  writeByte(0x34);
  writeByte(0x02);
  writeCmd(0xF7);
  writeByte(0x20);
  writeCmd(0xEA);
  writeByte(0x00);
  writeByte(0x00);
  writeCmd(0xC0);
  writeByte(0x23);
  writeCmd(0xC1);
  writeByte(0x10);
  writeCmd(0xC5);
  writeByte(0x3E);
  writeByte(0x28);
  writeCmd(0xC7);
  writeByte(0x86);
  writeCmd(ILI_MADCTL);
  writeByte(0x48); // portrait, BGR (bit3=1 swaps R↔B for panels wired BGR)
  writeCmd(ILI_COLMOD);
  writeByte(0x55); // RGB565
  writeCmd(0xB1);
  writeByte(0x00);
  writeByte(0x18);
  writeCmd(0xB6);
  writeByte(0x08);
  writeByte(0x82);
  writeByte(0x27);
  writeCmd(0xF2);
  writeByte(0x00);
  writeCmd(0x26);
  writeByte(0x01);
  writeCmd(0xE0);
  writeByte(0x0F);
  writeByte(0x31);
  writeByte(0x2B);
  writeByte(0x0C);
  writeByte(0x0E);
  writeByte(0x08);
  writeByte(0x4E);
  writeByte(0xF1);
  writeByte(0x37);
  writeByte(0x07);
  writeByte(0x10);
  writeByte(0x03);
  writeByte(0x0E);
  writeByte(0x09);
  writeByte(0x00);
  writeCmd(0xE1);
  writeByte(0x00);
  writeByte(0x0E);
  writeByte(0x14);
  writeByte(0x03);
  writeByte(0x11);
  writeByte(0x07);
  writeByte(0x31);
  writeByte(0xC1);
  writeByte(0x48);
  writeByte(0x08);
  writeByte(0x0F);
  writeByte(0x0C);
  writeByte(0x31);
  writeByte(0x36);
  writeByte(0x0F);
  writeCmd(ILI_INVOFF);
  writeCmd(ILI_DISPON);
  sleep_ms(10);
}

// ── UI helpers
// ────────────────────────────────────────────────────────────────

void TftDisplay::drawHeader() {
  fillRect(0, 0, ILI9341_WIDTH, HEAD_H, COL_HEADER_BG);
  fillRect(0, HEAD_H - 2, ILI9341_WIDTH, 2, 0x780Fu); // purple separator
  // Title centred
  const char *t = "RATTENFEST";
  drawString((ILI9341_WIDTH - strPixelW(t, 3)) / 2, (HEAD_H - 21) / 2, t,
             COL_HDR_FG, COL_HEADER_BG, 3);
}

// ── Public: updateActiveCategory ─────────────────────────────────────────────
void TftDisplay::updateActiveCategory(const char *name, bool lit) {
  // Clear left panel
  fillRect(0, BODY_Y, DIV_X, BODY_H, COL_BG);

  // Label
  const char *label = "Studiengang";
  int lw = strPixelW(label, 1);
  drawString((DIV_X - lw) / 2, BODY_Y + 100, label, COL_CTR_NAME, COL_BG, 1);

  if (!name) {
    // No selection
    const char *none = "-- none --";
    int nw = strPixelW(none, 2);
    drawString((DIV_X - nw) / 2, BODY_Y + 120, none, COL_DIV, COL_BG, 2);
  } else {
    uint16_t col = lit ? COL_CAT_ON : COL_CAT_DIM;
    // Choose largest scale that fits within the panel width
    uint8_t sc = 3;
    if (strPixelW(name, sc) > DIV_X - 8)
      sc = 2;
    if (strPixelW(name, sc) > DIV_X - 8)
      sc = 1;
    int nw = strPixelW(name, sc);
    drawString((DIV_X - nw) / 2, BODY_Y + 120, name, col, COL_BG, sc);
  }
}

// ── Public: showConnecting / showOffline
// ──────────────────────────────────────
void TftDisplay::showConnecting() {
  fillScreen(COL_BG);
  drawHeader();
  const char *msg1 = "Connecting to";
  const char *msg2 = "WiFi...";
  int w1 = strPixelW(msg1, 2);
  int w2 = strPixelW(msg2, 2);
  drawString((ILI9341_WIDTH - w1) / 2, 120, msg1, COL_HDR_FG, COL_BG, 2);
  drawString((ILI9341_WIDTH - w2) / 2, 150, msg2, COL_HDR_FG, COL_BG, 2);
}

void TftDisplay::showSetupMode() {
  fillScreen(COL_BG);
  drawHeader();
  const char *msg1 = "SETUP MODE";
  const char *msg2 = "Connect to:";
  const char *msg3 = "ButtonBox-Config";
  const char *msg4 = "Visit: 192.168.4.1";
  int w1 = strPixelW(msg1, 2);
  int w2 = strPixelW(msg2, 1);
  int w3 = strPixelW(msg3, 1);
  int w4 = strPixelW(msg4, 1);
  drawString((ILI9341_WIDTH - w1) / 2, 60, msg1, 0x07E0u, COL_BG, 2);
  drawString((ILI9341_WIDTH - w2) / 2, 110, msg2, COL_HDR_FG, COL_BG, 1);
  drawString((ILI9341_WIDTH - w3) / 2, 130, msg3, 0x07E0u, COL_BG, 1);
  drawString((ILI9341_WIDTH - w4) / 2, 160, msg4, COL_HDR_FG, COL_BG, 1);
  const char *msg5 = "Then fill the form";
  int w5 = strPixelW(msg5, 1);
  drawString((ILI9341_WIDTH - w5) / 2, 200, msg5, COL_HDR_FG, COL_BG, 1);
}

void TftDisplay::showOffline() {
  fillScreen(0xF800u); // Red background
  const char *msg1 = "WiFi Connection";
  const char *msg2 = "LOST!";
  const char *msg3 = "reconnecting...";
  int w1 = strPixelW(msg1, 2);
  int w2 = strPixelW(msg2, 3);
  int w3 = strPixelW(msg3, 2);
  drawString((ILI9341_WIDTH - w1) / 2, 120, msg1, COL_HDR_FG, 0xF800u, 2);
  drawString((ILI9341_WIDTH - w2) / 2, 160, msg2, COL_HDR_FG, 0xF800u, 3);
  drawString((ILI9341_WIDTH - w3) / 2, 200, msg3, COL_HDR_FG, 0xF800u, 2);
}

// ── Public: drawCounterPanelRaw
// ───────────────────────────────────────────────
void TftDisplay::drawCounterPanelRaw(int panel, const char *name,
                                     uint8_t value) {
  uint16_t x = CTR_X;
  uint16_t y = (uint16_t)(BODY_Y + panel * CTR_H);
  uint16_t w = CTR_W;

  fillRect(x, y, w, CTR_H, COL_CTR_BG);
  // Top divider
  fillRect(x, y, w, 1, COL_CTR_DIV);

  // Name (scale 1, centred, truncated to 10 chars)
  char nbuf[11];
  int ni = 0;
  while (name[ni] && ni < 10) {
    nbuf[ni] = name[ni];
    ni++;
  }
  nbuf[ni] = '\0';
  int nw = strPixelW(nbuf, 1);
  drawString(x + (w - nw) / 2, y + 6, nbuf, COL_CTR_NAME, COL_CTR_BG, 1);

  // Divider
  fillRect(x + 4, y + 17, w - 8, 1, COL_CTR_DIV);

  // Value (scale 3: 21px tall, up to 3 digits = 54px wide)
  char vbuf[5];
  snprintf(vbuf, sizeof(vbuf), "%d", value);
  uint16_t vcol = (value > 0) ? COL_CTR_VAL : COL_CTR_VAL_Z;
  int vw = strPixelW(vbuf, 3);
  drawString(x + (w - vw) / 2, y + 26, vbuf, vcol, COL_CTR_BG, 3);
}

// ── Public: drawAppUI
// ─────────────────────────────────────────────────────────
void TftDisplay::drawAppUI(const char *const *cat_names,
                           const char *const *ctr_names) {
  fillScreen(COL_BG);
  drawHeader();

  // Vertical divider between category grid and counters
  fillRect(DIV_X, BODY_Y, 2, BODY_H, COL_DIV);

  // Left panel: "no selection" text (updateActiveCategory called later)
  updateActiveCategory(nullptr, false);

  // All counter panels
  for (int p = 0; p < 3; p++)
    drawCounterPanelRaw(p, ctr_names[p], 0);
}

// ── Public: updateCounter
// ─────────────────────────────────────────────────────
void TftDisplay::updateCounter(int panel, const char *name, uint8_t value) {
  if (panel < 0 || panel > 2)
    return;
  drawCounterPanelRaw(panel, name, value);
}

// ── Public: showSent / showAlert
// ──────────────────────────────────────────────
void TftDisplay::drawOverlay(const char *line1, const char *line2,
                             uint16_t col) {
  // Semi-opaque box in the lower half (over category grid area)
  uint16_t bx = 10, by = 100, bw = 160, bh = (line2 ? 80 : 50);
  fillRect(bx, by, bw, bh, 0x0000u);
  fillRect(bx, by, bw, 2, col);
  fillRect(bx, by + bh - 2, bw, 2, col);
  fillRect(bx, by, 2, bh, col);
  fillRect(bx + bw - 2, by, 2, bh, col);

  int w1 = strPixelW(line1, 3);
  uint16_t y1 = (line2) ? by + 10 : by + (bh - 21) / 2;
  drawString(bx + (bw - w1) / 2, y1, line1, col, 0x0000u, 3);

  if (line2) {
    int w2 = strPixelW(line2, 2);
    drawString(bx + (bw - w2) / 2, by + 44, line2, col, 0x0000u, 2);
  }
}

void TftDisplay::showSent() {
  drawOverlay("SENT", nullptr, 0x07E0u); // green
}

void TftDisplay::showAlert(const char *line1, const char *line2) {
  drawOverlay(line1, line2, 0xF800u); // red
}
