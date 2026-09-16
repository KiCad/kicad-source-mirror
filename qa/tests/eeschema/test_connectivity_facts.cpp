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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>
#include <connectivity/conn_facade.h>
#include <connectivity/conn_facts.h>
#include <locale_io.h>
#include <project.h>
#include <schematic.h>
#include <sch_field.h>
#include <sch_label.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <settings/settings_manager.h>
#include <string_utils.h>

#include <algorithm>

using namespace SCH_CONNECTIVITY;

BOOST_AUTO_TEST_SUITE( ConnectivityFacts )

BOOST_AUTO_TEST_CASE( InputTextDoesNotReadThePreviousNet )
{
    LOCALE_IO                  locale;
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, wxS( "issue18346" ), schematic );
    const SCH_SHEET_PATH path = schematic->Hierarchy().front();
    SCH_SCREEN&          screen = *path.LastScreen();
    auto*                label = dynamic_cast<SCH_LABEL_BASE*>(
            screen.GetConnectivityItem( KIID( "aca19205-a0bf-4ee3-bb76-dee6f12714b1" ) ) );
    BOOST_REQUIRE( label );
    label->Move( VECTOR2I( 100000000, 100000000 ) );

    for( const wxString& previous : { wxString( "old_net" ), wxString( "another_net" ) } )
    {
        label->SetText( previous );
        schematic->RebuildConnectivity();
        const auto previousName = label->GetConnectionName( &path, false, true );
        BOOST_REQUIRE( previousName );
        BOOST_CHECK_EQUAL( *previousName, previous );

        for( const wxString& token : { wxString( "NET_NAME" ), wxString( "SHORT_NET_NAME" ), wxString( "NET_CLASS" ) } )
        {
            const wxString expression = wxS( "${" ) + token + wxS( "}" );
            schematic->Project().GetTextVars()[wxS( "S0_OUTPUT" )] = expression;

            for( const wxString& input : { expression, wxString( "${S0_OUTPUT}" ) } )
            {
                label->SetText( wxS( "input_" ) + input );
                const INSTANCE_FACTS instance = ExtractInstanceFacts( ExtractScreenFacts( screen ), screen, path );
                auto                 text = std::find_if( instance.items.begin(), instance.items.end(),
                                                          [&]( const ITEM_TEXT_FACT& aText )
                                                          {
                                              return aText.id == label->m_Uuid;
                                          } );
                BOOST_REQUIRE( text != instance.items.end() );
                BOOST_CHECK_EQUAL( text->name, wxString( "input_" ) );
            }
        }

        // Legacy is the reference for both engines; the hold keeps the engine row across the text bump
        PUBLICATION_HOLD hold( schematic->Connectivity() );
        label->SetText( wxS( "${SHORT_NET_NAME}" ) );
        BOOST_CHECK_EQUAL( label->GetShownText( &path, FOR_NETNAME ), previous );
    }
}

BOOST_AUTO_TEST_CASE( NestedLabelFieldsUseTheExplicitPath )
{
    LOCALE_IO                  locale;
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, wxS( "issue23962/issue23962" ), schematic );
    BOOST_REQUIRE_GT( schematic->Hierarchy().size(), 1 );
    size_t checked = 0;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        SCH_SCREEN& screen = *path.LastScreen();

        for( SCH_ITEM* item : screen.Items().OfType( SCH_DIRECTIVE_LABEL_T ) )
        {
            auto* label = static_cast<SCH_LABEL_BASE*>( item );

            if( label->GetFields().empty() )
                continue;

            SCH_FIELD& field = label->GetFields().front();
            field.SetText( wxS( "scope_${SHEETNAME}" ) );
            label->SetText( wxS( "${" ) + field.GetName() + wxS( "}" ) );
            const wxString     expected = EscapeString( field.GetShownText( &path, FOR_NETNAME ), CTX_NETNAME );
            const SCREEN_FACTS facts = ExtractScreenFacts( screen );

            for( const SCH_SHEET_PATH& current : schematic->Hierarchy() )
            {
                schematic->SetCurrentSheet( current );
                const INSTANCE_FACTS instance = ExtractInstanceFacts( facts, screen, path );
                auto                 text = std::find_if( instance.items.begin(), instance.items.end(),
                                                          [&]( const ITEM_TEXT_FACT& aText )
                                                          {
                                              return aText.id == label->m_Uuid;
                                          } );
                BOOST_REQUIRE( text != instance.items.end() );
                BOOST_CHECK_EQUAL( text->name, expected );
                ++checked;
            }
        }
    }

    BOOST_CHECK_GT( checked, 0 );
}

BOOST_AUTO_TEST_SUITE_END()
