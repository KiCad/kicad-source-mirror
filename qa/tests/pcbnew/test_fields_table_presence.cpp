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

#include <board.h>
#include <board_commit.h>
#include <footprint.h>
#include <footprint_fields_data_model.h>
#include <pcb_field.h>
#include <template_fieldnames.h>
#include <tool/tool_manager.h>


struct FOOTPRINT_FIELDS_TABLE_PRESENCE_FIXTURE
{
    FOOTPRINT_FIELDS_TABLE_PRESENCE_FIXTURE() :
            m_footprint( new FOOTPRINT( &m_board ) ),
            m_model( FOOTPRINT_REFERENCE_LIST{ FOOTPRINT_REF( *m_footprint ) } )
    {
        m_footprint->SetReference( wxS( "U1" ) );
        m_board.Add( m_footprint );

        m_model.AddColumn( GetCanonicalFieldName( FIELD_T::REFERENCE ), wxS( "Reference" ), false );

        int referenceCol = m_model.GetFieldNameCol( GetCanonicalFieldName( FIELD_T::REFERENCE ) );
        BOOST_REQUIRE( referenceCol >= 0 );
        m_model.SetShowColumn( referenceCol, true );
    }

    void AddTestColumn( const wxString& aFieldName, bool aAddedByUser = false )
    {
        m_model.AddColumn( aFieldName, aFieldName, aAddedByUser );
        m_col = m_model.GetFieldNameCol( aFieldName );
        BOOST_REQUIRE( m_col >= 0 );
        m_model.SetShowColumn( m_col, true );
        m_model.RebuildRows();

        BOOST_REQUIRE_EQUAL( m_model.GetNumberRows(), 1 );
    }

    void Apply( const wxString& aVariant = wxEmptyString )
    {
        TOOL_MANAGER toolMgr;
        toolMgr.SetEnvironment( &m_board, nullptr, nullptr, nullptr, nullptr );

        BOARD_COMMIT commit( &toolMgr, true, false );
        TEMPLATES    templates;

        m_model.ApplyData( commit, templates, aVariant );
    }

    BOARD                                   m_board;
    FOOTPRINT*                              m_footprint;
    FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL m_model;
    int                                     m_col = -1;
};


BOOST_FIXTURE_TEST_SUITE( FootprintFieldsTablePresence, FOOTPRINT_FIELDS_TABLE_PRESENCE_FIXTURE )


BOOST_AUTO_TEST_CASE( UntouchedMissingPresetFieldRemainsAbsent )
{
    const wxString fieldName = wxS( "UninstantiatedPresetField" );

    AddTestColumn( fieldName );

    BOOST_REQUIRE( m_model.IsCellClear( 0, m_col ) );

    Apply();

    BOOST_CHECK( m_footprint->GetField( fieldName ) == nullptr );
}


BOOST_AUTO_TEST_CASE( ExplicitEmptyValueCreatesField )
{
    const wxString fieldName = wxS( "ExplicitlyEmptyField" );

    AddTestColumn( fieldName );
    BOOST_REQUIRE( m_model.IsCellClear( 0, m_col ) );

    m_model.SetValue( 0, m_col, wxEmptyString );

    BOOST_REQUIRE( !m_model.IsCellClear( 0, m_col ) );
    BOOST_REQUIRE( m_model.IsCellEdited( 0, m_col ) );

    Apply();

    const PCB_FIELD* field = m_footprint->GetField( fieldName );
    BOOST_REQUIRE( field );
    BOOST_CHECK( field->GetText().IsEmpty() );
}


BOOST_AUTO_TEST_CASE( ExistingEmptyFieldCanBeCleared )
{
    const wxString fieldName = wxS( "ExistingEmptyField" );

    m_footprint->Add( new PCB_FIELD( m_footprint, FIELD_T::USER, fieldName ) );
    AddTestColumn( fieldName );

    BOOST_REQUIRE( !m_model.IsCellClear( 0, m_col ) );
    BOOST_REQUIRE( !m_model.IsCellEdited( 0, m_col ) );

    m_model.ClearCell( 0, m_col );

    BOOST_REQUIRE( m_model.IsCellClear( 0, m_col ) );
    BOOST_REQUIRE( m_model.IsCellEdited( 0, m_col ) );

    Apply();

    BOOST_CHECK( m_footprint->GetField( fieldName ) == nullptr );
}


BOOST_AUTO_TEST_CASE( UserAddedColumnCreatesEmptyField )
{
    const wxString fieldName = wxS( "UserAddedEmptyField" );

    AddTestColumn( fieldName, true );

    BOOST_REQUIRE( !m_model.IsCellClear( 0, m_col ) );
    BOOST_REQUIRE( m_model.IsCellEdited( 0, m_col ) );

    Apply();

    const PCB_FIELD* field = m_footprint->GetField( fieldName );
    BOOST_REQUIRE( field );
    BOOST_CHECK( field->GetText().IsEmpty() );
}


BOOST_AUTO_TEST_CASE( ExcludeFromSimulationAttributeIsEditable )
{
    const wxString fieldName = wxS( "${EXCLUDE_FROM_SIM}" );

    AddTestColumn( fieldName );

    BOOST_CHECK( !m_model.ColIsReadOnly( m_col ) );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxString( wxS( "0" ) ) );

    m_model.SetValue( 0, m_col, wxS( "1" ) );
    Apply();

    BOOST_CHECK( m_footprint->IsExcludedFromSim() );
}


BOOST_AUTO_TEST_CASE( ExcludeFromSimulationAttributeIsVariantAware )
{
    const wxString fieldName = wxS( "${EXCLUDE_FROM_SIM}" );
    const wxString variantName = wxS( "Production" );

    m_board.AddVariant( variantName );
    m_model.SetCurrentVariant( variantName );
    AddTestColumn( fieldName );

    m_model.SetValue( 0, m_col, wxS( "1" ) );
    Apply( variantName );

    BOOST_CHECK( !m_footprint->IsExcludedFromSim() );
    BOOST_CHECK( m_footprint->GetExcludedFromSimForVariant( variantName ) );
}


BOOST_AUTO_TEST_SUITE_END()
