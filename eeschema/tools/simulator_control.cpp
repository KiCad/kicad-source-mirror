/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx_filename.h>
#include <wx/stc/stc.h>

#include <kiway.h>
#include <confirm.h>
#include <wildcards_and_files_ext.h>
#include <project/project_file.h>
#include <sch_edit_frame.h>
#include <sim/sim_plot_frame.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/simulator_control.h>
#include <scintilla_tricks.h>


bool SIMULATOR_CONTROL::Init()
{
    Reset( MODEL_RELOAD );
    return true;
}


void SIMULATOR_CONTROL::Reset( RESET_REASON aReason )
{
    m_plotFrame = getEditFrame<SIM_PLOT_FRAME>();

    if( m_plotFrame )
    {
        m_schematicFrame = m_plotFrame->GetSchematicFrame();
        m_circuitModel = m_plotFrame->GetCircuitModel();
        m_simulator = m_plotFrame->GetSimulator();
    }
}


int SIMULATOR_CONTROL::NewPlot( const TOOL_EVENT& aEvent )
{
    SIM_TYPE type = m_circuitModel->GetSimType();

    if( SIM_PANEL_BASE::IsPlottable( type ) )
        m_plotFrame->NewPlotPanel( m_circuitModel->GetSimCommand(), m_circuitModel->GetSimOptions() );

    return 0;
}


int SIMULATOR_CONTROL::OpenWorkbook( const TOOL_EVENT& aEvent )
{
    wxFileDialog openDlg( m_plotFrame, _( "Open simulation workbook" ), getDefaultPath(), "",
                          WorkbookFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( openDlg.ShowModal() == wxID_CANCEL )
        return -1;

    m_plotFrame->LoadWorkbook( openDlg.GetPath() );
    return 0;
}


wxString SIMULATOR_CONTROL::getDefaultFilename()
{
    wxFileName filename = m_simulator->Settings()->GetWorkbookFilename();

    if( filename.GetName().IsEmpty() )
    {
        if( m_plotFrame->Prj().GetProjectName().IsEmpty() )
        {
            filename.SetName( _( "noname" ) );
            filename.SetExt( WorkbookFileExtension );
        }
        else
        {
            filename.SetName( m_plotFrame->Prj().GetProjectName() );
            filename.SetExt( WorkbookFileExtension );
        }
    }

    return filename.GetFullName();
}


wxString SIMULATOR_CONTROL::getDefaultPath()
{
    wxFileName path = m_simulator->Settings()->GetWorkbookFilename();

    path.Normalize( FN_NORMALIZE_FLAGS|wxPATH_NORM_ENV_VARS, m_plotFrame->Prj().GetProjectPath() );
    return path.GetPath();
}


int SIMULATOR_CONTROL::SaveWorkbook( const TOOL_EVENT& aEvent )
{
    wxString filename;

    if( aEvent.IsAction( &EE_ACTIONS::saveWorkbook ) )
        filename = m_simulator->Settings()->GetWorkbookFilename();

    if( filename.IsEmpty() )
    {
        wxFileDialog saveAsDlg( m_plotFrame, _( "Save Simulation Workbook As" ), getDefaultPath(),
                                getDefaultFilename(), WorkbookFileWildcard(),
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( saveAsDlg.ShowModal() == wxID_CANCEL )
            return -1;

        filename = saveAsDlg.GetPath();
    }

    m_plotFrame->SaveWorkbook( m_plotFrame->Prj().AbsolutePath( filename ) );
    return 0;
}


int SIMULATOR_CONTROL::ExportPlotAsPNG( const TOOL_EVENT& aEvent )
{
    if( !m_plotFrame->GetCurrentPlot() )
        return -1;

    wxFileDialog saveDlg( m_plotFrame, _( "Save Plot as Image" ), "", "", PngFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return -1;

    m_plotFrame->GetCurrentPlot()->GetPlotWin()->SaveScreenshot( saveDlg.GetPath(),
                                                                 wxBITMAP_TYPE_PNG );

    return 0;
}


int SIMULATOR_CONTROL::ExportPlotAsCSV( const TOOL_EVENT& aEvent )
{
    if( !m_plotFrame->GetCurrentPlot() )
        return -1;

    const wxChar SEPARATOR = ';';

    wxFileDialog saveDlg( m_plotFrame, _( "Save Plot Data" ), "", "", CsvFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return -1;

    wxFFile out( saveDlg.GetPath(), "wb" );

    std::map<wxString, TRACE*> traces = m_plotFrame->GetCurrentPlot()->GetTraces();

    if( traces.size() == 0 )
        return -1;

    SIM_TYPE simType = m_circuitModel->GetSimType();

    std::size_t rowCount = traces.begin()->second->GetDataX().size();

    // write column header names on the first row
    wxString xAxisName( m_simulator->GetXAxis( simType ) );
    out.Write( wxString::Format( wxT( "%s%c" ), xAxisName, SEPARATOR ) );

    for( const auto& [name, trace] : traces )
        out.Write( wxString::Format( wxT( "%s%c" ), name, SEPARATOR ) );

    out.Write( wxS( "\r\n" ) );

    // write each row's numerical value
    for ( std::size_t curRow=0; curRow < rowCount; curRow++ )
    {
        double xAxisValue = traces.begin()->second->GetDataX().at( curRow );
        out.Write( wxString::Format( wxT( "%g%c" ), xAxisValue, SEPARATOR ) );

        for( const auto& [name, trace] : traces )
        {
            double yAxisValue = trace->GetDataY().at( curRow );
            out.Write( wxString::Format( wxT( "%g%c" ), yAxisValue, SEPARATOR ) );
        }

        out.Write( wxS( "\r\n" ) );
    }

    out.Close();
    return 0;
}


int SIMULATOR_CONTROL::Close( const TOOL_EVENT& aEvent )
{
    m_plotFrame->Close();
    return 0;
}


int SIMULATOR_CONTROL::Zoom( const TOOL_EVENT& aEvent )
{
    if( m_plotFrame->GetCurrentPlot() )
    {
        if( aEvent.IsAction( &ACTIONS::zoomInCenter ) )
            m_plotFrame->GetCurrentPlot()->GetPlotWin()->ZoomIn();
        else if( aEvent.IsAction( &ACTIONS::zoomOutCenter ) )
            m_plotFrame->GetCurrentPlot()->GetPlotWin()->ZoomOut();
        else if( aEvent.IsAction( &ACTIONS::zoomFitScreen ) )
            m_plotFrame->GetCurrentPlot()->GetPlotWin()->Fit();
    }

    return 0;
}


int SIMULATOR_CONTROL::ToggleGrid( const TOOL_EVENT& aEvent )
{
    SIM_PLOT_PANEL* plot = m_plotFrame->GetCurrentPlot();

    if( plot )
        plot->ShowGrid( !plot->IsGridShown() );

    return 0;
}


int SIMULATOR_CONTROL::ToggleLegend( const TOOL_EVENT& aEvent )
{
    SIM_PLOT_PANEL* plot = m_plotFrame->GetCurrentPlot();

    if( plot )
        plot->ShowLegend( !plot->IsLegendShown() );

    return 0;
}


int SIMULATOR_CONTROL::ToggleDottedSecondary( const TOOL_EVENT& aEvent )
{
    SIM_PLOT_PANEL* plot = m_plotFrame->GetCurrentPlot();

    if( plot )
        plot->SetDottedSecondary( !plot->GetDottedSecondary() );

    return 0;
}


int SIMULATOR_CONTROL::ToggleDarkModePlots( const TOOL_EVENT& aEvent )
{
    m_plotFrame->ToggleDarkModePlots();
    return 0;
}


int SIMULATOR_CONTROL::EditSimCommand( const TOOL_EVENT& aEvent )
{
    m_plotFrame->EditSimCommand();
    return 0;
}


int SIMULATOR_CONTROL::RunSimulation( const TOOL_EVENT& aEvent )
{
    if( m_simulator->IsRunning() )
        m_simulator->Stop();
    else
        m_plotFrame->StartSimulation();

    return 0;
}


int SIMULATOR_CONTROL::Probe( const TOOL_EVENT& aEvent )
{
    if( m_schematicFrame == nullptr )
        return -1;

    wxWindow* blocking_dialog = m_schematicFrame->Kiway().GetBlockingDialog();

    if( blocking_dialog )
        blocking_dialog->Close( true );

    m_schematicFrame->GetToolManager()->RunAction( EE_ACTIONS::simProbe );
    m_schematicFrame->Raise();

    return 0;
}


int SIMULATOR_CONTROL::Tune( const TOOL_EVENT& aEvent )
{
    if( m_schematicFrame == nullptr )
        return -1;

    wxWindow* blocking_dialog = m_schematicFrame->Kiway().GetBlockingDialog();

    if( blocking_dialog )
        blocking_dialog->Close( true );

    m_schematicFrame->GetToolManager()->RunAction( EE_ACTIONS::simTune );
    m_schematicFrame->Raise();

    return 0;
}


class NETLIST_VIEW_DIALOG : public DIALOG_SHIM
{
public:
    enum
    {
        MARGIN_LINE_NUMBERS
    };

    void onClose( wxCloseEvent& evt )
    {
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_CANCEL ) );
    }

    NETLIST_VIEW_DIALOG( wxWindow* parent, const wxString& source) :
            DIALOG_SHIM( parent, wxID_ANY, _( "SPICE Netlist" ), wxDefaultPosition,
                         wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
    {
        wxStyledTextCtrl* textCtrl = new wxStyledTextCtrl( this, wxID_ANY );
        textCtrl->SetMinSize( wxSize( 600, 400 ) );

        textCtrl->SetMarginWidth( MARGIN_LINE_NUMBERS, 50 );
        textCtrl->StyleSetForeground( wxSTC_STYLE_LINENUMBER, wxColour( 75, 75, 75 ) );
        textCtrl->StyleSetBackground( wxSTC_STYLE_LINENUMBER, wxColour( 220, 220, 220 ) );
        textCtrl->SetMarginType( MARGIN_LINE_NUMBERS, wxSTC_MARGIN_NUMBER );

        wxFont fixedFont = KIUI::GetMonospacedUIFont();

        for( int i = 0; i < wxSTC_STYLE_MAX; ++i )
            textCtrl->StyleSetFont( i, fixedFont );

        textCtrl->StyleClearAll();  // Addresses a bug in wx3.0 where styles are not correctly set

        textCtrl->SetWrapMode( wxSTC_WRAP_WORD );

        textCtrl->SetText( source );

        textCtrl->SetLexer( wxSTC_LEX_SPICE );

        wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
        sizer->Add( textCtrl, 1, wxEXPAND | wxALL, 5 );
        SetSizer( sizer );

        Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( NETLIST_VIEW_DIALOG::onClose ),
                 nullptr, this );

        m_scintillaTricks = std::make_unique<SCINTILLA_TRICKS>( textCtrl, wxT( "{}" ), false );

        finishDialogSettings();
    }

    std::unique_ptr<SCINTILLA_TRICKS> m_scintillaTricks;
};


int SIMULATOR_CONTROL::ShowNetlist( const TOOL_EVENT& aEvent )
{
    if( m_schematicFrame == nullptr || m_simulator == nullptr )
        return -1;

    wxString           errors;
    WX_STRING_REPORTER reporter( &errors );
    STRING_FORMATTER   formatter;

    m_circuitModel->SetSimOptions( m_plotFrame->GetCurrentOptions() );
    m_circuitModel->GetNetlist( &formatter, reporter );

    NETLIST_VIEW_DIALOG dlg( m_plotFrame, errors.IsEmpty() ? formatter.GetString() : errors );
    dlg.ShowModal();

    return 0;
}


void SIMULATOR_CONTROL::setTransitions()
{
    Go( &SIMULATOR_CONTROL::NewPlot,                EE_ACTIONS::newPlot.MakeEvent() );
    Go( &SIMULATOR_CONTROL::OpenWorkbook,           EE_ACTIONS::openWorkbook.MakeEvent() );
    Go( &SIMULATOR_CONTROL::SaveWorkbook,           EE_ACTIONS::saveWorkbook.MakeEvent() );
    Go( &SIMULATOR_CONTROL::SaveWorkbook,           EE_ACTIONS::saveWorkbookAs.MakeEvent() );
    Go( &SIMULATOR_CONTROL::ExportPlotAsPNG,        EE_ACTIONS::exportPlotAsPNG.MakeEvent() );
    Go( &SIMULATOR_CONTROL::ExportPlotAsCSV,        EE_ACTIONS::exportPlotAsCSV.MakeEvent() );
    Go( &SIMULATOR_CONTROL::Close,                  ACTIONS::quit.MakeEvent() );

    Go( &SIMULATOR_CONTROL::Zoom,                   ACTIONS::zoomInCenter.MakeEvent() );
    Go( &SIMULATOR_CONTROL::Zoom,                   ACTIONS::zoomOutCenter.MakeEvent() );
    Go( &SIMULATOR_CONTROL::Zoom,                   ACTIONS::zoomFitScreen.MakeEvent() );
    Go( &SIMULATOR_CONTROL::ToggleGrid,             ACTIONS::toggleGrid.MakeEvent() );
    Go( &SIMULATOR_CONTROL::ToggleLegend,           EE_ACTIONS::toggleLegend.MakeEvent() );
    Go( &SIMULATOR_CONTROL::ToggleDottedSecondary,  EE_ACTIONS::toggleDottedSecondary.MakeEvent() );
    Go( &SIMULATOR_CONTROL::ToggleDarkModePlots,    EE_ACTIONS::toggleDarkModePlots.MakeEvent() );

    Go( &SIMULATOR_CONTROL::EditSimCommand,         EE_ACTIONS::simCommand.MakeEvent() );
    Go( &SIMULATOR_CONTROL::RunSimulation,          EE_ACTIONS::runSimulation.MakeEvent() );
    Go( &SIMULATOR_CONTROL::RunSimulation,          EE_ACTIONS::stopSimulation.MakeEvent() );
    Go( &SIMULATOR_CONTROL::Probe,                  EE_ACTIONS::simProbe.MakeEvent() );
    Go( &SIMULATOR_CONTROL::Tune,                   EE_ACTIONS::simTune.MakeEvent() );

    Go( &SIMULATOR_CONTROL::ShowNetlist,            EE_ACTIONS::showNetlist.MakeEvent() );
}
