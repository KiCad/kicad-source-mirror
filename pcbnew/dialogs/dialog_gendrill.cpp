/**
 * @file dialog_gendrill.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean_Pierre Charras <jp.charras@ujf-grenoble.fr>
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <pcbplot.h>
#include <gendrill.h>

#include <class_board.h>
#include <class_track.h>
#include <class_module.h>

#include <dialog_gendrill.h>


// Keywords for read and write config
#define ZerosFormatKey          wxT( "DrillZerosFormat" )
#define PrecisionKey            wxT( "DrilltPrecisionOpt" )
#define MirrorKey               wxT( "DrillMirrorYOpt" )
#define MinimalHeaderKey        wxT( "DrillMinHeader" )
#define UnitDrillInchKey        wxT( "DrillUnit" )
#define DrillOriginIsAuxAxisKey wxT( "DrillAuxAxis" )

// list of allowed precision for EXCELLON files, for integer format:
// Due to difference between inches and mm,
// there are 2 set of reasonnable precision values, one for inches and one for metric
static DRILL_PRECISION precisionListForInches[] =
{
    DRILL_PRECISION( 2, 3 ), DRILL_PRECISION( 2, 4 )
};

static DRILL_PRECISION precisionListForMetric[] =
{
    DRILL_PRECISION( 3, 2 ), DRILL_PRECISION( 3, 3 )
};


DIALOG_GENDRILL::DIALOG_GENDRILL( PCB_EDIT_FRAME* parent ) :
    DIALOG_GENDRILL_BASE( parent )
{
    m_parent = parent;
    m_board  = parent->GetBoard();

    SetReturnCode( 1 );
    initDialog();
    GetSizer()->SetSizeHints( this );
}


// Static members of DIALOG_GENDRILL
int DIALOG_GENDRILL:: m_UnitDrillIsInch = true;
int DIALOG_GENDRILL:: m_ZerosFormat     = EXCELLON_WRITER::DECIMAL_FORMAT;
bool DIALOG_GENDRILL::m_MinimalHeader   = false;
bool DIALOG_GENDRILL::m_Mirror = true;
bool DIALOG_GENDRILL::m_DrillOriginIsAuxAxis = false;
int DIALOG_GENDRILL:: m_PrecisionFormat = 1;
bool DIALOG_GENDRILL::m_createRpt = false;
int DIALOG_GENDRILL::m_createMap = 0;


DIALOG_GENDRILL::~DIALOG_GENDRILL()
{
    UpdateConfig();
}


void DIALOG_GENDRILL::initDialog()
{
    wxConfig* Config = wxGetApp().GetSettings();

    if( Config )
    {
        Config->Read( ZerosFormatKey, &DIALOG_GENDRILL::m_ZerosFormat );
        Config->Read( PrecisionKey, &DIALOG_GENDRILL::m_PrecisionFormat );
        Config->Read( MirrorKey, &DIALOG_GENDRILL::m_Mirror );
        Config->Read( MinimalHeaderKey, &DIALOG_GENDRILL::m_MinimalHeader );
        Config->Read( UnitDrillInchKey, &DIALOG_GENDRILL::m_UnitDrillIsInch );
        Config->Read( DrillOriginIsAuxAxisKey, &DIALOG_GENDRILL::m_DrillOriginIsAuxAxis );
    }

    InitDisplayParams();
}


void DIALOG_GENDRILL::InitDisplayParams()
{
    wxString msg;
    const PCB_PLOT_PARAMS& plot_opts = m_board->GetPlotOptions();

    m_Choice_Unit->SetSelection( m_UnitDrillIsInch ? 1 : 0 );
    m_Choice_Precision->SetSelection( m_PrecisionFormat );
    m_Choice_Zeros_Format->SetSelection( m_ZerosFormat );

    if( m_ZerosFormat == EXCELLON_WRITER::DECIMAL_FORMAT )
        m_Choice_Precision->Enable( false );

    UpdatePrecisionOptions();

    m_Check_Minimal->SetValue( m_MinimalHeader );

    if( m_DrillOriginIsAuxAxis )
        m_Choice_Drill_Offset->SetSelection( 1 );

    m_Check_Mirror->SetValue( m_Mirror );

    m_Choice_Drill_Map->SetSelection( m_createMap );
    m_Choice_Drill_Report->SetSelection( m_createRpt );

    m_ViaDrillValue->SetLabel( _( "Use Netclasses values" ) );

    m_MicroViaDrillValue->SetLabel( _( "Use Netclasses values" ) );

    msg.Empty();
    msg << plot_opts.m_HPGLPenNum;
    m_PenNum->SetValue( msg );

    msg.Empty();
    msg << plot_opts.m_HPGLPenSpeed;
    m_PenSpeed->SetValue( msg );

    // See if we have some buried vias or/and microvias, and display
    // microvias drill value if so
    m_throughViasCount = 0;
    m_microViasCount   = 0;
    m_blindOrBuriedViasCount = 0;

    for( TRACK* track = m_parent->GetBoard()->m_Track; track != NULL; track = track->Next() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        if( track->GetShape() == VIA_THROUGH )
            m_throughViasCount++;
        else if( track->GetShape() == VIA_MICROVIA )
            m_microViasCount++;
        else if( track->GetShape() == VIA_BLIND_BURIED )
            m_blindOrBuriedViasCount++;
    }

    m_MicroViaDrillValue->Enable( m_microViasCount );

    // Count plated pad holes and not plated pad holes:
    m_platedPadsHoleCount    = 0;
    m_notplatedPadsHoleCount = 0;

    for( MODULE* module = m_parent->GetBoard()->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            if( pad->GetDrillShape() == PAD_CIRCLE )
            {
                if( pad->GetDrillSize().x != 0 )
                {
                    if( pad->GetAttribute() == PAD_HOLE_NOT_PLATED )
                        m_notplatedPadsHoleCount++;
                    else
                        m_platedPadsHoleCount++;
                }
            }
            else
            {
                if( std::min( pad->GetDrillSize().x, pad->GetDrillSize().y ) != 0 )
                {
                    if( pad->GetAttribute() == PAD_HOLE_NOT_PLATED )
                        m_notplatedPadsHoleCount++;
                    else
                        m_platedPadsHoleCount++;
                }
            }
        }
    }

    // Display hole counts:
    msg = m_PlatedPadsCountInfoMsg->GetLabel();
    msg << wxT( " " ) << m_platedPadsHoleCount;
    m_PlatedPadsCountInfoMsg->SetLabel( msg );

    msg = m_NotPlatedPadsCountInfoMsg->GetLabel();
    msg << wxT( " " ) << m_notplatedPadsHoleCount;
    m_NotPlatedPadsCountInfoMsg->SetLabel( msg );

    msg = m_ThroughViasInfoMsg->GetLabel();
    msg << wxT( " " ) << m_throughViasCount;
    m_ThroughViasInfoMsg->SetLabel( msg );

    msg = m_MicroViasInfoMsg->GetLabel();
    msg << wxT( " " ) << m_microViasCount;
    m_MicroViasInfoMsg->SetLabel( msg );

    msg = m_BuriedViasInfoMsg->GetLabel();
    msg << wxT( " " ) << m_blindOrBuriedViasCount;
    m_BuriedViasInfoMsg->SetLabel( msg );
}


void DIALOG_GENDRILL::UpdateConfig()
{
    SetParams();

    wxConfig* config = wxGetApp().GetSettings();

    if( config )
    {
        config->Write( ZerosFormatKey, m_ZerosFormat );
        config->Write( PrecisionKey, m_PrecisionFormat );
        config->Write( MirrorKey, m_Mirror );
        config->Write( MinimalHeaderKey, m_MinimalHeader );
        config->Write( UnitDrillInchKey, m_UnitDrillIsInch );
        config->Write( DrillOriginIsAuxAxisKey, m_DrillOriginIsAuxAxis );
    }
}


void DIALOG_GENDRILL::OnSelDrillUnitsSelected( wxCommandEvent& event )
{
    UpdatePrecisionOptions();
}


void DIALOG_GENDRILL::OnOkClick( wxCommandEvent& event )
{
    GenDrillAndReportFiles();
    EndModal( wxID_OK );
}


void DIALOG_GENDRILL::OnCancelClick( wxCommandEvent& event )
{
    UpdateConfig();                 // Save drill options:
    EndModal( wxID_CANCEL );        // Process the default cancel event (close dialog)
}


void DIALOG_GENDRILL::OnSelZerosFmtSelected( wxCommandEvent& event )
{
    UpdatePrecisionOptions();
}


void DIALOG_GENDRILL::UpdatePrecisionOptions()
{
    if( m_Choice_Unit->GetSelection()== 1 )     // Units = inches
    {
        // inch options
        m_Choice_Precision->SetString( 0, precisionListForInches[0].GetPrecisionString() );
        m_Choice_Precision->SetString( 1, precisionListForInches[1].GetPrecisionString() );
    }
    else
    {
        // metric options
        m_Choice_Precision->SetString( 0, precisionListForMetric[0].GetPrecisionString() );
        m_Choice_Precision->SetString( 1, precisionListForMetric[1].GetPrecisionString() );
    }

    if( m_Choice_Zeros_Format->GetSelection() == EXCELLON_WRITER::DECIMAL_FORMAT )
        m_Choice_Precision->Enable( false );
    else
        m_Choice_Precision->Enable( true );
}


void DIALOG_GENDRILL::SetParams()
{
    wxString msg;
    long     ltmp;

    PCB_PLOT_PARAMS plot_opts = m_board->GetPlotOptions();

    m_createMap = m_Choice_Drill_Map->GetSelection();
    m_createRpt = m_Choice_Drill_Report->GetSelection();

    m_UnitDrillIsInch = (m_Choice_Unit->GetSelection() == 0) ? false : true;
    m_MinimalHeader   = m_Check_Minimal->IsChecked();
    m_Mirror = m_Check_Mirror->IsChecked();
    m_ZerosFormat = m_Choice_Zeros_Format->GetSelection();
    m_DrillOriginIsAuxAxis = m_Choice_Drill_Offset->GetSelection();
    m_PrecisionFormat = m_Choice_Precision->GetSelection();

    msg = m_PenSpeed->GetValue();

    if( msg.ToLong( &ltmp ) )
        plot_opts.m_HPGLPenSpeed = ltmp;

    msg = m_PenNum->GetValue();

    if( msg.ToLong( &ltmp ) )
        plot_opts.m_HPGLPenNum = ltmp;

    if( m_Choice_Drill_Offset->GetSelection() == 0 )
        m_FileDrillOffset = wxPoint( 0, 0 );
    else
        m_FileDrillOffset = m_parent->GetOriginAxisPosition();

    // get precision
    int idx = m_Choice_Precision->GetSelection();

    if( m_UnitDrillIsInch )
        m_Precision = precisionListForInches[idx];
    else
        m_Precision = precisionListForMetric[idx];

    m_board->SetPlotOptions( plot_opts );
}
