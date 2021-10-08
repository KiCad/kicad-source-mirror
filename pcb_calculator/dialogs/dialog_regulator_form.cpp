/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre.charras
 * Copyright (C) 1992-2021 Kicad Developers, see AUTHORS.txt for contributors.
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

DIALOG_REGULATOR_FORM::DIALOG_REGULATOR_FORM( PANEL_REGULATOR* parent, const wxString& aRegName ) :
        DIALOG_REGULATOR_FORM_BASE( parent )
{
    m_textCtrlName->SetValue( aRegName );
    m_textCtrlName->Enable( aRegName.IsEmpty() );
    UpdateDialog();

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}

DIALOG_REGULATOR_FORM::~DIALOG_REGULATOR_FORM()
{
}


bool DIALOG_REGULATOR_FORM::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    bool success = true;

    if( m_textCtrlName->GetValue().IsEmpty() )
        success = false;

    if( m_textCtrlVref->GetValue().IsEmpty() )
    {
        success = false;
    }
    else
    {
        double vref = DoubleFromString( m_textCtrlVref->GetValue() );

        if( fabs( vref ) < 0.01 )
            success = false;
    }

    if( m_choiceRegType->GetSelection() == 1 )
    {
        if( m_RegulIadjValue->GetValue().IsEmpty() )
            success = false;
    }

    return success;
}


void DIALOG_REGULATOR_FORM::UpdateDialog()
{
    bool enbl = m_choiceRegType->GetSelection() == 1;
    m_RegulIadjValue->Enable( enbl );
}


void DIALOG_REGULATOR_FORM::CopyRegulatorDataToDialog( REGULATOR_DATA* aItem )
{
    m_textCtrlName->SetValue( aItem->m_Name );
    wxString value;
    value.Printf( wxT( "%g" ), aItem->m_Vref );
    m_textCtrlVref->SetValue( value );
    value.Printf( wxT( "%g" ), aItem->m_Iadj );
    m_RegulIadjValue->SetValue( value );
    m_choiceRegType->SetSelection( aItem->m_Type );
    UpdateDialog();
}


REGULATOR_DATA* DIALOG_REGULATOR_FORM::BuildRegulatorFromData()
{
    double vref = DoubleFromString( m_textCtrlVref->GetValue() );
    double iadj = DoubleFromString( m_RegulIadjValue->GetValue() );
    int    type = m_choiceRegType->GetSelection();

    if( type != 1 )
        iadj = 0.0;

    REGULATOR_DATA* item = new REGULATOR_DATA( m_textCtrlName->GetValue(), vref, type, iadj );
    return item;
}
