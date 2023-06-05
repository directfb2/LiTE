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

#include <lite/button.h>
#include <lite/check.h>
#include <lite/cursor.h>
#include <lite/list.h>
#include <lite/lite.h>
#include <lite/lite_config.h>
#include <lite/lite_internal.h>
#include <lite/progressbar.h>
#include <lite/scrollbar.h>
#include <lite/textbutton.h>

#ifdef LITEIMAGEDIR
#include <direct/filesystem.h>
#else
#include "bottom.h"
#include "bottomleft.h"
#include "bottomright.h"
#include "button_normal.h"
#include "button_pressed.h"
#include "button_hilite.h"
#include "button_disabled.h"
#include "button_hilite_on.h"
#include "button_disabled_on.h"
#include "button_normal_on.h"
#include "checkbox.h"
#include "left.h"
#include "progressbar_bg.h"
#include "progressbar_fg.h"
#include "right.h"
#include "scrollbarbox.h"
#include "textbuttonbox.h"
#include "top.h"
#include "topleft.h"
#include "topright.h"
#include "wincursor.h"
#endif

D_DEBUG_DOMAIN( LiteCoreDomain, "LiTE/Core", "LiTE Core" );

/**********************************************************************************************************************/

IDirectFB             *lite_dfb   = NULL;
IDirectFBDisplayLayer *lite_layer = NULL;

static LiteCursor lite_cursor = { NULL, 0, 0 };
static int        lite_refs   = 0;

/**********************************************************************************************************************/

#ifdef LITEIMAGEDIR
static char *
get_image_path( const char *name )
{
     DirectResult  ret = DFB_FAILURE;
     int           len;
     char         *path;

     D_ASSERT( name != NULL );

     len  = strlen( LITEIMAGEDIR ) + 1 + strlen( name ) + 6 + 1;
     path = alloca( len );

     if (!getenv( "LITE_NO_DFIFF" )) {
          /* first try to find an image in DFIFF format */
          snprintf( path, len, LITEIMAGEDIR"/%s.dfiff", name );
          ret = direct_access( path, R_OK );
     }

     if (ret) {
          /* otherwise fall back on an image in PNG format */
          snprintf( path, len, LITEIMAGEDIR"/%s.png", name );
          ret = direct_access( path, R_OK );
     }

     return ret ? NULL : D_STRDUP( path );
}
#endif

DFBResult
lite_open( int   *argc,
           char **argv[] )
{
     DFBResult ret;

     if (!lite_refs) {
          int           i;
          const void   *file_data[8];
          unsigned int  length[8];

          D_DEBUG_AT( LiteCoreDomain, "Open new LiTE instance...\n" );

          ret = DirectFBInit( argc, argv );
          if (ret) {
               DirectFBError( "LiTE/Core: DirectFBInit() failed", ret );
               return ret;
          }

          ret = DirectFBCreate( &lite_dfb );
          if (ret) {
               DirectFBError( "LiTE/Core: DirectFBCreate() failed", ret );
               goto error;
          }

          ret = lite_dfb->GetDisplayLayer( lite_dfb, DLID_PRIMARY, &lite_layer );
          if (ret) {
               DirectFBError( "LiTE/Core: GetDisplayLayer() failed", ret );
               goto error;
          }

          for (i = 0; i < D_ARRAY_SIZE(length); i++)
               length[i] = 0;

          /* default window theme */

          if (!getenv( "LITE_NO_FRAME" )) {
               DFBColor bg_color;

               bg_color.r = DEFAULT_WINDOW_COLOR_R;
               bg_color.g = DEFAULT_WINDOW_COLOR_G;
               bg_color.b = DEFAULT_WINDOW_COLOR_B;
               bg_color.a = DEFAULT_WINDOW_COLOR_A;

#ifdef LITEIMAGEDIR
               file_data[0] = get_image_path( DEFAULT_WINDOW_TOP_FRAME );
               file_data[1] = get_image_path( DEFAULT_WINDOW_BOTTOM_FRAME );
               file_data[2] = get_image_path( DEFAULT_WINDOW_LEFT_FRAME );
               file_data[3] = get_image_path( DEFAULT_WINDOW_RIGHT_FRAME );
               file_data[4] = get_image_path( DEFAULT_WINDOW_TOP_LEFT_FRAME );
               file_data[5] = get_image_path( DEFAULT_WINDOW_TOP_RIGHT_FRAME );
               file_data[6] = get_image_path( DEFAULT_WINDOW_BOTTOM_LEFT_FRAME );
               file_data[7] = get_image_path( DEFAULT_WINDOW_BOTTOM_RIGHT_FRAME );
#else
               file_data[0] = top_data;
               length[0]    = sizeof(top_data);
               file_data[1] = bottom_data;
               length[1]    = sizeof(bottom_data);
               file_data[2] = left_data;
               length[2]    = sizeof(left_data);
               file_data[3] = right_data;
               length[3]    = sizeof(right_data);
               file_data[4] = topleft_data;
               length[4]    = sizeof(topleft_data);
               file_data[5] = topright_data;
               length[5]    = sizeof(topright_data);
               file_data[6] = bottomleft_data;
               length[6]    = sizeof(bottomleft_data);
               file_data[7] = bottomright_data;
               length[7]    = sizeof(bottomright_data);
#endif

               ret = lite_new_window_theme( &bg_color,
                                            DEFAULT_WINDOW_TITLE_FONT, LITE_FONT_PLAIN, 16, DEFAULT_FONT_ATTRIBUTE,
                                            file_data, length, &liteDefaultWindowTheme );

#ifdef LITEIMAGEDIR
               for (i = 0; i < LITE_THEME_FRAME_PART_NUM; i++)
                    D_FREE( (void*) file_data[i] );
#endif

               if (ret != DFB_OK)
                    goto error;
          }

          /* default button theme */

#ifdef LITEIMAGEDIR
          file_data[0] = get_image_path( DEFAULT_BUTTON_IMAGE_NORMAL );
          file_data[1] = get_image_path( DEFAULT_BUTTON_IMAGE_PRESSED );
          file_data[2] = get_image_path( DEFAULT_BUTTON_IMAGE_HILITE );
          file_data[3] = get_image_path( DEFAULT_BUTTON_IMAGE_DISABLED );
          file_data[4] = get_image_path( DEFAULT_BUTTON_IMAGE_HILITE_ON );
          file_data[5] = get_image_path( DEFAULT_BUTTON_IMAGE_DISABLED_ON );
          file_data[6] = get_image_path( DEFAULT_BUTTON_IMAGE_NORMAL_ON );
#else
          file_data[0] = button_normal_data;
          length[0]    = sizeof(button_normal_data);
          file_data[1] = button_pressed_data;
          length[1]    = sizeof(button_pressed_data);
          file_data[2] = button_hilite_data;
          length[2]    = sizeof(button_hilite_data);
          file_data[3] = button_disabled_data;
          length[3]    = sizeof(button_disabled_data);
          file_data[4] = button_hilite_on_data;
          length[4]    = sizeof(button_hilite_on_data);
          file_data[5] = button_disabled_on_data;
          length[5]    = sizeof(button_disabled_on_data);
          file_data[6] = button_normal_on_data;
          length[6]    = sizeof(button_normal_on_data);
#endif

          ret = lite_new_button_theme( file_data, length, &liteDefaultButtonTheme );

#ifdef LITEIMAGEDIR
          for (i = 0; i < LITE_BS_MAX; i++)
               D_FREE( (void*) file_data[i] );
#endif

          if (ret != DFB_OK)
               goto error;

          /* default check theme */

#ifdef LITEIMAGEDIR
          file_data[0] = get_image_path( DEFAULT_CHECKBOX_IMAGE );
#else
          file_data[0] = checkbox_data;
          length[0]    = sizeof(checkbox_data);
#endif

          ret = lite_new_check_theme( file_data[0], length[0], &liteDefaultCheckTheme );

#ifdef LITEIMAGEDIR
          D_FREE( (void*) file_data[0] );
#endif

          if (ret != DFB_OK)
               goto error;

          /* default list theme */

#ifdef LITEIMAGEDIR
          file_data[0] = get_image_path( DEFAULT_SCROLLBARBOX_IMAGE );
#else
          file_data[0] = scrollbarbox_data;
          length[0]    = sizeof(scrollbarbox_data);
#endif

          ret = lite_new_list_theme( file_data[0], length[0], 3, &liteDefaultListTheme );

#ifdef LITEIMAGEDIR
          D_FREE( (void*) file_data[0] );
#endif

          if (ret != DFB_OK)
               goto error;

          /* default progress bar theme */

#ifdef LITEIMAGEDIR
          file_data[0] = get_image_path( DEFAULT_PROGRESSBAR_IMAGE_FG );
          file_data[1] = get_image_path( DEFAULT_PROGRESSBAR_IMAGE_BG );
#else
          file_data[0] = progressbar_fg_data;
          length[0]    = sizeof(progressbar_fg_data);
          file_data[1] = progressbar_bg_data;
          length[1]    = sizeof(progressbar_bg_data);
#endif

          ret = lite_new_progressbar_theme( file_data[0], length[0], file_data[1], length[1],
                                            &liteDefaultProgressBarTheme );

#ifdef LITEIMAGEDIR
          D_FREE( (void*) file_data[0] );
          D_FREE( (void*) file_data[1] );
#endif

          if (ret != DFB_OK)
               goto error;

          /* default scrollbar theme */

#ifdef LITEIMAGEDIR
          file_data[0] = get_image_path( DEFAULT_SCROLLBARBOX_IMAGE );
#else
          file_data[0] = scrollbarbox_data;
          length[0]    = sizeof(scrollbarbox_data);
#endif

          ret = lite_new_scrollbar_theme( file_data[0], length[0], 3, &liteDefaultScrollbarTheme );

#ifdef LITEIMAGEDIR
          D_FREE( (void*) file_data[0] );
#endif

          if (ret != DFB_OK)
               goto error;

          /* default text button theme */

#ifdef LITEIMAGEDIR
          file_data[0] = get_image_path( DEFAULT_TEXTBUTTONBOX_IMAGE );
#else
          file_data[0] = textbuttonbox_data;
          length[0]    = sizeof(textbuttonbox_data);
#endif

          ret = lite_new_text_button_theme( file_data[0], length[0], &liteDefaultTextButtonTheme );

#ifdef LITEIMAGEDIR
          D_FREE( (void*) file_data[0] );
#endif

          if (ret != DFB_OK)
               goto error;

          /* default cursor */

          if (!getenv( "LITE_NO_CURSOR" )) {
#ifdef LITEIMAGEDIR
               file_data[0] = get_image_path( DEFAULT_WINDOW_CURSOR );
#else
               file_data[0] = wincursor_data;
               length[0]    = sizeof(wincursor_data);
#endif

               ret = lite_load_cursor( &lite_cursor, file_data[0], length[0] );

#ifdef LITEIMAGEDIR
               D_FREE( (void*) file_data[0] );
#endif

               if (ret != DFB_OK)
                    goto error;

               ret = lite_set_current_cursor( &lite_cursor );
               if (ret != DFB_OK)
                    goto error;

               /* adjust a possible cursor hotspot position */
               ret = lite_set_cursor_hotspot( &lite_cursor,
                                              DEFAULT_WINDOW_CURSOR_HOTSPOT_X, DEFAULT_WINDOW_CURSOR_HOTSPOT_Y );
               if (ret != DFB_OK)
                    goto error;
          }
     }
     else {
          D_DEBUG_AT( LiteCoreDomain, "Another ref (%d) to existing LiTE instance...\n", lite_refs );
     }

     lite_refs++;

     return DFB_OK;

error:
     if (lite_cursor.surface) {
          lite_cursor.surface->Release( lite_cursor.surface );
          lite_cursor.surface = NULL;
     }

     if (liteDefaultTextButtonTheme)
          lite_destroy_text_button_theme( liteDefaultTextButtonTheme );

     if (liteDefaultScrollbarTheme)
          lite_destroy_scrollbar_theme( liteDefaultScrollbarTheme );

     if (liteDefaultProgressBarTheme)
          lite_destroy_progressbar_theme( liteDefaultProgressBarTheme );

     if (liteDefaultListTheme)
          lite_destroy_list_theme( liteDefaultListTheme );

     if (liteDefaultCheckTheme)
          lite_destroy_check_theme( liteDefaultCheckTheme );

     if (liteDefaultButtonTheme)
          lite_destroy_button_theme( liteDefaultButtonTheme );

     if (liteDefaultWindowTheme)
          lite_destroy_window_theme( liteDefaultWindowTheme );

     if (lite_layer) {
          lite_layer->Release( lite_layer );
          lite_layer = NULL;
     }

     if (lite_dfb) {
          lite_dfb->Release( lite_dfb );
          lite_dfb = NULL;
     }

     return ret;
}

DFBResult
lite_close()
{
     D_DEBUG_AT( LiteCoreDomain, "Close LiTE instance...\n" );

     if (lite_refs) {
          if (!--lite_refs) {
               D_DEBUG_AT( LiteCoreDomain, "Release DirectFB resources...\n" );

               if (lite_cursor.surface) {
                    lite_cursor.surface->Release( lite_cursor.surface );
                    lite_cursor.surface = NULL;
               }

               if (liteDefaultTextButtonTheme)
                    lite_destroy_text_button_theme( liteDefaultTextButtonTheme );

               if (liteDefaultScrollbarTheme)
                    lite_destroy_scrollbar_theme( liteDefaultScrollbarTheme );

               if (liteDefaultProgressBarTheme)
                    lite_destroy_progressbar_theme( liteDefaultProgressBarTheme );

               if (liteDefaultListTheme)
                    lite_destroy_list_theme( liteDefaultListTheme );

               if (liteDefaultCheckTheme)
                    lite_destroy_check_theme( liteDefaultCheckTheme );

               if (liteDefaultButtonTheme)
                    lite_destroy_button_theme( liteDefaultButtonTheme );

               if (liteDefaultWindowTheme)
                    lite_destroy_window_theme( liteDefaultWindowTheme );

               prvlite_release_window_resources();

               prvlite_release_font_resources();

               lite_layer->Release( lite_layer );
               lite_layer = NULL;

               lite_dfb->Release( lite_dfb );
               lite_dfb = NULL;
          }
     }

     return DFB_OK;
}

IDirectFB *
lite_get_dfb_interface()
{
     D_DEBUG_AT( LiteCoreDomain, "Get IDirectFB interface\n" );

     return lite_dfb;
}

IDirectFBDisplayLayer *
lite_get_layer_interface()
{
     D_DEBUG_AT( LiteCoreDomain, "Get IDirectFBDisplayLayer interface\n" );

     return lite_layer;
}

DFBResult
lite_get_layer_size( int *ret_width,
                     int *ret_height )
{
     DFBResult             ret;
     DFBDisplayLayerConfig config;

     D_DEBUG_AT( LiteCoreDomain, "Get display layer size\n" );

     if (!lite_layer)
          return DFB_DEAD;

     ret = lite_layer->GetConfiguration( lite_layer, &config );
     if (ret) {
          DirectFBError( "LiTE/Core: GetConfiguration() failed", ret );
          return ret;
     }

     if (ret_width)
          *ret_width = config.width;

     if (ret_height)
          *ret_height = config.height;

     D_DEBUG_AT( LiteCoreDomain, "  -> %dx%d\n", config.width, config.height );

     return DFB_OK;
}
