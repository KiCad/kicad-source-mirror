/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <widgets/file_checklist_ctrl.h>


static FILE_CHECKLIST_ENTRY makeEntry( const wxString& aSection, const wxString& aPath, bool aChecked )
{
    FILE_CHECKLIST_ENTRY entry;
    entry.section = aSection;
    entry.path = aPath;
    entry.label = aPath.AfterLast( '/' );
    entry.status = wxT( "Modified" );
    entry.icon = BITMAPS::icon_pcbnew_16;
    entry.checked = aChecked;
    return entry;
}


BOOST_AUTO_TEST_SUITE( FileChecklistData )


BOOST_AUTO_TEST_CASE( SectionsAndRowsKeepInsertionOrder )
{
    FILE_CHECKLIST_DATA data;
    data.SetEntries( {
            makeEntry( wxT( "Changes" ), wxT( "proj/a.kicad_pcb" ), true ),
            makeEntry( wxT( "Shared libraries" ), wxT( "libs/x.kicad_sym" ), true ),
            makeEntry( wxT( "Changes" ), wxT( "proj/b.kicad_sch" ), true ),
    } );

    BOOST_REQUIRE_EQUAL( data.Sections().size(), 2 );
    BOOST_CHECK_EQUAL( data.Sections()[0], wxT( "Changes" ) );
    BOOST_CHECK_EQUAL( data.Sections()[1], wxT( "Shared libraries" ) );

    const std::vector<FILE_CHECKLIST_ENTRY>& changes = data.Rows( wxT( "Changes" ) );
    BOOST_REQUIRE_EQUAL( changes.size(), 2 );
    BOOST_CHECK_EQUAL( changes[0].path, wxT( "proj/a.kicad_pcb" ) );
    BOOST_CHECK_EQUAL( changes[1].path, wxT( "proj/b.kicad_sch" ) );
}


BOOST_AUTO_TEST_CASE( CheckedPathsReflectToggles )
{
    FILE_CHECKLIST_DATA data;
    data.SetEntries( {
            makeEntry( wxT( "Changes" ), wxT( "a" ), true ),
            makeEntry( wxT( "Changes" ), wxT( "b" ), false ),
            makeEntry( wxT( "Changes" ), wxT( "c" ), true ),
    } );

    std::vector<wxString> checked = data.CheckedPaths();
    BOOST_REQUIRE_EQUAL( checked.size(), 2 );
    BOOST_CHECK_EQUAL( checked[0], wxT( "a" ) );
    BOOST_CHECK_EQUAL( checked[1], wxT( "c" ) );

    data.SetChecked( wxT( "b" ), true );
    data.SetChecked( wxT( "a" ), false );

    checked = data.CheckedPaths();
    BOOST_REQUIRE_EQUAL( checked.size(), 2 );
    BOOST_CHECK_EQUAL( checked[0], wxT( "b" ) );
    BOOST_CHECK_EQUAL( checked[1], wxT( "c" ) );
}


// The header checkbox is ticked only when the whole section is, so a partly ticked section
// reads the same as an empty one.
BOOST_AUTO_TEST_CASE( SectionIsCheckedOnlyWhenEveryRowIs )
{
    FILE_CHECKLIST_DATA data;
    data.SetEntries( {
            makeEntry( wxT( "G" ), wxT( "a" ), true ),
            makeEntry( wxT( "G" ), wxT( "b" ), true ),
    } );

    BOOST_CHECK( data.IsSectionChecked( wxT( "G" ) ) );

    data.SetChecked( wxT( "a" ), false );
    BOOST_CHECK( !data.IsSectionChecked( wxT( "G" ) ) );

    data.SetChecked( wxT( "b" ), false );
    BOOST_CHECK( !data.IsSectionChecked( wxT( "G" ) ) );
}


BOOST_AUTO_TEST_CASE( SetSectionCheckedTogglesWholeGroup )
{
    FILE_CHECKLIST_DATA data;
    data.SetEntries( {
            makeEntry( wxT( "G" ), wxT( "a" ), false ),
            makeEntry( wxT( "G" ), wxT( "b" ), false ),
            makeEntry( wxT( "H" ), wxT( "c" ), false ),
    } );

    data.SetSectionChecked( wxT( "G" ), true );

    BOOST_CHECK( data.IsSectionChecked( wxT( "G" ) ) );
    BOOST_CHECK( !data.IsSectionChecked( wxT( "H" ) ) );

    std::vector<wxString> checked = data.CheckedPaths();
    BOOST_REQUIRE_EQUAL( checked.size(), 2 );
    BOOST_CHECK_EQUAL( checked[0], wxT( "a" ) );
    BOOST_CHECK_EQUAL( checked[1], wxT( "b" ) );
}


BOOST_AUTO_TEST_CASE( UnknownSectionReadsAsEmpty )
{
    FILE_CHECKLIST_DATA data;
    BOOST_CHECK( !data.IsSectionChecked( wxT( "missing" ) ) );
    BOOST_CHECK( data.Rows( wxT( "missing" ) ).empty() );
    BOOST_CHECK( data.CheckedPaths().empty() );
}


BOOST_AUTO_TEST_SUITE_END()
