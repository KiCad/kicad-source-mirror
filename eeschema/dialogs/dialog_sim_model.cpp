/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_sim_model.h>
#include <sim/sim_property.h>
#include <sim/sim_library_spice.h>
#include <widgets/wx_grid.h>
#include <kiplatform/ui.h>
#include <confirm.h>
#include <locale_io.h>
#include <wx/filedlg.h>

using TYPE = SIM_VALUE::TYPE;
using CATEGORY = SIM_MODEL::PARAM::CATEGORY;


template class DIALOG_SIM_MODEL<SCH_FIELD>;
template class DIALOG_SIM_MODEL<LIB_FIELD>;

template <typename T>
DIALOG_SIM_MODEL<T>::DIALOG_SIM_MODEL( wxWindow* aParent, SCH_SYMBOL& aSymbol,
                                           std::vector<T>& aFields )
    : DIALOG_SIM_MODEL_BASE( aParent ),
      m_symbol( aSymbol ),
      m_fields( aFields ),
      m_library( std::make_shared<SIM_LIBRARY_SPICE>() ),
      m_prevModel( nullptr ),
      m_firstCategory( nullptr )
{
    for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
    {
        m_models.push_back( SIM_MODEL::Create( type, m_symbol.GetAllPins().size() ) );

        SIM_MODEL::DEVICE_TYPE deviceType = SIM_MODEL::TypeInfo( type ).deviceType;
        
        if( !m_curModelTypeOfDeviceType.count( deviceType ) )
            m_curModelTypeOfDeviceType[deviceType] = type;
    }


    m_typeChoice->Clear();

    for( SIM_MODEL::DEVICE_TYPE deviceType : SIM_MODEL::DEVICE_TYPE_ITERATOR() )
        m_deviceTypeChoice->Append( SIM_MODEL::DeviceTypeInfo( deviceType ).description );


    m_scintillaTricks = std::make_unique<SCINTILLA_TRICKS>( m_codePreview, wxT( "{}" ), false );

    m_paramGridMgr->Bind( wxEVT_PG_SELECTED, &DIALOG_SIM_MODEL::onSelectionChange, this );

    m_paramGrid->SetValidationFailureBehavior( wxPG_VFB_STAY_IN_PROPERTY
                                               | wxPG_VFB_BEEP
                                               | wxPG_VFB_MARK_CELL );

    m_paramGrid->SetColumnProportion( static_cast<int>( PARAM_COLUMN::DESCRIPTION ), 50 );
    m_paramGrid->SetColumnProportion( static_cast<int>( PARAM_COLUMN::VALUE ), 18 );
    m_paramGrid->SetColumnProportion( static_cast<int>( PARAM_COLUMN::UNIT ), 10 );
    m_paramGrid->SetColumnProportion( static_cast<int>( PARAM_COLUMN::DEFAULT ), 12 );
    m_paramGrid->SetColumnProportion( static_cast<int>( PARAM_COLUMN::TYPE ), 10 );

    if( wxPropertyGrid* grid = m_paramGrid->GetGrid() )
    {
        grid->AddActionTrigger( wxPG_ACTION_EDIT, WXK_RETURN );
        grid->DedicateKey( WXK_RETURN );
        grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_RETURN );

        grid->DedicateKey( WXK_UP );
        grid->DedicateKey( WXK_DOWN );

        // Doesn't work for some reason.
        //grid->DedicateKey( WXK_TAB );
        //grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_TAB );
        //grid->AddActionTrigger( wxPG_ACTION_PREV_PROPERTY, WXK_TAB, wxMOD_SHIFT );
    }
    else
        wxFAIL;

    Layout();
}


template <typename T>
bool DIALOG_SIM_MODEL<T>::TransferDataToWindow()
{
    wxString libraryFilename = SIM_MODEL::GetFieldValue( &m_fields,
                                                         SIM_LIBRARY_SPICE::LIBRARY_FIELD );

    if( !libraryFilename.IsEmpty() )
    {
        // The model is sourced from a library, optionally with instance overrides.
        loadLibrary( libraryFilename );

        // Must be set before curModel() is used since the latter checks the combobox value.
        m_modelNameCombobox->SetStringSelection(
                SIM_MODEL::GetFieldValue( &m_fields, SIM_LIBRARY_SPICE::NAME_FIELD ) );

        m_overrideCheckbox->SetValue( curModel().HasNonPrincipalOverrides() );
    }
    else
    {
        // The model is sourced from the instance.
        SIM_MODEL::TYPE type = SIM_MODEL::ReadTypeFromFields( m_fields );

        try
        {
            m_models.at( static_cast<int>( SIM_MODEL::ReadTypeFromFields( m_fields ) ) )
                = SIM_MODEL::Create( m_symbol.GetAllPins().size(), m_fields );
        }
        catch( const KI_PARAM_ERROR& e )
        {
            DisplayErrorMessage( this, e.What() );
            return DIALOG_SIM_MODEL_BASE::TransferDataToWindow();
        }

        m_curModelType = type;
    }

    updateWidgets();

    return DIALOG_SIM_MODEL_BASE::TransferDataToWindow();
}


template <typename T>
bool DIALOG_SIM_MODEL<T>::TransferDataFromWindow()
{
    if( !DIALOG_SIM_MODEL_BASE::TransferDataFromWindow() )
        return false;

    if( m_useLibraryModelRadioButton->GetValue() )
    {
        SIM_MODEL::SetFieldValue( m_fields, SIM_LIBRARY_SPICE::NAME_FIELD,
                                  m_modelNameCombobox->GetValue() );

        wxString path = m_library->GetFilePath();
        wxFileName fn( path );

        if( fn.MakeRelativeTo( Prj().GetProjectPath() ) && !fn.GetFullPath().StartsWith( ".." ) )
            path = fn.GetFullPath();

        SIM_MODEL::SetFieldValue( m_fields, SIM_LIBRARY_SPICE::LIBRARY_FIELD, path );
    }

    curModel().WriteFields( m_fields );

    return true;
}


template <typename T>
void DIALOG_SIM_MODEL<T>::updateWidgets()
{
    updateModelParamsTab();
    updateModelCodeTab();
    updatePinAssignmentsTab();

    m_prevModel = &curModel();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::updateModelParamsTab()
{
    if( &curModel() != m_prevModel )
    {
        SIM_MODEL::DEVICE_TYPE deviceType = SIM_MODEL::TypeInfo( curModel().GetType() ).deviceType;
        m_deviceTypeChoice->SetSelection( static_cast<int>( deviceType ) );
        
        m_typeChoice->Clear();

        for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
        {
            if( SIM_MODEL::TypeInfo( type ).deviceType == deviceType )
            {
                wxString description = SIM_MODEL::TypeInfo( type ).description;

                if( !description.IsEmpty() )
                    m_typeChoice->Append( description );

                if( type == curModel().GetType() )
                    m_typeChoice->SetSelection( m_typeChoice->GetCount() - 1 );
            }
        }


        // This wxPropertyGridManager column and header stuff has to be here because it segfaults in
        // the constructor.

        m_paramGridMgr->SetColumnCount( static_cast<int>( PARAM_COLUMN::END_ ) );

        m_paramGridMgr->SetColumnTitle( static_cast<int>( PARAM_COLUMN::UNIT ), "Unit" );
        m_paramGridMgr->SetColumnTitle( static_cast<int>( PARAM_COLUMN::DEFAULT ), "Default" );
        m_paramGridMgr->SetColumnTitle( static_cast<int>( PARAM_COLUMN::TYPE ), "Type" );

        m_paramGridMgr->ShowHeader();


        m_paramGrid->Clear();

        m_firstCategory = m_paramGrid->Append( new wxPropertyCategory( "DC" ) );
        m_paramGrid->HideProperty( "DC" );

        m_paramGrid->Append( new wxPropertyCategory( "Temperature" ) );
        m_paramGrid->HideProperty( "Temperature" );

        m_paramGrid->Append( new wxPropertyCategory( "Noise" ) );
        m_paramGrid->HideProperty( "Noise" );

        m_paramGrid->Append( new wxPropertyCategory( "Distributed Quantities" ) );
        m_paramGrid->HideProperty( "Distributed Quantities" );

        m_paramGrid->Append( new wxPropertyCategory( "Geometry" ) );
        m_paramGrid->HideProperty( "Geometry" );

        m_paramGrid->Append( new wxPropertyCategory( "Limiting Values" ) );
        m_paramGrid->HideProperty( "Limiting Values" );

        m_paramGrid->Append( new wxPropertyCategory( "Advanced" ) );
        m_paramGrid->HideProperty( "Advanced" );

        m_paramGrid->Append( new wxPropertyCategory( "Flags" ) );
        m_paramGrid->HideProperty( "Flags" );

        for( int i = 0; i < curModel().GetParamCount(); ++i )
            addParamPropertyIfRelevant( i );

        m_paramGrid->CollapseAll();
    }

    // Either enable all properties or disable all except the principal ones.
    for( wxPropertyGridIterator it = m_paramGrid->GetIterator(); !it.AtEnd(); ++it )
    {
        SIM_PROPERTY* prop = dynamic_cast<SIM_PROPERTY*>( *it );

        if( !prop ) // Not all properties are SIM_PROPERTY yet. TODO.
            continue;

        // Model values other than the currently edited value may have changed. Update them.
        // This feature is called "autofill" and present only in certain models. Don't do it for
        // models that don't have it for performance reasons.
        if( curModel().HasAutofill() )
            prop->SetValueFromString( prop->GetParam().value->ToString() );

        // Most of the values are disabled when the override checkbox is unchecked.
        prop->Enable( m_useInstanceModelRadioButton->GetValue()
            || prop->GetParam().info.category == CATEGORY::PRINCIPAL
            || m_overrideCheckbox->GetValue() );
    }
}


template <typename T>
void DIALOG_SIM_MODEL<T>::updateModelCodeTab()
{
    wxString modelName = m_modelNameCombobox->GetStringSelection();

    if( m_useInstanceModelRadioButton->GetValue() || modelName.IsEmpty() )
        modelName = m_fields.at( REFERENCE_FIELD ).GetText();

    m_codePreview->SetText( curModel().GenerateSpicePreview( modelName ) );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::updatePinAssignmentsTab()
{
    if( &curModel() == m_prevModel )
        return;

    m_pinAssignmentsGrid->ClearRows();
    std::vector<SCH_PIN*> pinList = m_symbol.GetAllPins();

    m_pinAssignmentsGrid->AppendRows( static_cast<int>( pinList.size() ) );

    for( int i = 0; i < m_pinAssignmentsGrid->GetNumberRows(); ++i )
    {
        wxString symbolPinString = getSymbolPinString( i + 1 );

        m_pinAssignmentsGrid->SetReadOnly( i, static_cast<int>( PIN_COLUMN::SYMBOL ) );
        m_pinAssignmentsGrid->SetCellValue( i, static_cast<int>( PIN_COLUMN::SYMBOL ),
                                            symbolPinString );

        // Using `new` here shouldn't cause a memory leak because `SetCellEditor()` calls
        // `DecRef()` on its last editor.
        m_pinAssignmentsGrid->SetCellEditor( i, static_cast<int>( PIN_COLUMN::MODEL ),
                                             new wxGridCellChoiceEditor() );
        m_pinAssignmentsGrid->SetCellValue( i, static_cast<int>( PIN_COLUMN::MODEL ),
                                            "Not Connected" );
    }

    for( int i = 0; i < curModel().GetPinCount(); ++i )
    {
        int symbolPinNumber = curModel().GetPin( i ).symbolPinNumber;

        if( symbolPinNumber == SIM_MODEL::PIN::NOT_CONNECTED )
            continue;

        wxString modelPinString = getModelPinString( i + 1 );
        wxArrayString choices; 

        m_pinAssignmentsGrid->SetCellValue( symbolPinNumber - 1,
                                            static_cast<int>( PIN_COLUMN::MODEL ),
                                            modelPinString );
    }

    updatePinAssignmentsGridEditors();

    // TODO: Show a preview of the symbol with the pin numbers shown.
    // TODO: Maybe show a preview of the code for subcircuits and code models.
}


template <typename T>
void DIALOG_SIM_MODEL<T>::updatePinAssignmentsGridEditors()
{
    wxString modelPinChoicesString = "";
    bool isFirst = true;

    for( int i = 0; i < curModel().GetPinCount(); ++i )
    {
        const SIM_MODEL::PIN& modelPin = curModel().GetPin( i );
        int modelPinNumber = static_cast<int>( i + 1 );

        if( modelPin.symbolPinNumber != SIM_MODEL::PIN::NOT_CONNECTED )
            continue;

        if( isFirst )
        {
            modelPinChoicesString << getModelPinString( modelPinNumber );
            isFirst = false;
        }
        else
            modelPinChoicesString << "," << getModelPinString( modelPinNumber );
    }

    if( !isFirst )
        modelPinChoicesString << ",";

    modelPinChoicesString << "Not Connected";

    for( int i = 0; i < m_pinAssignmentsGrid->GetNumberRows(); ++i )
    {
        wxGridCellChoiceEditor* editor = static_cast<wxGridCellChoiceEditor*>(
            m_pinAssignmentsGrid->GetCellEditor( i, static_cast<int>( PIN_COLUMN::MODEL ) ) );

        if( !editor )
        {
            // Shouldn't happen.
            wxFAIL_MSG( "Grid cell editor is null" );
            return;
        }

        wxString curModelPinString = m_pinAssignmentsGrid->GetCellValue(
                i, static_cast<int>( PIN_COLUMN::MODEL ) );

        if( curModelPinString == "Not Connected" )
            editor->SetParameters( modelPinChoicesString );
        else
            editor->SetParameters( curModelPinString + "," + modelPinChoicesString );

    }
}


template <typename T>
void DIALOG_SIM_MODEL<T>::loadLibrary( const wxString& aFilePath )
{
    m_library->ReadFile( Prj().AbsolutePath( aFilePath ) );
    m_libraryPathInput->SetValue( aFilePath );

    m_libraryModels.clear();
    for( const SIM_MODEL& baseModel : m_library->GetModels() )
        m_libraryModels.push_back( SIM_MODEL::Create( baseModel, m_symbol.GetAllPins().size(),
                                                      m_fields ) );

    m_modelNameCombobox->Clear();
    for( const wxString& name : m_library->GetModelNames() )
        m_modelNameCombobox->Append( name );

    m_useLibraryModelRadioButton->SetValue( true );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::addParamPropertyIfRelevant( int aParamIndex )
{
    if( curModel().GetParam( aParamIndex ).info.dir == SIM_MODEL::PARAM::DIR::OUT )
        return;

    switch( curModel().GetParam( aParamIndex ).info.category )
    {
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

template <typename T>
wxPGProperty* DIALOG_SIM_MODEL<T>::newParamProperty( int aParamIndex ) const
{
    const SIM_MODEL::PARAM& param = curModel().GetParam( aParamIndex );
    wxString paramDescription;

    if( !param.info.description.IsEmpty() )
        paramDescription = wxString::Format( "%s (%s)",
                                             param.info.description,
                                             param.info.name );
    else
        paramDescription = wxString::Format( "%s", param.info.name );

    wxPGProperty* prop = nullptr;

    switch( param.info.type )
    {
    case TYPE::INT:
        prop = new SIM_PROPERTY( paramDescription, param.info.name, m_library, curModelSharedPtr(),
                                 aParamIndex, SIM_VALUE::TYPE::INT );
        break;

    case TYPE::FLOAT:
        prop = new SIM_PROPERTY( paramDescription, param.info.name, m_library, curModelSharedPtr(),
                                 aParamIndex, SIM_VALUE::TYPE::FLOAT );
        break;

    case TYPE::BOOL:
        prop = new wxBoolProperty( paramDescription, param.info.name );
        prop->SetAttribute( wxPG_BOOL_USE_CHECKBOX, true );
        break;

    default:
        prop = new wxStringProperty( paramDescription, param.info.name );
        break;
    }

    prop->SetAttribute( wxPG_ATTR_UNITS, param.info.unit );

    // Legacy due to the way we extracted the parameters from Ngspice.
    if( param.isOtherVariant )
        prop->SetCell( 3, param.info.defaultValueOfOtherVariant );
    else
        prop->SetCell( 3, param.info.defaultValue );

    wxString typeStr;

    switch( param.info.type )
    {
    case TYPE::BOOL:           typeStr = wxString( "Bool"           ); break;
    case TYPE::INT:            typeStr = wxString( "Int"            ); break;
    case TYPE::FLOAT:          typeStr = wxString( "Float"          ); break;
    case TYPE::COMPLEX:        typeStr = wxString( "Complex"        ); break;
    case TYPE::STRING:         typeStr = wxString( "String"         ); break;
    case TYPE::BOOL_VECTOR:    typeStr = wxString( "Bool Vector"    ); break;
    case TYPE::INT_VECTOR:     typeStr = wxString( "Int Vector"     ); break;
    case TYPE::FLOAT_VECTOR:   typeStr = wxString( "Float Vector"   ); break;
    case TYPE::COMPLEX_VECTOR: typeStr = wxString( "Complex Vector" ); break;
    }

    prop->SetCell( static_cast<int>( PARAM_COLUMN::TYPE ), typeStr );

    if( m_useLibraryModelRadioButton->GetValue()
        && !m_overrideCheckbox->GetValue()
        && param.info.category != SIM_MODEL::PARAM::CATEGORY::PRINCIPAL )
    {
        prop->Enable( false );
    }

    return prop;
}


template <typename T>
SIM_MODEL& DIALOG_SIM_MODEL<T>::curModel() const
{
    return *curModelSharedPtr();
}


template <typename T>
std::shared_ptr<SIM_MODEL> DIALOG_SIM_MODEL<T>::curModelSharedPtr() const
{
    if( m_useLibraryModelRadioButton->GetValue()
        && m_modelNameCombobox->GetSelection() != wxNOT_FOUND )
    {
        return m_libraryModels.at( m_modelNameCombobox->GetSelection() );
    }
    else
        return m_models.at( static_cast<int>( m_curModelType ) );
}


template <typename T>
wxString DIALOG_SIM_MODEL<T>::getSymbolPinString( int symbolPinNumber ) const
{
    wxString name = "";
    SCH_PIN* symbolPin = m_symbol.GetAllPins().at( symbolPinNumber - 1 );

    if( symbolPin )
        name = symbolPin->GetShownName();

    LOCALE_IO toggle;

    if( name.IsEmpty() )
        return wxString::Format( "%d", symbolPinNumber );
    else
        return wxString::Format( "%d (%s)", symbolPinNumber, symbolPin->GetShownName() );
}


template <typename T>
wxString DIALOG_SIM_MODEL<T>::getModelPinString( int modelPinNumber ) const
{
    const wxString& pinName = curModel().GetPin( modelPinNumber - 1 ).name;

    LOCALE_IO toggle;

    if( pinName.IsEmpty() )
        return wxString::Format( "%d", modelPinNumber, pinName );
    else
        return wxString::Format( "%d (%s)", modelPinNumber, pinName );
}


template <typename T>
int DIALOG_SIM_MODEL<T>::getModelPinNumber( const wxString& aModelPinString ) const
{
    if( aModelPinString == "Not Connected" )
        return SIM_MODEL::PIN::NOT_CONNECTED;

    int length = aModelPinString.Find( " " );

    if( length == wxNOT_FOUND )
        length = static_cast<int>( aModelPinString.Length() );

    long result = 0;
    aModelPinString.Mid( 0, length ).ToCLong( &result );

    return static_cast<int>( result );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onRadioButton( wxCommandEvent& aEvent )
{
    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onBrowseButtonClick( wxCommandEvent& aEvent )
{
    wxFileDialog dlg( this, _( "Browse Models" ), Prj().GetProjectPath() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString path = dlg.GetPath();
    wxFileName fn( path );

    if( fn.MakeRelativeTo( Prj().GetProjectPath() ) && !fn.GetFullPath().StartsWith( ".." ) )
        path = fn.GetFullPath();

    loadLibrary( path );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onModelNameCombobox( wxCommandEvent& aEvent )
{
    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onOverrideCheckbox( wxCommandEvent& aEvent )
{
    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onDeviceTypeChoice( wxCommandEvent& aEvent )
{
    SIM_MODEL::DEVICE_TYPE deviceType =
        static_cast<SIM_MODEL::DEVICE_TYPE>( m_deviceTypeChoice->GetSelection() );

    m_curModelType = m_curModelTypeOfDeviceType.at( deviceType );

    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onTypeChoice( wxCommandEvent& aEvent )
{
    SIM_MODEL::DEVICE_TYPE deviceType =
        static_cast<SIM_MODEL::DEVICE_TYPE>( m_deviceTypeChoice->GetSelection() );
    wxString typeDescription = m_typeChoice->GetString( m_typeChoice->GetSelection() );

    for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
    {
        if( deviceType == SIM_MODEL::TypeInfo( type ).deviceType
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
void DIALOG_SIM_MODEL<T>::onParamGridChanged( wxPropertyGridEvent& aEvent )
{
    updateWidgets();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onPinAssignmentsGridCellChange( wxGridEvent& aEvent )
{
    int symbolPinNumber = aEvent.GetRow() + 1;
    int oldModelPinNumber = getModelPinNumber( aEvent.GetString() );
    int modelPinNumber = getModelPinNumber(
            m_pinAssignmentsGrid->GetCellValue( aEvent.GetRow(), aEvent.GetCol() ) );

    if( oldModelPinNumber != SIM_MODEL::PIN::NOT_CONNECTED )
        curModel().SetPinSymbolPinNumber( oldModelPinNumber - 1, SIM_MODEL::PIN::NOT_CONNECTED );

    if( modelPinNumber != SIM_MODEL::PIN::NOT_CONNECTED )
        curModel().SetPinSymbolPinNumber( modelPinNumber - 1, symbolPinNumber );

    updatePinAssignmentsGridEditors();

    aEvent.Skip();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onPinAssignmentsGridSize( wxSizeEvent& aEvent )
{
    wxGridUpdateLocker deferRepaintsTillLeavingScope( m_pinAssignmentsGrid );

    int gridWidth = KIPLATFORM::UI::GetUnobscuredSize( m_pinAssignmentsGrid ).x;
    m_pinAssignmentsGrid->SetColSize( static_cast<int>( PIN_COLUMN::MODEL ), gridWidth / 2 );
    m_pinAssignmentsGrid->SetColSize( static_cast<int>( PIN_COLUMN::SYMBOL ), gridWidth / 2 );

    aEvent.Skip();
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onLibraryFilenameInputUpdate( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_useLibraryModelRadioButton->GetValue() );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onBrowseButtonUpdate( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_useLibraryModelRadioButton->GetValue() );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onModelNameComboboxUpdate( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_useLibraryModelRadioButton->GetValue() );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onOverrideCheckboxUpdate( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_useLibraryModelRadioButton->GetValue() );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onDeviceTypeChoiceUpdate( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_useInstanceModelRadioButton->GetValue() );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onTypeChoiceUpdate( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_useInstanceModelRadioButton->GetValue() );
}


template <typename T>
void DIALOG_SIM_MODEL<T>::onSelectionChange( wxPropertyGridEvent& aEvent )
{
    // TODO: Activate also when the whole property grid is selected with tab key.

    wxPropertyGrid* grid = m_paramGrid->GetGrid();
    if( !grid )
    {
        wxFAIL;
        return;
    }

    wxWindow* editorControl = grid->GetEditorControl();
    if( !editorControl )
    {
        wxFAIL;
        return;
    }
    
    // Without this, the user had to press tab before they could edit the field.
    editorControl->SetFocus();
}


/*template <typename T>
void DIALOG_SPICE_MODEL<T>::onPropertyChanged( wxPropertyGridEvent& aEvent )
{
    wxString name = aEvent.GetPropertyName();
    
    for( SIM_MODEL::PARAM& param : getCurModel().Params() )
    {
        if( param.info.name == name )
        {
            try
            {
                param.value->FromString( m_paramGrid->GetPropertyValueAsString( param.info.name ) );
            }
            catch( KI_PARAM_ERROR& e )
            {
                DisplayErrorMessage( this, e.What() );
            }
        }
    }
}*/
