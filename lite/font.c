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

#include <direct/thread.h>
#include <lite/font.h>
#include <lite/lite_config.h>
#include <lite/lite_internal.h>

#ifndef LITEFONTDIR
#include "Vera.h"
#include "VeraBd.h"
#include "VeraIt.h"
#include "VeraBI.h"
#include "VeraMo.h"
#include "VeraMoBd.h"
#include "VeraMoIt.h"
#include "VeraMoBI.h"
#include "VeraSe.h"
#include "VeraSeBd.h"
#include "whitrabt.h"
#endif

D_DEBUG_DOMAIN( LiteFontDomain, "LiTE/Font", "LiTE Font" );

/**********************************************************************************************************************/

struct _LiteFont {
     int                refs;

     char              *id;
     int                size;
     IDirectFBFont     *font;
     DFBFontAttributes  attr;

     LiteFont          *next;
     LiteFont          *prev;
};

char *lite_font_styles[4] = {
     "", "Bd", "It", "BI"
};

static LiteFont    *fonts       = NULL;
static DirectMutex  fonts_mutex = DIRECT_MUTEX_INITIALIZER();

#ifndef LITEFONTDIR
struct LiteFontData {
     const char *name;
     const void *data;
};

static const struct LiteFontData font_data[] = {
  { "Vera",     Vera_data     },
  { "VeraBd",   VeraBd_data   },
  { "VeraIt",   VeraIt_data   },
  { "VeraBI",   VeraBI_data   },
  { "VeraMo",   VeraMo_data   },
  { "VeraMoBd", VeraMoBd_data },
  { "VeraMoIt", VeraMoIt_data },
  { "VeraMoBI", VeraMoBI_data },
  { "VeraSe",   VeraSe_data   },
  { "VeraSeBd", VeraSeBd_data },
  { "whitrabt", whitrabt_data }
};

struct LiteFontSize {
     const char   *name;
     unsigned int  size;
};

static const struct LiteFontSize font_size[] = {
  { "Vera",     sizeof(Vera_data)     },
  { "VeraBd",   sizeof(VeraBd_data)   },
  { "VeraIt",   sizeof(VeraIt_data)   },
  { "VeraBI",   sizeof(VeraBI_data)   },
  { "VeraMo",   sizeof(VeraMo_data)   },
  { "VeraMoBd", sizeof(VeraMoBd_data) },
  { "VeraMoIt", sizeof(VeraMoIt_data) },
  { "VeraMoBI", sizeof(VeraMoBI_data) },
  { "VeraSe",   sizeof(VeraSe_data)   },
  { "VeraSeBd", sizeof(VeraSeBd_data) },
  { "whitrabt", sizeof(whitrabt_data) }
};
#endif

/* return an existing font entry from the cache after increasing its reference count, otherwise try creating a new one
   by specifying a font file or a font name */
static LiteFont *cache_get_entry            ( const char *name, int size, DFBFontAttributes attr );
#ifdef LITEFONTDIR
static LiteFont *cache_get_entry_from_file  ( const char *file, int size, DFBFontAttributes attr );
#else
static LiteFont *cache_get_entry_from_memory( const char *name, int size, DFBFontAttributes attr );
#endif

/* decrease the reference count of a cache entry and remove/destroy it if the count is zero */
static void cache_release_entry( LiteFont *entry );

/**********************************************************************************************************************/

DFBResult
lite_get_font( const char         *spec,
               LiteFontStyle       style,
               int                 size,
               DFBFontAttributes   attr,
               LiteFont          **ret_font )
{
     DFBResult  ret = DFB_OK;
     int        name_len;
     char      *name, *c;

     LITE_NULL_PARAMETER_CHECK( spec );
     LITE_NULL_PARAMETER_CHECK( ret_font );

     D_DEBUG_AT( LiteFontDomain, "Get font with spec: %s, style: %s, size: %d and attr: 0x%x\n",
                 spec, lite_font_styles[style&3], size, attr );

     /* translate the predefined specs or use the passed spec as the font name in case it does not match a spec */
     if (!strcasecmp( spec, "default" ))
          spec = DEFAULT_FONT_SYSTEM;
     else if (!strcasecmp( spec, "monospaced" ))
          spec = DEFAULT_FONT_MONOSPACED;
     else if (!strcasecmp( spec, "serif" ))
          spec = DEFAULT_FONT_SERIF;
     else if (!strcasecmp( spec, "sansserif" ))
          spec = DEFAULT_FONT_SANS_SERIF;

     /* append characters depending on font style */
     name_len = strlen( spec) + 3;
     name     = alloca( name_len );

     snprintf( name, name_len, "%s%s", spec, lite_font_styles[style&3] );

     /* replace spaces by underscores */
     while ((c = strchr( name, ' ' )) != NULL)
          *c = '_';

     /* get the font from the cache, if it does not exist yet it will be loaded */
     *ret_font = cache_get_entry( name, size, attr );

     /* if font loading failed, try to get our default font, but let's make sure we haven't already tried */
     if (*ret_font == NULL) {
          if (strcmp( spec, DEFAULT_FONT_SYSTEM )) {
               char fallback[12];

               snprintf( fallback, sizeof(fallback), DEFAULT_FONT_SYSTEM"%s", lite_font_styles[style&3] );
               *ret_font = cache_get_entry( fallback, size, attr );

               if (*ret_font == NULL) {
                    D_DEBUG_AT( LiteFontDomain,
                                "  -> could not load default font '"LITEFONTDIR"/%s' for '"LITEFONTDIR"/%s'\n",
                                fallback, name );
                    ret = DFB_FILENOTFOUND;
               }
          }
          else {
               D_DEBUG_AT( LiteFontDomain, "  -> could not load default font '"LITEFONTDIR"/%s'\n", name );
               ret = DFB_FILENOTFOUND;
          }
     }

     return ret;
}

DFBResult
lite_ref_font( LiteFont *font )
{
     DFBResult ret = DFB_OK;

     LITE_NULL_PARAMETER_CHECK( font );

     /* lock cache */
     direct_mutex_lock( &fonts_mutex );

     if (font->refs < 1) {
          D_DEBUG_AT( LiteFontDomain, "Cannot reference unloaded font\n" );
          ret = DFB_INVARG;
     }
     else
          font->refs++;

     D_DEBUG_AT( LiteFontDomain, "Increase the reference count for font: %p (interface: %p) now has %d refs\n",
                 font, font->font, font->refs );

     /* unlock cache */
     direct_mutex_unlock( &fonts_mutex );

     return ret;
}

DFBResult
lite_release_font( LiteFont *font )
{
     LITE_NULL_PARAMETER_CHECK( font );

     D_DEBUG_AT( LiteFontDomain, "Release font: %p\n", font );

     cache_release_entry( font );

     return DFB_OK;
}

DFBResult
lite_font( LiteFont       *font,
           IDirectFBFont **ret_font )
{
     LITE_NULL_PARAMETER_CHECK( font );
     LITE_NULL_PARAMETER_CHECK( font->font );
     LITE_NULL_PARAMETER_CHECK( ret_font );

     D_DEBUG_AT( LiteFontDomain, "Get IDirectFBFont interface\n" );

     *ret_font = font->font;

     return DFB_OK;
}

DFBResult
lite_set_active_font( LiteBox  *box,
                      LiteFont *font )
{
     LITE_NULL_PARAMETER_CHECK( box );
     LITE_NULL_PARAMETER_CHECK( font );

     D_DEBUG_AT( LiteFontDomain, "Set active font: %p\n", font );

     if (box->surface == NULL) {
          D_DEBUG_AT( LiteFontDomain, "  -> NULL surface\n" );
          return DFB_FAILURE;
     }

     if (font->font == NULL) {
          D_DEBUG_AT( LiteFontDomain, "  -> NULL font\n" );
          return DFB_FAILURE;
     }

     return box->surface->SetFont( box->surface, font->font );
}

DFBResult
lite_get_active_font( LiteBox   *box,
                      LiteFont **ret_font )
{
     DFBResult      ret = DFB_OK;
     IDirectFBFont *font;
     LiteFont      *entry;

     LITE_NULL_PARAMETER_CHECK( box );
     LITE_NULL_PARAMETER_CHECK( ret_font );

     D_DEBUG_AT( LiteFontDomain, "Get active font\n" );

     if (box->surface == NULL) {
          D_DEBUG_AT( LiteFontDomain, "  -> NULL surface\n" );
          return DFB_FAILURE;
     }

     ret = box->surface->GetFont( box->surface, &font );
     if (ret) {
          DirectFBError( "LiTE/Font: GetFont() failed", ret );
          return ret;
     }

     /* lock cache */
     direct_mutex_lock( &fonts_mutex );

     *ret_font = NULL;

     /* lookup font in cache */
     for (entry = fonts; entry; entry = entry->next) {
          if (entry->font == font) {
               *ret_font = entry;
               D_DEBUG_AT( LiteFontDomain, "  -> %p\n", entry );
               break;
          }
     }

     /* unlock cache */
     direct_mutex_unlock( &fonts_mutex );

     font->Release( font );

     if (*ret_font == NULL) {
          D_DEBUG_AT( LiteFontDomain, "  -> no font set\n" );
          ret = DFB_FAILURE;
     }

     return ret;
}

DFBResult
lite_get_font_attributes( LiteFont          *font,
                          DFBFontAttributes *ret_attr )
{
     D_DEBUG_AT( LiteFontDomain, "font: %p has attributes: 0x%x\n", font, font->attr );

     *ret_attr = font->attr;

     return DFB_OK;
}

/* internals */

#ifdef LITEFONTDIR
static LiteFont *
cache_get_entry_from_file( const char        *file,
                           int                size,
                           DFBFontAttributes  attr )
{
     DFBResult           ret;
     DFBFontDescription  desc;
     IDirectFBFont      *font;
     LiteFont           *entry;

     D_ASSERT( file != NULL );

     /* lock cache */
     direct_mutex_lock( &fonts_mutex );

     /* look for an existing font entry in the cache */
     for (entry = fonts; entry; entry = entry->next) {
          if (!strcmp( file, entry->id ) && size == entry->size && attr == entry->attr) {
               entry->refs++;

               D_DEBUG_AT( LiteFontDomain, "Existing cache entry '%s' with size: %d and attr: 0x%x (refs %d)\n",
                           file, size, entry->attr, entry->refs );

               /* unlock cache */
               direct_mutex_unlock( &fonts_mutex );

               return entry;
          }
     }

     D_DEBUG_AT( LiteFontDomain, "Loading cache entry '%s' with size: %d and attr: 0x%x\n", file, size, attr );

     /* load the font */
     desc.flags      = DFDESC_ATTRIBUTES | DFDESC_HEIGHT;
     desc.attributes = attr;
     desc.height     = size;

     ret = lite_dfb->CreateFont( lite_dfb, file, &desc, &font );
     if (ret) {
          DirectFBError( "LiTE/Font: CreateFont() failed", ret );

          /* unlock cache */
          direct_mutex_unlock( &fonts_mutex );

          return NULL;
     }
     else
          D_DEBUG_AT( LiteFontDomain, "  -> interface: %p\n", font );

     /* create a new entry for it */
     entry = D_CALLOC( 1, sizeof(LiteFont) );

     entry->refs = 1;
     entry->id   = D_STRDUP( file );
     entry->size = size;
     entry->font = font;
     entry->attr = attr;

     /* insert into cache */
     if (fonts) {
          fonts->prev = entry;
          entry->next = fonts;
     }
     fonts = entry;

     /* unlock cache */
     direct_mutex_unlock( &fonts_mutex );

     return entry;
}
#else
static LiteFont *
cache_get_entry_from_memory( const char        *name,
                             int                size,
                             DFBFontAttributes  attr )
{
     DFBResult                 ret;
     int                       i;
     DFBFontDescription        desc;
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
     IDirectFBFont            *font;
     LiteFont                 *entry;

     D_ASSERT( name != NULL );

     /* lock cache */
     direct_mutex_lock( &fonts_mutex );

     /* look for an existing font entry in the cache */
     for (entry = fonts; entry; entry = entry->next) {
          if (!strcmp( name, entry->id ) && size == entry->size && attr == entry->attr) {
               entry->refs++;

               D_DEBUG_AT( LiteFontDomain, "Existing cache entry '%s' with size: %d and attr: 0x%x (refs %d)\n",
                           name, size, entry->attr, entry->refs );

               /* unlock cache */
               direct_mutex_unlock( &fonts_mutex );

               return entry;
          }
     }

     D_DEBUG_AT( LiteFontDomain, "Loading cache entry '%s' with size: %d and attr: 0x%x\n", name, size, attr );

     /* load the font */
     desc.flags      = DFDESC_ATTRIBUTES | DFDESC_HEIGHT;
     desc.attributes = attr;
     desc.height     = size;

     for (i = 0; i < D_ARRAY_SIZE(font_data); i++) {
          if (!strcmp( name, font_data[i].name ))
               break;
     }

     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = font_data[i].data;
     ddsc.memory.length = font_size[i].size;

     ret = lite_dfb->CreateDataBuffer( lite_dfb, &ddsc, &buffer );
     if (ret) {
          DirectFBError( "LiTE/Font: CreateDataBuffer() failed", ret );

          /* unlock cache */
          direct_mutex_unlock( &fonts_mutex );

          return NULL;
     }

     ret = buffer->CreateFont( buffer, &desc, &font );
     if (ret) {
          DirectFBError( "LiTE/Font: CreateFont() failed", ret );

          buffer->Release( buffer );

          /* unlock cache */
          direct_mutex_unlock( &fonts_mutex );

          return NULL;
     }
     else
          D_DEBUG_AT( LiteFontDomain, "  -> interface: %p\n", font );

     /* create a new entry for it */
     entry = D_CALLOC( 1, sizeof(LiteFont) );

     entry->refs = 1;
     entry->id   = D_STRDUP( name );
     entry->size = size;
     entry->font = font;
     entry->attr = attr;

     /* insert into cache */
     if (fonts) {
          fonts->prev = entry;
          entry->next = fonts;
     }
     fonts = entry;

     /* unlock cache */
     direct_mutex_unlock( &fonts_mutex );

     return entry;
}
#endif

static LiteFont *
cache_get_entry( const char        *name,
                 int                size,
                 DFBFontAttributes  attr )
{
     LiteFont *entry = NULL;
#ifdef LITEFONTDIR
     int       len   = strlen( LITEFONTDIR ) + 1 + strlen( name ) + 6 + 1;
     char      file[len];

     D_ASSERT( name != NULL );

     if (!getenv( "LITE_NO_DGIFF" )) {
          /* first try to load a font in DGIFF format */
          snprintf( file, len, LITEFONTDIR"/%s.dgiff", name );
          entry = cache_get_entry_from_file( file, size, attr );
     }

     if (entry == NULL) {
          /* otherwise fall back on a font in TTF format */
          snprintf( file, len, LITEFONTDIR"/%s.ttf", name );
          entry = cache_get_entry_from_file( file, size, attr );
     }
#else
     D_ASSERT( name != NULL );

     entry = cache_get_entry_from_memory( name, size, attr );
#endif

     return entry;
}

static void
cache_release_entry( LiteFont *entry )
{
     D_ASSERT( entry != NULL );

     /* lock cache */
     direct_mutex_lock( &fonts_mutex );

     /* decrease the reference count and return if not zero */
     if (--entry->refs) {
          D_DEBUG_AT( LiteFontDomain, "Decrease the reference count for cache entry '%s' with size: %d and attr: 0x%x\n",
                      entry->id, entry->size, entry->attr );
          D_DEBUG_AT( LiteFontDomain, "  -> %p (interface: %p) now has %d refs\n", entry, entry->font, entry->refs );

          /* unlock cache */
          direct_mutex_unlock( &fonts_mutex );

          return;
     }

     D_DEBUG_AT( LiteFontDomain, "Destroying cache entry '%s' with size: %d and attr: 0x%x (interface: %p)\n",
                 entry->id, entry->size, entry->attr, entry->font );

     /* remove entry from cache */
     if (entry->next)
          entry->next->prev = entry->prev;
     if (entry->prev)
          entry->prev->next = entry->next;
     else
          fonts = entry->next;

     /* unlock cache */
     direct_mutex_unlock( &fonts_mutex );

     /* free font resources */
     entry->font->Release( entry->font );
     D_FREE( entry->id );
     D_FREE( entry );
}

DFBResult
prvlite_release_font_resources()
{
     LiteFont *entry, *temp;

     for (entry = fonts, temp = entry ? entry->next : NULL; entry; entry = temp, temp = entry ? entry->next : NULL) {
          entry->font->Release( entry->font );
          D_FREE( entry->id );
          D_FREE( entry );
     }

     fonts = NULL;

     return DFB_OK;
}

void
prvlite_make_truncated_text( char          *text,
                             int            width,
                             IDirectFBFont *font )
{
     DFBRectangle  ink_rect;
     int           i;
     char         *buffer;
     const char   *tail = "...";

     font->GetStringExtents( font, text, -1, NULL, &ink_rect );
     if (ink_rect.w <= width)
          return;

     font->GetStringExtents( font, tail, -1, NULL, &ink_rect );
     if (ink_rect.w >= width) {
          strncpy( text, tail, strlen( text ) );
          return;
     }

     buffer = calloc( strlen( text ) + 4, 1 );
     strncpy( buffer, text, MAX( 0, strlen( text ) - 3 ) );
     strcat( buffer, tail );

     while (1) {
          font->GetStringExtents( font, buffer, -1, NULL, &ink_rect );
          if (ink_rect.w <= width)
               break;

          i = strlen( buffer );
          buffer[i-1] = 0;
          buffer[i-4] = '.';
     }

     strcpy( text, buffer );

     free( buffer );
}
