/*
   This file is part of LiTE.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

/**
 * @brief This file contains definitions for the LiTE configuration.
 * @file lite_config.h
 **/

#ifndef __LITE__CONFIG_H__
#define __LITE__CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Default window title font. */
#define DEFAULT_WINDOW_TITLE_FONT         "whitrabt"

/** @brief Default window top frame. */
#define DEFAULT_WINDOW_TOP_FRAME          "top"

/** @brief Default window bottom frame. */
#define DEFAULT_WINDOW_BOTTOM_FRAME       "bottom"

/** @brief Default window left frame. */
#define DEFAULT_WINDOW_LEFT_FRAME         "left"

/** @brief Default window right frame. */
#define DEFAULT_WINDOW_RIGHT_FRAME        "right"

/** @brief Default window top left frame. */
#define DEFAULT_WINDOW_TOP_LEFT_FRAME     "topleft"

/** @brief Default window top right frame. */
#define DEFAULT_WINDOW_TOP_RIGHT_FRAME    "topright"

/** @brief Default window bottom left frame. */
#define DEFAULT_WINDOW_BOTTOM_LEFT_FRAME  "bottomleft"

/** @brief Default window bottom right frame. */
#define DEFAULT_WINDOW_BOTTOM_RIGHT_FRAME "bottomright"

/** @brief Default window background color (R component). */
#define DEFAULT_WINDOW_COLOR_R            0xee

/** @brief Default window background color (G component). */
#define DEFAULT_WINDOW_COLOR_G            0xee

/** @brief Default window background color (B component). */
#define DEFAULT_WINDOW_COLOR_B            0xee

/** @brief Default window background color (Alpha component). */
#define DEFAULT_WINDOW_COLOR_A            0xff

/** @brief Default window cursor. */
#define DEFAULT_WINDOW_CURSOR             "cursor"

/** @brief Default hotspot x-coordinate. */
#define DEFAULT_WINDOW_CURSOR_HOTSPOT_X   0

/** @brief Default hotspot y-coordinate. */
#define DEFAULT_WINDOW_CURSOR_HOTSPOT_Y   0

/** @brief Default System font. */
#define DEFAULT_FONT_SYSTEM               "Vera"

/** @brief Default Monospaced font. */
#define DEFAULT_FONT_MONOSPACED           "VeraMo"

/** @brief Default Serif font. */
#define DEFAULT_FONT_SERIF                "VeraSe"

/** @brief Default Sans Serif font. */
#define DEFAULT_FONT_SANS_SERIF           "Vera"

/** @brief Default button image (normal and off). */
#define DEFAULT_BUTTON_IMAGE_NORMAL       "button_normal"

/** @brief Default button image (pressed). */
#define DEFAULT_BUTTON_IMAGE_PRESSED      "button_pressed"

/** @brief Default button image (hilite and off). */
#define DEFAULT_BUTTON_IMAGE_HILITE       "button_hilite"

/** @brief Default button image (disabled and off). */
#define DEFAULT_BUTTON_IMAGE_DISABLED     "button_disabled"

/** @brief Default button image (hilite and on). */
#define DEFAULT_BUTTON_IMAGE_HILITE_ON    "button_hilite_on"

/** @brief Default button image (disabled and on). */
#define DEFAULT_BUTTON_IMAGE_DISABLED_ON  "button_disabled_on"

/** @brief Default button image (normal and on). */
#define DEFAULT_BUTTON_IMAGE_NORMAL_ON    "button_normal_on"

/** @brief Default check image. */
#define DEFAULT_CHECK_IMAGE               "check"

/** @brief Default progress bar image (foreground). */
#define DEFAULT_PROGRESSBAR_IMAGE_FG      "progressbar_fg"

/** @brief Default progress bar image (background). */
#define DEFAULT_PROGRESSBAR_IMAGE_BG      "progressbar_bg"

/** @brief Default scrollbar image. */
#define DEFAULT_SCROLLBAR_IMAGE           "scrollbar"

/** @brief Default text button image. */
#define DEFAULT_TEXTBUTTON_IMAGE          "textbutton"

#ifdef __cplusplus
}
#endif

#endif
