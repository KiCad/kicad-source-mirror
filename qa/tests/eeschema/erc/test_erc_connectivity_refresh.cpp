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
#include <schematic_utils/schematic_file_util.h>

#include <advanced_config.h>
#include <erc/erc.h>
#include <erc/erc_settings.h>
#include <locale_io.h>
#include <sch_label.h>
#include <sch_marker.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <scoped_set_reset.h>


BOOST_AUTO_TEST_CASE( ERCHeadlessRunRefreshesJobTextOverrides )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool useEngine : { false, true } )
    {
        BOOST_TEST_CONTEXT( "engine=" << useEngine )
        {
            enabled = useEngine;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "issue24201/issue24201", schematic );
            SCH_LABEL_BASE* label = nullptr;

            for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
            {
                for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
                    label = static_cast<SCH_LABEL_BASE*>( item );
            }

            BOOST_REQUIRE( label );
            const wxString original = label->GetText();
            label->SetText( "${ERC_JOB_PORT}" );
            schematic->Project().ApplyTextVars( { { "ERC_JOB_PORT", original } } );

            for( auto& [code, severity] : schematic->ErcSettings().m_ERCSeverities )
                severity = code == ERCE_HIERACHICAL_LABEL ? RPT_SEVERITY_ERROR : RPT_SEVERITY_IGNORE;

            schematic->RebuildConnectivity();
            const auto countMarkers = [&]()
            {
                size_t count = 0;

                for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
                {
                    for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                    {
                        auto* marker = static_cast<SCH_MARKER*>( item );
                        BOOST_CHECK_EQUAL( marker->GetRCItem()->GetErrorCode(), ERCE_HIERACHICAL_LABEL );
                        ++count;
                    }
                }

                return count;
            };
            ERC_TESTER tester( schematic.get() );
            tester.RunTests( nullptr, nullptr, nullptr, &schematic->Project(), nullptr );
            BOOST_REQUIRE_EQUAL( countMarkers(), 0 );
            schematic->Project().ApplyTextVars( { { "ERC_JOB_PORT", "ERC_CHANGED_PORT" } } );
            tester.RunTests( nullptr, nullptr, nullptr, &schematic->Project(), nullptr );
            BOOST_CHECK_EQUAL( countMarkers(), 2 );
            tester.RunTests( nullptr, nullptr, nullptr, &schematic->Project(), nullptr );
            BOOST_CHECK_EQUAL( countMarkers(), 2 );

            for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
            {
                for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                    static_cast<SCH_MARKER*>( item )->SetExcluded( true, "Retained job exclusion" );
            }

            tester.RunTests( nullptr, nullptr, nullptr, &schematic->Project(), nullptr );
            BOOST_CHECK_EQUAL( countMarkers(), 2 );

            for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
            {
                for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                {
                    auto* marker = static_cast<SCH_MARKER*>( item );
                    BOOST_CHECK( marker->IsExcluded() );
                    BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Retained job exclusion" ) );
                    marker->SetExcluded( false );
                }
            }

            schematic->Project().ApplyTextVars( { { "ERC_JOB_PORT", original } } );
            tester.RunTests( nullptr, nullptr, nullptr, &schematic->Project(), nullptr );
            BOOST_CHECK_EQUAL( countMarkers(), 0 );
        }
    }
}
