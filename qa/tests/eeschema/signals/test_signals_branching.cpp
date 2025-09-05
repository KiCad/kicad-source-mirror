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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>
#include <connection_graph.h>
#include <schematic.h>
#include <sch_symbol.h>
#include <locale_io.h>
#include <settings/settings_manager.h>

#include <set>
#include <algorithm>
#include <vector>

struct SIGNALS_BRANCH_TEST_FIXTURE
{
    SIGNALS_BRANCH_TEST_FIXTURE() : m_settingsManager( true ) {}
    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};

static void loadFixture( const wxString& name, std::unique_ptr<SCHEMATIC>& schematic,
                         SETTINGS_MANAGER& settings )
{
    settings.LoadProject( wxString() );
    KI_TEST::LoadSchematic( settings, name, schematic );
    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
}

enum class BRANCH_EXTRA_CHECK
{
    NONE,
    AVOID_GND,        // Ensure no net named GND appears in the grouped signal nets
    NET_TAIL_EQUAL    // For 2‑net signals ensure final character matches (naming consistency)
};

struct BRANCH_TEST_PARAM
{
    const char*        fixtureName;
    int                expectedSignalCount;
    int                expectedMaxNets;
    BRANCH_EXTRA_CHECK extra;    // Additional per‑fixture validations
};

// Table describing the four core branching scenarios previously expressed as individual tests.
static const BRANCH_TEST_PARAM BRANCH_PARAMS[] = {
    { "signals_branching_longer",     1, 4, BRANCH_EXTRA_CHECK::NONE },
    { "signals_branching_no_power",   2, 4, BRANCH_EXTRA_CHECK::AVOID_GND },
    { "signals_branching_named",      1, 2, BRANCH_EXTRA_CHECK::NONE },
    { "signals_branching_named2",     2, 2, BRANCH_EXTRA_CHECK::NET_TAIL_EQUAL }
};

BOOST_FIXTURE_TEST_CASE( SignalBuilder_BranchingVariants, SIGNALS_BRANCH_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    for( const BRANCH_TEST_PARAM& p : BRANCH_PARAMS )
    {
        BOOST_TEST_CONTEXT( "fixture=" << p.fixtureName )
        {
            loadFixture( wxString( p.fixtureName ), m_schematic, m_settingsManager );

            size_t count = 0;
            size_t maxNets = 0;

            // Use potential signals (auto-inferred, not yet user-created).
            for( const auto& sig : m_schematic->ConnectionGraph()->GetPotentialSignals() )
            {
                if( !sig )
                    continue;

                ++count;
                maxNets = std::max( maxNets, sig->GetNets().size() );

                if( p.extra == BRANCH_EXTRA_CHECK::AVOID_GND )
                {
                    for( const wxString& net : sig->GetNets() )
                        BOOST_CHECK_NE( net, wxS( "GND" ) );
                }
                else if( p.extra == BRANCH_EXTRA_CHECK::NET_TAIL_EQUAL && sig->GetNets().size() == 2 )
                {
                    const auto& it1 = *sig->GetNets().begin();
                    const auto& it2 = *std::next( sig->GetNets().begin() );
                    int tail1 = it1.Last();
                    int tail2 = it2.Last();
                    BOOST_CHECK_EQUAL( tail1, tail2 );
                }
            }

            BOOST_CHECK_EQUAL( (int) count, p.expectedSignalCount );
            BOOST_CHECK_EQUAL( (int) maxNets, p.expectedMaxNets );
        }
    }
}

// EOF
