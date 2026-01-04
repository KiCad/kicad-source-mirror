/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 CERN
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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <dialog_ibis_parser_reporter.h>
#include <dialog_sim_model.h>
#include <sim/sim_property.h>
#include <sim/sim_library_ibis.h>
#include <sim/sim_model.h>
#include <sim/sim_model_ibis.h>
#include <sim/sim_model_raw_spice.h>
#include <sim/sim_model_spice_fallback.h>
#include <sim/sim_model_subckt.h>
#include <grid_tricks.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <kiplatform/ui.h>
#include <confirm.h>
#include <string_utils.h>

#include <wx/filedlg.h>
#include <fmt/format.h>
#include <sch_edit_frame.h>
#include <sim/sim_model_l_mutual.h>
#include <sim/spice_circuit_model.h>
#include <widgets/filedlg_hook_embed_file.h>
#include <wx/filedlg.h>
#include <wx/log.h>

using CATEGORY = SIM_MODEL::PARAM::CATEGORY;

#define FORCE_REFRESH_FROM_MODEL true


bool equivalent( SIM_MODEL::DEVICE_T a, SIM_MODEL::DEVICE_T b )
{
    // A helper to handle SPICE's use of 'E' and 'H' for voltage sources and 'F' and 'G' for
    // current sources
    return a == b
           || SIM_MODEL::DeviceInfo( a ).description == SIM_MODEL::DeviceInfo( b ).description;
};


template <typename T>
DIALOG_SIM_MODEL<T>::DIALOG_SIM_MODEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame, T& aSymbol,
                                       std::vector<SCH_FIELD>& aFields ) :
        DIALOG_SIM_MODEL_BASE( aParent ),
        m_frame( aFrame ),
        m_symbol( aSymbol ),
        m_fields( aFields ),
        m_libraryModelsMgr( &Prj() ),
        m_builtinModelsMgr( &Prj() ),
        m_prevModel( nullptr ),
        m_curModelType( SIM_MODEL::TYPE::NONE ),
        m_scintillaTricksCode( nullptr ),
        m_scintillaTricksSubckt( nullptr ),
        m_firstCategory( nullptr ),
        m_prevParamGridSelection( nullptr ),
        m_lastParamGridWidth( 0 )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_infoBar->AddCloseButton();

    if constexpr (std::is_same_v<T, SCH_SYMBOL>)
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( &aSymbol );
        m_filesStack.push_back( symbol->Schematic()->GetEmbeddedFiles() );
    }

    if( EMBEDDED_FILES* symbolEmbeddedFiles = aSymbol.GetEmbeddedFiles() )
    {
        m_filesStack.push_back( symbolEmbeddedFiles );

        if constexpr (std::is_same_v<T, SCH_SYMBOL>)
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( &aSymbol );
            symbol->GetLibSymbolRef()->AppendParentEmbeddedFiles( m_filesStack );
        }
        else if constexpr (std::is_same_v<T, LIB_SYMBOL>)
        {
            LIB_SYMBOL* symbol = static_cast<LIB_SYMBOL*>( &aSymbol );
            symbol->AppendParentEmbeddedFiles( m_filesStack );
        }
    }

    m_libraryModelsMgr.SetFilesStack( m_filesStack );
    m_builtinModelsMgr.SetFilesStack( m_filesStack );

    for( SCH_PIN* pin : aSymbol.GetPins() )
    {
        // Body styles (including De Morgan variants) are equivalences, not additional items to simulate
        if( !pin->GetParentSymbol()->IsMultiBodyStyle() || pin->GetBodyStyle() < 2 )
            m_sortedPartPins.push_back( pin );
    }

    std::sort( m_sortedPartPins.begin(), m_sortedPartPins.end(),
               []( const SCH_PIN* lhs, const SCH_PIN* rhs )
               {
                   // We sort by StrNumCmp because SIM_MODEL_BASE sorts with it too.
                   return StrNumCmp( lhs->GetNumber(), rhs->GetNumber(), true ) < 0;
               } );

    m_waveformChoice->Clear();
    m_deviceChoice->Clear();
    m_deviceSubtypeChoice->Clear();

    m_scintillaTricksCode = new SCINTILLA_TRICKS( m_codePreview, wxT( "{}" ), false );
    m_scintillaTricksSubckt = new SCINTILLA_TRICKS( m_subckt, wxT( "()" ), false );

    m_paramGridMgr->Bind( wxEVT_PG_SELECTED, &DIALOG_SIM_MODEL::onParamGridSelectionChange, this );

    wxPropertyGrid* grid = m_paramGrid->GetGrid();
    grid->SetCellDisabledTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
    grid->Bind( wxEVT_SET_FOCUS, &DIALOG_SIM_MODEL::onParamGridSetFocus, this );
    grid->Bind( wxEVT_UPDATE_UI, &DIALOG_SIM_MODEL::onUpdateUI, this );

    grid->DedicateKey( WXK_RETURN );
    grid->DedicateKey( WXK_NUMPAD_ENTER );
    grid->DedicateKey( WXK_UP );
    grid->DedicateKey( WXK_DOWN );

#if wxCHECK_VERSION( 3, 3, 0 )
    grid->AddActionTrigger( wxPGKeyboardAction::Edit, WXK_RETURN );
    grid->AddActionTrigger( wxPGKeyboardAction::NextProperty, WXK_RETURN );
    grid->AddActionTrigger( wxPGKeyboardAction::Edit, WXK_NUMPAD_ENTER );
    grid->AddActionTrigger( wxPGKeyboardAction::NextProperty, WXK_NUMPAD_ENTER );
#else
    grid->AddActionTrigger( wxPG_ACTION_EDIT, WXK_RETURN );
    grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_RETURN );
    grid->AddActionTrigger( wxPG_ACTION_EDIT, WXK_NUMPAD_ENTER );
    grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_NUMPAD_ENTER );
#endif

    m_pinAssignmentsGrid->ClearRows();
    m_pinAssignmentsGrid->PushEventHandler( new GRID_TRICKS( m_pinAssignmentsGrid ) );

    m_subcktLabel->SetFont( KIUI::GetStatusFont( m_subcktLabel ) );
    finishDialogSettings();
}


template <typename T>
DIALOG_SIM_MODEL<T>::~DIALOG_SIM_MODEL()
{
    // Disable all properties. This is necessary because some of their methods are called after
    // destruction of DIALOG_SIM_MODEL, oddly. When disabled, they never access their models.
    for( wxPropertyGridIterator it = m_paramGrid->GetIterator(); !it.AtEnd(); ++it )
    {
        if( SIM_PROPERTY* prop = dynamic_cast<SIM_PROPERTY*>( *it ) )
            prop->Disable();
    }

    // Delete the GRID_TRICKS.
    m_pinAssignmentsGrid->PopEventHandler( true );

    delete m_scintillaTricksCode;
    delete m_scintillaTricksSubckt;
}


template <typename T>
bool DIALOG_SIM_MODEL<T>::TransferDataToWindow()
{
    wxCommandEvent dummyEvent;
    wxString       deviceType;
    wxString       modelType;
    wxString       modelParams;
    wxString       pinMap;
    bool           storeInValue = false;

    WX_STRING_REPORTER reporter;

    // Infer RLC and VI models if they aren't specified
    if( SIM_MODEL::InferSimModel( m_symbol, &m_fields, false, 0, SIM_VALUE_GRAMMAR::NOTATION::SI,
                                  &deviceType, &modelType, &modelParams, &pinMap ) )
    {
        SetFieldValue( m_fields, SIM_DEVICE_FIELD, deviceType.ToStdString() );

        if( !modelType.IsEmpty() )
            SetFieldValue( m_fields, SIM_DEVICE_SUBTYPE_FIELD, modelType.ToStdString() );

        SetFieldValue( m_fields, SIM_PARAMS_FIELD, modelParams.ToStdString() );

        SetFieldValue( m_fields, SIM_PINS_FIELD, pinMap.ToStdString() );

        storeInValue = true;

        // In case the storeInValue checkbox is turned off (if it's left on then we'll overwrite
        // this field with the actual value):
        FindField( m_fields, FIELD_T::VALUE )->SetText( wxT( "${SIM.PARAMS}" ) );
    }

    wxString libraryFilename = GetFieldValue( &m_fields, SIM_LIBRARY::LIBRARY_FIELD, true, 0 );
    wxFileName tmp( libraryFilename );

    if( !tmp.GetFullName().IsEmpty() )
    {
        // The model is sourced from a library, optionally with instance overrides.
        m_rbLibraryModel->SetValue( true );

        if( !loadLibrary( libraryFilename, reporter ) )
        {
            if( reporter.HasMessage() )
                m_infoBar->ShowMessage( reporter.GetMessages() );

            m_libraryPathText->ChangeValue( libraryFilename );
            m_curModelType = SIM_MODEL::ReadTypeFromFields( m_fields, true, 0, reporter );

            m_libraryModelsMgr.CreateModel( nullptr, m_sortedPartPins, m_fields, true, 0, reporter );

            m_modelListBox->Clear();
            m_modelListBox->Append( _( "<unknown>" ) );
            m_modelListBox->SetSelection( 0 );
        }
        else
        {
            std::string modelName = GetFieldValue( &m_fields, SIM_LIBRARY::NAME_FIELD, true, 0 );
            int         modelIdx = m_modelListBox->FindString( modelName );

            if( modelIdx == wxNOT_FOUND )
            {
                m_infoBar->ShowMessage( wxString::Format( _( "No model named '%s' in library." ),
                                                          modelName ) );

                // Default to first item in library
                m_modelListBox->SetSelection( 0 );
            }
            else
            {
                m_infoBar->Hide();
                m_modelListBox->SetSelection( modelIdx );
            }

            m_curModelType = curModel().GetType();
        }

        if( isIbisLoaded() && ( m_modelListBox->GetSelection() >= 0 ) )
        {
            int      idx = 0;
            wxString sel = m_modelListBox->GetStringSelection();

            if( m_modelListBoxEntryToLibraryIdx.contains( sel ) )
                idx = m_modelListBoxEntryToLibraryIdx.at( sel );

            SIM_MODEL_IBIS* ibismodel = dynamic_cast<SIM_MODEL_IBIS*>( &m_libraryModelsMgr.GetModels()[idx].get() );

            if( ibismodel )
            {
                onModelNameChoice( dummyEvent ); // refresh list of pins

                int i = 0;

                for( const std::pair<std::string, std::string>& strs : ibismodel->GetIbisPins() )
                {
                    if( strs.first == GetFieldValue( &m_fields, SIM_LIBRARY_IBIS::PIN_FIELD, true, 0 ) )
                    {
                        auto ibisLibrary = static_cast<const SIM_LIBRARY_IBIS*>( library() );

                        ibismodel->ChangePin( *ibisLibrary, strs.first );
                        m_pinCombobox->SetSelection( static_cast<int>( i ) );
                        break;
                    }
                    i++;
                }

                if( i < static_cast<int>( ibismodel->GetIbisPins().size() ) )
                {
                    onPinCombobox( dummyEvent ); // refresh list of models

                    wxString ibisModel = GetFieldValue( &m_fields, SIM_LIBRARY_IBIS::MODEL_FIELD, true, 0 );
                    m_pinModelCombobox->SetStringSelection( ibisModel );
                }

                if( GetFieldValue( &m_fields, SIM_LIBRARY_IBIS::DIFF_FIELD, true, 0 ) == "1" )
                {
                    ibismodel->SwitchSingleEndedDiff( true );
                    m_differentialCheckbox->SetValue( true );
                }
                else
                {
                    ibismodel->SwitchSingleEndedDiff( false );
                    m_differentialCheckbox->SetValue( false );
                }
            }
        }
    }
    else if( !GetFieldValue( &m_fields, SIM_DEVICE_FIELD, false, 0 ).empty()
                || !GetFieldValue( &m_fields, SIM_DEVICE_SUBTYPE_FIELD, false, 0 ).empty() )
    {
        // The model is sourced from the instance.
        m_rbBuiltinModel->SetValue( true );

        reporter.Clear();
        m_curModelType = SIM_MODEL::ReadTypeFromFields( m_fields, true, 0, reporter );

        if( reporter.HasMessage() )
            DisplayErrorMessage( this, reporter.GetMessages() );
    }

    for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
    {
        if( m_rbBuiltinModel->GetValue() && type == m_curModelType )
        {
            reporter.Clear();
            m_builtinModelsMgr.CreateModel( m_fields, true, 0, m_sortedPartPins, reporter );

            if( reporter.HasMessage() )
            {
                DisplayErrorMessage( this, _( "Failed to read simulation model from fields." )
                                                   + wxT( "\n\n" ) + reporter.GetMessages() );
            }
        }
        else
        {
            m_builtinModelsMgr.CreateModel( type, m_sortedPartPins, reporter );
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


template <typename T>
bool DIALOG_SIM_MODEL<T>::TransferDataFromWindow()
{
    if( !m_pinAssignmentsGrid->CommitPendingChanges() )
        return false;

    if( !m_paramGrid->GetGrid()->CommitChangesFromEditor() )
        return false;

    if( !DIALOG_SIM_MODEL_BASE::TransferDataFromWindow() )
        return false;

    SIM_MODEL& model = curModel();
    std::string path;
    std::string name;

    if( m_rbLibraryModel->GetValue() )
    {
        path = m_libraryPathText->GetValue();
        wxFileName fn( path );

        if( !path.starts_with( FILEEXT::KiCadUriPrefix ) && fn.MakeRelativeTo( Prj().GetProjectPath() )
            && !fn.GetFullPath().StartsWith( ".." ) )
        {
            path = fn.GetFullPath();
        }

        if( m_modelListBox->GetSelection() >= 0 )
            name = m_modelListBox->GetStringSelection().ToStdString();
        else if( dynamic_cast<SIM_MODEL_SPICE_FALLBACK*>( &model ) )
            name = GetFieldValue( &m_fields, SIM_LIBRARY::NAME_FIELD, false, 0 );
    }

    SetFieldValue( m_fields, SIM_LIBRARY::LIBRARY_FIELD, path, false );
    SetFieldValue( m_fields, SIM_LIBRARY::NAME_FIELD, name, false );

    if( isIbisLoaded() && !m_libraryModelsMgr.GetModels().empty() )
    {
        int      idx = 0;
        wxString sel = m_modelListBox->GetStringSelection();

        if( m_modelListBoxEntryToLibraryIdx.contains( sel ) )
            idx = m_modelListBoxEntryToLibraryIdx.at( sel );

        auto* ibismodel =
                static_cast<SIM_MODEL_IBIS*>( &m_libraryModelsMgr.GetModels().at( idx ).get() );

        if( ibismodel )
        {
            std::string pins;
            std::string modelName = std::string( m_pinModelCombobox->GetValue().c_str() );
            std::string differential;

            if( m_pinCombobox->GetSelection() >= 0 )
                pins = ibismodel->GetIbisPins().at( m_pinCombobox->GetSelection() ).first;

            if( ibismodel->CanDifferential() && m_differentialCheckbox->GetValue() )
                differential = "1";

            SetFieldValue( m_fields, SIM_LIBRARY_IBIS::PIN_FIELD, pins );
            SetFieldValue( m_fields, SIM_LIBRARY_IBIS::MODEL_FIELD, modelName );
            SetFieldValue( m_fields, SIM_LIBRARY_IBIS::DIFF_FIELD, differential );
        }
    }

    if( model.GetType() == SIM_MODEL::TYPE::RAWSPICE )
    {
        if( m_modelNotebook->GetSelection() == 0 )
            updateModelCodeTab( &model );

        wxString code = m_codePreview->GetText().Trim( true ).Trim( false );
        model.SetParamValue( "model", std::string( code.ToUTF8() ) );
    }

    model.SetIsStoredInValue( m_saveInValueCheckbox->GetValue() );

    for( int row = 0; row < m_pinAssignmentsGrid->GetNumberRows(); ++row )
    {
        wxString modelPinName = m_pinAssignmentsGrid->GetCellValue( row, PIN_COLUMN::MODEL );
        wxString symbolPinName = m_sortedPartPins.at( row )->GetShownNumber();

        model.AssignSymbolPinNumberToModelPin( getModelPinIndex( modelPinName ),
                                               std::string( symbolPinName.ToUTF8() ) );
    }

    removeOrphanedPinAssignments( &model );

    curModel().WriteFields( m_fields );

    return true;
}


template <typename T>
void DIALOG_SIM_MODEL<T>::updateWidgets()
{
    // always enable the library browser button -- it makes for fewer clicks if the user has a
    // whole bunch of inferred passives that they want to specify library models for
    m_browseButton->Enable();

    // if we're in an undetermined state then enable everything for faster access
    bool undetermined = !m_rbLibraryModel->GetValue() && !m_rbBuiltinModel->GetValue();
    bool enableLibCtrls = m_rbLibraryModel->GetValue() || undetermined;
    bool enableBuiltinCtrls = m_rbBuiltinModel->GetValue() || undetermined;

    m_pathLabel->Enable( enableLibCtrls );
    m_libraryPathText->Enable( enableLibCtrls );
    m_modelNameLabel->Enable( enableLibCtrls );
    m_modelFilter->Enable( enableLibCtrls && !isIbisLoaded() );
    m_modelListBox->Enable( enableLibCtrls );
    m_pinLabel->Enable( enableLibCtrls );
    m_pinCombobox->Enable( enableLibCtrls );
    m_differentialCheckbox->Enable( enableLibCtrls );
    m_pinModelLabel->Enable( enableLibCtrls );
    m_pinModelCombobox->Enable( enableLibCtrls );
    m_waveformLabel->Enable( enableLibCtrls );
    m_waveformChoice->Enable( enableLibCtrls );

    m_deviceLabel->Enable( enableBuiltinCtrls );
    m_deviceChoice->Enable( enableBuiltinCtrls );
    m_deviceSubtypeLabel->Enable( enableBuiltinCtrls );
    m_deviceSubtypeChoice->Enable( enableBuiltinCtrls );

    SIM_MODEL* model = &curModel();

    updateIbisWidgets( model );
    updateBuiltinModelWidgets( model );
    updateModelParamsTab( model );
    updateModelCodeTab( model );
    updatePinAssignments( model, false );

    std::string ref = GetFieldValue( &m_fields, SIM_REFERENCE_FIELD, false, 0 );

    m_modelPanel->Layout();
    m_pinAssignmentsPanel->Layout();
    m_parametersPanel->Layout();
    m_codePanel->Layout();

    SendSizeEvent( wxSEND_EVENT_POST );

    m_prevModel = &curModel();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::updateIbisWidgets( SIM_MODEL* aModel )
{
    SIM_MODEL_IBIS* modelibis = isIbisLoaded() ? dynamic_cast<SIM_MODEL_IBIS*>( aModel ) : nullptr;

    m_pinLabel->Show( isIbisLoaded() );
    m_pinCombobox->Show( isIbisLoaded() );
    m_pinModelLabel->Show( isIbisLoaded() );
    m_pinModelCombobox->Show( isIbisLoaded() );
    m_waveformLabel->Show( isIbisLoaded() );
    m_waveformChoice->Show( isIbisLoaded() );

    if( aModel != m_prevModel )
    {
        m_waveformChoice->Clear();

        if( isIbisLoaded() )
        {
            for( SIM_MODEL::TYPE type : { SIM_MODEL::TYPE::KIBIS_DEVICE,
                                          SIM_MODEL::TYPE::KIBIS_DRIVER_DC,
                                          SIM_MODEL::TYPE::KIBIS_DRIVER_RECT,
                                          SIM_MODEL::TYPE::KIBIS_DRIVER_PRBS } )
            {
                SIM_MODEL::DEVICE_T deviceType = SIM_MODEL::TypeInfo( type ).deviceType;
                const std::string& deviceTypeDesc = SIM_MODEL::DeviceInfo( deviceType ).description;

                if( deviceType == aModel->GetDeviceType()
                    || deviceTypeDesc == aModel->GetDeviceInfo().description )
                {
                    m_waveformChoice->Append( SIM_MODEL::TypeInfo( type ).description );

                    if( type == aModel->GetType() )
                        m_waveformChoice->SetSelection( m_waveformChoice->GetCount() - 1 );
                }
            }
        }
    }

    m_differentialCheckbox->Show( isIbisLoaded() && modelibis && modelibis->CanDifferential() );
    m_modelNameLabel->SetLabel( isIbisLoaded() ? _( "Component:" ) : _( "Model:" ) );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::updateBuiltinModelWidgets( SIM_MODEL* aModel )
{
    // Change the Type choice to match the current device type.
    if( aModel != m_prevModel )
    {
        m_deviceChoice->Clear();
        m_deviceSubtypeChoice->Clear();

        if( !m_rbLibraryModel->GetValue() )
        {
            for( SIM_MODEL::DEVICE_T deviceType : SIM_MODEL::DEVICE_T_ITERATOR() )
            {
                if( !SIM_MODEL::DeviceInfo( deviceType ).showInMenu )
                    continue;

                m_deviceChoice->Append( SIM_MODEL::DeviceInfo( deviceType ).description );

                if( equivalent( deviceType, aModel->GetDeviceType() ) )
                    m_deviceChoice->SetSelection( m_deviceChoice->GetCount() - 1 );
            }

            for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
            {
                if( type == SIM_MODEL::TYPE::KIBIS_DEVICE
                        || type == SIM_MODEL::TYPE::KIBIS_DRIVER_DC
                        || type == SIM_MODEL::TYPE::KIBIS_DRIVER_RECT
                        || type == SIM_MODEL::TYPE::KIBIS_DRIVER_PRBS )
                {
                    continue;
                }

                SIM_MODEL::DEVICE_T deviceType = SIM_MODEL::TypeInfo( type ).deviceType;
                const std::string& deviceTypeDesc = SIM_MODEL::DeviceInfo( deviceType ).description;

                if( deviceType == aModel->GetDeviceType()
                    || deviceTypeDesc == aModel->GetDeviceInfo().description )
                {
                    m_deviceSubtypeChoice->Append( SIM_MODEL::TypeInfo( type ).description );

                    if( type == aModel->GetType() )
                        m_deviceSubtypeChoice->SetSelection( m_deviceSubtypeChoice->GetCount() - 1 );
                }
            }
        }

        m_deviceSubtypeLabel->Show( m_deviceSubtypeChoice->GetCount() > 1 );
        m_deviceSubtypeChoice->Show( m_deviceSubtypeChoice->GetCount() > 1 );
    }

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


template <typename T>
void DIALOG_SIM_MODEL<T>::updateModelParamsTab( SIM_MODEL* aModel )
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


template <typename T>
void DIALOG_SIM_MODEL<T>::updateModelCodeTab( SIM_MODEL* aModel )
{
    if( dynamic_cast<SIM_MODEL_SPICE_FALLBACK*>( aModel ) )
        return;

    wxString   text;
    SPICE_ITEM item;

    item.modelName = m_modelListBox->GetStringSelection();

    if( m_rbBuiltinModel->GetValue() || item.modelName == "" )
        item.modelName = GetFieldValue( &m_fields, FIELD_T::REFERENCE );

    text << aModel->SpiceGenerator().Preview( item );

    m_codePreview->SetText( text );
    m_codePreview->SelectNone();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::updatePinAssignments( SIM_MODEL* aModel, bool aForceRefreshFromModel )
{
    if( m_pinAssignmentsGrid->GetNumberRows() == 0 )
    {
        m_pinAssignmentsGrid->AppendRows( (int) m_sortedPartPins.size() );

        for( int ii = 0; ii < m_pinAssignmentsGrid->GetNumberRows(); ++ii )
        {
            wxString symbolPinString = getSymbolPinString( ii );

            m_pinAssignmentsGrid->SetReadOnly( ii, PIN_COLUMN::SYMBOL );
            m_pinAssignmentsGrid->SetCellValue( ii, PIN_COLUMN::SYMBOL, symbolPinString );
        }

        aForceRefreshFromModel = true;
    }

    if( aForceRefreshFromModel )
    {
        // Reset the grid.
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
    }

    for( int ii = 0; ii < m_pinAssignmentsGrid->GetNumberRows(); ++ii )
    {
        // Set up the Model column cell editors with dropdown options.
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

        // This is not a memory leak; `SetCellEditor()` calls `DecRef()` on its previous editor.
        m_pinAssignmentsGrid->SetCellEditor( ii, PIN_COLUMN::MODEL,
                                             new GRID_CELL_ICON_TEXT_POPUP( modelPinIcons, modelPinChoices ) );

        // Assignment stays the same, but model pin names need to be updated
        int modelPinIndex = getModelPinIndex( m_pinAssignmentsGrid->GetCellValue( ii, PIN_COLUMN::MODEL ) );

        if( modelPinIndex >= 0 )
            m_pinAssignmentsGrid->SetCellValue( ii, PIN_COLUMN::MODEL, getModelPinString( aModel, modelPinIndex ) );
    }

    // TODO: Show a preview of the symbol with the pin numbers shown.

    if( aModel->GetType() == SIM_MODEL::TYPE::SUBCKT )
    {
        SIM_MODEL_SUBCKT* subckt = static_cast<SIM_MODEL_SUBCKT*>( aModel );
        m_subckt->SetText( subckt->GetSpiceCode() );
        m_subckt->SetEditable( false );
    }
    else
    {
        m_subcktLabel->Show( false );
        m_subckt->Show( false );
    }
}


template <typename T>
void DIALOG_SIM_MODEL<T>::removeOrphanedPinAssignments( SIM_MODEL* aModel )
{
    for( int i = 0; i < aModel->GetPinCount(); ++i )
    {
        if( !m_symbol.GetPin( aModel->GetPin( i ).symbolPinNumber ) )
            aModel->AssignSymbolPinNumberToModelPin( i, "" );
    }
}


template <typename T>
bool DIALOG_SIM_MODEL<T>::loadLibrary( const wxString& aLibraryPath, REPORTER& aReporter, bool aForceReload )
{
    if( m_prevLibrary == aLibraryPath && !aForceReload )
        return true;

    m_libraryModelsMgr.SetForceFullParse();
    m_libraryModelsMgr.SetLibrary( aLibraryPath, aReporter );
    m_libraryPathText->ChangeValue( aLibraryPath );
    m_modelListBoxEntryToLibraryIdx.clear();

    if( aReporter.HasMessageOfSeverity( RPT_SEVERITY_UNDEFINED | RPT_SEVERITY_ERROR ) )
    {
        m_libraryModelsMgr.Clear();

        if( m_modelListBox->GetSelection() != wxNOT_FOUND )
            m_modelListBox->SetSelection( wxNOT_FOUND );

        if( m_modelListBox->GetCount() )
            m_modelListBox->Clear();

        wxArrayString emptyArray;
        m_pinModelCombobox->Set( emptyArray );
        m_pinCombobox->Set( emptyArray );
        m_pinModelCombobox->SetSelection( -1 );
        m_pinCombobox->SetSelection( -1 );
        m_waveformChoice->Clear();
        m_prevLibrary.Clear();

        return false;
    }

    std::string modelName = GetFieldValue( &m_fields, SIM_LIBRARY::NAME_FIELD, true, 0 );

    for( const auto& [baseModelName, baseModel] : library()->GetModels() )
    {
        if( baseModelName == modelName )
            m_libraryModelsMgr.CreateModel( &baseModel, m_sortedPartPins, m_fields, true, 0, aReporter );
        else
            m_libraryModelsMgr.CreateModel( &baseModel, m_sortedPartPins, aReporter );
    }

    m_rbLibraryModel->SetValue( true );

    wxArrayString modelNames;

    bool modelNameExists = false;
    for( const auto& [name, model] : library()->GetModels() )
    {
        modelNames.Add( name );
        m_modelListBoxEntryToLibraryIdx[name] = m_modelListBoxEntryToLibraryIdx.size();

        if( name == modelName )
            modelNameExists = true;
    }

    modelNames.Sort();

    m_modelListBox->Clear();
    m_modelListBox->Append( modelNames );

    if( !modelNameExists )
    {
        m_infoBar->ShowMessage( wxString::Format( _( "No model named '%s' in '%s'." ),
                                                  modelName,
                                                  aLibraryPath ) );
        return false;
    }

    if( isIbisLoaded() )
    {
        wxArrayString emptyArray;
        m_pinModelCombobox->Set( emptyArray );
        m_pinCombobox->Set( emptyArray );
        m_pinModelCombobox->SetSelection( -1 );
        m_pinCombobox->SetSelection( -1 );
    }

    m_modelListBox->SetStringSelection( modelName );

    if( m_modelListBox->GetSelection() < 0 && m_modelListBox->GetCount() > 0 )
        m_modelListBox->SetSelection( 0 );

    m_curModelType = curModel().GetType();

    m_prevLibrary = aLibraryPath;
    return true;
}


template <typename T>
void DIALOG_SIM_MODEL<T>::addParamPropertyIfRelevant( SIM_MODEL* aModel, int aParamIndex )
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


template <typename T>
wxPGProperty* DIALOG_SIM_MODEL<T>::newParamProperty( SIM_MODEL* aModel, int aParamIndex ) const
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
        // Special case: K-line mutual inductance statement parameters l1 and l2 are references
        // to other inductors in the circuit.
        if( dynamic_cast<SIM_MODEL_L_MUTUAL*>( aModel ) != nullptr
                    && ( param.info.name == "l1" || param.info.name == "l2" ) )
        {
            wxArrayString inductors;

            if( SCH_EDIT_FRAME* schEditFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
            {
                SPICE_CIRCUIT_MODEL circuit( &schEditFrame->Schematic() );
                NULL_REPORTER       devNul;

                circuit.ReadSchematicAndLibraries( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS, devNul );

                for( const SPICE_ITEM& item : circuit.GetItems() )
                {
                    if( item.model->GetDeviceType() == SIM_MODEL::DEVICE_T::L )
                        inductors.push_back( item.refName );
                }

                inductors.Sort(
                        []( const wxString& a, const wxString& b ) -> int
                        {
                            return StrNumCmp( a, b, true );
                        } );
            }

            if( inductors.empty() )
            {
                prop = new SIM_STRING_PROPERTY( paramDescription, param.info.name, *aModel, aParamIndex,
                                                SIM_VALUE::TYPE_STRING );
            }
            else
            {
                prop = new SIM_ENUM_PROPERTY( paramDescription, param.info.name, *aModel, aParamIndex,
                                              inductors );
            }
        }
        else if( param.info.enumValues.empty() )
        {
            prop = new SIM_STRING_PROPERTY( paramDescription, param.info.name, *aModel, aParamIndex,
                                            SIM_VALUE::TYPE_STRING );
        }
        else
        {
            wxArrayString values;

            for( const std::string& string : aModel->GetParam( aParamIndex ).info.enumValues )
                values.Add( string );

            prop = new SIM_ENUM_PROPERTY( paramDescription, param.info.name, *aModel, aParamIndex,
                                          values );
        }
        break;

    default:
        prop = new wxStringProperty( paramDescription, param.info.name );
        break;
    }

    prop->SetAttribute( wxPG_ATTR_UNITS, wxString::FromUTF8( param.info.unit.c_str() ) );

    // Legacy due to the way we extracted the parameters from Ngspice.
    prop->SetCell( 3, wxString::FromUTF8( param.info.defaultValue ) );

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


template <typename T>
int DIALOG_SIM_MODEL<T>::findSymbolPinRow( const wxString& aSymbolPinNumber ) const
{
    for( int row = 0; row < static_cast<int>( m_sortedPartPins.size() ); ++row )
    {
        SCH_PIN* pin = m_sortedPartPins[row];

        if( pin->GetNumber() == aSymbolPinNumber )
            return row;
    }

    return -1;
}


template <typename T>
SIM_MODEL& DIALOG_SIM_MODEL<T>::curModel() const
{
    if( m_rbLibraryModel->GetValue() )
    {
        wxString sel = m_modelListBox->GetStringSelection();

        if( m_modelListBoxEntryToLibraryIdx.contains( sel ) )
        {
            int idx = m_modelListBoxEntryToLibraryIdx.at( sel );

            if( idx >= 0 && idx < (int) m_libraryModelsMgr.GetModels().size() )
                return m_libraryModelsMgr.GetModels().at( idx ).get();
        }
    }
    else
    {
        if( (int) m_curModelType < (int) m_builtinModelsMgr.GetModels().size() )
            return m_builtinModelsMgr.GetModels().at( static_cast<int>( m_curModelType ) );
    }

    return m_builtinModelsMgr.GetModels().at( (int) SIM_MODEL::TYPE::NONE );
}


template <typename T>
const SIM_LIBRARY* DIALOG_SIM_MODEL<T>::library() const
{
    if( m_libraryModelsMgr.GetLibraries().size() == 1 )
        return &m_libraryModelsMgr.GetLibraries().begin()->second.get();

    return nullptr;
}


template <typename T>
wxString DIALOG_SIM_MODEL<T>::getSymbolPinString( int symbolPinIndex ) const
{
    SCH_PIN* pin = m_sortedPartPins.at( symbolPinIndex );
    wxString pinNumber;
    wxString pinName;

    if( pin )
    {
        pinNumber = pin->GetShownNumber();
        pinName = pin->GetShownName();
    }

    if( !pinName.IsEmpty() && pinName != pinNumber )
        pinNumber += wxString::Format( wxT( " (\"%s\")" ), pinName );

    return pinNumber;
}


template <typename T>
wxString DIALOG_SIM_MODEL<T>::getModelPinString( SIM_MODEL* aModel, int aModelPinIndex ) const
{
    wxString modelPinName;

    if( aModelPinIndex >= 0 && aModelPinIndex < aModel->GetPinCount() )
        modelPinName = aModel->GetPin( aModelPinIndex ).modelPinName;

    wxString modelPinNumber = wxString::Format( "%d", aModelPinIndex + 1 );

    if( !modelPinName.IsEmpty() && modelPinName != modelPinNumber )
        modelPinNumber += wxString::Format( wxT( " (\"%s\")" ), modelPinName );

    return modelPinNumber;
}


template <typename T>
int DIALOG_SIM_MODEL<T>::getModelPinIndex( const wxString& aModelPinString ) const
{
    if( aModelPinString == "Not Connected" )
        return SIM_MODEL_PIN::NOT_CONNECTED;

    int length = aModelPinString.Find( " " );

    if( length == wxNOT_FOUND )
        length = static_cast<int>( aModelPinString.Length() );

    long result = 0;
    aModelPinString.Mid( 0, length ).ToCLong( &result );

    return static_cast<int>( result - 1 );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onRadioButton( wxCommandEvent& aEvent )
{
    m_prevModel = nullptr;  // Ensure the Model panel will be rebuild after updating other params.
    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onLibraryPathText( wxCommandEvent& aEvent )
{
    m_rbLibraryModel->SetValue( true );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onLibraryPathTextEnter( wxCommandEvent& aEvent )
{
    m_rbLibraryModel->SetValue( true );

    WX_STRING_REPORTER reporter;
    wxString           path = m_libraryPathText->GetValue();

    if( loadLibrary( path, reporter, true ) || path.IsEmpty() )
        m_infoBar->Hide();
    else if( reporter.HasMessage() )
        m_infoBar->ShowMessage( reporter.GetMessages() );

    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onLibraryPathTextKillFocus( wxFocusEvent& aEvent )
{
    CallAfter(
            [this]()
            {
                // Disable logging -- otherwise we'll end up in an endless loop of show-log,
                // kill-focus, show-log, kill-focus, etc.
                wxLogNull doNotLog;

                wxCommandEvent dummy;
                onLibraryPathTextEnter( dummy );
            } );

    aEvent.Skip();  // mandatory in wxFocusEvent events
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onBrowseButtonClick( wxCommandEvent& aEvent )
{
    static wxString s_mruPath;

    wxString                path = s_mruPath.IsEmpty() ? Prj().GetProjectPath() : s_mruPath;
    wxFileDialog            dlg( this, _( "Browse Models" ), path );
    FILEDLG_HOOK_EMBED_FILE customize( false );

    dlg.SetCustomizeHook( customize );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_rbLibraryModel->SetValue( true );

    path = dlg.GetPath();
    wxFileName fn( path );
    s_mruPath = fn.GetPath();

    if( customize.GetEmbed() )
    {
        EMBEDDED_FILES::EMBEDDED_FILE* result = m_filesStack[0]->AddFile( fn, false );
        path = result->GetLink();
    }
    else if( fn.MakeRelativeTo( Prj().GetProjectPath() ) && !fn.GetFullPath().StartsWith( wxS( ".." ) ) )
    {
        path = fn.GetFullPath();
    }

    WX_STRING_REPORTER reporter;

    if( loadLibrary( path, reporter, true ) )
        m_infoBar->Hide();
    else
        m_infoBar->ShowMessage( reporter.GetMessages() );

    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onFilterCharHook( wxKeyEvent& aKeyStroke )
{
    int sel = m_modelListBox->GetSelection();

    switch( aKeyStroke.GetKeyCode() )
    {
    case WXK_UP:
        if( sel == wxNOT_FOUND )
            sel = m_modelListBox->GetCount() - 1;
        else
            sel--;

        break;

    case WXK_DOWN:
        if( sel == wxNOT_FOUND )
            sel = 0;
        else
            sel++;

        break;

    case WXK_RETURN:
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
        return;

    default:
        aKeyStroke.Skip();      // Any other key: pass on to search box directly.
        return;
    }

    if( sel >= 0 && sel < (int) m_modelListBox->GetCount() )
        m_modelListBox->SetSelection( sel );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onModelFilter( wxCommandEvent& aEvent )
{
    if( library() == nullptr )  // No library loaded
        return;

    wxArrayString modelNames;
    wxString      current = m_modelListBox->GetStringSelection();
    wxString      filter = wxT( "*" ) + m_modelFilter->GetValue() + wxT( "*" );

    for( const auto& [name, model] : library()->GetModels() )
    {
        wxString wx_name( name );

        if( wx_name.Matches( filter ) )
            modelNames.Add( wx_name );
    }

    modelNames.Sort();

    m_modelListBox->Clear();
    m_modelListBox->Append( modelNames );

    if( !m_modelListBox->IsEmpty() )
    {
        if( !m_modelListBox->SetStringSelection( current ) )
            m_modelListBox->SetSelection( 0 );
    }
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onModelNameChoice( wxCommandEvent& aEvent )
{
    if( isIbisLoaded() )
    {
        wxArrayString    pinLabels;
        SIM_MODEL_IBIS* modelkibis = dynamic_cast<SIM_MODEL_IBIS*>( &curModel() );

        wxCHECK2( modelkibis, return );

        for( std::pair<wxString, wxString> strs : modelkibis->GetIbisPins() )
            pinLabels.Add( strs.first + wxT( " - " ) + strs.second );

        m_pinCombobox->Set( pinLabels );

        wxArrayString emptyArray;
        m_pinModelCombobox->Set( emptyArray );
    }

    m_rbLibraryModel->SetValue( true );

    if( SIM_MODEL_SPICE_FALLBACK* fallback =
                dynamic_cast<SIM_MODEL_SPICE_FALLBACK*>( &curModel() ) )
    {
        wxArrayString lines = wxSplit( fallback->GetSpiceCode(), '\n' );
        wxString code;

        for( const wxString& line : lines )
        {
            if( !line.StartsWith( '*' ) )
            {
                if( !code.IsEmpty() )
                    code += "\n";

                code += line;
            }
        }

        m_infoBar->ShowMessage( wxString::Format( _( "Failed to parse:\n\n"
                                                     "%s\n"
                                                     "Using generic SPICE model." ),
                                                  code ) );
    }
    else
    {
        m_infoBar->Hide();
    }

    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onPinCombobox( wxCommandEvent& aEvent )
{
    wxArrayString modelLabels;

    SIM_MODEL_IBIS& ibisModel = static_cast<SIM_MODEL_IBIS&>( curModel() );

    std::vector<std::pair<std::string, std::string>> strs = ibisModel.GetIbisPins();
    std::string pinNumber = strs.at( m_pinCombobox->GetSelection() ).first;

    const SIM_LIBRARY_IBIS* ibisLibrary = dynamic_cast<const SIM_LIBRARY_IBIS*>( library() );

    ibisModel.ChangePin( *ibisLibrary, pinNumber );

    ibisModel.m_enableDiff = ibisLibrary->isPinDiff( ibisModel.GetComponentName(), pinNumber );

    for( wxString modelName : ibisModel.GetIbisModels() )
        modelLabels.Add( modelName );

    m_pinModelCombobox->Set( modelLabels );

    if( m_pinModelCombobox->GetCount() == 1 )
        m_pinModelCombobox->SetSelection( 0 );
    else
        m_pinModelCombobox->SetSelection( -1 );

    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onPinComboboxTextEnter( wxCommandEvent& aEvent )
{
    m_pinCombobox->SetSelection( m_pinCombobox->FindString( m_pinCombobox->GetValue() ) );

    onPinModelCombobox( aEvent );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onPinModelCombobox( wxCommandEvent& aEvent )
{
    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onPinModelComboboxTextEnter( wxCommandEvent& aEvent )
{
    m_pinModelCombobox->SetSelection( m_pinModelCombobox->FindString( m_pinModelCombobox->GetValue() ) );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onDifferentialCheckbox( wxCommandEvent& aEvent )
{
    if( SIM_MODEL_IBIS* modelibis = dynamic_cast<SIM_MODEL_IBIS*>( &curModel() ) )
    {
        bool diff = m_differentialCheckbox->GetValue() && modelibis->CanDifferential();
        modelibis->SwitchSingleEndedDiff( diff );

        updateWidgets();
    }
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onDeviceTypeChoice( wxCommandEvent& aEvent )
{
    m_rbBuiltinModel->SetValue( true );

    for( SIM_MODEL::DEVICE_T deviceType : SIM_MODEL::DEVICE_T_ITERATOR() )
    {
        if( SIM_MODEL::DeviceInfo( deviceType ).description == m_deviceChoice->GetStringSelection() )
        {
            m_curModelType = m_curModelTypeOfDeviceType.at( deviceType );
            break;
        }
    }

    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onWaveformChoice( wxCommandEvent& aEvent )
{
    SIM_MODEL::DEVICE_T deviceType = curModel().GetDeviceType();
    wxString            typeDescription = m_waveformChoice->GetStringSelection();

    for( SIM_MODEL::TYPE type : { SIM_MODEL::TYPE::KIBIS_DEVICE,
                                  SIM_MODEL::TYPE::KIBIS_DRIVER_DC,
                                  SIM_MODEL::TYPE::KIBIS_DRIVER_RECT,
                                  SIM_MODEL::TYPE::KIBIS_DRIVER_PRBS } )
    {
        if( equivalent( deviceType, SIM_MODEL::TypeInfo( type ).deviceType )
            && typeDescription == SIM_MODEL::TypeInfo( type ).description )
        {
            int      idx = 0;
            wxString sel = m_modelListBox->GetStringSelection();

            if( m_modelListBoxEntryToLibraryIdx.contains( sel ) )
                idx = m_modelListBoxEntryToLibraryIdx.at( sel );

            SIM_MODEL_IBIS& baseModel = static_cast<SIM_MODEL_IBIS&>( m_libraryModelsMgr.GetModels()[idx].get() );

            m_libraryModelsMgr.SetModel( idx, std::make_unique<SIM_MODEL_IBIS>( type, baseModel ) );

            try
            {
                m_libraryModelsMgr.GetModels()[idx].get().ReadDataFields( &m_fields, true, 0, m_sortedPartPins );
            }
            catch( IO_ERROR& err )
            {
                DisplayErrorMessage( this, err.What() );
            }

            m_curModelType = type;
            break;
        }
    }

    m_curModelTypeOfDeviceType.at( deviceType ) = m_curModelType;
    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onTypeChoice( wxCommandEvent& aEvent )
{
    SIM_MODEL::DEVICE_T deviceType = curModel().GetDeviceType();
    wxString            typeDescription = m_deviceSubtypeChoice->GetStringSelection();

    for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
    {
        if( equivalent( deviceType, SIM_MODEL::TypeInfo( type ).deviceType )
            && typeDescription == SIM_MODEL::TypeInfo( type ).description )
        {
            m_curModelType = type;
            break;
        }
    }

    m_curModelTypeOfDeviceType.at( deviceType ) = m_curModelType;
    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onPageChanging( wxBookCtrlEvent& event )
{
    updateModelCodeTab( &curModel() );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onPinAssignmentsGridCellChange( wxGridEvent& aEvent )
{
    int      symbolPinIndex = aEvent.GetRow();
    wxString oldModelPinName = aEvent.GetString();
    wxString modelPinName = m_pinAssignmentsGrid->GetCellValue( aEvent.GetRow(), aEvent.GetCol() );

    int oldModelPinIndex = getModelPinIndex( oldModelPinName );
    int modelPinIndex = getModelPinIndex( modelPinName );

    if( oldModelPinIndex != SIM_MODEL_PIN::NOT_CONNECTED )
        curModel().AssignSymbolPinNumberToModelPin( oldModelPinIndex, "" );

    if( modelPinIndex != SIM_MODEL_PIN::NOT_CONNECTED )
    {
        SCH_PIN* symbolPin = m_sortedPartPins.at( symbolPinIndex );

        curModel().AssignSymbolPinNumberToModelPin( modelPinIndex, symbolPin->GetShownNumber() );
    }

    updatePinAssignments( &curModel(), FORCE_REFRESH_FROM_MODEL );

    aEvent.Skip();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onPinAssignmentsGridSize( wxSizeEvent& aEvent )
{
    wxGridUpdateLocker deferRepaintsTillLeavingScope( m_pinAssignmentsGrid );

    int gridWidth = KIPLATFORM::UI::GetUnobscuredSize( m_pinAssignmentsGrid ).x;
    m_pinAssignmentsGrid->SetColSize( PIN_COLUMN::MODEL, gridWidth / 2 );
    m_pinAssignmentsGrid->SetColSize( PIN_COLUMN::SYMBOL, gridWidth / 2 );

    aEvent.Skip();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onParamGridSetFocus( wxFocusEvent& aEvent )
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


template <typename T>
void DIALOG_SIM_MODEL<T>::onParamGridSelectionChange( wxPropertyGridEvent& aEvent )
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


template <typename T>
void DIALOG_SIM_MODEL<T>::onUpdateUI( wxUpdateUIEvent& aEvent )
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


template <typename T>
void DIALOG_SIM_MODEL<T>::adjustParamGridColumns( int aWidth, bool aForce )
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


template <typename T>
void DIALOG_SIM_MODEL<T>::onSizeParamGrid( wxSizeEvent& event )
{
    adjustParamGridColumns( event.GetSize().GetX(), false );

    event.Skip();
}


template class DIALOG_SIM_MODEL<SCH_SYMBOL>;
template class DIALOG_SIM_MODEL<LIB_SYMBOL>;
