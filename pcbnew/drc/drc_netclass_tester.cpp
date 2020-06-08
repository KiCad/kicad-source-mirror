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
    NETCLASSES& netclasses = m_board->GetDesignSettings().GetNetClasses();

    success &= checkNetClass( netclasses.GetDefault() );

    for( NETCLASSES::const_iterator i = netclasses.begin();  i != netclasses.end();  ++i )
        success &= checkNetClass( i->second );

    return success;
}


bool DRC_NETCLASS_TESTER::checkNetClass( const NETCLASSPTR& nc )
{
    const BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    if( nc->GetClearance() < bds.m_MinClearance )
        return false;

    if( nc->GetTrackWidth() < bds.m_TrackMinWidth )
        return false;

    if( nc->GetViaDiameter() < bds.m_ViasMinSize )
        return false;

    if( nc->GetViaDrill() < bds.m_MinThroughDrill )
        return false;

    int ncViaAnnulus = ( nc->GetViaDiameter() - nc->GetViaDrill() ) / 2;

    if( ncViaAnnulus < bds.m_ViasMinAnnulus )
        return false;

    if( nc->GetuViaDiameter() < bds.m_MicroViasMinSize )
        return false;

    if( nc->GetuViaDrill() < bds.m_MicroViasMinDrill )
        return false;

    return true;
}


