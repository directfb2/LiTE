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
#include <lite/button.h>
#include <lite/lite_internal.h>

D_DEBUG_DOMAIN( LiteButtonDomain, "LiTE/Button", "LiTE Button" );

/**********************************************************************************************************************/

LiteButtonTheme *liteDefaultButtonTheme = NULL;

struct _LiteButton {
     LiteBox              box;
     LiteButtonTheme     *theme;

     int                  activated;
     int                  enabled;
     LiteButtonType       type;
     LiteButtonState      state;
     IDirectFBSurface    *surfaces[LITE_BS_MAX];

     LiteButtonPressFunc  press;
     void                *press_data;
};

static int       on_enter      ( LiteBox *box, int x, int y );
static int       on_leave      ( LiteBox *box, int x, int y );
static int       on_button_down( LiteBox *box, int x, int y, DFBInputDeviceButtonIdentifier button_id );
static int       on_button_up  ( LiteBox *box, int x, int y, DFBInputDeviceButtonIdentifier button_id );
static DFBResult draw_button   ( LiteBox *box, const DFBRegion *region, DFBBoolean clear );
static DFBResult destroy_button( LiteBox *box );

/**********************************************************************************************************************/

DFBResult
lite_new_button( LiteBox          *parent,
                 DFBRectangle     *rect,
                 LiteButtonTheme  *theme,
                 LiteButton      **ret_button )
{
     DFBResult   ret;
     LiteButton *button;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_button );

     button = D_CALLOC( 1, sizeof(LiteButton) );

     button->box.parent = parent;
     button->box.rect   = *rect;
     button->theme      = theme;

     ret = lite_init_box( LITE_BOX(button) );
     if (ret != DFB_OK) {
          D_FREE( button );
          return ret;
     }

     button->box.type         = LITE_TYPE_BUTTON;
     button->box.OnEnter      = on_enter;
     button->box.OnLeave      = on_leave;
     button->box.OnButtonDown = on_button_down;
     button->box.OnButtonUp   = on_button_up;
     button->box.Draw         = draw_button;
     button->box.Destroy      = destroy_button;
     button->enabled          = 1;
     button->type             = LITE_BT_PUSH;
     button->state            = LITE_BS_NORMAL;

     *ret_button = button;

     D_DEBUG_AT( LiteButtonDomain, "Created new button object: %p\n", button );

     return DFB_OK;
}

DFBResult
lite_enable_button( LiteButton *button,
                    int         enabled )
{
     LITE_NULL_PARAMETER_CHECK( button );
     LITE_BOX_TYPE_PARAMETER_CHECK( button, LITE_TYPE_BUTTON );

     D_DEBUG_AT( LiteButtonDomain, "%s button: %p\n", enabled ? "Enable" : "Disable", button );

     if (button->enabled == enabled)
          return DFB_OK;

     button->enabled = enabled;

     return lite_update_box( LITE_BOX(button), NULL );
}

DFBResult
lite_set_button_type( LiteButton     *button,
                      LiteButtonType  type )
{
     LITE_NULL_PARAMETER_CHECK( button );
     LITE_BOX_TYPE_PARAMETER_CHECK( button, LITE_TYPE_BUTTON );

     D_DEBUG_AT( LiteButtonDomain, "Set button: %p as %s\n", button, type ? "toggle" : "push" );

     button->type = type;

     return DFB_OK;
}

DFBResult
lite_set_button_state( LiteButton      *button,
                       LiteButtonState  state)
{
     int i;

     LITE_NULL_PARAMETER_CHECK( button );
     LITE_BOX_TYPE_PARAMETER_CHECK( button, LITE_TYPE_BUTTON );

     /* check for valid state */
     if (state >= LITE_BS_MAX)
          return DFB_INVARG;

     D_DEBUG_AT( LiteButtonDomain, "Set button: %p to state %u\n", button, state );

     if (button->state == state)
          return DFB_OK;

     for (i = 0; i < LITE_BS_MAX; i++) {
          if (button->surfaces[i])
               break;
     }

     if (i < LITE_BS_MAX) {
          if (!button->surfaces[state])
               return DFB_OK;
     }
     else {
          if (button->theme != liteNoButtonTheme && !button->theme->surfaces[state])
               return DFB_OK;
     }

     button->state = state;

     if (button->enabled) {
          return lite_update_box( LITE_BOX(button), NULL );
     }

     return DFB_OK;
}

DFBResult
lite_get_button_state( LiteButton      *button,
                       LiteButtonState *ret_state )
{
     LiteButtonState state;

     LITE_NULL_PARAMETER_CHECK( button );
     LITE_NULL_PARAMETER_CHECK( ret_state );
     LITE_BOX_TYPE_PARAMETER_CHECK( button, LITE_TYPE_BUTTON );

     D_DEBUG_AT( LiteButtonDomain, "button: %p is in state: %u\n", button, button->state );

     if (button->enabled)
          state = button->state;
     else
          state = button->activated ? LITE_BS_DISABLED_ON : LITE_BS_DISABLED;

     *ret_state = state;

     return DFB_OK;
}

static DFBResult
set_button_image( LiteButton      *button,
                  LiteButtonState  state,
                  const void      *file_data,
                  unsigned int     length )
{
     LITE_NULL_PARAMETER_CHECK( button );
     LITE_BOX_TYPE_PARAMETER_CHECK( button, LITE_TYPE_BUTTON );

     /* check for valid state */
     if (state >= LITE_BS_MAX)
          return DFB_INVARG;

     D_DEBUG_AT( LiteButtonDomain, "Set button: %p for state: %u\n", button, state );

     if (file_data) {
          DFBResult         ret;
          IDirectFBSurface *surface;

          ret = prvlite_load_image( file_data, length, &surface, NULL, NULL, NULL );
          if (ret != DFB_OK)
               return ret;

          if (button->surfaces[state])
               button->surfaces[state]->Release( button->surfaces[state] );

          button->surfaces[state] = surface;
     }
     else if (button->surfaces[state]) {
          button->surfaces[state]->Release( button->surfaces[state] );
          button->surfaces[state] = NULL;
          return DFB_OK;
     }

     if (state == button->state || (state == LITE_BS_DISABLED && !button->enabled)) {
          if (file_data)
               return lite_update_box( LITE_BOX(button), NULL );
          else
               return lite_clear_box( LITE_BOX(button), NULL );
     }

     return DFB_OK;
}

DFBResult
lite_set_button_image( LiteButton      *button,
                       LiteButtonState  state,
                       const char      *image_path )
{
     return set_button_image( button, state, image_path, 0 );
}

DFBResult
lite_set_button_image_data( LiteButton      *button,
                            LiteButtonState  state,
                            const void      *data,
                            unsigned int     length )
{
     return set_button_image( button, state, data, length );
}

DFBResult
lite_set_button_image_surface( LiteButton       *button,
                               LiteButtonState   state,
                               IDirectFBSurface *surface )
{
     LITE_NULL_PARAMETER_CHECK( button );
     LITE_BOX_TYPE_PARAMETER_CHECK( button, LITE_TYPE_BUTTON );

     D_DEBUG_AT( LiteButtonDomain, "Set button: %p for state: %u with surface: %p\n", button, state, surface );

     /* check for valid state */
     if (state >= LITE_BS_MAX)
          return DFB_INVARG;

     /* set the image surface */
     button->surfaces[state] = surface;

     return DFB_OK;
}

DFBResult
lite_on_button_press( LiteButton          *button,
                      LiteButtonPressFunc  callback,
                      void                *data )
{
     LITE_NULL_PARAMETER_CHECK( button );
     LITE_BOX_TYPE_PARAMETER_CHECK( button, LITE_TYPE_BUTTON );

     D_DEBUG_AT( LiteButtonDomain, "Install callback %p( %p, %p )\n", callback, button, data );

     button->press      = callback;
     button->press_data = data;

     return DFB_OK;
}

DFBResult
lite_new_button_theme( const void       *file_data[LITE_BS_MAX],
                       unsigned int      length[LITE_BS_MAX],
                       LiteButtonTheme **ret_theme )
{
     DFBResult        ret;
     int              i;
     LiteButtonTheme *theme;

     LITE_NULL_PARAMETER_CHECK( file_data );
     LITE_NULL_PARAMETER_CHECK( ret_theme );

     if (liteDefaultButtonTheme && *ret_theme == liteDefaultButtonTheme)
          return DFB_OK;

     theme = D_CALLOC( 1, sizeof(LiteButtonTheme) );

     for (i = 0; i < LITE_BS_MAX; i++) {
          if (file_data[i]) {
               ret = prvlite_load_image( file_data[i], length[i], &theme->surfaces[i], NULL, NULL, NULL );
               if (ret != DFB_OK)
                    goto error;
          }
     }

     *ret_theme = theme;

     D_DEBUG_AT( LiteButtonDomain, "Created new button theme: %p\n", theme );

     return DFB_OK;

error:
     for (i = 0; i < LITE_BS_MAX; i++)
          if (theme->surfaces[i])
               theme->surfaces[i]->Release( theme->surfaces[i] );

     D_FREE( theme );

     return ret;
}

DFBResult
lite_destroy_button_theme( LiteButtonTheme *theme )
{
     int i;

     LITE_NULL_PARAMETER_CHECK( theme );

     D_DEBUG_AT( LiteButtonDomain, "Destroy button theme: %p\n", theme );

     for (i = 0; i < LITE_BS_MAX; i++)
          theme->surfaces[i]->Release( theme->surfaces[i] );

     D_FREE( theme );

     if (theme == liteDefaultButtonTheme)
          liteDefaultButtonTheme = NULL;

     return DFB_OK;
}

/* internals */

static int
on_enter( LiteBox *box,
          int      x,
          int      y )
{
     LiteButton *button = LITE_BUTTON(box);

     D_ASSERT( box != NULL );

     if (button->state == LITE_BS_NORMAL_ON)
          lite_set_button_state( button, LITE_BS_HILITE_ON );
     else
          lite_set_button_state( button, LITE_BS_HILITE );

     return 1;
}

static int
on_leave( LiteBox *box,
          int      x,
          int      y )
{
     LiteButton *button = LITE_BUTTON(box);

     D_ASSERT( box != NULL );

     if (button->activated)
         lite_set_button_state( button, LITE_BS_NORMAL_ON );
     else
         lite_set_button_state( button, LITE_BS_NORMAL );

     return 1;
}

static int
on_button_down( LiteBox                        *box,
                int                             x,
                int                             y,
                DFBInputDeviceButtonIdentifier  button_id )
{
     LiteButton *button = LITE_BUTTON(box);

     D_ASSERT( box != NULL );

     if (button->type == LITE_BT_TOGGLE && button->enabled)
          button->activated = !button->activated;

     lite_set_button_state( button, LITE_BS_PRESSED );

     return 1;
}

static int
on_button_up( LiteBox                        *box,
              int                             x,
              int                             y,
              DFBInputDeviceButtonIdentifier  button_id )
{
     LiteButton *button = LITE_BUTTON(box);

     D_ASSERT( box != NULL );

     /* check that button up occurs within the bounds of the widget */
     if (x >= 0 && x < box->rect.w && y >= 0 && y < box->rect.h) {
          switch (button->type) {
               case LITE_BT_PUSH:
                    lite_set_button_state( button, LITE_BS_HILITE );
                    break;

               case LITE_BT_TOGGLE:
                    if (button->activated)
                         lite_set_button_state( button, LITE_BS_HILITE_ON );
                    else
                         lite_set_button_state( button, LITE_BS_HILITE );
                    break;
          }

          if (button->enabled && button->press)
               button->press( button, button->press_data );
     }
     else {
          if (button->type == LITE_BT_TOGGLE && button->enabled) {
               button->activated = !button->activated;
               if (button->activated)
                    lite_set_button_state( button, LITE_BS_NORMAL_ON );
               else
                    lite_set_button_state( button, LITE_BS_NORMAL );
          }
     }

     return 1;
}

static DFBResult
draw_button( LiteBox         *box,
             const DFBRegion *region,
             DFBBoolean       clear )
{
     int               i;
     LiteButtonState   state;
     IDirectFBSurface *surface;
     LiteButton       *button = LITE_BUTTON(box);

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteButtonDomain, "Draw button: %p (enabled:%d, state:%u, clear:%u)\n",
                 button, button->enabled, button->state, clear );

     if (clear)
          lite_clear_box( box, region );

     surface->SetClip( surface, NULL );

     surface->SetBlittingFlags( surface, DSBLIT_BLEND_ALPHACHANNEL );

     if (button->enabled)
          state = button->state;
     else
          state = button->activated ? LITE_BS_DISABLED_ON : LITE_BS_DISABLED;

     for (i = 0; i < LITE_BS_MAX; i++) {
          if (button->surfaces[i])
               break;
     }

     if (i < LITE_BS_MAX) {
          if (button->surfaces[state])
               surface->Blit( surface, button->surfaces[state], NULL, 0, 0 );
     }
     else {
          if (button->theme != liteNoButtonTheme && button->theme->surfaces[state])
               surface->Blit( surface, button->theme->surfaces[state], NULL, 0, 0 );
     }

     return DFB_OK;
}

static DFBResult
destroy_button( LiteBox *box )
{
     int         i;
     LiteButton *button = LITE_BUTTON(box);

     D_ASSERT( box != NULL );

     D_DEBUG_AT( LiteButtonDomain, "Destroy button: %p\n", button );

     for (i = 0; i < LITE_BS_MAX; i++)
          if (button->surfaces[i])
               button->surfaces[i]->Release( button->surfaces[i] );

     return lite_destroy_box( box );
}
