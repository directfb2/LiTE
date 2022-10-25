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

#include <direct/util.h>
#include <directfb_util.h>
#include <lite/lite_internal.h>
#include <lite/scrollbar.h>

D_DEBUG_DOMAIN( LiteScrollbarDomain, "LiTE/Scrollbar", "LiTE Scrollbar" );

/**********************************************************************************************************************/

LiteScrollbarTheme *liteDefaultScrollbarTheme = NULL;

typedef enum {
     SS_NORMAL,
     SS_HILITE,
     SS_PRESSED_BTN1,
     SS_PRESSED_BTN2,
     SS_PRESSED_THUMB,
     SS_DISABLED
} ScrollbarState;

struct _LiteScrollbar {
     LiteBox                  box;
     LiteScrollbarTheme      *theme;

     LiteScrollInfo           info;
     int                      vertical;
     ScrollbarState           state;
     int                      thumb_press_offset;
     int                      image_margin;
     struct {
          IDirectFBSurface   *surface;
          int                 width;
          int                 height;
     } all_images;

     LiteScrollbarUpdateFunc  update;
     void                    *update_data;
};

static int       on_enter         ( LiteBox *box, int x, int y );
static int       on_leave         ( LiteBox *box, int x, int y );
static int       on_motion        ( LiteBox *box, int x, int y, DFBInputDeviceButtonMask button_mask );
static int       on_button_down   ( LiteBox *box, int x, int y, DFBInputDeviceButtonIdentifier button_id );
static int       on_button_up     ( LiteBox *box, int x, int y, DFBInputDeviceButtonIdentifier button_id );
static DFBResult draw_scrollbar   ( LiteBox *box, const DFBRegion *region, DFBBoolean clear );
static DFBResult destroy_scrollbar( LiteBox *box );

/**********************************************************************************************************************/

DFBResult
lite_new_scrollbar( LiteBox             *parent,
                    DFBRectangle        *rect,
                    int                  vertical,
                    LiteScrollbarTheme  *theme,
                    LiteScrollbar      **ret_scrollbar )
{
     DFBResult      ret;
     LiteScrollbar *scrollbar;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_scrollbar );

     scrollbar = D_CALLOC( 1, sizeof(LiteScrollbar) );

     scrollbar->box.parent = parent;
     scrollbar->box.rect   = *rect;
     scrollbar->theme      = theme;

     ret = lite_init_box( LITE_BOX(scrollbar) );
     if (ret != DFB_OK) {
          D_FREE( scrollbar );
          return ret;
     }

     scrollbar->box.type         = LITE_TYPE_SCROLLBAR;
     scrollbar->box.OnEnter      = on_enter;
     scrollbar->box.OnLeave      = on_leave;
     scrollbar->box.OnMotion     = on_motion;
     scrollbar->box.OnButtonDown = on_button_down;
     scrollbar->box.OnButtonUp   = on_button_up;
     scrollbar->box.Draw         = draw_scrollbar;
     scrollbar->box.Destroy      = destroy_scrollbar;
     scrollbar->info.max         = 20;
     scrollbar->info.page_size   = 7;
     scrollbar->info.line_size   = 1;
     scrollbar->info.track_pos   = -1;
     scrollbar->vertical         = vertical;
     scrollbar->state            = SS_NORMAL;

     lite_update_box( LITE_BOX(scrollbar), NULL );

     *ret_scrollbar = scrollbar;

     D_DEBUG_AT( LiteScrollbarDomain, "Created new scrollbar object: %p\n", scrollbar );

     return DFB_OK;
}

DFBResult
lite_enable_scrollbar( LiteScrollbar *scrollbar,
                       int            enabled )
{
     LITE_NULL_PARAMETER_CHECK( scrollbar );
     LITE_BOX_TYPE_PARAMETER_CHECK( scrollbar, LITE_TYPE_SCROLLBAR );

     D_DEBUG_AT( LiteScrollbarDomain, "%s scrollbar: %p\n", enabled ? "Enable" : "Disable", scrollbar );

     if ((scrollbar->state != SS_DISABLED) == enabled)
          return DFB_OK;

     scrollbar->state = enabled ? SS_NORMAL : SS_DISABLED;

     scrollbar->box.is_active = enabled;

     return lite_update_box( &scrollbar->box, NULL );
}

DFBResult
lite_get_scrollbar_thickness( LiteScrollbar *scrollbar,
                              int           *ret_thickness )
{
     LITE_NULL_PARAMETER_CHECK( scrollbar );
     LITE_NULL_PARAMETER_CHECK( ret_thickness );
     LITE_BOX_TYPE_PARAMETER_CHECK( scrollbar, LITE_TYPE_SCROLLBAR );

     if (scrollbar->all_images.width || scrollbar->theme != liteNoScrollbarTheme)
          *ret_thickness = scrollbar->all_images.width ? scrollbar->all_images.width / 8 :
                                                         scrollbar->theme->all_images.width / 8;
     else
          *ret_thickness = 0;

     D_DEBUG_AT( LiteScrollbarDomain, "scrollbar: %p has a thickness of: %d\n", scrollbar, *ret_thickness );

     return DFB_OK;
}

DFBResult
lite_set_scroll_pos( LiteScrollbar *scrollbar,
                     int            pos )
{
     LITE_NULL_PARAMETER_CHECK( scrollbar );
     LITE_BOX_TYPE_PARAMETER_CHECK( scrollbar, LITE_TYPE_SCROLLBAR );

     if (pos < scrollbar->info.min)
          pos = scrollbar->info.min;
     else if (pos > scrollbar->info.max)
          pos = scrollbar->info.max;

     D_DEBUG_AT( LiteScrollbarDomain, "Set scrollbar: %p with scroll position to: %d\n", scrollbar, pos );

     if (pos == scrollbar->info.pos)
          return DFB_OK;

     scrollbar->info.pos = pos;

     return lite_update_box( LITE_BOX(scrollbar), NULL );
}

DFBResult
lite_get_scroll_pos( LiteScrollbar *scrollbar,
                     int           *ret_pos )
{
     LITE_NULL_PARAMETER_CHECK( scrollbar );
     LITE_NULL_PARAMETER_CHECK( ret_pos );
     LITE_BOX_TYPE_PARAMETER_CHECK( scrollbar, LITE_TYPE_SCROLLBAR );

     if (scrollbar->info.track_pos != -1)
          *ret_pos = scrollbar->info.track_pos;
     else
          *ret_pos = scrollbar->info.pos;

     D_DEBUG_AT( LiteScrollbarDomain, "scrollbar: %p has %s position of: %d\n", scrollbar,
                 scrollbar->info.track_pos != -1 ? "tracking" : "scroll", *ret_pos );

     return DFB_OK;
}

DFBResult lite_set_scroll_info( LiteScrollbar  *scrollbar,
                                LiteScrollInfo *info )
{
     bool dragging;
     int  max;

     LITE_NULL_PARAMETER_CHECK( scrollbar );
     LITE_NULL_PARAMETER_CHECK( info );
     LITE_BOX_TYPE_PARAMETER_CHECK( scrollbar, LITE_TYPE_SCROLLBAR );

     D_DEBUG_AT( LiteScrollbarDomain, "Set scrollbar: %p with info: (%u,%u,%u),(%d,%d)\n", scrollbar,
                 scrollbar->info.min, scrollbar->info.page_size, scrollbar->info.max,
                 scrollbar->info.pos, scrollbar->info.track_pos );

     dragging = (scrollbar->info.track_pos != -1);

     scrollbar->info = *info;

     if (scrollbar->info.pos < scrollbar->info.min)
          scrollbar->info.pos = scrollbar->info.min;

     max = scrollbar->info.max - ((scrollbar->info.page_size > 0) ? scrollbar->info.page_size : 0);

     if (scrollbar->info.pos > max)
          scrollbar->info.pos = max;

     if (!dragging) {
          scrollbar->info.track_pos = -1;
     }
     else if (scrollbar->info.track_pos != -1) {
          if (scrollbar->info.track_pos < scrollbar->info.min)
               scrollbar->info.track_pos = scrollbar->info.min;

          if (scrollbar->info.track_pos > max)
               scrollbar->info.track_pos = max;
     }

     if (info->max <= info->min || info->page_size >= (info->max - info->min)) {
          scrollbar->box.is_active = 0;
          scrollbar->info.pos = scrollbar->info.min;
     }
     else {
          if (scrollbar->state != SS_DISABLED)
               scrollbar->box.is_active = 1;
     }

     return lite_update_box( LITE_BOX(scrollbar), NULL );
}

DFBResult
lite_get_scroll_info( LiteScrollbar  *scrollbar,
                      LiteScrollInfo *ret_info )
{
     LITE_NULL_PARAMETER_CHECK( scrollbar );
     LITE_NULL_PARAMETER_CHECK( ret_info );
     LITE_BOX_TYPE_PARAMETER_CHECK( scrollbar, LITE_TYPE_SCROLLBAR );

     D_DEBUG_AT( LiteScrollbarDomain, "scrollbar: %p has info: (%u,%u,%u),(%d,%d)\n", scrollbar,
                 scrollbar->info.min, scrollbar->info.page_size, scrollbar->info.max,
                 scrollbar->info.pos, scrollbar->info.track_pos );

     *ret_info = scrollbar->info;

     return DFB_OK;
}

DFBResult
lite_set_scrollbar_all_images( LiteScrollbar *scrollbar,
                               const char    *image_path,
                               int            image_margin )
{
     LITE_NULL_PARAMETER_CHECK( scrollbar );
     LITE_BOX_TYPE_PARAMETER_CHECK( scrollbar, LITE_TYPE_SCROLLBAR );

     D_DEBUG_AT( LiteScrollbarDomain, "Set scrollbar: %p with image: %s for all subsections\n", scrollbar, image_path );

     if (image_path) {
          DFBResult         ret;
          int               all_images_width, all_images_height;
          IDirectFBSurface *all_images_surface;

          ret = prvlite_load_image( image_path, &all_images_surface, &all_images_width, &all_images_height, NULL );
          if (ret != DFB_OK)
               return ret;

          if (scrollbar->all_images.surface)
               scrollbar->all_images.surface->Release( scrollbar->all_images.surface );

          scrollbar->all_images.surface = all_images_surface;
          scrollbar->all_images.width   = all_images_width;
          scrollbar->all_images.height  = all_images_height;
          scrollbar->image_margin       = image_margin;
     }
     else if (scrollbar->all_images.surface) {
          scrollbar->all_images.surface->Release( scrollbar->all_images.surface );
          scrollbar->all_images.surface = NULL;
          scrollbar->all_images.width   = 0;
          scrollbar->all_images.height  = 0;
          scrollbar->image_margin       = 0;
     }

     return lite_update_box( LITE_BOX(scrollbar), NULL );
}

DFBResult
lite_on_scrollbar_update( LiteScrollbar           *scrollbar,
                          LiteScrollbarUpdateFunc  callback,
                          void                    *data )
{
     LITE_NULL_PARAMETER_CHECK( scrollbar );
     LITE_BOX_TYPE_PARAMETER_CHECK( scrollbar, LITE_TYPE_SCROLLBAR );

     D_DEBUG_AT( LiteScrollbarDomain, "Install callback %p( %p, %p )\n", callback, scrollbar, data );

     scrollbar->update      = callback;
     scrollbar->update_data = data;

     return DFB_OK;
}

DFBResult lite_new_scrollbar_theme( const char          *image_path,
                                    int                  image_margin,
                                    LiteScrollbarTheme **ret_theme )
{
     DFBResult           ret;
     LiteScrollbarTheme *theme;

     LITE_NULL_PARAMETER_CHECK( image_path );
     LITE_NULL_PARAMETER_CHECK( ret_theme );

     if (liteDefaultScrollbarTheme && *ret_theme == liteDefaultScrollbarTheme)
          return DFB_OK;

     theme = D_CALLOC( 1, sizeof(LiteScrollbarTheme) );

     theme->image_margin = image_margin;

     ret = prvlite_load_image( image_path, &theme->all_images.surface,
                               &theme->all_images.width, &theme->all_images.height, NULL );
     if (ret != DFB_OK) {
          D_FREE( theme );
          return ret;
     }

     *ret_theme = theme;

     D_DEBUG_AT( LiteScrollbarDomain, "Created new scrollbar theme: %p\n", theme );

     return DFB_OK;
}

DFBResult
lite_destroy_scrollbar_theme( LiteScrollbarTheme *theme )
{
     LITE_NULL_PARAMETER_CHECK( theme );

     D_DEBUG_AT( LiteScrollbarDomain, "Destroy scrollbar theme: %p\n", theme );

     theme->all_images.surface->Release( theme->all_images.surface );

     D_FREE( theme );

     if (theme == liteDefaultScrollbarTheme)
          liteDefaultScrollbarTheme = NULL;

     return DFB_OK;
}

/* internals */

static int
on_enter( LiteBox *box,
          int      x,
          int      y )
{
     LiteScrollbar *scrollbar = LITE_SCROLLBAR(box);

     D_ASSERT( box != NULL );

     if (scrollbar->state == SS_NORMAL) {
          scrollbar->state = SS_HILITE;
          lite_update_box( box, NULL );
     }

     return 1;
}

static int
on_leave( LiteBox *box,
          int      x,
          int      y )
{
     LiteScrollbar *scrollbar = LITE_SCROLLBAR(box);

     D_ASSERT( box != NULL );

     if (scrollbar->state == SS_HILITE) {
          scrollbar->state = SS_NORMAL;
          lite_update_box( box, NULL );
     }

     return 1;
}

typedef enum {
     SA_BTN1,
     SA_BTN2,
     SA_THUMB
} ScrollbarArea;

static void
get_scrollbar_rect( LiteScrollbar *scrollbar,
                    ScrollbarArea  area,
                    DFBRectangle  *rc )
{
     int      thickness, len, max, pos;
     LiteBox *box = LITE_BOX(scrollbar);

     if (!scrollbar || !rc)
          return;

     lite_get_scrollbar_thickness( scrollbar, &thickness );

     switch (area) {
          case SA_BTN1:
               rc->x = 0;
               rc->y = 0;
               if (scrollbar->vertical) {
                    rc->w = box->rect.w;
                    rc->h = thickness;
               }
               else {
                    rc->w = thickness;
                    rc->h = box->rect.h;
               }
               break;

          case SA_BTN2:
               if (scrollbar->vertical) {
                    rc->x = 0;
                    rc->y = box->rect.h - thickness;
                    rc->w = box->rect.w;
                    rc->h = thickness;
               }
               else {
                    rc->x = box->rect.w - thickness;
                    rc->y = 0;
                    rc->w = thickness;
                    rc->h = box->rect.h;
               }
               break;

          case SA_THUMB:
               if (scrollbar->info.track_pos == -1)
                    pos = scrollbar->info.pos;
               else
                    pos = scrollbar->info.track_pos;

               if (scrollbar->vertical) {
                    rc->x = 0;
                    rc->w = box->rect.w;
                    len = box->rect.h - 2 * thickness;

                    if (len < 0) {
                         rc->y = box->rect.h / 2;
                         rc->h = 0;
                         break;
                    }

                    if (len <= thickness) {
                         rc->y = thickness;
                         rc->h = len;
                         break;
                    }

                    if (scrollbar->info.page_size > 0) {
                         rc->h = len * scrollbar->info.page_size / (scrollbar->info.max - scrollbar->info.min);
                         if (rc->h < thickness)
                              rc->h = thickness;
                         max = scrollbar->info.max - scrollbar->info.page_size;
                    }
                    else {
                         rc->h = thickness;
                         max = scrollbar->info.max;
                    }

                    if (max == scrollbar->info.min)
                         rc->y = thickness;
                    else
                         rc->y = (len - rc->h) * (pos - scrollbar->info.min) / (max - scrollbar->info.min) + thickness;
               }
               else {
                    rc->y = 0;
                    rc->h = box->rect.h;
                    len = box->rect.w - 2 * thickness;

                    if (len < 0) {
                         rc->x = box->rect.w/2;
                         rc->w = 0;
                         break;
                    }

                    if (len <= thickness) {
                         rc->x = thickness;
                         rc->w = len;
                         break;
                    }

                    if (scrollbar->info.page_size > 0) {
                         rc->w = len * scrollbar->info.page_size / (scrollbar->info.max - scrollbar->info.min );
                         if (rc->w < thickness)
                              rc->w = thickness;
                         max = scrollbar->info.max - scrollbar->info.page_size;
                    }
                    else {
                         rc->w = thickness;
                         max = scrollbar->info.max;
                    }

                    if (max == scrollbar->info.min)
                         rc->x = thickness;
                    else
                         rc->x = (len - rc->w) * (pos - scrollbar->info.min) / (max - scrollbar->info.min) + thickness;
               }
               break;
     }
}

typedef enum {
     SM_LINE,
     SM_PAGE,
     SM_ABSOLUTE_POS
} ScrollMode;

static void
scroll( LiteScrollbar *scrollbar,
        ScrollMode     mode,
        int            param,
        bool           dragging )
{
     int            max, pos;
     LiteScrollInfo info;

     pos = scrollbar->info.pos;

     switch (mode) {
          case SM_PAGE:
               if (scrollbar->info.page_size > 0) {
                    if (param > 0)
                         pos += scrollbar->info.page_size;
                    else if (param < 0)
                         pos -= scrollbar->info.page_size;
                    break;
               }
               /* fall through */

          case SM_LINE:
               if (param > 0)
                    pos += scrollbar->info.line_size;
               else if (param < 0)
                    pos -= scrollbar->info.line_size;
               break;

          case SM_ABSOLUTE_POS:
               pos = param;
               break;
     }

     max = scrollbar->info.max;

     if (scrollbar->info.page_size > 0)
          max -= scrollbar->info.page_size;

     if (max < scrollbar->info.min)
          max = scrollbar->info.min;

     if (pos < (int) scrollbar->info.min)
          pos = scrollbar->info.min;

     if (pos > max)
          pos = max;

     if (pos == scrollbar->info.pos)
          return;

     if (dragging) {
          scrollbar->info.track_pos = pos;
     }
     else {
          scrollbar->info.pos = pos;
          scrollbar->info.track_pos = -1;
     }

     if (scrollbar->update) {
          info = scrollbar->info;
          scrollbar->update( scrollbar, &info, scrollbar->update_data );
     }
}

static int
on_motion( LiteBox                  *box,
           int                       x,
           int                       y,
           DFBInputDeviceButtonMask  button_mask )
{
     DFBRectangle   rcThumb;
     int            thickness, lenThumb, len, max, pos, value;
     LiteScrollbar *scrollbar = LITE_SCROLLBAR(box);

     D_ASSERT( box != NULL );

     lite_get_scrollbar_thickness( scrollbar, &thickness );

     if (button_mask) {
          if (scrollbar->state == SS_PRESSED_THUMB) {
               max = scrollbar->info.max;
               if (scrollbar->info.page_size > 0)
                    max = scrollbar->info.max - scrollbar->info.page_size;

               get_scrollbar_rect( scrollbar, SA_THUMB, &rcThumb );

               if (scrollbar->vertical) {
                    lenThumb = rcThumb.h;
                    len = box->rect.h - 2 * thickness;
                    value = y - thickness - scrollbar->thumb_press_offset;
               }
               else {
                    lenThumb = rcThumb.w;
                    len = box->rect.w - 2 * thickness;
                    value = x - thickness - scrollbar->thumb_press_offset;
               }

               if (value < 0)
                    value = 0;

               if (value > (len - lenThumb))
                    value = len - lenThumb;

               pos = scrollbar->info.min + (max - scrollbar->info.min) * value / (len - lenThumb);

               if (pos != scrollbar->info.track_pos) {
                    scrollbar->info.track_pos = pos;
                    scroll( scrollbar, SM_ABSOLUTE_POS, pos, true );
                    lite_update_box( box, NULL );
               }
          }
     }

     return 1;
}

typedef enum {
     SHA_OUTSIDE,
     SHA_BTN1,
     SHA_BTN2,
     SHA_THUMB,
     SHA_BETWEEN_THUMB_BTN1,
     SHA_BETWEEN_THUMB_BTN2,
     SAH_MAXVALUE
} ScrollbarHittestArea;

static ScrollbarHittestArea
scrollbar_hittest( LiteScrollbar *scrollbar,
                   int            x,
                   int            y )
{
     DFBRectangle rc, rcBtn1, rcBtn2, rcThumb;

     rc.x = 0;
     rc.y = 0;
     rc.w = scrollbar->box.rect.w;
     rc.h = scrollbar->box.rect.h;
     if (!DFB_RECTANGLE_CONTAINS_POINT( &rc, x,y ))
          return SHA_OUTSIDE;

     get_scrollbar_rect( scrollbar, SA_BTN1, &rcBtn1 );
     if (DFB_RECTANGLE_CONTAINS_POINT( &rcBtn1, x,y ))
          return SHA_BTN1;

     get_scrollbar_rect( scrollbar, SA_BTN2, &rcBtn2 );
     if (DFB_RECTANGLE_CONTAINS_POINT( &rcBtn2, x,y ))
          return SHA_BTN2;

     get_scrollbar_rect( scrollbar, SA_THUMB, &rcThumb );
     if (DFB_RECTANGLE_CONTAINS_POINT( &rcThumb, x,y ))
          return SHA_THUMB;

     if (scrollbar->vertical) {
          if (y < rcThumb.y)
               return SHA_BETWEEN_THUMB_BTN1;
          else
               return SHA_BETWEEN_THUMB_BTN2;
     }
     else {
          if (x < rcThumb.x)
               return SHA_BETWEEN_THUMB_BTN1;
          else
               return SHA_BETWEEN_THUMB_BTN2;
     }
}

static int
on_button_down( LiteBox                        *box,
                int                             x,
                int                             y,
                DFBInputDeviceButtonIdentifier  button_id )
{
     DFBRectangle          rcThumb;
     ScrollbarHittestArea  hittest;
     LiteScrollbar        *scrollbar = LITE_SCROLLBAR(box);

     D_ASSERT( box != NULL );

     hittest = scrollbar_hittest( scrollbar, x,y );

     switch (hittest) {
          case SHA_BTN1:
               scrollbar->state = SS_PRESSED_BTN1;
               break;

          case SHA_BTN2:
               scrollbar->state = SS_PRESSED_BTN2;
               break;

          case SHA_THUMB:
               scrollbar->state = SS_PRESSED_THUMB;
               get_scrollbar_rect( scrollbar, SA_THUMB, &rcThumb );
               if (scrollbar->vertical)
                    scrollbar->thumb_press_offset = y - rcThumb.y;
               else
                    scrollbar->thumb_press_offset = x - rcThumb.x;
               break;

          case SHA_BETWEEN_THUMB_BTN1:
               scroll( scrollbar, SM_PAGE, -1, false );
               break;

          case SHA_BETWEEN_THUMB_BTN2:
               scroll( scrollbar, SM_PAGE, 1, false );
               break;

          default:
               break;
     }


     lite_update_box( box, NULL );

     return 1;
}

static int
on_button_up( LiteBox                        *box,
              int                             x,
              int                             y,
              DFBInputDeviceButtonIdentifier  button_id )
{
     ScrollbarHittestArea  hittest;
     LiteScrollbar        *scrollbar = LITE_SCROLLBAR(box);

     D_ASSERT( box != NULL );

     hittest = scrollbar_hittest( scrollbar, x, y );

     switch (scrollbar->state) {
          case SS_PRESSED_BTN1:
               if (hittest == SHA_BTN1)
                    scroll( scrollbar, SM_LINE, -1, false );
               break;

          case SS_PRESSED_BTN2:
               if (hittest == SHA_BTN2)
                    scroll( scrollbar, SM_LINE, 1, false );
               break;

          case SS_PRESSED_THUMB:
               if (scrollbar->info.track_pos != -1 && scrollbar->info.track_pos != scrollbar->info.pos)
                    scroll( scrollbar, SM_ABSOLUTE_POS, scrollbar->info.track_pos, false );
               break;

          default:
               break;
     }

     if (hittest == SHA_OUTSIDE)
          scrollbar->state = SS_NORMAL;
     else
          scrollbar->state = SS_HILITE;

     lite_update_box( box, NULL );

     return 1;
}

static DFBResult
draw_scrollbar( LiteBox         *box,
                const DFBRegion *region,
                DFBBoolean       clear )
{
     DFBRectangle      rc, rcBtn1, rcBtn2, rcThumb;
     DFBRectangle      imgRcBack, imgRcBtn1, imgRcBtn2, imgRcThumb;
     int               thickness;
     IDirectFBSurface *surface;
     LiteScrollbar    *scrollbar = LITE_SCROLLBAR(box);

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteScrollbarDomain, "Draw scrollbar: %p (vertical:%d, info:(%u,%u,%u),(%d,%d), state:%u, clear:%u)\n",
                 scrollbar, scrollbar->vertical, scrollbar->info.min, scrollbar->info.page_size, scrollbar->info.max,
                 scrollbar->info.pos, scrollbar->info.track_pos, scrollbar->state, clear );

     if (clear)
          lite_clear_box( box, region );

     surface->SetClip( surface, region );

     lite_get_scrollbar_thickness( scrollbar, &thickness );

     /* draw background */
     rc.x = 0;
     rc.y = 0;
     rc.w = box->rect.w;
     rc.h = box->rect.h;

     imgRcBack.x = thickness * 3;
     imgRcBack.y = thickness * 2;
     imgRcBack.w = thickness;
     imgRcBack.h = thickness;
     if (!scrollbar->vertical)
          imgRcBack.x += thickness * 4;

     if (scrollbar->all_images.surface ||
         (scrollbar->theme != liteNoScrollbarTheme && scrollbar->theme->all_images.surface))
          surface->StretchBlit( surface, scrollbar->all_images.surface ?: scrollbar->theme->all_images.surface,
                                &imgRcBack, &rc );

     /* draw btn1 */
     get_scrollbar_rect( scrollbar, SA_BTN1, &rcBtn1 );

     imgRcBtn1.x = 0;
     imgRcBtn1.y = 0;
     imgRcBtn1.w = thickness;
     imgRcBtn1.h = thickness;
     if (!scrollbar->vertical)
          imgRcBtn1.x += thickness * 4;
     if (scrollbar->state == SS_DISABLED            ||
         scrollbar->info.max <= scrollbar->info.min ||
         scrollbar->info.page_size >= (scrollbar->info.max - scrollbar->info.min))
          imgRcBtn1.x += thickness * 3;
     else if (scrollbar->state == SS_NORMAL)
          imgRcBtn1.x += 0;
     else if (scrollbar->state == SS_PRESSED_BTN1)
          imgRcBtn1.x += thickness * 2;
     else
          imgRcBtn1.x += thickness;

     if (scrollbar->all_images.surface ||
         (scrollbar->theme != liteNoScrollbarTheme && scrollbar->theme->all_images.surface))
          surface->StretchBlit( surface, scrollbar->all_images.surface ?: scrollbar->theme->all_images.surface,
                                &imgRcBtn1, &rcBtn1 );

     /* draw btn2 */
     get_scrollbar_rect( scrollbar, SA_BTN2, &rcBtn2 );

     imgRcBtn2.x = 0;
     imgRcBtn2.y = thickness;
     imgRcBtn2.w = thickness;
     imgRcBtn2.h = thickness;
     if (!scrollbar->vertical)
          imgRcBtn2.x += thickness * 4;
     if (scrollbar->state == SS_DISABLED            ||
         scrollbar->info.max <= scrollbar->info.min ||
         scrollbar->info.page_size >= (scrollbar->info.max - scrollbar->info.min))
          imgRcBtn2.x += thickness * 3;
     else if (scrollbar->state == SS_NORMAL)
          imgRcBtn2.x += 0;
     else if (scrollbar->state == SS_PRESSED_BTN2)
          imgRcBtn2.x += thickness * 2;
     else
          imgRcBtn2.x += thickness;

     if (scrollbar->all_images.surface ||
         (scrollbar->theme != liteNoScrollbarTheme && scrollbar->theme->all_images.surface))
          surface->StretchBlit( surface, scrollbar->all_images.surface ?: scrollbar->theme->all_images.surface,
                                &imgRcBtn2, &rcBtn2 );

     /* draw thumb */
     if (scrollbar->state != SS_DISABLED           &&
         scrollbar->info.max > scrollbar->info.min &&
         scrollbar->info.page_size < (scrollbar->info.max - scrollbar->info.min)) {
          int          i, image_margin;
          DFBRectangle rect_dst[9], rect_img[9];

          get_scrollbar_rect( scrollbar, SA_THUMB, &rcThumb );

          imgRcThumb.x = 0;
          imgRcThumb.y = thickness * 2;
          imgRcThumb.w = thickness;
          imgRcThumb.h = thickness;
          if (!scrollbar->vertical)
               imgRcThumb.x += thickness * 4;
          if (scrollbar->state == SS_NORMAL)
               imgRcThumb.x += 0;
          else if (scrollbar->state == SS_PRESSED_THUMB)
               imgRcThumb.x += thickness * 2;
          else
               imgRcThumb.x += thickness;

          if (!scrollbar->all_images.surface && scrollbar->theme != liteNoScrollbarTheme)
               image_margin = scrollbar->theme->image_margin;
          else
               image_margin = scrollbar->image_margin;

          /*
             +---+------------------------------------------------+---+
             | 0 |                       1                        | 2 |
             +---+------------------------------------------------+---+
             |   |                                                |   |
             |   |                                                |   |
             | 3 |                       4                        | 5 |
             |   |                                                |   |
             +---+------------------------------------------------+---+
             | 6 |                       7                        | 8 |
             +---+------------------------------------------------+---+
          */

          /* left column */
          rect_dst[0].x = rect_dst[3].x = rect_dst[6].x = 0;
          rect_dst[0].w = rect_dst[3].w = rect_dst[6].w = image_margin;
          rect_img[0].x = rect_img[3].x = rect_img[6].x = 0;
          rect_img[0].w = rect_img[3].w = rect_img[6].w = image_margin;

          /* center column */
          rect_dst[1].x = rect_dst[4].x = rect_dst[7].x = image_margin;
          rect_dst[1].w = rect_dst[4].w = rect_dst[7].w = rcThumb.w - 2 * image_margin;
          rect_img[1].x = rect_img[4].x = rect_img[7].x = image_margin;
          rect_img[1].w = rect_img[4].w = rect_img[7].w = imgRcThumb.w - 2 * image_margin;

          /* right column */
          rect_dst[2].x = rect_dst[5].x = rect_dst[8].x = rcThumb.w - image_margin;
          rect_dst[2].w = rect_dst[5].w = rect_dst[8].w = image_margin;
          rect_img[2].x = rect_img[5].x = rect_img[8].x = imgRcThumb.w - image_margin;
          rect_img[2].w = rect_img[5].w = rect_img[8].w = image_margin;

          /* top row */
          rect_dst[0].y = rect_dst[1].y = rect_dst[2].y = 0;
          rect_dst[0].h = rect_dst[1].h = rect_dst[2].h = image_margin;
          rect_img[0].y = rect_img[1].y = rect_img[2].y = 0;
          rect_img[0].h = rect_img[1].h = rect_img[2].h = image_margin;

          /* center row */
          rect_dst[3].y = rect_dst[4].y = rect_dst[5].y = image_margin;
          rect_dst[3].h = rect_dst[4].h = rect_dst[5].h = rcThumb.h - 2 * image_margin;
          rect_img[3].y = rect_img[4].y = rect_img[5].y = image_margin;
          rect_img[3].h = rect_img[4].h = rect_img[5].h = imgRcThumb.h - 2 * image_margin;

          /* bottom row */
          rect_dst[6].y = rect_dst[7].y = rect_dst[8].y = rcThumb.h - image_margin;
          rect_dst[6].h = rect_dst[7].h = rect_dst[8].h = image_margin;
          rect_img[6].y = rect_img[7].y = rect_img[8].y = imgRcThumb.h - image_margin;
          rect_img[6].h = rect_img[7].h = rect_img[8].h = image_margin;

          for (i = 0; i < D_ARRAY_SIZE(rect_dst); ++i) {
               rect_dst[i].x += rcThumb.x;
               rect_dst[i].y += rcThumb.y;
               rect_img[i].x += imgRcThumb.x;
               rect_img[i].y += imgRcThumb.y;
          }

          for (i = 0; i < D_ARRAY_SIZE(rect_dst); ++i) {
               if (scrollbar->all_images.surface ||
                   (scrollbar->theme != liteNoScrollbarTheme && scrollbar->theme->all_images.surface))
                    surface->StretchBlit( surface, scrollbar->all_images.surface ?: scrollbar->theme->all_images.surface,
                                          &rect_img[i], &rect_dst[i] );
          }
     }

     return DFB_OK;
}

static DFBResult
destroy_scrollbar( LiteBox *box )
{
     LiteScrollbar *scrollbar = LITE_SCROLLBAR(box);

     D_ASSERT( box != NULL );

     D_DEBUG_AT( LiteScrollbarDomain, "Destroy scrollbar: %p\n", scrollbar );

     if (scrollbar->all_images.surface)
          scrollbar->all_images.surface->Release( scrollbar->all_images.surface );

     return lite_destroy_box( box );
}
