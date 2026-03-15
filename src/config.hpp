#pragma once

/**
 * config.hpp — Edit this file to configure button box names.
 *
 * Category buttons: col 0-3, rows 0-2 (4×3 = 12 buttons)
 *   Index layout: cat_idx = row * 4 + col
 *
 * Counter buttons: col 4, rows 0-2 (3 buttons)
 *   Index: ctr_idx = row
 */

// Category names (12 total, row-major: index = row*4 + col)
// Keep names short (≤8 chars) for best display fit.
static const char *const CAT_NAMES[12] = {
    "ET", "I",   "M-I", "ABLR", // Row 0
    "EEU", "WING-F", "ETH",  "HSG",     // Row 1
    "UZH-PH", "Buezer", "Soz-Ges", "MSE",    // Row 2
};

// Counter names (3 total, one per row of column 4)
// Keep names short (≤8 chars).
static const char *const CTR_NAMES[3] = {
    "Bier",
    "Cocktail",
    "Shot",
};
