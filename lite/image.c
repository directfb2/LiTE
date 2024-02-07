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

#include <direct/util.h>
#include <directfb_util.h>
#include <lite/image.h>
#include <lite/lite_internal.h>

D_DEBUG_DOMAIN( LiteImageDomain, "LiTE/Image", "LiTE Image" );

/**********************************************************************************************************************/

LiteImageTheme *liteDefaultImageTheme = NULL;

struct _LiteImage {
     LiteBox                  box;
     LiteImageTheme          *theme;

     DFBRectangle             clipping_rect;
     int                      width, height;
     IDirectFBSurface        *surface;
     DFBImageDescription      desc;
     DFBSurfaceBlittingFlags  blitting_flags;
};

static DFBResult draw_image   ( LiteBox *box, const DFBRegion *region, DFBBoolean clear );
static DFBResult destroy_image( LiteBox *box );

/**********************************************************************************************************************/

DFBResult
lite_new_image( LiteBox         *parent,
                DFBRectangle    *rect,
                LiteImageTheme  *theme,
                LiteImage      **ret_image )
{
     DFBResult  ret;
     LiteImage *image;

     LITE_NULL_PARAMETER_CHECK( parent );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_NULL_PARAMETER_CHECK( ret_image );

     image = D_CALLOC( 1, sizeof(LiteImage) );

     image->box.parent = parent;
     image->box.rect   = *rect;
     image->theme      = theme;

     ret = lite_init_box( LITE_BOX(image) );
     if (ret != DFB_OK) {
          D_FREE( image );
          return ret;
     }

     image->box.type    = LITE_TYPE_IMAGE;
     image->box.Draw    = draw_image;
     image->box.Destroy = destroy_image;

     *ret_image = image;

     D_DEBUG_AT( LiteImageDomain, "Created new image object: %p\n", image );

     return DFB_OK;
}

static DFBResult
load_image( LiteImage    *image,
            const void   *file_data,
            unsigned int  length )
{
     DFBResult         ret;
     IDirectFBSurface *surface;

     LITE_NULL_PARAMETER_CHECK( image );
     LITE_NULL_PARAMETER_CHECK( file_data );
     LITE_BOX_TYPE_PARAMETER_CHECK( image, LITE_TYPE_IMAGE );

     D_DEBUG_AT( LiteImageDomain, "Load image: %p\n", image );

     ret = prvlite_load_image( file_data, length, &surface, &image->width, &image->height, &image->desc );
     if (ret != DFB_OK)
          return ret;

     if (image->surface)
          image->surface->Release( image->surface );

     image->surface = surface;

     if (image->desc.caps & DICAPS_ALPHACHANNEL)
          image->blitting_flags = DSBLIT_BLEND_ALPHACHANNEL;
     else
          image->blitting_flags = DSBLIT_NOFX;

     return lite_update_box( LITE_BOX(image), NULL );
}

DFBResult
lite_load_image( LiteImage  *image,
                 const char *filename )
{
     return load_image( image, filename, 0 );
}

DFBResult
lite_load_image_data( LiteImage    *image,
                      const void   *data,
                      unsigned int  length )
{
     return load_image( image, data, length );
}

DFBResult
lite_set_image_clipping( LiteImage          *image,
                         const DFBRectangle *rect )
{
     LITE_NULL_PARAMETER_CHECK( image );
     LITE_NULL_PARAMETER_CHECK( rect );
     LITE_BOX_TYPE_PARAMETER_CHECK( image, LITE_TYPE_IMAGE );

     D_DEBUG_AT( LiteImageDomain, "Set image: %p with clipping: " DFB_RECT_FORMAT "\n", image,
                 DFB_RECTANGLE_VALS( rect ) );

     image->clipping_rect.x = rect->x;
     image->clipping_rect.y = rect->y;
     image->clipping_rect.w = rect->w;
     image->clipping_rect.h = rect->h;

     if (!image->surface)
          return DFB_OK;

     return lite_update_box( LITE_BOX(image), NULL );
}

DFBResult
lite_get_image_description( LiteImage           *image,
                            DFBImageDescription *ret_desc )
{
     LITE_NULL_PARAMETER_CHECK( image );
     LITE_NULL_PARAMETER_CHECK( ret_desc );
     LITE_BOX_TYPE_PARAMETER_CHECK( image, LITE_TYPE_IMAGE );

     D_DEBUG_AT( LiteImageDomain, "image: %p has %salphachannel and %scolorkey\n", image,
                 (image->desc.caps & DICAPS_ALPHACHANNEL) ? "" : "no ",
                 (image->desc.caps & DICAPS_COLORKEY)     ? "" : "no " );

     *ret_desc = image->desc;

     return DFB_OK;
}

DFBResult
lite_get_image_size( LiteImage *image,
                     int       *ret_width,
                     int       *ret_height )
{
     LITE_NULL_PARAMETER_CHECK( image );
     LITE_NULL_PARAMETER_CHECK( ret_width );
     LITE_NULL_PARAMETER_CHECK( ret_height );
     LITE_BOX_TYPE_PARAMETER_CHECK( image, LITE_TYPE_IMAGE );

     D_DEBUG_AT( LiteWindowDomain, "image: %p has a size of: %dx%d\n", image, image->width, image->height );

     *ret_width  = image->width;
     *ret_height = image->height;

     return DFB_OK;
}

/* internals */

static DFBResult
draw_image( LiteBox         *box,
            const DFBRegion *region,
            DFBBoolean       clear )
{
     IDirectFBSurface *surface;
     LiteImage        *image = LITE_IMAGE(box);

     D_ASSERT( box != NULL );

     surface = box->surface;

     D_DEBUG_AT( LiteImageDomain, "Draw image: %p (blitting_flags:0x%x, clear:%u)\n",
                 image, image->blitting_flags, clear );

     if (clear)
          lite_clear_box( box, region );

     surface->SetClip( surface, region );

     surface->SetBlittingFlags( surface, image->blitting_flags );

     if (image->clipping_rect.w != 0 && image->clipping_rect.h != 0)
          surface->StretchBlit( surface, image->surface, &image->clipping_rect, NULL );
     else
          surface->StretchBlit( surface, image->surface, NULL, NULL );

     return DFB_OK;
}

static DFBResult
destroy_image( LiteBox *box )
{
     LiteImage *image = LITE_IMAGE(box);

     D_ASSERT( box != NULL );

     D_DEBUG_AT( LiteImageDomain, "Destroy image: %p\n", image );

     if (image->surface)
          image->surface->Release( image->surface );

     return lite_destroy_box( box );
}

DFBResult
prvlite_load_image( const void           *file_data,
                    unsigned int          length,
                    IDirectFBSurface    **ret_surface,
                    int                  *ret_width,
                    int                  *ret_height,
                    DFBImageDescription  *ret_desc )
{
     DFBResult                 ret;
     DFBDataBufferDescription  ddsc;
     DFBSurfaceDescription     sdsc;
     IDirectFBDataBuffer      *buffer;
     IDirectFBSurface         *surface;
     IDirectFBImageProvider   *provider;

     LITE_NULL_PARAMETER_CHECK( file_data );
     LITE_NULL_PARAMETER_CHECK( ret_surface );

     /* create an image provider for loading the image */
     if (!length) {
          ddsc.flags         = DBDESC_FILE;
          ddsc.file          = file_data;
     }
     else {
          ddsc.flags         = DBDESC_MEMORY;
          ddsc.memory.data   = file_data;
          ddsc.memory.length = length;
     }

     ret = lite_dfb->CreateDataBuffer( lite_dfb, &ddsc, &buffer );
     if (ret) {
          DirectFBError( "LiTE/Image: CreateDataBuffer() failed", ret );
          return ret;
     }

     ret = buffer->CreateImageProvider( buffer, &provider );
     if (ret) {
          DirectFBError( "LiTE/Image: CreateImageProvider() failed", ret );
          buffer->Release( buffer );
          return ret;
     }

     /* retrieve a surface description for the image */
     ret = provider->GetSurfaceDescription( provider, &sdsc );
     if (ret) {
          DirectFBError( "LiTE/Image: GetSurfaceDescription() failed", ret );
          provider->Release( provider );
          return ret;
     }

     /* create a surface using the description */
     ret = lite_dfb->CreateSurface( lite_dfb, &sdsc, &surface );
     if (ret) {
          DirectFBError( "LiTE/Image: CreateSurface() failed", ret );
          provider->Release( provider );
          return ret;
     }

     /* render the image to the created surface */
     ret = provider->RenderTo( provider, surface, NULL );
     if (ret) {
          DirectFBError( "LiTE/Image: RenderTo() failed", ret );
          surface->Release( surface );
          provider->Release( provider );
          return ret;
     }

     /* return surface */
     *ret_surface = surface;

     /* return width */
     if (ret_width)
          *ret_width = sdsc.width;

     /* return height */
     if (ret_height)
          *ret_height = sdsc.height;

     /* return image description */
     if (ret_desc)
          provider->GetImageDescription( provider, ret_desc );

     /* release the provider */
     provider->Release( provider );

     return DFB_OK;
}
