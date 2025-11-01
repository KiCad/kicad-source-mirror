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

#include <wx/choicdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/clipbrd.h>

#include <bitmaps.h>
#include <calculator_panels/panel_regulator.h>
#include <class_regulator_data.h>
#include <dialogs/dialog_regulator_form.h>
#include <pcb_calculator_settings.h>

extern double DoubleFromString( const wxString& TextValue );

// extension of pcb_calculator data filename:
static const wxString DataFileNameExt( wxT( "pcbcalc" ) );


PANEL_REGULATOR::PANEL_REGULATOR( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                  const wxSize& size, long style, const wxString& name ) :
        PANEL_REGULATOR_BASE( parent, id, pos, size, style, name ),
        m_RegulatorListChanged( false )
{
    m_bitmapRegul3pins->SetBitmap( KiBitmapBundle( BITMAPS::regul_3pins ) );
    m_bitmapRegul4pins->SetBitmap( KiBitmapBundle( BITMAPS::regul ) );

    m_choiceRegulatorSelector->Append( m_RegulatorList.GetRegList() );
    SelectLastSelectedRegulator();

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}

PANEL_REGULATOR::~PANEL_REGULATOR()
{
}


void PANEL_REGULATOR::ThemeChanged()
{
    // Update the bitmaps
    m_bitmapRegul3pins->SetBitmap( KiBitmapBundle( BITMAPS::regul_3pins ) );
    m_bitmapRegul4pins->SetBitmap( KiBitmapBundle( BITMAPS::regul ) );
}


void PANEL_REGULATOR::OnRegulatorCalcButtonClick( wxCommandEvent& event )
{
    RegulatorsSolve();
}


void PANEL_REGULATOR::OnRegulatorResetButtonClick( wxCommandEvent& event )
{
    m_resTolVal->SetValue( wxT( DEFAULT_REGULATOR_RESTOL ) );

    m_r1MinVal->SetValue( wxT( "" ) );
    m_r1TypVal->SetValue( wxT( DEFAULT_REGULATOR_R1 ) );
    m_r1MaxVal->SetValue( wxT( "" ) );

    m_r2MinVal->SetValue( wxT( "" ) );
    m_r2TypVal->SetValue( wxT( DEFAULT_REGULATOR_R2 ) );
    m_r2MaxVal->SetValue( wxT( "" ) );

    m_vrefMinVal->SetValue( wxT( DEFAULT_REGULATOR_VREF_MIN ) );
    m_vrefTypVal->SetValue( wxT( DEFAULT_REGULATOR_VREF_TYP ) );
    m_vrefMaxVal->SetValue( wxT( DEFAULT_REGULATOR_VREF_MAX ) );

    m_voutMinVal->SetValue( wxT( "" ) );
    m_voutTypVal->SetValue( wxT( DEFAULT_REGULATOR_VOUT_TYP ) );
    m_voutMaxVal->SetValue( wxT( "" ) );

    m_iadjTypVal->SetValue( wxT( DEFAULT_REGULATOR_IADJ_TYP ) );
    m_iadjMaxVal->SetValue( wxT( DEFAULT_REGULATOR_IADJ_MAX ) );

    m_tolTotalMin->SetValue( wxT( "" ) );
    m_TolTotalMax->SetValue( wxT( "" ) );

    m_choiceRegType->SetSelection( 1 );
    m_rbRegulR1->SetValue( false );
    m_rbRegulR2->SetValue( false );
    m_rbRegulVout->SetValue( true );
    RegulatorPageUpdate();
}


void PANEL_REGULATOR::RegulatorPageUpdate()
{
    switch( m_choiceRegType->GetSelection() )
    {
    default:
    case 0:
        m_bitmapRegul4pins->Show( true );
        m_bitmapRegul3pins->Show( false );

        m_RegulIadjTitle->Show( false );
        m_iadjTypVal->Show( false );
        m_iadjMaxVal->Show( false );
        m_labelUnitsIadj->Show( false );

        m_RegulFormula->SetLabel( wxT( "Vout = Vref * (R1 + R2) / R2" ) );
        break;

    case 1:
        m_bitmapRegul4pins->Show( false );
        m_bitmapRegul3pins->Show( true );

        m_RegulIadjTitle->Show( true );
        m_iadjTypVal->Show( true );
        m_iadjMaxVal->Show( true );
        m_labelUnitsIadj->Show( true );

        m_RegulFormula->SetLabel( wxT( "Vout = Vref * (R1 + R2) / R1 + Iadj * R2" ) );
        break;
    }

    // The new icon size must be taken in account
    GetSizer()->Layout();

    // Enable/disable buttons:
    bool enbl = m_choiceRegulatorSelector->GetCount() > 0;
    m_buttonEditItem->Enable( enbl );
    m_buttonRemoveItem->Enable( enbl );

    Refresh();
}


void PANEL_REGULATOR::OnRegulTypeSelection( wxCommandEvent& event )
{
    RegulatorPageUpdate();
}


void PANEL_REGULATOR::OnRegulatorSelection( wxCommandEvent& event )
{
    wxString        name = m_choiceRegulatorSelector->GetStringSelection();
    REGULATOR_DATA* item = m_RegulatorList.GetReg( name );

    if( item )
    {
        m_lastSelectedRegulatorName = item->m_Name;
        m_choiceRegType->SetSelection( item->m_Type );
        wxString value;

        value.Printf( wxT( "%g" ), item->m_VrefMin );
        m_vrefMinVal->SetValue( value );
        value.Printf( wxT( "%g" ), item->m_VrefTyp );
        m_vrefTypVal->SetValue( value );
        value.Printf( wxT( "%g" ), item->m_VrefMax );
        m_vrefMaxVal->SetValue( value );

        value.Printf( wxT( "%g" ), item->m_IadjTyp );
        m_iadjTypVal->SetValue( value );

        value.Printf( wxT( "%g" ), item->m_IadjMax );
        m_iadjMaxVal->SetValue( value );
    }

    // Call RegulatorPageUpdate to enable/disable tools,
    // even if no item selected
    RegulatorPageUpdate();
}


void PANEL_REGULATOR::OnDataFileSelection( wxCommandEvent& event )
{
    wxString fullfilename = GetDataFilename();

    wxString wildcard;
    wildcard.Printf( _( "PCB Calculator data file" ) + wxT( " (*.%s)|*.%s" ),
                     DataFileNameExt, DataFileNameExt );

    wxWindow* topLevelParent = wxGetTopLevelParent( this );

    // Must be wxFD_SAVE, otherwise you cannot assign a file name
    wxFileDialog dlg( topLevelParent, _( "Select PCB Calculator Data File" ), wxEmptyString,
                      fullfilename, wildcard, wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fullfilename = dlg.GetPath();

    if( fullfilename == GetDataFilename() )
        return;

    SetDataFilename( fullfilename );

    if( wxFileExists( fullfilename ) && m_RegulatorList.GetCount() > 0 )  // Read file
    {
        if( wxMessageBox( _( "Do you want to load this file and replace current regulator list?" ),
                          wxASCII_STR( wxMessageBoxCaptionStr ), wxOK | wxCANCEL | wxCENTER, this )
            != wxOK )
        {
            return;
        }
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
        msg.Printf( _( "Unable to read data file '%s'." ), fullfilename );
        wxMessageBox( msg );
    }
}


void PANEL_REGULATOR::OnAddRegulator( wxCommandEvent& event )
{
    DIALOG_REGULATOR_FORM dlg( wxGetTopLevelParent( this ), wxEmptyString );

    if( dlg.ShowModal() != wxID_OK )
        return;

    REGULATOR_DATA* new_item = dlg.BuildRegulatorFromData();

    // Add new item, if not existing
    if( m_RegulatorList.GetReg( new_item->m_Name ) == nullptr )
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
        wxMessageBox( _( "This regulator is already in list. Aborted" ) );
        delete new_item;
    }
}


void PANEL_REGULATOR::OnEditRegulator( wxCommandEvent& event )
{
    wxString name = m_choiceRegulatorSelector->GetStringSelection();
    REGULATOR_DATA* item  = m_RegulatorList.GetReg( name );

    if( item == nullptr )
        return;

    DIALOG_REGULATOR_FORM dlg( wxGetTopLevelParent( this ), name );

    dlg.CopyRegulatorDataToDialog( item );

    if( dlg.ShowModal() != wxID_OK )
        return;

    REGULATOR_DATA* new_item = dlg.BuildRegulatorFromData();
    m_RegulatorList.Replace( new_item );

    m_RegulatorListChanged = true;

    SelectLastSelectedRegulator();
}


void PANEL_REGULATOR::OnRemoveRegulator( wxCommandEvent& event )
{
    wxString name = wxGetSingleChoice( _( "Remove Regulator" ), wxEmptyString,
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


void PANEL_REGULATOR::SelectLastSelectedRegulator()
{
    // Find last selected in regulator list:
    int idx = -1;

    if( !m_lastSelectedRegulatorName.IsEmpty() )
    {
        for( unsigned ii = 0; ii < m_RegulatorList.GetCount(); ii++ )
        {
            if( m_RegulatorList.m_List[ii]->m_Name == m_lastSelectedRegulatorName )
            {
                idx = ii;
                break;
            }
        }
    }

    m_choiceRegulatorSelector->SetSelection( idx );
    wxCommandEvent event;
    OnRegulatorSelection( event );
}


void PANEL_REGULATOR::OnCopyCB( wxCommandEvent& event )
{
    if( wxTheClipboard->Open() )
    {
        // This data objects are held by the clipboard,
        // so do not delete them in the app.
        wxTheClipboard->SetData( new wxTextDataObject( m_textPowerComment->GetValue() ) );
        wxTheClipboard->Close();
    }
}


void PANEL_REGULATOR::RegulatorsSolve()
{
    int id;

    if( m_rbRegulR1->GetValue() )
    {
        id = 0;     // for R1 calculation
    }
    else if( m_rbRegulR2->GetValue() )
    {
        id = 1;     // for R2 calculation
    }
    else if( m_rbRegulVout->GetValue() )
    {
        id = 2;     // for Vout calculation
    }
    else
    {
        wxMessageBox( wxT("Selection error" ) );
        return;
    }

    double restol;
    double r1min, r1typ, r1max;
    double r2min, r2typ, r2max;
    double vrefmin, vreftyp, vrefmax;
    double voutmin, vouttyp, voutmax;
    double toltotalmin, toltotalmax;

    wxString txt;

    m_RegulMessage->SetLabel( wxEmptyString);

    // Convert r1 and r2 in ohms
    int r1scale = 1000;
    int r2scale = 1000;

    // Read values from panel:
    txt = m_resTolVal->GetValue();
    restol = DoubleFromString( txt ) / 100;

    txt = m_r1TypVal->GetValue();
    r1typ = DoubleFromString( txt ) * r1scale;

    txt = m_r2TypVal->GetValue();
    r2typ = DoubleFromString( txt ) * r2scale;

    txt = m_vrefMinVal->GetValue();
    vrefmin = DoubleFromString( txt );
    txt = m_vrefTypVal->GetValue();
    vreftyp = DoubleFromString( txt );
    txt = m_vrefMaxVal->GetValue();
    vrefmax = DoubleFromString( txt );

    txt = m_voutTypVal->GetValue();
    vouttyp = DoubleFromString( txt );

    // Some tests:
    if( ( vouttyp < vrefmin || vouttyp < vreftyp || vouttyp < vrefmax ) && id != 2 )
    {
        m_RegulMessage->SetLabel( _( "Vout must be greater than Vref" ) );
        return;
    }

    if( vrefmin == 0.0 || vreftyp == 0.0 || vrefmax == 0.0 )
    {
        m_RegulMessage->SetLabel( _( "Vref set to 0 !" ) );
        return;
    }

    if( vrefmin > vreftyp || vreftyp > vrefmax )
    {
        m_RegulMessage->SetLabel( _( "Vref must VrefMin < VrefTyp < VrefMax" ) );
        return;
    }

    if( ( r1typ < 0 && id != 0 ) || ( r2typ <= 0 && id != 1 ) )
    {
        m_RegulMessage->SetLabel( _( "Incorrect value for R1 R2" ) );
        return;
    }

    // Calculate
    if( m_choiceRegType->GetSelection() == 1)
    {
        // 3 terminal regulator
        txt = m_iadjTypVal->GetValue();
        double iadjtyp = DoubleFromString( txt );
        txt = m_iadjMaxVal->GetValue();
        double iadjmax = DoubleFromString( txt );

        if( iadjtyp > iadjmax )
        {
            m_RegulMessage->SetLabel( _( "Iadj must IadjTyp < IadjMax" ) );
            return;
        }

        // iadj is given in micro amp, so convert it in amp.
        iadjtyp /= 1000000;
        iadjmax /= 1000000;

        switch( id )
        {
        case 0:
            // typical formula
            r1typ = vreftyp * r2typ / ( vouttyp - vreftyp - ( r2typ * iadjtyp ) );
            break;

        case 1:
            // typical formula
            r2typ = ( vouttyp - vreftyp ) / ( iadjtyp + ( vreftyp / r1typ ) );
            break;

        case 2:
            // typical formula
            vouttyp = vreftyp * ( r1typ + r2typ ) / r1typ;
            vouttyp += r2typ * iadjtyp;
            break;
        }

        r1min = r1typ - r1typ * restol;
        r1max = r1typ + r1typ * restol;

        r2min = r2typ - r2typ * restol;
        r2max = r2typ + r2typ * restol;

        voutmin = vrefmin * ( r1max + r2min ) / r1max;
        voutmin += r2min * iadjtyp;

        voutmax = vrefmax * ( r1min + r2max ) / r1min;
        voutmax += r2max * iadjmax;
    }
    else
    {   // Standard 4 terminal regulator
        switch( id )
        {
        case 0:
            // typical formula
            r1typ = ( vouttyp / vreftyp - 1 ) * r2typ;
            break;

        case 1:
            // typical formula
            r2typ = r1typ / ( vouttyp / vreftyp - 1 );
            break;

        case 2:
            // typical formula
            vouttyp = vreftyp * ( r1typ + r2typ ) / r2typ;
            break;
        }

        r1min = r1typ - r1typ * restol;
        r1max = r1typ + r1typ * restol;

        r2min = r2typ - r2typ * restol;
        r2max = r2typ + r2typ * restol;

        voutmin = vrefmin * ( r1min + r2max ) / r2max;
        voutmax = vrefmax * ( r1max + r2min ) / r2min;
    }

    toltotalmin = ( voutmin - vouttyp ) / vouttyp * 100.0;
    toltotalmax = ( voutmax - vouttyp ) / voutmax * 100.0;

    // write values to panel:
    txt.Printf( wxT( "%g" ), round_to( r1min / r1scale ) );
    m_r1MinVal->SetValue( txt );
    txt.Printf( wxT( "%g" ), round_to( r1typ / r1scale ) );
    m_r1TypVal->SetValue( txt );
    txt.Printf( wxT( "%g" ), round_to( r1max / r1scale ) );
    m_r1MaxVal->SetValue( txt );

    txt.Printf( wxT( "%g" ), round_to( r2min / r2scale ) );
    m_r2MinVal->SetValue( txt );
    txt.Printf( wxT( "%g" ), round_to( r2typ / r2scale ) );
    m_r2TypVal->SetValue( txt );
    txt.Printf( wxT( "%g" ), round_to( r2max / r2scale ) );
    m_r2MaxVal->SetValue( txt );

    txt.Printf( wxT( "%g" ), round_to( voutmin ) );
    m_voutMinVal->SetValue( txt );
    txt.Printf( wxT( "%g" ), round_to( vouttyp ) );
    m_voutTypVal->SetValue( txt );
    txt.Printf( wxT( "%g" ), round_to( voutmax ) );
    m_voutMaxVal->SetValue( txt );

    txt.Printf( wxT( "%g" ), round_to( toltotalmin, 0.01 ) );
    m_tolTotalMin->SetValue( txt );
    txt.Printf( wxT( "%g" ), round_to( toltotalmax, 0.01 ) );
    m_TolTotalMax->SetValue( txt );

    txt = wxString::Format( "%gV [%gV ... %gV]", round_to( vouttyp, 0.01 ),
                            round_to( voutmin, 0.01 ), round_to( voutmax, 0.01 ) );
    m_textPowerComment->SetValue( txt );
}


void PANEL_REGULATOR::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_resTolVal->SetValue( aCfg->m_Regulators.resTol );

    m_r1TypVal->SetValue( aCfg->m_Regulators.r1 );
    m_r2TypVal->SetValue( aCfg->m_Regulators.r2 );

    m_vrefMinVal->SetValue( aCfg->m_Regulators.vrefMin );
    m_vrefTypVal->SetValue( aCfg->m_Regulators.vrefTyp );
    m_vrefMaxVal->SetValue( aCfg->m_Regulators.vrefMax );

    m_voutTypVal->SetValue( aCfg->m_Regulators.voutTyp );

    m_iadjTypVal->SetValue( aCfg->m_Regulators.iadjTyp );
    m_iadjMaxVal->SetValue( aCfg->m_Regulators.iadjMax );

    SetDataFilename( aCfg->m_Regulators.data_file );
    m_lastSelectedRegulatorName = aCfg->m_Regulators.selected_regulator;
    m_choiceRegType->SetSelection( aCfg->m_Regulators.type );

    wxRadioButton* regprms[3] = { m_rbRegulR1, m_rbRegulR2, m_rbRegulVout };

    if( aCfg->m_Regulators.last_param >= 3 )
        aCfg->m_Regulators.last_param = 0;

    for( int ii = 0; ii < 3; ii++ )
        regprms[ii]->SetValue( aCfg->m_Regulators.last_param == ii );

    RegulatorPageUpdate();
}


void PANEL_REGULATOR::SaveSettings( PCB_CALCULATOR_SETTINGS *aCfg )
{
    aCfg->m_Regulators.resTol = m_resTolVal->GetValue();

    aCfg->m_Regulators.r1 = m_r1TypVal->GetValue();
    aCfg->m_Regulators.r2 = m_r2TypVal->GetValue();

    aCfg->m_Regulators.vrefMin = m_vrefMinVal->GetValue();
    aCfg->m_Regulators.vrefTyp = m_vrefTypVal->GetValue();
    aCfg->m_Regulators.vrefMax = m_vrefMaxVal->GetValue();

    m_voutTypVal->SetValue( aCfg->m_Regulators.voutTyp );

    aCfg->m_Regulators.iadjTyp = m_iadjTypVal->GetValue();
    aCfg->m_Regulators.iadjMax = m_iadjMaxVal->GetValue();

    aCfg->m_Regulators.data_file = GetDataFilename();
    aCfg->m_Regulators.selected_regulator = m_lastSelectedRegulatorName;
    aCfg->m_Regulators.type = m_choiceRegType->GetSelection();

    wxRadioButton* regprms[3] = { m_rbRegulR1, m_rbRegulR2, m_rbRegulVout };

    aCfg->m_Regulators.last_param = 0;

    for( int ii = 0; ii < 3; ii++ )
    {
        if( regprms[ii]->GetValue() )
        {
            aCfg->m_Regulators.last_param = ii;
            break;
        }
    }
}


const wxString PANEL_REGULATOR::GetDataFilename()
{
    if( m_regulators_fileNameCtrl->GetValue().IsEmpty() )
        return wxEmptyString;

    wxFileName fn( m_regulators_fileNameCtrl->GetValue() );
    fn.SetExt( DataFileNameExt );
    return fn.GetFullPath();
}


void PANEL_REGULATOR::SetDataFilename( const wxString& aFilename )
{
    if( aFilename.IsEmpty() )
    {
        m_regulators_fileNameCtrl->SetValue( wxEmptyString );
    }
    else
    {
        wxFileName fn( aFilename );
        fn.SetExt( DataFileNameExt );
        m_regulators_fileNameCtrl->SetValue( fn.GetFullPath() );
    }
}

double PANEL_REGULATOR::round_to( double value, double precision )
{
    return std::round( value / precision ) * precision;
}
