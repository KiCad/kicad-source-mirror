/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file basepcbframe.cpp
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <wxstruct.h>
#include <confirm.h>
#include <kiface_i.h>
#include <dialog_helpers.h>
#include <kicad_device_context.h>
#include <wxBasePcbFrame.h>
#include <base_units.h>
#include <msgpanel.h>

#include <pcbnew.h>
#include <fp_lib_table.h>
#include <pcbnew_id.h>
#include <class_board.h>
#include <class_track.h>
#include <class_module.h>
#include <class_drawsegment.h>

#include <collectors.h>
#include <class_drawpanel.h>
#include <pcb_draw_panel_gal.h>
#include <view/view.h>
#include <math/vector2d.h>
#include <trigo.h>
#include <pcb_painter.h>
#include <worksheet_viewitem.h>
#include <ratsnest_data.h>
#include <ratsnest_viewitem.h>

#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>

const wxChar PCB_BASE_FRAME::CANVAS_TYPE_KEY[] = wxT( "canvas_type" );

// Configuration entry names.
static const wxChar UserGridSizeXEntry[] = wxT( "PcbUserGrid_X" );
static const wxChar UserGridSizeYEntry[] = wxT( "PcbUserGrid_Y" );
static const wxChar UserGridUnitsEntry[] = wxT( "PcbUserGrid_Unit" );
static const wxChar DisplayPadFillEntry[] = wxT( "DiPadFi" );
static const wxChar DisplayViaFillEntry[] = wxT( "DiViaFi" );
static const wxChar DisplayPadNumberEntry[] = wxT( "DiPadNu" );
static const wxChar DisplayModuleEdgeEntry[] = wxT( "DiModEd" );
static const wxChar DisplayModuleTextEntry[] = wxT( "DiModTx" );
static const wxChar FastGrid1Entry[] = wxT( "FastGrid1" );
static const wxChar FastGrid2Entry[] = wxT( "FastGrid2" );


BEGIN_EVENT_TABLE( PCB_BASE_FRAME, EDA_DRAW_FRAME )
    EVT_MENU_RANGE( ID_POPUP_PCB_ITEM_SELECTION_START, ID_POPUP_PCB_ITEM_SELECTION_END,
                    PCB_BASE_FRAME::ProcessItemSelection )

    EVT_TOOL( ID_TB_OPTIONS_SHOW_POLAR_COORD, PCB_BASE_FRAME::OnTogglePolarCoords )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_PADS_SKETCH, PCB_BASE_FRAME::OnTogglePadDrawMode )

    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_POLAR_COORD, PCB_BASE_FRAME::OnUpdateCoordType )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_PADS_SKETCH, PCB_BASE_FRAME::OnUpdatePadDrawMode )
    EVT_UPDATE_UI( ID_ON_GRID_SELECT, PCB_BASE_FRAME::OnUpdateSelectGrid )
    EVT_UPDATE_UI( ID_ON_ZOOM_SELECT, PCB_BASE_FRAME::OnUpdateSelectZoom )

    EVT_UPDATE_UI_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, PCB_BASE_FRAME::OnUpdateSelectZoom )
END_EVENT_TABLE()


PCB_BASE_FRAME::PCB_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
        const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
        long aStyle, const wxString & aFrameName ) :
    EDA_DRAW_FRAME( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName )
{
    m_Pcb                 = NULL;
    m_Draw3DFrame         = NULL;   // Display Window in 3D mode (OpenGL)

    m_UserGridSize        = wxRealPoint( 100.0, 100.0 );
    m_UserGridUnit        = INCHES;
    m_Collector           = new GENERAL_COLLECTOR();

    m_FastGrid1           = 0;
    m_FastGrid2           = 0;

    m_auxiliaryToolBar    = NULL;

    m_zoomLevelCoeff      = 110.0 * IU_PER_DECIMILS;  // Adjusted to roughly displays zoom level = 1
                                        // when the screen shows a 1:1 image
                                        // obviously depends on the monitor,
                                        // but this is an acceptable value
}


PCB_BASE_FRAME::~PCB_BASE_FRAME()
{
    delete m_Collector;
    delete m_Pcb;
}


FP_LIB_TABLE* PROJECT::PcbFootprintLibs()
{
    // This is a lazy loading function, it loads the project specific table when
    // that table is asked for, not before.

    FP_LIB_TABLE*   tbl = (FP_LIB_TABLE*) GetElem( ELEM_FPTBL );

    // its gotta be NULL or a FP_LIB_TABLE, or a bug.
    wxASSERT( !tbl || dynamic_cast<FP_LIB_TABLE*>( tbl ) );

    if( !tbl )
    {
        // Stack the project specific FP_LIB_TABLE overlay on top of the global table.
        // ~FP_LIB_TABLE() will not touch the fallback table, so multiple projects may
        // stack this way, all using the same global fallback table.
        tbl = new FP_LIB_TABLE( &GFootprintTable );

        SetElem( ELEM_FPTBL, tbl );

        wxString projectFpLibTableFileName = FootprintLibTblName();

        try
        {
            tbl->Load( projectFpLibTableFileName );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( NULL, ioe.errorText );
        }
    }

    return tbl;
}


void PCB_BASE_FRAME::SetBoard( BOARD* aBoard )
{
    if( m_Pcb != aBoard )
    {
        delete m_Pcb;
        m_Pcb = aBoard;
    }
}


void PCB_BASE_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    wxASSERT( m_Pcb );
    m_Pcb->SetPageSettings( aPageSettings );

    if( GetScreen() )
        GetScreen()->InitDataPoints( aPageSettings.GetSizeIU() );
}


const PAGE_INFO& PCB_BASE_FRAME::GetPageSettings() const
{
    wxASSERT( m_Pcb );
    return m_Pcb->GetPageSettings();
}


const wxSize PCB_BASE_FRAME::GetPageSizeIU() const
{
    wxASSERT( m_Pcb );

    // this function is only needed because EDA_DRAW_FRAME is not compiled
    // with either -DPCBNEW or -DEESCHEMA, so the virtual is used to route
    // into an application specific source file.
    return m_Pcb->GetPageSettings().GetSizeIU();
}


const wxPoint& PCB_BASE_FRAME::GetAuxOrigin() const
{
    wxASSERT( m_Pcb );
    return m_Pcb->GetAuxOrigin();
}


void PCB_BASE_FRAME::SetAuxOrigin( const wxPoint& aPoint )
{
    wxASSERT( m_Pcb );
    m_Pcb->SetAuxOrigin( aPoint );
}


const wxPoint& PCB_BASE_FRAME::GetGridOrigin() const
{
    wxASSERT( m_Pcb );
    return m_Pcb->GetGridOrigin();
}


void PCB_BASE_FRAME::SetGridOrigin( const wxPoint& aPoint )
{
    wxASSERT( m_Pcb );
    m_Pcb->SetGridOrigin( aPoint );
}


const TITLE_BLOCK& PCB_BASE_FRAME::GetTitleBlock() const
{
    wxASSERT( m_Pcb );
    return m_Pcb->GetTitleBlock();
}


void PCB_BASE_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    wxASSERT( m_Pcb );
    m_Pcb->SetTitleBlock( aTitleBlock );
}


BOARD_DESIGN_SETTINGS& PCB_BASE_FRAME::GetDesignSettings() const
{
    wxASSERT( m_Pcb );
    return m_Pcb->GetDesignSettings();
}


void PCB_BASE_FRAME::SetDesignSettings( const BOARD_DESIGN_SETTINGS& aSettings )
{
    wxASSERT( m_Pcb );
    m_Pcb->SetDesignSettings( aSettings );
}


const ZONE_SETTINGS& PCB_BASE_FRAME::GetZoneSettings() const
{
    wxASSERT( m_Pcb );
    return m_Pcb->GetZoneSettings();
}


void PCB_BASE_FRAME::SetZoneSettings( const ZONE_SETTINGS& aSettings )
{
    wxASSERT( m_Pcb );
    m_Pcb->SetZoneSettings( aSettings );
}


const PCB_PLOT_PARAMS& PCB_BASE_FRAME::GetPlotSettings() const
{
    wxASSERT( m_Pcb );
    return m_Pcb->GetPlotOptions();
}


void PCB_BASE_FRAME::SetPlotSettings( const PCB_PLOT_PARAMS& aSettings )
{
    wxASSERT( m_Pcb );
    m_Pcb->SetPlotOptions( aSettings );
}


EDA_RECT PCB_BASE_FRAME::GetBoardBoundingBox( bool aBoardEdgesOnly ) const
{
    wxASSERT( m_Pcb );

    EDA_RECT area = m_Pcb->ComputeBoundingBox( aBoardEdgesOnly );

    if( area.GetWidth() == 0 && area.GetHeight() == 0 )
    {
        wxSize pageSize = GetPageSizeIU();

        if( m_showBorderAndTitleBlock )
        {
            area.SetOrigin( 0, 0 );
            area.SetEnd( pageSize.x, pageSize.y );
        }
        else
        {
            area.SetOrigin( -pageSize.x / 2, -pageSize.y / 2 );
            area.SetEnd( pageSize.x / 2, pageSize.y / 2 );
        }
    }

    return area;
}


double PCB_BASE_FRAME::BestZoom()
{
    if( m_Pcb == NULL )
        return 1.0;

    EDA_RECT    ibbbox  = GetBoardBoundingBox();
    DSIZE       clientz = m_canvas->GetClientSize();
    DSIZE       boardz( ibbbox.GetWidth(), ibbbox.GetHeight() );

    double iu_per_du_X = clientz.x ? boardz.x / clientz.x : 1.0;
    double iu_per_du_Y = clientz.y ? boardz.y / clientz.y : 1.0;

    double bestzoom = std::max( iu_per_du_X, iu_per_du_Y );

    SetScrollCenterPosition( ibbbox.Centre() );

    return bestzoom;
}


void PCB_BASE_FRAME::CursorGoto( const wxPoint& aPos, bool aWarp )
{
    // factored out of pcbnew/find.cpp

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    // There may be need to reframe the drawing.
    if( !m_canvas->IsPointOnDisplay( aPos ) )
    {
        SetCrossHairPosition( aPos );
        RedrawScreen( aPos, aWarp );
    }
    else
    {
        // Put cursor on item position
        m_canvas->CrossHairOff( &dc );
        SetCrossHairPosition( aPos );

        if( aWarp )
            m_canvas->MoveCursorToCrossHair();
    }
    m_canvas->CrossHairOn( &dc );
    m_canvas->CrossHairOn( &dc );
}


// Virtual function
void PCB_BASE_FRAME::ReCreateMenuBar( void )
{
}


// Virtual functions: Do nothing for PCB_BASE_FRAME window
void PCB_BASE_FRAME::Show3D_Frame( wxCommandEvent& event )
{
}


// Note: virtual, overridden in PCB_EDIT_FRAME;
void PCB_BASE_FRAME::SwitchLayer( wxDC* DC, LAYER_ID layer )
{
    LAYER_ID preslayer = GetActiveLayer();
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();

    // Check if the specified layer matches the present layer
    if( layer == preslayer )
        return;

    // Copper layers cannot be selected unconditionally; how many
    // of those layers are currently enabled needs to be checked.
    if( IsCopperLayer( layer ) )
    {
        // If only one copper layer is enabled, the only such layer
        // that can be selected to is the "Copper" layer (so the
        // selection of any other copper layer is disregarded).
        if( m_Pcb->GetCopperLayerCount() < 2 )
        {
            if( layer != B_Cu )
            {
                return;
            }
        }

        // If more than one copper layer is enabled, the "Copper"
        // and "Component" layers can be selected, but the total
        // number of copper layers determines which internal
        // layers are also capable of being selected.
        else
        {
            if( ( layer != B_Cu ) && ( layer != F_Cu )
                && ( layer >= m_Pcb->GetCopperLayerCount() - 1 ) )
            {
                return;
            }
        }
    }

    // Is yet more checking required? E.g. when the layer to be selected
    // is a non-copper layer, or when switching between a copper layer
    // and a non-copper layer, or vice-versa?
    // ...

    GetScreen()->m_Active_Layer = layer;

    if( displ_opts->m_ContrastModeDisplay )
        m_canvas->Refresh();
}


void PCB_BASE_FRAME::OnTogglePolarCoords( wxCommandEvent& aEvent )
{
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();
    SetStatusText( wxEmptyString );

    displ_opts->m_DisplayPolarCood = !displ_opts->m_DisplayPolarCood;

    UpdateStatusBar();
}


void PCB_BASE_FRAME::OnTogglePadDrawMode( wxCommandEvent& aEvent )
{
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();

    displ_opts->m_DisplayPadFill = !displ_opts->m_DisplayPadFill;
    EDA_DRAW_PANEL_GAL* gal = GetGalCanvas();

    if( gal )
    {
    // Apply new display options to the GAL canvas
        KIGFX::PCB_PAINTER* painter =
                static_cast<KIGFX::PCB_PAINTER*> ( gal->GetView()->GetPainter() );
        KIGFX::PCB_RENDER_SETTINGS* settings =
                static_cast<KIGFX::PCB_RENDER_SETTINGS*> ( painter->GetSettings() );
        settings->LoadDisplayOptions( displ_opts );

        // Update pads
        BOARD* board = GetBoard();
        for( MODULE* module = board->m_Modules; module; module = module->Next() )
        {
            for( D_PAD* pad = module->Pads(); pad; pad = pad->Next() )
                pad->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    m_canvas->Refresh();
}


void PCB_BASE_FRAME::OnUpdateCoordType( wxUpdateUIEvent& aEvent )
{
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();

    aEvent.Check( displ_opts->m_DisplayPolarCood );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                        displ_opts->m_DisplayPolarCood ?
                                        _( "Display rectangular coordinates" ) :
                                        _( "Display polar coordinates" ) );
}


void PCB_BASE_FRAME::OnUpdatePadDrawMode( wxUpdateUIEvent& aEvent )
{
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();
    aEvent.Check( !displ_opts->m_DisplayPadFill );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                        displ_opts->m_DisplayPadFill ?
                                        _( "Show pads in outline mode" ) :
                                        _( "Show pads in fill mode" ) );
}


void PCB_BASE_FRAME::OnUpdateSelectGrid( wxUpdateUIEvent& aEvent )
{
    // No need to update the grid select box if it doesn't exist or the grid setting change
    // was made using the select box.
    if( m_gridSelectBox == NULL || m_auxiliaryToolBar == NULL )
        return;

    int select = wxNOT_FOUND;

    for( size_t i = 0; i < GetScreen()->GetGridCount(); i++ )
    {
        if( GetScreen()->GetGridCmdId() == GetScreen()->GetGrid( i ).m_CmdId )
        {
            select = (int) i;
            break;
        }
    }

    if( select != m_gridSelectBox->GetSelection() )
        m_gridSelectBox->SetSelection( select );
}


void PCB_BASE_FRAME::OnUpdateSelectZoom( wxUpdateUIEvent& aEvent )
{
    if( m_zoomSelectBox == NULL || m_auxiliaryToolBar == NULL )
        return;

    int current = 0;
    double zoom = IsGalCanvasActive() ? GetGalCanvas()->GetLegacyZoom() : GetScreen()->GetZoom();

    for( unsigned i = 0; i < GetScreen()->m_ZoomList.size(); i++ )
    {
        if( std::fabs( zoom - GetScreen()->m_ZoomList[i] ) < 1e-6 )
        {
            current = i + 1;
            break;
        }
    }

    if( current != m_zoomSelectBox->GetSelection() )
        m_zoomSelectBox->SetSelection( current );
}


void PCB_BASE_FRAME::ProcessItemSelection( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();

    // index into the collector list:
    int itemNdx = id - ID_POPUP_PCB_ITEM_SELECTION_START;

    if( id >= ID_POPUP_PCB_ITEM_SELECTION_START && id <= ID_POPUP_PCB_ITEM_SELECTION_END )
    {
        BOARD_ITEM* item = (*m_Collector)[itemNdx];
        m_canvas->SetAbortRequest( false );

#if 0 && defined (DEBUG)
        item->Show( 0, std::cout );
#endif

        SetCurItem( item );
    }
}


void PCB_BASE_FRAME::SetCurItem( BOARD_ITEM* aItem, bool aDisplayInfo )
{
    GetScreen()->SetCurItem( aItem );

    if( aDisplayInfo )
        UpdateMsgPanel();
}


void PCB_BASE_FRAME::UpdateMsgPanel()
{
    BOARD_ITEM* item = GetScreen()->GetCurItem();
    MSG_PANEL_ITEMS items;

    if( item )
    {
        item->GetMsgPanelInfo( items );
    }
    else       // show general information about the board
    {
        if( IsGalCanvasActive() )
            GetGalCanvas()->GetMsgPanelInfo( items );
        else
            m_Pcb->GetMsgPanelInfo( items );
    }

    SetMsgPanel( items );
}


BOARD_ITEM* PCB_BASE_FRAME::GetCurItem()
{
    return GetScreen()->GetCurItem();
}


GENERAL_COLLECTORS_GUIDE PCB_BASE_FRAME::GetCollectorsGuide()
{
    GENERAL_COLLECTORS_GUIDE guide( m_Pcb->GetVisibleLayers(),
                                    GetActiveLayer() );

    // account for the globals
    guide.SetIgnoreMTextsMarkedNoShow( ! m_Pcb->IsElementVisible( MOD_TEXT_INVISIBLE ));
    guide.SetIgnoreMTextsOnBack( ! m_Pcb->IsElementVisible( MOD_TEXT_BK_VISIBLE ));
    guide.SetIgnoreMTextsOnFront( ! m_Pcb->IsElementVisible( MOD_TEXT_FR_VISIBLE ));
    guide.SetIgnoreModulesOnBack( ! m_Pcb->IsElementVisible( MOD_BK_VISIBLE ) );
    guide.SetIgnoreModulesOnFront( ! m_Pcb->IsElementVisible( MOD_FR_VISIBLE ) );
    guide.SetIgnorePadsOnBack( ! m_Pcb->IsElementVisible( PAD_BK_VISIBLE ) );
    guide.SetIgnorePadsOnFront( ! m_Pcb->IsElementVisible( PAD_FR_VISIBLE ) );
    guide.SetIgnoreModulesVals( ! m_Pcb->IsElementVisible( MOD_VALUES_VISIBLE ) );
    guide.SetIgnoreModulesRefs( ! m_Pcb->IsElementVisible( MOD_REFERENCES_VISIBLE ) );

    return guide;
}

void PCB_BASE_FRAME::SetToolID( int aId, int aCursor, const wxString& aToolMsg )
{
    bool redraw = false;

    EDA_DRAW_FRAME::SetToolID( aId, aCursor, aToolMsg );

    if( aId < 0 )
        return;

    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();

    // handle color changes for transitions in and out of ID_TRACK_BUTT
    if( ( GetToolId() == ID_TRACK_BUTT && aId != ID_TRACK_BUTT )
        || ( GetToolId() != ID_TRACK_BUTT && aId == ID_TRACK_BUTT ) )
    {
        if( displ_opts->m_ContrastModeDisplay )
            redraw = true;
    }

    // must do this after the tool has been set, otherwise pad::Draw() does
    // not show proper color when GetDisplayOptions().ContrastModeDisplay is true.
    if( redraw && m_canvas )
        m_canvas->Refresh();
}


/*
 * Update the status bar information.
 */
void PCB_BASE_FRAME::UpdateStatusBar()
{
    PCB_SCREEN* screen = GetScreen();

    if( !screen )
        return;

    int dx;
    int dy;
    double dXpos;
    double dYpos;
    wxString line;
    wxString locformatter;
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();

    EDA_DRAW_FRAME::UpdateStatusBar();

    if( displ_opts->m_DisplayPolarCood )  // display polar coordinates
    {
        double       theta, ro;

        dx = GetCrossHairPosition().x - screen->m_O_Curseur.x;
        dy = GetCrossHairPosition().y - screen->m_O_Curseur.y;

        theta = ArcTangente( -dy, dx ) / 10;

        ro = hypot( dx, dy );
        wxString formatter;
        switch( g_UserUnit )
        {
        case INCHES:
            formatter = wxT( "Ro %.6f  Th %.1f" );
            break;

        case MILLIMETRES:
            formatter = wxT( "Ro %.6f  Th %.1f" );
            break;

        case UNSCALED_UNITS:
            formatter = wxT( "Ro %f  Th %f" );
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

    // The following sadly is an if Eeschema/if Pcbnew
    wxString absformatter;

    switch( g_UserUnit )
    {
    case INCHES:
        absformatter = wxT( "X %.6f  Y %.6f" );
        locformatter = wxT( "dx %.6f  dy %.6f  dist %.4f" );
        break;

    case MILLIMETRES:
        absformatter = wxT( "X %.6f  Y %.6f" );
        locformatter = wxT( "dx %.6f  dy %.6f  dist %.3f" );
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

    if( !displ_opts->m_DisplayPolarCood )  // display relative cartesian coordinates
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


void PCB_BASE_FRAME::unitsChangeRefresh()
{
    EDA_DRAW_FRAME::unitsChangeRefresh();    // Update the status bar.

    updateGridSelectBox();
}


void PCB_BASE_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    // Ensure grid id is an existent grid id:
    if( (m_LastGridSizeId <= 0) ||
        (m_LastGridSizeId > (ID_POPUP_GRID_USER - ID_POPUP_GRID_LEVEL_1000)) )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_500 - ID_POPUP_GRID_LEVEL_1000;

    wxString baseCfgName = GetName();

    aCfg->Read( baseCfgName + UserGridSizeXEntry, &m_UserGridSize.x, 0.01 );
    aCfg->Read( baseCfgName + UserGridSizeYEntry, &m_UserGridSize.y, 0.01 );

    long itmp;
    aCfg->Read( baseCfgName + UserGridUnitsEntry, &itmp, ( long )INCHES );
    m_UserGridUnit = (EDA_UNITS_T) itmp;
    aCfg->Read( baseCfgName + DisplayPadFillEntry, &m_DisplayOptions.m_DisplayPadFill, true );
    aCfg->Read( baseCfgName + DisplayViaFillEntry, &m_DisplayOptions.m_DisplayViaFill, true );
    aCfg->Read( baseCfgName + DisplayPadNumberEntry, &m_DisplayOptions.m_DisplayPadNum, true );
    aCfg->Read( baseCfgName + DisplayModuleEdgeEntry, &m_DisplayOptions.m_DisplayModEdgeFill, true );

    aCfg->Read( baseCfgName + FastGrid1Entry, &itmp, ( long )0);
    m_FastGrid1 = itmp;
    aCfg->Read( baseCfgName + FastGrid2Entry, &itmp, ( long )0);
    m_FastGrid2 = itmp;

    aCfg->Read( baseCfgName + DisplayModuleTextEntry, &m_DisplayOptions.m_DisplayModTextFill, true );
}


void PCB_BASE_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    wxString baseCfgName = GetName();

    aCfg->Write( baseCfgName + UserGridSizeXEntry, m_UserGridSize.x );
    aCfg->Write( baseCfgName + UserGridSizeYEntry, m_UserGridSize.y );
    aCfg->Write( baseCfgName + UserGridUnitsEntry, ( long )m_UserGridUnit );
    aCfg->Write( baseCfgName + DisplayPadFillEntry, m_DisplayOptions.m_DisplayPadFill );
    aCfg->Write( baseCfgName + DisplayViaFillEntry, m_DisplayOptions.m_DisplayViaFill );
    aCfg->Write( baseCfgName + DisplayPadNumberEntry, m_DisplayOptions.m_DisplayPadNum );
    aCfg->Write( baseCfgName + DisplayModuleEdgeEntry, m_DisplayOptions.m_DisplayModEdgeFill );
    aCfg->Write( baseCfgName + DisplayModuleTextEntry, m_DisplayOptions.m_DisplayModTextFill );
    aCfg->Write( baseCfgName + FastGrid1Entry, ( long )m_FastGrid1 );
    aCfg->Write( baseCfgName + FastGrid2Entry, ( long )m_FastGrid2 );
}


void PCB_BASE_FRAME::OnModify()
{
    GetScreen()->SetModify();
    GetScreen()->SetSave();

    if( IsGalCanvasActive() )
    {
        UpdateStatusBar();
        UpdateMsgPanel();
    }
}


const wxString PCB_BASE_FRAME::GetZoomLevelIndicator() const
{
    return EDA_DRAW_FRAME::GetZoomLevelIndicator();
}


void PCB_BASE_FRAME::updateGridSelectBox()
{
    UpdateStatusBar();
    DisplayUnitsMsg();

    if( m_gridSelectBox == NULL )
        return;

    // Update grid values with the current units setting.
    m_gridSelectBox->Clear();
    wxArrayString gridsList;
    int icurr = GetScreen()->BuildGridsChoiceList( gridsList, g_UserUnit != INCHES );

    for( size_t i = 0; i < GetScreen()->GetGridCount(); i++ )
    {
        GRID_TYPE& grid = GetScreen()->GetGrid( i );
        m_gridSelectBox->Append( gridsList[i], (void*) &grid.m_CmdId );
    }

    m_gridSelectBox->SetSelection( icurr );
}

void PCB_BASE_FRAME::updateZoomSelectBox()
{
    if( m_zoomSelectBox == NULL )
        return;

    wxString msg;

    m_zoomSelectBox->Clear();
    m_zoomSelectBox->Append( _( "Zoom Auto" ) );
    m_zoomSelectBox->SetSelection( 0 );

    for( unsigned i = 0;  i < GetScreen()->m_ZoomList.size();  ++i )
    {
        msg = _( "Zoom " );

        double level =  m_zoomLevelCoeff / (double)GetScreen()->m_ZoomList[i];
        wxString value = wxString::Format( wxT( "%.2f" ), level );
        msg += value;

        m_zoomSelectBox->Append( msg );

        if( GetScreen()->GetZoom() == GetScreen()->m_ZoomList[i] )
            m_zoomSelectBox->SetSelection( i + 1 );
    }
}


void PCB_BASE_FRAME::SetFastGrid1()
{
    if( m_FastGrid1 >= (int)GetScreen()->GetGridCount() )
        return;

    int cmdId = GetScreen()->GetGrids()[m_FastGrid1].m_CmdId;
    SetPresetGrid( cmdId - ID_POPUP_GRID_LEVEL_1000 );

    if( m_gridSelectBox )
    {
        wxCommandEvent cmd( wxEVT_CHOICE );
        cmd.SetEventObject( this );
        OnSelectGrid( cmd );
    }
    else
        GetCanvas()->Refresh();
}


void PCB_BASE_FRAME::SetFastGrid2()
{
    if( m_FastGrid2 >= (int)GetScreen()->GetGridCount() )
        return;

    int cmdId = GetScreen()->GetGrids()[m_FastGrid2].m_CmdId;
    SetPresetGrid( cmdId - ID_POPUP_GRID_LEVEL_1000 );

    if( m_gridSelectBox )
    {
        wxCommandEvent cmd( wxEVT_CHOICE );
        cmd.SetEventObject( this );
        OnSelectGrid( cmd );
    }
    else
        GetCanvas()->Refresh();
}

void PCB_BASE_FRAME::SetNextGrid()
{
    EDA_DRAW_FRAME::SetNextGrid();

    if( m_gridSelectBox )
    {
        wxCommandEvent cmd( wxEVT_CHOICE );
        cmd.SetEventObject( this );
        OnSelectGrid( cmd );
    }
    else
        GetCanvas()->Refresh();
}


void PCB_BASE_FRAME::SetPrevGrid()
{
    EDA_DRAW_FRAME::SetPrevGrid();

    if( m_gridSelectBox )
    {
        wxCommandEvent cmd( wxEVT_CHOICE );
        cmd.SetEventObject( this );
        OnSelectGrid( cmd );
    }
    else
        GetCanvas()->Refresh();
}


void PCB_BASE_FRAME::SwitchCanvas( wxCommandEvent& aEvent )
{
    bool use_gal = false;
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;

    switch( aEvent.GetId() )
    {
    case ID_MENU_CANVAS_DEFAULT:
        break;

    case ID_MENU_CANVAS_CAIRO:
        use_gal = GetGalCanvas()->SwitchBackend( EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO );

        if( use_gal )
            canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
        break;

    case ID_MENU_CANVAS_OPENGL:
        use_gal = GetGalCanvas()->SwitchBackend( EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL );

        if( use_gal )
            canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
        break;
    }

    SaveCanvasTypeSetting( canvasType );
    UseGalCanvas( use_gal );
}


void PCB_BASE_FRAME::UseGalCanvas( bool aEnable )
{
    EDA_DRAW_FRAME::UseGalCanvas( aEnable );

    EDA_DRAW_PANEL_GAL* galCanvas = GetGalCanvas();

    if( m_toolManager )
        m_toolManager->SetEnvironment( m_Pcb, GetGalCanvas()->GetView(),
                                    GetGalCanvas()->GetViewControls(), this );

    if( aEnable )
    {
        SetBoard( m_Pcb );

        if( m_toolManager )
            m_toolManager->ResetTools( TOOL_BASE::GAL_SWITCH );

        static_cast<PCB_DRAW_PANEL_GAL*>( galCanvas )->SyncLayersVisibility( m_Pcb );
        galCanvas->GetView()->RecacheAllItems( true );
        galCanvas->SetEventDispatcher( m_toolDispatcher );
        galCanvas->StartDrawing();
    }
    else
    {
        if( m_toolManager )
            m_toolManager->ResetTools( TOOL_BASE::GAL_SWITCH );

        // Redirect all events to the legacy canvas
        galCanvas->SetEventDispatcher( NULL );
    }
}


EDA_DRAW_PANEL_GAL::GAL_TYPE PCB_BASE_FRAME::LoadCanvasTypeSetting() const
{
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    wxConfigBase* cfg = Kiface().KifaceSettings();

    if( cfg )
        canvasType = (EDA_DRAW_PANEL_GAL::GAL_TYPE) cfg->ReadLong( CANVAS_TYPE_KEY,
                                                                   EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE );

    if( canvasType < EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            || canvasType >= EDA_DRAW_PANEL_GAL::GAL_TYPE_LAST )
    {
        assert( false );
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    }

    return canvasType;
}


bool PCB_BASE_FRAME::SaveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    if( aCanvasType < EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            || aCanvasType >= EDA_DRAW_PANEL_GAL::GAL_TYPE_LAST )
    {
        assert( false );
        return false;
    }

    wxConfigBase* cfg = Kiface().KifaceSettings();

    if( cfg )
        return cfg->Write( CANVAS_TYPE_KEY, (long) aCanvasType );

    return false;
}
