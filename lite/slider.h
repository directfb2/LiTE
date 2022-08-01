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
 * @brief This file contains definitions for the LiTE slider interface.
 * @file slider.h
 */

#ifndef __LITE__SLIDER_H__
#define __LITE__SLIDER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/box.h>
#include <lite/theme.h>

/** @brief Macro to convert a generic LiteBox into a LiteSlider. */
#define LITE_SLIDER(l) ((LiteSlider*) (l))

/** @brief LiteSlider theme. */
typedef struct {
     LiteTheme theme; /**< Base LiTE theme */
} LiteSliderTheme;

/** @brief No slider theme. */
#define liteNoSliderTheme NULL

/** @brief Default slider theme. */
extern LiteSliderTheme *liteDefaultSliderTheme;

/** @brief LiteSlider structure. */
typedef struct _LiteSlider LiteSlider;

/** @brief Callback function prototype for slider updates. */
typedef void (*LiteSliderUpdateFunc)( LiteSlider *slider, float pos, void *data );

/**
 * @brief Create a new LiteSlider object.
 *
 * This function will create a new LiteSlider object.
 *
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  rect                          Rectangle for the LiteSlider object
 * @param[in]  theme                         Slider theme
 * @param[out] ret_slider                    Valid LiteSlider object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_slider                  ( LiteBox          *parent,
                                             DFBRectangle     *rect,
                                             LiteSliderTheme  *theme,
                                             LiteSlider      **ret_slider );

/**
 * @brief Set the current indicator position.
 *
 * This function will set the position indicator.
 *
 * @param[in]  slider                        Valid LiteSlider object
 * @param[in]  pos                           Indicator position between 0 and 1
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_slider_pos              ( LiteSlider *slider,
                                             float       pos );

/**
 * @brief Install a callback function for slider updates.
 *
 * This function will install a callback triggered every time the
 * indicator position is changed.
 *
 * @param[in]  slider                        Valid LiteSlider object
 * @param[in]  callback                      Callback function
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_slider_update            ( LiteSlider           *slider,
                                             LiteSliderUpdateFunc  callback,
                                             void                 *data );

/**
 * @brief Create a slider theme.
 *
 * This function makes the theme.
 *
 * @param[in]  bg_color                      Background color
 * @param[in]  fg_color                      Foreground color
 * @param[out] ret_theme                     New theme
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_slider_theme            ( const DFBColor   *bg_color,
                                             const DFBColor   *fg_color,
                                             LiteSliderTheme **ret_theme );

/**
 * @brief Destroy a slider theme.
 *
 * This function will release the theme resources.
 *
 * @param[in]  theme                         Theme to destroy

 * @return DFB_OK If successful.
 */
DFBResult lite_destroy_slider_theme        ( LiteSliderTheme *theme );

#ifdef __cplusplus
}
#endif

#endif
