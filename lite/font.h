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
 * @brief This file contains definitions for the LiTE font interface.
 * @file font.h
 */

#ifndef __LITE__FONT_H__
#define __LITE__FONT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/box.h>

/** @brief Font style. */
typedef enum {
  LITE_FONT_PLAIN  = 0, /**< Plain font */
  LITE_FONT_BOLD   = 1, /**< Bold font */
  LITE_FONT_ITALIC = 2  /**< Italics font */
} LiteFontStyle;

/** @brief LiteFont structure. */
typedef struct _LiteFont LiteFont;

/** @brief Default font attribute. */
#define DEFAULT_FONT_ATTRIBUTE DFFA_NONE

/**
 * @brief Get a LiteFont object based on specifications.
 *
 * This function will get a LiteFont object based on specifications
 * like style, size, etc.
 *
 * @param[in]  spec                          Font specification
 * @param[in]  style                         Font style
 * @param[in]  size                          Font size
 * @param[in]  attr                          Font attributes
 * @param[out] ret_font                      Valid LiteFont object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_font                    ( const char         *spec,
                                             LiteFontStyle       style,
                                             int                 size,
                                             DFBFontAttributes   attr,
                                             LiteFont          **ret_font );

/**
 * @brief Increase the reference count of a LiteFont object.
 *
 * This function will increase the reference count of a LiteFont
 * object. This is used to indicate that this LiteFont object is
 * used in another context and should not be purged.
 *
 * @param font                               Valid LiteFont object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_ref_font                    ( LiteFont *font );

/**
 * @brief Release a LiteFont object.
 *
 * This function will decrease the reference count of a LiteFont,
 * so this LiteFont object can eventually be purged.
 *
 * @param[in]  font                          Valid LiteFont object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_release_font                ( LiteFont *font );

/**
 * @brief Get the underlying IDirectFBFont interface.
 *
 * This function will retrieve the underlying IDirectFBFont
 * interface of a LiteFont object.
 *
 * @param[in]  font                          Valid LiteFont object
 * @param[out] ret_font                      IDirectFBFont object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_font                        ( LiteFont       *font,
                                             IDirectFBFont **ret_font );

/**
 * @brief Set the active font in a LiteBox object.
 *
 * This function will set the active font in a LiteBox object.
 *
 * @param[in]  box                           Valid LiteBox object
 * @param[in]  font                          Valid LiteFont object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_active_font             ( LiteBox  *box,
                                             LiteFont *font );

/**
 * @brief Get the currently active font in a LiteBox object.
 *
 * This function will retrieve the currently active font in a
 * LiteBox object.
 *
 * @param[in]  box                           Valid LiteBox object
 * @param[in]  ret_font                      Valid LiteFont object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_active_font             ( LiteBox   *box,
                                             LiteFont **ret_font );

/**
 * @brief Get the attributes of the font.
 *
 * This function will retrieve the attributes of the font.
 *
 * @param[in]  font                          Valid LiteFont object
 * @param[out] ret_attr                      Font attributes
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_font_attributes         ( LiteFont          *font,
                                             DFBFontAttributes *ret_attr );

#ifdef __cplusplus
}
#endif

#endif
