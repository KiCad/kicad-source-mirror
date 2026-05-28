/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <api/api_plugin.h>
#include <bitmaps.h>
#include <dialog_footprint_wizard_list.h>
#include <grid_tricks.h>
#include <kiface_base.h>
#include <kiplatform/ui.h>
#include <panel_pcbnew_action_plugins.h>
#include <paths.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <reporter.h>
#include <settings/common_settings.h>
#include <api/api_plugin_manager.h>
#include <dialog_HTML_reporter_base.h>
#include <launch_ext.h>
#include <widgets/kistatusbar.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/paged_dialog.h>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_html_report_box.h>
#include <wx/app.h>

#include <algorithm>


#define GRID_CELL_MARGIN 4

enum
{
    MYID_RECREATE_ENV = GRIDTRICKS_FIRST_CLIENT_ID
};

class PLUGINS_GRID_TRICKS : public GRID_TRICKS
{
public:
    PLUGINS_GRID_TRICKS( WX_GRID* aGrid ) :
        GRID_TRICKS( aGrid )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;
};


void PLUGINS_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    const int clickedRow = aEvent.GetRow();

    if( clickedRow >= 0 )
    {
        m_grid->SetGridCursor( clickedRow, m_grid->GetGridCursorCol() );
        m_grid->ClearSelection();
        m_grid->SelectRow( clickedRow );

#ifdef KICAD_IPC_API
        API_PLUGIN_MANAGER& mgr = Pgm().GetPluginManager();
        wxString id = m_grid->GetCellValue( clickedRow,
                                            PANEL_PCBNEW_ACTION_PLUGINS::COLUMN_SETTINGS_IDENTIFIER );

        if( std::optional<const PLUGIN_ACTION*> action = mgr.GetAction( id );
            action && ( *action )->plugin.Runtime().type == PLUGIN_RUNTIME_TYPE::PYTHON )
        {
            menu.Append( MYID_RECREATE_ENV, _( "Recreate Plugin Environment" ), _( "Recreate Plugin Environment" ) );
            menu.AppendSeparator();
        }
#endif
    }

    GRID_TRICKS::showPopupMenu( menu, aEvent );
}


void PLUGINS_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    if( event.GetId() == MYID_RECREATE_ENV )
    {
#ifdef KICAD_IPC_API
        API_PLUGIN_MANAGER& mgr = Pgm().GetPluginManager();
        wxString id = m_grid->GetCellValue( m_grid->GetGridCursorRow(),
                                            PANEL_PCBNEW_ACTION_PLUGINS::COLUMN_SETTINGS_IDENTIFIER );

        if( std::optional<const PLUGIN_ACTION*> action = mgr.GetAction( id );
            action && ( *action )->plugin.Runtime().type == PLUGIN_RUNTIME_TYPE::PYTHON )
        {
            mgr.RecreatePluginEnvironment( ( *action )->plugin.Identifier() );
        }
#endif
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


PANEL_PCBNEW_ACTION_PLUGINS::PANEL_PCBNEW_ACTION_PLUGINS( wxWindow* aParent ) :
        PANEL_PCBNEW_ACTION_PLUGINS_BASE( aParent )
{
    m_genericIcon = KiBitmapBundle( BITMAPS::puzzle_piece );
    m_grid->PushEventHandler( new PLUGINS_GRID_TRICKS( m_grid ) );
    m_grid->SetUseNativeColLabels();

    // Pin best size before TransferDataToWindow grows columns past the screen (#24408).
    m_grid->OverrideMinSize( 1.0, 1.0 );

    m_moveUpButton->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_moveDownButton->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_openDirectoryButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_reloadButton->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );
    m_showErrorsButton->SetBitmap( KiBitmapBundle( BITMAPS::small_warning ) );

    m_errorDialog = new DIALOG_HTML_REPORTER( aParent );
    m_allowErrorDialog = false;

    wxTheApp->Bind( EDA_EVT_PLUGIN_AVAILABILITY_CHANGED,
          &PANEL_PCBNEW_ACTION_PLUGINS::onPluginAvailabilityChanged, this );
}


PANEL_PCBNEW_ACTION_PLUGINS::~PANEL_PCBNEW_ACTION_PLUGINS()
{
    delete m_errorDialog;
    wxTheApp->Unbind( EDA_EVT_PLUGIN_AVAILABILITY_CHANGED,
            &PANEL_PCBNEW_ACTION_PLUGINS::onPluginAvailabilityChanged, this );
    m_grid->PopEventHandler( true );
}


void PANEL_PCBNEW_ACTION_PLUGINS::onPluginAvailabilityChanged( wxCommandEvent& aEvt )
{
    m_grid->Enable();
    TransferDataToWindow();

    if( m_allowErrorDialog && m_errorDialog->m_Reporter->HasMessage() )
    {
        m_errorDialog->m_Reporter->Flush();
        m_allowErrorDialog = false;
        m_errorDialog->ShowModal();
    }

    aEvt.Skip();
}


void PANEL_PCBNEW_ACTION_PLUGINS::OnGridCellClick( wxGridEvent& event )
{
    SelectRow( event.GetRow() );
}


void PANEL_PCBNEW_ACTION_PLUGINS::SelectRow( int aRow )
{
    m_grid->ClearSelection();
    m_grid->SelectRow( aRow );
}


void PANEL_PCBNEW_ACTION_PLUGINS::OnMoveUpButtonClick( wxCommandEvent& event )
{
    m_grid->OnMoveRowUp(
            [&]( int row )
            {
                SwapRows( row, row - 1 );
            } );
}


void PANEL_PCBNEW_ACTION_PLUGINS::OnMoveDownButtonClick( wxCommandEvent& event )
{
    m_grid->OnMoveRowDown(
            [&]( int row )
            {
                SwapRows( row, row + 1 );
            } );
}


void PANEL_PCBNEW_ACTION_PLUGINS::SwapRows( int aRowA, int aRowB )
{
    m_grid->Freeze();

    m_grid->SwapRows( aRowA, aRowB );

    // Swap icon column renderers
    auto cellRenderer = m_grid->GetCellRenderer( aRowA, COLUMN_ACTION_NAME );
    m_grid->SetCellRenderer( aRowA, COLUMN_ACTION_NAME, m_grid->GetCellRenderer( aRowB, COLUMN_ACTION_NAME ) );
    m_grid->SetCellRenderer( aRowB, COLUMN_ACTION_NAME, cellRenderer );

    m_grid->Thaw();
}


void PANEL_PCBNEW_ACTION_PLUGINS::OnReloadButtonClick( wxCommandEvent& event )
{
    API_PLUGIN_MANAGER& mgr = Pgm().GetPluginManager();
    m_errorDialog->m_Reporter->Clear();
    auto reporter = std::make_shared<REDIRECT_REPORTER>( m_errorDialog->m_Reporter );
    m_allowErrorDialog = true;
    mgr.ReloadPlugins( std::nullopt, reporter );
    m_grid->Disable();
}


bool PANEL_PCBNEW_ACTION_PLUGINS::TransferDataFromWindow()
{
    PCBNEW_SETTINGS* settings = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( settings );

#ifdef KICAD_IPC_API
    API_PLUGIN_MANAGER& mgr = Pgm().GetPluginManager();

    if( settings )
    {
        settings->m_Plugins.actions.clear();

        for( int ii = 0; ii < m_grid->GetNumberRows(); ii++ )
        {
            wxString id = m_grid->GetCellValue( ii, COLUMN_SETTINGS_IDENTIFIER );

            if( mgr.GetAction( id ) != std::nullopt )
            {
                settings->m_Plugins.actions.emplace_back( std::make_pair(
                        id, m_grid->GetCellValue( ii, COLUMN_VISIBLE ) == wxT( "1" ) ) );
            }
        }
    }
#endif

    return true;
}


bool PANEL_PCBNEW_ACTION_PLUGINS::TransferDataToWindow()
{
    m_grid->Freeze();

    m_grid->ClearRows();

    const std::vector<const PLUGIN_ACTION*>& orderedPlugins = PCB_EDIT_FRAME::GetOrderedPluginActions();
    m_grid->AppendRows( orderedPlugins.size() );

    int size = Pgm().GetCommonSettings()->m_Appearance.toolbar_icon_size;
    wxSize iconSize( size, size );

    for( size_t row = 0; row < orderedPlugins.size(); row++ )
    {
#ifdef KICAD_IPC_API
            const PLUGIN_ACTION* action = orderedPlugins[row];

            const wxBitmapBundle& icon = KIPLATFORM::UI::IsDarkTheme() && action->icon_dark.IsOk() ? action->icon_dark
                                                                                                   : action->icon_light;

            // Icon
            m_grid->SetCellRenderer( row, COLUMN_ACTION_NAME, new GRID_CELL_ICON_TEXT_RENDERER(
                                     icon.IsOk() ? icon : m_genericIcon, iconSize ) );
            m_grid->SetCellValue( row, COLUMN_ACTION_NAME, action->name );
            m_grid->SetCellValue( row, COLUMN_SETTINGS_IDENTIFIER, action->identifier );

            // Toolbar button checkbox
            m_grid->SetCellRenderer( row, COLUMN_VISIBLE, new wxGridCellBoolRenderer() );
            m_grid->SetCellAlignment( row, COLUMN_VISIBLE, wxALIGN_CENTER, wxALIGN_CENTER );

            bool show = PCB_EDIT_FRAME::GetPluginActionButtonVisible( action->identifier, action->show_button );

            m_grid->SetCellValue( row, COLUMN_VISIBLE, show ? wxT( "1" ) : wxEmptyString );

            m_grid->SetCellValue( row, COLUMN_PLUGIN_NAME, action->plugin.Name() );
            m_grid->SetCellValue( row, COLUMN_DESCRIPTION, action->description );
#endif
    }

    const int colMaxWidth = FromDIP( 400 );

    for( int col = 0; col < m_grid->GetNumberCols(); col++ )
    {
        const wxString& heading = m_grid->GetColLabelValue( col );
        int             headingWidth = GetTextExtent( heading ).x + 2 * GRID_CELL_MARGIN;

        m_grid->SetColMinimalWidth( col, headingWidth );
        int width = std::min( m_grid->GetVisibleWidth( col ), colMaxWidth );
        m_grid->SetColSize( col, std::max( headingWidth, width ) );
    }

    m_grid->AutoSizeRows();
    // AutoSizeColumns() would re-expand columns to full content width (setAsMin=true) and undo
    // the cap above (#24408).
    m_grid->HideCol( COLUMN_SETTINGS_IDENTIFIER );

    m_grid->Thaw();

    // Show errors button should be disabled if there are no errors.
    wxString trace;

    if( trace.empty() )
    {
        m_showErrorsButton->Disable();
        m_showErrorsButton->Hide();
    }
    else
    {
        m_showErrorsButton->Enable();
        m_showErrorsButton->Show();
    }

    return true;
}


void PANEL_PCBNEW_ACTION_PLUGINS::OnOpenDirectoryButtonClick( wxCommandEvent& event )
{
    wxString dir( PATHS::GetUserPluginsPath() );
    LaunchExternal( dir );
}


void PANEL_PCBNEW_ACTION_PLUGINS::OnShowErrorsButtonClick( wxCommandEvent& event )
{
    wxString trace;

    // Now display the filtered trace in our dialog
    // (a simple wxMessageBox is really not suitable for long messages)
    DIALOG_FOOTPRINT_WIZARD_LOG logWindow( wxGetTopLevelParent( this ) );
    logWindow.m_Message->SetValue( trace );
    logWindow.ShowModal();
}
