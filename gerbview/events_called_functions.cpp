/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pgm_base.h>
#include <gestfich.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <dialog_helpers.h>
#include <DCodeSelectionbox.h>
#include <gerbview_layer_widget.h>
#include <gerbview_draw_panel_gal.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>
#include <tool/selection.h>
#include <tools/gerbview_selection_tool.h>
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

    // menu Postprocess
    EVT_MENU( ID_GERBVIEW_SHOW_LIST_DCODES, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_MENU( ID_GERBVIEW_SHOW_SOURCE, GERBVIEW_FRAME::OnShowGerberSourceFile )

    // menu Miscellaneous
    EVT_MENU( ID_GERBVIEW_ERASE_CURR_LAYER, GERBVIEW_FRAME::Process_Special_Functions )

    EVT_COMBOBOX( ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER, GERBVIEW_FRAME::OnSelectActiveLayer )

    EVT_SELECT_DCODE( ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE, GERBVIEW_FRAME::OnSelectActiveDCode )

    // Option toolbar
    EVT_TOOL( ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
              GERBVIEW_FRAME::OnToggleShowLayerManager )
    EVT_TOOL_RANGE( ID_TB_OPTIONS_SHOW_GBR_MODE_0, ID_TB_OPTIONS_SHOW_GBR_MODE_2,
                    GERBVIEW_FRAME::OnSelectDisplayMode )

    // Auxiliary horizontal toolbar
    EVT_CHOICE( ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE, GERBVIEW_FRAME::OnSelectHighlightChoice )
    EVT_CHOICE( ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE, GERBVIEW_FRAME::OnSelectHighlightChoice )
    EVT_CHOICE( ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE,
                GERBVIEW_FRAME::OnSelectHighlightChoice )
    EVT_CHOICE( ID_ON_ZOOM_SELECT, GERBVIEW_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, GERBVIEW_FRAME::OnSelectGrid )

    // Right click context menu
    EVT_MENU( ID_HIGHLIGHT_CMP_ITEMS, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_MENU( ID_HIGHLIGHT_NET_ITEMS, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_MENU( ID_HIGHLIGHT_APER_ATTRIBUTE_ITEMS, GERBVIEW_FRAME::Process_Special_Functions )
    EVT_MENU( ID_HIGHLIGHT_REMOVE_ALL, GERBVIEW_FRAME::Process_Special_Functions )

    EVT_UPDATE_UI( ID_ON_GRID_SELECT, GERBVIEW_FRAME::OnUpdateSelectGrid )
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
    int                      id = event.GetId();
    GERBVIEW_SELECTION_TOOL* selTool = GetToolManager()->GetTool<GERBVIEW_SELECTION_TOOL>();
    GERBVIEW_SELECTION&      selection = selTool->GetSelection();
    GERBER_DRAW_ITEM*        currItem = (GERBER_DRAW_ITEM*) selection.Front();

    switch( id )
    {
    case ID_GERBVIEW_ERASE_CURR_LAYER:
        Erase_Current_DrawLayer( true );
        ClearMsgPanel();
        break;

    case ID_GERBVIEW_SHOW_LIST_DCODES:
        Liste_D_Codes();
        break;

    case ID_HIGHLIGHT_CMP_ITEMS:
        m_SelComponentBox->SetStringSelection( currItem->GetNetAttributes().m_Cmpref );
        break;

    case ID_HIGHLIGHT_NET_ITEMS:
        m_SelNetnameBox->SetStringSelection( UnescapeString( currItem->GetNetAttributes().m_Netname ) );
        break;

    case ID_HIGHLIGHT_APER_ATTRIBUTE_ITEMS:
        m_SelAperAttributesBox->SetStringSelection( currItem->GetDcodeDescr()->m_AperFunction );
        break;

    case ID_HIGHLIGHT_REMOVE_ALL:
        m_SelComponentBox->SetSelection( 0 );
        m_SelNetnameBox->SetSelection( 0 );
        m_SelAperAttributesBox->SetSelection( 0 );

        if( GetGbrImage( GetActiveLayer() ) )
            GetGbrImage( GetActiveLayer() )->m_Selected_Tool = 0;
        break;

    default:
        wxFAIL_MSG( wxT( "GERBVIEW_FRAME::Process_Special_Functions error" ) );
        break;
    }

    GetCanvas()->Refresh();
}


void GERBVIEW_FRAME::OnSelectHighlightChoice( wxCommandEvent& event )
{
    auto settings = static_cast<KIGFX::GERBVIEW_PAINTER*>( GetCanvas()->GetView()->GetPainter() )->GetSettings();

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

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::COLOR );
    GetCanvas()->Refresh();
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
            GetCanvas()->Refresh();
        }
    }
}


void GERBVIEW_FRAME::OnSelectActiveLayer( wxCommandEvent& event )
{
    SetActiveLayer( event.GetSelection(), true );

    // Rebuild the DCode list in toolbar (but not the Layer Box) after change
    syncLayerBox( false );
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
    switch( event.GetId() )
    {
    case ID_TB_OPTIONS_SHOW_GBR_MODE_0: SetDisplayMode( 0 ); break;
    case ID_TB_OPTIONS_SHOW_GBR_MODE_1: SetDisplayMode( 1 ); break;
    case ID_TB_OPTIONS_SHOW_GBR_MODE_2: SetDisplayMode( 2 ); break;
    }

    GetCanvas()->Refresh();
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
