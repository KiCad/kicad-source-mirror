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

#include <libraries/symbol_library_adapter.h>
#include <libraries/library_manager.h>

#include <wx/textfile.h>

#include <filesystem>


namespace
{

/**
 * Exposes a way to seed a LOAD_ERROR sentinel entry into the project-scope map,
 * exactly as the async loader leaves it when a library fails to load (status set
 * to LOAD_ERROR, plugin and row left null).
 */
class TEST_SYMBOL_LIBRARY_ADAPTER : public SYMBOL_LIBRARY_ADAPTER
{
public:
    using SYMBOL_LIBRARY_ADAPTER::SYMBOL_LIBRARY_ADAPTER;

    void SeedLoadError( const wxString& aNickname )
    {
        m_libraries[aNickname].status.load_status = LOAD_STATUS::LOAD_ERROR;
    }

    /// Mark a table row as successfully loaded so GetLibraryNames() reports it.
    void SeedLoaded( const wxString& aNickname )
    {
        m_libraries[aNickname].status.load_status = LOAD_STATUS::LOADED;
    }
};


/// Writes a project sym-lib-table listing the given nicknames in the order supplied.
class SCOPED_SYM_LIB_TABLE
{
public:
    SCOPED_SYM_LIB_TABLE( const std::vector<wxString>& aNicknamesInTableOrder )
    {
        std::error_code ec;
        m_dir = std::filesystem::temp_directory_path( ec )
                / std::filesystem::path( wxString::Format( wxS( "kicad_qa_symlibtbl_%p" ), (void*) this )
                                                 .ToStdString() );
        std::filesystem::create_directories( m_dir, ec );

        wxTextFile file( wxString( ( m_dir / "sym-lib-table" ).string() ) );
        file.Create();
        file.AddLine( wxS( "(sym_lib_table" ) );
        file.AddLine( wxS( "  (version 7)" ) );

        for( const wxString& nickname : aNicknamesInTableOrder )
        {
            file.AddLine( wxString::Format(
                    wxS( "  (lib (name \"%s\")(type \"KiCad\")(uri \"${KIPRJMOD}/%s.kicad_sym\")(options \"\")(descr \"\"))" ),
                    nickname, nickname ) );
        }

        file.AddLine( wxS( ")" ) );
        file.Write();
        file.Close();
    }

    ~SCOPED_SYM_LIB_TABLE()
    {
        std::error_code ec;
        std::filesystem::remove_all( m_dir, ec );
    }

    wxString Path() const { return wxString( m_dir.string() ); }

private:
    std::filesystem::path m_dir;
};

} // namespace


BOOST_AUTO_TEST_SUITE( SymbolLibraryAdapter )


/**
 * Regression test for a null-plugin dereference crash.
 *
 * A symbol library that fails to load leaves a LOAD_ERROR sentinel with a null
 * plugin in the adapter's map. IsSymbolLibWritable() is called for every table row
 * by the symbol editor and chooser, so it must report such a library as not writable
 * instead of dereferencing the null plugin.
 */
BOOST_AUTO_TEST_CASE( IsSymbolLibWritableHandlesFailedLoad )
{
    LIBRARY_MANAGER             manager;
    TEST_SYMBOL_LIBRARY_ADAPTER adapter( manager );

    adapter.SeedLoadError( wxS( "BadLib" ) );

    BOOST_CHECK_EQUAL( adapter.IsSymbolLibWritable( wxS( "BadLib" ) ), false );

    // A library that was never even attempted must also be safe.
    BOOST_CHECK_EQUAL( adapter.IsSymbolLibWritable( wxS( "NeverSeen" ) ), false );
}


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23066.
 *
 * The library browsers list nicknames straight from GetLibraryNames(). When personal
 * libraries were appended to the table after the default ones, the raw table order left
 * them grouped at the bottom instead of interleaved alphabetically. GetLibraryNames() must
 * return the available nicknames in case-insensitive natural order regardless of table order.
 */
BOOST_AUTO_TEST_CASE( GetLibraryNamesSortedRegardlessOfTableOrder )
{
    const std::vector<wxString> tableOrder = { wxS( "Zebra" ), wxS( "amplifier" ), wxS( "Device" ),
                                               wxS( "Board_2" ), wxS( "Board_10" ) };

    SCOPED_SYM_LIB_TABLE tableFile( tableOrder );

    LIBRARY_MANAGER manager;
    manager.LoadProjectTables( tableFile.Path(), { LIBRARY_TABLE_TYPE::SYMBOL } );

    TEST_SYMBOL_LIBRARY_ADAPTER adapter( manager );

    for( const wxString& nickname : tableOrder )
        adapter.SeedLoaded( nickname );

    const std::vector<wxString> names = adapter.GetLibraryNames();

    const std::vector<wxString> expected = { wxS( "amplifier" ), wxS( "Board_2" ), wxS( "Board_10" ),
                                             wxS( "Device" ), wxS( "Zebra" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( names.begin(), names.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_SUITE_END()
