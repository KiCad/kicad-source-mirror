/**
 * @file regulators_funct.cpp
 */
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre.charras
 * Copyright (C) 1992-2011 Kicad Developers, see AUTHORS.txt for contributors.
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
#include <wx/wx.h>
#include <wx/config.h>

#include <macros.h>
#include <pcb_calculator.h>
#include <class_regulator_data.h>
#include <dialog_regulator_data_base.h>


extern double DoubleFromString( const wxString& TextValue );

class DIALOG_EDITOR_DATA: public DIALOG_EDITOR_DATA_BASE
{
public:
    DIALOG_EDITOR_DATA( PCB_CALCULATOR_FRAME * parent, const wxString & aRegName )
        : DIALOG_EDITOR_DATA_BASE( parent )
    {
        m_textCtrlName->SetValue( aRegName );
        m_textCtrlName->Enable( aRegName.IsEmpty() );
        UpdateDialog();

        m_sdbSizerOK->SetDefault();

        // Now all widgets have the size fixed, call FinishDialogSettings
        FinishDialogSettings();
    }

    ~DIALOG_EDITOR_DATA() {};

    // Event called functions:
    void OnOKClick( wxCommandEvent& event ) override;

    /**
     * Function IsOK()
     * @return true if regulator parameters are acceptable
     */
    bool IsOK();

    /**
     * Function CopyRegulatorDataToDialog
     * Transfert data from dialog to aItem
     * @param aItem = a pointer to the REGULATOR_DATA
     */
    void CopyRegulatorDataToDialog( REGULATOR_DATA * aItem );

    /**
     * Function BuildRegulatorFromData
     * Creates a new REGULATOR_DATA from dialog
     * @return a pointer to the new REGULATOR_DATA
     */
    REGULATOR_DATA * BuildRegulatorFromData();

    /**
     * Enable/disable Iadj realted widgets, according to
     * the regulator type
     */
    void UpdateDialog()
    {
        bool enbl = m_choiceRegType->GetSelection() == 1;
        m_RegulIadjValue->Enable( enbl );
        m_RegulIadjTitle->Enable( enbl );
        m_IadjUnitLabel->Enable( enbl );
    }

    /**
     * called when the current regulator type is changed
     */
    void OnRegTypeSelection( wxCommandEvent& event ) override
    {
        UpdateDialog();
    }
};


void DIALOG_EDITOR_DATA::OnOKClick( wxCommandEvent& event )
{
    if( !IsOK() )
    {
        wxMessageBox( _("Bad or missing parameters!") );
        return;
    }

    EndModal( wxID_OK );
}

bool DIALOG_EDITOR_DATA::IsOK()
{
    bool success = true;

    if( m_textCtrlName->GetValue().IsEmpty() )
        success = false;
    if( m_textCtrlVref->GetValue().IsEmpty() )
        success = false;
    else
    {
        double vref = DoubleFromString( m_textCtrlVref->GetValue() );
        if( fabs(vref) < 0.01 )
            success = false;
    }
    if( m_choiceRegType->GetSelection() == 1 )
    {
        if( m_RegulIadjValue->GetValue().IsEmpty() )
        success = false;
    }

    return success;
}

void DIALOG_EDITOR_DATA::CopyRegulatorDataToDialog( REGULATOR_DATA * aItem )
{
    m_textCtrlName->SetValue( aItem->m_Name );
    wxString value;
    value.Printf( wxT("%g"), aItem->m_Vref );
    m_textCtrlVref->SetValue( value );
    value.Printf( wxT("%g"), aItem->m_Iadj );
    m_RegulIadjValue->SetValue( value );
    m_choiceRegType->SetSelection( aItem->m_Type );
    UpdateDialog();
}

REGULATOR_DATA * DIALOG_EDITOR_DATA::BuildRegulatorFromData()
{
    double vref = DoubleFromString( m_textCtrlVref->GetValue() );
    double iadj = DoubleFromString( m_RegulIadjValue->GetValue() );
    int type = m_choiceRegType->GetSelection();
    if( type != 1 )
        iadj = 0.0;
    REGULATOR_DATA * item = new REGULATOR_DATA( m_textCtrlName->GetValue(),
                                                vref, type, iadj );
    return item;
}

void PCB_CALCULATOR_FRAME::OnRegulatorCalcButtonClick( wxCommandEvent& event )
{
    RegulatorsSolve();
}

void PCB_CALCULATOR_FRAME::RegulatorPageUpdate()
{
    switch( m_choiceRegType->GetSelection() )
    {
        default:
        case 0:
            m_bitmapRegul4pins->Show( true );
            m_bitmapRegul3pins->Show( false );
            m_RegulIadjValue->Enable( false );
            m_RegulIadjTitle->Enable( false );
            m_IadjUnitLabel->Enable( false );
            m_RegulFormula->SetLabel( wxT("Vout = Vref * (R1 + R2) / R2") );
            break;

        case 1:
            m_bitmapRegul4pins->Show( false );
            m_bitmapRegul3pins->Show( true );
            m_RegulIadjValue->Enable( true );
            m_RegulIadjTitle->Enable( true );
            m_IadjUnitLabel->Enable( true );
            m_RegulFormula->SetLabel( wxT("Vout = Vref * (R1 + R2) / R1 + Iadj * R2") );
            break;
    }
    // The new icon size must be taken in account
    m_panelRegulators->GetSizer()->Layout();

    // Enable/disable buttons:
    bool enbl = m_choiceRegulatorSelector->GetCount() > 0;
    m_buttonEditItem->Enable( enbl );
    m_buttonRemoveItem->Enable( enbl );

    m_panelRegulators->Refresh();
}

void PCB_CALCULATOR_FRAME::OnRegulTypeSelection( wxCommandEvent& event )
{
    RegulatorPageUpdate();
}

void PCB_CALCULATOR_FRAME::OnRegulatorSelection( wxCommandEvent& event )
{
    wxString name = m_choiceRegulatorSelector->GetStringSelection();
    REGULATOR_DATA * item = m_RegulatorList.GetReg( name );
    if( item )
    {
        m_lastSelectedRegulatorName = item->m_Name;
        m_choiceRegType->SetSelection( item->m_Type );
        wxString value;
        value.Printf( wxT("%g"), item->m_Vref );
        m_RegulVrefValue->SetValue( value );
        value.Printf( wxT("%g"), item->m_Iadj );
        m_RegulIadjValue->SetValue( value );
    }

    // Call RegulatorPageUpdate to enable/sisable tools,
    // even if no item selected
    RegulatorPageUpdate();
}

/*
 * Called when ckicking on button Browse:
 * Select a new data file, and load it on request
 */
void PCB_CALCULATOR_FRAME::OnDataFileSelection( wxCommandEvent& event )
{
    wxString fullfilename = GetDataFilename();

    wxString wildcard;
    wildcard.Printf( _("PCB Calculator data file (*.%s)|*.%s"),
                     DataFileNameExt, DataFileNameExt );

    wxFileDialog dlg( m_panelRegulators,
                      _("Select PCB Calculator Data File"),
                      wxEmptyString, fullfilename,
                      wildcard, wxFD_OPEN );

    if (dlg.ShowModal() == wxID_CANCEL)
        return;

    fullfilename = dlg.GetPath();

    if( fullfilename == GetDataFilename() )
        return;

    SetDataFilename( fullfilename );
    if( wxFileExists( fullfilename ) && m_RegulatorList.GetCount() > 0 )  // Read file
    {
        if( wxMessageBox( _("Do you want to load this file and replace current regulator list?" ) )
                         != wxID_OK )
            return;
    }

    if( ReadDataFile() )
    {
        m_RegulatorListChanged = false;
        m_choiceRegulatorSelector->Clear();
        m_choiceRegulatorSelector->Append( m_RegulatorList.GetRegList() );
        SelectLastSelectedRegulator();
    }

    else
    {
        wxString msg;
        msg.Printf( _("Unable to read data file \"%s\""), fullfilename );
        wxMessageBox( msg );
    }
}

void PCB_CALCULATOR_FRAME::OnAddRegulator( wxCommandEvent& event )
{
    DIALOG_EDITOR_DATA dlg( this, wxEmptyString );
    if( dlg.ShowModal() != wxID_OK )
        return;
    if( !dlg.IsOK() )
    {
        wxMessageBox( _("Bad or missing parameters!") );
        return;
    }

    REGULATOR_DATA * new_item = dlg.BuildRegulatorFromData();

    // Add new item, if not existing
    if( m_RegulatorList.GetReg( new_item->m_Name ) == NULL )
    {
        // Add item in list
        m_RegulatorList.Add( new_item );
        m_RegulatorListChanged = true;
        m_choiceRegulatorSelector->Clear();
        m_choiceRegulatorSelector->Append( m_RegulatorList.GetRegList() );
        m_lastSelectedRegulatorName = new_item->m_Name;
        SelectLastSelectedRegulator();
    }
    else
    {
        wxMessageBox( _("This regulator is already in list. Aborted") );
        delete new_item;
    }
}

void PCB_CALCULATOR_FRAME::OnEditRegulator( wxCommandEvent& event )
{
    wxString name = m_choiceRegulatorSelector->GetStringSelection();
    REGULATOR_DATA * item  = m_RegulatorList.GetReg( name );
    if( item == NULL )
        return;

    DIALOG_EDITOR_DATA dlg( this, name );

    dlg.CopyRegulatorDataToDialog( item );
    if( dlg.ShowModal() != wxID_OK )
        return;

    REGULATOR_DATA * new_item = dlg.BuildRegulatorFromData();
    m_RegulatorList.Replace( new_item );

    m_RegulatorListChanged = true;

    SelectLastSelectedRegulator();
}

void PCB_CALCULATOR_FRAME::OnRemoveRegulator( wxCommandEvent& event )
{
    wxString name = wxGetSingleChoice( _("Remove Regulator"), wxEmptyString,
                                       m_RegulatorList.GetRegList() );
    if( name.IsEmpty() )
        return;

    m_RegulatorList.Remove( name );
    m_RegulatorListChanged = true;
    m_choiceRegulatorSelector->Clear();
    m_choiceRegulatorSelector->Append( m_RegulatorList.GetRegList() );
    if( m_lastSelectedRegulatorName == name )
        m_lastSelectedRegulatorName.Empty();

    SelectLastSelectedRegulator();
}

void PCB_CALCULATOR_FRAME::SelectLastSelectedRegulator()
{
    // Find last selected in regulator list:
    int idx = -1;
    if( ! m_lastSelectedRegulatorName.IsEmpty() )
    {
        for( unsigned ii = 0; ii < m_RegulatorList.GetCount(); ii++ )
            if( m_RegulatorList.m_List[ii]->m_Name == m_lastSelectedRegulatorName )
            {
                idx = ii;
                break;
            }
    }

    m_choiceRegulatorSelector->SetSelection( idx );
    wxCommandEvent event;
    OnRegulatorSelection( event );
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

    // Convert r1 and r2 in ohms
    int r1scale = 1000;
    int r2scale = 1000;

    // Read values from panel:
    txt = m_RegulR1Value->GetValue();
    r1 = DoubleFromString(txt) * r1scale;
    txt = m_RegulR2Value->GetValue();
    r2 = DoubleFromString(txt) * r2scale;
    txt = m_RegulVrefValue->GetValue();
    vref = DoubleFromString(txt);
    txt = m_RegulVoutValue->GetValue();
    vout = DoubleFromString(txt);


    // Some tests:
    if( vout < vref && id != 2)
    {
        m_RegulMessage->SetLabel( _("Vout must be greater than vref" ) );
        return;
    }

    if( vref == 0.0 )
    {
        m_RegulMessage->SetLabel( _("Vref set to 0 !" ) );
        return;
    }

    if( (r1 < 0 && id != 0 ) || (r2 <= 0 && id != 1) )
    {
        m_RegulMessage->SetLabel( _("Incorrect value for R1 R2" ) );
        return;
    }

    // Calculate
    if( m_choiceRegType->GetSelection() == 1)
    {
        // 3 terminal regulator
        txt = m_RegulIadjValue->GetValue();
        double iadj = DoubleFromString(txt);
        // iadj is given in micro amp, so convert it in amp.
        iadj /= 1000000;

        switch( id )
        {
            case 0:
                r1 = vref * r2 / ( vout - vref - (r2 * iadj) );
                break;

            case 1:
                r2 = ( vout - vref ) / ( iadj + (vref/r1) );
                break;

            case 2:
                vout = vref * (r1 + r2) / r1;
                vout += r2 * iadj;
                break;
        }
    }
    else
    {   // Standard 4 terminal regulator
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
    }
    // write values to panel:
    txt.Printf(wxT("%g"), r1 / r1scale );
    m_RegulR1Value->SetValue(txt);
    txt.Printf(wxT("%g"), r2 / r2scale);
    m_RegulR2Value->SetValue(txt);
    txt.Printf(wxT("%g"), vref);
    m_RegulVrefValue->SetValue(txt);
    txt.Printf(wxT("%g"), vout);
    m_RegulVoutValue->SetValue(txt);

}

