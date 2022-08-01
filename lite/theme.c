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

#include <lite/lite_internal.h>
#include <lite/theme.h>

D_DEBUG_DOMAIN( LiteThemeDomain, "LiTE/Theme", "LiTE Theme" );

/**********************************************************************************************************************/

DFBResult
lite_theme_frame_load( LiteThemeFrame  *frame,
                       const char     **filenames )
{
     DFBResult              ret;
     int                    i, y;
     DFBSurfaceDescription  dsc;
     IDirectFBSurface      *compact;
     int                    width  = 0;
     int                    height = 0;

     D_DEBUG_AT( LiteThemeDomain, "%s( %p, %p )\n", __FUNCTION__, frame, filenames );

     D_ASSERT( frame != NULL );
     D_ASSERT( filenames != NULL );

     for (i = 0; i < LITE_THEME_FRAME_PART_NUM; i++) {
          D_ASSERT( filenames[i] != NULL );

          ret = prvlite_load_image( filenames[i], &frame->parts[i].source,
                                    &frame->parts[i].rect.w, &frame->parts[i].rect.h, NULL );
          if (ret != DFB_OK) {
               while (i--)
                    frame->parts[i].source->Release( frame->parts[i].source );

               return ret;
          }

          if (width < frame->parts[i].rect.w)
               width = frame->parts[i].rect.w;

          height += frame->parts[i].rect.h;
     }

     dsc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
     dsc.width       = width;
     dsc.height      = height;
     dsc.pixelformat = DSPF_ARGB;

     lite_dfb->CreateSurface( lite_dfb, &dsc, &compact );

     compact->Clear( compact, 0, 0, 0, 0 );

     for (i = 0, y = 0; i < LITE_THEME_FRAME_PART_NUM; i++) {
          compact->Blit( compact, frame->parts[i].source, &frame->parts[i].rect, 0, y );

          frame->parts[i].source->Release( frame->parts[i].source );

          frame->parts[i].source = compact;

          compact->AddRef( compact );

          frame->parts[i].rect.x = 0;
          frame->parts[i].rect.y = y;

          y += frame->parts[i].rect.h;
     }

     compact->ReleaseSource( compact );
     compact->Release( compact );

     D_MAGIC_SET( frame, LiteThemeFrame );

     return DFB_OK;
}

void
lite_theme_frame_unload( LiteThemeFrame *frame )
{
     int i;

     D_DEBUG_AT( LiteThemeDomain, "%s( %p )\n", __FUNCTION__, frame );

     D_MAGIC_ASSERT( frame, LiteThemeFrame );

     for (i = 0; i < LITE_THEME_FRAME_PART_NUM; i++) {
          if (frame->parts[i].source)
               frame->parts[i].source->Release( frame->parts[i].source );
     }

     D_MAGIC_CLEAR( frame );
}

void
lite_theme_frame_target_update( DFBRectangle         *frame_target,
                                const LiteThemeFrame *frame,
                                const DFBDimension   *size )
{
     D_DEBUG_AT( LiteThemeDomain, "%s( %p, size %dx%d )\n", __FUNCTION__, frame, size->w, size->h );

     D_MAGIC_ASSERT( frame, LiteThemeFrame );

     /* top left */
     frame_target[LITE_THEME_FRAME_PART_TOPLEFT].x     = 0;
     frame_target[LITE_THEME_FRAME_PART_TOPLEFT].y     = 0;
     frame_target[LITE_THEME_FRAME_PART_TOPLEFT].w     = frame->parts[LITE_THEME_FRAME_PART_TOPLEFT].rect.w;
     frame_target[LITE_THEME_FRAME_PART_TOPLEFT].h     = frame->parts[LITE_THEME_FRAME_PART_TOPLEFT].rect.h;
     /* top right */
     frame_target[LITE_THEME_FRAME_PART_TOPRIGHT].x    = size->w - frame->parts[LITE_THEME_FRAME_PART_TOPRIGHT].rect.w;
     frame_target[LITE_THEME_FRAME_PART_TOPRIGHT].y    = 0;
     frame_target[LITE_THEME_FRAME_PART_TOPRIGHT].w    = frame->parts[LITE_THEME_FRAME_PART_TOPRIGHT].rect.w;
     frame_target[LITE_THEME_FRAME_PART_TOPRIGHT].h    = frame->parts[LITE_THEME_FRAME_PART_TOPRIGHT].rect.h;
     /* bottom left */
     frame_target[LITE_THEME_FRAME_PART_BOTTOMLEFT].x  = 0;
     frame_target[LITE_THEME_FRAME_PART_BOTTOMLEFT].y  = size->h - frame->parts[LITE_THEME_FRAME_PART_BOTTOMLEFT].rect.h;
     frame_target[LITE_THEME_FRAME_PART_BOTTOMLEFT].w  = frame->parts[LITE_THEME_FRAME_PART_BOTTOMLEFT].rect.w;
     frame_target[LITE_THEME_FRAME_PART_BOTTOMLEFT].h  = frame->parts[LITE_THEME_FRAME_PART_BOTTOMLEFT].rect.h;
     /* bottom right */
     frame_target[LITE_THEME_FRAME_PART_BOTTOMRIGHT].x = size->w - frame->parts[LITE_THEME_FRAME_PART_BOTTOMRIGHT].rect.w;
     frame_target[LITE_THEME_FRAME_PART_BOTTOMRIGHT].y = size->h - frame->parts[LITE_THEME_FRAME_PART_BOTTOMRIGHT].rect.h;
     frame_target[LITE_THEME_FRAME_PART_BOTTOMRIGHT].w = frame->parts[LITE_THEME_FRAME_PART_BOTTOMRIGHT].rect.w;
     frame_target[LITE_THEME_FRAME_PART_BOTTOMRIGHT].h = frame->parts[LITE_THEME_FRAME_PART_BOTTOMRIGHT].rect.h;
     /* top */
     frame_target[LITE_THEME_FRAME_PART_TOP].x         = frame_target[LITE_THEME_FRAME_PART_TOPLEFT].w;
     frame_target[LITE_THEME_FRAME_PART_TOP].y         = 0;
     frame_target[LITE_THEME_FRAME_PART_TOP].w         = frame_target[LITE_THEME_FRAME_PART_TOPRIGHT].x -
                                                         frame_target[LITE_THEME_FRAME_PART_TOP].x;
     frame_target[LITE_THEME_FRAME_PART_TOP].h         = frame->parts[LITE_THEME_FRAME_PART_TOP].rect.h;
     /* bottom */
     frame_target[LITE_THEME_FRAME_PART_BOTTOM].x      = frame_target[LITE_THEME_FRAME_PART_BOTTOMLEFT].w;
     frame_target[LITE_THEME_FRAME_PART_BOTTOM].y      = size->h - frame->parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h;
     frame_target[LITE_THEME_FRAME_PART_BOTTOM].w      = frame_target[LITE_THEME_FRAME_PART_BOTTOMRIGHT].x -
                                                         frame_target[LITE_THEME_FRAME_PART_BOTTOM].x;
     frame_target[LITE_THEME_FRAME_PART_BOTTOM].h      = frame->parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h;
     /* left */
     frame_target[LITE_THEME_FRAME_PART_LEFT].x        = 0;
     frame_target[LITE_THEME_FRAME_PART_LEFT].y        = frame_target[LITE_THEME_FRAME_PART_TOPLEFT].h;
     frame_target[LITE_THEME_FRAME_PART_LEFT].w        = frame->parts[LITE_THEME_FRAME_PART_LEFT].rect.w;
     frame_target[LITE_THEME_FRAME_PART_LEFT].h        = frame_target[LITE_THEME_FRAME_PART_BOTTOMLEFT].y -
                                                         frame_target[LITE_THEME_FRAME_PART_LEFT].y;
     /* right */
     frame_target[LITE_THEME_FRAME_PART_RIGHT].x       = size->w - frame->parts[LITE_THEME_FRAME_PART_RIGHT].rect.w;
     frame_target[LITE_THEME_FRAME_PART_RIGHT].y       = frame_target[LITE_THEME_FRAME_PART_TOPRIGHT].h;
     frame_target[LITE_THEME_FRAME_PART_RIGHT].w       = frame->parts[LITE_THEME_FRAME_PART_RIGHT].rect.w;
     frame_target[LITE_THEME_FRAME_PART_RIGHT].h       = frame_target[LITE_THEME_FRAME_PART_BOTTOMRIGHT].y -
                                                         frame_target[LITE_THEME_FRAME_PART_RIGHT].y;
}
