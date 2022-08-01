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
 * @brief This file contains definitions for the LiTE button interface.
 * @file button.h
 */

#ifndef __LITE__BUTTON_H__
#define __LITE__BUTTON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/box.h>
#include <lite/theme.h>

/** @brief Macro to convert a generic LiteBox into a LiteButton. */
#define LITE_BUTTON(l) ((LiteButton*) (l))

/** @brief Button state. */
typedef enum {
     LITE_BS_NORMAL,                          /**< Button is in a normal draw state and off */
     LITE_BS_PRESSED,                         /**< Button is in a pressed draw state */
     LITE_BS_HILITE,                          /**< Button is in a hilite draw state and off */
     LITE_BS_DISABLED,                        /**< Button is in a disabled draw state and off */
     LITE_BS_HILITE_ON,                       /**< Button is in a hilite draw state and on */
     LITE_BS_DISABLED_ON,                     /**< Button is in a disabled draw state and on */
     LITE_BS_NORMAL_ON,                       /**< Button is in a normal draw state and on */
     LITE_BS_MAX                              /**< Number of possible states */
} LiteButtonState;

/** @brief Button type. */
typedef enum {
     LITE_BT_PUSH,                            /**< Push button */
     LITE_BT_TOGGLE                           /**< Toggle button */
} LiteButtonType;

/** @brief LiteButton theme. */
typedef struct {
     LiteTheme         theme;                 /**< Base LiTE theme */

     IDirectFBSurface *surfaces[LITE_BS_MAX]; /**< Push and Toggle button images (normal, pressed, hilite, disabled) */
} LiteButtonTheme;

/** @brief No button theme. */
#define liteNoButtonTheme NULL

/** @brief Default button theme. */
extern LiteButtonTheme *liteDefaultButtonTheme;

/** @brief LiteButton structure. */
typedef struct _LiteButton LiteButton;

/** @brief Callback function prototype for a button press. */
typedef void (*LiteButtonPressFunc)( LiteButton *button, void *data );

/**
 * @brief Create a new LiteButton object.
 *
 * This function will create a new LiteButton object.
 *
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  rect                          Rectangle for the LiteButton object
 * @param[in]  theme                         Button theme
 * @param[out] ret_button                    Valid LiteButton object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_button                  ( LiteBox          *parent,
                                             DFBRectangle     *rect,
                                             LiteButtonTheme  *theme,
                                             LiteButton      **ret_button );

/**
 * @brief Enable/disable button.
 *
 * This function will enable or disable the button.
 *
 * @param[in]  button                        Valid LiteButton object
 * @param[in]  enabled                       Enabled or not
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_enable_button               ( LiteButton *button,
                                             int         enabled );

/**
 * @brief Set the button type.
 *
 * This function will set the button type.
 *
 * @param[in]  button                        Valid LiteButton object
 * @param[in]  type                          Button type
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_button_type             ( LiteButton     *button,
                                             LiteButtonType  type );

/**
 * @brief Set the button state.
 *
 * This function will set the button state.
 *
 * @param[in]  button                        Valid LiteButton object
 * @param[in]  state                         Button state
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_button_state            ( LiteButton      *button,
                                             LiteButtonState  state );

/**
 * @brief Get button state.
 *
 * This function will get the button state.
 *
 * @param[in]  button                        Valid LiteButton object
 * @param[out] ret_state                     Button state
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_button_state            ( LiteButton      *button,
                                             LiteButtonState *ret_state );

/**
 * @brief Set button image.
 *
 * This function will set the image of a button.
 * Each button state corresponds to a separate image.
 *
 * @param[in]  button                        Valid LiteButton object
 * @param[in]  state                         Button state
 * @param[in]  image_path                    File path with an image
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_button_image            ( LiteButton      *button,
                                             LiteButtonState  state,
                                             const char      *image_path );

/**
 * @brief Set the button image using an IDirectFBSurface object.
 *
 * This function will set the image of a button using an
 * IDirectFBSurface object. This can be used to directly set the
 * button image using a set of reusable surface elements.
 *
 * @param[in]  button                        Valid LiteButton object
 * @param[in]  state                         Button state
 * @param[in]  surface                       IDirectFBSurface object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_button_image_surface    ( LiteButton       *button,
                                             LiteButtonState   state,
                                             IDirectFBSurface *surface );

/**
 * @brief Install a callback function for a button press.
 *
 * This function will install a callback triggered when a button
 * is pressed.
 *
 * @param[in]  button                        Valid LiteButton object
 * @param[in]  callback                      Callback function
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_button_press             ( LiteButton          *button,
                                             LiteButtonPressFunc  callback,
                                             void                *data );

/**
 * @brief Create a button theme.
 *
 * This function makes the theme.
 *
 * @param[in]  image_paths                   File paths with button images
 * @param[out] ret_theme                     New theme
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_button_theme            ( const char       *image_paths[LITE_BS_MAX],
                                             LiteButtonTheme **ret_theme );

/**
 * @brief Destroy a button theme.
 *
 * This function will release the theme resources.
 *
 * @param[in]  theme                         Theme to destroy
 *
 * @return DFB_OK If successful.
 */
DFBResult lite_destroy_button_theme        ( LiteButtonTheme *theme );

#ifdef __cplusplus
}
#endif

#endif
