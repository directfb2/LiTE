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
#include <lite/progressbar.h>

D_DEBUG_DOMAIN( LiteProgressBarDomain, "LiTE/ProgressBar", "LiTE ProgressBar" );

/**********************************************************************************************************************/

LiteProgressBarTheme *liteDefaultProgressBarTheme = NULL;

struct _LiteProgressBar {
     LiteBox               box;
     LiteProgressBarTheme *theme;

     IDirectFBSurface     *surface_fg;
     IDirectFBSurface     *surface_bg;
     float                 value;
};

static DFBResult draw_progressbar   ( LiteBox *box, const DFBRegion *region, DFBBoolean clear );
static DFBResult destroy_progressbar( LiteBox *box );

/**********************************************************************************************************************/

DFBResult
lite_new_progressbar( LiteBox               *parent,
                      DFBRectangle          *rect,
                      LiteProgressBarTheme  *theme,
                      LiteProgressBar      **ret_progressbar )
{
     DFBResult        ret;
     LiteProgressBar *progressbar;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_progressbar );

     progressbar = D_CALLOC( 1, sizeof(LiteProgressBar) );

     progressbar->box.parent = parent;
     progressbar->box.rect   = *rect;
     progressbar->theme      = theme;

     ret = lite_init_box( LITE_BOX(progressbar) );
     if (ret != DFB_OK) {
          D_FREE( progressbar );
          return ret;
     }

     progressbar->box.type    = LITE_TYPE_PROGRESSBAR;
     progressbar->box.Draw    = draw_progressbar;
     progressbar->box.Destroy = destroy_progressbar;
     progressbar->value       = 0.0f;

     *ret_progressbar = progressbar;

     D_DEBUG_AT( LiteProgressBarDomain, "Created new progressbar object: %p\n", progressbar );

     return DFB_OK;
}

DFBResult lite_set_progressbar_value( LiteProgressBar *progressbar,
                                      float            value )
{
     LITE_NULL_PARAMETER_CHECK( progressbar );
     LITE_BOX_TYPE_PARAMETER_CHECK( progressbar, LITE_TYPE_PROGRESSBAR );

     if (value < 0.0f)
          value = 0.0f;
     else if (value > 1.0f)
          value = 1.0f;

     D_DEBUG_AT( LiteProgressBarDomain, "Set progressbar: %p with value: %f\n", progressbar, value );

     if (value == progressbar->value)
          return DFB_OK;

     progressbar->value = value;

     return lite_update_box( LITE_BOX(progressbar), NULL );
}

DFBResult lite_get_progressbar_value( LiteProgressBar *progressbar,
                                      float           *ret_value )
{
     LITE_NULL_PARAMETER_CHECK( progressbar );
     LITE_NULL_PARAMETER_CHECK( ret_value );
     LITE_BOX_TYPE_PARAMETER_CHECK( progressbar, LITE_TYPE_PROGRESSBAR );

     D_DEBUG_AT( LiteProgressBarDomain, "progressbar: %p has value: %f\n", progressbar, progressbar->value );

     *ret_value = progressbar->value;

     return DFB_OK;
}

DFBResult
lite_set_progressbar_images( LiteProgressBar *progressbar,
                             const char      *image_fg_path,
                             const char      *image_bg_path )
{
     DFBResult         ret;
     IDirectFBSurface *surface;

     LITE_NULL_PARAMETER_CHECK( progressbar );
     LITE_BOX_TYPE_PARAMETER_CHECK( progressbar, LITE_TYPE_PROGRESSBAR );

     D_DEBUG_AT( LiteProgressBarDomain, "Set progressbar: %p with images: %s %s\n", progressbar,
                 image_fg_path, image_bg_path );

     if (image_fg_path) {
          ret = prvlite_load_image( image_fg_path, &surface, NULL, NULL, NULL );
          if (ret != DFB_OK)
               return ret;

          if (progressbar->surface_fg)
               progressbar->surface_fg->Release( progressbar->surface_fg );

          progressbar->surface_fg = surface;
     }
     else if (progressbar->surface_fg) {
          progressbar->surface_fg->Release( progressbar->surface_fg );
          progressbar->surface_fg = NULL;
     }

     if (image_fg_path && image_bg_path) {
          ret = prvlite_load_image( image_bg_path, &surface, NULL, NULL, NULL );
          if (ret != DFB_OK) {
               progressbar->surface_fg->Release( progressbar->surface_fg );
               return ret;
          }

          if (progressbar->surface_bg)
               progressbar->surface_bg->Release( progressbar->surface_bg );

          progressbar->surface_bg = surface;
     }
     else if (progressbar->surface_bg) {
          progressbar->surface_bg->Release( progressbar->surface_bg );
          progressbar->surface_bg = NULL;
     }

     return lite_update_box( LITE_BOX(progressbar), NULL );
}

DFBResult
lite_new_progressbar_theme( const char            *image_fg_path,
                            const char            *image_bg_path,
                            LiteProgressBarTheme **ret_theme )
{
     DFBResult             ret;
     LiteProgressBarTheme *theme;

     LITE_NULL_PARAMETER_CHECK( image_fg_path );
     LITE_NULL_PARAMETER_CHECK( ret_theme );

     if (liteDefaultProgressBarTheme && *ret_theme == liteDefaultProgressBarTheme)
          return DFB_OK;

     theme = D_CALLOC( 1, sizeof(LiteProgressBarTheme) );

     ret = prvlite_load_image( image_fg_path, &theme->surface_fg, NULL, NULL, NULL );
     if (ret != DFB_OK) {
          D_FREE( theme );
          return ret;
     }

     if (image_bg_path) {
          ret = prvlite_load_image( image_bg_path, &theme->surface_bg, NULL, NULL, NULL );
          if (ret != DFB_OK) {
               theme->surface_fg->Release( theme->surface_fg );
               D_FREE( theme );
               return ret;
          }
     }

     *ret_theme = theme;

     D_DEBUG_AT( LiteProgressBarDomain, "Created new progress bar theme: %p\n", theme );

     return DFB_OK;
}

DFBResult
lite_destroy_progressbar_theme( LiteProgressBarTheme *theme )
{
     LITE_NULL_PARAMETER_CHECK( theme );

     D_DEBUG_AT( LiteProgressBarDomain, "Destroy progress bar theme: %p\n", theme );

     if (theme->surface_bg)
          theme->surface_bg->Release( theme->surface_bg );

     theme->surface_fg->Release( theme->surface_fg );

     D_FREE( theme );

     if (theme == liteDefaultProgressBarTheme)
          liteDefaultProgressBarTheme = NULL;

     return DFB_OK;
}

/* internals */

static DFBResult
draw_progressbar( LiteBox         *box,
                  const DFBRegion *region,
                  DFBBoolean       clear )
{
     DFBRectangle      rect;
     IDirectFBSurface *surface;
     LiteProgressBar  *progressbar = LITE_PROGRESSBAR(box);

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteProgressBarDomain, "Draw progressbar: %p (value:%f, clear:%u)\n",
                 progressbar, progressbar->value, clear );

     if (clear)
          lite_clear_box( box, region );

     surface->SetClip( surface, region );

     rect.x = 0;
     rect.y = 0;
     rect.h = box->rect.h;
     rect.w = box->rect.w * progressbar->value;

     if (progressbar->surface_fg || (progressbar->theme != liteNoProgressBarTheme && progressbar->theme->surface_fg)) {
          if (progressbar->surface_bg) {
               surface->Blit( surface, progressbar->surface_bg, NULL, 0, 0 );
          }
          else if (!progressbar->surface_fg &&
                   (progressbar->theme != liteNoProgressBarTheme && progressbar->theme->surface_bg)) {
               surface->Blit( surface, progressbar->theme->surface_bg, NULL, 0, 0 );
          }

          surface->Blit( surface, progressbar->surface_fg ?: progressbar->theme->surface_fg, &rect, 0, 0 );
     }

     return DFB_OK;
}

static DFBResult
destroy_progressbar( LiteBox *box )
{
     LiteProgressBar *progressbar = LITE_PROGRESSBAR(box);

     D_ASSERT( box != NULL );

     D_DEBUG_AT( LiteProgressBarDomain, "Destroy progressbar: %p\n", progressbar );

     if (progressbar->surface_bg)
          progressbar->surface_bg->Release( progressbar->surface_bg );

     if (progressbar->surface_fg)
          progressbar->surface_fg->Release( progressbar->surface_fg );

     return lite_destroy_box( box );
}
