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
 * @brief This file contains definitions for the LiTE check interface.
 * @file check.h
 */

#ifndef __LITE__CHECK_H__
#define __LITE__CHECK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/box.h>
#include <lite/theme.h>

/** @brief Macro to convert a generic LiteBox into a LiteCheck. */
#define LITE_CHECK(l) ((LiteCheck*) (l))

/** @brief Check box state. */
typedef enum {
     LITE_CHS_UNCHECKED,             /**< Unchecked */
     LITE_CHS_CHECKED                /**< Checked */
} LiteCheckState;

/** @brief LiteCheck theme. */
typedef struct {
     LiteTheme theme;                /**< Base LiTE theme */

     struct {
          IDirectFBSurface *surface;
          int               width;
          int               height;
     } all_images;                   /**< All check box images (checked, unchecked, hilite, disabled) */
} LiteCheckTheme;

/** @brief No check theme. */
#define liteNoCheckTheme NULL

/** @brief Default check theme. */
extern LiteCheckTheme *liteDefaultCheckTheme;

/** @brief LiteCheck structure. */
typedef struct _LiteCheck LiteCheck;

/** @brief Callback function prototype for checked/unchecked presses. */
typedef void (*LiteCheckPressFunc)( LiteCheck *check, LiteCheckState state, void *data );

/**
 * @brief Create a new LiteCheck object.
 *
 * This function will create a new LiteCheck object.
 *
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  caption_text                  Check box caption text
 * @param[in]  rect                          Rectangle for the LiteCheck object
 * @param[in]  theme                         Check theme
 * @param[out] ret_check                     Valid LiteCheck object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_check                   ( LiteBox         *parent,
                                             DFBRectangle    *rect,
                                             const char      *caption_text,
                                             LiteCheckTheme  *theme,
                                             LiteCheck      **ret_check );

/**
 * @brief Set the check box caption text.
 *
 * This function will set the caption text of the check box.
 * If the string is too long, part of the end of the string will
 * be drawn as "...".
 *
 * @param[in]  check                         Valid LiteCheck object
 * @param[in]  caption_text                  Check box caption text
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_check_caption           ( LiteCheck  *check,
                                             const char *caption_text );

/**
 * @brief Enable/disable check box control.
 *
 * This function will enable or disable check box control.
 *
 * @param[in]  check                         Valid Check object
 * @param[in]  enabled                       Enabled or not
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_enable_check                ( LiteCheck *check,
                                             int        enabled );


/**
 * @brief Check/uncheck the box.
 *
 * This function will check/uncheck the check box.
 *
 * @param[in]  check                         Valid Check object
 * @param[in]  state                         Check box state
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_check_check                 ( LiteCheck      *check,
                                             LiteCheckState  state );

/**
 * @brief Get check box state.
 *
 * This function will get the check box state.
 *
 * @param[in]  check                         Valid LiteCheck object
 * @param[out] ret_state                     Check box state
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_check_state             ( LiteCheck      *check,
                                             LiteCheckState *ret_state );

/**
 * @brief Set all check box images.
 *
 * This function will set the images of a check box.
 *
 * @param[in]  check                         Valid LiteCheck object
 * @param[in]  image_path                    File path with image for all check box states
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_check_all_images        ( LiteCheck  *check,
                                             const char *image_path );

/**
 * @brief Set all check box images from memory.
 *
 * This function will set the images of a check box from memory.
 *
 * @param[in]  check                         Valid LiteCheck object
 * @param[in]  data                          Data with image for all check box states
 * @param[in]  length                        Length of buffer
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_check_all_images_data   ( LiteCheck    *check,
                                             const void   *data,
                                             unsigned int  length );

/**
 * @brief Install a callback function for checked/unchecked presses.
 *
 * This function will install a callback triggered when a check box
 * is pressed.
 *
 * @param[in]  check                          Valid LiteCheck object
 * @param[in]  callback                       Callback function
 * @param[in]  data                           Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_check_press              ( LiteCheck          *check,
                                             LiteCheckPressFunc  callback,
                                             void               *data );

/**
 * @brief Create a check theme.
 *
 * This function makes the theme.
 *
 * @param[in]  file_data                     File data (file path or data pointer) with image for all check box states
 * @param[in]  length                        Length (0 if file path, length of buffer if data pointer)
 * @param[out] ret_theme                     New theme
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_check_theme             ( const void      *file_data,
                                             unsigned int     length,
                                             LiteCheckTheme **ret_theme );

/**
 * @brief Destroy a check theme.
 *
 * This function will release the theme resources.
 *
 * @param[in]  theme                         Theme to destroy
 *
 * @return DFB_OK If successful.
 */
DFBResult lite_destroy_check_theme         ( LiteCheckTheme *theme );

#ifdef __cplusplus
}
#endif

#endif
