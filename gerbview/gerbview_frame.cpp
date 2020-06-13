/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <eda_base_frame.h>
#include <trigo.h>
#include <base_units.h>
#include <gbr_layer_box_selector.h>
#include <msgpanel.h>
#include <bitmaps.h>
#include <wildcards_and_files_ext.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <DCodeSelectionbox.h>
#include <gerbview_layer_widget.h>
#include <gerbview_draw_panel_gal.h>
#include <gerbview_settings.h>
#include <gal/graphics_abstraction_layer.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tool/action_toolbar.h>
#include <tool/tool_dispatcher.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/zoom_tool.h>
#include <tools/gerbview_actions.h>
#include <tools/gerbview_selection_tool.h>
#include <tools/gerbview_control.h>
#include <view/view.h>
#include <base_screen.h>
#include <gerbview_painter.h>
#include <geometry/shape_poly_set.h>
#include <widgets/paged_dialog.h>
#include <dialogs/panel_gerbview_settings.h>
#include <dialogs/panel_gerbview_display_options.h>
#include <panel_hotkeys_editor.h>
#include <wx/wupdlock.h>
#include <tool/grid_menu.h>

GERBVIEW_FRAME::GERBVIEW_FRAME( KIWAY* aKiway, wxWindow* aParent )
        : EDA_DRAW_FRAME( aKiway, aParent, FRAME_GERBER, wxT( "GerbView" ), wxDefaultPosition,
                wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, GERBVIEW_FRAME_NAME ),
          m_activeLayer( 0 ),
          m_zipFileHistory( DEFAULT_FILE_HISTORY_SIZE, ID_GERBVIEW_ZIP_FILE1,
                  ID_GERBVIEW_ZIP_FILE_LIST_CLEAR, _( "Clear Recent Zip Files" ) ),
          m_drillFileHistory( DEFAULT_FILE_HISTORY_SIZE, ID_GERBVIEW_DRILL_FILE1,
                  ID_GERBVIEW_DRILL_FILE_LIST_CLEAR, _( "Clear Recent Drill Files" ) ),
          m_jobFileHistory( DEFAULT_FILE_HISTORY_SIZE, ID_GERBVIEW_JOB_FILE1,
                  ID_GERBVIEW_JOB_FILE_LIST_CLEAR, _( "Clear Recent Job Files" ) ),
          m_TextInfo( nullptr )
{
    m_gerberLayout = NULL;
    m_show_layer_manager_tools = true;
    m_showBorderAndTitleBlock = false;      // true for reference drawings.
    m_SelLayerBox = NULL;
    m_DCodeSelector = NULL;
    m_SelComponentBox = nullptr;
    m_SelNetnameBox = nullptr;
    m_SelAperAttributesBox = nullptr;
    m_displayMode = 0;
    m_AboutTitle = "GerbView";

    SHAPE_POLY_SET dummy;   // A ugly trick to force the linker to include
                            // some methods in code and avoid link errors

    int fileHistorySize = Pgm().GetCommonSettings()->m_System.file_history_size;
    m_drillFileHistory.SetMaxFiles( fileHistorySize );
    m_zipFileHistory.SetMaxFiles( fileHistorySize );
    m_jobFileHistory.SetMaxFiles( fileHistorySize );

    auto* galCanvas = new GERBVIEW_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                                   GetGalDisplayOptions(),
                                                   EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE );

    SetCanvas( galCanvas );

    // GerbView requires draw priority for rendering negative objects
    galCanvas->GetView()->UseDrawPriority( true );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_gerbview_xpm ) );
    SetIcon( icon );

    // Be sure a page info is set. this default value will be overwritten later.
    PAGE_INFO pageInfo( wxT( "GERBER" ) );
    SetLayout( new GBR_LAYOUT() );
    SetPageSettings( pageInfo );

    SetVisibleLayers( LSET::AllLayersMask() );         // All draw layers visible.

    SetScreen( new BASE_SCREEN( GetPageSettings().GetSizeIU() ) );

    // Create the PCB_LAYER_WIDGET *after* SetLayout():
    m_LayersManager = new GERBER_LAYER_WIDGET( this, GetCanvas() );

    // LoadSettings() *after* creating m_LayersManager, because LoadSettings()
    // initialize parameters in m_LayersManager
    LoadSettings( config() );

    setupTools();
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateOptToolbar();
    ReCreateAuxiliaryToolbar();

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_auxiliaryToolBar, EDA_PANE().HToolbar().Name( "AuxToolbar" ).Top().Layer(4) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" ).Left().Layer(3) );
    m_auimgr.AddPane( m_LayersManager, EDA_PANE().Palette().Name( "LayersManager" ).Right().Layer(3)
                      .Caption( _( "Layers Manager" ) ).PaneBorder( false )
                      .MinSize( 80, -1 ).BestSize( m_LayersManager->GetBestSize() ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    ReFillLayerWidget();                // this is near end because contents establish size
    m_auimgr.Update();

    SetActiveLayer( 0, true );
    GetToolManager()->RunAction( ACTIONS::zoomFitScreen, false );

    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = LoadCanvasTypeSetting();

    // Nudge user to switch to OpenGL if they are on legacy or Cairo
    if( m_firstRunDialogSetting < 1 )
    {
        if( canvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL )
        {
            wxString msg = _( "KiCad can use your graphics card to give you a smoother "
                              "and faster experience. This option is turned off by "
                              "default since it is not compatible with all computers.\n\n"
                              "Would you like to try enabling graphics acceleration?\n\n"
                              "If you'd like to choose later, select Accelerated Graphics "
                              "in the Preferences menu." );

            wxMessageDialog dlg( this, msg, _( "Enable Graphics Acceleration" ), wxYES_NO );

            dlg.SetYesNoLabels( _( "&Enable Acceleration" ), _( "&No Thanks" ) );

            if( dlg.ShowModal() == wxID_YES )
            {
                // Save Cairo as default in case OpenGL crashes
                saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO );

                // Switch to OpenGL, which will save the new setting if successful
                GetToolManager()->RunAction( ACTIONS::acceleratedGraphics, true );

                // Switch back to Cairo if OpenGL is not supported
                if( GetCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE )
                    GetToolManager()->RunAction( ACTIONS::standardGraphics, true );
            }
            else
            {
                // If they were on legacy, switch to Cairo
                GetToolManager()->RunAction( ACTIONS::standardGraphics, true );
            }
        }

        m_firstRunDialogSetting = 1;
        SaveSettings( config() );
    }

    SwitchCanvas( canvasType );

    // Enable the axes to match legacy draw style
    auto& galOptions = GetGalDisplayOptions();
    galOptions.m_axesEnabled = true;
    galOptions.NotifyChanged();

    m_LayersManager->ReFill();
    m_LayersManager->ReFillRender();    // Update colors in Render after the config is read

    GetToolManager()->RunAction( ACTIONS::zoomFitScreen, true );

    // Update the checked state of tools
    SyncToolbars();

    // Ensure the window is on top
    Raise();
}


GERBVIEW_FRAME::~GERBVIEW_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    GetCanvas()->GetView()->Clear();

    GetGerberLayout()->GetImagesList()->DeleteAllImages();
    delete m_gerberLayout;
}


void GERBVIEW_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
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
    // It happens when starting Gerbview with a gerber job file to load
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
            wxLogDebug( wxT( "MRU path: %s." ), GetChars( path ) );
            m_mruPath = path;
            return true;
        }

        const unsigned limit = std::min( unsigned( aFileSet.size() ),
                                         unsigned( GERBER_DRAWLAYERS_COUNT ) );

        int layer = 0;

        for( unsigned i = 0; i < limit; ++i, ++layer )
        {
            SetActiveLayer( layer );

            // Try to guess the type of file by its ext
            // if it is .drl (Kicad files), .nc or .xnc it is a drill file
            wxFileName fn( aFileSet[i] );
            wxString ext = fn.GetExt();

            if( ext == DrillFileExtension ||    // our Excellon format
                ext == "nc" || ext == "xnc" )   // alternate ext for Excellon format
                LoadExcellonFiles( aFileSet[i] );
            else if( ext == GerberJobFileExtension )
                LoadGerberJobFile( aFileSet[i] );
            else
                LoadGerberFiles( aFileSet[i] );
        }
    }

    Zoom_Automatique( true );        // Zoom fit in frame

    return true;
}


void GERBVIEW_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    if( aCfg->m_Window.grid.sizes.empty() )
    {
        aCfg->m_Window.grid.sizes = { "100 mil",
                                      "50 mil",
                                      "25 mil",
                                      "20 mil",
                                      "10 mil",
                                      "5 mil",
                                      "2.5 mil",
                                      "2 mil",
                                      "1 mil",
                                      "0.5 mil",
                                      "0.2 mil",
                                      "0.1 mil",
                                      "5.0 mm",
                                      "2.5 mm",
                                      "1.0 mm",
                                      "0.5 mm",
                                      "0.25 mm",
                                      "0.2 mm",
                                      "0.1 mm",
                                      "0.05 mm",
                                      "0.025 mm",
                                      "0.01 mm" };
    }

    if( aCfg->m_Window.zoom_factors.empty() )
    {
        aCfg->m_Window.zoom_factors = { 0.022,
                                        0.035,
                                        0.05,
                                        0.08,
                                        0.13,
                                        0.22,
                                        0.35,
                                        0.6,
                                        1.0,
                                        2.2,
                                        3.5,
                                        5.0,
                                        8.0,
                                        13.0,
                                        22.0,
                                        35.0,
                                        50.0,
                                        80.0,
                                        130.0,
                                        220.0 };
    }

    for( double& factor : aCfg->m_Window.zoom_factors )
        factor = std::min( factor, MAX_ZOOM_FACTOR );

    GERBVIEW_SETTINGS* cfg = dynamic_cast<GERBVIEW_SETTINGS*>( aCfg );
    wxCHECK( cfg, /*void*/ );

    SetElementVisibility( LAYER_WORKSHEET, cfg->m_Appearance.show_border_and_titleblock );

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
    cfg->m_Appearance.show_border_and_titleblock = m_showBorderAndTitleBlock;
    cfg->m_Appearance.show_dcodes = IsElementVisible( LAYER_DCODES );
    cfg->m_Appearance.show_negative_objects = IsElementVisible( LAYER_NEGATIVE_OBJECTS );

    m_drillFileHistory.Save( &cfg->m_DrillFileHistory );
    m_zipFileHistory.Save( &cfg->m_ZipFileHistory );
    m_jobFileHistory.Save( &cfg->m_JobFileHistory );

    COLOR_SETTINGS* cs = Pgm().GetSettingsManager().GetColorSettings();
    Pgm().GetSettingsManager().SaveColorSettings( cs, "gerbview" );
}


void GERBVIEW_FRAME::ReFillLayerWidget()
{
    wxWindowUpdateLocker no_update( m_LayersManager );

    m_LayersManager->ReFill();
    m_SelLayerBox->Resync();
    ReCreateAuxiliaryToolbar();
    // Update the checked state of tools
    SyncToolbars();

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
    bool dcodes_changed = false;

    switch( aLayerID )
    {
    case LAYER_DCODES:
        dcodes_changed = m_DisplayOptions.m_DisplayDCodes != aNewState;
        m_DisplayOptions.m_DisplayDCodes = aNewState;
        break;

    case LAYER_NEGATIVE_OBJECTS:
    {
        m_DisplayOptions.m_DisplayNegativeObjects = aNewState;

        auto view = GetCanvas()->GetView();

        view->UpdateAllItemsConditionally( KIGFX::REPAINT, []( KIGFX::VIEW_ITEM* aItem )
        {
            auto item = dynamic_cast<GERBER_DRAW_ITEM*>( aItem );

            // GetLayerPolarity() returns true for negative items
            return ( item && item->GetLayerPolarity() );
        } );
        break;
    }

    case LAYER_GERBVIEW_WORKSHEET:
        m_showBorderAndTitleBlock = aNewState;
        // NOTE: LAYER_WORKSHEET always used for visibility, but the layer manager passes
        // LAYER_GERBVIEW_WORKSHEET because of independent color control
        GetCanvas()->GetView()->SetLayerVisible( LAYER_WORKSHEET, aNewState );
        break;

    case LAYER_GERBVIEW_GRID:
        SetGridVisibility( aNewState );
        break;

    default:
        wxLogDebug( wxT( "GERBVIEW_FRAME::SetElementVisibility(): bad arg %d" ), aLayerID );
    }

    if( dcodes_changed )
    {
        auto view = GetCanvas()->GetView();

        for( int i = 0; i < GERBER_DRAWLAYERS_COUNT; i++ )
        {
            int layer = GERBER_DRAW_LAYER( i );
            int dcode_layer = GERBER_DCODE_LAYER( layer );
            view->SetLayerVisible( dcode_layer, aNewState && view->IsLayerVisible( layer ) );
        }
    }

    applyDisplaySettingsToGAL();
    m_LayersManager->SetRenderState( aLayerID, aNewState );
}


void GERBVIEW_FRAME::applyDisplaySettingsToGAL()
{
    auto painter = static_cast<KIGFX::GERBVIEW_PAINTER*>( GetCanvas()->GetView()->GetPainter() );
    KIGFX::GERBVIEW_RENDER_SETTINGS* settings = painter->GetSettings();
    settings->LoadDisplayOptions( m_DisplayOptions );
    settings->LoadColors( Pgm().GetSettingsManager().GetColorSettings() );

    GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
}


int GERBVIEW_FRAME::getNextAvailableLayer( int aLayer ) const
{
    int layer = aLayer;

    for( unsigned i = 0; i < ImagesMaxCount(); ++i )
    {
        const GERBER_FILE_IMAGE* gerber = GetGbrImage( layer );

        if( gerber == NULL )    // this graphic layer is available: use it
            return layer;

        ++layer;                // try next graphic layer

        if( layer >= (int)ImagesMaxCount() )
            layer = 0;
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
        m_DCodeSelector->Enable( gerber != NULL );
    }
}


void GERBVIEW_FRAME::Liste_D_Codes()
{
    int             ii, jj;
    wxString        Line;
    wxArrayString   list;
    double          scale = GetUserUnits() == EDA_UNITS::INCHES ? IU_PER_MILS * 1000 : IU_PER_MM;
    int       curr_layer = GetActiveLayer();

    for( int layer = 0; layer < (int)ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = GetGbrImage( layer );

        if( gerber == NULL )
            continue;

        if( gerber->GetDcodesCount() == 0 )
            continue;

        if( layer == curr_layer )
            Line.Printf( wxT( "*** Active layer (%2.2d) ***" ), layer + 1 );
        else
            Line.Printf( wxT( "*** layer %2.2d  ***" ), layer + 1 );

        list.Add( Line );

        const char* units = GetUserUnits() == EDA_UNITS::INCHES ? "\"" : "mm";

        for( ii = 0, jj = 1; ii < TOOLS_MAX_COUNT; ii++ )
        {
            D_CODE* pt_D_code = gerber->GetDCODE( ii + FIRST_DCODE );

            if( pt_D_code == NULL )
                continue;

            if( !pt_D_code->m_InUse && !pt_D_code->m_Defined )
                continue;

            Line.Printf( wxT( "tool %2.2d:   D%2.2d   V %.4f %s  H %.4f %s   %s  attribute '%s'" ),
                         jj,
                         pt_D_code->m_Num_Dcode,
                         pt_D_code->m_Size.y / scale, units,
                         pt_D_code->m_Size.x / scale, units,
                         D_CODE::ShowApertureType( pt_D_code->m_Shape ),
                         pt_D_code->m_AperFunction.IsEmpty()? wxT( "none" ) : GetChars( pt_D_code->m_AperFunction )
                         );

            if( !pt_D_code->m_Defined )
                Line += wxT( " (not defined)" );

            if( pt_D_code->m_InUse )
                Line += wxT( " (in use)" );

            list.Add( Line );
            jj++;
        }
    }

    wxSingleChoiceDialog    dlg( this, wxEmptyString, _( "D Codes" ), list, (void**) NULL,
                                 wxCHOICEDLG_STYLE & ~wxCANCEL );

    dlg.ShowModal();
}


void GERBVIEW_FRAME::SortLayersByX2Attributes()
{
    auto remapping = GetImagesList()->SortImagesByZOrder();

    ReFillLayerWidget();
    syncLayerBox( true );

    std::unordered_map<int, int> view_remapping;

    for( auto it : remapping )
    {
        view_remapping[ GERBER_DRAW_LAYER( it.first) ] = GERBER_DRAW_LAYER( it.second );
        view_remapping[ GERBER_DCODE_LAYER( GERBER_DRAW_LAYER( it.first) ) ] =
            GERBER_DCODE_LAYER( GERBER_DRAW_LAYER( it.second ) );
    }

    GetCanvas()->GetView()->ReorderLayerData( view_remapping );
    GetCanvas()->Refresh();
}


void GERBVIEW_FRAME::UpdateDisplayOptions( const GBR_DISPLAY_OPTIONS& aOptions )
{
    bool update_flashed =   ( m_DisplayOptions.m_DisplayFlashedItemsFill !=
                              aOptions.m_DisplayFlashedItemsFill );
    bool update_lines =     ( m_DisplayOptions.m_DisplayLinesFill !=
                              aOptions.m_DisplayLinesFill );
    bool update_polygons =  ( m_DisplayOptions.m_DisplayPolygonsFill !=
                              aOptions.m_DisplayPolygonsFill );

    m_DisplayOptions = aOptions;

    applyDisplaySettingsToGAL();

    auto view = GetCanvas()->GetView();

    if( update_flashed )
    {
        view->UpdateAllItemsConditionally( KIGFX::REPAINT, []( KIGFX::VIEW_ITEM* aItem )
        {
            auto item = static_cast<GERBER_DRAW_ITEM*>( aItem );

            switch( item->m_Shape )
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
    else if( update_lines )
    {
        view->UpdateAllItemsConditionally( KIGFX::REPAINT, []( KIGFX::VIEW_ITEM* aItem )
        {
            auto item = static_cast<GERBER_DRAW_ITEM*>( aItem );

            switch( item->m_Shape )
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
    else if( update_polygons )
    {
        view->UpdateAllItemsConditionally( KIGFX::REPAINT, []( KIGFX::VIEW_ITEM* aItem )
        {
            auto item = static_cast<GERBER_DRAW_ITEM*>( aItem );

            return ( item->m_Shape == GBR_POLYGON );
        } );
    }

    view->UpdateAllItems( KIGFX::COLOR );
    GetCanvas()->Refresh();
}


void GERBVIEW_FRAME::UpdateTitleAndInfo()
{
    GERBER_FILE_IMAGE* gerber = GetGbrImage( GetActiveLayer() );

    // Display the gerber filename
    if( gerber == NULL )
    {
        SetTitle( "GerbView" );

        SetStatusText( wxEmptyString, 0 );

        wxString info;
        info.Printf( _( "Drawing layer %d not in use" ), GetActiveLayer() + 1 );
        m_TextInfo->SetValue( info );

        if( EnsureTextCtrlWidth( m_TextInfo, &info ) )  // Resized
           m_auimgr.Update();

        ClearMsgPanel();
        return;
    }
    else
    {
        wxString title;
        title.Printf( _( "GerbView" ) + wxT( " \u2014 %s%s" ),
                      gerber->m_FileName,
                      gerber->m_IsX2_file ? _( " (with X2 attributes)" )
                                          : wxString( wxEmptyString ) );
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

        if( EnsureTextCtrlWidth( m_TextInfo, &info ) )  // Resized
            m_auimgr.Update();
    }
}


bool GERBVIEW_FRAME::IsElementVisible( int aLayerID ) const
{
    switch( aLayerID )
    {
    case LAYER_DCODES:              return m_DisplayOptions.m_DisplayDCodes;
    case LAYER_NEGATIVE_OBJECTS:    return m_DisplayOptions.m_DisplayNegativeObjects;
    case LAYER_GERBVIEW_GRID:       return IsGridVisible();
    case LAYER_WORKSHEET:           return m_showBorderAndTitleBlock;
    case LAYER_GERBVIEW_BACKGROUND: return true;

    default:
        wxLogDebug( wxT( "GERBVIEW_FRAME::IsElementVisible(): bad arg %d" ), aLayerID );
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
                                                     m_DisplayOptions.m_DisplayDCodes && v );
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

    switch( aLayerID )
    {
    case LAYER_NEGATIVE_OBJECTS:
    case LAYER_DCODES:
    case LAYER_GERBVIEW_WORKSHEET:
    case LAYER_GERBVIEW_BACKGROUND:
        color = Pgm().GetSettingsManager().GetColorSettings()->GetColor( aLayerID );
        break;

    case LAYER_GERBVIEW_GRID:
        color = GetGridColor();
        break;

    default:
        wxLogDebug( wxT( "GERBVIEW_FRAME::GetVisibleElementColor(): bad arg %d" ), aLayerID );
    }

    return color;
}


void GERBVIEW_FRAME::SetGridVisibility( bool aVisible )
{
    EDA_DRAW_FRAME::SetGridVisibility( aVisible );
    m_LayersManager->SetRenderState( LAYER_GERBVIEW_GRID, aVisible );
}


void GERBVIEW_FRAME::SetVisibleElementColor( int aLayerID, COLOR4D aColor )
{
    COLOR_SETTINGS* settings = Pgm().GetSettingsManager().GetColorSettings();

    switch( aLayerID )
    {
    case LAYER_NEGATIVE_OBJECTS:
    case LAYER_DCODES:
    case LAYER_GERBVIEW_WORKSHEET:
        settings->SetColor( aLayerID, aColor );
        break;

    case LAYER_GERBVIEW_GRID:
        SetGridColor( aColor );
        settings->SetColor( aLayerID, aColor );
        break;

    case LAYER_GERBVIEW_BACKGROUND:
        SetDrawBgColor( aColor );
        settings->SetColor( aLayerID, aColor );
        break;

    default:
        wxLogDebug( wxT( "GERBVIEW_FRAME::SetVisibleElementColor(): bad arg %d" ), aLayerID );
    }
}


COLOR4D GERBVIEW_FRAME::GetNegativeItemsColor()
{
    if( IsElementVisible( LAYER_NEGATIVE_OBJECTS ) )
        return GetVisibleElementColor( LAYER_NEGATIVE_OBJECTS );
    else
        return GetDrawBgColor();
}


COLOR4D GERBVIEW_FRAME::GetLayerColor( int aLayer ) const
{
    return Pgm().GetSettingsManager().GetColorSettings()->GetColor( aLayer );
}


void GERBVIEW_FRAME::SetLayerColor( int aLayer, COLOR4D aColor )
{
    Pgm().GetSettingsManager().GetColorSettings()->SetColor( aLayer, aColor );
    applyDisplaySettingsToGAL();
}


void GERBVIEW_FRAME::SetActiveLayer( int aLayer, bool doLayerWidgetUpdate )
{
    m_activeLayer = aLayer;

    if( doLayerWidgetUpdate )
        m_LayersManager->SelectLayer( aLayer );

    UpdateTitleAndInfo();

    m_toolManager->RunAction( GERBVIEW_ACTIONS::layerChanged );       // notify other tools
    GetCanvas()->SetFocus();                 // otherwise hotkeys are stuck somewhere

    GetCanvas()->SetHighContrastLayer( GERBER_DRAW_LAYER( aLayer ) );
    GetCanvas()->Refresh();
}


void GERBVIEW_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    m_paper = aPageSettings;

    if( GetScreen() )
        GetScreen()->InitDataPoints( aPageSettings.GetSizeIU() );

    auto drawPanel = static_cast<GERBVIEW_DRAW_PANEL_GAL*>( GetCanvas() );

    // Prepare worksheet template
    auto worksheet = new KIGFX::WS_PROXY_VIEW_ITEM( IU_PER_MILS, &GetPageSettings(),
                                                    &Prj(), &GetTitleBlock() );

    if( GetScreen() )
    {
        worksheet->SetSheetNumber( 1 );
        worksheet->SetSheetCount( 1 );
    }

    worksheet->SetColorLayer( LAYER_GERBVIEW_WORKSHEET );

    // Draw panel takes ownership of the worksheet
    drawPanel->SetWorksheet( worksheet );
}


const PAGE_INFO& GERBVIEW_FRAME::GetPageSettings() const
{
    return m_paper;
}


const wxSize GERBVIEW_FRAME::GetPageSizeIU() const
{
    // this function is only needed because EDA_DRAW_FRAME is not compiled
    // with either -DPCBNEW or -DEESCHEMA, so the virtual is used to route
    // into an application specific source file.
    return GetPageSettings().GetSizeIU();
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


const wxPoint& GERBVIEW_FRAME::GetAuxOrigin() const
{
    wxASSERT( m_gerberLayout );
    return m_gerberLayout->GetAuxOrigin();
}


void GERBVIEW_FRAME::SetAuxOrigin( const wxPoint& aPosition )
{
    wxASSERT( m_gerberLayout );
    m_gerberLayout->SetAuxOrigin( aPosition );
}


void GERBVIEW_FRAME::SetGridColor( COLOR4D aColor )
{
    GetCanvas()->GetGAL()->SetGridColor( aColor );
    m_gridColor = aColor;
}


/*
 * Display the grid status.
 */
void GERBVIEW_FRAME::DisplayGridMsg()
{
    wxString line;
    wxString gridformatter;

    switch( m_userUnits )
    {
    case EDA_UNITS::INCHES:      gridformatter = "grid X %.6f  Y %.6f"; break;
    case EDA_UNITS::MILLIMETRES: gridformatter = "grid X %.6f  Y %.6f"; break;
    default:                     gridformatter = "grid X %f  Y %f";     break;
    }

    double grid_x = To_User_Unit( m_userUnits, GetCanvas()->GetGAL()->GetGridSize().x );
    double grid_y = To_User_Unit( m_userUnits, GetCanvas()->GetGAL()->GetGridSize().y );
    line.Printf( gridformatter, grid_x, grid_y );

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
        double   dx = cursorPos.x - GetScreen()->m_LocalOrigin.x;
        double   dy = cursorPos.y - GetScreen()->m_LocalOrigin.y;
        double   theta = RAD2DEG( atan2( -dy, dx ) );
        double   ro = hypot( dx, dy );
        wxString formatter;

        switch( GetUserUnits() )
        {
        case EDA_UNITS::INCHES:      formatter = wxT( "r %.6f  theta %.1f" ); break;
        case EDA_UNITS::MILLIMETRES: formatter = wxT( "r %.5f  theta %.1f" ); break;
        case EDA_UNITS::UNSCALED:    formatter = wxT( "r %f  theta %f" );     break;
        default:                     wxASSERT( false );                       break;
        }

        line.Printf( formatter, To_User_Unit( GetUserUnits(), ro ), theta );

        SetStatusText( line, 3 );
    }

    // Display absolute coordinates:
    double dXpos = To_User_Unit( GetUserUnits(), cursorPos.x );
    double dYpos = To_User_Unit( GetUserUnits(), cursorPos.y );

    wxString absformatter;
    wxString relformatter;

    switch( GetUserUnits() )
    {
    case EDA_UNITS::INCHES:
        absformatter = wxT( "X %.6f  Y %.6f" );
        relformatter = wxT( "dx %.6f  dy %.6f  dist %.4f" );
        break;

    case EDA_UNITS::MILLIMETRES:
        absformatter = wxT( "X %.5f  Y %.5f" );
        relformatter = wxT( "dx %.5f  dy %.5f  dist %.3f" );
        break;

    case EDA_UNITS::UNSCALED:
        absformatter = wxT( "X %f  Y %f" );
        relformatter = wxT( "dx %f  dy %f  dist %f" );
        break;

    default:
        wxASSERT( false );
        break;
    }

    line.Printf( absformatter, dXpos, dYpos );
    SetStatusText( line, 2 );

    if( !GetShowPolarCoords() )  // display relative cartesian coordinates
    {
        // Display relative coordinates:
        dXpos = To_User_Unit( GetUserUnits(), cursorPos.x - GetScreen()->m_LocalOrigin.x );
        dYpos = To_User_Unit( GetUserUnits(), cursorPos.y - GetScreen()->m_LocalOrigin.y );

        // We already decided the formatter above
        line.Printf( relformatter, dXpos, dYpos, hypot( dXpos, dYpos ) );
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
    updateGridSelectBox();
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

    // Update the checked state of tools
    SyncToolbars();
}


void GERBVIEW_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                         PANEL_HOTKEYS_EDITOR* aHotkeysPanel )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new wxPanel( book ), _( "Gerbview" ) );
    book->AddSubPage( new PANEL_GERBVIEW_DISPLAY_OPTIONS( this, book ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_GERBVIEW_SETTINGS( this, book ), _( "Editing Options" ) );

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}



void GERBVIEW_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( m_gerberLayout, GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), config(), this );
    m_actions = new GERBVIEW_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new GERBVIEW_SELECTION_TOOL );
    m_toolManager->RegisterTool( new GERBVIEW_CONTROL );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    m_toolManager->InvokeTool( "gerbview.InteractiveSelection" );
}


void GERBVIEW_FRAME::updateGridSelectBox()
{
    UpdateStatusBar();
    DisplayUnitsMsg();

    if( m_gridSelectBox == NULL )
        return;

    // Update grid values with the current units setting.
    m_gridSelectBox->Clear();
    wxArrayString gridsList;

    GRID_MENU::BuildChoiceList( &gridsList, config(), GetUserUnits() != EDA_UNITS::INCHES );

    for( const wxString& grid : gridsList )
        m_gridSelectBox->Append( grid );

    m_gridSelectBox->SetSelection( config()->m_Window.grid.last_size_idx );
}


void GERBVIEW_FRAME::updateZoomSelectBox()
{
    if( m_zoomSelectBox == NULL )
        return;

    double zoom = GetCanvas()->GetGAL()->GetZoomFactor() / ZOOM_COEFF;

    m_zoomSelectBox->Clear();
    m_zoomSelectBox->Append( _( "Zoom Auto" ) );
    m_zoomSelectBox->SetSelection( 0 );

    for( unsigned i = 0;  i < config()->m_Window.zoom_factors.size();  ++i )
    {
        double current = config()->m_Window.zoom_factors[i];

        m_zoomSelectBox->Append( wxString::Format( _( "Zoom %.2f" ), current ) );

        if( zoom == current )
            m_zoomSelectBox->SetSelection( i + 1 );
    }
}


void GERBVIEW_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    EDA_DRAW_FRAME::CommonSettingsChanged( aEnvVarsChanged );

    RecreateToolbars();
    Layout();
    SendSizeEvent();
}

