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


bool equivalent( SIM_MODEL::DEVICE_T a, SIM_MODEL::DEVICE_T b )
{
    // A helper to handle SPICE's use of 'E' and 'H' for voltage sources and 'F' and 'G' for
    // current sources
    return a == b
           || SIM_MODEL::DeviceInfo( a ).description == SIM_MODEL::DeviceInfo( b ).description;
};


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

    for( LIB_PIN* pin : aSymbol.GetAllLibPins() )
    {
        // De Morgan conversions are equivalences, not additional items to simulate
        if( !pin->GetParent()->HasConversion() || pin->GetConvert() < 2 )
            m_sortedPartPins.push_back( pin );
    }

    std::sort( m_sortedPartPins.begin(), m_sortedPartPins.end(),
               []( const LIB_PIN* lhs, const LIB_PIN* rhs )
               {
                   // We sort by StrNumCmp because SIM_MODEL_BASE sorts with it too.
                   return StrNumCmp( lhs->GetNumber(), rhs->GetNumber(), true ) < 0;
               } );

    m_typeChoice->Clear();

    m_scintillaTricks = new SCINTILLA_TRICKS( m_codePreview, wxT( "{}" ), false );

    m_paramGridMgr->Bind( wxEVT_PG_SELECTED, &DIALOG_SIM_MODEL::onParamGridSelectionChange, this );

    wxPropertyGrid* grid = m_paramGrid->GetGrid();

    // In wx 3.0 the color will be wrong sometimes.
    grid->SetCellDisabledTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    grid->Bind( wxEVT_SET_FOCUS, &DIALOG_SIM_MODEL::onParamGridSetFocus, this );
    grid->Bind( wxEVT_UPDATE_UI, &DIALOG_SIM_MODEL::onUpdateUI, this );

    grid->AddActionTrigger( wxPG_ACTION_EDIT, WXK_RETURN );
    grid->DedicateKey( WXK_RETURN );
    grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_RETURN );

    grid->AddActionTrigger( wxPG_ACTION_EDIT, WXK_NUMPAD_ENTER );
    grid->DedicateKey( WXK_NUMPAD_ENTER );
    grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_NUMPAD_ENTER );

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

    wxString           msg;
    WX_STRING_REPORTER reporter( &msg );

    // Infer RLC and VI models if they aren't specified
    if( SIM_MODEL::InferSimModel( m_symbol, &m_fields, false, SIM_VALUE_GRAMMAR::NOTATION::SI,
                                  &deviceType, &modelType, &modelParams, &pinMap ) )
    {
        m_fields.emplace_back( &m_symbol, -1, SIM_DEVICE_TYPE_FIELD );
        m_fields.back().SetText( deviceType );

        if( !modelType.IsEmpty() )
        {
            m_fields.emplace_back( &m_symbol, -1, SIM_TYPE_FIELD );
            m_fields.back().SetText( modelType );
        }

        m_fields.emplace_back( &m_symbol, -1, SIM_PARAMS_FIELD );
        m_fields.back().SetText( modelParams );

        m_fields.emplace_back( &m_symbol, -1, SIM_PINS_FIELD );
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
        m_rbLibraryModel->SetValue( true );

        if( !loadLibrary( libraryFilename ) )
        {
            m_libraryPathText->ChangeValue( libraryFilename );
            m_curModelType = SIM_MODEL::ReadTypeFromFields( m_fields, &reporter );
            m_libraryModelsMgr.CreateModel( nullptr, m_sortedPartPins, m_fields );

            m_modelNameChoice->Append( _( "<unknown>" ) );
            m_modelNameChoice->SetSelection( 0 );
        }
        else
        {
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

            m_curModelType = curModel().GetType();
        }

        if( isIbisLoaded() && ( m_modelNameChoice->GetSelection() >= 0 ) )
        {
            int  idx = m_modelNameChoice->GetSelection();
            auto kibismodel = dynamic_cast<SIM_MODEL_KIBIS*>( &m_libraryModelsMgr.GetModels()[idx].get() );

            if( kibismodel )
            {
                onModelNameChoice( dummyEvent ); // refresh list of pins

                int i = 0;

                for( const std::pair<std::string, std::string>& strs : kibismodel->GetIbisPins() )
                {
                    if( strs.first == SIM_MODEL::GetFieldValue( &m_fields, SIM_LIBRARY_KIBIS::PIN_FIELD ) )
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
    }
    else if( !SIM_MODEL::GetFieldValue( &m_fields, SIM_DEVICE_TYPE_FIELD ).empty()
                || !SIM_MODEL::GetFieldValue( &m_fields, SIM_TYPE_FIELD ).empty() )
    {
        // The model is sourced from the instance.
        m_rbBuiltinModel->SetValue( true );

        msg.clear();
        m_curModelType = SIM_MODEL::ReadTypeFromFields( m_fields, &reporter );

        if( reporter.HasMessage() )
            DisplayErrorMessage( this, msg );
    }

    m_builtinModelsMgr.SetReporter( &reporter );

    for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
    {
        if( m_rbBuiltinModel->GetValue() && type == m_curModelType )
        {
            msg.clear();
            m_builtinModelsMgr.CreateModel( m_fields, m_sortedPartPins, false );

            if( reporter.HasMessage() )
            {
                DisplayErrorMessage( this, _( "Failed to read simulation model from fields." )
                                           + wxT( "\n\n" ) + msg );
            }
        }
        else
        {
            m_builtinModelsMgr.CreateModel( type, m_sortedPartPins );
        }

        SIM_MODEL::DEVICE_T deviceTypeT = SIM_MODEL::TypeInfo( type ).deviceType;

        if( !m_curModelTypeOfDeviceType.count( deviceTypeT ) )
            m_curModelTypeOfDeviceType[deviceTypeT] = type;
    }

    if( storeInValue )
        curModel().SetIsStoredInValue( true );

    m_saveInValueCheckbox->SetValue( curModel().IsStoredInValue() );

    onRadioButton( dummyEvent );
    return DIALOG_SIM_MODEL_BASE::TransferDataToWindow();
}


template <typename T_symbol, typename T_field>
bool DIALOG_SIM_MODEL<T_symbol, T_field>::TransferDataFromWindow()
{
    m_pinAssignmentsGrid->CommitPendingChanges();

    if( !DIALOG_SIM_MODEL_BASE::TransferDataFromWindow() )
        return false;

    std::string path;
    std::string name;

    if( m_rbLibraryModel->GetValue() )
    {
        path = m_libraryPathText->GetValue();
        wxFileName fn( path );

        if( fn.MakeRelativeTo( Prj().GetProjectPath() ) && !fn.GetFullPath().StartsWith( ".." ) )
            path = fn.GetFullPath();

        if( !m_modelNameChoice->IsEmpty() )
            name = m_modelNameChoice->GetStringSelection().ToStdString();
        else if( dynamic_cast<SIM_MODEL_SPICE_FALLBACK*>( &curModel() ) )
            name = SIM_MODEL::GetFieldValue( &m_fields, SIM_LIBRARY::NAME_FIELD, false );
    }

    SIM_MODEL::SetFieldValue( m_fields, SIM_LIBRARY::LIBRARY_FIELD, path );
    SIM_MODEL::SetFieldValue( m_fields, SIM_LIBRARY::NAME_FIELD, name );

    if( isIbisLoaded() )
    {
        SIM_MODEL_KIBIS* ibismodel = static_cast<SIM_MODEL_KIBIS*>(
                &m_libraryModelsMgr.GetModels().at( m_modelNameChoice->GetSelection() ).get() );

        if( ibismodel )
        {
            std::string pins;
            std::string modelName = std::string( m_ibisModelCombobox->GetValue().c_str() );
            std::string differential;

            if( m_ibisPinCombobox->GetSelection() >= 0 )
                pins = ibismodel->GetIbisPins().at( m_ibisPinCombobox->GetSelection() ).first;

            if( ibismodel->CanDifferential() && m_differentialCheckbox->GetValue() )
                differential = "1";

            SIM_MODEL::SetFieldValue( m_fields, SIM_LIBRARY_KIBIS::PIN_FIELD, pins );
            SIM_MODEL::SetFieldValue( m_fields, SIM_LIBRARY_KIBIS::MODEL_FIELD, modelName );
            SIM_MODEL::SetFieldValue( m_fields, SIM_LIBRARY_KIBIS::DIFF_FIELD, differential );
        }
    }

    if( curModel().GetType() == SIM_MODEL::TYPE::RAWSPICE )
    {
        if( m_modelNotebook->GetSelection() == 0 )
            updateModelCodeTab( &curModel() );

        wxString code = m_codePreview->GetText().Trim( true ).Trim( false );
        curModel().SetParamValue( "model", std::string( code.ToUTF8() ) );
    }

    curModel().SetIsStoredInValue( m_saveInValueCheckbox->GetValue() );

    curModel().WriteFields( m_fields );

    return true;
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::updateWidgets()
{
    SIM_MODEL* model = &curModel();

    updateIbisWidgets( model );
    updateBuiltinModelWidgets( model );
    updateModelParamsTab( model );
    updateModelCodeTab( model );
    updatePinAssignments( model );

    std::string ref = SIM_MODEL::GetFieldValue( &m_fields, SIM_REFERENCE_FIELD );

    m_modelPanel->Layout();
    m_pinAssignmentsPanel->Layout();
    m_parametersPanel->Layout();
    m_codePanel->Layout();

    SendSizeEvent( wxSEND_EVENT_POST );

    m_prevModel = &curModel();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::updateIbisWidgets( SIM_MODEL* aModel )
{
    SIM_MODEL_KIBIS* modelkibis = isIbisLoaded() ? dynamic_cast<SIM_MODEL_KIBIS*>( aModel )
                                                 : nullptr;

    m_ibisModelCombobox->Show( isIbisLoaded() );
    m_ibisPinCombobox->Show( isIbisLoaded() );
    m_ibisModelLabel->Show( isIbisLoaded() );
    m_ibisPinLabel->Show( isIbisLoaded() );

    m_differentialCheckbox->Show( isIbisLoaded() && modelkibis && modelkibis->CanDifferential() );
    m_modelNameLabel->SetLabel( isIbisLoaded() ? _( "Component:" ) : _( "Model:" ) );
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::updateBuiltinModelWidgets( SIM_MODEL* aModel )
{
    // Change the Type choice to match the current device type.
    if( aModel != m_prevModel )
    {
        m_deviceTypeChoice->Clear();

        if( m_rbLibraryModel->GetValue() )
        {
            m_deviceTypeChoice->Append( aModel->GetDeviceInfo().description );
            m_deviceTypeChoice->SetSelection( 0 );
        }
        else
        {
            for( SIM_MODEL::DEVICE_T deviceType : SIM_MODEL::DEVICE_T_ITERATOR() )
            {
                if( !SIM_MODEL::DeviceInfo( deviceType ).showInMenu )
                    continue;

                m_deviceTypeChoice->Append( SIM_MODEL::DeviceInfo( deviceType ).description );

                if( equivalent( deviceType, aModel->GetDeviceType() ) )
                    m_deviceTypeChoice->SetSelection( m_deviceTypeChoice->GetCount() - 1 );
            }
        }

        m_typeChoice->Clear();

        for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
        {
            SIM_MODEL::DEVICE_T deviceType = SIM_MODEL::TypeInfo( type ).deviceType;

            if( deviceType == aModel->GetDeviceType()
                || SIM_MODEL::DeviceInfo( deviceType ).description == aModel->GetDeviceInfo().description )
            {
                m_typeChoice->Append( SIM_MODEL::TypeInfo( type ).description );

                if( type == aModel->GetType() )
                    m_typeChoice->SetSelection( m_typeChoice->GetCount() - 1 );
            }
        }
    }

    m_typeChoice->Enable( !m_rbLibraryModel->GetValue() || isIbisLoaded() );

    if( dynamic_cast<SIM_MODEL_RAW_SPICE*>( aModel ) )
        m_modelNotebook->SetSelection( 1 );
    else
        m_modelNotebook->SetSelection( 0 );

    if( aModel->HasPrimaryValue() )
    {
        const SIM_MODEL::PARAM& primary = aModel->GetParam( 0 );

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
void DIALOG_SIM_MODEL<T_symbol, T_field>::updateModelParamsTab( SIM_MODEL* aModel )
{
    if( aModel != m_prevModel )
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

        m_paramGrid->Append( new wxPropertyCategory( "S-Parameters" ) );
        m_paramGrid->HideProperty( "S-Parameters" );

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

        for( int i = 0; i < aModel->GetParamCount(); ++i )
            addParamPropertyIfRelevant( aModel, i );

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
        if( aModel->HasAutofill() )
            ( *it )->SetValueFromString( param.value );
    }
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::updateModelCodeTab( SIM_MODEL* aModel )
{
    if( dynamic_cast<SIM_MODEL_SPICE_FALLBACK*>( aModel ) )
        return;

    wxString   text;
    SPICE_ITEM item;

    item.modelName = m_modelNameChoice->GetStringSelection();

    if( m_rbBuiltinModel->GetValue() || item.modelName == "" )
        item.modelName = m_fields.at( REFERENCE_FIELD ).GetText();

    text << aModel->SpiceGenerator().Preview( item );

    m_codePreview->SetText( text );
    m_codePreview->SelectNone();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::updatePinAssignments( SIM_MODEL* aModel )
{
    removeOrphanedPinAssignments( aModel );

    // Reset the grid.

    m_pinAssignmentsGrid->ClearRows();
    m_pinAssignmentsGrid->AppendRows( static_cast<int>( m_sortedPartPins.size() ) );

    for( int row = 0; row < m_pinAssignmentsGrid->GetNumberRows(); ++row )
        m_pinAssignmentsGrid->SetCellValue( row, PIN_COLUMN::MODEL, _( "Not Connected" ) );

    // Now set up the grid values in the Model column.
    for( int modelPinIndex = 0; modelPinIndex < aModel->GetPinCount(); ++modelPinIndex )
    {
        wxString symbolPinNumber = aModel->GetPin( modelPinIndex ).symbolPinNumber;

        if( symbolPinNumber == "" )
            continue;

        int symbolPinRow = findSymbolPinRow( symbolPinNumber );

        if( symbolPinRow == -1 )
            continue;

        wxString modelPinString = getModelPinString( aModel, modelPinIndex );
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

        for( int jj = 0; jj < aModel->GetPinCount(); ++jj )
        {
            if( aModel->GetPin( jj ).symbolPinNumber != "" )
                modelPinIcons.push_back( PinShapeGetBitmap( GRAPHIC_PINSHAPE::LINE ) );
            else
                modelPinIcons.push_back( BITMAPS::INVALID_BITMAP );

            modelPinChoices.Add( getModelPinString( aModel, jj ) );
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

    if( aModel->GetType() == SIM_MODEL::TYPE::SUBCKT )
    {
        SIM_MODEL_SUBCKT* subckt = static_cast<SIM_MODEL_SUBCKT*>( aModel );
        m_subckt->SetText( subckt->GetSpiceCode() );
    }
    else
    {
        m_subcktLabel->Show( false );
        m_subckt->Show( false );
    }
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::removeOrphanedPinAssignments( SIM_MODEL* aModel )
{
    for( int i = 0; i < aModel->GetPinCount(); ++i )
    {
        if( !m_symbol.GetPin( aModel->GetPin( i ).symbolPinNumber ) )
            aModel->SetPinSymbolPinNumber( i, "" );
    }
}


template <typename T_symbol, typename T_field>
bool DIALOG_SIM_MODEL<T_symbol, T_field>::loadLibrary( const wxString& aLibraryPath,
                                                       bool aForceReload )
{
    if( m_prevLibrary == aLibraryPath && !aForceReload )
        return true;

    wxString           msg;
    WX_STRING_REPORTER reporter( &msg );

    m_libraryModelsMgr.SetReporter( &reporter );
    m_libraryModelsMgr.SetForceFullParse();
    m_libraryModelsMgr.SetLibrary( aLibraryPath );

    if( reporter.HasMessage() )
    {
        DisplayErrorMessage( this, msg );
        return false;
    }

    std::string modelName = SIM_MODEL::GetFieldValue( &m_fields, SIM_LIBRARY::NAME_FIELD );

    for( const auto& [baseModelName, baseModel] : library()->GetModels() )
    {
        if( baseModelName == modelName )
            m_libraryModelsMgr.CreateModel( &baseModel, m_sortedPartPins, m_fields );
        else
            m_libraryModelsMgr.CreateModel( &baseModel, m_sortedPartPins );
    }

    if( reporter.HasMessage() )
        DisplayErrorMessage( this, msg );

    m_rbLibraryModel->SetValue( true );
    m_libraryPathText->ChangeValue( aLibraryPath );

    wxArrayString modelNames;

    for( const auto& [name, model] : library()->GetModels() )
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

    m_prevLibrary = aLibraryPath;
    return true;
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::addParamPropertyIfRelevant( SIM_MODEL* aModel,
                                                                      int aParamIndex )
{
    if( aModel->GetParam( aParamIndex ).info.dir == SIM_MODEL::PARAM::DIR_OUT )
        return;

    switch( aModel->GetParam( aParamIndex ).info.category )
    {
    case CATEGORY::AC:
        m_paramGrid->HideProperty( "AC", false );
        m_paramGrid->AppendIn( "AC", newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::DC:
        m_paramGrid->HideProperty( "DC", false );
        m_paramGrid->AppendIn( "DC", newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::S_PARAM:
        m_paramGrid->HideProperty( "S-Parameters", false );
        m_paramGrid->AppendIn( "S-Parameters", newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::CAPACITANCE:
        m_paramGrid->HideProperty( "Capacitance", false );
        m_paramGrid->AppendIn( "Capacitance", newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::TEMPERATURE:
        m_paramGrid->HideProperty( "Temperature", false );
        m_paramGrid->AppendIn( "Temperature", newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::NOISE:
        m_paramGrid->HideProperty( "Noise", false );
        m_paramGrid->AppendIn( "Noise", newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::DISTRIBUTED_QUANTITIES:
        m_paramGrid->HideProperty( "Distributed Quantities", false );
        m_paramGrid->AppendIn( "Distributed Quantities", newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::WAVEFORM:
        m_paramGrid->HideProperty( "Waveform", false );
        m_paramGrid->AppendIn( "Waveform", newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::GEOMETRY:
        m_paramGrid->HideProperty( "Geometry", false );
        m_paramGrid->AppendIn( "Geometry", newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::LIMITING_VALUES:
        m_paramGrid->HideProperty( "Limiting Values", false );
        m_paramGrid->AppendIn( "Limiting Values", newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::ADVANCED:
        m_paramGrid->HideProperty( "Advanced", false );
        m_paramGrid->AppendIn( "Advanced", newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::FLAGS:
        m_paramGrid->HideProperty( "Flags", false );
        m_paramGrid->AppendIn( "Flags", newParamProperty( aModel, aParamIndex ) );
        break;

    default:
        m_paramGrid->Insert( m_firstCategory, newParamProperty( aModel, aParamIndex ) );
        break;

    case CATEGORY::INITIAL_CONDITIONS:
    case CATEGORY::SUPERFLUOUS:
        return;
    }
}


template <typename T_symbol, typename T_field>
wxPGProperty* DIALOG_SIM_MODEL<T_symbol, T_field>::newParamProperty( SIM_MODEL* aModel ,
                                                                     int aParamIndex ) const
{
    const SIM_MODEL::PARAM& param = aModel->GetParam( aParamIndex );
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
        prop = new SIM_BOOL_PROPERTY( paramDescription, param.info.name, *aModel, aParamIndex );
        prop->SetAttribute( wxPG_BOOL_USE_CHECKBOX, true );
        break;

    case SIM_VALUE::TYPE_INT:
        prop = new SIM_STRING_PROPERTY( paramDescription, param.info.name, *aModel, aParamIndex,
                                        SIM_VALUE::TYPE_INT );
        break;

    case SIM_VALUE::TYPE_FLOAT:
        prop = new SIM_STRING_PROPERTY( paramDescription, param.info.name, *aModel, aParamIndex,
                                        SIM_VALUE::TYPE_FLOAT );
        break;

    //case TYPE_COMPLEX:
    //  break;

    case SIM_VALUE::TYPE_STRING:
        if( param.info.enumValues.empty() )
        {
            prop = new SIM_STRING_PROPERTY( paramDescription, param.info.name, *aModel,
                                            aParamIndex, SIM_VALUE::TYPE_STRING );
        }
        else
        {
            prop = new SIM_ENUM_PROPERTY( paramDescription, param.info.name, *aModel, aParamIndex );
        }
        break;

    default:
        prop = new wxStringProperty( paramDescription, param.info.name );
        break;
    }

    prop->SetAttribute( wxPG_ATTR_UNITS, wxString::FromUTF8( param.info.unit.c_str() ) );

    // Legacy due to the way we extracted the parameters from Ngspice.
    #if wxCHECK_VERSION( 3, 1, 0 )
        prop->SetCell( 3, wxString::FromUTF8( param.info.defaultValue ) );
    #else
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
    if( m_rbLibraryModel->GetValue() )
    {
        int sel = m_modelNameChoice->GetSelection();

        if( sel >= 0 && sel < (int) m_libraryModelsMgr.GetModels().size() )
            return m_libraryModelsMgr.GetModels().at( sel ).get();
    }
    else
    {
        if( (int) m_curModelType < (int) m_builtinModelsMgr.GetModels().size() )
            return m_builtinModelsMgr.GetModels().at( (int) m_curModelType );
    }

    return m_builtinModelsMgr.GetModels().at( (int) SIM_MODEL::TYPE::NONE );
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
wxString DIALOG_SIM_MODEL<T_symbol, T_field>::getModelPinString( SIM_MODEL* aModel,
                                                                 int aModelPinIndex ) const
{
    const wxString& pinName = aModel->GetPin( aModelPinIndex ).name;

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
    bool fromLibrary = m_rbLibraryModel->GetValue();

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

    m_prevModel = nullptr;  // Ensure the Model panel will be rebuild after updating other params.
    updateWidgets();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onLibraryPathTextEnter( wxCommandEvent& aEvent )
{
    if( m_rbLibraryModel->GetValue() )
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
    static wxString s_mruPath;

    wxString     path = s_mruPath.IsEmpty() ? Prj().GetProjectPath() : s_mruPath;
    wxFileDialog dlg( this, _( "Browse Models" ), path );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    path = dlg.GetPath();
    wxFileName fn( path );

    s_mruPath = fn.GetPath();

    if( fn.MakeRelativeTo( Prj().GetProjectPath() ) && !fn.GetFullPath().StartsWith( wxS( ".." ) ) )
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

        SIM_MODEL_KIBIS& ibisModel = static_cast<SIM_MODEL_KIBIS&>( curModel() );

        std::vector<std::pair<std::string, std::string>> strs = ibisModel.GetIbisPins();
        std::string pinNumber = strs.at( m_ibisPinCombobox->GetSelection() ).first;

        const SIM_LIBRARY_KIBIS* ibisLibrary = dynamic_cast<const SIM_LIBRARY_KIBIS*>( library() );

        ibisModel.ChangePin( *ibisLibrary, pinNumber );

        ibisModel.m_enableDiff = ibisLibrary->isPinDiff( ibisModel.GetComponentName(), pinNumber );

        for( wxString modelName : ibisModel.GetIbisModels() )
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

        wxCHECK( modelkibis, /* void */ );

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

    for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
    {
        if( equivalent( deviceType, SIM_MODEL::TypeInfo( type ).deviceType )
            && typeDescription == SIM_MODEL::TypeInfo( type ).description )
        {
            if( isIbisLoaded()
                && ( type == SIM_MODEL::TYPE::KIBIS_DEVICE
                     || type == SIM_MODEL::TYPE::KIBIS_DRIVER_DC
                     || type == SIM_MODEL::TYPE::KIBIS_DRIVER_RECT
                     || type == SIM_MODEL::TYPE::KIBIS_DRIVER_PRBS ) )
            {
                int idx = m_modelNameChoice->GetSelection();

                auto& baseModel = static_cast<SIM_MODEL_KIBIS&>( m_libraryModelsMgr.GetModels()[idx].get() );

                m_libraryModelsMgr.SetModel( idx, std::make_unique<SIM_MODEL_KIBIS>( type, baseModel ) );

                try
                {
                    m_libraryModelsMgr.GetModels()[idx].get().ReadDataFields( &m_fields,
                                                                              m_sortedPartPins );
                }
                catch( IO_ERROR& err )
                {
                    DisplayErrorMessage( this, err.What() );
                }
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
    updateModelCodeTab( &curModel() );
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

    updatePinAssignments( &curModel() );

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
void DIALOG_SIM_MODEL<T_symbol, T_field>::onParamGridSetFocus( wxFocusEvent& aEvent )
{
    // By default, when a property grid is focused, the textbox is not immediately focused until
    // Tab key is pressed. This is inconvenient, so we fix that here.

    wxPropertyGrid* grid = m_paramGrid->GetGrid();
    wxPGProperty*   selected = grid->GetSelection();

    if( !selected )
        selected = grid->wxPropertyGridInterface::GetFirst();

#if wxCHECK_VERSION( 3, 3, 0 )
    if( selected )
        grid->DoSelectProperty( selected, wxPGSelectPropertyFlags::Focus );
#else
    if( selected )
        grid->DoSelectProperty( selected, wxPG_SEL_FOCUS );
#endif

    aEvent.Skip();
}


template <typename T_symbol, typename T_field>
void DIALOG_SIM_MODEL<T_symbol, T_field>::onParamGridSelectionChange( wxPropertyGridEvent& aEvent )
{
    wxPropertyGrid* grid = m_paramGrid->GetGrid();

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
void DIALOG_SIM_MODEL<T_symbol, T_field>::onUpdateUI( wxUpdateUIEvent& aEvent )
{
    // This is currently patched in wxPropertyGrid::ScrollWindow() in the Mac wxWidgets fork.
    // However, we may need this version if it turns out to be an issue on other platforms and
    // we can't get it upstreamed.
#if 0
    // It's a shame to do this on the UpdateUI event, but neither the wxPropertyGridManager,
    // wxPropertyGridPage, wxPropertyGrid, nor the wxPropertyGrid's GetCanvas() window appear
    // to get scroll events.

    wxPropertyGrid* grid = m_paramGrid->GetGrid();
    wxTextCtrl*     ctrl = grid->GetEditorTextCtrl();

    if( ctrl )
    {
        wxRect ctrlRect = ctrl->GetScreenRect();
        wxRect gridRect = grid->GetScreenRect();

        if( ctrlRect.GetTop() < gridRect.GetTop() || ctrlRect.GetBottom() > gridRect.GetBottom() )
            grid->ClearSelection();
    }
#endif
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
            if( ii == PARAM_COLUMN::DESCRIPTION )
                colWidths.push_back( grid->GetState()->GetColumnWidth( ii ) + margin + indent );
            else if( ii == PARAM_COLUMN::VALUE )
                colWidths.push_back( std::max( 72, grid->GetState()->GetColumnWidth( ii ) ) + margin );
            else
                colWidths.push_back( 60 + margin );

            aWidth -= colWidths[ ii ];
        }

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
