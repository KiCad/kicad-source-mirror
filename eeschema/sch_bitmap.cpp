/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
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

/**
 * @file sch_bitmap.cpp
 */

#include <sch_draw_panel.h>
#include <plotter.h>
#include <settings/color_settings.h>
#include <bitmaps.h>
#include <base_units.h>
#include <common.h>
#include <eda_draw_frame.h>
#include <core/mirror.h>
#include <sch_bitmap.h>
#include <trigo.h>

#include <wx/mstream.h>


SCH_BITMAP::SCH_BITMAP( const wxPoint& pos ) :
    SCH_ITEM( nullptr, SCH_BITMAP_T )
{
    m_pos   = pos;
    m_layer = LAYER_NOTES;              // used only to draw/plot a rectangle,
                                        // when a bitmap cannot be drawn or plotted
    m_image = new BITMAP_BASE();
}


SCH_BITMAP::SCH_BITMAP( const SCH_BITMAP& aSchBitmap ) :
    SCH_ITEM( aSchBitmap )
{
    m_pos   = aSchBitmap.m_pos;
    m_layer = aSchBitmap.m_layer;
    m_image = new BITMAP_BASE( *aSchBitmap.m_image );
}


SCH_BITMAP& SCH_BITMAP::operator=( const SCH_ITEM& aItem )
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
                                   aItem->GetClass() ) );

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


void SCH_BITMAP::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset  )
{
    wxPoint pos = m_pos + aOffset;

    m_image->DrawBitmap( aSettings->GetPrintDC(), pos );
}


wxSize SCH_BITMAP::GetSize() const
{
    return m_image->GetSize();
}


void SCH_BITMAP::MirrorVertically( int aCenter )
{
    MIRROR( m_pos.y, aCenter );
    m_image->Mirror( true );
}


void SCH_BITMAP::MirrorHorizontally( int aCenter )
{
    MIRROR( m_pos.x, aCenter );
    m_image->Mirror( false );
}


void SCH_BITMAP::Rotate( const wxPoint& aCenter )
{
    RotatePoint( &m_pos, aCenter, 900 );
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


void SCH_BITMAP::Plot( PLOTTER* aPlotter ) const
{
    m_image->PlotImage( aPlotter, m_pos, aPlotter->RenderSettings()->GetLayerColor( GetLayer() ),
                        aPlotter->RenderSettings()->GetDefaultPenWidth() );
}


BITMAPS SCH_BITMAP::GetMenuImage() const
{
    return BITMAPS::image;
}


void SCH_BITMAP::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    aList.push_back( MSG_PANEL_ITEM( _( "Bitmap" ), wxEmptyString ) );

    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), MessageTextFromValue( aFrame->GetUserUnits(),
                                                                         GetSize().x ) ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Height" ), MessageTextFromValue( aFrame->GetUserUnits(),
                                                                          GetSize().y ) ) );
}


void SCH_BITMAP::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;
    aLayers[0] = LAYER_DRAW_BITMAPS;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}
