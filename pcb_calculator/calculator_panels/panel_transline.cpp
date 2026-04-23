/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
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


#include "transline_ident.h"
#include <cmath>
#include <bitmaps.h>
#include <calculator_panels/panel_transline.h>
#include <pcb_calculator_settings.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/unit_selector.h>
#include <properties/property_mgr.h>
#include <properties/property.h>
#include <transline_calculations/transline_calculation_base.h>


extern double DoubleFromString( const wxString& TextValue );


PANEL_TRANSLINE::PANEL_TRANSLINE( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                  const wxSize& size, long style, const wxString& name ) :
        PANEL_TRANSLINE_BASE( parent, id, pos, size, style, name ),
        m_currTransLine( nullptr ),
        m_currTransLineType( DEFAULT_TYPE )
{
    m_AnalyseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_SynthetizeButton->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );


    // Populate transline list ordered like in dialog menu list
    const static TRANSLINE_TYPE_ID tltype_list[9] = { MICROSTRIP_TYPE,    C_MICROSTRIP_TYPE, STRIPLINE_TYPE,
                                                      C_STRIPLINE_TYPE,   CPW_TYPE,          GROUNDED_CPW_TYPE,
                                                      RECTWAVEGUIDE_TYPE, COAX_TYPE,         TWISTEDPAIR_TYPE };

    for( int ii = 0; ii < 9; ii++ )
        m_transline_list.push_back( new TRANSLINE_IDENT( tltype_list[ii] ) );

    m_EpsilonR_label->SetLabel( wxT( "εr" ) );
    m_substrate_prm3_labelUnit->SetLabel( wxT( "Ω ∙ m" ) );

    m_dielectricModelChoice->SetToolTip(
            _( "'Constant': εr and tan δ applied at all frequencies.\n"
               "'Djordjevic-Sarkar': causal wideband Debye anchored at the spec frequency." ) );

    UpdateSpecFrequencyEnable();

    m_soldermaskPresentCheck->SetToolTip(
            _( "Enable solder resist / LPI overlay correction.  Affects εeff, Z0, and "
               "dielectric loss for microstrip, coupled microstrip, CPW, and CBCPW." ) );

    m_soldermaskThicknessLabel->SetToolTip(
            _( "Cured mask thickness.  Typical LPI is 15-30 um; set to 0 or uncheck to "
               "disable." ) );
    m_soldermaskThicknessValue->SetToolTip( m_soldermaskThicknessLabel->GetToolTipText() );

    m_soldermaskEpsilonRLabel->SetToolTip(
            _( "Mask relative permittivity.  Default 3.5 for standard green LPI.  Range "
               "3.3-3.8 for typical resins." ) );
    m_soldermaskEpsilonRValue->SetToolTip( m_soldermaskEpsilonRLabel->GetToolTipText() );

    m_soldermaskTanDLabel->SetToolTip(
            _( "Mask loss tangent.  Default 0.025 for LPI." ) );
    m_soldermaskTanDValue->SetToolTip( m_soldermaskTanDLabel->GetToolTipText() );

    m_soldermaskFillsGapsCheck->SetToolTip(
            _( "Enable when the mask fills the CPW slots (standard LPI process).\n"
               "Disable for selective mask that covers only the traces." ) );

    // Seed the displayed defaults that match TRANSLINE::Init().  LoadSettings will overwrite
    // these from persisted values when the calculator is re-opened.
    m_soldermaskThicknessValue->SetValue( wxT( "20" ) );
    m_soldermaskEpsilonRValue->SetValue( wxT( "3.5" ) );
    m_soldermaskTanDValue->SetValue( wxT( "0.025" ) );
    m_soldermaskFillsGapsCheck->SetValue( true );

    updateSoldermaskEnables();
}


void PANEL_TRANSLINE::updateSoldermaskEnables()
{
    const bool groupVisible = m_soldermaskPresentCheck->IsShown();
    const bool present = groupVisible && m_soldermaskPresentCheck->GetValue();

    m_soldermaskThicknessLabel->Enable( present );
    m_soldermaskThicknessValue->Enable( present );
    m_soldermaskThicknessUnit->Enable( present );
    m_soldermaskEpsilonRLabel->Enable( present );
    m_soldermaskEpsilonRValue->Enable( present );
    m_soldermaskTanDLabel->Enable( present );
    m_soldermaskTanDValue->Enable( present );

    // Fills-gaps only meaningful on CPW / CBCPW; caller gates visibility per calculator type.
    m_soldermaskFillsGapsCheck->Enable( present && m_soldermaskFillsGapsCheck->IsShown() );
}


PANEL_TRANSLINE::~PANEL_TRANSLINE()
{
    for( TRANSLINE_IDENT* transline : m_transline_list )
        delete transline;
}


void PANEL_TRANSLINE::ThemeChanged()
{
    // Update the bitmaps
    m_AnalyseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_SynthetizeButton->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_translineBitmap->SetBitmap( KiBitmapBundle( m_transline_list[m_currTransLineType]->m_BitmapName ) );
}


void PANEL_TRANSLINE::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    // Ensure parameters from current selection are up to date before saving
    if( m_currTransLine )
        TransfDlgDataToTranslineParams();

    aCfg->m_TransLine.type = m_currTransLineType;
    aCfg->m_TransLine.dielectric_model = m_dielectricModelChoice->GetSelection();
    aCfg->m_TransLine.spec_frequency = m_Value_SpecFrequency_Ctrl->GetValue();
    aCfg->m_TransLine.spec_frequency_unit = m_choiceUnit_SpecFrequency->GetSelection();

    // Persist soldermask panel state.  The thickness field is entered in micrometres to match
    // the displayed unit; convert back to metres for the stored value.
    aCfg->m_TransLine.soldermask_present = m_soldermaskPresentCheck->GetValue() ? 1 : 0;

    // Use DoubleFromString so unit-suffixed and locale-formatted entries persist correctly.
    // On parse failure DoubleFromString returns NaN; std::isfinite then guards the cfg
    // write so a transient bad value does not overwrite the last good persisted setting.
    const double thicknessUm = DoubleFromString( m_soldermaskThicknessValue->GetValue() );

    if( std::isfinite( thicknessUm ) )
        aCfg->m_TransLine.soldermask_thickness = thicknessUm * 1.0e-6;

    const double epsR = DoubleFromString( m_soldermaskEpsilonRValue->GetValue() );

    if( std::isfinite( epsR ) )
        aCfg->m_TransLine.soldermask_epsilonr = epsR;

    const double tand = DoubleFromString( m_soldermaskTanDValue->GetValue() );

    if( std::isfinite( tand ) )
        aCfg->m_TransLine.soldermask_tand = tand;

    aCfg->m_TransLine.soldermask_fills_gaps = m_soldermaskFillsGapsCheck->GetValue() ? 1 : 0;

    for( TRANSLINE_IDENT* transline : m_transline_list )
        transline->WriteConfig();
}


void PANEL_TRANSLINE::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_currTransLineType = static_cast<TRANSLINE_TYPE_ID>( aCfg->m_TransLine.type );

    int modelSel = aCfg->m_TransLine.dielectric_model;

    if( modelSel < 0 || modelSel > 1 )
        modelSel = 0;

    m_dielectricModelChoice->SetSelection( modelSel );

    if( !aCfg->m_TransLine.spec_frequency.IsEmpty() )
        m_Value_SpecFrequency_Ctrl->SetValue( aCfg->m_TransLine.spec_frequency );

    m_choiceUnit_SpecFrequency->SetSelection( aCfg->m_TransLine.spec_frequency_unit );

    UpdateSpecFrequencyEnable();

    // Soldermask panel state.  Thickness is stored in metres but displayed in micrometres.
    m_soldermaskPresentCheck->SetValue( aCfg->m_TransLine.soldermask_present != 0 );
    m_soldermaskThicknessValue->SetValue(
            wxString::Format( wxT( "%g" ), aCfg->m_TransLine.soldermask_thickness * 1.0e6 ) );
    m_soldermaskEpsilonRValue->SetValue(
            wxString::Format( wxT( "%g" ), aCfg->m_TransLine.soldermask_epsilonr ) );
    m_soldermaskTanDValue->SetValue(
            wxString::Format( wxT( "%g" ), aCfg->m_TransLine.soldermask_tand ) );
    m_soldermaskFillsGapsCheck->SetValue( aCfg->m_TransLine.soldermask_fills_gaps != 0 );

    for( TRANSLINE_IDENT* transline : m_transline_list )
        transline->ReadConfig();

    TranslineTypeSelection( m_currTransLineType );
    m_TranslineSelection->SetSelection( m_currTransLineType );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    // It also remove a minor cosmetic issue on wxWidgets 3.5 on MSW
    // Called here after the current selected transline bitmaps are enabled/disabled
    GetSizer()->SetSizeHints( this );
}


void PANEL_TRANSLINE::OnDielectricModelChanged( wxCommandEvent& event )
{
    UpdateSpecFrequencyEnable();
}


void PANEL_TRANSLINE::UpdateSpecFrequencyEnable()
{
    const bool dsActive =
            m_dielectricModelChoice->GetSelection() == static_cast<int>( DIELECTRIC_MODEL::DJORDJEVIC_SARKAR );

    m_Value_SpecFrequency_Ctrl->Enable( dsActive );
    m_choiceUnit_SpecFrequency->Enable( dsActive );
}


void PANEL_TRANSLINE::OnSoldermaskChanged( wxCommandEvent& event )
{
    updateSoldermaskEnables();
}



void PANEL_TRANSLINE::OnTranslineAnalyse( wxCommandEvent& event )
{
    if( m_currTransLine )
    {
        TransfDlgDataToTranslineParams();
        m_currTransLine->analyze();
    }
}


void PANEL_TRANSLINE::OnTranslineSynthetize( wxCommandEvent& event )
{
    if( m_currTransLine )
    {
        TransfDlgDataToTranslineParams();
        m_currTransLine->synthesize();
    }
}
