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
 * @mainpage
 * LiTE stands for Lightweight Toolkit Enabler and is a simple user interface library on top of DirectFB.
 * Widgets can be used with a default theme, or with its own custom theme.
 */

/**
 * @brief This file contains definitions for the LiTE general interface.
 * @file lite.h
 */

#ifndef __LITE__LITE_H__
#define __LITE__LITE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <directfb.h>

/**
 * @brief Start the LiTE framework.
 *
 * This function will start the LiTE framework by creating the
 * underlying DirectFB resources.
 *
 * @param argc                               The argc value passed from main
 * @param argv                               The argv values passed from main
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_open                        ( int   *argc,
                                             char **argv[] );

/**
 * @brief Close the LiTE framework.
 *
 * This function will close the LiTE framework by releasing all
 * DirectFB resources created during the lite_open() sequence.
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_close                       ( void );

/**
 * @brief Get the underlying IDirectFB interface.
 *
 * This function will return the IDirectFB interface
 * that was created during the lite_open() sequence.
 *
 * @return a pointer to the main DirectFB interface.
 */
IDirectFB *
lite_get_dfb_interface                     ( void );

/**
 * @brief Get the underlying IDirectFBDisplayLayer interface.
 *
 * This function will return the IDirectFBDisplayLayer interface
 * that was created during the lite_open() sequence.
 *
 * @return a pointer to the DirectFB display layer interface.
 */
IDirectFBDisplayLayer *
lite_get_layer_interface                   ( void );

/**
 * @brief Get the display layer size.
 *
 * This function will retrieve the size of the display layer.
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_layer_size              ( int *ret_width,
                                             int *ret_height );

#ifdef __cplusplus
}
#endif

#endif
