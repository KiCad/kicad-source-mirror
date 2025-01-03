/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiface_base.h>
#include <pgm_base.h>
#include <bitmaps.h>
#include <core/arraydim.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <panel_hotkeys_editor.h>
#include <confirm.h>
#include <kiplatform/app.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/painter.h>
#include <wildcards_and_files_ext.h>
#include <tool/selection.h>
#include <tool/action_toolbar.h>
#include <tool/editor_conditions.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/picker_tool.h>
#include <tool/zoom_tool.h>
#include <settings/settings_manager.h>
#include "pl_editor_frame.h"
#include "pl_editor_id.h"
#include "pl_editor_settings.h"
#include "properties_frame.h"
#include <toolbars_pl_editor.h>
#include "tools/pl_actions.h"
#include "tools/pl_selection_tool.h"
#include "tools/pl_drawing_tools.h"
#include "tools/pl_edit_tool.h"
#include "tools/pl_point_editor.h"
#include "invoke_pl_editor_dialog.h"
#include "tools/pl_editor_control.h"
#include <view/view_controls.h>

#include <wx/filedlg.h>
#include <wx/print.h>
#include <wx/treebook.h>
#include <wx/msgdlg.h>
#include <wx/log.h>

#ifndef __linux__
#include <navlib/nl_pl_editor_plugin.h>
#else
#include <spacenav/spnav_2d_plugin.h>
#endif


BEGIN_EVENT_TABLE( PL_EDITOR_FRAME, EDA_DRAW_FRAME )
    EVT_MENU( wxID_CLOSE, PL_EDITOR_FRAME::OnExit )
    EVT_MENU( wxID_EXIT, PL_EDITOR_FRAME::OnExit )

    EVT_MENU( wxID_FILE, PL_EDITOR_FRAME::Files_io )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, PL_EDITOR_FRAME::OnFileHistory )
    EVT_MENU( ID_FILE_LIST_CLEAR, PL_EDITOR_FRAME::OnClearFileHistory )

    EVT_CHOICE( ID_SELECT_COORDINATE_ORIGIN, PL_EDITOR_FRAME::OnSelectCoordOriginCorner )
    EVT_CHOICE( ID_SELECT_PAGE_NUMBER, PL_EDITOR_FRAME::OnSelectPage )

    // Drop files event
    EVT_DROP_FILES( PL_EDITOR_FRAME::OnDropFiles )
END_EVENT_TABLE()


PL_EDITOR_FRAME::PL_EDITOR_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        EDA_DRAW_FRAME( aKiway, aParent, FRAME_PL_EDITOR, wxT( "PlEditorFrame" ), wxDefaultPosition,
                        wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, PL_EDITOR_FRAME_NAME,
                        drawSheetIUScale ),
        m_propertiesPagelayout( nullptr ), m_propertiesFrameWidth( 200 ),
        m_originSelectBox( nullptr ), m_originSelectChoice( 0 ), m_pageSelectBox( nullptr ),
        m_mruImagePath( wxEmptyString )
{
    m_maximizeByDefault = true;
    SetUserUnits( EDA_UNITS::MM );

    m_showBorderAndTitleBlock   = true; // true for reference drawings.
    DS_DATA_MODEL::GetTheInstance().m_EditMode = true;
    m_aboutTitle = _HKI( "KiCad Drawing Sheet Editor" );

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pagelayout_editor, 48 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pagelayout_editor, 128 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pagelayout_editor, 256 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pagelayout_editor_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pagelayout_editor_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    // Create GAL canvas
    auto* drawPanel = new PL_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_frameSize,
                                             GetGalDisplayOptions(),
                                             EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE );
    SetCanvas( drawPanel );

    LoadSettings( config() );

    m_acceptedExts.emplace( FILEEXT::DrawingSheetFileExtension, nullptr );
    DragAcceptFiles( true );

    VECTOR2I pageSizeIU = GetPageLayout().GetPageSettings().GetSizeIU( drawSheetIUScale.IU_PER_MILS );
    SetScreen( new BASE_SCREEN( pageSizeIU ) );

    setupTools();
    setupUIConditions();
    ReCreateMenuBar();

    m_toolbarSettings = GetToolbarSettings<PL_EDITOR_TOOLBAR_SETTINGS>( "pl_editor-toolbars" );
    configureToolbars();
    RecreateToolbars();

    wxWindow* stsbar = GetStatusBar();
    int       spacer = KIUI::GetTextSize( wxT( "M" ), stsbar ).x * 2;

    int dims[] = {

        // balance of status bar on far left is set to a default or whatever is left over.
        -1,

        // When using GetTextSize() remember the width of '1' is not the same
        // as the width of '0' unless the font is fixed width, and it usually won't be.

        // zoom:
        KIUI::GetTextSize( wxT( "Z 762000" ), stsbar ).x + spacer,

        // cursor coords
        KIUI::GetTextSize( wxT( "X 0234.567  Y 0234.567" ), stsbar ).x + spacer,

        // delta distances
        KIUI::GetTextSize( wxT( "dx 0234.567  dx 0234.567" ), stsbar ).x + spacer,

        // grid size
        KIUI::GetTextSize( wxT( "grid 0234.567" ), stsbar ).x + spacer,

        // Coord origin (use the bigger message)
        KIUI::GetTextSize( _( "coord origin: Right Bottom page corner" ), stsbar ).x + spacer,

        // units display, Inches is bigger than mm
        KIUI::GetTextSize( _( "Inches" ), stsbar ).x + spacer,

        // constraint mode
        KIUI::GetTextSize( _( "Constrain to H, V, 45" ), stsbar ).x + spacer
    };

    SetStatusWidths( arrayDim( dims ), dims );

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();
    m_propertiesPagelayout = new PROPERTIES_FRAME( this );

    // Rows; layers 4 - 6
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( "TopMainToolbar" )
                      .Top().Layer( 6 ) );
    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( "LeftToolbar" )
                      .Left().Layer( 3 ) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" )
                      .Bottom().Layer( 6 ) );

    // Columns; layers 1 - 3
    m_auimgr.AddPane( m_tbRight, EDA_PANE().VToolbar().Name( "RightToolbar" )
                      .Right().Layer( 2 ) );

    m_auimgr.AddPane( m_propertiesPagelayout, EDA_PANE().Palette().Name( "Props" )
                      .Right().Layer( 3 )
                      .Caption( _( "Properties" ) )
                      .MinSize( m_propertiesPagelayout->GetMinSize() )
                      .BestSize( m_propertiesFrameWidth, -1 ) );

    // Center
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" )
                      .Center() );

    FinishAUIInitialization();

    resolveCanvasType();
    SwitchCanvas( m_canvasType );

    // Add the exit key handler
    setupUnits( config() );

    VECTOR2I originCoord = ReturnCoordOriginCorner();
    SetGridOrigin( originCoord );

    // Initialize the current drawing sheet
#if 0       //start with empty layout
    DS_DATA_MODEL::GetTheInstance().AllowVoidList( true );
    DS_DATA_MODEL::GetTheInstance().ClearList();
#else       // start with the default KiCad layout
    DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( wxEmptyString, nullptr );
#endif
    OnNewDrawingSheet();

    // Ensure the window is on top
    Raise();

    // Register a call to update the toolbar sizes. It can't be done immediately because
    // it seems to require some sizes calculated that aren't yet (at least on GTK).
    CallAfter( [this]()
               {
                   // Ensure the controls on the toolbars all are correctly sized
                    UpdateToolbarControlSizes();
               } );

    try
    {
        if( !m_spaceMouse )
        {
#ifndef __linux__
            m_spaceMouse = std::make_unique<NL_PL_EDITOR_PLUGIN>();
#else
            m_spaceMouse = std::make_unique<SPNAV_2D_PLUGIN>( GetCanvas() );
            m_spaceMouse->SetScale( drawSheetIUScale.IU_PER_MILS / pcbIUScale.IU_PER_MILS );
#endif
        }

        m_spaceMouse->SetCanvas( GetCanvas() );
    }
    catch( const std::system_error& e )
    {
        wxLogTrace( wxT( "KI_TRACE_NAVLIB" ), e.what() );
    }
}


PL_EDITOR_FRAME::~PL_EDITOR_FRAME()
{
    // Ensure m_canvasType is up to date, to save it in config
    m_canvasType = GetCanvas()->GetBackend();

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
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

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
    m_toolManager->InvokeTool( "common.InteractiveSelection" );
}


void PL_EDITOR_FRAME::setupUIConditions()
{
    EDA_DRAW_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( ACTIONS::save,              ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( ACTIONS::undo,              ENABLE( cond.UndoAvailable() ) );
    mgr->SetConditions( ACTIONS::redo,              ENABLE( cond.RedoAvailable() ) );

    mgr->SetConditions( ACTIONS::toggleGrid,        CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::toggleCursorStyle, CHECK( cond.FullscreenCursor() ) );
    mgr->SetConditions( ACTIONS::millimetersUnits,  CHECK( cond.Units( EDA_UNITS::MM ) ) );
    mgr->SetConditions( ACTIONS::inchesUnits,       CHECK( cond.Units( EDA_UNITS::INCH ) ) );
    mgr->SetConditions( ACTIONS::milsUnits,         CHECK( cond.Units( EDA_UNITS::MILS ) ) );

    mgr->SetConditions( ACTIONS::cut,               ENABLE( SELECTION_CONDITIONS::NotEmpty ) );
    mgr->SetConditions( ACTIONS::copy,              ENABLE( SELECTION_CONDITIONS::NotEmpty ) );
    mgr->SetConditions( ACTIONS::paste,             ENABLE( SELECTION_CONDITIONS::Idle && cond.NoActiveTool() ) );
    mgr->SetConditions( ACTIONS::doDelete,          ENABLE( SELECTION_CONDITIONS::NotEmpty ) );

    mgr->SetConditions( ACTIONS::zoomTool,
                        CHECK( cond.CurrentTool( ACTIONS::zoomTool ) ) );
    mgr->SetConditions( ACTIONS::selectionTool,
                        CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );
    mgr->SetConditions( ACTIONS::deleteTool,
                        CHECK( cond.CurrentTool( ACTIONS::deleteTool ) ) );

    mgr->SetConditions( PL_ACTIONS::drawLine,
                        CHECK( cond.CurrentTool( PL_ACTIONS::drawLine ) ) );
    mgr->SetConditions( PL_ACTIONS::drawRectangle,
                        CHECK( cond.CurrentTool( PL_ACTIONS::drawRectangle ) ) );
    mgr->SetConditions( PL_ACTIONS::placeText,
                        CHECK( cond.CurrentTool( PL_ACTIONS::placeText ) ) );
    mgr->SetConditions( PL_ACTIONS::placeImage,
                        CHECK( cond.CurrentTool( PL_ACTIONS::placeImage ) ) );

    // Not a tool, just a way to activate the action
    mgr->SetConditions( PL_ACTIONS::appendImportedDrawingSheet,
                        CHECK( SELECTION_CONDITIONS::ShowNever ) );

    auto titleBlockNormalMode =
            [] ( const SELECTION& )
            {
                return DS_DATA_MODEL::GetTheInstance().m_EditMode == false;
            };

    auto titleBlockEditMode =
            [] ( const SELECTION& )
            {
                return DS_DATA_MODEL::GetTheInstance().m_EditMode == true;
            };

    mgr->SetConditions( PL_ACTIONS::layoutNormalMode, CHECK( titleBlockNormalMode ) );
    mgr->SetConditions( PL_ACTIONS::layoutEditMode,   CHECK( titleBlockEditMode ) );

#undef CHECK
#undef ENABLE
}


bool PL_EDITOR_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    wxString fn = aFileSet[0];

    if( !LoadDrawingSheetFile( fn ) )
    {
        wxMessageBox( wxString::Format( _( "Error loading drawing sheet '%s'." ), fn ) );
        return false;
    }
    else
    {
        OnNewDrawingSheet();
        return true;
    }
}


void PL_EDITOR_FRAME::OnModify()
{
    // Must be called after a change in order to set the "modify" flag and update
    // the frame title.
    EDA_BASE_FRAME::OnModify();

    GetScreen()->SetContentModified();

    if( m_isClosing )
        return;

    UpdateTitleAndInfo();
}


bool PL_EDITOR_FRAME::IsContentModified() const
{
    return GetScreen() && GetScreen()->IsContentModified();
}


void PL_EDITOR_FRAME::OnExit( wxCommandEvent& aEvent )
{
    if( aEvent.GetId() == wxID_EXIT )
        Kiway().OnKiCadExit();

    if( aEvent.GetId() == wxID_CLOSE || Kiface().IsSingle() )
        Close( false );
}


bool PL_EDITOR_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    // Shutdown blocks must be determined and vetoed as early as possible
    if( KIPLATFORM::APP::SupportsShutdownBlockReason()
            && aEvent.GetId() == wxEVT_QUERY_END_SESSION
            && IsContentModified() )
    {
        return false;
    }

    if( IsContentModified() )
    {
        wxFileName filename = GetCurrentFileName();
        wxString   msg      = _( "Save changes to '%s' before closing?" );

        if( !HandleUnsavedChanges( this, wxString::Format( msg, filename.GetFullName() ),
                                   [&]() -> bool
                                   {
                                       return saveCurrentPageLayout();
                                   } ) )
        {
            return false;
        }
    }

    return true;
}


void PL_EDITOR_FRAME::doCloseWindow()
{
    // do not show the window because we do not want any paint event
    Show( false );

    // clean up the data before the view is destroyed
    DS_DATA_MODEL::GetTheInstance().ClearList();

    // On Linux, m_propertiesPagelayout must be destroyed
    // before deleting the main frame to avoid a crash when closing
    m_propertiesPagelayout->Destroy();
    Destroy();
}


void PL_EDITOR_FRAME::OnSelectPage( wxCommandEvent& event )
{
    KIGFX::VIEW* view = GetCanvas()->GetView();
    view->SetLayerVisible( LAYER_DRAWINGSHEET_PAGE1, m_pageSelectBox->GetSelection() == 0 );
    view->SetLayerVisible( LAYER_DRAWINGSHEET_PAGEn, m_pageSelectBox->GetSelection() == 1 );
    GetCanvas()->Refresh();
}


void PL_EDITOR_FRAME::OnSelectCoordOriginCorner( wxCommandEvent& event )
{
    m_originSelectChoice = m_originSelectBox->GetSelection();
    UpdateStatusBar();  // Update grid origin
    GetCanvas()->DisplayDrawingSheet();
    GetCanvas()->Refresh();
}


void PL_EDITOR_FRAME::ToPrinter( bool doPreview )
{
    // static print data and page setup data, to remember settings during the session
    static wxPrintData* s_PrintData;
    static wxPageSetupDialogData* s_pageSetupData = nullptr;

    const PAGE_INFO& pageInfo = GetPageSettings();

    if( s_PrintData == nullptr )  // First print
    {
        s_PrintData = new wxPrintData();
        s_PrintData->SetQuality( wxPRINT_QUALITY_HIGH );      // Default resolution = HIGH;
    }

    if( !s_PrintData->Ok() )
    {
        wxMessageBox( _( "Error Init Printer info" ) );
        return;
    }

    if( s_pageSetupData == nullptr )
        s_pageSetupData = new wxPageSetupDialogData( *s_PrintData );

    s_pageSetupData->SetPaperId( pageInfo.GetPaperId() );
    s_pageSetupData->GetPrintData().SetOrientation( pageInfo.GetWxOrientation() );

    if( pageInfo.IsCustom() )
    {
        if( pageInfo.IsPortrait() )
            s_pageSetupData->SetPaperSize( wxSize( EDA_UNIT_UTILS::Mils2mm( pageInfo.GetWidthMils() ),
                                                   EDA_UNIT_UTILS::Mils2mm( pageInfo.GetHeightMils() ) ) );
        else
            s_pageSetupData->SetPaperSize( wxSize( EDA_UNIT_UTILS::Mils2mm( pageInfo.GetHeightMils() ),
                                                   EDA_UNIT_UTILS::Mils2mm( pageInfo.GetWidthMils() ) ) );
    }

    *s_PrintData = s_pageSetupData->GetPrintData();

    if( doPreview )
        InvokeDialogPrintPreview( this, s_PrintData );
    else
        InvokeDialogPrint( this, s_PrintData, s_pageSetupData );
}


const BOX2I PL_EDITOR_FRAME::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    BOX2I rv( VECTOR2I( 0, 0 ), GetPageLayout().GetPageSettings().GetSizeIU( drawSheetIUScale.IU_PER_MILS ) );
    return rv;
}


void PL_EDITOR_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

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
    wxFileName file( GetCurrentFileName() );

    if( IsContentModified() )
        title = wxT( "*" );

    if( file.IsOk() )
        title += file.GetName();
    else
        title += _( "[no drawing sheet loaded]" );

    title += wxT( " \u2014 " ) + _( "Drawing Sheet Editor" ),

    SetTitle( title );
}


wxString PL_EDITOR_FRAME::GetCurrentFileName() const
{
    return BASE_SCREEN::m_DrawingSheetFileName;
}


void PL_EDITOR_FRAME::SetCurrentFileName( const wxString& aName )
{
    BASE_SCREEN::m_DrawingSheetFileName = aName;
}


void PL_EDITOR_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    m_pageLayout.SetPageSettings( aPageSettings );

    if( GetScreen() )
        GetScreen()->InitDataPoints( aPageSettings.GetSizeIU( drawSheetIUScale.IU_PER_MILS ) );
}


const PAGE_INFO& PL_EDITOR_FRAME::GetPageSettings() const
{
    return m_pageLayout.GetPageSettings();
}


const VECTOR2I PL_EDITOR_FRAME::GetPageSizeIU() const
{
    // this function is only needed because EDA_DRAW_FRAME is not compiled
    // with either -DPCBNEW or -DEESCHEMA, so the virtual is used to route
    // into an application specific source file.
    return m_pageLayout.GetPageSettings().GetSizeIU( drawSheetIUScale.IU_PER_MILS );
}


const TITLE_BLOCK& PL_EDITOR_FRAME::GetTitleBlock() const
{
    return GetPageLayout().GetTitleBlock();
}


void PL_EDITOR_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    m_pageLayout.SetTitleBlock( aTitleBlock );
}


void PL_EDITOR_FRAME::CommonSettingsChanged( int aFlags )
{
    EDA_DRAW_FRAME::CommonSettingsChanged( aFlags );

    PL_EDITOR_SETTINGS* cfg = GetAppSettings<PL_EDITOR_SETTINGS>( "pl_editor" );
    COLOR_SETTINGS*     colors = ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );

    // Update gal display options like cursor shape, grid options:
    GetGalDisplayOptions().ReadWindowSettings( cfg->m_Window );

    GetCanvas()->GetView()->GetPainter()->GetSettings()->LoadColors( colors );

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::COLOR );
    GetCanvas()->ForceRefresh();

    RecreateToolbars();
    Layout();
    SendSizeEvent();
}


VECTOR2I PL_EDITOR_FRAME::ReturnCoordOriginCorner() const
{
    // calculate the position (in page, in iu) of the corner used as coordinate origin
     // coordinate origin can be the paper Top Left corner, or each of 4 page corners
    VECTOR2I originCoord;

    // To avoid duplicate code, we use a dummy segment starting at 0,0 in relative coord
    DS_DATA_ITEM dummy( DS_DATA_ITEM::DS_SEGMENT );

    switch( m_originSelectChoice )
    {
    default:
    case 0: // Origin = paper Left Top corner
        break;

    case 1: // Origin = page Right Bottom corner
        dummy.SetStart( 0, 0, RB_CORNER );
        originCoord = dummy.GetStartPosIU();
        break;

    case 2: // Origin = page Left Bottom corner
        dummy.SetStart( 0, 0, LB_CORNER );
        originCoord = dummy.GetStartPosIU();
        break;

    case 3: // Origin = page Right Top corner
        dummy.SetStart( 0, 0, RT_CORNER );
        originCoord = dummy.GetStartPosIU();
        break;

    case 4: // Origin = page Left Top corner
        dummy.SetStart( 0, 0, LT_CORNER );
        originCoord = dummy.GetStartPosIU();
        break;
    }

    return originCoord;
}


void PL_EDITOR_FRAME::DisplayGridMsg()
{
    wxString line;
    wxString gridformatter;

    switch( GetUserUnits() )
    {
    case EDA_UNITS::INCH: gridformatter = wxS( "grid %.3f" ); break;
    case EDA_UNITS::MM:   gridformatter = wxS( "grid %.4f" ); break;
    default:              gridformatter = wxS( "grid %f" );   break;
    }

    double grid = EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, GetUserUnits(),
                                                  GetCanvas()->GetGAL()->GetGridSize().x );
    line.Printf( gridformatter, grid );

    SetStatusText( line, 4 );
}


void PL_EDITOR_FRAME::UpdateStatusBar()
{
    // Display Zoom level:
    SetStatusText( GetZoomLevelIndicator(), 1 );

    // coordinate origin can be the paper Top Left corner, or each of 4 page corners
    VECTOR2I originCoord = ReturnCoordOriginCorner();
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
    double   dXpos =
            EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, GetUserUnits(), coord.x * Xsign );
    double dYpos =
            EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, GetUserUnits(), coord.y * Ysign );

    wxString absformatter = wxT( "X %.4g  Y %.4g" );
    wxString locformatter = wxT( "dx %.4g  dy %.4g" );

    switch( GetUserUnits() )
    {
    case EDA_UNITS::INCH:     SetStatusText( _( "inches" ), 6 ); break;
    case EDA_UNITS::MILS:     SetStatusText( _( "mils" ), 6 );   break;
    case EDA_UNITS::MM:       SetStatusText( _( "mm" ), 6 );     break;
    case EDA_UNITS::UNSCALED: SetStatusText( wxEmptyString, 6 ); break;
    default:                  wxASSERT( false );                 break;
    }

    wxString line;

    // Display abs coordinates
    line.Printf( absformatter, dXpos, dYpos );
    SetStatusText( line, 2 );

    // Display relative coordinates:
    if( GetScreen() )
    {
        double dx = cursorPos.x - GetScreen()->m_LocalOrigin.x;
        double dy = cursorPos.y - GetScreen()->m_LocalOrigin.y;
        dXpos = EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, GetUserUnits(), dx * Xsign );
        dYpos = EDA_UNIT_UTILS::UI::ToUserUnit( drawSheetIUScale, GetUserUnits(), dy * Ysign );
        line.Printf( locformatter, dXpos, dYpos );
        SetStatusText( line, 3 );
    }

    DisplayGridMsg();

    // Display corner reference for coord origin
    line.Printf( _("coord origin: %s"),
                 m_originSelectBox->GetString( m_originSelectChoice ).GetData() );
    SetStatusText( line, 5 );
}


PL_DRAW_PANEL_GAL* PL_EDITOR_FRAME::GetCanvas() const
{
    return static_cast<PL_DRAW_PANEL_GAL*>( EDA_DRAW_FRAME::GetCanvas() );
}


SELECTION& PL_EDITOR_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<PL_SELECTION_TOOL>()->GetSelection();
}


void PL_EDITOR_FRAME::HardRedraw()
{
    GetCanvas()->DisplayDrawingSheet();

    PL_SELECTION_TOOL*  selTool = m_toolManager->GetTool<PL_SELECTION_TOOL>();
    PL_SELECTION&       selection = selTool->GetSelection();
    DS_DATA_ITEM*       item = nullptr;

    if( selection.GetSize() == 1 )
        item = static_cast<DS_DRAW_ITEM_BASE*>( selection.Front() )->GetPeer();

    m_propertiesPagelayout->CopyPrmsFromItemToPanel( item );
    m_propertiesPagelayout->CopyPrmsFromGeneralToPanel();
    UpdateMsgPanelInfo();
    GetCanvas()->Refresh();
}


DS_DATA_ITEM* PL_EDITOR_FRAME::AddDrawingSheetItem( int aType )
{
    DS_DATA_ITEM * item = nullptr;

    switch( aType )
    {
    case DS_DATA_ITEM::DS_TEXT:
        item = new DS_DATA_ITEM_TEXT( wxT( "Text") );
        break;

    case DS_DATA_ITEM::DS_SEGMENT:
        item = new DS_DATA_ITEM( DS_DATA_ITEM::DS_SEGMENT );
        break;

    case DS_DATA_ITEM::DS_RECT:
        item = new DS_DATA_ITEM( DS_DATA_ITEM::DS_RECT );
        break;

    case DS_DATA_ITEM::DS_POLYPOLYGON:
        item = new DS_DATA_ITEM_POLYGONS();
        break;

    case DS_DATA_ITEM::DS_BITMAP:
    {
        wxFileDialog fileDlg( this, _( "Choose Image" ), m_mruImagePath, wxEmptyString,
                              _( "Image Files" ) + wxS( " " ) + wxImage::GetImageExtWildcard(),
                              wxFD_OPEN );

        if( fileDlg.ShowModal() != wxID_OK )
            return nullptr;

        wxString fullFilename = fileDlg.GetPath();
        m_mruImagePath = wxPathOnly( fullFilename );

        if( !wxFileExists( fullFilename ) )
        {
            wxMessageBox( _( "Could not load image from '%s'." ), fullFilename );
            break;
        }

        BITMAP_BASE* image = new BITMAP_BASE();

        if( !image->ReadImageFile( fullFilename ) )
        {
            wxMessageBox( _( "Could not load image from '%s'." ), fullFilename );
            delete image;
            break;
        }

        // Set the scale factor for pl_editor (it is set for Eeschema by default)
        image->SetPixelSizeIu( drawSheetIUScale.IU_PER_MILS * 1000.0 / image->GetPPI() );
        item = new DS_DATA_ITEM_BITMAP( image );
    }
    break;
    }

    if( item == nullptr )
        return nullptr;

    DS_DATA_MODEL::GetTheInstance().Append( item );
    item->SyncDrawItems( nullptr, GetCanvas()->GetView() );

    return item;
}


void PL_EDITOR_FRAME::OnNewDrawingSheet()
{
    ClearUndoRedoList();
    GetScreen()->SetContentModified( false );
    GetCanvas()->DisplayDrawingSheet();

    m_propertiesPagelayout->CopyPrmsFromItemToPanel( nullptr );
    m_propertiesPagelayout->CopyPrmsFromGeneralToPanel();

    UpdateTitleAndInfo();

    m_toolManager->RunAction( ACTIONS::zoomFitScreen );

    if( GetCurrentFileName().IsEmpty() )
    {
        // Default shutdown reason until a file is loaded
        KIPLATFORM::APP::SetShutdownBlockReason( this, _( "New drawing sheet file is unsaved" ) );
    }
    else
    {
        KIPLATFORM::APP::SetShutdownBlockReason( this, _( "Drawing sheet changes are unsaved" ) );
    }
}


void PL_EDITOR_FRAME::ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    UNDO_REDO_CONTAINER& list = ( whichList == UNDO_LIST ) ? m_undoList : m_redoList;

    if( aItemCount < 0 )
    {
        list.ClearCommandList();
    }
    else
    {
        for( int ii = 0; ii < aItemCount; ii++ )
        {
            if( list.m_CommandsList.size() == 0 )
                break;

            PICKED_ITEMS_LIST* curr_cmd = list.m_CommandsList[0];
            list.m_CommandsList.erase( list.m_CommandsList.begin() );

            curr_cmd->ClearListAndDeleteItems( []( EDA_ITEM* aItem )
                                               {
                                                   delete aItem;
                                               } );
            delete curr_cmd;    // Delete command
        }
    }
}


bool PL_EDITOR_FRAME::GetPageNumberOption() const
{
    return m_pageSelectBox->GetSelection() == 0;
}

#if 1
void PL_EDITOR_FRAME::UpdateMsgPanelInfo()
{
    VECTOR2D size = GetPageSettings().GetSizeIU( drawSheetIUScale.IU_PER_MILS );

    std::vector<MSG_PANEL_ITEM> msgItems;
    msgItems.emplace_back( _( "Page Width" ), MessageTextFromValue( size.x ) );
    msgItems.emplace_back( _( "Page Height" ), MessageTextFromValue( size.y ) );

    SetMsgPanel( msgItems );
}
#endif

void PL_EDITOR_FRAME::handleActivateEvent( wxActivateEvent& aEvent )
{
    EDA_DRAW_FRAME::handleActivateEvent(aEvent);

    if( m_spaceMouse )
        m_spaceMouse->SetFocus( aEvent.GetActive() );
}


void PL_EDITOR_FRAME::handleIconizeEvent( wxIconizeEvent& aEvent )
{
    EDA_DRAW_FRAME::handleIconizeEvent(aEvent);

    if( m_spaceMouse )
        m_spaceMouse->SetFocus( false );
}
