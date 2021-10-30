/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>

#include <advanced_config.h>
#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <kiface_base.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <pcbnew_id.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <router/pns_routing_settings.h>
#include <router/router_tool.h>
#include <settings/color_settings.h>
#include <tool/action_toolbar.h>
#include <tool/actions.h>
#include <tool/common_tools.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <widgets/appearance_controls.h>
#include <widgets/wx_aui_utils.h>
#include <wx/wupdlock.h>
#include <wx/dcmemory.h>
#include <wx/choice.h>

#include "../scripting/python_scripting.h"


/* Data to build the layer pair indicator button */
static std::unique_ptr<wxBitmap> LayerPairBitmap;

#define BM_LAYERICON_SIZE 24
static const char s_BitmapLayerIcon[BM_LAYERICON_SIZE][BM_LAYERICON_SIZE] =
{
    // 0 = draw pixel with white
    // 1 = draw pixel with black
    // 2 = draw pixel with top layer from router pair
    // 3 = draw pixel with bottom layer from router pair
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
};

static COLOR4D ICON_WHITE { 0.86, 0.86, 0.86, 1.0 };
static COLOR4D ICON_BLACK { 0.28, 0.28, 0.28, 1.0 };


void PCB_EDIT_FRAME::PrepareLayerIndicator( bool aForceRebuild )
{
    int        ii, jj;
    COLOR4D    top_color, bottom_color, background_color;
    bool       change = aForceRebuild;

    int requested_scale = Pgm().GetCommonSettings()->m_Appearance.icon_scale;

    if( m_prevIconVal.previous_requested_scale != requested_scale )
    {
        m_prevIconVal.previous_requested_scale = requested_scale;
        change = true;
    }

    top_color = GetColorSettings()->GetColor( GetScreen()->m_Route_Layer_TOP );

    if( m_prevIconVal.previous_Route_Layer_TOP_color != top_color )
    {
        m_prevIconVal.previous_Route_Layer_TOP_color = top_color;
        change = true;
    }

    bottom_color = GetColorSettings()->GetColor( GetScreen()->m_Route_Layer_BOTTOM );

    if( m_prevIconVal.previous_Route_Layer_BOTTOM_color != bottom_color )
    {
        m_prevIconVal.previous_Route_Layer_BOTTOM_color = bottom_color;
        change = true;
    }

    background_color = GetColorSettings()->GetColor( LAYER_PCB_BACKGROUND );

    if( m_prevIconVal.previous_background_color != background_color )
    {
        m_prevIconVal.previous_background_color = background_color;
        change = true;
    }

    if( change || !LayerPairBitmap )
    {
        LayerPairBitmap = std::make_unique<wxBitmap>( 24, 24 );

        // Draw the icon, with colors according to the router's layer pair
        wxMemoryDC iconDC;
        iconDC.SelectObject( *LayerPairBitmap );
        wxBrush    brush;
        wxPen      pen;
        int buttonColor = -1;

        brush.SetStyle( wxBRUSHSTYLE_SOLID );
        brush.SetColour( background_color.WithAlpha(1.0).ToColour() );
        iconDC.SetBrush( brush );
        iconDC.DrawRectangle( 0, 0, BM_LAYERICON_SIZE, BM_LAYERICON_SIZE );

        for( ii = 0; ii < BM_LAYERICON_SIZE; ii++ )
        {
            for( jj = 0; jj < BM_LAYERICON_SIZE; jj++ )
            {
                if( s_BitmapLayerIcon[ii][jj] != buttonColor )
                {
                    switch( s_BitmapLayerIcon[ii][jj] )
                    {
                    default:
                    case 0: pen.SetColour( ICON_WHITE.ToColour() );   break;
                    case 1: pen.SetColour( ICON_BLACK.ToColour() );   break;
                    case 2: pen.SetColour( top_color.ToColour() );    break;
                    case 3: pen.SetColour( bottom_color.ToColour() ); break;
                    }

                    buttonColor = s_BitmapLayerIcon[ii][jj];
                    iconDC.SetPen( pen );
                }

                iconDC.DrawPoint( jj, ii );
            }
        }

        // Deselect the bitmap from the DC in order to delete the MemoryDC safely without
        // deleting the bitmap
        iconDC.SelectObject( wxNullBitmap );

        // Scale the bitmap
        const int scale = ( requested_scale <= 0 ) ? KiIconScale( this ) : requested_scale;
        wxImage image = LayerPairBitmap->ConvertToImage();

        // "NEAREST" causes less mixing of colors
        image.Rescale( scale * image.GetWidth() / 4, scale * image.GetHeight() / 4,
                       wxIMAGE_QUALITY_NEAREST );

        LayerPairBitmap = std::make_unique<wxBitmap>( image );

        if( m_mainToolBar )
        {
            m_mainToolBar->SetToolBitmap( PCB_ACTIONS::selectLayerPair, *LayerPairBitmap );
            m_mainToolBar->Refresh();
        }
    }
}


void PCB_EDIT_FRAME::ReCreateHToolbar()
{
    // Note:
    // To rebuild the aui toolbar, the more easy way is to clear ( calling m_mainToolBar.Clear() )
    // all wxAuiToolBarItems.
    // However the wxAuiToolBarItems are not the owners of controls managed by
    // them and therefore do not delete them
    // So we do not recreate them after clearing the tools.

    wxWindowUpdateLocker dummy( this );

    if( m_mainToolBar )
    {
        m_mainToolBar->ClearToolbar();
    }
    else
    {
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT |
                                            wxAUI_TB_HORIZONTAL );
        m_mainToolBar->SetAuiManager( &m_auimgr );

        // The layer indicator is special, so we register a callback directly that will
        // regenerate the bitmap instead of using the conditions system.
        auto layerIndicatorUpdate =
            [this] ( wxUpdateUIEvent& )
            {
                PrepareLayerIndicator();
            };

        Bind( wxEVT_UPDATE_UI, layerIndicatorUpdate, PCB_ACTIONS::selectLayerPair.GetUIId() );
    }

    // Set up toolbar
    if( Kiface().IsSingle() )
    {
        m_mainToolBar->Add( ACTIONS::doNew );
        m_mainToolBar->Add( ACTIONS::open );
    }

    m_mainToolBar->Add( ACTIONS::save );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( PCB_ACTIONS::boardSetup );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::pageSettings );
    m_mainToolBar->Add( ACTIONS::print );
    m_mainToolBar->Add( ACTIONS::plot );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::undo );
    m_mainToolBar->Add( ACTIONS::redo );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::find );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomFitObjects );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE, ACTION_TOOLBAR::CANCEL );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( PCB_ACTIONS::rotateCcw );
    m_mainToolBar->Add( PCB_ACTIONS::rotateCw );
    m_mainToolBar->Add( PCB_ACTIONS::group );
    m_mainToolBar->Add( PCB_ACTIONS::ungroup );
    m_mainToolBar->Add( PCB_ACTIONS::lock );
    m_mainToolBar->Add( PCB_ACTIONS::unlock );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::showFootprintEditor );
    m_mainToolBar->Add( ACTIONS::showFootprintBrowser );

    m_mainToolBar->AddScaledSeparator( this );

    if( !Kiface().IsSingle() )
        m_mainToolBar->Add( ACTIONS::updatePcbFromSchematic );
    else
        m_mainToolBar->Add( PCB_ACTIONS::importNetlist );

    m_mainToolBar->Add( PCB_ACTIONS::runDRC );

    m_mainToolBar->AddScaledSeparator( this );

    if( m_SelLayerBox == nullptr )
    {
        m_SelLayerBox = new PCB_LAYER_BOX_SELECTOR( m_mainToolBar, ID_TOOLBARH_PCB_SELECT_LAYER );
        m_SelLayerBox->SetBoardFrame( this );
    }

    ReCreateLayerBox( false );
    m_mainToolBar->AddControl( m_SelLayerBox );

    m_mainToolBar->Add( PCB_ACTIONS::selectLayerPair );
    PrepareLayerIndicator( true );    // Force rebuild of the bitmap with the active layer colors

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( PCB_ACTIONS::showEeschema );

    // Access to the scripting console
    if( SCRIPTING::IsWxAvailable() )
    {
        m_mainToolBar->AddScaledSeparator( this );
        m_mainToolBar->Add( PCB_ACTIONS::showPythonConsole, ACTION_TOOLBAR::TOGGLE );
        AddActionPluginTools();
    }

    // Go through and ensure the comboboxes are the correct size, since the strings in the
    // box could have changed widths.
    m_mainToolBar->UpdateControlWidth( ID_TOOLBARH_PCB_SELECT_LAYER );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->KiRealize();
}


void PCB_EDIT_FRAME::ReCreateOptToolbar()
{
    // Note:
    // To rebuild the aui toolbar, the more easy way is to clear ( calling m_mainToolBar.Clear() )
    // all wxAuiToolBarItems.
    // However the wxAuiToolBarItems are not the owners of controls managed by
    // them and therefore do not delete them
    // So we do not recreate them after clearing the tools.

    wxWindowUpdateLocker dummy( this );

    if( m_optionsToolBar )
    {
        m_optionsToolBar->ClearToolbar();
    }
    else
    {
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR,
                                               wxDefaultPosition, wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );
        m_optionsToolBar->SetAuiManager( &m_auimgr );
    }

    m_optionsToolBar->Add( ACTIONS::toggleGrid,               ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->Add( PCB_ACTIONS::togglePolarCoords,    ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::inchesUnits,              ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::milsUnits,                ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::millimetersUnits,         ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::toggleCursorStyle,        ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( PCB_ACTIONS::showRatsnest,         ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::ratsnestLineMode,     ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( ACTIONS::highContrastMode,         ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::toggleNetHighlight,   ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( PCB_ACTIONS::zoneDisplayFilled,    ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::zoneDisplayOutline,   ACTION_TOOLBAR::TOGGLE );

    if( ADVANCED_CFG::GetCfg().m_ExtraZoneDisplayModes )
    {
        m_optionsToolBar->Add( PCB_ACTIONS::zoneDisplayFractured,    ACTION_TOOLBAR::TOGGLE );
        m_optionsToolBar->Add( PCB_ACTIONS::zoneDisplayTriangulated, ACTION_TOOLBAR::TOGGLE );
    }

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( PCB_ACTIONS::padDisplayMode,       ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::viaDisplayMode,       ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::trackDisplayMode,     ACTION_TOOLBAR::TOGGLE );

    // Tools to show/hide toolbars:
    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( PCB_ACTIONS::showLayersManager,    ACTION_TOOLBAR::TOGGLE  );

    PCB_SELECTION_TOOL*          selTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>();
    std::unique_ptr<ACTION_MENU> gridMenu = std::make_unique<ACTION_MENU>( false, selTool );
    gridMenu->Add( ACTIONS::gridProperties );
    m_optionsToolBar->AddToolContextMenu( ACTIONS::toggleGrid, std::move( gridMenu ) );

    m_optionsToolBar->KiRealize();
}


void PCB_EDIT_FRAME::ReCreateVToolbar()
{
    wxWindowUpdateLocker dummy( this );

    if( m_drawToolBar )
    {
        m_drawToolBar->ClearToolbar();
    }
    else
    {
        m_drawToolBar = new ACTION_TOOLBAR( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );
        m_drawToolBar->SetAuiManager( &m_auimgr );
    }

    // Groups contained on this toolbar
    static ACTION_GROUP* dimensionGroup = nullptr;
    static ACTION_GROUP* originGroup    = nullptr;
    static ACTION_GROUP* routingGroup   = nullptr;
    static ACTION_GROUP* tuneGroup      = nullptr;

    if( !dimensionGroup )
    {
        dimensionGroup = new ACTION_GROUP( "group.pcbDimensions",
                                           { &PCB_ACTIONS::drawAlignedDimension,
                                             &PCB_ACTIONS::drawOrthogonalDimension,
                                             &PCB_ACTIONS::drawCenterDimension,
                                             &PCB_ACTIONS::drawLeader } );
    }

    if( !originGroup )
    {
        originGroup = new ACTION_GROUP( "group.pcbOrigins",
                                        { &PCB_ACTIONS::drillOrigin,
                                          &PCB_ACTIONS::gridSetOrigin } );
    }

    if( !routingGroup )
    {
        routingGroup = new ACTION_GROUP( "group.pcbRouting",
                                        { &PCB_ACTIONS::routeSingleTrack,
                                          &PCB_ACTIONS::routeDiffPair } );
    }

    if( !tuneGroup )
    {
        tuneGroup = new ACTION_GROUP( "group.pcbTune",
                                      { &PCB_ACTIONS::routerTuneSingleTrace,
                                        &PCB_ACTIONS::routerTuneDiffPair,
                                        &PCB_ACTIONS::routerTuneDiffPairSkew } );
    }

    m_drawToolBar->Add( ACTIONS::selectionTool,            ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::localRatsnestTool,    ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->AddScaledSeparator( this );
    m_drawToolBar->Add( PCB_ACTIONS::placeFootprint,       ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->AddGroup( routingGroup,                 ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->AddGroup( tuneGroup,                    ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawVia,              ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawZone,             ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawRuleArea,         ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->AddScaledSeparator( this );
    m_drawToolBar->Add( PCB_ACTIONS::drawLine,             ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawArc,              ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawRectangle,        ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawCircle,           ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawPolygon,          ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::placeText,            ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->AddGroup( dimensionGroup,               ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::placeTarget,          ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( ACTIONS::deleteTool,               ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->AddScaledSeparator( this );
    m_drawToolBar->AddGroup( originGroup,                  ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( ACTIONS::measureTool,              ACTION_TOOLBAR::TOGGLE );

    PCB_SELECTION_TOOL* selTool   = m_toolManager->GetTool<PCB_SELECTION_TOOL>();

    auto makeRouteMenu =
            [&]()
            {
                std::unique_ptr<ACTION_MENU> routeMenu =
                        std::make_unique<ACTION_MENU>( false, selTool );

                routeMenu->Add( PCB_ACTIONS::routerHighlightMode,  ACTION_MENU::CHECK );
                routeMenu->Add( PCB_ACTIONS::routerShoveMode,      ACTION_MENU::CHECK );
                routeMenu->Add( PCB_ACTIONS::routerWalkaroundMode, ACTION_MENU::CHECK );

                routeMenu->AppendSeparator();
                routeMenu->Add( PCB_ACTIONS::routerSettingsDialog );

                return routeMenu;
            };

    m_drawToolBar->AddToolContextMenu( PCB_ACTIONS::routeSingleTrack, makeRouteMenu() );
    m_drawToolBar->AddToolContextMenu( PCB_ACTIONS::routeDiffPair, makeRouteMenu() );

    auto makeTuneMenu =
        [&]()
        {
            std::unique_ptr<ACTION_MENU> tuneMenu = std::make_unique<ACTION_MENU>( false, selTool );
            tuneMenu->Add( PCB_ACTIONS::lengthTunerSettingsDialog );
            return tuneMenu;
        };

    m_drawToolBar->AddToolContextMenu( PCB_ACTIONS::routerTuneSingleTrace, makeTuneMenu() );
    m_drawToolBar->AddToolContextMenu( PCB_ACTIONS::routerTuneDiffPair, makeTuneMenu() );
    m_drawToolBar->AddToolContextMenu( PCB_ACTIONS::routerTuneDiffPairSkew, makeTuneMenu() );

    std::unique_ptr<ACTION_MENU> zoneMenu = std::make_unique<ACTION_MENU>( false, selTool );
    zoneMenu->Add( PCB_ACTIONS::zoneFillAll );
    zoneMenu->Add( PCB_ACTIONS::zoneUnfillAll );
    m_drawToolBar->AddToolContextMenu( PCB_ACTIONS::drawZone, std::move( zoneMenu ) );

    std::unique_ptr<ACTION_MENU> lineMenu = std::make_unique<ACTION_MENU>( false, selTool );
    m_drawToolBar->AddToolContextMenu( PCB_ACTIONS::drawLine, std::move( lineMenu ) );

    m_drawToolBar->KiRealize();
}


void PCB_EDIT_FRAME::ReCreateAuxiliaryToolbar()
{
    wxWindowUpdateLocker dummy( this );

    if( m_auxiliaryToolBar )
    {
        m_auxiliaryToolBar->ClearToolbar();
    }
    else
    {
        m_auxiliaryToolBar = new ACTION_TOOLBAR( this, ID_AUX_TOOLBAR, wxDefaultPosition,
                                                 wxDefaultSize,
                                                 KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );
        m_auxiliaryToolBar->SetAuiManager( &m_auimgr );
    }

    /* Set up toolbar items */

    // Creates box to display and choose tracks widths:
    if( m_SelTrackWidthBox == nullptr )
        m_SelTrackWidthBox = new wxChoice( m_auxiliaryToolBar, ID_AUX_TOOLBAR_PCB_TRACK_WIDTH,
                                           wxDefaultPosition, wxDefaultSize, 0, nullptr );

    UpdateTrackWidthSelectBox( m_SelTrackWidthBox );
    m_auxiliaryToolBar->AddControl( m_SelTrackWidthBox );
    m_SelTrackWidthBox->SetToolTip( _( "Select the default width for new tracks. Note that this "
                                       "width can be overridden by the board minimum width, or by "
                                       "the width of an existing track if the 'Use Existing Track "
                                       "Width' feature is enabled." ) );

    m_auxiliaryToolBar->AddTool( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH, wxEmptyString,
                                 KiScaledBitmap( BITMAPS::auto_track_width, this ),
                                 _( "When routing from an existing track use its width instead "
                                    "of the current width setting" ),
                                 wxITEM_CHECK );

    m_auxiliaryToolBar->AddScaledSeparator( this );

    // Creates box to display and choose vias diameters:

    if( m_SelViaSizeBox == nullptr )
        m_SelViaSizeBox = new wxChoice( m_auxiliaryToolBar, ID_AUX_TOOLBAR_PCB_VIA_SIZE,
                                        wxDefaultPosition, wxDefaultSize, 0, nullptr );

    UpdateViaSizeSelectBox( m_SelViaSizeBox );
    m_auxiliaryToolBar->AddControl( m_SelViaSizeBox );

    // Add the box to display and select the current grid size:
    m_auxiliaryToolBar->AddScaledSeparator( this );

    if( m_gridSelectBox == nullptr )
        m_gridSelectBox = new wxChoice( m_auxiliaryToolBar, ID_ON_GRID_SELECT,
                                        wxDefaultPosition, wxDefaultSize, 0, nullptr );

    UpdateGridSelectBox();

    m_auxiliaryToolBar->AddControl( m_gridSelectBox );

    //  Add the box to display and select the current Zoom
    m_auxiliaryToolBar->AddScaledSeparator( this );

    if( m_zoomSelectBox == nullptr )
        m_zoomSelectBox = new wxChoice( m_auxiliaryToolBar, ID_ON_ZOOM_SELECT,
                                        wxDefaultPosition, wxDefaultSize, 0, nullptr );

    UpdateZoomSelectBox();
    m_auxiliaryToolBar->AddControl( m_zoomSelectBox );

    // Go through and ensure the comboboxes are the correct size, since the strings in the
    // box could have changed widths.
    m_auxiliaryToolBar->UpdateControlWidth( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH );
    m_auxiliaryToolBar->UpdateControlWidth( ID_AUX_TOOLBAR_PCB_VIA_SIZE );
    m_auxiliaryToolBar->UpdateControlWidth( ID_ON_ZOOM_SELECT );
    m_auxiliaryToolBar->UpdateControlWidth( ID_ON_GRID_SELECT );

    // after adding the buttons to the toolbar, must call Realize()
    m_auxiliaryToolBar->KiRealize();
}


void PCB_EDIT_FRAME::UpdateToolbarControlSizes()
{
    if( m_mainToolBar )
    {
        // Update the item widths
        m_mainToolBar->UpdateControlWidth( ID_TOOLBARH_PCB_SELECT_LAYER );
    }

    if( m_auxiliaryToolBar )
    {
        // Update the item widths
        m_auxiliaryToolBar->UpdateControlWidth( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH );
        m_auxiliaryToolBar->UpdateControlWidth( ID_AUX_TOOLBAR_PCB_VIA_SIZE );
        m_auxiliaryToolBar->UpdateControlWidth( ID_ON_ZOOM_SELECT );
        m_auxiliaryToolBar->UpdateControlWidth( ID_ON_GRID_SELECT );
    }
}


static wxString ComboBoxUnits( EDA_UNITS aUnits, double aValue, bool aIncludeLabel = true )
{
    wxString      text;
    const wxChar* format;

    switch( aUnits )
    {
    default:                     wxASSERT_MSG( false, "Invalid unit" ); KI_FALLTHROUGH;
    case EDA_UNITS::UNSCALED:    format = wxT( "%.0f" );                break;
    case EDA_UNITS::MILLIMETRES: format = wxT( "%.3f" );                break;
    case EDA_UNITS::MILS:        format = wxT( "%.2f" );                break;
    case EDA_UNITS::INCHES:      format = wxT( "%.5f" );                break;
    }

    text.Printf( format, To_User_Unit( aUnits, aValue ) );

    if( aIncludeLabel )
    {
        text += " ";
        text += GetAbbreviatedUnitsLabel( aUnits, EDA_DATA_TYPE::DISTANCE );
    }

    return text;
}


void PCB_EDIT_FRAME::UpdateTrackWidthSelectBox( wxChoice* aTrackWidthSelectBox, bool aEdit )
{
    if( aTrackWidthSelectBox == nullptr )
        return;

    EDA_UNITS primaryUnit;
    EDA_UNITS secondaryUnit;

    GetUnitPair( primaryUnit, secondaryUnit );

    wxString msg;

    aTrackWidthSelectBox->Clear();

    aTrackWidthSelectBox->Append( _( "Track: use netclass width" ) );

    for( unsigned ii = 1; ii < GetDesignSettings().m_TrackWidthList.size(); ii++ )
    {
        int size = GetDesignSettings().m_TrackWidthList[ii];

        msg.Printf( _( "Track: %s (%s)" ), ComboBoxUnits( primaryUnit, size ),
                                           ComboBoxUnits( secondaryUnit, size ) );

        aTrackWidthSelectBox->Append( msg );
    }

    if( aEdit )
    {
        aTrackWidthSelectBox->Append( wxT( "---" ) );
        aTrackWidthSelectBox->Append( _( "Edit Pre-defined Sizes..." ) );
    }

    if( GetDesignSettings().GetTrackWidthIndex() >= GetDesignSettings().m_TrackWidthList.size() )
        GetDesignSettings().SetTrackWidthIndex( 0 );

    aTrackWidthSelectBox->SetSelection( GetDesignSettings().GetTrackWidthIndex() );
}


void PCB_EDIT_FRAME::UpdateViaSizeSelectBox( wxChoice* aViaSizeSelectBox, bool aEdit )
{
    if( aViaSizeSelectBox == nullptr )
        return;

    aViaSizeSelectBox->Clear();

    COMMON_TOOLS* cmnTool  = m_toolManager->GetTool<COMMON_TOOLS>();

    EDA_UNITS primaryUnit   = GetUserUnits();
    EDA_UNITS secondaryUnit = EDA_UNITS::MILS;

    if( EDA_UNIT_UTILS::IsImperialUnit( primaryUnit ) )
    {
        if( cmnTool )
            secondaryUnit = cmnTool->GetLastMetricUnits();
        else
            secondaryUnit = EDA_UNITS::MILLIMETRES;
    }
    else
    {
        if( cmnTool )
            secondaryUnit = cmnTool->GetLastImperialUnits();
        else
            secondaryUnit = EDA_UNITS::MILS;
    }

    aViaSizeSelectBox->Append( _( "Via: use netclass sizes" ) );

    for( unsigned ii = 1; ii < GetDesignSettings().m_ViasDimensionsList.size(); ii++ )
    {
        VIA_DIMENSION viaDimension = GetDesignSettings().m_ViasDimensionsList[ii];
        wxString      msg, priStr, secStr;

        double diam = viaDimension.m_Diameter;
        double hole = viaDimension.m_Drill;

        if( hole > 0 )
        {
            priStr = ComboBoxUnits( primaryUnit, diam, false ) + " / "
                        + ComboBoxUnits( primaryUnit, hole, true );
            secStr = ComboBoxUnits( secondaryUnit, diam, false ) + " / "
                        + ComboBoxUnits( secondaryUnit, hole, true );
        }
        else
        {
            priStr = ComboBoxUnits( primaryUnit, diam, true );
            secStr = ComboBoxUnits( secondaryUnit, diam, true );
        }

        msg.Printf( _( "Via: %s (%s)" ), priStr, secStr );

        aViaSizeSelectBox->Append( msg );
    }

    if( aEdit )
    {
        aViaSizeSelectBox->Append( wxT( "---" ) );
        aViaSizeSelectBox->Append( _( "Edit Pre-defined Sizes..." ) );
    }

    if( GetDesignSettings().GetViaSizeIndex() >= GetDesignSettings().m_ViasDimensionsList.size() )
        GetDesignSettings().SetViaSizeIndex( 0 );

    aViaSizeSelectBox->SetSelection( GetDesignSettings().GetViaSizeIndex() );
}


void PCB_EDIT_FRAME::ReCreateLayerBox( bool aForceResizeToolbar )
{
    if( m_SelLayerBox == nullptr || m_mainToolBar == nullptr )
        return;

    m_SelLayerBox->SetToolTip( _( "+/- to switch" ) );
    m_SelLayerBox->Resync();

    if( aForceResizeToolbar )
        UpdateToolbarControlSizes();
}


void PCB_EDIT_FRAME::ToggleLayersManager()
{
    PCBNEW_SETTINGS* settings      = GetPcbNewSettings();
    wxAuiPaneInfo&   layersManager = m_auimgr.GetPane( "LayersManager" );
    wxAuiPaneInfo&   selectionFilter = m_auimgr.GetPane( "SelectionFilter" );

    // show auxiliary Vertical layers and visibility manager toolbar
    m_show_layer_manager_tools = !m_show_layer_manager_tools;
    layersManager.Show( m_show_layer_manager_tools );
    selectionFilter.Show( m_show_layer_manager_tools );

    if( m_show_layer_manager_tools )
    {
        SetAuiPaneSize( m_auimgr, layersManager, settings->m_AuiPanels.right_panel_width, -1 );
    }
    else
    {
        settings->m_AuiPanels.right_panel_width = m_appearancePanel->GetSize().x;
        m_auimgr.Update();
    }
}


void PCB_EDIT_FRAME::OnUpdateSelectTrackWidth( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetId() == ID_AUX_TOOLBAR_PCB_TRACK_WIDTH )
    {
        BOARD_DESIGN_SETTINGS& bds = GetDesignSettings();
        int                    sel;

        if( bds.UseCustomTrackViaSize() )
            sel = wxNOT_FOUND;
        else
            sel = bds.GetTrackWidthIndex();

        if( m_SelTrackWidthBox->GetSelection() != sel )
            m_SelTrackWidthBox->SetSelection( sel );
    }
}


void PCB_EDIT_FRAME::OnUpdateSelectViaSize( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetId() == ID_AUX_TOOLBAR_PCB_VIA_SIZE )
    {
        BOARD_DESIGN_SETTINGS& bds = GetDesignSettings();
        int                    sel = 0;

        if( bds.UseCustomTrackViaSize() )
            sel = wxNOT_FOUND;
        else
            sel = bds.GetViaSizeIndex();

        if( m_SelViaSizeBox->GetSelection() != sel )
            m_SelViaSizeBox->SetSelection( sel );
    }
}


void PCB_EDIT_FRAME::OnUpdateSelectAutoWidth( wxUpdateUIEvent& aEvent )
{
    BOARD_DESIGN_SETTINGS& bds = GetDesignSettings();

    aEvent.Check( bds.m_UseConnectedTrackWidth && !bds.UseCustomTrackViaSize() );
}


void PCB_EDIT_FRAME::OnUpdateLayerSelectBox( wxUpdateUIEvent& aEvent )
{
    if( m_SelLayerBox->GetLayerSelection() != GetActiveLayer() )
        m_SelLayerBox->SetLayerSelection( GetActiveLayer() );
}
