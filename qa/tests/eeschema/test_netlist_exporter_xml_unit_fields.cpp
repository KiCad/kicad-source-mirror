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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <netlist_exporter_xml.h>
#include <sch_field.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <wx/filename.h>
#include <wx/xml/xml.h>


namespace
{

struct UNIT_FIELDS_FIXTURE
{
    UNIT_FIELDS_FIXTURE()
    {
        // U4 is split over three sheets, with unit 3 alone on the oscillator sheet
        KI_TEST::LoadSchematic( m_settingsManager, wxT( "issue19646/issue19646" ), m_schematic );
    }

    void AddField( int aUnit, const wxString& aName, const wxString& aText )
    {
        SCH_SYMBOL* unit = nullptr;

        for( const SCH_SHEET_PATH& sheet : m_schematic->Hierarchy() )
        {
            for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheet ) == wxS( "U4" ) && symbol->GetUnitSelection( &sheet ) == aUnit )
                    unit = symbol;
            }
        }

        BOOST_REQUIRE( unit );

        SCH_FIELD field( unit, FIELD_T::USER, aName );
        field.SetText( aText );
        unit->AddField( field );
    }

    wxString ExportedField( const wxString& aName )
    {
        wxFileName netFile = m_schematic->Project().GetProjectFullName();
        netFile.SetName( netFile.GetName() + wxT( "_unit_fields" ) );
        netFile.SetExt( wxT( "xml" ) );

        WX_STRING_REPORTER   reporter;
        NETLIST_EXPORTER_XML exporter( m_schematic.get(), nullptr );

        BOOST_REQUIRE( exporter.WriteNetlist( netFile.GetFullPath(), 0, reporter ) );

        wxXmlDocument doc;
        BOOST_REQUIRE( doc.Load( netFile.GetFullPath() ) );
        wxRemoveFile( netFile.GetFullPath() );

        wxXmlNode* comp = doc.GetRoot()->GetChildren();

        while( comp && comp->GetName() != wxT( "components" ) )
            comp = comp->GetNext();

        BOOST_REQUIRE( comp );

        for( comp = comp->GetChildren(); comp; comp = comp->GetNext() )
        {
            if( comp->GetAttribute( wxT( "ref" ) ) != wxT( "U4" ) )
                continue;

            for( wxXmlNode* fields = comp->GetChildren(); fields; fields = fields->GetNext() )
            {
                if( fields->GetName() != wxT( "fields" ) )
                    continue;

                for( wxXmlNode* field = fields->GetChildren(); field; field = field->GetNext() )
                {
                    if( field->GetAttribute( wxT( "name" ) ) == aName )
                        return field->GetNodeContent();
                }
            }
        }

        BOOST_FAIL( "U4 exports no " + aName.ToStdString() );
        return wxEmptyString;
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};

} // namespace


BOOST_FIXTURE_TEST_CASE( NetlistExporterXML_BlankUnitFieldDoesNotHideText, UNIT_FIELDS_FIXTURE )
{
    AddField( 1, wxS( "Maker" ), wxEmptyString );
    AddField( 3, wxS( "Maker" ), wxS( "Acme" ) );

    BOOST_CHECK_EQUAL( ExportedField( wxS( "Maker" ) ), wxS( "Acme" ) );
}


BOOST_FIXTURE_TEST_CASE( NetlistExporterXML_UnitFieldUsesItsOwnSheet, UNIT_FIELDS_FIXTURE )
{
    AddField( 3, wxS( "Page" ), wxS( "${SHEETNAME}" ) );

    BOOST_CHECK_EQUAL( ExportedField( wxS( "Page" ) ), wxS( "F28P65x OSC, RESET AND DEBUG" ) );
}
