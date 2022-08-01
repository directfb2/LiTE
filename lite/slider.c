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
#include <lite/slider.h>

D_DEBUG_DOMAIN( LiteSliderDomain, "LiTE/Slider", "LiTE Slider" );

/**********************************************************************************************************************/

LiteSliderTheme *liteDefaultSliderTheme = NULL;

struct _LiteSlider {
     LiteBox               box;
     LiteSliderTheme      *theme;

     float                 pos;
     bool                  vertical;

     LiteSliderUpdateFunc  update;
     void                 *update_data;
};

static int       on_focus_in   ( LiteBox *box );
static int       on_focus_out  ( LiteBox *box );
static int       on_enter      ( LiteBox *box, int x, int y );
static int       on_motion     ( LiteBox *box, int x, int y, DFBInputDeviceButtonMask button_mask );
static int       on_button_down( LiteBox *box, int x, int y, DFBInputDeviceButtonIdentifier button_id );
static DFBResult draw_slider   ( LiteBox *box, const DFBRegion *region, DFBBoolean clear );

/**********************************************************************************************************************/

DFBResult
lite_new_slider( LiteBox          *parent,
                 DFBRectangle     *rect,
                 LiteSliderTheme  *theme,
                 LiteSlider      **ret_slider )
{
     DFBResult   ret;
     LiteSlider *slider;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_slider );

     slider = D_CALLOC( 1, sizeof(LiteSlider) );

     slider->box.parent = parent;
     slider->box.rect   = *rect;
     slider->theme      = theme;

     ret = lite_init_box( LITE_BOX(slider) );
     if (ret != DFB_OK) {
          D_FREE( slider );
          return ret;
     }

     slider->box.type         = LITE_TYPE_SLIDER;
     slider->box.OnFocusIn    = on_focus_in;
     slider->box.OnFocusOut   = on_focus_out;
     slider->box.OnEnter      = on_enter;
     slider->box.OnMotion     = on_motion;
     slider->box.OnButtonDown = on_button_down;
     slider->box.Draw         = draw_slider;
     slider->vertical         = (slider->box.rect.h > slider->box.rect.w) ? true : false;

     *ret_slider = slider;

     D_DEBUG_AT( LiteSliderDomain, "Created new slider object: %p\n", slider );

     return DFB_OK;
}

DFBResult
lite_set_slider_pos( LiteSlider *slider,
                     float       pos )
{
     LITE_NULL_PARAMETER_CHECK( slider );
     LITE_BOX_TYPE_PARAMETER_CHECK( slider, LITE_TYPE_SLIDER );

     if (pos < 0.0f)
          pos = 0.0f;
     else if (pos > 1.0f)
          pos = 1.0f;

     D_DEBUG_AT( LiteSliderDomain, "Set slider: %p with indicator position to: %f\n", slider, pos );

     if (pos == slider->pos)
          return DFB_OK;

     slider->pos = pos;

     return lite_update_box( LITE_BOX(slider), NULL );
}

DFBResult
lite_on_slider_update( LiteSlider           *slider,
                       LiteSliderUpdateFunc  callback,
                       void                 *data )
{
     LITE_NULL_PARAMETER_CHECK( slider );
     LITE_BOX_TYPE_PARAMETER_CHECK( slider, LITE_TYPE_SLIDER );

     D_DEBUG_AT( LiteSliderDomain, "Install callback %p( %p, %p )\n", callback, slider, data );

     slider->update      = callback;
     slider->update_data = data;

     return DFB_OK;
}

DFBResult
lite_new_slider_theme( const DFBColor   *bg_color,
                       const DFBColor   *fg_color,
                       LiteSliderTheme **ret_theme )
{
     LiteSliderTheme *theme;

     LITE_NULL_PARAMETER_CHECK( bg_color );
     LITE_NULL_PARAMETER_CHECK( fg_color );
     LITE_NULL_PARAMETER_CHECK( ret_theme );

     if (liteDefaultSliderTheme && *ret_theme == liteDefaultSliderTheme)
          return DFB_OK;

     theme = D_CALLOC( 1, sizeof(LiteSliderTheme) );

     theme->theme.bg_color = *bg_color;
     theme->theme.fg_color = *fg_color;

     *ret_theme = theme;

     D_DEBUG_AT( LiteSliderDomain, "Created new slider theme: %p\n", theme );

     return DFB_OK;
}

DFBResult
lite_destroy_slider_theme( LiteSliderTheme *theme )
{
     LITE_NULL_PARAMETER_CHECK( theme );

     D_DEBUG_AT( LiteSliderDomain, "Destroy slider theme: %p\n", theme );

     D_FREE( theme );

     if (theme == liteDefaultSliderTheme)
          liteDefaultSliderTheme = NULL;

     return DFB_OK;
}

/* internals */

static int
on_focus_in( LiteBox *box )
{
     D_ASSERT( box != NULL );

     lite_update_box( box, NULL );

     return 1;
}

static int
on_focus_out( LiteBox *box )
{
     D_ASSERT( box != NULL );

     lite_update_box( box, NULL );

     return 1;
}

static int
on_enter( LiteBox *box,
          int      x,
          int      y )
{
     D_ASSERT( box != NULL );

     lite_focus_box( box );

     return 1;
}

static int
on_motion( LiteBox                  *box,
           int                       x,
           int                       y,
           DFBInputDeviceButtonMask  button_mask )
{
     D_ASSERT( box != NULL );

     if (button_mask)
          return on_button_down( box, x, y, DIBI_LEFT );

     return 1;
}

static int
on_button_down( LiteBox                        *box,
                int                             x,
                int                             y,
                DFBInputDeviceButtonIdentifier  button_id )
{
     float       pos    = (x - 2) / (float) (box->rect.w - 5);
     LiteSlider *slider = LITE_SLIDER(box);

     D_ASSERT( box != NULL );

     if (pos < 0.0f)
          pos = 0.0f;
     else if (pos > 1.0f)
          pos = 1.0f;

     if (pos == slider->pos)
          return 1;

     slider->pos = pos;

     if (slider->update)
          slider->update( slider, pos, slider->update_data );

     lite_update_box( box, NULL );

     return 1;
}

static DFBResult
draw_slider( LiteBox         *box,
             const DFBRegion *region,
             DFBBoolean       clear )
{
     IDirectFBSurface *surface;
     LiteSlider       *slider = LITE_SLIDER(box);

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteSliderDomain, "Draw slider: %p (vertical:%d, pos:%f, clear:%u)\n",
                 slider, slider->vertical, slider->pos, clear );

     if (clear)
          lite_clear_box( box, region );

     surface->SetClip( surface, region );

     surface->SetDrawingFlags( surface, DSDRAW_NOFX );

     if (slider->vertical) {
          int w2  = box->rect.w / 2;
          int pos = slider->pos * box->rect.h;

          /* draw vertical line */
          surface->SetColor( surface, 0xe0, 0xe0, 0xe0, 0xff );
          surface->DrawRectangle( surface, w2 - 3, 0, 8, box->rect.h );
          surface->SetColor( surface, 0xb0, 0xb0, 0xb0, 0xff );
          surface->DrawRectangle( surface, w2 - 2, 1, 6 , box->rect.h - 2 );
          if (box->is_focused) {
               if (slider->theme != liteNoSliderTheme)
                    surface->SetColor( surface,
                                       slider->theme->theme.fg_color.r,
                                       slider->theme->theme.fg_color.g,
                                       slider->theme->theme.fg_color.b,
                                       slider->theme->theme.fg_color.a );
               else
                    surface->SetColor( surface, 0xc0, 0xc0, 0xff, 0xf0 );
          }
          else {
               if (slider->theme != liteNoSliderTheme)
                    surface->SetColor( surface,
                                       slider->theme->theme.bg_color.r,
                                       slider->theme->theme.bg_color.g,
                                       slider->theme->theme.bg_color.b,
                                       slider->theme->theme.bg_color.a );
               else
                    surface->SetColor( surface, 0xf0, 0xf0, 0xf0, 0xd0 );
          }
          surface->FillRectangle( surface, w2 - 1, 2, 4, box->rect.h - 4 );

          /* draw the position indicator */
          surface->SetDrawingFlags( surface, DSDRAW_BLEND );
          surface->SetColor( surface, 0x80, 0x80, 0xa0, 0xe0 );
          surface->FillRectangle( surface, 0, pos - 1, box->rect.w, 1 );
          surface->FillRectangle( surface, 0, pos + 1, box->rect.w, 1 );
          surface->SetColor( surface, 0xb0, 0xb0, 0xc0, 0xff );
          surface->FillRectangle( surface, 0, pos, box->rect.w, 1 );
     }
     else {
          int h2  = box->rect.h / 2;
          int pos = slider->pos * (box->rect.w - 5) + 2;

          /* draw horizontal line */
          surface->SetColor( surface, 0xe0, 0xe0, 0xe0, 0xff );
          surface->DrawRectangle( surface, 0, h2 - 3, box->rect.w, 8 );
          surface->SetColor( surface, 0xb0, 0xb0, 0xb0, 0xff );
          surface->DrawRectangle( surface, 1, h2 - 2, box->rect.w - 2, 6 );
          if (box->is_focused) {
               if (slider->theme != liteNoSliderTheme)
                    surface->SetColor( surface,
                                       slider->theme->theme.fg_color.r,
                                       slider->theme->theme.fg_color.g,
                                       slider->theme->theme.fg_color.b,
                                       slider->theme->theme.fg_color.a );
               else
                    surface->SetColor( surface, 0xc0, 0xc0, 0xff, 0xf0 );
          }
          else {
               if (slider->theme != liteNoSliderTheme)
                    surface->SetColor( surface,
                                       slider->theme->theme.bg_color.r,
                                       slider->theme->theme.bg_color.g,
                                       slider->theme->theme.bg_color.b,
                                       slider->theme->theme.bg_color.a );
               else
                    surface->SetColor( surface, 0xf0, 0xf0, 0xf0, 0xd0 );
          }
          surface->FillRectangle( surface, 2, h2 - 1, box->rect.w - 4, 4 );

          /* draw the position indicator */
          surface->SetDrawingFlags( surface, DSDRAW_BLEND );
          surface->SetColor( surface, 0x80, 0x80, 0xa0, 0xe0 );
          surface->FillRectangle( surface, pos - 1, 0, 1, box->rect.h );
          surface->FillRectangle( surface, pos + 1, 0, 1, box->rect.h );
          surface->SetColor( surface, 0xb0, 0xb0, 0xc0, 0xff );
          surface->FillRectangle( surface, pos, 0, 1, box->rect.h );
     }

     return DFB_OK;
}
