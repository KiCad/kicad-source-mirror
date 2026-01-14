/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <string>

#include <advanced_config.h>
#include <api/api_plugin_manager.h>
#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <kiface_base.h>
#include <kiplatform/ui.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <pcbnew_id.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <router/pns_routing_settings.h>
#include <router/router_tool.h>
#include <settings/color_settings.h>
#include <tool/action_menu.h>
#include <tool/action_toolbar.h>
#include <tool/actions.h>
#include <tool/common_tools.h>
#include <tool/tool_manager.h>
#include <tool/ui/toolbar_context_menu_registry.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <widgets/appearance_controls.h>
#include <widgets/pcb_design_block_pane.h>
#include <widgets/layer_box_selector.h>
#include <widgets/layer_presentation.h>
#include <widgets/pcb_properties_panel.h>
#include <widgets/net_inspector_panel.h>
#include <widgets/pcb_search_pane.h>
#include <widgets/wx_aui_utils.h>
#include <wx/wupdlock.h>
#include <wx/combobox.h>
#include <toolbars_pcb_editor.h>
#include <settings/settings_manager.h>

#include "../scripting/python_scripting.h"


/* Data to build the layer pair indicator button */
static wxBitmapBundle LayerPairBitmap;


void PCB_EDIT_FRAME::PrepareLayerIndicator( bool aForceRebuild )
{
    COLOR4D    top_color, bottom_color, background_color;
    bool       change = aForceRebuild;

    int icon_size = Pgm().GetCommonSettings()->m_Appearance.toolbar_icon_size;

    if( m_prevIconVal.previous_icon_size != icon_size )
    {
        m_prevIconVal.previous_icon_size = icon_size;
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

    if( change || !LayerPairBitmap.IsOk() )
    {
        LayerPairBitmap = LAYER_PRESENTATION::CreateLayerPairIcon( top_color, bottom_color, icon_size );

        if( m_tbTopAux )
        {
            m_tbTopAux->SetToolBitmap( PCB_ACTIONS::selectLayerPair, LayerPairBitmap );
            m_tbTopAux->Refresh();
        }
    }
}


ACTION_TOOLBAR_CONTROL PCB_ACTION_TOOLBAR_CONTROLS::trackWidth( "control.PCBTrackWidth",
                                                                _( "Track width selector" ),
                                                                _( "Control to select the track width" ),
                                                                { FRAME_PCB_EDITOR } );
ACTION_TOOLBAR_CONTROL PCB_ACTION_TOOLBAR_CONTROLS::viaDiameter( "control.PCBViaDia",
                                                                 _( "Via diameter selector" ),
                                                                 _( "Control to select the via diameter" ),
                                                                 { FRAME_PCB_EDITOR } );
ACTION_TOOLBAR_CONTROL PCB_ACTION_TOOLBAR_CONTROLS::currentVariant( "control.PCBCurrentVariant",
                                                                    _( "Current variant" ),
                                                                    _( "Control to select the current variant" ),
                                                                    { FRAME_PCB_EDITOR } );


std::optional<TOOLBAR_CONFIGURATION> PCB_EDIT_TOOLBAR_SETTINGS::DefaultToolbarConfig( TOOLBAR_LOC aToolbar )
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    switch( aToolbar )
    {
    case TOOLBAR_LOC::LEFT:
        config.AppendAction( ACTIONS::toggleGrid )
              .WithContextMenu(
                  []( TOOL_MANAGER* aMgr ) -> std::unique_ptr<ACTION_MENU>
                  {
                      PCB_SELECTION_TOOL* selTool = aMgr->GetTool<PCB_SELECTION_TOOL>();
                      std::unique_ptr<ACTION_MENU> menu =
                              std::make_unique<ACTION_MENU>( false, selTool );

                      menu->Add( ACTIONS::gridProperties );
                      menu->Add( ACTIONS::gridOrigin );

                      return menu;
                  } )
              .AppendAction( ACTIONS::toggleGridOverrides )
              .AppendAction( PCB_ACTIONS::togglePolarCoords )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Units" ) )
                            .AddAction( ACTIONS::millimetersUnits )
                            .AddAction( ACTIONS::inchesUnits )
                            .AddAction( ACTIONS::milsUnits ) )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Crosshair modes" ) )
                            .AddAction( ACTIONS::cursorSmallCrosshairs )
                            .AddAction( ACTIONS::cursorFullCrosshairs )
                            .AddAction( ACTIONS::cursor45Crosshairs ) );

        config.AppendSeparator()
            .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Line modes" ) )
                        .AddAction( PCB_ACTIONS::lineModeFree )
                        .AddAction( PCB_ACTIONS::lineMode90 )
                        .AddAction( PCB_ACTIONS::lineMode45 ) );

        config.AppendSeparator()
              .AppendAction( PCB_ACTIONS::showRatsnest )
              .AppendAction( PCB_ACTIONS::ratsnestLineMode );

        config.AppendSeparator()
              .AppendAction( ACTIONS::highContrastMode )
              .AppendAction( PCB_ACTIONS::toggleNetHighlight );

        config.AppendSeparator()
              .AppendAction( PCB_ACTIONS::zoneDisplayFilled )
              .AppendAction( PCB_ACTIONS::zoneDisplayOutline );

        if( ADVANCED_CFG::GetCfg().m_ExtraZoneDisplayModes )
        {
            config.AppendAction( PCB_ACTIONS::zoneDisplayFractured );
            config.AppendAction( PCB_ACTIONS::zoneDisplayTriangulated );
        }

        config.AppendSeparator()
              .AppendAction( PCB_ACTIONS::padDisplayMode )
              .AppendAction( PCB_ACTIONS::viaDisplayMode )
              .AppendAction( PCB_ACTIONS::trackDisplayMode );

        if( ADVANCED_CFG::GetCfg().m_DrawBoundingBoxes )
            config.AppendAction( ACTIONS::toggleBoundingBoxes );

        // Tools to show/hide toolbars:
        config.AppendSeparator()
              .AppendAction( PCB_ACTIONS::showLayersManager )
              .AppendAction( ACTIONS::showProperties );

        break;

    case TOOLBAR_LOC::RIGHT:
        config.AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Selection modes" ) )
                            .AddAction( ACTIONS::selectSetRect )
                            .AddAction( ACTIONS::selectSetLasso ) )
              .AppendAction( PCB_ACTIONS::localRatsnestTool );

        config.AppendSeparator()
              .AppendAction( PCB_ACTIONS::placeFootprint )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Track routing tools" ) )
                            .AddAction( PCB_ACTIONS::routeSingleTrack )
                            .AddAction( PCB_ACTIONS::routeDiffPair )
                            .AddContextMenu(
                                []( TOOL_MANAGER* aMgr ) -> std::unique_ptr<ACTION_MENU>
                                {
                                    PCB_SELECTION_TOOL* selTool = aMgr->GetTool<PCB_SELECTION_TOOL>();
                                    std::unique_ptr<ACTION_MENU> menu =
                                            std::make_unique<ACTION_MENU>( false, selTool );

                                    menu->Add( PCB_ACTIONS::routerHighlightMode, ACTION_MENU::CHECK );
                                    menu->Add( PCB_ACTIONS::routerShoveMode, ACTION_MENU::CHECK );
                                    menu->Add( PCB_ACTIONS::routerWalkaroundMode, ACTION_MENU::CHECK );
                                    menu->AppendSeparator();
                                    menu->Add( PCB_ACTIONS::routerSettingsDialog );

                                    return menu;
                                } ) )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Track tuning tools" ) )
                            .AddAction( PCB_ACTIONS::tuneSingleTrack )
                            .AddAction( PCB_ACTIONS::tuneDiffPair )
                            .AddAction( PCB_ACTIONS::tuneSkew ) )
              .AppendAction( PCB_ACTIONS::drawVia )
              .AppendAction( PCB_ACTIONS::drawZone )
              .WithContextMenu(
                  []( TOOL_MANAGER* aMgr ) -> std::unique_ptr<ACTION_MENU>
                  {
                      PCB_SELECTION_TOOL* selTool = aMgr->GetTool<PCB_SELECTION_TOOL>();
                      std::unique_ptr<ACTION_MENU> menu =
                              std::make_unique<ACTION_MENU>( false, selTool );

                      menu->Add( PCB_ACTIONS::zoneFillAll );
                      menu->Add( PCB_ACTIONS::zoneUnfillAll );

                      return menu;
                  } )
              .AppendAction( PCB_ACTIONS::drawRuleArea );

        config.AppendSeparator()
              .AppendAction( PCB_ACTIONS::drawLine )
              .AppendAction( PCB_ACTIONS::drawArc )
              .WithContextMenu(
                  []( TOOL_MANAGER* aMgr ) -> std::unique_ptr<ACTION_MENU>
                  {
                      PCB_SELECTION_TOOL* selTool = aMgr->GetTool<PCB_SELECTION_TOOL>();
                      std::unique_ptr<ACTION_MENU> menu =
                              std::make_unique<ACTION_MENU>( false, selTool );

                      menu->Add( ACTIONS::pointEditorArcKeepCenter, ACTION_MENU::CHECK  );
                      menu->Add( ACTIONS::pointEditorArcKeepEndpoint, ACTION_MENU::CHECK  );
                      menu->Add( ACTIONS::pointEditorArcKeepRadius, ACTION_MENU::CHECK  );

                      return menu;
                  } )
              .AppendAction( PCB_ACTIONS::drawRectangle )
              .AppendAction( PCB_ACTIONS::drawCircle )
              .AppendAction( PCB_ACTIONS::drawPolygon )
              .AppendAction( PCB_ACTIONS::drawBezier )
              .AppendAction( PCB_ACTIONS::placeReferenceImage )
              .AppendAction( PCB_ACTIONS::placeText )
              .AppendAction( PCB_ACTIONS::drawTextBox )
              .AppendAction( PCB_ACTIONS::drawTable )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Dimension objects" ) )
                            .AddAction( PCB_ACTIONS::drawOrthogonalDimension )
                            .AddAction( PCB_ACTIONS::drawAlignedDimension )
                            .AddAction( PCB_ACTIONS::drawCenterDimension )
                            .AddAction( PCB_ACTIONS::drawRadialDimension )
                            .AddAction( PCB_ACTIONS::drawLeader ) )
              .AppendAction( PCB_ACTIONS::placeBarcode )
              .AppendAction( ACTIONS::deleteTool );

        config.AppendSeparator()
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "PCB origins and points" ) )
                            .AddAction( ACTIONS::gridSetOrigin )
                            .AddAction( PCB_ACTIONS::drillOrigin ) )
                            .AppendAction( PCB_ACTIONS::placePoint )
              .AppendAction( ACTIONS::measureTool );

        break;

    case TOOLBAR_LOC::TOP_MAIN:
        if( Kiface().IsSingle() )
        {
            config.AppendAction( ACTIONS::doNew );
            config.AppendAction( ACTIONS::open );
        }

        config.AppendAction( ACTIONS::save );

        config.AppendSeparator()
              .AppendAction( PCB_ACTIONS::boardSetup );

        config.AppendSeparator()
              .AppendAction( ACTIONS::pageSettings )
              .AppendAction( ACTIONS::print )
              .AppendAction( ACTIONS::plot );

        config.AppendSeparator()
              .AppendAction( ACTIONS::undo )
              .AppendAction( ACTIONS::redo );

        config.AppendSeparator()
              .AppendAction( ACTIONS::find );

        config.AppendSeparator()
              .AppendAction( ACTIONS::zoomRedraw )
              .AppendAction( ACTIONS::zoomInCenter )
              .AppendAction( ACTIONS::zoomOutCenter )
              .AppendAction( ACTIONS::zoomFitScreen )
              .AppendAction( ACTIONS::zoomFitObjects )
              .AppendAction( ACTIONS::zoomTool );

        config.AppendSeparator()
              .AppendAction( PCB_ACTIONS::rotateCcw )
              .AppendAction( PCB_ACTIONS::rotateCw )
              .AppendAction( PCB_ACTIONS::mirrorV )
              .AppendAction( PCB_ACTIONS::mirrorH )
              .AppendAction( ACTIONS::group )
              .AppendAction( ACTIONS::ungroup )
              .AppendAction( PCB_ACTIONS::lock )
              .AppendAction( PCB_ACTIONS::unlock );

        config.AppendSeparator()
              .AppendAction( ACTIONS::showFootprintEditor )
              .AppendAction( ACTIONS::showFootprintBrowser )
              .AppendAction( ACTIONS::show3DViewer );

        config.AppendSeparator();

        if( !Kiface().IsSingle() )
            config.AppendAction( ACTIONS::updatePcbFromSchematic );
        else
            config.AppendAction( PCB_ACTIONS::importNetlist );

        config.AppendAction( PCB_ACTIONS::runDRC );

        config.AppendSeparator();
        config.AppendAction( PCB_ACTIONS::showEeschema );
        config.AppendControl( PCB_ACTION_TOOLBAR_CONTROLS::currentVariant );
        config.AppendControl( ACTION_TOOLBAR_CONTROLS::ipcScripting );

        break;

    case TOOLBAR_LOC::TOP_AUX:
        config.AppendControl( PCB_ACTION_TOOLBAR_CONTROLS::trackWidth )
              .AppendAction( PCB_ACTIONS::autoTrackWidth );

        config.AppendSeparator()
              .AppendControl( PCB_ACTION_TOOLBAR_CONTROLS::viaDiameter );

        config.AppendSeparator()
              .AppendControl( ACTION_TOOLBAR_CONTROLS::layerSelector )
              .AppendAction( PCB_ACTIONS::selectLayerPair );

        config.AppendSeparator()
              .AppendControl( ACTION_TOOLBAR_CONTROLS::gridSelect );

        config.AppendSeparator()
              .AppendControl( ACTION_TOOLBAR_CONTROLS::zoomSelect );

        config.AppendSeparator()
              .AppendControl( ACTION_TOOLBAR_CONTROLS::overrideLocks );

        break;
    }

    // clang-format on
    return config;
}


void PCB_EDIT_FRAME::configureToolbars()
{
    PCB_BASE_EDIT_FRAME::configureToolbars();

    // Box to display and choose track widths
    auto trackWidthSelectorFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                if( !m_SelTrackWidthBox )
                {
                    m_SelTrackWidthBox = new wxChoice( aToolbar, ID_AUX_TOOLBAR_PCB_TRACK_WIDTH,
                                                       wxDefaultPosition, wxDefaultSize, 0, nullptr );
                }

                m_SelTrackWidthBox->SetToolTip( _( "Select the default width for new tracks. Note that this "
                                                   "width can be overridden by the board minimum width, or by "
                                                   "the width of an existing track if the 'Use Existing Track "
                                                   "Width' feature is enabled." ) );

                UpdateTrackWidthSelectBox( m_SelTrackWidthBox, true, true );

                aToolbar->Add( m_SelTrackWidthBox );
            };

    RegisterCustomToolbarControlFactory( PCB_ACTION_TOOLBAR_CONTROLS::trackWidth, trackWidthSelectorFactory );


    // Box to display and choose vias diameters
    auto viaDiaSelectorFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                if( !m_SelViaSizeBox )
                {
                    m_SelViaSizeBox = new wxChoice( aToolbar, ID_AUX_TOOLBAR_PCB_VIA_SIZE,
                                                    wxDefaultPosition, wxDefaultSize, 0, nullptr );
                }

                UpdateViaSizeSelectBox( m_SelViaSizeBox, true, true );
                aToolbar->Add( m_SelViaSizeBox );
            };

    RegisterCustomToolbarControlFactory( PCB_ACTION_TOOLBAR_CONTROLS::viaDiameter, viaDiaSelectorFactory );

    // Variant selection drop down control on main tool bar
    auto variantSelectionCtrlFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                if( !m_currentVariantCtrl )
                {
                    m_currentVariantCtrl = new wxChoice( aToolbar, ID_AUX_TOOLBAR_PCB_VARIANT_SELECT,
                                                         wxDefaultPosition, wxDefaultSize, 0, nullptr );
                }

                m_currentVariantCtrl->SetToolTip( _( "Select the current variant to display and edit." ) );

                UpdateVariantSelectionCtrl();

                aToolbar->Add( m_currentVariantCtrl );
            };

    RegisterCustomToolbarControlFactory( PCB_ACTION_TOOLBAR_CONTROLS::currentVariant, variantSelectionCtrlFactory );

    // IPC/Scripting plugin control
    // TODO (ISM): Clean this up to make IPC actions just normal tool actions to get rid of this entire
    // control
    auto pluginControlFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                // Add scripting console and API plugins
                bool scriptingAvailable = SCRIPTING::IsWxAvailable();

#ifdef KICAD_IPC_API
                bool haveApiPlugins = Pgm().GetCommonSettings()->m_Api.enable_server
                                        && !Pgm().GetPluginManager().GetActionsForScope( PluginActionScope() ).empty();
#else
                bool haveApiPlugins = false;
#endif

                if( scriptingAvailable || haveApiPlugins )
                {
                    aToolbar->AddScaledSeparator( aToolbar->GetParent() );

                    if( scriptingAvailable )
                    {
                        aToolbar->Add( PCB_ACTIONS::showPythonConsole );
                        addActionPluginTools( aToolbar );
                    }

                    if( haveApiPlugins )
                        AddApiPluginTools( aToolbar );
                }
            };

    RegisterCustomToolbarControlFactory( ACTION_TOOLBAR_CONTROLS::ipcScripting, pluginControlFactory );
}


void PCB_EDIT_FRAME::ClearToolbarControl( int aId )
{
    PCB_BASE_EDIT_FRAME::ClearToolbarControl( aId );

    switch( aId )
    {
    case ID_AUX_TOOLBAR_PCB_TRACK_WIDTH:    m_SelTrackWidthBox = nullptr;   break;
    case ID_AUX_TOOLBAR_PCB_VIA_SIZE:       m_SelViaSizeBox = nullptr;      break;
    case ID_AUX_TOOLBAR_PCB_VARIANT_SELECT: m_currentVariantCtrl = nullptr; break;
    }
}


void PCB_EDIT_FRAME::UpdateVariantSelectionCtrl()
{
    if( !m_currentVariantCtrl )
        return;

    if( !GetBoard() )
        return;

    wxArrayString variantNames = GetBoard()->GetVariantNamesForUI();

    m_currentVariantCtrl->Set( variantNames );

    int selectionIndex = 0;
    wxString currentVariant = GetBoard()->GetCurrentVariant();

    if( !currentVariant.IsEmpty() )
    {
        int foundIndex = m_currentVariantCtrl->FindString( currentVariant );

        if( foundIndex != wxNOT_FOUND )
            selectionIndex = foundIndex;
    }

    if( m_currentVariantCtrl->GetCount() > 0 )
        m_currentVariantCtrl->SetSelection( selectionIndex );
}


void PCB_EDIT_FRAME::onVariantSelected( wxCommandEvent& aEvent )
{
    if( !m_currentVariantCtrl )
        return;

    int selection = m_currentVariantCtrl->GetSelection();

    if( selection == wxNOT_FOUND || selection == 0 )
    {
        // "<Default>" selected - clear the current variant
        GetBoard()->SetCurrentVariant( wxEmptyString );
    }
    else
    {
        wxString selectedVariant = m_currentVariantCtrl->GetString( selection );
        GetBoard()->SetCurrentVariant( selectedVariant );
    }

    // Refresh the view and properties panel to show the new variant state
    UpdateProperties();
    GetCanvas()->Refresh();
}


static wxString ComboBoxUnits( EDA_UNITS aUnits, double aValue, bool aIncludeLabel = true )
{
    wxString      text;
    const wxChar* format;

    switch( aUnits )
    {
    default:
        wxASSERT_MSG( false, wxT( "Invalid unit" ) );
        KI_FALLTHROUGH;
    case EDA_UNITS::UNSCALED: format = wxT( "%.0f" ); break;
    case EDA_UNITS::MM:       format = wxT( "%.3f" ); break;
    case EDA_UNITS::MILS:     format = wxT( "%.2f" ); break;
    case EDA_UNITS::INCH:     format = wxT( "%.5f" ); break;
    }

    text.Printf( format, EDA_UNIT_UTILS::UI::ToUserUnit( pcbIUScale, aUnits, aValue ) );

    if( aIncludeLabel )
        text += EDA_UNIT_UTILS::GetText( aUnits, EDA_DATA_TYPE::DISTANCE );

    return text;
}


void PCB_EDIT_FRAME::UpdateTrackWidthSelectBox( wxChoice* aTrackWidthSelectBox, bool aShowNetclass,
                                                bool aShowEdit )
{
    if( aTrackWidthSelectBox == nullptr )
        return;

    EDA_UNITS primaryUnit;
    EDA_UNITS secondaryUnit;

    GetUnitPair( primaryUnit, secondaryUnit );

    wxString msg;

    aTrackWidthSelectBox->Clear();

    if( aShowNetclass )
        aTrackWidthSelectBox->Append( _( "Track: use netclass width" ) );

    for( unsigned ii = 1; ii < GetDesignSettings().m_TrackWidthList.size(); ii++ )
    {
        int size = GetDesignSettings().m_TrackWidthList[ii];

        msg.Printf( _( "Track: %s (%s)" ), ComboBoxUnits( primaryUnit, size ),
                                           ComboBoxUnits( secondaryUnit, size ) );

        aTrackWidthSelectBox->Append( msg );
    }

    if( aShowEdit )
    {
        aTrackWidthSelectBox->Append( wxT( "---" ) );
        aTrackWidthSelectBox->Append( _( "Edit Pre-defined Sizes..." ) );
    }

    if( GetDesignSettings().GetTrackWidthIndex() >= (int) GetDesignSettings().m_TrackWidthList.size() )
        GetDesignSettings().SetTrackWidthIndex( 0 );

    // GetDesignSettings().GetTrackWidthIndex() can be < 0 if no board loaded
    // So in this case select the first select box item available (use netclass)
    aTrackWidthSelectBox->SetSelection( std::max( 0, GetDesignSettings().GetTrackWidthIndex() ) );
}


void PCB_EDIT_FRAME::UpdateViaSizeSelectBox( wxChoice* aViaSizeSelectBox, bool aShowNetclass,
                                             bool aShowEdit )
{
    if( aViaSizeSelectBox == nullptr )
        return;

    aViaSizeSelectBox->Clear();

    COMMON_TOOLS* cmnTool   = m_toolManager->GetTool<COMMON_TOOLS>();
    EDA_UNITS primaryUnit   = GetUserUnits();
    EDA_UNITS secondaryUnit = EDA_UNITS::MILS;

    if( EDA_UNIT_UTILS::IsImperialUnit( primaryUnit ) )
        secondaryUnit = cmnTool ? cmnTool->GetLastMetricUnits() : EDA_UNITS::MM;
    else
        secondaryUnit = cmnTool ? cmnTool->GetLastImperialUnits() : EDA_UNITS::MILS;

    if( aShowNetclass )
        aViaSizeSelectBox->Append( _( "Via: use netclass sizes" ) );

    for( unsigned ii = 1; ii < GetDesignSettings().m_ViasDimensionsList.size(); ii++ )
    {
        VIA_DIMENSION viaDimension = GetDesignSettings().m_ViasDimensionsList[ii];
        wxString      msg, priStr, secStr;

        double diam = viaDimension.m_Diameter;
        double hole = viaDimension.m_Drill;

        if( hole > 0 )
        {
            priStr = ComboBoxUnits( primaryUnit, diam, false ) + wxT( " / " )
                        + ComboBoxUnits( primaryUnit, hole, true );
            secStr = ComboBoxUnits( secondaryUnit, diam, false ) + wxT( " / " )
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

    if( aShowEdit )
    {
        aViaSizeSelectBox->Append( wxT( "---" ) );
        aViaSizeSelectBox->Append( _( "Edit Pre-defined Sizes..." ) );
    }

    if( GetDesignSettings().GetViaSizeIndex() >= (int) GetDesignSettings().m_ViasDimensionsList.size() )
        GetDesignSettings().SetViaSizeIndex( 0 );

    // GetDesignSettings().GetViaSizeIndex() can be < 0 if no board loaded
    // So in this case select the first select box item available (use netclass)
    aViaSizeSelectBox->SetSelection( std::max( 0, GetDesignSettings().GetViaSizeIndex() ) );
}


void PCB_EDIT_FRAME::ReCreateLayerBox( bool aForceResizeToolbar )
{
    if( m_SelLayerBox == nullptr || m_tbTopAux == nullptr )
        return;

    m_SelLayerBox->SetToolTip( _( "+/- to switch" ) );
    m_SelLayerBox->Resync();

    if( aForceResizeToolbar )
        UpdateToolbarControlSizes();
}


void PCB_EDIT_FRAME::ToggleLayersManager()
{
    PCBNEW_SETTINGS* settings      = GetPcbNewSettings();
    wxAuiPaneInfo&   layersManager = m_auimgr.GetPane( AppearancePanelName() );
    wxAuiPaneInfo&   selectionFilter = m_auimgr.GetPane( "SelectionFilter" );

    // show auxiliary Vertical layers and visibility manager toolbar
    m_show_layer_manager_tools = layersManager.IsShown();

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


void PCB_EDIT_FRAME::ToggleNetInspector()
{
    PCBNEW_SETTINGS* settings          = GetPcbNewSettings();
    wxAuiPaneInfo&   netInspectorPanel = m_auimgr.GetPane( NetInspectorPanelName() );

    m_show_net_inspector = netInspectorPanel.IsShown();

    m_show_net_inspector = !m_show_net_inspector;

    netInspectorPanel.Show( m_show_net_inspector );

    if( m_show_net_inspector )
    {
        SetAuiPaneSize( m_auimgr, netInspectorPanel, settings->m_AuiPanels.net_inspector_width, -1 );
        m_netInspectorPanel->OnShowPanel();
    }
    else
    {
        m_netInspectorPanel->SaveSettings();
        settings->m_AuiPanels.net_inspector_width = m_netInspectorPanel->GetSize().x;
        m_auimgr.Update();
    }
}


void PCB_EDIT_FRAME::ToggleSearch()
{
    PCBNEW_SETTINGS* settings = GetPcbNewSettings();

    // Ensure m_show_search is up to date (the pane can be closed outside the menu)
    m_show_search = m_auimgr.GetPane( SearchPaneName() ).IsShown();

    m_show_search = !m_show_search;

    wxAuiPaneInfo& searchPaneInfo = m_auimgr.GetPane( SearchPaneName() );
    searchPaneInfo.Show( m_show_search );

    if( m_show_search )
    {
        searchPaneInfo.Direction( settings->m_AuiPanels.search_panel_dock_direction );

        if( settings->m_AuiPanels.search_panel_dock_direction == wxAUI_DOCK_TOP
            || settings->m_AuiPanels.search_panel_dock_direction == wxAUI_DOCK_BOTTOM )
        {
            SetAuiPaneSize( m_auimgr, searchPaneInfo,
                            -1, settings->m_AuiPanels.search_panel_height );
        }
        else if( settings->m_AuiPanels.search_panel_dock_direction == wxAUI_DOCK_LEFT
                 || settings->m_AuiPanels.search_panel_dock_direction == wxAUI_DOCK_RIGHT )
        {
            SetAuiPaneSize( m_auimgr, searchPaneInfo,
                            settings->m_AuiPanels.search_panel_width, -1 );
        }
        m_searchPane->FocusSearch();
        m_searchPane->RefreshSearch();
    }
    else
    {
        settings->m_AuiPanels.search_panel_height = m_searchPane->GetSize().y;
        settings->m_AuiPanels.search_panel_width = m_searchPane->GetSize().x;
        settings->m_AuiPanels.search_panel_dock_direction = searchPaneInfo.dock_direction;
        m_auimgr.Update();
        GetCanvas()->SetFocus();
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
            // if GetTrackWidthIndex() < 0, display the "use netclass" option
            sel = std::max( 0, bds.GetTrackWidthIndex() );

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
            // if GetViaSizeIndex() < 0, display the "use netclass" option
            sel = std::max( 0, bds.GetViaSizeIndex() );

        if( m_SelViaSizeBox->GetSelection() != sel )
            m_SelViaSizeBox->SetSelection( sel );
    }
}


void PCB_EDIT_FRAME::ToggleLibraryTree()
{
    PCBNEW_SETTINGS* cfg = GetPcbNewSettings();

    wxCHECK( cfg, /* void */ );

    wxAuiPaneInfo& db_library_pane = m_auimgr.GetPane( DesignBlocksPaneName() );

    db_library_pane.Show( !db_library_pane.IsShown() );

    if( db_library_pane.IsShown() )
    {
        if( db_library_pane.IsFloating() )
        {
            db_library_pane.FloatingSize( cfg->m_AuiPanels.design_blocks_panel_float_width,
                                          cfg->m_AuiPanels.design_blocks_panel_float_height );
            m_auimgr.Update();
        }
        else if( cfg->m_AuiPanels.design_blocks_panel_docked_width > 0 )
        {
            // SetAuiPaneSize also updates m_auimgr
            SetAuiPaneSize( m_auimgr, db_library_pane, cfg->m_AuiPanels.design_blocks_panel_docked_width, -1 );
        }
    }
    else
    {
        if( db_library_pane.IsFloating() )
        {
            cfg->m_AuiPanels.design_blocks_panel_float_width = db_library_pane.floating_size.x;
            cfg->m_AuiPanels.design_blocks_panel_float_height = db_library_pane.floating_size.y;
        }
        else
        {
            cfg->m_AuiPanels.design_blocks_panel_docked_width = m_designBlocksPane->GetSize().x;
        }

        m_auimgr.Update();
    }
}
