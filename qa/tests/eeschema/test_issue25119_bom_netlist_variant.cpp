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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/25119
 *
 * The intermediate XML netlist feeding the legacy BOM tool is the flat view of the design,
 * so it has to be resolved against the current variant.  The board netlist is not: pcbnew
 * reads the base design plus every variant and resolves it itself.
 *
 * Reuses the issue24217 fixture, where variant H0 overrides R1's value to 10k and R3's to
 * 2M, and marks R2 and R3 DNP.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <eeschema_helpers.h>
#include <locale_io.h>
#include <netlist_exporter_kicad.h>
#include <netlist_exporter_xml.h>
#include <reporter.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/xml/xml.h>


namespace
{

wxXmlNode* findChild( wxXmlNode* aParent, const wxString& aName )
{
    for( wxXmlNode* child = aParent->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == aName )
            return child;
    }

    return nullptr;
}


wxXmlNode* findComponent( wxXmlNode* aRoot, const wxString& aRef )
{
    wxXmlNode* components = findChild( aRoot, wxT( "components" ) );

    BOOST_REQUIRE( components );

    for( wxXmlNode* comp = components->GetChildren(); comp; comp = comp->GetNext() )
    {
        if( comp->GetName() == wxT( "comp" ) && comp->GetAttribute( wxT( "ref" ) ) == aRef )
            return comp;
    }

    return nullptr;
}


wxString componentValue( wxXmlNode* aRoot, const wxString& aRef )
{
    wxXmlNode* comp = findComponent( aRoot, aRef );

    BOOST_REQUIRE( comp );

    wxXmlNode* value = findChild( comp, wxT( "value" ) );

    BOOST_REQUIRE( value );

    return value->GetNodeContent();
}


bool hasProperty( wxXmlNode* aRoot, const wxString& aRef, const wxString& aName )
{
    wxXmlNode* comp = findComponent( aRoot, aRef );

    BOOST_REQUIRE( comp );

    for( wxXmlNode* prop = comp->GetChildren(); prop; prop = prop->GetNext() )
    {
        if( prop->GetName() == wxT( "property" ) && prop->GetAttribute( wxT( "name" ) ) == aName )
            return true;
    }

    return false;
}


struct BOM_NETLIST_FIXTURE
{
    BOM_NETLIST_FIXTURE()
    {
        wxString schPath =
                wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + wxS( "issue24217/issue24217.kicad_sch" );

        m_schematic = EESCHEMA_HELPERS::LoadSchematic( schPath, true, true );
        BOOST_REQUIRE( m_schematic );

        m_netlistFile = wxFileName::CreateTempFileName( wxS( "kicad_issue25119" ) );
        BOOST_REQUIRE( !m_netlistFile.IsEmpty() );
    }

    ~BOM_NETLIST_FIXTURE()
    {
        if( wxFileExists( m_netlistFile ) )
            wxRemoveFile( m_netlistFile );
    }

    void WriteBomNetlist( wxXmlDocument& aDoc )
    {
        WX_STRING_REPORTER   reporter;
        NETLIST_EXPORTER_XML exporter( m_schematic );

        BOOST_REQUIRE( exporter.WriteNetlist( m_netlistFile, GNL_OPT_BOM, reporter ) );
        BOOST_REQUIRE( aDoc.Load( m_netlistFile ) );
        BOOST_REQUIRE( aDoc.GetRoot() );
    }

    LOCALE_IO  m_locale;
    SCHEMATIC* m_schematic = nullptr;
    wxString   m_netlistFile;
};

} // namespace


BOOST_FIXTURE_TEST_CASE( Issue25119_BomNetlistFollowsCurrentVariant, BOM_NETLIST_FIXTURE )
{
    m_schematic->SetCurrentVariant( wxS( "H0" ) );

    BOOST_REQUIRE_EQUAL( m_schematic->GetCurrentVariant(), wxS( "H0" ) );

    wxXmlDocument doc;
    WriteBomNetlist( doc );

    BOOST_CHECK_MESSAGE( !findChild( findComponent( doc.GetRoot(), wxS( "R1" ) ), wxS( "variants" ) ),
                         "resolved BOM netlist must not carry base-relative variant deltas" );

    BOOST_CHECK_EQUAL( componentValue( doc.GetRoot(), wxS( "R1" ) ), wxS( "10k" ) );
    BOOST_CHECK_EQUAL( componentValue( doc.GetRoot(), wxS( "R3" ) ), wxS( "2M" ) );

    BOOST_CHECK_MESSAGE( hasProperty( doc.GetRoot(), wxS( "R2" ), wxS( "dnp" ) ), "R2 should be DNP in variant H0" );
    BOOST_CHECK_MESSAGE( hasProperty( doc.GetRoot(), wxS( "R3" ), wxS( "dnp" ) ), "R3 should be DNP in variant H0" );
    BOOST_CHECK_MESSAGE( !hasProperty( doc.GetRoot(), wxS( "R1" ), wxS( "dnp" ) ),
                         "R1 should not be DNP in variant H0" );
}


BOOST_FIXTURE_TEST_CASE( Issue25119_BomNetlistDefaultVariantUnchanged, BOM_NETLIST_FIXTURE )
{
    wxXmlDocument doc;
    WriteBomNetlist( doc );

    BOOST_CHECK_EQUAL( componentValue( doc.GetRoot(), wxS( "R1" ) ), wxS( "6k2" ) );
    BOOST_CHECK_EQUAL( componentValue( doc.GetRoot(), wxS( "R3" ) ), wxS( "1M" ) );
    BOOST_CHECK( !hasProperty( doc.GetRoot(), wxS( "R2" ), wxS( "dnp" ) ) );
}


BOOST_FIXTURE_TEST_CASE( Issue25119_BoardExclusionFollowsCurrentVariant, BOM_NETLIST_FIXTURE )
{
    SCH_SHEET_PATH sheet = m_schematic->Hierarchy().at( 0 );
    SCH_SYMBOL*    r1 = nullptr;

    for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        if( static_cast<SCH_SYMBOL*>( item )->GetRef( &sheet ) == wxS( "R1" ) )
            r1 = static_cast<SCH_SYMBOL*>( item );
    }

    BOOST_REQUIRE( r1 );
    BOOST_REQUIRE( !r1->GetExcludedFromBoard() );

    // Same call the symbol properties dialog makes, so this lands on the variant only.
    r1->SetExcludedFromBoard( true, &sheet, wxS( "H0" ) );

    m_schematic->SetCurrentVariant( wxS( "H0" ) );
    BOOST_REQUIRE_EQUAL( m_schematic->GetCurrentVariant(), wxS( "H0" ) );

    wxXmlDocument doc;
    WriteBomNetlist( doc );

    BOOST_CHECK_MESSAGE( hasProperty( doc.GetRoot(), wxS( "R1" ), wxS( "exclude_from_board" ) ),
                         "R1 is excluded from the board in variant H0" );

    m_schematic->SetCurrentVariant( wxEmptyString );

    wxXmlDocument baseDoc;
    WriteBomNetlist( baseDoc );

    BOOST_CHECK_MESSAGE( !hasProperty( baseDoc.GetRoot(), wxS( "R1" ), wxS( "exclude_from_board" ) ),
                         "the base design must be unaffected" );
}


BOOST_FIXTURE_TEST_CASE( Issue25119_BoardNetlistKeepsBaseDesign, BOM_NETLIST_FIXTURE )
{
    m_schematic->SetCurrentVariant( wxS( "H0" ) );
    BOOST_REQUIRE_EQUAL( m_schematic->GetCurrentVariant(), wxS( "H0" ) );

    WX_STRING_REPORTER     reporter;
    NETLIST_EXPORTER_KICAD exporter( m_schematic );

    BOOST_REQUIRE( exporter.WriteNetlist( m_netlistFile, 0, reporter ) );

    wxFFile  file( m_netlistFile, wxS( "rb" ) );
    wxString netlist;

    BOOST_REQUIRE( file.IsOpened() );
    BOOST_REQUIRE( file.ReadAll( &netlist ) );

    BOOST_CHECK_MESSAGE( netlist.Contains( wxS( "(value \"6k2\")" ) ), "board netlist must keep R1's base value" );
    BOOST_CHECK_MESSAGE( !netlist.Contains( wxS( "(value \"10k\")" ) ),
                         "board netlist must not resolve the variant into the base value" );

    BOOST_CHECK_MESSAGE( netlist.Contains( wxS( "(name \"Value\") \"10k\"" ) ),
                         "board netlist must still carry R1's variant override for pcbnew" );
}
