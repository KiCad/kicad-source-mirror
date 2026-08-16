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

#include <memory>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>

#include <lib_id.h>
#include <lib_symbol.h>
#include <pgm_base.h>
#include <project.h>
#include <project_sch.h>
#include <reporter.h>
#include <schematic.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <libraries/symbol_library_adapter.h>

#include <symbol_import_reconciler.h>


namespace
{
/// Stage a fresh, private project directory and wire up the global library manager for it.
wxString stageProject( const wxString& aStem )
{
    wxString sep = wxFileName::GetPathSeparator();
    wxString dir = wxFileName::GetTempDir() + sep + aStem + wxT( "-symreconcile-qa" );

    if( wxDirExists( dir ) )
        wxFileName::Rmdir( dir, wxPATH_RMDIR_RECURSIVE );

    wxFileName::Mkdir( dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxString projectPath = dir + sep + aStem + wxT( ".kicad_pro" );

    Pgm().GetSettingsManager().LoadProject( projectPath );
    Pgm().GetLibraryManager().LoadProjectTables(
            Pgm().GetSettingsManager().Prj().GetProjectDirectory() );

    return Pgm().GetSettingsManager().Prj().GetProjectPath();
}


wxString easyEdaProV3Archive()
{
    return wxString::FromUTF8( KI_TEST::GetTestDataRootDir()
                               + "pcbnew/plugins/easyedapro/ProProject_LS2K0300Core_2025-11-14.epro2" );
}


/// Any .kicad_sym in @p aPath, so the check does not depend on the importer's nickname derivation.
bool hasSymbolLib( const wxString& aPath )
{
    wxDir    dir( aPath );
    wxString name;

    if( !dir.IsOpened() )
        return false;

    wxString spec = wxS( "*." ) + wxString( FILEEXT::KiCadSymbolLibFileExtension );

    return dir.GetFirst( &name, spec, wxDIR_FILES );
}


/// The EasyEDA Pro v3 sample, imported from a private copy so a stray write is visible.
struct IMPORTED_SAMPLE
{
    IO_RELEASER<SCH_IO>                      m_plugin;
    std::unique_ptr<SCHEMATIC>               m_schematic;
    std::vector<std::unique_ptr<LIB_SYMBOL>> m_definitions;
    wxString                                 m_sourceDir;
};


IMPORTED_SAMPLE importSample( PROJECT& aProject )
{
    IMPORTED_SAMPLE sample;

    wxString sep = wxFileName::GetPathSeparator();
    sample.m_sourceDir = aProject.GetProjectPath() + wxT( "import-src" );
    BOOST_REQUIRE( wxFileName::Mkdir( sample.m_sourceDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    wxFileName archiveFile( sample.m_sourceDir + sep, wxS( "sample" ), wxS( "epro2" ) );
    BOOST_REQUIRE( wxCopyFile( easyEdaProV3Archive(), archiveFile.GetFullPath() ) );

    sample.m_schematic = std::make_unique<SCHEMATIC>( nullptr );
    sample.m_schematic->SetProject( &aProject );

    sample.m_plugin.reset( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( sample.m_plugin );

    SCH_SHEET* rootSheet = nullptr;
    BOOST_REQUIRE_NO_THROW( rootSheet = sample.m_plugin->LoadSchematicFile(
                                    archiveFile.GetFullPath(), sample.m_schematic.get() ) );
    BOOST_REQUIRE( rootSheet );

    sample.m_schematic->RefreshHierarchy();

    for( LIB_SYMBOL* symbol : sample.m_plugin->GetImportedCachedLibrarySymbols() )
        sample.m_definitions.emplace_back( symbol );

    BOOST_REQUIRE_GT( sample.m_definitions.size(), 0 );

    return sample;
}


std::vector<SCH_SYMBOL*> placedSymbols( SCHEMATIC& aSchematic )
{
    std::vector<SCH_SYMBOL*> symbols;

    for( const SCH_SHEET_PATH& sheetPath : aSchematic.Hierarchy() )
    {
        if( SCH_SCREEN* screen = sheetPath.LastScreen() )
        {
            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
                symbols.push_back( static_cast<SCH_SYMBOL*>( item ) );
        }
    }

    return symbols;
}


/// Write @p aSymbol into a project library and register it, standing in for a user library.
void publishLibrary( PROJECT& aProject, SYMBOL_LIBRARY_ADAPTER& aAdapter, const wxString& aNickname,
                     const wxString& aFileStem, const LIB_SYMBOL& aSymbol )
{
    wxFileName libFn( aProject.GetProjectPath(), aFileStem, FILEEXT::KiCadSymbolLibFileExtension );

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    BOOST_REQUIRE( pi );

    pi->CreateLibrary( libFn.GetFullPath() );

    auto copy = std::make_unique<LIB_SYMBOL>( aSymbol );
    copy->SetLibId( LIB_ID( aNickname, copy->GetName() ) );
    pi->SaveSymbol( libFn.GetFullPath(), copy.release() );

    LIBRARY_TABLE* table = aAdapter.ProjectTable().value_or( nullptr );
    BOOST_REQUIRE( table );

    LIBRARY_TABLE_ROW& row = table->InsertRow();
    row.SetNickname( aNickname );
    row.SetURI( wxS( "${KIPRJMOD}/" ) + libFn.GetFullName() );
    row.SetType( wxS( "KiCad" ) );
    row.SetScope( LIBRARY_TABLE_SCOPE::PROJECT );

    BOOST_REQUIRE( table->Save().has_value() );
    aAdapter.LoadOne( aNickname );
}


/// A placed symbol the importer also defined, i.e. one the reconciler has to place somewhere.
struct COLLISION_CANDIDATE
{
    SCH_SYMBOL* m_symbol = nullptr;
    LIB_SYMBOL* m_definition = nullptr;
    wxString    m_nickname;
    wxString    m_name;
};


COLLISION_CANDIDATE findCandidate( IMPORTED_SAMPLE& aSample )
{
    COLLISION_CANDIDATE candidate;

    for( SCH_SYMBOL* symbol : placedSymbols( *aSample.m_schematic ) )
    {
        wxString nick = symbol->GetLibId().GetUniStringLibNickname();
        wxString name = symbol->GetLibId().GetUniStringLibItemName();

        if( nick.IsEmpty() || name.IsEmpty() )
            continue;

        for( const std::unique_ptr<LIB_SYMBOL>& def : aSample.m_definitions )
        {
            if( def->GetLibId().GetUniStringLibItemName() != name || def->GetPins().empty() )
                continue;

            candidate.m_symbol = symbol;
            candidate.m_definition = def.get();
            candidate.m_nickname = nick;
            candidate.m_name = name;

            return candidate;
        }
    }

    return candidate;
}
} // namespace


BOOST_AUTO_TEST_SUITE( SymbolImportReconciler )


// EasyEDA Pro v3 hands definitions over the standard hook, leaving the source directory untouched
// fails on revert, since LoadSchematicFile then publishes its own .kicad_sym beside the archive
BOOST_AUTO_TEST_CASE( EasyEdaProV3SchematicResolvesToGeneratedCache )
{
    wxString projectPath = stageProject( wxS( "easyedapro_v3" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    IMPORTED_SAMPLE sample = importSample( project );
    SCHEMATIC&      schematic = *sample.m_schematic;

    // the importer must not have published anything of its own beside the archive
    BOOST_CHECK_MESSAGE( !hasSymbolLib( sample.m_sourceDir ),
                         "LoadSchematicFile wrote a library into the source directory" );

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    const wxString           cacheNick = wxS( "ls2k0300-import-syms" );
    SYMBOL_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );

    SYMBOL_IMPORT_RECONCILE_RESULT result =
            reconciler.Reconcile( &schematic, std::move( sample.m_definitions ), cacheNick, {} );

    // the cache lands in the project directory, not next to the source archive
    BOOST_CHECK_EQUAL( result.m_cacheNickname, cacheNick );
    BOOST_CHECK_GT( result.m_savedToCache, 0 );

    wxFileName cacheFile( project.GetProjectPath(), cacheNick,
                          wxString( FILEEXT::KiCadSymbolLibFileExtension ) );
    BOOST_CHECK_MESSAGE( wxFileExists( cacheFile.GetFullPath() ),
                         "Generated .kicad_sym was not published into the project" );

    LIBRARY_TABLE* projectTable = adapter->ProjectTable().value_or( nullptr );
    BOOST_REQUIRE( projectTable );
    BOOST_CHECK( projectTable->HasRow( cacheNick ) );

    // every placed symbol resolves through the adapter after reconciliation
    int resolved = 0;

    for( const SCH_SHEET_PATH& sheetPath : schematic.Hierarchy() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    name = symbol->GetLibId().GetUniStringLibItemName();

            if( name.IsEmpty() )
                continue;

            wxString nick = symbol->GetLibId().GetUniStringLibNickname();

            BOOST_CHECK_MESSAGE( !nick.IsEmpty(),
                                 wxString::Format( "Symbol '%s' left with empty nickname", name ) );
            BOOST_CHECK_MESSAGE( adapter->LoadSymbol( nick, name ) != nullptr,
                                 wxString::Format( "LIB_ID '%s:%s' does not resolve after "
                                                   "reconciliation", nick, name ) );
            resolved++;
        }
    }

    BOOST_CHECK_GT( resolved, 0 );
    BOOST_CHECK_EQUAL( result.m_unresolved, 0 );

    wxFileName::Rmdir( projectPath, wxPATH_RMDIR_RECURSIVE );
}


// EasyEDA Pro v2 wrote its .kicad_sym beside the source archive, exactly as v3 did
BOOST_AUTO_TEST_CASE( EasyEdaProV2LeavesSourceDirectoryClean )
{
    wxString projectPath = stageProject( wxS( "easyedapro_v2" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    wxString sep = wxFileName::GetPathSeparator();
    wxString srcDir = projectPath + wxT( "import-src" );
    BOOST_REQUIRE( wxFileName::Mkdir( srcDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    wxString source = wxString::FromUTF8(
            KI_TEST::GetTestDataRootDir()
            + "pcbnew/plugins/easyedapro/ProProject_Yuzuki Chameleon_2023-09-02.epro" );
    BOOST_REQUIRE( wxFileExists( source ) );

    wxFileName archiveFile( srcDir + sep, wxS( "sample" ), wxS( "epro" ) );
    BOOST_REQUIRE( wxCopyFile( source, archiveFile.GetFullPath() ) );

    SCHEMATIC schematic( nullptr );
    schematic.SetProject( &project );

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EASYEDAPRO ) );
    BOOST_REQUIRE( plugin );

    SCH_SHEET* rootSheet = nullptr;
    BOOST_REQUIRE_NO_THROW(
            rootSheet = plugin->LoadSchematicFile( archiveFile.GetFullPath(), &schematic ) );
    BOOST_REQUIRE( rootSheet );

    BOOST_CHECK_MESSAGE( !hasSymbolLib( srcDir ),
                         "LoadSchematicFile wrote a library into the source directory" );

    // the definitions still reach the reconciler through the standard hook
    std::vector<LIB_SYMBOL*> raw = plugin->GetImportedCachedLibrarySymbols();
    BOOST_CHECK_GT( raw.size(), 0 );

    for( LIB_SYMBOL* symbol : raw )
        delete symbol;

    wxFileName::Rmdir( projectPath, wxPATH_RMDIR_RECURSIVE );
}


// An unrelated library that happens to carry the nickname the importer emitted must not swallow
// the imported definition; without the provenance check the symbol relinks to the wrong part
BOOST_AUTO_TEST_CASE( CollidingNicknameDoesNotStealTheLink )
{
    wxString projectPath = stageProject( wxS( "symreconcile_collide" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    IMPORTED_SAMPLE     sample = importSample( project );
    COLLISION_CANDIDATE candidate = findCandidate( sample );
    BOOST_REQUIRE( candidate.m_symbol );

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    // a pinless namesake registered under the importer's nickname: same name, different part
    LIB_SYMBOL impostor( candidate.m_name );
    BOOST_REQUIRE( impostor.GetPins().empty() );
    publishLibrary( project, *adapter, candidate.m_nickname, wxS( "impostor" ), impostor );

    const wxString           cacheNick = wxS( "collide-import-syms" );
    SYMBOL_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );

    SYMBOL_IMPORT_RECONCILE_RESULT result = reconciler.Reconcile(
            sample.m_schematic.get(), std::move( sample.m_definitions ), cacheNick, {} );

    BOOST_CHECK_EQUAL( result.m_cacheNickname, cacheNick );

    // the imported definition is kept in the cache rather than dropped for the namesake
    BOOST_CHECK_EQUAL( candidate.m_symbol->GetLibId().GetUniStringLibNickname(), cacheNick );

    LIB_SYMBOL* linked = adapter->LoadSymbol( cacheNick, candidate.m_name );
    BOOST_REQUIRE( linked );
    BOOST_CHECK_GT( linked->GetPins().size(), 0 );

    wxFileName::Rmdir( projectPath, wxPATH_RMDIR_RECURSIVE );
}


// The same nickname collision resolves the other way when the library really does hold the part,
// so the equivalence escape hatch keeps a genuine match out of the generated cache
BOOST_AUTO_TEST_CASE( EquivalentNamesakeTakesTheLink )
{
    wxString projectPath = stageProject( wxS( "symreconcile_equivalent" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    IMPORTED_SAMPLE     sample = importSample( project );
    COLLISION_CANDIDATE candidate = findCandidate( sample );
    BOOST_REQUIRE( candidate.m_symbol );

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    publishLibrary( project, *adapter, candidate.m_nickname, wxS( "genuine" ),
                    *candidate.m_definition );

    const wxString           cacheNick = wxS( "equivalent-import-syms" );
    wxString                 sourceNick = candidate.m_nickname;
    SYMBOL_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );

    SYMBOL_IMPORT_RECONCILE_RESULT result = reconciler.Reconcile(
            sample.m_schematic.get(), std::move( sample.m_definitions ), cacheNick, {} );

    BOOST_CHECK_EQUAL( candidate.m_symbol->GetLibId().GetUniStringLibNickname(), sourceNick );
    BOOST_CHECK_GT( result.m_linkedToSource, 0 );

    wxFileName::Rmdir( projectPath, wxPATH_RMDIR_RECURSIVE );
}


// Two source libraries supplying different symbols under one bare name must both survive the
// cache; keying the cache by the bare name alone dropped the second and relinked its instance
BOOST_AUTO_TEST_CASE( SameNameFromDifferentLibrariesKeepsBothDefinitions )
{
    wxString projectPath = stageProject( wxS( "symreconcile_namecollide" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    IMPORTED_SAMPLE sample = importSample( project );

    // two real imported symbols that a pin count tells apart
    SCH_SYMBOL* firstPlaced = nullptr;
    SCH_SYMBOL* secondPlaced = nullptr;

    for( SCH_SYMBOL* symbol : placedSymbols( *sample.m_schematic ) )
    {
        const std::unique_ptr<LIB_SYMBOL>& part = symbol->GetLibSymbolRef();

        if( !part || part->GetPins().empty() )
            continue;

        if( !firstPlaced )
            firstPlaced = symbol;
        else if( part->GetPins().size() != firstPlaced->GetLibSymbolRef()->GetPins().size() )
            secondPlaced = symbol;

        if( secondPlaced )
            break;
    }

    BOOST_REQUIRE( firstPlaced );
    BOOST_REQUIRE( secondPlaced );

    const wxString sharedName = wxS( "SHARED_SYM" );
    const size_t   firstPins = firstPlaced->GetLibSymbolRef()->GetPins().size();
    const size_t   secondPins = secondPlaced->GetLibSymbolRef()->GetPins().size();

    std::vector<std::unique_ptr<LIB_SYMBOL>> defs;

    // the same bare name under two source libraries, both placed and defined
    auto relabel = [&]( SCH_SYMBOL* aSymbol, const wxString& aNickname )
    {
        auto def = std::make_unique<LIB_SYMBOL>( *aSymbol->GetLibSymbolRef() );
        def->SetName( sharedName );
        def->SetLibId( LIB_ID( aNickname, sharedName ) );
        defs.push_back( std::move( def ) );

        aSymbol->SetLibId( LIB_ID( aNickname, sharedName ) );
    };

    relabel( firstPlaced, wxS( "libAlpha" ) );
    relabel( secondPlaced, wxS( "libBeta" ) );

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    const wxString           cacheNick = wxS( "namecollide-import-syms" );
    WX_STRING_REPORTER       reporter;
    SYMBOL_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath(), reporter );

    SYMBOL_IMPORT_RECONCILE_RESULT result =
            reconciler.Reconcile( sample.m_schematic.get(), std::move( defs ), cacheNick, {} );

    BOOST_CHECK_EQUAL( result.m_cacheNickname, cacheNick );

    LIB_ID firstId = firstPlaced->GetLibId();
    LIB_ID secondId = secondPlaced->GetLibId();

    BOOST_CHECK_EQUAL( firstId.GetUniStringLibNickname(), cacheNick );
    BOOST_CHECK_EQUAL( secondId.GetUniStringLibNickname(), cacheNick );
    BOOST_CHECK_MESSAGE( firstId.GetUniStringLibItemName() != secondId.GetUniStringLibItemName(),
                         "Symbols from two source libraries share one cache item name" );

    // each instance still resolves to the symbol it was imported as
    LIB_SYMBOL* firstLinked = adapter->LoadSymbol( cacheNick, firstId.GetUniStringLibItemName() );
    LIB_SYMBOL* secondLinked = adapter->LoadSymbol( cacheNick, secondId.GetUniStringLibItemName() );

    BOOST_REQUIRE( firstLinked );
    BOOST_REQUIRE( secondLinked );
    BOOST_CHECK_EQUAL( firstLinked->GetPins().size(), firstPins );
    BOOST_CHECK_EQUAL( secondLinked->GetPins().size(), secondPins );

    // the user is told which symbol the cache renamed
    const wxString renamed = firstId.GetUniStringLibItemName() == sharedName
                                     ? secondId.GetUniStringLibItemName()
                                     : firstId.GetUniStringLibItemName();

    BOOST_CHECK_MESSAGE( reporter.GetMessages().Contains(
                                 wxString::Format( wxS( "renamed to '%s'" ), renamed ) ),
                         "Cache rename was not reported" );

    wxFileName::Rmdir( projectPath, wxPATH_RMDIR_RECURSIVE );
}


// A project row already owning the cache nickname is a user library even when no file sits at the
// generated path, so publishing must not rewrite its URI
BOOST_AUTO_TEST_CASE( ExistingUserRowIsNotRepurposed )
{
    wxString projectPath = stageProject( wxS( "symreconcile_row" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    IMPORTED_SAMPLE sample = importSample( project );

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    const wxString cacheNick = wxS( "row-import-syms" );

    // the user's row owns the nickname but points at a file of its own
    LIB_SYMBOL owned( wxS( "UserPart" ) );
    publishLibrary( project, *adapter, cacheNick, wxS( "user-owned" ), owned );

    LIBRARY_TABLE* table = adapter->ProjectTable().value_or( nullptr );
    BOOST_REQUIRE( table );

    LIBRARY_TABLE_ROW* row = table->Row( cacheNick ).value_or( nullptr );
    BOOST_REQUIRE( row );

    const wxString uriBefore = row->URI();

    SYMBOL_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );

    SYMBOL_IMPORT_RECONCILE_RESULT result = reconciler.Reconcile(
            sample.m_schematic.get(), std::move( sample.m_definitions ), cacheNick, {} );

    BOOST_CHECK( result.m_cacheNickname.IsEmpty() );
    BOOST_CHECK_EQUAL( row->URI(), uriBefore );

    wxFileName cacheFile( project.GetProjectPath(), cacheNick,
                          wxString( FILEEXT::KiCadSymbolLibFileExtension ) );
    BOOST_CHECK_MESSAGE( !wxFileExists( cacheFile.GetFullPath() ),
                         "Published a cache over a nickname the user already owns" );

    wxFileName::Rmdir( projectPath, wxPATH_RMDIR_RECURSIVE );
}


// A cache whose table row cannot be persisted is gone on restart, so it must not be claimed
BOOST_AUTO_TEST_CASE( UnsavableTableLeavesCacheUnclaimed )
{
    wxString projectPath = stageProject( wxS( "symreconcile_rotable" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    LIBRARY_TABLE* table = adapter->ProjectTable().value_or( nullptr );
    BOOST_REQUIRE( table );
    BOOST_REQUIRE( table->Save().has_value() );

    wxFileName tableFn( table->Path() );
    tableFn.SetPermissions( wxS_IRUSR | wxS_IRGRP | wxS_IROTH );

    // CI containers commonly run as root, where the permission bits do not deny access
    if( tableFn.IsFileWritable() )
    {
        BOOST_TEST_MESSAGE( "Skipping read-only table check; file remains writable (running as "
                            "root?)" );
        tableFn.SetPermissions( wxS_IRUSR | wxS_IWUSR );
        wxFileName::Rmdir( projectPath, wxPATH_RMDIR_RECURSIVE );
        return;
    }

    IMPORTED_SAMPLE sample = importSample( project );

    const wxString           cacheNick = wxS( "rotable-import-syms" );
    SYMBOL_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );

    SYMBOL_IMPORT_RECONCILE_RESULT result = reconciler.Reconcile(
            sample.m_schematic.get(), std::move( sample.m_definitions ), cacheNick, {} );

    BOOST_CHECK( result.m_cacheNickname.IsEmpty() );
    BOOST_CHECK_EQUAL( result.m_linkedToCache, 0 );
    BOOST_CHECK_GT( result.m_unresolved, 0 );

    tableFn.SetPermissions( wxS_IRUSR | wxS_IWUSR );
    wxFileName::Rmdir( projectPath, wxPATH_RMDIR_RECURSIVE );
}


// An unregistered library already sitting at the cache path must never be overwritten
BOOST_AUTO_TEST_CASE( ExistingUserLibraryIsNotClobbered )
{
    wxString projectPath = stageProject( wxS( "symreconcile_clobber" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    IMPORTED_SAMPLE sample = importSample( project );

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    const wxString cacheNick = wxS( "victim-import-syms" );

    wxFileName victim( project.GetProjectPath(), cacheNick,
                       wxString( FILEEXT::KiCadSymbolLibFileExtension ) );

    const wxString sentinel = wxS( "(kicad_symbol_lib (version 20231120) (generator \"qa\"))" );
    BOOST_REQUIRE( wxFile( victim.GetFullPath(), wxFile::write ).Write( sentinel ) );

    SYMBOL_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );

    SYMBOL_IMPORT_RECONCILE_RESULT result = reconciler.Reconcile(
            sample.m_schematic.get(), std::move( sample.m_definitions ), cacheNick, {} );

    // the symbols stay unresolved rather than the file being taken over
    BOOST_CHECK( result.m_cacheNickname.IsEmpty() );
    BOOST_CHECK_GT( result.m_unresolved, 0 );

    wxString contents;
    wxFile   readBack( victim.GetFullPath(), wxFile::read );
    BOOST_REQUIRE( readBack.ReadAll( &contents ) );
    BOOST_CHECK_EQUAL( contents, sentinel );

    wxFileName::Rmdir( projectPath, wxPATH_RMDIR_RECURSIVE );
}


BOOST_AUTO_TEST_SUITE_END()
