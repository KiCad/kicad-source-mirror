/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 jean-pierre.charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "bitmap_base.h"

#include <gr_basic.h>
#include <math/util.h>    // for KiROUND
#include <memory>         // for make_unique, unique_ptr
#include <plotters/plotter.h>
#include <richio.h>
#include <wx/bitmap.h>    // for wxBitmap
#include <wx/mstream.h>
#include <wx/stream.h>    // for wxInputStream, wxOutputStream
#include <wx/string.h>    // for wxString
#include <wx/wfstream.h>  // for wxFileInputStream



BITMAP_BASE::BITMAP_BASE( const VECTOR2I& pos )
{
    m_scale  = 1.0;                     // 1.0 = original bitmap size
    m_imageType = wxBITMAP_TYPE_INVALID;
    m_bitmap = nullptr;
    m_image  = nullptr;
    m_originalImage = nullptr;
    m_ppi    = 300;                     // the bitmap definition. the default is 300PPI
    m_pixelSizeIu = 254000.0 / m_ppi;   // a pixel size value OK for bitmaps using 300 PPI
                                        // for Eeschema which uses currently 254000PPI
    m_isMirroredX = false;
    m_isMirroredY = false;
    m_rotation   = ANGLE_0;
}


BITMAP_BASE::BITMAP_BASE( const BITMAP_BASE& aSchBitmap )
{
    m_scale = aSchBitmap.m_scale;
    m_ppi   = aSchBitmap.m_ppi;
    m_pixelSizeIu = aSchBitmap.m_pixelSizeIu;
    m_isMirroredX = aSchBitmap.m_isMirroredX;
    m_isMirroredY = aSchBitmap.m_isMirroredY;
    m_rotation = aSchBitmap.m_rotation;
    m_imageType = aSchBitmap.m_imageType;

    m_image = nullptr;
    m_bitmap = nullptr;
    m_originalImage = nullptr;

    if( aSchBitmap.m_image )
    {
        m_image   = new wxImage( *aSchBitmap.m_image );
        m_bitmap  = new wxBitmap( *m_image );
        m_originalImage = new wxImage( *aSchBitmap.m_originalImage );
        m_imageType = aSchBitmap.m_imageType;
        m_imageData = aSchBitmap.m_imageData;
        m_imageId = aSchBitmap.m_imageId;
    }
}


void BITMAP_BASE::rebuildBitmap( bool aResetID )
{
    if( m_bitmap )
        delete m_bitmap;

    m_bitmap  = new wxBitmap( *m_image );

    if( aResetID )
        m_imageId = KIID();

}


void BITMAP_BASE::updatePPI()
{
    // Todo: eventually we need to support dpi / scaling in both dimensions
    int dpiX = m_originalImage->GetOptionInt( wxIMAGE_OPTION_RESOLUTIONX );

    if( dpiX > 1 )
    {
        if( m_originalImage->GetOptionInt( wxIMAGE_OPTION_RESOLUTIONUNIT ) == wxIMAGE_RESOLUTION_CM )
            m_ppi = KiROUND( dpiX * 2.54 );
        else
            m_ppi = dpiX;
    }
}


void BITMAP_BASE::ImportData( BITMAP_BASE& aItem )
{
    *m_image = *aItem.m_image;
    *m_bitmap = *aItem.m_bitmap;
    *m_originalImage = *aItem.m_originalImage;
    m_imageId = aItem.m_imageId;
    m_scale = aItem.m_scale;
    m_ppi = aItem.m_ppi;
    m_pixelSizeIu = aItem.m_pixelSizeIu;
    m_isMirroredX = aItem.m_isMirroredX;
    m_isMirroredY = aItem.m_isMirroredY;
    m_rotation = aItem.m_rotation;
    m_imageType = aItem.m_imageType;
    m_imageData = aItem.m_imageData;
}


bool BITMAP_BASE::ReadImageFile( wxInputStream& aInStream )
{
    // Store the original image data in m_imageData
    size_t dataSize = aInStream.GetLength();
    m_imageData.SetBufSize( dataSize );
    aInStream.Read( m_imageData.GetData(), dataSize );
    m_imageData.SetDataLen( dataSize );

    std::unique_ptr<wxImage> new_image = std::make_unique<wxImage>();

    // Load the image from the stream into new_image
    wxMemoryInputStream mem_stream( m_imageData.GetData(), dataSize );
    if( !new_image->LoadFile( mem_stream ) )
        return false;

    return SetImage( *new_image );
}


bool BITMAP_BASE::ReadImageFile( wxMemoryBuffer& aBuf )
{
    // Store the original image data in m_imageData
    m_imageData = aBuf;

    std::unique_ptr<wxImage> new_image = std::make_unique<wxImage>();

    // Load the image from the buffer into new_image
    wxMemoryInputStream mem_stream( m_imageData.GetData(), m_imageData.GetBufSize() );

    if( !new_image->LoadFile( mem_stream ) )
        return false;

    return SetImage( *new_image );
}


bool BITMAP_BASE::ReadImageFile(const wxString& aFullFilename)
{
    wxFileInputStream file_stream(aFullFilename);

    // Check if the file could be opened successfully
    if (!file_stream.IsOk())
        return false;

    return ReadImageFile(file_stream);
}


bool BITMAP_BASE::SetImage( const wxImage& aImage )
{
    if( !aImage.IsOk() || aImage.GetWidth() == 0 || aImage.GetHeight() == 0 )
        return false;

    delete m_image;
    m_image = new wxImage( aImage );

    // Create a new wxImage object from m_image
    delete m_originalImage;
    m_originalImage = new wxImage( *m_image );

    rebuildBitmap();
    updatePPI();

    return true;
}


bool BITMAP_BASE::SaveImageData( wxOutputStream& aOutStream ) const
{
    if( m_imageData.IsEmpty() )
    {
        // If m_imageData is empty, use wxImage::Save() method to write m_image contents to
        // the stream.
        wxBitmapType type = m_imageType == wxBITMAP_TYPE_JPEG ? wxBITMAP_TYPE_JPEG
                                                              : wxBITMAP_TYPE_PNG;

        if( !m_image->SaveFile( aOutStream, type ) )
        {
            return false;
        }
    }
    else
    {
        // Write the contents of m_imageData to the stream.
        aOutStream.Write( m_imageData.GetData(), m_imageData.GetDataLen() );
    }

    return true;
}


bool BITMAP_BASE::LoadLegacyData( LINE_READER& aLine, wxString& aErrorMsg )
{
    wxMemoryOutputStream stream;
    char* line;

    while( true )
    {
        if( !aLine.ReadLine() )
        {
            aErrorMsg = wxT( "Unexpected end of data" );
            return false;
        }

        line = aLine.Line();

        if( strncasecmp( line, "EndData", 4 ) == 0 )
        {
            // all the PNG date is read.
            // We expect here m_image and m_bitmap are void
            m_image = new wxImage();
            wxMemoryInputStream istream( stream );
            m_image->LoadFile( istream, wxBITMAP_TYPE_ANY );
            m_bitmap = new wxBitmap( *m_image );
            m_originalImage = new wxImage( *m_image );
            updateImageDataBuffer();
            break;
        }

        // Read PNG data, stored in hexadecimal,
        // each byte = 2 hexadecimal digits and a space between 2 bytes
        // and put it in memory stream buffer
        int len = strlen( line );

        for( ; len > 0; len -= 3, line += 3 )
        {
            int value = 0;

            if( sscanf( line, "%X", &value ) == 1 )
                stream.PutC( (char) value );
            else
                break;
        }
    }

    return true;
}


const BOX2I BITMAP_BASE::GetBoundingBox() const
{
    BOX2I    bbox;
    VECTOR2I size = GetSize();

    bbox.Inflate( size.x / 2, size.y / 2 );

    return bbox;
}


void BITMAP_BASE::DrawBitmap( wxDC* aDC, const VECTOR2I& aPos,
                              const KIGFX::COLOR4D& aBackgroundColor ) const
{
    if( m_bitmap == nullptr )
        return;

    VECTOR2I pos = aPos;
    VECTOR2I size = GetSize();

    // This fixes a bug in OSX that should be fixed in the 3.0.3 version or later.
    if( ( size.x == 0 ) || ( size.y == 0 ) )
        return;

    // To draw the bitmap, pos is the upper left corner position
    pos.x -= size.x / 2;
    pos.y -= size.y / 2;

    double scale;
    int    logicalOriginX, logicalOriginY;
    aDC->GetUserScale( &scale, &scale );
    aDC->GetLogicalOrigin( &logicalOriginX, &logicalOriginY );

    // We already have issues to draw a bitmap on the wxDC, depending on wxWidgets version.
    // Now we have an issue on wxWidgets 3.1.6 to fix the clip area
    // and the bitmap position when using TransformMatrix
    // So for version == 3.1.6  do not use it
    // Be careful before changing the code.
    bool useTransform = aDC->CanUseTransformMatrix();

    wxAffineMatrix2D init_matrix = aDC->GetTransformMatrix();

    // Note: clipping bitmap area was made to fix a minor issue in old versions of
    // KiCad/wxWidgets (5.1 / wx 3.0)
    // However SetClippingRegion creates a lot of issues (different ways to fix the
    // position and size of the area, depending on wxWidgets version)because it changes with
    // each versions of wxWidgets, so it is now disabled
    // However the code is still here, just in case
    // #define USE_CLIP_AREA

    wxPoint clipAreaPos;

    if( useTransform )
    {
        wxAffineMatrix2D matrix = aDC->GetTransformMatrix();
        matrix.Translate( pos.x, pos.y );
        matrix.Scale( GetScalingFactor(), GetScalingFactor() );
        aDC->SetTransformMatrix( matrix );

        // Needed on wx <= 3.1.5, and this is strange...
        // Nevertheless, this code has problem (the bitmap is not seen)
        // with wx version > 3.1.5
        clipAreaPos.x = pos.x;
        clipAreaPos.y = pos.y;

        pos.x = pos.y = 0;
    }
    else
    {
        aDC->SetUserScale( scale * GetScalingFactor(), scale * GetScalingFactor() );
        aDC->SetLogicalOrigin( logicalOriginX / GetScalingFactor(),
                               logicalOriginY / GetScalingFactor() );

        pos.x  = KiROUND( pos.x / GetScalingFactor() );
        pos.y  = KiROUND( pos.y / GetScalingFactor() );
        size.x = KiROUND( size.x / GetScalingFactor() );
        size.y = KiROUND( size.y / GetScalingFactor() );
        clipAreaPos.x = pos.x;
        clipAreaPos.y = pos.y;
    }

#ifdef USE_CLIP_AREA
    aDC->DestroyClippingRegion();
    aDC->SetClippingRegion( clipAreaPos, wxSize( size.x, size.y ) );
#endif

    if( aBackgroundColor != COLOR4D::UNSPECIFIED && m_bitmap->HasAlpha() )
    {
        // Most printers don't support transparent images properly,
        // so blend the image with background color.

        int w = m_bitmap->GetWidth();
        int h = m_bitmap->GetHeight();

        wxImage  image( w, h );
        wxColour bgColor = aBackgroundColor.ToColour();

        image.SetRGB( wxRect( 0, 0, w, h ), bgColor.Red(), bgColor.Green(), bgColor.Blue() );
        image.Paste( m_bitmap->ConvertToImage(), 0, 0, wxIMAGE_ALPHA_BLEND_COMPOSE );

        if( GetGRForceBlackPenState() )
            image = image.ConvertToGreyscale();

        aDC->DrawBitmap( wxBitmap( image ), pos.x, pos.y, true );
    }
    else if( GetGRForceBlackPenState() )
    {
        wxBitmap result( m_bitmap->ConvertToImage().ConvertToGreyscale() );
        aDC->DrawBitmap( result, pos.x, pos.y, true );
    }
    else
    {
        aDC->DrawBitmap( *m_bitmap, pos.x, pos.y, true );
    }

    if( useTransform )
        aDC->SetTransformMatrix( init_matrix );
    else
    {
        aDC->SetUserScale( scale, scale );
        aDC->SetLogicalOrigin( logicalOriginX, logicalOriginY );
    }

#ifdef USE_CLIP_AREA
    aDC->DestroyClippingRegion();
#endif
}


VECTOR2I BITMAP_BASE::GetSize() const
{
    VECTOR2I size;

    if( m_bitmap )
    {
        size.x = m_bitmap->GetWidth();
        size.y = m_bitmap->GetHeight();

        size.x = KiROUND( size.x * GetScalingFactor() );
        size.y = KiROUND( size.y * GetScalingFactor() );
    }

    return size;
}


void BITMAP_BASE::Mirror( FLIP_DIRECTION aFlipDirection )
{
    if( m_image )
    {
        // wxImage::Mirror() clear some parameters of the original image.
        // We need to restore them, especially resolution and unit, to be
        // sure image parameters saved in file are the right parameters, not
        // the default values
        int resX = m_image->GetOptionInt( wxIMAGE_OPTION_RESOLUTIONX );
        int resY = m_image->GetOptionInt( wxIMAGE_OPTION_RESOLUTIONY );
        int unit = m_image->GetOptionInt( wxIMAGE_OPTION_RESOLUTIONUNIT );

        *m_image = m_image->Mirror( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT );

        m_image->SetOption( wxIMAGE_OPTION_RESOLUTIONUNIT , unit);
        m_image->SetOption( wxIMAGE_OPTION_RESOLUTIONX, resX);
        m_image->SetOption( wxIMAGE_OPTION_RESOLUTIONY, resY);

        if( aFlipDirection == FLIP_DIRECTION::TOP_BOTTOM )
            m_isMirroredY = !m_isMirroredY;
        else
            m_isMirroredX = !m_isMirroredX;

        rebuildBitmap( false );
        updateImageDataBuffer();
    }
}


void BITMAP_BASE::Rotate( bool aRotateCCW )
{
    if( m_image )
    {
        // wxImage::Rotate90() clear some parameters of the original image.
        // We need to restore them, especially resolution and unit, to be
        // sure image parameters saved in file are the right parameters, not
        // the default values
        int resX = m_image->GetOptionInt( wxIMAGE_OPTION_RESOLUTIONX );
        int resY = m_image->GetOptionInt( wxIMAGE_OPTION_RESOLUTIONY );
        int unit = m_image->GetOptionInt( wxIMAGE_OPTION_RESOLUTIONUNIT );

        *m_image = m_image->Rotate90( aRotateCCW );

        m_image->SetOption( wxIMAGE_OPTION_RESOLUTIONUNIT, unit );
        m_image->SetOption( wxIMAGE_OPTION_RESOLUTIONX, resX );
        m_image->SetOption( wxIMAGE_OPTION_RESOLUTIONY, resY );

        m_rotation += ( aRotateCCW ? ANGLE_90 : -ANGLE_90 );
        rebuildBitmap( false );
        updateImageDataBuffer();
    }
}


void BITMAP_BASE::ConvertToGreyscale()
{
    if( m_image )
    {
        *m_image  = m_image->ConvertToGreyscale();
        *m_originalImage = m_originalImage->ConvertToGreyscale();
        rebuildBitmap();
        updateImageDataBuffer();
    }
}


void BITMAP_BASE::PlotImage( PLOTTER*       aPlotter, const VECTOR2I& aPos,
                             const COLOR4D& aDefaultColor,
                             int            aDefaultPensize ) const
{
    if( m_image == nullptr )
        return;

    // These 2 lines are useful only for plotters that cannot plot a bitmap
    // and plot a rectangle instead of.
    aPlotter->SetColor( aDefaultColor );
    aPlotter->SetCurrentLineWidth( aDefaultPensize );
    aPlotter->PlotImage( *m_image, aPos, GetScalingFactor() );
}


void BITMAP_BASE::updateImageDataBuffer()
{
    if( m_image )
    {
        wxMemoryOutputStream stream;
        wxBitmapType type = m_imageType == wxBITMAP_TYPE_JPEG ? wxBITMAP_TYPE_JPEG
                                                              : wxBITMAP_TYPE_PNG;

        if( !m_image->SaveFile( stream, type ) )
            return;

        m_imageData.GetWriteBuf( stream.GetLength() );
        stream.CopyTo( m_imageData.GetData(), stream.GetLength() );
        m_imageData.SetDataLen( stream.GetLength() );
    }
}
