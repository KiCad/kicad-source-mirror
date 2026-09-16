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

        m_model.AddColumn( GetDefaultFieldName( FIELD_T::REFERENCE, UNTRANSLATED ), wxS( "Reference" ), false );

        int referenceCol = m_model.GetFieldNameCol( GetDefaultFieldName( FIELD_T::REFERENCE, UNTRANSLATED ) );
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

    void Apply()
    {
        TOOL_MANAGER toolMgr;
        toolMgr.SetEnvironment( &m_board, nullptr, nullptr, nullptr, nullptr );

        BOARD_COMMIT commit( &toolMgr, true, false );
        TEMPLATES    templates;

        m_model.ApplyData( commit, templates );
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
    Apply();

    BOOST_CHECK( !m_footprint->IsExcludedFromSim() );
    BOOST_CHECK( m_footprint->GetExcludedFromSimForVariant( variantName ) );
}


BOOST_AUTO_TEST_CASE( RenameAcrossComputedBoundaryResetsStoredValue )
{
    const wxString ordinaryFieldName = wxS( "OrdinaryField" );
    const wxString computedFieldName = wxS( "@{1+2}" );
    PCB_FIELD*     field = new PCB_FIELD( m_footprint, FIELD_T::USER, ordinaryFieldName );

    field->SetText( wxS( "Ordinary value" ) );
    m_footprint->Add( field );
    AddTestColumn( ordinaryFieldName );

    m_model.RenameColumn( m_col, computedFieldName );

    BOOST_REQUIRE( m_model.ColIsComputed( m_col ) );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), computedFieldName );
    BOOST_CHECK_EQUAL( m_model.GetResolvedValue( 0, m_col ), wxString( wxS( "3" ) ) );

    m_model.RenameColumn( m_col, ordinaryFieldName );

    BOOST_REQUIRE( !m_model.ColIsComputed( m_col ) );
    BOOST_CHECK( m_model.GetValue( 0, m_col ).IsEmpty() );

    Apply();

    field = m_footprint->GetField( ordinaryFieldName );
    BOOST_REQUIRE( field );
    BOOST_CHECK( field->GetText().IsEmpty() );
    BOOST_CHECK( m_footprint->GetField( computedFieldName ) == nullptr );
}


// Repose #2771: an item change event need not mean that any field changed.
BOOST_AUTO_TEST_CASE( RefreshMergesLiveFieldsWithoutLosingStagedEdits )
{
    const wxString name = wxS( "MPN" );
    PCB_FIELD*     field = new PCB_FIELD( m_footprint, FIELD_T::USER, name );
    field->SetText( wxS( "original" ) );
    m_footprint->Add( field );
    AddTestColumn( name );
    int editedCol = m_col;
    AddTestColumn( GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED ) );

    m_model.SetValue( 0, editedCol, wxS( "staged" ) );
    m_footprint->SetPosition( VECTOR2I( 100, 200 ) );
    m_footprint->SetValue( wxS( "external value" ) );
    m_model.UpdateReferences( m_model.GetReferenceList() );

    BOOST_CHECK_EQUAL( m_model.GetValue( 0, editedCol ), wxS( "staged" ) );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "external value" ) );
    BOOST_CHECK( m_model.IsEdited() );

    Apply();
    BOOST_CHECK_EQUAL( field->GetText(), wxS( "staged" ) );
    BOOST_CHECK_EQUAL( m_footprint->GetValue(), wxS( "external value" ) );
    BOOST_CHECK( !m_model.IsEdited() );

    m_model.SetValue( 0, editedCol, wxS( "another staged edit" ) );
    field->SetText( wxS( "newer external edit" ) );
    m_model.UpdateReferences( m_model.GetReferenceList() );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, editedCol ), wxS( "newer external edit" ) );
    BOOST_CHECK( !m_model.IsEdited() );
}


BOOST_AUTO_TEST_CASE( RefreshPreservesClearAndExplicitEmptyCreation )
{
    const wxString removed = wxS( "Removed" );
    m_footprint->Add( new PCB_FIELD( m_footprint, FIELD_T::USER, removed ) );
    AddTestColumn( removed );
    int removedCol = m_col;
    m_model.ClearCell( 0, removedCol );

    const wxString created = wxS( "Created" );
    AddTestColumn( created );
    m_model.SetValue( 0, m_col, wxEmptyString );
    m_model.UpdateReferences( m_model.GetReferenceList() );

    BOOST_CHECK( m_model.IsCellClear( 0, removedCol ) );
    BOOST_CHECK( !m_model.IsCellClear( 0, m_col ) );
    Apply();
    BOOST_CHECK( !m_footprint->GetField( removed ) );
    BOOST_REQUIRE( m_footprint->GetField( created ) );
    BOOST_CHECK( m_footprint->GetField( created )->GetText().IsEmpty() );
}


// Repose #4001: switching the displayed variant must not change the board.
BOOST_AUTO_TEST_CASE( VariantSwitchStagesAllEditsUntilApply )
{
    const wxString name = GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED );
    m_footprint->SetValue( wxS( "base" ) );
    m_board.AddVariant( wxS( "A" ) );
    m_board.AddVariant( wxS( "B" ) );
    AddTestColumn( name );

    m_model.SetCurrentVariant( wxS( "A" ) );
    m_model.SetValue( 0, m_col, wxS( "A staged" ) );
    m_model.SetCurrentVariant( wxS( "B" ) );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "base" ) );
    m_model.SetValue( 0, m_col, wxS( "B staged" ) );
    m_model.SetCurrentVariant( wxS( "A" ) );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "A staged" ) );
    BOOST_CHECK_EQUAL( m_footprint->GetFieldValueForVariant( wxS( "A" ), name ), wxS( "base" ) );
    BOOST_CHECK_EQUAL( m_footprint->GetFieldValueForVariant( wxS( "B" ), name ), wxS( "base" ) );
    BOOST_CHECK( m_model.IsEdited() );

    Apply();
    BOOST_CHECK_EQUAL( m_footprint->GetValue(), wxS( "base" ) );
    BOOST_CHECK_EQUAL( m_footprint->GetFieldValueForVariant( wxS( "A" ), name ), wxS( "A staged" ) );
    BOOST_CHECK_EQUAL( m_footprint->GetFieldValueForVariant( wxS( "B" ), name ), wxS( "B staged" ) );
    BOOST_CHECK( !m_model.IsEdited() );
    m_model.UpdateReferences( m_model.GetReferenceList() );
    BOOST_CHECK( !m_model.IsEdited() );
}


BOOST_AUTO_TEST_CASE( DiscardingModelAfterVariantSwitchLeavesBoardUnchanged )
{
    m_footprint->SetValue( wxS( "base" ) );
    const wxString name = GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED );

    {
        FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL model( m_model.GetReferenceList() );
        model.AddColumn( GetDefaultFieldName( FIELD_T::REFERENCE, UNTRANSLATED ), wxS( "Reference" ), false );
        model.SetShowColumn( 0, true );
        model.AddColumn( name, name, false );
        model.RebuildRows();
        model.SetValue( 0, 1, wxS( "discard me" ) );
        model.SetCurrentVariant( wxS( "Another" ) );
        BOOST_CHECK( model.IsEdited() );
    }

    BOOST_CHECK_EQUAL( m_footprint->GetValue(), wxS( "base" ) );
}


BOOST_AUTO_TEST_CASE( ApplyingBaseEditDoesNotCreateUntouchedVariantOverride )
{
    const wxString name = GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED );
    m_footprint->SetValue( wxS( "base" ) );
    AddTestColumn( name );
    m_model.SetCurrentVariant( wxS( "Untouched" ) );
    m_model.SetCurrentVariant( wxEmptyString );
    m_model.SetValue( 0, m_col, wxS( "new base" ) );
    m_model.SetCurrentVariant( wxS( "Untouched" ) );
    Apply();

    BOOST_CHECK_EQUAL( m_footprint->GetFieldValueForVariant( wxS( "Untouched" ), name ), wxS( "new base" ) );
    BOOST_CHECK( !m_footprint->GetVariant( wxS( "Untouched" ) ) );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "new base" ) );
}


BOOST_AUTO_TEST_CASE( FieldClearIsSharedWithNewVariantAndCanBeReverted )
{
    const wxString name = wxS( "SharedPresence" );
    PCB_FIELD*     field = new PCB_FIELD( m_footprint, FIELD_T::USER, name );
    field->SetText( wxS( "base" ) );
    m_footprint->Add( field );
    AddTestColumn( name );
    m_model.SetCurrentVariant( wxS( "A" ) );
    m_model.ClearCell( 0, m_col );
    m_model.SetCurrentVariant( wxS( "B" ) );
    BOOST_CHECK( m_model.IsCellClear( 0, m_col ) );
    m_model.RevertRow( 0 );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "base" ) );
    BOOST_CHECK( !m_model.IsEdited() );
    m_model.ClearCell( 0, m_col );
    m_model.SetCurrentVariant( wxS( "A" ) );
    Apply();
    BOOST_CHECK( !m_footprint->GetField( name ) );
}


BOOST_AUTO_TEST_CASE( UndoRestoresEditsInTheirVariantsAndPreservesExternalChanges )
{
    m_footprint->SetValue( wxS( "base" ) );
    AddTestColumn( GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED ) );
    m_model.SetCurrentVariant( wxS( "A" ) );
    m_model.SetValue( 0, m_col, wxS( "A staged" ) );
    wxString snapshot = m_model.SerializeUndoState();
    m_model.SetCurrentVariant( wxS( "B" ) );
    m_model.SetValue( 0, m_col, wxS( "B staged" ) );
    m_model.RestoreUndoState( snapshot );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "base" ) );
    m_model.SetCurrentVariant( wxS( "A" ) );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "A staged" ) );

    m_footprint->SetValue( wxS( "external" ) );
    m_model.UpdateReferences( m_model.GetReferenceList() );
    m_model.RestoreUndoState( snapshot );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "external" ) );
    BOOST_CHECK( !m_model.IsEdited() );
}


BOOST_AUTO_TEST_CASE( RefreshMergesInactiveVariantAndExternalFieldRemoval )
{
    const wxString name = wxS( "MPN" );
    PCB_FIELD*     field = new PCB_FIELD( m_footprint, FIELD_T::USER, name );
    field->SetText( wxS( "base" ) );
    m_footprint->Add( field );
    AddTestColumn( name );
    m_model.SetCurrentVariant( wxS( "A" ) );
    m_model.SetValue( 0, m_col, wxS( "A staged" ) );
    m_model.SetCurrentVariant( wxS( "B" ) );
    m_model.SetValue( 0, m_col, wxS( "B staged" ) );

    m_footprint->AddVariant( wxS( "A" ) )->SetFieldValue( name, wxS( "A external" ) );
    m_model.UpdateReferences( m_model.GetReferenceList() );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "B staged" ) );
    m_model.SetCurrentVariant( wxS( "A" ) );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "A external" ) );
    m_model.SetCurrentVariant( wxS( "B" ) );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "B staged" ) );

    m_footprint->Remove( field );
    delete field;
    m_model.UpdateReferences( m_model.GetReferenceList() );
    BOOST_CHECK( m_model.IsCellClear( 0, m_col ) );
    BOOST_CHECK( !m_model.IsEdited() );
    Apply();
    BOOST_CHECK( !m_footprint->GetField( name ) );
}


BOOST_AUTO_TEST_CASE( RevertingVariantRetainsOtherVariantsEdits )
{
    m_footprint->SetValue( wxS( "base" ) );
    AddTestColumn( GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED ) );
    m_model.SetCurrentVariant( wxS( "A" ) );
    m_model.SetValue( 0, m_col, wxS( "A staged" ) );
    m_model.SetCurrentVariant( wxS( "B" ) );
    m_model.SetValue( 0, m_col, wxS( "B staged" ) );
    m_model.RevertRow( 0 );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "base" ) );
    BOOST_CHECK( m_model.IsEdited() );
    m_model.SetCurrentVariant( wxS( "A" ) );
    BOOST_CHECK_EQUAL( m_model.GetValue( 0, m_col ), wxS( "A staged" ) );
    Apply();
    BOOST_CHECK_EQUAL(
            m_footprint->GetFieldValueForVariant( wxS( "B" ), GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED ) ),
            wxS( "base" ) );
}


// Repose #2772: display and export must resolve field references before Apply.
BOOST_AUTO_TEST_CASE( MixedVariablesUseStagedFieldsForDisplayAndExport )
{
    m_footprint->SetValue( wxS( "10k" ) );
    PCB_FIELD* nested = new PCB_FIELD( m_footprint, FIELD_T::USER, wxS( "Nested" ) );
    nested->SetText( wxS( "Value: ${VALUE}" ) );
    m_footprint->Add( nested );
    PCB_FIELD* report = new PCB_FIELD( m_footprint, FIELD_T::USER, wxS( "Report" ) );
    report->SetText( wxS( "${REFERENCE}: ${Nested} ${NESTED}" ) );
    m_footprint->Add( report );
    AddTestColumn( GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED ) );
    int valueCol = m_col;
    AddTestColumn( wxS( "Nested" ) );
    AddTestColumn( wxS( "Report" ) );
    m_model.SetCurrentVariant( wxS( "A" ) );
    m_model.SetValue( 0, valueCol, wxS( "4.7k" ) );

    // User field names remain case-sensitive, unlike the built-in VALUE alias.
    const wxString expected = wxS( "U1: Value: 4.7k ${NESTED}" );
    BOOST_CHECK_EQUAL( m_model.GetResolvedValue( 0, m_col ), expected );
    BOOST_CHECK( m_model.Export( BOM_FMT_PRESET() ).Contains( expected ) );

    m_model.SetCurrentVariant( wxS( "B" ) );
    m_model.SetValue( 0, valueCol, wxS( "B staged" ) );
    BOOST_CHECK_EQUAL( m_model.GetResolvedValue( 0, m_col ), wxS( "U1: Value: B staged ${NESTED}" ) );
    m_model.SetCurrentVariant( wxS( "A" ) );
    BOOST_CHECK_EQUAL( m_model.GetResolvedValue( 0, m_col ), expected );
    BOOST_CHECK( m_board.GetCurrentVariant().IsEmpty() );
    BOOST_CHECK_EQUAL( m_footprint->GetValue(), wxS( "10k" ) );
}


BOOST_AUTO_TEST_CASE( MixedVariablesRespectClearEmptyAndMissingFields )
{
    PCB_FIELD* field = new PCB_FIELD( m_footprint, FIELD_T::USER, wxS( "MPN" ) );
    field->SetText( wxS( "live" ) );
    m_footprint->Add( field );
    AddTestColumn( wxS( "MPN" ) );
    int fieldCol = m_col;
    AddTestColumn( wxS( "Report" ), true );
    m_model.SetValue( 0, m_col, wxS( "[${MPN}] ${Unknown}" ) );
    m_model.ClearCell( 0, fieldCol );
    BOOST_CHECK_EQUAL( m_model.GetResolvedValue( 0, m_col ), wxS( "[] ${Unknown}" ) );
    m_model.SetValue( 0, fieldCol, wxEmptyString );
    BOOST_CHECK_EQUAL( m_model.GetResolvedValue( 0, m_col ), wxS( "[] ${Unknown}" ) );
    m_model.SetValue( 0, fieldCol, wxS( "staged" ) );
    BOOST_CHECK_EQUAL( m_model.GetResolvedValue( 0, m_col ), wxS( "[staged] ${Unknown}" ) );
    m_model.RemoveColumn( fieldCol );
    int reportCol = m_model.GetFieldNameCol( wxS( "Report" ) );
    BOOST_CHECK_EQUAL( m_model.GetResolvedValue( 0, reportCol ), wxS( "[] ${Unknown}" ) );
    BOOST_CHECK_EQUAL( field->GetText(), wxS( "live" ) );
}


BOOST_AUTO_TEST_SUITE_END()
