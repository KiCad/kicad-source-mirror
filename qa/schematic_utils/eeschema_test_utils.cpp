/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "eeschema_test_utils.h"

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <sch_io/sch_io.h>
#include <wildcards_and_files_ext.h>

#include <cstdlib>
#include <memory>

#include <eeschema/sch_io/sch_io.h>
#include <eeschema/sch_io/sch_io_mgr.h>
#include <eeschema/sch_screen.h>
#include <eeschema/schematic.h>
#include <eeschema/connection_graph.h>
#include <eeschema/sch_rule_area.h>


KI_TEST::SCHEMATIC_TEST_FIXTURE::SCHEMATIC_TEST_FIXTURE() :
        m_schematic( nullptr ),
        m_pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) )
{
}


KI_TEST::SCHEMATIC_TEST_FIXTURE::~SCHEMATIC_TEST_FIXTURE()
{
}


void KI_TEST::SCHEMATIC_TEST_FIXTURE::LoadSchematic( const wxFileName& aFn )
{
    wxFileName fn( aFn );

    BOOST_TEST_CHECKPOINT( "Loading schematic " << fn.GetFullPath() );

    wxFileName pro( fn );
    pro.SetExt( FILEEXT::ProjectFileExtension );

    // Schematic must be reset before a project is reloaded
    m_schematic.release();

    m_manager.LoadProject( pro.GetFullPath() );

    m_manager.Prj().SetElem( PROJECT::ELEM::LEGACY_SYMBOL_LIBS, nullptr );

    m_schematic = std::make_unique<SCHEMATIC>( &m_manager.Prj() );
    m_schematic->Reset();
    SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );

    SCH_SHEET* root = m_pi->LoadSchematicFile( fn.GetFullPath(), m_schematic.get() );
    m_schematic->AddTopLevelSheet( root );

    m_schematic->RemoveTopLevelSheet( defaultSheet );
    delete defaultSheet;

    BOOST_REQUIRE_EQUAL( m_pi->GetError().IsEmpty(), true );


    SCH_SCREENS screens( m_schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->UpdateLocalLibSymbolLinks();

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    // Restore all of the loaded symbol instances from the root sheet screen.
    if( m_schematic->RootScreen()->GetFileFormatVersionAtLoad() < 20221002 )
        sheets.UpdateSymbolInstanceData( m_schematic->RootScreen()->GetSymbolInstances() );

    if( m_schematic->RootScreen()->GetFileFormatVersionAtLoad() < 20221110 )
        sheets.UpdateSheetInstanceData( m_schematic->RootScreen()->GetSheetInstances() );

    if( m_schematic->RootScreen()->GetFileFormatVersionAtLoad() < 20221206 )
    {
        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
            screen->MigrateSimModels();
    }

    if( m_schematic->RootScreen()->GetFileFormatVersionAtLoad() < 20230221 )
        screens.FixLegacyPowerSymbolMismatches();

    sheets.AnnotatePowerSymbols();

    // NOTE: This is required for multi-unit symbols to be correct
    for( SCH_SHEET_PATH& sheet : sheets )
        sheet.UpdateAllScreenReferences();

    // NOTE: SchematicCleanUp is not called; QA schematics must already be clean or else
    // SchematicCleanUp must be freed from its UI dependencies.

    std::unordered_set<SCH_SCREEN*> all_screens;

    for( const SCH_SHEET_PATH& path : sheets )
        all_screens.insert( path.LastScreen() );

    SCH_RULE_AREA::UpdateRuleAreasInScreens( all_screens, nullptr );

    m_schematic->ConnectionGraph()->Recalculate( sheets, true );
}


wxFileName KI_TEST::SCHEMATIC_TEST_FIXTURE::SchematicQAPath( const wxString& aBaseName )
{
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( "netlists" );
    fn.AppendDir( aBaseName );
    fn.SetName( aBaseName );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    return fn;
}


template <typename Exporter>
wxString TEST_NETLIST_EXPORTER_FIXTURE<Exporter>::GetNetlistPath( bool aTest )
{
    wxFileName netFile = m_schematic->Project().GetProjectFullName();

    if( aTest )
        netFile.SetName( netFile.GetName() + "_test" );

    netFile.SetExt( FILEEXT::NetlistFileExtension );

    return netFile.GetFullPath();
}


template <typename Exporter>
void TEST_NETLIST_EXPORTER_FIXTURE<Exporter>::WriteNetlist()
{
    wxString netlistPath = GetNetlistPath( true );
    BOOST_TEST_CHECKPOINT( "Writing netlist " << netlistPath );

    // In case of a crash the file may not have been deleted.
    if( wxFileExists( netlistPath ) )
        wxRemoveFile( netlistPath );

    WX_STRING_REPORTER        reporter;
    std::unique_ptr<Exporter> exporter = std::make_unique<Exporter>( m_schematic.get() );

    bool success = exporter->WriteNetlist( netlistPath, GetNetlistOptions(), reporter );

    BOOST_REQUIRE( success && reporter.GetMessages().IsEmpty() );
}


template <typename Exporter>
void TEST_NETLIST_EXPORTER_FIXTURE<Exporter>::Cleanup()
{
    wxRemoveFile( GetNetlistPath( true ) );
    m_schematic->Reset();
}


template <typename Exporter>
void TEST_NETLIST_EXPORTER_FIXTURE<Exporter>::TestNetlist( const wxString& aBaseName )
{
    LoadSchematic( SchematicQAPath( aBaseName ) );
    WriteNetlist();
    CompareNetlists();
    Cleanup();
}


template class TEST_NETLIST_EXPORTER_FIXTURE<NETLIST_EXPORTER_KICAD>;
template class TEST_NETLIST_EXPORTER_FIXTURE<NETLIST_EXPORTER_SPICE>;
