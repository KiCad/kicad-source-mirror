/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiface_base.h>
#include <base_units.h>
#include <pgm_base.h>
#include <bitmaps.h>
#include <wildcards_and_files_ext.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <excellon_image.h>
#include <gerbview_draw_panel_gal.h>
#include <gerbview_settings.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tool/action_toolbar.h>
#include <tool/tool_dispatcher.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/editor_conditions.h>
#include <tool/zoom_tool.h>
#include <tools/gerbview_actions.h>
#include <tools/gerbview_inspection_tool.h>
#include <tools/gerbview_selection.h>
#include <tools/gerbview_selection_tool.h>
#include <tools/gerbview_control.h>
#include <trigo.h>
#include <view/view.h>
#include <base_screen.h>
#include <gerbview_painter.h>
#include <wx/wupdlock.h>

#include "widgets/gbr_layer_box_selector.h"
#include "widgets/gerbview_layer_widget.h"
#include "widgets/dcode_selection_box.h"
#include <zoom_defines.h>


GERBVIEW_FRAME::GERBVIEW_FRAME( KIWAY* aKiway, wxWindow* aParent )
        : EDA_DRAW_FRAME( aKiway, aParent, FRAME_GERBER, wxT( "GerbView" ), wxDefaultPosition,
                        wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, GERBVIEW_FRAME_NAME,
                        gerbIUScale ),
          m_TextInfo( nullptr ),
          m_zipFileHistory( DEFAULT_FILE_HISTORY_SIZE, ID_GERBVIEW_ZIP_FILE1,
                            ID_GERBVIEW_ZIP_FILE_LIST_CLEAR, _( "Clear Recent Zip Files" ) ),
          m_drillFileHistory( DEFAULT_FILE_HISTORY_SIZE, ID_GERBVIEW_DRILL_FILE1,
                              ID_GERBVIEW_DRILL_FILE_LIST_CLEAR, _( "Clear Recent Drill Files" ) ),
          m_jobFileHistory( DEFAULT_FILE_HISTORY_SIZE, ID_GERBVIEW_JOB_FILE1,
                            ID_GERBVIEW_JOB_FILE_LIST_CLEAR, _( "Clear Recent Job Files" ) ),
          m_activeLayer( 0 )
{
    m_maximizeByDefault = true;
    m_gerberLayout = nullptr;
    m_show_layer_manager_tools = true;
    m_showBorderAndTitleBlock = false;      // true for reference drawings.
    m_SelLayerBox = nullptr;
    m_DCodeSelector = nullptr;
    m_SelComponentBox = nullptr;
    m_SelNetnameBox = nullptr;
    m_SelAperAttributesBox = nullptr;
    m_cmpText = nullptr;
    m_netText = nullptr;
    m_apertText = nullptr;
    m_dcodeText = nullptr;
    m_displayMode = 0;
    m_aboutTitle = _HKI( "KiCad Gerber Viewer" );

    SHAPE_POLY_SET dummy;   // A ugly trick to force the linker to include
                            // some methods in code and avoid link errors

    int fileHistorySize = Pgm().GetCommonSettings()->m_System.file_history_size;
    m_drillFileHistory.SetMaxFiles( fileHistorySize );
    m_zipFileHistory.SetMaxFiles( fileHistorySize );
    m_jobFileHistory.SetMaxFiles( fileHistorySize );

    auto* galCanvas = new GERBVIEW_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_frameSize,
                                                   GetGalDisplayOptions(),
                                                   EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE );

    SetCanvas( galCanvas );

    // GerbView requires draw priority for rendering negative objects
    galCanvas->GetView()->UseDrawPriority( true );

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_gerbview ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_gerbview_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_gerbview_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    // Be sure a page info is set. this default value will be overwritten later.
    PAGE_INFO pageInfo( wxT( "GERBER" ) );
    SetLayout( new GBR_LAYOUT() );
    SetPageSettings( pageInfo );

    SetVisibleLayers( LSET::AllLayersMask() );         // All draw layers visible.

    SetScreen( new BASE_SCREEN( GetPageSettings().GetSizeIU( gerbIUScale.IU_PER_MILS ) ) );

    // Create the PCB_LAYER_WIDGET *after* SetLayout():
    m_LayersManager = new GERBER_LAYER_WIDGET( this, GetCanvas() );

    // Update the minimum string length in the layer panel with the length of the last default layer
    wxString lyrName = GetImagesList()->GetDisplayName( GetImagesList()->ImagesMaxCount(),
                                                        false, true );
    m_LayersManager->SetSmallestLayerString( lyrName );

    // LoadSettings() *after* creating m_LayersManager, because LoadSettings()
    // initialize parameters in m_LayersManager
    LoadSettings( config() );

    setupTools();
    setupUIConditions();
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateOptToolbar();
    ReCreateAuxiliaryToolbar();

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer( 6 ) );
    m_auimgr.AddPane( m_auxiliaryToolBar, EDA_PANE().HToolbar().Name( "AuxToolbar" ).Top()
                      .Layer(4) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom()
                      .Layer( 6 ) );
    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" ).Left()
                      .Layer( 3 ) );
    m_auimgr.AddPane( m_LayersManager, EDA_PANE().Palette().Name( "LayersManager" ).Right()
                      .Layer( 3 ).Caption( _( "Layers Manager" ) ).PaneBorder( false )
                      .MinSize( 80, -1 ).BestSize( m_LayersManager->GetBestSize() ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    ReFillLayerWidget();                // this is near end because contents establish size
    m_auimgr.Update();

    SetActiveLayer( 0, true );
    GetToolManager()->PostAction( ACTIONS::zoomFitScreen );

    resolveCanvasType();

    SwitchCanvas( m_canvasType );

    setupUnits( config() );

    // Enable the axes to match legacy draw style
    auto& galOptions = GetGalDisplayOptions();
    galOptions.m_axesEnabled = true;
    galOptions.NotifyChanged();

    m_LayersManager->ReFill();
    m_LayersManager->ReFillRender();    // Update colors in Render after the config is read

    // Drag and drop
    // Note that all gerber files are aliased as GerberFileExtension
    m_acceptedExts.emplace( GerberFileExtension, &GERBVIEW_ACTIONS::loadGerbFiles );
    m_acceptedExts.emplace( ArchiveFileExtension, &GERBVIEW_ACTIONS::loadZipFile );
    m_acceptedExts.emplace( DrillFileExtension, &GERBVIEW_ACTIONS::loadGerbFiles );
    DragAcceptFiles( true );

    GetToolManager()->RunAction( ACTIONS::zoomFitScreen );

    // Ensure the window is on top
    Raise();

    // Register a call to update the toolbar sizes. It can't be done immediately because
    // it seems to require some sizes calculated that aren't yet (at least on GTK).
    CallAfter( [&]()
               {
                   // Ensure the controls on the toolbars all are correctly sized
                    UpdateToolbarControlSizes();
               } );
}


GERBVIEW_FRAME::~GERBVIEW_FRAME()
{
    // Ensure m_canvasType is up to date, to save it in config
    m_canvasType = GetCanvas()->GetBackend();

    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    GetCanvas()->GetView()->Clear();

    GetGerberLayout()->GetImagesList()->DeleteAllImages();
    delete m_gerberLayout;
}


void GERBVIEW_FRAME::doCloseWindow()
{
    // No more vetos
    m_isClosing = true;
    GetCanvas()->StopDrawing();
    GetCanvas()->GetView()->Clear();

    if( m_toolManager )
        m_toolManager->DeactivateTool();

    // Be sure any OpenGL event cannot be fired after frame deletion:
    GetCanvas()->SetEvtHandlerEnabled( false );

    Destroy();
}


bool GERBVIEW_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    // Ensure the frame is shown when opening the file(s), to avoid issues (crash) on GAL
    // when trying to change the view if it is not fully initialized.
    // It happens when starting GerbView with a gerber job file to load
    if( !IsShown() )
        Show();

    // The current project path is also a valid command parameter.  Check if a single path
    // rather than a file name was passed to GerbView and use it as the initial MRU path.
    if( aFileSet.size() > 0 )
    {
        wxString path = aFileSet[0];

        // For some reason wxApp appears to leave the trailing double quote on quoted
        // parameters which are required for paths with spaces.  Maybe this should be
        // pushed back into PGM_SINGLE_TOP::OnPgmInit() but that may cause other issues.
        // We can't buy a break!
        if( path.Last() ==  wxChar( '\"' ) )
            path.RemoveLast();

        if( !wxFileExists( path ) && wxDirExists( path ) )
        {
            m_mruPath = path;
            return true;
        }

        const unsigned limit = std::min( unsigned( aFileSet.size() ),
                                         unsigned( GERBER_DRAWLAYERS_COUNT ) );

        for( unsigned i = 0; i < limit; ++i )
        {
            wxString ext = wxFileName( aFileSet[i] ).GetExt().Lower();

            if( ext == ArchiveFileExtension )
                LoadZipArchiveFile( aFileSet[i] );
            else if( ext == GerberJobFileExtension )
                LoadGerberJobFile( aFileSet[i] );
            else
            {
                GERBER_ORDER_ENUM fnameLayer;
                wxString          fnameExtensionMatched;

                GERBER_FILE_IMAGE_LIST::GetGerberLayerFromFilename( aFileSet[i], fnameLayer,
                                                                    fnameExtensionMatched );

                switch( fnameLayer )
                {
                case GERBER_ORDER_ENUM::GERBER_DRILL:
                    LoadExcellonFiles( aFileSet[i] );
                    break;
                case GERBER_ORDER_ENUM::GERBER_LAYER_UNKNOWN:
                    LoadAutodetectedFiles( aFileSet[i] );
                    break;
                default:
                    LoadGerberFiles( aFileSet[i] );
                }
            }
        }
    }

    Zoom_Automatique( true );        // Zoom fit in frame

    return true;
}


GERBVIEW_SETTINGS* GERBVIEW_FRAME::gvconfig() const
{
    return dynamic_cast<GERBVIEW_SETTINGS*>( config() );
}


void GERBVIEW_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    if( aCfg->m_Window.grid.sizes.empty() )
    {
        aCfg->m_Window.grid.sizes = { wxT( "100 mil" ),
                                      wxT( "50 mil" ),
                                      wxT( "25 mil" ),
                                      wxT( "20 mil" ),
                                      wxT( "10 mil" ),
                                      wxT( "5 mil" ),
                                      wxT( "2.5 mil" ),
                                      wxT( "2 mil" ),
                                      wxT( "1 mil" ),
                                      wxT( "0.5 mil" ),
                                      wxT( "0.2 mil" ),
                                      wxT( "0.1 mil" ),
                                      wxT( "5.0 mm" ),
                                      wxT( "2.5 mm" ),
                                      wxT( "1.0 mm" ),
                                      wxT( "0.5 mm" ),
                                      wxT( "0.25 mm" ),
                                      wxT( "0.2 mm" ),
                                      wxT( "0.1 mm" ),
                                      wxT( "0.05 mm" ),
                                      wxT( "0.025 mm" ),
                                      wxT( "0.01 mm" ) };
    }

    if( aCfg->m_Window.zoom_factors.empty() )
    {
        aCfg->m_Window.zoom_factors = { ZOOM_LIST_GERBVIEW };
    }

    GERBVIEW_SETTINGS* cfg = dynamic_cast<GERBVIEW_SETTINGS*>( aCfg );
    wxCHECK( cfg, /*void*/ );

    SetElementVisibility( LAYER_GERBVIEW_DRAWINGSHEET,
                          cfg->m_Appearance.show_border_and_titleblock );
    SetElementVisibility( LAYER_GERBVIEW_PAGE_LIMITS,
                          cfg->m_Display.m_DisplayPageLimits );

    PAGE_INFO pageInfo( wxT( "GERBER" ) );
    pageInfo.SetType( cfg->m_Appearance.page_type );
    SetPageSettings( pageInfo );

    SetElementVisibility( LAYER_DCODES, cfg->m_Appearance.show_dcodes );
    SetElementVisibility( LAYER_NEGATIVE_OBJECTS, cfg->m_Appearance.show_negative_objects );

    m_drillFileHistory.Load( cfg->m_DrillFileHistory );
    m_zipFileHistory.Load( cfg->m_ZipFileHistory );
    m_jobFileHistory.Load( cfg->m_JobFileHistory );
}


void GERBVIEW_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    GERBVIEW_SETTINGS* cfg = dynamic_cast<GERBVIEW_SETTINGS*>( aCfg );
    wxCHECK( cfg, /*void*/ );

    cfg->m_Appearance.page_type = GetPageSettings().GetType();

    m_drillFileHistory.Save( &cfg->m_DrillFileHistory );
    m_zipFileHistory.Save( &cfg->m_ZipFileHistory );
    m_jobFileHistory.Save( &cfg->m_JobFileHistory );

    COLOR_SETTINGS* cs = Pgm().GetSettingsManager().GetColorSettings();
    Pgm().GetSettingsManager().SaveColorSettings( cs, "gerbview" );
}


COLOR_SETTINGS* GERBVIEW_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    SETTINGS_MANAGER&  mgr = Pgm().GetSettingsManager();
    GERBVIEW_SETTINGS* cfg = mgr.GetAppSettings<GERBVIEW_SETTINGS>();
    wxString currentTheme = cfg->m_ColorTheme;
    return mgr.GetColorSettings( currentTheme );
}


void GERBVIEW_FRAME::ReFillLayerWidget()
{
    wxWindowUpdateLocker no_update( m_LayersManager );

    m_LayersManager->ReFill();
    m_SelLayerBox->Resync();
    ReCreateAuxiliaryToolbar();

    wxAuiPaneInfo&  lyrs = m_auimgr.GetPane( m_LayersManager );
    wxSize          bestz = m_LayersManager->GetBestSize();
    bestz.x += 5;   // gives a little margin

    lyrs.MinSize( bestz );
    lyrs.BestSize( bestz );
    lyrs.FloatingSize( bestz );

    if( lyrs.IsDocked() )
        m_auimgr.Update();
    else
        m_LayersManager->SetSize( bestz );

    syncLayerWidget();
}


void GERBVIEW_FRAME::SetElementVisibility( int aLayerID, bool aNewState )
{
    KIGFX::VIEW* view = GetCanvas()->GetView();

    switch( aLayerID )
    {
    case LAYER_DCODES:
        gvconfig()->m_Appearance.show_dcodes = aNewState;

        for( int i = 0; i < GERBER_DRAWLAYERS_COUNT; i++ )
        {
            int layer = GERBER_DRAW_LAYER( i );
            int dcode_layer = GERBER_DCODE_LAYER( layer );
            view->SetLayerVisible( dcode_layer, aNewState && view->IsLayerVisible( layer ) );
        }

        break;

    case LAYER_NEGATIVE_OBJECTS:
    {
        gvconfig()->m_Appearance.show_negative_objects = aNewState;

        view->UpdateAllItemsConditionally( KIGFX::REPAINT,
                []( KIGFX::VIEW_ITEM* aItem )
                {
                    GERBER_DRAW_ITEM* item = dynamic_cast<GERBER_DRAW_ITEM*>( aItem );

                    // GetLayerPolarity() returns true for negative items
                    return ( item && item->GetLayerPolarity() );
                } );

        break;
    }

    case LAYER_GERBVIEW_DRAWINGSHEET:
        gvconfig()->m_Appearance.show_border_and_titleblock = aNewState;

        m_showBorderAndTitleBlock = gvconfig()->m_Appearance.show_border_and_titleblock;

        // NOTE: LAYER_DRAWINGSHEET always used for visibility, but the layer manager passes
        // LAYER_GERBVIEW_DRAWINGSHEET because of independent color control
        GetCanvas()->GetView()->SetLayerVisible( LAYER_DRAWINGSHEET, aNewState );
        break;

    case LAYER_GERBVIEW_GRID:
        SetGridVisibility( aNewState );
        break;

    case LAYER_GERBVIEW_PAGE_LIMITS:
        gvconfig()->m_Display.m_DisplayPageLimits = aNewState;
        SetPageSettings( GetPageSettings() );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "GERBVIEW_FRAME::SetElementVisibility(): bad arg %d" ),
                                      aLayerID ) );
    }

    ApplyDisplaySettingsToGAL();
    m_LayersManager->SetRenderState( aLayerID, aNewState );
}


void GERBVIEW_FRAME::ApplyDisplaySettingsToGAL()
{
    auto painter = static_cast<KIGFX::GERBVIEW_PAINTER*>( GetCanvas()->GetView()->GetPainter() );
    KIGFX::GERBVIEW_RENDER_SETTINGS* settings = painter->GetSettings();
    settings->SetHighContrast( gvconfig()->m_Display.m_HighContrastMode );
    settings->LoadColors( GetColorSettings() );

    GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
}


int GERBVIEW_FRAME::getNextAvailableLayer() const
{
    for( unsigned i = 0; i < ImagesMaxCount(); ++i )
    {
        const GERBER_FILE_IMAGE* gerber = GetGbrImage( i );

        if( gerber == nullptr )    // this graphic layer is available: use it
            return i;
    }

    return NO_AVAILABLE_LAYERS;
}


void GERBVIEW_FRAME::syncLayerWidget()
{
    m_LayersManager->SelectLayer( GetActiveLayer() );
}


void GERBVIEW_FRAME::syncLayerBox( bool aRebuildLayerBox )
{
    if( aRebuildLayerBox )
        m_SelLayerBox->Resync();

    m_SelLayerBox->SetSelection( GetActiveLayer() );

    int dcodeSelected = -1;
    GERBER_FILE_IMAGE*   gerber = GetGbrImage( GetActiveLayer() );

    if( gerber )
        dcodeSelected = gerber->m_Selected_Tool;

    if( m_DCodeSelector )
    {
        updateDCodeSelectBox();
        m_DCodeSelector->SetDCodeSelection( dcodeSelected );
        m_DCodeSelector->Enable( gerber != nullptr );
    }
}


void GERBVIEW_FRAME::SortLayersByFileExtension()
{
    RemapLayers( GetImagesList()->SortImagesByFileExtension() );
}


void GERBVIEW_FRAME::SortLayersByX2Attributes()
{
    RemapLayers( GetImagesList()->SortImagesByZOrder() );
}


void GERBVIEW_FRAME::RemapLayers( std::unordered_map<int, int> remapping )
{
    ReFillLayerWidget();
    syncLayerBox( true );

    std::unordered_map<int, int> view_remapping;

    for( const std::pair<const int, int>& entry : remapping )
    {
        view_remapping[ GERBER_DRAW_LAYER( entry.first ) ] = GERBER_DRAW_LAYER( entry.second );
        view_remapping[ GERBER_DCODE_LAYER( entry.first ) ] = GERBER_DCODE_LAYER( entry.second );
    }

    GetCanvas()->GetView()->ReorderLayerData( view_remapping );
    GetCanvas()->Refresh();
}


void GERBVIEW_FRAME::UpdateXORLayers()
{
    auto target = GetCanvas()->GetBackend() == GERBVIEW_DRAW_PANEL_GAL::GAL_TYPE_OPENGL
                          ? KIGFX::TARGET_CACHED
                          : KIGFX::TARGET_NONCACHED;
    KIGFX::VIEW* view = GetCanvas()->GetView();

    int lastVisibleLayer = -1;

    for( int i = 0; i < GERBER_DRAWLAYERS_COUNT; i++ )
    {
        view->SetLayerDiff( GERBER_DRAW_LAYER( i ), gvconfig()->m_Display.m_XORMode );

        // Caching doesn't work with layered rendering of XOR'd layers
        if( gvconfig()->m_Display.m_XORMode )
            view->SetLayerTarget( GERBER_DRAW_LAYER( i ), KIGFX::TARGET_NONCACHED );
        else
            view->SetLayerTarget( GERBER_DRAW_LAYER( i ), target );

        // We want the last visible layer, but deprioritize the active layer unless it's the
        // only layer
        if( ( lastVisibleLayer == -1 )
            || ( view->IsLayerVisible( GERBER_DRAW_LAYER( i ) ) && i != GetActiveLayer() ) )
        {
            lastVisibleLayer = i;
        }
    }

    //We don't want to diff the last visible layer onto the background, etc.
    if( lastVisibleLayer != -1 )
    {
        view->SetLayerTarget( GERBER_DRAW_LAYER( lastVisibleLayer ), target );
        view->SetLayerDiff( GERBER_DRAW_LAYER( lastVisibleLayer ), false );
    }

    view->RecacheAllItems();
    view->MarkDirty();
    view->UpdateAllItems( KIGFX::ALL );
}


void GERBVIEW_FRAME::UpdateTitleAndInfo()
{
    GERBER_FILE_IMAGE* gerber = GetGbrImage( GetActiveLayer() );

    // Display the gerber filename
    if( gerber == nullptr )
    {
        SetTitle( _("Gerber Viewer") );

        SetStatusText( wxEmptyString, 0 );

        wxString info;
        info.Printf( _( "Drawing layer not in use" ) );
        m_TextInfo->SetValue( info );

        if( KIUI::EnsureTextCtrlWidth( m_TextInfo, &info ) ) // Resized
           m_auimgr.Update();

        ClearMsgPanel();
        return;
    }
    else
    {
        wxString   title;
        wxFileName filename( gerber->m_FileName );

        title = filename.GetFullName();

        if( gerber->m_IsX2_file )
            title += wxS( " " ) + _( "(with X2 attributes)" );

        title += wxT( " \u2014 " ) + _( "Gerber Viewer" );
        SetTitle( title );

        gerber->DisplayImageInfo( this );

        // Display Image Name and Layer Name (from the current gerber data):
        wxString status;
        status.Printf( _( "Image name: \"%s\"  Layer name: \"%s\"" ),
                       gerber->m_ImageName,
                       gerber->GetLayerParams().m_LayerName );
        SetStatusText( status, 0 );

        // Display data format like fmt in X3.4Y3.4 no LZ or fmt mm X2.3 Y3.5 no TZ in main toolbar
        wxString info;
        info.Printf( wxT( "fmt: %s X%d.%d Y%d.%d no %cZ" ),
                     gerber->m_GerbMetric ? wxT( "mm" ) : wxT( "in" ),
                     gerber->m_FmtLen.x - gerber->m_FmtScale.x,
                     gerber->m_FmtScale.x,
                     gerber->m_FmtLen.y - gerber->m_FmtScale.y,
                     gerber->m_FmtScale.y,
                     gerber->m_NoTrailingZeros ? 'T' : 'L' );

        if( gerber->m_IsX2_file )
            info << wxT(" ") << _( "X2 attr" );

        m_TextInfo->SetValue( info );

        if( KIUI::EnsureTextCtrlWidth( m_TextInfo, &info ) ) // Resized
            m_auimgr.Update();
    }
}


bool GERBVIEW_FRAME::IsElementVisible( int aLayerID ) const
{
    switch( aLayerID )
    {
    case LAYER_DCODES:                return gvconfig()->m_Appearance.show_dcodes;
    case LAYER_NEGATIVE_OBJECTS:      return gvconfig()->m_Appearance.show_negative_objects;
    case LAYER_GERBVIEW_GRID:         return IsGridVisible();
    case LAYER_GERBVIEW_DRAWINGSHEET: return gvconfig()->m_Appearance.show_border_and_titleblock;
    case LAYER_GERBVIEW_PAGE_LIMITS:  return gvconfig()->m_Display.m_DisplayPageLimits;
    case LAYER_GERBVIEW_BACKGROUND:   return true;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "GERBVIEW_FRAME::IsElementVisible(): bad arg %d" ),
                                      aLayerID ) );
    }

    return true;
}


LSET GERBVIEW_FRAME::GetVisibleLayers() const
{
    LSET visible = LSET::AllLayersMask();

    if( GetCanvas() )
    {
        for( int i = 0; i < GERBER_DRAWLAYERS_COUNT; i++ )
            visible[i] = GetCanvas()->GetView()->IsLayerVisible( GERBER_DRAW_LAYER( i ) );
    }

    return visible;
}


void GERBVIEW_FRAME::SetVisibleLayers( LSET aLayerMask )
{
    if( GetCanvas() )
    {
        for( int i = 0; i < GERBER_DRAWLAYERS_COUNT; i++ )
        {
            bool v = aLayerMask[i];
            int layer = GERBER_DRAW_LAYER( i );
            GetCanvas()->GetView()->SetLayerVisible( layer, v );
            GetCanvas()->GetView()->SetLayerVisible( GERBER_DCODE_LAYER( layer ),
                                                     gvconfig()->m_Appearance.show_dcodes && v );
        }
    }
}


bool GERBVIEW_FRAME::IsLayerVisible( int aLayer ) const
{
    return m_LayersManager->IsLayerVisible( aLayer );
}


COLOR4D GERBVIEW_FRAME::GetVisibleElementColor( int aLayerID )
{
    COLOR4D color = COLOR4D::UNSPECIFIED;
    COLOR_SETTINGS* settings = GetColorSettings();

    switch( aLayerID )
    {
    case LAYER_NEGATIVE_OBJECTS:
    case LAYER_DCODES:
    case LAYER_GERBVIEW_DRAWINGSHEET:
    case LAYER_GERBVIEW_PAGE_LIMITS:
    case LAYER_GERBVIEW_BACKGROUND:
        color = settings->GetColor( aLayerID );
        break;

    case LAYER_GERBVIEW_GRID:
        color = GetGridColor();
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "GERBVIEW_FRAME::GetVisibleElementColor(): bad arg %d" ),
                                      aLayerID ) );
    }

    return color;
}


void GERBVIEW_FRAME::SetGridVisibility( bool aVisible )
{
    EDA_DRAW_FRAME::SetGridVisibility( aVisible );
    m_LayersManager->SetRenderState( LAYER_GERBVIEW_GRID, aVisible );
}


void GERBVIEW_FRAME::SetVisibleElementColor( int aLayerID, const COLOR4D& aColor )
{
    COLOR_SETTINGS* settings = GetColorSettings();

    settings->SetColor( aLayerID, aColor );

    switch( aLayerID )
    {
    case LAYER_GERBVIEW_DRAWINGSHEET:
    case LAYER_GERBVIEW_PAGE_LIMITS:
        SetPageSettings( GetPageSettings() );
        break;

    case LAYER_GERBVIEW_GRID:
        SetGridColor( aColor );
        break;

    case LAYER_GERBVIEW_BACKGROUND:
        SetDrawBgColor( aColor );
        break;

    default:
        break;
    }
}


COLOR4D GERBVIEW_FRAME::GetLayerColor( int aLayer ) const
{
    return GetColorSettings()->GetColor( aLayer );
}


void GERBVIEW_FRAME::SetLayerColor( int aLayer, const COLOR4D& aColor )
{
    GetColorSettings()->SetColor( aLayer, aColor );
    ApplyDisplaySettingsToGAL();
}


void GERBVIEW_FRAME::SetActiveLayer( int aLayer, bool doLayerWidgetUpdate )
{
    m_activeLayer = aLayer;

    if( gvconfig()->m_Display.m_XORMode )
        UpdateXORLayers();

    if( doLayerWidgetUpdate )
        m_LayersManager->SelectLayer( aLayer );

    UpdateTitleAndInfo();

    m_toolManager->PostAction( GERBVIEW_ACTIONS::layerChanged );       // notify other tools
    GetCanvas()->SetFocus();                 // otherwise hotkeys are stuck somewhere

    GetCanvas()->SetHighContrastLayer( GERBER_DRAW_LAYER( aLayer ) );
    GetCanvas()->Refresh();
}


void GERBVIEW_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    m_paper = aPageSettings;

    if( GetScreen() )
        GetScreen()->InitDataPoints( aPageSettings.GetSizeIU( gerbIUScale.IU_PER_MILS ) );

    GERBVIEW_DRAW_PANEL_GAL* drawPanel = static_cast<GERBVIEW_DRAW_PANEL_GAL*>( GetCanvas() );

    // Prepare drawing-sheet template
    DS_PROXY_VIEW_ITEM* drawingSheet = new DS_PROXY_VIEW_ITEM( gerbIUScale, &GetPageSettings(),
                                                               &Prj(), &GetTitleBlock(), nullptr );

    if( GetScreen() )
    {
        drawingSheet->SetPageNumber( "1" );
        drawingSheet->SetSheetCount( 1 );
    }

    drawingSheet->SetColorLayer( LAYER_GERBVIEW_DRAWINGSHEET );
    drawingSheet->SetPageBorderColorLayer( LAYER_GERBVIEW_PAGE_LIMITS );

    // Draw panel takes ownership of the drawing-sheet
    drawPanel->SetDrawingSheet( drawingSheet );
}


const PAGE_INFO& GERBVIEW_FRAME::GetPageSettings() const
{
    return m_paper;
}


const VECTOR2I GERBVIEW_FRAME::GetPageSizeIU() const
{
    // this function is only needed because EDA_DRAW_FRAME is not compiled
    // with either -DPCBNEW or -DEESCHEMA, so the virtual is used to route
    // into an application specific source file.
    return GetPageSettings().GetSizeIU( gerbIUScale.IU_PER_MILS );
}


const TITLE_BLOCK& GERBVIEW_FRAME::GetTitleBlock() const
{
    wxASSERT( m_gerberLayout );
    return m_gerberLayout->GetTitleBlock();
}


void GERBVIEW_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    wxASSERT( m_gerberLayout );
    m_gerberLayout->SetTitleBlock( aTitleBlock );
}


COLOR4D GERBVIEW_FRAME::GetGridColor()
{
    return GetColorSettings()->GetColor( LAYER_GERBVIEW_GRID );
}


void GERBVIEW_FRAME::SetGridColor( const COLOR4D& aColor )
{
    GetColorSettings()->SetColor( LAYER_GERBVIEW_GRID, aColor );
    GetCanvas()->GetGAL()->SetGridColor( aColor );
    m_gridColor = aColor;
}


void GERBVIEW_FRAME::DisplayGridMsg()
{
    VECTOR2D gridSize = GetCanvas()->GetGAL()->GetGridSize();
    wxString line;

    line.Printf( wxT( "grid X %s  Y %s" ),
                 MessageTextFromValue( gridSize.x, false ),
                 MessageTextFromValue( gridSize.y, false ) );

    SetStatusText( line, 4 );
    SetStatusText( line, 4 );
}


void GERBVIEW_FRAME::UpdateStatusBar()
{
    EDA_DRAW_FRAME::UpdateStatusBar();

    if( !GetScreen() )
        return;

    wxString line;
    VECTOR2D cursorPos = GetCanvas()->GetViewControls()->GetCursorPosition();

    if( GetShowPolarCoords() )  // display relative polar coordinates
    {
        VECTOR2D  v = cursorPos - GetScreen()->m_LocalOrigin;
        EDA_ANGLE theta( VECTOR2D( v.x, -v.y ) );
        double    ro = hypot( v.x, v.y );

        line.Printf( wxT( "r %s  theta %s" ),
                     MessageTextFromValue( ro, false ),
                     MessageTextFromValue( theta, false ) );

        SetStatusText( line, 3 );
    }

    // Display absolute coordinates:
    line.Printf( wxT( "X %s  Y %s" ),
                 MessageTextFromValue( cursorPos.x, false ),
                 MessageTextFromValue( cursorPos.y, false ) );
    SetStatusText( line, 2 );

    if( !GetShowPolarCoords() )
    {
        // Display relative cartesian coordinates:
        double dXpos = cursorPos.x - GetScreen()->m_LocalOrigin.x;
        double dYpos = cursorPos.y - GetScreen()->m_LocalOrigin.y;

        line.Printf( wxT( "dx %s  dy %s  dist %s" ),
                     MessageTextFromValue( dXpos, false ),
                     MessageTextFromValue( dYpos,false ),
                     MessageTextFromValue( hypot( dXpos, dYpos ), false ) );
        SetStatusText( line, 3 );
    }

    DisplayGridMsg();
}


GERBER_FILE_IMAGE* GERBVIEW_FRAME::GetGbrImage( int aIdx ) const
{
    return m_gerberLayout->GetImagesList()->GetGbrImage( aIdx );
}


unsigned GERBVIEW_FRAME::ImagesMaxCount() const
{
    return m_gerberLayout->GetImagesList()->ImagesMaxCount();
}


void GERBVIEW_FRAME::unitsChangeRefresh()
{
    // Called on units change (see EDA_DRAW_FRAME)
    EDA_DRAW_FRAME::unitsChangeRefresh();
    updateDCodeSelectBox();
    UpdateGridSelectBox();
}


void GERBVIEW_FRAME::ActivateGalCanvas()
{
    EDA_DRAW_FRAME::ActivateGalCanvas();

    EDA_DRAW_PANEL_GAL* galCanvas = GetCanvas();

    if( m_toolManager )
    {
        m_toolManager->SetEnvironment( m_gerberLayout, GetCanvas()->GetView(),
                                       GetCanvas()->GetViewControls(), config(), this );
        m_toolManager->ResetTools( TOOL_BASE::GAL_SWITCH );
    }

    galCanvas->GetGAL()->SetGridColor( GetLayerColor( LAYER_GERBVIEW_GRID ) );

    SetPageSettings( GetPageSettings() );

    galCanvas->GetView()->RecacheAllItems();
    galCanvas->SetEventDispatcher( m_toolDispatcher );
    galCanvas->StartDrawing();

    m_LayersManager->ReFill();
    m_LayersManager->ReFillRender();

    ReCreateOptToolbar();
    ReCreateMenuBar();
}


void GERBVIEW_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( m_gerberLayout, GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), config(), this );
    m_actions = new GERBVIEW_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new GERBVIEW_SELECTION_TOOL );
    m_toolManager->RegisterTool( new GERBVIEW_CONTROL );
    m_toolManager->RegisterTool( new GERBVIEW_INSPECTION_TOOL );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    m_toolManager->InvokeTool( "gerbview.InteractiveSelection" );
}


void GERBVIEW_FRAME::setupUIConditions()
{
    EDA_DRAW_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( ACTIONS::zoomTool,      CHECK( cond.CurrentTool( ACTIONS::zoomTool ) ) );
    mgr->SetConditions( ACTIONS::selectionTool, CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );
    mgr->SetConditions( ACTIONS::measureTool,   CHECK( cond.CurrentTool( ACTIONS::measureTool ) ) );

    mgr->SetConditions( ACTIONS::toggleGrid,        CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::togglePolarCoords, CHECK( cond.PolarCoordinates() ) );
    mgr->SetConditions( ACTIONS::toggleCursorStyle, CHECK( cond.FullscreenCursor() ) );

    mgr->SetConditions( ACTIONS::millimetersUnits,  CHECK( cond.Units( EDA_UNITS::MILLIMETRES ) ) );
    mgr->SetConditions( ACTIONS::inchesUnits,       CHECK( cond.Units( EDA_UNITS::INCHES ) ) );
    mgr->SetConditions( ACTIONS::milsUnits,         CHECK( cond.Units( EDA_UNITS::MILS ) ) );

    auto flashedDisplayOutlinesCond =
        [this] ( const SELECTION& )
        {
            return !gvconfig()->m_Display.m_DisplayFlashedItemsFill;
        };

    auto linesFillCond =
        [this] ( const SELECTION& )
        {
            return !gvconfig()->m_Display.m_DisplayLinesFill;
        };

    auto polygonsFilledCond =
        [this] ( const SELECTION& )
        {
            return !gvconfig()->m_Display.m_DisplayPolygonsFill;
        };

    auto negativeObjectsCond =
        [this] ( const SELECTION& )
        {
            return gvconfig()->m_Appearance.show_negative_objects;
        };

    auto dcodeCond =
        [this] ( const SELECTION& )
        {
            return gvconfig()->m_Appearance.show_dcodes;
        };

    auto diffModeCond =
        [this] ( const SELECTION& )
        {
            return gvconfig()->m_Display.m_DiffMode;
        };

    auto xorModeCond =
        [this] ( const SELECTION& )
        {
            return gvconfig()->m_Display.m_XORMode;
        };

    auto highContrastModeCond =
        [this] ( const SELECTION& )
        {
            return gvconfig()->m_Display.m_HighContrastMode;
        };

    auto flipGerberCond =
        [this] ( const SELECTION& )
        {
            return gvconfig()->m_Display.m_FlipGerberView;
        };

    auto layersManagerShownCondition =
        [this] ( const SELECTION& aSel )
        {
            return m_show_layer_manager_tools;
        };

    mgr->SetConditions( GERBVIEW_ACTIONS::flashedDisplayOutlines,  CHECK( flashedDisplayOutlinesCond ) );
    mgr->SetConditions( GERBVIEW_ACTIONS::linesDisplayOutlines,    CHECK( linesFillCond ) );
    mgr->SetConditions( GERBVIEW_ACTIONS::polygonsDisplayOutlines, CHECK( polygonsFilledCond ) );
    mgr->SetConditions( GERBVIEW_ACTIONS::negativeObjectDisplay,   CHECK( negativeObjectsCond ) );
    mgr->SetConditions( GERBVIEW_ACTIONS::dcodeDisplay,            CHECK( dcodeCond ) );
    mgr->SetConditions( GERBVIEW_ACTIONS::toggleDiffMode,          CHECK( diffModeCond ) );
    mgr->SetConditions( GERBVIEW_ACTIONS::toggleXORMode,           CHECK( xorModeCond ) );
    mgr->SetConditions( GERBVIEW_ACTIONS::flipGerberView,          CHECK( flipGerberCond ) );
    mgr->SetConditions( ACTIONS::highContrastMode,                 CHECK( highContrastModeCond ) );
    mgr->SetConditions( GERBVIEW_ACTIONS::toggleLayerManager,      CHECK( layersManagerShownCondition ) );

#undef CHECK
#undef ENABLE
}


void GERBVIEW_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    EDA_DRAW_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    // Update gal display options like cursor shape, grid options:
    GERBVIEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<GERBVIEW_SETTINGS>();
    GetGalDisplayOptions().ReadWindowSettings( cfg->m_Window );

    SetPageSettings( PAGE_INFO( gvconfig()->m_Appearance.page_type ) );

    UpdateXORLayers();

    SetElementVisibility( LAYER_DCODES, gvconfig()->m_Appearance.show_dcodes );

    GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
    GetCanvas()->ForceRefresh();

    RecreateToolbars();
    ReFillLayerWidget();                // Update the layers list
    m_LayersManager->ReFillRender();    // Update colors in Render after the config is read

    Layout();
    SendSizeEvent();
}


SELECTION& GERBVIEW_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<GERBVIEW_SELECTION_TOOL>()->GetSelection();
}


void GERBVIEW_FRAME::ToggleLayerManager()
{
    m_show_layer_manager_tools = !m_show_layer_manager_tools;

    // show/hide auxiliary Vertical layers and visibility manager toolbar
    m_auimgr.GetPane( "LayersManager" ).Show( m_show_layer_manager_tools );
    m_auimgr.Update();
}
