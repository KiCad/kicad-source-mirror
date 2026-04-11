/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file drc_test_utils.h
 * General utilities for DRC-related PCB tests
 */

#ifndef QA_PCBNEW_DRC_TEST_UTILS__H
#define QA_PCBNEW_DRC_TEST_UTILS__H

#include <iostream>

#include <pcb_marker.h>

/**
 * Define a stream function for logging #PCB_MARKER test assertions.
 *
 * This has to be in the same namespace as #PCB_MARKER
 */
std::ostream& boost_test_print_type( std::ostream& os, const PCB_MARKER& aMarker );


namespace KI_TEST
{
/**
 * Predicate for testing the type of a DRC marker
 * @param  aMarker      the marker to test
 * @param  aErrorCode   the expected DRC violation code
 * @return              true if the marker has this code
 */
bool IsDrcMarkerOfType( const PCB_MARKER& aMarker, int aErrorCode );

} // namespace KI_TEST

#endif // QA_PCBNEW_DRC_TEST_UTILS__H