/**
 * @file class_bitmap_base.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011 KiCad Developers, see change_log.txt for contributors.
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

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "trigo.h"
#include "common.h"
#include "richio.h"
#include "plot_common.h"

#include "class_bitmap_base.h"

#include <wx/mstream.h>


/**********************/
/* class BITMAP_BASE */
/**********************/

BITMAP_BASE::BITMAP_BASE( const wxPoint& pos )
{
    m_Scale  = 1.0;                 // 1.0 = original bitmap size
    m_bitmap = NULL;
    m_image  = NULL;
    m_ppi    = 300;                 // the bitmap definition. the default is 300PPI
    m_pixelScaleFactor = 1000.0 / m_ppi;    // a value OK for bitmaps using 300 PPI
                                            // for Eeschema which uses currently 1000PPI
}


BITMAP_BASE::BITMAP_BASE( const BITMAP_BASE& aSchBitmap )
{
    m_Scale = aSchBitmap.m_Scale;
    m_pixelScaleFactor = aSchBitmap.m_pixelScaleFactor;
    m_image = new wxImage( *aSchBitmap.m_image );
    m_bitmap = new wxBitmap( *m_image );
}


/**
 * Function ImportData
 * Copy aItem image to me and update m_bitmap
 */
void BITMAP_BASE::ImportData( BITMAP_BASE* aItem )
{
    *m_image  = *aItem->m_image;
    *m_bitmap = *aItem->m_bitmap;
    m_Scale   = aItem->m_Scale;
    m_ppi     = aItem->m_ppi;
    m_pixelScaleFactor = aItem->m_pixelScaleFactor;
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
        int             ii;
        for( ii = 0; begin <= buffer->GetBufferEnd(); begin++, ii++ )
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
        wxString line;
        for( int ii = 0; begin <= buffer->GetBufferEnd(); begin++, ii++ )
        {
            if( ii >= 32 )
            {
                ii = 0;
                aPngStrings.Add( line );
                line.Empty();
            }

            line << wxString::Format( wxT("%2.2X "), *begin & 0xFF );
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

        if( strnicmp( line, "EndData", 4 ) == 0 )
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


void BITMAP_BASE::DrawBitmap( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPos )
{
    if( m_bitmap == NULL )
        return;

    wxPoint pos  = aPos;
    wxSize  size = GetSize();

    // To draw the bitmap, pos is the upper left corner position
    pos.x -= size.x / 2;
    pos.y -= size.y / 2;

    double scale;
    int    logicalOriginX, logicalOriginY;
    aDC->GetUserScale( &scale, &scale );
    aDC->GetLogicalOrigin( &logicalOriginX, &logicalOriginY );
    aDC->SetUserScale( scale * GetScalingFactor(), scale * GetScalingFactor() );
    aDC->SetLogicalOrigin( logicalOriginX / GetScalingFactor(),
                          logicalOriginY / GetScalingFactor() );
    aDC->DrawBitmap( *m_bitmap,
                     KiROUND( pos.x / GetScalingFactor() ),
                     KiROUND( pos.y / GetScalingFactor() ),
                     true );
    aDC->SetUserScale( scale, scale );
    aDC->SetLogicalOrigin( logicalOriginX, logicalOriginY );
}


/* Function GetSize
 * returns the actual size (in user units, not in pixels) of the image
 */
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


/*
 * Mirror image vertically (i.e. relative to its horizontal X axis )
 *  or horizontally (i.e relative to its vertical Y axis)
 * param aVertically = false to mirror horizontally
 *                      or true to mirror vertically
 */
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
                             EDA_COLOR_T    aDefaultColor,
                             int            aDefaultPensize )
{
    if( m_image == NULL )
        return;

    // These 2 lines are useful only fot plotters that cannot plot a bitmap
    // and plot a rectangle instead of.
    aPlotter->SetColor( aDefaultColor );
    aPlotter->SetCurrentLineWidth( aDefaultPensize );

    aPlotter->PlotImage( *m_image, aPos, GetScalingFactor() );
}
