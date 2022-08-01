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
 * @brief This file contains definitions for the LiTE progress bar interface.
 * @file progressbar.h
 */

#ifndef __LITE__PROGRESSBAR_H__
#define __LITE__PROGRESSBAR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/box.h>
#include <lite/theme.h>

/** @brief Macro to convert a generic LiteBox into a LiteProgressBar. */
#define LITE_PROGRESSBAR(l) ((LiteProgressBar*) (l))

/** @brief LiteProgressBar theme. */
typedef struct {
     LiteTheme theme;              /**< Base LiTE theme */

     IDirectFBSurface *surface_fg; /**< Foreground image */
     IDirectFBSurface *surface_bg; /**< Background image */
} LiteProgressBarTheme;

/** @brief No progress bar theme. */
#define liteNoProgressBarTheme NULL

/** @brief Default progress bar theme. */
extern LiteProgressBarTheme *liteDefaultProgressBarTheme;

/** @brief LiteProgressBar structure. */
typedef struct _LiteProgressBar LiteProgressBar;

/**
 * @brief Create a new LiteProgressBar object.
 *
 * This function will create a new LiteProgressBar object.
 *
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  rect                          Rectangle for the LiteProgressBar object
 * @param[in]  theme                         Progress bar theme
 * @param[out] ret_progressbar               Valid LiteProgressBar object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_progressbar             ( LiteBox               *parent,
                                             DFBRectangle          *rect,
                                             LiteProgressBarTheme  *theme,
                                             LiteProgressBar      **ret_progressbar );

/**
 * @brief Set the current value of the progress bar.
 *
 * This function will set the progress bar value.
 *
 * @param[in]  progressbar                   Valid LiteProgressBar object
 * @param[in]  value                         Percentage value between 0 and 1
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_progressbar_value       ( LiteProgressBar *progressbar,
                                             float            value );

/**
 * @brief Get progress bar value.
 *
 * This function will get the current value of the progress bar.
 *
 * @param[in]  progressbar                   Valid LiteProgressBar object
 * @param[in]  ret_value                     Percentage value between 0 and 1
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_progressbar_value       ( LiteProgressBar *progressbar,
                                             float           *ret_value );

/**
 * @brief Set progress bar images.
 *
 * This function will set the images of a progress bar.
 *
 * @param[in]  progressbar                   Valid LiteProgressBar object
 * @param[in]  image_fg_path                 File path with progress bar image (foreground)
 * @param[in]  image_bg_path                 File path with progress bar image (background)
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_progressbar_images      ( LiteProgressBar *progressbar,
                                             const char      *image_fg_path,
                                             const char      *image_bg_path );

/**
 * @brief Create a progress bar theme.
 *
 * This function makes the theme.
 *
 * @param[in]  image_fg_path                 File path with progress bar image (foreground)
 * @param[in]  image_bg_path                 File path with progress bar image (background)
 * @param[out] ret_theme                     New theme
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_progressbar_theme       ( const char            *image_fg_path,
                                             const char            *image_bg_path,
                                             LiteProgressBarTheme **ret_theme );

/**
 * @brief Destroy a progress bar theme.
 *
 * This function will release the theme resources.
 *
 * @param[in]  theme                         Theme to destroy
 *
 * @return DFB_OK If successful.
 */
DFBResult lite_destroy_progressbar_theme   ( LiteProgressBarTheme *theme );

#ifdef __cplusplus
}
#endif

#endif
