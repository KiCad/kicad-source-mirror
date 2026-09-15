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
#include <locale_io.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <scoped_set_reset.h>
#include <settings/settings_manager.h>

#include <map>


// Members of different buses joined by breakout wires in subsheets reach the parent buses, so R1
// shares nets with R3 (issue 15792)
BOOST_AUTO_TEST_CASE( Issue15792BusBreakoutsJoinAcrossHierarchy )
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
            KI_TEST::LoadSchematic( settings, "issue15792/bustests", schematic );
            schematic->RebuildConnectivity();
            std::map<wxString, wxString> nets;

            for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
            {
                for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
                {
                    auto* symbol = static_cast<SCH_SYMBOL*>( item );
                    const wxString ref = symbol->GetRef( &path );

                    if( ref != wxS( "R1" ) && ref != wxS( "R3" ) )
                        continue;

                    for( SCH_PIN* pin : symbol->GetPins( &path ) )
                    {
                        nets[ref + wxS( "." ) + pin->GetNumber()] =
                                pin->GetConnectionName( &path ).value_or( wxString() );
                    }
                }
            }

            for( const wxString& key : { wxS( "R1.1" ), wxS( "R1.2" ), wxS( "R3.1" ), wxS( "R3.2" ) } )
            {
                BOOST_REQUIRE_MESSAGE( nets.contains( key ), "Missing pin " << key );
                BOOST_CHECK_MESSAGE( !nets[key].IsEmpty(), "Pin " << key << " has no net" );
            }

            BOOST_CHECK_EQUAL( nets[wxS( "R1.1" )], nets[wxS( "R3.1" )] );
            BOOST_CHECK_EQUAL( nets[wxS( "R1.2" )], nets[wxS( "R3.2" )] );
            BOOST_CHECK_NE( nets[wxS( "R1.1" )], nets[wxS( "R1.2" )] );
        }
    }
}
