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

#include <directfb_util.h>
#include <lite/label.h>
#include <lite/lite_internal.h>

D_DEBUG_DOMAIN( LiteLabelDomain, "LiTE/Label", "LiTE Label" );

/**********************************************************************************************************************/

LiteLabelTheme *liteDefaultLabelTheme = NULL;

struct _LiteLabel {
     LiteBox             box;
     LiteLabelTheme     *theme;

     LiteFont           *font;
     char               *text;
     DFBColor            text_color;
     LiteLabelAlignment  alignment;
};

static DFBResult draw_label   ( LiteBox *box, const DFBRegion *region, DFBBoolean clear );
static DFBResult destroy_label( LiteBox *box );

/**********************************************************************************************************************/

DFBResult
lite_new_label( LiteBox         *parent,
                DFBRectangle    *rect,
                LiteLabelTheme  *theme,
                int              size,
                LiteLabel      **ret_label )
{
     DFBResult  ret;
     LiteLabel *label;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_label );

     label = D_CALLOC( 1, sizeof(LiteLabel) );

     label->box.parent = parent;
     label->box.rect   = *rect;
     label->theme      = theme;

     ret = lite_init_box( LITE_BOX(label) );
     if (ret != DFB_OK) {
          D_FREE( label );
          return ret;
     }

     ret = lite_get_font( "default", LITE_FONT_PLAIN, size, DEFAULT_FONT_ATTRIBUTE, &label->font );
     if (ret != DFB_OK) {
          D_FREE( label );
          return ret;
     }

     label->box.type     = LITE_TYPE_LABEL;
     label->box.Draw     = draw_label;
     label->box.Destroy  = destroy_label;
     label->text         = D_STRDUP( "" );
     label->text_color.a = 0xff;

     *ret_label = label;

     D_DEBUG_AT( LiteLabelDomain, "Created new label object: %p\n", label );

     return DFB_OK;
}

DFBResult
lite_set_label_text( LiteLabel  *label,
                     const char *text )
{
     LITE_NULL_PARAMETER_CHECK( label );
     LITE_NULL_PARAMETER_CHECK( text );
     LITE_BOX_TYPE_PARAMETER_CHECK( label, LITE_TYPE_LABEL );

     D_DEBUG_AT( LiteLabelDomain, "Set label: %p with text: %s\n", label, text );

     if (!strcmp( label->text, text ))
          return DFB_OK;

     D_FREE( label->text );

     label->text = D_STRDUP( text );

     return lite_update_box( LITE_BOX(label), NULL );
}

DFBResult
lite_set_label_alignment( LiteLabel          *label,
                          LiteLabelAlignment  alignment )
{
     LITE_NULL_PARAMETER_CHECK( label );
     LITE_BOX_TYPE_PARAMETER_CHECK( label, LITE_TYPE_LABEL );


     D_DEBUG_AT( LiteLabelDomain, "Set label: %p with alignment: %u\n", label, alignment );

     if (label->alignment == alignment)
          return DFB_OK;

     label->alignment = alignment;

     return lite_update_box( LITE_BOX(label), NULL );
}

DFBResult
lite_set_label_font( LiteLabel         *label,
                     const char        *spec,
                     LiteFontStyle      style,
                     int                size,
                     DFBFontAttributes  attr )
{
     DFBResult  ret;
     LiteFont  *font;

     LITE_NULL_PARAMETER_CHECK( label );
     LITE_NULL_PARAMETER_CHECK( spec );
     LITE_BOX_TYPE_PARAMETER_CHECK( label, LITE_TYPE_LABEL );

     D_DEBUG_AT( LiteLabelDomain, "Set label: %p with font spec: %s, style: %s, size: %d and attr: 0x%x\n", label,
                 spec, lite_font_styles[style&3], size, attr );

     ret = lite_get_font( spec, style, size, attr, &font );
     if (ret != DFB_OK)
          return ret;

     lite_release_font( label->font );

     label->font = font;

     return lite_update_box( LITE_BOX(label), NULL );
}

DFBResult
lite_set_label_color( LiteLabel *label,
                      DFBColor  *color )
{
     LITE_NULL_PARAMETER_CHECK( label );
     LITE_NULL_PARAMETER_CHECK( color );
     LITE_BOX_TYPE_PARAMETER_CHECK( label, LITE_TYPE_LABEL );

     D_DEBUG_AT( LiteLabelDomain, "Set label: %p with color: " DFB_COLOR_FORMAT "\n", label, DFB_COLOR_VALS( color ) );

     if (!DFB_COLOR_EQUAL( label->text_color, *color )) {
          label->text_color = *color;
          return lite_update_box( LITE_BOX(label), NULL );
     }

     return DFB_OK;
}

/* internals */

static DFBResult
draw_label( LiteBox         *box,
            const DFBRegion *region,
            DFBBoolean       clear )
{
     DFBResult            ret;
     IDirectFBFont       *font;
     IDirectFBSurface    *surface;
     int                  x     = 0;
     DFBSurfaceTextFlags  flags = DSTF_TOP;
     LiteLabel           *label = LITE_LABEL(box);

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteLabelDomain, "Draw label: %p (alignment:%u, clear:%u)\n", label, label->alignment, clear );

     ret = lite_font( label->font, &font );
     if (ret != DFB_OK)
          return ret;

     if (clear)
          lite_clear_box( box, region );

     surface->SetClip( surface, region );

     surface->SetFont( surface, font );

     switch (label->alignment) {
          case LITE_LABEL_LEFT:
               flags |= DSTF_LEFT;
               break;

          case LITE_LABEL_RIGHT:
               x = box->rect.w - 1;
               flags |= DSTF_RIGHT;
               break;

          case LITE_LABEL_CENTER:
               x = box->rect.w / 2;
               flags |= DSTF_CENTER;
               break;
     }

     /* draw the text */
     surface->SetColor( surface, label->text_color.r, label->text_color.g, label->text_color.b, label->text_color.a );
     surface->DrawString( surface, label->text, -1, x, 0, flags );

     return DFB_OK;
}

static DFBResult
destroy_label( LiteBox *box )
{
     LiteLabel *label = LITE_LABEL(box);

     D_DEBUG_AT( LiteLabelDomain, "Destroy label: %p\n", label );

     D_ASSERT( box != NULL );

     D_FREE( label->text );

     lite_release_font( label->font );

     return lite_destroy_box( box );
}
