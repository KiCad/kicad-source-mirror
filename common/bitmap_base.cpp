/**
 * @file class_bitmap_base.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 jean-pierre.charras
 * Copyright (C) 2011-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmap_base.h>
#include <eda_rect.h>     // for EDA_RECT
#include <gal/color4d.h>  // for COLOR4D
#include <gr_basic.h>
#include <math/util.h>      // for KiROUND
#include <memory>         // for make_unique, unique_ptr
#include <plotter.h>
#include <richio.h>
#include <wx/bitmap.h>    // for wxBitmap
#include <wx/mstream.h>


BITMAP_BASE::BITMAP_BASE( const wxPoint& pos )
{
    m_scale  = 1.0;                     // 1.0 = original bitmap size
    m_bitmap = nullptr;
    m_image  = nullptr;
    m_ppi    = 300;                     // the bitmap definition. the default is 300PPI
    m_pixelSizeIu = 254000.0 / m_ppi;   // a pixel size value OK for bitmaps using 300 PPI
                                        // for Eeschema which uses currently 254000PPI
}


BITMAP_BASE::BITMAP_BASE( const BITMAP_BASE& aSchBitmap )
{
    m_scale = aSchBitmap.m_scale;
    m_ppi   = aSchBitmap.m_ppi;
    m_pixelSizeIu = aSchBitmap.m_pixelSizeIu;

    m_image = nullptr;
    m_bitmap = nullptr;

    if( aSchBitmap.m_image )
    {
        m_image = new wxImage( *aSchBitmap.m_image );
        m_bitmap = new wxBitmap( *m_image );
    }
}


void BITMAP_BASE::ImportData( BITMAP_BASE* aItem )
{
    *m_image  = *aItem->m_image;
    *m_bitmap = *aItem->m_bitmap;
    m_scale   = aItem->m_scale;
    m_ppi     = aItem->m_ppi;
    m_pixelSizeIu = aItem->m_pixelSizeIu;
}


bool BITMAP_BASE::ReadImageFile( wxInputStream& aInStream )
{
    auto new_image = std::make_unique<wxImage>();

    if( !new_image->LoadFile( aInStream ) )
    {
        return false;
    }

    delete m_image;
    m_image = new_image.release();
    m_bitmap = new wxBitmap( *m_image );

    return true;
}


bool BITMAP_BASE::ReadImageFile( const wxString& aFullFilename )
{
    wxImage* new_image = new wxImage();

    if( !new_image->LoadFile( aFullFilename ) )
    {
        delete new_image;
        return false;
    }

    delete m_image;
    m_image  = new_image;
    m_bitmap = new wxBitmap( *m_image );

    return true;
}


bool BITMAP_BASE::SaveData( FILE* aFile ) const
{
    if( m_image )
    {
        wxMemoryOutputStream stream;
        m_image->SaveFile( stream, wxBITMAP_TYPE_PNG );

        // Write binary data in hexadecimal form (ASCII)
        wxStreamBuffer* buffer = stream.GetOutputStreamBuffer();
        char*           begin  = (char*) buffer->GetBufferStart();

        for( int ii = 0; begin < buffer->GetBufferEnd(); begin++, ii++ )
        {
            if( ii >= 32 )
            {
                ii = 0;

                if( fprintf( aFile, "\n" ) == EOF )
                    return false;
            }

            if( fprintf( aFile, "%2.2X ", *begin & 0xFF ) == EOF )
                return false;
        }
    }

    return true;
}


void BITMAP_BASE::SaveData( wxArrayString& aPngStrings ) const
{
    if( m_image )
    {
        wxMemoryOutputStream stream;
        m_image->SaveFile( stream, wxBITMAP_TYPE_PNG );

        // Write binary data in hexadecimal form (ASCII)
        wxStreamBuffer* buffer = stream.GetOutputStreamBuffer();
        char*           begin  = (char*) buffer->GetBufferStart();
        wxString        line;

        for( int ii = 0; begin < buffer->GetBufferEnd(); begin++, ii++ )
        {
            if( ii >= 32 )
            {
                ii = 0;
                aPngStrings.Add( line );
                line.Empty();
            }

            line << wxString::Format( wxT( "%2.2X " ), *begin & 0xFF );
        }

        // Add last line:
        if( !line.IsEmpty() )
            aPngStrings.Add( line );
    }
}


bool BITMAP_BASE::LoadData( LINE_READER& aLine, wxString& aErrorMsg )
{
    wxMemoryOutputStream stream;
    char* line;

    while( true )
    {
        if( !aLine.ReadLine() )
        {
            aErrorMsg = wxT("Unexpected end of data");
            return false;
        }

        line = aLine.Line();

        if( strncasecmp( line, "EndData", 4 ) == 0 )
        {
            // all the PNG date is read.
            // We expect here m_image and m_bitmap are void
            m_image = new wxImage();
            wxMemoryInputStream istream( stream );
            m_image->LoadFile( istream, wxBITMAP_TYPE_PNG );
            m_bitmap = new wxBitmap( *m_image );
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


const EDA_RECT BITMAP_BASE::GetBoundingBox() const
{
    EDA_RECT rect;

    wxSize   size = GetSize();

    rect.Inflate( size.x / 2, size.y / 2 );

    return rect;
}


void BITMAP_BASE::DrawBitmap( wxDC* aDC, const wxPoint& aPos )
{
    if( m_bitmap == nullptr )
        return;

    wxPoint pos  = aPos;
    wxSize  size = GetSize();

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

    bool useTransform = aDC->CanUseTransformMatrix();
    wxAffineMatrix2D init_matrix = aDC->GetTransformMatrix();

    if( useTransform )
    {
        wxAffineMatrix2D matrix = aDC->GetTransformMatrix();
        matrix.Translate( pos.x, pos.y );
        matrix.Scale( GetScalingFactor(), GetScalingFactor() );
        aDC->SetTransformMatrix( matrix );
        pos.x = pos.y = 0;
    }
    else
    {
        aDC->SetUserScale( scale * GetScalingFactor(), scale * GetScalingFactor() );
        aDC->SetLogicalOrigin( logicalOriginX / GetScalingFactor(),
                               logicalOriginY / GetScalingFactor() );

        pos.x  = KiROUND( pos.x / GetScalingFactor() );
        pos.y  = KiROUND( pos.y / GetScalingFactor() );
    }

    if( GetGRForceBlackPenState() )
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
}


wxSize BITMAP_BASE::GetSize() const
{
    wxSize size;

    if( m_bitmap )
    {
        size.x = m_bitmap->GetWidth();
        size.y = m_bitmap->GetHeight();

        size.x = KiROUND( size.x * GetScalingFactor() );
        size.y = KiROUND( size.y * GetScalingFactor() );
    }

    return size;
}


void BITMAP_BASE::Mirror( bool aVertically )
{
    if( m_image )
    {
        *m_image  = m_image->Mirror( not aVertically );
        RebuildBitmap();
    }
}


void BITMAP_BASE::Rotate( bool aRotateCCW )
{
    if( m_image )
    {
        *m_image  = m_image->Rotate90( aRotateCCW );
        RebuildBitmap();
    }
}


void BITMAP_BASE::PlotImage( PLOTTER*       aPlotter,
                             const wxPoint& aPos,
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
