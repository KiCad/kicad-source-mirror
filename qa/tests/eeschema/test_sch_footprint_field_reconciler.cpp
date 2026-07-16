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
#include <utility>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <lib_id.h>
#include <pgm_base.h>
#include <project.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>
#include <libraries/library_manager.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>

#include <sch_footprint_field_reconciler.h>


namespace
{
/// Gather every non-empty footprint-field FPID (nickname, item name) across the schematic.
std::vector<std::pair<wxString, wxString>> collectFootprintFpids( SCHEMATIC& aSchematic )
{
    std::vector<std::pair<wxString, wxString>> out;
    SCH_SCREENS                                screens( aSchematic.Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    text = symbol->GetField( FIELD_T::FOOTPRINT )->GetText();

            if( text.IsEmpty() )
                continue;

            LIB_ID fpid;

            if( fpid.Parse( text ) >= 0 || fpid.GetUniStringLibItemName().IsEmpty() )
                continue;

            out.emplace_back( fpid.GetUniStringLibNickname(), fpid.GetUniStringLibItemName() );
        }
    }

    return out;
}
} // namespace


BOOST_AUTO_TEST_SUITE( SchFootprintFieldReconciler )


// Eagle sets "<project>:<package>" footprint fields on a non-real lib; reconciler re-points nick to
// cache, keeps package; fails on revert since fields keep the bogus nick
BOOST_AUTO_TEST_CASE( EagleFootprintFieldsRelinkToCache )
{
    wxString sep = wxFileName::GetPathSeparator();
    wxString dir = wxStandardPaths::Get().GetTempDir() + sep + wxT( "sch_fpfield_reconcile-qa" );

    if( wxDirExists( dir ) )
        wxFileName::Rmdir( dir, wxPATH_RMDIR_RECURSIVE );

    wxFileName::Mkdir( dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxString projectPath = dir + sep + wxT( "sch_fpfield.kicad_pro" );
    Pgm().GetSettingsManager().LoadProject( projectPath );
    PROJECT& project = Pgm().GetSettingsManager().Prj();
    Pgm().GetLibraryManager().LoadProjectTables( project.GetProjectDirectory() );

    wxFileName eagleFn( KI_TEST::GetEeschemaTestDataDir() );
    eagleFn.AppendDir( wxS( "io" ) );
    eagleFn.AppendDir( wxS( "eagle" ) );
    eagleFn.SetFullName( wxS( "eagle-import-testfile.sch" ) );
    BOOST_REQUIRE( eagleFn.FileExists() );

    std::unique_ptr<SCHEMATIC> schematic = std::make_unique<SCHEMATIC>( &project );
    schematic->Reset();

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EAGLE ) );
    BOOST_REQUIRE( pi.get() );

    SCH_SHEET* loadedSheet = pi->LoadSchematicFile( eagleFn.GetFullPath(), schematic.get() );
    BOOST_REQUIRE( loadedSheet );

    std::vector<std::pair<wxString, wxString>> before = collectFootprintFpids( *schematic );
    BOOST_REQUIRE_GT( before.size(), 0 );

    const wxString                 cacheNick = wxS( "eagle_test-import-fps" );
    SCH_FOOTPRINT_FIELD_RECONCILER reconciler( cacheNick, {} );

    SCH_FP_FIELD_RECONCILE_RESULT result = reconciler.Reconcile( *schematic );

    BOOST_CHECK_GT( result.m_relinkedToCache, 0 );

    std::vector<std::pair<wxString, wxString>> after = collectFootprintFpids( *schematic );
    BOOST_CHECK_EQUAL( after.size(), before.size() );

    for( const auto& [nick, item] : after )
    {
        BOOST_CHECK_MESSAGE( nick == cacheNick,
                             wxString::Format( "Footprint field '%s:%s' not re-linked to cache",
                                               nick, item ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()
