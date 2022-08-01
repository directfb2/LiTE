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
 * @brief This file contains definitions for the LiTE animation interface.
 * @file animation.h
 */

#ifndef __LITE__ANIMATION_H__
#define __LITE__ANIMATION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/box.h>
#include <lite/theme.h>

/** @brief Macro to convert a generic LiteBox into a LiteAnimation. */
#define LITE_ANIMATION(l) ((LiteAnimation*) (l))

/** @brief LiteAnimation theme. */
typedef struct {
     LiteTheme theme; /**< Base LiTE theme */
} LiteAnimationTheme;

/** @brief No animation theme. */
#define liteNoAnimationTheme NULL

/** @brief Default animation theme. */
extern LiteAnimationTheme *liteDefaultAnimationTheme;

/** @brief LiteAnimation structure. */
typedef struct _LiteAnimation LiteAnimation;

/**
 * @brief Create a new LiteAnimation object.
 *
 * This function will create a new LiteAnimation object.
 *
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  rect                          Rectangle for the LiteAnimation object
 * @param[in]  theme                         Animation theme
 * @param[out] ret_animation                 Valid LiteAnimation object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_animation               ( LiteBox             *parent,
                                             DFBRectangle        *rect,
                                             LiteAnimationTheme  *theme,
                                             LiteAnimation      **ret_animation );

/**
 * @brief Load the animation sequence.
 *
 * This function will load an image containing the animation.
 *
 * @param[in]  animation                     Valid LiteAnimation object
 * @param[in]  filename                      File path with an image containing the animation frames
 * @param[in]  still_frame                   Index of the animation frame
 * @param[in]  frame_width                   Width of the animation frame
 * @param[in]  frame_height                  Heigth of the animation frame
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_load_animation              ( LiteAnimation *animation,
                                             const char    *filename,
                                             int            still_frame,
                                             int            frame_width,
                                             int            frame_height );

/**
 * @brief Start the animation sequence.
 *
 * This function will start the animation with a specified
 * timeout between frames.
 *
 * @param[in]  animation                     Valid LiteAnimation object
 * @param[in]  timeout                       Timeout in milliseconds
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_start_animation             ( LiteAnimation *animation,
                                             unsigned int   timeout );

/**
 * @brief Update animation.
 *
 * This function will update the animation.
 *
 * @param[in]  animation                     Valid LiteAnimation object
 *
 * @return 1 if the animation has been updated,
 *         0 otherwise.
 */
int lite_update_animation                  ( LiteAnimation *animation );

/**
 * @brief Stop the animation sequence.
 *
 * This function will stop the animation.
 *
 * @param[in]  animation                     Valid LiteAnimation object
 *
 * @return DFB_OK if succcessful.
 */
DFBResult lite_stop_animation              ( LiteAnimation *animation );

/**
 * @brief Check if the animation sequence is running or not.
 *
 * This function will check if the animation is running or not.
 *
 * @param[in]  animation                     Valid LiteAnimation object
 *
 * @return 1 if animation is running,
 *         0 otherwise.
 */
int lite_animation_running                 ( LiteAnimation *animation );

#ifdef __cplusplus
}
#endif

#endif
