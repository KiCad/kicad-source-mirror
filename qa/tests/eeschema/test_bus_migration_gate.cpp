/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <dialog_migrate_buses.h>
#include <sch_file_versions.h>

// First s-expression schematic version (KiCad 6.0); older files predate migration enforcement.
static constexpr int FIRST_SEXPR_VERSION = 20200310;

BOOST_AUTO_TEST_SUITE( BusMigrationGate )

// A legacy .sch file (version 0) with conflicting bus labels must still prompt.
BOOST_AUTO_TEST_CASE( LegacyFileWithConflictsPrompts )
{
    BOOST_CHECK( DIALOG_MIGRATE_BUSES::ShouldPrompt( 0, 3 ) );
}

// The pre-6.0 boundary is inclusive-exclusive on the first s-expression version.
BOOST_AUTO_TEST_CASE( JustBelowThresholdPrompts )
{
    BOOST_CHECK( DIALOG_MIGRATE_BUSES::ShouldPrompt( FIRST_SEXPR_VERSION - 1, 1 ) );
}

// A 6.0+ file cannot store the ambiguity, so a transient nonzero count must not re-prompt.
BOOST_AUTO_TEST_CASE( FirstSexprVersionDoesNotPrompt )
{
    BOOST_CHECK( !DIALOG_MIGRATE_BUSES::ShouldPrompt( FIRST_SEXPR_VERSION, 3 ) );
}

BOOST_AUTO_TEST_CASE( CurrentVersionDoesNotPrompt )
{
    BOOST_CHECK( !DIALOG_MIGRATE_BUSES::ShouldPrompt( SEXPR_SCHEMATIC_FILE_VERSION, 5 ) );
}

// No conflicts never prompts, regardless of file age.
BOOST_AUTO_TEST_CASE( NoConflictsNeverPrompts )
{
    BOOST_CHECK( !DIALOG_MIGRATE_BUSES::ShouldPrompt( 0, 0 ) );
    BOOST_CHECK( !DIALOG_MIGRATE_BUSES::ShouldPrompt( SEXPR_SCHEMATIC_FILE_VERSION, 0 ) );
}

BOOST_AUTO_TEST_SUITE_END()
