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

#include <direct/filesystem.h>
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

D_DEBUG_DOMAIN( LiteCoreDomain, "LiTE/Core", "LiTE Core" );

/**********************************************************************************************************************/

IDirectFB             *lite_dfb   = NULL;
IDirectFBDisplayLayer *lite_layer = NULL;

static LiteCursor lite_cursor = { NULL, 0, 0 };
static int        lite_refs   = 0;

/**********************************************************************************************************************/

static char *
get_image_path( const char *name )
{
     DirectResult  ret;
     int           len;
     char         *path;

     len  = strlen( DATADIR ) + 1 + strlen( name ) + 6 + 1;
     path = alloca( len );

     /* try to find an image in DFIFF format */
     snprintf( path, len, DATADIR"/%s.dfiff", name );
     ret = direct_access( path, R_OK );

     if (ret) {
         /* otherwise fall back on an image in PNG format */
         snprintf( path, len, DATADIR"/%s.png", name );
         ret = direct_access( path, R_OK );
     }

     return ret ? NULL : D_STRDUP( path );
}

DFBResult
lite_open( int   *argc,
           char **argv[] )
{
     DFBResult ret;

     if (!lite_refs) {
          int         i;
          const char *image_paths[8];

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

          /* default window theme */

          if (!getenv( "LITE_NO_FRAME" )) {
               DFBColor bg_color;

               bg_color.r = DEFAULT_WINDOW_COLOR_R;
               bg_color.g = DEFAULT_WINDOW_COLOR_G;
               bg_color.b = DEFAULT_WINDOW_COLOR_B;
               bg_color.a = DEFAULT_WINDOW_COLOR_A;

               image_paths[0] = get_image_path( DEFAULT_WINDOW_TOP_FRAME );
               image_paths[1] = get_image_path( DEFAULT_WINDOW_BOTTOM_FRAME );
               image_paths[2] = get_image_path( DEFAULT_WINDOW_LEFT_FRAME );
               image_paths[3] = get_image_path( DEFAULT_WINDOW_RIGHT_FRAME );
               image_paths[4] = get_image_path( DEFAULT_WINDOW_TOP_LEFT_FRAME );
               image_paths[5] = get_image_path( DEFAULT_WINDOW_TOP_RIGHT_FRAME );
               image_paths[6] = get_image_path( DEFAULT_WINDOW_BOTTOM_LEFT_FRAME );
               image_paths[7] = get_image_path( DEFAULT_WINDOW_BOTTOM_RIGHT_FRAME );

               ret = lite_new_window_theme( &bg_color, image_paths, &liteDefaultWindowTheme );
               if (ret != DFB_OK)
                    goto error;

               for (i = 0; i < LITE_THEME_FRAME_PART_NUM; i++)
                    D_FREE( (char*) image_paths[i] );
          }

          /* default button theme */

          image_paths[0] = get_image_path( DEFAULT_BUTTON_IMAGE_NORMAL );
          image_paths[1] = get_image_path( DEFAULT_BUTTON_IMAGE_PRESSED );
          image_paths[2] = get_image_path( DEFAULT_BUTTON_IMAGE_HILITE );
          image_paths[3] = get_image_path( DEFAULT_BUTTON_IMAGE_DISABLED );
          image_paths[4] = get_image_path( DEFAULT_BUTTON_IMAGE_HILITE_ON );
          image_paths[5] = get_image_path( DEFAULT_BUTTON_IMAGE_DISABLED_ON );
          image_paths[6] = get_image_path( DEFAULT_BUTTON_IMAGE_NORMAL_ON );

          ret = lite_new_button_theme( image_paths, &liteDefaultButtonTheme );
          if (ret != DFB_OK)
               goto error;

          for (i = 0; i < LITE_BS_MAX; i++)
               D_FREE( (char*) image_paths[i] );

          /* default check theme */

          image_paths[0] = get_image_path( DEFAULT_CHECK_IMAGE );

          ret = lite_new_check_theme( image_paths[0], &liteDefaultCheckTheme );
          if (ret != DFB_OK)
               goto error;

          D_FREE( (char*) image_paths[0] );

          /* default list theme */

          image_paths[0] = get_image_path( DEFAULT_SCROLLBAR_IMAGE );

          ret = lite_new_list_theme( image_paths[0], 3, &liteDefaultListTheme );
          if (ret != DFB_OK)
               goto error;

          D_FREE( (char*) image_paths[0] );

          /* default progress bar theme */

          image_paths[0] = get_image_path( DEFAULT_PROGRESSBAR_IMAGE_FG );
          image_paths[1] = get_image_path( DEFAULT_PROGRESSBAR_IMAGE_BG );

          ret = lite_new_progressbar_theme( image_paths[0], image_paths[1], &liteDefaultProgressBarTheme );
          if (ret != DFB_OK)
               goto error;

          D_FREE( (char*) image_paths[0] );
          D_FREE( (char*) image_paths[1] );

          /* default scrollbar theme */

          image_paths[0] = get_image_path( DEFAULT_SCROLLBAR_IMAGE );

          ret = lite_new_scrollbar_theme( image_paths[0], 3, &liteDefaultScrollbarTheme );
          if (ret != DFB_OK)
               goto error;

          D_FREE( (char*) image_paths[0] );

          /* default text button theme */

          image_paths[0] = get_image_path( DEFAULT_TEXTBUTTON_IMAGE );

          ret = lite_new_text_button_theme( image_paths[0], &liteDefaultTextButtonTheme );
          if (ret != DFB_OK)
               goto error;

          D_FREE( (char*) image_paths[0] );

          /* default cursor */

          if (!getenv( "LITE_NO_CURSOR" )) {
               image_paths[0] = get_image_path( DEFAULT_WINDOW_CURSOR );

               ret = lite_load_cursor_from_file( &lite_cursor, image_paths[0] );
               if (ret != DFB_OK)
                    goto error;

               D_FREE( (char*) image_paths[0] );

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
