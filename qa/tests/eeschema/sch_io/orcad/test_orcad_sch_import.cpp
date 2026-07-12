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

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_io/orcad/sch_io_orcad.h>

#include <schematic.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <settings/settings_manager.h>

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>

#include <memory>
#include <string>


namespace
{

/// Throw-away temp file; impostor fixtures synthesized at runtime since user designs not redistributable.
struct TEMP_TEST_FILE
{
    TEMP_TEST_FILE( const wxString& aFileName, const wxString& aContents ) :
            m_path( wxFileName( wxFileName::GetTempDir(), aFileName ).GetFullPath() )
    {
        wxFFile file( m_path, wxS( "w" ) );

        if( file.IsOpened() )
            file.Write( aContents );
    }

    ~TEMP_TEST_FILE() { wxRemoveFile( m_path ); }

    wxString m_path;
};

} // namespace


struct ORCAD_SCH_IMPORT_FIXTURE
{
    ORCAD_SCH_IMPORT_FIXTURE() :
            m_schematic( new SCHEMATIC( nullptr ) )
    {
        m_manager.LoadProject( "" );
        m_schematic->SetProject( &m_manager.Prj() );
        m_schematic->CurrentSheet().clear();
        m_schematic->CurrentSheet().push_back( &m_schematic->Root() );
    }

    ~ORCAD_SCH_IMPORT_FIXTURE()
    {
        m_schematic.reset();
    }

    std::string dataPath( const std::string& aRelPath ) const
    {
        return KI_TEST::GetEeschemaTestDataDir() + "io/orcad/" + aRelPath;
    }

    SCH_SHEET* LoadOrcadSchematic( const std::string& aRelPath )
    {
        return m_plugin.LoadSchematicFile( dataPath( aRelPath ), m_schematic.get() );
    }

    SCH_IO_ORCAD               m_plugin;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SETTINGS_MANAGER           m_manager;
};


BOOST_FIXTURE_TEST_SUITE( OrcadSchImport, ORCAD_SCH_IMPORT_FIXTURE )


// ============================================================================
// File discrimination tests
//
// .dsn shared with SPECCTRA PCB text files; OrCAD Capture is OLE2/CFB (magic
// D0 CF 11 E0...) with a "Library" stream and "Views"/"Schematics" storage.
// Anything failing those checks rejected.
// ============================================================================

BOOST_AUTO_TEST_CASE( RejectsSpecctraTextDsn )
{
    TEMP_TEST_FILE specctra( wxS( "qa_orcad_specctra_impostor.dsn" ),
                             wxS( "(pcb \"impostor.dsn\"\n  (parser\n    (string_quote \")\n  )\n)\n" ) );

    BOOST_REQUIRE( wxFileName::FileExists( specctra.m_path ) );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( specctra.m_path ) );
}


BOOST_AUTO_TEST_CASE( RejectsNonexistentFile )
{
    wxFileName missing( wxFileName::GetTempDir(), wxS( "qa_orcad_does_not_exist.dsn" ) );

    BOOST_REQUIRE( !missing.FileExists() );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( missing.GetFullPath() ) );
}


BOOST_AUTO_TEST_CASE( RejectsWrongExtension )
{
    TEMP_TEST_FILE textFile( wxS( "qa_orcad_impostor.txt" ),
                             wxS( "Just some text, not a schematic.\n" ) );

    BOOST_REQUIRE( wxFileName::FileExists( textFile.m_path ) );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( textFile.m_path ) );
}


// Positive-load coverage requires a redistributable .dsn fixture under
// qa/data/eeschema/io/orcad/; once one exists, add a test here that calls
// LoadOrcadSchematic() and inspects the returned sheet's screen.


BOOST_AUTO_TEST_SUITE_END()
