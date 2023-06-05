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
 * @brief This file contains definitions for the LiTE cursor interface.
 * @file cursor.h
 */

#ifndef __LITE__CURSOR_H__
#define __LITE__CURSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/window.h>

/** @brief LiteCursor structure. */
typedef struct {
     IDirectFBSurface *surface; /**< Cursor image */
     int               width;   /**< Cursor width */
     int               height;  /**< Cursor height */
     int               hot_x;   /**< Hotspot x-coordinate */
     int               hot_y;   /**< Hotspot y-coordinate */
} LiteCursor;

/**
 * @brief Get the currently active global cursor.
 *
 * This function will retrieve the currently active global cursor.
 *
 * @param[out] ret_cursor                    Valid LiteCursor object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_current_cursor          ( LiteCursor **ret_cursor );

/**
 * @brief Set the active global cursor.
 *
 * This function will set the active global cursor.
 *
 * @param[in]  cursor                        Valid LiteCursor object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_current_cursor          ( LiteCursor *cursor );

/**
 * @brief Load a cursor.
 *
 * This function will load a cursor.
 *
 * @param[in]  cursor                        Valid LiteCursor object
 * @param[in]  file_data                     File data (file path or data pointer) with the cursor image
 * @param[in]  length                        Length (0 if file path, length of buffer if data pointer)
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_load_cursor                 ( LiteCursor   *cursor,
                                             const void   *file_data,
                                             unsigned int  length );

/**
 * @brief Free cursor resources.
 *
 * This function will release the cursor resources.
 *
 * @param[in]  cursor                        Valid LiteCursor object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_free_cursor                 ( LiteCursor *cursor );

/**
 * @brief Set window cursor.
 *
 * This function will set the active cursor for a specific window.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  cursor                        Valid LiteCursor object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_window_cursor           ( LiteWindow *window,
                                             LiteCursor *cursor );

/**
 * @brief Hide current cursor.
 *
 * This function will hide the currently active cursor.
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_hide_cursor                 ( void );

/**
 * @brief Show current cursor.
 *
 * This function will show the current active cursor in case it
 * was hidden before.
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_show_cursor                 ( void );

/**
 * @brief Change cursor opacity.
 *
 * This function will change the opacity level of the currently
 * active cursor. Maximum opacity level 0xff means that the cursor
 * is not transparent (fully visible), value 0 means that the
 * cursor is not visible.
 *
 * @param[in]  opacity                       Opacity level (0 to 0xff)
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_change_cursor_opacity       ( u8 opacity );

/**
 * @brief Get cursor opacity.
 *
 * This function will retrieve the cursor opacity level of the
 * currently active cursor.
 *
 * @param[out] ret_opacity                   Opacity level (0 to 0xff)
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_cursor_opacity          ( u8 *ret_opacity );

/**
 * @brief Set the cursor hotspot.
 *
 * This function will set the cursor hotspot. Call this function
 * before the cursor is used.
 *
 * @param[in]  cursor                        Valid LiteCursor object
 * @param[in]  hot_x                         Hotspot x-coordinate
 * @param[in]  hot_y                         Hotspot y-coordinate
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_cursor_hotspot          ( LiteCursor   *cursor,
                                             unsigned int  hot_x,
                                             unsigned int  hot_y );

#ifdef __cplusplus
}
#endif

#endif
