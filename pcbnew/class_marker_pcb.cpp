/**
 * @file class_marker_pcb.cpp
 * @brief Functions to handle markers used to show something (usually a drc problem)
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_drawpanel.h>
#include <wxstruct.h>
#include <trigo.h>
#include <msgpanel.h>

#include <pcbnew.h>
#include <class_marker_pcb.h>
#include <layers_id_colors_and_visibility.h>


/// Adjust the actual size of markers, when using default shape
#define SCALING_FACTOR      DMils2iu( 30 )


MARKER_PCB::MARKER_PCB( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, PCB_MARKER_T ),
    MARKER_BASE(), m_item( NULL )
{
    m_Color = WHITE;
    m_ScalingFactor = SCALING_FACTOR;
}


MARKER_PCB::MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                        const wxString& aText, const wxPoint& aPos,
                        const wxString& bText, const wxPoint& bPos ) :
    BOARD_ITEM( NULL, PCB_MARKER_T ),  // parent set during BOARD::Add()
    MARKER_BASE( aErrorCode, aMarkerPos, aText, aPos, bText, bPos ), m_item( NULL )
{
    m_Color = WHITE;
    m_ScalingFactor = SCALING_FACTOR;
}

MARKER_PCB::MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                        const wxString& aText, const wxPoint& aPos ) :
    BOARD_ITEM( NULL, PCB_MARKER_T ),  // parent set during BOARD::Add()
    MARKER_BASE( aErrorCode, aMarkerPos, aText,  aPos ), m_item( NULL )
{
    m_Color = WHITE;
    m_ScalingFactor = SCALING_FACTOR;
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
bool MARKER_PCB::IsOnLayer( LAYER_NUM aLayer ) const
{
    return IsCopperLayer( aLayer );
}

void MARKER_PCB::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    const DRC_ITEM& rpt = m_drc;

    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), _( "Marker" ), DARKCYAN ) );

    wxString errorTxt;

    errorTxt.Printf( _( "ErrType (%d)- %s:" ),
            rpt.GetErrorCode(),
            GetChars( rpt.GetErrorText() ) );

    aList.push_back( MSG_PANEL_ITEM( errorTxt, wxEmptyString, RED ) );

    wxString txtA;
    txtA << DRC_ITEM::ShowCoord( rpt.GetPointA() ) << wxT( ": " ) << rpt.GetTextA();

    wxString txtB;

    if ( rpt.HasSecondItem() )
        txtB << DRC_ITEM::ShowCoord( rpt.GetPointB() ) << wxT( ": " ) << rpt.GetTextB();

    aList.push_back( MSG_PANEL_ITEM( txtA, txtB, DARKBROWN ) );
}


void MARKER_PCB::Rotate(const wxPoint& aRotCentre, double aAngle)
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );
}


void MARKER_PCB::Flip(const wxPoint& aCentre )
{
    m_Pos.y  = aCentre.y - (m_Pos.y - aCentre.y);
}


wxString MARKER_PCB::GetSelectMenuText() const
{
    wxString text;
    text.Printf( _( "Marker @(%d,%d)" ), GetPos().x, GetPos().y );

    return text;
}


void MARKER_PCB::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;
    aLayers[0] = ITEM_GAL_LAYER( DRC_VISIBLE );
}
