/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 CERN
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <dialog_ibis_parser_reporter.h>
#include <dialog_sim_model.h>
#include <sim/sim_property.h>
#include <sim/sim_library_kibis.h>
#include <sim/sim_library_spice.h>
#include <sim/sim_model.h>
#include <sim/sim_model_kibis.h>
#include <sim/sim_model_raw_spice.h>
#include <sim/sim_model_spice_fallback.h>
#include <sim/sim_model_subckt.h>
#include <grid_tricks.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <kiplatform/ui.h>
#include <confirm.h>
#include <string_utils.h>
#include <locale_io.h>
#include <wx/filedlg.h>
#include <wx/textfile.h>
#include <fmt/format.h>

using CATEGORY = SIM_MODEL::PARAM::CATEGORY;


template <typename T_symbol, typename T_field>
DIALOG_SIM_MODEL<T_symbol, T_field>::DIALOG_SIM_MODEL( wxWindow* aParent, T_symbol& aSymbol,
                                                       std::vector<T_field>& aFields )
    : DIALOG_SIM_MODEL_BASE( aParent ),
      m_symbol( aSymbol ),
      m_fields( aFields ),
      m_libraryModelsMgr( &Prj() ),
      m_builtinModelsMgr( &Prj() ),
      m_prevModel( nullptr ),
      m_curModelType( SIM_MODEL::TYPE::NONE ),
      m_scintillaTricks( nullptr ),
      m_firstCategory( nullptr ),
      m_prevParamGridSelection( nullptr ),
      m_lastParamGridWidth( 0 ),
      m_inKillFocus( false )
{
    m_browseButton->SetBitmap( KiBitmap( BITMAPS::small_folder ) );

    m_sortedPartPins = m_symbol.GetAllLibPins();

    std::sort( m_sortedPartPins.begin(), m_sortedPartPins.end(),
               []( const LIB_PIN* lhs, const LIB_PIN* rhs )
               {
                   // We sort by StrNumCmp because SIM_MODEL_BASE sorts with it too.
                   return StrNumCmp( lhs->GetNumber(), rhs->GetNumber(), true ) < 0;
               } );


    m_typeChoice->Clear();

    m_scintillaTricks = new SCINTILLA_TRICKS( m_codePreview, wxT( "{}" ), false );

    m_paramGridMgr->Bind( wxEVT_PG_SELECTED, &DIALOG_SIM_MODEL::onParamGridSelectionChange, this );

    m_paramGrid->SetValidationFailureBehavior( wxPG_VFB_STAY_IN_PROPERTY
                                               | wxPG_VFB_BEEP
                                               | wxPG_VFB_MARK_CELL );

    wxPropertyGrid* grid = m_paramGrid->GetGrid();

    //grid->SetCellBackgroundColour( grid->GetPropertyDefaultCell().GetBgCol() );
    //grid->SetCellTextColour( grid->GetPropertyDefaultCell().GetFgCol();

    // In wx 3.0 the color will be wrong sometimes.
    grid->SetCellDisabledTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    grid->Bind( wxEVT_SET_FOCUS, &DIALOG_SIM_MODEL::onParamGridSetFocus, this );

    grid->AddActionTrigger( wxPG_ACTION_EDIT, WXK_RETURN );
    grid->DedicateKey( WXK_RETURN );
    grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_RETURN );

    grid->DedicateKey( WXK_UP );
    grid->DedicateKey( WXK_DOWN );

    m_pinAssignmentsGrid->PushEventHandler( new GRID_TRICKS( m_pinAssignmentsGrid ) );

    m_subcktLabel->SetFont( KIUI::GetInfoFont( m_subcktLabel ) );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


template <typename T_symbol, typename T_field>
DIALOG_SIM_MODEL<T_symbol, T_field>::~DIALOG_SIM_MODEL()
{
    // Disable all properties. This is necessary because some of their methods are called after
    // destruction of DIALOG_SIM_MODEL, oddly. When disabled, they never access their models.
    for( wxPropertyGridIterator it = m_paramGrid->GetIterator(); !it.AtEnd(); ++it )
    {
        SIM_PROPERTY* prop = dynamic_cast<SIM_PROPERTY*>( *it );

        if( !prop )
            continue;

        prop->Disable();
    }

    // Delete the GRID_TRICKS.
    m_pinAssignmentsGrid->PopEventHandler( true );

    delete m_scintillaTricks;
}


template <typename T_symbol, typename T_field>
bool DIALOG_SIM_MODEL<T_symbol, T_field>::TransferDataToWindow()
{
    wxCommandEvent dummyEvent;
    wxString       deviceType;
    wxString       modelType;
    wxString       modelParams;
    wxString       pinMap;
    bool           storeInValue = false;

    // Infer RLC and VI models if they aren't specified
    if( SIM_MODEL::InferSimModel( m_symbol, &m_fields, false, SIM_VALUE_GRAMMAR::NOTATION::SI,
                                  &deviceType, &modelType, &modelParams, &pinMap ) )
    {
        m_fields.emplace_back( &m_symbol, -1, SIM_MODEL::DEVICE_TYPE_FIELD );
        m_fields.back().SetText( deviceType );

        if( !modelType.IsEmpty() )
        {
            m_fields.emplace_back( &m_symbol, -1, SIM_MODEL::TYPE_FIELD );
            m_fields.back().SetText( modelType );
        }

        m_fields.emplace_back( &m_symbol, -1, SIM_MODEL::PARAMS_FIELD );
        m_fields.back().SetText( modelParams );

        m_fields.emplace_back( &m_symbol, -1, SIM_MODEL::PINS_FIELD );
        m_fields.back().SetText( pinMap );

        storeInValue = true;

        // In case the storeInValue checkbox is turned off (if it's left on then we'll overwrite
        // this field with the actual value):
        m_fields[ VALUE_FIELD ].SetText( wxT( "${SIM.PARAMS}" ) );
    }

    std::string libraryFilename = SIM_MODEL::GetFieldValue( &m_fields, SIM_LIBRARY::LIBRARY_FIELD );

    if( libraryFilename != "" )
    {
        // The model is sourced from a library, optionally with instance overrides.
        m_useLibraryModelRadioButton->SetValue( true );
        loadLibrary( libraryFilename );

        // Must be set before curModel() is used since the latter checks the combobox value.
        std::string modelName = SIM_MODEL::GetFieldValue( &m_fields, SIM_LIBRARY::NAME_FIELD );
        int         modelIdx = m_modelNameChoice->FindString( modelName );

        if( modelIdx == wxNOT_FOUND )
        {
            DisplayErrorMessage( this, wxString::Format( _( "No model named '%s' in library." ),
                                                         modelName ) );

            // Default to first item in library
            m_modelNameChoice->SetSelection( 0 );
        }
        else
        {
            m_modelNameChoice->SetSelection( modelIdx );
        }

        if( isIbisLoaded() && ( m_modelNameChoice->GetSelection() >= 0 ) )
        {
            SIM_MODEL_KIBIS* kibismodel = dynamic_cast<SIM_MODEL_KIBIS*>(
                    &m_libraryModelsMgr.GetModels().at( m_modelNameChoice->GetSelection() ).get() );

            if( kibismodel )
            {
                onModelNameChoice( dummyEvent ); // refresh list of pins

                int i = 0;

                for( const std::pair<std::string, std::string>& strs : kibismodel->GetIbisPins() )
                {
                    if( strs.first
                        == SIM_MODEL::GetFieldValue( &m_fields, SIM_LIBRARY_KIBIS::PIN_FIELD ) )
                    {
                        auto kibisLibrary = static_cast<const SIM_LIBRARY_KIBIS*>( library() );

                        kibismodel->ChangePin( *kibisLibrary, strs.first );
                        m_ibisPinCombobox->SetSelection( static_cast<int>( i ) );
                        break;
                    }
                    i++;
                }

                if( i < static_cast<int>( kibismodel->GetIbisPins().size() ) )
                {
                    onIbisPinCombobox( dummyEvent ); // refresh list of models

                    m_ibisModelCombobox->SetStringSelection(
                            SIM_MODEL::GetFieldValue( &m_fields, SIM_LIBRARY_KIBIS::MODEL_FIELD ) );
                }

                if( SIM_MODEL::GetFieldValue( &m_fields, SIM_LIBRARY_KIBIS::DIFF_FIELD ) == "1" )
                {
                    kibismodel->SwitchSingleEndedDiff( true );
                    m_differentialCheckbox->SetValue( true );
                }
                else
                {
                    kibismodel->SwitchSingleEndedDiff( false );
                    m_differentialCheckbox->SetValue( false );
                }
            }
        }

        m_curModelType = curModel().GetType();
    }
    else
    {
        // The model is sourced from the instance.
        m_useInstanceModelRadioButton->SetValue( true );
        m_curModelType = SIM_MODEL::ReadTypeFromFields( m_fields );
    }

    std::vector<LIB_PIN*> sourcePins = m_symbol.GetAllLibPins();

    std::sort( sourcePins.begin(), sourcePins.end(),
               []( const LIB_PIN* lhs, const LIB_PIN* rhs )
               {
                   return StrNumCmp( lhs->GetNumber(), rhs->GetNumber(), true ) < 0;
               } );

    for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
    {
        wxString           msg;
        WX_STRING_REPORTER reporter( &msg );

        m_builtinModelsMgr.SetReporter( &reporter );

        if( m_useInstanceModelRadioButton->GetValue() && type == m_curModelType )
            m_builtinModelsMgr.CreateModel( m_fields, sourcePins, false );
        else
            m_builtinModelsMgr.CreateModel( type, sourcePins );

        if( reporter.HasMessage() )
        {
            DisplayErrorMessage( this, _( "Failed to read simulation model from fields." )
                                       + wxT( "\n\n" ) + msg );
        }

        SIM_MODEL::DEVICE_T deviceTypeT = SIM_MODEL::TypeInfo( type ).deviceType;

        if( !m_curModelTypeOfDeviceType.count( deviceTypeT ) )
            m_curModelTypeOfDeviceType[deviceTypeT] = type;
    }

    curModel().SetIsStoredInValue( storeInValue );

    m_saveInValueCheckbox->SetValue( curModel().IsStoredInValue() );

    onRadioButton( dummyEvent );
    return DIALOG_SIM_MODEL_BASE::TransferDataToWindow();
}


template <typename T_symbol, typename T_field>
bool DIALOG_SIM_MODEL<T_symbol, T_field>::TransferDataFromWindow()
{
    m_pinAssignmentsGrid->CommitPendingChanges();

    // This should have been done in wxPGTextCtrlEditor::OnTextCtrlEvent(), but something must
    // be clearing it before we get here, resulting in CommitChangesFromEditor() doing nothing
    m_paramGrid->GetGrid()->EditorsValueWasModified();
    m_paramGrid->GetGrid()->CommitChangesFromEditor();

    if( !DIALOG_SIM_MODEL_BASE::TransferDataFromWindow() )
        return false;

    std::string modelName = m_modelNameChoice->GetStringSelection().ToStdString();

    SIM_MODEL::SetFieldValue( m_fields, SIM_LIBRARY::NAME_FIELD, modelName );

    std::string path;

    if( ( library() && m_useLibraryModelRadioButton->GetValue() ) || isIbisLoaded() )
    {
        path = m_libraryPathText->GetValue();
        wxFileName fn( path );

        if( fn.MakeRelativeTo( Prj().GetProjectPath() ) && !fn.GetFullPath().StartsWith( ".." ) )
            path = fn.GetFullPath();
    }

    SIM_MODEL::SetFieldValue( m_fields, SIM_LIBRARY::LIBRARY_FIELD, path );

    if( isIbisLoaded() )
    {
        SIM_MODEL_KIBIS* kibismodel = dynamic_cast<SIM_MODEL_KIBIS*>(
                &m_libraryModelsMgr.GetModels().at( m_modelNameChoice->GetSelection() ).get() );

        if( kibismodel )
        {
            SIM_MODEL::SetFieldValue(
                    m_fields, SIM_LIBRARY_KIBIS::PIN_FIELD,
                    kibismodel->GetIbisPins().at( m_ibisPinCombobox->GetSelection() ).first );

            SIM_MODEL::SetFieldValue(
                    m_fields, SIM_LIBRARY_KIBIS::MODEL_FIELD,
                    std::string( m_ibisModelCombobox->GetValue().c_str() ) );

            SIM_MODEL::SetFieldValue(
                    m_fields, SIM_LIBRARY_KIBIS::DIFF_FIELD,
                    ( kibismodel->CanDifferential() && m_differentialCheckbox->GetValue() ) ? "1" : "" );
        }
    }

    curModel().WriteFields( m_fields );

    return true;
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::updateWidgets()
{
    updateIbisWidgets();
    updateInstanceWidgets();
    updateModelParamsTab();
    updateModelCodeTab();
    updatePinAssignments();

    std::string ref = SIM_MODEL::GetFieldValue( &m_fields, SIM_MODEL::REFERENCE_FIELD );

    m_modelPanel->Layout();
    m_pinAssignmentsPanel->Layout();
    m_parametersPanel->Layout();
    m_codePanel->Layout();

    SendSizeEvent( wxSEND_EVENT_POST );

    m_prevModel = &curModel();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::updateIbisWidgets()
{
    SIM_MODEL_KIBIS* modelkibis = isIbisLoaded() ? dynamic_cast<SIM_MODEL_KIBIS*>( &curModel() )
                                                 : nullptr;

    m_ibisModelCombobox->Show( isIbisLoaded() );
    m_ibisPinCombobox->Show( isIbisLoaded() );
    m_ibisModelLabel->Show( isIbisLoaded() );
    m_ibisPinLabel->Show( isIbisLoaded() );

    m_differentialCheckbox->Show( isIbisLoaded() && modelkibis && modelkibis->CanDifferential() );
    m_modelNameLabel->SetLabel( isIbisLoaded() ? _( "Component:" ) : _( "Model:" ) );
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::updateInstanceWidgets()
{
    // Change the Type choice to match the current device type.
    if( &curModel() != m_prevModel )
    {
        m_deviceTypeChoice->Clear();

        if( m_useLibraryModelRadioButton->GetValue() )
        {
            m_deviceTypeChoice->Append( curModel().GetDeviceInfo().description );
            m_deviceTypeChoice->SetSelection( 0 );
        }
        else
        {
            for( SIM_MODEL::DEVICE_T deviceType : SIM_MODEL::DEVICE_T_ITERATOR() )
            {
                if( !SIM_MODEL::DeviceInfo( deviceType ).isBuiltin )
                    continue;

                m_deviceTypeChoice->Append( SIM_MODEL::DeviceInfo( deviceType ).description );

                if( deviceType == curModel().GetDeviceType() )
                    m_deviceTypeChoice->SetSelection( m_deviceTypeChoice->GetCount() - 1 );
            }
        }

        m_typeChoice->Clear();

        for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
        {
            if( SIM_MODEL::TypeInfo( type ).deviceType == curModel().GetDeviceType() )
            {
                m_typeChoice->Append( SIM_MODEL::TypeInfo( type ).description );

                if( type == curModel().GetType() )
                    m_typeChoice->SetSelection( m_typeChoice->GetCount() - 1 );
            }
        }
    }

    m_typeChoice->Enable( !m_useLibraryModelRadioButton->GetValue() || isIbisLoaded() );

    if( dynamic_cast<SIM_MODEL_RAW_SPICE*>( &curModel() )
            || dynamic_cast<SIM_MODEL_SPICE_FALLBACK*>( &curModel() ) )
    {
        m_modelNotebook->SetSelection( 1 );
    }
    else
    {
        m_modelNotebook->SetSelection( 0 );
    }

    if( curModel().HasPrimaryValue() )
    {
        const SIM_MODEL::PARAM& primary = curModel().GetParam( 0 );

        m_saveInValueCheckbox->SetLabel( wxString::Format( _( "Save parameter '%s (%s)' in Value "
                                                              "field" ),
                                                           primary.info.description,
                                                           primary.info.name ) );
        m_saveInValueCheckbox->Enable( true );
    }
    else
    {
        m_saveInValueCheckbox->SetLabel( _( "Save primary parameter in Value field" ) );
        m_saveInValueCheckbox->SetValue( false );
        m_saveInValueCheckbox->Enable( false );
    }
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::updateModelParamsTab()
{
    if( &curModel() != m_prevModel )
    {
        // This wxPropertyGridManager column and header stuff has to be here because it segfaults in
        // the constructor.

        m_paramGridMgr->SetColumnCount( PARAM_COLUMN::END_ );

        m_paramGridMgr->SetColumnTitle( PARAM_COLUMN::DESCRIPTION, _( "Parameter" ) );
        m_paramGridMgr->SetColumnTitle( PARAM_COLUMN::UNIT, _( "Unit" ) );
        m_paramGridMgr->SetColumnTitle( PARAM_COLUMN::DEFAULT, _( "Default" ) );
        m_paramGridMgr->SetColumnTitle( PARAM_COLUMN::TYPE, _( "Type" ) );

        m_paramGridMgr->ShowHeader();


        m_paramGrid->Clear();

        m_firstCategory = m_paramGrid->Append( new wxPropertyCategory( "Geometry" ) );
        m_paramGrid->HideProperty( "Geometry" );

        m_paramGrid->Append( new wxPropertyCategory( "AC" ) );
        m_paramGrid->HideProperty( "AC" );

        m_paramGrid->Append( new wxPropertyCategory( "DC" ) );
        m_paramGrid->HideProperty( "DC" );

        m_paramGrid->Append( new wxPropertyCategory( "Capacitance" ) );
        m_paramGrid->HideProperty( "Capacitance" );

        m_paramGrid->Append( new wxPropertyCategory( "Temperature" ) );
        m_paramGrid->HideProperty( "Temperature" );

        m_paramGrid->Append( new wxPropertyCategory( "Noise" ) );
        m_paramGrid->HideProperty( "Noise" );

        m_paramGrid->Append( new wxPropertyCategory( "Distributed Quantities" ) );
        m_paramGrid->HideProperty( "Distributed Quantities" );

        m_paramGrid->Append( new wxPropertyCategory( "Waveform" ) );
        m_paramGrid->HideProperty( "Waveform" );

        m_paramGrid->Append( new wxPropertyCategory( "Limiting Values" ) );
        m_paramGrid->HideProperty( "Limiting Values" );

        m_paramGrid->Append( new wxPropertyCategory( "Advanced" ) );
        m_paramGrid->HideProperty( "Advanced" );

        m_paramGrid->Append( new wxPropertyCategory( "Flags" ) );
        m_paramGrid->HideProperty( "Flags" );

        m_paramGrid->CollapseAll();

        for( int i = 0; i < curModel().GetParamCount(); ++i )
            addParamPropertyIfRelevant( i );

        m_paramGrid->CollapseAll();
        m_paramGrid->Expand( "AC" );
        m_paramGrid->Expand( "Waveform" );
    }

    adjustParamGridColumns( m_paramGrid->GetGrid()->GetSize().GetX(), true );

    // Set all properties to default colors.
    // Update properties in models that have autofill.
    for( wxPropertyGridIterator it = m_paramGrid->GetIterator(); !it.AtEnd(); ++it )
    {
        wxColour bgCol = m_paramGrid->GetGrid()->GetPropertyDefaultCell().GetBgCol();
        wxColour fgCol = m_paramGrid->GetGrid()->GetPropertyDefaultCell().GetFgCol();

        for( int col = 0; col < m_paramGridMgr->GetColumnCount(); ++col )
        {
            ( *it )->GetCell( col ).SetBgCol( bgCol );
            ( *it )->GetCell( col ).SetFgCol( fgCol );
        }

        SIM_PROPERTY* prop = dynamic_cast<SIM_PROPERTY*>( *it );

        if( !prop )
            continue;

        const SIM_MODEL::PARAM& param = prop->GetParam();

        // Model values other than the currently edited value may have changed. Update them.
        // This feature is called "autofill" and present only in certain models. Don't do it for
        // models that don't have it for performance reasons.
        if( curModel().HasAutofill() )
            ( *it )->SetValueFromString( param.value->ToString() );
    }
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::updateModelCodeTab()
{
    wxString   text;
    wxString   pin( _( "Pin" ) );
    SPICE_ITEM item;
    item.modelName = m_modelNameChoice->GetStringSelection();

    for( size_t ii = 1; ii <= m_symbol.GetFullPinCount(); ++ii )
    {
        item.pinNumbers.push_back( fmt::format( "{}", ii ) );
        item.pinNetNames.push_back( pin.ToStdString() + fmt::format( "{}", ii ) );
    }

    if( m_useInstanceModelRadioButton->GetValue() || item.modelName == "" )
        item.modelName = m_fields.at( REFERENCE_FIELD ).GetText();

    SIM_MODEL& model = curModel();

    text << model.SpiceGenerator().Preview( item );

    m_codePreview->SetText( text );
    m_codePreview->SelectNone();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::updatePinAssignments()
{
    removeOrphanedPinAssignments();

    // Reset the grid.

    m_pinAssignmentsGrid->ClearRows();
    m_pinAssignmentsGrid->AppendRows( static_cast<int>( m_sortedPartPins.size() ) );

    for( int row = 0; row < m_pinAssignmentsGrid->GetNumberRows(); ++row )
        m_pinAssignmentsGrid->SetCellValue( row, PIN_COLUMN::MODEL, _( "Not Connected" ) );

    // Now set up the grid values in the Model column.
    for( int modelPinIndex = 0; modelPinIndex < curModel().GetPinCount(); ++modelPinIndex )
    {
        wxString symbolPinNumber = curModel().GetPin( modelPinIndex ).symbolPinNumber;

        if( symbolPinNumber == "" )
            continue;

        int symbolPinRow = findSymbolPinRow( symbolPinNumber );

        if( symbolPinRow == -1 )
            continue;

        wxString modelPinString = getModelPinString( modelPinIndex );
        m_pinAssignmentsGrid->SetCellValue( symbolPinRow, PIN_COLUMN::MODEL, modelPinString );
    }

    // Set up the Symbol column grid values and Model column cell editors with dropdown options.
    for( int ii = 0; ii < m_pinAssignmentsGrid->GetNumberRows(); ++ii )
    {
        wxString symbolPinString = getSymbolPinString( ii );

        m_pinAssignmentsGrid->SetReadOnly( ii, PIN_COLUMN::SYMBOL );
        m_pinAssignmentsGrid->SetCellValue( ii, PIN_COLUMN::SYMBOL, symbolPinString );

        wxString curModelPinString = m_pinAssignmentsGrid->GetCellValue( ii, PIN_COLUMN::MODEL );

        std::vector<BITMAPS> modelPinIcons;
        wxArrayString        modelPinChoices;

        for( int jj = 0; jj < curModel().GetPinCount(); ++jj )
        {
            if( curModel().GetPin( jj ).symbolPinNumber != "" )
                modelPinIcons.push_back( PinShapeGetBitmap( GRAPHIC_PINSHAPE::LINE ) );
            else
                modelPinIcons.push_back( BITMAPS::INVALID_BITMAP );

            modelPinChoices.Add( getModelPinString( jj ) );
        }

        modelPinIcons.push_back( BITMAPS::INVALID_BITMAP );
        modelPinChoices.Add( _( "Not Connected" ) );

        // Using `new` here shouldn't cause a memory leak because `SetCellEditor()` calls
        // `DecRef()` on its last editor.
        m_pinAssignmentsGrid->SetCellEditor( ii, PIN_COLUMN::MODEL,
                                             new GRID_CELL_ICON_TEXT_POPUP( modelPinIcons,
                                                                            modelPinChoices ) );
    }

    // TODO: Show a preview of the symbol with the pin numbers shown.

    SIM_MODEL* model = &curModel();

    if( model->GetType() == SIM_MODEL::TYPE::SUBCKT )
    {
        SIM_MODEL_SUBCKT* subckt = static_cast<SIM_MODEL_SUBCKT*>( model );
        m_subckt->SetText( subckt->GetSpiceCode() );
    }
    else
    {
        m_subcktLabel->Show( false );
        m_subckt->Show( false );
    }
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::removeOrphanedPinAssignments()
{
    for( int i = 0; i < curModel().GetPinCount(); ++i )
    {
        if( !m_symbol.GetPin( curModel().GetPin( i ).symbolPinNumber ) )
            curModel().SetPinSymbolPinNumber( i, "" );
    }
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::loadLibrary( const wxString& aLibraryPath,
                                                       bool aForceReload )
{
    auto libraries = m_libraryModelsMgr.GetLibraries();

    wxString           msg;
    WX_STRING_REPORTER reporter( &msg );

    m_libraryModelsMgr.SetReporter( &reporter );
    m_libraryModelsMgr.SetLibrary( aLibraryPath );

    if( reporter.HasMessage() )
    {
        DisplayErrorMessage( this, msg );
        return;
    }

    std::vector<LIB_PIN*> sourcePins = m_symbol.GetAllLibPins();

    std::sort( sourcePins.begin(), sourcePins.end(),
               []( const LIB_PIN* lhs, const LIB_PIN* rhs )
               {
                   return StrNumCmp( lhs->GetNumber(), rhs->GetNumber(), true ) < 0;
               } );

    std::string modelName = SIM_MODEL::GetFieldValue( &m_fields, SIM_LIBRARY::NAME_FIELD );

    for( auto& [baseModelName, baseModel] : library()->GetModels() )
    {
        if( baseModelName == modelName )
            m_libraryModelsMgr.CreateModel( &baseModel, sourcePins, m_fields );
        else
            m_libraryModelsMgr.CreateModel( &baseModel, sourcePins );
    }

    if( reporter.HasMessage() )
        DisplayErrorMessage( this, msg );

    m_useLibraryModelRadioButton->SetValue( true );
    m_libraryPathText->ChangeValue( aLibraryPath );

    wxArrayString modelNames;

    for( auto& [name, model] : library()->GetModels() )
        modelNames.Add( name );

    m_modelNameChoice->Clear();
    m_modelNameChoice->Append( modelNames );

    if( isIbisLoaded() )
    {
        wxArrayString emptyArray;
        m_ibisModelCombobox->Set( emptyArray );
        m_ibisPinCombobox->Set( emptyArray );
        m_ibisModelCombobox->SetSelection( -1 );
        m_ibisPinCombobox->SetSelection( -1 );
    }
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::addParamPropertyIfRelevant( int aParamIndex )
{
    if( curModel().GetParam( aParamIndex ).info.dir == SIM_MODEL::PARAM::DIR_OUT )
        return;

    switch( curModel().GetParam( aParamIndex ).info.category )
    {
    case CATEGORY::AC:
        m_paramGrid->HideProperty( "AC", false );
        m_paramGrid->AppendIn( "AC", newParamProperty( aParamIndex ) );
        break;

    case CATEGORY::DC:
        m_paramGrid->HideProperty( "DC", false );
        m_paramGrid->AppendIn( "DC", newParamProperty( aParamIndex ) );
        break;

    case CATEGORY::CAPACITANCE:
        m_paramGrid->HideProperty( "Capacitance", false );
        m_paramGrid->AppendIn( "Capacitance", newParamProperty( aParamIndex ) );
        break;

    case CATEGORY::TEMPERATURE:
        m_paramGrid->HideProperty( "Temperature", false );
        m_paramGrid->AppendIn( "Temperature", newParamProperty( aParamIndex ) );
        break;

    case CATEGORY::NOISE:
        m_paramGrid->HideProperty( "Noise", false );
        m_paramGrid->AppendIn( "Noise", newParamProperty( aParamIndex ) );
        break;

    case CATEGORY::DISTRIBUTED_QUANTITIES:
        m_paramGrid->HideProperty( "Distributed Quantities", false );
        m_paramGrid->AppendIn( "Distributed Quantities", newParamProperty( aParamIndex ) );
        break;

    case CATEGORY::WAVEFORM:
        m_paramGrid->HideProperty( "Waveform", false );
        m_paramGrid->AppendIn( "Waveform", newParamProperty( aParamIndex ) );
        break;

    case CATEGORY::GEOMETRY:
        m_paramGrid->HideProperty( "Geometry", false );
        m_paramGrid->AppendIn( "Geometry", newParamProperty( aParamIndex ) );
        break;

    case CATEGORY::LIMITING_VALUES:
        m_paramGrid->HideProperty( "Limiting Values", false );
        m_paramGrid->AppendIn( "Limiting Values", newParamProperty( aParamIndex ) );
        break;

    case CATEGORY::ADVANCED:
        m_paramGrid->HideProperty( "Advanced", false );
        m_paramGrid->AppendIn( "Advanced", newParamProperty( aParamIndex ) );
        break;

    case CATEGORY::FLAGS:
        m_paramGrid->HideProperty( "Flags", false );
        m_paramGrid->AppendIn( "Flags", newParamProperty( aParamIndex ) );
        break;

    default:
        m_paramGrid->Insert( m_firstCategory, newParamProperty( aParamIndex ) );
        break;

    case CATEGORY::INITIAL_CONDITIONS:
    case CATEGORY::SUPERFLUOUS:
        return;
    }
}


template <typename T_symbol, typename T_field>
wxPGProperty* DIALOG_SIM_MODEL<T_symbol, T_field>::newParamProperty( int aParamIndex ) const
{
    const SIM_MODEL::PARAM& param = curModel().GetParam( aParamIndex );
    wxString paramDescription;

    if( param.info.description == "" )
        paramDescription = wxString::Format( "%s", param.info.name );
    else
        paramDescription = wxString::Format( "%s (%s)", param.info.description, param.info.name );

    wxPGProperty* prop = nullptr;

    switch( param.info.type )
    {
    case SIM_VALUE::TYPE_BOOL:
        // TODO.
        prop = new SIM_BOOL_PROPERTY( paramDescription, param.info.name, curModel(), aParamIndex );
        prop->SetAttribute( wxPG_BOOL_USE_CHECKBOX, true );
        break;

    case SIM_VALUE::TYPE_INT:
        prop = new SIM_STRING_PROPERTY( paramDescription, param.info.name, curModel(),
                                        aParamIndex, SIM_VALUE::TYPE_INT );
        break;

    case SIM_VALUE::TYPE_FLOAT:
        prop = new SIM_STRING_PROPERTY( paramDescription, param.info.name, curModel(),
                                        aParamIndex, SIM_VALUE::TYPE_FLOAT );
        break;

    //case TYPE_COMPLEX:
    //  break;

    case SIM_VALUE::TYPE_STRING:
        if( param.info.enumValues.empty() )
        {
            prop = new SIM_STRING_PROPERTY( paramDescription, param.info.name, curModel(),
                                            aParamIndex, SIM_VALUE::TYPE_STRING );
        }
        else
        {
            prop = new SIM_ENUM_PROPERTY( paramDescription, param.info.name, curModel(),
                                          aParamIndex );
        }
        break;

    default:
        prop = new wxStringProperty( paramDescription, param.info.name );
        break;
    }

    prop->SetAttribute( wxPG_ATTR_UNITS, wxString::FromUTF8( param.info.unit.c_str() ) );

    // Legacy due to the way we extracted the parameters from Ngspice.
    #if wxCHECK_VERSION( 3, 1, 0 )
        if( param.isOtherVariant )
            prop->SetCell( 3, wxString::FromUTF8( param.info.defaultValueOfOtherVariant ) );
        else
            prop->SetCell( 3, wxString::FromUTF8( param.info.defaultValue ) );
    #else
        if( param.isOtherVariant )
            prop->SetCell( 3, wxString::FromUTF8( param.info.defaultValueOfOtherVariant.c_str() ) );
        else
            prop->SetCell( 3, wxString::FromUTF8( param.info.defaultValue.c_str() ) );
    #endif

    wxString typeStr;

    switch( param.info.type )
    {
    case SIM_VALUE::TYPE_BOOL:           typeStr = wxT( "Bool"           ); break;
    case SIM_VALUE::TYPE_INT:            typeStr = wxT( "Int"            ); break;
    case SIM_VALUE::TYPE_FLOAT:          typeStr = wxT( "Float"          ); break;
    case SIM_VALUE::TYPE_COMPLEX:        typeStr = wxT( "Complex"        ); break;
    case SIM_VALUE::TYPE_STRING:         typeStr = wxT( "String"         ); break;
    case SIM_VALUE::TYPE_BOOL_VECTOR:    typeStr = wxT( "Bool Vector"    ); break;
    case SIM_VALUE::TYPE_INT_VECTOR:     typeStr = wxT( "Int Vector"     ); break;
    case SIM_VALUE::TYPE_FLOAT_VECTOR:   typeStr = wxT( "Float Vector"   ); break;
    case SIM_VALUE::TYPE_COMPLEX_VECTOR: typeStr = wxT( "Complex Vector" ); break;
    }

    prop->SetCell( PARAM_COLUMN::TYPE, typeStr );

    return prop;
}


template <typename T_symbol, typename T_field>
int DIALOG_SIM_MODEL<T_symbol, T_field>::findSymbolPinRow( const wxString& aSymbolPinNumber ) const
{
    for( int row = 0; row < static_cast<int>( m_sortedPartPins.size() ); ++row )
    {
        LIB_PIN* pin = m_sortedPartPins[row];

        if( pin->GetNumber() == aSymbolPinNumber )
            return row;
    }

    return -1;
}


template <typename T_symbol, typename T_field>
SIM_MODEL& DIALOG_SIM_MODEL<T_symbol, T_field>::curModel() const
{
    if( m_useLibraryModelRadioButton->GetValue()
        && m_modelNameChoice->GetSelection() != wxNOT_FOUND )
    {
        return m_libraryModelsMgr.GetModels().at( m_modelNameChoice->GetSelection() ).get();
    }
    else
    {
        return m_builtinModelsMgr.GetModels().at( static_cast<int>( m_curModelType ) );
    }
}


template <typename T_symbol, typename T_field>
const SIM_LIBRARY* DIALOG_SIM_MODEL<T_symbol, T_field>::library() const
{
    if( m_libraryModelsMgr.GetLibraries().size() == 1 )
        return &m_libraryModelsMgr.GetLibraries().begin()->second.get();

    return nullptr;
}


template <typename T_symbol, typename T_field>
wxString DIALOG_SIM_MODEL<T_symbol, T_field>::getSymbolPinString( int symbolPinIndex ) const
{
    LIB_PIN* pin = m_sortedPartPins.at( symbolPinIndex );
    wxString pinNumber;
    wxString pinName;

    if( pin )
    {
        pinNumber = pin->GetShownNumber();
        pinName = pin->GetShownName();
    }

    if( !pinName.IsEmpty() && pinName != pinNumber )
        pinNumber += wxString::Format( wxT( " (%s)" ), pinName );

    return pinNumber;
}


template <typename T_symbol, typename T_field>
wxString DIALOG_SIM_MODEL<T_symbol, T_field>::getModelPinString( int aModelPinIndex ) const
{
    const wxString& pinName = curModel().GetPin( aModelPinIndex ).name;

    LOCALE_IO toggle;

    wxString pinNumber = wxString::Format( "%d", aModelPinIndex + 1 );

    if( !pinName.IsEmpty() && pinName != pinNumber )
        pinNumber += wxString::Format( wxT( " (%s)" ), pinName );

    return pinNumber;
}


template <typename T_symbol, typename T_field>
int DIALOG_SIM_MODEL<T_symbol, T_field>::getModelPinIndex( const wxString& aModelPinString ) const
{
    if( aModelPinString == "Not Connected" )
        return SIM_MODEL::PIN::NOT_CONNECTED;

    int length = aModelPinString.Find( " " );

    if( length == wxNOT_FOUND )
        length = static_cast<int>( aModelPinString.Length() );

    long result = 0;
    aModelPinString.Mid( 0, length ).ToCLong( &result );

    return static_cast<int>( result - 1 );
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onRadioButton( wxCommandEvent& aEvent )
{
    bool fromLibrary = m_useLibraryModelRadioButton->GetValue();

    m_pathLabel->Enable( fromLibrary );
    m_libraryPathText->Enable( fromLibrary );
    m_browseButton->Enable( fromLibrary );
    m_modelNameLabel->Enable( fromLibrary );
    m_modelNameChoice->Enable( fromLibrary );
    m_ibisPinLabel->Enable( fromLibrary );
    m_ibisPinCombobox->Enable( fromLibrary );
    m_differentialCheckbox->Enable( fromLibrary );
    m_ibisModelLabel->Enable( fromLibrary );
    m_ibisModelCombobox->Enable( fromLibrary );

    m_staticTextDevType->Enable( !fromLibrary );
    m_deviceTypeChoice->Enable( !fromLibrary );
    m_staticTextSpiceType->Enable( !fromLibrary );

    updateWidgets();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onLibraryPathTextEnter( wxCommandEvent& aEvent )
{
    if( m_useLibraryModelRadioButton->GetValue() )
    {
        wxString path = m_libraryPathText->GetValue();

        if( !path.IsEmpty() )
        {
            try
            {
                loadLibrary( path );
                updateWidgets();
            }
            catch( const IO_ERROR& )
            {
                // TODO: Add an infobar to report the error?
            }
        }
    }
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onLibraryPathTextKillFocus( wxFocusEvent& aEvent )
{
    if( !m_inKillFocus )
    {
        m_inKillFocus = true;

        wxCommandEvent dummy;
        onLibraryPathTextEnter( dummy );

        m_inKillFocus = false;
    }
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onBrowseButtonClick( wxCommandEvent& aEvent )
{
    wxFileDialog dlg( this, _( "Browse Models" ), Prj().GetProjectPath() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString path = dlg.GetPath();
    wxFileName fn( path );

    if( fn.MakeRelativeTo( Prj().GetProjectPath() ) && !fn.GetFullPath().StartsWith( ".." ) )
        path = fn.GetFullPath();

    loadLibrary( path, true );
    updateWidgets();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onModelNameChoice( wxCommandEvent& aEvent )
{
    if( isIbisLoaded() )
    {
        wxArrayString    pinLabels;
        SIM_MODEL_KIBIS* modelkibis = dynamic_cast<SIM_MODEL_KIBIS*>( &curModel() );

        wxCHECK2( modelkibis, return );

        for( std::pair<wxString, wxString> strs : modelkibis->GetIbisPins() )
            pinLabels.Add( strs.first + wxT( " - " ) + strs.second );

        m_ibisPinCombobox->Set( pinLabels );

        wxArrayString emptyArray;
        m_ibisModelCombobox->Set( emptyArray );
    }

    updateWidgets();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onIbisPinCombobox( wxCommandEvent& aEvent )
{
    if( isIbisLoaded() )
    {
        wxArrayString modelLabels;

        SIM_MODEL_KIBIS& kibisModel = static_cast<SIM_MODEL_KIBIS&>( curModel() );

        std::vector<std::pair<std::string, std::string>> strs = kibisModel.GetIbisPins();
        std::string pinNumber = strs.at( m_ibisPinCombobox->GetSelection() ).first;

        const SIM_LIBRARY_KIBIS* kibisLibrary = dynamic_cast<const SIM_LIBRARY_KIBIS*>( library() );

        kibisModel.ChangePin( *kibisLibrary, pinNumber );

        kibisModel.m_enableDiff = static_cast<const SIM_LIBRARY_KIBIS*>( library() )
                ->isPinDiff( kibisModel.GetComponentName(), pinNumber );

        for( wxString modelName : kibisModel.GetIbisModels() )
            modelLabels.Add( modelName );

        m_ibisModelCombobox->Set( modelLabels );
    }

    updateWidgets();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onIbisPinComboboxTextEnter( wxCommandEvent& aEvent )
{
    m_ibisPinCombobox->SetSelection(
            m_ibisPinCombobox->FindString( m_ibisPinCombobox->GetValue() ) );

    onIbisPinCombobox( aEvent );
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onIbisModelCombobox( wxCommandEvent& aEvent )
{
    updateWidgets();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onIbisModelComboboxTextEnter( wxCommandEvent& aEvent )
{
    m_ibisModelCombobox->SetSelection(
            m_ibisModelCombobox->FindString( m_ibisModelCombobox->GetValue() ) );

    onIbisPinCombobox( aEvent );
}

template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onDifferentialCheckbox( wxCommandEvent& aEvent )
{
    if( isIbisLoaded() )
    {
        SIM_MODEL_KIBIS* modelkibis = dynamic_cast<SIM_MODEL_KIBIS*>( &curModel() );
        bool             diff = m_differentialCheckbox->GetValue() && modelkibis->CanDifferential();
        modelkibis->SwitchSingleEndedDiff( diff );
    }

    updateWidgets();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onDeviceTypeChoice( wxCommandEvent& aEvent )
{
    for( SIM_MODEL::DEVICE_T deviceType : SIM_MODEL::DEVICE_T_ITERATOR() )
    {
        if( SIM_MODEL::DeviceInfo( deviceType ).description == m_deviceTypeChoice->GetStringSelection() )
        {
            m_curModelType = m_curModelTypeOfDeviceType.at( deviceType );
            break;
        }
    }

    updateWidgets();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onTypeChoice( wxCommandEvent& aEvent )
{
    SIM_MODEL::DEVICE_T   deviceType = curModel().GetDeviceType();
    wxString              typeDescription = m_typeChoice->GetString( m_typeChoice->GetSelection() );
    std::vector<LIB_PIN*> sourcePins = m_symbol.GetAllLibPins();

    std::sort( sourcePins.begin(), sourcePins.end(),
               []( const LIB_PIN* lhs, const LIB_PIN* rhs )
               {
                   return StrNumCmp( lhs->GetNumber(), rhs->GetNumber(), true ) < 0;
               } );

    for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
    {
        if( deviceType == SIM_MODEL::TypeInfo( type ).deviceType
            && typeDescription == SIM_MODEL::TypeInfo( type ).description )
        {
            if( isIbisLoaded()
                && ( type == SIM_MODEL::TYPE::KIBIS_DEVICE
                     || type == SIM_MODEL::TYPE::KIBIS_DRIVER_DC
                     || type == SIM_MODEL::TYPE::KIBIS_DRIVER_RECT
                     || type == SIM_MODEL::TYPE::KIBIS_DRIVER_PRBS ) )
            {
                int idx = m_modelNameChoice->GetSelection();

                auto& kibisModel = static_cast<SIM_MODEL_KIBIS&>( m_libraryModelsMgr.GetModels().at( idx ).get() );

                m_libraryModelsMgr.SetModel( idx, std::make_unique<SIM_MODEL_KIBIS>( type, kibisModel,
                                                                                     m_fields, sourcePins ) );
            }

            m_curModelType = type;
            break;
        }
    }

    m_curModelTypeOfDeviceType.at( deviceType ) = m_curModelType;
    updateWidgets();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onPageChanging( wxBookCtrlEvent& event )
{
    // This should have been done in wxPGTextCtrlEditor::OnTextCtrlEvent(), but something must
    // be clearing it before we get here, resulting in CommitChangesFromEditor() doing nothing
    m_paramGrid->GetGrid()->EditorsValueWasModified();
    m_paramGrid->GetGrid()->CommitChangesFromEditor();

    updateModelCodeTab();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onPinAssignmentsGridCellChange( wxGridEvent& aEvent )
{
    int      symbolPinIndex = aEvent.GetRow();
    wxString oldModelPinName = aEvent.GetString();
    wxString modelPinName = m_pinAssignmentsGrid->GetCellValue( aEvent.GetRow(), aEvent.GetCol() );

    int oldModelPinIndex = getModelPinIndex( oldModelPinName );
    int modelPinIndex = getModelPinIndex( modelPinName );

    if( oldModelPinIndex != SIM_MODEL::PIN::NOT_CONNECTED )
        curModel().SetPinSymbolPinNumber( oldModelPinIndex, "" );

    if( modelPinIndex != SIM_MODEL::PIN::NOT_CONNECTED )
    {
        curModel().SetPinSymbolPinNumber( modelPinIndex,
            std::string( m_sortedPartPins.at( symbolPinIndex )->GetShownNumber().ToUTF8() ) );
    }

    updatePinAssignments();

    aEvent.Skip();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onPinAssignmentsGridSize( wxSizeEvent& aEvent )
{
    wxGridUpdateLocker deferRepaintsTillLeavingScope( m_pinAssignmentsGrid );

    int gridWidth = KIPLATFORM::UI::GetUnobscuredSize( m_pinAssignmentsGrid ).x;
    m_pinAssignmentsGrid->SetColSize( PIN_COLUMN::MODEL, gridWidth / 2 );
    m_pinAssignmentsGrid->SetColSize( PIN_COLUMN::SYMBOL, gridWidth / 2 );

    aEvent.Skip();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onSaveInValueCheckbox( wxCommandEvent& aEvent )
{
    curModel().SetIsStoredInValue( m_saveInValueCheckbox->GetValue() );
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onParamGridSetFocus( wxFocusEvent& aEvent )
{
    // By default, when a property grid is focused, the textbox is not immediately focused until
    // Tab key is pressed. This is inconvenient, so we fix that here.

    wxPropertyGrid* grid = m_paramGrid->GetGrid();

    grid->CommitChangesFromEditor();

    wxPGProperty* selected = grid->GetSelection();

    if( !selected )
        selected = grid->wxPropertyGridInterface::GetFirst();

    if( selected )
        grid->DoSelectProperty( selected, wxPG_SEL_FOCUS );

    aEvent.Skip();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onParamGridSelectionChange( wxPropertyGridEvent& aEvent )
{
    wxPropertyGrid* grid = m_paramGrid->GetGrid();

    grid->CommitChangesFromEditor();

    // Jump over categories.
    if( grid->GetSelection() && grid->GetSelection()->IsCategory() )
    {
        wxPGProperty* selection = grid->GetSelection();

        // If the new selection is immediately above the previous selection, we jump up. Otherwise
        // we jump down. We do this by simulating up or down arrow keys.

        wxPropertyGridIterator it = grid->GetIterator( wxPG_ITERATE_VISIBLE, selection );
        it.Next();

        wxKeyEvent* keyEvent = new wxKeyEvent( wxEVT_KEY_DOWN );

        if( *it == m_prevParamGridSelection )
        {
            if( !selection->IsExpanded() )
            {
                grid->Expand( selection );
                keyEvent->m_keyCode = WXK_DOWN;
                wxQueueEvent( grid, keyEvent );

                // Does not work for some reason.
                /*m_paramGrid->DoSelectProperty( selection->Item( selection->GetChildCount() - 1 ),
                                               wxPG_SEL_FOCUS );*/
            }
            else
            {
                keyEvent->m_keyCode = WXK_UP;
                wxQueueEvent( grid, keyEvent );
            }
        }
        else
        {
            if( !selection->IsExpanded() )
                grid->Expand( selection );

            keyEvent->m_keyCode = WXK_DOWN;
            wxQueueEvent( grid, keyEvent );
        }

        m_prevParamGridSelection = grid->GetSelection();
        return;
    }

    wxWindow* editorControl = grid->GetEditorControl();
    if( !editorControl )
    {
        m_prevParamGridSelection = grid->GetSelection();
        return;
    }

    // Without this the user had to press tab before they could edit the field.
    editorControl->SetFocus();
    m_prevParamGridSelection = grid->GetSelection();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::adjustParamGridColumns( int aWidth, bool aForce )
{
    wxPropertyGrid* grid = m_paramGridMgr->GetGrid();
    int             margin = 15;
    int             indent = 20;

    if( aWidth != m_lastParamGridWidth || aForce )
    {
        m_lastParamGridWidth = aWidth;

        grid->FitColumns();

        std::vector<int> colWidths;

        for( size_t ii = 0; ii < grid->GetColumnCount(); ii++ )
        {
            if( ii == 0 )
                colWidths.push_back( grid->GetState()->GetColumnWidth( ii ) + margin + indent );
            else if( ii == 1 )
                colWidths.push_back( grid->GetState()->GetColumnWidth( ii ) + margin );
            else
                colWidths.push_back( 50 );

            aWidth -= colWidths[ ii ];
        }

        // Account for scroll bars
        aWidth -= ( grid->GetSize().x - grid->GetClientSize().x );

        if( aWidth > 0 )
            colWidths[ PARAM_COLUMN::VALUE ] += aWidth;

        for( size_t ii = 0; ii < grid->GetColumnCount(); ii++ )
            grid->SetColumnProportion( ii, colWidths[ ii ] );

        grid->ResetColumnSizes();
        grid->RefreshEditor();
    }
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onSizeParamGrid( wxSizeEvent& event )
{
    adjustParamGridColumns( event.GetSize().GetX(), false );

    event.Skip();
}



template class DIALOG_SIM_MODEL<SCH_SYMBOL, SCH_FIELD>;
template class DIALOG_SIM_MODEL<LIB_SYMBOL, LIB_FIELD>;
