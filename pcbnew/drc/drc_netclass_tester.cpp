/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <drc/drc_netclass_tester.h>


DRC_NETCLASS_TESTER::DRC_NETCLASS_TESTER( MARKER_HANDLER aMarkerHandler ) :
        DRC_TEST_PROVIDER( std::move( aMarkerHandler ) ),
        m_units( EDA_UNITS::MILLIMETRES ),
        m_board( nullptr )
{
}


bool DRC_NETCLASS_TESTER::RunDRC( EDA_UNITS aUnits, BOARD& aBoard )
{
    m_units = aUnits;
    m_board = &aBoard;

    bool        success = true;
    NETCLASSES& netclasses = m_board->GetDesignSettings().m_NetClasses;

    success &= checkNetClass( netclasses.GetDefault() );

    for( NETCLASSES::const_iterator i = netclasses.begin();  i != netclasses.end();  ++i )
        success &= checkNetClass( i->second );

    return success;
}


bool DRC_NETCLASS_TESTER::checkNetClass( const NETCLASSPTR& nc )
{
    bool ret = true;

    const BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    if( nc->GetClearance() < bds.m_MinClearance )
    {
        DRC_ITEM* drcItem = new DRC_ITEM( DRCE_NETCLASS_CLEARANCE );

        m_msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; %s netclass %s)" ),
                      MessageTextFromValue( m_units, bds.m_MinClearance, true ),
                      nc->GetName(),
                      MessageTextFromValue( m_units, nc->GetClearance(), true ) );

        drcItem->SetErrorMessage( m_msg );
        HandleMarker( new MARKER_PCB( drcItem, wxPoint() ) );
        ret = false;
    }

    if( nc->GetTrackWidth() < bds.m_TrackMinWidth )
    {
        DRC_ITEM* drcItem = new DRC_ITEM( DRCE_NETCLASS_TRACKWIDTH );

        m_msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; %s netclass %s)" ),
                      MessageTextFromValue( m_units, bds.m_TrackMinWidth, true ),
                      nc->GetName(),
                      MessageTextFromValue( m_units, nc->GetTrackWidth(), true ) );

        drcItem->SetErrorMessage( m_msg );
        HandleMarker( new MARKER_PCB( drcItem, wxPoint() ) );
        ret = false;
    }

    if( nc->GetViaDiameter() < bds.m_ViasMinSize )
    {
        DRC_ITEM* drcItem = new DRC_ITEM( DRCE_NETCLASS_VIASIZE );

        m_msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; %s netclass %s)" ),
                      MessageTextFromValue( m_units, bds.m_ViasMinSize, true ),
                      nc->GetName(),
                      MessageTextFromValue( m_units, nc->GetViaDiameter(), true ) );

        drcItem->SetErrorMessage( m_msg );
        HandleMarker( new MARKER_PCB( drcItem, wxPoint() ) );
        ret = false;
    }

    if( nc->GetViaDrill() < bds.m_MinThroughDrill )
    {
        DRC_ITEM* drcItem = new DRC_ITEM( DRCE_NETCLASS_VIADRILLSIZE );

        m_msg.Printf( drcItem->GetErrorText() + _( " (board min through hole %s; %s netclass %s)" ),
                      MessageTextFromValue( m_units, bds.m_MinThroughDrill, true ),
                      nc->GetName(),
                      MessageTextFromValue( m_units, nc->GetViaDrill(), true ) );

        drcItem->SetErrorMessage( m_msg );
        HandleMarker( new MARKER_PCB( drcItem, wxPoint() ) );
        ret = false;
    }

    int ncViaAnnulus = ( nc->GetViaDiameter() - nc->GetViaDrill() ) / 2;

    if( ncViaAnnulus < bds.m_ViasMinAnnulus )
    {
        DRC_ITEM* drcItem = new DRC_ITEM( DRCE_NETCLASS_VIAANNULUS );

        m_msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; %s netclass %s)" ),
                      MessageTextFromValue( m_units, bds.m_ViasMinAnnulus, true ),
                      nc->GetName(),
                      MessageTextFromValue( m_units, ncViaAnnulus, true ) );

        drcItem->SetErrorMessage( m_msg );
        HandleMarker( new MARKER_PCB( drcItem, wxPoint() ) );
        ret = false;
    }

    if( nc->GetuViaDiameter() < bds.m_MicroViasMinSize )
    {
        DRC_ITEM* drcItem = new DRC_ITEM( DRCE_NETCLASS_uVIASIZE );

        m_msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; %s netclass %s)" ),
                      MessageTextFromValue( m_units, bds.m_MicroViasMinSize, true ),
                      nc->GetName(),
                      MessageTextFromValue( m_units, nc->GetuViaDiameter(), true ) );

        drcItem->SetErrorMessage( m_msg );
        HandleMarker( new MARKER_PCB( drcItem, wxPoint() ) );
        ret = false;
    }

    if( nc->GetuViaDrill() < bds.m_MicroViasMinDrill )
    {
        DRC_ITEM* drcItem = new DRC_ITEM( DRCE_NETCLASS_uVIADRILLSIZE );

        m_msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; %s netclass %s)" ),
                      MessageTextFromValue( m_units, bds.m_MicroViasMinDrill, true ),
                      nc->GetName(),
                      MessageTextFromValue( m_units, nc->GetuViaDrill(), true ) );

        drcItem->SetErrorMessage( m_msg );
        HandleMarker( new MARKER_PCB( drcItem, wxPoint() ) );
        ret = false;
    }

    return ret;
}


