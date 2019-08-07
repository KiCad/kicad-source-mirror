/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <sch_draw_panel.h>
#include <trigo.h>
#include <msgpanel.h>
#include <bitmaps.h>
#include <base_units.h>

#include <sch_marker.h>

/// Factor to convert the maker unit shape to internal units:
#define SCALING_FACTOR  Millimeter2iu( 0.1 )


SCH_MARKER::SCH_MARKER() : SCH_ITEM( NULL, SCH_MARKER_T ), MARKER_BASE( SCALING_FACTOR )
{
}


SCH_MARKER::SCH_MARKER( const wxPoint& pos, const wxString& text ) :
    SCH_ITEM( NULL, SCH_MARKER_T ),
    MARKER_BASE( 0, pos, text, pos, SCALING_FACTOR )
{
}


EDA_ITEM* SCH_MARKER::Clone() const
{
    return new SCH_MARKER( *this );
}


void SCH_MARKER::SwapData( SCH_ITEM* aItem )
{
    std::swap( *((SCH_MARKER*) this), *((SCH_MARKER*) aItem ) );
}


#if defined(DEBUG)

void SCH_MARKER::Show( int nestLevel, std::ostream& os ) const
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << GetPos() << "/>\n";
}

#endif


void SCH_MARKER::Print( wxDC* aDC, const wxPoint& aOffset )
{
    COLOR4D tmp = m_Color;

    if( GetMarkerType() == MARKER_BASE::MARKER_ERC )
    {
        m_Color = ( GetErrorLevel() == MARKER_BASE::MARKER_SEVERITY_ERROR ) ?
                    GetLayerColor( LAYER_ERC_ERR ) : GetLayerColor( LAYER_ERC_WARN );
    }

    PrintMarker( aDC, aOffset );
    m_Color = tmp;
}


bool SCH_MARKER::Matches( wxFindReplaceData& aSearchData, void* aAuxData )
{
    return SCH_ITEM::Matches( m_drc.GetErrorText(), aSearchData )
                || SCH_ITEM::Matches( m_drc.GetMainText(), aSearchData )
                || SCH_ITEM::Matches( m_drc.GetAuxiliaryText(), aSearchData );
}


void SCH_MARKER::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 2;
    aLayers[0] = this->m_ErrorLevel == MARKER_SEVERITY_ERROR ? LAYER_ERC_ERR : LAYER_ERC_WARN;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


const EDA_RECT SCH_MARKER::GetBoundingBox() const
{
    return GetBoundingBoxMarker();
}


void SCH_MARKER::GetMsgPanelInfo( EDA_UNITS_T aUnits, MSG_PANEL_ITEMS& aList )
{
    aList.push_back( MSG_PANEL_ITEM( _( "Electronics Rule Check Error" ),
                                     GetReporter().GetErrorText(), DARKRED ) );
}


BITMAP_DEF SCH_MARKER::GetMenuImage() const
{
    return erc_xpm;
}


void SCH_MARKER::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_Pos, aPosition, 900 );
}


void SCH_MARKER::MirrorX( int aXaxis_position )
{
    m_Pos.y -= aXaxis_position;
    m_Pos.y  = -m_Pos.y;
    m_Pos.y += aXaxis_position;
}


void SCH_MARKER::MirrorY( int aYaxis_position )
{
    m_Pos.x -= aYaxis_position;
    m_Pos.x  = -m_Pos.x;
    m_Pos.x += aYaxis_position;
}


bool SCH_MARKER::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    return HitTestMarker( aPosition, aAccuracy );
}
