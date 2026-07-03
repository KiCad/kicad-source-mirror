/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <map>
#include <set>

#include <core/ignore.h>
#include <kiway.h>
#include <pgm_base.h>
#include <sch_io/sch_io.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_sheet_path.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <libraries/library_manager.h>

/**
 * Checks that the SCH_IO manager finds the Eagle plugin
 */
BOOST_AUTO_TEST_CASE( FindPlugin )
{
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EAGLE ) );
    BOOST_CHECK_NE( pi.get(), nullptr );
}


/**
 * Get a schematic file from the test data eagle subdir
 */
static wxFileName getEagleTestSchematic( const wxString& sch_file )
{
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( "io" );
    fn.AppendDir( "eagle" );
    fn.SetFullName( sch_file );

    return fn;
}


/**
 * Common Eagle-import scaffolding: stage a fresh project under tempDir, configure the
 * library manager, run the Eagle SCH_IO plugin, and return the loaded virtual root.
 *
 * Eagle import uses Pgm().GetLibraryManager() for the symbol-library adapter, so the
 * project has to come through the global settings manager rather than a bare PROJECT
 * instance.  Writing to a temp dir keeps the generated .kicad_sym out of the source tree.
 */
static SCH_SHEET* loadEagleSchematic( const wxFileName& aEagleFn,
                                      const wxString& aProjectStem,
                                      std::unique_ptr<SCHEMATIC>& aSchematic )
{
    // Stage the project in a private, freshly-emptied directory.  The Eagle importer writes a
    // "<stem>-eagle-import.kicad_sym" library plus a sym-lib-table into the project directory and
    // keys duplicate-symbol-name disambiguation off the on-disk library, so leftover files from a
    // previous run in the shared temp directory would make the imported symbol names
    // non-deterministic.
    wxString sep = wxFileName::GetPathSeparator();
    wxString projectDir = wxStandardPaths::Get().GetTempDir() + sep + aProjectStem
                          + wxT( "-eagle-qa" );

    if( wxDirExists( projectDir ) )
        wxFileName::Rmdir( projectDir, wxPATH_RMDIR_RECURSIVE );

    wxFileName::Mkdir( projectDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxString projectPath = projectDir + sep + aProjectStem + wxT( ".kicad_pro" );

    Pgm().GetSettingsManager().LoadProject( projectPath );
    PROJECT& project = Pgm().GetSettingsManager().Prj();
    Pgm().GetLibraryManager().LoadProjectTables( project.GetProjectDirectory() );

    aSchematic = std::make_unique<SCHEMATIC>( &project );
    aSchematic->Reset();

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EAGLE ) );
    BOOST_REQUIRE( pi.get() != nullptr );

    SCH_SHEET* loadedSheet = nullptr;

    try
    {
        loadedSheet = pi->LoadSchematicFile( aEagleFn.GetFullPath(), aSchematic.get() );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( std::string( "LoadSchematicFile threw: " ) + e.what() );
    }
    catch( ... )
    {
        BOOST_FAIL( "LoadSchematicFile threw unknown exception" );
    }

    BOOST_REQUIRE( loadedSheet != nullptr );
    return loadedSheet;
}


/**
 * Verify that a multi-page Eagle import produces the correct top-level sheet hierarchy.
 *
 * Regression test for issue #23645: Eagle import was inserting a spurious empty default
 * sheet at index 0 in m_topLevelSheets, causing GetTopLevelSheet(0) to return an empty
 * sheet that then received the project filename.  On save, only that empty sheet was
 * written; on re-open all components were missing.
 */
BOOST_AUTO_TEST_CASE( ImportHierarchy )
{
    const wxFileName eagleFn = getEagleTestSchematic( "eagle-import-testfile.sch" );
    BOOST_REQUIRE( wxFileExists( eagleFn.GetFullPath() ) );

    std::unique_ptr<SCHEMATIC> schematic;
    SCH_SHEET* loadedSheet = loadEagleSchematic( eagleFn, wxS( "eagle_test" ), schematic );

    // The returned sheet must be the virtual root (niluuid).
    BOOST_CHECK( loadedSheet->m_Uuid == niluuid );

    const std::vector<SCH_SHEET*>& topSheets = schematic->GetTopLevelSheets();

    // The test file has exactly 2 Eagle pages; there must be no spurious default sheet.
    BOOST_CHECK_EQUAL( topSheets.size(), 2 );

    for( SCH_SHEET* sheet : topSheets )
    {
        // No top-level sheet should be the virtual root.
        BOOST_CHECK( sheet != nullptr );
        BOOST_CHECK( sheet->m_Uuid != niluuid );

        // Every page must have a screen with a non-empty filename so SaveProject can
        // write it to disk.
        BOOST_REQUIRE( sheet->GetScreen() != nullptr );
        BOOST_CHECK( !sheet->GetScreen()->GetFileName().IsEmpty() );
    }

    // Verify the hierarchy paths match the symbol instance paths so PCB update works.
    // Each symbol's hierarchical reference must have a path matching one of the top-level
    // sheet paths in the hierarchy.
    SCH_SHEET_LIST hierarchy = schematic->BuildSheetListSortedByPageNumbers();
    BOOST_CHECK_EQUAL( hierarchy.size(), 2 );

    std::set<KIID_PATH> hierPaths;

    for( const SCH_SHEET_PATH& path : hierarchy )
        hierPaths.insert( path.Path() );

    int totalSymbols = 0;
    int orphanedSymbols = 0;

    for( const SCH_SHEET_PATH& sheetPath : hierarchy )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( const EDA_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            const SCH_SYMBOL* sym = static_cast<const SCH_SYMBOL*>( item );
            ++totalSymbols;

            bool foundMatch = false;

            for( const SCH_SYMBOL_INSTANCE& inst : sym->GetInstances() )
            {
                if( hierPaths.count( inst.m_Path ) )
                {
                    foundMatch = true;
                    break;
                }
            }

            if( !foundMatch )
                ++orphanedSymbols;
        }
    }

    BOOST_CHECK_GT( totalSymbols, 0 );
    BOOST_CHECK_EQUAL( orphanedSymbols, 0 );
}


/**
 * Verify that Eagle-named nets remain global after import, preserving the flat namespace
 * Eagle uses for net names.
 *
 * Regression test for issue #24311 (and #24296): the Eagle importer was creating local
 * SCH_LABEL elements for nets that appeared on only one sheet, which caused KiCad to
 * prepend a sheet-path prefix to the resulting net name (e.g. "+24V_SWD" became
 * "/+24V_SWD").  The Eagle PCB importer keeps signal names verbatim, so on Update PCB
 * from Schematic the netlist updater reported "Reconnect via from +24V_SWD to /+24V_SWD"
 * and the copper pour for the affected net was orphaned on the old name.  The fix is to
 * emit SCH_GLOBALLABEL for every explicit Eagle net label and every fallback label, so
 * KiCad and the matching Eagle BRD agree on net names.
 */
BOOST_AUTO_TEST_CASE( LabelsRemainGlobalForFlatNamespace )
{
    const wxFileName eagleFn = getEagleTestSchematic( "eagle-import-testfile.sch" );
    BOOST_REQUIRE( wxFileExists( eagleFn.GetFullPath() ) );

    std::unique_ptr<SCHEMATIC> schematic;
    loadEagleSchematic( eagleFn, wxS( "eagle_label_kind" ), schematic );

    // Bucket every label in the imported schematic by kind + text so we can verify that
    // each named Eagle net produced a global label and no leftover local SCH_LABEL exists
    // with that name.  Bus labels (e.g. "A[1..3]") must stay local.
    SCH_SHEET_LIST hierarchy = schematic->BuildSheetListSortedByPageNumbers();

    std::unordered_set<wxString> globalTexts;
    std::unordered_set<wxString> localTexts;

    for( const SCH_SHEET_PATH& sheetPath : hierarchy )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
            globalTexts.insert( static_cast<SCH_LABEL_BASE*>( item )->GetText() );

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LABEL_T ) )
            localTexts.insert( static_cast<SCH_LABEL_BASE*>( item )->GetText() );
    }

    // Named Eagle nets must produce at least one global label each, and none of these
    // names may appear as a local SCH_LABEL — that would reintroduce the "/X" sheet-path
    // prefix that broke board update.
    const std::vector<wxString> expectedGlobalNets = {
        wxS( "A1" ), wxS( "A2" ), wxS( "A3" ),
        wxS( "B1" ), wxS( "B2" ), wxS( "B3" )
    };

    for( const wxString& netName : expectedGlobalNets )
    {
        BOOST_CHECK_MESSAGE( globalTexts.count( netName ) > 0,
                             "Missing global Eagle net label '" << netName.ToStdString()
                             << "' — flat namespace requires SCH_GLOBALLABEL." );
        BOOST_CHECK_MESSAGE( localTexts.count( netName ) == 0,
                             "Found local SCH_LABEL '" << netName.ToStdString()
                             << "' — Eagle net should be global." );
    }

    // Bus labels (e.g. "A[1..3]") must stay local: Eagle buses are visual groupings, not
    // electrical signals, so globalizing them would merge same-named buses project-wide.
    const std::vector<wxString> expectedLocalBuses = {
        wxS( "A[1..3]" ), wxS( "B[1..3]" )
    };

    for( const wxString& busName : expectedLocalBuses )
    {
        BOOST_CHECK_MESSAGE( globalTexts.count( busName ) == 0,
                             "Found global SCH_GLOBALLABEL '" << busName.ToStdString()
                             << "' — Eagle bus should remain a local SCH_LABEL." );
    }
}


/**
 * Verify that the Eagle "@<tag>" linking hint on a pin name is stripped from the displayed
 * pin name while still resolving the device <connect> mapping correctly.
 *
 * Regression test for issue #24483: Eagle disambiguates duplicate pin names within a symbol
 * with a trailing "@<tag>" (e.g. "IN@1", "IN@2", "NC@3").  This tag is metadata used only to
 * link pins to pads via <connect>; it should not appear as visible pin text.  The importer
 * was setting the raw Eagle name as the KiCad pin name, leaking "@1"/"@2"/"@3" into the
 * schematic.
 */
BOOST_AUTO_TEST_CASE( PinNameTagStripped )
{
    const wxFileName eagleFn = getEagleTestSchematic( "issue24483_pin_tag.sch" );
    BOOST_REQUIRE( wxFileExists( eagleFn.GetFullPath() ) );

    std::unique_ptr<SCHEMATIC> schematic;
    loadEagleSchematic( eagleFn, wxS( "eagle_pin_tag" ), schematic );

    SCH_SHEET_LIST hierarchy = schematic->BuildSheetListSortedByPageNumbers();

    // Key symbols by their library item name (e.g. "MYDEV"); the annotated reference is
    // unreliable here because the no-connect device is annotated with a '#' power prefix.  A
    // single Eagle library imports without a library-name prefix on the symbol; the prefix is
    // only added to disambiguate duplicate names across multiple Eagle libraries.
    std::map<wxString, const SCH_SYMBOL*> symbolByLibItem;

    for( const SCH_SHEET_PATH& sheetPath : hierarchy )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( const EDA_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            const SCH_SYMBOL* sym = static_cast<const SCH_SYMBOL*>( item );
            symbolByLibItem[sym->GetLibId().GetLibItemName().wx_str()] = sym;
        }
    }

    // The connected device maps pins to pads through <connect>; the no-connect device has no
    // <connect> so pins are auto-numbered. Both paths must strip the tag from the display name.
    BOOST_REQUIRE( symbolByLibItem.count( wxS( "MYDEV" ) ) == 1 );
    BOOST_REQUIRE( symbolByLibItem.count( wxS( "MYDEV_NC" ) ) == 1 );

    auto collectNames = []( const SCH_SYMBOL* aSym )
    {
        std::multiset<wxString> names;

        for( const SCH_PIN* pin : aSym->GetAllLibPins() )
            names.insert( pin->GetName() );

        return names;
    };

    // No display name on either symbol may retain the "@<tag>" suffix.
    for( const auto& [libItem, sym] : symbolByLibItem )
    {
        for( const SCH_PIN* pin : sym->GetAllLibPins() )
        {
            BOOST_CHECK_MESSAGE( pin->GetName().Find( '@' ) == wxNOT_FOUND,
                                 libItem.ToStdString() << " pin kept Eagle tag '"
                                 << pin->GetName().ToStdString() << "'." );
        }
    }

    // For the connected device, the two "IN@n" pins must both display as "IN" yet keep their
    // distinct pad mapping, and "NC@3" must strip to "NC".
    std::map<wxString, wxString> nameByPad;

    for( const SCH_PIN* pin : symbolByLibItem[wxS( "MYDEV" )]->GetAllLibPins() )
        nameByPad[pin->GetNumber()] = pin->GetName();

    BOOST_CHECK_EQUAL( nameByPad["1"], wxString( wxS( "IN" ) ) );
    BOOST_CHECK_EQUAL( nameByPad["2"], wxString( wxS( "IN" ) ) );
    BOOST_CHECK_EQUAL( nameByPad["3"], wxString( wxS( "GND" ) ) );
    BOOST_CHECK_EQUAL( nameByPad["4"], wxString( wxS( "NC" ) ) );

    // The no-connect device keeps every pin, so both tagged "IN" pins survive the strip.
    std::multiset<wxString> ncNames = collectNames( symbolByLibItem[wxS( "MYDEV_NC" )] );
    BOOST_CHECK_EQUAL( ncNames.count( wxS( "IN" ) ), 2 );
    BOOST_CHECK_EQUAL( ncNames.count( wxS( "GND" ) ), 1 );
    BOOST_CHECK_EQUAL( ncNames.count( wxS( "NC" ) ), 1 );
}


/**
 * Verify that a detached Eagle net label snaps onto its wire by perpendicular projection,
 * preserving its position along the wire.
 *
 * Regression test for issue #24810.  Eagle lets a net label float off its wire.  On import
 * KiCad relocates such a label onto the wire, but the old snap only considered the wire's
 * two endpoints and midpoint, so a label offset perpendicular from a long wire slid far
 * along it (e.g. 11 mm) and parallel labels piled onto the wire centre.  The fixture's net
 * has a 40 mm horizontal wire (80->120 mm) with a label 2.54 mm above it at x=90 mm; the
 * label must land at x=90 mm on the wire, not at an endpoint or the 100 mm midpoint.
 */
BOOST_AUTO_TEST_CASE( DetachedLabelProjectsOntoWire )
{
    const wxFileName eagleFn = getEagleTestSchematic( "eagle-import-testfile.sch" );
    BOOST_REQUIRE( wxFileExists( eagleFn.GetFullPath() ) );

    std::unique_ptr<SCHEMATIC> schematic;
    loadEagleSchematic( eagleFn, wxS( "eagle_detached_label" ), schematic );

    SCH_LABEL_BASE* detached = nullptr;
    SCH_SCREEN*     screen = nullptr;

    for( const SCH_SHEET_PATH& sheetPath : schematic->BuildSheetListSortedByPageNumbers() )
    {
        SCH_SCREEN* sheetScreen = sheetPath.LastScreen();

        if( !sheetScreen )
            continue;

        for( SCH_ITEM* item : sheetScreen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            if( static_cast<SCH_LABEL_BASE*>( item )->GetText() == wxS( "DETACHEDLABEL" ) )
            {
                detached = static_cast<SCH_LABEL_BASE*>( item );
                screen   = sheetScreen;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( detached, "DETACHEDLABEL global label not imported" );

    // Positions carry an import-time sheet translation, so all checks are made relative to
    // the wire's own endpoints.
    const VECTOR2I labelPos = detached->GetPosition();
    SEG            wire;
    double         wireDist = std::numeric_limits<double>::max();

    for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( !line->IsWire() )
            continue;

        SEG    seg( line->GetStartPoint(), line->GetEndPoint() );
        double d = seg.Distance( labelPos );

        if( d < wireDist )
        {
            wireDist = d;
            wire     = seg;
        }
    }

    // The label must be on the wire.
    BOOST_CHECK_MESSAGE( wireDist <= schIUScale.MilsToIU( 1 ),
                         "Label is " << ( wireDist / schIUScale.IU_PER_MM )
                                     << " mm from its wire" );

    // The wire is 40 mm long and the label sat 10 mm from the left end.  Perpendicular
    // projection preserves that; the old endpoint/midpoint snap would put it at 0 mm
    // (endpoint) or 20 mm (midpoint) from the left end.
    const VECTOR2I  leftEnd = wire.A.x <= wire.B.x ? wire.A : wire.B;
    const double    offsetMM = std::abs( labelPos.x - leftEnd.x ) / (double) schIUScale.IU_PER_MM;

    BOOST_CHECK_MESSAGE( std::abs( offsetMM - 10.0 ) < 1.0,
                         "Label projected to " << offsetMM
                                               << " mm from the wire's left end, expected 10 mm" );
}
