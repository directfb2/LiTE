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

#include <direct/clock.h>
#include <lite/animation.h>
#include <lite/lite_internal.h>

D_DEBUG_DOMAIN( LiteAnimationDomain, "LiTE/Animation", "LiTE Animation" );

/**********************************************************************************************************************/

LiteAnimationTheme *liteDefaultAnimationTheme = NULL;

struct _LiteAnimation {
     LiteBox             box;
     LiteAnimationTheme *theme;

     int                 stretch;
     int                 still_frame;
     int                 current;
     int                 timeout;
     long long           last_time;

     IDirectFBSurface   *image;
     int                 frame_width;
     int                 frame_height;
     int                 frames;
     int                 frames_h;
     int                 frames_v;
};

static DFBResult draw_animation   ( LiteBox *box, const DFBRegion *region, DFBBoolean clear );
static DFBResult destroy_animation( LiteBox *box );

/**********************************************************************************************************************/

DFBResult
lite_new_animation( LiteBox             *parent,
                    DFBRectangle        *rect,
                    LiteAnimationTheme  *theme,
                    LiteAnimation      **ret_animation )
{
     DFBResult      ret;
     LiteAnimation *animation;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_animation );

     animation = D_CALLOC( 1, sizeof(LiteAnimation) );

     animation->box.parent = parent;
     animation->box.rect   = *rect;
     animation->theme      = theme;

     ret = lite_init_box( LITE_BOX(animation) );
     if (ret != DFB_OK) {
          D_FREE( animation );
          return ret;
     }

     animation->box.type    = LITE_TYPE_ANIMATION;
     animation->box.Draw    = draw_animation;
     animation->box.Destroy = destroy_animation;

     *ret_animation = animation;

     D_DEBUG_AT( LiteAnimationDomain, "Created new animation object: %p\n", animation );

     return DFB_OK;
}

static DFBResult
load_animation( LiteAnimation *animation,
                const void    *file_data,
                unsigned int   length,
                int            still_frame,
                int            frame_width,
                int            frame_height )
{
     DFBResult         ret;
     int               frames_h, frames_v, frames;
     int               image_width, image_height;
     IDirectFBSurface *image;

     LITE_NULL_PARAMETER_CHECK( animation );
     LITE_NULL_PARAMETER_CHECK( file_data );
     LITE_BOX_TYPE_PARAMETER_CHECK( animation, LITE_TYPE_ANIMATION );

     D_DEBUG_AT( LiteAnimationDomain, "Load animation: %p\n", animation );

     if (frame_width < 1 || frame_height < 1)
          return DFB_INVARG;

     ret = prvlite_load_image( file_data, length, &image, &image_width, &image_height, NULL );
     if (ret != DFB_OK)
          return ret;

     if ((image_width % frame_width) || (image_height % frame_height)) {
          D_DEBUG_AT( LiteAnimationDomain, "  -> image width/height not a multiple of frame width/height!\n" );
          image->Release( image );
          return DFB_FAILURE;
     }

     frames_h = image_width  / frame_width;
     frames_v = image_height / frame_height;
     frames   = frames_h * frames_v;

     if (still_frame >= frames) {
          D_DEBUG_AT( LiteAnimationDomain, "  -> index of the animation frame out of bounds!\n" );
          image->Release( image );
          return DFB_FAILURE;
     }

     lite_stop_animation( animation );

     if (animation->image)
          animation->image->Release( animation->image );

     if (frame_width != animation->box.rect.w || frame_height != animation->box.rect.h)
          animation->stretch = 1;

     animation->still_frame  = still_frame;
     animation->current      = -1;
     animation->image        = image;
     animation->frame_width  = frame_width;
     animation->frame_height = frame_height;
     animation->frames       = frames;
     animation->frames_h     = frames_h;
     animation->frames_v     = frames_v;

     return DFB_OK;
}

DFBResult
lite_load_animation( LiteAnimation *animation,
                     const char    *filename,
                     int            still_frame,
                     int            frame_width,
                     int            frame_height )
{
     return load_animation( animation, filename, 0, still_frame, frame_width, frame_height );
}

DFBResult
lite_load_animation_data( LiteAnimation *animation,
                          const void    *data,
                          unsigned int   length,
                          int            still_frame,
                          int            frame_width,
                          int            frame_height )
{
     return load_animation( animation, data, length, still_frame, frame_width, frame_height );
}

DFBResult
lite_start_animation( LiteAnimation *animation,
                      unsigned int   ms_timeout )
{
     LITE_NULL_PARAMETER_CHECK( animation );
     LITE_BOX_TYPE_PARAMETER_CHECK( animation, LITE_TYPE_ANIMATION );

     D_DEBUG_AT( LiteAnimationDomain, "Start animation: %p\n", animation );

     animation->current = (animation->still_frame < 0) ? 0 : animation->still_frame;
     animation->timeout = ms_timeout ?: 1;

     return lite_update_box( LITE_BOX(animation), NULL );
}

int
lite_update_animation( LiteAnimation *animation )
{
     long long new_time, diff;

     LITE_NULL_PARAMETER_CHECK( animation );
     LITE_BOX_TYPE_PARAMETER_CHECK( animation, LITE_TYPE_ANIMATION );

     D_DEBUG_AT( LiteAnimationDomain, "Update animation: %p\n", animation );

     if (!animation->timeout)
          return 0;

     new_time = direct_clock_get_millis();
     diff     = new_time - animation->last_time;

     if (diff >= animation->timeout) {
          DFBResult ret;
          int       advance = diff / animation->timeout;

          animation->current += advance;
          animation->current %= animation->frames;

          animation->last_time += advance * animation->timeout;

          ret = lite_update_box( LITE_BOX(animation), NULL );
          if (ret != DFB_OK)
               return 0;

          return 1;
     }

     return 0;
}

DFBResult
lite_stop_animation( LiteAnimation *animation )
{
     LITE_NULL_PARAMETER_CHECK( animation );
     LITE_BOX_TYPE_PARAMETER_CHECK( animation, LITE_TYPE_ANIMATION );

     D_DEBUG_AT( LiteAnimationDomain, "Stop animation: %p\n", animation );

     animation->timeout = 0;

     if (animation->still_frame >= 0 && animation->current != animation->still_frame) {
          animation->current = animation->still_frame;

          return lite_update_box( LITE_BOX(animation), NULL );
     }

     return DFB_OK;
}

int
lite_animation_running( LiteAnimation *animation )
{
     LITE_NULL_PARAMETER_CHECK( animation );
     LITE_BOX_TYPE_PARAMETER_CHECK( animation, LITE_TYPE_ANIMATION );

     D_DEBUG_AT( LiteAnimationDomain, "animation: %p is %srunning\n", animation, animation->timeout > 0 ? "" : "not " );

     return animation->timeout > 0;
}

/* internals */

static DFBResult
draw_animation( LiteBox         *box,
                const DFBRegion *region,
                DFBBoolean       clear )
{
     DFBRectangle      rect;
     IDirectFBSurface *surface;
     LiteAnimation    *animation = LITE_ANIMATION(box);

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteAnimationDomain, "Draw animation: %p (current:%d, stretch:%d, clear:%u)\n",
                 animation, animation->current, animation->stretch, clear );

     if (clear)
          lite_clear_box( box, region );

     surface->SetClip( surface, region );

     surface->SetBlittingFlags( surface, DSBLIT_BLEND_ALPHACHANNEL );

     rect.w = animation->frame_width;
     rect.h = animation->frame_height;
     rect.x = (animation->current % animation->frames_h) * rect.w;
     rect.y = (animation->current / animation->frames_h) * rect.h;

     if (animation->stretch)
          surface->StretchBlit( surface, animation->image, &rect, NULL );
     else
          surface->Blit( surface, animation->image, &rect, 0, 0 );

     return DFB_OK;
}

static DFBResult
destroy_animation( LiteBox *box )
{
     LiteAnimation *animation = LITE_ANIMATION(box);

     D_ASSERT( box != NULL );

     D_DEBUG_AT( LiteAnimationDomain, "Destroy animation: %p\n", animation );

     if (animation->image)
          animation->image->Release( animation->image );

     return lite_destroy_box( box );
}
