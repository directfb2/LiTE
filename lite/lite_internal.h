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

#ifndef __LITE__LITE_INTERNAL_H__
#define __LITE__LITE_INTERNAL_H__

#include <directfb.h>

/* test for NULL parameter and return DFB_INVARG */
#define LITE_NULL_PARAMETER_CHECK(exp) \
    do {                               \
        if ((exp) == NULL) {           \
            return DFB_INVARG;         \
        }                              \
    } while (0)

/* pointer to the main interface */
extern IDirectFB *lite_dfb;

/* pointer to the display layer interface */
extern IDirectFBDisplayLayer *lite_layer;

/* font styles */
extern char *lite_font_styles[4];

/* clean up resources allocated for window usage on app shutdown */
DFBResult prvlite_release_window_resources ( void );

/* clean up resources allocated for font usage on app shutdown */
DFBResult prvlite_release_font_resources   ( void );

/* truncate text */
void      prvlite_make_truncated_text      ( char          *text,
                                             int            width,
                                             IDirectFBFont *font );

/* load an image */
DFBResult prvlite_load_image               ( const char           *filename,
                                             IDirectFBSurface    **ret_surface,
                                             int                  *ret_width,
                                             int                  *ret_height,
                                             DFBImageDescription  *ret_desc );

#endif
