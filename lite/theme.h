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
 * @brief This file contains definitions for the LiTE theme interface.
 * @file theme.h
 */

#ifndef __LITE__THEME_H__
#define __LITE__THEME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <directfb.h>

/** @brief Base LiTE theme structure. */
typedef struct {
     DFBColor bg_color;                                   /**< Background color */
     DFBColor fg_color;                                   /**< Foreground color */
} LiteTheme;

/** @brief LiteThemeFramePart structure. */
typedef struct {
     IDirectFBSurface *source;                            /**< Surface holding the parts of the frame */
     DFBRectangle      rect;                              /**< Portion of surface containing part of frame */
} LiteThemeFramePart;

/** @brief Frame parts. */
typedef enum {
     LITE_THEME_FRAME_PART_TOP,                           /**< Top part of the frame */
     LITE_THEME_FRAME_PART_BOTTOM,                        /**< Bottom part of the frame */
     LITE_THEME_FRAME_PART_LEFT,                          /**< Left part of the frame */
     LITE_THEME_FRAME_PART_RIGHT,                         /**< Right part of the frame */
     LITE_THEME_FRAME_PART_TOPLEFT,                       /**< Top left part of the frame */
     LITE_THEME_FRAME_PART_TOPRIGHT,                      /**< Top right part of the frame */
     LITE_THEME_FRAME_PART_BOTTOMLEFT,                    /**< Bottom left part of the frame */
     LITE_THEME_FRAME_PART_BOTTOMRIGHT,                   /**< Bottom right part of the frame */
     LITE_THEME_FRAME_PART_NUM                            /**< Number of parts */
} LiteThemeFramePartIndex;

/** @brief LiteThemeFrame structure. */
typedef struct {
     int                magic;                            /**< Magic field */
     LiteThemeFramePart parts[LITE_THEME_FRAME_PART_NUM]; /**< Frame parts */
} LiteThemeFrame;

/** @brief Load a frame. */
DFBResult lite_theme_frame_load            ( LiteThemeFrame  *frame,
                                             const char     **filenames );

/** @brief Unload a frame. */
void      lite_theme_frame_unload          ( LiteThemeFrame *frame );

/** @brief Update the frame rectangles for the target destination. */
void      lite_theme_frame_target_update   ( DFBRectangle         *frame_target,
                                             const LiteThemeFrame *frame,
                                             const DFBDimension   *size );

#ifdef __cplusplus
}
#endif

#endif
