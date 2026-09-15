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
#include <qa_utils/file_utils.h>
#include <schematic_utils/schematic_file_util.h>
#include <advanced_config.h>
#include <connection_graph.h>
#include <connectivity/conn_facade.h>
#include <netlist_exporters/netlist_exporter_pads.h>
#include <netlist_exporters/netlist_exporter_cadstar.h>
#include <netlist_exporters/netlist_exporter_allegro.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <netlist_exporters/netlist_exporter_spice.h>
#include <richio.h>
#include <wx/dir.h>
#include <boost/mpl/list.hpp>
#include <type_traits>
#include <algorithm>
#include <reporter.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_label.h>
#include <sch_symbol.h>
#include <sch_rule_area.h>
#include <sch_netchain.h>
#include <locale_io.h>
#include <wx/xml/xml.h>
#include <settings/settings_manager.h>
#include <wx/ffile.h>
#include <scoped_set_reset.h>

BOOST_AUTO_TEST_SUITE( ConnectivityExport )

BOOST_AUTO_TEST_CASE( CurrentSheetExportPreservesFullInstancePaths )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    class EXPORTER : public NETLIST_EXPORTER_SPICE
    {
    public:
        using NETLIST_EXPORTER_SPICE::NETLIST_EXPORTER_SPICE;

        SCH_SHEET_LIST SelectedSheets()
        {
            CONNECTIVITY_SCOPE connectivity( *this );
            return BuildSheetList( OPTION_CUR_SHEET_AS_ROOT );
        }
    };

    enabled = false;
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/hierarchy_aliases/hierarchy_aliases", schematic );
    const SCH_SHEET_LIST hierarchy = schematic->Hierarchy();
    const auto selected = std::find_if( hierarchy.begin(), hierarchy.end(),
            []( const SCH_SHEET_PATH& path )
            {
                return path.PathHumanReadable() == "/First Subsheet/";
            } );
    BOOST_REQUIRE( selected != hierarchy.end() );
    schematic->SetCurrentSheet( *selected );
    EXPORTER exporter( schematic.get() );

    for( bool published : { false, true } )
    {
        BOOST_TEST_CONTEXT( "published=" << published )
        {
            enabled = published;
            const SCH_SHEET_LIST sheets = exporter.SelectedSheets();
            BOOST_REQUIRE_EQUAL( sheets.size(), 2 );
            std::set<wxString> names;

            for( const SCH_SHEET_PATH& path : sheets )
            {
                BOOST_CHECK( path.at( 0 ) == selected->at( 0 ) );
                names.insert( path.PathHumanReadable() );
            }

            BOOST_CHECK( names.contains( "/First Subsheet/" ) );
            BOOST_CHECK( names.contains( "/First Subsheet/Sub2/" ) );
        }
    }

    const wxString variant = wxS( "Simulation exclusion" );
    schematic->AddVariant( variant );
    SCH_SHEET_PATH parent = *selected;
    parent.pop_back();

    for( bool published : { false, true } )
    {
        enabled = published;

        for( bool baseExcluded : { false, true } )
        {
            selected->Last()->SetExcludedFromSim( baseExcluded );
            selected->Last()->SetExcludedFromSim( !baseExcluded, &parent, variant );

            for( const wxString& active : { wxString(), variant } )
            {
                schematic->SetCurrentVariant( active );
                const auto sheets = exporter.SelectedSheets();
                const bool excluded = active.IsEmpty() ? baseExcluded : !baseExcluded;
                BOOST_CHECK_EQUAL( sheets.size(), excluded ? 0 : 2 );
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( SpiceGroundNamesIgnorePowerScope )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool published : { false, true } )
    {
        enabled = published;

        for( bool local : { false, true } )
        {
            for( const wxString& name : { wxString( "0" ), wxString( "GND" ), wxString( "gNd" ) } )
            {
                BOOST_TEST_CONTEXT( "published=" << published << ", local=" << local << ", name=" << name )
                {
                    SETTINGS_MANAGER settings;
                    std::unique_ptr<SCHEMATIC> schematic;
                    KI_TEST::LoadSchematic( settings, "ERC_dynamic_power_symbol_test", schematic );
                    SCH_SYMBOL* power = nullptr;

                    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
                    {
                        if( path.size() < 2 )
                            continue;

                        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
                        {
                            auto* symbol = static_cast<SCH_SYMBOL*>( item );

                            if( symbol->IsPower()
                                && symbol->GetField( FIELD_T::VALUE )->GetText().StartsWith( "REF" ) )
                                power = symbol;
                        }
                    }

                    BOOST_REQUIRE( power );
                    power->GetField( FIELD_T::VALUE )->SetText( name );

                    if( local )
                        power->GetLibSymbolRef()->SetLocalPower();
                    else
                        power->GetLibSymbolRef()->SetGlobalPower();

                    NETLIST_EXPORTER_SPICE exporter( schematic.get() );
                    WX_STRING_REPORTER reporter;
                    BOOST_REQUIRE_MESSAGE( exporter.ReadSchematicAndLibraries( 0, reporter ), reporter.GetMessages() );
                    const auto nets = exporter.GetNets();
                    BOOST_CHECK( nets.contains( name ) );
                    BOOST_CHECK( std::none_of( nets.begin(), nets.end(), [&]( const wxString& net )
                                              { return net.Lower().EndsWith( wxS( "/" ) + name.Lower() ); } ) );
                }
            }
        }
    }
}


using PUBLISHED_EXPORTERS = boost::mpl::list<NETLIST_EXPORTER_PADS, NETLIST_EXPORTER_CADSTAR>;

BOOST_AUTO_TEST_CASE_TEMPLATE( UsesPublishedNetsWithoutLegacyGraph, EXPORTER, PUBLISHED_EXPORTERS )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( const auto& [fixture, expectsSignals] :
         { std::pair{ "issue7203", false },
           std::pair{ "netlists/hierarchy_aliases/hierarchy_aliases", true },
           std::pair{ "netlists/video/video", true } } )
    {
        BOOST_TEST_CONTEXT( fixture )
        {
            enabled = false;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, fixture, schematic );
            auto exportText = [&]()
            {
                KI_TEST::SCOPED_TEMP_DIR directory( "connectivity-published-export" );
                const wxString path = directory.PathStr() + "/netlist.net";
                WX_STRING_REPORTER reporter;
                EXPORTER exporter( schematic.get() );
                BOOST_REQUIRE_MESSAGE( exporter.WriteNetlist( path, 0, reporter ), reporter.GetMessages() );
                wxFFile file( path, "r" );
                BOOST_REQUIRE( file.IsOpened() );
                wxString text;
                BOOST_REQUIRE( file.ReadAll( &text ) );

                if constexpr( std::is_same_v<EXPORTER, NETLIST_EXPORTER_CADSTAR> )
                {
                    const size_t start = text.find( ".TIM " );
                    BOOST_REQUIRE( start != wxString::npos );
                    const size_t end = text.find( '\n', start );
                    BOOST_REQUIRE( end != wxString::npos );
                    text.erase( start, end - start );
                }

                return text;
            };
            const wxString expected = exportText();
            const wxString marker = std::is_same_v<EXPORTER, NETLIST_EXPORTER_PADS> ? "*SIGNAL*" : ".ADD_TER";
            BOOST_CHECK_EQUAL( expected.Contains( marker ), expectsSignals );
            schematic->ConnectionGraph()->Reset();
            schematic->Connectivity().Clear();
            BOOST_REQUIRE( schematic->ConnectionGraph()->GetNetMap().empty() );
            enabled = true;
            const wxString actual = exportText();
            BOOST_CHECK_EQUAL( actual, expected );
        }
    }
}

BOOST_AUTO_TEST_CASE( AllegroUsesPublishedNetsAndPreservesDeviceFiles )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( const char* fixture : { "issue7203", "netlists/hierarchy_aliases/hierarchy_aliases",
                                "netlists/video/video" } )
    {
        BOOST_TEST_CONTEXT( fixture )
        {
            enabled = false;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, fixture, schematic );
            auto exportFiles = [&]()
            {
                KI_TEST::SCOPED_TEMP_DIR temp( "connectivity-allegro" );
                const wxString directory = temp.PathStr();
                WX_STRING_REPORTER reporter;
                NETLIST_EXPORTER_ALLEGRO exporter( schematic.get() );
                BOOST_REQUIRE_MESSAGE( exporter.WriteNetlist( directory + "/netlist.net", 0, reporter ),
                                       reporter.GetMessages() );
                wxArrayString paths;
                wxDir::GetAllFiles( directory, &paths );
                std::map<wxString, wxString> result;

                for( const wxString& path : paths )
                {
                    wxFFile file( path, "r" );
                    BOOST_REQUIRE( file.IsOpened() );
                    wxString text;
                    BOOST_REQUIRE( file.ReadAll( &text ) );
                    const wxString relative = path.Mid( directory.length() + 1 );

                    if( relative == "netlist.net" )
                    {
                        const size_t start = text.find( "(Date: " );
                        BOOST_REQUIRE( start != wxString::npos );
                        const size_t end = text.find( '\n', start );
                        BOOST_REQUIRE( end != wxString::npos );
                        text.erase( start, end - start );
                    }

                    result.emplace( relative, text );
                }

                BOOST_REQUIRE_GT( result.size(), 1u );
                return result;
            };
            const auto expected = exportFiles();
            schematic->ConnectionGraph()->Reset();
            schematic->Connectivity().Clear();
            BOOST_REQUIRE( schematic->ConnectionGraph()->GetNetMap().empty() );
            enabled = true;
            const auto actual = exportFiles();
            BOOST_REQUIRE_EQUAL( actual.size(), expected.size() );

            for( const auto& [path, text] : expected )
            {
                BOOST_TEST_CONTEXT( path )
                {
                    const auto found = actual.find( path );
                    BOOST_REQUIRE( found != actual.end() );
                    BOOST_CHECK_EQUAL( found->second, text );
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( PcbFormattingAndFileExportRebuildUnnotifiedSourceChanges )
{
    struct UNNOTIFIED_LABEL : SCH_LABEL
    {
        explicit UNNOTIFIED_LABEL( const SCH_LABEL& source ) : SCH_LABEL( source ) {}

        void cacheShownText() override { EDA_TEXT::cacheShownText(); }
    };

    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, true );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue7203", schematic );
    SCH_SCREEN& screen = *schematic->Hierarchy().front().LastScreen();
    auto labels = screen.Items().OfType( SCH_LABEL_T );
    BOOST_REQUIRE( labels.begin() != labels.end() );
    auto* native = static_cast<SCH_LABEL*>( *labels.begin() );
    auto replacement = std::make_unique<UNNOTIFIED_LABEL>( *native );
    screen.Remove( native, false );
    std::unique_ptr<SCH_LABEL> original( native );
    auto* label = replacement.get();
    screen.Append( replacement.release() );
    schematic->Connectivity().Update( *schematic );
    schematic->ConnectionGraph()->Reset();
    const auto revision = screen.ConnectivityRevision();
    label->EDA_TEXT::SetText( "EXPORT_FORMAT_FRESH" );
    BOOST_REQUIRE_EQUAL( screen.ConnectivityRevision(), revision );
    NETLIST_EXPORTER_KICAD exporter( schematic.get() );
    STRING_FORMATTER formatter;
    BOOST_REQUIRE_NO_THROW( exporter.Format( &formatter, GNL_ALL | GNL_OPT_KICAD ) );
    BOOST_CHECK( formatter.GetString().find( "EXPORT_FORMAT_FRESH" ) != std::string::npos );
    const auto nextRevision = screen.ConnectivityRevision();
    label->EDA_TEXT::SetText( "EXPORT_FILE_FRESH" );
    BOOST_REQUIRE_EQUAL( screen.ConnectivityRevision(), nextRevision );
    KI_TEST::SCOPED_TEMP_DIR directory( "connectivity-pcb-export" );
    const wxString path = directory.PathStr() + "/netlist.net";
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE_MESSAGE( exporter.WriteNetlist( path, 0, reporter ), reporter.GetMessages() );
    wxFFile file( path, "r" );
    BOOST_REQUIRE( file.IsOpened() );
    wxString text;
    BOOST_REQUIRE( file.ReadAll( &text ) );
    BOOST_CHECK( text.Contains( "EXPORT_FILE_FRESH" ) );
    BOOST_CHECK( !text.Contains( "EXPORT_FORMAT_FRESH" ) );
}

BOOST_AUTO_TEST_CASE( ExportRefreshesSymbolRuleAreaMembership )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool engine : { false, true } )
    {
        BOOST_TEST_CONTEXT( "engine=" << engine )
        {
            enabled = engine;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "netlists/multinetclasses/multinetclasses", schematic );
            const auto sheet = schematic->Hierarchy().front();
            SCH_SCREEN& screen = *sheet.LastScreen();
            SCH_SYMBOL* symbol = nullptr;

            for( SCH_ITEM* item : screen.Items().OfType( SCH_SYMBOL_T ) )
            {
                auto* candidate = static_cast<SCH_SYMBOL*>( item );

                if( !candidate->IsPower() && !candidate->GetRef( &sheet ).StartsWith( "#" ) )
                {
                    symbol = candidate;
                    break;
                }
            }

            BOOST_REQUIRE( symbol );
            auto areas = screen.Items().OfType( SCH_RULE_AREA_T );
            BOOST_REQUIRE( areas.begin() != areas.end() );
            auto* area = static_cast<SCH_RULE_AREA*>( *areas.begin() );
            screen.Remove( symbol, false );
            symbol->SetPosition( area->GetBoundingBox().Centre() );
            screen.Append( symbol );
            std::unordered_set<SCH_SCREEN*> screens{ &screen };
            SCH_RULE_AREA::UpdateRuleAreasInScreens( screens, nullptr );
            BOOST_REQUIRE( symbol->GetRuleAreaCache().contains( area ) );
            const VECTOR2I originalPosition = symbol->GetPosition();
            area->SetDNP( true );
            symbol->SetDNP( false, &sheet );
            symbol->SetPosition( VECTOR2I( -100000000, -100000000 ) );
            BOOST_REQUIRE( symbol->ResolveDNP( &sheet ) );
            const wxString reference = symbol->GetRef( &sheet );
            KI_TEST::SCOPED_TEMP_DIR directory( "connectivity-export-areas" );
            const wxString path = directory.PathStr() + "/netlist.xml";
            NETLIST_EXPORTER_XML exporter( schematic.get() );
            auto exportDnp = [&]()
            {
                WX_STRING_REPORTER reporter;
                BOOST_REQUIRE_MESSAGE( exporter.WriteNetlist( path, 0, reporter ), reporter.GetMessages() );
                wxXmlDocument document;
                BOOST_REQUIRE( document.Load( path ) );
                bool found = false;
                bool dnp = false;

                for( wxXmlNode* section = document.GetRoot()->GetChildren(); section; section = section->GetNext() )
                {
                    if( section->GetName() != "components" )
                        continue;

                    for( wxXmlNode* component = section->GetChildren(); component; component = component->GetNext() )
                    {
                        if( component->GetAttribute( "ref" ) != reference )
                            continue;

                        found = true;

                        for( wxXmlNode* property = component->GetChildren(); property; property = property->GetNext() )
                            dnp |= property->GetName() == "property" && property->GetAttribute( "name" ) == "dnp";
                    }
                }

                BOOST_REQUIRE( found );
                return dnp;
            };

            BOOST_CHECK( !exportDnp() );
            BOOST_CHECK( !symbol->ResolveDNP( &sheet ) );
            symbol->SetPosition( originalPosition );
            BOOST_CHECK( exportDnp() );
            BOOST_CHECK( symbol->ResolveDNP( &sheet ) );
        }
    }
}

BOOST_AUTO_TEST_CASE( FormattingFailureAllowsExporterReuse )
{
    struct FAILING_FORMATTER : STRING_FORMATTER
    {
        void write( const char*, int ) override
        {
            THROW_IO_ERROR( wxString( "Netlist output failed" ) );
        }
    } failing;

    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool engine : { false, true } )
    {
        BOOST_TEST_CONTEXT( "engine=" << engine )
        {
            enabled = engine;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "issue7203", schematic );
            NETLIST_EXPORTER_KICAD exporter( schematic.get() );
            BOOST_CHECK_EXCEPTION( exporter.Format( &failing, GNL_ALL | GNL_OPT_KICAD ), IO_ERROR,
                                  []( const IO_ERROR& error )
                                  {
                                      return error.Problem().Contains( "Netlist output failed" );
                                  } );
            STRING_FORMATTER output;
            BOOST_REQUIRE_NO_THROW( exporter.Format( &output, GNL_ALL | GNL_OPT_KICAD ) );
            BOOST_CHECK( !output.GetString().empty() );
        }
    }
}

BOOST_AUTO_TEST_CASE( ExportRefreshesCommittedChainAfterNetRename )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "net_chains_four_nets_labeled", schematic );
    auto* graph = schematic->ConnectionGraph();
    BOOST_REQUIRE( !graph->GetPotentialNetChains().empty() );
    auto* chain = graph->CreateNetChainFromPotential( graph->GetPotentialNetChains().front().get(),
                                                     "EXPORT_CHAIN" );
    BOOST_REQUIRE( chain );
    SCH_LABEL* label = nullptr;
    wxString oldName;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_LABEL_T ) )
        {
            auto* candidate = static_cast<SCH_LABEL*>( item );
            auto* connection = candidate->Connection( &path );

            if( connection && chain->GetNets().contains( connection->Name() ) )
            {
                label = candidate;
                oldName = connection->Name();
                break;
            }
        }

        if( label )
            break;
    }

    BOOST_REQUIRE( label );
    label->SetText( "EXPORT_CHAIN_RENAMED" );

    for( bool usePublished : { true, false } )
    {
        BOOST_TEST_CONTEXT( "published=" << usePublished )
        {
            enabled = usePublished;
            NETLIST_EXPORTER_XML exporter( schematic.get() );
            WX_STRING_REPORTER reporter;
            KI_TEST::SCOPED_TEMP_DIR directory( "connectivity-chain-export" );
            const wxString file = directory.PathStr() + "/netlist.xml";
            BOOST_REQUIRE_MESSAGE( exporter.WriteNetlist( file, GNL_OPT_KICAD, reporter ), reporter.GetMessages() );
            wxXmlDocument document;
            BOOST_REQUIRE( document.Load( file ) );
            std::set<wxString> members;
            auto collect = [&]( auto&& self, wxXmlNode* node ) -> void
            {
                for( ; node; node = node->GetNext() )
                {
                    if( node->GetName() == "member" )
                        members.insert( node->GetAttribute( "net" ) );

                    self( self, node->GetChildren() );
                }
            };
            collect( collect, document.GetRoot() );
            BOOST_CHECK( members.contains( "/EXPORT_CHAIN_RENAMED" ) );
            BOOST_CHECK( !members.contains( oldName ) );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
