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

#include <libraries/library_table.h>
#include <mdw/client/mdw_library_sources.h>

#include <qa_utils/file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <filesystem>
#include <set>

namespace fs = std::filesystem;


namespace
{

fs::path acmeSeed()
{
    return fs::path( KI_TEST::GetTestDataRootDir() ) / "common/git/acme-library";
}


/// The libraries of the acme-library fixture, named as its folder lists them
std::vector<wxString> acmeLibraryNames()
{
    std::vector<wxString> names;

    for( const fs::directory_entry& entry : fs::directory_iterator( acmeSeed() ) )
    {
        const fs::path path = entry.path();

        if( path.extension() == ".kicad_sym" || path.extension() == ".kicad_symdir" )
            names.push_back( wxString::FromUTF8( path.stem().string() ) );
    }

    return names;
}


/// An unpinned copy of acme-library, indexed as a sync leaves it
struct ACME_COPY
{
    explicit ACME_COPY( const wxString& aName = wxS( "acme-library" ), const MDW_SNAPSHOT& aPin = {} ) :
            temp( wxS( "mdw-library-rows" ) ),
            sources( temp.ChildPathStr( wxS( "sources.json" ) ), temp.ChildPathStr( wxS( "site.json" ) ),
                     temp.CreateChildDirStr( wxS( "copies" ) ) )
    {
        config.backend_id = wxS( "org.kicad.mdw.git" );
        config.locator = wxS( "https://git.example.com/acme-library.git" );
        config.name = aName;
        config.prefix = wxS( "acme-" );
        BOOST_REQUIRE( sources.Add( config ).IsOk() );

        copy.source_id = config.id;
        copy.backend_id = config.backend_id;
        copy.locator = config.locator;
        copy.local_root = sources.LocalRoot( config, aPin );
        copy.pinned = !aPin.token.IsEmpty();
        copy.snapshot = aPin;

        for( const wxString& name : acmeLibraryNames() )
        {
            const bool directory = fs::exists( acmeSeed() / ( name.utf8_string() + ".kicad_symdir" ) );

            copy.libraries.push_back( { config.id + wxS( ":" ) + name, config.prefix + name, MDW_LIBRARY_TYPE::SYMBOL,
                                        name + ( directory ? wxS( ".kicad_symdir" ) : wxS( ".kicad_sym" ) ) } );
        }

        BOOST_REQUIRE( sources.WriteCopy( copy ).IsOk() );
    }

    KI_TEST::SCOPED_TEMP_DIR  temp;
    MDW_LIBRARY_SOURCES       sources;
    MDW_LIBRARY_SOURCE_CONFIG config;
    MDW_LIBRARY_COPY          copy;
};

} // namespace


BOOST_AUTO_TEST_SUITE( MdwGitLibraryRows )


BOOST_AUTO_TEST_CASE( DefaultNameIsLocatorTail )
{
    BOOST_CHECK_EQUAL(
            MDW_LIBRARY_SOURCES::DefaultName( wxS( "https://user:secret@git.example.com/acme-library.git" ) ),
            wxS( "acme-library" ) );
    BOOST_CHECK_EQUAL( MDW_LIBRARY_SOURCES::DefaultName( wxS( "git@git.example.com:acme/acme-legacy.git/" ) ),
                       wxS( "acme-legacy" ) );
    BOOST_CHECK_EQUAL( MDW_LIBRARY_SOURCES::DefaultName( wxS( "acme-library" ) ), wxS( "acme-library" ) );
    BOOST_CHECK_EQUAL( MDW_LIBRARY_SOURCES::DefaultName( wxS( "https://git.example.com/" ) ),
                       wxS( "git.example.com" ) );
}


// A locator goes into design release tags, so a token typed into it must never be kept
BOOST_AUTO_TEST_CASE( LocatorDropsEmbeddedCredentials )
{
    BOOST_CHECK_EQUAL( MDW_LIBRARY_SOURCES::StripCredentials( wxS( "https://user:tok@git.example.com/acme.git" ) ),
                       wxS( "https://git.example.com/acme.git" ) );
    BOOST_CHECK_EQUAL( MDW_LIBRARY_SOURCES::StripCredentials( wxS( "https://tok@git.example.com/acme.git" ) ),
                       wxS( "https://git.example.com/acme.git" ) );
    BOOST_CHECK_EQUAL( MDW_LIBRARY_SOURCES::StripCredentials( wxS( "ssh://git:pw@git.example.com/acme.git" ) ),
                       wxS( "ssh://git@git.example.com/acme.git" ) );
    BOOST_CHECK_EQUAL( MDW_LIBRARY_SOURCES::StripCredentials( wxS( "C:\\libs\\acme.git" ) ),
                       wxS( "C:\\libs\\acme.git" ) );
}


// Set Up from a project's record names the copy as the project did, so every teammate's descriptions match
BOOST_AUTO_TEST_CASE( SetUpNamesTheCopyAsRecorded )
{
    MDW_LIBRARY_REQUIREMENT requirement;
    requirement.locator = wxS( "https://git.example.com/acme-library.git" );
    requirement.name = wxS( "acme" );

    BOOST_CHECK_EQUAL( MDW_LIBRARY_SOURCES::NameFor( requirement ), wxS( "acme" ) );

    // Anyone can write the project file, so a name that leaves team-libraries is not taken
    requirement.name = wxS( "../acme" );
    BOOST_CHECK_EQUAL( MDW_LIBRARY_SOURCES::NameFor( requirement ), wxS( "acme-library" ) );
}


BOOST_AUTO_TEST_CASE( UnpinnedRowsUseTeamLibrariesVariable )
{
    ACME_COPY acme;

    LIBRARY_TABLE         table( false, wxS( "(sym_lib_table (version 7))" ), LIBRARY_TABLE_SCOPE::GLOBAL );
    std::vector<wxString> conflicts;

    BOOST_REQUIRE( MDW_SETUP::SyncLibraryRows( table, MDW_LIBRARY_TYPE::SYMBOL, { acme.config }, conflicts, {},
                                               acme.sources ) );
    BOOST_REQUIRE_EQUAL( table.Rows().size(), acme.copy.libraries.size() );

    for( const LIBRARY_TABLE_ROW& row : table.Rows() )
    {
        BOOST_CHECK_MESSAGE( row.URI().StartsWith( wxS( "${KICAD_TEAM_LIBRARIES}/acme-library/" ) ), row.URI() );
        BOOST_CHECK_EQUAL( row.Description(), wxS( "acme-library (team library)" ) );
    }
}


// A pinned row sits in a committed project table, so users naming the source differently must agree on it
BOOST_AUTO_TEST_CASE( PinnedRowDescriptionIgnoresLocalSourceName )
{
    MDW_SNAPSHOT pin;
    pin.token = wxS( "0123abc" );

    std::vector<wxString> descriptions;

    for( const wxString& name : { wxS( "acme-library" ), wxS( "my-acme" ) } )
    {
        ACME_COPY acme( name, pin );

        LIBRARY_TABLE         table( false, wxS( "(sym_lib_table (version 7))" ), LIBRARY_TABLE_SCOPE::PROJECT );
        std::vector<wxString> conflicts;

        BOOST_REQUIRE( MDW_SETUP::SyncPinnedRows( table, MDW_LIBRARY_TYPE::SYMBOL,
                                                  { MDW_LIBRARY_SOURCES::RequirementFor( acme.config, pin ) }, {},
                                                  conflicts, {}, acme.sources ) );
        BOOST_REQUIRE( !table.Rows().empty() );
        descriptions.push_back( table.Rows().front().Description() );
    }

    BOOST_CHECK_EQUAL( descriptions[0], wxS( "acme-library (team library)" ) );
    BOOST_CHECK_EQUAL( descriptions[0], descriptions[1] );
}


// A row an earlier version wrote is brought to the variable form once, and a row the user added is left alone
BOOST_AUTO_TEST_CASE( MarkedRowMigratesOnceAndUserRowStays )
{
    ACME_COPY acme;

    const wxString user = wxS( "(lib (name \"local\")(type \"KiCad\")(uri \"/home/u/local.kicad_sym\")"
                               "(options \"\")(descr \"mine\"))" );
    const wxString old = wxString::Format( wxS( "(lib (name \"acme-passives\")(type \"KiCad\")"
                                                "(uri \"/home/u/team-libraries/acme-library/passives.kicad_sym\")"
                                                "(options \"mdw=%s\")(descr \"\"))" ),
                                           acme.config.id );

    LIBRARY_TABLE         table( false, wxS( "(sym_lib_table (version 7)" ) + user + old + wxS( ")" ),
                                 LIBRARY_TABLE_SCOPE::GLOBAL );
    std::vector<wxString> conflicts;

    BOOST_REQUIRE_EQUAL( table.Rows().size(), 2u );

    const LIBRARY_TABLE_ROW userBefore = *table.Row( wxS( "local" ) ).value();

    BOOST_CHECK( MDW_SETUP::SyncLibraryRows( table, MDW_LIBRARY_TYPE::SYMBOL, { acme.config }, conflicts, {},
                                             acme.sources ) );
    BOOST_CHECK( *table.Row( wxS( "local" ) ).value() == userBefore );
    BOOST_CHECK_EQUAL( table.Row( wxS( "acme-passives" ) ).value()->URI(),
                       wxS( "${KICAD_TEAM_LIBRARIES}/acme-library/passives.kicad_sym" ) );
    BOOST_CHECK_EQUAL( table.Row( wxS( "acme-passives" ) ).value()->Description(),
                       wxS( "acme-library (team library)" ) );

    BOOST_CHECK( !MDW_SETUP::SyncLibraryRows( table, MDW_LIBRARY_TYPE::SYMBOL, { acme.config }, conflicts, {},
                                              acme.sources ) );
}


BOOST_AUTO_TEST_CASE( BackupRootIsBesideCopy )
{
    ACME_COPY acme;

    const fs::path root = fs::path( acme.temp.Path() ) / "copies";
    const fs::path backups = fs::path( acme.sources.BackupRoot( acme.copy ).utf8_string() );

    BOOST_CHECK( backups.lexically_relative( root ).begin()->string() != ".." );
    BOOST_CHECK_EQUAL( backups.parent_path().parent_path().filename().string(), ".kicad" );
    BOOST_CHECK_EQUAL( backups.filename().string(), "acme-library" );
}


BOOST_AUTO_TEST_SUITE_END()
