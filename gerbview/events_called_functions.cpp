/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gestfich.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <kicad_device_context.h>
#include <gerbview_id.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <dialog_helpers.h>
#include <DCodeSelectionbox.h>
#include <gerbview_layer_widget.h>

#include <gerbview_draw_panel_gal.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>
#include <gerbview_painter.h>
#include <view/view.h>


// Event table:

BEGIN_EVENT_TABLE( GERBVIEW_FRAME, EDA_DRAW_FRAME )
    EVT_CLOSE( GERBVIEW_FRAME::OnCloseWindow )
    EVT_SIZE( GERBVIEW_FRAME::OnSize )

    EVT_TOOL( wxID_FILE, GERBVIEW_FRAME::Files_io )
    EVT_TOOL( ID_GERBVIEW_ERASE_ALL, GERBVIEW_FRAME::Files_io )
    EVT_TOOL( ID_GERBVIEW_RELOAD_ALL, GERBVIEW_FRAME::Files_io )
    EVT_TOOL( ID_GERBVIEW_LOAD_DRILL_FILE, GERBVIEW_FRAME::Files_io )
    EVT_TOOL( ID_GERBVIEW_LOAD_ZIP_ARCHIVE_FILE, GERBVIEW_FRAME::Files_io )
    EVT_TOOL( ID_GERBVIEW_LOAD_JOB_FILE, GERBVIEW_FRAME::Files_io )
    EVT_TOOL( ID_NEW_BOARD, GERBVIEW_FRAME::Files_io )

    // Menu Files:
    EVT_MENU( wxID_FILE, GERBVIEW_FRAME::Files_io )
    EVT_MENU( ID_NEW_BOARD, GERBVIEW_FRAME::Files_io )
    EVT_MENU( ID_GERBVIEW_EXPORT_TO_PCBNEW, GERBVIEW_FRAME::ExportDataInPcbnewFormat )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, GERBVIEW_FRAME::OnGbrFileHistory )

    EVT_MENU_RANGE( ID_GERBVIEW_DRILL_FILE1, ID_GERBVIEW_DRILL_FILEMAX,
                    GERBVIEW_FRAME::OnDrlFileHistory )

    EVT_MENU_RANGE( ID_GERBVIEW_ZIP_FILE1, ID_GERBVIEW_ZIP_FILEMAX,
                    GERBVIEW_FRAME::OnZipFileHistory )

    EVT_MENU_RANGE( ID_GERBVIEW_JOB_FILE1, ID_GERBVIEW_JOB_FILEMAX,
                    GERBVIEW_FRAME::OnJobFileHistory )

    EVT_MENU( wxID_EXIT, GERBVIEW_FRAME::OnQuit )

    // menu Preferences
    EVT_MENU( ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST, GERBVIEW_FRAME::Process_Config )

    EVT_MENU( wxID_PREFERENCES, GERBVIEW_FRAME::Process_Config )
    EVT_UPDATE_UI( ID_MENU_CANVAS_LEGACY, GERBVIEW_FRAME::OnUpdateSwitchCanvas )
    EVT_UPDATE_UI( ID_MENU_CANVAS_CAIRO, GERBVIEW_FRAME::OnUpdateSwitchCanvas )
    EVT_UPDATE_UI( ID_MENU_CANVAS_OPENGL, GERBVIEW_FRAME::OnUpdateSwitchCanvas )
    EVT_MENU( ID_MENU_CANVAS_LEGACY, GERBVIEW_FRAME::OnSwitchCanvas )
    EVT_MENU( ID_MENU_CANVAS_CAIRO, GERBVIEW_FRAME::OnSwitchCanvas )
    EVT_MENU( ID_MENU_CANVAS_OPENGL, GERBVIEW_FRAME::OnSwitchCanvas )

    // menu Postprocess
    EVT_MENU( ID_GERBVIEW_SHOW_LIST_DCODES, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_MENU( ID_GERBVIEW_SHOW_SOURCE, GERBVIEW_FRAME::OnShowGerberSourceFile )

    // menu Miscellaneous
    EVT_MENU( ID_GERBVIEW_ERASE_CURR_LAYER, GERBVIEW_FRAME::Process_Special_Functions )

    // Menu Help
    EVT_MENU( wxID_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_INDEX, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( ID_HELP_GET_INVOLVED, EDA_DRAW_FRAME::GetKicadContribute )
    EVT_MENU( wxID_ABOUT, EDA_DRAW_FRAME::GetKicadAbout )

    EVT_TOOL( wxID_UNDO, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_PRINT, GERBVIEW_FRAME::ToPrinter )
    EVT_COMBOBOX( ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER,
                  GERBVIEW_FRAME::OnSelectActiveLayer )

    EVT_SELECT_DCODE( ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE, GERBVIEW_FRAME::OnSelectActiveDCode )

    EVT_MENU( ID_MENU_ZOOM_SELECTION, GERBVIEW_FRAME::Process_Special_Functions )

    // Vertical toolbar:
    EVT_TOOL( ID_NO_TOOL_SELECTED, GERBVIEW_FRAME::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    GERBVIEW_FRAME::Process_Special_Functions )

    // Option toolbar
    //EVT_TOOL( ID_NO_TOOL_SELECTED, GERBVIEW_FRAME::Process_Special_Functions ) // mentioned below
    EVT_TOOL( ID_ZOOM_SELECTION, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_TB_MEASUREMENT_TOOL, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_POLAR_COORD, GERBVIEW_FRAME::OnToggleCoordType )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH, GERBVIEW_FRAME::OnTogglePolygonDrawMode )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH, GERBVIEW_FRAME::OnToggleFlashItemDrawMode )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_LINES_SKETCH, GERBVIEW_FRAME::OnToggleLineDrawMode )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
              GERBVIEW_FRAME::OnToggleShowLayerManager )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_DCODES, GERBVIEW_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_NEGATIVE_ITEMS, GERBVIEW_FRAME::OnSelectOptionToolbar )
    EVT_TOOL_RANGE( ID_TB_OPTIONS_SHOW_GBR_MODE_0, ID_TB_OPTIONS_SHOW_GBR_MODE_2,
                    GERBVIEW_FRAME::OnSelectDisplayMode )
    EVT_TOOL( ID_TB_OPTIONS_DIFF_MODE, GERBVIEW_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_HIGH_CONTRAST_MODE, GERBVIEW_FRAME::OnSelectOptionToolbar )

    // Auxiliary horizontal toolbar
    EVT_COMBOBOX( ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE, GERBVIEW_FRAME::OnSelectHighlightChoice )
    EVT_COMBOBOX( ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE, GERBVIEW_FRAME::OnSelectHighlightChoice )
    EVT_COMBOBOX( ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE,
                GERBVIEW_FRAME::OnSelectHighlightChoice )
    EVT_COMBOBOX( ID_ON_ZOOM_SELECT, GERBVIEW_FRAME::OnSelectZoom )
    EVT_COMBOBOX( ID_ON_GRID_SELECT, GERBVIEW_FRAME::OnSelectGrid )

    // Right click context menu
    EVT_MENU( ID_HIGHLIGHT_CMP_ITEMS, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_MENU( ID_HIGHLIGHT_NET_ITEMS, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_MENU( ID_HIGHLIGHT_APER_ATTRIBUTE_ITEMS, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_MENU( ID_HIGHLIGHT_REMOVE_ALL, GERBVIEW_FRAME::Process_Special_Functions )

    EVT_UPDATE_UI( ID_NO_TOOL_SELECTED, GERBVIEW_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI( ID_ZOOM_SELECTION, GERBVIEW_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI( ID_TB_MEASUREMENT_TOOL, GERBVIEW_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_POLAR_COORD, GERBVIEW_FRAME::OnUpdateCoordType )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH,
                   GERBVIEW_FRAME::OnUpdateFlashedItemsDrawMode )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_LINES_SKETCH, GERBVIEW_FRAME::OnUpdateLineDrawMode )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH, GERBVIEW_FRAME::OnUpdatePolygonDrawMode )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_DCODES, GERBVIEW_FRAME::OnUpdateShowDCodes )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_NEGATIVE_ITEMS, GERBVIEW_FRAME::OnUpdateShowNegativeItems )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
                   GERBVIEW_FRAME::OnUpdateShowLayerManager )
    EVT_UPDATE_UI( ID_TB_OPTIONS_DIFF_MODE, GERBVIEW_FRAME::OnUpdateDiffMode )
    EVT_UPDATE_UI( ID_TB_OPTIONS_HIGH_CONTRAST_MODE, GERBVIEW_FRAME::OnUpdateHighContrastMode )
    EVT_UPDATE_UI( ID_ON_GRID_SELECT, GERBVIEW_FRAME::OnUpdateSelectGrid )
    EVT_UPDATE_UI( ID_ON_ZOOM_SELECT, GERBVIEW_FRAME::OnUpdateSelectZoom )

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
            SetNoToolSelected();
        else
            m_canvas->SetCursor( (wxStockCursor) m_canvas->GetCurrentCursor() );
        break;

    default:
        m_canvas->EndMouseCapture();
        break;
    }

    GERBER_DRAW_ITEM* currItem = (GERBER_DRAW_ITEM*) GetScreen()->GetCurItem();

    switch( id )
    {
    case ID_GERBVIEW_ERASE_CURR_LAYER:
        Erase_Current_DrawLayer( true );
        ClearMsgPanel();
        break;

    case ID_NO_TOOL_SELECTED:
        SetNoToolSelected();
        break;

    case ID_MENU_ZOOM_SELECTION:
    case ID_ZOOM_SELECTION:
        // This tool is located on the main toolbar: switch it on or off on click
        if( GetToolId() != ID_ZOOM_SELECTION )
            SetToolID( ID_ZOOM_SELECTION, wxCURSOR_MAGNIFIER, _( "Zoom to selection" ) );
        else
            SetNoToolSelected();
        break;

    case ID_TB_MEASUREMENT_TOOL:
        SetToolID( id, wxCURSOR_DEFAULT, _( "Unsupported tool in this canvas" ) );
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
        if( !IsGalCanvasActive() )
        {
            INSTALL_UNBUFFERED_DC( dc, m_canvas );
            GetScreen()->m_BlockLocate.SetCommand( BLOCK_MOVE );
            m_canvas->SetAutoPanRequest( false );
            HandleBlockPlace( &dc );
        }
        break;

    case ID_POPUP_ZOOM_BLOCK:
        if( !IsGalCanvasActive() )
        {
            INSTALL_UNBUFFERED_DC( dc, m_canvas );
            GetScreen()->m_BlockLocate.SetCommand( BLOCK_ZOOM );
            GetScreen()->m_BlockLocate.SetMessageBlock( this );
            HandleBlockEnd( &dc );
        }
        break;

    case ID_HIGHLIGHT_CMP_ITEMS:
        if( m_SelComponentBox->SetStringSelection( currItem->GetNetAttributes().m_Cmpref ) )
            m_canvas->Refresh();
        break;

    case ID_HIGHLIGHT_NET_ITEMS:
        if( m_SelNetnameBox->SetStringSelection( currItem->GetNetAttributes().m_Netname ) )
            m_canvas->Refresh();
        break;

    case ID_HIGHLIGHT_APER_ATTRIBUTE_ITEMS:
        {
        D_CODE* apertDescr = currItem->GetDcodeDescr();
        if( m_SelAperAttributesBox->SetStringSelection( apertDescr->m_AperFunction ) )
            m_canvas->Refresh();
        }
        break;

    case ID_HIGHLIGHT_REMOVE_ALL:
        m_SelComponentBox->SetSelection( 0 );
        m_SelNetnameBox->SetSelection( 0 );
        m_SelAperAttributesBox->SetSelection( 0 );

        if( GetGbrImage( GetActiveLayer() ) )
            GetGbrImage( GetActiveLayer() )->m_Selected_Tool = 0;

        m_canvas->Refresh();
        break;

    default:
        wxFAIL_MSG( wxT( "GERBVIEW_FRAME::Process_Special_Functions error" ) );
        break;
    }
}


void GERBVIEW_FRAME::OnSelectHighlightChoice( wxCommandEvent& event )
{
    if( IsGalCanvasActive() )
    {
        auto settings = static_cast<KIGFX::GERBVIEW_PAINTER*>( GetGalCanvas()->GetView()->GetPainter() )->GetSettings();

        switch( event.GetId() )
        {
        case ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE:
            settings->m_componentHighlightString = m_SelComponentBox->GetStringSelection();
            break;

        case ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE:
            settings->m_netHighlightString = m_SelNetnameBox->GetStringSelection();
            break;

        case ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE:
            settings->m_attributeHighlightString = m_SelAperAttributesBox->GetStringSelection();
            break;

        }

        GetGalCanvas()->GetView()->UpdateAllItems( KIGFX::COLOR );
        GetGalCanvas()->Refresh();
    }
    else
        m_canvas->Refresh();
}


void GERBVIEW_FRAME::OnSelectActiveDCode( wxCommandEvent& event )
{
    GERBER_FILE_IMAGE* gerber_image = GetGbrImage( GetActiveLayer() );

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
    SetActiveLayer( event.GetSelection(), true );
}


void GERBVIEW_FRAME::OnShowGerberSourceFile( wxCommandEvent& event )
{
    int     layer = GetActiveLayer();
    GERBER_FILE_IMAGE* gerber_layer = GetGbrImage( layer );

    if( gerber_layer )
    {
        wxString editorname = Pgm().GetEditorName();

        if( !editorname.IsEmpty() )
        {
            wxFileName fn( gerber_layer->m_FileName );

            // Call the editor only if the Gerber/drill source file is available.
            // This is not always the case, because it can be a temporary file
            // if it comes from a zip archive.
            if( !fn.FileExists() )
            {
                wxString msg;
                msg.Printf( _( "Source file \"%s\" is not available" ),
                            GetChars( fn.GetFullPath() ) );
                wxMessageBox( msg );
            }
            else
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


void GERBVIEW_FRAME::OnToggleShowLayerManager( wxCommandEvent& aEvent )
{
    m_show_layer_manager_tools = !m_show_layer_manager_tools;

    // show/hide auxiliary Vertical layers and visibility manager toolbar
    m_auimgr.GetPane( "LayersManager" ).Show( m_show_layer_manager_tools );
    m_auimgr.Update();
}


void GERBVIEW_FRAME::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int     id = event.GetId();
    bool    needs_refresh = false;

    GBR_DISPLAY_OPTIONS options = m_DisplayOptions;

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_DCODES:
        SetElementVisibility( LAYER_DCODES, !IsElementVisible( LAYER_DCODES ) );
        m_canvas->Refresh( true );
        break;

    case ID_TB_OPTIONS_SHOW_NEGATIVE_ITEMS:
        SetElementVisibility( LAYER_NEGATIVE_OBJECTS, !IsElementVisible( LAYER_NEGATIVE_OBJECTS ) );
        m_canvas->Refresh( true );
        break;

    case ID_TB_OPTIONS_DIFF_MODE:
        options.m_DiffMode = !options.m_DiffMode;
        needs_refresh = true;
        break;

    case ID_TB_OPTIONS_HIGH_CONTRAST_MODE:
        options.m_HighContrastMode = !options.m_HighContrastMode;
        needs_refresh = true;
        break;

    // collect GAL-only tools here:
    case ID_TB_MEASUREMENT_TOOL:
        SetToolID( id, wxCURSOR_DEFAULT, _( "Unsupported tool in this canvas" ) );
        break;

    default:
        wxMessageBox( wxT( "GERBVIEW_FRAME::OnSelectOptionToolbar error" ) );
        break;
    }

    if( needs_refresh )
        UpdateDisplayOptions( options );
}


void GERBVIEW_FRAME::OnTogglePolygonDrawMode( wxCommandEvent& aEvent )
{
    GBR_DISPLAY_OPTIONS options = m_DisplayOptions;
    options.m_DisplayPolygonsFill = !m_DisplayOptions.m_DisplayPolygonsFill;

    UpdateDisplayOptions( options );
}


void GERBVIEW_FRAME::OnToggleLineDrawMode( wxCommandEvent& aEvent )
{
    GBR_DISPLAY_OPTIONS options = m_DisplayOptions;
    options.m_DisplayLinesFill = !m_DisplayOptions.m_DisplayLinesFill;

    UpdateDisplayOptions( options );
}


void GERBVIEW_FRAME::OnToggleFlashItemDrawMode( wxCommandEvent& aEvent )
{
    GBR_DISPLAY_OPTIONS options = m_DisplayOptions;
    options.m_DisplayFlashedItemsFill = !m_DisplayOptions.m_DisplayFlashedItemsFill;

    UpdateDisplayOptions( options );
}


void GERBVIEW_FRAME::OnUpdateSelectTool( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( GetToolId() == aEvent.GetId() );
}


void GERBVIEW_FRAME::OnSwitchCanvas( wxCommandEvent& aEvent )
{
    switch( aEvent.GetId() )
    {
    case ID_MENU_CANVAS_LEGACY:
        SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE );
        break;

    case ID_MENU_CANVAS_CAIRO:
        SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO );
        break;

    case ID_MENU_CANVAS_OPENGL:
        SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL );
        break;
    }
}


void GERBVIEW_FRAME::OnUpdateSwitchCanvas( wxUpdateUIEvent& aEvent )
{
    wxMenuBar* menuBar = GetMenuBar();
    EDA_DRAW_PANEL_GAL* gal_canvas = GetGalCanvas();
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;

    if( IsGalCanvasActive() && gal_canvas )
        canvasType = gal_canvas->GetBackend();

    struct { int menuId; int galType; } menuList[] =
    {
        { ID_MENU_CANVAS_LEGACY,    EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE },
        { ID_MENU_CANVAS_OPENGL,    EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL },
        { ID_MENU_CANVAS_CAIRO,     EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO },
    };

    for( auto ii: menuList )
    {
        wxMenuItem* item = menuBar->FindItem( ii.menuId );
        if( ii.galType == canvasType )
        {
            item->Check( true );
        }
    }
}
