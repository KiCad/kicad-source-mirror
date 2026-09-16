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

// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/25112

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>
#include <set>

#include <eeschema_helpers.h>
#include <symbol_fields_data_model.h>
#include <locale_io.h>
#include <sch_commit.h>
#include <sch_field.h>
#include <sch_reference_list.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <template_fieldnames.h>
#include <tool/tool_manager.h>


// The reproduction case instantiates level1.kicad_sch twice from the root and level2.kicad_sch
// twice from level1, so most symbols live in a single SCH_SYMBOL reachable through several
// sheet paths.  Editing a field with the fields table scoped to one sheet only updated the data
// store entries for the visible sheet paths; ApplyData walks every path, so the paths left out
// of scope wrote their stale value back over the edit and the footprint appeared to revert as
// soon as the dialog was applied.
struct ISSUE25112_FIXTURE
{
    ISSUE25112_FIXTURE()
    {
        wxString schPath = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() )
                           + wxS( "issue25112/issue25112.kicad_sch" );

        m_schematic.reset( EESCHEMA_HELPERS::LoadSchematic( schPath, true, false ) );
        BOOST_REQUIRE( m_schematic != nullptr );

        m_schematic->Hierarchy().GetSymbols( m_refs, SYMBOL_FILTER_NON_POWER );
    }

    /**
     * Build a model scoped to the first sheet path of a symbol that a later path also reaches.
     *
     * ApplyData follows the model's reference list, so scoping to that first path guarantees a
     * stale entry follows the edited one.
     */
    std::unique_ptr<SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL> MakeScopedModel( const wxString& aVariantName,
                                                                           const wxString& aFieldName,
                                                                           bool aAddedByUser = false )
    {
        auto model = std::make_unique<SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL>( m_refs );

        model->SetCurrentVariant( aVariantName );
        model->AddColumn( GetDefaultFieldName( FIELD_T::REFERENCE, UNTRANSLATED ), wxS( "Reference" ), false );
        model->AddColumn( aFieldName, aFieldName, aAddedByUser );

        int referenceCol = model->GetFieldNameCol( GetDefaultFieldName( FIELD_T::REFERENCE, UNTRANSLATED ) );
        BOOST_REQUIRE( referenceCol >= 0 );
        model->SetShowColumn( referenceCol, true );

        m_col = model->GetFieldNameCol( aFieldName );
        BOOST_REQUIRE( m_col >= 0 );
        model->SetShowColumn( m_col, true );

        const SCH_REFERENCE_LIST& modelRefs = model->GetReferenceList();
        bool                      found = false;

        for( size_t ii = 0; ii < modelRefs.GetCount() && !found; ++ii )
        {
            for( size_t jj = ii + 1; jj < modelRefs.GetCount(); ++jj )
            {
                if( modelRefs[jj].GetSymbol() == modelRefs[ii].GetSymbol() )
                {
                    m_symbol = modelRefs[ii].GetSymbol();
                    m_scopePath = modelRefs[ii].GetSheetPath();
                    m_siblingPath = modelRefs[jj].GetSheetPath();
                    found = true;
                    break;
                }
            }
        }

        BOOST_REQUIRE( found );

        model->SetPath( m_scopePath );
        model->SetScope( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE::SCOPE_SHEET );
        model->RebuildRows();

        m_row = -1;

        for( int ii = 0; ii < model->GetNumberRows() && m_row < 0; ++ii )
        {
            for( const SCH_REFERENCE& ref : model->GetRowReferences( ii ) )
            {
                if( ref.GetSymbol() == m_symbol )
                {
                    m_row = ii;
                    break;
                }
            }
        }

        BOOST_REQUIRE( m_row >= 0 );

        // Sanity: the sheet scope hides the symbol's other paths, which is what left their data
        // store entries stale.
        BOOST_REQUIRE_EQUAL( model->GetRowReferences( m_row ).size(), 1u );

        return model;
    }

    void Apply( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL& aModel )
    {
        TOOL_MANAGER toolMgr;
        SCH_COMMIT   commit( &toolMgr );
        TEMPLATES    templates;

        aModel.ApplyData( commit, templates );
    }

    LOCALE_IO                  m_locale;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SCH_REFERENCE_LIST         m_refs;
    SCH_SYMBOL*                m_symbol = nullptr;
    SCH_SHEET_PATH             m_scopePath;
    SCH_SHEET_PATH             m_siblingPath;
    int                        m_col = -1;
    int                        m_row = -1;
};


BOOST_FIXTURE_TEST_CASE( SheetScopedFieldEditSurvivesApply, ISSUE25112_FIXTURE )
{
    std::unique_ptr<SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL> model =
            MakeScopedModel( wxEmptyString, GetDefaultFieldName( FIELD_T::FOOTPRINT, UNTRANSLATED ) );

    const wxString newFootprint = wxS( "Resistor_SMD:R_0603_1608Metric" );

    model->SetValue( m_row, m_col, newFootprint );
    Apply( *model );

    BOOST_CHECK_EQUAL( m_symbol->GetField( FIELD_T::FOOTPRINT )->GetText(), newFootprint );
}


BOOST_FIXTURE_TEST_CASE( SheetScopedFieldClearCanBeReverted, ISSUE25112_FIXTURE )
{
    const wxString fieldName = wxS( "MPN" );
    const wxString fieldValue = wxS( "ABC123" );

    std::unique_ptr<SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL> model = MakeScopedModel( wxEmptyString, fieldName );

    SCH_FIELD field( m_symbol, FIELD_T::USER, fieldName );
    field.SetText( fieldValue );
    m_symbol->AddField( field );
    model->UpdateReferences( m_refs );

    BOOST_REQUIRE( !model->IsCellClear( m_row, m_col ) );

    model->ClearCell( m_row, m_col );
    BOOST_REQUIRE( model->IsCellClear( m_row, m_col ) );

    model->RevertRow( m_row );
    BOOST_CHECK( !model->IsCellClear( m_row, m_col ) );
    BOOST_CHECK( !model->IsEdited() );

    Apply( *model );

    const SCH_FIELD* appliedField = m_symbol->GetField( fieldName );
    BOOST_REQUIRE( appliedField );
    BOOST_CHECK_EQUAL( appliedField->GetText(), fieldValue );
}


BOOST_FIXTURE_TEST_CASE( GroupedEditStateChecksEveryItem, ISSUE25112_FIXTURE )
{
    const wxString        fieldName = wxS( "GroupedState" );
    const wxString        fieldValue = wxS( "Original" );
    std::set<SCH_SYMBOL*> symbols;

    for( const SCH_REFERENCE& ref : m_refs )
    {
        SCH_SYMBOL* symbol = ref.GetSymbol();

        if( !symbols.insert( symbol ).second )
            continue;

        SCH_FIELD field( symbol, FIELD_T::USER, fieldName );
        field.SetText( fieldValue );
        symbol->AddField( field );
    }

    SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL model( m_refs );
    model.AddColumn( GetDefaultFieldName( FIELD_T::REFERENCE, UNTRANSLATED ), wxS( "Reference" ), false );
    model.AddColumn( fieldName, fieldName, false );

    int referenceCol = model.GetFieldNameCol( GetDefaultFieldName( FIELD_T::REFERENCE, UNTRANSLATED ) );
    int fieldCol = model.GetFieldNameCol( fieldName );
    BOOST_REQUIRE( referenceCol >= 0 );
    BOOST_REQUIRE( fieldCol >= 0 );

    model.SetShowColumn( referenceCol, true );
    model.SetShowColumn( fieldCol, true );
    model.SetGroupingEnabled( true );
    model.SetGroupColumn( fieldCol, true );
    model.RebuildRows();

    int         groupedRow = -1;
    SCH_SYMBOL* symbolToModify = nullptr;

    for( int row = 0; row < model.GetNumberRows() && groupedRow < 0; ++row )
    {
        const std::vector<SCH_REFERENCE> refs = model.GetRowReferences( row );

        for( size_t ii = 1; ii < refs.size(); ++ii )
        {
            if( refs[ii].GetSymbol() != refs[0].GetSymbol() )
            {
                groupedRow = row;
                symbolToModify = refs[ii].GetSymbol();
                break;
            }
        }
    }

    BOOST_REQUIRE( groupedRow >= 0 );
    BOOST_REQUIRE( symbolToModify );
    BOOST_REQUIRE( !model.IsRowEdited( groupedRow ) );

    symbolToModify->GetField( fieldName )->SetText( wxS( "Modified" ) );

    BOOST_CHECK( !model.IsCellEdited( groupedRow, referenceCol ) );
    BOOST_CHECK( model.IsCellEdited( groupedRow, fieldCol ) );
    BOOST_CHECK( model.IsRowEdited( groupedRow ) );
}


// Variant field values are stored per symbol instance, so an edit made against one sheet path
// must not be stamped onto the paths that share the symbol.
BOOST_FIXTURE_TEST_CASE( SheetScopedVariantEditStaysOnItsPath, ISSUE25112_FIXTURE )
{
    const wxString variant = wxS( "Assembly" );

    std::unique_ptr<SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL> model =
            MakeScopedModel( variant, GetDefaultFieldName( FIELD_T::FOOTPRINT, UNTRANSLATED ) );

    const wxString baseFootprint = m_symbol->GetField( FIELD_T::FOOTPRINT )->GetText();
    const wxString newFootprint = wxS( "Resistor_SMD:R_0603_1608Metric" );

    BOOST_REQUIRE( baseFootprint != newFootprint );

    model->SetValue( m_row, m_col, newFootprint );
    Apply( *model );

    BOOST_CHECK_EQUAL( m_symbol->GetField( FIELD_T::FOOTPRINT )->GetText( &m_scopePath, variant ), newFootprint );
    BOOST_CHECK_EQUAL( m_symbol->GetField( FIELD_T::FOOTPRINT )->GetText( &m_siblingPath, variant ), baseFootprint );
    BOOST_CHECK_EQUAL( m_symbol->GetField( FIELD_T::FOOTPRINT )->GetText(), baseFootprint );
}


// The board exclusion has no variant form in SCH_REFERENCE, so it lands on the symbol even with
// a variant selected and still has to reach the sheet paths the scope hides.
BOOST_FIXTURE_TEST_CASE( SheetScopedBoardExclusionSurvivesApply, ISSUE25112_FIXTURE )
{
    const wxString variant = wxS( "Assembly" );

    std::unique_ptr<SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL> model =
            MakeScopedModel( variant, wxS( "${EXCLUDE_FROM_BOARD}" ) );

    BOOST_REQUIRE( !m_symbol->GetExcludedFromBoard() );

    model->SetValue( m_row, m_col, wxS( "1" ) );
    Apply( *model );

    BOOST_CHECK( m_symbol->GetExcludedFromBoard() );
}


BOOST_FIXTURE_TEST_CASE( UntouchedMissingPresetFieldRemainsAbsent, ISSUE25112_FIXTURE )
{
    const wxString fieldName = wxS( "UninstantiatedPresetField" );

    std::unique_ptr<SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL> model =
            MakeScopedModel( wxEmptyString, fieldName );

    BOOST_REQUIRE( model->IsCellClear( m_row, m_col ) );

    Apply( *model );

    BOOST_CHECK( m_symbol->GetField( fieldName ) == nullptr );
}


BOOST_FIXTURE_TEST_CASE( ExplicitEmptyValueCreatesField, ISSUE25112_FIXTURE )
{
    const wxString fieldName = wxS( "ExplicitlyEmptyField" );

    std::unique_ptr<SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL> model =
            MakeScopedModel( wxEmptyString, fieldName );

    BOOST_REQUIRE( model->IsCellClear( m_row, m_col ) );

    model->SetValue( m_row, m_col, wxEmptyString );

    BOOST_REQUIRE( !model->IsCellClear( m_row, m_col ) );
    BOOST_REQUIRE( model->IsCellEdited( m_row, m_col ) );

    Apply( *model );

    const SCH_FIELD* field = m_symbol->GetField( fieldName );
    BOOST_REQUIRE( field );
    BOOST_CHECK( field->GetText().IsEmpty() );
}


BOOST_FIXTURE_TEST_CASE( ExistingEmptyFieldCanBeCleared, ISSUE25112_FIXTURE )
{
    const wxString fieldName = wxS( "ExistingEmptyField" );

    std::unique_ptr<SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL> model =
            MakeScopedModel( wxEmptyString, fieldName );

    m_symbol->AddField( SCH_FIELD( m_symbol, FIELD_T::USER, fieldName ) );
    model->UpdateReferences( m_refs );

    BOOST_REQUIRE( !model->IsCellClear( m_row, m_col ) );
    BOOST_REQUIRE( !model->IsCellEdited( m_row, m_col ) );

    model->ClearCell( m_row, m_col );

    BOOST_REQUIRE( model->IsCellClear( m_row, m_col ) );
    BOOST_REQUIRE( model->IsCellEdited( m_row, m_col ) );

    Apply( *model );

    BOOST_CHECK( m_symbol->GetField( fieldName ) == nullptr );
}


BOOST_FIXTURE_TEST_CASE( UserAddedColumnCreatesEmptyField, ISSUE25112_FIXTURE )
{
    const wxString fieldName = wxS( "UserAddedEmptyField" );

    std::unique_ptr<SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL> model =
            MakeScopedModel( wxEmptyString, fieldName, true );

    BOOST_REQUIRE( !model->IsCellClear( m_row, m_col ) );
    BOOST_REQUIRE( model->IsCellEdited( m_row, m_col ) );

    Apply( *model );

    const SCH_FIELD* field = m_symbol->GetField( fieldName );
    BOOST_REQUIRE( field );
    BOOST_CHECK( field->GetText().IsEmpty() );
}


BOOST_FIXTURE_TEST_CASE( RevertingVariantFieldCreationRestoresAbsence, ISSUE25112_FIXTURE )
{
    const wxString fieldName = wxS( "RevertedVariantField" );

    std::unique_ptr<SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL> model =
            MakeScopedModel( wxS( "Assembly" ), fieldName );

    model->SetValue( m_row, m_col, wxEmptyString );
    BOOST_REQUIRE( !model->IsCellClear( m_row, m_col ) );

    model->RevertRow( m_row );

    BOOST_REQUIRE( model->IsCellClear( m_row, m_col ) );
    BOOST_REQUIRE( !model->IsEdited() );

    Apply( *model );

    BOOST_CHECK( m_symbol->GetField( fieldName ) == nullptr );
}


// Repose #2771: refresh every instance of a moved symbol without replacing pending fields.
BOOST_FIXTURE_TEST_CASE( RefreshPreservesStagedFieldsAcrossSharedSheetPaths, ISSUE25112_FIXTURE )
{
    const wxString name = GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED );
    auto           model = MakeScopedModel( wxEmptyString, name );
    m_symbol->GetField( FIELD_T::VALUE )->SetText( wxS( "base" ) );
    model->UpdateReferences( m_refs );
    model->SetValue( m_row, m_col, wxS( "staged" ) );

    m_symbol->SetPosition( VECTOR2I( 100, 200 ) );
    model->UpdateReferences( m_refs );
    BOOST_CHECK_EQUAL( model->GetValue( m_row, m_col ), wxS( "staged" ) );
    BOOST_CHECK( model->IsEdited() );
    Apply( *model );
    BOOST_CHECK_EQUAL( m_symbol->GetField( FIELD_T::VALUE )->GetText(), wxS( "staged" ) );
    BOOST_CHECK( !model->IsEdited() );

    model->SetValue( m_row, m_col, wxS( "another edit" ) );
    m_symbol->GetField( FIELD_T::VALUE )->SetText( wxS( "external" ) );
    model->UpdateReferences( m_refs );
    BOOST_CHECK_EQUAL( model->GetValue( m_row, m_col ), wxS( "external" ) );
    BOOST_CHECK( !model->IsEdited() );
}


BOOST_FIXTURE_TEST_CASE( RefreshPreservesFieldPresenceEditsAcrossVariantsAndPaths, ISSUE25112_FIXTURE )
{
    const wxString name = wxS( "PresenceAcrossVariants" );
    auto           model = MakeScopedModel( wxS( "A" ), name );
    m_symbol->AddField( SCH_FIELD( m_symbol, FIELD_T::USER, name ) );
    model->UpdateReferences( m_refs );
    model->ClearCell( m_row, m_col );
    model->SetCurrentVariant( wxS( "B" ) );
    model->UpdateReferences( m_refs );
    BOOST_CHECK( model->IsCellClear( m_row, m_col ) );
    Apply( *model );
    BOOST_CHECK( !m_symbol->GetField( name ) );

    model->SetValue( m_row, m_col, wxEmptyString );
    model->UpdateReferences( m_refs );
    BOOST_CHECK( !model->IsCellClear( m_row, m_col ) );
    Apply( *model );
    BOOST_REQUIRE( m_symbol->GetField( name ) );
    BOOST_CHECK( m_symbol->GetField( name )->GetText().IsEmpty() );
}


// Repose #1699 and #4001: Add Variant must activate fresh values while retaining old edits.
BOOST_FIXTURE_TEST_CASE( NewlyAddedVariantDoesNotReceivePreviousVariantsValues, ISSUE25112_FIXTURE )
{
    const wxString name = GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED );
    m_schematic->AddVariant( wxS( "A" ) );
    auto       model = MakeScopedModel( wxS( "A" ), name );
    SCH_FIELD* field = m_symbol->GetField( FIELD_T::VALUE );
    field->SetText( wxS( "base" ) );
    field->SetText( wxS( "A live" ), &m_scopePath, wxS( "A" ) );
    model->UpdateReferences( m_refs );
    model->SetValue( m_row, m_col, wxS( "A staged" ) );

    m_schematic->AddVariant( wxS( "New" ) );
    m_schematic->SetCurrentVariant( wxS( "New" ) );
    model->SetCurrentVariant( wxS( "New" ) );
    BOOST_CHECK_EQUAL( model->GetValue( m_row, m_col ), wxS( "base" ) );
    BOOST_CHECK_EQUAL( field->GetText( &m_scopePath, wxS( "A" ) ), wxS( "A live" ) );
    BOOST_CHECK( model->IsEdited() );

    Apply( *model );
    BOOST_CHECK_EQUAL( field->GetText( &m_scopePath, wxS( "New" ) ), wxS( "base" ) );
    BOOST_CHECK_EQUAL( field->GetText( &m_scopePath, wxS( "A" ) ), wxS( "A staged" ) );
    BOOST_CHECK_EQUAL( field->GetText( &m_siblingPath, wxS( "A" ) ), wxS( "base" ) );
    BOOST_CHECK( !model->IsEdited() );
}


BOOST_FIXTURE_TEST_CASE( VariantRoundTripRetainsEditsAndCancelLeavesSchematicAlone, ISSUE25112_FIXTURE )
{
    const wxString name = GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED );
    auto           model = MakeScopedModel( wxEmptyString, name );
    SCH_FIELD*     field = m_symbol->GetField( FIELD_T::VALUE );
    field->SetText( wxS( "base" ) );
    model->UpdateReferences( m_refs );
    model->SetCurrentVariant( wxS( "A" ) );
    model->SetValue( m_row, m_col, wxS( "A staged" ) );
    model->SetCurrentVariant( wxS( "B" ) );
    model->SetValue( m_row, m_col, wxS( "B staged" ) );
    model->SetCurrentVariant( wxS( "A" ) );
    BOOST_CHECK_EQUAL( model->GetValue( m_row, m_col ), wxS( "A staged" ) );
    model->SetCurrentVariant( wxS( "B" ) );
    BOOST_CHECK_EQUAL( model->GetValue( m_row, m_col ), wxS( "B staged" ) );
    model.reset();
    BOOST_CHECK_EQUAL( field->GetText( &m_scopePath, wxS( "A" ) ), wxS( "base" ) );
    BOOST_CHECK_EQUAL( field->GetText( &m_scopePath, wxS( "B" ) ), wxS( "base" ) );
}


BOOST_FIXTURE_TEST_CASE( RenamingAndDeletingVariantsRetargetsPendingEdits, ISSUE25112_FIXTURE )
{
    const wxString name = GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED );
    auto           model = MakeScopedModel( wxEmptyString, name );
    SCH_FIELD*     field = m_symbol->GetField( FIELD_T::VALUE );
    field->SetText( wxS( "base" ) );
    model->UpdateReferences( m_refs );
    m_schematic->AddVariant( wxS( "A" ) );
    model->SetCurrentVariant( wxS( "A" ) );
    model->SetValue( m_row, m_col, wxS( "A staged" ) );
    model->RenameStoredVariant( wxS( "A" ), wxS( "Renamed" ) );
    m_schematic->RenameVariant( wxS( "A" ), wxS( "Renamed" ) );
    BOOST_CHECK_EQUAL( model->GetCurrentVariant(), wxS( "Renamed" ) );
    BOOST_CHECK_EQUAL( model->GetValue( m_row, m_col ), wxS( "A staged" ) );

    m_schematic->AddVariant( wxS( "Deleted" ) );
    model->SetCurrentVariant( wxS( "Deleted" ) );
    model->SetValue( m_row, m_col, wxS( "discarded" ) );
    model->DeleteStoredVariant( wxS( "Deleted" ) );
    m_schematic->DeleteVariant( wxS( "Deleted" ) );
    model->SetCurrentVariant( wxS( "Renamed" ) );
    Apply( *model );

    BOOST_CHECK_EQUAL( field->GetText( &m_scopePath, wxS( "Renamed" ) ), wxS( "A staged" ) );
    BOOST_CHECK_EQUAL( field->GetText( &m_scopePath, wxS( "Deleted" ) ), wxS( "base" ) );
}


BOOST_FIXTURE_TEST_CASE( RevertingVariantKeepsBaseEditsConsistentAcrossPaths, ISSUE25112_FIXTURE )
{
    const wxString name = GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED );
    auto           model = MakeScopedModel( wxEmptyString, name );
    SCH_FIELD*     field = m_symbol->GetField( FIELD_T::VALUE );
    field->SetText( wxS( "base" ) );
    model->UpdateReferences( m_refs );
    model->SetValue( m_row, m_col, wxS( "staged base" ) );
    model->SetCurrentVariant( wxS( "A" ) );
    model->SetValue( m_row, m_col, wxS( "discarded variant edit" ) );
    model->RevertRow( m_row );
    BOOST_CHECK( model->IsEdited() );
    model->SetCurrentVariant( wxEmptyString );
    BOOST_CHECK_EQUAL( model->GetValue( m_row, m_col ), wxS( "staged base" ) );

    Apply( *model );
    BOOST_CHECK_EQUAL( field->GetText(), wxS( "staged base" ) );
    BOOST_CHECK_EQUAL( field->GetText( &m_scopePath, wxS( "A" ) ), wxS( "staged base" ) );
    BOOST_CHECK_EQUAL( field->GetText( &m_siblingPath, wxS( "A" ) ), wxS( "staged base" ) );
    BOOST_CHECK( !model->IsEdited() );
}
