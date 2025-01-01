/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre.charras
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/msgdlg.h>

#include <calculator_panels/panel_regulator.h>
#include <class_regulator_data.h>
#include <pcb_calculator_frame.h>
#include "dialog_regulator_form.h"


extern double DoubleFromString( const wxString& TextValue );

DIALOG_REGULATOR_FORM::DIALOG_REGULATOR_FORM( wxWindow* parent, const wxString& aRegName ) :
        DIALOG_REGULATOR_FORM_BASE( parent )
{
    m_textCtrlName->SetValue( aRegName );
    m_textCtrlName->Enable( aRegName.IsEmpty() );
    UpdateDialog();

    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}

DIALOG_REGULATOR_FORM::~DIALOG_REGULATOR_FORM()
{
}


bool DIALOG_REGULATOR_FORM::TransferDataFromWindow()
{
    wxTextCtrl* vref_values[] = { m_vrefMinVal, m_vrefTypVal, m_vrefMaxVal };
    wxTextCtrl* iadj_values[] = { m_iadjTypVal, m_iadjMaxVal };

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    bool success = true;

    if( m_textCtrlName->GetValue().IsEmpty() )
        success = false;

    for( const auto& val : vref_values )
    {
        if( val->GetValue().IsEmpty() )
        {
            success = false;
        }
        else
        {
            double vref = DoubleFromString( val->GetValue() );

            if( fabs( vref ) < 0.1 )
                success = false;
        }
    }

    if( m_choiceRegType->GetSelection() == 1 )
    {
        for( const auto& val : iadj_values )
        {
            if( val->GetValue().IsEmpty() )
            {
                success = false;
            }
            else
            {
                int  iadj = 0;
                bool res = val->GetValue().ToInt( &iadj );

                if( res == false || fabs( iadj ) < 1 )
                    success = false;
            }
        }
    }

    return success;
}


void DIALOG_REGULATOR_FORM::UpdateDialog()
{
    bool enbl = m_choiceRegType->GetSelection() == 1;
    m_RegulIadjTitle->Show( enbl );
    m_iadjTypVal->Show( enbl );
    m_iadjMaxVal->Show( enbl );
    m_labelUnitIadj->Show( enbl );
}


void DIALOG_REGULATOR_FORM::CopyRegulatorDataToDialog( REGULATOR_DATA* aItem )
{
    m_textCtrlName->SetValue( aItem->m_Name );
    wxString value;
    value.Printf( wxT( "%.3g" ), aItem->m_VrefMin );
    m_vrefMinVal->SetValue( value );
    value.Printf( wxT( "%.3g" ), aItem->m_VrefTyp );
    m_vrefTypVal->SetValue( value );
    value.Printf( wxT( "%.3g" ), aItem->m_VrefMax );
    m_vrefMaxVal->SetValue( value );


    value.Printf( wxT( "%.3g" ), aItem->m_IadjTyp );
    m_iadjTypVal->SetValue( value );
    value.Printf( wxT( "%.3g" ), aItem->m_IadjMax );
    m_iadjMaxVal->SetValue( value );

    m_choiceRegType->SetSelection( aItem->m_Type );
    UpdateDialog();
}


REGULATOR_DATA* DIALOG_REGULATOR_FORM::BuildRegulatorFromData()
{
    double vrefmin = DoubleFromString( m_vrefMinVal->GetValue() );
    double vreftyp = DoubleFromString( m_vrefTypVal->GetValue() );
    double vrefmax = DoubleFromString( m_vrefMaxVal->GetValue() );

    double iadjtyp = DoubleFromString( m_iadjTypVal->GetValue() );
    double iadjmax = DoubleFromString( m_iadjMaxVal->GetValue() );

    int    type = m_choiceRegType->GetSelection();

    if( type != 1 )
    {
        iadjtyp = 0.0;
        iadjmax = 0.0;
    }

    REGULATOR_DATA* item = new REGULATOR_DATA( m_textCtrlName->GetValue(), vrefmin, vreftyp,
                                               vrefmax, type, iadjtyp, iadjmax );
    return item;
}
