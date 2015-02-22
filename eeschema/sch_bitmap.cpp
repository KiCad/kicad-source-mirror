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

/**
 * @file sch_bitmap.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <trigo.h>
#include <macros.h>
#include <sch_bitmap.h>

#include <wx/mstream.h>


/*
 * class SCH_BITMAP
 * This class handle a bitmap image that can be inserted in a schematic.
 */

SCH_BITMAP::SCH_BITMAP( const wxPoint& pos ) :
    SCH_ITEM( NULL, SCH_BITMAP_T )
{
    m_Pos   = pos;
    m_Layer = LAYER_NOTES;              // used only to draw/plot a rectangle,
                                        // when a bitmap cannot be drawn or plotted
    m_Image = new BITMAP_BASE();
}


SCH_BITMAP::SCH_BITMAP( const SCH_BITMAP& aSchBitmap ) :
    SCH_ITEM( aSchBitmap )
{
    m_Pos   = aSchBitmap.m_Pos;
    m_Layer = aSchBitmap.m_Layer;
    m_Image = new BITMAP_BASE( *aSchBitmap.m_Image );
}


SCH_ITEM& SCH_BITMAP::operator=( const SCH_ITEM& aItem )
{
    wxCHECK_MSG( Type() == aItem.Type(), *this,
                 wxT( "Cannot assign object type " ) + aItem.GetClass() + wxT( " to type " ) +
                 GetClass() );

    if( &aItem != this )
    {
        SCH_ITEM::operator=( aItem );

        SCH_BITMAP* bitmap = (SCH_BITMAP*) &aItem;

        delete m_Image;
        m_Image = new BITMAP_BASE( *bitmap->m_Image );
        m_Pos = bitmap->m_Pos;
    }

    return *this;
}


bool SCH_BITMAP::ReadImageFile( const wxString& aFullFilename )
{
    return m_Image->ReadImageFile( aFullFilename );
}


bool SCH_BITMAP::Save( FILE* aFile ) const
{
    if( fprintf( aFile, "$Bitmap\n" ) == EOF )
        return false;

    if( fprintf( aFile, "Pos %-4d %-4d\n", m_Pos.x, m_Pos.y ) == EOF )
        return false;

    if( fprintf( aFile, "Scale %f\n", m_Image->m_Scale ) == EOF )
        return false;

    if( fprintf( aFile, "Data\n" ) == EOF )
        return false;

    if( !m_Image->SaveData( aFile ) )
        return false;

    if( fprintf( aFile, "\nEndData\n" ) == EOF )
        return false;


    if( fprintf( aFile, "$EndBitmap\n" ) == EOF )
        return false;

    return true;
}


EDA_ITEM* SCH_BITMAP::Clone() const
{
    return new SCH_BITMAP( *this );
}


void SCH_BITMAP::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem->Type() == SCH_BITMAP_T,
                 wxString::Format( wxT( "SCH_BITMAP object cannot swap data with %s object." ),
                                   GetChars( aItem->GetClass() ) ) );

    SCH_BITMAP* item = (SCH_BITMAP*) aItem;
    EXCHG( m_Pos, item->m_Pos );
    EXCHG( m_Image, item->m_Image );
}


bool SCH_BITMAP::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char* line = aLine.Line();

    if( strnicmp( line, "$Bitmap", 7 ) != 0 )
    {
        aErrorMsg.Printf( wxT( "Eeschema file bitmap image load error at line %d, aborted" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( (char*) aLine );
        return false;
    }

    for( ; ; )
    {
        if( !aLine.ReadLine() )
            return false;

        line = aLine.Line();

        if( strnicmp( line, "Pos", 3 ) == 0 )
        {
            sscanf( line + 3, "%d %d", &m_Pos.x, &m_Pos.y );
            continue;
        }

        if( strnicmp( line, "Scale", 5 ) == 0 )
        {
            sscanf( line + 5, "%lf", &m_Image->m_Scale );
            continue;
        }

        if( strnicmp( line, "Data", 4 ) == 0 )
        {
            m_Image->LoadData( aLine, aErrorMsg );
        }

        if( strnicmp( line, "$EndBitmap", 4 ) == 0 )
            break;
    }

    return true;
}


const EDA_RECT SCH_BITMAP::GetBoundingBox() const
{
    EDA_RECT rect = m_Image->GetBoundingBox();

    rect.Move( m_Pos );

    return rect;
}


void SCH_BITMAP::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                       GR_DRAWMODE aDrawMode, EDA_COLOR_T aColor )
{
    wxPoint pos = m_Pos + aOffset;

    GRSetDrawMode( aDC, aDrawMode );

    if( aColor < 0 )    // Use normal drawing function
    {
        m_Image->DrawBitmap( aPanel, aDC, pos );
    }
    else    // draws bounding box only (used to move items)
    {
        // To draw the rect, pos is the upper left corner position
        wxSize size = m_Image->GetSize();
        pos.x -= size.x / 2;
        pos.y -= size.y / 2;
        GRRect( aPanel->GetClipBox(), aDC, pos.x, pos.y,
                pos.x + size.x, pos.y + size.y, 0, aColor );
    }
}


/* Function GetSize
 * returns the actual size (in user units, not in pixels) of the image
 */
wxSize SCH_BITMAP::GetSize() const
{
    return m_Image->GetSize();
}


/*
 * Mirror image relative to a horizontal X axis )
 */
void SCH_BITMAP::MirrorX( int aXaxis_position )
{
    m_Pos.y -= aXaxis_position;
    NEGATE( m_Pos.y );
    m_Pos.y += aXaxis_position;

    m_Image->Mirror( true );
}


/*
 * Mirror image relative to a vertical Y axis
 */
void SCH_BITMAP::MirrorY( int aYaxis_position )
{
    m_Pos.x -= aYaxis_position;
    NEGATE( m_Pos.x );
    m_Pos.x += aYaxis_position;
    m_Image->Mirror( false );
}


void SCH_BITMAP::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_Pos, aPosition, 900 );
    m_Image->Rotate( false );
}


bool SCH_BITMAP::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    if( aRect.Contains( m_Pos ) )
        SetFlags( SELECTED );
    else
        ClearFlags( SELECTED );

    return previousState != IsSelected();
}


#if defined(DEBUG)
void SCH_BITMAP::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << m_Pos << "/>\n";
}


#endif


bool SCH_BITMAP::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool SCH_BITMAP::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void SCH_BITMAP::Plot( PLOTTER* aPlotter )
{
    m_Image->PlotImage( aPlotter, m_Pos, GetLayerColor( GetLayer() ), GetPenSize() );
}
