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
#include <widgets/wx_html_report_box.h>
#include <project/project_file.h>
#include <sch_edit_frame.h>
#include <sim/simulator_frame.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/simulator_control.h>
#include <scintilla_tricks.h>
#include <dialogs/dialog_user_defined_signals.h>


bool SIMULATOR_CONTROL::Init()
{
    Reset( MODEL_RELOAD );
    return true;
}


void SIMULATOR_CONTROL::Reset( RESET_REASON aReason )
{
    m_simulatorFrame = getEditFrame<SIMULATOR_FRAME>();

    if( m_simulatorFrame )
    {
        m_schematicFrame = m_simulatorFrame->GetSchematicFrame();
        m_circuitModel = m_simulatorFrame->GetCircuitModel();
        m_simulator = m_simulatorFrame->GetSimulator();
    }
}


int SIMULATOR_CONTROL::NewAnalysisTab( const TOOL_EVENT& aEvent )
{
    DIALOG_SIM_COMMAND dlg( m_simulatorFrame, m_circuitModel, m_simulator->Settings() );
    wxString           errors;
    WX_STRING_REPORTER reporter( &errors );

    if( !m_circuitModel->ReadSchematicAndLibraries( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS,
                                                    reporter ) )
    {
        DisplayErrorMessage( m_simulatorFrame,
                             _( "Errors during netlist generation.\n\n" ) + errors );
    }

    dlg.SetSimCommand( wxS( "*" ) );
    dlg.SetSimOptions( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS );

    if( dlg.ShowModal() == wxID_OK )
    {
        SIM_TAB* tab = m_simulatorFrame->NewSimTab( dlg.GetSimCommand() );
        dlg.ApplySettings( tab );
    }

    return 0;
}


int SIMULATOR_CONTROL::OpenWorkbook( const TOOL_EVENT& aEvent )
{
    wxFileDialog openDlg( m_simulatorFrame, _( "Open simulation workbook" ), getDefaultPath(), "",
                          WorkbookFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( openDlg.ShowModal() == wxID_CANCEL )
        return -1;

    m_simulatorFrame->LoadWorkbook( openDlg.GetPath() );
    return 0;
}


wxString SIMULATOR_CONTROL::getDefaultFilename()
{
    wxFileName filename = m_simulator->Settings()->GetWorkbookFilename();

    if( filename.GetName().IsEmpty() )
    {
        if( m_simulatorFrame->Prj().GetProjectName().IsEmpty() )
        {
            filename.SetName( _( "noname" ) );
            filename.SetExt( WorkbookFileExtension );
        }
        else
        {
            filename.SetName( m_simulatorFrame->Prj().GetProjectName() );
            filename.SetExt( WorkbookFileExtension );
        }
    }

    return filename.GetFullName();
}


wxString SIMULATOR_CONTROL::getDefaultPath()
{
    wxFileName path = m_simulator->Settings()->GetWorkbookFilename();

    path.Normalize( FN_NORMALIZE_FLAGS|wxPATH_NORM_ENV_VARS,
                    m_simulatorFrame->Prj().GetProjectPath() );
    return path.GetPath();
}


int SIMULATOR_CONTROL::SaveWorkbook( const TOOL_EVENT& aEvent )
{
    wxString filename;

    if( aEvent.IsAction( &EE_ACTIONS::saveWorkbook ) )
        filename = m_simulator->Settings()->GetWorkbookFilename();

    if( filename.IsEmpty() )
    {
        wxFileDialog saveAsDlg( m_simulatorFrame, _( "Save Simulation Workbook As" ),
                                getDefaultPath(), getDefaultFilename(), WorkbookFileWildcard(),
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( saveAsDlg.ShowModal() == wxID_CANCEL )
            return -1;

        filename = saveAsDlg.GetPath();
    }

    m_simulatorFrame->SaveWorkbook( m_simulatorFrame->Prj().AbsolutePath( filename ) );
    return 0;
}


int SIMULATOR_CONTROL::ExportPlotAsPNG( const TOOL_EVENT& aEvent )
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( getCurrentSimTab() ) )
    {
        wxFileDialog saveDlg( m_simulatorFrame, _( "Save Plot as Image" ), "", "",
                              PngFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( saveDlg.ShowModal() == wxID_CANCEL )
            return -1;

        plotTab->GetPlotWin()->SaveScreenshot( saveDlg.GetPath(), wxBITMAP_TYPE_PNG );
    }

    return 0;
}


SIM_TAB* SIMULATOR_CONTROL::getCurrentSimTab()
{
    return m_simulatorFrame->GetCurrentSimTab();
}


int SIMULATOR_CONTROL::ExportPlotAsCSV( const TOOL_EVENT& aEvent )
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( getCurrentSimTab() ) )
    {
        const wxChar SEPARATOR = ';';

        wxFileDialog saveDlg( m_simulatorFrame, _( "Save Plot Data" ), "", "", CsvFileWildcard(),
                              wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( saveDlg.ShowModal() == wxID_CANCEL )
            return -1;

        wxFFile out( saveDlg.GetPath(), "wb" );

        std::map<wxString, TRACE*> traces = plotTab->GetTraces();

        if( traces.size() == 0 )
            return -1;

        SIM_TYPE simType = plotTab->GetSimType();

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
    }

    return 0;
}


int SIMULATOR_CONTROL::Close( const TOOL_EVENT& aEvent )
{
    m_simulatorFrame->Close();
    return 0;
}


int SIMULATOR_CONTROL::Zoom( const TOOL_EVENT& aEvent )
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( getCurrentSimTab() ) )
    {
        if(      aEvent.IsAction( &ACTIONS::zoomInCenter ) )
            plotTab->GetPlotWin()->ZoomIn();
        else if( aEvent.IsAction( &ACTIONS::zoomOutCenter ) )
            plotTab->GetPlotWin()->ZoomOut();
        else if( aEvent.IsAction( &ACTIONS::zoomFitScreen ) )
            plotTab->GetPlotWin()->Fit();
    }

    return 0;
}


int SIMULATOR_CONTROL::ToggleGrid( const TOOL_EVENT& aEvent )
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( getCurrentSimTab() ) )
    {
        plotTab->ShowGrid( !plotTab->IsGridShown() );
        m_simulatorFrame->OnModify();
    }

    return 0;
}


int SIMULATOR_CONTROL::ToggleLegend( const TOOL_EVENT& aEvent )
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( getCurrentSimTab() ) )
    {
        plotTab->ShowLegend( !plotTab->IsLegendShown() );
        m_simulatorFrame->OnModify();
    }

    return 0;
}


int SIMULATOR_CONTROL::ToggleDottedSecondary( const TOOL_EVENT& aEvent )
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( getCurrentSimTab() ) )
    {
        plotTab->SetDottedSecondary( !plotTab->GetDottedSecondary() );
        m_simulatorFrame->OnModify();
    }

    return 0;
}


int SIMULATOR_CONTROL::ToggleDarkModePlots( const TOOL_EVENT& aEvent )
{
    m_simulatorFrame->ToggleDarkModePlots();
    return 0;
}


int SIMULATOR_CONTROL::EditAnalysisTab( const TOOL_EVENT& aEvent )
{
    m_simulatorFrame->EditAnalysis();
    return 0;
}


int SIMULATOR_CONTROL::RunSimulation( const TOOL_EVENT& aEvent )
{
    if( m_simulator->IsRunning() )
    {
        m_simulator->Stop();
        return 0;
    }

    if( !getCurrentSimTab() )
        NewAnalysisTab( aEvent );

    if( !getCurrentSimTab() )
        return 0;

    m_simulatorFrame->StartSimulation();

    return 0;
}


int SIMULATOR_CONTROL::Probe( const TOOL_EVENT& aEvent )
{
    if( m_schematicFrame == nullptr )
        return -1;

    wxWindow* blocking_dialog = m_schematicFrame->Kiway().GetBlockingDialog();

    if( blocking_dialog )
        blocking_dialog->Close( true );

    m_schematicFrame->GetToolManager()->PostAction( EE_ACTIONS::simProbe );
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

    m_schematicFrame->GetToolManager()->PostAction( EE_ACTIONS::simTune );
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

    NETLIST_VIEW_DIALOG( wxWindow* parent ) :
            DIALOG_SHIM( parent, wxID_ANY, _( "SPICE Netlist" ), wxDefaultPosition,
                         wxSize( 800, 800 ), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
            m_textCtrl( nullptr ),
            m_reporter( nullptr )
    {
        m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                           wxSP_LIVE_UPDATE | wxSP_NOBORDER | wxSP_3DSASH );

        //Avoid the splitter window being assigned as the Parent to additional windows
        m_splitter->SetExtraStyle( wxWS_EX_TRANSIENT );

        m_textCtrl = new wxStyledTextCtrl( m_splitter, wxID_ANY );

        m_textCtrl->SetMarginWidth( MARGIN_LINE_NUMBERS, 50 );
        m_textCtrl->StyleSetForeground( wxSTC_STYLE_LINENUMBER, wxColour( 75, 75, 75 ) );
        m_textCtrl->StyleSetBackground( wxSTC_STYLE_LINENUMBER, wxColour( 220, 220, 220 ) );
        m_textCtrl->SetMarginType( MARGIN_LINE_NUMBERS, wxSTC_MARGIN_NUMBER );

        wxFont fixedFont = KIUI::GetMonospacedUIFont();

        for( int i = 0; i < wxSTC_STYLE_MAX; ++i )
            m_textCtrl->StyleSetFont( i, fixedFont );

        m_textCtrl->StyleClearAll();  // Addresses a bug in wx3.0 where styles are not correctly set

        m_textCtrl->SetWrapMode( wxSTC_WRAP_WORD );
        m_textCtrl->SetLexer( wxSTC_LEX_SPICE );
        m_textCtrl->SetMinSize( wxSize( 40, 40 ) );
        m_textCtrl->SetSize( wxSize( 40, 40 ) );

        m_reporter = new WX_HTML_REPORT_BOX( m_splitter, wxID_ANY );
        m_reporter->SetMinSize( wxSize( 40, 40 ) );
        m_reporter->SetSize( wxSize( 40, 40 ) );

        m_splitter->SetMinimumPaneSize( 40 );
        m_splitter->SetSashPosition( 760 );
        m_splitter->SetSashGravity( 0.9 );
        m_splitter->SplitHorizontally( m_textCtrl, m_reporter );

        wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
        sizer->Add( m_splitter, 1, wxEXPAND | wxALL, 5 );
        SetSizer( sizer );
        Layout();

        Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( NETLIST_VIEW_DIALOG::onClose ),
                 nullptr, this );

        m_scintillaTricks = std::make_unique<SCINTILLA_TRICKS>( m_textCtrl, wxT( "{}" ), false );

        finishDialogSettings();
    }

    void SetNetlist( const wxString& aSource )
    {
        m_textCtrl->SetText( aSource );
        m_textCtrl->SetEditable( false );

        m_reporter->Flush();
    }

    REPORTER* GetReporter() { return m_reporter; }

private:
    wxSplitterWindow*                 m_splitter;
    wxStyledTextCtrl*                 m_textCtrl;
    WX_HTML_REPORT_BOX*               m_reporter;

    std::unique_ptr<SCINTILLA_TRICKS> m_scintillaTricks;
};


int SIMULATOR_CONTROL::EditUserDefinedSignals( const TOOL_EVENT& aEvent )
{
    std::map<int, wxString> userSignals = m_simulatorFrame->UserDefinedSignals();

    DIALOG_USER_DEFINED_SIGNALS dlg( m_simulatorFrame, &userSignals );

    if( dlg.ShowQuasiModal() == wxID_OK )
        m_simulatorFrame->SetUserDefinedSignals( userSignals );

    return 0;
}


int SIMULATOR_CONTROL::ShowNetlist( const TOOL_EVENT& aEvent )
{
    if( m_schematicFrame == nullptr || m_simulator == nullptr )
        return -1;

    STRING_FORMATTER    formatter;
    NETLIST_VIEW_DIALOG dlg( m_simulatorFrame );

    m_circuitModel->GetNetlist( m_simulatorFrame->GetCurrentSimCommand(),
                                m_simulatorFrame->GetCurrentOptions(),
                                &formatter, *dlg.GetReporter() );

    dlg.SetNetlist( wxString( formatter.GetString() ) );
    dlg.ShowModal();

    return 0;
}


void SIMULATOR_CONTROL::setTransitions()
{
    Go( &SIMULATOR_CONTROL::NewAnalysisTab,         EE_ACTIONS::newAnalysisTab.MakeEvent() );
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

    Go( &SIMULATOR_CONTROL::EditAnalysisTab,        EE_ACTIONS::simAnalysisProperties.MakeEvent() );
    Go( &SIMULATOR_CONTROL::RunSimulation,          EE_ACTIONS::runSimulation.MakeEvent() );
    Go( &SIMULATOR_CONTROL::RunSimulation,          EE_ACTIONS::stopSimulation.MakeEvent() );
    Go( &SIMULATOR_CONTROL::Probe,                  EE_ACTIONS::simProbe.MakeEvent() );
    Go( &SIMULATOR_CONTROL::Tune,                   EE_ACTIONS::simTune.MakeEvent() );

    Go( &SIMULATOR_CONTROL::EditUserDefinedSignals, EE_ACTIONS::editUserDefinedSignals.MakeEvent() );
    Go( &SIMULATOR_CONTROL::ShowNetlist,            EE_ACTIONS::showNetlist.MakeEvent() );
}
