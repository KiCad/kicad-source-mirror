/**
 * @file drc_marker_functions.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <drc/courtyard_overlap.h>

#include <class_module.h>
#include <tools/drc.h>

#include <drc/drc_marker_factory.h>


/**
 * Flag to enable courtyard DRC debug tracing.
 *
 * Use "KICAD_DRC_COURTYARD" to enable.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* DRC_COURTYARD_TRACE = wxT( "KICAD_DRC_COURTYARD" );


DRC_COURTYARD_OVERLAP::DRC_COURTYARD_OVERLAP(
        const DRC_MARKER_FACTORY& aMarkerFactory, MARKER_HANDLER aMarkerHandler )
        : DRC_PROVIDER( aMarkerFactory, aMarkerHandler )
{
}


bool DRC_COURTYARD_OVERLAP::RunDRC( BOARD& aBoard ) const
{
    wxLogTrace( DRC_COURTYARD_TRACE, "Running DRC: Courtyard" );

    // Detects missing (or malformed) footprint courtyard,
    // and for footprint with courtyard, courtyards overlap.
    wxString msg;
    bool     success = true;

    const DRC_MARKER_FACTORY& marker_factory = GetMarkerFactory();

    // Update courtyard polygons, and test for missing courtyard definition:
    for( auto footprint : aBoard.Modules() )
    {
        wxPoint pos = footprint->GetPosition();
        bool    is_ok = footprint->BuildPolyCourtyard();

        if( !is_ok && aBoard.GetDesignSettings().m_ProhibitOverlappingCourtyards )
        {
            auto marker = std::unique_ptr<MARKER_PCB>( marker_factory.NewMarker(
                    pos, footprint, DRCE_MALFORMED_COURTYARD_IN_FOOTPRINT ) );
            HandleMarker( std::move( marker ) );
            success = false;
        }

        if( !aBoard.GetDesignSettings().m_RequireCourtyards )
            continue;

        if( footprint->GetPolyCourtyardFront().OutlineCount() == 0
                && footprint->GetPolyCourtyardBack().OutlineCount() == 0 && is_ok )
        {
            auto marker = std::unique_ptr<MARKER_PCB>( marker_factory.NewMarker(
                    pos, footprint, DRCE_MISSING_COURTYARD_IN_FOOTPRINT ) );
            HandleMarker( std::move( marker ) );
            success = false;
        }
    }

    if( !aBoard.GetDesignSettings().m_ProhibitOverlappingCourtyards )
        return success;

    wxLogTrace( DRC_COURTYARD_TRACE, "Checking for courtyard overlap" );

    // Now test for overlapping on top layer:
    SHAPE_POLY_SET courtyard; // temporary storage of the courtyard of current footprint

    for( auto it1 = aBoard.Modules().begin(); it1 != aBoard.Modules().end(); it1++ )
    {
        auto footprint = *it1;

        if( footprint->GetPolyCourtyardFront().OutlineCount() == 0 )
            continue; // No courtyard defined

        for( auto it2 = it1 + 1; it2 != aBoard.Modules().end(); it2++ )
        {
            auto candidate = *it2;

            if( candidate->GetPolyCourtyardFront().OutlineCount() == 0 )
                continue; // No courtyard defined

            courtyard.RemoveAllContours();
            courtyard.Append( footprint->GetPolyCourtyardFront() );

            // Build the common area between footprint and the candidate:
            courtyard.BooleanIntersection(
                    candidate->GetPolyCourtyardFront(), SHAPE_POLY_SET::PM_FAST );

            // If no overlap, courtyard is empty (no common area).
            // Therefore if a common polygon exists, this is a DRC error
            if( courtyard.OutlineCount() )
            {
                //Overlap between footprint and candidate
                VECTOR2I& pos = courtyard.Vertex( 0, 0, -1 );
                auto      marker = std::unique_ptr<MARKER_PCB>(
                        marker_factory.NewMarker( wxPoint( pos.x, pos.y ), footprint, candidate,
                                DRCE_OVERLAPPING_FOOTPRINTS ) );
                HandleMarker( std::move( marker ) );
                success = false;
            }
        }
    }

    // Test for overlapping on bottom layer:
    for( auto it1 = aBoard.Modules().begin(); it1 != aBoard.Modules().end(); it1++ )
    {
        auto footprint = *it1;

        if( footprint->GetPolyCourtyardBack().OutlineCount() == 0 )
            continue; // No courtyard defined

        for( auto it2 = it1 + 1; it2 != aBoard.Modules().end(); it2++ )
        {
            auto candidate = *it2;

            if( candidate->GetPolyCourtyardBack().OutlineCount() == 0 )
                continue; // No courtyard defined

            courtyard.RemoveAllContours();
            courtyard.Append( footprint->GetPolyCourtyardBack() );

            // Build the common area between footprint and the candidate:
            courtyard.BooleanIntersection(
                    candidate->GetPolyCourtyardBack(), SHAPE_POLY_SET::PM_FAST );

            // If no overlap, courtyard is empty (no common area).
            // Therefore if a common polygon exists, this is a DRC error
            if( courtyard.OutlineCount() )
            {
                //Overlap between footprint and candidate
                VECTOR2I& pos = courtyard.Vertex( 0, 0, -1 );
                auto      marker = std::unique_ptr<MARKER_PCB>(
                        marker_factory.NewMarker( wxPoint( pos.x, pos.y ), footprint, candidate,
                                DRCE_OVERLAPPING_FOOTPRINTS ) );
                HandleMarker( std::move( marker ) );
                success = false;
            }
        }
    }

    return success;
}
