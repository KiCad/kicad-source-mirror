/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <advanced_config.h>
#include <connectivity/conn_facade.h>
#include <sch_marker.h>
#include <sch_field.h>
#include <sch_symbol.h>
#include <map>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <tuple>
#include <schematic.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <erc/erc_exclusion.h>
#include <api/schematic/schematic_rules.pb.h>
#include <json_common.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <scoped_set_reset.h>


struct ERC_TEXT_VAR_FIXTURE
{
    void RefreshTextChecks( bool aRebuild )
    {
        if( aRebuild )
            m_schematic->RebuildConnectivity();
        else
            m_schematic->Connectivity().Recalculate( *m_schematic );

        if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
            m_schematic->Connectivity().PrepareTextChecks( *m_schematic );
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// Verifies that ${ERC_ERROR ...} and ${ERC_WARNING ...} tokens trigger an
// ERC violation regardless of position in the text.  Prior to the fix the
// regex was anchored with `^...$` so only text starting with the token would
// fire, breaking placeholder patterns like "Fill in: ${ERC_ERROR ...}".
BOOST_FIXTURE_TEST_CASE( ERCTextVarIssue24442, ERC_TEXT_VAR_FIXTURE )
{
    LOCALE_IO dummy;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool useEngine : { false, true } )
    {
        BOOST_TEST_CONTEXT( "engine=" << useEngine )
        {
            enabled = useEngine;
            KI_TEST::LoadSchematic( m_settingsManager, "issue24442", m_schematic );

            if( useEngine )
                RefreshTextChecks( true );

            ERC_SETTINGS& settings = m_schematic->ErcSettings();

            // Silence noise from other ERC checks; we care only about generic ERC
            // text-variable violations here.
            for( int ii = 0; ii < ERCE_LAST; ++ii )
                settings.m_ERCSeverities[ii] = RPT_SEVERITY_IGNORE;

            settings.m_ERCSeverities[ERCE_GENERIC_ERROR] = RPT_SEVERITY_ERROR;
            settings.m_ERCSeverities[ERCE_GENERIC_WARNING] = RPT_SEVERITY_WARNING;
            settings.m_ERCSeverities[ERCE_UNRESOLVED_VARIABLE] = RPT_SEVERITY_ERROR;

            ERC_TESTER tester( m_schematic.get() );
            tester.TestTextVars( nullptr );

            SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );
            errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

            int genericErrors = 0;
            int genericWarnings = 0;
            int unresolvedVars = 0;
            std::vector<wxString> errorMessages;
            std::vector<wxString> warningMessages;

            for( int ii = 0; ii < errors.GetCount(); ++ii )
            {
                std::shared_ptr<RC_ITEM> rc = errors.GetItem( ii );

                if( rc->GetErrorCode() == ERCE_GENERIC_ERROR )
                {
                    genericErrors++;
                    errorMessages.push_back( rc->GetErrorMessage( false ) );
                }
                else if( rc->GetErrorCode() == ERCE_GENERIC_WARNING )
                {
                    genericWarnings++;
                    warningMessages.push_back( rc->GetErrorMessage( false ) );
                }
                else if( rc->GetErrorCode() == ERCE_UNRESOLVED_VARIABLE )
                {
                    unresolvedVars++;
                }
            }

            // The fixture schematic contains two ${ERC_ERROR ...} texts (one at start,
            // one mid-string), two ${ERC_WARNING ...} texts (start + middle), and one
            // escaped \${ERC_ERROR ...} that must NOT count.
            BOOST_CHECK_EQUAL( genericErrors, 2 );
            BOOST_CHECK_EQUAL( genericWarnings, 2 );

            // The four matched markers must not double-report as unresolved variables;
            // The escaped literal should also not trigger an unresolved variable warning
            // as that's not likely the user's expectation.
            BOOST_CHECK_EQUAL( unresolvedVars, 0 );

            auto containsMsg =
                    []( const std::vector<wxString>& aList, const wxString& aNeedle )
                    {
                        for( const wxString& msg : aList )
                        {
                            if( msg.Contains( aNeedle ) )
                                return true;
                        }

                        return false;
                    };

            BOOST_CHECK( containsMsg( errorMessages, "start_of_text" ) );
            BOOST_CHECK( containsMsg( errorMessages, "placeholder_text" ) );
            BOOST_CHECK( containsMsg( warningMessages, "this_is_warning" ) );
            BOOST_CHECK( containsMsg( warningMessages, "embedded_warning" ) );
            BOOST_CHECK( !containsMsg( errorMessages, "not_a_real_error" ) );
        }
    }
}


BOOST_FIXTURE_TEST_CASE( ERCTextFieldsUseCapturedSharedSources, ERC_TEXT_VAR_FIXTURE )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, true );
    KI_TEST::LoadSchematic( m_settingsManager, "legacy_hierarchy/legacy_hierarchy", m_schematic );
    std::vector<SCH_SHEET_PATH> paths;

    for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
    {
        if( path.LastScreen()->GetFileName().EndsWith( "ampli_ht.kicad_sch" ) )
            paths.push_back( path );
    }

    BOOST_REQUIRE_EQUAL( paths.size(), 2u );
    SCH_SCREEN& screen = *paths[0].LastScreen();
    BOOST_REQUIRE( &screen == paths[1].LastScreen() );
    SCH_SYMBOL* symbol = nullptr;

    for( SCH_ITEM* item : screen.Items().OfType( SCH_SYMBOL_T ) )
    {
        symbol = static_cast<SCH_SYMBOL*>( item );
        break;
    }

    BOOST_REQUIRE( symbol );
    const wxString name( "Capture assertion" );
    symbol->AddField( SCH_FIELD( symbol, FIELD_T::USER, name ) );
    SCH_FIELD* field = symbol->GetField( name );
    BOOST_REQUIRE( field );
    const VECTOR2I position = symbol->GetPosition();
    auto collect = [&]()
    {
        ERC_TESTER tester( m_schematic.get() );
        tester.TestTextVars( nullptr );
        std::map<KIID_PATH, std::tuple<KIID, int, wxString, VECTOR2I>> found;
        std::vector<SCH_MARKER*> markers;

        for( SCH_ITEM* item : screen.Items().OfType( SCH_MARKER_T ) )
        {
            auto* marker = static_cast<SCH_MARKER*>( item );
            const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

            if( error->GetMainItemID() == field->m_Uuid || error->GetMainItemID() == symbol->m_Uuid )
            {
                BOOST_REQUIRE( error->IsSheetSpecific() );
                BOOST_CHECK( error->MainItemHasSheetPath() );

                if( error->MainItemHasSheetPath() )
                    BOOST_CHECK( error->GetMainItemSheetPath().PathRef() == error->GetSpecificSheetPath().PathRef() );

                BOOST_CHECK( found.emplace( error->GetSpecificSheetPath().PathRef(),
                        std::make_tuple( error->GetMainItemID(), error->GetErrorCode(),
                                error->GetErrorMessage( false ), marker->GetPosition() ) ).second );
            }

            markers.push_back( marker );
        }

        for( SCH_MARKER* marker : markers )
            screen.DeleteItem( marker );

        return found;
    };

    for( int change = 0; change < 2; ++change )
    {
        BOOST_TEST_CONTEXT( "shared field source edit=" << change )
        {
            field->SetText( "${ERC_WARNING Captured}" );
            field->SetPosition( position );
            RefreshTextChecks( true );
            const auto before = collect();
            BOOST_REQUIRE_EQUAL( before.size(), 2u );

            for( const SCH_SHEET_PATH& path : paths )
            {
                BOOST_REQUIRE( before.contains( path.PathRef() ) );
                BOOST_CHECK( std::get<0>( before.at( path.PathRef() ) ) == field->m_Uuid );
                BOOST_CHECK_EQUAL( std::get<1>( before.at( path.PathRef() ) ), ERCE_GENERIC_WARNING );
                m_schematic->SetCurrentSheet( path );
                BOOST_CHECK( collect() == before );
            }

            if( change == 0 )
                field->SetText( "${ConnectivityCaptureMissingVariable}" );
            else
                field->SetPosition( position + VECTOR2I( 25400000, 25400000 ) );

            screen.BumpConnectivityRevision();
            RefreshTextChecks( false );
            const auto after = collect();
            BOOST_REQUIRE_EQUAL( after.size(), 2u );

            for( const SCH_SHEET_PATH& path : paths )
            {
                BOOST_REQUIRE( after.contains( path.PathRef() ) );
                const auto& error = after.at( path.PathRef() );
                BOOST_CHECK( std::get<0>( error ) == ( change == 0 ? symbol->m_Uuid : field->m_Uuid ) );
                BOOST_CHECK_EQUAL( std::get<1>( error ),
                                   change == 0 ? ERCE_UNRESOLVED_VARIABLE : ERCE_GENERIC_WARNING );
                BOOST_CHECK( std::get<3>( error ) == field->GetPosition() );
            }

            BOOST_CHECK( after != before );
            RefreshTextChecks( true );
            BOOST_CHECK( collect() == after );
            enabled = false;
            RefreshTextChecks( true );
            BOOST_CHECK( collect() == after );
            enabled = true;
        }
    }

    auto targetMarkers = [&]()
    {
        std::vector<SCH_MARKER*> result;

        for( SCH_ITEM* item : screen.Items().OfType( SCH_MARKER_T ) )
        {
            auto* marker = static_cast<SCH_MARKER*>( item );
            const KIID& owner = marker->GetRCItem()->GetMainItemID();

            if( owner == field->m_Uuid || owner == symbol->m_Uuid )
                result.push_back( marker );
        }

        return result;
    };
    auto& exclusions = m_schematic->ErcSettings().m_ErcExclusions;

    for( const wxString& raw : { wxString( "${ERC_WARNING Captured}" ), wxString( "${ERC_ERROR Captured}" ),
                                wxString( "${ConnectivityCaptureMissingVariable}" ) } )
    {
        field->SetText( raw );

        for( int format : { 0, 1, 2 } )
        {
            BOOST_TEST_CONTEXT( "text=" << raw << "; exclusion format=" << format )
            {
                const bool legacyString = format == 2;
                auto& legacyExclusions = m_schematic->ErcSettings().m_ErcExclusionsLegacy;
                exclusions.clear();
                legacyExclusions.clear();
                enabled = true;
                RefreshTextChecks( true );
                ERC_TESTER tester( m_schematic.get() );
                tester.TestTextVars( nullptr );
                BOOST_REQUIRE_EQUAL( targetMarkers().size(), 2u );

                for( SCH_MARKER* marker : targetMarkers() )
                {
                    const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                    BOOST_REQUIRE( error->IsSheetSpecific() );

                    if( error->GetSpecificSheetPath().PathRef() != paths[0].PathRef() )
                        continue;

                    error->SetItemsSheetPaths( paths[0] );
                    auto proto = ERC_EXCLUSION::FromMarker( *marker ).ToProto();

                    if( format == 1 )
                        proto.mutable_marker()->clear_main_item_sheet_path();

                    auto exclusion = ERC_EXCLUSION::FromProto( proto );
                    exclusion.SetComment( "Retained text exclusion" );

                    if( legacyString )
                    {
                        BOOST_REQUIRE_GT( proto.marker().items_size(), 0 );
                        const wxString data = wxString::Format( wxS( "%s|%d|%d|%s|%s|%s|%s|" ),
                                error->GetSettingsKey(), marker->GetPosition().x, marker->GetPosition().y,
                                wxString::FromUTF8( proto.marker().items( 0 ).value() ),
                                proto.marker().has_child()
                                        ? wxString::FromUTF8( proto.marker().child().text_value() )
                                        : niluuid.AsString(),
                                error->GetSpecificSheetPath().PathRef().AsString(),
                                error->GetMainItemSheetPath().PathRef().AsString() );
                        legacyExclusions.emplace( data, exclusion.GetComment() );
                    }
                    else
                    {
                        const nlohmann::json saved = exclusion;
                        exclusions.insert( saved.get<ERC_EXCLUSION>() );
                    }
                }

                BOOST_REQUIRE_EQUAL( exclusions.size(), legacyString ? 0u : 1u );
                BOOST_REQUIRE_EQUAL( legacyExclusions.size(), legacyString ? 1u : 0u );
                const auto savedExclusions = exclusions;
                const auto savedLegacyExclusions = legacyExclusions;

                for( SCH_MARKER* marker : targetMarkers() )
                    screen.DeleteItem( marker );

                for( bool backend : { true, false } )
                {
                    BOOST_TEST_CONTEXT( "new engine=" << backend )
                    {
                        enabled = backend;
                        exclusions = savedExclusions;
                        legacyExclusions = savedLegacyExclusions;
                        RefreshTextChecks( true );
                        ERC_TESTER rescan( m_schematic.get() );
                        rescan.TestTextVars( nullptr );
                        m_schematic->ResolveERCExclusionsPostUpdate();
                        BOOST_CHECK( legacyExclusions.empty() );
                        BOOST_CHECK_EQUAL( m_schematic->GetUnresolvedERCExclusionCount(), 0u );
                        BOOST_REQUIRE_EQUAL( targetMarkers().size(), 2u );
                        size_t excluded = 0;

                        for( SCH_MARKER* marker : targetMarkers() )
                        {
                            const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                            const bool expected = error->GetSpecificSheetPath().PathRef() == paths[0].PathRef();
                            BOOST_CHECK_EQUAL( marker->IsExcluded(), expected );

                            if( marker->IsExcluded() )
                            {
                                ++excluded;
                                BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Retained text exclusion" ) );
                            }

                            screen.DeleteItem( marker );
                        }

                        BOOST_CHECK_EQUAL( excluded, 1u );
                    }
                }
            }
        }
    }

    for( bool backend : { true, false } )
    {
        BOOST_TEST_CONTEXT( "retired shared text path backend=" << backend )
        {
            enabled = backend;
            field->SetText( "${ERC_WARNING Retained shared field}" );
            m_schematic->SetCurrentSheet( m_schematic->Hierarchy().front() );
            RefreshTextChecks( true );
            const auto before = collect();
            BOOST_REQUIRE_EQUAL( before.size(), 2u );
            SCH_SHEET_PATH parent = paths[0];
            SCH_SHEET* removed = parent.Last();
            parent.pop_back();
            BOOST_REQUIRE( parent.LastScreen()->CheckIfOnDrawList( removed ) );
            parent.LastScreen()->Remove( removed );
            std::unique_ptr<SCH_SHEET> undoOwnedSheet( removed );
            m_schematic->RefreshHierarchy();
            auto expected = before;
            expected.erase( paths[0].PathRef() );
            BOOST_REQUIRE_EQUAL( expected.size(), 1u );
            BOOST_CHECK( collect() == expected );

            if( backend )
                RefreshTextChecks( false );
            else
                RefreshTextChecks( true );

            BOOST_CHECK( collect() == expected );
            RefreshTextChecks( true );
            BOOST_CHECK( collect() == expected );
            parent.LastScreen()->Append( undoOwnedSheet.release() );
            m_schematic->RefreshHierarchy();

            if( backend )
                RefreshTextChecks( false );
            else
                RefreshTextChecks( true );

            BOOST_CHECK( collect() == before );
            RefreshTextChecks( true );
            BOOST_CHECK( collect() == before );
        }
    }
}
