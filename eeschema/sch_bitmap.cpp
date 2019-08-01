/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011-2019 KiCad Developers, see change_log.txt for contributors.
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
#include <sch_draw_panel.h>
#include <trigo.h>
#include <macros.h>
#include <bitmaps.h>
#include <base_units.h>

#include <sch_bitmap.h>

#include <wx/mstream.h>


SCH_BITMAP::SCH_BITMAP( const wxPoint& pos ) :
    SCH_ITEM( NULL, SCH_BITMAP_T )
{
    m_pos   = pos;
    m_Layer = LAYER_NOTES;              // used only to draw/plot a rectangle,
                                        // when a bitmap cannot be drawn or plotted
    m_image = new BITMAP_BASE();
}


SCH_BITMAP::SCH_BITMAP( const SCH_BITMAP& aSchBitmap ) :
    SCH_ITEM( aSchBitmap )
{
    m_pos   = aSchBitmap.m_pos;
    m_Layer = aSchBitmap.m_Layer;
    m_image = new BITMAP_BASE( *aSchBitmap.m_image );
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

        delete m_image;
        m_image = new BITMAP_BASE( *bitmap->m_image );
        m_pos = bitmap->m_pos;
    }

    return *this;
}


bool SCH_BITMAP::ReadImageFile( const wxString& aFullFilename )
{
    return m_image->ReadImageFile( aFullFilename );
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
    std::swap( m_pos, item->m_pos );
    std::swap( m_image, item->m_image );
}


const EDA_RECT SCH_BITMAP::GetBoundingBox() const
{
    EDA_RECT rect = m_image->GetBoundingBox();

    rect.Move( m_pos );

    return rect;
}


void SCH_BITMAP::Print( wxDC* aDC, const wxPoint& aOffset  )
{
    wxPoint pos = m_pos + aOffset;

    m_image->DrawBitmap( aDC, pos );
}


wxSize SCH_BITMAP::GetSize() const
{
    return m_image->GetSize();
}


void SCH_BITMAP::MirrorX( int aXaxis_position )
{
    MIRROR( m_pos.y, aXaxis_position );
    m_image->Mirror( true );
}


void SCH_BITMAP::MirrorY( int aYaxis_position )
{
    MIRROR( m_pos.x, aYaxis_position );
    m_image->Mirror( false );
}


void SCH_BITMAP::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_pos, aPosition, 900 );
    m_image->Rotate( false );
}


#if defined(DEBUG)
void SCH_BITMAP::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << m_pos << "/>\n";
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
    m_image->PlotImage( aPlotter, m_pos, GetLayerColor( GetLayer() ), GetPenSize() );
}


BITMAP_DEF SCH_BITMAP::GetMenuImage() const
{
    return image_xpm;
}


void SCH_BITMAP::GetMsgPanelInfo( EDA_UNITS_T aUnits, MSG_PANEL_ITEMS& aList )
{
    aList.push_back( MSG_PANEL_ITEM( _( "Bitmap" ), wxEmptyString, RED ) );

    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), MessageTextFromValue( aUnits, GetSize().x ), RED ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Height" ), MessageTextFromValue( aUnits, GetSize().y ), RED ) );
}


void SCH_BITMAP::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;
    aLayers[0] = LAYER_DRAW_BITMAPS;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}
