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
 * @brief This file contains definitions for the LiTE label interface.
 * @file label.h
 */

#ifndef __LITE__LABEL_H__
#define __LITE__LABEL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/font.h>
#include <lite/theme.h>

/** @brief Macro to convert a generic LiteBox into a LiteLabel. */
#define LITE_LABEL(l) ((LiteLabel*) (l))

/** @brief Label alignment. */
typedef enum {
     LITE_LABEL_LEFT,  /**< Align left  */
     LITE_LABEL_RIGHT, /**< Align right */
     LITE_LABEL_CENTER /**< Align center*/
} LiteLabelAlignment;

/** @brief LiteLabel theme. */
typedef struct {
     LiteTheme theme;  /**< Base LiTE theme */
} LiteLabelTheme;

/** @brief No label theme. */
#define liteNoLabelTheme NULL

/** @brief Default label theme. */
extern LiteLabelTheme *liteDefaultLabelTheme;

/** @brief LiteLabel structure. */
typedef struct _LiteLabel LiteLabel;

/**
 * @brief Create a new LiteLabel object.
 *
 * This function will create a new LiteLabel object.
 *
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  rect                          Rectangle for the LiteLabel object
 * @param[in]  theme                         Label theme
 * @param[in]  size                          Font size
 * @param[out] ret_label                     Valid LiteLabel object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_label                   ( LiteBox         *parent,
                                             DFBRectangle    *rect,
                                             LiteLabelTheme  *theme,
                                             int              size,
                                             LiteLabel      **ret_label );

/**
 * @brief Set label text.
 *
 * This function will set the label text.
 *
 * @param[in]  label                         Valid LiteLabel object
 * @param[in]  text                          Text string
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_label_text              ( LiteLabel  *label,
                                             const char *text );

/**
 * @brief Set the label text alignment.
 *
 * This function will set the label text alignment.
 *
 * @param[in]  label                         Valid LiteLabel object
 * @param[in]  alignment                     Label alignment
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_label_alignment         ( LiteLabel          *label,
                                             LiteLabelAlignment  alignment );

/**
 * @brief Set label font.
 *
 * This function will set the label font.
 *
 * @param[in]  label                         Valid LiteLabel object
 * @param[in]  spec                          Font specification
 * @param[in]  style                         Font style
 * @param[in]  size                          Font size
 * @param[in]  attr                          Font attributes
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_label_font              ( LiteLabel         *label,
                                             const char        *spec,
                                             LiteFontStyle      style,
                                             int                size,
                                             DFBFontAttributes  attr );

/**
 * @brief Set label text color.
 *
 * This function will set the label text color.
 *
 * @param[in]  label                         Valid LiteLabel object
 * @param[in]  color                         Color
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_label_color             ( LiteLabel *label,
                                             DFBColor  *color );

#ifdef __cplusplus
}
#endif

#endif
