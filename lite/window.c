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
#include <directfb_util.h>
#include <lite/cursor.h>
#include <lite/lite_config.h>
#include <lite/lite_internal.h>
#include <lite/window.h>

D_DEBUG_DOMAIN( LiteWindowDomain, "LiTE/Window", "LiTE Window" );
D_DEBUG_DOMAIN( LiteUpdateDomain, "LiTE/Update", "LiTE Window Update" );
D_DEBUG_DOMAIN( LiteMotionDomain, "LiTE/Motion", "LiTE Window Motion" );

/**********************************************************************************************************************/

LiteWindowTheme *liteDefaultWindowTheme = NULL;

typedef struct _LiteWindowTimeout {
     long long                  timeout;       /* time in milliseconds that triggers the timeout */
     int                        id;            /* id value used to remove the timeout callback */
     LiteTimeoutFunc            callback;      /* callback called when timeout occurs in event loop */
     void                      *callback_data; /* context data passed to timeout callback */
     struct _LiteWindowTimeout *next;          /* pointer to the next timeout callback in the queue */
} LiteWindowTimeout;

typedef struct _LiteWindowIdle {
     int                        id;            /* id value used to remove the idle callback */
     LiteTimeoutFunc            callback;      /* callback called when event loop becomes idle */
     void                      *callback_data; /* context data passed to idle callback */
     struct _LiteWindowIdle    *next;          /* pointer to the next idle callback in the queue */
} LiteWindowIdle;

static IDirectFBEventBuffer  *event_buffer_global   = NULL;

static int                    num_windows_global    = 0;
static LiteWindow           **window_array_global   = NULL;

static LiteWindow            *modal_window_global   = NULL;
static LiteWindow            *entered_window_global = NULL;

static IDirectFBWindow       *grabbed_window_global = NULL;

static int                    key_modifier_global   = 0;

static long long              last_update_time      = 0;    /* milliseconds */
static long long              minimum_update_freq   = 200;  /* milliseconds */

static DirectMutex            timeout_mutex         = DIRECT_MUTEX_INITIALIZER();
static LiteWindowTimeout     *timeout_queue         = NULL;
static int                    timeout_next_id       = 1;

static DirectMutex            idle_mutex            = DIRECT_MUTEX_INITIALIZER();
static LiteWindowIdle        *idle_queue            = NULL;
static int                    idle_next_id          = 1;

static bool                   event_loop_alive      = false;

static DFBResult draw_window( LiteBox *box, const DFBRegion *region, DFBBoolean clear );

static void      render_title ( LiteWindow *window );
static void      render_border( LiteWindow *window );

static int       handle_move      ( LiteWindow *window, DFBWindowEvent *ev );
static int       handle_resize    ( LiteWindow *window, DFBWindowEvent *ev );
static int       handle_close     ( LiteWindow *window );
static int       handle_destroy   ( LiteWindow *window );
static int       handle_got_focus ( LiteWindow *window );
static int       handle_lost_focus( LiteWindow *window );
static int       handle_enter     ( LiteWindow *window, DFBWindowEvent *ev );
static int       handle_leave     ( LiteWindow *window, DFBWindowEvent *ev );
static int       handle_motion    ( LiteWindow *window, DFBWindowEvent *ev );
static int       handle_button    ( LiteWindow *window, DFBWindowEvent *ev );
static int       handle_key_up    ( LiteWindow *window, DFBWindowEvent *ev );
static int       handle_key_down  ( LiteWindow *window, DFBWindowEvent *ev );
static int       handle_wheel     ( LiteWindow *window, DFBWindowEvent *ev );

/**********************************************************************************************************************/

DFBResult
lite_new_window( IDirectFBDisplayLayer  *layer,
                 DFBRectangle           *rect,
                 DFBWindowCapabilities   caps,
                 LiteWindowTheme        *theme,
                 const char             *title,
                 LiteWindow            **ret_window )
{
     DFBResult              ret;
     DFBDisplayLayerConfig  dlc;
     DFBWindowDescription   desc;
     DFBWindowOptions       options;
     LiteCursor            *cursor;
     LiteWindow            *window;

     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_window );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p, %p, 0x%x, %p, '%s' )\n", __FUNCTION__, layer, rect, caps, theme, title );
     D_DEBUG_AT( LiteWindowDomain, "  -> " DFB_RECT_FORMAT "\n", DFB_RECTANGLE_VALS( rect ) );

     window = D_CALLOC( 1, sizeof(LiteWindow) );

     window->box.rect = *rect;
     window->theme    = theme;

     if (window->box.rect.w <= 0 || window->box.rect.h <= 0) {
          ret = DFB_INVAREA;
          goto error;
     }

     window->box.type       = LITE_TYPE_WINDOW;
     window->box.is_visible = 1;
     window->box.is_active  = 1;
     window->box.Draw       = draw_window;
     window->box.Destroy    = lite_destroy_box;

     /* get layer configuration */
     if (!layer)
          layer = lite_layer;

     layer->GetConfiguration( layer, &dlc );

     /* get the current global cursor */
     lite_get_current_cursor( &cursor );

     /* create window */
     desc.flags  = DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS;
     desc.width  = window->box.rect.w;
     desc.height = window->box.rect.h;
     desc.caps   = getenv( "LITE_WINDOW_DOUBLEBUFFER" ) ? caps | DWCAPS_DOUBLEBUFFER : caps;

     if (theme != liteNoWindowTheme) {
          desc.width  += (theme->frame.parts[LITE_THEME_FRAME_PART_LEFT].rect.w +
                          theme->frame.parts[LITE_THEME_FRAME_PART_RIGHT].rect.w);
          desc.height += (theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect.h +
                          theme->frame.parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h);
          desc.caps   |= DWCAPS_NODECORATION;
     }

     if (LITE_CENTER_HORIZONTALLY == window->box.rect.x)
          desc.posx = (dlc.width - desc.width) / 2;
     else {
          desc.posx = window->box.rect.x;

          if (theme != liteNoWindowTheme)
               desc.posx -= theme->frame.parts[LITE_THEME_FRAME_PART_LEFT].rect.w;
     }

     if (LITE_CENTER_VERTICALLY == window->box.rect.y)
          desc.posy = (dlc.height - desc.height) / 2;
     else {
          desc.posy = window->box.rect.y;

          if (theme != liteNoWindowTheme)
               desc.posy -= theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect.h;
     }

     ret = layer->CreateWindow( layer, &desc, &window->window );
     if (ret) {
          DirectFBError( "LiTE/Window: CreateWindow() failed", ret );
          goto error;
     }

     /* set size */
     window->width  = desc.width;
     window->height = desc.height;

     /* set cursor shape */
     if (cursor)
          lite_set_window_cursor( window, cursor );

     /* get ID */
     window->window->GetID( window->window, &window->id );

     /* get surface */
     ret = window->window->GetSurface( window->window, &window->surface );
     if (ret) {
          DirectFBError( "LiTE/Window: GetSurface() failed", ret );
          goto error;
     }

     /* get sub surface */
     if (theme != liteNoWindowTheme) {
          window->box.rect.x = theme->frame.parts[LITE_THEME_FRAME_PART_LEFT].rect.w;
          window->box.rect.y = theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect.h;
     }
     else {
          window->box.rect.x = 0;
          window->box.rect.y = 0;
     }

     ret = window->surface->GetSubSurface( window->surface, &window->box.rect, &window->box.surface );
     if (ret) {
          DirectFBError( "LiTE/Window: GetSubSurface() failed", ret );
          goto error;
     }

     /* set opaque content region */
     window->window->SetOpaqueRegion( window->window, window->box.rect.x, window->box.rect.y,
                                      window->box.rect.x + window->box.rect.w - 1,
                                      window->box.rect.y + window->box.rect.h - 1 );

     window->window->GetOptions( window->window, &options );
     window->window->SetOptions( window->window, options | DWOP_OPAQUE_REGION );

     /* set window flags */
     window->flags = LITE_WINDOW_RESIZE | LITE_WINDOW_MINIMIZE;

     /* initialize the update mutex */
     direct_recursive_mutex_init( &window->updates.lock );

     /* set background colors */
     window->bg.enabled = DFB_TRUE;

     if (theme == liteNoWindowTheme) {
          window->bg.color.a  = DEFAULT_WINDOW_COLOR_A;
          window->bg.color.r  = DEFAULT_WINDOW_COLOR_R;
          window->bg.color.g  = DEFAULT_WINDOW_COLOR_G;
          window->bg.color.b  = DEFAULT_WINDOW_COLOR_B;
     }
     else {
          window->bg.color.a  = theme->theme.bg_color.a;
          window->bg.color.r  = theme->theme.bg_color.r;
          window->bg.color.g  = theme->theme.bg_color.g;
          window->bg.color.b  = theme->theme.bg_color.b;
     }

     /* give the focus of the child box */
     window->focused_box = &window->box;

     /* initialize the event loop reference count */
     window->internal_ref_count = 1;

     /* create or attach event buffer */
     if (!event_buffer_global) {
          ret = window->window->CreateEventBuffer( window->window, &event_buffer_global );
          if (ret) {
               DirectFBError( "LiTE/Window: CreateEventBuffer() failed", ret );
               goto error;
          }

          event_buffer_global->AddRef( event_buffer_global );
     }
     else {
          event_buffer_global->AddRef( event_buffer_global );

          ret = window->window->AttachEventBuffer( window->window, event_buffer_global );
          if (ret) {
               DirectFBError( "LiTE/Window: AttachEventBuffer() failed", ret );
               goto error;
          }
     }

     /* set window title */
     if (title)
          window->title = D_STRDUP( title );

     /* render title bar and borders */
     if (theme != liteNoWindowTheme) {
          DFBDimension size = { window->width, window->height };

          /* default titles are black and centered horizontally */
          window->title_color.r  = 0;
          window->title_color.g  = 0;
          window->title_color.b  = 0;
          window->title_color.a  = 0xff;
          window->title_x_offset = -1;
          window->title_y_offset = -1;

          lite_theme_frame_target_update( window->frame_target, &theme->frame, &size );

          render_title( window );
          render_border( window );
     }

     /* initial update */
     lite_update_box( LITE_BOX(window), NULL );

     /* add the window to the global linked list of windows */
     num_windows_global++;
     window_array_global = D_REALLOC( window_array_global, num_windows_global * sizeof(LiteWindow*) );
     window_array_global[num_windows_global - 1] = window;

     *ret_window = window;

     D_DEBUG_AT( LiteWindowDomain, "Created new window object: %p\n", window );

     return DFB_OK;

error:
     if (window->box.surface)
          window->box.surface->Release( window->box.surface );

     if (window->surface)
          window->surface->Release( window->surface );

     if (window->window)
          window->window->Release( window->window );

     D_FREE( window );

     return ret;
}

DFBResult
lite_window_set_creator( LiteWindow *window,
                         LiteWindow *creator )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Set window: %p with creator: %p\n", window, creator );

     window->creator = creator;

     return DFB_OK;
}

DFBResult
lite_window_get_creator( LiteWindow  *window,
                         LiteWindow **ret_creator )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_NULL_PARAMETER_CHECK( ret_creator );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "window: %p has creator: %p\n", window, window->creator );

     *ret_creator = window->creator;

     return DFB_OK;
}

static void
release_grabs()
{
     if (grabbed_window_global) {
          D_DEBUG_AT( LiteWindowDomain, "  -> release grabbed_window_global\n" );

          grabbed_window_global->UngrabPointer( grabbed_window_global );
          grabbed_window_global->UngrabKeyboard( grabbed_window_global );
          grabbed_window_global = NULL;
     }
}

static LiteBox *
find_child( LiteBox *box,
            int     *x,
            int     *y )
{
     int i;

search_again:
     if (box->catches_all_events)
          return box;

     for (i = box->n_children - 1; i >= 0; i--) {
          LiteBox *child = box->children[i];

          if (child->is_visible && DFB_RECTANGLE_CONTAINS_POINT( &child->rect, *x, *y )) {
               *x -= child->rect.x;
               *y -= child->rect.y;
               box = child;
               goto search_again;
          }
     }

     return box;
}

DFBResult
lite_window_set_modal( LiteWindow *window,
                       int         modal )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Set window: %p as %smodal\n", window, modal ? "" : "not " );

     if (modal) {
          int n;

          if (window == modal_window_global)
               return DFB_OK;

          if (modal_window_global) {
               modal_window_global->window->UngrabPointer( modal_window_global->window );
               modal_window_global->window->UngrabKeyboard( modal_window_global->window );
          }

          window->flags |= LITE_WINDOW_MODAL;

          release_grabs();

          grabbed_window_global = window->window;
          grabbed_window_global->GrabKeyboard( grabbed_window_global );
          grabbed_window_global->GrabPointer( grabbed_window_global );

          /* if the window was created during a button down event, adjust the drag box */
          for (n = 0; n < num_windows_global; n++) {
               if (window_array_global[n]->drag_box) {
                    int cx, cy;
                    int dx, dy;
                    int wx, wy;

                    window_array_global[n]->drag_box = NULL;

                    lite_layer->GetCursorPosition( lite_layer, &cx, &cy );

                    window->window->GetPosition( window->window, &wx, &wy );

                    dx = cx - wx;
                    dy = cy - wy;

                    window->drag_box = find_child( LITE_BOX(window), &dx, &dy );
                    break;
               }
          }

          modal_window_global = window;
     }
     else {
          window->flags &= ~LITE_WINDOW_MODAL;

          if (window == modal_window_global) {
               release_grabs();

               modal_window_global = NULL;

               if (window->creator && (window->creator->flags & LITE_WINDOW_MODAL)) {
                    modal_window_global = window->creator;

                    grabbed_window_global = window->creator->window;
                    grabbed_window_global->GrabKeyboard( grabbed_window_global );
                    grabbed_window_global->GrabPointer( grabbed_window_global );
               }
               else {
                    int n;

                    /* find the last modal window and restore its modality */
                    for (n = 0; n < num_windows_global; n++)
                         if (window_array_global[n] == window)
                              break;

                    for (; n > 0; n--) {
                         if (window_array_global[n-1]->flags & LITE_WINDOW_MODAL) {
                              lite_window_set_modal( window_array_global[n-1], true );
                              break;
                         }
                    }
               }
          }
     }

     return DFB_OK;
}

static void
draw_updated_windows()
{
     int i, n;

     D_DEBUG_AT( LiteUpdateDomain, "%s()\n", __FUNCTION__ );

     for (n = 0; n < num_windows_global; n++) {
          int         pending;
          LiteWindow *window = window_array_global[n];

          if (window->flags & LITE_WINDOW_DESTROYED)
               continue;

          direct_mutex_lock( &window->updates.lock );

          pending = window->updates.pending;
          if (pending) {
               D_DEBUG_AT( LiteUpdateDomain, "  -> updating window %u (%p)\n", window->id, window );

               if (getenv( "LITE_BOUNDING_UPDATES" )) {
                    DFBRegion bounding;

                    dfb_regions_unite( &bounding, window->updates.regions, window->updates.pending );

                    D_DEBUG_AT( LiteUpdateDomain, "  -> " DFB_RECT_FORMAT " (bounding of %d regions)\n",
                                DFB_RECTANGLE_VALS_FROM_REGION( &bounding ), window->updates.pending );

                    window->updates.pending = 0;

                    lite_draw_box( LITE_BOX(window), &bounding, DFB_TRUE );
               }
               else {
                    for (i = 0; i < pending && window->updates.pending; i++) {
                         DFBRegion region = window->updates.regions[0];

                         if (--window->updates.pending)
                              direct_memmove( &window->updates.regions[0], &window->updates.regions[1],
                                              window->updates.pending * sizeof(DFBRegion) );

                         D_DEBUG_AT( LiteUpdateDomain, "  -> " DFB_RECT_FORMAT " (%d regions pending)\n",
                                     DFB_RECTANGLE_VALS_FROM_REGION( &region ), window->updates.pending );

                         lite_draw_box( LITE_BOX(window), &region, DFB_TRUE );
                    }
               }

               /* apply opacity change */
               if (!(window->flags & LITE_WINDOW_DRAWN)) {
                    window->flags |= LITE_WINDOW_DRAWN;

                    window->window->SetOpacity( window->window, window->opacity );
               }
          }

          direct_mutex_unlock( &window->updates.lock );
     }

     last_update_time = direct_clock_get_millis();
}

static LiteWindow *
find_window_by_id( DFBWindowID id )
{
     int n;

     for (n = 0; n < num_windows_global; n++)
          if (window_array_global[n]->id == id)
               return window_array_global[n];

     return NULL;
}

static DFBResult
get_time_until_next_timeout( long long *remaining )
{
     DFBResult ret = DFB_FAILURE;

     direct_mutex_lock( &timeout_mutex );

     if (timeout_queue) {
          long long now = direct_clock_get_millis();

          *remaining = timeout_queue->timeout - now;

          ret = DFB_OK;
     }

     direct_mutex_unlock( &timeout_mutex );

     return ret;
}

static DFBResult
remove_next_timeout_callback( LiteTimeoutFunc  *callback,
                              void            **callback_data )
{
     DFBResult ret = DFB_FAILURE;

     direct_mutex_lock( &timeout_mutex );

     long long now = direct_clock_get_millis();

     if (timeout_queue && timeout_queue->timeout <= now) {
          LiteWindowTimeout *node = timeout_queue;

          *callback      = node->callback;
          *callback_data = node->callback_data;

          timeout_queue = node->next;

          D_FREE( node );

          ret = DFB_OK;
     }

     direct_mutex_unlock( &timeout_mutex );

     return ret;
}

static DFBResult
remove_top_idle_callback( LiteTimeoutFunc  *callback,
                          void            **callback_data )
{
     DFBResult ret = DFB_FAILURE;

     direct_mutex_lock( &idle_mutex );

     if (idle_queue) {
          LiteWindowIdle *node = idle_queue;

          *callback      = node->callback;
          *callback_data = node->callback_data;

          idle_queue = node->next;

          D_FREE( node );

          ret = DFB_OK;
     }

     direct_mutex_unlock(&idle_mutex);

     return ret;
}

DFBResult
lite_window_event_loop( LiteWindow *window,
                        int         timeout )
{
     DFBResult        ret;
     LiteTimeoutFunc  callback;
     void            *callback_data;
     long long        remaining;
     int              timeout_id           = 0;
     bool             handle_window_events = true;

     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Enter window event loop with timeout: %d\n", timeout );

     event_loop_alive = true;

     /* destroy window only when event loop ends */
     ++window->internal_ref_count;

     /* add stop timeout */
     if (timeout > 0)
          lite_enqueue_timeout_callback( timeout, NULL, NULL, &timeout_id );

     while (true) {
          /* first test if the alive flag is still set, otherwise exit immediately */
          if (event_loop_alive == false) {
               ret = DFB_OK;
               break;
          }

          if (direct_clock_get_millis() - last_update_time >= minimum_update_freq) {
              /* always flush window-specific events after processing all events */
              lite_flush_window_events( NULL );

              /* redraw all windows that have been changed due to events */
              draw_updated_windows();
          }

          ret = event_buffer_global->HasEvent( event_buffer_global );
          if (ret == DFB_OK) {
               DFBEvent evt;

               /* get the event, dispatch it to the window-specific event handler */
               ret = event_buffer_global->GetEvent( event_buffer_global, &evt );
               if (ret != DFB_OK)
                    continue;

               if (evt.clazz == DFEC_USER) {
                    if (window->user_event_func)
                         window->user_event_func( (DFBUserEvent*) &evt, window->user_event_data );
               }
               else if (evt.clazz == DFEC_UNIVERSAL) {
                    if (window->universal_event_func)
                         window->universal_event_func( (DFBUniversalEvent*) &evt, window->universal_event_data );
               }
               else if (evt.clazz == DFEC_WINDOW) {
                    DFBWindowEvent *win_event = (DFBWindowEvent*) &evt;

                    /* if we have a window event callback, the event is intercepted and can be handled */
                    if (window->window_event_func) {
                        ret = window->window_event_func( win_event, window->window_event_data );
                        if (ret != DFB_OK) {
                            /* the window event callback indicates that the event is handled and should no longer be
                               processed */
                            handle_window_events = false;
                        }
                    }

                    /* process the event unles a possible window event callback indicates that it was handled */
                    if (handle_window_events == true) {
                         int result = lite_handle_window_event( find_window_by_id( win_event->window_id ), win_event );

                         /* the last window is destroyed or closed, or the current window is destroyed */
                         if (win_event->type == DWET_DESTROYED || result < 0) {
                              if (num_windows_global == 0 || window->id == win_event->window_id) {
                                   ret = DFB_DESTROYED;
                                   break;
                              }
                         }
                         else if (win_event->type == DWET_CLOSE) {
                              if (num_windows_global == 1) {
                                   lite_destroy_window( window );
                                   ret = DFB_DESTROYED;
                                   break;
                              }
                         }
                    }
               }

               continue;
          }

          /* always flush window-specific events after processing all events */
          lite_flush_window_events( NULL );

          /* redraw any windows that have been changed due to events */
          draw_updated_windows();

          /* now check timeout callbacks */
          ret = remove_next_timeout_callback( &callback, &callback_data );
          if (ret == DFB_OK) {
               if (callback) {
                    ret = callback( callback_data );
                    if (ret != DFB_OK)
                         break;
               }
               else {
                    ret = DFB_TIMEOUT;
                    break;
               }

               continue;
          }

          /* now check idle callbacks */
          ret = remove_top_idle_callback( &callback, &callback_data );
          if (ret == DFB_OK) {
               if (callback) {
                    ret = callback( callback_data );
                    if (ret != DFB_OK)
                         break;
               }
               else {
                    ret = DFB_TIMEOUT;
                    break;
               }

               continue;
          }

          /* wait for the next event */
          ret = get_time_until_next_timeout( &remaining );
          if (ret == DFB_OK) {
               event_buffer_global->WaitForEventWithTimeout( event_buffer_global, remaining / 1000, remaining % 1000 );
          }
          else if (timeout >= 0) {
               event_buffer_global->WaitForEvent( event_buffer_global );
          }
          else {
               ret = DFB_OK;
               break;
          }
     }

     /* remove stop timeout */
     if (timeout_id)
          lite_remove_timeout_callback( timeout_id );

     /* handle deferred destruction */
     if ((--window->internal_ref_count == 0) && (window->flags & LITE_WINDOW_DESTROYED))
          handle_destroy( window );

     D_DEBUG_AT( LiteWindowDomain, "Exit window event loop\n" );

     return ret;
}

static DFBResult
wakeup_event_loop()
{
     DFBResult ret;

     ret = event_buffer_global->WakeUp( event_buffer_global );
     if (ret == DFB_INTERRUPTED)
          ret = DFB_OK;

     return ret;
}

DFBResult
lite_exit_event_loop()
{
     event_loop_alive = false;

     D_DEBUG_AT( LiteWindowDomain, "Exit event loop\n" );

     return wakeup_event_loop();
}

DFBResult
lite_set_exit_idle_loop( int state )
{
     static int idle_id = 0;

     D_DEBUG_AT( LiteWindowDomain, "%s event loop exit when idle\n",  state ? "Enable" : "Disable" );

     if (state) {
          if (idle_id == 0)
               lite_enqueue_idle_callback( NULL, NULL, &idle_id );
     }
     else {
          if (idle_id != 0) {
               lite_remove_idle_callback( idle_id );
               idle_id = 0;
          }
     }

     return DFB_OK;
}

DFBResult
lite_enqueue_timeout_callback( int              timeout,
                               LiteTimeoutFunc  callback,
                               void            *callback_data,
                               int             *ret_timeout_id )
{
     LiteWindowTimeout *new_item;

     direct_mutex_lock( &timeout_mutex );

     new_item = D_CALLOC( 1, sizeof(LiteWindowTimeout) );

     new_item->timeout       = direct_clock_get_millis() + timeout;
     new_item->callback      = callback;
     new_item->callback_data = callback_data;
     new_item->id            = timeout_next_id++;

     if (timeout_next_id == 0)
          timeout_next_id = 1;

     if (ret_timeout_id)
          *ret_timeout_id = new_item->id;

     D_DEBUG_AT( LiteWindowDomain, "Enqueue timeout (id %d) of %d ms (trigger time %d.%d) with callback: %p( %p )\n",
                 new_item->id, timeout, (int)(new_item->timeout / 1000), (int)(new_item->timeout % 1000),
                 new_item->callback, new_item->callback_data );

     /* insert after all other items with same or newer timeout */
     LiteWindowTimeout **queue = &timeout_queue;
     while (*queue && (*queue)->timeout <= new_item->timeout)
          queue = &(*queue)->next;
     new_item->next = *queue;
     *queue = new_item;

     direct_mutex_unlock( &timeout_mutex );

     wakeup_event_loop();

     return DFB_OK;
}

DFBResult
lite_remove_timeout_callback( int timeout_id )
{
     DFBResult ret = DFB_INVARG;

     D_DEBUG_AT( LiteWindowDomain, "Remove timeout callback with id %d\n", timeout_id );

     direct_mutex_lock( &timeout_mutex );

     LiteWindowTimeout **queue = &timeout_queue;
     LiteWindowTimeout  *node  = timeout_queue;

     while (node) {
          if (node->id == timeout_id) {
               *queue = node->next;

               D_FREE( node );

               ret = DFB_OK;
               break;
          }

          queue = &node->next;
          node  = node->next;
     }

     direct_mutex_unlock( &timeout_mutex );

     wakeup_event_loop();

     return ret;
}

DFBResult
lite_rebase_window_timeouts( long long adjustment )
{
     direct_mutex_lock( &timeout_mutex );

     D_DEBUG_AT( LiteWindowDomain, "Rebase all timeout callbacks\n" );

     LiteWindowTimeout *current = timeout_queue;
     while (current) {
          current->timeout += adjustment;
          current = current->next;
     }

     direct_mutex_unlock( &timeout_mutex );

     wakeup_event_loop();

     return DFB_OK;
}

DFBResult
lite_enqueue_idle_callback( LiteTimeoutFunc  callback,
                            void            *callback_data,
                            int             *ret_idle_id )
{
     LiteWindowIdle *new_item;

     direct_mutex_lock( &idle_mutex );

     new_item = D_CALLOC( 1, sizeof(LiteWindowIdle) );

     new_item->callback      = callback;
     new_item->callback_data = callback_data;
     new_item->id            = idle_next_id++;

     if (idle_next_id == 0)
          idle_next_id = 1;

     if (ret_idle_id)
          *ret_idle_id = new_item->id;

     D_DEBUG_AT( LiteWindowDomain, "Enqueue idle (id %d) with callback: %p( %p )\n",
                 new_item->id, new_item->callback, new_item->callback_data );

     /* insert after all other items */
     LiteWindowIdle **queue = &idle_queue;
     while (*queue)
          queue = &(*queue)->next;
     *queue = new_item;

     direct_mutex_unlock( &idle_mutex );

     wakeup_event_loop();

     return DFB_OK;
}

DFBResult
lite_remove_idle_callback( int idle_id )
{
     DFBResult ret = DFB_INVARG;

     D_DEBUG_AT( LiteWindowDomain, "Remove idle callback with id %d\n", idle_id );

     direct_mutex_lock( &idle_mutex );

     LiteWindowIdle **queue = &idle_queue;
     LiteWindowIdle  *node  = idle_queue;

     while (node) {
          if ((*queue)->id == idle_id) {
               *queue = node->next;

               D_FREE( node );

               ret = DFB_OK;
               break;
          }

          queue = &node->next;
          node  = node->next;
     }

     direct_mutex_unlock( &idle_mutex );

     wakeup_event_loop();

     return ret;
}

DFBResult
lite_update_window( LiteWindow      *window,
                    const DFBRegion *region )
{
     int       i;
     DFBRegion update;

     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     DFB_REGION_ASSERT_IF( region );

     D_DEBUG_AT( LiteUpdateDomain, "%s( %p, %p )\n", __FUNCTION__, window, region );

     update.x1 = 0;
     update.y1 = 0;
     update.x2 = window->box.rect.w - 1;
     update.y2 = window->box.rect.h - 1;

     /* check requested region */
     if (region) {
         D_DEBUG_AT( LiteUpdateDomain, "  -> " DFB_RECT_FORMAT "\n", DFB_RECTANGLE_VALS_FROM_REGION( region ) );

         if (!dfb_region_region_intersect( &update, region )) {
             D_DEBUG_AT( LiteUpdateDomain, "  -> fully clipped\n" );
             return DFB_OK;
         }
     }

     D_DEBUG_AT( LiteUpdateDomain, "  -> " DFB_RECT_FORMAT " (clipped)\n", DFB_RECTANGLE_VALS_FROM_REGION( &update ) );

     direct_mutex_lock( &window->updates.lock );

     if (window->flags & LITE_WINDOW_PENDING_RESIZE) {
          D_DEBUG_AT( LiteUpdateDomain, "  -> resize is pending, not queuing an update...\n" );
          direct_mutex_unlock( &window->updates.lock );
          return DFB_OK;
     }

     if (window->updates.pending == LITE_WINDOW_MAX_UPDATES) {
         D_DEBUG_AT( LiteUpdateDomain, "  -> max updates (%d) reached, merging...\n", LITE_WINDOW_MAX_UPDATES );

         for (i = 1; i < window->updates.pending; i++)
             dfb_region_region_union( &window->updates.regions[0], &window->updates.regions[i] );

         window->updates.pending = 1;

         D_DEBUG_AT( LiteUpdateDomain, "  -> new single update: " DFB_RECT_FORMAT " [0]\n",
                     DFB_RECTANGLE_VALS_FROM_REGION( &window->updates.regions[0] ) );
     }

     for (i = 0; i < window->updates.pending; i++) {
         if (dfb_region_region_intersects( &update, &window->updates.regions[i] )) {
             D_DEBUG_AT( LiteUpdateDomain, "  -> intersection, merging...\n" );

             dfb_region_region_union( &window->updates.regions[i], &update );

             D_DEBUG_AT( LiteUpdateDomain, "  -> new update: " DFB_RECT_FORMAT " [%d]\n",
                         DFB_RECTANGLE_VALS_FROM_REGION( &window->updates.regions[i] ), i );
             break;
         }
     }

     if (i == window->updates.pending) {
         D_DEBUG_AT( LiteUpdateDomain, "  -> adding: " DFB_RECT_FORMAT " [%d]\n",
                     DFB_RECTANGLE_VALS_FROM_REGION( &update ), i );

         window->updates.regions[i] = update;

         window->updates.pending++;
     }

     direct_mutex_unlock( &window->updates.lock );

     wakeup_event_loop();

     return DFB_OK;
}

DFBResult
lite_update_all_windows()
{
     int i;

     D_DEBUG_AT( LiteUpdateDomain, "%s()\n", __FUNCTION__ );

     for (i = 0; i < num_windows_global; i++) {
         LiteWindow *window = window_array_global[i];

         /* render title bar and borders */
         if (window->theme != liteNoWindowTheme) {
              DFBDimension size = { window->width, window->height };

              lite_theme_frame_target_update( window->frame_target, &window->theme->frame, &size );

              render_title( window );
              render_border( window );
         }

         lite_update_window( window, NULL );
     }

     return DFB_OK;
}

DFBResult
lite_set_window_title( LiteWindow *window,
                       const char *title )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_NULL_PARAMETER_CHECK( title );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Set window: %p with title: %s\n", window, title );

     /* free old title */
     if (window->title)
          D_FREE( window->title );

     /* set new title */
     if (title)
         window->title = D_STRDUP( title );
     else
         window->title = NULL;

     /* render title bar */
     if (window->theme != liteNoWindowTheme) {
          DFBRegion         region;
          IDirectFBSurface *surface = window->surface;

          render_title( window );

          region = DFB_REGION_INIT_FROM_RECTANGLE( &window->theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect );

          surface->Flip( surface, &region, getenv( "LITE_WINDOW_DOUBLEBUFFER" ) ? DSFLIP_BLIT: DSFLIP_NONE );
     }

     return DFB_OK;
}

DFBResult
lite_set_window_enabled( LiteWindow *window,
                         int         enabled )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "%s window: %p\n", enabled ? "Enable" : "Disable", window );

     if (enabled)
          window->flags &= ~LITE_WINDOW_DISABLED;
     else
          window->flags |= LITE_WINDOW_DISABLED;

     return DFB_OK;
}

DFBResult
lite_set_window_opacity( LiteWindow *window,
                         u8          opacity )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     if (window->opacity_mode == LITE_BLEND_NEVER && opacity && opacity < 0xff)
          opacity = 0xff;

     D_DEBUG_AT( LiteWindowDomain, "Set window: %p with opacity: %d\n", window, opacity );

     window->opacity = opacity;

     if ((window->flags & LITE_WINDOW_DRAWN) || !opacity)
          return window->window->SetOpacity( window->window, opacity );

     return DFB_OK;
}

DFBResult
lite_set_window_background( LiteWindow     *window,
                            const DFBColor *bg_color )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Set window: %p with background color\n", window );
     if (bg_color)
          D_DEBUG_AT( LiteWindowDomain, "  -> " DFB_COLOR_FORMAT "\n", DFB_COLOR_VALS( bg_color ) );

     if (bg_color) {
          if (!window->bg.enabled || !DFB_COLOR_EQUAL( window->bg.color, *bg_color )) {
               window->bg.enabled = DFB_TRUE;
               window->bg.color   = *bg_color;

               lite_update_box( LITE_BOX(window), NULL );
          }
     }
     else
          window->bg.enabled = DFB_FALSE;

    return DFB_OK;
}

DFBResult
lite_set_window_blend_mode( LiteWindow    *window,
                            LiteBlendMode  content,
                            LiteBlendMode  opacity )
{
     DFBGraphicsDeviceDescription desc;
     DFBWindowOptions             options;

     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Set window: %p with content blend mode: %u and opacity blend mode: %u\n", window,
                 content, opacity );

     lite_dfb->GetDeviceDescription( lite_dfb, &desc );

     switch (content) {
          case LITE_BLEND_ALWAYS:
          case LITE_BLEND_NEVER:
               window->content_mode = content;
               break;
          default:
               if (desc.blitting_flags & DSBLIT_BLEND_ALPHACHANNEL)
                    window->content_mode = LITE_BLEND_ALWAYS;
               else
                    window->content_mode = LITE_BLEND_NEVER;
               break;
     }

     window->window->GetOptions( window->window, &options );

     if (window->content_mode == LITE_BLEND_NEVER)
          options |= DWOP_OPAQUE_REGION;
     else
          options &= ~DWOP_OPAQUE_REGION;

     window->window->SetOptions( window->window, options );

     switch (opacity) {
          case LITE_BLEND_ALWAYS:
          case LITE_BLEND_NEVER:
               window->opacity_mode = opacity;
               break;
          default:
               if (desc.blitting_flags & DSBLIT_BLEND_COLORALPHA)
                    window->opacity_mode = LITE_BLEND_ALWAYS;
               else
                    window->opacity_mode = LITE_BLEND_NEVER;
               break;
     }

     return DFB_OK;
}

DFBResult
lite_resize_window( LiteWindow   *window,
                    unsigned int  width,
                    unsigned int  height )
{
     DFBResult    ret;
     unsigned int nw, nh;

     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Resize window: %p to %ux%u\n", window, width, height );

     if (width == 0 || width > INT_MAX || height == 0 || height > INT_MAX)
          return DFB_INVAREA;

     /* calculate the new size */
     nw = width;
     nh = height;

     if (window->theme != liteNoWindowTheme) {
          nw += window->theme->frame.parts[LITE_THEME_FRAME_PART_LEFT].rect.w +
                window->theme->frame.parts[LITE_THEME_FRAME_PART_RIGHT].rect.w;
          nh += window->theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect.h +
                window->theme->frame.parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h;
     }

     if (nw == window->width && nh == window->height)
          return DFB_OK;

     direct_mutex_lock( &window->updates.lock );

     window->flags |= LITE_WINDOW_PENDING_RESIZE;
     window->flags &= ~LITE_WINDOW_DRAWN;

     window->updates.pending = 0;

     direct_mutex_unlock( &window->updates.lock );

     /* set the new size */
     ret = window->window->Resize( window->window, nw, nh );
     if (ret) {
          DirectFBError( "LiTE/Window: Resize() failed", ret );
     }
     else {
          window->width  = nw;
          window->height = nh;
     }

     return ret;
}

DFBResult
lite_set_window_bounds( LiteWindow   *window,
                        int           x,
                        int           y,
                        unsigned int  width,
                        unsigned int  height )
{
     DFBResult    ret;
     unsigned int nw, nh;

     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Set window: %p with bounds %4d,%4d-%4ux%4u\n", window, x, y, width, height );

     if (width == 0 || width > INT_MAX || height == 0 || height > INT_MAX)
          return DFB_INVAREA;

     /* calculate the new size */
     nw = width;
     nh = height;

     if (window->theme != liteNoWindowTheme) {
          nw += window->theme->frame.parts[LITE_THEME_FRAME_PART_LEFT].rect.w +
                window->theme->frame.parts[LITE_THEME_FRAME_PART_RIGHT].rect.w;
          nh += window->theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect.h +
                window->theme->frame.parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h;
     }

     /* don't resize if we're just repositioning */
     if (nw == window->width && nh == window->height) {
         ret = window->window->MoveTo( window->window, x, y );
         if (ret)
               DirectFBError( "LiTE/Window: MoveTo() failed", ret );

         return ret;
     }

     direct_mutex_lock( &window->updates.lock );

     window->flags |= LITE_WINDOW_PENDING_RESIZE;
     window->flags &= ~LITE_WINDOW_DRAWN;

     window->updates.pending = 0;

     direct_mutex_unlock( &window->updates.lock );

     /* set the new size */
     ret = window->window->SetBounds( window->window, x, y, nw, nh );
     if (ret) {
          DirectFBError( "LiTE/Window: SetBounds() failed", ret );
     }
     else {
          window->width  = nw;
          window->height = nh;
     }

     return ret;
}

DFBResult
lite_get_window_size( LiteWindow *window,
                      int        *ret_width,
                      int        *ret_height )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_NULL_PARAMETER_CHECK( ret_width );
     LITE_NULL_PARAMETER_CHECK( ret_height );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "window: %p has a size of: %dx%d\n", window, window->box.rect.w, window->box.rect.h );

     *ret_width  = window->box.rect.w;
     *ret_height = window->box.rect.h;

     return DFB_OK;
}

LiteWindow *
lite_find_my_window( LiteBox *box )
{
     if (box == NULL)
          return NULL;

     D_DEBUG_AT( LiteWindowDomain, "Find window for box: %p\n", box );

     while (box->parent)
          box = box->parent;

     if (box->type == LITE_TYPE_WINDOW) {
          D_DEBUG_AT( LiteWindowDomain, "  -> %p\n", LITE_WINDOW(box) );
          return LITE_WINDOW(box);
     }

     return NULL;
}

DFBResult
lite_minimize_window( LiteWindow *window )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Minimize window: %p\n", window );

     window->last_width  = window->width  - (window->theme->frame.parts[LITE_THEME_FRAME_PART_LEFT].rect.w +
                                             window->theme->frame.parts[LITE_THEME_FRAME_PART_RIGHT].rect.w);
     window->last_height = window->height - (window->theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect.h +
                                             window->theme->frame.parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h);

     lite_resize_window( window, window->min_width, window->min_height );

     window->window->Move( window->window, (window->last_width - window->min_width) / 2, 0 );

     return DFB_OK;
}

DFBResult
lite_restore_window( LiteWindow *window )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Restore window: %p\n", window );

     window->window->Move( window->window, (window->min_width - window->last_width) / 2, 0 );

     lite_resize_window( window, window->last_width, window->last_height );

     return DFB_OK;
}

DFBResult
lite_release_window_drag_box( LiteWindow *window )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Release drag box for window: %p\n", window );

     if (window->drag_box != NULL) {
          D_DEBUG_AT( LiteWindowDomain, "  -> drag_box: %p\n", window->drag_box );

          if (!(window->flags & LITE_WINDOW_MODAL)) {
               release_grabs();
          }

          window->drag_box = NULL;
     }

     return DFB_OK;
}

DFBResult
lite_get_event_buffer( IDirectFBEventBuffer **buffer )
{
     D_DEBUG_AT( LiteWindowDomain, "Get IDirectFBDisplayLayer interface\n" );

     *buffer = event_buffer_global;

     return DFB_OK;
}

DFBResult
lite_post_event_to_window( LiteWindow *window,
                           DFBEvent   *event )
{
     D_DEBUG_AT( LiteWindowDomain, "Post event to window: %p\n", window );

     return event_buffer_global->PostEvent( event_buffer_global, event );
}

DFBResult
lite_window_event_available()
{
     DFBResult ret;
     long long remaining;

     D_DEBUG_AT( LiteWindowDomain, "Check event availability\n" );

     ret = event_buffer_global->HasEvent( event_buffer_global );
     if (ret == DFB_OK)
          return DFB_OK;

     ret = get_time_until_next_timeout( &remaining );
     if (ret == DFB_OK && remaining <= 0)
          return DFB_OK;

     return DFB_BUFFEREMPTY;
}

int
lite_handle_window_event( LiteWindow     *window,
                          DFBWindowEvent *event )
{
     int       result = 0;
     DFBResult ret;

     D_DEBUG_AT( LiteWindowDomain, "Handle event: %p for window: %p\n", event, window );

     if (window == NULL)
          return 0;

     if (event->type == DWET_DESTROYED) {
          /* remove the initial reference added during creation */
          --window->internal_ref_count;
          handle_destroy( window );
          return -1;
     }
     else if (window->flags & LITE_WINDOW_DESTROYED) {
          /* ignore other events for destroyed windows */
          return 0;
     }

     /* no destruction when handling window events */
     ++window->internal_ref_count;

     /* raw callbacks that can be installed to intercept events */
     if (!(window->flags & LITE_WINDOW_DISABLED)) {
          if (event->type == DWET_BUTTONUP || event->type == DWET_BUTTONDOWN) {
               if (window->raw_mouse_func) {
                    ret = window->raw_mouse_func( event, window->raw_mouse_data );
                    /* return if the callback indicates tot stop processing the event */
                    if (ret != DFB_OK) {
                         --window->internal_ref_count;
                         return 0;
                    }
               }
          }
          else if (event->type == DWET_MOTION) {
               if (window->raw_mouse_moved_func) {
                    ret = window->raw_mouse_moved_func( event, window->raw_mouse_data );
                    /* return if the callback indicates tot stop processing the event */
                    if (ret != DFB_OK) {
                         --window->internal_ref_count;
                         return 0;
                    }
               }
          }
          else if (event->type == DWET_KEYUP || event->type == DWET_KEYDOWN) {
               if (window->raw_keyboard_func) {
                    ret = window->raw_keyboard_func( event, window->raw_keyboard_data );
                    /* return if the callback indicates tot stop processing the event */
                    if (ret != DFB_OK) {
                         --window->internal_ref_count;
                         return 0;
                    }
               }
          }
          else if (event->type == DWET_WHEEL) {
               if (window->raw_wheel_func) {
                    ret = window->raw_wheel_func( event, window->raw_wheel_data );
                    /* return if the callback indicates tot stop processing the event */
                    if (ret != DFB_OK) {
                         --window->internal_ref_count;
                         return 0;
                    }
               }
          }
     }

     /* events handled by all windows */
     switch (event->type) {
          case DWET_POSITION:
               result = handle_move( window, event );
               break;
          case DWET_SIZE:
               window->last_resize = *event;
               break;
          case DWET_POSITION_SIZE:
               result = handle_move( window, event );
               if (window->flags & LITE_WINDOW_CONFIGURED)
                    window->last_resize = *event;
               else
                    window->flags |= LITE_WINDOW_CONFIGURED;
               break;
          case DWET_CLOSE:
               result = handle_close( window );
               break;
          case DWET_LOSTFOCUS:
               result = handle_lost_focus( window );
               break;
          case DWET_GOTFOCUS:
               result = handle_got_focus( window );
               break;
          default:
               break;
     }

     /* events handled by enabled windows */
     if (!(window->flags & LITE_WINDOW_DISABLED)) {
          switch (event->type) {
               case DWET_ENTER:
                    window->last_motion = *event;
                    result = handle_enter( window, event );
                    break;
               case DWET_LEAVE:
                    result = handle_leave( window, event );
                    break;
               case DWET_MOTION:
                    window->last_motion = *event;
                    if (window->mouse_func)
                         window->mouse_func( event, window->mouse_data );
                    break;
               case DWET_BUTTONUP:
               case DWET_BUTTONDOWN:
                    result = handle_button( window, event );
                    if (window->mouse_func && !(window->flags & LITE_WINDOW_DESTROYED))
                         window->mouse_func( event, window->mouse_data );
                    break;
               case DWET_KEYUP:
                    result = handle_key_up( window, event );
                    if (window->keyboard_func && !(window->flags & LITE_WINDOW_DESTROYED))
                         window->keyboard_func( event, window->keyboard_data );
                    break;
               case DWET_KEYDOWN:
                    result = handle_key_down( window, event );
                    if (window->keyboard_func && !(window->flags & LITE_WINDOW_DESTROYED))
                         window->keyboard_func( event, window->keyboard_data );
                    break;
               case DWET_WHEEL:
                    result = handle_wheel( window, event );
                    if (window->wheel_func && !(window->flags & LITE_WINDOW_DESTROYED))
                         window->wheel_func( event, window->wheel_data );
                    break;
               default:
                    break;
          }
     }

     /* remove the reference */
     --window->internal_ref_count;

     /* if the window has been marked for destruction, destroy it */
     if (window->flags & LITE_WINDOW_DESTROYED)
          return -1;

     return result;
}

int
lite_get_current_key_modifier()
{
     D_DEBUG_AT( LiteWindowDomain, "Current key modifier: %d\n", key_modifier_global );

     return key_modifier_global;
}

static void
child_coords( LiteBox *box,
              int     *x,
              int     *y )
{
     int tx = *x;
     int ty = *y;

     while (box) {
          tx -= box->rect.x;
          ty -= box->rect.y;
          box = box->parent;
     }

     *x = tx;
     *y = ty;
}

DFBResult
lite_flush_window_events( LiteWindow *window )
{
     D_DEBUG_AT( LiteWindowDomain, "Flush events for window: %p\n", window );

     if (window) {
          LITE_WINDOW_PARAMETER_CHECK( window );

          if (!(window->flags & LITE_WINDOW_DESTROYED)) {
               if (window->last_resize.type) {
                    handle_resize( window, &window->last_resize );

                    window->last_resize.type = DWET_NONE;
               }

               /* if we just finished processing events with no cursor motion, check if the cursor has moved */
               if (window->entered_box != NULL && window->last_motion.type == DWET_NONE) {
                    int      x, y;
                    LiteBox *box = LITE_BOX(window);

                    /* get the cached cursor coordinates */
                    x = window->last_motion.x;
                    y = window->last_motion.y;

                    if (DFB_RECTANGLE_CONTAINS_POINT( &box->rect, x, y )) {
                         if (window->entered_box && window->entered_box->parent) {
                              int parent_x = x;
                              int parent_y = y;

                              child_coords( window->entered_box->parent, &parent_x, &parent_y );

                              if (DFB_RECTANGLE_CONTAINS_POINT( &window->entered_box->rect, parent_x, parent_y )) {
                                   box = window->entered_box;

                                   x = parent_x - box->rect.x;
                                   y = parent_y - box->rect.y;
                              }
                         }
                         else {
                              x -=  box->rect.x;
                              y -=  box->rect.y;
                         }

                         box = find_child( box, &x, &y );

                         if (box->is_active && box != window->entered_box) {
                              D_DEBUG_AT( LiteMotionDomain, "  -> validate entered box %p at (%d,%d), leaving box %p\n",
                                          box, window->last_motion.x, window->last_motion.y, window->entered_box );

                              if (window->entered_box && window->entered_box->OnLeave)
                                   window->entered_box->OnLeave( window->entered_box, -1, -1 );

                              window->entered_box = box;

                              if (box->OnEnter)
                                   box->OnEnter( box, x, y );
                         }
                    }
               }
               else if (window->last_motion.type) {
                    handle_motion( window, &window->last_motion );

                    window->last_motion.type = DWET_NONE;
               }
          }
     }
     else {
          int n;

          for (n = 0; n < num_windows_global; n++) {
               lite_flush_window_events( window_array_global[n] );
          }
     }

     return DFB_OK;
}

DFBResult
lite_close_window( LiteWindow *window )
{
     D_DEBUG_AT( LiteWindowDomain, "Close window: %p\n", window );

     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     return window->window->Close( window->window );
}

static void
destroy_window_data( LiteWindow *window )
{
     window->flags |= LITE_WINDOW_DESTROYED;

     lite_window_set_modal( window, false );

     if (entered_window_global == window)
          entered_window_global = NULL;

     lite_release_window_drag_box( window );

     if (window->title)
          D_FREE( window->title );

     direct_mutex_deinit( &window->updates.lock );

     lite_destroy_box( &window->box );

     window->surface->Release( window->surface );

     if (window->window)
          window->window->Destroy( window->window );
}

DFBResult
lite_destroy_window( LiteWindow *window )
{
     D_DEBUG_AT( LiteWindowDomain, "Destroy window: %p\n", window );

     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     /* don't allow recursive deletion */
     if (window->flags & LITE_WINDOW_DESTROYED) {
          return DFB_OK;
     }

     lite_set_window_opacity( window, liteNoWindowOpacity );

     destroy_window_data( window );

     handle_destroy( window );

     return DFB_OK;
}

DFBResult
lite_destroy_all_windows()
{
     int n = num_windows_global;

     D_DEBUG_AT( LiteWindowDomain, "Destroy all windows\n" );

     /* destroy all windows in order of most recent first */
     while (--n >= 0) {
          /* if we deleted a number of windows in the previous iteration, we may need to reset the counter */
          if (n >= num_windows_global) {
               n = num_windows_global;
               continue;
          }

          LiteWindow *window = window_array_global[n];
          /* don't destroy windows already destroyed and don't destroy a window with a creator, because the creator will
             be responsible for destroying the window */
          if (!(window->flags & LITE_WINDOW_DESTROYED) && window->creator == NULL)
               lite_destroy_window( window );
     }

     return DFB_OK;
}

DFBResult
lite_on_raw_window_mouse( LiteWindow          *window,
                          LiteWindowEventFunc  callback,
                          void                *data )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Install raw mouse event callback\n" );

     window->raw_mouse_func = callback;
     window->raw_mouse_data = data;

     return 0;
}

DFBResult
lite_on_raw_window_mouse_moved( LiteWindow          *window,
                                LiteWindowEventFunc  callback,
                                void                *data )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Install raw mouse move event callback\n" );

     window->raw_mouse_moved_func = callback;
     window->raw_mouse_moved_data = data;

     return 0;
}

DFBResult
lite_on_window_mouse( LiteWindow          *window,
                      LiteWindowEventFunc  callback,
                      void                *data )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Install mouse event callback\n" );

     window->mouse_func = callback;
     window->mouse_data = data;

     return 0;
}

DFBResult
lite_on_raw_window_keyboard( LiteWindow          *window,
                             LiteWindowEventFunc  callback,
                             void                *data )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Install raw keyboard event callback\n" );

     window->raw_keyboard_func = callback;
     window->raw_keyboard_data = data;

     return 0;
}

DFBResult
lite_on_window_keyboard( LiteWindow          *window,
                         LiteWindowEventFunc  callback,
                         void                *data )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Install keyboard event callback\n" );

     window->keyboard_func = callback;
     window->keyboard_data = data;

     return 0;
}

DFBResult
lite_on_window_event( LiteWindow          *window,
                      LiteWindowEventFunc  callback,
                      void                *data )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Install window event callback\n" );

     window->window_event_func = callback;
     window->window_event_data = data;

     return 0;
}

DFBResult
lite_on_window_universal_event( LiteWindow                   *window,
                                LiteWindowUniversalEventFunc  callback,
                                void                         *data )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Install universal event callback\n" );

     window->universal_event_func = callback;
     window->universal_event_data = data;

     return 0;
}

DFBResult
lite_on_window_user_event( LiteWindow              *window,
                           LiteWindowUserEventFunc  callback,
                           void                    *data )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Install user event callback\n" );

     window->user_event_func = callback;
     window->user_event_data = data;

     return 0;
}

DFBResult
lite_on_raw_window_wheel( LiteWindow          *window,
                          LiteWindowEventFunc  callback,
                          void                *data )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Install raw scroll wheel event callback\n" );

     window->raw_wheel_func = callback;
     window->raw_wheel_data = data;

     return DFB_OK;
}

DFBResult
lite_on_window_wheel( LiteWindow          *window,
                      LiteWindowEventFunc  callback,
                      void                *data )
{
     LITE_NULL_PARAMETER_CHECK( window );
     LITE_WINDOW_PARAMETER_CHECK( window );

     D_DEBUG_AT( LiteWindowDomain, "Install scroll wheel event callback\n" );

     window->wheel_func = callback;
     window->wheel_data = data;

     return DFB_OK;
}

DFBResult
lite_new_window_theme( const DFBColor     *bg_color,
                       const char         *spec,
                       LiteFontStyle       style,
                       int                 size,
                       DFBFontAttributes   attr,
                       const void         *file_data[LITE_THEME_FRAME_PART_NUM],
                       unsigned int        length[LITE_THEME_FRAME_PART_NUM],
                       LiteWindowTheme   **ret_theme )
{
     DFBResult        ret;
     LiteWindowTheme *theme;

     LITE_NULL_PARAMETER_CHECK( bg_color );
     LITE_NULL_PARAMETER_CHECK( file_data );
     LITE_NULL_PARAMETER_CHECK( ret_theme );

     if (liteDefaultWindowTheme && *ret_theme == liteDefaultWindowTheme)
          return DFB_OK;

     theme = D_CALLOC( 1, sizeof(LiteWindowTheme) );

     /* configure the default background color */
     theme->theme.bg_color = *bg_color;

     /* load title font */
     ret = lite_get_font( spec, style, size, attr, &theme->title_font );
     if (ret != DFB_OK) {
          D_FREE( theme );
          return ret;
     }

     /* load frame bitmaps */
     ret = lite_theme_frame_load( &theme->frame, file_data, length );
     if (ret != DFB_OK) {
          lite_release_font( theme->title_font );
          D_FREE( theme );
          return ret;
     }

     *ret_theme = theme;

     D_DEBUG_AT( LiteWindowDomain, "Created new window theme: %p\n", theme );

     return DFB_OK;
}

DFBResult
lite_destroy_window_theme( LiteWindowTheme *theme )
{
     LITE_NULL_PARAMETER_CHECK( theme );

     D_DEBUG_AT( LiteWindowDomain, "Destroy window theme: %p\n", theme );

     lite_theme_frame_unload( &theme->frame );

     lite_release_font( theme->title_font );

     D_FREE( theme );

     if (theme == liteDefaultWindowTheme)
          liteDefaultWindowTheme = NULL;

     return DFB_OK;
}

/* internals */

static DFBResult
draw_window( LiteBox         *box,
             const DFBRegion *region,
             DFBBoolean       clear )
{
     IDirectFBSurface *surface;
     LiteWindow       *window = LITE_WINDOW(box);

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteWindowDomain, "Draw window: %p (bg.enabled:%u, clear:%u)\n", window, window->bg.enabled, clear );

     if (window->bg.enabled) {
          surface->SetClip( surface, region );

          surface->Clear( surface, window->bg.color.r, window->bg.color.g, window->bg.color.b, window->bg.color.a );
     }

     return DFB_OK;
}

static void
render_title( LiteWindow *window )
{
     DFBResult         ret;
     int               string_width;
     IDirectFBFont    *font;
     IDirectFBSurface *surface;

     D_ASSERT( window != NULL );

     ret = lite_font( window->theme->title_font, &font );
     if (ret != DFB_OK)
          return;

     surface = window->surface;

     surface->SetClip( surface, NULL );

     surface->SetRenderOptions( surface, DSRO_NONE );

     /* fill title bar background */
     surface->StretchBlit( surface, window->theme->frame.parts[LITE_THEME_FRAME_PART_TOP].source,
                           &window->theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect,
                           &window->frame_target[LITE_THEME_FRAME_PART_TOP] );

     /* draw title */
     if (window->title) {
          int x, y;

          surface->SetColor( surface, window->title_color.r, window->title_color.g, window->title_color.b,
                             window->title_color.a );

          surface->SetFont( surface, font );

          font->GetStringWidth( font, window->title, -1, &string_width );

          x = window->theme->frame.parts[LITE_THEME_FRAME_PART_LEFT].rect.w;
          if (window->title_x_offset == -1)
               x += (window->box.rect.w - string_width) / 2;
          else
               x += window->title_x_offset;

          if (window->title_y_offset == -1)
               y = 6;
          else
               y = window->title_y_offset;

          surface->DrawString( surface, window->title, -1, x, y, DSTF_TOPLEFT );
     }
     else {
          string_width = 0;
     }

     window->min_width  = window->frame_target[LITE_THEME_FRAME_PART_TOPLEFT].w + string_width +
                          window->frame_target[LITE_THEME_FRAME_PART_TOPRIGHT].w;
     window->min_height = 1;
}

static void
render_border( LiteWindow *window )
{
     int               i;
     IDirectFBSurface *surface;

     D_ASSERT( window != NULL );

     surface = window->surface;

     surface->SetClip( surface, NULL );

     surface->SetRenderOptions( surface, DSRO_NONE );

     /* fill borders background except title bar */
     for (i = LITE_THEME_FRAME_PART_BOTTOM; i < LITE_THEME_FRAME_PART_NUM; i++) {
          surface->StretchBlit( surface, window->theme->frame.parts[i].source,
                                &window->theme->frame.parts[i].rect, &window->frame_target[i] );
     }

     if (getenv( "LITE_WINDOW_DOUBLEBUFFER" ))
          surface->Flip( surface, NULL, DSFLIP_BLIT );
}

static int
handle_move( LiteWindow     *window,
             DFBWindowEvent *ev )
{
     D_ASSERT( window != NULL );
     D_ASSERT( ev != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p, %p )\n", __FUNCTION__, window, ev );

     if (window->OnMove)
          return window->OnMove( window, ev->x, ev->y );

     return 0;
}

static int
handle_resize( LiteWindow     *window,
               DFBWindowEvent *ev )
{
     DFBResult         ret;
     DFBRectangle      rect;
     IDirectFBSurface *surface;

     D_ASSERT( window != NULL );
     D_ASSERT( ev != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p, %p )\n", __FUNCTION__, window, ev );

     surface = window->surface;

     /* new sub surface */
     if (window->theme != liteNoWindowTheme) {
          rect.x = window->theme->frame.parts[LITE_THEME_FRAME_PART_LEFT].rect.w;
          rect.y = window->theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect.h;
          rect.w = ev->w - window->theme->frame.parts[LITE_THEME_FRAME_PART_LEFT].rect.w -
                           window->theme->frame.parts[LITE_THEME_FRAME_PART_RIGHT].rect.w;
          rect.h = ev->h - window->theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect.h -
                           window->theme->frame.parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h;
     }
     else {
          rect.x = 0;
          rect.y = 0;
          rect.w = ev->w;
          rect.h = ev->h;
     }

     ret = window->box.surface->MakeSubSurface( window->box.surface, surface, &rect );
     if (ret)
          DirectFBError( "LiTE/Window: MakeSubSurface() failed", ret );
     else
          window->box.rect = rect;

     if (window->OnResize)
          window->OnResize( window, rect.w, rect.h );

     /* give each box a new sub surface */
     lite_reinit_box_and_children( LITE_BOX(window) );

     /* set opaque content region */
     window->window->SetOpaqueRegion( window->window, rect.x, rect.y, rect.x + rect.w - 1, rect.y + rect.h - 1 );

     /* update new size */
     window->width  = ev->w;
     window->height = ev->h;

     /* render title bar and borders */
     if (window->theme != liteNoWindowTheme) {
          DFBDimension size = { window->width, window->height };

          lite_theme_frame_target_update( window->frame_target, &window->theme->frame, &size );

          render_title( window );
          render_border( window );
     }

     /* redraw window content */

     direct_mutex_lock( &window->updates.lock );

     window->flags &= ~LITE_WINDOW_PENDING_RESIZE;

     lite_draw_box( LITE_BOX(window), NULL, DFB_FALSE );

     direct_mutex_unlock( &window->updates.lock );

     surface->Flip( surface, NULL, getenv( "LITE_WINDOW_DOUBLEBUFFER" ) ? DSFLIP_BLIT: 0 );

     return 1;
}

static int
handle_close( LiteWindow *window )
{
     D_ASSERT( window != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p )\n", __FUNCTION__, window );

     if (window->OnClose)
          return window->OnClose( window );

     return 0;
}

static int
handle_destroy( LiteWindow *window )
{
     int ret = DFB_OK;
     int n;

     D_ASSERT( window != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p )\n", __FUNCTION__, window );

     /* if the window is processing events, delay destruction */
     if (window->internal_ref_count > 0)
          return ret;

     if (window->OnDestroy)
          ret = window->OnDestroy( window );

     if (window->window) {
          event_buffer_global->WaitForEvent( event_buffer_global );

          window->window->DetachEventBuffer( window->window, event_buffer_global );

          event_buffer_global->Release( event_buffer_global );

          window->window->Release( window->window );
          window->window = NULL;
     }

     for (n = 0; n < num_windows_global; n++)
          if (window_array_global[n] == window)
               break;

     if (n == num_windows_global) {
          D_DEBUG_AT( LiteWindowDomain, "  -> window not found\n" );
          D_FREE( window );
          return ret;
     }

     num_windows_global--;

     for (; n < num_windows_global; n++)
          window_array_global[n] = window_array_global[n+1];

     window_array_global = D_REALLOC( window_array_global, num_windows_global * sizeof(LiteWindow*) );

     D_FREE( window );

     return ret;
}

static int
handle_got_focus( LiteWindow *window )
{
     D_ASSERT( window != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p )\n", __FUNCTION__, window );

     window->has_focus = 1;

     if (window->OnFocusIn)
          window->OnFocusIn( window );

     return 0;
}

static int
handle_lost_focus( LiteWindow *window )
{
     D_ASSERT( window != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p )\n", __FUNCTION__, window );

     window->has_focus = 0;

     lite_release_window_drag_box( window );

     if (window->OnFocusOut)
          window->OnFocusOut( window );

     return 0;
}

static int
handle_enter( LiteWindow     *window,
              DFBWindowEvent *ev )
{
     D_ASSERT( window != NULL );
     D_ASSERT( ev != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p, %p )\n", __FUNCTION__, window, ev );

     entered_window_global = window;

     if (window->OnEnter)
          window->OnEnter( window, ev->x, ev->y );

     handle_motion( window, ev );

     return 0;
}

static int
handle_leave( LiteWindow     *window,
              DFBWindowEvent *ev )
{
     D_ASSERT( window != NULL );
     D_ASSERT( ev != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p, %p )\n", __FUNCTION__, window, ev );

     if (window == entered_window_global) {
          if (window->entered_box && window->entered_box->OnLeave)
               window->entered_box->OnLeave( window->entered_box, -1, -1 );

          window->entered_box = NULL;

          entered_window_global = NULL;
     }

     lite_release_window_drag_box( window );

     if (window->OnLeave)
          return window->OnLeave( window, ev->x, ev->y );

     window->last_motion.type = DWET_NONE;

     return 0;
}

static int
handle_motion( LiteWindow     *window,
               DFBWindowEvent *ev )
{
     DFBResult  ret;
     LiteBox   *box = LITE_BOX(window);

     D_ASSERT( window != NULL );
     D_ASSERT( ev != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p, %p )\n", __FUNCTION__, window, ev );

     if (window->moving) {
          window->window->Move(window->window, ev->cx - window->old_x, ev->cy - window->old_y);

          window->old_x = ev->cx;
          window->old_y = ev->cy;

          return 1;
     }

     if (window->resizing) {
          int             dx    = ev->cx - window->old_x;
          int             dy    = ev->cy - window->old_y;

          if (window->width + dx < window->min_width)
               dx = window->min_width - window->width;

          if (window->theme != liteNoWindowTheme) {
               if (window->height + dy < window->min_height +
                                         window->theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect.h +
                                         window->theme->frame.parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h)
                    dy = window->min_height + window->theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect.h +
                         window->theme->frame.parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h - window->height;
          }
          else {
               if (window->height + dy < window->min_height)
                    dy = window->min_height - window->height;
          }

          if (window->step_x) {
               if (dx < 0)
                    dx += (-dx) % window->step_x;
               else
                    dx -= dx % window->step_x;
          }

          if (window->step_y) {
               if (dy < 0)
                    dy += (-dy) % window->step_y;
               else
                    dy -= dy % window->step_y;
          }

          ret = window->window->Resize( window->window, window->width + dx, window->height + dy );
          if (ret) {
               DirectFBError( "LiTE/Window: Resize() failed", ret );
          }
          else {
               window->old_x += dx;
               window->old_y += dy;
          }

          return 1;
     }

     if (window->drag_box) {
          int      x        = ev->x;
          int      y        = ev->y;
          LiteBox *drag_box = window->drag_box;

          child_coords( drag_box, &x, &y );

          if (drag_box->OnMotion)
               return drag_box->OnMotion( drag_box, x, y, ev->buttons );

          return 0;
     }

     if (DFB_RECTANGLE_CONTAINS_POINT( &box->rect, ev->x, ev->y )) {
          int x = ev->x - box->rect.x;
          int y = ev->y - box->rect.y;

          if (window->entered_box && window->entered_box->parent) {
               int parent_x = x;
               int parent_y = y;

               child_coords( window->entered_box->parent, &parent_x, &parent_y );

               if (DFB_RECTANGLE_CONTAINS_POINT( &window->entered_box->rect, parent_x, parent_y )) {
                    box = window->entered_box;

                    x = parent_x - box->rect.x;
                    y = parent_y - box->rect.y;
               }
          }

          box = find_child( box, &x, &y );

          if (!box->is_active)
               return 0;

          if (window->entered_box != box) {
               D_DEBUG_AT( LiteMotionDomain, "  -> validate entered box %p at (%d,%d), leaving box %p\n",
                           box, window->last_motion.x, window->last_motion.y, window->entered_box );

               if (window->entered_box && window->entered_box->OnLeave)
                    window->entered_box->OnLeave( window->entered_box, -1, -1 );

               window->entered_box = box;

               if (box->OnEnter)
                    return box->OnEnter( box, x, y );
          }
          else if (box->OnMotion)
               return box->OnMotion( box, x, y, ev->buttons );
     }
     else if (window == entered_window_global)
          handle_leave( window, ev );

     return 0;
}

static int
handle_button( LiteWindow     *window,
               DFBWindowEvent *ev )
{
     DFBResult  ret;
     LiteBox   *box = LITE_BOX(window);

     D_ASSERT( window != NULL );
     D_ASSERT( ev != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p, %p )\n", __FUNCTION__, window, ev );

     if (window->moving || window->resizing) {
          if (ev->type == DWET_BUTTONUP && ev->button == DIBI_LEFT) {
               if (!(window->flags & LITE_WINDOW_MODAL)) {
                    release_grabs();
               }

               window->moving = window->resizing = 0;
          }

          return 1;
     }

     if (window->drag_box) {
          int      x        = ev->x;
          int      y        = ev->y;
          LiteBox *drag_box = window->drag_box;

          child_coords( drag_box, &x, &y );

          switch (ev->type) {
               case DWET_BUTTONDOWN:
                    if (drag_box->OnButtonDown)
                         return drag_box->OnButtonDown( drag_box, x, y, ev->button );
                    break;

               case DWET_BUTTONUP:
                    if (!ev->buttons)
                         lite_release_window_drag_box( window );
                    if (drag_box->OnButtonUp)
                         return drag_box->OnButtonUp( drag_box, x, y, ev->button );
                    break;

               default:
                    break;
          }

          return 0;
     }

     if (DFB_RECTANGLE_CONTAINS_POINT( &box->rect, ev->x, ev->y )) {
          int x = ev->x - box->rect.x;
          int y = ev->y - box->rect.y;

          box = find_child( box, &x, &y );

          if (!box->is_active)
               return 0;

          switch (ev->type) {
               case DWET_BUTTONDOWN:
                    if (!window->drag_box) {
                         if (!(window->flags & LITE_WINDOW_MODAL)) {
                              ret = window->window->GrabPointer(window->window);
                              if (ret) {
                                   DirectFBError( "LiTE/Window: GrabPointer() failed", ret );
                              }
                              else {
                                   window->drag_box = box;
                                   grabbed_window_global = window->window;

                                   D_DEBUG_AT( LiteWindowDomain, "  -> set drag_box %p and grabbed_window_global %p\n",
                                               window->drag_box, grabbed_window_global );
                              }
                         }
                         else {
                              window->drag_box = box;
                         }
                    }

                    if (box->OnButtonDown)
                         return box->OnButtonDown( box, x, y, ev->button );
                    break;

               case DWET_BUTTONUP:
                    if (box->OnButtonUp)
                         return box->OnButtonUp( box, x, y, ev->button );
                    break;

               default:
                    break;
          }
     }
     else if (ev->type == DWET_BUTTONDOWN) {
          if (!(window->flags & LITE_WINDOW_FIXED)) {
               long long diff;

               switch (ev->button) {
                    case DIBI_LEFT:
                         if (!(window->flags & LITE_WINDOW_MODAL)) {
                              ret = window->window->GrabPointer(window->window);
                              if (ret) {
                                    DirectFBError( "LiTE/Window: GrabPointer() failed", ret );
                                    return 0;
                              }
                              else {
                                  grabbed_window_global = window->window;
                              }
                         }

                         diff = (ev->timestamp.tv_sec  - window->last_click.tv_sec) * 1000000 +
                                (ev->timestamp.tv_usec - window->last_click.tv_usec);

                         window->last_click = ev->timestamp;

                         if (ev->x >= (box->rect.x + box->rect.w - 10) && ev->y >= (box->rect.y + box->rect.h)) {
                              if (window->flags & LITE_WINDOW_RESIZE) {
                                 window->resizing = 1;
                              }
                              else {
                                 if (!(window->flags & LITE_WINDOW_MODAL)) {
                                        release_grabs();
                                 }
                              }
                         }
                         else if (ev->y < box->rect.y && diff < 400000) {
                              int            string_width;
                              IDirectFBFont *font;

                              lite_font( window->theme->title_font, &font );
                              font->GetStringWidth( font, window->title, -1, &string_width );

                              /* title double click */
                              if (ev->x > (window->width - string_width) / 2 &&
                                  ev->x < (window->width + string_width) / 2 &&
                                  (window->flags & LITE_WINDOW_MINIMIZE)) {
                                    if (window->width > (window->min_width +
                                                         window->theme->frame.parts[LITE_THEME_FRAME_PART_LEFT].rect.w +
                                                         window->theme->frame.parts[LITE_THEME_FRAME_PART_RIGHT].rect.w) ||
                                        window->height > (window->min_height +
                                                          window->theme->frame.parts[LITE_THEME_FRAME_PART_TOP].rect.h +
                                                          window->theme->frame.parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h))
                                         lite_minimize_window( window );
                                    else
                                         lite_restore_window( window );
                              }

                              if (!(window->flags & LITE_WINDOW_MODAL)) {
                                   release_grabs();
                              }
                         }
                         else {
                              if (ev->x > 0 && ev->y > 0 && ev->x < window->width && ev->y < window->height) {
                                   window->moving = 1;

                                   window->window->RaiseToTop( window->window );
                              }
                              else {
                                  if (!(window->flags & LITE_WINDOW_MODAL)) {
                                       release_grabs();
                                  }
                              }
                         }

                         window->old_x = ev->cx;
                         window->old_y = ev->cy;

                         return 1;

                    case DIBI_RIGHT:
                         window->window->LowerToBottom( window->window );

                         return 1;

                    default:
                         break;
               }
          }
     }

     return 0;
}

static int
handle_key_up( LiteWindow     *window,
               DFBWindowEvent *ev )
{
     D_ASSERT( window != NULL );
     D_ASSERT( ev != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p, %p )\n", __FUNCTION__, window, ev );

     key_modifier_global = ev->modifiers;

     if (window->focused_box) {
          if (window->focused_box->is_visible == 0)
               return 0;

          if (window->focused_box->handle_keys == 0)
               return 0;

          if (!window->focused_box->is_active)
               return 0;

          if (window->focused_box->OnKeyUp)
               return window->focused_box->OnKeyUp( window->focused_box, ev );
     }

     return 0;
}

static int
handle_key_down( LiteWindow     *window,
                 DFBWindowEvent *ev )
{
     D_ASSERT( window != NULL );
     D_ASSERT( ev != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p, %p )\n", __FUNCTION__, window, ev );

     key_modifier_global = ev->modifiers;

     if (window->focused_box) {
          if (window->focused_box->is_visible == 0)
               return 0;

          if (window->focused_box->handle_keys == 0)
               return 0;

          if (!window->focused_box->is_active)
               return 0;

          if (window->focused_box->OnKeyDown)
               return window->focused_box->OnKeyDown( window->focused_box, ev );
     }

     return 0;
}

static int
handle_wheel( LiteWindow     *window,
              DFBWindowEvent *ev )
{
     D_ASSERT( window != NULL );
     D_ASSERT( ev != NULL );

     D_DEBUG_AT( LiteWindowDomain, "%s( %p, %p )\n", __FUNCTION__, window, ev );

     if (window->focused_box) {
          if (window->focused_box->is_visible == 0)
               return 0;

          if (window->focused_box->is_active == 0)
               return 0;

          if (window->focused_box->OnWheel)
               return window->focused_box->OnWheel( window->focused_box, ev );
     }

     return 0;
}

DFBResult
prvlite_release_window_resources()
{
     int n = num_windows_global;

     key_modifier_global = 0;

     release_grabs();

     /* destroy all windows in order of most recent first */
     while (--n >= 0) {
          /* if we deleted a number of windows in the previous iteration, we may need to reset the counter */
          if (n >= num_windows_global) {
               n = num_windows_global;
               continue;
          }

          LiteWindow *window = window_array_global[n];
          /* don't destroy a window with a creator, because the creator will be responsible for destroying the window */
          if (window->creator == NULL) {
               window->internal_ref_count = 0;

               if (!(window->flags & LITE_WINDOW_DESTROYED))
                    destroy_window_data( window );

               handle_destroy( window );
          }
     }

     if (event_buffer_global) {
          event_buffer_global->Release( event_buffer_global );
          event_buffer_global = NULL;
     }

     LiteWindowTimeout *timeout_node = timeout_queue;
     while (timeout_node) {
         LiteWindowTimeout *next = timeout_node->next;
         D_FREE( timeout_node );
         timeout_node = next;
     }

     LiteWindowIdle *idle_node = idle_queue;
     while (idle_node) {
         LiteWindowIdle *next = idle_node->next;
         D_FREE( idle_node );
         idle_node = next;
     }

     return DFB_OK;
}
