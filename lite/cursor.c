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

#include <lite/cursor.h>
#include <lite/lite_internal.h>

D_DEBUG_DOMAIN( LiteCursorDomain, "LiTE/Cursor", "LiTE Cursor" );

/**********************************************************************************************************************/

static LiteCursor *cursor_global         = NULL;
static u8          cursor_opacity_global = 255;

DFBResult
lite_get_current_cursor( LiteCursor **ret_cursor )
{
     LITE_NULL_PARAMETER_CHECK( ret_cursor );

     D_DEBUG_AT( LiteCursorDomain, "Get current cursor: %p\n", cursor_global );

     *ret_cursor = cursor_global;

     return DFB_OK;
}

DFBResult
lite_set_current_cursor( LiteCursor *cursor )
{
     LITE_NULL_PARAMETER_CHECK( cursor );

     D_DEBUG_AT( LiteCursorDomain, "Set current cursor: %p\n", cursor );

     cursor_global = cursor;

     return DFB_OK;
}

DFBResult
lite_load_cursor_from_file( LiteCursor *cursor,
                            const char *cursor_path )
{
     DFBResult ret;
     int       width;
     int       height;

     LITE_NULL_PARAMETER_CHECK( cursor );
     LITE_NULL_PARAMETER_CHECK( cursor_path );

     D_DEBUG_AT( LiteCursorDomain, "Load cursor: %p from file: %s\n", cursor, cursor_path );

     ret = prvlite_load_image( cursor_path, &cursor->surface, &width, &height, NULL );
     if (ret != DFB_OK)
          return ret;

     cursor->height = height;
     cursor->width  = width;
     cursor->hot_x  = 0;
     cursor->hot_y  = 0;

     return ret;
}

DFBResult
lite_free_cursor( LiteCursor *cursor )
{
     LITE_NULL_PARAMETER_CHECK( cursor );

     D_DEBUG_AT( LiteCursorDomain, "Free cursor: %p\n", cursor );

     if (cursor_global == cursor)
          cursor_global = NULL;

     if (cursor->surface)
          cursor->surface->Release( cursor->surface );

     return DFB_OK;
}

DFBResult
lite_set_window_cursor( LiteWindow *window,
                        LiteCursor *cursor )
{
     DFBResult ret;

     LITE_NULL_PARAMETER_CHECK( window );
     LITE_NULL_PARAMETER_CHECK( cursor );

     D_DEBUG_AT( LiteCursorDomain, "Set cursor: %p for window: %p\n", cursor, window );

     ret = window->window->SetCursorFlags( window->window, cursor->surface ? DWCF_NONE : DWCF_INVISIBLE );
     if (ret) {
          DirectFBError( "LiTE/Cursor: SetCursorFlags() failed", ret );
          return ret;
     }

     ret = window->window->SetCursorShape( window->window, cursor->surface, cursor->hot_x, cursor->hot_y );
     if (ret) {
          DirectFBError( "LiTE/Cursor: SetCursorShape() failed", ret );
          return ret;
     }

     return ret;
}

DFBResult
lite_hide_cursor()
{
     D_DEBUG_AT( LiteCursorDomain, "Hide cursor\n" );

     return lite_change_cursor_opacity( 0 );
}

DFBResult
lite_show_cursor()
{
     D_DEBUG_AT( LiteCursorDomain, "Show cursor\n" );

     return lite_change_cursor_opacity( 255 );
}

DFBResult
lite_change_cursor_opacity( u8 opacity )
{
     DFBResult ret;

     D_DEBUG_AT( LiteCursorDomain, "Change cursor opacity to: %d\n", opacity );

     ret = lite_layer->SetCooperativeLevel( lite_layer, DLSCL_ADMINISTRATIVE );
     if (ret) {
          DirectFBError( "LiTE/Cursor: SetCooperativeLevel() failed", ret );
          return ret;
     }

     ret = lite_layer->SetCursorOpacity( lite_layer, opacity );
     if (ret) {
          DirectFBError( "LiTE/Cursor: SetCursorOpacity() failed", ret );
          return ret;
     }

     ret = lite_layer->SetCooperativeLevel( lite_layer, DLSCL_SHARED );
     if (ret) {
          DirectFBError( "LiTE/Cursor: SetCooperativeLevel() failed", ret );
          return ret;
     }

     cursor_opacity_global = opacity;

     return ret;
}

DFBResult
lite_get_cursor_opacity( u8 *ret_opacity )
{
     LITE_NULL_PARAMETER_CHECK( ret_opacity );

     D_DEBUG_AT( LiteCursorDomain, "Get cursor opacity: %d\n", cursor_opacity_global );

     *ret_opacity = cursor_opacity_global;

     return DFB_OK;
}

DFBResult
lite_set_cursor_hotspot( LiteCursor   *cursor,
                         unsigned int  hot_x,
                         unsigned int  hot_y )
{
     LITE_NULL_PARAMETER_CHECK( cursor );

     D_DEBUG_AT( LiteCursorDomain, "Set cursor: %p with hotspot: %u,%u\n", cursor, hot_x, hot_y );

     cursor->hot_x = hot_x;
     cursor->hot_y = hot_y;

     return DFB_OK;
}
