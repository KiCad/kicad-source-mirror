/*
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/25869
 *
 * A variant's alternate symbol must be saved in lib_symbols so the schematic does not depend on
 * the project's symbol libraries to draw it.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <libraries/library_manager.h>
#include <locale_io.h>
#include <pgm_base.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <wx/ffile.h>
#include <wx/filename.h>


BOOST_AUTO_TEST_CASE( Issue25869_VariantAlternateInLibSymbols )
{
    LOCALE_IO                  locale;
    SETTINGS_MANAGER           settingsManager;
    std::unique_ptr<SCHEMATIC> schematic;

    KI_TEST::LoadSchematic( settingsManager, "issue25869/issue25869", schematic );
    BOOST_REQUIRE( schematic );

    Pgm().GetLibraryManager().LoadProjectTables(
            wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + wxS( "issue25869" ),
            { LIBRARY_TABLE_TYPE::SYMBOL } );

    wxString saved = wxFileName::CreateTempFileName( wxS( "kicad_issue25869" ) );
    SCH_IO_KICAD_SEXPR().SaveSchematicFile( saved, schematic->GetTopLevelSheets()[0], schematic.get() );

    wxString contents;
    wxFFile( saved ).ReadAll( &contents );
    wxRemoveFile( saved );

    BOOST_CHECK( contents.Contains( wxS( "(symbol \"vo_lib:R_ALT\"" ) ) );
}
