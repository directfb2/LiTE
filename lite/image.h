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
 * @brief This file contains definitions for the LiTE image interface.
 * @file image.h
 */

#ifndef __LITE__IMAGE_H__
#define __LITE__IMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/box.h>
#include <lite/theme.h>

/** @brief Macro to convert a generic LiteBox into a LiteImage. */
#define LITE_IMAGE(l) ((LiteImage*) (l))

/** @brief LiteImage theme. */
typedef struct {
     LiteTheme theme; /**< Base LiTE theme */
} LiteImageTheme;

/** @brief No image theme. */
#define liteNoImageTheme NULL

/** @brief Default image theme. */
extern LiteImageTheme *liteDefaultImageTheme;

/** @brief LiteImage structure. */
typedef struct _LiteImage LiteImage;

/**
 * @brief Create a new LiteImage object.
 *
 * This function will create a new LiteImage object.
 *
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  rect                          Rectangle for the LiteImage object
 * @param[in]  theme                         Image theme
 * @param[out] ret_image                     Valid LiteImage object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_image                   ( LiteBox         *parent,
                                             DFBRectangle    *rect,
                                             LiteImageTheme  *theme,
                                             LiteImage      **ret_image );

/**
 * @brief Load an image.
 *
 * This function will load an image.
 *
 * @param[in]  image                         Valid LiteImage object
 * @param[in]  filename                      File path with an image
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_load_image                  ( LiteImage  *image,
                                             const char *filename );

/**
 * @brief Load an image from memory.
 *
 * This function will load an image from memory.
 *
 * @param[in]  image                         Valid LiteImage object
 * @param[in]  data                          Data with an image
 * @param[in]  length                        Length of buffer
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_load_image_data             ( LiteImage    *image,
                                             const void   *data,
                                             unsigned int  length );

/**
 * @brief Set the image clipping area.
 *
 * This function will set the image clipping area for the blitting
 * operation. If not specified, the image will be stretch blitted
 * to its destination.
 *
 * @param[in]  image                         Valid LiteImage object
 * @param[in]  rect                          Rectangle for image source
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_image_clipping          ( LiteImage          *image,
                                             const DFBRectangle *rect );

/**
 * @brief Get the image description.
 *
 * This function will retrieve the description of the image.
 *
 * @param[in]  image                         Valid LiteImage object
 * @param[out] ret_desc                      Image description
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_image_description       ( LiteImage           *image,
                                             DFBImageDescription *ret_desc );

/**
 * @brief Get image size.
 *
 * This function will retrieve the size of the image.
 *
 * @param[in]  image                         Valid LiteImage object
 * @param[out] ret_width                     Image width
 * @param[out] ret_height                    Image height
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_image_size              ( LiteImage *image,
                                             int       *ret_width,
                                             int       *ret_height );

#ifdef __cplusplus
}
#endif

#endif
