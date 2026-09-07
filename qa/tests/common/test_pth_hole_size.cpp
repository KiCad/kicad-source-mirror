/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <base_units.h>
#include <pth_hole_size.h>

/**
 * Tests for the plated through hole size rules, based on the worked example in the KiCad
 * libraries wiki page "THT hole sizes".
 */
BOOST_AUTO_TEST_SUITE( PthHoleSize )

BOOST_AUTO_TEST_CASE( Ipc2222RectangularLead )
{
    const PTH_HOLE_SIZE_STANDARD* standard = FindPthHoleSizeStandard( "IPC-2222B" );

    BOOST_REQUIRE( standard );
    BOOST_CHECK_EQUAL( standard->GetLevelCount(), 3 );

    // Recom RBE module lead: 0.51 x 0.25 mm nominal with tolerances, so the diagonal ranges
    // from sqrt(0.46^2 + 0.20^2) to sqrt(0.61^2 + 0.30^2).
    PTH_LEAD_DEF lead;
    lead.m_shape = PTH_LEAD_SHAPE::RECTANGULAR;
    lead.m_minX = pcbIUScale.mmToIU( 0.46 );
    lead.m_maxX = pcbIUScale.mmToIU( 0.61 );
    lead.m_minY = pcbIUScale.mmToIU( 0.20 );
    lead.m_maxY = pcbIUScale.mmToIU( 0.30 );

    // Level B (moderate density), rounding to 0.1 mm.
    PTH_HOLE_SIZE_RESULT result = standard->ComputeHoleSize( 1, lead );

    BOOST_CHECK_CLOSE( pcbIUScale.IUTomm( result.m_leadMin ), 0.5016, 0.1 );
    BOOST_CHECK_CLOSE( pcbIUScale.IUTomm( result.m_leadMax ), 0.6798, 0.1 );
    BOOST_CHECK_CLOSE( pcbIUScale.IUTomm( result.m_holeMin ), 0.8798, 0.1 );
    BOOST_CHECK_CLOSE( pcbIUScale.IUTomm( result.m_holeMax ), 1.2016, 0.1 );

    const int hole = ComputeRecommendedPthHoleSize( result, pcbIUScale.mmToIU( 0.1 ), PTH_HOLE_ROUNDING::NEAREST );
    BOOST_CHECK_CLOSE( pcbIUScale.IUTomm( hole ), 1.0, 0.1 );
}

BOOST_AUTO_TEST_CASE( Ipc2222SquareLead )
{
    const PTH_HOLE_SIZE_STANDARD* standard = FindPthHoleSizeStandard( "IPC-2222B" );

    BOOST_REQUIRE( standard );

    // A 0.64 mm nominal 0.1" header pin; the diagonal is the size that must fit in the hole.
    PTH_LEAD_DEF lead;
    lead.m_shape = PTH_LEAD_SHAPE::SQUARE;
    lead.m_minX = pcbIUScale.mmToIU( 0.60 );
    lead.m_maxX = pcbIUScale.mmToIU( 0.65 );

    // Level B, rounding to 0.05 mm.
    PTH_HOLE_SIZE_RESULT result = standard->ComputeHoleSize( 1, lead );

    BOOST_CHECK_CLOSE( pcbIUScale.IUTomm( result.m_leadMin ), 0.8485, 0.1 );
    BOOST_CHECK_CLOSE( pcbIUScale.IUTomm( result.m_leadMax ), 0.9192, 0.1 );
    BOOST_CHECK_CLOSE( pcbIUScale.IUTomm( result.m_holeMin ), 1.1192, 0.1 );
    BOOST_CHECK_CLOSE( pcbIUScale.IUTomm( result.m_holeMax ), 1.5485, 0.1 );

    const int hole = ComputeRecommendedPthHoleSize( result, pcbIUScale.mmToIU( 0.05 ), PTH_HOLE_ROUNDING::NEAREST );
    BOOST_CHECK_CLOSE( pcbIUScale.IUTomm( hole ), 1.35, 0.1 );
}

BOOST_AUTO_TEST_SUITE_END()
