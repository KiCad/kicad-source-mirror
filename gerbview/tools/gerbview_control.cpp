/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <dialogs/dialog_layers_select_to_pcb.h>
#include <export_to_pcbnew.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <gerbview_painter.h>
#include <gerbview_frame.h>
#include <string_utils.h>
#include <excellon_image.h>
#include <menus_helpers.h>
#include <tool/tool_manager.h>
#include <project.h>
#include <view/view.h>
#include <wildcards_and_files_ext.h>
#include <wx/filedlg.h>

#include "gerbview_actions.h"
#include "gerbview_control.h"
#include "gerbview_selection_tool.h"


GERBVIEW_CONTROL::GERBVIEW_CONTROL() : TOOL_INTERACTIVE( "gerbview.Control" ), m_frame( nullptr )
{
}


void GERBVIEW_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<GERBVIEW_FRAME>();
}


int GERBVIEW_CONTROL::OpenGerber( const TOOL_EVENT& aEvent )
{
    m_frame->LoadGerberFiles( wxEmptyString );
    // loadListOfGerberAndDrillFiles() refreshes the canvas

    return 0;
}


int GERBVIEW_CONTROL::OpenDrillFile( const TOOL_EVENT& aEvent )
{
    m_frame->LoadExcellonFiles( wxEmptyString );
    canvas()->Refresh();

    return 0;
}


int GERBVIEW_CONTROL::OpenJobFile( const TOOL_EVENT& aEvent )
{
    m_frame->LoadGerberJobFile( wxEmptyString );
    canvas()->Refresh();

    return 0;
}


int GERBVIEW_CONTROL::OpenZipFile( const TOOL_EVENT& aEvent )
{
    m_frame->LoadZipArchiveFile( wxEmptyString );
    canvas()->Refresh();

    return 0;
}


int GERBVIEW_CONTROL::ToggleLayerManager( const TOOL_EVENT& aEvent )
{
    m_frame->ToggleLayerManager();

    return 0;
}


int GERBVIEW_CONTROL::ExportToPcbnew( const TOOL_EVENT& aEvent )
{
    int layercount = 0;

    GERBER_FILE_IMAGE_LIST* images = m_frame->GetGerberLayout()->GetImagesList();

    // Count the Gerber layers which are actually currently used
    for( LAYER_NUM ii = 0; ii < (LAYER_NUM)images->ImagesMaxCount(); ++ii )
    {
        if( images->GetGbrImage( ii ) )
            layercount++;
    }

    if( layercount == 0 )
    {
        DisplayInfoMessage( m_frame, _( "None of the Gerber layers contain any data" ) );
        return 0;
    }

    wxString     fileDialogName( NAMELESS_PROJECT + wxT( "." ) + KiCadPcbFileExtension );
    wxString     path = m_frame->GetMruPath();

    wxFileDialog filedlg( m_frame, _( "Board File Name" ), path, fileDialogName, PcbFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( filedlg.ShowModal() == wxID_CANCEL )
        return 0;

    wxFileName fileName = filedlg.GetPath();

    /* Install a dialog frame to choose the mapping
     * between gerber layers and Pcbnew layers
     */
    LAYERS_MAP_DIALOG* layerdlg = new LAYERS_MAP_DIALOG( m_frame );
    int ok = layerdlg->ShowModal();
    layerdlg->Destroy();

    if( ok != wxID_OK )
        return 0;

    // If no extension was entered, then force the extension to be a KiCad PCB file
    if( !fileName.HasExt() )
        fileName.SetExt( KiCadPcbFileExtension );

    m_frame->SetMruPath( fileName.GetPath() );

    GBR_TO_PCB_EXPORTER gbr_exporter( m_frame, fileName.GetFullPath() );

    gbr_exporter.ExportPcb( layerdlg->GetLayersLookUpTable(), layerdlg->GetCopperLayersCount() );

    return 0;
}


int GERBVIEW_CONTROL::HighlightControl( const TOOL_EVENT& aEvent )
{
    auto settings = static_cast<KIGFX::GERBVIEW_PAINTER*>( getView()->GetPainter() )->GetSettings();
    const auto& selection = m_toolMgr->GetTool<GERBVIEW_SELECTION_TOOL>()->GetSelection();
    GERBER_DRAW_ITEM* item = nullptr;

    if( selection.Size() == 1 )
    {
        item = static_cast<GERBER_DRAW_ITEM*>( selection[0] );
    }

    if( aEvent.IsAction( &GERBVIEW_ACTIONS::highlightClear ) )
    {
        m_frame->m_SelComponentBox->SetSelection( 0 );
        m_frame->m_SelNetnameBox->SetSelection( 0 );
        m_frame->m_SelAperAttributesBox->SetSelection( 0 );

        settings->ClearHighlightSelections();

        GERBER_FILE_IMAGE* gerber = m_frame->GetGbrImage( m_frame->GetActiveLayer() );

        if( gerber )
            gerber->m_Selected_Tool = settings->m_dcodeHighlightValue;
    }
    else if( item && aEvent.IsAction( &GERBVIEW_ACTIONS::highlightNet ) )
    {
        wxString net_name = item->GetNetAttributes().m_Netname;
        settings->m_netHighlightString = net_name;
        m_frame->m_SelNetnameBox->SetStringSelection( UnescapeString( net_name ) );
    }
    else if( item && aEvent.IsAction( &GERBVIEW_ACTIONS::highlightComponent ) )
    {
        wxString net_attr = item->GetNetAttributes().m_Cmpref;
        settings->m_componentHighlightString = net_attr;
        m_frame->m_SelComponentBox->SetStringSelection( net_attr );
    }
    else if( item && aEvent.IsAction( &GERBVIEW_ACTIONS::highlightAttribute ) )
    {
        D_CODE* apertDescr = item->GetDcodeDescr();

        if( apertDescr )
        {
            wxString ap_name = apertDescr->m_AperFunction;
            settings->m_attributeHighlightString = ap_name;
            m_frame->m_SelAperAttributesBox->SetStringSelection( ap_name );
        }
    }
    else if( item && aEvent.IsAction( &GERBVIEW_ACTIONS::highlightDCode ) )
    {
        D_CODE* apertDescr = item->GetDcodeDescr();

        if( apertDescr )
        {
            int dcodeSelected = -1;
            GERBER_FILE_IMAGE* gerber = m_frame->GetGbrImage( m_frame->GetActiveLayer() );

            if( gerber )
                dcodeSelected = apertDescr->m_Num_Dcode;

            if( dcodeSelected > 0 )
            {
                settings->m_dcodeHighlightValue = dcodeSelected;
                gerber->m_Selected_Tool = dcodeSelected;
                m_frame->syncLayerBox( false );
            }
        }
    }

    canvas()->GetView()->UpdateAllItems( KIGFX::COLOR );
    canvas()->Refresh();

    return 0;
}


int GERBVIEW_CONTROL::DisplayControl( const TOOL_EVENT& aEvent )
{
    bool state;
    bool needs_refresh = false;
    auto options = m_frame->GetDisplayOptions();

    if( aEvent.IsAction( &GERBVIEW_ACTIONS::linesDisplayOutlines ) )
    {
        options.m_DisplayLinesFill = !options.m_DisplayLinesFill;
        needs_refresh = true;
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::flashedDisplayOutlines ) )
    {
        options.m_DisplayFlashedItemsFill = !options.m_DisplayFlashedItemsFill;
        needs_refresh = true;
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::polygonsDisplayOutlines ) )
    {
        options.m_DisplayPolygonsFill = !options.m_DisplayPolygonsFill;
        needs_refresh = true;
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::negativeObjectDisplay ) )
    {
        state = !m_frame->IsElementVisible( LAYER_NEGATIVE_OBJECTS );
        m_frame->SetElementVisibility( LAYER_NEGATIVE_OBJECTS, state );
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::dcodeDisplay ) )
    {
        state = !m_frame->IsElementVisible( LAYER_DCODES );
        m_frame->SetElementVisibility( LAYER_DCODES, state );
    }
    else if( aEvent.IsAction( &ACTIONS::highContrastMode ) )
    {
        options.m_HighContrastMode = !options.m_HighContrastMode;
        needs_refresh = true;
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::toggleDiffMode ) )
    {
        options.m_DiffMode = !options.m_DiffMode;
        needs_refresh = true;
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::flipGerberView ) )
    {
        options.m_FlipGerberView = !options.m_FlipGerberView;

        KIGFX::VIEW* view = canvas()->GetView();
        view->SetMirror( options.m_FlipGerberView, false );
        needs_refresh = true;
    }

    if( needs_refresh )
        m_frame->UpdateDisplayOptions( options );

    return 0;
}


int GERBVIEW_CONTROL::LayerNext( const TOOL_EVENT& aEvent )
{
    int layer = m_frame->GetActiveLayer();

    if( layer < GERBER_DRAWLAYERS_COUNT - 1 )
        m_frame->SetActiveLayer( layer + 1, true );

    return 0;
}


int GERBVIEW_CONTROL::LayerPrev( const TOOL_EVENT& aEvent )
{
    int layer = m_frame->GetActiveLayer();

    if( layer > 0 )
        m_frame->SetActiveLayer( layer - 1, true );

    return 0;
}


int GERBVIEW_CONTROL::ClearLayer( const TOOL_EVENT& aEvent )
{
    m_frame->Erase_Current_DrawLayer( true );
    m_frame->ClearMsgPanel();

    return 0;
}


int GERBVIEW_CONTROL::ClearAllLayers( const TOOL_EVENT& aEvent )
{
    m_frame->Clear_DrawLayers( false );
    m_toolMgr->RunAction( ACTIONS::zoomFitScreen, true );
    canvas()->Refresh();
    m_frame->ClearMsgPanel();

    // Clear pending highlight selections, now outdated
    KIGFX::GERBVIEW_RENDER_SETTINGS* settings =
            static_cast<KIGFX::GERBVIEW_PAINTER*>( getView()->GetPainter() )->GetSettings();
    settings->ClearHighlightSelections();

    return 0;
}


int GERBVIEW_CONTROL::ReloadAllLayers( const TOOL_EVENT& aEvent )
{
    // Store filenames
    wxArrayString           listOfGerberFiles;
    std::vector<int>        fileType;
    GERBER_FILE_IMAGE_LIST* list = m_frame->GetImagesList();

    for( unsigned i = 0; i < list->ImagesMaxCount(); i++ )
    {
        if( list->GetGbrImage( i ) == nullptr )
            continue;

        if( !list->GetGbrImage( i )->m_InUse )
            continue;

        EXCELLON_IMAGE* drill_file = dynamic_cast<EXCELLON_IMAGE*>( list->GetGbrImage( i ) );

        if( drill_file )
            fileType.push_back( 1 );
        else
            fileType.push_back( 0 );

        listOfGerberFiles.Add( list->GetGbrImage( i )->m_FileName );
    }

    // Clear all layers
    m_frame->Clear_DrawLayers( false );
    m_frame->ClearMsgPanel();

    // Load the layers from stored paths
    wxBusyCursor wait;
    m_frame->LoadListOfGerberAndDrillFiles( wxEmptyString, listOfGerberFiles, &fileType );

    return 0;
}


int GERBVIEW_CONTROL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    GERBVIEW_SELECTION_TOOL* selTool = m_toolMgr->GetTool<GERBVIEW_SELECTION_TOOL>();
    GERBVIEW_SELECTION&      selection = selTool->GetSelection();

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = (EDA_ITEM*) selection.Front();

        MSG_PANEL_ITEMS msgItems;
        item->GetMsgPanelInfo( m_frame, msgItems );
        m_frame->SetMsgPanel( msgItems );
    }
    else
    {
        m_frame->EraseMsgBox();
    }

    return 0;
}


void GERBVIEW_CONTROL::setTransitions()
{
    Go( &GERBVIEW_CONTROL::OpenGerber,         GERBVIEW_ACTIONS::openGerber.MakeEvent() );
    Go( &GERBVIEW_CONTROL::OpenDrillFile,      GERBVIEW_ACTIONS::openDrillFile.MakeEvent() );
    Go( &GERBVIEW_CONTROL::OpenJobFile,        GERBVIEW_ACTIONS::openJobFile.MakeEvent() );
    Go( &GERBVIEW_CONTROL::OpenZipFile,        GERBVIEW_ACTIONS::openZipFile.MakeEvent() );
    Go( &GERBVIEW_CONTROL::ToggleLayerManager, GERBVIEW_ACTIONS::toggleLayerManager.MakeEvent() );
    Go( &GERBVIEW_CONTROL::ExportToPcbnew,     GERBVIEW_ACTIONS::exportToPcbnew.MakeEvent() );
    Go( &GERBVIEW_CONTROL::Print,              ACTIONS::print.MakeEvent() );

    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightClear.MakeEvent() );
    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightNet.MakeEvent() );
    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightComponent.MakeEvent() );
    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightAttribute.MakeEvent() );
    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightDCode.MakeEvent() );

    Go( &GERBVIEW_CONTROL::LayerNext,          GERBVIEW_ACTIONS::layerNext.MakeEvent() );
    Go( &GERBVIEW_CONTROL::LayerPrev,          GERBVIEW_ACTIONS::layerPrev.MakeEvent() );
    Go( &GERBVIEW_CONTROL::ClearLayer,         GERBVIEW_ACTIONS::clearLayer.MakeEvent() );
    Go( &GERBVIEW_CONTROL::ClearAllLayers,     GERBVIEW_ACTIONS::clearAllLayers.MakeEvent() );
    Go( &GERBVIEW_CONTROL::ReloadAllLayers,    GERBVIEW_ACTIONS::reloadAllLayers.MakeEvent() );

    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::linesDisplayOutlines.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::flashedDisplayOutlines.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::polygonsDisplayOutlines.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::negativeObjectDisplay.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::dcodeDisplay.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     ACTIONS::highContrastMode.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::toggleDiffMode.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::flipGerberView.MakeEvent() );

    Go( &GERBVIEW_CONTROL::UpdateMessagePanel, EVENTS::SelectedEvent );
    Go( &GERBVIEW_CONTROL::UpdateMessagePanel, EVENTS::UnselectedEvent );
    Go( &GERBVIEW_CONTROL::UpdateMessagePanel, EVENTS::ClearedEvent );
}
