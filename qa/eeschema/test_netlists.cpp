/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <connection_graph.h>
#include <netlist_exporter_kicad.h>
#include <netlist_reader/netlist_reader.h>
#include <netlist_reader/pcb_netlist.h>
#include <project.h>
#include <sch_io_mgr.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>


class TEST_NETLISTS_FIXTURE
{
public:
    TEST_NETLISTS_FIXTURE() :
            m_schematic( nullptr ),
            m_manager( true )
    {
        m_pi = SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD );
    }

    virtual ~TEST_NETLISTS_FIXTURE()
    {
        m_schematic.Reset();
        SCH_IO_MGR::ReleasePlugin( m_pi );
    }

    void loadSchematic( const wxString& aBaseName );

    wxString getNetlistFileName( bool aTest = false );

    void writeNetlist();

    void compareNetlists();

    void cleanup();

    void doNetlistTest( const wxString& aBaseName );

    ///> Schematic to load
    SCHEMATIC m_schematic;

    SCH_PLUGIN* m_pi;

    SETTINGS_MANAGER m_manager;
};


static wxString getSchematicFile( const wxString& aBaseName )
{
    wxFileName fn = KI_TEST::GetEeschemaTestDataDir();
    fn.AppendDir( "netlists" );
    fn.AppendDir( aBaseName );
    fn.SetName( aBaseName );
    fn.SetExt( KiCadSchematicFileExtension );

    return fn.GetFullPath();
}


void TEST_NETLISTS_FIXTURE::loadSchematic( const wxString& aBaseName )
{
    wxString fn = getSchematicFile( aBaseName );

    BOOST_TEST_MESSAGE( fn );

    wxFileName pro( fn );
    pro.SetExt( ProjectFileExtension );

    m_manager.LoadProject( pro.GetFullPath() );

    m_manager.Prj().SetElem( PROJECT::ELEM_SCH_SYMBOL_LIBS, nullptr );

    m_schematic.Reset();
    m_schematic.SetProject( &m_manager.Prj() );
    m_schematic.SetRoot( m_pi->Load( fn, &m_schematic ) );

    BOOST_REQUIRE_EQUAL( m_pi->GetError().IsEmpty(), true );

    m_schematic.CurrentSheet().push_back( &m_schematic.Root() );

    SCH_SCREENS screens( m_schematic.Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->UpdateLocalLibSymbolLinks();

    SCH_SHEET_LIST sheets = m_schematic.GetSheets();

    // Restore all of the loaded symbol instances from the root sheet screen.
    sheets.UpdateSymbolInstances( m_schematic.RootScreen()->GetSymbolInstances() );

    sheets.AnnotatePowerSymbols();

    // NOTE: This is required for multi-unit symbols to be correct
    // Normally called from SCH_EDIT_FRAME::FixupJunctions() but could be refactored
    for( SCH_SHEET_PATH& sheet : sheets )
        sheet.UpdateAllScreenReferences();

    // NOTE: SchematicCleanUp is not called; QA schematics must already be clean or else
    // SchematicCleanUp must be freed from its UI dependencies.

    m_schematic.ConnectionGraph()->Recalculate( sheets, true );
}


wxString TEST_NETLISTS_FIXTURE::getNetlistFileName( bool aTest )
{
    wxFileName netFile = m_schematic.Prj().GetProjectFullName();

    if( aTest )
        netFile.SetName( netFile.GetName() + "_test" );

    netFile.SetExt( NetlistFileExtension );

    return netFile.GetFullPath();
}


void TEST_NETLISTS_FIXTURE::writeNetlist()
{
    auto exporter = std::make_unique<NETLIST_EXPORTER_KICAD>( &m_schematic );
    BOOST_REQUIRE_EQUAL( exporter->WriteNetlist( getNetlistFileName( true ), 0 ), true );
}


void TEST_NETLISTS_FIXTURE::compareNetlists()
{
    NETLIST golden;
    NETLIST test;

    {
        std::unique_ptr<NETLIST_READER> netlistReader( NETLIST_READER::GetNetlistReader(
                                            &golden, getNetlistFileName(), wxEmptyString ) );

        BOOST_REQUIRE_NO_THROW( netlistReader->LoadNetlist() );
    }

    {
        std::unique_ptr<NETLIST_READER> netlistReader( NETLIST_READER::GetNetlistReader(
                                            &test, getNetlistFileName( true ), wxEmptyString ) );

        BOOST_REQUIRE_NO_THROW( netlistReader->LoadNetlist() );
    }

    // Number of components should match
    BOOST_REQUIRE_EQUAL( golden.GetCount(), test.GetCount() );

    for( unsigned i = 0; i < golden.GetCount(); i++ )
    {
        COMPONENT* goldenComp = golden.GetComponent( i );
        COMPONENT* refComp    = test.GetComponentByReference( goldenComp->GetReference() );

        // Retrieval by reference
        BOOST_REQUIRE_NE( refComp, nullptr );

        // Retrieval by KIID
        KIID_PATH path = goldenComp->GetPath();

        BOOST_REQUIRE( !goldenComp->GetKIIDs().empty() );

        path.push_back( goldenComp->GetKIIDs().front() );

        COMPONENT* pathComp = test.GetComponentByPath( path );
        BOOST_REQUIRE_NE( pathComp, nullptr );

        // We should have found the same component
        BOOST_REQUIRE_EQUAL( refComp->GetReference(), pathComp->GetReference() );

        // And that component should have the same number of attached nets
        BOOST_REQUIRE_EQUAL( goldenComp->GetNetCount(), refComp->GetNetCount() );

        for( unsigned net = 0; net < goldenComp->GetNetCount(); net++ )
        {
            const COMPONENT_NET& goldenNet = goldenComp->GetNet( net );
            const COMPONENT_NET& testNet   = refComp->GetNet( net );

            // The two nets at the same index should be identical
            BOOST_REQUIRE_EQUAL( goldenNet.GetPinName(), testNet.GetPinName() );
            BOOST_REQUIRE_EQUAL( goldenNet.GetNetName(), testNet.GetNetName() );
        }
    }
}


void TEST_NETLISTS_FIXTURE::cleanup()
{
    wxRemoveFile( getNetlistFileName( true ) );
    m_schematic.Reset();
}


void TEST_NETLISTS_FIXTURE::doNetlistTest( const wxString& aBaseName )
{
    loadSchematic( aBaseName );
    writeNetlist();
    compareNetlists();
    cleanup();
}


BOOST_FIXTURE_TEST_SUITE( Netlists, TEST_NETLISTS_FIXTURE )


BOOST_AUTO_TEST_CASE( FindPlugin )
{
    BOOST_CHECK_NE( m_pi, nullptr );
}


BOOST_AUTO_TEST_CASE( GlobalPromotion )
{
    doNetlistTest( "test_global_promotion" );
}


BOOST_AUTO_TEST_CASE( GlobalPromotion2 )
{
    doNetlistTest( "test_global_promotion_2" );
}


BOOST_AUTO_TEST_CASE( Video )
{
    doNetlistTest( "video" );
}


BOOST_AUTO_TEST_CASE( ComplexHierarchy )
{
    doNetlistTest( "complex_hierarchy" );
}


BOOST_AUTO_TEST_CASE( WeakVectorBusDisambiguation )
{
    doNetlistTest( "weak_vector_bus_disambiguation" );
}


BOOST_AUTO_TEST_CASE( BusJunctions )
{
    doNetlistTest( "bus_junctions" );
}


BOOST_AUTO_TEST_CASE( HierRenaming )
{
    doNetlistTest( "test_hier_renaming" );
}


BOOST_AUTO_TEST_CASE( NoConnects )
{
    doNetlistTest( "noconnects" );
}


BOOST_AUTO_TEST_CASE( PrefixBusAlias )
{
    doNetlistTest( "prefix_bus_alias" );
}


BOOST_AUTO_TEST_CASE( GroupBusMatching )
{
    doNetlistTest( "group_bus_matching" );
}


BOOST_AUTO_TEST_CASE( TopLevelHierPins )
{
    doNetlistTest( "top_level_hier_pins" );
}


BOOST_AUTO_TEST_SUITE_END()
