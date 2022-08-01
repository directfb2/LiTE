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
 * @brief This file contains definitions for the LiTE box interface.
 * @file box.h
 */

#ifndef __LITE__BOX_H__
#define __LITE__BOX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <directfb.h>

/** @brief Macro to convert a subclassed structure into a generic LiteBox. */
#define LITE_BOX(l) ((LiteBox*) (l))

/** @brief Test for valid LiteBox structure, return DFB_INVARG if invalid. */
#define LITE_BOX_TYPE_PARAMETER_CHECK(box,box_type) \
     do {                                           \
          if (LITE_BOX(box)->type != (box_type)) {  \
               return DFB_INVARG;                   \
          }                                         \
     } while (0)

/**
 * @brief Box type.
 */
typedef enum {
     LITE_TYPE_WINDOW      = 0x1000,        /**< LiteWindow type */
     LITE_TYPE_BOX         = 0x8000,        /**< LiteBox type */
     LITE_TYPE_BUTTON      = 0x8001,        /**< LiteButton type */
     LITE_TYPE_ANIMATION   = 0x8002,        /**< LiteAnimation type */
     LITE_TYPE_IMAGE       = 0x8003,        /**< LiteImage type */
     LITE_TYPE_LABEL       = 0x8004,        /**< LiteLabel type */
     LITE_TYPE_SLIDER      = 0x8005,        /**< LiteSlider type */
     LITE_TYPE_TEXTLINE    = 0x8006,        /**< LiteTextLine type */
     LITE_TYPE_PROGRESSBAR = 0x8007,        /**< LiteProgressBar type */
     LITE_TYPE_TEXT_BUTTON = 0x8008,        /**< LiteTextButton type */
     LITE_TYPE_CHECK       = 0x8009,        /**< LiteCheck type */
     LITE_TYPE_SCROLLBAR   = 0x800A,        /**< LiteScrollbar type */
     LITE_TYPE_LIST        = 0x800B,        /**< LiteList type */
} LiteBoxType;

/**
 * @brief LiteBox structure.
 */
typedef struct _LiteBox {
     struct _LiteBox   *parent;             /**< Parent of the LiteBox */

     int                n_children;         /**< Number of children in child array */
     struct _LiteBox  **children;           /**< Child array */

     LiteBoxType        type;               /**< LiteBox type */
     DFBRectangle       rect;               /**< Rectangle of the LiteBox */
     IDirectFBSurface  *surface;            /**< LiteBox surface */
     DFBColor          *background;         /**< Background color */
     void              *user_data;          /**< User data */
     int                is_focused;         /**< LiteBox is focused or not */
     int                is_visible;         /**< LiteBox is visible or not */
     int                is_active;          /**< LiteBox receives input events or not */
     int                catches_all_events; /**< LiteBox prevents events from being handled by its children or not */
     int                handle_keys;        /**< LiteBox handles keyboard events or not */

     int              (*OnFocusIn)        ( struct _LiteBox                *self );
                                            /**< Focus in callback */

     int              (*OnFocusOut)       ( struct _LiteBox                *self );
                                            /**< Focus out callback  */

     int              (*OnEnter)          ( struct _LiteBox                *self,
                                            int                             x,
                                            int                             y );
                                            /**< Enter callback */

     int              (*OnLeave)          ( struct _LiteBox                *self,
                                            int                             x,
                                            int                             y );
                                            /**< Leave callback */

     int              (*OnMotion)         ( struct _LiteBox                *self,
                                            int                             x,
                                            int                             y,
                                            DFBInputDeviceButtonMask        button_mask );
                                            /**< Motion callback */

     int              (*OnButtonDown)     ( struct _LiteBox                *self,
                                            int                             x,
                                            int                             y,
                                            DFBInputDeviceButtonIdentifier  button_id );
                                            /**< Button down callback */

     int              (*OnButtonUp)       ( struct _LiteBox                *self,
                                            int                             x,
                                            int                             y,
                                            DFBInputDeviceButtonIdentifier  button_id );
                                            /**< Button up callback */

     int              (*OnKeyDown)        ( struct _LiteBox                *self,
                                            DFBWindowEvent                 *evt );
                                            /**< Key down callback */

     int              (*OnKeyUp)          ( struct _LiteBox                *self,
                                            DFBWindowEvent                 *evt );
                                            /**< Key up callback */

     int              (*OnWheel)          ( struct _LiteBox                *self,
                                            DFBWindowEvent                 *evt );
                                            /**< Scroll wheel callback */

     DFBResult        (*Draw)             ( struct _LiteBox                *self,
                                            const DFBRegion                *region,
                                            DFBBoolean                      clear );
                                            /**< Draw callback */

     DFBResult        (*DrawAfter)        ( struct _LiteBox                *self,
                                            const DFBRegion                *region );
                                            /**< DrawAfter callback */

     DFBResult        (*Destroy)          ( struct _LiteBox                *self );
                                            /**< Destroy callback */
} LiteBox;

/**
 * @brief Initialize a LiteBox.
 *
 * This function will initialize a LiteBox.
 *
 * @param[in]  box                           Valid LiteBox
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_init_box                    ( LiteBox *box );

/**
 * @brief Draw the contents of a LiteBox.
 *
 * This function will draw the contents of a LiteBox in the
 * specified region.
 * If NULL is passed, the entire LiteBox will be drawn.
 * Use this operation if you want immediate updates inside the
 * LiteBox but it's more expensive than setting a dirty flag
 * using lite_update_box() and handling all dirty LiteBoxes later
 * in the event loop with a single update.
 *
 * @param[in]  box                           Valid LiteBox
 * @param[in]  region                        Region to draw
 * @param[in]  flip                          Flip is done or not
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_draw_box                    ( LiteBox         *box,
                                             const DFBRegion *region,
                                             DFBBoolean       flip );

/**
 * @brief Update the LiteBox.
 *
 * This function will update the contents of a LiteBox in the
 * specified region by setting a dirty flag. Later in the event
 * loop, all dirty LiteBoxes will be updated with a single update.
 * If NULL is passed, the entire LiteBox will be updated.
 * Use this operation in most cases, as it's less expensive than
 * lite_draw_box().
 *
 * @param[in]  box                           Valid LiteBox
 * @param[in]  region                        Region to update
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_update_box                  ( LiteBox         *box,
                                             const DFBRegion *region );

/**
 * @brief Destroy a LiteBox.
 *
 * This function will destroy a LiteBox and all of its children.
 *
 * @param[in]  box                           Valid LiteBox
 *
 * @return DFB_OK if sucessful.
 */
DFBResult lite_destroy_box                 ( LiteBox *box );

/**
 * @brief Initialize a LiteBox with additional parameters.
 *
 * This function will initialize a LiteBox by specifying its
 * parent and the rectangle for this LiteBox.
 *
 * @param[in]  box                           Valid LiteBox
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  rect                          Rectangle for the LiteBox object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_init_box_at                 ( LiteBox            *box,
                                             LiteBox            *parent,
                                             const DFBRectangle *rect );

/**
 * @brief Reinitialize the LiteBox and its children.
 *
 * This function will reinitialize the LiteBox and all children.
 *
 * @param[in]  box                           Valid LiteBox
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_reinit_box_and_children     ( LiteBox *box );

/**
 * @brief Clear the contents of a LiteBox.
 *
 * This function will clear the contents of a LiteBox in the
 * specified region.
 * If NULL is passed, the entire LiteBox will be cleared.
 * This operation includes updates to the parent area.
 *
 * @param[in]  box                           Valid LiteBox
 * @param[in]  region                        Region
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_clear_box                   ( LiteBox         *box,
                                             const DFBRegion *region );

/**
 * @brief Add a LiteBox child in the parent's child array.
 *
 * This function will add a LiteBox child in the parent's child
 * array. This can be used to change parent/child hierarchy.
 *
 * @param[in]  parent                        Valid LiteBox
 * @param[in]  child                         Valid LiteBox
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_add_child                   ( LiteBox *parent,
                                             LiteBox *child );

/**
 * @brief Remove a LiteBox child from the parent's child array.
 *
 * This function will remove a LiteBox child from a parent's child
 * array. This can be used to change parent/child hierarchy.
 *
 * @param[in]  parent                        Valid LiteBox
 * @param[in]  child                         Valid LiteBox
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_remove_child                ( LiteBox *parent,
                                             LiteBox *child );

/**
 * @brief Change the visibility of a LiteBox.
 *
 * This function will change the visibility of a LiteBox.
 *
 * @param[in]  box                           Valid LiteBox
 * @param[in]  visible                       Visible or not
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_box_visible             ( LiteBox *box,
                                             int      visible );

/**
 * @brief Set focus to a specific LiteBox.
 *
 * This function will set focus on a LiteBox of the window.
 *
 * @param[in]  box                           LiteBox to be focused
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_focus_box                   ( LiteBox *box );

#ifdef __cplusplus
}
#endif

#endif
