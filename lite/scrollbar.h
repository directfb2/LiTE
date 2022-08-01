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
 * @brief This file contains definitions for the LiTE scrollbar interface.
 * @file scrollbar.h
 */

#ifndef __LITE__SCROLLBAR_H__
#define __LITE__SCROLLBAR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/box.h>
#include <lite/theme.h>

/** @brief Macro to convert a generic LiteBox into a LiteScrollbar. */
#define LITE_SCROLLBAR(l) ((LiteScrollbar*) (l))

/** @brief LiteScrollInfo structure. */
typedef struct {
     unsigned int min;                    /**< Minimum range */
     unsigned int max;                    /**< Maximum range */
     unsigned int page_size;              /**< Page size */
     unsigned int line_size;              /**< Line size */
     int          pos;                    /**< Scroll position, does not change while dragging */
     int          track_pos;              /**< Tracking position, -1 while not dragging */
} LiteScrollInfo;

/** @brief LiteScrollbar theme. */
typedef struct {
     LiteTheme              theme;        /**< Base LiTE theme */

     int                    image_margin; /**< Thumb image pixel margin */
     struct {
          IDirectFBSurface *surface;
          int               width;
          int               height;
     } all_images;                        /**< All scrollbar images (button1, button2, thumb) */
} LiteScrollbarTheme;

/** @brief No scrollbar theme. */
#define liteNoScrollbarTheme NULL

/** @brief Default scrollbar theme. */
extern LiteScrollbarTheme *liteDefaultScrollbarTheme;

/** @brief LiteScrollbar structure. */
typedef struct _LiteScrollbar LiteScrollbar;

/** @brief Callback function prototype for scrollbar updates. */
typedef void (*LiteScrollbarUpdateFunc)( LiteScrollbar *scrollbar, LiteScrollInfo *info, void *data );

/**
 * @brief Create a new LiteScrollbar object.
 *
 * This function will create a new LiteScrollbar object.
 *
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  rect                          Rectangle for the LiteScrollbar object
 * @param[in]  vertical                      1 if vertical, 0 if horizontal
 * @param[in]  theme                         Scrollbar theme
 * @param[out] ret_scrollbar                 Valid LiteScrollbar object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_scrollbar               ( LiteBox             *parent,
                                             DFBRectangle        *rect,
                                             int                  vertical,
                                             LiteScrollbarTheme  *theme,
                                             LiteScrollbar      **ret_scrollbar );

/**
 * @brief Enable/disable scrollbar.
 *
 * This function will enable or disable the scrollbar.
 *
 * @param[in]  scrollbar                     Valid LiteScrollbar object
 * @param[in]  enabled                       Enabled or not
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_enable_scrollbar            ( LiteScrollbar *scrollbar,
                                             int            enabled );

/**
 * @brief Get scrollbar thickness.
 *
 * This function will retrieve the thickness of the scrollbar.
 *
 * @param[in]  scrollbar                     Valid LiteScrollbar object
 * @param[in]  ret_thickness                 Scrollbar thickness
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_scrollbar_thickness     ( LiteScrollbar *scrollbar,
                                             int           *ret_thickness );

/**
 * @brief Set the current scroll position.
 *
 * This function will set the scroll position.
 *
 * @param[in]  scrollbar                     Valid LiteScrollbar object
 * @param[in]  pos                           Scroll position
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_scroll_pos              ( LiteScrollbar *scrollbar,
                                             int            pos );

/**
 * @brief Get scroll/tracking position.
 *
 * This function will get the current scroll position or the
 * tracking position if dragging.
 *
 * @param[in]  scrollbar                     Valid LiteScrollbar object
 * @param[out] ret_pos                       Scroll position, tracking position if dragging
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_scroll_pos              ( LiteScrollbar *scrollbar,
                                             int           *ret_pos );

/**
 * @brief Set the current scroll information.
 *
 * This function will set the scroll information.
 *
 * @param[in]  scrollbar                     Valid LiteScrollbar object
 * @param[in]  info                          Valid LiteScrollInfo object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_scroll_info             ( LiteScrollbar  *scrollbar,
                                             LiteScrollInfo *info );

/**
 * @brief Get scroll information.
 *
 * This function will get the current scroll information.
 *
 * @param[in]  scrollbar                     Valid LiteScrollbar object
 * @param[out] ret_info                      Valid LiteScrollInfo object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_scroll_info             ( LiteScrollbar  *scrollbar,
                                             LiteScrollInfo *ret_info );

/**
 * @brief Set all scrollbar images.
 *
 * This function will set the images of a scrollbar.
 *
 * @param[in]  scrollbar                     Valid LiteScrollbar object
 * @param[in]  image_path                    File path with image for all scrollbar subsections (button1, button2, thumb)
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_scrollbar_all_images    ( LiteScrollbar *scrollbar,
                                             const char    *image_path );

/**
 * @brief Set the margin in pixels for the thumb image.
 *
 * This function will set the margin in pixels for the thumb image.
 *
 * @param[in]  scrollbar                     Valid LiteScrollbar object
 * @param[in]  image_margin                  Thumb image pixel margin
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_scrollbar_image_margin  ( LiteScrollbar *scrollbar,
                                             int            image_margin );

/**
 * @brief Install a callback function for scrollbar updates.
 *
 * This function will install a callback triggered every time the
 * thumb position is changed or dragged.
 *
 * @param[in]  scrollbar                     Valid LiteScrollbar object
 * @param[in]  callback                      Callback function
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_scrollbar_update         ( LiteScrollbar           *scrollbar,
                                             LiteScrollbarUpdateFunc  callback,
                                             void                    *data );

/**
 * @brief Create a scrollbar theme.
 *
 * This function makes the theme.
 *
 * @param[in]  image_path                    File path with image for all scrollbar subsections (button1, button2, thumb)
 * @param[in]  image_margin                  Thumb subsection pixel margin
 * @param[out] ret_theme                     New theme
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_scrollbar_theme         ( const char          *image_path,
                                             int                  image_margin,
                                             LiteScrollbarTheme **ret_theme );

/**
 * @brief Destroy a scrollbar theme.
 *
 * This function will release the theme resources.
 *
 * @param[in]  theme                         Theme to destroy
 *
 * @return DFB_OK If successful.
 */
DFBResult lite_destroy_scrollbar_theme     ( LiteScrollbarTheme *theme );

#ifdef __cplusplus
}
#endif

#endif
