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


template class DIALOG_SPICE_MODEL<SCH_FIELD>;
template class DIALOG_SPICE_MODEL<LIB_FIELD>;

template <typename T>
DIALOG_SPICE_MODEL<T>::DIALOG_SPICE_MODEL( wxWindow* aParent, SCH_SYMBOL& aSymbol,
                                           std::vector<T>* aFields )
    : DIALOG_SPICE_MODEL_BASE( aParent ),
      m_symbol( aSymbol ),
      m_fields( aFields )
{
    try
    {
        SPICE_MODEL::TYPE typeFromFields = SPICE_MODEL::ReadTypeFromFields( aFields );

        for( SPICE_MODEL::TYPE type : SPICE_MODEL::TYPE_ITERATOR() )
        {
            if( type == typeFromFields )
            {
                m_models.emplace_back( aFields );
                m_curModelType = type;
            }
            else
                m_models.emplace_back( type );

            SPICE_MODEL::DEVICE_TYPE deviceType = SPICE_MODEL::TypeInfo( type ).deviceType;

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

    for( SPICE_MODEL::DEVICE_TYPE deviceType : SPICE_MODEL::DEVICE_TYPE_ITERATOR() )
        m_deviceTypeChoice->Append( SPICE_MODEL::DeviceTypeInfo( deviceType ).description );

    m_scintillaTricks = std::make_unique<SCINTILLA_TRICKS>( m_codePreview, wxT( "{}" ), false );

    Layout();
}


template <typename T>
bool DIALOG_SPICE_MODEL<T>::TransferDataFromWindow()
{
    if( !DIALOG_SPICE_MODEL_BASE::TransferDataFromWindow() )
        return false;

    m_models[static_cast<int>( m_curModelType )].WriteFields( m_fields );

    return true;
}


template <typename T>
bool DIALOG_SPICE_MODEL<T>::TransferDataToWindow()
{
    try
    {
        m_models[static_cast<int>( SPICE_MODEL::ReadTypeFromFields( m_fields ) )]
            = SPICE_MODEL( m_fields );
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
    SPICE_MODEL::DEVICE_TYPE deviceType = SPICE_MODEL::TypeInfo( m_curModelType ).deviceType;

    m_deviceTypeChoice->SetSelection( static_cast<int>( deviceType ) );
    
    m_typeChoice->Clear();

    for( SPICE_MODEL::TYPE type : SPICE_MODEL::TYPE_ITERATOR() )
    {
        if( SPICE_MODEL::TypeInfo( type ).deviceType == deviceType )
        {
            wxString description = SPICE_MODEL::TypeInfo( type ).description;

            if( !description.IsEmpty() )
                m_typeChoice->Append( description );

            if( type == m_curModelType )
                m_typeChoice->SetSelection( m_typeChoice->GetCount() - 1 );
        }
    }

    //m_typeChoice->SetSelection( static_cast<int>( m_curModelType ) );

    m_paramGrid->ClearRows();

    //if( m_model.Get

    /*if( m_model.GetSubtype() != SPICE_MODEL::SUBTYPE::NONE )
    {
        m_paramGrid->AppendRows( 1 );

        m_paramGrid->SetReadOnly( 0, COLUMN::DESCRIPTION );
        m_paramGrid->SetReadOnly( 0, COLUMN::NAME );
        m_paramGrid->SetReadOnly( 0, COLUMN::UNIT );

        m_paramGrid->SetCellValue( 0, COLUMN::VALUE,
                                   SPICE_MODEL::SubtypeInfo( m_model.GetSubtype() ).description );
    }*/
}


template <typename T>
void DIALOG_SPICE_MODEL<T>::onDeviceTypeChoice( wxCommandEvent& aEvent )
{
    //SPICE_MODEL::DEVICE_TYPE deviceType = SPICE_MODEL::TypeInfo( m_curModelType ).deviceType;
    SPICE_MODEL::DEVICE_TYPE deviceType =
        static_cast<SPICE_MODEL::DEVICE_TYPE>( m_deviceTypeChoice->GetSelection() );

    m_curModelType = m_curModelTypeOfDeviceType[deviceType];

    updateWidgets();
}


template <typename T>
void DIALOG_SPICE_MODEL<T>::onTypeChoice( wxCommandEvent& aEvent )
{
    SPICE_MODEL::DEVICE_TYPE deviceType =
        static_cast<SPICE_MODEL::DEVICE_TYPE>( m_deviceTypeChoice->GetSelection() );
    wxString typeDescription = m_typeChoice->GetString( m_typeChoice->GetSelection() );

    for( SPICE_MODEL::TYPE type : SPICE_MODEL::TYPE_ITERATOR() )
    {
        if( deviceType == SPICE_MODEL::TypeInfo( type ).deviceType
            && typeDescription == SPICE_MODEL::TypeInfo( type ).description )
        {
            m_curModelType = type;
            break;
        }
    }

    m_curModelTypeOfDeviceType[deviceType] = m_curModelType;
    updateWidgets();
}


template <typename T>
void DIALOG_SPICE_MODEL<T>::onGridCellChange( wxGridEvent& aEvent )
{
    /*updateModel();
    updateWidgets();*/
}
