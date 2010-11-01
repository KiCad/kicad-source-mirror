/*******************/
/* gerberframe.cpp */
/*******************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"

#include "gerbview.h"
#include "class_gerber_draw_item.h"
#include "pcbplot.h"
#include "bitmaps.h"
#include "gerbview_id.h"
#include "hotkeys.h"
#include "class_GERBER.h"

#include "build_version.h"


/****************************************/
/* class WinEDA_GerberFrame for GerbView*/
/****************************************/

BEGIN_EVENT_TABLE( WinEDA_GerberFrame, WinEDA_BasePcbFrame )
    EVT_CLOSE( WinEDA_GerberFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_GerberFrame::OnSize )

    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, WinEDA_GerberFrame::OnZoom )

    EVT_TOOL( wxID_FILE, WinEDA_GerberFrame::Files_io )
    EVT_TOOL( ID_INC_LAYER_AND_APPEND_FILE, WinEDA_GerberFrame::Files_io )
    EVT_TOOL( ID_GERBVIEW_LOAD_DRILL_FILE, WinEDA_GerberFrame::Files_io )
    EVT_TOOL( ID_GERBVIEW_LOAD_DCODE_FILE, WinEDA_GerberFrame::Files_io )
    EVT_TOOL( ID_NEW_BOARD, WinEDA_GerberFrame::Files_io )

// Menu Files:
    EVT_MENU( wxID_FILE, WinEDA_GerberFrame::Files_io )
    EVT_MENU( ID_MENU_INC_LAYER_AND_APPEND_FILE, WinEDA_GerberFrame::Files_io )
    EVT_MENU( ID_NEW_BOARD, WinEDA_GerberFrame::Files_io )
    EVT_MENU( ID_GEN_PLOT, WinEDA_GerberFrame::ToPlotter )
    EVT_MENU( ID_GERBVIEW_EXPORT_TO_PCBNEW,
              WinEDA_GerberFrame::ExportDataInPcbnewFormat )

    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, WinEDA_GerberFrame::OnFileHistory )

    EVT_MENU( ID_EXIT, WinEDA_GerberFrame::Process_Special_Functions )

// menu Preferences
    EVT_MENU( ID_CONFIG_REQ,
                    WinEDA_GerberFrame::Process_Config )
    EVT_MENU( ID_CONFIG_SAVE,
                    WinEDA_GerberFrame::Process_Config )
    EVT_MENU_RANGE( ID_PREFERENCES_HOTKEY_START,
                    ID_PREFERENCES_HOTKEY_END,
                    WinEDA_GerberFrame::Process_Config )

    EVT_MENU( ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG, WinEDA_GerberFrame::OnSelectOptionToolbar )
    EVT_MENU( ID_GERBVIEW_OPTIONS_SETUP, WinEDA_GerberFrame::InstallGerberOptionsDialog )

    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                    WinEDA_DrawFrame::SetLanguage )

// menu Postprocess
    EVT_MENU( ID_GERBVIEW_SHOW_LIST_DCODES,
              WinEDA_GerberFrame::Process_Special_Functions )
    EVT_MENU( ID_GERBVIEW_POPUP_DELETE_DCODE_ITEMS,
              WinEDA_GerberFrame::Process_Special_Functions )
    EVT_MENU( ID_GERBVIEW_SHOW_SOURCE,
              WinEDA_GerberFrame::Process_Special_Functions )


// menu Miscellaneous
    EVT_MENU( ID_GERBVIEW_GLOBAL_DELETE,
              WinEDA_GerberFrame::Process_Special_Functions )

// Menu Help
    EVT_MENU( ID_GENERAL_HELP, WinEDA_DrawFrame::GetKicadHelp )
    EVT_MENU( ID_KICAD_ABOUT, WinEDA_DrawFrame::GetKicadAbout )

    EVT_TOOL( wxID_CUT, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( wxID_COPY, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( wxID_PASTE, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( wxID_PRINT, WinEDA_GerberFrame::ToPrinter )
    EVT_TOOL( ID_FIND_ITEMS, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_KICAD_CHOICEBOX( ID_TOOLBARH_GERBVIEW_SELECT_LAYER,
                         WinEDA_GerberFrame::Process_Special_Functions )

    EVT_KICAD_CHOICEBOX( ID_TOOLBARH_GERBER_SELECT_TOOL,
                         WinEDA_GerberFrame::Process_Special_Functions )

// Vertical toolbar:
    EVT_TOOL( ID_NO_SELECT_BUTT, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( ID_GERBVIEW_DELETE_ITEM_BUTT,
          WinEDA_GerberFrame::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    WinEDA_GerberFrame::Process_Special_Functions )

// Pop up menu
    EVT_MENU( ID_GERBVIEW_POPUP_DELETE_DCODE_ITEMS,
              WinEDA_GerberFrame::Process_Special_Functions )

// Option toolbar
    EVT_TOOL_RANGE( ID_TB_OPTIONS_START, ID_TB_OPTIONS_END,
                    WinEDA_GerberFrame::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH,
                    WinEDA_GerberFrame::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_LINES_SKETCH,
                    WinEDA_GerberFrame::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
                    WinEDA_GerberFrame::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_DCODES,
                    WinEDA_GerberFrame::OnSelectOptionToolbar )

END_EVENT_TABLE()

WinEDA_GerberFrame::WinEDA_GerberFrame( wxWindow*       father,
                                        const wxString& title,
                                        const wxPoint&  pos,
                                        const wxSize&   size,
                                        long            style ) :
    WinEDA_BasePcbFrame( father, GERBER_FRAME, title, pos, size, style )
{
    m_FrameName = wxT( "GerberFrame" );
    m_show_layer_manager_tools = true;

    m_Draw_Axis = true;         // true to show X and Y axis on screen
    m_Draw_Sheet_Ref = false;   // true for reference drawings.
    m_HotkeysZoomAndGridList = s_Gerbview_Hokeys_Descr;
    m_SelLayerBox = NULL;
    m_SelLayerTool = NULL;

    if( DrawPanel )
        DrawPanel->m_Block_Enable = true;

    // Give an icon
#ifdef __WINDOWS__
    SetIcon( wxICON( a_icon_gerbview ) );
#else
    SetIcon( wxICON( icon_gerbview ) );
#endif

    SetBaseScreen( ScreenPcb );
    ActiveScreen = ScreenPcb;

    SetBoard( new BOARD( NULL, this ) );
    GetBoard()->SetEnabledLayers( FULL_LAYERS );     // All 32 layers enabled at first.

    // Create the PCB_LAYER_WIDGET *after* SetBoard():
    wxFont font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    int pointSize = font.GetPointSize();
    int screenHeight = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );
    if( screenHeight <= 900 )
        pointSize = (pointSize * 8) / 10;
    m_LayersManager = new GERBER_LAYER_WIDGET( this, DrawPanel, pointSize );

    // LoadSettings() *after* creating m_LayersManager, because LoadSettings()
    // initialize parameters in m_LayersManager
    LoadSettings();
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateMenuBar();
    ReCreateHToolbar();
//    ReCreateVToolbar();       // Currently: no right vertical toolbar
    ReCreateOptToolbar();

    m_auimgr.SetManagedWindow( this );

    wxAuiPaneInfo horiz;
    horiz.Gripper( false );
    horiz.DockFixed( true );
    horiz.Movable( false );
    horiz.Floatable( false );
    horiz.CloseButton( false );
    horiz.CaptionVisible( false );

    wxAuiPaneInfo vert( horiz );

    vert.TopDockable( false ).BottomDockable( false );
    horiz.LeftDockable( false ).RightDockable( false );

    // LAYER_WIDGET is floatable, but initially docked at far right
    wxAuiPaneInfo   lyrs;
    lyrs.CloseButton( false );
    lyrs.Caption( _( "Visibles" ) );
    lyrs.IsFloatable();

    if( m_HToolBar )
        m_auimgr.AddPane( m_HToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top().Row( 0 ) );

    if( m_VToolBar )
        m_auimgr.AddPane( m_VToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right().Row( 1 ) );

    m_auimgr.AddPane( m_LayersManager, lyrs.Name( wxT( "m_LayersManagerToolBar" ) ).Right().Row( 0 ) );

    if( m_OptionsToolBar )
        m_auimgr.AddPane( m_OptionsToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_OptionsToolBar" ) ).Left() );

    if( DrawPanel )
        m_auimgr.AddPane( DrawPanel,
                          wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    if( MsgPanel )
        m_auimgr.AddPane( MsgPanel,
                          wxAuiPaneInfo( horiz ).Name( wxT( "MsgPanel" ) ).Bottom() );

    ReFillLayerWidget();    // this is near end because contents establish size

    m_auimgr.Update();

}


WinEDA_GerberFrame::~WinEDA_GerberFrame()
{
    SetBaseScreen( ScreenPcb );
    extern PARAM_CFG_BASE* ParamCfgList[];
    wxGetApp().SaveCurrentSetupValues( ParamCfgList );
}


void WinEDA_GerberFrame::OnCloseWindow( wxCloseEvent& Event )
{
    SaveSettings();
    Destroy();
}


int WinEDA_GerberFrame::BestZoom()
{
    double x, y;
    EDA_Rect bbox;

    BOARD_ITEM* item = GetBoard()->m_Drawings;
    for( ; item; item = item->Next() )
    {
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;
        bbox.Merge( gerb_item->GetBoundingBox() );
    }

    wxSize size = DrawPanel->GetClientSize();
    x = ( bbox.GetWidth() + GetScreen()->GetGridSize().x ) / (double) size.x;
    y = ( bbox.GetHeight() + GetScreen()->GetGridSize().y ) / (double) size.y;
    GetScreen()->m_Curseur = bbox.Centre();

    return wxRound( MAX( x, y ) * (double) GetScreen()->m_ZoomScalar );
}

/**************************************/
void WinEDA_GerberFrame::LoadSettings()
/**************************************/
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config == NULL )
        return;

    WinEDA_BasePcbFrame::LoadSettings();
    long pageSize_opt;
    config->Read( GerbviewShowPageSizeOption, &pageSize_opt, 0l );
    int imax = 0;
    for( ; g_GerberPageSizeList[imax] != NULL; imax++ );
    if( pageSize_opt < 0 || pageSize_opt >= imax )
        pageSize_opt = 0;
    GetScreen()->m_CurrentSheetDesc = g_GerberPageSizeList[pageSize_opt];
    if ( pageSize_opt > 0 )
    {
        m_Draw_Sheet_Ref = true;
    }

    long tmp;
    config->Read( GerbviewShowDCodes, &tmp, 1 );
    SetElementVisibility( DCODES_VISIBLE, tmp );
}

/**************************************/
void WinEDA_GerberFrame::SaveSettings()
/**************************************/
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config == NULL )
        return;

    WinEDA_BasePcbFrame::SaveSettings();

    wxRealPoint GridSize = GetScreen()->GetGridSize();

    long pageSize_opt = 0;
    if( m_Draw_Sheet_Ref )
    {
        for( int ii = 1; g_GerberPageSizeList[ii] != NULL; ii++ )
        {
            if( GetScreen()->m_CurrentSheetDesc == g_GerberPageSizeList[ii] )
            {
                pageSize_opt = ii;
                break;
            }
        }
    }
    config->Write( GerbviewShowPageSizeOption, pageSize_opt );
    config->Write( GerbviewShowDCodes, IsElementVisible( DCODES_VISIBLE ) );
}


void WinEDA_GerberFrame::ReFillLayerWidget()
{
    m_LayersManager->ReFill();

    wxAuiPaneInfo& lyrs = m_auimgr.GetPane( m_LayersManager );

    wxSize bestz = m_LayersManager->GetBestSize();

    lyrs.MinSize( bestz );
    lyrs.BestSize( bestz );
    lyrs.FloatingSize( bestz );

    if( lyrs.IsDocked() )
        m_auimgr.Update();
    else
        m_LayersManager->SetSize( bestz );

    syncLayerWidget();
}

/** Function IsGridVisible() , virtual
 * @return true if the grid must be shown
 */
bool WinEDA_GerberFrame::IsGridVisible()
{
    return IsElementVisible( GERBER_GRID_VISIBLE );
}

/** Function SetGridVisibility() , virtual
 * It may be overloaded by derived classes
 * if you want to store/retrieve the grid visiblity in configuration.
 * @param aVisible = true if the grid must be shown
 */
void WinEDA_GerberFrame::SetGridVisibility(bool aVisible)
{
    SetElementVisibility(GERBER_GRID_VISIBLE, aVisible);
}

/** Function GetGridColor() , virtual
 * @return the color of the grid
 */
int WinEDA_GerberFrame::GetGridColor()
{
    return GetBoard()->GetVisibleElementColor( GERBER_GRID_VISIBLE );
}

/** Function SetGridColor() , virtual
 * @param aColor = the new color of the grid
 */
void WinEDA_GerberFrame::SetGridColor(int aColor)
{
    GetBoard()->SetVisibleElementColor( GERBER_GRID_VISIBLE, aColor );
}


/**
 * Function SetElementVisibility
 * changes the visibility of an element category
 * @param aGERBER_VISIBLE is from the enum by the same name
 * @param aNewState = The new visibility state of the element category
 * @see enum aGERBER_VISIBLE
 */
void WinEDA_GerberFrame::SetElementVisibility( int aGERBER_VISIBLE, bool aNewState )
{
    GetBoard()->SetElementVisibility( aGERBER_VISIBLE, aNewState );
    m_LayersManager->SetRenderState( aGERBER_VISIBLE, aNewState );
}


void WinEDA_GerberFrame::syncLayerWidget( )
{
    m_LayersManager->SelectLayer( getActiveLayer() );
    UpdateTitleAndInfo();
}

/**
 * Function syncLayerBox
 * updates the currently "selected" layer within m_SelLayerBox
 * The currently active layer, as defined by the return value of
 * getActiveLayer().  And updates the colored icon in the toolbar.
 */
void WinEDA_GerberFrame::syncLayerBox()
{
    m_SelLayerBox->SetSelection( getActiveLayer() );
    UpdateTitleAndInfo();
}

/** function SetLanguage
 * called on a language menu selection
 * Update Layer manager title and tabs texts
 */
void WinEDA_GerberFrame::SetLanguage( wxCommandEvent& event )
{
    WinEDA_DrawFrame::SetLanguage( event );
    m_LayersManager->SetLayersManagerTabsText();
    wxAuiPaneInfo& pane_info = m_auimgr.GetPane( m_LayersManager );
    pane_info.Caption( _( "Visibles" ) );
    m_auimgr.Update();

    ReFillLayerWidget();
}


void WinEDA_GerberFrame::Liste_D_Codes( )
{
    int               ii, jj;
    D_CODE*           pt_D_code;
    wxString          Line;
    WinEDA_TextFrame* List;
    int               scale = 10000;
    int               curr_layer = GetScreen()->m_Active_Layer;

    List = new WinEDA_TextFrame( this, _( "List D codes" ) );

    for( int layer = 0; layer < 32; layer++ )
    {
        GERBER_IMAGE* gerber = g_GERBER_List[layer];
        if( gerber == NULL )
            continue;

        if( gerber->ReturnUsedDcodeNumber() == 0 )
            continue;

        if( layer == curr_layer )
            Line.Printf( wxT( "*** Active layer (%2.2d) ***" ), layer + 1 );
        else
            Line.Printf( wxT( "*** layer %2.2d  ***" ), layer + 1 );
        List->Append( Line );

        for( ii = 0, jj = 1; ii < TOOLS_MAX_COUNT; ii++ )
        {
            pt_D_code = gerber->GetDCODE( ii + FIRST_DCODE, false );
            if( pt_D_code == NULL )
                continue;

            if( !pt_D_code->m_InUse && !pt_D_code->m_Defined )
                continue;

            Line.Printf( wxT(
                             "tool %2.2d:   D%2.2d  V %2.4f  H %2.4f  %s" ),
                         jj,
                         pt_D_code->m_Num_Dcode,
                         (float) pt_D_code->m_Size.y / scale,
                         (float) pt_D_code->m_Size.x / scale,
                         D_CODE::ShowApertureType( pt_D_code->m_Shape )
                         );

            if( !pt_D_code->m_Defined )
                Line += wxT( " ?" );

            if( !pt_D_code->m_InUse )
                Line += wxT( " *" );

            List->Append( Line );
            jj++;
        }
    }

    ii = List->ShowModal();
    List->Destroy();
    if( ii < 0 )
        return;
}

/** function UpdateTitleAndInfo
 * displays the short filename (if exists) of the selected layer
 *  on the caption of the main gerbview window
 * displays image name and the last layer name (found in the gerber file: LN <name> command)
 *  in the status bar
 * Note layer name can change when reading a gerber file, and the layer name is the last found.
 * So, show the layer name is not very useful, and can be seen as a debug feature.
 */
void WinEDA_GerberFrame::UpdateTitleAndInfo()
{
    GERBER_IMAGE* gerber = g_GERBER_List[GetScreen()->m_Active_Layer];
    wxString text;
    // Display the gerber filename
    if( gerber == NULL )
    {
        text = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();
        SetTitle( text );
        SetStatusText( wxEmptyString, 0 );
        m_TextInfo->Clear();
        ClearMsgPanel();
        return;
    }

    text = _( "File:" );
    text << wxT( " " ) << gerber->m_FileName;
    SetTitle( text );

    gerber->DisplayImageInfo( );

    // Display Image Name and Layer Name (from the current gerber data):
    text.Printf( _("Image name: \"%s\"  Layer name: \"%s\""),
        GetChars(gerber->m_ImageName), GetChars(gerber->GetLayerParams( ).m_LayerName) );
    SetStatusText( text, 0 );

    // Display data format like fmt in X3.4Y3.4 no LZ or fmt mm X2.3 Y3.5 no TZ in main toolbar
    text.Printf(wxT("fmt: %s X%d.%d Y%d.%d no %cZ"),
        gerber->m_GerbMetric ? wxT("mm") : wxT("in"),
        gerber->m_FmtLen.x - gerber->m_FmtScale.x, gerber->m_FmtScale.x,
        gerber->m_FmtLen.y - gerber->m_FmtScale.y, gerber->m_FmtScale.y,
        gerber->m_NoTrailingZeros ? 'T' : 'L');

    m_TextInfo->SetValue( text );
}


