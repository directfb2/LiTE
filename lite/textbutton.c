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
#include <lite/font.h>
#include <lite/lite_internal.h>
#include <lite/textbutton.h>

D_DEBUG_DOMAIN( LiteTextButtonDomain, "LiTE/TextButton", "LiTE TextButton" );

/**********************************************************************************************************************/

LiteTextButtonTheme *liteDefaultTextButtonTheme = NULL;

struct _LiteTextButton {
     LiteBox                  box;
     LiteTextButtonTheme     *theme;

     LiteFont                *font;
     char                    *caption_text;
     int                      enabled;
     LiteTextButtonState      state;
     struct {
          IDirectFBSurface   *surface;
          int                 width;
          int                 height;
     } all_images;

     LiteTextButtonPressFunc  press;
     void                    *press_data;
};

static int       on_enter           ( LiteBox *box, int x, int y );
static int       on_leave           ( LiteBox *box, int x, int y );
static int       on_button_down     ( LiteBox *box, int x, int y, DFBInputDeviceButtonIdentifier button_id );
static int       on_button_up       ( LiteBox *box, int x, int y, DFBInputDeviceButtonIdentifier button_id );
static DFBResult draw_text_button   ( LiteBox *box, const DFBRegion *region, DFBBoolean clear );
static DFBResult destroy_text_button( LiteBox *box );

#define IMG_MARGIN 4

/**********************************************************************************************************************/

DFBResult
lite_new_text_button( LiteBox              *parent,
                      DFBRectangle         *rect,
                      const char           *caption_text,
                      LiteTextButtonTheme  *theme,
                      LiteTextButton      **ret_textbutton )
{
     DFBResult       ret;
     LiteTextButton *textbutton;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_textbutton );

     if( caption_text == NULL )
          caption_text = "";

     textbutton = D_CALLOC( 1, sizeof(LiteTextButton) );

     textbutton->box.parent = parent;
     textbutton->box.rect   = *rect;
     textbutton->theme      = theme;

     ret = lite_init_box( LITE_BOX(textbutton) );
     if (ret != DFB_OK) {
          D_FREE( textbutton );
          return ret;
     }

     ret = lite_get_font( "default", LITE_FONT_PLAIN, 13, DEFAULT_FONT_ATTRIBUTE, &textbutton->font );
     if (ret != DFB_OK) {
          D_FREE( textbutton );
          return ret;
     }

     textbutton->box.type         = LITE_TYPE_TEXT_BUTTON;
     textbutton->box.OnEnter      = on_enter;
     textbutton->box.OnLeave      = on_leave;
     textbutton->box.OnButtonDown = on_button_down;
     textbutton->box.OnButtonUp   = on_button_up;
     textbutton->box.Draw         = draw_text_button;
     textbutton->box.Destroy      = destroy_text_button;
     textbutton->caption_text     = D_STRDUP( caption_text );
     textbutton->enabled          = 1;
     textbutton->state            = LITE_TBS_NORMAL;

     *ret_textbutton = textbutton;

     D_DEBUG_AT( LiteTextButtonDomain, "Created new textbutton object: %p\n", textbutton );

     return DFB_OK;
}

DFBResult
lite_set_text_button_caption( LiteTextButton *textbutton,
                              const char     *caption_text )
{
     LITE_NULL_PARAMETER_CHECK( textbutton );
     LITE_NULL_PARAMETER_CHECK( caption_text );
     LITE_BOX_TYPE_PARAMETER_CHECK( textbutton, LITE_TYPE_TEXT_BUTTON );

     D_DEBUG_AT( LiteTextButtonDomain, "Set textbutton: %p with caption text: %s\n", textbutton, caption_text );

     if (!strcmp( textbutton->caption_text, caption_text ))
          return DFB_OK;

     textbutton->caption_text = D_STRDUP( caption_text );

     return lite_update_box( LITE_BOX(textbutton), NULL );
}

DFBResult
lite_enable_text_button( LiteTextButton *textbutton,
                         int             enabled )
{
     LITE_NULL_PARAMETER_CHECK( textbutton );
     LITE_BOX_TYPE_PARAMETER_CHECK( textbutton, LITE_TYPE_TEXT_BUTTON );

     D_DEBUG_AT( LiteTextButtonDomain, "%s textbutton: %p\n", enabled ? "Enable" : "Disable", textbutton );

     if (textbutton->enabled == enabled)
          return DFB_OK;

     textbutton->enabled = enabled;

     return lite_update_box( LITE_BOX(textbutton), NULL );
}

DFBResult
lite_set_text_button_state( LiteTextButton      *textbutton,
                            LiteTextButtonState  state )
{
     LITE_NULL_PARAMETER_CHECK( textbutton );
     LITE_BOX_TYPE_PARAMETER_CHECK( textbutton, LITE_TYPE_TEXT_BUTTON );

     /* check for valid state */
     if (state >= LITE_TBS_MAX)
          return DFB_INVARG;

     D_DEBUG_AT( LiteTextButtonDomain, "Set textbutton: %p to state %u\n", textbutton, state );

     if (textbutton->state == state)
          return DFB_OK;

     textbutton->state = state;

     if (textbutton->enabled) {
          return lite_update_box( LITE_BOX(textbutton), NULL );
     }

     return DFB_OK;
}

DFBResult
lite_get_text_button_state( LiteTextButton      *textbutton,
                            LiteTextButtonState *ret_state )
{
     LiteTextButtonState state;

     LITE_NULL_PARAMETER_CHECK( textbutton );
     LITE_NULL_PARAMETER_CHECK( ret_state );
     LITE_BOX_TYPE_PARAMETER_CHECK( textbutton, LITE_TYPE_TEXT_BUTTON );

     D_DEBUG_AT( LiteTextButtonDomain, "textbutton: %p is in state: %u\n", textbutton, textbutton->state );

     if (textbutton->enabled)
          state = textbutton->state;
     else
          state = LITE_TBS_DISABLED;

     *ret_state = state;

     return DFB_OK;
}

DFBResult
lite_set_text_button_all_images( LiteTextButton *textbutton,
                                 const char     *image_path )
{
     LITE_NULL_PARAMETER_CHECK( textbutton );
     LITE_BOX_TYPE_PARAMETER_CHECK( textbutton, LITE_TYPE_TEXT_BUTTON );

     D_DEBUG_AT( LiteTextButtonDomain, "Set textbutton: %p with image: %s for all states\n", textbutton, image_path );

     if (image_path) {
          DFBResult         ret;
          int               all_images_width, all_images_height;
          IDirectFBSurface *all_images_surface;

          ret = prvlite_load_image( image_path, &all_images_surface, &all_images_width, &all_images_height, NULL );
          if (ret != DFB_OK)
               return ret;

          if (textbutton->all_images.surface)
               textbutton->all_images.surface->Release( textbutton->all_images.surface );

          textbutton->all_images.surface = all_images_surface;
          textbutton->all_images.width   = all_images_width;
          textbutton->all_images.height  = all_images_height;
     }
     else if (textbutton->all_images.surface) {
          textbutton->all_images.surface->Release( textbutton->all_images.surface );
          textbutton->all_images.surface = NULL;
          textbutton->all_images.width   = 0;
          textbutton->all_images.height  = 0;
     }

     return lite_update_box( LITE_BOX(textbutton), NULL );
}

DFBResult
lite_on_text_button_press( LiteTextButton          *textbutton,
                           LiteTextButtonPressFunc  callback,
                           void                    *data )
{
     LITE_NULL_PARAMETER_CHECK( textbutton );
     LITE_BOX_TYPE_PARAMETER_CHECK( textbutton, LITE_TYPE_TEXT_BUTTON );

     D_DEBUG_AT( LiteTextButtonDomain, "Install callback %p( %p, %p )\n", callback, textbutton, data );

     textbutton->press      = callback;
     textbutton->press_data = data;

     return DFB_OK;
}

DFBResult
lite_new_text_button_theme( const char           *image_path,
                            LiteTextButtonTheme **ret_theme )
{
     DFBResult            ret;
     LiteTextButtonTheme *theme;

     LITE_NULL_PARAMETER_CHECK( image_path );
     LITE_NULL_PARAMETER_CHECK( ret_theme );

     if (liteDefaultTextButtonTheme && *ret_theme == liteDefaultTextButtonTheme)
          return DFB_OK;

     theme = D_CALLOC(1, sizeof(LiteTextButtonTheme));

     ret = prvlite_load_image( image_path, &theme->all_images.surface,
                               &theme->all_images.width, &theme->all_images.height, NULL );
     if (ret != DFB_OK) {
          D_FREE( theme );
          return ret;
     }

     *ret_theme = theme;

     D_DEBUG_AT( LiteTextButtonDomain, "Created new text button theme: %p\n", theme );

     return DFB_OK;
}

DFBResult
lite_destroy_text_button_theme( LiteTextButtonTheme *theme )
{
     LITE_NULL_PARAMETER_CHECK( theme );

     D_DEBUG_AT( LiteTextButtonDomain, "Destroy text button theme: %p\n", theme );

     theme->all_images.surface->Release( theme->all_images.surface );

     D_FREE( theme );

     if (theme == liteDefaultTextButtonTheme)
          liteDefaultTextButtonTheme = NULL;

     return DFB_OK;
}

/* internals */

static int
on_enter( LiteBox *box,
          int      x,
          int      y )
{
     LiteTextButton *textbutton = LITE_TEXTBUTTON(box);

     D_ASSERT( box != NULL );

     lite_set_text_button_state( textbutton, LITE_TBS_HILITE );

     return 1;
}

static int
on_leave( LiteBox *box,
          int      x,
          int      y )
{
     LiteTextButton *textbutton = LITE_TEXTBUTTON(box);

     D_ASSERT( box != NULL );

     lite_set_text_button_state( textbutton, LITE_TBS_NORMAL );

     return 1;
}

static int
on_button_down( LiteBox                        *box,
                int                             x,
                int                             y,
                DFBInputDeviceButtonIdentifier  button_id )
{
     LiteTextButton *textbutton = LITE_TEXTBUTTON(box);

     D_ASSERT( box != NULL );

     lite_set_text_button_state( textbutton, LITE_TBS_PRESSED );

     return 1;
}

static int
on_button_up( LiteBox                        *box,
              int                             x,
              int                             y,
              DFBInputDeviceButtonIdentifier  button_id )
{
     LiteTextButton *textbutton = LITE_TEXTBUTTON(box);

     D_ASSERT( box != NULL );

     /* check that button up occurs within the bounds of the widget */
     if (x >= 0 && x < box->rect.w && y >= 0 && y < box->rect.h) {
          lite_set_text_button_state( textbutton, LITE_TBS_HILITE );

          if (textbutton->enabled && textbutton->press)
               textbutton->press( textbutton, textbutton->press_data );
     }
     else
          lite_set_text_button_state( textbutton, LITE_TBS_NORMAL );

     return 1;
}

static DFBResult
draw_text_button( LiteBox         *box,
                  const DFBRegion *region,
                  DFBBoolean       clear )
{
     int               i;
     DFBRectangle      rcBtn;
     DFBRectangle      rect_image;
     DFBRectangle      rc_dst[9], rc_img[9];
     IDirectFBSurface *surface;
     int               all_images_width  = 0;
     int               all_images_height = 0;
     LiteTextButton   *textbutton        = LITE_TEXTBUTTON(box);

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteTextButtonDomain, "Draw textbutton: %p (enabled:%d, state:%u, clear:%u)\n",
                 textbutton, textbutton->enabled, textbutton->state, clear );

     if (clear)
          lite_clear_box(box, region);

     surface->SetClip( surface, NULL );

     surface->SetBlittingFlags( surface, DSBLIT_BLEND_ALPHACHANNEL );

     if ((textbutton->all_images.width && textbutton->all_images.height) || textbutton->theme != liteNoTextButtonTheme) {
          all_images_width  = textbutton->all_images.width  ?: textbutton->theme->all_images.width;
          all_images_height = textbutton->all_images.height ?: textbutton->theme->all_images.height;
     }

     /* draw btn area */
     rcBtn.x = 0;
     rcBtn.y = 0;
     rcBtn.w = box->rect.w;
     rcBtn.h = box->rect.h;

     rect_image.x = 0;
     rect_image.y = 0;
     rect_image.w = all_images_width;
     rect_image.h = all_images_height / 4;
     if (!textbutton->enabled)
          rect_image.y = rect_image.h * 3;
     else if (textbutton->state == LITE_TBS_HILITE)
          rect_image.y = rect_image.h * 2;
     else if (textbutton->state == LITE_TBS_PRESSED)
          rect_image.y = rect_image.h;

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
     rc_dst[0].x = rc_dst[3].x = rc_dst[6].x = 0;
     rc_dst[0].w = rc_dst[3].w = rc_dst[6].w = IMG_MARGIN;
     rc_img[0].x = rc_img[3].x = rc_img[6].x = 0;
     rc_img[0].w = rc_img[3].w = rc_img[6].w = IMG_MARGIN;

     /* center column */
     rc_dst[1].x = rc_dst[4].x = rc_dst[7].x = IMG_MARGIN;
     rc_dst[1].w = rc_dst[4].w = rc_dst[7].w = rcBtn.w - 2 * IMG_MARGIN;
     rc_img[1].x = rc_img[4].x = rc_img[7].x = IMG_MARGIN;
     rc_img[1].w = rc_img[4].w = rc_img[7].w = rect_image.w - 2 * IMG_MARGIN;

     /* right column */
     rc_dst[2].x = rc_dst[5].x = rc_dst[8].x = rcBtn.w - IMG_MARGIN;
     rc_dst[2].w = rc_dst[5].w = rc_dst[8].w = IMG_MARGIN;
     rc_img[2].x = rc_img[5].x = rc_img[8].x = rect_image.w - IMG_MARGIN;
     rc_img[2].w = rc_img[5].w = rc_img[8].w = IMG_MARGIN;

     /* top row */
     rc_dst[0].y = rc_dst[1].y = rc_dst[2].y = 0;
     rc_dst[0].h = rc_dst[1].h = rc_dst[2].h = IMG_MARGIN;
     rc_img[0].y = rc_img[1].y = rc_img[2].y = 0;
     rc_img[0].h = rc_img[1].h = rc_img[2].h = IMG_MARGIN;

     /* center row */
     rc_dst[3].y = rc_dst[4].y = rc_dst[5].y = IMG_MARGIN;
     rc_dst[3].h = rc_dst[4].h = rc_dst[5].h = rcBtn.h - 2 * IMG_MARGIN;
     rc_img[3].y = rc_img[4].y = rc_img[5].y = IMG_MARGIN;
     rc_img[3].h = rc_img[4].h = rc_img[5].h = rect_image.h - 2 * IMG_MARGIN;

     /* bottom row */
     rc_dst[6].y = rc_dst[7].y = rc_dst[8].y = rcBtn.h - IMG_MARGIN;
     rc_dst[6].h = rc_dst[7].h = rc_dst[8].h = IMG_MARGIN;
     rc_img[6].y = rc_img[7].y = rc_img[8].y = rect_image.h - IMG_MARGIN;
     rc_img[6].h = rc_img[7].h = rc_img[8].h = IMG_MARGIN;

     for (i = 0; i < D_ARRAY_SIZE(rc_dst); ++i) {
          rc_img[i].y += rect_image.y;
     }

     for (i = 0; i < D_ARRAY_SIZE(rc_dst); ++i) {
          if (textbutton->all_images.surface ||
              (textbutton->theme != liteNoTextButtonTheme && textbutton->theme->all_images.surface))
               surface->StretchBlit( surface, textbutton->all_images.surface ?: textbutton->theme->all_images.surface,
                                     &rc_img[i], &rc_dst[i] );
     }

     /* draw caption area */
     if (textbutton->caption_text) {
          DFBResult      ret;
          char           truncated_text[64];
          int            font_height, x, y;
          IDirectFBFont *font;

          ret = lite_font( textbutton->font, &font );
          if (ret != DFB_OK)
               return ret;

          surface->SetFont( surface, font );

          strncpy( truncated_text, textbutton->caption_text, sizeof(truncated_text) );
          truncated_text[sizeof(truncated_text)-1] = 0;
          prvlite_make_truncated_text( truncated_text, box->rect.w - 2 * IMG_MARGIN, font );

          font->GetHeight( font, &font_height );

          x = box->rect.w / 2;
          y = (box->rect.h - font_height) / 2;

          surface->DrawString( surface, truncated_text, -1, x, y, DSTF_CENTER | DSTF_TOP );
     }

     return DFB_OK;
}

static DFBResult
destroy_text_button( LiteBox *box )
{
     LiteTextButton *textbutton = LITE_TEXTBUTTON(box);

     D_ASSERT( box != NULL );

     D_DEBUG_AT( LiteTextButtonDomain, "Destroy textbutton: %p\n", textbutton );

     D_FREE( textbutton->caption_text );

     lite_release_font( textbutton->font );

     if (textbutton->all_images.surface)
          textbutton->all_images.surface->Release( textbutton->all_images.surface );

     return lite_destroy_box(box);
}
