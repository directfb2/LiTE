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
 * @brief This file contains definitions for the LiTE text button interface.
 * @file textbutton.h
 */

#ifndef __LITE__TEXT_BUTTON_H__
#define __LITE__TEXT_BUTTON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/box.h>
#include <lite/theme.h>

/** @brief Macro to convert a generic LiteBox into a LiteTextButton. */
#define LITE_TEXTBUTTON(l) ((LiteTextButton*) (l))

/** @brief Text button state. */
typedef enum {
     LITE_TBS_NORMAL,                /**< Button is in a normal draw state */
     LITE_TBS_PRESSED,               /**< Button is in a pressed draw state */
     LITE_TBS_HILITE,                /**< Button is in a hilite draw state */
     LITE_TBS_DISABLED,              /**< Button is in a disabled draw state */
     LITE_TBS_MAX                    /**< Number of possible states */
} LiteTextButtonState;

/** @brief LiteTextButton theme. */
typedef struct {
     LiteTheme              theme;   /**< Base LiTE theme */

     struct {
          IDirectFBSurface *surface;
          int               width;
          int               height;
     } all_images;                   /**< All text button images (normal, pressed, hilite, disabled) */
} LiteTextButtonTheme;

/** @brief No text button theme. */
#define liteNoTextButtonTheme NULL

/** @brief Default text button theme. */
extern LiteTextButtonTheme *liteDefaultTextButtonTheme;

/** @brief LiteTextButton structure. */
typedef struct _LiteTextButton LiteTextButton;

/** @brief Callback function prototype for a text button press. */
typedef void (*LiteTextButtonPressFunc)( LiteTextButton *textbutton, void *data );

/**
 * @brief Create a new LiteTextButton object.
 *
 * This function will create a new LiteTextButton object.
 *
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  caption_text                  Button caption text
 * @param[in]  rect                          Rectangle for the LiteTextButton object
 * @param[in]  theme                         Text button theme
 * @param[out] ret_textbutton                Valid LiteTextButton object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_text_button             ( LiteBox              *parent,
                                             DFBRectangle         *rect,
                                             const char           *caption_text,
                                             LiteTextButtonTheme  *theme,
                                             LiteTextButton      **ret_textbutton );

/**
 * @brief Set the button caption text.
 *
 * This function will set the caption text of the button.
 * If the string is too long, part of the end of the string will
 * be drawn as "...".
 *
 * @param[in]  textbutton                    Valid LiteTextButton object
 * @param[in]  caption_text                  Button caption text
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_text_button_caption     ( LiteTextButton *textbutton,
                                             const char     *caption_text );

/**
 * @brief Enable/disable text button.
 *
 * This function will enable or disable the text button.
 *
 * @param[in]  textbutton                    Valid LiteTextButton object
 * @param[in]  enabled                       Enabled or not
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_enable_text_button          ( LiteTextButton *textbutton,
                                             int             enabled );

/**
 * @brief Set the text button state.
 *
 * This function will set the text button state.
 *
 * @param[in]  textbutton                    Valid LiteTextButton object
 * @param[in]  state                         Text button state
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_text_button_state       ( LiteTextButton      *textbutton,
                                             LiteTextButtonState  state );

/**
 * @brief Get text button state.
 *
 * This function will get the text button state.
 *
 * @param[in]  textbutton                    Valid LiteTextButton object
 * @param[out] ret_state                     Text button state
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_text_button_state       ( LiteTextButton      *textbutton,
                                             LiteTextButtonState *ret_state );

/**
 * @brief Set all text button images.
 *
 * This function will set the images of a text button.
 *
 * @param[in]  textbutton                    Valid LiteTextButton object
 * @param[in]  image_path                    File path with image for all text button states
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_text_button_all_images  ( LiteTextButton *textbutton,
                                             const char     *image_path );

/**
 * @brief Set all text button images from memory.
 *
 * This function will set the images of a text button from memory.
 *
 * @param[in]  textbutton                    Valid LiteTextButton object
 * @param[in]  data                          Data with image for all text button states
 * @param[in]  length                        Length of buffer
 *
 * @return DFB_OK if successful.
 */
DFBResult
lite_set_text_button_all_images_data       ( LiteTextButton *textbutton,
                                             const void     *data,
                                             unsigned int    length );

/**
 * @brief Install a callback function for a text button press.
 *
 * This function will install a callback triggered when a text
 * button is pressed.
 *
 * @param[in]  textbutton                    Valid LiteTextButton object
 * @param[in]  callback                      Callback function
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_text_button_press        ( LiteTextButton          *textbutton,
                                             LiteTextButtonPressFunc  callback,
                                             void                    *data );

/**
 * @brief Create a text button theme.
 *
 * This function makes the theme.
 *
 * @param[in]  file_data                     File data (file path or data pointer) with image for all text button states
 * @param[in]  length                        Length (0 if file path, length of buffer if data pointer)
 * @param[out] ret_theme                     New theme
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_text_button_theme       ( const void           *file_data,
                                             unsigned int          length,
                                             LiteTextButtonTheme **ret_theme );

/**
 * @brief Destroy a text button theme.
 *
 * This function will release the theme resources.
 *
 * @param[in]  theme                         Theme to destroy
 *
 * @return DFB_OK If successful.
 */
DFBResult lite_destroy_text_button_theme   ( LiteTextButtonTheme *theme );

#ifdef __cplusplus
}
#endif

#endif
