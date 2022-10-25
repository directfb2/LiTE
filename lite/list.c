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
#include <lite/list.h>
#include <lite/lite_internal.h>

D_DEBUG_DOMAIN( LiteListDomain, "LiTE/List", "LiTE List" );

/**********************************************************************************************************************/

LiteListTheme *liteDefaultListTheme = NULL;

struct _LiteList {
     LiteBox                box;
     LiteListTheme         *theme;

     LiteScrollbar         *scrollbar;
     int                    item_count;
     int                    cur_item_index;
     int                    enabled;
     int                    row_height;
     LiteListItemData      *item_data_array;

     LiteListSelChangeFunc  sel_change;
     void                  *sel_change_data;
     LiteListDrawItemFunc   draw_item;
     void                  *draw_item_data;
};

static int       on_button_down( LiteBox *box, int x, int y, DFBInputDeviceButtonIdentifier button_id );
static int       on_key_down   ( LiteBox *box, DFBWindowEvent *evt );
static DFBResult draw_list     ( LiteBox *box, const DFBRegion *region, DFBBoolean clear );
static DFBResult destroy_list  ( LiteBox *box );

static void get_scrollbar_rect( LiteList *list, DFBRectangle *ret_rect );
static void update_scrollbar  ( LiteList *list );

/**********************************************************************************************************************/

static void
scrollbar_updated( LiteScrollbar  *scrollbar,
                   LiteScrollInfo *info,
                   void           *data )
{
     LiteList *list = LITE_LIST(data);

     lite_update_box( &list->box, NULL );
}

DFBResult
lite_new_list( LiteBox        *parent,
               DFBRectangle   *rect,
               LiteListTheme  *theme,
               LiteList      **ret_list )
{
     DFBResult     ret;
     DFBRectangle  scrollbar_rect;
     LiteList     *list;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_list );

     list = D_CALLOC( 1, sizeof(LiteList) );

     list->box.parent = parent;
     list->box.rect   = *rect;
     list->theme      = theme;

     ret = lite_init_box( LITE_BOX(list) );
     if (ret != DFB_OK) {
          D_FREE( list );
          return ret;
     }

     if (list->theme != liteNoListTheme) {
          get_scrollbar_rect( list, &scrollbar_rect );

          /* vertical scrollbar */
          ret = lite_new_scrollbar( LITE_BOX(list), &scrollbar_rect, true, theme->scrollbar_theme, &list->scrollbar );
          if (ret != DFB_OK) {
               D_FREE( list );
               return ret;
          }

          lite_on_scrollbar_update( list->scrollbar, scrollbar_updated, list );
     }

     list->box.type         = LITE_TYPE_LIST;
     list->box.OnButtonDown = on_button_down;
     list->box.OnKeyDown    = on_key_down;
     list->box.Draw         = draw_list;
     list->box.Destroy      = destroy_list;
     list->cur_item_index   = -1;
     list->enabled          = 1;
     list->row_height       = 20;

     *ret_list = list;

     D_DEBUG_AT( LiteListDomain, "Created new list object: %p\n", list );

     return DFB_OK;
}

DFBResult
lite_list_set_row_height( LiteList *list,
                          int       row_height )
{
     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     if (row_height < 1)
          return DFB_INVARG;

     D_DEBUG_AT( LiteListDomain, "Set list: %p with row height: %d\n", list, row_height );

     if (list->row_height == row_height)
          return DFB_OK;

     list->row_height = row_height;

     update_scrollbar( list );

     return lite_update_box( &list->box, NULL );
}

DFBResult
lite_list_get_row_height( LiteList *list,
                          int      *ret_row_height )
{
     LITE_NULL_PARAMETER_CHECK( list );
     LITE_NULL_PARAMETER_CHECK( ret_row_height );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     D_DEBUG_AT( LiteListDomain, "list: %p has row height: %d\n", list, list->row_height );

     *ret_row_height = list->row_height;

     return DFB_OK;
}

DFBResult
lite_enable_list( LiteList *list,
                  int       enabled )
{
     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     D_DEBUG_AT( LiteListDomain, "%s list: %p\n", enabled ? "Enable" : "Disable", list );

     if (list->enabled == enabled)
          return DFB_OK;

     list->enabled = enabled;

     list->box.is_active = enabled;

     if (list->scrollbar) {
          if (!list->enabled)
               lite_enable_scrollbar( list->scrollbar, false );
          else
               lite_enable_scrollbar( list->scrollbar, true );
     }

     return lite_update_box( &list->box, NULL );
}

DFBResult
lite_list_insert_item( LiteList         *list,
                       int               index,
                       LiteListItemData  item_data )
{
     LiteListItemData *item_data_array;

     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     if (index < 0 || index > list->item_count)
          index = list->item_count;

     D_DEBUG_AT( LiteListDomain, "Insert item data value: %lu at index: %d in list: %p\n", item_data, index, list );

     item_data_array = D_CALLOC( list->item_count + 1, sizeof(LiteListItemData) );
     item_data_array[index] = item_data;

     if (list->item_data_array) {
          if (index > 0)
               memcpy( item_data_array, list->item_data_array, index * sizeof(LiteListItemData) );

          if (index < list->item_count)
               memcpy( item_data_array + index + 1, list->item_data_array + index,
                       (list->item_count - index) * sizeof(LiteListItemData) );

          D_FREE( list->item_data_array );
     }

     list->item_data_array = item_data_array;

     ++list->item_count;

     if (index <= list->cur_item_index)
          ++list->cur_item_index;

     update_scrollbar( list );

     return lite_update_box( &list->box, NULL );
}

DFBResult
lite_list_get_item( LiteList         *list,
                    int               index,
                    LiteListItemData *ret_item_data )
{
     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     if (index < 0 || index >= list->item_count)
          return DFB_INVARG;

     D_DEBUG_AT( LiteListDomain, "Get item data value: %lu at index: %d in list: %p\n", list->item_data_array[index],
                 index, list );

     *ret_item_data = list->item_data_array[index];

     return DFB_OK;
}

DFBResult
lite_list_set_item( LiteList         *list,
                    int               index,
                    LiteListItemData  item_data )
{
     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     if (index < 0 || index >= list->item_count)
          return DFB_INVARG;

     D_DEBUG_AT( LiteListDomain, "Set item data value: %lu at index: %d in list: %p\n", item_data, index, list );

     list->item_data_array[index] = item_data;

     return lite_update_box( &list->box, NULL );
}

DFBResult
lite_list_del_item( LiteList *list,
                    int       index )
{

     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     if (index < 0 || index >= list->item_count)
          return DFB_INVARG;

     D_DEBUG_AT( LiteListDomain, "Delete item data value: %lu at index: %d in list: %p\n", list->item_data_array[index],
                 index, list );

     --list->item_count;

     if (list->item_count == 0) {
          D_FREE( list->item_data_array );
          list->item_data_array = NULL;
          list->cur_item_index  = -1;
     }
     else {
          /* don't reallocate memory but just shift the data after the index position */
          memmove( list->item_data_array + index, list->item_data_array + index + 1,
                   (list->item_count - index) * sizeof(LiteListItemData) );

          if (list->cur_item_index != -1) {
               if (index < list->cur_item_index || (index == list->cur_item_index && index == list->item_count)) {
                    --list->cur_item_index;
                    if (list->cur_item_index == -1)
                         list->cur_item_index = 0;
               }
          }
     }

     update_scrollbar( list );

     return lite_update_box( &list->box, NULL );
}

DFBResult
lite_list_get_item_count( LiteList *list,
                          int      *ret_count )
{
     LITE_NULL_PARAMETER_CHECK( list );
     LITE_NULL_PARAMETER_CHECK( ret_count );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     D_DEBUG_AT( LiteListDomain, "list: %p contains %d items\n", list, list->item_count );

     *ret_count = list->item_count;

     return DFB_OK;
}

DFBResult
lite_list_sort_items( LiteList            *list,
                      LiteListCompareFunc  compare )
{
     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     D_DEBUG_AT( LiteListDomain, "Sort list: %p\n", list );

     if (list->item_count < 2)
          return DFB_OK;

     qsort( list->item_data_array, list->item_count, sizeof(LiteListItemData),
            (int (*)( const void *, const void * )) compare );

     return lite_update_box( &list->box, NULL );
}

DFBResult
lite_list_set_selected_item_index( LiteList *list,
                                   int       index )
{
     DFBResult ret;

     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     if (index < 0 || index >= list->item_count)
          return DFB_INVARG;

     D_DEBUG_AT( LiteListDomain, "Set item data value: %lu at index: %d selected in list: %p\n",
                 list->item_data_array[index], index, list );

     list->cur_item_index = index;

     ret = lite_update_box( &list->box, NULL );
     if (ret != DFB_OK)
          return ret;

     return lite_list_ensure_visible( list, index );
}

DFBResult
lite_list_get_selected_item_index( LiteList *list,
                                   int      *ret_index )
{
     LITE_NULL_PARAMETER_CHECK( list );
     LITE_NULL_PARAMETER_CHECK( ret_index );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     D_DEBUG_AT( LiteListDomain, "item data value: %lu at index: %d selected in list: %p\n",
                 list->item_data_array[list->cur_item_index], list->cur_item_index, list );

     *ret_index = list->cur_item_index;

     return DFB_OK;
}

DFBResult
lite_list_ensure_visible( LiteList *list,
                          int       index )
{
     int            y_center, y_item_center, y_item_bottom, y_item_top;
     LiteScrollInfo info;

     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     if (index < 0 || index >= list->item_count)
          return DFB_INVARG;

     if (!list->scrollbar)
          return DFB_OK;

     D_DEBUG_AT( LiteListDomain, "Ensure item data value: %lu at index: %d is visible in list: %p\n",
                 list->item_data_array[index], index, list );

     lite_get_scroll_info( list->scrollbar, &info );

     /* don't scroll to item while dragging */
     if (info.track_pos != -1)
          return DFB_FAILURE;

     y_center      = info.pos + info.page_size / 2;
     y_item_bottom = list->row_height * index + list->row_height;
     y_item_top    = list->row_height * index;
     y_item_center = (y_item_bottom + y_item_top) / 2;

     if (y_item_center < y_center) {
          if (y_item_top < info.pos) {
               info.pos = y_item_top;
               lite_set_scroll_info( list->scrollbar, &info );
               return lite_update_box( &list->box, NULL );
          }
     }
     else if (y_item_center > y_center) {
          if (y_item_bottom > info.pos + info.page_size) {
               info.pos = y_item_bottom - info.page_size;
               lite_set_scroll_info( list->scrollbar, &info );
               return lite_update_box( &list->box, NULL );
          }
     }

     return DFB_OK;
}

DFBResult
lite_list_recalc_layout( LiteList *list )
{
     DFBRectangle   scrollbar_rect;
     LiteScrollInfo info;

     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     if (!list->scrollbar)
          return DFB_OK;

     D_DEBUG_AT( LiteListDomain, "Recalculate layout for list: %p\n", list );

     get_scrollbar_rect( list, &scrollbar_rect );

     LITE_BOX(list->scrollbar)->rect = scrollbar_rect;

     lite_get_scroll_info( list->scrollbar, &info );

     info.page_size = list->box.rect.h;

     lite_set_scroll_info( list->scrollbar, &info );

     return lite_update_box( LITE_BOX(list->scrollbar), NULL );
}

DFBResult lite_set_list_scrollbar( LiteList      *list,
                                   LiteScrollbar *scrollbar )
{
     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     D_DEBUG_AT( LiteListDomain, "Set list: %p with scrollbar: %p\n", list, scrollbar );

     list->scrollbar = scrollbar;

     if (list->scrollbar)
          lite_on_scrollbar_update( list->scrollbar, scrollbar_updated, list );

     return DFB_OK;
}

DFBResult
lite_list_on_draw_item( LiteList             *list,
                        LiteListDrawItemFunc  callback,
                        void                 *data )
{
     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     D_DEBUG_AT( LiteListDomain, "Install callback %p( %p, %p )\n", callback, list, data );

     list->draw_item      = callback;
     list->draw_item_data = data;

     return DFB_OK;
}

DFBResult
lite_list_on_sel_change( LiteList              *list,
                         LiteListSelChangeFunc  callback,
                         void                  *data )
{
     LITE_NULL_PARAMETER_CHECK( list );
     LITE_BOX_TYPE_PARAMETER_CHECK( list, LITE_TYPE_LIST );

     D_DEBUG_AT( LiteListDomain, "Install callback %p( %p, %p )\n", callback, list, data );

     list->sel_change      = callback;
     list->sel_change_data = data;

     return DFB_OK;
}

DFBResult
lite_new_list_theme( const char     *image_path,
                     int             image_margin,
                     LiteListTheme **ret_theme )
{
     DFBResult      ret;
     LiteListTheme *theme;

     LITE_NULL_PARAMETER_CHECK( image_path );
     LITE_NULL_PARAMETER_CHECK( ret_theme );

     if (liteDefaultListTheme && *ret_theme == liteDefaultListTheme)
          return DFB_OK;

     theme = D_CALLOC( 1, sizeof(LiteListTheme) );

     ret = lite_new_scrollbar_theme( image_path, image_margin, &theme->scrollbar_theme );
     if (ret != DFB_OK) {
          D_FREE( theme );
          return ret;
     }

     *ret_theme = theme;

     D_DEBUG_AT( LiteListDomain, "Created new list theme: %p\n", theme );

     return DFB_OK;
}

DFBResult
lite_destroy_list_theme( LiteListTheme *theme )
{
     LITE_NULL_PARAMETER_CHECK( theme );

     D_DEBUG_AT( LiteListDomain, "Destroy list theme: %p\n", theme );

     lite_destroy_scrollbar_theme( theme->scrollbar_theme );

     D_FREE( theme );

     if (theme == liteDefaultListTheme)
          liteDefaultListTheme = NULL;

     return DFB_OK;
}

/* internals */

static int
on_button_down( LiteBox                        *box,
                int                             x,
                int                             y,
                DFBInputDeviceButtonIdentifier  button_id )
{
     int       cur_selected, new_selected, pos;
     LiteList *list = LITE_LIST(box);

     D_ASSERT( box != NULL );

     if (list->item_count < 1)
          return 1;

     lite_focus_box( box );

     cur_selected = list->cur_item_index;

     if (list->scrollbar)
          lite_get_scroll_pos( list->scrollbar, &pos );
     else
          pos = 0;

     new_selected = (y + pos) / list->row_height;

     if (new_selected < 0)
          new_selected = 0;
     if (new_selected >= list->item_count)
          new_selected = list->item_count - 1;

     if (new_selected != cur_selected) {
          list->cur_item_index = new_selected;

          lite_update_box( box, NULL );

          if (list->sel_change)
               list->sel_change( list, new_selected, list->sel_change_data );
     }

     return 1;
}

static int
on_key_down(LiteBox *box, DFBWindowEvent *evt)
{
     int       cur_selected, new_selected;
     LiteList *list = LITE_LIST(box);

     D_ASSERT( box != NULL );

     if (evt->key_symbol != DIKS_CURSOR_UP && evt->key_symbol != DIKS_CURSOR_DOWN &&
         evt->key_symbol != DIKS_PAGE_UP   && evt->key_symbol != DIKS_PAGE_DOWN)
          return 1;

     if (list->item_count < 1)
          return 1;

     cur_selected = list->cur_item_index;

     if( evt->key_symbol == DIKS_CURSOR_UP )
          new_selected = cur_selected - 1;
     else if( evt->key_symbol == DIKS_CURSOR_DOWN )
          new_selected = cur_selected + 1;
     else if( evt->key_symbol == DIKS_PAGE_UP )
          new_selected = cur_selected - list->box.rect.h / list->row_height;
     else if( evt->key_symbol == DIKS_PAGE_DOWN )
          new_selected = cur_selected + list->box.rect.h / list->row_height;

     if (new_selected < 0)
          new_selected = 0;
     if (new_selected >= list->item_count)
          new_selected = list->item_count - 1;

     if (new_selected != cur_selected) {
          lite_list_set_selected_item_index( list, new_selected );
     }

     return 1;
}

static DFBResult
draw_list( LiteBox         *box,
           const DFBRegion *region,
           DFBBoolean       clear )
{
     int               i;
     DFBRectangle      rect;
     DFBRectangle      rc_item;
     DFBRectangle      scrollbar_rect;
     LiteListDrawItem  draw_item;
     LiteScrollInfo    info;
     IDirectFBSurface *surface;
     int               scroll_pos   = 0;
     int               scroll_width = 0;
     LiteList         *list         = LITE_LIST(box);

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteListDomain, "Draw list: %p (enabled:%d, cur_item_index:%d, item_count:%d, clear:%u)\n",
                 list, list->enabled, list->cur_item_index, list->item_count, clear );

     if (!list->draw_item || list->item_count < 1)
          return DFB_OK;

     if (clear)
          lite_clear_box( box, region );

     surface->SetClip( surface, region );

     if (list->scrollbar) {
          get_scrollbar_rect( list, &scrollbar_rect );
          scroll_width = scrollbar_rect.w;

          lite_get_scroll_info( list->scrollbar, &info );
          scroll_pos = info.track_pos == -1 ? info.pos : info.track_pos;
     }

     rect.x = 0;
     rect.y = scroll_pos;
     rect.w = list->box.rect.w;
     rect.h = list->box.rect.h;

     rc_item.x = 0;
     rc_item.w = list->box.rect.w - scroll_width;
     rc_item.h = list->row_height;

     for (i = 0; i < list->item_count; ++i) {
          rc_item.y = i * list->row_height;

          if (DFB_RECTANGLE_CONTAINS_POINT( &rect, rc_item.x, rc_item.y ) ||
              DFB_RECTANGLE_CONTAINS_POINT( &rect, rc_item.x, rc_item.y + rc_item.h )) {
               rc_item.y -= rect.y;

               draw_item.index_item = i;
               draw_item.item_data  = list->item_data_array[i];
               draw_item.surface    = surface;
               draw_item.rc_item    = rc_item;
               draw_item.selected   = (i == list->cur_item_index);
               draw_item.disabled   = !list->enabled;

               list->draw_item( list, &draw_item, list->draw_item_data );
          }
     }

     return DFB_OK;
}

static DFBResult
destroy_list( LiteBox *box )
{
     LiteList *list = LITE_LIST(box);

     D_ASSERT( box != NULL );

     D_DEBUG_AT( LiteListDomain, "Destroy list: %p\n", list );

     if (list->item_data_array)
          D_FREE( list->item_data_array );

     return lite_destroy_box( box );
}

static void
get_scrollbar_rect( LiteList     *list,
                    DFBRectangle *ret_rect )
{
     int          scrollbar_thickness;
     DFBRectangle rect;

     if (list->theme != liteNoListTheme)
          scrollbar_thickness = list->theme->scrollbar_theme->all_images.width / 8;
     else
          lite_get_scrollbar_thickness( list->scrollbar, &scrollbar_thickness );

     rect = list->box.rect;

     rect.x = rect.w - scrollbar_thickness;
     if (rect.x < 0)
          rect.x = 0;

     rect.y = 0;

     rect.w = scrollbar_thickness;
     if (rect.w > list->box.rect.w)
          rect.w = list->box.rect.w;

     *ret_rect = rect;
}

static void
update_scrollbar( LiteList *list )
{
     LiteScrollInfo curInfo, newInfo;

     if (!list->scrollbar)
          return;

     lite_get_scroll_info( list->scrollbar, &curInfo );

     newInfo.min       = 0;
     newInfo.max       = list->row_height * list->item_count;
     newInfo.page_size = list->box.rect.h;
     newInfo.line_size = list->row_height;
     newInfo.pos       = curInfo.pos;
     newInfo.track_pos = curInfo.track_pos;

     lite_set_scroll_info( list->scrollbar, &newInfo );
}
