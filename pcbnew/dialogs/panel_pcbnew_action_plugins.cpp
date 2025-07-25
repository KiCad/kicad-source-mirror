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

#include <action_plugin.h>
#include <api/api_plugin.h>
#include <bitmaps.h>
#include <dialog_footprint_wizard_list.h>
#include <grid_tricks.h>
#include <kiface_base.h>
#include <kiplatform/ui.h>
#include <panel_pcbnew_action_plugins.h>
#include <pcb_edit_frame.h>
#include <python/scripting/pcbnew_scripting.h>
#include <pcb_scripting_tool.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <api/api_plugin_manager.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/paged_dialog.h>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>


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
#ifdef KICAD_IPC_API
    API_PLUGIN_MANAGER& mgr = Pgm().GetPluginManager();
    wxString id = m_grid->GetCellValue( m_grid->GetGridCursorRow(),
                                        PANEL_PCBNEW_ACTION_PLUGINS::COLUMN_SETTINGS_IDENTIFIER );

    if( std::optional<const PLUGIN_ACTION*> action = mgr.GetAction( id ) )
    {
        menu.Append( MYID_RECREATE_ENV, _( "Recreate Plugin Environment" ), _( "Recreate Plugin Environment" ) );
        menu.AppendSeparator();
    }
#endif

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

        if( std::optional<const PLUGIN_ACTION*> action = mgr.GetAction( id ) )
            mgr.RecreatePluginEnvironment( ( *action )->plugin.Identifier() );
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

    m_moveUpButton->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_moveDownButton->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_openDirectoryButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_reloadButton->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );
    m_showErrorsButton->SetBitmap( KiBitmapBundle( BITMAPS::small_warning ) );
}


PANEL_PCBNEW_ACTION_PLUGINS::~PANEL_PCBNEW_ACTION_PLUGINS()
{
    m_grid->PopEventHandler( true );
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
    SCRIPTING_TOOL::ReloadPlugins();
    TransferDataToWindow();
}


bool PANEL_PCBNEW_ACTION_PLUGINS::TransferDataFromWindow()
{
    PCBNEW_SETTINGS* settings = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( settings );

#ifdef KICAD_IPC_API
    API_PLUGIN_MANAGER& mgr = Pgm().GetPluginManager();

    if( settings )
    {
        settings->m_VisibleActionPlugins.clear();
        settings->m_Plugins.actions.clear();

        for( int ii = 0; ii < m_grid->GetNumberRows(); ii++ )
        {
            wxString id = m_grid->GetCellValue( ii, COLUMN_SETTINGS_IDENTIFIER );

            if( mgr.GetAction( id ) != std::nullopt )
            {
                settings->m_Plugins.actions.emplace_back( std::make_pair(
                        id, m_grid->GetCellValue( ii, COLUMN_VISIBLE ) == wxT( "1" ) ) );
            }
            else
            {
                settings->m_VisibleActionPlugins.emplace_back( std::make_pair(
                        id, m_grid->GetCellValue( ii, COLUMN_VISIBLE ) == wxT( "1" ) ) );
            }
        }
    }
#else
    if( settings )
    {
        settings->m_VisibleActionPlugins.clear();

        for( int ii = 0; ii < m_grid->GetNumberRows(); ii++ )
        {
            wxString id = m_grid->GetCellValue( ii, COLUMN_SETTINGS_IDENTIFIER );

            settings->m_VisibleActionPlugins.emplace_back( std::make_pair(
                    id, m_grid->GetCellValue( ii, COLUMN_VISIBLE ) == wxT( "1" ) ) );
        }
    }
#endif

    return true;
}


bool PANEL_PCBNEW_ACTION_PLUGINS::TransferDataToWindow()
{
    m_grid->Freeze();

    m_grid->ClearRows();

    const std::vector<LEGACY_OR_API_PLUGIN>& orderedPlugins = PCB_EDIT_FRAME::GetOrderedActionPlugins();
    m_grid->AppendRows( orderedPlugins.size() );

    int size = Pgm().GetCommonSettings()->m_Appearance.toolbar_icon_size;
    wxSize iconSize( size, size );

    for( size_t row = 0; row < orderedPlugins.size(); row++ )
    {
        if( std::holds_alternative<ACTION_PLUGIN*>( orderedPlugins[row] ) )
        {
            auto ap = std::get<ACTION_PLUGIN*>( orderedPlugins[row] );

            // Icon
            m_grid->SetCellRenderer( row, COLUMN_ACTION_NAME,
                    new GRID_CELL_ICON_TEXT_RENDERER( ap->iconBitmap.IsOk() ? wxBitmapBundle( ap->iconBitmap )
                                                                            : m_genericIcon,
                                                      iconSize ) );
            m_grid->SetCellValue( row, COLUMN_ACTION_NAME, ap->GetName() );
            m_grid->SetCellValue( row, COLUMN_SETTINGS_IDENTIFIER, ap->GetPluginPath() );

            // Toolbar button checkbox
            m_grid->SetCellRenderer( row, COLUMN_VISIBLE, new wxGridCellBoolRenderer() );
            m_grid->SetCellAlignment( row, COLUMN_VISIBLE, wxALIGN_CENTER, wxALIGN_CENTER );

            bool show = PCB_EDIT_FRAME::GetActionPluginButtonVisible( ap->GetPluginPath(),
                                                                      ap->GetShowToolbarButton() );

            m_grid->SetCellValue( row, COLUMN_VISIBLE, show ? wxT( "1" ) : wxEmptyString );

            m_grid->SetCellValue( row, COLUMN_PLUGIN_NAME, ap->GetClassName() );
            m_grid->SetCellValue( row, COLUMN_DESCRIPTION, ap->GetDescription() );
        }
        else
        {
#ifdef KICAD_IPC_API
            auto action = std::get<const PLUGIN_ACTION*>( orderedPlugins[row] );

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

            bool show = PCB_EDIT_FRAME::GetActionPluginButtonVisible( action->identifier, action->show_button );

            m_grid->SetCellValue( row, COLUMN_VISIBLE, show ? wxT( "1" ) : wxEmptyString );

            m_grid->SetCellValue( row, COLUMN_PLUGIN_NAME, action->plugin.Name() );
            m_grid->SetCellValue( row, COLUMN_DESCRIPTION, action->description );
#endif
        }
    }

    for( int col = 0; col < m_grid->GetNumberCols(); col++ )
    {
        const wxString& heading = m_grid->GetColLabelValue( col );
        int             headingWidth = GetTextExtent( heading ).x + 2 * GRID_CELL_MARGIN;

        // Set the minimal width to the column label size.
        m_grid->SetColMinimalWidth( col, headingWidth );
        // Set the width to see the full contents
        m_grid->SetColSize( col, m_grid->GetVisibleWidth( col ) );
    }

    m_grid->AutoSizeRows();
    m_grid->AutoSizeColumns();
    m_grid->HideCol( COLUMN_SETTINGS_IDENTIFIER );

    m_grid->Thaw();

    // Show errors button should be disabled if there are no errors.
    wxString trace;

    if( ACTION_PLUGINS::GetActionsCount() )
        pcbnewGetWizardsBackTrace( trace );

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
    SCRIPTING_TOOL::ShowPluginFolder();
}


void PANEL_PCBNEW_ACTION_PLUGINS::OnShowErrorsButtonClick( wxCommandEvent& event )
{
    wxString trace;
    pcbnewGetWizardsBackTrace( trace );

    // Now display the filtered trace in our dialog
    // (a simple wxMessageBox is really not suitable for long messages)
    DIALOG_FOOTPRINT_WIZARD_LOG logWindow( wxGetTopLevelParent( this ) );
    logWindow.m_Message->SetValue( trace );
    logWindow.ShowModal();
}
