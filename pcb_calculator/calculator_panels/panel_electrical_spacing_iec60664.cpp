/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
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

/* see
 * http://www.desmith.net/NMdS/Electronics/TraceWidth.html
 * http://www.ultracad.com/articles/pcbtemp.pdf
 * for more info
 */
#include <calculator_panels/panel_electrical_spacing.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>
#include "iec60664.h"
#include <wx/string.h>
#include <widgets/html_window.h>
#include <bitmaps.h>


#include <i18n_utility.h> // For _HKI definition in iec60664_help.h
wxString iec60664help =
#include "iec60664_help.h"

extern double DoubleFromString( const wxString& TextValue );

PANEL_ELECTRICAL_SPACING_IEC60664::PANEL_ELECTRICAL_SPACING_IEC60664( wxWindow* parent,
                                                                      wxWindowID id,
                                                                      const wxPoint& pos,
                                                                      const wxSize& size,
                                                                      long style,
                                                                      const wxString& name ) :
        PANEL_ELECTRICAL_SPACING_IEC60664_BASE( parent, id, pos, size, style, name )
{
    m_OVCchoice->SetSelection( 0 );
    m_insulationType->SetSelection( 0 );
    m_pollutionDegree->SetSelection( 0 );
    m_materialGroup->SetSelection( 0 );

    wxString msg;
    ConvertMarkdown2Html( wxGetTranslation( iec60664help ), msg );
    m_panelHelp->SetPage( msg );

    m_creepageclearanceBitmap->SetBitmap( KiBitmapBundle( BITMAPS::creepage_clearance ) );

	Layout();
}


PANEL_ELECTRICAL_SPACING_IEC60664::~PANEL_ELECTRICAL_SPACING_IEC60664()
{
}


void PANEL_ELECTRICAL_SPACING_IEC60664::ThemeChanged()
{
    m_creepageclearanceBitmap->SetBitmap( KiBitmapBundle( BITMAPS::creepage_clearance ) );
}


void PANEL_ELECTRICAL_SPACING_IEC60664::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    if( !aCfg )
        return;

    aCfg->m_Electrical.iec60664_ratedVoltage = DoubleFromString( m_ratedVoltage->GetValue() );
    aCfg->m_Electrical.iec60664_RMSvoltage = DoubleFromString( m_RMSVoltage->GetValue() );
    aCfg->m_Electrical.iec60664_transientOV = DoubleFromString( m_transientOvervoltage->GetValue() );
    aCfg->m_Electrical.iec60664_peakOV = DoubleFromString( m_peakVoltage->GetValue() );
    aCfg->m_Electrical.iec60664_altitude = DoubleFromString( m_altitude->GetValue() );

    aCfg->m_Electrical.iec60664_OVC = m_OVCchoice->GetSelection();
    aCfg->m_Electrical.iec60664_insulationType = m_insulationType->GetSelection();
    aCfg->m_Electrical.iec60664_pollutionDegree = m_pollutionDegree->GetSelection();
    aCfg->m_Electrical.iec60664_materialGroup = m_materialGroup->GetSelection();

    aCfg->m_Electrical.iec60664_pcbMaterial = m_pcbMaterial->GetValue() ? 1 : 0;
}


void PANEL_ELECTRICAL_SPACING_IEC60664::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    if( !aCfg )
        return;

    m_ratedVoltage->SetValue( wxString( "" ) << aCfg->m_Electrical.iec60664_ratedVoltage );
    m_RMSVoltage->SetValue( wxString( "" ) << aCfg->m_Electrical.iec60664_RMSvoltage );
    m_transientOvervoltage->SetValue( wxString( "" ) << aCfg->m_Electrical.iec60664_transientOV );
    m_peakVoltage->SetValue( wxString( "" ) << aCfg->m_Electrical.iec60664_peakOV );
    m_altitude->SetValue( wxString( "" ) << aCfg->m_Electrical.iec60664_altitude );

    m_OVCchoice->SetSelection( aCfg->m_Electrical.iec60664_OVC );
    m_insulationType->SetSelection( aCfg->m_Electrical.iec60664_insulationType );
    m_pollutionDegree->SetSelection( aCfg->m_Electrical.iec60664_pollutionDegree );
    m_materialGroup->SetSelection( aCfg->m_Electrical.iec60664_materialGroup );

    m_pcbMaterial->SetValue( aCfg->m_Electrical.iec60664_pcbMaterial ? true : false );

    CalculateTransientImpulse();
    CalculateClearanceCreepage();
}


void PANEL_ELECTRICAL_SPACING_IEC60664::CalculateTransientImpulse()
{
    IEC60664::OV_CATEGORY ovc;

    switch( m_OVCchoice->GetSelection() )
    {
    case 0:  ovc = IEC60664::OV_CATEGORY::OV_I;   break;
    case 1:  ovc = IEC60664::OV_CATEGORY::OV_II;  break;
    case 2:  ovc = IEC60664::OV_CATEGORY::OV_III; break;
    default: ovc = IEC60664::OV_CATEGORY::OV_IV;  break;
    }

    wxString string = m_ratedVoltage->GetValue();
    double   value = 0;

    if( string.ToDouble( &value ) )
    {
        IEC60664 cal;
        cal.SetOvervoltageCategory( ovc );
        cal.SetRatedVoltage( value );
        string = "";
        double result = cal.GetRatedImpulseWithstandVoltage();

        if( result >= 0 )
            string << result / 1000;
        else
            string << _( "Error" );

        m_impulseVotlage1TxtCtrl->SetValue( string );
    }
}


void PANEL_ELECTRICAL_SPACING_IEC60664::UpdateTransientImpulse( wxCommandEvent& event )
{
    CalculateTransientImpulse();
}


void PANEL_ELECTRICAL_SPACING_IEC60664::CalculateClearanceCreepage()
{
    IEC60664::INSULATION_TYPE  insul;
    IEC60664::POLLUTION_DEGREE pd;
    IEC60664::MATERIAL_GROUP   mg;

    switch( m_insulationType->GetSelection() )
    {
    case 0:  insul = IEC60664::INSULATION_TYPE::FUNCTIONAL; break;
    case 1:  insul = IEC60664::INSULATION_TYPE::BASIC;      break;
    default: insul = IEC60664::INSULATION_TYPE::REINFORCED; break;
    }

    switch( m_pollutionDegree->GetSelection() )
    {
    case 0:  pd = IEC60664::POLLUTION_DEGREE::PD1; break;
    case 1:  pd = IEC60664::POLLUTION_DEGREE::PD2; break;
    case 2:  pd = IEC60664::POLLUTION_DEGREE::PD3; break;
    default: pd = IEC60664::POLLUTION_DEGREE::PD4; break;
    }

    switch( m_materialGroup->GetSelection() )
    {
    case 0:  mg = IEC60664::MATERIAL_GROUP::MG_I;    break;
    case 1:  mg = IEC60664::MATERIAL_GROUP::MG_II;   break;
    case 2:  mg = IEC60664::MATERIAL_GROUP::MG_IIIa; break;
    default: mg = IEC60664::MATERIAL_GROUP::MG_IIIb; break;
    }

    double altitude = 0;
    double transientV = 0;
    double RMSV = 0;
    double peakV = 0;

    if( m_altitude->GetValue().ToDouble( &altitude )
        && m_transientOvervoltage->GetValue().ToDouble( &transientV )
        && m_peakVoltage->GetValue().ToDouble( &peakV )
        && m_RMSVoltage->GetValue().ToDouble( &RMSV ) )
    {
        IEC60664 cal;
        cal.SetPollutionDegree( pd );
        cal.SetMaterialGroup( mg );
        cal.SetInsulationType( insul );
        cal.SetAltitude( altitude );
        cal.SetTransientVoltage( transientV );
        cal.SetPeakVoltage( peakV );
        cal.SetRMSVoltage( RMSV );
        cal.SetPCBMaterial( m_pcbMaterial->GetValue() );

        cal.Compute();

        double groove = cal.GetMinGrooveWidth();
        double creepage = cal.GetCreepageDistance();
        double clearange = cal.GetClearanceDistance();

        wxString string;

        if( groove >= 0 )
            string << groove;
        else
            string << _( "Error" );

        m_minGrooveWidth->SetValue( string );

        string.Clear();

        if( creepage >= 0 )
            string << creepage;
        else
            string << _( "Error" );

        m_creepage->SetValue( string );

        string.Clear();

        if( clearange >= 0 )
            string << clearange;
        else
            string << _( "Error" );

        m_clearance->SetValue( string );
    }
    else
    {
        m_minGrooveWidth->SetValue( _( "Error" ) );
        m_creepage->SetValue( _( "Error" ) );
        m_clearance->SetValue( _( "Error" ) );
    }
}


void PANEL_ELECTRICAL_SPACING_IEC60664::UpdateClearanceCreepage( wxCommandEvent& event )
{
    CalculateClearanceCreepage();
}
