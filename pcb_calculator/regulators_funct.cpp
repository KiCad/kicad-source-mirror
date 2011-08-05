/**
 * @file regulators_funct.cpp
 */
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre.charras
 * Copyright (C) 1992-2011 Kicad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#include "wx/wx.h"
#include "wx/config.h"

#include "pcb_calculator.h"

extern double ReturnDoubleFromString( const wxString& TextValue );

void PCB_CALCULATOR_FRAME::OnRegulatorCalcButtonClick( wxCommandEvent& event )
{
    RegulatorsSolve();
}

// Calculate a value from the 3 other values
// Vref is given by the regulator properties, so
// we can calculate only R1, R2 or Vout
void PCB_CALCULATOR_FRAME::RegulatorsSolve()
{
    int id;
    if( m_rbRegulR1->GetValue() )
        id = 0;     // for R1 calculation
    else if( m_rbRegulR2->GetValue() )
        id = 1;     // for R2 calculation
    else if( m_rbRegulVout->GetValue() )
        id = 2;     // for Vout calculation
    else
    {
        wxMessageBox( wxT("Selection error" ) );
        return;
    }

    double r1, r2, vref, vout;

    wxString txt;

    m_RegulMessage->SetLabel( wxEmptyString);

    // Read values from panel:
    txt = m_RegulR1Value->GetValue();
    r1 = ReturnDoubleFromString(txt);
    txt = m_RegulR2Value->GetValue();
    r2 = ReturnDoubleFromString(txt);
    txt = m_RegulVrefValue->GetValue();
    vref = ReturnDoubleFromString(txt);
    txt = m_RegulVoutValue->GetValue();
    vout = ReturnDoubleFromString(txt);

    // Some tests:
    if( vout < vref && id != 2)
    {
        m_RegulMessage->SetLabel( _(" Vout must be greater than vref" ) );
        return;
    }

    if( vref == 0.0 )
    {
        m_RegulMessage->SetLabel( _(" Vref set to 0 !" ) );
        return;
    }

    if( (r1 < 0 && id != 0 ) || (r2 <= 0 && id != 1) )
    {
        m_RegulMessage->SetLabel( _("Incorrect value for R1 R2" ) );
        return;
    }

    // Calculate
    switch( id )
    {
        case 0:
            r1 = ( vout / vref - 1 ) * r2;
            break;

        case 1:
            r2 = r1 / ( vout / vref - 1);
            break;

        case 2:
            vout = vref * (r1 + r2) / r2;
            break;
    }
    // write values to panel:
    txt.Printf(wxT("%f"), r1);
    m_RegulR1Value->SetValue(txt);
    txt.Printf(wxT("%f"), r2);
    m_RegulR2Value->SetValue(txt);
    txt.Printf(wxT("%f"), vref);
    m_RegulVrefValue->SetValue(txt);
    txt.Printf(wxT("%f"), vout);
    m_RegulVoutValue->SetValue(txt);

}

