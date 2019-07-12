/**
 * @file class_marker_pcb.cpp
 * @brief Functions to handle markers used to show something (usually a drc problem)
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gr_basic.h>
#include <trigo.h>
#include <msgpanel.h>
#include <bitmaps.h>
#include <base_units.h>
#include <pcbnew.h>
#include <class_marker_pcb.h>
#include <layers_id_colors_and_visibility.h>


/// Factor to convert the maker unit shape to internal units:
#define SCALING_FACTOR  Millimeter2iu( 0.1 )

MARKER_PCB::MARKER_PCB( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, PCB_MARKER_T ),
    MARKER_BASE( SCALING_FACTOR ), m_item( nullptr )
{
    m_Color = WHITE;
}


MARKER_PCB::MARKER_PCB( EDA_UNITS_T aUnits, int aErrorCode, const wxPoint& aMarkerPos,
                        BOARD_ITEM* aItem, const wxPoint& aPos,
                        BOARD_ITEM* bItem, const wxPoint& bPos ) :
    BOARD_ITEM( nullptr, PCB_MARKER_T ),  // parent set during BOARD::Add()
    MARKER_BASE( aUnits, aErrorCode, aMarkerPos, aItem, aPos, bItem, bPos, SCALING_FACTOR ), m_item( nullptr )
{
    m_Color = WHITE;
}


MARKER_PCB::MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                        const wxString& aText, const wxPoint& aPos,
                        const wxString& bText, const wxPoint& bPos ) :
    BOARD_ITEM( nullptr, PCB_MARKER_T ),  // parent set during BOARD::Add()
    MARKER_BASE( aErrorCode, aMarkerPos, aText, aPos, bText, bPos, SCALING_FACTOR ), m_item( nullptr )
{
    m_Color = WHITE;
}


/* destructor */
MARKER_PCB::~MARKER_PCB()
{
}

/* tests to see if this object is on the given layer.
 * DRC markers are not really on a copper layer, but
 * MARKER_PCB::IsOnCopperLayer return true if aLayer is a cooper layer,
 * because this test is often used to locad a marker
 * param aLayer The layer to test for.
 * return bool - true if on given layer, else false.
 */
bool MARKER_PCB::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
    return IsCopperLayer( aLayer );
}

void MARKER_PCB::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString errorTxt, txtA, txtB;

    aList.emplace_back( MSG_PANEL_ITEM( _( "Type" ), _( "Marker" ), DARKCYAN ) );

    errorTxt.Printf( _( "ErrType (%d)- %s:" ), m_drc.GetErrorCode(), m_drc.GetErrorText() );

    aList.emplace_back( MSG_PANEL_ITEM( errorTxt, wxEmptyString, RED ) );

    txtA.Printf( wxT( "%s: %s" ), DRC_ITEM::ShowCoord( aUnits, m_drc.GetPointA() ), m_drc.GetTextA() );

    if( m_drc.HasSecondItem() )
        txtB.Printf( wxT( "%s: %s" ), DRC_ITEM::ShowCoord( aUnits, m_drc.GetPointB() ), m_drc.GetTextB() );

    aList.emplace_back( MSG_PANEL_ITEM( txtA, txtB, DARKBROWN ) );
}


void MARKER_PCB::Rotate(const wxPoint& aRotCentre, double aAngle)
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );
}


void MARKER_PCB::Flip(const wxPoint& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
        m_Pos.x = aCentre.x - ( m_Pos.x - aCentre.x );
    else
        m_Pos.y = aCentre.y - ( m_Pos.y - aCentre.y );
}


wxString MARKER_PCB::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Marker @(%s, %s)" ),
                             MessageTextFromValue( aUnits, m_Pos.x ),
                             MessageTextFromValue( aUnits, m_Pos.y ) );
}


BITMAP_DEF MARKER_PCB::GetMenuImage() const
{
    return drc_xpm;
}


void MARKER_PCB::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;
    aLayers[0] = LAYER_DRC;
}

const EDA_RECT MARKER_PCB::GetBoundingBox() const
{
    EDA_RECT bbox = m_ShapeBoundingBox;

    wxPoint pos = m_Pos;
    pos.x += int( bbox.GetOrigin().x * MarkerScale() );
    pos.y += int( bbox.GetOrigin().y * MarkerScale() );

    return EDA_RECT( pos, wxSize( int( bbox.GetWidth() * MarkerScale() ),
                                  int( bbox.GetHeight() * MarkerScale() ) ) );
}


const BOX2I MARKER_PCB::ViewBBox() const
{
    EDA_RECT bbox = GetBoundingBox();
    return BOX2I( bbox.GetOrigin(), VECTOR2I( bbox.GetWidth(), bbox.GetHeight() ) );
}
