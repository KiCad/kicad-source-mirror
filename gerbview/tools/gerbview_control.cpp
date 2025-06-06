/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
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

#include <confirm.h>
#include <common.h>
#include <dialogs/dialog_map_gerber_layers_to_pcb.h>
#include <export_to_pcbnew.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <gerbview_painter.h>
#include <gerbview_frame.h>
#include <gerbview_settings.h>
#include <string_utils.h>
#include <excellon_image.h>
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


int GERBVIEW_CONTROL::OpenAutodetected( const TOOL_EVENT& aEvent )
{
    m_frame->LoadAutodetectedFiles( wxEmptyString );

    return 0;
}


int GERBVIEW_CONTROL::OpenGerber( const TOOL_EVENT& aEvent )
{
    m_frame->LoadGerberFiles( wxEmptyString );

    return 0;
}


int GERBVIEW_CONTROL::OpenDrillFile( const TOOL_EVENT& aEvent )
{
    m_frame->LoadExcellonFiles( wxEmptyString );

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
    for( int ii = 0; ii < (int) images->ImagesMaxCount(); ++ii )
    {
        if( images->GetGbrImage( ii ) )
            layercount++;
    }

    if( layercount == 0 )
    {
        DisplayInfoMessage( m_frame, _( "None of the Gerber layers contain any data" ) );
        return 0;
    }

    wxString fileDialogName( NAMELESS_PROJECT + wxT( "." ) + FILEEXT::KiCadPcbFileExtension );
    wxString     path = m_frame->GetMruPath();

    wxFileDialog filedlg( m_frame, _( "Export as KiCad Board File" ), path, fileDialogName,
                          FILEEXT::PcbFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( filedlg.ShowModal() == wxID_CANCEL )
        return 0;

    wxFileName fileName = EnsureFileExtension( filedlg.GetPath(), FILEEXT::KiCadPcbFileExtension );

    DIALOG_MAP_GERBER_LAYERS_TO_PCB layerdlg( m_frame );

    if( layerdlg.ShowModal() != wxID_OK )
        return 0;

    m_frame->SetMruPath( fileName.GetPath() );

    GBR_TO_PCB_EXPORTER gbr_exporter( m_frame, fileName.GetFullPath() );

    gbr_exporter.ExportPcb( layerdlg.GetLayersLookUpTable(), layerdlg.GetCopperLayersCount() );

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
    GERBVIEW_SETTINGS* cfg = m_frame->gvconfig();
    KIGFX::VIEW*       view = m_frame->GetCanvas()->GetView();

    if( aEvent.IsAction( &GERBVIEW_ACTIONS::linesDisplayOutlines ) )
    {
        cfg->m_Display.m_DisplayLinesFill = !cfg->m_Display.m_DisplayLinesFill;

        view->UpdateAllItemsConditionally( KIGFX::REPAINT,
                []( KIGFX::VIEW_ITEM* aItem )
                {
                    GERBER_DRAW_ITEM* item = static_cast<GERBER_DRAW_ITEM*>( aItem );

                    switch( item->m_ShapeType )
                    {
                    case GBR_CIRCLE:
                    case GBR_ARC:
                    case GBR_SEGMENT:
                        return true;

                    default:
                        return false;
                    }
                } );
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::flashedDisplayOutlines ) )
    {
        cfg->m_Display.m_DisplayFlashedItemsFill = !cfg->m_Display.m_DisplayFlashedItemsFill;

        view->UpdateAllItemsConditionally( KIGFX::REPAINT,
                []( KIGFX::VIEW_ITEM* aItem )
                {
                    GERBER_DRAW_ITEM* item = static_cast<GERBER_DRAW_ITEM*>( aItem );

                    switch( item->m_ShapeType )
                    {
                    case GBR_SPOT_CIRCLE:
                    case GBR_SPOT_RECT:
                    case GBR_SPOT_OVAL:
                    case GBR_SPOT_POLY:
                    case GBR_SPOT_MACRO:
                        return true;

                    default:
                        return false;
                    }
                } );
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::polygonsDisplayOutlines ) )
    {
        cfg->m_Display.m_DisplayPolygonsFill = !cfg->m_Display.m_DisplayPolygonsFill;

        view->UpdateAllItemsConditionally( KIGFX::REPAINT,
                []( KIGFX::VIEW_ITEM* aItem )
                {
                    GERBER_DRAW_ITEM* item = static_cast<GERBER_DRAW_ITEM*>( aItem );

                    return ( item->m_ShapeType == GBR_POLYGON );
                } );
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::negativeObjectDisplay ) )
    {
        m_frame->SetElementVisibility( LAYER_NEGATIVE_OBJECTS, !cfg->m_Appearance.show_negative_objects );
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::dcodeDisplay ) )
    {
        m_frame->SetElementVisibility( LAYER_DCODES, !cfg->m_Appearance.show_dcodes );
    }
    else if( aEvent.IsAction( &ACTIONS::highContrastMode )
             || aEvent.IsAction( &ACTIONS::highContrastModeCycle ) )
    {
        cfg->m_Display.m_HighContrastMode = !cfg->m_Display.m_HighContrastMode;
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::toggleForceOpacityMode ) )
    {
        cfg->m_Display.m_ForceOpacityMode = !cfg->m_Display.m_ForceOpacityMode;

        if( cfg->m_Display.m_ForceOpacityMode && cfg->m_Display.m_XORMode )
            cfg->m_Display.m_XORMode = false;

        m_frame->UpdateXORLayers();
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::toggleXORMode ) )
    {
        cfg->m_Display.m_XORMode = !cfg->m_Display.m_XORMode;

        if( cfg->m_Display.m_XORMode && cfg->m_Display.m_ForceOpacityMode )
            cfg->m_Display.m_ForceOpacityMode = false;

        m_frame->UpdateXORLayers();
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::flipGerberView ) )
    {
        cfg->m_Display.m_FlipGerberView = !cfg->m_Display.m_FlipGerberView;
        view->SetMirror( cfg->m_Display.m_FlipGerberView, false );
    }

    m_frame->ApplyDisplaySettingsToGAL();

    view->UpdateAllItems( KIGFX::COLOR );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int GERBVIEW_CONTROL::LayerNext( const TOOL_EVENT& aEvent )
{
    int layer = m_frame->GetActiveLayer();

    if( layer < ( (int) GERBER_FILE_IMAGE_LIST::GetImagesList().GetLoadedImageCount() - 1 ) )
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


int GERBVIEW_CONTROL::MoveLayerUp( const TOOL_EVENT& aEvent )
{
    int layer = m_frame->GetActiveLayer();

    if( layer > 0 )
    {
        m_frame->RemapLayers(
                GERBER_FILE_IMAGE_LIST::GetImagesList().SwapImages( layer, layer - 1 ) );
        m_frame->SetActiveLayer( layer - 1 );
    }

    return 0;
}


int GERBVIEW_CONTROL::MoveLayerDown( const TOOL_EVENT& aEvent )
{
    int                     layer = m_frame->GetActiveLayer();
    GERBER_FILE_IMAGE_LIST& list = GERBER_FILE_IMAGE_LIST::GetImagesList();

    if( layer < ( (int) list.GetLoadedImageCount() - 1 ) )
    {
        m_frame->RemapLayers( list.SwapImages( layer, layer + 1 ) );
        m_frame->SetActiveLayer( layer + 1 );
    }

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
    m_toolMgr->RunAction( ACTIONS::zoomFitScreen );
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

        std::vector<MSG_PANEL_ITEM> msgItems;
        item->GetMsgPanelInfo( m_frame, msgItems );
        m_frame->SetMsgPanel( msgItems );
    }
    else
    {
        m_frame->EraseMsgBox();
    }

    return 0;
}


int GERBVIEW_CONTROL::LoadZipfile( const TOOL_EVENT& aEvent )
{
    m_frame->LoadZipArchiveFile( *aEvent.Parameter<wxString*>() );
    canvas()->Refresh();

    return 0;
}


int GERBVIEW_CONTROL::LoadGerbFiles( const TOOL_EVENT& aEvent )
{
    // The event parameter is a string containing names of dropped files.
    // Each file name has been enclosed with "", so file names with space character are allowed.
    wxString files = *aEvent.Parameter<wxString*>();
    // ie : files = "file1""another file""file3"...

    std::vector<wxString> aFileNameList;

    // Isolate each file name, deletting ""
    files = files.AfterFirst( '"' );

    // Gerber files are enclosed with "".
    // Load files names in array.
    while( !files.empty() )
    {
        wxString fileName = files.BeforeFirst( '"' );
        // Check if file exists. If not, keep on and ignore fileName
        if( wxFileName( fileName ).Exists() )
            aFileNameList.push_back( fileName );
        files = files.AfterFirst( '"' );
        files = files.AfterFirst( '"' );
    }

    if( !aFileNameList.empty() )
        m_frame->OpenProjectFiles( aFileNameList, KICTL_CREATE );

    return 0;
}


void GERBVIEW_CONTROL::setTransitions()
{
    Go( &GERBVIEW_CONTROL::OpenAutodetected,   GERBVIEW_ACTIONS::openAutodetected.MakeEvent() );
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
    Go( &GERBVIEW_CONTROL::MoveLayerUp,        GERBVIEW_ACTIONS::moveLayerUp.MakeEvent() );
    Go( &GERBVIEW_CONTROL::MoveLayerDown,      GERBVIEW_ACTIONS::moveLayerDown.MakeEvent() );
    Go( &GERBVIEW_CONTROL::ClearLayer,         GERBVIEW_ACTIONS::clearLayer.MakeEvent() );
    Go( &GERBVIEW_CONTROL::ClearAllLayers,     GERBVIEW_ACTIONS::clearAllLayers.MakeEvent() );
    Go( &GERBVIEW_CONTROL::ReloadAllLayers,    GERBVIEW_ACTIONS::reloadAllLayers.MakeEvent() );

    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::linesDisplayOutlines.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::flashedDisplayOutlines.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::polygonsDisplayOutlines.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::negativeObjectDisplay.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::dcodeDisplay.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     ACTIONS::highContrastMode.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     ACTIONS::highContrastModeCycle.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::toggleForceOpacityMode.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::toggleXORMode.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::flipGerberView.MakeEvent() );

    Go( &GERBVIEW_CONTROL::UpdateMessagePanel, EVENTS::SelectedEvent );
    Go( &GERBVIEW_CONTROL::UpdateMessagePanel, EVENTS::UnselectedEvent );
    Go( &GERBVIEW_CONTROL::UpdateMessagePanel, EVENTS::ClearedEvent );
    Go( &GERBVIEW_CONTROL::UpdateMessagePanel, ACTIONS::updateUnits.MakeEvent() );

    Go( &GERBVIEW_CONTROL::LoadZipfile,        GERBVIEW_ACTIONS::loadZipFile.MakeEvent() );
    Go( &GERBVIEW_CONTROL::LoadGerbFiles,      GERBVIEW_ACTIONS::loadGerbFiles.MakeEvent() );
}
