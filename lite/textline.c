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

#include <direct/memcpy.h>
#include <lite/font.h>
#include <lite/lite_internal.h>
#include <lite/textline.h>

D_DEBUG_DOMAIN( LiteTextLineDomain, "LiTE/TextLine", "LiTE TextLine" );

/**********************************************************************************************************************/

LiteTextLineTheme *liteDefaultTextLineTheme = NULL;

struct _LiteTextLine {
     LiteBox                box;
     LiteTextLineTheme     *theme;

     LiteFont              *font;
     char                  *text;
     char                  *backup;
     bool                   modified;
     unsigned int           cursor_pos;

     LiteTextLineEnterFunc  enter;
     void                  *enter_data;
     LiteTextLineAbortFunc  abort;
     void                  *abort_data;
};

static int       on_focus_in     ( LiteBox *box );
static int       on_focus_out    ( LiteBox *box );
static int       on_button_down  ( LiteBox *box, int x, int y, DFBInputDeviceButtonIdentifier button_id );
static int       on_key_down     ( LiteBox *box, DFBWindowEvent *evt );
static DFBResult draw_textline   ( LiteBox *box, const DFBRegion *region, DFBBoolean clear );
static DFBResult destroy_textline( LiteBox *box );

/**********************************************************************************************************************/

DFBResult
lite_new_textline( LiteBox            *parent,
                   DFBRectangle       *rect,
                   LiteTextLineTheme  *theme,
                   LiteTextLine      **ret_textline )
{
     DFBResult     ret;
     LiteTextLine *textline;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_textline );

     textline = D_CALLOC( 1, sizeof(LiteTextLine) );

     textline->box.parent = parent;
     textline->box.rect   = *rect;
     textline->theme      = theme;

     ret = lite_init_box( LITE_BOX(textline) );
     if (ret != DFB_OK) {
          D_FREE( textline );
          return ret;
     }

     ret = lite_get_font( "default", LITE_FONT_PLAIN, rect->h * 9 / 10 - 6, DEFAULT_FONT_ATTRIBUTE, &textline->font );
     if (ret != DFB_OK) {
          D_FREE( textline );
          return ret;
     }

     textline->box.type         = LITE_TYPE_TEXTLINE;
     textline->box.OnFocusIn    = on_focus_in;
     textline->box.OnFocusOut   = on_focus_out;
     textline->box.OnButtonDown = on_button_down;
     textline->box.OnKeyDown    = on_key_down;
     textline->box.Draw         = draw_textline;
     textline->box.Destroy      = destroy_textline;
     textline->text             = D_STRDUP( "" );

     *ret_textline = textline;

     D_DEBUG_AT( LiteTextLineDomain, "Created new textline object: %p\n", textline );

     return DFB_OK;
}

DFBResult
lite_set_textline_text( LiteTextLine *textline,
                        const char   *text )
{
     LITE_NULL_PARAMETER_CHECK( textline );
     LITE_NULL_PARAMETER_CHECK( text );
     LITE_BOX_TYPE_PARAMETER_CHECK( textline, LITE_TYPE_TEXTLINE );

     D_DEBUG_AT( LiteTextLineDomain, "Set textline: %p with text: %s\n", textline, text );

     if (textline->modified) {
          D_FREE( textline->backup );
          textline->modified = false;
     }

     if (!strcmp( textline->text, text ))
          return DFB_OK;

     D_FREE( textline->text );
     textline->text       = D_STRDUP( text );
     textline->cursor_pos = strlen( text );

     return lite_update_box( LITE_BOX(textline), NULL );
}

DFBResult
lite_get_textline_text( LiteTextLine  *textline,
                        char         **ret_text )
{
     LITE_NULL_PARAMETER_CHECK( textline );
     LITE_NULL_PARAMETER_CHECK( ret_text );
     LITE_BOX_TYPE_PARAMETER_CHECK( textline, LITE_TYPE_TEXTLINE );

     D_DEBUG_AT( LiteTextLineDomain, "textline: %p has text: %s\n", textline, textline->text );

     *ret_text = D_STRDUP( textline->text );

     return DFB_OK;
}

DFBResult
lite_on_textline_enter( LiteTextLine          *textline,
                        LiteTextLineEnterFunc  callback,
                        void                  *data )
{
     LITE_NULL_PARAMETER_CHECK( textline );
     LITE_BOX_TYPE_PARAMETER_CHECK( textline, LITE_TYPE_TEXTLINE );

     D_DEBUG_AT( LiteTextLineDomain, "Install callback %p( %p, %p )\n", callback, textline, data );

     textline->enter      = callback;
     textline->enter_data = data;

     return DFB_OK;
}

DFBResult
lite_on_textline_abort( LiteTextLine          *textline,
                        LiteTextLineAbortFunc  callback,
                        void                  *data )
{
     LITE_NULL_PARAMETER_CHECK( textline );
     LITE_BOX_TYPE_PARAMETER_CHECK( textline, LITE_TYPE_TEXTLINE );

     D_DEBUG_AT( LiteTextLineDomain, "Install callback %p( %p, %p )\n", callback, textline, data );

     textline->abort      = callback;
     textline->abort_data = data;

     return DFB_OK;
}

DFBResult
lite_new_textline_theme( const DFBColor     *bg_color,
                         const DFBColor     *bg_color_changed,
                         LiteTextLineTheme **ret_theme )
{
     LiteTextLineTheme *theme;

     LITE_NULL_PARAMETER_CHECK( bg_color );
     LITE_NULL_PARAMETER_CHECK( bg_color_changed );
     LITE_NULL_PARAMETER_CHECK( ret_theme );

     if (liteDefaultTextLineTheme && *ret_theme == liteDefaultTextLineTheme)
          return DFB_OK;

     theme = D_CALLOC( 1, sizeof(LiteTextLineTheme) );

     theme->theme.bg_color   = *bg_color;
     theme->bg_color_changed = *bg_color_changed;

     *ret_theme = theme;

     D_DEBUG_AT( LiteTextLineDomain, "Created new text line theme: %p\n", theme );

     return DFB_OK;
}

DFBResult
lite_destroy_textline_theme( LiteTextLineTheme *theme )
{
     LITE_NULL_PARAMETER_CHECK( theme );

     D_DEBUG_AT( LiteTextLineDomain, "Destroy text line theme: %p\n", theme );

     if (theme == liteDefaultTextLineTheme)
          liteDefaultTextLineTheme = NULL;

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
     LiteTextLine *textline = LITE_TEXTLINE(box);

     D_ASSERT( box != NULL );

     if (textline->modified) {
          if (textline->enter)
               textline->enter( textline->text, textline->enter_data );

          D_FREE( textline->backup );
          textline->modified   = false;
     }

     textline->cursor_pos = strlen( textline->text );

     lite_update_box( box, NULL );

     return 1;
}

static int
on_button_down( LiteBox                        *box,
                int                             x,
                int                             y,
                DFBInputDeviceButtonIdentifier  button_id )
{
     D_ASSERT( box != NULL );

     lite_focus_box( box );

     lite_update_box( box, NULL );

     return 1;
}

static void
set_modified( LiteTextLine *textline )
{
     if (textline->modified)
          return;

     textline->modified = true;

     textline->backup = D_STRDUP( textline->text );
}

static int
on_key_down( LiteBox        *box,
             DFBWindowEvent *evt )
{
     LiteTextLine *textline = LITE_TEXTLINE(box);

     D_ASSERT( box != NULL );

     switch (evt->key_symbol) {
          case DIKS_ENTER:
               if (textline->modified) {
                    if (textline->enter)
                         textline->enter( textline->text, textline->enter_data );

                    D_FREE( textline->backup );
                    textline->modified   = false;
                    textline->cursor_pos = 0;

                    lite_update_box( box, NULL );
               }
               break;

          case DIKS_ESCAPE:
               if (textline->abort)
                    textline->abort( textline->abort_data );
               if (textline->modified) {
                    D_FREE( textline->text );
                    textline->text       = textline->backup;
                    textline->modified   = false;
                    textline->cursor_pos = 0;

                    lite_update_box( box, NULL );
               }
               break;

          case DIKS_CURSOR_LEFT:
               if (textline->cursor_pos > 0) {
                    textline->cursor_pos--;

                    lite_update_box( box, NULL );
               }
               break;

          case DIKS_CURSOR_RIGHT:
               if (textline->cursor_pos < strlen( textline->text) ) {
                    textline->cursor_pos++;

                    lite_update_box( box, NULL );
               }
               break;

          case DIKS_HOME:
               if (textline->cursor_pos > 0) {
                    textline->cursor_pos = 0;

                    lite_update_box( box, NULL );
               }
               break;

          case DIKS_END:
               if (textline->cursor_pos < strlen( textline->text )) {
                    textline->cursor_pos = strlen( textline->text );

                    lite_update_box( box, NULL );
               }
               break;

          case DIKS_DELETE:
               if (textline->cursor_pos < strlen( textline->text )) {
                    int len = strlen( textline->text );

                    set_modified( textline );

                    direct_memmove( textline->text + textline->cursor_pos, textline->text + textline->cursor_pos + 1,
                                    len - textline->cursor_pos );

                    textline->text = D_REALLOC( textline->text, len );

                    lite_update_box( box, NULL );
               }
               break;

          case DIKS_BACKSPACE:
               if (textline->cursor_pos > 0) {
                    int len = strlen( textline->text );

                    set_modified( textline );

                    direct_memmove( textline->text + textline->cursor_pos - 1, textline->text + textline->cursor_pos,
                                    len - textline->cursor_pos + 1 );

                    textline->text = D_REALLOC( textline->text, len );

                    textline->cursor_pos--;

                    lite_update_box( box, NULL );
               }
               break;

          default:
               if (evt->key_symbol >= 32 && evt->key_symbol <= 127) {
                    int len = strlen( textline->text );

                    set_modified( textline );

                    textline->text = D_REALLOC( textline->text, len + 2 );

                    direct_memmove( textline->text + textline->cursor_pos + 1, textline->text + textline->cursor_pos,
                                    len - textline->cursor_pos + 1 );

                    textline->text[textline->cursor_pos] = evt->key_symbol;

                    textline->cursor_pos++;

                    lite_update_box( box, NULL );
               }
               break;
     }

     return 1;
}

static DFBResult
draw_textline( LiteBox         *box,
               const DFBRegion *region,
               DFBBoolean       clear )
{
     DFBResult         ret;
     IDirectFBSurface *surface  = box->surface;
     LiteTextLine     *textline = LITE_TEXTLINE(box);
     int               text_x, cursor_x = 0;
     IDirectFBFont    *font = NULL;

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteTextLineDomain, "Draw textline: %p (modified:%d, cursor_pos:%u, clear:%u)\n",
                 textline, textline->modified, textline->cursor_pos, clear );

     ret = lite_font( textline->font, &font );
     if (ret != DFB_OK)
          return ret;

     if (clear)
          lite_clear_box( box, region );

     surface->SetClip( surface, region );

     surface->SetDrawingFlags( surface, DSDRAW_NOFX );

     surface->SetFont( surface, font );

     /* draw border */
     if (box->is_focused)
          surface->SetColor( surface, 0xa0, 0xa0, 0xff, 0xff );
     else
          surface->SetColor( surface, 0xe0, 0xe0, 0xe0, 0xff );
     surface->DrawRectangle( surface, 0, 0, box->rect.w, box->rect.h );
     surface->SetColor( surface, 0xc0, 0xc0, 0xc0, 0xff );
     surface->DrawRectangle( surface, 1, 1, box->rect.w - 2, box->rect.h - 2 );

     /* draw background */
     if (textline->modified) {
          if (textline->theme != liteNoTextLineTheme)
               surface->SetColor( surface,
                                  textline->theme->bg_color_changed.r,
                                  textline->theme->bg_color_changed.g,
                                  textline->theme->bg_color_changed.b,
                                  textline->theme->bg_color_changed.a );
          else
               surface->SetColor( surface, 0xd0, 0xd0, 0xd0, 0xff );
     }
     else {
          if (textline->theme != liteNoTextLineTheme)
               surface->SetColor( surface,
                                  textline->theme->theme.bg_color.r,
                                  textline->theme->theme.bg_color.g,
                                  textline->theme->theme.bg_color.b,
                                  textline->theme->theme.bg_color.a );
          else
               surface->SetColor( surface, 0xf0, 0xf0, 0xf0, 0xf0 );
     }
     surface->FillRectangle( surface, 2, 2, box->rect.w - 4, box->rect.h - 4 );

     /* draw the text */
     font->GetStringWidth( font, textline->text, textline->cursor_pos, &cursor_x );
     surface->SetColor( surface, 0x30, 0x30, 0x30, 0xff );
     text_x = 5;
     if (cursor_x > box->rect.w - 5)
         text_x = box->rect.w - 5 - cursor_x;
     surface->DrawString( surface, textline->text, -1, text_x, 2, DSTF_TOPLEFT );
     cursor_x += text_x - 5;

     /* draw the cursor */
     surface->SetDrawingFlags( surface, DSDRAW_BLEND );
     if (box->is_focused)
          surface->SetColor( surface, 0x40, 0x40, 0x80, 0x80 );
     else
          surface->SetColor( surface, 0x80, 0x80, 0x80, 0x80 );
     surface->FillRectangle( surface, cursor_x + 5, 4, 1, box->rect.h - 8 );

     return DFB_OK;
}

static DFBResult
destroy_textline( LiteBox *box )
{
     LiteTextLine *textline = LITE_TEXTLINE(box);

     D_ASSERT( box != NULL );

     D_DEBUG_AT( LiteTextLineDomain, "Destroy textline: %p\n", textline );

     if (textline->modified)
          D_FREE( textline->backup );

     D_FREE( textline->text );

     return lite_destroy_box( box );
}
