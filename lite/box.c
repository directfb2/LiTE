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
#include <lite/box.h>
#include <lite/lite_internal.h>
#include <lite/window.h>

D_DEBUG_DOMAIN( LiteBoxDomain, "LiTE/Box", "LiTE Box" );

/**********************************************************************************************************************/

static void draw_box_and_children( LiteBox *box, const DFBRegion *region, DFBBoolean clear );

static void defocus_me_or_children( LiteWindow *top, LiteBox *box );
static void deenter_me_or_children( LiteWindow *top, LiteBox *box );
static void  undrag_me_or_children( LiteWindow *top, LiteBox *box );

/**********************************************************************************************************************/

DFBResult
lite_init_box( LiteBox *box )
{
     DFBResult ret;

     LITE_NULL_PARAMETER_CHECK( box );

     D_DEBUG_AT( LiteBoxDomain, "Initialize box: %p\n", box );

     if (box->rect.w < 0 || box->rect.h < 0) {
          D_DEBUG_AT( LiteBoxDomain, "  -> negative box width (%d) or height (%d)\n", box->rect.w, box->rect.h );
          return DFB_INVAREA;
     }

     if (box->parent == NULL) {
          /* no parent specified, complete registration later with lite_init_box_at() */
          return DFB_OK;
     }

     ret = box->parent->surface->GetSubSurface( box->parent->surface, &box->rect, &box->surface );
     if (ret) {
          DirectFBError( "LiTE/Box: GetSubSurface() failed", ret );
          return ret;
     }

     if (!box->Destroy)
          box->Destroy = lite_destroy_box;

     lite_add_child( box->parent, box );

     box->is_focused         = 0; /* by default all liteboxes are not focused */
     box->is_visible         = 1; /* by default all liteboxes are visible */
     box->is_active          = 1; /* by default all liteboxes receive input events */
     box->catches_all_events = 0; /* by default all liteboxes allow events to be handled by their children */
     box->handle_keys        = 1; /* by default all liteboxes handle keyboard events */

     if (!box->type)
          box->type = LITE_TYPE_BOX;

     return ret;
}

DFBResult
lite_draw_box( LiteBox         *box,
               const DFBRegion *region,
               DFBBoolean       flip )
{
     LITE_NULL_PARAMETER_CHECK( box );

     if (box->is_visible == 0)
          return DFB_OK;

     if (box->rect.h == 0 || box->rect.w == 0)
          return DFB_OK;

     DFBRegion reg = { 0, 0, box->rect.w - 1,  box->rect.h - 1 };
     if (region == NULL) {
          region = &reg;
     }

     D_DEBUG_AT( LiteBoxDomain, "Draw box: %p at %4d,%4d-%4dx%4d\n", box,
                 region->x1, region->y1, region->x2 - region->x1 + 1, region->y2 - region->y1 + 1 );


     if (box->type == LITE_TYPE_WINDOW && (LITE_WINDOW(box)->flags & LITE_WINDOW_PENDING_RESIZE)) {
          D_DEBUG_AT( LiteBoxDomain, "  -> resize is pending, not drawing...\n" );
          return DFB_OK;
     }

     if (getenv( "LITE_DEBUG_UPDATES" ) && flip) {
          if (box->surface->SetClip( box->surface, region ) == DFB_OK) {
               box->surface->Clear   ( box->surface, 0x00, 0x00, 0xff, 0xff );
               box->surface->SetColor( box->surface, 0xff, 0xff, 0xff, 0xff );
               box->surface->DrawLine( box->surface, region->x1, region->y1, region->x2, region->y2 );
               box->surface->DrawLine( box->surface, region->x1, region->y2, region->x2, region->y1 );

               box->surface->Flip( box->surface, NULL, DSFLIP_NONE );

               direct_thread_sleep( 200000 );
          }
     }

     draw_box_and_children( box, region, DFB_TRUE );

     if (flip)
          box->surface->Flip( box->surface, region, getenv( "LITE_WINDOW_DOUBLEBUFFER" ) ? DSFLIP_BLIT: DSFLIP_NONE );

     if (box->type == LITE_TYPE_WINDOW) {
          LiteWindow *window = LITE_WINDOW(box);

          window->flags |= LITE_WINDOW_DRAWN;

          window->window->SetOpacity( window->window, window->opacity );
     }

     return DFB_OK;
}

DFBResult
lite_update_box( LiteBox         *box,
                 const DFBRegion *region )
{
     LITE_NULL_PARAMETER_CHECK( box );

     DFBRegion reg;
     if (region == NULL) {
          reg.x1 = 0;
          reg.y1 = 0;
          reg.x2 = box->rect.w - 1;
          reg.y2 = box->rect.h - 1;
     }
     else {
          reg = *region;
     }

     D_DEBUG_AT( LiteBoxDomain, "Update box: %p at %4d,%4d-%4dx%4d\n", box,
                 reg.x1, reg.y1, reg.x2 - reg.x1 + 1, reg.y2 - reg.y1 + 1 );

     if (getenv( "LITE_DEBUG_UPDATES" )) {
          if (box->surface->SetClip( box->surface, region ) == DFB_OK) {
               box->surface->Clear   ( box->surface, 0xff, 0x00, 0x00, 0xff );
               box->surface->SetColor( box->surface, 0xff, 0xff, 0xff, 0xff );
               box->surface->DrawLine( box->surface, reg.x1, reg.y1, reg.x2, reg.y2 );
               box->surface->DrawLine( box->surface, reg.x1, reg.y2, reg.x2, reg.y1 );

               box->surface->Flip( box->surface, NULL, DSFLIP_NONE );

               direct_thread_sleep( 200000 );
          }
     }

     while (1) {
          if (reg.x2 < reg.x1 || reg.y2 < reg.y1)
               return DFB_OK;

          if (reg.x1 > box->rect.w - 1 || reg.x2 < 0 || reg.y1 > box->rect.h - 1 || reg.y2 < 0)
               return DFB_OK;

          if (box->is_visible == 0)
               return DFB_OK;

          if (box->parent) {
               dfb_region_translate( &reg, box->rect.x, box->rect.y );
               box = box->parent;
          }
          else {
               if (box->type == LITE_TYPE_WINDOW)
                    lite_update_window( LITE_WINDOW(box), &reg );
               else
                    D_DEBUG_AT( LiteBoxDomain, "  -> can't update a box without a top level parent!\n" );

               return DFB_OK;
          }
     }
}

DFBResult
lite_destroy_box( LiteBox *box )
{
     int i;

     LITE_NULL_PARAMETER_CHECK( box );

     D_DEBUG_AT( LiteBoxDomain, "Destroy box: %p\n", box );

     /* remove the child from the parent's child array, unless it's a window (no parent) */
     if (box->parent != NULL && box->type != LITE_TYPE_WINDOW) {
          lite_remove_child( box->parent, box );
     }

     /* destroy children */
     for (i = 0; i < box->n_children; i++) {
          box->children[i]->parent = NULL;
          D_DEBUG_AT( LiteBoxDomain, "Destroy child box: %p\n", box->children[i] );
          box->children[i]->Destroy( box->children[i] );
     }

     /* free surface */
     box->surface->Release( box->surface );

     if (box->children)
        D_FREE( box->children );

     box->children   = NULL;
     box->n_children = 0;

     return DFB_OK;
}

DFBResult
lite_init_box_at( LiteBox            *box,
                  LiteBox            *parent,
                  const DFBRectangle *rect )
{
     LITE_NULL_PARAMETER_CHECK( box );
     LITE_NULL_PARAMETER_CHECK( rect );

     D_DEBUG_AT( LiteBoxDomain, "Initialize box: %p (parent: %p) at " DFB_RECT_FORMAT "\n", box,
                 parent, DFB_RECTANGLE_VALS( rect ) );

     box->parent = parent;
     box->rect   = *rect;

     return lite_init_box( box );
}

DFBResult
lite_reinit_box_and_children( LiteBox *box )
{
     DFBResult ret = DFB_OK;
     int       i;

     LITE_NULL_PARAMETER_CHECK( box );

     D_DEBUG_AT( LiteBoxDomain, "Give each box a new sub surface\n" );

     /* get new sub surface */
     if (box->parent)
          ret = box->surface->MakeSubSurface( box->surface, box->parent->surface, &box->rect );

     /* reinit children */
     for (i = 0; i < box->n_children; i++)
          ret = lite_reinit_box_and_children( box->children[i] );

    return ret;
}

DFBResult
lite_clear_box( LiteBox         *box,
                const DFBRegion *region )
{
     LITE_NULL_PARAMETER_CHECK( box );

     D_DEBUG_AT( LiteBoxDomain, "Clear box: %p\n", box );

     if (box->parent) {
          DFBRegion reg;

          if (region) {
               reg = *region;
               dfb_region_translate( &reg, box->rect.x, box->rect.y );
          }
          else {
               if (box->rect.h == 0 || box->rect.w == 0)
                    return DFB_OK;

               dfb_region_from_rectangle( &reg, &box->rect );
          }

          if (box->parent->Draw)
               box->parent->Draw( box->parent, &reg, DFB_TRUE );
          else
               lite_clear_box( box->parent, &reg );
     }
     else {
          D_DEBUG_AT(LiteBoxDomain, "  -> no parent present\n");
     }

     return DFB_OK;
}

DFBResult
lite_add_child( LiteBox *parent,
                LiteBox *child )
{
     LiteWindow *window;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( child );

     D_DEBUG_AT( LiteBoxDomain, "Add child: %p\n", child );

     /* update the child array */
     parent->n_children++;
     parent->children = D_REALLOC( parent->children, sizeof(LiteBox*) * parent->n_children );
     parent->children[parent->n_children-1] = child;

     /* get a possible window in which the box is included */
     window = lite_find_my_window( parent );

     if (window != NULL) {
          if (window->OnBoxAdded)
               window->OnBoxAdded( window, child );
     }

     return DFB_OK;
}

DFBResult
lite_remove_child( LiteBox *parent,
                   LiteBox *child )
{
     LiteWindow *window;
     int         i;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( child );

     D_DEBUG_AT( LiteBoxDomain, "Remove child: %p\n", child );

     /* get a possible window in which the box is included */
     window = lite_find_my_window( child );

     if (window != NULL) {
          if (window->OnBoxToBeRemoved)
               window->OnBoxToBeRemoved( window, child );

          defocus_me_or_children( window, child );
          deenter_me_or_children( window, child );
           undrag_me_or_children( window, child );
     }

     /* find the child ito remove from the parent's child array */
     for (i = 0; i < parent->n_children; i++) {
          if (parent->children[i] == child)
               break;
     }

     if (i == parent->n_children) {
          D_DEBUG_AT( LiteBoxDomain, "  -> could not find the child in parent's child array for removal\n" );
          return DFB_FAILURE;
     }

     /* force an update for the area occupied by the child */
     lite_update_box( child, NULL );

     /* update the child array */
     parent->n_children--;
     for (; i < parent->n_children; i++)
          parent->children[i] = parent->children[i+1];

     parent->children = D_REALLOC( parent->children, sizeof(LiteBox*) * parent->n_children );

     return DFB_OK;
}

DFBResult
lite_set_box_visible( LiteBox *box,
                      int      visible )
{
     LITE_NULL_PARAMETER_CHECK( box );

     D_DEBUG_AT( LiteBoxDomain, "Change box visiblity: %p %svisible\n", box, visible ? "" : "not " );

     if (box->is_visible == visible)
          return DFB_OK;

     if (visible) {
          box->is_visible = 1;

          return lite_update_box( box, NULL );
     }

     lite_update_box( box, NULL );

     box->is_visible = 0;

     return DFB_OK;
}

DFBResult
lite_focus_box( LiteBox *box )
{
     LiteWindow *window = lite_find_my_window( box );
     LiteBox    *old    = window->focused_box;

     D_DEBUG_AT( LiteBoxDomain, "Focus box: %p\n", box );

     if (old == box)
          return DFB_OK;

     if (old) {
          old->is_focused = 0;

          if (old->OnFocusOut)
               old->OnFocusOut( old );
     }

     window->focused_box = box;

     box->is_focused = 1;

     if (box->OnFocusIn)
          box->OnFocusIn( box );

     return DFB_OK;
}

/* internals */

static void
draw_box_and_children( LiteBox         *box,
                       const DFBRegion *region,
                       DFBBoolean       clear )
{
     int i;

     D_ASSERT( box != NULL );

     if (box->is_visible == 0)
          return;

     if (region->x2 < region->x1 || region->y2 < region->y1)
          return;

     if (region->x1 > box->rect.w - 1 || region->x2 < 0 || region->y1 > box->rect.h - 1 || region->y2 < 0)
          return;

     D_DEBUG_AT( LiteBoxDomain, "Draw box:   %p at %4d,%4d-%4dx%4d\n", box,
                 box->rect.x, box->rect.y, box->rect.w, box->rect.h );

     box->surface->SetClip( box->surface, region );

     if (box->background)
          box->surface->Clear( box->surface,
                               box->background->r, box->background->g, box->background->b, box->background->a );

     /* draw box */
     if (box->Draw)
          box->Draw( box, region, clear );

     /* draw children */
     for (i = 0; i < box->n_children; i++) {
          DFBRegion  reg   = *region;
          LiteBox   *child = box->children[i];

          dfb_region_translate( &reg, -child->rect.x, -child->rect.y );
          draw_box_and_children( child, &reg, DFB_FALSE );
     }

     if (box->DrawAfter)
          box->DrawAfter( box, region );
}

static void
defocus_me_or_children( LiteWindow *top,
                        LiteBox    *box )
{
     LiteBox *traverse;

     D_ASSERT( box != NULL );

     /* traverse from the focused box to see if box is its ancestor */
     if (top != NULL) {
          traverse = top->focused_box;
          while (traverse) {
               if (traverse == box) {
                    top->focused_box = LITE_BOX(top);
                    break;
               }
               traverse = traverse->parent;
          }
     }
}

static void
deenter_me_or_children( LiteWindow *top,
                        LiteBox    *box )
{
     LiteBox *traverse;

     D_ASSERT( box != NULL );

     /* traverse from the entered box to see if box is its ancestor */
     if (top != NULL) {
          traverse = top->entered_box;
          while (traverse) {
               if (traverse == box) {
                    top->entered_box = NULL;
                    break;
               }
               traverse = traverse->parent;
          }
     }
}

static void
undrag_me_or_children( LiteWindow *top,
                       LiteBox    *box )
{
     LiteBox *traverse;

     D_ASSERT( box != NULL );

     /* traverse from the drag box to see if box is its ancestor */
     if (top != NULL) {
          traverse = top->drag_box;
          while (traverse) {
               if (traverse == box) {
                    lite_release_window_drag_box( top );
                    break;
               }
               traverse = traverse->parent;
          }
     }
}
