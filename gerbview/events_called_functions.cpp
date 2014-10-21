/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file events_called_functions.cpp
 * @brief GerbView command event functions.
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <common.h>
#include <gestfich.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <kicad_device_context.h>
#include <gerbview_id.h>
#include <class_GERBER.h>
#include <dialog_helpers.h>
#include <class_DCodeSelectionbox.h>
#include <class_gerbview_layer_widget.h>
#include <dialog_show_page_borders.h>


// Event table:

BEGIN_EVENT_TABLE( GERBVIEW_FRAME, EDA_DRAW_FRAME )
    EVT_CLOSE( GERBVIEW_FRAME::OnCloseWindow )
    EVT_SIZE( GERBVIEW_FRAME::OnSize )

    EVT_TOOL( wxID_FILE, GERBVIEW_FRAME::Files_io )
    EVT_TOOL( ID_GERBVIEW_ERASE_ALL, GERBVIEW_FRAME::Files_io )
    EVT_TOOL( ID_GERBVIEW_LOAD_DRILL_FILE, GERBVIEW_FRAME::Files_io )
    EVT_TOOL( ID_NEW_BOARD, GERBVIEW_FRAME::Files_io )
    EVT_TOOL( ID_GERBVIEW_SET_PAGE_BORDER, GERBVIEW_FRAME::Process_Special_Functions )

    // Menu Files:
    EVT_MENU( wxID_FILE, GERBVIEW_FRAME::Files_io )
    EVT_MENU( ID_NEW_BOARD, GERBVIEW_FRAME::Files_io )
    EVT_MENU( ID_GEN_PLOT, GERBVIEW_FRAME::ToPlotter )
    EVT_MENU( ID_GERBVIEW_EXPORT_TO_PCBNEW, GERBVIEW_FRAME::ExportDataInPcbnewFormat )

    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, GERBVIEW_FRAME::OnGbrFileHistory )
    EVT_MENU_RANGE( ID_GERBVIEW_DRILL_FILE1, ID_GERBVIEW_DRILL_FILE9,
                    GERBVIEW_FRAME::OnDrlFileHistory )

    EVT_MENU( wxID_EXIT, GERBVIEW_FRAME::OnQuit )

    // menu Preferences
    EVT_MENU_RANGE( ID_PREFERENCES_HOTKEY_START, ID_PREFERENCES_HOTKEY_END,
                    GERBVIEW_FRAME::Process_Config )

    EVT_MENU( ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
              GERBVIEW_FRAME::OnSelectOptionToolbar )
    EVT_MENU( wxID_PREFERENCES, GERBVIEW_FRAME::InstallGerberOptionsDialog )

    // menu Postprocess
    EVT_MENU( ID_GERBVIEW_SHOW_LIST_DCODES, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_MENU( ID_GERBVIEW_SHOW_SOURCE, GERBVIEW_FRAME::OnShowGerberSourceFile )
    EVT_MENU( ID_MENU_GERBVIEW_SELECT_PREFERED_EDITOR,
              EDA_BASE_FRAME::OnSelectPreferredEditor )

    // menu Miscellaneous
    EVT_MENU( ID_GERBVIEW_GLOBAL_DELETE, GERBVIEW_FRAME::Process_Special_Functions )

    // Menu Help
    EVT_MENU( wxID_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_ABOUT, EDA_DRAW_FRAME::GetKicadAbout )

    EVT_TOOL( wxID_CUT, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_COPY, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_PASTE, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_PRINT, GERBVIEW_FRAME::ToPrinter )
    EVT_COMBOBOX( ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER,
                  GERBVIEW_FRAME::OnSelectActiveLayer )

    EVT_SELECT_DCODE( ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE, GERBVIEW_FRAME::OnSelectActiveDCode )

    // Vertical toolbar:
    EVT_TOOL( ID_NO_TOOL_SELECTED, GERBVIEW_FRAME::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    GERBVIEW_FRAME::Process_Special_Functions )

    // Option toolbar
    EVT_TOOL( ID_TB_OPTIONS_SHOW_POLAR_COORD, GERBVIEW_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH, GERBVIEW_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH, GERBVIEW_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_LINES_SKETCH, GERBVIEW_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
              GERBVIEW_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_DCODES, GERBVIEW_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_NEGATIVE_ITEMS, GERBVIEW_FRAME::OnSelectOptionToolbar )
    EVT_TOOL_RANGE( ID_TB_OPTIONS_SHOW_GBR_MODE_0, ID_TB_OPTIONS_SHOW_GBR_MODE_2,
                    GERBVIEW_FRAME::OnSelectDisplayMode )

    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_POLAR_COORD, GERBVIEW_FRAME::OnUpdateCoordType )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH,
                   GERBVIEW_FRAME::OnUpdateFlashedItemsDrawMode )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_LINES_SKETCH, GERBVIEW_FRAME::OnUpdateLinesDrawMode )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH, GERBVIEW_FRAME::OnUpdatePolygonsDrawMode )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_DCODES, GERBVIEW_FRAME::OnUpdateShowDCodes )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_NEGATIVE_ITEMS, GERBVIEW_FRAME::OnUpdateShowNegativeItems )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
                   GERBVIEW_FRAME::OnUpdateShowLayerManager )

    EVT_UPDATE_UI( ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE, GERBVIEW_FRAME::OnUpdateSelectDCode )
    EVT_UPDATE_UI( ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER,
                   GERBVIEW_FRAME::OnUpdateLayerSelectBox )
    EVT_UPDATE_UI_RANGE( ID_TB_OPTIONS_SHOW_GBR_MODE_0, ID_TB_OPTIONS_SHOW_GBR_MODE_2,
                         GERBVIEW_FRAME::OnUpdateDrawMode )

END_EVENT_TABLE()


/* Handles the selection of tools, menu, and popup menu commands.
 */
void GERBVIEW_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int           id = event.GetId();

    switch( id )
    {
    case wxID_CUT:
    case wxID_COPY:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_ZOOM_BLOCK:
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        m_canvas->EndMouseCapture();

        if( GetScreen()->m_BlockLocate.GetCommand() != BLOCK_IDLE )
        {
            /* Should not be executed, except bug */
            GetScreen()->m_BlockLocate.SetCommand( BLOCK_IDLE );
            GetScreen()->m_BlockLocate.SetState( STATE_NO_BLOCK );
            GetScreen()->m_BlockLocate.ClearItemsList();
        }

        if( GetToolId() == ID_NO_TOOL_SELECTED )
            SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        else
            m_canvas->SetCursor( (wxStockCursor) m_canvas->GetCurrentCursor() );
        break;

    default:
        m_canvas->EndMouseCapture();
        break;
    }

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    switch( id )
    {
    case ID_GERBVIEW_SET_PAGE_BORDER:
        {
            DIALOG_PAGE_SHOW_PAGE_BORDERS dlg( this );

            if( dlg.ShowModal() == wxID_OK )
                m_canvas->Refresh();
        }
        break;

    case ID_GERBVIEW_GLOBAL_DELETE:
        Erase_Current_DrawLayer( true );
        ClearMsgPanel();
        break;

    case ID_NO_TOOL_SELECTED:
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        break;

    case ID_POPUP_CLOSE_CURRENT_TOOL:
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        break;

    case ID_GERBVIEW_SHOW_LIST_DCODES:
        Liste_D_Codes();
        break;

    case ID_POPUP_PLACE_BLOCK:
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_MOVE );
        m_canvas->SetAutoPanRequest( false );
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_ZOOM_BLOCK:
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_ZOOM );
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_DELETE );
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    default:
        wxFAIL_MSG( wxT( "GERBVIEW_FRAME::Process_Special_Functions error" ) );
        break;
    }
}


void GERBVIEW_FRAME::OnSelectActiveDCode( wxCommandEvent& event )
{
    GERBER_IMAGE* gerber_image = g_GERBER_List[getActiveLayer()];

    if( gerber_image )
    {
        int tool = m_DCodeSelector->GetSelectedDCodeId();

        if( tool != gerber_image->m_Selected_Tool )
        {
            gerber_image->m_Selected_Tool = tool;
            m_canvas->Refresh();
        }
    }
}


void GERBVIEW_FRAME::OnSelectActiveLayer( wxCommandEvent& event )
{
    int layer = getActiveLayer();

    setActiveLayer( event.GetSelection() );

    if( layer != getActiveLayer() )
    {
        if( m_LayersManager->OnLayerSelected() )
            m_canvas->Refresh();
    }
}


void GERBVIEW_FRAME::OnShowGerberSourceFile( wxCommandEvent& event )
{
    int     layer = getActiveLayer();
    GERBER_IMAGE* gerber_layer = g_GERBER_List[layer];

    if( gerber_layer )
    {
        wxString editorname = Pgm().GetEditorName();
        if( !editorname.IsEmpty() )
        {
            wxFileName fn( gerber_layer->m_FileName );
            ExecuteFile( this, editorname, QuoteFullPath( fn ) );
        }
        else
            wxMessageBox( _( "No editor defined. Please select one" ) );
    }

    else
    {
        wxString msg;
        msg.Printf( _( "No file loaded on the active layer %d" ), layer + 1 );
        wxMessageBox( msg );
    }
}


void GERBVIEW_FRAME::OnSelectDisplayMode( wxCommandEvent& event )
{
    int oldMode = GetDisplayMode();

    switch( event.GetId() )
    {
    case ID_TB_OPTIONS_SHOW_GBR_MODE_0:
        SetDisplayMode( 0 );
        break;

    case ID_TB_OPTIONS_SHOW_GBR_MODE_1:
        SetDisplayMode( 1 );
        break;

    case ID_TB_OPTIONS_SHOW_GBR_MODE_2:
        SetDisplayMode( 2 );
        break;
    }

    if( GetDisplayMode() != oldMode )
        m_canvas->Refresh();
}


void GERBVIEW_FRAME::OnQuit( wxCommandEvent& event )
{
    Close( true );
}


void GERBVIEW_FRAME::ShowChangedLanguage()
{
    // call my base class
    EDA_DRAW_FRAME::ShowChangedLanguage();

    m_LayersManager->SetLayersManagerTabsText();

    wxAuiPaneInfo& pane_info = m_auimgr.GetPane( m_LayersManager );
    pane_info.Caption( _( "Visibles" ) );
    m_auimgr.Update();

    ReFillLayerWidget();
}


void GERBVIEW_FRAME::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int     id = event.GetId();
    bool    state;

    switch( id )
    {
        case ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG:
            state = ! m_show_layer_manager_tools;
            id = ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR;
            break;

        default:
            state = m_optionsToolBar->GetToolToggled( id );
            break;
    }

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_POLAR_COORD:
        m_DisplayOptions.m_DisplayPolarCood = state;
        break;

    case ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH:
        m_DisplayOptions.m_DisplayFlashedItemsFill = not state;
        m_canvas->Refresh( true );
        break;

    case ID_TB_OPTIONS_SHOW_LINES_SKETCH:
        m_DisplayOptions.m_DisplayLinesFill = not state;
        m_canvas->Refresh( true );
        break;

    case ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH:
        m_DisplayOptions.m_DisplayPolygonsFill = not state;
        m_canvas->Refresh( true );
        break;

    case ID_TB_OPTIONS_SHOW_DCODES:
        SetElementVisibility( DCODES_VISIBLE, state );
        m_canvas->Refresh( true );
        break;

    case ID_TB_OPTIONS_SHOW_NEGATIVE_ITEMS:
        SetElementVisibility( NEGATIVE_OBJECTS_VISIBLE, state );
        m_canvas->Refresh( true );
        break;

    case ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR:

        // show/hide auxiliary Vertical layers and visibility manager toolbar
        m_show_layer_manager_tools = state;
        m_auimgr.GetPane( wxT( "m_LayersManagerToolBar" ) ).Show( m_show_layer_manager_tools );
        m_auimgr.Update();
        GetMenuBar()->SetLabel( ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                                m_show_layer_manager_tools ?
                                _("Hide &Layers Manager" ) : _("Show &Layers Manager" ));
        break;

    default:
        wxMessageBox( wxT( "GERBVIEW_FRAME::OnSelectOptionToolbar error" ) );
        break;
    }
}

