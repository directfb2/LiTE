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
 * @brief This file contains definitions for the LiTE list interface.
 * @file list.h
 */

#ifndef __LITE__LIST_H__
#define __LITE__LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lite/scrollbar.h>

/** @brief Macro to convert a generic LiteBox into a LiteList. */
#define LITE_LIST(l) ((LiteList*) (l))

/** @brief LiteListItemData type. */
typedef unsigned long LiteListItemData;

/** @brief LiteListDrawItem structure. */
typedef struct {
     int               index_item;        /**< Index corresponding to the item */
     LiteListItemData  item_data;         /**< Item data value */
     IDirectFBSurface *surface;           /**< LiteBox surface */
     DFBRectangle      rc_item;           /**< Rectangle for the item */
     int               selected;          /**< Item currently selected or not */
     int               disabled;          /**< List disabled or not */
} LiteListDrawItem;

/** @brief LiteList theme. */
typedef struct {
     LiteTheme           theme;           /**< Base LiTE theme */

     LiteScrollbarTheme *scrollbar_theme; /**< Vertical scrollbar theme */
} LiteListTheme;

/** @brief No list theme. */
#define liteNoListTheme NULL

/** @brief Default list theme. */
extern LiteListTheme *liteDefaultListTheme;

/** @brief LiteList structure. */
typedef struct _LiteList LiteList;

/** @brief Callback function prototype for comparing data items. */
typedef int (*LiteListCompareFunc)( const LiteListItemData *item_data1, const LiteListItemData *item_data2 );

/** @brief Callback function prototype for list selection change. */
typedef void (*LiteListSelChangeFunc)( LiteList *list, int index_item, void *data );

/** @brief Callback function prototype for drawing list items. */
typedef void (*LiteListDrawItemFunc)( LiteList *list, LiteListDrawItem *draw_item, void *data );

/**
 * @brief Create a new LiteList object.
 *
 * This function will create a new LiteList object.
 *
 * @param[in]  parent                        Valid parent LiteBox
 * @param[in]  rect                          Rectangle for the LiteList object
 * @param[in]  theme                         List theme
 * @param[out] ret_list                      Valid LiteList object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_list                    ( LiteBox        *parent,
                                             DFBRectangle   *rect,
                                             LiteListTheme  *theme,
                                             LiteList      **ret_list );

/**
 * @brief Set row height of list items.
 *
 * This function will set the row height of the list items.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  row_height                    Row height
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_set_row_height         ( LiteList *list,
                                             int       row_height);

/**
 * @brief Get row height of list items.
 *
 * This function will get the row height of the list items.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[out] ret_row_height                Row height
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_get_row_height         ( LiteList *list,
                                             int      *ret_row_height );

/**
 * @brief Enable/disable list.
 *
 * This function will enable or disable the list.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  enabled                       Enabled or not
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_enable_list                 ( LiteList *list,
                                             int       enabled );

/**
 * @brief Insert a data item into the list.
 *
 * This function will insert a data item into the list. If index
 * is a negative value or out of range, it is inserted at the end.
 * The list doesn't care what the data is: it just treats data as a
 * ListItemData type (4 byte value). The data value of the inserted
 * item will be passed to the LiteListDrawItemFunc callback.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  index                         Index
 * @param[in]  item_data                     Item data value
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_insert_item            ( LiteList         *list,
                                             int               index,
                                             LiteListItemData  item_data );

/**
 * @brief Get the data value corresponding to a list item.
 *
 * This function will retrieve the data value of a list item.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  index                         Index
 * @param[out] ret_item_data                 Item data value
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_get_item               ( LiteList         *list,
                                             int               index,
                                             LiteListItemData *ret_item_data );

/**
 * @brief Set the data value of an item in the list.
 *
 * This function will replace the data value of an item in the
 * list with a new value.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  index                         Index
 * @param[in]  item_data                     Item data value
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_set_item               ( LiteList         *list,
                                             int               index,
                                             LiteListItemData  item_data );

/**
 * @brief Delete an item from the list.
 *
 * This function will remove an item from the list.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  index                         Index
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_del_item               ( LiteList *list,
                                             int       index );

/**
 * @brief Get total number of items in list.
 *
 * This function will retrieve the number of items in the list.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[out] ret_count                     Number of items
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_get_item_count         ( LiteList *list,
                                             int      *ret_count );

/**
 * @brief Sort list items.
 *
 * This function will sort the items of the list according to a
 * comparison function.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  compare                       Comparison function
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_sort_items             ( LiteList            *list,
                                             LiteListCompareFunc  compare );

/**
 * @brief Select a new item from the list.
 *
 * This function will set the item at the specified index as
 * selected.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  index                         Index
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_set_selected_item_index( LiteList *list,
                                             int       index );

/**
 * @brief Get the index of the currently selected item.
 *
 * This function will get the index of the currently selected item.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[out] ret_index                     Index
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_get_selected_item_index( LiteList *list,
                                             int      *ret_index );

/**
 * @brief Scroll the list so that an item is visible.
 *
 * This function will scroll the list to ensure that the item at
 * the specified index is visible.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  index                         Index
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_ensure_visible         ( LiteList *list,
                                             int       index);

/**
 * @brief Request to update the internal vertical scrollbar.
 *
 * This function will update the vertical scrollbar used by the
 * list and must be called each time the list is resized.
 *
 * @param[in]  list                          Valid LiteList object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_recalc_layout          ( LiteList *list);

/**
 * @brief Set the LiteScrollbar object used by the list.
 *
 * This function will set the LiteScrollbar object used by the
 * list.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  scrollbar                     Valid LiteScrollbar object
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_set_list_scrollbar          ( LiteList      *list,
                                             LiteScrollbar *scrollbar );

/**
 * @brief Install a callback function to draw list items.
 *
 * This function will install a callback triggered when an item
 * in the list needs to be drawn.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  callback                      Callback function
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_on_draw_item           ( LiteList             *list,
                                             LiteListDrawItemFunc  callback,
                                             void                 *data );

/**
 * @brief Install a callback function for list selection change.
 *
 * This function will install a callback triggered when changing
 * the selected item in the list.
 *
 * @param[in]  list                          Valid LiteList object
 * @param[in]  callback                      Callback function
 * @param[in]  data                          Context data
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_list_on_sel_change          ( LiteList              *list,
                                             LiteListSelChangeFunc  callback,
                                             void                  *data );

/**
 * @brief Create a list theme.
 *
 * This function makes the theme.
 *
 * @param[in]  image_path                    File path with image for all scrollbar subsections (button1, button2, thumb)
 * @param[in]  image_margin                  Thumb subsection pixel margin
 * @param[out] ret_theme                     New theme
 *
 * @return DFB_OK if successful.
 */
DFBResult lite_new_list_theme              ( const char     *image_path,
                                             int             image_margin,
                                             LiteListTheme **ret_theme );

/**
 * @brief Destroy a list theme.
 *
 * This function will release the theme resources.
 *
 * @param[in]  theme                         Theme to destroy
 *
 * @return DFB_OK If successful.
 */
DFBResult lite_destroy_list_theme          ( LiteListTheme *theme );

#ifdef __cplusplus
}
#endif

#endif
