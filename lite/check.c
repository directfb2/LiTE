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
#include <lite/check.h>
#include <lite/font.h>
#include <lite/lite_internal.h>

D_DEBUG_DOMAIN( LiteCheckDomain, "LiTE/Check", "LiTE Check" );

/**********************************************************************************************************************/

LiteCheckTheme *liteDefaultCheckTheme = NULL;

struct _LiteCheck {
     LiteBox                box;
     LiteCheckTheme        *theme;

     LiteFont              *font;
     char                  *caption_text;
     int                    hilite;
     int                    enabled;
     LiteCheckState         state;
     struct {
          IDirectFBSurface *surface;
          int               width;
          int               height;
     } all_images;

     LiteCheckPressFunc     press;
     void                  *press_data;
};

static int       on_enter     ( LiteBox *box, int x, int y );
static int       on_leave     ( LiteBox *box, int x, int y );
static int       on_button_up ( LiteBox *box, int x, int y, DFBInputDeviceButtonIdentifier button_id );
static DFBResult draw_check   ( LiteBox *box, const DFBRegion *region, DFBBoolean clear );
static DFBResult destroy_check( LiteBox *box );

#define MARKER_CAPTION_GAP 6

/**********************************************************************************************************************/

DFBResult
lite_new_check( LiteBox         *parent,
                DFBRectangle    *rect,
                const char      *caption_text,
                LiteCheckTheme  *theme,
                LiteCheck      **ret_check )
{
     DFBResult  ret;
     LiteCheck *check;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_check );

     if (caption_text == NULL)
          caption_text = "";

     check = D_CALLOC( 1, sizeof(LiteCheck) );

     check->box.parent = parent;
     check->box.rect   = *rect;
     check->theme      = theme;

     ret = lite_init_box( LITE_BOX(check) );
     if (ret != DFB_OK) {
          D_FREE( check );
          return ret;
     }

     ret = lite_get_font( "default", LITE_FONT_PLAIN, 13, DEFAULT_FONT_ATTRIBUTE, &check->font );
     if (ret != DFB_OK) {
          D_FREE( check );
          return ret;
     }

     check->box.type       = LITE_TYPE_CHECK;
     check->box.OnEnter    = on_enter;
     check->box.OnLeave    = on_leave;
     check->box.OnButtonUp = on_button_up;
     check->box.Draw       = draw_check;
     check->box.Destroy    = destroy_check;
     check->caption_text   = D_STRDUP( caption_text );
     check->enabled        = 1;
     check->state          = LITE_CHS_UNCHECKED;

     *ret_check = check;

     D_DEBUG_AT( LiteCheckDomain, "Created new check object: %p\n", check );

     return DFB_OK;
}

DFBResult
lite_set_check_caption( LiteCheck  *check,
                        const char *caption_text )
{
     LITE_NULL_PARAMETER_CHECK( check );
     LITE_NULL_PARAMETER_CHECK( caption_text );
     LITE_BOX_TYPE_PARAMETER_CHECK( check, LITE_TYPE_CHECK );

     D_DEBUG_AT( LiteCheckDomain, "Set check: %p with caption text: %s\n", check, caption_text );

     if (!strcmp( check->caption_text, caption_text ))
          return DFB_OK;

     D_FREE( check->caption_text );

     check->caption_text = D_STRDUP( caption_text );

     return lite_update_box( LITE_BOX(check), NULL );
}

DFBResult
lite_enable_check( LiteCheck *check,
                   int        enabled )
{
     LITE_NULL_PARAMETER_CHECK( check );
     LITE_BOX_TYPE_PARAMETER_CHECK( check, LITE_TYPE_CHECK );

     D_DEBUG_AT( LiteCheckDomain, "%s check: %p\n", enabled ? "Enable" : "Disable", check );

     if (check->enabled == enabled)
          return DFB_OK;

     check->enabled = enabled;

     return lite_update_box( LITE_BOX(check), NULL );
}

DFBResult
lite_check_check( LiteCheck      *check,
                  LiteCheckState  state )
{
     LITE_NULL_PARAMETER_CHECK( check );
     LITE_BOX_TYPE_PARAMETER_CHECK( check, LITE_TYPE_CHECK );

     /* check for valid state */
     switch (state) {
          case LITE_CHS_UNCHECKED:
          case LITE_CHS_CHECKED:
               break;
          default:
               return DFB_INVARG;
     }

     D_DEBUG_AT( LiteCheckDomain, "Set check: %p %s\n", check, state ? "checked" : "unchecked" );

     if (check->state == state)
          return DFB_OK;

     check->state = state;

     return lite_update_box( LITE_BOX(check), NULL );
}

DFBResult
lite_get_check_state( LiteCheck      *check,
                      LiteCheckState *ret_state )
{
     LITE_NULL_PARAMETER_CHECK( check );
     LITE_NULL_PARAMETER_CHECK( ret_state );
     LITE_BOX_TYPE_PARAMETER_CHECK( check, LITE_TYPE_CHECK );

     D_DEBUG_AT( LiteCheckDomain, "check: %p is in state: %u\n", check, check->state );

     *ret_state = check->state;

     return DFB_OK;
}

DFBResult
lite_set_check_all_images( LiteCheck  *check,
                           const char *image_path )
{
     LITE_NULL_PARAMETER_CHECK( check );
     LITE_BOX_TYPE_PARAMETER_CHECK( check, LITE_TYPE_CHECK );

     D_DEBUG_AT( LiteCheckDomain, "Set check: %p with image: %s for all states\n", check, image_path );

     if (image_path) {
          DFBResult         ret;
          int               all_images_width, all_images_height;
          IDirectFBSurface *all_images_surface;

          ret = prvlite_load_image( image_path, &all_images_surface, &all_images_width, &all_images_height, NULL );
          if (ret != DFB_OK)
               return ret;

          if (check->all_images.surface)
               check->all_images.surface->Release( check->all_images.surface );

          check->all_images.surface = all_images_surface;
          check->all_images.width   = all_images_width;
          check->all_images.height  = all_images_height;
     }
     else if (check->all_images.surface) {
          check->all_images.surface->Release( check->all_images.surface );
          check->all_images.surface = NULL;
          check->all_images.width   = 0;
          check->all_images.height  = 0;
     }

     return lite_update_box( LITE_BOX(check), NULL );
}

DFBResult
lite_on_check_press( LiteCheck          *check,
                     LiteCheckPressFunc  callback,
                     void               *data )
{
     LITE_NULL_PARAMETER_CHECK( check );
     LITE_BOX_TYPE_PARAMETER_CHECK( check, LITE_TYPE_CHECK );

     D_DEBUG_AT( LiteCheckDomain, "Install callback %p( %p, %p )\n", callback, check, data );

     check->press      = callback;
     check->press_data = data;

     return DFB_OK;
}

DFBResult
lite_new_check_theme( const char      *image_path,
                      LiteCheckTheme **ret_theme )
{
     DFBResult       ret;
     LiteCheckTheme *theme;

     LITE_NULL_PARAMETER_CHECK( image_path );
     LITE_NULL_PARAMETER_CHECK( ret_theme );

     if (liteDefaultCheckTheme && *ret_theme == liteDefaultCheckTheme)
          return DFB_OK;

     theme = D_CALLOC( 1, sizeof(LiteCheckTheme) );

     ret = prvlite_load_image( image_path, &theme->all_images.surface,
                               &theme->all_images.width, &theme->all_images.height, NULL );
     if (ret != DFB_OK) {
          D_FREE( theme );
          return ret;
     }

     *ret_theme = theme;

     D_DEBUG_AT( LiteCheckDomain, "Created new check theme: %p\n", theme );

     return DFB_OK;
}

DFBResult
lite_destroy_check_theme( LiteCheckTheme *theme )
{
     LITE_NULL_PARAMETER_CHECK( theme );

     D_DEBUG_AT( LiteCheckDomain, "Destroy check theme: %p\n", theme );

     theme->all_images.surface->Release( theme->all_images.surface );

     D_FREE( theme );

     if (theme == liteDefaultCheckTheme)
          liteDefaultCheckTheme = NULL;

     return DFB_OK;
}

/* internals */

static int
on_enter( LiteBox *box,
          int      x,
          int      y )
{
     LiteCheck *check = LITE_CHECK(box);

     D_ASSERT( box != NULL );

     if (!check->hilite) {
          check->hilite = 1;
          lite_update_box( box, NULL );
     }

     return 1;
}

static int
on_leave( LiteBox *box,
          int      x,
          int      y )
{
     LiteCheck *check = LITE_CHECK(box);

     D_ASSERT( box != NULL );

     if (check->hilite) {
          check->hilite = 0;
          lite_update_box( box, NULL );
     }

     return 1;
}

static int
on_button_up( LiteBox                        *box,
              int                             x,
              int                             y,
              DFBInputDeviceButtonIdentifier  button_id )
{
     LiteCheck *check = LITE_CHECK(box);

     D_ASSERT( box != NULL );

     /* check that button up occurs within the bounds of the widget */
     if (x >= 0 && x < box->rect.w && y >= 0 && y < box->rect.h) {
          if (check->enabled) {
               if (check->state == LITE_CHS_UNCHECKED)
                    check->state = LITE_CHS_CHECKED;
               else
                    check->state = LITE_CHS_UNCHECKED;

               lite_update_box( box, NULL );

               if (check->press)
                    check->press( check, check->state, check->press_data );
          }
     }

     return 1;
}

static DFBResult
draw_check( LiteBox         *box,
            const DFBRegion *region,
            DFBBoolean       clear )
{
     DFBRectangle      rect_image;
     int               x, y;
     IDirectFBSurface *surface;
     int               all_images_width  = 0;
     int               all_images_height = 0;
     LiteCheck        *check             = LITE_CHECK(box);

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteCheckDomain, "Draw check: %p (enabled:%d, hilite:%d, state:%u, clear:%u)\n",
                 check, check->enabled, check->hilite, check->state, clear );

     if (clear)
          lite_clear_box( box, region );

     surface->SetClip( surface, NULL );

     surface->SetBlittingFlags( surface, DSBLIT_BLEND_ALPHACHANNEL );

     if ((check->all_images.width && check->all_images.height) || check->theme != liteNoCheckTheme) {
          all_images_width  = check->all_images.width  ?: check->theme->all_images.width;
          all_images_height = check->all_images.height ?: check->theme->all_images.height;
     }

     /* draw marker area */
     rect_image.x = 0;
     rect_image.y = 0;
     rect_image.w = all_images_width / 6;
     rect_image.h = all_images_height;
     if (check->state == LITE_CHS_CHECKED)
          rect_image.x += all_images_width / 2;
     if (!check->enabled)
          rect_image.x += all_images_width / 3;
     else if (check->hilite)
          rect_image.x += all_images_width / 6;

     x = 0;
     y = 0;

     if (rect_image.h < box->rect.h)
          y = (box->rect.h - rect_image.h) / 2;

     if (check->all_images.surface || (check->theme != liteNoCheckTheme && check->theme->all_images.surface))
          surface->Blit( surface, check->all_images.surface ?: check->theme->all_images.surface, &rect_image, x, y );

     /* draw caption area */
     if (check->caption_text) {
          DFBResult      ret;
          char           truncated_text[64];
          int            font_height;
          IDirectFBFont *font;

          ret = lite_font( check->font, &font );
          if (ret != DFB_OK)
               return ret;

          surface->SetFont( surface, font );

          strncpy( truncated_text, check->caption_text, sizeof(truncated_text) );
          truncated_text[sizeof(truncated_text)-1] = 0;
          prvlite_make_truncated_text( truncated_text, box->rect.w - (rect_image.w + MARKER_CAPTION_GAP), font );

          font->GetHeight( font, &font_height );

          x = rect_image.w + MARKER_CAPTION_GAP;
          y = (box->rect.h - font_height) / 2;

          surface->DrawString( surface, truncated_text, -1, x, y, DSTF_LEFT | DSTF_TOP );
     }

     return DFB_OK;
}

static DFBResult
destroy_check( LiteBox *box )
{
     LiteCheck *check = LITE_CHECK(box);

     D_ASSERT( box != NULL );

     D_DEBUG_AT( LiteCheckDomain, "Destroy check: %p\n", check );

     D_FREE( check->caption_text );

     lite_release_font( check->font );

     if (check->all_images.surface)
          check->all_images.surface->Release( check->all_images.surface );

     return lite_destroy_box( box );
}
