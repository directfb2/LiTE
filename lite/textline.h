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
 * @brief This file contains definitions for the LiTE text line interface.
 * @file textline.h
 */

#ifndef __LITE__TEXTLINE_H__
#define __LITE__TEXTLINE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/box.h>
#include <lite/theme.h>

/** @brief Macro to convert a generic LiteBox into a LiteTextLine. */
#define LITE_TEXTLINE(l) ((LiteTextLine*) (l))

/** @brief LiteTextLine theme. */
typedef struct {
     LiteTheme theme;            /**< Base LiTE theme */

     DFBColor  bg_color_changed; /**< Background color for changed text line */
} LiteTextLineTheme;

/** @brief No text line theme. */
#define liteNoTextLineTheme NULL

/** @brief Default text line theme. */
extern LiteTextLineTheme *liteDefaultTextLineTheme;

/** @brief LiteTextLine structure. */
typedef struct _LiteTextLine LiteTextLine;

/** @brief Callback function prototype when the Enter key is triggered in a focused text line. */
typedef void (*LiteTextLineEnterFunc)( const char *text, void *data );

/** @brief Callback function prototype when the Escape key is triggered in a focused text line. */
typedef void (*LiteTextLineAbortFunc)( void *data );

/**
 * @brief Create a new LiteTextLine object.
 *
 * This function will create a new LiteTextLine object.
 *
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  rect                          Rectangle for the LiteTextLine object
 * @param[in]  theme                         Text line theme
 * @param[out] ret_textline                  Valid LiteTextLine object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_textline                ( LiteBox            *parent,
                                             DFBRectangle       *rect,
                                             LiteTextLineTheme  *theme,
                                             LiteTextLine      **ret_textline );

/**
 * @brief Set the text field of a text line.
 *
 * This function will set the text field of a text line.
 *
 * @param[in]  textline                       Valid LiteTextLine object
 * @param[in]  text                           Text
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_textline_text           ( LiteTextLine *textline,
                                             const char   *text );

/**
 * @brief Get the text field of a text line.
 *
 * This function will get the text field of a text line.
 *
 * @param[in]  textline                       Valid LiteTextLine object
 * @param[in]  ret_text                       Text
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_textline_text           ( LiteTextLine  *textline,
                                             char         **ret_text );

/**
 * @brief Install a callback function for the Enter key press.
 *
 * This function will install a callback triggered when the Enter
 * key is pressed.
 *
 * @param[in]  textline                      Valid LiteTextLine object
 * @param[in]  callback                      Callback function
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_textline_enter           ( LiteTextLine          *textline,
                                             LiteTextLineEnterFunc  callback,
                                             void                  *data );

/**
 * @brief Install a callback function for the Escape key press.
 *
 * This function will install a callback triggered when the Escape
 * key is pressed.
 *
 * @param[in]  textline                      Valid LiteTextLine object
 * @param[in]  callback                      Callback function
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_textline_abort           ( LiteTextLine          *textline,
                                             LiteTextLineAbortFunc  callback,
                                             void                  *data );

/**
 * @brief Create a text line theme.
 *
 * This function makes the theme.
 *
 * @param[in]  bg_color                      Background color
 * @param[in]  bg_color_changed              Background color changed
 * @param[out] ret_theme                     New theme
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_textline_theme          ( const DFBColor     *bg_color,
                                             const DFBColor     *bg_color_changed,
                                             LiteTextLineTheme **ret_theme );

/**
 * @brief Destroy a text line theme.
 *
 * This function will release the theme resources.
 *
 * @param[in]  theme                         Theme to destroy

 * @return DFB_OK If successful.
 */
DFBResult lite_destroy_textline_theme      ( LiteTextLineTheme *theme );

#ifdef __cplusplus
}
#endif

#endif
