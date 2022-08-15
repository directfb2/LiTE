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

/**
 * @brief This file contains definitions for the LiTE window interface.
 * @file window.h
 */

#ifndef __LITE__WINDOW_H__
#define __LITE__WINDOW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <direct/thread.h>
#include <lite/font.h>
#include <lite/theme.h>

/** @brief Maximum number of update areas. */
#define LITE_WINDOW_MAX_UPDATES 4

/** @brief Macro to convert a generic LiteBox into a LiteWindow. */
#define LITE_WINDOW(l) ((LiteWindow*) (l))

/** @brief Test for valid LiteWindow structure, return DFB_INVARG if invalid. */
#define LITE_WINDOW_PARAMETER_CHECK(win)                  \
     do {                                                 \
         if (!(LITE_BOX(win)->type >= LITE_TYPE_WINDOW && \
               LITE_BOX(win)->type < LITE_TYPE_BOX)) {    \
             return DFB_INVARG;                           \
         }                                                \
     } while (0)

/** @brief Window blend mode. */
typedef enum {
     LITE_BLEND_ALWAYS,                                   /**< Always blend */
     LITE_BLEND_NEVER,                                    /**< Never blend */
     LITE_BLEND_AUTO                                      /**< Automatically blend */
} LiteBlendMode;

/** @brief Window flags. */
typedef enum {
    LITE_WINDOW_MODAL          = 1 << 1,                  /**< Modal window */
    LITE_WINDOW_RESIZE         = 1 << 2,                  /**< Resizable window */
    LITE_WINDOW_MINIMIZE       = 1 << 3,                  /**< Window that can be minimized */

    LITE_WINDOW_DESTROYED      = 1 << 5,                  /**< Window is marked for destruction */
    LITE_WINDOW_FIXED          = 1 << 6,                  /**< Window can't be moved or resized */
    LITE_WINDOW_DRAWN          = 1 << 7,                  /**< Window has been rendered at least once */
    LITE_WINDOW_PENDING_RESIZE = 1 << 8,                  /**< At least one resize event is pending */
    LITE_WINDOW_DISABLED       = 1 << 9,                  /**< Window does not respond to events */
    LITE_WINDOW_CONFIGURED     = 1 << 10                  /**< Window received the initial position/size event */
} LiteWindowFlags;

/** @brief Window alignment flags. */
typedef enum {
     LITE_CENTER_HORIZONTALLY = -1,                       /**< Center window on x-axis */
     LITE_CENTER_VERTICALLY   = -2                        /**< Center window on y-axis */
} LiteAlignmentFlags;

/** @brief Full window opacity level (no alpha blending). */
#define liteFullWindowOpacity 0xff

/** @brief No window opacity (window invisible). */
#define liteNoWindowOpacity 0x00

/** @brief LiteWindow theme. */
typedef struct {
     LiteTheme       theme;                               /**< Base LiTE Theme */

     LiteFont       *title_font;                          /**< Title font */
     LiteThemeFrame  frame;                               /**< Frame bitmaps */
} LiteWindowTheme;

/** @brief No window theme. */
#define liteNoWindowTheme NULL

/** @brief Default window theme. */
extern LiteWindowTheme *liteDefaultWindowTheme;

/** @brief Window event callback. */
typedef DFBResult (*LiteWindowEventFunc) (DFBWindowEvent* evt, void *data);

/** @brief Window universal event callback. */
typedef DFBResult (*LiteWindowUniversalEventFunc) (DFBUniversalEvent* evt, void *data);

/** @brief Window user event callback. */
typedef DFBResult (*LiteWindowUserEventFunc) (DFBUserEvent* evt, void *data);

/** @brief LiteWindow structure. */
typedef struct _LiteWindow {
     LiteBox                        box;                  /**< Underlying LiteBox */

     struct _LiteWindow            *creator;              /**< Creator window */

     int                            width;                /**< Window width */
     int                            height;               /**< Window height */
     u8                             opacity;              /**< Window opacity */
     DFBWindowID                    id;                   /**< Window ID */
     char                          *title;                /**< Window title */
     IDirectFBWindow               *window;               /**< Underlying DirectFB window */
     IDirectFBSurface              *surface;              /**< Underlying DirectFB surface */
     LiteWindowFlags                flags;                /**< Window flags */
     int                            moving;               /**< Window moving */
     int                            resizing;             /**< Window resizing */
     int                            old_x;                /**< Original x position */
     int                            old_y;                /**< Original y position */
     int                            step_x;               /**< X step */
     int                            step_y;               /**< Y step */
     int                            min_width;            /**< Minimum width */
     int                            min_height;           /**< Minimum height */
     int                            last_width;           /**< Last width */
     int                            last_height;          /**< Last height */
     DFBWindowEvent                 last_resize;          /**< Last resize event */
     DFBWindowEvent                 last_motion;          /**< Last motion event */
     struct timeval                 last_click;           /**< Last time window was clicked */
     int                            has_focus;            /**< Window has focus or not */

     LiteWindowEventFunc            raw_mouse_func;       /**< Raw mouse callback */
     void                          *raw_mouse_data;       /**< Raw mouse callback data*/
     LiteWindowEventFunc            raw_mouse_moved_func; /**< Raw mouse move callback */
     void                          *raw_mouse_moved_data; /**< Raw mouse move callback data */
     LiteWindowEventFunc            mouse_func;           /**< Mouse callback */
     void                          *mouse_data;           /**< Mouse callback data */
     LiteWindowEventFunc            raw_keyboard_func;    /**< Raw keyboard callback */
     void                          *raw_keyboard_data;    /**< Raw keyboard callback data */
     LiteWindowEventFunc            keyboard_func;        /**< Keyboard callback */
     void                          *keyboard_data;        /**< Keyboard callback data */
     LiteWindowEventFunc            window_event_func;    /**< Window event callback */
     void                          *window_event_data;    /**< Window event callback data */
     LiteWindowUniversalEventFunc   universal_event_func; /**< Universal event callback */
     void                          *universal_event_data; /**< Universal event callback data */
     LiteWindowUserEventFunc        user_event_func;      /**< User event callback */
     void                          *user_event_data;      /**< User event callback data */
     LiteWindowEventFunc            raw_wheel_func;       /**< Raw scroll wheel callback */
     void                          *raw_wheel_data;       /**< Raw scroll wheel callback data*/
     LiteWindowEventFunc            wheel_func;           /**< Scroll wheel callback */
     void                          *wheel_data;           /**< Scroll wheel callback data */

     struct {
          DirectMutex               lock;
          int                       pending;
          DFBRegion                 regions[LITE_WINDOW_MAX_UPDATES];
     } updates;                                           /**< Update areas */

     LiteBlendMode                  content_mode;         /**< Content blend mode */
     LiteBlendMode                  opacity_mode;         /**< Opacity blend mode */

     struct {
          DFBBoolean                enabled;
          DFBColor                  color;
     } bg;                                                /**< Background color*/

     LiteBox                       *entered_box;          /**< Current entered box */
     LiteBox                       *focused_box;          /**< Current focused box */
     LiteBox                       *drag_box;             /**< Current drag box */

     LiteWindowTheme               *theme;                /**< Window theme */

     DFBRectangle                   frame_target[LITE_THEME_FRAME_PART_NUM];
                                                          /**< Rectangles of the frame on window */

     int                          (*OnMove)             ( struct _LiteWindow *self,
                                                          int                 x,
                                                          int                 y );
                                                          /**< Move callback */

     int                          (*OnResize)           ( struct _LiteWindow *self,
                                                          int                 width,
                                                          int                 height );
                                                          /**< Resize callback */

     int                          (*OnClose)            ( struct _LiteWindow *self );
                                                          /**< Close callback */

     int                          (*OnDestroy)          ( struct _LiteWindow *self );
                                                          /**< Destroy callback */

     int                          (*OnFocusIn)          ( struct _LiteWindow *self );
                                                          /**< Focus in callback */

     int                          (*OnFocusOut)         ( struct _LiteWindow *self );
                                                          /**< Focus out callback */

     int                          (*OnEnter)            ( struct _LiteWindow *self,
                                                          int                 x,
                                                          int                 y );
                                                          /**< Enter callback */

     int                          (*OnLeave)            ( struct _LiteWindow *self,
                                                          int                 x,
                                                          int                 y );
                                                          /**< Leave callback */

     int                          (*OnBoxAdded)         ( struct _LiteWindow *self,
                                                          LiteBox            *box );
                                                          /**< Box added callback */

     int                          (*OnBoxToBeRemoved)   ( struct _LiteWindow *self,
                                                          LiteBox            *box );
                                                          /**< Box to be Removed callback */

     int                            internal_ref_count;   /**< Event loop reference count */

     DFBColor                       title_color;          /**< Title color */
     int                            title_x_offset;       /**< Title x offset */
     int                            title_y_offset;       /**< Title y offset */
} LiteWindow;

/** @brief Callback function prototype when a timeout occurs in event loop or when the event loop becomes idle. */
typedef DFBResult (*LiteTimeoutFunc)( void *data );

/**
 * @brief Create a new LiteWindow object.
 *
 * This function will create a new LiteWindow object. You can pass
 * size, layer used, various capabilities (like alpha blending),
 * and a title when using a window theme with frame.
 *
 * @param[in]  layer                         Window display layer (if NULL the default layer is used)
 * @param[in]  rect                          Rectangle for the LiteWindow object
 * @param[in]  caps                          Window capabilities
 * @param[in]  theme                         Window theme
 * @param[in]  title                         Window title
 * @param[out] ret_window                    Valid LiteWindow object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_window                  ( IDirectFBDisplayLayer  *layer,
                                             DFBRectangle           *rect,
                                             DFBWindowCapabilities   caps,
                                             LiteWindowTheme        *theme,
                                             const char             *title,
                                             LiteWindow            **ret_window );

/**
 * @brief Set the window creator of a specific window.
 *
 * This function will store the original window that created a
 * specific window.
 *
 * @param[in]  window                        The new LiteWindow object that needs to know its creator
 * @param[in]  creator                       The LiteWindow that created this new LiteWindow object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_window_set_creator          ( LiteWindow *window,
                                             LiteWindow *creator );

/**
 * @brief Get the window creator of a specific window.
 *
 * This function will get the creator window of a specific window.
 * It is assumed that an earlier lite_window_set_creator() has
 * been called. If not, NULL will be returned.
 *
 * @param[in]  window                        The LiteWindow that needs to know about its creator
 * @param[out] ret_creator                   The original LiteWindow that created this LiteWindow
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_window_get_creator          ( LiteWindow  *window,
                                             LiteWindow **ret_creator );

/**
 * @brief Set the modal state of a window.
 *
 * This function will set the modal state of a window.
 * If enabled, the window will be the only one to receive events.
 * If disabled, all created windows will receive events.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  modal                         Modal state of the window
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_window_set_modal            ( LiteWindow *window,
                                             int         modal );

/**
 * @brief Start the window event loop.
 *
 * This function starts the event loop. You just have to do this
 * for one (main) window, all other windows will also receive
 * events unless a window has been set to modal state.
 * If you pass a timeout value, the event loop will run until
 * the timeout expires.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  timeout                       Timeout value for the event loop,
 *                                            0 means don't leave the event loop after timeout,
 *                                           -1 means to run the event loop once and return
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_window_event_loop           ( LiteWindow *window,
                                             int         timeout );

/**
 * @brief Exit the event loop.
 *
 * This function will exit the current event loop.
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_exit_event_loop             ( void );

/**
 * @brief Set state to exit event loop when idle.
 *
 * This function will enable or disable the state indicating to
 * exit the event loop if no pending events are available.
 *
 * @param[in]  state                         Exit the event loop if idle or not
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_exit_idle_loop          ( int state );

/**
 * @brief Enqueue a timeout callback.
 *
 * This function enqueues a timeout callback that will be called
 * in the future. These callback functions are called from the
 * window event loop and are one-shot.
 *
 * @param[in]  timeout                       Time from now to active callback function
 * @param[in]  callback                      Callback function to call after timeout, NULL to exit event loop at timeout
 * @param[in]  callback_data                 Context data to pass to the callback
 * @param[out] ret_timeout_id                ID of the new timeout callback
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_enqueue_timeout_callback    ( int              timeout,
                                             LiteTimeoutFunc  callback,
                                             void            *callback_data,
                                             int             *ret_timeout_id );

/**
 * @brief Remove a timeout callback from the queue.
 *
 * This function removes a timeout callback that was previously
 * added using lite_enqueue_timeout_callback(). This is used to
 * remove a timeout callback that is no longer needed before it
 * is called.
 *
 * @param[in]  timeout_id                    ID of the timeout callback to remove
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_remove_timeout_callback     ( int timeout_id );

/**
 * @brief Adjust timeouts for time change.
 *
 * This function will rebase all timeouts. This can be used when
 * the system time changes.
 *
 * @param[in]  adjustment                    Adjustment in milliseconds to apply to all timeouts
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_rebase_window_timeouts      ( long long adjustment );

/**
 * @brief Enqueue an idle callback.
 *
 * This function enqueues an idle callback that will be called
 * in the future. These callback functions are called from the
 * window event loop and are one-shot.
 *
 * @param[in]  callback                      Callback function to call on idle, NULL to exit event loop at idle
 * @param[in]  callback_data                 Context data to pass to the callback
 * @param[out] ret_idle_id                   ID of the new idle callback
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_enqueue_idle_callback       ( LiteTimeoutFunc  callback,
                                             void            *callback_data,
                                             int             *ret_idle_id );

/**
 * @brief Remove an idle callback from the queue.
 *
 * This function removes an idle callback that was previously
 * added using lite_enqueue_idle_callback(). This is used to
 * remove an idle callback that is no longer needed before it
 * is called.
 *
 * @param[in]  idle_id                       ID of the idle callback to remove
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_remove_idle_callback        ( int idle_id );

/**
 * @brief Update window.
 *
 * This function will update the window in the specified region.
 * If NULL is passed, the whole window area will be updated.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  region                        Window region
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_update_window               ( LiteWindow      *window,
                                             const DFBRegion *region );

/**
 * @brief Update all windows.
 *
 * This function will update all windows.
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_update_all_windows          ( void );

/**
 * @brief Set the window title.
 *
 * This function will set the window title.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  title                         Window title
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_window_title            ( LiteWindow *window,
                                             const char *title );

/**
 * @brief Enable/disable window.
 *
 * This function will tell a window if it should handle the events
 * sent to it. Destroyed events will still be handled.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  enabled                       Events handled or not
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_window_enabled          ( LiteWindow *window,
                                             int         enabled );

/**
 * @brief Set the window opacity level.
 *
 * This function will set the window opacity level. After creating
 * a window, the opacity level is 0 (liteNoWindowOpacity), so to
 * show the window, you need to set it to a higher value such as
 * 0xff (liteFullWindowOpacity).
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  opacity                       Opacity level (0 to 0xff)
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_window_opacity          ( LiteWindow *window,
                                             u8          opacity );

/**
 * @brief Set window background color.
 *
 * This function will set the background color of the window.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  bg_color                      Background color
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_window_background       ( LiteWindow     *window,
                                             const DFBColor *bg_color );

/**
 * @brief Set the window blend mode.
 *
 * This function will set the window blend mode.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  content                       Content blend mode
 * @param[in]  opacity                       Opacity blend mode
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_window_blend_mode       ( LiteWindow   *window,
                                             LiteBlendMode content,
                                             LiteBlendMode opacity );

/**
 * @brief Resize window.
 *
 * This function will resize the window.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  width                         New window width
 * @param[in]  height                        New window height
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_resize_window               ( LiteWindow   *window,
                                             unsigned int  width,
                                             unsigned int  height );

/**
 * @brief Set window position and size.
 *
 * This function will move and resize the window in one operation.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  x                             New x position
 * @param[in]  y                             New y position
 * @param[in]  width                         New window width
 * @param[in]  height                        New window height
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_window_bounds           ( LiteWindow   *window,
                                             int           x,
                                             int           y,
                                             unsigned int  width,
                                             unsigned int  height );

/**
 * @brief Get window size.
 *
 * This function will get the current window size (without the
 * possible frame).
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[out] ret_width                     Window width
 * @param[out] ret_height                    Window height
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_window_size             ( LiteWindow *window,
                                             int        *ret_width,
                                             int        *ret_height );

/**
 * @brief Find the window to which a LiteBox belongs.
 *
 * This function will find which window a specific LiteBox
 * belongs to.
 *
 * @param[in]  box                           Valid LiteBox
 *
 * @return DFB_OK if successful.
 */
LiteWindow *lite_find_my_window            ( LiteBox *box );

/**
 * @brief Minimize window.
 *
 * This function will minimize the window.
 *
 * @param[in]  window                        Valid LiteWindow object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_minimize_window             ( LiteWindow *window );

/**
 * @brief Maximize window.
 *
 * This function will restore the window if the window has been
 * minimized.
 *
 * @param[in]  window                        Valid LiteWindow object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_restore_window              ( LiteWindow *window );

/**
 * @brief Remove drag box attribute of a window.
 *
 * This function will release all grabs taken to manage the
 * drag box.
 *
 * @param[in]  window                        Valid LiteWindow object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_release_window_drag_box     ( LiteWindow *window );

/**
 * @brief Get the underlying IDirectFBEventBuffer interface.
 *
 * This function will access the underlying IDirectFBEventBuffer
 * used by the event loop.
 *
 * @param[out] ret_event_buffer              IDirectFBEventBuffer object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_get_event_buffer            ( IDirectFBEventBuffer **ret_event_buffer );

/**
 * @brief Post custom events.
 *
 * This function will post events to the event queue.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  event                         Custom event
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_post_event_to_window        ( LiteWindow *window,
                                             DFBEvent   *event );

/**
 * @brief Check if an event is available.
 *
 * This function is used to see if there are any pending events
 * or timeouts ready to be called.
 *
 * @return DFB_OK if events or timeouts,
 *         DFB_BUFFEREMPTY otherwise.
 */
DFBResult lite_window_event_available      ( void );

/**
 * @brief Handle window events.
 *
 * This function will handle incoming window events.
 * It is called from the window's event loop by default, and it
 * is generally not needed unless you want to call window event
 * handling from external code.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  event                         Valid window event
 *
 * @return 1 if event was handled,
 *         0 otherwise.
 */
int lite_handle_window_event               ( LiteWindow     *window,
                                             DFBWindowEvent *event );

/**
 * @brief Get the current key modifier.
 *
 * This function is used to get the currently active key modifier.
 *
 * @return the value of the current key modifier.
 */
int lite_get_current_key_modifier          ( void );

/**
 * @brief Flush window events.
 *
 * This function will flush pending window events.
 *
 * @param[in]  window                       Valid LiteWindow object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_flush_window_events         ( LiteWindow *window );

/**
 * @brief Close window.
 *
 * This function will close a window. If this window is the last one
 * in the application, it will cause the event loop to exit.
 *
 * @param[in]  window                        Valid LiteWindow object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_close_window                ( LiteWindow *window );

/**
 * @brief Destroy window.
 *
 * This function will close a window and destroy its resources.
 *
 * @param[in]  window                        Valid LiteWindow object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_destroy_window              ( LiteWindow *window );

/**
 * @brief Destroy all windows.
 *
 * This function will destroy all application windows.
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_destroy_all_windows         ( void );

/**
 * @brief Install a raw mouse event callback.
 *
 * This function will install a raw mouse event callback
 * that is triggered before the event loop processes the mouse
 * button down or up event.
 * If the callback does not return DFB_OK, then event handling
 * for this mouse event is terminated.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  callback                      Callback to trigger
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_raw_window_mouse         ( LiteWindow          *window,
                                             LiteWindowEventFunc  callback,
                                             void                *data );

/**
 * @brief Install a raw mouse move event callback.
 *
 * This function will install a raw mouse move event callback
 * that is triggered before the event loop processes the mouse
 * move event.
 * If the callback does not return DFB_OK, then event handling
 * for this mouse move event is terminated.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  callback                      Callback to trigger
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_raw_window_mouse_moved   ( LiteWindow          *window,
                                             LiteWindowEventFunc  callback,
                                             void                *data );

/**
 * @brief Install a mouse event callback.
 *
 * This function will install a mouse event callback that is
 * triggered after the event loop processes the mouse event.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  callback                      Callback to trigger
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_window_mouse             ( LiteWindow          *window,
                                             LiteWindowEventFunc  callback,
                                             void                *data );

/**
 * @brief Install a raw keyboard event callback.
 *
 * This function will install a raw keyboard event callback
 * that is triggered before the event loop processes the keyboard
 * event.
 * If the callback does not return DFB_OK, then event handling
 * for this keyboard event is terminated.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  callback                      Callback to trigger
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_raw_window_keyboard      ( LiteWindow          *window,
                                             LiteWindowEventFunc  callback,
                                             void                *data );

/**
 * @brief Install a keyboard event callback.
 *
 * This function will install a keyboard event callback that is
 * triggered after the event loop processes the keyboard event.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  callback                      Callback to trigger
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_window_keyboard          ( LiteWindow          *window,
                                             LiteWindowEventFunc  callback,
                                             void                *data );

/**
 * @brief Install a window event callback.
 *
 * This function will install a window event callback that is
 * triggered before the actual event processing.
 * This can be used to filter or override any default window
 * event processing.
 * If the callback does not return DFB_OK, then event handling
 * for this window event is terminated.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  callback                      Callback to trigger
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_window_event             ( LiteWindow          *window,
                                             LiteWindowEventFunc  callback,
                                             void                *data );

/**
 * @brief Install a universal event callback.
 *
 * This function will install a universal event callback that is
 * triggered when the event loop is processing universal events.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  callback                      Callback to trigger
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_window_universal_event   ( LiteWindow                   *window,
                                             LiteWindowUniversalEventFunc  callback,
                                             void                         *data );

/**
 * @brief Install a user event callback.
 *
 * This function will install a user event callback that is
 * triggered when the event loop is processing user events.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  callback                      Callback to trigger
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_window_user_event        ( LiteWindow              *window,
                                             LiteWindowUserEventFunc  callback,
                                             void                    *data );

/**
 * @brief Install a raw scroll wheel event callback.
 *
 * This function will install a raw scroll wheel event callback
 * that is triggered before the event loop processes the scroll
 * wheel event.
 * If the callback does not return DFB_OK, then event handling
 * for this scroll wheel event is terminated.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  callback                      Callback to trigger
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_raw_window_wheel         ( LiteWindow          *window,
                                             LiteWindowEventFunc  callback,
                                             void                *data );

/**
 * @brief Install a scroll wheel event callback.
 *
 * This function will install a scroll wheel event callback that is
 * triggered after the event loop processes the scroll wheel event.
 *
 * @param[in]  window                        Valid LiteWindow object
 * @param[in]  callback                      Callback to trigger
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_on_window_wheel             ( LiteWindow          *window,
                                             LiteWindowEventFunc  callback,
                                             void                *data );

/**
 * @brief Create a window theme.
 *
 * This function makes the theme.
 *
 * @param[in]  bg_color                      Background color
 * @param[in]  spec                          Title font specification
 * @param[in]  style                         Title font style
 * @param[in]  size                          Title font size
 * @param[in]  attr                          Title font attributes
 * @param[in]  image_paths                   File paths with window frame images
 * @param[out] ret_theme                     New theme
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_window_theme            ( const DFBColor     *bg_color,
                                             const char         *spec,
                                             LiteFontStyle       style,
                                             int                 size,
                                             DFBFontAttributes   attr,
                                             const char         *image_paths[LITE_THEME_FRAME_PART_NUM],
                                             LiteWindowTheme   **ret_theme );

/**
 * @brief Destroy a window theme.
 *
 * This function will release the theme resources.
 *
 * @param[in]  theme                         Theme to destroy
 *
 * @return DFB_OK If successful.
 */
DFBResult lite_destroy_window_theme        ( LiteWindowTheme *theme );

#ifdef __cplusplus
}
#endif

#endif
