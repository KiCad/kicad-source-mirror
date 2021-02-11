/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 jean-pierre.charras
 * Copyright (C) 1992-2020 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <geometry/shape_poly_set.h>
#include <kiface_i.h>
#include "bitmaps/color_code_value_and_name.xpm"
#include "bitmaps/color_code_value.xpm"
#include "bitmaps/color_code_multiplier.xpm"
#include "bitmaps/color_code_tolerance.xpm"
#include "attenuators/attenuator_classes.h"
#include "class_regulator_data.h"
#include "pcb_calculator_frame.h"
#include "pcb_calculator_settings.h"


// extension of pcb_calculator data filename:
const wxString DataFileNameExt( wxT("pcbcalc") );

PCB_CALCULATOR_FRAME::PCB_CALCULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    PCB_CALCULATOR_FRAME_BASE( aParent ),
    m_lastNotebookPage( -1 )
{
    m_bpButtonCalcAtt->SetBitmap( KiBitmap( small_down_xpm ) );
    m_bpButtonAnalyze->SetBitmap( KiBitmap( small_down_xpm ) );
    m_bpButtonSynthetize->SetBitmap( KiBitmap( small_up_xpm ) );

    SetKiway( this, aKiway );
    m_currTransLine     = NULL;
    m_currTransLineType = DEFAULT_TYPE;
    m_currAttenuator    = NULL;
    m_RegulatorListChanged = false;
    m_TWMode = TW_MASTER_CURRENT;
    m_TWNested = false;

    // TODO: make regulator bitmaps transparent so we can remove this
    m_panelRegulatorBitmaps->SetBackgroundColour( *wxWHITE );

    SHAPE_POLY_SET dummy;   // A ugly trick to force the linker to include
                            // some methods in code and avoid link errors

    // Populate transline list ordered like in dialog menu list
    const static TRANSLINE_TYPE_ID tltype_list[8] =
            {
                MICROSTRIP_TYPE,
                CPW_TYPE,
                GROUNDED_CPW_TYPE,
                RECTWAVEGUIDE_TYPE,
                COAX_TYPE,
                C_MICROSTRIP_TYPE,
                STRIPLINE_TYPE,
                TWISTEDPAIR_TYPE
            };

    for( int ii = 0; ii < 8; ii++ )
        m_transline_list.push_back( new TRANSLINE_IDENT( tltype_list[ii] ) );

    // Populate attenuator list ordered like in dialog menu list
    m_attenuator_list.push_back( new ATTENUATOR_PI() );
    m_attenuator_list.push_back( new ATTENUATOR_TEE() );
    m_attenuator_list.push_back( new ATTENUATOR_BRIDGE() );
    m_attenuator_list.push_back( new ATTENUATOR_SPLITTER() );
    m_currAttenuator = m_attenuator_list[0];

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_staticTextAttMsg->SetFont( infoFont );

    LoadSettings( config() );

    ReadDataFile();

    TranslineTypeSelection( m_currTransLineType );
    m_TranslineSelection->SetSelection( m_currTransLineType );

    TW_Init();   // Track Width
    VS_Init();   // Via Size
    ES_Init();   // E-Series

    SetAttenuator( m_AttenuatorsSelection->GetSelection() );

    ToleranceSelection( m_rbToleranceSelection->GetSelection() );

    BoardClassesUpdateData( m_BoardClassesUnitsSelector->GetUnitScale() );

    ElectricalSpacingUpdateData( m_ElectricalSpacingUnitsSelector->GetUnitScale() );

    m_choiceRegulatorSelector->Append( m_RegulatorList.GetRegList() );
    SelectLastSelectedRegulator();

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( icon_pcbcalculator_xpm ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( icon_pcbcalculator_32_xpm ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( icon_pcbcalculator_16_xpm ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    GetSizer()->SetSizeHints( this );

    // Set previous size and position
    SetSize( m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    if( m_framePos == wxDefaultPosition )
        Centre();
}


PCB_CALCULATOR_FRAME::~PCB_CALCULATOR_FRAME()
{
    for( unsigned ii = 0; ii < m_transline_list.size(); ii++ )
        delete m_transline_list[ii];

    for( unsigned ii = 0; ii < m_attenuator_list.size(); ii++ )
        delete m_attenuator_list[ii];

    // This needed for OSX: avoids furter OnDraw processing after this destructor and before
    // the native window is destroyed
    this->Freeze();
}


void PCB_CALCULATOR_FRAME::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_Notebook->GetSelection() != m_lastNotebookPage )
    {
        // Kick all the things that wxWidgets can't seem to redraw on its own.

        wxCommandEvent event2( wxEVT_RADIOBUTTON );
        event2.SetEventObject( m_TranslineSelection );
        event2.SetInt( m_currTransLineType );
        m_TranslineSelection->ProcessCommand( event2 );

        for( int i = 0; i < m_attenuator_list.size(); ++i )
        {
            if( m_attenuator_list[i] == m_currAttenuator )
            {
                event2.SetEventObject( m_AttenuatorsSelection );
                event2.SetInt( i );
                m_AttenuatorsSelection->ProcessCommand( event2 );
                break;
            }
        }

        static wxBitmap* valueNameBitmap = new wxBitmap( color_code_value_and_name_xpm );
        m_Band1bitmap->SetBitmap( *valueNameBitmap );

        static wxBitmap* valueBitmap = new wxBitmap( color_code_value_xpm );
       	m_Band2bitmap->SetBitmap( *valueBitmap );
       	m_Band3bitmap->SetBitmap( *valueBitmap );
       	m_Band4bitmap->SetBitmap( *valueBitmap );

       	static wxBitmap* multiplierBitmap = new wxBitmap( color_code_multiplier_xpm );
       	m_Band_mult_bitmap->SetBitmap( *multiplierBitmap );

       	static wxBitmap* toleranceBitmap = new wxBitmap( color_code_tolerance_xpm );
       	m_Band_tol_bitmap->SetBitmap( *toleranceBitmap );

        // Until it's shown on screen the above won't work; but doing it anyway at least keeps
        // putting new OnUpdateUI events into the queue until it *is* shown on screen.
        if( m_Notebook->IsShownOnScreen() )
            m_lastNotebookPage = m_Notebook->GetSelection();
    }
}


void PCB_CALCULATOR_FRAME::OnClosePcbCalc( wxCloseEvent& event )
{
    if( m_RegulatorListChanged )
    {
        wxString msg;
        wxString title = _( "Write Data Failed" );

        if( GetDataFilename().IsEmpty() )
        {
            msg = _( "No data filename to save modifications.\n"
                     "Do you want to exit and abandon your changes?" );

            if( wxMessageBox( msg, title, wxYES_NO | wxICON_QUESTION ) == wxNO )
                return;
        }
        else
        {
            if( !WriteDataFile() )
            {
                msg.Printf( _( "Unable to write file '%s'\n"
                               "Do you want to exit and abandon your changes?"),
                            GetDataFilename() );

                if( wxMessageBox( msg, title, wxYES_NO | wxICON_ERROR ) == wxNO )
                    return;
            }
        }
    }

    event.Skip();
}


void PCB_CALCULATOR_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    if( aCfg == NULL )
        return;

    EDA_BASE_FRAME::LoadSettings( aCfg );

    PCB_CALCULATOR_SETTINGS* cfg = static_cast<PCB_CALCULATOR_SETTINGS*>( aCfg );

    m_currTransLineType = static_cast<TRANSLINE_TYPE_ID>( cfg->m_TransLine.type );
    m_Notebook->ChangeSelection( cfg->m_LastPage );
    m_rbToleranceSelection->SetSelection( cfg->m_ColorCodeTolerance );
    m_AttenuatorsSelection->SetSelection( cfg->m_Attenuators.type );
    m_BoardClassesUnitsSelector->SetSelection( cfg->m_BoardClassUnits );

    // Regul panel config:
    m_RegulR1Value->SetValue( cfg->m_Regulators.r1 );
    m_RegulR2Value->SetValue( cfg->m_Regulators.r2 );
    m_RegulVrefValue->SetValue( cfg->m_Regulators.vref );
    m_RegulVoutValue->SetValue( cfg->m_Regulators.vout );
    SetDataFilename( cfg->m_Regulators.data_file );
    m_lastSelectedRegulatorName = cfg->m_Regulators.selected_regulator;
    m_choiceRegType->SetSelection( cfg->m_Regulators.type );

    wxRadioButton* regprms[3] = { m_rbRegulR1, m_rbRegulR2, m_rbRegulVout };

    if( cfg->m_Regulators.last_param >= 3 )
        cfg->m_Regulators.last_param = 0;

    for( int ii = 0; ii < 3; ii++ )
        regprms[ii]->SetValue( cfg->m_Regulators.last_param == ii );

    // Electrical panel config
    m_ElectricalSpacingUnitsSelector->SetSelection( cfg->m_Electrical.spacing_units );
    m_ElectricalSpacingVoltage->SetValue( cfg->m_Electrical.spacing_voltage );

    for( TRANSLINE_IDENT* transline : m_transline_list )
        transline->ReadConfig();

    for( ATTENUATOR* attenuator : m_attenuator_list )
        attenuator->ReadConfig();
}


void PCB_CALCULATOR_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    if( aCfg == NULL )
        return;

    EDA_BASE_FRAME::SaveSettings( aCfg );

    // Save current parameters values in config.
    auto cfg = dynamic_cast<PCB_CALCULATOR_SETTINGS*>( Kiface().KifaceSettings() );

    if( cfg )
    {
        cfg->m_LastPage = m_Notebook->GetSelection();
        cfg->m_TransLine.type = m_currTransLineType;
        cfg->m_Attenuators.type = m_AttenuatorsSelection->GetSelection();
        cfg->m_ColorCodeTolerance = m_rbToleranceSelection->GetSelection();
        cfg->m_BoardClassUnits = m_BoardClassesUnitsSelector->GetSelection();

        cfg->m_Electrical.spacing_units = m_ElectricalSpacingUnitsSelector->GetSelection();
        cfg->m_Electrical.spacing_voltage = m_ElectricalSpacingVoltage->GetValue();

        Regulators_WriteConfig( cfg );
    }

    TW_WriteConfig();

    VS_WriteConfig();

    for( unsigned ii = 0; ii < m_transline_list.size(); ii++ )
        m_transline_list[ii]->WriteConfig();

    for( unsigned ii = 0; ii < m_attenuator_list.size(); ii++ )
        m_attenuator_list[ii]->WriteConfig();
}


/**
 * Function OnTranslineAnalyse
 * Run a new analyse for the current transline with current parameters
 * and displays the electrical parameters
 */
void PCB_CALCULATOR_FRAME::OnTranslineAnalyse( wxCommandEvent& event )
{
    if( m_currTransLine )
    {
        TransfDlgDataToTranslineParams();
        m_currTransLine->analyze();
    }
}


/**
 * Function OnTranslineSynthetize
 * Run a new synthezis for the current transline with current parameters
 * and displays the geometrical parameters
 */
void PCB_CALCULATOR_FRAME::OnTranslineSynthetize( wxCommandEvent& event )
{
    if( m_currTransLine )
    {
        TransfDlgDataToTranslineParams();
        m_currTransLine->synthesize();
    }
}


/* returns the full filename of the selected pcb_calculator data file
 * the extension file is forced
 */
const wxString PCB_CALCULATOR_FRAME::GetDataFilename()
{
    if( m_regulators_fileNameCtrl->GetValue().IsEmpty() )
        return wxEmptyString;

    wxFileName fn( m_regulators_fileNameCtrl->GetValue() );
    fn.SetExt( DataFileNameExt );
    return fn.GetFullPath();
}


/* Initialize the full filename of the selected pcb_calculator data file
 * force the standard extension of the file (.pcbcalc)
 * aFilename = the full filename, with or without extension
 */
void PCB_CALCULATOR_FRAME::SetDataFilename( const wxString & aFilename)
{
    if( aFilename.IsEmpty() )
        m_regulators_fileNameCtrl->SetValue( wxEmptyString );

    else
    {
        wxFileName fn( aFilename );
        fn.SetExt( DataFileNameExt );
        m_regulators_fileNameCtrl->SetValue( fn.GetFullPath() );
    }
}
