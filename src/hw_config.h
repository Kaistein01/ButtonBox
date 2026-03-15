#pragma once

#include "hardware/spi.h"

// TFT resolution
#define ILI9341_WIDTH   240
#define ILI9341_HEIGHT  320

// SPI (for LCD display)
#define PIN_TFT_CS   17
#define PIN_TFT_DC   20
#define PIN_TFT_RST  21

#define PIN_SPI_SCK  18
#define PIN_SPI_TX   19
#define PIN_SPI_RX   16

#define SPI_PORT spi0

// LED Matrix 3 x 5
#define PIN_LEDM_ROW_1 0
#define PIN_LEDM_ROW_2 1
#define PIN_LEDM_ROW_3 2

#define PIN_LEDM_COL_1 3
#define PIN_LEDM_COL_2 4
#define PIN_LEDM_COL_3 5
#define PIN_LEDM_COL_4 6
#define PIN_LEDM_COL_5 7

// Button Matrix 3 x 5
#define PIN_BUTM_ROW_1 8
#define PIN_BUTM_ROW_2 9
#define PIN_BUTM_ROW_3 10

#define PIN_BUTM_COL_1 11
#define PIN_BUTM_COL_2 12
#define PIN_BUTM_COL_3 13
#define PIN_BUTM_COL_4 14
#define PIN_BUTM_COL_5 15

// Single buttons and LEDs
#define PIN_BUT_ACCEPT 27
#define PIN_BUT_CANCEL 26

#define PIN_LED_ACCEPT 22
#define PIN_LED_CANCEL 28

// Some RGB565 colors
#define COLOR_BLUE      0x001F
#define COLOR_PURPLE    0xF81F
#define COLOR_WHITE     0xFFFF
#define COLOR_BLACK     0x0000
#define COLOR_GRAY      0x7BEF
#define COLOR_GREEN     0x07E0
#define COLOR_RED       0xF800
