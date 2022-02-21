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

#include <widgets/wx_grid.h>
#include <dialog_spice_model.h>
#include <confirm.h>

using TYPE = SIM_VALUE_BASE::TYPE;
using CATEGORY = SIM_MODEL::PARAM::CATEGORY;


template class DIALOG_SPICE_MODEL<SCH_FIELD>;
template class DIALOG_SPICE_MODEL<LIB_FIELD>;

template <typename T>
DIALOG_SPICE_MODEL<T>::DIALOG_SPICE_MODEL( wxWindow* aParent, SCH_SYMBOL& aSymbol,
                                           std::vector<T>& aFields )
    : DIALOG_SPICE_MODEL_BASE( aParent ),
      m_symbol( aSymbol ),
      m_fields( aFields ),
      m_firstCategory( nullptr )
{
    try
    {
        SIM_MODEL::TYPE typeFromFields = SIM_MODEL::ReadTypeFromFields( aFields );

        for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
        {
            if( type == typeFromFields )
            {
                m_models.push_back( SIM_MODEL::Create( aFields ) );
                m_curModelType = type;
            }
            else
                m_models.push_back( SIM_MODEL::Create( type ) );

            SIM_MODEL::DEVICE_TYPE deviceType = SIM_MODEL::TypeInfo( type ).deviceType;

            // By default choose the first model type of each device type.
            if( !m_curModelTypeOfDeviceType.count( deviceType ) )
                m_curModelTypeOfDeviceType[deviceType] = type;
        }
    }
    catch( KI_PARAM_ERROR& e )
    {
        DisplayErrorMessage( this, e.What() );
        return;
    }


    m_typeChoice->Clear();

    for( SIM_MODEL::DEVICE_TYPE deviceType : SIM_MODEL::DEVICE_TYPE_ITERATOR() )
        m_deviceTypeChoice->Append( SIM_MODEL::DeviceTypeInfo( deviceType ).description );

    m_paramGrid = m_paramGridMgr->AddPage();


    m_scintillaTricks = std::make_unique<SCINTILLA_TRICKS>( m_codePreview, wxT( "{}" ), false );

    Layout();
}


template <typename T>
bool DIALOG_SPICE_MODEL<T>::TransferDataFromWindow()
{
    if( !DIALOG_SPICE_MODEL_BASE::TransferDataFromWindow() )
        return false;

    m_models[static_cast<int>( m_curModelType )]->WriteFields( m_fields );

    return true;
}


template <typename T>
bool DIALOG_SPICE_MODEL<T>::TransferDataToWindow()
{
    try
    {
        m_models[static_cast<int>( SIM_MODEL::ReadTypeFromFields( m_fields ) )]
            = SIM_MODEL::Create( m_fields );
    }
    catch( KI_PARAM_ERROR& e )
    {
        DisplayErrorMessage( this, e.What() );
        return DIALOG_SPICE_MODEL_BASE::TransferDataToWindow();
    }

    updateWidgets();

    return DIALOG_SPICE_MODEL_BASE::TransferDataToWindow();
}


template <typename T>
void DIALOG_SPICE_MODEL<T>::updateWidgets()
{
    SIM_MODEL::DEVICE_TYPE deviceType = SIM_MODEL::TypeInfo( m_curModelType ).deviceType;

    m_deviceTypeChoice->SetSelection( static_cast<int>( deviceType ) );
    
    m_typeChoice->Clear();

    for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
    {
        if( SIM_MODEL::TypeInfo( type ).deviceType == deviceType )
        {
            wxString description = SIM_MODEL::TypeInfo( type ).description;

            if( !description.IsEmpty() )
                m_typeChoice->Append( description );

            if( type == m_curModelType )
                m_typeChoice->SetSelection( m_typeChoice->GetCount() - 1 );
        }
    }


    // This wxPropertyGridManager stuff has to be here because it segfaults in the constructor.

    m_paramGridMgr->SetColumnCount( static_cast<int>( COLUMN::END_ ) );

    m_paramGridMgr->SetColumnTitle( static_cast<int>( COLUMN::UNIT ), "Unit" );
    m_paramGridMgr->SetColumnTitle( static_cast<int>( COLUMN::DEFAULT ), "Default" );
    m_paramGridMgr->SetColumnTitle( static_cast<int>( COLUMN::TYPE ), "Type" );

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

    SIM_MODEL& curModel = *m_models[static_cast<int>( m_curModelType )];

    for( const SIM_MODEL::PARAM& param : curModel.Params() )
        addParamPropertyIfRelevant( param );

    m_paramGrid->CollapseAll();
}


template <typename T>
void DIALOG_SPICE_MODEL<T>::onDeviceTypeChoice( wxCommandEvent& aEvent )
{
    SIM_MODEL::DEVICE_TYPE deviceType =
        static_cast<SIM_MODEL::DEVICE_TYPE>( m_deviceTypeChoice->GetSelection() );

    m_curModelType = m_curModelTypeOfDeviceType.at( deviceType );

    updateWidgets();
}


template <typename T>
void DIALOG_SPICE_MODEL<T>::onTypeChoice( wxCommandEvent& aEvent )
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
void DIALOG_SPICE_MODEL<T>::addParamPropertyIfRelevant( const SIM_MODEL::PARAM& aParam )
{
    if( aParam.info.dir == SIM_MODEL::PARAM::DIR::OUT )
        return;

    switch( aParam.info.category )
    {
    case CATEGORY::DC:
        m_paramGrid->HideProperty( "DC", false );
        m_paramGrid->AppendIn( "DC", newParamProperty( aParam ) );
        break;

    case CATEGORY::CAPACITANCE:
        m_paramGrid->HideProperty( "Capacitance", false );
        m_paramGrid->AppendIn( "Capacitance", newParamProperty( aParam ) );
        break;

    case CATEGORY::TEMPERATURE:
        m_paramGrid->HideProperty( "Temperature", false );
        m_paramGrid->AppendIn( "Temperature", newParamProperty( aParam ) );
        break;

    case CATEGORY::NOISE:
        m_paramGrid->HideProperty( "Noise", false );
        m_paramGrid->AppendIn( "Noise", newParamProperty( aParam ) );
        break;

    case CATEGORY::DISTRIBUTED_QUANTITIES:
        m_paramGrid->HideProperty( "Distributed Quantities", false );
        m_paramGrid->AppendIn( "Distributed Quantities", newParamProperty( aParam ) );
        break;

    case CATEGORY::GEOMETRY:
        m_paramGrid->HideProperty( "Geometry", false );
        m_paramGrid->AppendIn( "Geometry", newParamProperty( aParam ) );
        break;

    case CATEGORY::LIMITING_VALUES:
        m_paramGrid->HideProperty( "Limiting Values", false );
        m_paramGrid->AppendIn( "Limiting Values", newParamProperty( aParam ) );
        break;

    case CATEGORY::ADVANCED:
        m_paramGrid->HideProperty( "Advanced", false );
        m_paramGrid->AppendIn( "Advanced", newParamProperty( aParam ) );
        break;

    case CATEGORY::FLAGS:
        m_paramGrid->HideProperty( "Flags", false );
        m_paramGrid->AppendIn( "Flags", newParamProperty( aParam ) );
        break;

    default:
        //m_paramGrid->AppendIn( nullptr, newParamProperty( aParam ) );
        m_paramGrid->Insert( m_firstCategory, newParamProperty( aParam ) );
        //m_paramGrid->Append( newParamProperty( aParam ) );
        break;

    case CATEGORY::INITIAL_CONDITIONS:
    case CATEGORY::SUPERFLUOUS:
        return;
    }
}

template <typename T>
wxPGProperty* DIALOG_SPICE_MODEL<T>::newParamProperty( const SIM_MODEL::PARAM& aParam ) const
{
    wxString paramDescription = wxString::Format( "%s (%s)",
                                                  aParam.info.description,
                                                  aParam.info.name );
    wxPGProperty* prop = nullptr;

    switch( aParam.info.type )
    {
    case TYPE::INT:
        prop = new wxIntProperty( paramDescription );
        break;

    case TYPE::FLOAT:
        prop = new wxFloatProperty( paramDescription );
        break;

    case TYPE::BOOL:
        prop = new wxBoolProperty( paramDescription );
        prop->SetAttribute( wxPG_BOOL_USE_CHECKBOX, true );
        break;

    default:
        prop = new wxStringProperty( paramDescription );
        break;
    }

    prop->SetAttribute( wxPG_ATTR_UNITS, aParam.info.unit );

    // Legacy due to the way we extracted parameters from Ngspice.
    if( aParam.isOtherVariant )
        prop->SetCell( 3, aParam.info.defaultValueOfOtherVariant );
    else
        prop->SetCell( 3, aParam.info.defaultValue );

    wxString typeStr;

    switch( aParam.info.type )
    {
    case TYPE::BOOL:           typeStr = wxString( "Bool"           ); break;
    case TYPE::INT:            typeStr = wxString( "Integer"        ); break;
    case TYPE::FLOAT:          typeStr = wxString( "Float"          ); break;
    case TYPE::COMPLEX:        typeStr = wxString( "Complex"        ); break;
    case TYPE::STRING:         typeStr = wxString( "String"         ); break;
    case TYPE::BOOL_VECTOR:    typeStr = wxString( "Bool Vector"    ); break;
    case TYPE::INT_VECTOR:     typeStr = wxString( "Int Vector"     ); break;
    case TYPE::FLOAT_VECTOR:   typeStr = wxString( "Float Vector"   ); break;
    case TYPE::COMPLEX_VECTOR: typeStr = wxString( "Complex Vector" ); break;
    }

    prop->SetCell( static_cast<int>( COLUMN::TYPE ), typeStr );

    return prop;
}
