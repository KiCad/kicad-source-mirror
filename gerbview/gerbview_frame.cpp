/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1994 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <appl_wxstruct.h>
#include <wxstruct.h>
#include <class_drawpanel.h>
#include <build_version.h>
#include <macros.h>
#include <base_units.h>
#include <colors_selection.h>
#include <class_gbr_layer_box_selector.h>

#include <gerbview.h>
#include <class_gerber_draw_item.h>
#include <pcbplot.h>
#include <gerbview_id.h>
#include <hotkeys.h>
#include <class_GERBER.h>
#include <dialog_helpers.h>
#include <class_DCodeSelectionbox.h>
#include <class_gerbview_layer_widget.h>
#include <class_gbr_screen.h>


// Config keywords
static const wxString   cfgShowPageSizeOption( wxT( "ShowPageSizeOpt" ) );
static const wxString   cfgShowDCodes( wxT( "ShowDCodesOpt" ) );
static const wxString   cfgShowBorderAndTitleBlock( wxT( "ShowBorderAndTitleBlock" ) );


/*************************************/
/* class GERBVIEW_FRAME for GerbView */
/*************************************/

GERBVIEW_FRAME::GERBVIEW_FRAME( wxWindow*       father,
                                const wxString& title,
                                const wxPoint&  pos,
                                const wxSize&   size,
                                long            style ) :
    EDA_DRAW_FRAME( father, GERBER_FRAME, title, pos, size, style )
{
    m_colorsSettings = &g_ColorsSettings;
    m_Layout = NULL;

    m_FrameName = wxT( "GerberFrame" );
    m_show_layer_manager_tools = true;

    m_showAxis = true;                      // true to show X and Y axis on screen
    m_showBorderAndTitleBlock   = false;    // true for reference drawings.
    m_HotkeysZoomAndGridList    = s_Gerbview_Hokeys_Descr;
    m_SelLayerBox   = NULL;
    m_DCodeSelector = NULL;
    m_displayMode   = 0;
    m_drillFileHistory.SetBaseId( ID_GERBVIEW_DRILL_FILE1 );

    if( m_canvas )
        m_canvas->SetEnableBlockCommands( true );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_gerbview_xpm ) );
    SetIcon( icon );

    SetLayout( new GBR_LAYOUT() );

    SetVisibleLayers( -1 );     // All 32 layers visible.

    SetScreen( new GBR_SCREEN( GetLayout()->GetPageSettings().GetSizeIU() ) );

    // Create the PCB_LAYER_WIDGET *after* SetLayout():
    wxFont  font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    int     pointSize       = font.GetPointSize();
    int     screenHeight    = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );

    if( screenHeight <= 900 )
        pointSize = (pointSize * 8) / 10;

    m_LayersManager = new GERBER_LAYER_WIDGET( this, m_canvas, pointSize );

    // LoadSettings() *after* creating m_LayersManager, because LoadSettings()
    // initialize parameters in m_LayersManager
    LoadSettings();
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    if( m_LastGridSizeId < ID_POPUP_GRID_LEVEL_1000 )
        m_LastGridSizeId = m_LastGridSizeId;
    if( m_LastGridSizeId > ID_POPUP_GRID_LEVEL_0_0_1MM )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_0_0_1MM;
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateOptToolbar();

    m_auimgr.SetManagedWindow( this );

    EDA_PANEINFO    horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO    vert;
    vert.VerticalToolbarPane();

    EDA_PANEINFO    mesg;
    mesg.MessageToolbarPane();

    EDA_PANEINFO    lyrs;
    lyrs.LayersToolbarPane();
    lyrs.MinSize( m_LayersManager->GetBestSize() );
    lyrs.BestSize( m_LayersManager->GetBestSize() );
    lyrs.Caption( _( "Visibles" ) );


    if( m_mainToolBar )
        m_auimgr.AddPane( m_mainToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_mainToolBar" ) ).Top().Row( 0 ) );

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
}


GERBVIEW_FRAME::~GERBVIEW_FRAME()
{
    wxGetApp().SaveCurrentSetupValues( m_configSettings );
}


void GERBVIEW_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    SaveSettings();
    Destroy();
}


double GERBVIEW_FRAME::BestZoom()
{
    GERBER_DRAW_ITEM* item = GetLayout()->m_Drawings;

    // gives a minimal value to zoom, if no item in list
    if( item == NULL  )
        return ZOOM_FACTOR( 350.0 );

    EDA_RECT bbox = GetLayout()->ComputeBoundingBox();

    wxSize  size = m_canvas->GetClientSize();

    double  x   = (double) bbox.GetWidth() / (double) size.x;
    double  y   = (double) bbox.GetHeight() / (double) size.y;
    GetScreen()->SetScrollCenterPosition( bbox.Centre() );

    double  best_zoom = MAX( x, y );
    return best_zoom;
}


void GERBVIEW_FRAME::LoadSettings()
{
    wxConfig* config = wxGetApp().GetSettings();

    if( config == NULL )
        return;

    EDA_DRAW_FRAME::LoadSettings();

    wxGetApp().ReadCurrentSetupValues( GetConfigurationSettings() );

    PAGE_INFO pageInfo( wxT( "GERBER" ) );

    config->Read( cfgShowBorderAndTitleBlock, &m_showBorderAndTitleBlock, false );

    if( m_showBorderAndTitleBlock )
    {
        wxString pageType;

        config->Read( cfgShowPageSizeOption, &pageType, wxT( "GERBER" ) );

        pageInfo.SetType( pageType );
    }

    SetPageSettings( pageInfo );

    GetScreen()->InitDataPoints( pageInfo.GetSizeIU() );

    long tmp;
    config->Read( cfgShowDCodes, &tmp, 1 );
    SetElementVisibility( DCODES_VISIBLE, tmp );

    // because we have 2 file historues, we must read this one
    // using a specific path
    config->SetPath( wxT( "drl_files" ) );
    m_drillFileHistory.Load( *config );
    config->SetPath( wxT( ".." ) );

    // WxWidgets 2.9.1 seems call setlocale( LC_NUMERIC, "" )
    // when reading doubles in config,
    // but forget to back to current locale. So we call SetLocaleTo_Default
    SetLocaleTo_Default();
}


void GERBVIEW_FRAME::SaveSettings()
{
    wxConfig* config = wxGetApp().GetSettings();

    if( config == NULL )
        return;

    EDA_DRAW_FRAME::SaveSettings();

    wxGetApp().SaveCurrentSetupValues( GetConfigurationSettings() );

    config->Write( cfgShowPageSizeOption, GetPageSettings().GetType() );
    config->Write( cfgShowBorderAndTitleBlock, m_showBorderAndTitleBlock );
    config->Write( cfgShowDCodes, IsElementVisible( DCODES_VISIBLE ) );

    // Save the drill file history list.
    // Because we have 2 file histories, we must save this one
    // in a specific path
    config->SetPath( wxT( "drl_files" ) );
    m_drillFileHistory.Save( *config );
    config->SetPath( wxT( ".." ) );
}


void GERBVIEW_FRAME::ReFillLayerWidget()
{
    m_LayersManager->ReFill();

    wxAuiPaneInfo&  lyrs = m_auimgr.GetPane( m_LayersManager );

    wxSize          bestz = m_LayersManager->GetBestSize();

    lyrs.MinSize( bestz );
    lyrs.BestSize( bestz );
    lyrs.FloatingSize( bestz );

    if( lyrs.IsDocked() )
        m_auimgr.Update();
    else
        m_LayersManager->SetSize( bestz );

    syncLayerWidget();
}

/**
 * Function SetElementVisibility
 * changes the visibility of an element category
 * @param aItemIdVisible is from the enum by the same name
 * @param aNewState = The new visibility state of the element category
 * @see enum aGERBER_VISIBLE
 */
void GERBVIEW_FRAME::SetElementVisibility( int aItemIdVisible, bool aNewState )
{
    switch( aItemIdVisible )
    {
    case DCODES_VISIBLE:
        m_DisplayOptions.m_DisplayDCodes = aNewState;
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

    for( int i = 0; i < GERBVIEW_LAYER_COUNT; i++ )
    {
        GERBER_IMAGE* gerber = g_GERBER_List[ layer ];

        if( gerber == NULL || gerber->m_FileName.IsEmpty() )
            return layer;

        layer++;

        if( layer >= GERBVIEW_LAYER_COUNT )
            layer = 0;
    }

    return NO_AVAILABLE_LAYERS;
}


void GERBVIEW_FRAME::syncLayerWidget()
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
void GERBVIEW_FRAME::syncLayerBox()
{
    m_SelLayerBox->SetSelection( getActiveLayer() );
    int             dcodeSelected = -1;
    GERBER_IMAGE*   gerber = g_GERBER_List[getActiveLayer()];

    if( gerber )
        dcodeSelected = gerber->m_Selected_Tool;

    if( m_DCodeSelector )
    {
        m_DCodeSelector->SetDCodeSelection( dcodeSelected );
        m_DCodeSelector->Enable( gerber != NULL );
    }

    UpdateTitleAndInfo();
}


void GERBVIEW_FRAME::Liste_D_Codes()
{
    int             ii, jj;
    D_CODE*         pt_D_code;
    wxString        Line;
    wxArrayString   list;
    double          scale = IU_PER_MILS * 1000;
    int             curr_layer = getActiveLayer();

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

        list.Add( Line );

        for( ii = 0, jj = 1; ii < TOOLS_MAX_COUNT; ii++ )
        {
            pt_D_code = gerber->GetDCODE( ii + FIRST_DCODE, false );

            if( pt_D_code == NULL )
                continue;

            if( !pt_D_code->m_InUse && !pt_D_code->m_Defined )
                continue;

            Line.Printf( wxT( "tool %2.2d:   D%2.2d  V %2.4f  H %2.4f  %s" ),
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

            list.Add( Line );
            jj++;
        }
    }

#if wxCHECK_VERSION( 2, 9, 4 )
    wxSingleChoiceDialog    dlg( this, wxEmptyString, _( "D Codes" ), list, (void**) NULL,
                                 wxCHOICEDLG_STYLE & ~wxCANCEL );
#else
    wxSingleChoiceDialog    dlg( this, wxEmptyString, _( "D Codes" ), list, (char**) NULL,
                                 wxCHOICEDLG_STYLE & ~wxCANCEL );
#endif

    dlg.ShowModal();
}


/*
 * Function UpdateTitleAndInfo
 * displays the short filename (if exists) of the selected layer
 *  on the caption of the main GerbView window
 * displays image name and the last layer name (found in the gerber file: LN <name> command)
 *  in the status bar
 * Note layer name can change when reading a gerber file, and the layer name is the last found.
 * So, show the layer name is not very useful, and can be seen as a debug feature.
 */
void GERBVIEW_FRAME::UpdateTitleAndInfo()
{
    GERBER_IMAGE*   gerber = g_GERBER_List[ getActiveLayer() ];
    wxString        text;

    // Display the gerber filename
    if( gerber == NULL )
    {
        text.Printf( wxT( "GerbView %s" ), GetChars( GetBuildVersion() ) );
        SetTitle( text );
        SetStatusText( wxEmptyString, 0 );
        text.Printf( _( "Layer %d not in use" ), getActiveLayer() + 1 );
        m_TextInfo->SetValue( text );
        ClearMsgPanel();
        return;
    }

    text = _( "File:" );
    text << wxT( " " ) << gerber->m_FileName;
    SetTitle( text );

    gerber->DisplayImageInfo();

    // Display Image Name and Layer Name (from the current gerber data):
    text.Printf( _( "Image name: \"%s\"  Layer name: \"%s\"" ),
                 GetChars( gerber->m_ImageName ),
                 GetChars( gerber->GetLayerParams().m_LayerName ) );
    SetStatusText( text, 0 );

    // Display data format like fmt in X3.4Y3.4 no LZ or fmt mm X2.3 Y3.5 no TZ in main toolbar
    text.Printf( wxT( "fmt: %s X%d.%d Y%d.%d no %cZ" ),
                 gerber->m_GerbMetric ? wxT( "mm" ) : wxT( "in" ),
                 gerber->m_FmtLen.x - gerber->m_FmtScale.x, gerber->m_FmtScale.x,
                 gerber->m_FmtLen.y - gerber->m_FmtScale.y, gerber->m_FmtScale.y,
                 gerber->m_NoTrailingZeros ? 'T' : 'L' );

    m_TextInfo->SetValue( text );
}


/*
 * Function IsElementVisible
 * tests whether a given element category is visible. Keep this as an
 * inline function.
 * @param aGERBER_VISIBLE is from the enum by the same name
 * @return bool - true if the element is visible.
 * @see enum PCB_VISIBLE
 */
bool GERBVIEW_FRAME::IsElementVisible( int aItemIdVisible )
{
    switch( aItemIdVisible )
    {
    case DCODES_VISIBLE:
        return m_DisplayOptions.m_DisplayDCodes;
        break;

    case GERBER_GRID_VISIBLE:
        return IsGridVisible();
        break;

    default:
        wxLogDebug( wxT( "GERBVIEW_FRAME::SetVisibleElementColor(): bad arg %d" ), aItemIdVisible );
    }

    return true;
}


/**
 * Function SetVisibleAlls
 * Set the status of all layers to VISIBLE
 */
void GERBVIEW_FRAME::SetVisibleAlls()
{
}

/**
 * Function GetVisibleLayers
 * is a proxy function that calls the correspondent function in m_BoardSettings
 * Returns a bit-mask of all the layers that are visible
 * @return int - the visible layers in bit-mapped form.
 */
int GERBVIEW_FRAME::GetVisibleLayers() const
{
    return -1;    // TODO
}


/**
 * Function SetVisibleLayers
 * is a proxy function that calls the correspondent function in m_BoardSettings
 * changes the bit-mask of visible layers
 * @param aLayerMask = The new bit-mask of visible layers
 */
void GERBVIEW_FRAME::SetVisibleLayers( int aLayerMask )
{
    GetLayout()->SetVisibleLayers( aLayerMask );
}


/**
 * Function IsLayerVisible
 * tests whether a given layer is visible
 * @param aLayerIndex = The index of the layer to be tested
 * @return bool - true if the layer is visible.
 */
bool GERBVIEW_FRAME::IsLayerVisible( int aLayerIndex ) const
{
    if( ! m_DisplayOptions.m_IsPrinting )
        return m_LayersManager->IsLayerVisible( aLayerIndex );
    else
        return GetLayout()->IsLayerVisible( aLayerIndex );
}


/**
 * Function GetVisibleElementColor
 * returns the color of a pcb visible element.
 * @see enum PCB_VISIBLE
 */
int GERBVIEW_FRAME::GetVisibleElementColor( int aItemIdVisible )
{
    int color = -1;

    switch( aItemIdVisible )
    {
    case DCODES_VISIBLE:
        color = m_colorsSettings->GetItemColor( aItemIdVisible );
        break;

    case GERBER_GRID_VISIBLE:
        color = GetGridColor();
        break;

    default:
        wxLogDebug( wxT( "GERBVIEW_FRAME::GetVisibleElementColor(): bad arg %d" ), aItemIdVisible );
    }

    return color;
}

/*
 * Virtual from EDA_DRAW_FRAME
 */
void GERBVIEW_FRAME::SetGridVisibility( bool aVisible )
{
    EDA_DRAW_FRAME::SetGridVisibility( aVisible );
    m_LayersManager->SetRenderState( GERBER_GRID_VISIBLE, aVisible );
}


void GERBVIEW_FRAME::SetVisibleElementColor( int aItemIdVisible, int aColor )
{
    switch( aItemIdVisible )
    {
    case DCODES_VISIBLE:
        m_colorsSettings->SetItemColor( aItemIdVisible, aColor );
        break;

    case GERBER_GRID_VISIBLE:
        SetGridColor( aColor );
        m_colorsSettings->SetItemColor( aItemIdVisible, aColor );
        break;

    default:
        wxLogDebug( wxT( "GERBVIEW_FRAME::SetVisibleElementColor(): bad arg %d" ), aItemIdVisible );
    }
}


/**
 * Function GetLayerColor
 * gets a layer color for any valid layer, including non-copper ones.
 */
int GERBVIEW_FRAME::GetLayerColor( int aLayer )
{
    return m_colorsSettings->GetLayerColor( aLayer );
}


/**
 * Function SetLayerColor
 * changes a layer color for any valid layer, including non-copper ones.
 */
void GERBVIEW_FRAME::SetLayerColor( int aLayer, int aColor )
{
    m_colorsSettings->SetLayerColor( aLayer, aColor );
}


/**
 * Function getActiveLayer
 * returns the active layer
 */
int GERBVIEW_FRAME::getActiveLayer()
{
    return ( (GBR_SCREEN*) GetScreen() )->m_Active_Layer;
}


/**
 * Function setActiveLayer
 * will change the currently active layer to \a aLayer and also
 * update the PCB_LAYER_WIDGET.
 */
void GERBVIEW_FRAME::setActiveLayer( int aLayer, bool doLayerWidgetUpdate )
{
    ( (GBR_SCREEN*) GetScreen() )->m_Active_Layer = aLayer;

    if( doLayerWidgetUpdate )
        m_LayersManager->SelectLayer( getActiveLayer() );
}


void GERBVIEW_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    wxASSERT( m_Layout );
    m_Layout->SetPageSettings( aPageSettings );

    if( GetScreen() )
        GetScreen()->InitDataPoints( aPageSettings.GetSizeIU() );
}


const PAGE_INFO& GERBVIEW_FRAME::GetPageSettings() const
{
    wxASSERT( m_Layout );
    return m_Layout->GetPageSettings();
}


const wxSize GERBVIEW_FRAME::GetPageSizeIU() const
{
    wxASSERT( m_Layout );

    // this function is only needed because EDA_DRAW_FRAME is not compiled
    // with either -DPCBNEW or -DEESCHEMA, so the virtual is used to route
    // into an application specific source file.
    return m_Layout->GetPageSettings().GetSizeIU();
}


const TITLE_BLOCK& GERBVIEW_FRAME::GetTitleBlock() const
{
    wxASSERT( m_Layout );
    return m_Layout->GetTitleBlock();
}


void GERBVIEW_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    wxASSERT( m_Layout );
    m_Layout->SetTitleBlock( aTitleBlock );
}


const wxPoint& GERBVIEW_FRAME::GetOriginAxisPosition() const
{
    wxASSERT( m_Layout );
    return m_Layout->GetOriginAxisPosition();
}


void GERBVIEW_FRAME::SetOriginAxisPosition( const wxPoint& aPosition )
{
    wxASSERT( m_Layout );
    m_Layout->SetOriginAxisPosition( aPosition );
}

void GERBVIEW_FRAME::SetCurItem( GERBER_DRAW_ITEM* aItem, bool aDisplayInfo )
{
    GetScreen()->SetCurItem( aItem );

    if( aItem )
    {
        if( aDisplayInfo )
            aItem->DisplayInfo( this );
    }
    else
        EraseMsgBox();
}

/*
 * Function GetLayoutBoundingBox
 * returns the bounding box containing all gerber items.
 */
EDA_RECT GERBVIEW_FRAME::GetLayoutBoundingBox()
{
    GetLayout()->ComputeBoundingBox();
    return GetLayout()->GetBoundingBox();
}

/*
 * Update the status bar information.
 */
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

        dx = screen->GetCrossHairPosition().x - screen->m_O_Curseur.x;
        dy = screen->GetCrossHairPosition().y - screen->m_O_Curseur.y;

        if( dx==0 && dy==0 )
            theta = 0.0;
        else
            theta = atan2( (double) -dy, (double) dx );

        theta = theta * 180.0 / M_PI;

        ro = sqrt( ( (double) dx * dx ) + ( (double) dy * dy ) );
        wxString formatter;
        switch( g_UserUnit )
        {
        case INCHES:
            formatter = wxT( "Ro %.4f Th %.1f" );
            break;

        case MILLIMETRES:
            formatter = wxT( "Ro %.3f Th %.1f" );
            break;

        case UNSCALED_UNITS:
            formatter = wxT( "Ro %f Th %f" );
            break;
        }

        line.Printf( formatter, To_User_Unit( g_UserUnit, ro ), theta );

        SetStatusText( line, 3 );
    }

    // Display absolute coordinates:
    dXpos = To_User_Unit( g_UserUnit, screen->GetCrossHairPosition().x );
    dYpos = To_User_Unit( g_UserUnit, screen->GetCrossHairPosition().y );

    if ( g_UserUnit == MILLIMETRES )
    {
        dXpos = RoundTo0( dXpos, 1000.0 );
        dYpos = RoundTo0( dYpos, 1000.0 );
    }

    // The following sadly is an if Eeschema/if Pcbnew
    wxString absformatter;

    switch( g_UserUnit )
    {
    case INCHES:
        absformatter = wxT( "X %.4f  Y %.4f" );
        locformatter = wxT( "dx %.4f  dy %.4f  d %.4f" );
        break;

    case MILLIMETRES:
        absformatter = wxT( "X %.3f  Y %.3f" );
        locformatter = wxT( "dx %.3f  dy %.3f  d %.3f" );
        break;

    case UNSCALED_UNITS:
        absformatter = wxT( "X %f  Y %f" );
        locformatter = wxT( "dx %f  dy %f  d %f" );
        break;
    }

    line.Printf( absformatter, dXpos, dYpos );
    SetStatusText( line, 2 );

    if( !m_DisplayOptions.m_DisplayPolarCood )  // display relative cartesian coordinates
    {
        // Display relative coordinates:
        dx = screen->GetCrossHairPosition().x - screen->m_O_Curseur.x;
        dy = screen->GetCrossHairPosition().y - screen->m_O_Curseur.y;
        dXpos = To_User_Unit( g_UserUnit, dx );
        dYpos = To_User_Unit( g_UserUnit, dy );

        if ( g_UserUnit == MILLIMETRES )
        {
            dXpos = RoundTo0( dXpos, 1000.0 );
            dYpos = RoundTo0( dYpos, 1000.0 );
        }

        // We already decided the formatter above
        line.Printf( locformatter, dXpos, dYpos, sqrt( dXpos * dXpos + dYpos * dYpos ) );
        SetStatusText( line, 3 );
    }
}

