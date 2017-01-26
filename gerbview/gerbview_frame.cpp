/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file gerbview_frame.cpp
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <wxstruct.h>
#include <class_drawpanel.h>
#include <build_version.h>
#include <trigo.h>
#include <base_units.h>
#include <colors_selection.h>
#include <class_gbr_layer_box_selector.h>
#include <msgpanel.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <hotkeys.h>
#include <class_gerber_file_image.h>
#include <class_gerber_file_image_list.h>
#include <dialog_helpers.h>
#include <class_DCodeSelectionbox.h>
#include <class_gerbview_layer_widget.h>


// Config keywords
static const wxString   cfgShowPageSizeOption( wxT( "PageSizeOpt" ) );
static const wxString   cfgShowDCodes( wxT( "ShowDCodesOpt" ) );
static const wxString   cfgShowNegativeObjects( wxT( "ShowNegativeObjectsOpt" ) );
static const wxString   cfgShowBorderAndTitleBlock( wxT( "ShowBorderAndTitleBlock" ) );


GERBVIEW_FRAME::GERBVIEW_FRAME( KIWAY* aKiway, wxWindow* aParent ):
    EDA_DRAW_FRAME( aKiway, aParent, FRAME_GERBER, wxT( "GerbView" ),
        wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, GERBVIEW_FRAME_NAME )
{
    m_auxiliaryToolBar = NULL;
    m_colorsSettings = &g_ColorsSettings;
    m_gerberLayout = NULL;
    m_zoomLevelCoeff = ZOOM_FACTOR( 110 );   // Adjusted to roughly displays zoom level = 1
                                             // when the screen shows a 1:1 image
                                             // obviously depends on the monitor,
                                             // but this is an acceptable value

    PAGE_INFO pageInfo( wxT( "GERBER" ) );
    SetPageSettings( pageInfo );

    m_show_layer_manager_tools = true;

    m_showAxis = true;                      // true to show X and Y axis on screen
    m_showBorderAndTitleBlock = false;      // true for reference drawings.
    m_hotkeysDescrList = GerbviewHokeysDescr;
    m_SelLayerBox   = NULL;
    m_DCodeSelector = NULL;
    m_displayMode   = 0;
    m_drillFileHistory.SetBaseId( ID_GERBVIEW_DRILL_FILE1 );
    m_zipFileHistory.SetBaseId( ID_GERBVIEW_ZIP_FILE1 );

    if( m_canvas )
        m_canvas->SetEnableBlockCommands( true );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_gerbview_xpm ) );
    SetIcon( icon );

    SetLayout( new GBR_LAYOUT() );

    SetVisibleLayers( -1 );         // All draw layers visible.

    SetScreen( new GBR_SCREEN( GetPageSettings().GetSizeIU() ) );

    // Create the PCB_LAYER_WIDGET *after* SetLayout():
    wxFont  font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    int     pointSize       = font.GetPointSize();
    int     screenHeight    = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );

    if( screenHeight <= 900 )
        pointSize = (pointSize * 8) / 10;

    m_LayersManager = new GERBER_LAYER_WIDGET( this, m_canvas, pointSize );

    // LoadSettings() *after* creating m_LayersManager, because LoadSettings()
    // initialize parameters in m_LayersManager
    LoadSettings( config() );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    if( m_LastGridSizeId < 0 )
        m_LastGridSizeId = 0;
    if( m_LastGridSizeId > ID_POPUP_GRID_LEVEL_0_0_1MM-ID_POPUP_GRID_LEVEL_1000 )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_0_0_1MM-ID_POPUP_GRID_LEVEL_1000;
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateOptToolbar();
    ReCreateAuxiliaryToolbar();

    m_auimgr.SetManagedWindow( this );

    EDA_PANEINFO    horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO    vert;
    vert.VerticalToolbarPane();

    EDA_PANEINFO    mesg;
    mesg.MessageToolbarPane();

    // Create a wxAuiPaneInfo for the Layers Manager, not derived from the template.
    // the Layers Manager is floatable, but initially docked at far right
    EDA_PANEINFO    lyrs;
    lyrs.LayersToolbarPane();
    lyrs.MinSize( m_LayersManager->GetBestSize() );
    lyrs.BestSize( m_LayersManager->GetBestSize() );
    lyrs.Caption( _( "Visibles" ) );
    lyrs.TopDockable( false ).BottomDockable( false );


    if( m_mainToolBar )
        m_auimgr.AddPane( m_mainToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_mainToolBar" ) ).Top().Row( 0 ) );

    if( m_auxiliaryToolBar )    // the auxiliary horizontal toolbar, that shows component and netname lists
    {
        m_auimgr.AddPane( m_auxiliaryToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_auxiliaryToolBar" ) ).Top().Row( 1 ) );
    }

    if( m_drawToolBar )
        m_auimgr.AddPane( m_drawToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_drawToolBar" ) ).Right().Row( 1 ) );

    m_auimgr.AddPane( m_LayersManager,
                      lyrs.Name( wxT( "m_LayersManagerToolBar" ) ).Right().Layer( 0 ) );

    if( m_optionsToolBar )
        m_auimgr.AddPane( m_optionsToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_optionsToolBar" ) ).Left() );

    if( m_canvas )
        m_auimgr.AddPane( m_canvas,
                          wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    if( m_messagePanel )
        m_auimgr.AddPane( m_messagePanel,
                          wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom().Layer( 10 ) );

    ReFillLayerWidget();                // this is near end because contents establish size
    m_LayersManager->ReFillRender();    // Update colors in Render after the config is read
    m_auimgr.Update();

    setActiveLayer( 0, true );
    Zoom_Automatique( false );           // Gives a default zoom value
    UpdateTitleAndInfo();
}


GERBVIEW_FRAME::~GERBVIEW_FRAME()
{
}


void GERBVIEW_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    Destroy();
}


bool GERBVIEW_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
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

        for( unsigned i=0;  i<limit;  ++i, ++layer )
        {
            setActiveLayer( layer );

            // Try to guess the type of file by its ext
            // if it is .drl (Kicad files), it is a drill file
            wxFileName fn( aFileSet[i] );
            wxString ext = fn.GetExt();

            if( ext == "drl" )
                LoadExcellonFiles( aFileSet[i] );
            else
                LoadGerberFiles( aFileSet[i] );
        }
    }

    Zoom_Automatique( true );        // Zoom fit in frame

    UpdateTitleAndInfo();

    return true;
}


double GERBVIEW_FRAME::BestZoom()
{
    EDA_RECT bbox = GetGerberLayout()->ComputeBoundingBox();

    // gives a size to bbox (current page size), if no item in list
    if( bbox.GetWidth() == 0 || bbox.GetHeight() == 0 )
    {
        wxSize pagesize = GetPageSettings().GetSizeMils();
        bbox.SetSize( wxSize( Mils2iu( pagesize.x ), Mils2iu( pagesize.y ) ) );
    }

    // Compute best zoom:
    wxSize  size = m_canvas->GetClientSize();
    double  x   = (double) bbox.GetWidth() / (double) size.x;
    double  y   = (double) bbox.GetHeight() / (double) size.y;
    double  best_zoom = std::max( x, y ) * 1.1;

    SetScrollCenterPosition( bbox.Centre() );

    return best_zoom;
}


void GERBVIEW_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    // was: wxGetApp().ReadCurrentSetupValues( GetConfigurationSettings() );
    wxConfigLoadSetups( aCfg, GetConfigurationSettings() );

    PAGE_INFO pageInfo( wxT( "GERBER" ) );

    aCfg->Read( cfgShowBorderAndTitleBlock, &m_showBorderAndTitleBlock, false );

    if( m_showBorderAndTitleBlock )
    {
        wxString pageType;
        aCfg->Read( cfgShowPageSizeOption, &pageType, wxT( "GERBER" ) );
        pageInfo.SetType( pageType );
    }

    SetPageSettings( pageInfo );

    GetScreen()->InitDataPoints( pageInfo.GetSizeIU() );

    bool tmp;
    aCfg->Read( cfgShowDCodes, &tmp, true );
    SetElementVisibility( DCODES_VISIBLE, tmp );
    aCfg->Read( cfgShowNegativeObjects, &tmp, false );
    SetElementVisibility( NEGATIVE_OBJECTS_VISIBLE, tmp );

    // because we have more than one file history, we must read this one
    // using a specific path
    aCfg->SetPath( wxT( "drl_files" ) );
    m_drillFileHistory.Load( *aCfg );
    aCfg->SetPath( wxT( ".." ) );

    // because we have more than one file history, we must read this one
    // using a specific path
    aCfg->SetPath( wxT( "zip_files" ) );
    m_zipFileHistory.Load( *aCfg );
    aCfg->SetPath( wxT( ".." ) );
}


void GERBVIEW_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    // was: wxGetApp().SaveCurrentSetupValues( GetConfigurationSettings() );
    wxConfigSaveSetups( aCfg, GetConfigurationSettings() );

    aCfg->Write( cfgShowPageSizeOption, GetPageSettings().GetType() );
    aCfg->Write( cfgShowBorderAndTitleBlock, m_showBorderAndTitleBlock );
    aCfg->Write( cfgShowDCodes, IsElementVisible( DCODES_VISIBLE ) );
    aCfg->Write( cfgShowNegativeObjects,
                 IsElementVisible( NEGATIVE_OBJECTS_VISIBLE ) );

    // Save the drill file history list.
    // Because we have  more than one file history, we must save this one
    // in a specific path
    aCfg->SetPath( wxT( "drl_files" ) );
    m_drillFileHistory.Save( *aCfg );
    aCfg->SetPath( wxT( ".." ) );

    // Save the zip file history list.
    aCfg->SetPath( wxT( "zip_files" ) );
    m_zipFileHistory.Save( *aCfg );
    aCfg->SetPath( wxT( ".." ) );
}


void GERBVIEW_FRAME::ReFillLayerWidget()
{
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


void GERBVIEW_FRAME::SetElementVisibility( GERBER_VISIBLE_ID aItemIdVisible,
                                           bool aNewState )
{
    switch( aItemIdVisible )
    {
    case DCODES_VISIBLE:
        m_DisplayOptions.m_DisplayDCodes = aNewState;
        break;

    case NEGATIVE_OBJECTS_VISIBLE:
        m_DisplayOptions.m_DisplayNegativeObjects = aNewState;
        break;

    case GERBER_GRID_VISIBLE:
        SetGridVisibility( aNewState );
        break;

    default:
        wxLogDebug( wxT( "GERBVIEW_FRAME::SetElementVisibility(): bad arg %d" ), aItemIdVisible );
    }

    m_LayersManager->SetRenderState( aItemIdVisible, aNewState );
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
    m_LayersManager->SelectLayer( getActiveLayer() );
    UpdateTitleAndInfo();
}


void GERBVIEW_FRAME::syncLayerBox( bool aRebuildLayerBox )
{
    if( aRebuildLayerBox )
        m_SelLayerBox->Resync();

    m_SelLayerBox->SetSelection( getActiveLayer() );

    int dcodeSelected = -1;
    GERBER_FILE_IMAGE*   gerber = GetGbrImage( getActiveLayer() );

    if( gerber )
        dcodeSelected = gerber->m_Selected_Tool;

    if( m_DCodeSelector )
    {
        updateDCodeSelectBox();
        m_DCodeSelector->SetDCodeSelection( dcodeSelected );
        m_DCodeSelector->Enable( gerber != NULL );
    }

    UpdateTitleAndInfo();
}


void GERBVIEW_FRAME::Liste_D_Codes()
{
    int             ii, jj;
    wxString        Line;
    wxArrayString   list;
    double          scale = g_UserUnit == INCHES ? IU_PER_MILS * 1000 : IU_PER_MM;
    int       curr_layer = getActiveLayer();

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

        const char* units = g_UserUnit == INCHES ? "\"" : "mm";

        for( ii = 0, jj = 1; ii < TOOLS_MAX_COUNT; ii++ )
        {
            D_CODE* pt_D_code = gerber->GetDCODE( ii + FIRST_DCODE, false );

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


void GERBVIEW_FRAME::UpdateTitleAndInfo()
{
    GERBER_FILE_IMAGE* gerber = GetGbrImage( getActiveLayer() );

    // Display the gerber filename
    if( gerber == NULL )
    {
        SetTitle( "GerbView" );

        SetStatusText( wxEmptyString, 0 );

        wxString info;
        info.Printf( _( "Drawing layer %d not in use" ), getActiveLayer() + 1 );
        m_TextInfo->SetValue( info );

        if( EnsureTextCtrlWidth( m_TextInfo, &info ) )  // Resized
           m_auimgr.Update();

        ClearMsgPanel();
        return;
    }
    else
    {
        wxString title;
        title.Printf( L"GerbView \u2014 %s%s",
                gerber->m_FileName,
                gerber->m_IsX2_file
                    ? " " + _( "(with X2 attributes)" )
                    : wxString( wxEmptyString ) );
        SetTitle( title );

        gerber->DisplayImageInfo( this );

        // Display Image Name and Layer Name (from the current gerber data):
        wxString status;
        status.Printf( _( "Image name: '%s'  Layer name: '%s'" ),
                GetChars( gerber->m_ImageName ),
                GetChars( gerber->GetLayerParams().m_LayerName ) );
        SetStatusText( status, 0 );

        // Display data format like fmt in X3.4Y3.4 no LZ or fmt mm X2.3 Y3.5 no TZ in main toolbar
        wxString info;
        info.Printf( wxT( "fmt: %s X%d.%d Y%d.%d no %cZ" ),
                gerber->m_GerbMetric ? wxT( "mm" ) : wxT( "in" ),
                gerber->m_FmtLen.x - gerber->m_FmtScale.x, gerber->m_FmtScale.x,
                gerber->m_FmtLen.y - gerber->m_FmtScale.y, gerber->m_FmtScale.y,
                gerber->m_NoTrailingZeros ? 'T' : 'L' );

        if( gerber->m_IsX2_file )
            info << wxT(" ") << _( "X2 attr" );

        m_TextInfo->SetValue( info );

        if( EnsureTextCtrlWidth( m_TextInfo, &info ) )  // Resized
            m_auimgr.Update();
    }
}


bool GERBVIEW_FRAME::IsElementVisible( GERBER_VISIBLE_ID aItemIdVisible ) const
{
    switch( aItemIdVisible )
    {
    case DCODES_VISIBLE:
        return m_DisplayOptions.m_DisplayDCodes;
        break;

    case NEGATIVE_OBJECTS_VISIBLE:
        return m_DisplayOptions.m_DisplayNegativeObjects;
        break;

    case GERBER_GRID_VISIBLE:
        return IsGridVisible();
        break;

    default:
        wxLogDebug( wxT( "GERBVIEW_FRAME::IsElementVisible(): bad arg %d" ), aItemIdVisible );
    }

    return true;
}


long GERBVIEW_FRAME::GetVisibleLayers() const
{
    return -1;    // TODO
}


void GERBVIEW_FRAME::SetVisibleLayers( long aLayerMask )
{
//    GetGerberLayout()->SetVisibleLayers( aLayerMask );
}


bool GERBVIEW_FRAME::IsLayerVisible( int aLayer ) const
{
    if( ! m_DisplayOptions.m_IsPrinting )
        return m_LayersManager->IsLayerVisible( aLayer );
    else
        return GetGerberLayout()->IsLayerPrintable( aLayer );
}


EDA_COLOR_T GERBVIEW_FRAME::GetVisibleElementColor( GERBER_VISIBLE_ID aItemIdVisible ) const
{
    EDA_COLOR_T color = UNSPECIFIED_COLOR;

    switch( aItemIdVisible )
    {
    case NEGATIVE_OBJECTS_VISIBLE:
    case DCODES_VISIBLE:
        color = m_colorsSettings->GetItemColor( aItemIdVisible );
        break;

    case GERBER_GRID_VISIBLE:
        color = GetGridColor();
        break;

    default:
        wxLogDebug( wxT( "GERBVIEW_FRAME::GetVisibleElementColor(): bad arg %d" ),
                    (int)aItemIdVisible );
    }

    return color;
}


void GERBVIEW_FRAME::SetGridVisibility( bool aVisible )
{
    EDA_DRAW_FRAME::SetGridVisibility( aVisible );
    m_LayersManager->SetRenderState( GERBER_GRID_VISIBLE, aVisible );
}


void GERBVIEW_FRAME::SetVisibleElementColor( GERBER_VISIBLE_ID aItemIdVisible,
                                             EDA_COLOR_T aColor )
{
    switch( aItemIdVisible )
    {
    case NEGATIVE_OBJECTS_VISIBLE:
    case DCODES_VISIBLE:
        m_colorsSettings->SetItemColor( aItemIdVisible, aColor );
        break;

    case GERBER_GRID_VISIBLE:
        SetGridColor( aColor );
        m_colorsSettings->SetItemColor( aItemIdVisible, aColor );
        break;

    default:
        wxLogDebug( wxT( "GERBVIEW_FRAME::SetVisibleElementColor(): bad arg %d" ),
                    (int) aItemIdVisible );
    }
}

EDA_COLOR_T GERBVIEW_FRAME::GetNegativeItemsColor() const
{
    if( IsElementVisible( NEGATIVE_OBJECTS_VISIBLE ) )
        return GetVisibleElementColor( NEGATIVE_OBJECTS_VISIBLE );
    else
        return GetDrawBgColor();
}


EDA_COLOR_T GERBVIEW_FRAME::GetLayerColor( int aLayer ) const
{
    return m_colorsSettings->GetLayerColor( aLayer );
}


void GERBVIEW_FRAME::SetLayerColor( int aLayer, EDA_COLOR_T aColor )
{
    m_colorsSettings->SetLayerColor( aLayer, aColor );
}


int GERBVIEW_FRAME::getActiveLayer()
{
    return ( (GBR_SCREEN*) GetScreen() )->m_Active_Layer;
}


void GERBVIEW_FRAME::setActiveLayer( int aLayer, bool doLayerWidgetUpdate )
{
    ( (GBR_SCREEN*) GetScreen() )->m_Active_Layer = aLayer;

    if( doLayerWidgetUpdate )
        m_LayersManager->SelectLayer( getActiveLayer() );
}


void GERBVIEW_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    m_paper = aPageSettings;

    if( GetScreen() )
        GetScreen()->InitDataPoints( aPageSettings.GetSizeIU() );
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


void GERBVIEW_FRAME::SetCurItem( GERBER_DRAW_ITEM* aItem, bool aDisplayInfo )
{
    GetScreen()->SetCurItem( aItem );

    if( aItem )
    {
        if( aDisplayInfo )
        {
            MSG_PANEL_ITEMS items;
            aItem->GetMsgPanelInfo( items );
            SetMsgPanel( items );
        }
    }
    else
    {
        EraseMsgBox();
    }
}


EDA_RECT GERBVIEW_FRAME::GetGerberLayoutBoundingBox()
{
    GetGerberLayout()->ComputeBoundingBox();
    return GetGerberLayout()->GetBoundingBox();
}

void GERBVIEW_FRAME::UpdateStatusBar()
{
    EDA_DRAW_FRAME::UpdateStatusBar();

    GBR_SCREEN* screen = (GBR_SCREEN*) GetScreen();

    if( !screen )
        return;

    int dx;
    int dy;
    double dXpos;
    double dYpos;
    wxString line;
    wxString locformatter;

    if( m_DisplayOptions.m_DisplayPolarCood )  // display relative polar coordinates
    {
        double       theta, ro;

        dx = GetCrossHairPosition().x - screen->m_O_Curseur.x;
        dy = GetCrossHairPosition().y - screen->m_O_Curseur.y;

        // atan2 in the 0,0 case returns 0
        theta = RAD2DEG( atan2( (double) -dy, (double) dx ) );

        ro = hypot( dx, dy );
        wxString formatter;
        switch( g_UserUnit )
        {
        case INCHES:
            formatter = wxT( "Ro %.6f Th %.1f" );
            break;

        case MILLIMETRES:
            formatter = wxT( "Ro %.5f Th %.1f" );
            break;

        case UNSCALED_UNITS:
            formatter = wxT( "Ro %f Th %f" );
            break;

        case DEGREES:
            wxASSERT( false );
            break;
        }

        line.Printf( formatter, To_User_Unit( g_UserUnit, ro ), theta );

        SetStatusText( line, 3 );
    }

    // Display absolute coordinates:
    dXpos = To_User_Unit( g_UserUnit, GetCrossHairPosition().x );
    dYpos = To_User_Unit( g_UserUnit, GetCrossHairPosition().y );

    wxString absformatter;

    switch( g_UserUnit )
    {
    case INCHES:
        absformatter = wxT( "X %.6f  Y %.6f" );
        locformatter = wxT( "dx %.6f  dy %.6f  dist %.4f" );
        break;

    case MILLIMETRES:
        absformatter = wxT( "X %.5f  Y %.5f" );
        locformatter = wxT( "dx %.5f  dy %.5f  dist %.3f" );
        break;

    case UNSCALED_UNITS:
        absformatter = wxT( "X %f  Y %f" );
        locformatter = wxT( "dx %f  dy %f  dist %f" );
        break;

    case DEGREES:
        wxASSERT( false );
        break;
    }

    line.Printf( absformatter, dXpos, dYpos );
    SetStatusText( line, 2 );

    if( !m_DisplayOptions.m_DisplayPolarCood )  // display relative cartesian coordinates
    {
        // Display relative coordinates:
        dx = GetCrossHairPosition().x - screen->m_O_Curseur.x;
        dy = GetCrossHairPosition().y - screen->m_O_Curseur.y;
        dXpos = To_User_Unit( g_UserUnit, dx );
        dYpos = To_User_Unit( g_UserUnit, dy );

        // We already decided the formatter above
        line.Printf( locformatter, dXpos, dYpos, hypot( dXpos, dYpos ) );
        SetStatusText( line, 3 );
    }
}


const wxString GERBVIEW_FRAME::GetZoomLevelIndicator() const
{
    return EDA_DRAW_FRAME::GetZoomLevelIndicator();
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
{   // Called on units change (see EDA_DRAW_FRAME)
    EDA_DRAW_FRAME::unitsChangeRefresh();
    updateDCodeSelectBox();
}
