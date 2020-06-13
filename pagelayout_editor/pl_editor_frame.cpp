/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2017-2020 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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
#include <kiface_i.h>
#include <pgm_base.h>
#include <base_units.h>
#include <msgpanel.h>
#include <bitmaps.h>
#include <dialogs/panel_pl_editor_color_settings.h>
#include <pl_editor_frame.h>
#include <pl_editor_id.h>
#include <pl_editor_settings.h>
#include <pl_draw_panel_gal.h>
#include <pl_editor_screen.h>
#include <ws_data_model.h>
#include <properties_frame.h>
#include <widgets/paged_dialog.h>
#include <panel_display_options.h>
#include <panel_hotkeys_editor.h>
#include <view/view.h>
#include <confirm.h>
#include <tool/selection.h>
#include <tool/action_toolbar.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/picker_tool.h>
#include <tool/zoom_tool.h>
#include <tools/pl_actions.h>
#include <tools/pl_selection_tool.h>
#include <tools/pl_drawing_tools.h>
#include <tools/pl_edit_tool.h>
#include <tools/pl_point_editor.h>
#include <invoke_pl_editor_dialog.h>
#include <tools/pl_editor_control.h>
#include <widgets/infobar.h>

BEGIN_EVENT_TABLE( PL_EDITOR_FRAME, EDA_DRAW_FRAME )
    EVT_CLOSE( PL_EDITOR_FRAME::OnCloseWindow )
    EVT_MENU( wxID_CLOSE, PL_EDITOR_FRAME::OnExit )
    EVT_MENU( wxID_EXIT, PL_EDITOR_FRAME::OnExit )

    EVT_MENU( wxID_FILE, PL_EDITOR_FRAME::Files_io )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, PL_EDITOR_FRAME::OnFileHistory )
    EVT_MENU( ID_FILE_LIST_CLEAR, PL_EDITOR_FRAME::OnClearFileHistory )

    EVT_TOOL( ID_SHOW_REAL_MODE, PL_EDITOR_FRAME::OnSelectTitleBlockDisplayMode )
    EVT_TOOL( ID_SHOW_PL_EDITOR_MODE, PL_EDITOR_FRAME::OnSelectTitleBlockDisplayMode )
    EVT_CHOICE( ID_SELECT_COORDINATE_ORIGIN, PL_EDITOR_FRAME::OnSelectCoordOriginCorner )
    EVT_CHOICE( ID_SELECT_PAGE_NUMBER, PL_EDITOR_FRAME::OnSelectPage )

    EVT_UPDATE_UI( ID_SHOW_REAL_MODE, PL_EDITOR_FRAME::OnUpdateTitleBlockDisplayNormalMode )
    EVT_UPDATE_UI( ID_SHOW_PL_EDITOR_MODE, PL_EDITOR_FRAME::OnUpdateTitleBlockDisplayEditMode )
END_EVENT_TABLE()


PL_EDITOR_FRAME::PL_EDITOR_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        EDA_DRAW_FRAME( aKiway, aParent, FRAME_PL_EDITOR, wxT( "PlEditorFrame" ),
                        wxDefaultPosition, wxDefaultSize,
                        KICAD_DEFAULT_DRAWFRAME_STYLE, PL_EDITOR_FRAME_NAME )
{
    m_userUnits = EDA_UNITS::MILLIMETRES;

    m_showBorderAndTitleBlock   = true; // true for reference drawings.
    m_originSelectChoice = 0;
    WS_DATA_MODEL::GetTheInstance().m_EditMode = true;
    SetShowPageLimits( true );
    m_AboutTitle = "PlEditor";

    m_propertiesFrameWidth = 200;

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_pagelayout_editor_xpm ) );
    SetIcon( icon );

    // Create GAL canvas
#ifdef __WXMAC__
    // Cairo renderer doesn't handle Retina displays
    m_canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
#else
    m_canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
#endif

    auto* drawPanel = new PL_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                             GetGalDisplayOptions(), m_canvasType );
    SetCanvas( drawPanel );

    LoadSettings( config() );

    wxSize pageSizeIU = GetPageLayout().GetPageSettings().GetSizeIU();
    SetScreen( new PL_EDITOR_SCREEN( pageSizeIU ) );

    setupTools();
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();

    // Create the infobar
    m_infoBar = new WX_INFOBAR( this, &m_auimgr );

    wxWindow* stsbar = GetStatusBar();
    int dims[] = {

        // balance of status bar on far left is set to a default or whatever is left over.
        -1,

        // When using GetTextSize() remember the width of '1' is not the same
        // as the width of '0' unless the font is fixed width, and it usually won't be.

        // zoom:
        GetTextSize( wxT( "Z 762000" ), stsbar ).x + 10,

        // cursor coords
        GetTextSize( wxT( "X 0234.567  Y 0234.567" ), stsbar ).x + 10,

        // delta distances
        GetTextSize( wxT( "dx 0234.567  dx 0234.567" ), stsbar ).x + 10,

        // grid size
        GetTextSize( wxT( "grid 0234.567" ), stsbar ).x + 10,

        // Coord origin (use the bigger message)
        GetTextSize( _( "coord origin: Right Bottom page corner" ), stsbar ).x + 10,

        // units display, Inches is bigger than mm
        GetTextSize( _( "Inches" ), stsbar ).x + 20
    };

    SetStatusWidths( arrayDim( dims ), dims );

    m_auimgr.SetManagedWindow( this );

    m_propertiesPagelayout = new PROPERTIES_FRAME( this );

    // Horizontal items; layers 4 - 6
    m_auimgr.AddPane( m_mainToolBar,
                      EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel,
                      EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );
    m_auimgr.AddPane( m_infoBar,
                      EDA_PANE().InfoBar().Name( "InfoBar" ).Top().Layer(1) );

    // Vertical items; layers 1 - 3
    m_auimgr.AddPane( m_drawToolBar,
                      EDA_PANE().VToolbar().Name( "ToolsToolbar" ).Right().Layer(2) );

    m_auimgr.AddPane( m_propertiesPagelayout, EDA_PANE().Palette().Name( "Props" ).Right().Layer(3)
                      .Caption( _( "Properties" ) ).MinSize( m_propertiesPagelayout->GetMinSize() )
                      .BestSize( m_propertiesFrameWidth, -1 ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    ActivateGalCanvas();

    // Call Update() to fix all pane default sizes, especially the "InfoBar" pane before
    // hidding it.
    m_auimgr.Update();

    // We don't want the infobar displayed right away
    m_auimgr.GetPane( "InfoBar" ).Hide();
    m_auimgr.Update();

    // Add the exit key handler
    InitExitKey();

    wxPoint originCoord = ReturnCoordOriginCorner();
    SetGridOrigin( originCoord );

    // Initialize the current page layout
    WS_DATA_MODEL& pglayout = WS_DATA_MODEL::GetTheInstance();
#if 0       //start with empty layout
    pglayout.AllowVoidList( true );
    pglayout.ClearList();
#else       // start with the default Kicad layout
    pglayout.SetPageLayout();
#endif
    OnNewPageLayout();

    // Ensure the window is on top
    Raise();
}


PL_EDITOR_FRAME::~PL_EDITOR_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();
}

void PL_EDITOR_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( nullptr, GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), config(), this );
    m_actions = new PL_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );

    GetCanvas()->SetEventDispatcher( m_toolDispatcher );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new PL_SELECTION_TOOL );
    m_toolManager->RegisterTool( new PL_EDITOR_CONTROL );
    m_toolManager->RegisterTool( new PL_DRAWING_TOOLS );
    m_toolManager->RegisterTool( new PL_EDIT_TOOL );
    m_toolManager->RegisterTool( new PL_POINT_EDITOR );
    m_toolManager->RegisterTool( new PICKER_TOOL );
    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    m_toolManager->InvokeTool( "plEditor.InteractiveSelection" );
}


bool PL_EDITOR_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    wxString fn = aFileSet[0];

    if( !LoadPageLayoutDescrFile( fn ) )
    {
        wxMessageBox( wxString::Format( _( "Error when loading file \"%s\"" ), fn ) );
        return false;
    }
    else
    {
        OnNewPageLayout();
        return true;
    }
}


bool PL_EDITOR_FRAME::IsContentModified()
{
    return GetScreen() && GetScreen()->IsModify();
}


void PL_EDITOR_FRAME::OnExit( wxCommandEvent& aEvent )
{
    if( aEvent.GetId() == wxID_EXIT )
        Kiway().OnKiCadExit();

    if( aEvent.GetId() == wxID_CLOSE || Kiface().IsSingle() )
        Close( false );
}


void PL_EDITOR_FRAME::OnCloseWindow( wxCloseEvent& aEvent )
{
    // Shutdown blocks must be determined and vetoed as early as possible
    if( SupportsShutdownBlockReason() && aEvent.GetId() == wxEVT_QUERY_END_SESSION
            && IsContentModified() )
    {
        aEvent.Veto();
        return;
    }

    if( IsContentModified() )
    {
        wxFileName filename = GetCurrentFileName();
        wxString msg = _( "Save changes to \"%s\" before closing?" );

        if( !HandleUnsavedChanges( this, wxString::Format( msg, filename.GetFullName() ),
                                   [&]()->bool { return saveCurrentPageLayout(); } ) )
        {
            aEvent.Veto();
            return;
        }
    }

    // do not show the window because we do not want any paint event
    Show( false );

    // On Linux, m_propertiesPagelayout must be destroyed
    // before deleting the main frame to avoid a crash when closing
    m_propertiesPagelayout->Destroy();
    Destroy();
}


/* Handles the selection of tools, menu, and popup menu commands.
 */
void PL_EDITOR_FRAME::OnSelectPage( wxCommandEvent& event )
{
    KIGFX::VIEW* view = GetCanvas()->GetView();
    view->SetLayerVisible( LAYER_WORKSHEET_PAGE1, m_pageSelectBox->GetSelection() == 0 );
    view->SetLayerVisible( LAYER_WORKSHEET_PAGEn, m_pageSelectBox->GetSelection() == 1 );
    GetCanvas()->Refresh();
}


/* called when the user select one of the 4 page corner as corner
 * reference (or the left top paper corner)
 */
void PL_EDITOR_FRAME::OnSelectCoordOriginCorner( wxCommandEvent& event )
{
    m_originSelectChoice = m_originSelectBox->GetSelection();
    UpdateStatusBar();  // Update grid origin
    GetCanvas()->DisplayWorksheet();
    GetCanvas()->Refresh();
}


void PL_EDITOR_FRAME::OnSelectTitleBlockDisplayMode( wxCommandEvent& event )
{
    WS_DATA_MODEL::GetTheInstance().m_EditMode = (event.GetId() == ID_SHOW_PL_EDITOR_MODE);
    HardRedraw();
}


void PL_EDITOR_FRAME::ToPrinter( bool doPreview )
{
    // static print data and page setup data, to remember settings during the session
    static wxPrintData* s_PrintData;
    static wxPageSetupDialogData* s_pageSetupData = nullptr;

    const PAGE_INFO& pageInfo = GetPageSettings();

    if( s_PrintData == NULL )  // First print
    {
        s_PrintData = new wxPrintData();
        s_PrintData->SetQuality( wxPRINT_QUALITY_HIGH );      // Default resolution = HIGH;
    }

    if( !s_PrintData->Ok() )
    {
        wxMessageBox( _( "Error Init Printer info" ) );
        return;
    }

    if( s_pageSetupData == NULL )
        s_pageSetupData = new wxPageSetupDialogData( *s_PrintData );

    s_pageSetupData->SetPaperId( pageInfo.GetPaperId() );
    s_pageSetupData->GetPrintData().SetOrientation( pageInfo.GetWxOrientation() );

    if( pageInfo.IsCustom() )
    {
        if( pageInfo.IsPortrait() )
            s_pageSetupData->SetPaperSize( wxSize( Mils2mm( pageInfo.GetWidthMils() ),
                                                   Mils2mm( pageInfo.GetHeightMils() ) ) );
        else
            s_pageSetupData->SetPaperSize( wxSize( Mils2mm( pageInfo.GetHeightMils() ),
                                                   Mils2mm( pageInfo.GetWidthMils() ) ) );
    }

    *s_PrintData = s_pageSetupData->GetPrintData();

    if( doPreview )
        InvokeDialogPrintPreview( this, s_PrintData );
    else
        InvokeDialogPrint( this, s_PrintData, s_pageSetupData );
}


void PL_EDITOR_FRAME::OnUpdateTitleBlockDisplayNormalMode( wxUpdateUIEvent& event )
{
    event.Check( WS_DATA_MODEL::GetTheInstance().m_EditMode == false );
}


void PL_EDITOR_FRAME::OnUpdateTitleBlockDisplayEditMode( wxUpdateUIEvent& event )
{
    event.Check( WS_DATA_MODEL::GetTheInstance().m_EditMode == true );
}


const BOX2I PL_EDITOR_FRAME::GetDocumentExtents() const
{
    BOX2I rv( VECTOR2I( 0, 0 ), GetPageLayout().GetPageSettings().GetSizeIU() );
    return rv;
}


void PL_EDITOR_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                          PANEL_HOTKEYS_EDITOR* aHotkeysPanel  )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new PANEL_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );
    book->AddPage( new PANEL_PL_EDITOR_COLOR_SETTINGS( this, aParent ), _( "Colors" ) );

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


void PL_EDITOR_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    if( aCfg->m_Window.grid.sizes.empty() )
    {
        aCfg->m_Window.grid.sizes = { "1.0 mm",
                                      "0.50 mm",
                                      "0.25 mm",
                                      "0.20 mm",
                                      "0.10 mm" };
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

    PL_EDITOR_SETTINGS* cfg = dynamic_cast<PL_EDITOR_SETTINGS*>( aCfg );
    wxCHECK( cfg, /*void*/ );

    m_propertiesFrameWidth = cfg->m_PropertiesFrameWidth;
    m_originSelectChoice = cfg->m_CornerOrigin;

    SetDrawBgColor( cfg->m_BlackBackground ? BLACK : WHITE );

    PAGE_INFO::SetCustomWidthMils( cfg->m_LastCustomWidth );
    PAGE_INFO::SetCustomHeightMils( cfg->m_LastCustomHeight );

    PAGE_INFO pageInfo = GetPageSettings();
    pageInfo.SetType( cfg->m_LastPaperSize, cfg->m_LastWasPortrait );
    SetPageSettings( pageInfo );
}


void PL_EDITOR_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    auto cfg = static_cast<PL_EDITOR_SETTINGS*>( aCfg );

    m_propertiesFrameWidth = m_propertiesPagelayout->GetSize().x;

    cfg->m_PropertiesFrameWidth = m_propertiesFrameWidth;
    cfg->m_CornerOrigin         = m_originSelectChoice;
    cfg->m_BlackBackground      = GetDrawBgColor() == BLACK;
    cfg->m_LastPaperSize        = GetPageSettings().GetType();
    cfg->m_LastWasPortrait      = GetPageSettings().IsPortrait();
    cfg->m_LastCustomWidth      = PAGE_INFO::GetCustomWidthMils();
    cfg->m_LastCustomHeight     = PAGE_INFO::GetCustomHeightMils();
}


void PL_EDITOR_FRAME::UpdateTitleAndInfo()
{
    wxString title;
    wxString file = GetCurrentFileName();

    title.Printf( _( "Page Layout Editor" ) + wxT( " \u2014 %s" ),
                  file.Length() ? file : _( "no file selected" ) );
    SetTitle( title );
}


wxString PL_EDITOR_FRAME::GetCurrentFileName() const
{
    return BASE_SCREEN::m_PageLayoutDescrFileName;
}


void PL_EDITOR_FRAME::SetCurrentFileName( const wxString& aName )
{
    BASE_SCREEN::m_PageLayoutDescrFileName = aName;
}


void PL_EDITOR_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    m_pageLayout.SetPageSettings( aPageSettings );

    if( GetScreen() )
        GetScreen()->InitDataPoints( aPageSettings.GetSizeIU() );
}


const PAGE_INFO& PL_EDITOR_FRAME::GetPageSettings() const
{
    return m_pageLayout.GetPageSettings();
}


const wxSize PL_EDITOR_FRAME::GetPageSizeIU() const
{
    // this function is only needed because EDA_DRAW_FRAME is not compiled
    // with either -DPCBNEW or -DEESCHEMA, so the virtual is used to route
    // into an application specific source file.
    return m_pageLayout.GetPageSettings().GetSizeIU();
}


const TITLE_BLOCK& PL_EDITOR_FRAME::GetTitleBlock() const
{
    return GetPageLayout().GetTitleBlock();
}


void PL_EDITOR_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    m_pageLayout.SetTitleBlock( aTitleBlock );
}


wxPoint PL_EDITOR_FRAME::ReturnCoordOriginCorner() const
{
    // calculate the position (in page, in iu) of the corner used as coordinate origin
     // coordinate origin can be the paper Top Left corner, or each of 4 page corners
    wxPoint originCoord;

    // To avoid duplicate code, we use a dummy segment starting at 0,0 in relative coord
    WS_DATA_ITEM dummy( WS_DATA_ITEM::WS_SEGMENT );

    switch( m_originSelectChoice )
    {
    default:
    case 0: // Origin = paper Left Top corner
        break;

    case 1: // Origin = page Right Bottom corner
        dummy.SetStart( 0, 0, RB_CORNER );
        originCoord = dummy.GetStartPosUi();
        break;

    case 2: // Origin = page Left Bottom corner
        dummy.SetStart( 0, 0, LB_CORNER );
        originCoord = dummy.GetStartPosUi();
        break;

    case 3: // Origin = page Right Top corner
        dummy.SetStart( 0, 0, RT_CORNER );
        originCoord = dummy.GetStartPosUi();
        break;

    case 4: // Origin = page Left Top corner
        dummy.SetStart( 0, 0, LT_CORNER );
        originCoord = dummy.GetStartPosUi();
        break;
    }

    return originCoord;
}


/*
 * Display the grid status.
 */
void PL_EDITOR_FRAME::DisplayGridMsg()
{
    wxString line;
    wxString gridformatter;

    switch( m_userUnits )
    {
    case EDA_UNITS::INCHES:      gridformatter = "grid %.3f"; break;
    case EDA_UNITS::MILLIMETRES: gridformatter = "grid %.4f"; break;
    default:                     gridformatter = "grid %f";   break;
    }

    double grid = To_User_Unit( m_userUnits, GetCanvas()->GetGAL()->GetGridSize().x );
    line.Printf( gridformatter, grid );

    SetStatusText( line, 4 );
}


void PL_EDITOR_FRAME::UpdateStatusBar()
{
    PL_EDITOR_SCREEN* screen = (PL_EDITOR_SCREEN*) GetScreen();

    if( !screen )
        return;

    // Display Zoom level:
    EDA_DRAW_FRAME::UpdateStatusBar();

    // coordinate origin can be the paper Top Left corner, or each of 4 page corners
    wxPoint originCoord = ReturnCoordOriginCorner();
    SetGridOrigin( originCoord );

    // We need the orientation of axis (sign of coordinates)
    int Xsign = 1;
    int Ysign = 1;

    switch( m_originSelectChoice )
    {
    default:
    case 0: // Origin = paper Left Top corner
        break;

    case 1: // Origin = page Right Bottom corner
        Xsign = -1;
        Ysign = -1;
        break;

    case 2: // Origin = page Left Bottom corner
        Ysign = -1;
         break;

    case 3: // Origin = page Right Top corner
        Xsign = -1;
        break;

    case 4: // Origin = page Left Top corner
        break;
    }

    // Display absolute coordinates:
    VECTOR2D cursorPos = GetCanvas()->GetViewControls()->GetCursorPosition();
    VECTOR2D coord = cursorPos - originCoord;
    double   dXpos = To_User_Unit( GetUserUnits(), coord.x * Xsign );
    double   dYpos = To_User_Unit( GetUserUnits(), coord.y * Ysign );

    wxString pagesizeformatter = _( "Page size: width %.4g height %.4g" );
    wxString absformatter = wxT( "X %.4g  Y %.4g" );
    wxString locformatter = wxT( "dx %.4g  dy %.4g" );

    switch( GetUserUnits() )
    {
    case EDA_UNITS::INCHES:
        SetStatusText( _( "inches" ), 6 );
        break;
    case EDA_UNITS::MILLIMETRES:
        SetStatusText( _( "mm" ), 6 );
        break;
    case EDA_UNITS::UNSCALED:
        SetStatusText( wxEmptyString, 6 );
        break;
    default:             wxASSERT( false );                 break;
    }

    wxString line;

    // Display page size
    #define MILS_TO_MM (25.4/1000)
    DSIZE size = GetPageSettings().GetSizeMils();
    size = size * MILS_TO_MM;
    line.Printf( pagesizeformatter, size.x, size.y );
    SetStatusText( line, 0 );

    // Display abs coordinates
    line.Printf( absformatter, dXpos, dYpos );
    SetStatusText( line, 2 );

    // Display relative coordinates:
    double dx = cursorPos.x - screen->m_LocalOrigin.x;
    double dy = cursorPos.y - screen->m_LocalOrigin.y;
    dXpos = To_User_Unit( GetUserUnits(), dx * Xsign );
    dYpos = To_User_Unit( GetUserUnits(), dy * Ysign );
    line.Printf( locformatter, dXpos, dYpos );
    SetStatusText( line, 3 );

    DisplayGridMsg();

    // Display corner reference for coord origin
    line.Printf( _("coord origin: %s"),
                m_originSelectBox->GetString( m_originSelectChoice ).GetData() );
    SetStatusText( line, 5 );

    // Display units
}


void PL_EDITOR_FRAME::PrintPage( RENDER_SETTINGS* aSettings )
{
    GetScreen()->m_ScreenNumber = GetPageNumberOption() ? 1 : 2;
    WS_DATA_MODEL&     model = WS_DATA_MODEL::GetTheInstance();

    for( WS_DATA_ITEM* dataItem : model.GetItems() )
    {
        // Ensure the scaling factor (used only in printing) of bitmaps is up to date
        if( dataItem->GetType() != WS_DATA_ITEM::WS_BITMAP )
            continue;

        WS_DATA_ITEM_BITMAP* itemBM = static_cast<WS_DATA_ITEM_BITMAP*>( dataItem );
        itemBM->m_ImageBitmap->SetPixelScaleFactor( IU_PER_MILS * 1000
                                                    / itemBM->m_ImageBitmap->GetPPI() );
    }

    PrintWorkSheet( aSettings, GetScreen(), IU_PER_MILS, wxEmptyString );

    GetCanvas()->DisplayWorksheet();
    GetCanvas()->Refresh();
}


PL_DRAW_PANEL_GAL* PL_EDITOR_FRAME::GetCanvas() const
{
    return static_cast<PL_DRAW_PANEL_GAL*>( EDA_DRAW_FRAME::GetCanvas() );
}


void PL_EDITOR_FRAME::HardRedraw()
{
    GetCanvas()->DisplayWorksheet();

    PL_SELECTION_TOOL*  selTool = m_toolManager->GetTool<PL_SELECTION_TOOL>();
    PL_SELECTION&       selection = selTool->GetSelection();
    WS_DATA_ITEM*       item = nullptr;

    if( selection.GetSize() == 1 )
        item = static_cast<WS_DRAW_ITEM_BASE*>( selection.Front() )->GetPeer();

    m_propertiesPagelayout->CopyPrmsFromItemToPanel( item );
    m_propertiesPagelayout->CopyPrmsFromGeneralToPanel();
    GetCanvas()->Refresh();
}


WS_DATA_ITEM* PL_EDITOR_FRAME::AddPageLayoutItem( int aType )
{
    WS_DATA_ITEM * item = NULL;

    switch( aType )
    {
    case WS_DATA_ITEM::WS_TEXT:
        item = new WS_DATA_ITEM_TEXT( wxT( "Text") );
        break;

    case WS_DATA_ITEM::WS_SEGMENT:
        item = new WS_DATA_ITEM( WS_DATA_ITEM::WS_SEGMENT );
        break;

    case WS_DATA_ITEM::WS_RECT:
        item = new WS_DATA_ITEM( WS_DATA_ITEM::WS_RECT );
        break;

    case WS_DATA_ITEM::WS_POLYPOLYGON:
        item = new WS_DATA_ITEM_POLYGONS();
        break;

    case WS_DATA_ITEM::WS_BITMAP:
    {
        wxFileDialog fileDlg( this, _( "Choose Image" ), wxEmptyString, wxEmptyString,
                              _( "Image Files " ) + wxImage::GetImageExtWildcard(), wxFD_OPEN );

        if( fileDlg.ShowModal() != wxID_OK )
            return NULL;

        wxString fullFilename = fileDlg.GetPath();

        if( !wxFileExists( fullFilename ) )
        {
            wxMessageBox( _( "Couldn't load image from \"%s\"" ), fullFilename );
            break;
        }

        BITMAP_BASE* image = new BITMAP_BASE();

        if( !image->ReadImageFile( fullFilename ) )
        {
            wxMessageBox( _( "Couldn't load image from \"%s\"" ), fullFilename );
            delete image;
            break;
        }

        // Set the scale factor for pl_editor (it is set for eeschema by default)
        image->SetPixelScaleFactor( 25400.0 / image->GetPPI() );
        item = new WS_DATA_ITEM_BITMAP( image );
    }
    break;
    }

    if( item == NULL )
        return NULL;

    WS_DATA_MODEL::GetTheInstance().Append( item );
    item->SyncDrawItems( nullptr, GetCanvas()->GetView() );

    return item;
}


void PL_EDITOR_FRAME::OnNewPageLayout()
{
    GetScreen()->ClearUndoRedoList();
    GetScreen()->ClrModify();
    GetCanvas()->DisplayWorksheet();

    m_propertiesPagelayout->CopyPrmsFromItemToPanel( nullptr );
    m_propertiesPagelayout->CopyPrmsFromGeneralToPanel();

    UpdateTitleAndInfo();

    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );

    if( GetCurrentFileName().IsEmpty() )
    {
        // Default shutdown reason until a file is loaded
        SetShutdownBlockReason( _( "New page layout file is unsaved" ) );
    }
    else
    {
        SetShutdownBlockReason( _( "Page layout changes are unsaved" ) );
    }
}


