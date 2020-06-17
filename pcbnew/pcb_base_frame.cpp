/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <confirm.h>
#include <dialog_helpers.h>
#include <pcb_base_frame.h>
#include <base_units.h>
#include <msgpanel.h>
#include <pgm_base.h>
#include <3d_viewer/eda_3d_viewer.h>          // To include VIEWER3D_FRAMENAME
#include <pcbnew.h>
#include <footprint_editor_settings.h>
#include <fp_lib_table.h>
#include <pcbnew_id.h>
#include <class_board.h>
#include <class_module.h>
#include <class_drawsegment.h>
#include <collectors.h>
#include <pcb_draw_panel_gal.h>
#include <math/vector2d.h>
#include <trigo.h>
#include <pcb_painter.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/pcb_actions.h>
#include <tool/grid_menu.h>
#include "cleanup_item.h"

wxDEFINE_EVENT( BOARD_CHANGED, wxCommandEvent );

PCB_BASE_FRAME::PCB_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
        const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
        long aStyle, const wxString & aFrameName ) :
    EDA_DRAW_FRAME( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName ),
    m_Pcb( nullptr )
{
    m_Settings = static_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() );
}


PCB_BASE_FRAME::~PCB_BASE_FRAME()
{
    // Ensure m_canvasType is up to date, to save it in config
    m_canvasType = GetCanvas()->GetBackend();

    delete m_Pcb;
}


EDA_3D_VIEWER* PCB_BASE_FRAME::Get3DViewerFrame()
{
    return dynamic_cast<EDA_3D_VIEWER*>( FindWindowByName( QUALIFIED_VIEWER3D_FRAMENAME( this ) ) );
}


void PCB_BASE_FRAME::Update3DView( bool aForceReload, const wxString* aTitle )
{
    EDA_3D_VIEWER* draw3DFrame = Get3DViewerFrame();

    if( draw3DFrame )
    {
        if( aTitle )
            draw3DFrame->SetTitle( *aTitle );

        draw3DFrame->NewDisplay( aForceReload );
    }
}


FP_LIB_TABLE* PROJECT::PcbFootprintLibs()
{
    // This is a lazy loading function, it loads the project specific table when
    // that table is asked for, not before.

    FP_LIB_TABLE*   tbl = (FP_LIB_TABLE*) GetElem( ELEM_FPTBL );

    // its gotta be NULL or a FP_LIB_TABLE, or a bug.
    wxASSERT( !tbl || tbl->Type() == FP_LIB_TABLE_T );

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
            DisplayErrorMessage( nullptr, _( "Error loading project footprint libraries" ),
                                 ioe.What() );
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
        m_Pcb->SetGeneralSettings( m_Settings );

        wxCommandEvent e( BOARD_CHANGED );
        ProcessEventLocally( e );
    }
}


void PCB_BASE_FRAME::AddModuleToBoard( MODULE* module )
{
    if( module )
    {
        GetBoard()->Add( module, ADD_MODE::APPEND );

        module->SetFlags( IS_NEW );
        module->SetPosition( wxPoint( 0, 0 ) ); // cursor in GAL may not be initialized yet

        // Put it on FRONT layer (note that it might be stored flipped if the lib is an archive
        // built from a board)
        if( module->IsFlipped() )
            module->Flip( module->GetPosition(), m_Settings->m_FlipLeftRight );

        // Place it in orientation 0 even if it is not saved with orientation 0 in lib (note that
        // it might be stored in another orientation if the lib is an archive built from a board)
        module->SetOrientation( 0 );
    }
}


EDA_ITEM* PCB_BASE_FRAME::GetItem( const KIID& aId )
{
    return GetBoard()->GetItem( aId );
}


void PCB_BASE_FRAME::FocusOnItem( BOARD_ITEM* aItem )
{
    static KIID lastBrightenedItemID( niluuid );

    BOARD_ITEM* lastItem = GetBoard()->GetItem( lastBrightenedItemID );

    if( lastItem && lastItem != aItem )
    {
        lastItem->ClearBrightened();

        if( lastItem->Type() == PCB_MODULE_T )
        {
            static_cast<MODULE*>( lastItem )->RunOnChildren( [&] ( BOARD_ITEM* child )
            {
                child->ClearBrightened();
            });
        }

        GetCanvas()->GetView()->Update( lastItem );
        lastBrightenedItemID = niluuid;
        GetCanvas()->Refresh();
    }

    if( aItem )
    {
        aItem->SetBrightened();

        if( aItem->Type() == PCB_MODULE_T )
        {
            static_cast<MODULE*>( aItem )->RunOnChildren( [&] ( BOARD_ITEM* child )
            {
                child->SetBrightened();
            });
        }

        GetCanvas()->GetView()->Update( aItem );
        lastBrightenedItemID = aItem->m_Uuid;
        FocusOnLocation( aItem->GetFocusPosition() );
        GetCanvas()->Refresh();
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


void PCB_BASE_FRAME::SetDrawBgColor( COLOR4D aColor )
{
    m_drawBgColor= aColor;
    m_auimgr.Update();
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

    EDA_RECT area = aBoardEdgesOnly ? m_Pcb->GetBoardEdgesBoundingBox() : m_Pcb->GetBoundingBox();

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


// Virtual function
void PCB_BASE_FRAME::ReCreateMenuBar()
{
}


void PCB_BASE_FRAME::ShowChangedLanguage()
{
    // call my base class
    EDA_DRAW_FRAME::ShowChangedLanguage();

    // tooltips in toolbars
    RecreateToolbars();

    // status bar
    UpdateMsgPanel();
}


EDA_3D_VIEWER* PCB_BASE_FRAME::CreateAndShow3D_Frame()
{
    EDA_3D_VIEWER* draw3DFrame = Get3DViewerFrame();

    if( !draw3DFrame )
        draw3DFrame = new EDA_3D_VIEWER( &Kiway(), this, _( "3D Viewer" ) );

    // Raising the window does not show the window on Windows if iconized. This should work
    // on any platform.
    if( draw3DFrame->IsIconized() )
         draw3DFrame->Iconize( false );

    draw3DFrame->Raise();
    draw3DFrame->Show( true );

    // Raising the window does not set the focus on Linux.  This should work on any platform.
    if( wxWindow::FindFocus() != draw3DFrame )
        draw3DFrame->SetFocus();

    return draw3DFrame;
}


// Note: virtual, overridden in PCB_EDIT_FRAME;
void PCB_BASE_FRAME::SwitchLayer( wxDC* DC, PCB_LAYER_ID layer )
{
    PCB_LAYER_ID preslayer = GetActiveLayer();
    auto& displ_opts = GetDisplayOptions();

    // Check if the specified layer matches the present layer
    if( layer == preslayer )
        return;

    // Copper layers cannot be selected unconditionally; how many of those layers are
    // currently enabled needs to be checked.
    if( IsCopperLayer( layer ) )
    {
        // If only one copper layer is enabled, the only such layer that can be selected to
        // is the "Copper" layer (so the selection of any other copper layer is disregarded).
        if( m_Pcb->GetCopperLayerCount() < 2 )
        {
            if( layer != B_Cu )
                return;
        }

        // If more than one copper layer is enabled, the "Copper" and "Component" layers
        // can be selected, but the total number of copper layers determines which internal
        // layers are also capable of being selected.
        else
        {
            if( layer != B_Cu && layer != F_Cu && layer >= ( m_Pcb->GetCopperLayerCount() - 1 ) )
                return;
        }
    }

    // Is yet more checking required? E.g. when the layer to be selected is a non-copper
    // layer, or when switching between a copper layer and a non-copper layer, or vice-versa?
    // ...

    SetActiveLayer( layer );

    if( displ_opts.m_ContrastModeDisplay )
        GetCanvas()->Refresh();
}


GENERAL_COLLECTORS_GUIDE PCB_BASE_FRAME::GetCollectorsGuide()
{
    GENERAL_COLLECTORS_GUIDE guide( m_Pcb->GetVisibleLayers(), GetActiveLayer(),
                                    GetCanvas()->GetView() );

    // account for the globals
    guide.SetIgnoreMTextsMarkedNoShow( ! m_Pcb->IsElementVisible( LAYER_MOD_TEXT_INVISIBLE ) );
    guide.SetIgnoreMTextsOnBack( ! m_Pcb->IsElementVisible( LAYER_MOD_TEXT_BK ) );
    guide.SetIgnoreMTextsOnFront( ! m_Pcb->IsElementVisible( LAYER_MOD_TEXT_FR ) );
    guide.SetIgnoreModulesOnBack( ! m_Pcb->IsElementVisible( LAYER_MOD_BK ) );
    guide.SetIgnoreModulesOnFront( ! m_Pcb->IsElementVisible( LAYER_MOD_FR ) );
    guide.SetIgnorePadsOnBack( ! m_Pcb->IsElementVisible( LAYER_PAD_BK ) );
    guide.SetIgnorePadsOnFront( ! m_Pcb->IsElementVisible( LAYER_PAD_FR ) );
    guide.SetIgnoreThroughHolePads( ! m_Pcb->IsElementVisible( LAYER_PADS_TH ) );
    guide.SetIgnoreModulesVals( ! m_Pcb->IsElementVisible( LAYER_MOD_VALUES ) );
    guide.SetIgnoreModulesRefs( ! m_Pcb->IsElementVisible( LAYER_MOD_REFERENCES ) );
    guide.SetIgnoreThroughVias( ! m_Pcb->IsElementVisible( LAYER_VIA_THROUGH ) );
    guide.SetIgnoreBlindBuriedVias( ! m_Pcb->IsElementVisible( LAYER_VIA_BBLIND ) );
    guide.SetIgnoreMicroVias( ! m_Pcb->IsElementVisible( LAYER_VIA_MICROVIA ) );
    guide.SetIgnoreTracks( ! m_Pcb->IsElementVisible( LAYER_TRACKS ) );

    return guide;
}


/*
 * Display the grid status.
 */
void PCB_BASE_FRAME::DisplayGridMsg()
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


/*
 * Update the status bar information.
 */
void PCB_BASE_FRAME::UpdateStatusBar()
{
    EDA_DRAW_FRAME::UpdateStatusBar();

    BASE_SCREEN* screen = GetScreen();

    if( !screen )
        return;

    wxString line;
    VECTOR2D cursorPos = GetCanvas()->GetViewControls()->GetCursorPosition();

    if( GetShowPolarCoords() )  // display polar coordinates
    {
        double   dx = cursorPos.x - screen->m_LocalOrigin.x;
        double   dy = cursorPos.y - screen->m_LocalOrigin.y;
        double   theta = RAD2DEG( atan2( -dy, dx ) );
        double   ro = hypot( dx, dy );
        wxString formatter;

        switch( GetUserUnits() )
        {
        case EDA_UNITS::INCHES:
            formatter = wxT( "r %.6f  theta %.1f" );
            break;
        case EDA_UNITS::MILLIMETRES:
            formatter = wxT( "r %.6f  theta %.1f" );
            break;
        case EDA_UNITS::UNSCALED:
            formatter = wxT( "r %f  theta %f" );
            break;
        default:             wxASSERT( false );                       break;
        }

        line.Printf( formatter, To_User_Unit( GetUserUnits(), ro ), theta );

        SetStatusText( line, 3 );
    }

    // Display absolute coordinates:
    double dXpos = To_User_Unit( GetUserUnits(), cursorPos.x );
    double dYpos = To_User_Unit( GetUserUnits(), cursorPos.y );

    // The following sadly is an if Eeschema/if Pcbnew
    wxString absformatter;
    wxString locformatter;

    switch( GetUserUnits() )
    {
    case EDA_UNITS::INCHES:
        absformatter = "X %.6f  Y %.6f";
        locformatter = "dx %.6f  dy %.6f  dist %.4f";
        break;

    case EDA_UNITS::MILLIMETRES:
        absformatter = "X %.6f  Y %.6f";
        locformatter = "dx %.6f  dy %.6f  dist %.3f";
        break;

    case EDA_UNITS::UNSCALED:
        absformatter = "X %f  Y %f";
        locformatter = "dx %f  dy %f  dist %f";
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
        dXpos = To_User_Unit( GetUserUnits(), cursorPos.x - screen->m_LocalOrigin.x );
        dYpos = To_User_Unit( GetUserUnits(), cursorPos.y - screen->m_LocalOrigin.y );

        // We already decided the formatter above
        line.Printf( locformatter, dXpos, dYpos, hypot( dXpos, dYpos ) );
        SetStatusText( line, 3 );
    }

    DisplayGridMsg();
}


void PCB_BASE_FRAME::unitsChangeRefresh()
{
    EDA_DRAW_FRAME::unitsChangeRefresh();    // Update the status bar.

    // Notify all tools the units have changed
    if( m_toolManager )
        m_toolManager->RunAction( PCB_ACTIONS::updateUnits, true );

    UpdateGridSelectBox();
}


void PCB_BASE_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    if( aCfg->m_Window.grid.sizes.empty() )
    {
        aCfg->m_Window.grid.sizes = { "1000 mil",
                                      "500 mil",
                                      "250 mil",
                                      "200 mil",
                                      "100 mil",
                                      "50 mil",
                                      "25 mil",
                                      "20 mil",
                                      "10 mil",
                                      "5 mil",
                                      "2 mil",
                                      "1 mil",
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
        aCfg->m_Window.zoom_factors = { 0.035,
                                        0.05,
                                        0.08,
                                        0.13,
                                        0.22,
                                        0.35,
                                        0.6,
                                        1.0,
                                        1.5,
                                        2.2,
                                        3.5,
                                        5.0,
                                        8.0,
                                        13.0,
                                        20.0,
                                        35.0,
                                        50.0,
                                        80.0,
                                        130.0,
                                        220.0,
                                        300.0 };
    }

    for( double& factor : aCfg->m_Window.zoom_factors )
        factor = std::min( factor, MAX_ZOOM_FACTOR );

    // Some, but not all derived classes have a PCBNEW_SETTINGS.
    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );

    if( cfg )
    {
        m_DisplayOptions = cfg->m_Display;
        m_PolarCoords = cfg->m_PolarCoords;
    }
}


int PCB_BASE_FRAME::GetSeverity( int aErrorCode ) const
{
    if( aErrorCode >= CLEANUP_FIRST )
        return RPT_SEVERITY_ACTION;

    BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

    return bds.m_DRCSeverities[ aErrorCode ];
}


void PCB_BASE_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    // Some, but not all derived classes have a PCBNEW_SETTINGS.
    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );

    if( cfg )
    {
        cfg->m_Display = m_DisplayOptions;
        cfg->m_PolarCoords = m_PolarCoords;
    }
}


PCBNEW_SETTINGS* PCB_BASE_FRAME::GetPcbNewSettings()
{
    return Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();
}


FOOTPRINT_EDITOR_SETTINGS* PCB_BASE_FRAME::GetFootprintEditorSettings()
{
    return Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>();
}

MAGNETIC_SETTINGS* PCB_BASE_FRAME::GetMagneticItemsSettings()
{
    wxCHECK( m_Settings, nullptr );
    return &m_Settings->m_MagneticItems;
}


void PCB_BASE_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    EDA_DRAW_FRAME::CommonSettingsChanged( aEnvVarsChanged );

    RecreateToolbars();

    // The 3D viewer isn't in the Kiway, so send its update manually
    EDA_3D_VIEWER* viewer = Get3DViewerFrame();

    if( viewer )
        viewer->CommonSettingsChanged( aEnvVarsChanged );
}


void PCB_BASE_FRAME::OnModify()
{
    GetScreen()->SetModify();
    GetScreen()->SetSave();

    UpdateStatusBar();
    UpdateMsgPanel();
}


PCB_DRAW_PANEL_GAL* PCB_BASE_FRAME::GetCanvas() const
{
    return static_cast<PCB_DRAW_PANEL_GAL*>( EDA_DRAW_FRAME::GetCanvas() );
}


void PCB_BASE_FRAME::ActivateGalCanvas()
{
    EDA_DRAW_FRAME::ActivateGalCanvas();

    EDA_DRAW_PANEL_GAL* canvas = GetCanvas();

    if( m_toolManager )
    {
        m_toolManager->SetEnvironment( m_Pcb, GetCanvas()->GetView(),
                                       GetCanvas()->GetViewControls(), config(), this );
    }

    SetBoard( m_Pcb );

    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::GAL_SWITCH );

    // Transfer latest current display options from legacy to GAL canvas:
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( canvas->GetView()->GetPainter() );
    auto settings = painter->GetSettings();
    auto displ_opts = GetDisplayOptions();
    settings->LoadDisplayOptions( displ_opts, ShowPageLimits() );
    settings->LoadColors( GetColorSettings() );

    canvas->GetView()->RecacheAllItems();
    canvas->SetEventDispatcher( m_toolDispatcher );
    canvas->StartDrawing();
}

