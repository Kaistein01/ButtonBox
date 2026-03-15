#pragma once

#include <stdint.h>

/**
 * TftDisplay — ILI9341 240×320 portrait display driver.
 *
 * Application UI layout:
 *   y 0-31   : Header — title centred, WiFi dot top-left
 *   y 32-319 : Body
 *     x 0-171  : Active category text panel
 *     x 172-173: vertical divider
 *     x 174-239: 3 counter panels (each 96px = 30% of 288px body)
 *
 * PIO note: Hardware SPI0 used (SCK=18, MOSI=19 are PL022-capable).
 */
class TftDisplay {
public:
  void init();

  // ── Application UI ────────────────────────────────────────────────

  /** Draw the full UI. Call after init() and after any reset. */
  void drawAppUI(const char *const *cat_names, const char *const *ctr_names);

  /** Update the active-category text on the left panel.
   *  @param name  Category name, or nullptr for "no selection".
   *  @param lit   Blink phase: true=bright green, false=dim green.
   */
  void updateActiveCategory(const char *name, bool lit);

  /** Show full-screen "Connecting to WiFi..." message. */
  void showConnecting();

  /** Show full-screen setup mode message with instructions. */
  void showSetupMode();

  /** Show full-screen "WiFi Offline!" error blocking the UI. */
  void showOffline();

  /** Redraw one counter panel with the current value. */
  void updateCounter(int panel, const char *name, uint8_t value);

  /** Show "SENT" overlay centred on the display. */
  void showSent();

  /**
   * Show an error alert centred on the display.
   * @param line1  First line (e.g. "No category")
   * @param line2  Second line (e.g. "selected!")
   */
  void showAlert(const char *line1, const char *line2 = nullptr);

private:
  // ── SPI primitives ───────────────────────────────────────────────
  void csLow();
  void csHigh();
  void writeCmd(uint8_t cmd);
  void writeByte(uint8_t data);
  void writeWord(uint16_t data);

  // ── Drawing primitives ───────────────────────────────────────────
  void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
  void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
  void fillScreen(uint16_t color);

  // ── Text (5×7 bitmap font, scalable) ─────────────────────────────
  void drawChar(int16_t x, int16_t y, char c, uint16_t fg, uint16_t bg,
                uint8_t scale);
  void drawString(int16_t x, int16_t y, const char *str, uint16_t fg,
                  uint16_t bg, uint8_t scale);
  int strPixelW(const char *str, uint8_t scale);

  // ── Private UI helpers ───────────────────────────────────────────
  void drawHeader();
  void drawCounterPanelRaw(int panel, const char *name, uint8_t value);
  void drawOverlay(const char *line1, const char *line2, uint16_t color);
};
