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


#include <drc/drc_drilled_hole_tester.h>

#include <class_module.h>
#include <drc/drc.h>

#include <widgets/ui_common.h>


DRC_DRILLED_HOLE_TESTER::DRC_DRILLED_HOLE_TESTER( MARKER_HANDLER aMarkerHandler ) :
        DRC_TEST_PROVIDER( std::move( aMarkerHandler ) ),
        m_units( EDA_UNITS::MILLIMETRES ),
        m_board( nullptr ),
        m_largestRadius( 0 )
{
}


bool DRC_DRILLED_HOLE_TESTER::RunDRC( EDA_UNITS aUnits, BOARD& aBoard )
{
    bool success = true;

    // Test drilled holes to minimize drill bit breakage.
    //
    // Check pad & std. via circular holes for hole-to-hole-min (non-circular holes are milled)
    // Check pad & std. via holes for via-min-drill (minimum hole classification)
    // Check uvia holes for uvia-min-drill (laser drill classification)

    m_units = aUnits;
    m_board = &aBoard;
    m_holes.clear();
    m_largestRadius = 0;

    for( MODULE* mod : aBoard.Modules() )
    {
        for( D_PAD* pad : mod->Pads( ) )
            success &= checkPad( pad );
    }

    for( TRACK* track : aBoard.Tracks() )
    {
        VIA* via = dynamic_cast<VIA*>( track );

        if( via )
        {
            if( via->GetViaType() == VIATYPE::MICROVIA )
                success &= checkMicroVia( via );
            else
                success &= checkVia( via );
        }
    }

    success &= checkHoles();

    return success;
}


bool DRC_DRILLED_HOLE_TESTER::checkPad( D_PAD* aPad )
{
    bool                   success = true;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    int holeSize = std::min( aPad->GetDrillSize().x, aPad->GetDrillSize().y );

    if( holeSize == 0 )
        return true;

    if( !bds.Ignore( DRCE_TOO_SMALL_DRILL ) )
    {
        int       minHole = bds.m_MinThroughDrill;
        wxString  minHoleSource = _( "board minimum" );
        DRC_RULE* rule = GetRule( aPad, nullptr, HOLE_CONSTRAINT );

        if( rule )
        {
            minHole = rule->m_MinHole;
            minHoleSource = wxString::Format( _( "'%s' rule" ), rule->m_Name );
        }

        if( holeSize < minHole )
        {
            DRC_ITEM* drcItem = DRC_ITEM::Create( DRCE_TOO_SMALL_DRILL );

            m_msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                          minHoleSource,
                          MessageTextFromValue( m_units, minHole, true ),
                          MessageTextFromValue( m_units, holeSize, true ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( aPad );

            HandleMarker( new MARKER_PCB( drcItem, aPad->GetPosition() ) );
            success = false;
        }
    }

    if( !bds.Ignore( DRCE_DRILLED_HOLES_TOO_CLOSE ) && bds.m_HoleToHoleMin != 0 )
    {
        if( aPad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
            addHole( aPad->GetPosition(), aPad->GetDrillSize().x / 2, aPad );
    }

    return success;
}

bool DRC_DRILLED_HOLE_TESTER::checkVia( VIA* via )
{
    bool                   success = true;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    if( !bds.Ignore( DRCE_TOO_SMALL_DRILL ) )
    {
        int       minHole = bds.m_MinThroughDrill;
        wxString  minHoleSource = _( "board minimum" );
        DRC_RULE* rule = GetRule( via, nullptr, HOLE_CONSTRAINT );

        if( rule )
        {
            minHole = rule->m_MinHole;
            minHoleSource = wxString::Format( _( "'%s' rule" ), rule->m_Name );
        }

        if( via->GetDrillValue() < minHole )
        {
            DRC_ITEM* drcItem = DRC_ITEM::Create( DRCE_TOO_SMALL_DRILL );

            m_msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                          minHoleSource,
                          MessageTextFromValue( m_units, minHole, true ),
                          MessageTextFromValue( m_units, via->GetDrillValue(), true ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( via );

            HandleMarker( new MARKER_PCB( drcItem, via->GetPosition() ) );
            success = false;
        }
    }

    if( !bds.Ignore( DRCE_DRILLED_HOLES_TOO_CLOSE ) && bds.m_HoleToHoleMin != 0 )
    {
        addHole( via->GetPosition(), via->GetDrillValue() / 2, via );
    }

    return success;
}


bool DRC_DRILLED_HOLE_TESTER::checkMicroVia( VIA* via )
{
    bool                   success = true;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    if( !bds.Ignore( DRCE_TOO_SMALL_MICROVIA_DRILL ) )
    {
        int       minHole = bds.m_MicroViasMinDrill;
        wxString  minHoleSource = _( "board minimum" );
        DRC_RULE* rule = GetRule( via, nullptr, HOLE_CONSTRAINT );

        if( rule )
        {
            minHole = rule->m_MinHole;
            minHoleSource = wxString::Format( _( "'%s' rule" ), rule->m_Name );
        }

        if(  via->GetDrillValue() < minHole )
        {
            DRC_ITEM* drcItem = DRC_ITEM::Create( DRCE_TOO_SMALL_MICROVIA_DRILL );

            m_msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                          minHoleSource,
                          MessageTextFromValue( m_units, minHole, true ),
                          MessageTextFromValue( m_units, via->GetDrillValue(), true ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( via );

            HandleMarker( new MARKER_PCB( drcItem, via->GetPosition() ) );
            success = false;
        }
    }

    return success;
}


void DRC_DRILLED_HOLE_TESTER::addHole( const wxPoint& aLocation, int aRadius, BOARD_ITEM* aOwner )
{
    DRILLED_HOLE hole;

    hole.m_location = aLocation;
    hole.m_drillRadius = aRadius;
    hole.m_owner = aOwner;

    m_largestRadius = std::max( m_largestRadius, aRadius );

    m_holes.push_back( hole );
}


bool DRC_DRILLED_HOLE_TESTER::checkHoles()
{
    bool                   success = true;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // No need to check if we're ignoring DRCE_DRILLED_HOLES_TOO_CLOSE; if we are then we
    // won't have collected any holes to test.

    // Sort holes by X for performance.  In the nested iteration we then need to look at
    // following holes only while they are within the refHole's neighborhood as defined by
    // the refHole radius + the minimum hole-to-hole clearance + the largest radius any of
    // the following holes can have.
    std::sort( m_holes.begin(), m_holes.end(),
               []( const DRILLED_HOLE& a, const DRILLED_HOLE& b )
               {
                   if( a.m_location.x == b.m_location.x )
                       return a.m_location.y < b.m_location.y;
                   else
                       return a.m_location.x < b.m_location.x;
               } );

    for( size_t ii = 0; ii < m_holes.size(); ++ii )
    {
        const DRILLED_HOLE& refHole = m_holes[ ii ];
        int neighborhood = refHole.m_drillRadius + bds.m_HoleToHoleMin + m_largestRadius;

        for( size_t jj = ii + 1; jj < m_holes.size(); ++jj )
        {
            const DRILLED_HOLE& checkHole = m_holes[ jj ];

            if( refHole.m_location.x + neighborhood < checkHole.m_location.x )
                break;

            // Holes with identical locations are allowable
            if( checkHole.m_location == refHole.m_location )
                continue;

            int actual = KiROUND( GetLineLength( checkHole.m_location, refHole.m_location ) );
            actual = std::max( 0, actual - checkHole.m_drillRadius - refHole.m_drillRadius );

            if( actual < bds.m_HoleToHoleMin )
            {
                DRC_ITEM* drcItem = DRC_ITEM::Create( DRCE_DRILLED_HOLES_TOO_CLOSE );

                m_msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; actual %s)" ),
                              MessageTextFromValue( m_units, bds.m_HoleToHoleMin, true ),
                              MessageTextFromValue( m_units, actual, true ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( refHole.m_owner, checkHole.m_owner );

                HandleMarker( new MARKER_PCB( drcItem, refHole.m_location ) );
                success = false;
            }
        }
    }

    return success;
}
