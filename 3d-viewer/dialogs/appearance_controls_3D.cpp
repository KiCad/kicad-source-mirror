/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/appearance_controls_3D.h>

#include <bitmaps.h>
#include <confirm.h>
#include <pgm_base.h>
#include <dpi_scaling_common.h>
#include <eda_list_dialog.h>
#include <pcb_display_options.h>
#include <eda_3d_viewer_frame.h>
#include <pcbnew_settings.h>
#include <project.h>
#include <board.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/eda_3d_actions.h>
#include <widgets/bitmap_toggle.h>
#include <widgets/color_swatch.h>
#include <widgets/grid_bitmap_toggle.h>
#include <dialogs/eda_view_switcher.h>
#include <wx/bmpbuttn.h>
#include <wx/sizer.h>
#include <wx/textdlg.h>
#include <wx/checkbox.h>

#include <../3d_rendering/opengl/render_3d_opengl.h>
#include <algorithm>

/// Render Row abbreviation to reduce source width.
#define RR  APPEARANCE_CONTROLS_3D::APPEARANCE_SETTING_3D

/// Template for object appearance settings
const APPEARANCE_CONTROLS_3D::APPEARANCE_SETTING_3D APPEARANCE_CONTROLS_3D::s_layerSettings[] = {

    //        text                           id                        tooltip
    RR( _HKI( "Board Body" ),    LAYER_3D_BOARD,             _HKI( "Show board body" ) ),
    RR( _HKI( "Plated Barrels" ),LAYER_3D_PLATED_BARRELS,    _HKI( "Show barrels of plated through-holes and vias" ) ),
    RR(  wxS( "F.Cu" ),          LAYER_3D_COPPER_TOP,        _HKI( "Show front copper / surface finish color" ) ),
    RR(  wxS( "B.Cu" ),          LAYER_3D_COPPER_BOTTOM,     _HKI( "Show back copper / surface finish color" ) ),
    RR( _HKI( "Adhesive" ),      LAYER_3D_ADHESIVE,          _HKI( "Show adhesive" ) ),
    RR( _HKI( "Solder Paste" ),  LAYER_3D_SOLDERPASTE,       _HKI( "Show solder paste" ) ),
    RR(  wxS( "F.Silkscreen" ),  LAYER_3D_SILKSCREEN_TOP,    _HKI( "Show front silkscreen" ) ),
    RR(  wxS( "B.Silkscreen" ),  LAYER_3D_SILKSCREEN_BOTTOM, _HKI( "Show back silkscreen" ) ),
    RR(  wxS( "F.Mask" ),        LAYER_3D_SOLDERMASK_TOP,    _HKI( "Show front solder mask" ) ),
    RR(  wxS( "B.Mask" ),        LAYER_3D_SOLDERMASK_BOTTOM, _HKI( "Show back solder mask" ) ),
    RR(  wxS( "User.Drawings" ), LAYER_3D_USER_DRAWINGS,     _HKI( "Show user drawings layer" ) ),
    RR(  wxS( "User.Comments" ), LAYER_3D_USER_COMMENTS,     _HKI( "Show user comments layer" ) ),
    RR(  wxS( "User.Eco1" ),     LAYER_3D_USER_ECO1,         _HKI( "Show user ECO1 layer" ) ),
    RR(  wxS( "User.Eco2" ),     LAYER_3D_USER_ECO2,         _HKI( "Show user ECO2 layer" ) ),
    RR(  wxS( "User.1" ),        LAYER_3D_USER_1,            _HKI( "Show user defined layer 1" ) ),
    RR(  wxS( "User.2" ),        LAYER_3D_USER_2,            _HKI( "Show user defined layer 2" ) ),
    RR(  wxS( "User.3" ),        LAYER_3D_USER_3,            _HKI( "Show user defined layer 3" ) ),
    RR(  wxS( "User.4" ),        LAYER_3D_USER_4,            _HKI( "Show user defined layer 4" ) ),
    RR(  wxS( "User.5" ),        LAYER_3D_USER_5,            _HKI( "Show user defined layer 5" ) ),
    RR(  wxS( "User.6" ),        LAYER_3D_USER_6,            _HKI( "Show user defined layer 6" ) ),
    RR(  wxS( "User.7" ),        LAYER_3D_USER_7,            _HKI( "Show user defined layer 7" ) ),
    RR(  wxS( "User.8" ),        LAYER_3D_USER_8,            _HKI( "Show user defined layer 8" ) ),
    RR(  wxS( "User.9" ),        LAYER_3D_USER_9,            _HKI( "Show user defined layer 9" ) ),
    RR(  wxS( "User.10" ),       LAYER_3D_USER_10,           _HKI( "Show user defined layer 10" ) ),
    RR(  wxS( "User.11" ),       LAYER_3D_USER_11,           _HKI( "Show user defined layer 11" ) ),
    RR(  wxS( "User.12" ),       LAYER_3D_USER_12,           _HKI( "Show user defined layer 12" ) ),
    RR(  wxS( "User.13" ),       LAYER_3D_USER_13,           _HKI( "Show user defined layer 13" ) ),
    RR(  wxS( "User.14" ),       LAYER_3D_USER_14,           _HKI( "Show user defined layer 14" ) ),
    RR(  wxS( "User.15" ),       LAYER_3D_USER_15,           _HKI( "Show user defined layer 15" ) ),
    RR(  wxS( "User.16" ),       LAYER_3D_USER_16,           _HKI( "Show user defined layer 16" ) ),
    RR(  wxS( "User.17" ),       LAYER_3D_USER_17,           _HKI( "Show user defined layer 17" ) ),
    RR(  wxS( "User.18" ),       LAYER_3D_USER_18,           _HKI( "Show user defined layer 18" ) ),
    RR(  wxS( "User.19" ),       LAYER_3D_USER_19,           _HKI( "Show user defined layer 19" ) ),
    RR(  wxS( "User.20" ),       LAYER_3D_USER_20,           _HKI( "Show user defined layer 20" ) ),
    RR(  wxS( "User.21" ),       LAYER_3D_USER_21,           _HKI( "Show user defined layer 21" ) ),
    RR(  wxS( "User.22" ),       LAYER_3D_USER_22,           _HKI( "Show user defined layer 22" ) ),
    RR(  wxS( "User.23" ),       LAYER_3D_USER_23,           _HKI( "Show user defined layer 23" ) ),
    RR(  wxS( "User.24" ),       LAYER_3D_USER_24,           _HKI( "Show user defined layer 24" ) ),
    RR(  wxS( "User.25" ),       LAYER_3D_USER_25,           _HKI( "Show user defined layer 25" ) ),
    RR(  wxS( "User.26" ),       LAYER_3D_USER_26,           _HKI( "Show user defined layer 26" ) ),
    RR(  wxS( "User.27" ),       LAYER_3D_USER_27,           _HKI( "Show user defined layer 27" ) ),
    RR(  wxS( "User.28" ),       LAYER_3D_USER_28,           _HKI( "Show user defined layer 28" ) ),
    RR(  wxS( "User.29" ),       LAYER_3D_USER_29,           _HKI( "Show user defined layer 29" ) ),
    RR(  wxS( "User.30" ),       LAYER_3D_USER_30,           _HKI( "Show user defined layer 30" ) ),
    RR(  wxS( "User.31" ),       LAYER_3D_USER_31,           _HKI( "Show user defined layer 31" ) ),
    RR(  wxS( "User.32" ),       LAYER_3D_USER_32,           _HKI( "Show user defined layer 32" ) ),
    RR(  wxS( "User.33" ),       LAYER_3D_USER_33,           _HKI( "Show user defined layer 33" ) ),
    RR(  wxS( "User.34" ),       LAYER_3D_USER_34,           _HKI( "Show user defined layer 34" ) ),
    RR(  wxS( "User.35" ),       LAYER_3D_USER_35,           _HKI( "Show user defined layer 35" ) ),
    RR(  wxS( "User.36" ),       LAYER_3D_USER_36,           _HKI( "Show user defined layer 36" ) ),
    RR(  wxS( "User.37" ),       LAYER_3D_USER_37,           _HKI( "Show user defined layer 37" ) ),
    RR(  wxS( "User.38" ),       LAYER_3D_USER_38,           _HKI( "Show user defined layer 38" ) ),
    RR(  wxS( "User.39" ),       LAYER_3D_USER_39,           _HKI( "Show user defined layer 39" ) ),
    RR(  wxS( "User.40" ),       LAYER_3D_USER_40,           _HKI( "Show user defined layer 40" ) ),
    RR(  wxS( "User.41" ),       LAYER_3D_USER_41,           _HKI( "Show user defined layer 41" ) ),
    RR(  wxS( "User.42" ),       LAYER_3D_USER_42,           _HKI( "Show user defined layer 42" ) ),
    RR(  wxS( "User.43" ),       LAYER_3D_USER_43,           _HKI( "Show user defined layer 43" ) ),
    RR(  wxS( "User.44" ),       LAYER_3D_USER_44,           _HKI( "Show user defined layer 44" ) ),
    RR(  wxS( "User.45" ),       LAYER_3D_USER_45,           _HKI( "Show user defined layer 45" ) ),
    RR(),
    RR( _HKI( "Through-hole Models" ),    LAYER_3D_TH_MODELS,         EDA_3D_ACTIONS::showTHT ),
    RR( _HKI( "SMD Models" ),             LAYER_3D_SMD_MODELS,        EDA_3D_ACTIONS::showSMD ),
    RR( _HKI( "Virtual Models" ),         LAYER_3D_VIRTUAL_MODELS,    EDA_3D_ACTIONS::showVirtual ),
    RR( _HKI( "Models not in POS File" ), LAYER_3D_MODELS_NOT_IN_POS, EDA_3D_ACTIONS::showNotInPosFile ),
    RR( _HKI( "Models marked DNP" ),      LAYER_3D_MODELS_MARKED_DNP, EDA_3D_ACTIONS::showDNP ),
    RR( _HKI( "Model Bounding Boxes" ),   LAYER_3D_BOUNDING_BOXES,    EDA_3D_ACTIONS::showBBoxes ),
    RR(),
    RR( _HKI( "Values" ),               LAYER_FP_VALUES,           _HKI( "Show footprint values" ) ),
    RR( _HKI( "References" ),           LAYER_FP_REFERENCES,       _HKI( "Show footprint references" ) ),
    RR( _HKI( "Footprint Text" ),       LAYER_FP_TEXT,             _HKI( "Show all footprint text" ) ),
    RR( _HKI( "Off-board Silkscreen" ), LAYER_3D_OFF_BOARD_SILK,   _HKI( "Do not clip silk layers to board outline" ) ),
    RR(),
    RR( _HKI( "3D Navigator" ),         LAYER_3D_NAVIGATOR,         EDA_3D_ACTIONS::showNavigator ),
    RR( _HKI( "Background Start" ),     LAYER_3D_BACKGROUND_TOP,    _HKI( "Background gradient start color" ) ),
    RR( _HKI( "Background End" ),       LAYER_3D_BACKGROUND_BOTTOM, _HKI( "Background gradient end color" ) ),
};

// The list of IDs that can have colors coming from the board stackup, and cannot be
// modified if use colors from stackup is activated
static std::vector<int> inStackupColors{ LAYER_3D_BOARD,
                                         LAYER_3D_COPPER_TOP, LAYER_3D_COPPER_BOTTOM,
                                         LAYER_3D_SOLDERPASTE,
                                         LAYER_3D_SILKSCREEN_TOP, LAYER_3D_SILKSCREEN_BOTTOM,
                                         LAYER_3D_SOLDERMASK_TOP, LAYER_3D_SOLDERMASK_BOTTOM
                                        };

APPEARANCE_CONTROLS_3D::APPEARANCE_CONTROLS_3D( EDA_3D_VIEWER_FRAME* aParent, wxWindow* aFocusOwner ) :
        APPEARANCE_CONTROLS_3D_BASE( aParent ),
        m_frame( aParent ),
        m_focusOwner( aFocusOwner ),
        m_lastSelectedViewport( nullptr )
{
    DPI_SCALING_COMMON dpi( nullptr, m_frame );

    int screenHeight  = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );
    m_pointSize       = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ).GetPointSize();

    m_layerPanelColour = m_panelLayers->GetBackgroundColour().ChangeLightness( 110 );
    SetBorders( true, false, false, false );

    m_layersOuterSizer = new wxBoxSizer( wxVERTICAL );
    m_windowLayers->SetSizer( m_layersOuterSizer );
    m_windowLayers->SetScrollRate( 0, 5 );
    m_windowLayers->Bind( wxEVT_SET_FOCUS, &APPEARANCE_CONTROLS_3D::OnSetFocus, this );

    m_envOuterSizer = new wxBoxSizer( wxVERTICAL );

    wxFont infoFont = KIUI::GetInfoFont( this );
    m_panelLayers->SetFont( infoFont );
    m_windowLayers->SetFont( infoFont );
    m_presetsLabel->SetFont( infoFont );
    m_viewportsLabel->SetFont( infoFont );

    // Create display options
    m_cbUseBoardStackupColors = new wxCheckBox( m_panelLayers, wxID_ANY,
                                                _( "Use board stackup colors" ) );
    m_cbUseBoardStackupColors->SetFont( infoFont );

    m_cbUseBoardStackupColors->Bind( wxEVT_CHECKBOX,
            [this]( wxCommandEvent& aEvent )
            {
                EDA_3D_VIEWER_SETTINGS* cfg = m_frame->GetAdapter().m_Cfg;
                cfg->m_UseStackupColors = aEvent.IsChecked();

                UpdateLayerCtls();
                m_frame->NewDisplay( true );
            } );

    m_cbUseBoardEditorCopperColors = new wxCheckBox( m_panelLayers, wxID_ANY,
                                                     _( "Use PCB editor copper colors" ) );
    m_cbUseBoardEditorCopperColors->SetFont( infoFont );
    m_cbUseBoardEditorCopperColors->SetToolTip( _( "Use the board editor layer colors for copper "
                                                   "layers (realtime renderer only)" ) );

    m_cbUseBoardEditorCopperColors->Bind( wxEVT_CHECKBOX,
            [this]( wxCommandEvent& aEvent )
            {
                EDA_3D_VIEWER_SETTINGS* cfg = m_frame->GetAdapter().m_Cfg;
                cfg->m_Render.use_board_editor_copper_colors = aEvent.IsChecked();

                UpdateLayerCtls();
                m_frame->NewDisplay( true );
            } );

    m_panelLayersSizer->Add( m_cbUseBoardStackupColors, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 5 );
    m_panelLayersSizer->Add( m_cbUseBoardEditorCopperColors, 0,
                             wxEXPAND | wxALL, 5 );

    m_cbLayerPresets->SetToolTip( wxString::Format( _( "Save and restore color and visibility combinations.\n"
                                                       "Use %s+Tab to activate selector.\n"
                                                       "Successive Tabs while holding %s down will cycle through "
                                                       "presets in the popup." ),
                                                    KeyNameFromKeyCode( PRESET_SWITCH_KEY ),
                                                    KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) );

    m_cbViewports->SetToolTip( wxString::Format( _( "Save and restore camera position and zoom.\n"
                                                    "Use %s+Tab to activate selector.\n"
                                                    "Successive Tabs while holding %s down will cycle through "
                                                    "viewports in the popup." ),
                                                 KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ),
                                                 KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ) ) );

    if( screenHeight <= 900 && m_pointSize >= FromDIP( KIUI::c_IndicatorSizeDIP ) )
        m_pointSize = m_pointSize * 8 / 10;

    m_cbLayerPresets->Bind( wxEVT_CHOICE, &APPEARANCE_CONTROLS_3D::onLayerPresetChanged, this );

    m_toggleGridRenderer = new GRID_BITMAP_TOGGLE_RENDERER(
            KiBitmapBundle( BITMAPS::visibility ), KiBitmapBundle( BITMAPS::visibility_off ) );

    m_frame->Bind( EDA_LANG_CHANGED, &APPEARANCE_CONTROLS_3D::OnLanguageChanged, this );
}


APPEARANCE_CONTROLS_3D::~APPEARANCE_CONTROLS_3D()
{
    m_frame->Unbind( EDA_LANG_CHANGED, &APPEARANCE_CONTROLS_3D::OnLanguageChanged, this );
}


wxSize APPEARANCE_CONTROLS_3D::GetBestSize() const
{
    DPI_SCALING_COMMON dpi( nullptr, m_frame );
    wxSize      size( 220 * dpi.GetScaleFactor(), 480 * dpi.GetScaleFactor() );
    return size;
}


void APPEARANCE_CONTROLS_3D::OnSetFocus( wxFocusEvent& aEvent )
{
#ifdef __WXMSW__
    // In wxMSW, buttons won't process events unless they have focus, so we'll let it take the
    // focus and give it back to the parent in the button event handler.
    if( wxBitmapButton* btn = dynamic_cast<wxBitmapButton*>( aEvent.GetEventObject() ) )
    {
        wxCommandEvent evt( wxEVT_BUTTON );
        wxPostEvent( btn, evt );
    }
#endif

    passOnFocus();
    aEvent.Skip();
}


void APPEARANCE_CONTROLS_3D::OnSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();
}


void APPEARANCE_CONTROLS_3D::rebuildControls()
{
    Freeze();

    rebuildLayers();
    m_cbUseBoardStackupColors->SetLabel( _( "Use board stackup colors" ) );
    rebuildLayerPresetsWidget( true );
    rebuildViewportsWidget();

    Thaw();
    Refresh();
}


void APPEARANCE_CONTROLS_3D::OnLanguageChanged( wxCommandEvent& aEvent )
{
    rebuildControls();

    aEvent.Skip();
}


void APPEARANCE_CONTROLS_3D::OnDarkModeToggle()
{
    // This is essentially a list of hacks because DarkMode isn't yet implemented inside
    // wxWidgets.
    //
    // The individual wxPanels, COLOR_SWATCHes and GRID_CELL_COLOR_RENDERERs should really be
    // overriding some virtual method or responding to some wxWidgets event so that the parent
    // doesn't have to know what it contains.  But, that's not where we are, so... :shrug:

    m_layerPanelColour = m_panelLayers->GetBackgroundColour().ChangeLightness( 110 );

    m_windowLayers->SetBackgroundColour( m_layerPanelColour );

    for( wxSizerItem* child : m_layersOuterSizer->GetChildren() )
    {
        if( child && child->GetWindow() )
            child->GetWindow()->SetBackgroundColour( m_layerPanelColour );
    }
}


void APPEARANCE_CONTROLS_3D::CommonSettingsChanged()
{
    rebuildControls();

    UpdateLayerCtls();

    const wxString& currentPreset = m_frame->GetAdapter().m_Cfg->m_CurrentPreset;

    if( currentPreset == FOLLOW_PCB || currentPreset == FOLLOW_PLOT_SETTINGS )
        updateLayerPresetWidget( currentPreset );
    else
        syncLayerPresetSelection();
}


void APPEARANCE_CONTROLS_3D::ApplyLayerPreset( const wxString& aPresetName )
{
    doApplyLayerPreset( aPresetName );

    // Move to front of MRU list
    if( m_presetMRU.Index( aPresetName ) != wxNOT_FOUND )
        m_presetMRU.Remove( aPresetName );

    m_presetMRU.Insert( aPresetName, 0 );

    updateLayerPresetWidget( aPresetName );
}


std::vector<VIEWPORT3D> APPEARANCE_CONTROLS_3D::GetUserViewports() const
{
    std::vector<VIEWPORT3D> ret;

    for( const auto& [name, viewport] : m_viewports )
        ret.emplace_back( viewport );

    return ret;
}


void APPEARANCE_CONTROLS_3D::SetUserViewports( std::vector<VIEWPORT3D>& aViewportList )
{
    m_viewports.clear();

    for( const VIEWPORT3D& viewport : aViewportList )
    {
        if( m_viewports.count( viewport.name ) )
            continue;

        m_viewports[viewport.name] = viewport;

        m_viewportMRU.Add( viewport.name );
    }

    rebuildViewportsWidget();

    // Now is as good a time as any to initialize the layer presets as well.
    rebuildLayerPresetsWidget( true );

    m_presetMRU.Add( FOLLOW_PCB );
    m_presetMRU.Add( FOLLOW_PLOT_SETTINGS );

    for( const LAYER_PRESET_3D& preset : m_frame->GetAdapter().m_Cfg->m_LayerPresets )
        m_presetMRU.Add( preset.name );
}


void APPEARANCE_CONTROLS_3D::ApplyViewport( const wxString& aViewportName )
{
    int idx = m_cbViewports->FindString( aViewportName );

    if( idx >= 0 && idx < (int)m_cbViewports->GetCount() - 3 /* separator */ )
    {
        m_cbViewports->SetSelection( idx );
        m_lastSelectedViewport = static_cast<VIEWPORT3D*>( m_cbViewports->GetClientData( idx ) );
    }
    else
    {
        m_cbViewports->SetSelection( m_cbViewports->GetCount() - 3 ); // separator
        m_lastSelectedViewport = nullptr;
    }

    if( m_lastSelectedViewport )
        doApplyViewport( *m_lastSelectedViewport );
}


void APPEARANCE_CONTROLS_3D::OnLayerVisibilityChanged( int aLayer, bool isVisible )
{
    std::bitset<LAYER_3D_END>     visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
    const std::map<int, COLOR4D>& colors = m_frame->GetAdapter().GetLayerColors();
    bool                          killFollow = false;
    bool                          doFastRefresh = false;    // true to just refresh the display

    // Special-case controls
    switch( aLayer )
    {
    case LAYER_FP_TEXT:
        // Because Footprint Text is a meta-control that also can disable values/references,
        // drag them along here so that the user is less likely to be confused.
        if( !isVisible )
        {
            visibleLayers.set( LAYER_FP_REFERENCES, false );
            visibleLayers.set( LAYER_FP_VALUES, false );
        }

        visibleLayers.set( LAYER_FP_TEXT, isVisible );
        killFollow = true;
        break;

    case LAYER_FP_REFERENCES:
    case LAYER_FP_VALUES:
        // In case that user changes Footprint Value/References when the Footprint Text
        // meta-control is disabled, we should put it back on.
        if( isVisible )
            visibleLayers.set( LAYER_FP_TEXT, true );

        visibleLayers.set( aLayer, isVisible );
        killFollow = true;
        break;

    case LAYER_3D_BOARD:
    case LAYER_3D_COPPER_TOP:
    case LAYER_3D_COPPER_BOTTOM:
    case LAYER_3D_PLATED_BARRELS:
    case LAYER_3D_SILKSCREEN_BOTTOM:
    case LAYER_3D_SILKSCREEN_TOP:
    case LAYER_3D_SOLDERMASK_BOTTOM:
    case LAYER_3D_SOLDERMASK_TOP:
    case LAYER_3D_SOLDERPASTE:
    case LAYER_3D_ADHESIVE:
    case LAYER_3D_USER_COMMENTS:
    case LAYER_3D_USER_DRAWINGS:
    case LAYER_3D_USER_ECO1:
    case LAYER_3D_USER_ECO2:
        visibleLayers.set( aLayer, isVisible );
        killFollow = true;
        break;

    case LAYER_3D_TH_MODELS:
    case LAYER_3D_SMD_MODELS:
    case LAYER_3D_VIRTUAL_MODELS:
    case LAYER_3D_MODELS_NOT_IN_POS:
    case LAYER_3D_MODELS_MARKED_DNP:
        doFastRefresh = true;
        visibleLayers.set( aLayer, isVisible );
        break;

    default:
        visibleLayers.set( aLayer, isVisible );

        if( aLayer >= LAYER_3D_USER_1 && aLayer <= LAYER_3D_USER_45 )
            killFollow = true;

        break;
    }

    m_frame->GetAdapter().SetVisibleLayers( visibleLayers );
    m_frame->GetAdapter().SetLayerColors( colors );

    const wxString& currentPreset = m_frame->GetAdapter().m_Cfg->m_CurrentPreset;

    if( ( currentPreset != FOLLOW_PCB && currentPreset != FOLLOW_PLOT_SETTINGS ) || killFollow )
        syncLayerPresetSelection();

    UpdateLayerCtls();

    if( doFastRefresh && m_frame->GetAdapter().m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
    {
        RENDER_3D_OPENGL* renderer = static_cast<RENDER_3D_OPENGL*>( m_frame->GetCanvas()->GetCurrentRender() );
        renderer->Load3dModelsIfNeeded();
        m_frame->GetCanvas()->Request_refresh();
    }
    else
    {
        m_frame->NewDisplay( true );
    }
}


void APPEARANCE_CONTROLS_3D::onColorSwatchChanged( COLOR_SWATCH* aSwatch )
{
    std::bitset<LAYER_3D_END> visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
    std::map<int, COLOR4D>    colors = m_frame->GetAdapter().GetLayerColors();

    m_frame->GetAdapter().SetVisibleLayers( visibleLayers );

    int     layer = aSwatch->GetId();
    COLOR4D newColor = aSwatch->GetSwatchColor();

    colors[ layer ] = newColor;

    // The internals of the 3D viewer only supports a single color for copper, which must
    // be applied to all copper layers.
    COLOR_SWATCH* otherSwatch = nullptr;

    const std::vector<int> copperIndices{
        LAYER_3D_COPPER_TOP,
        LAYER_3D_COPPER_BOTTOM,
        LAYER_3D_PLATED_BARRELS,
    };

    // If the changed swatch is one of the copper layers, we need to update the other copper layers
    if( std::find( copperIndices.begin(), copperIndices.end(), layer ) != copperIndices.end() )
    {
        for( int index : copperIndices )
        {
            if( layer != index && colors.count( index ) )
            {
                colors[index] = newColor;
                otherSwatch = m_layerSettingsMap[index]->m_Ctl_color;
            }
        }
    }

    if( otherSwatch )
    {
        // Don't send an event, because that will cause an event loop
        otherSwatch->SetSwatchColor( newColor, false );
    }


    m_frame->GetAdapter().SetLayerColors( colors );

    const wxString& currentPreset = m_frame->GetAdapter().m_Cfg->m_CurrentPreset;

    if( currentPreset == FOLLOW_PCB || currentPreset == FOLLOW_PLOT_SETTINGS )
        updateLayerPresetWidget( currentPreset );
    else
        syncLayerPresetSelection();

    m_frame->NewDisplay( true );
}


void APPEARANCE_CONTROLS_3D::rebuildLayers()
{
    int swatchWidth = m_windowLayers->ConvertDialogToPixels( wxSize( 8, 0 ) ).x;

    std::bitset<LAYER_3D_END> visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
    std::map<int, COLOR4D>    colors = m_frame->GetAdapter().GetLayerColors();
    std::map<int, COLOR4D>    defaultColors = m_frame->GetAdapter().GetDefaultColors();
    LSET                      enabled = m_frame->GetBoard()->GetEnabledLayers();

    m_layerSettings.clear();
    m_layersOuterSizer->Clear( true );
    m_layersOuterSizer->AddSpacer( 5 );

    m_envOuterSizer->Clear( true );

    auto appendLayer =
            [&]( const std::unique_ptr<APPEARANCE_SETTING_3D>& aSetting )
            {
                wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
                int         layer = aSetting->m_Id;

                aSetting->m_Visible = visibleLayers.test( layer );

                if( colors.count( layer ) )
                {
                    COLOR_SWATCH* swatch = new COLOR_SWATCH( m_windowLayers, colors[ layer ], layer,
                                                             COLOR4D::WHITE, defaultColors[ layer ],
                                                             SWATCH_SMALL );
                    swatch->SetToolTip( _( "Left double click or middle click to change color" ) );

                    swatch->SetReadOnlyCallback(
                            [this]()
                            {
                                WX_INFOBAR* infobar = m_frame->GetInfoBar();

                                infobar->RemoveAllButtons();
                                infobar->AddCloseButton();

                                infobar->ShowMessageFor( _( "Uncheck 'Use board stackup colors' to "
                                                            "allow color editing." ),
                                                         10000, wxICON_INFORMATION );
                            } );

                    sizer->Add( swatch, 0,  wxALIGN_CENTER_VERTICAL, 0 );
                    aSetting->m_Ctl_color = swatch;

                    swatch->Bind( COLOR_SWATCH_CHANGED,
                            [this]( wxCommandEvent& event )
                            {
                                auto swatch = static_cast<COLOR_SWATCH*>( event.GetEventObject() );
                                onColorSwatchChanged( swatch );

                                passOnFocus();
                            } );
                }
                else
                {
                    sizer->AddSpacer( swatchWidth  );
                }

                sizer->AddSpacer( 5 );

                wxString     layerName = aSetting->GetLabel();
                PCB_LAYER_ID boardLayer = Map3DLayerToPCBLayer( layer );

                if( boardLayer != UNDEFINED_LAYER )
                    layerName = m_frame->GetBoard()->GetLayerName( boardLayer );

                wxStaticText* label = new wxStaticText( m_windowLayers, layer, layerName );
                label->Wrap( -1 );
                label->SetToolTip( aSetting->GetTooltip() );

                if( layer == LAYER_3D_BACKGROUND_TOP || layer == LAYER_3D_BACKGROUND_BOTTOM )
                {
                    sizer->AddSpacer( swatchWidth );
                }
                else
                {
                    BITMAP_TOGGLE* btn_visible = new BITMAP_TOGGLE( m_windowLayers, layer,
                                                                    KiBitmapBundle( BITMAPS::visibility ),
                                                                    KiBitmapBundle( BITMAPS::visibility_off ),
                                                                    aSetting->m_Visible );

                    btn_visible->Bind( TOGGLE_CHANGED,
                            [this]( wxCommandEvent& aEvent )
                            {
                                int id = static_cast<wxWindow*>( aEvent.GetEventObject() )->GetId();
                                bool isVisible = aEvent.GetInt();
                                OnLayerVisibilityChanged( id, isVisible );

                                passOnFocus();
                            } );

                    wxString tip;
                    tip.Printf( _( "Show or hide %s" ), aSetting->GetLabel().Lower() );
                    btn_visible->SetToolTip( tip );

                    aSetting->m_Ctl_visibility = btn_visible;
                    sizer->Add( btn_visible, 0, wxALIGN_CENTER_VERTICAL, 0 );
                }

                sizer->AddSpacer( 5 );
                sizer->Add( label, 0, wxALIGN_CENTER_VERTICAL, 0 );

                m_layersOuterSizer->Add( sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );
                m_layersOuterSizer->AddSpacer( 2 );
            };

    for( const APPEARANCE_SETTING_3D& s_setting : s_layerSettings )
    {
        m_layerSettings.emplace_back( std::make_unique<APPEARANCE_SETTING_3D>( s_setting ) );
        std::unique_ptr<APPEARANCE_SETTING_3D>& setting = m_layerSettings.back();

        if( setting->m_Spacer )
        {
            m_layersOuterSizer->AddSpacer( m_pointSize );
        }
        else if( setting->m_Id >= LAYER_3D_USER_1 && setting->m_Id <= LAYER_3D_USER_45 )
        {
            if( enabled.test( Map3DLayerToPCBLayer( setting->m_Id ) ) )
                appendLayer( setting );
        }
        else
        {
            appendLayer( setting );
        }

        m_layerSettingsMap[setting->m_Id] = setting.get();
    }

    m_sizerOuter->Layout();
}


void APPEARANCE_CONTROLS_3D::UpdateLayerCtls()
{
    EDA_3D_VIEWER_SETTINGS*   cfg = m_frame->GetAdapter().m_Cfg;
    std::bitset<LAYER_3D_END> visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
    std::map<int, COLOR4D>    colors = m_frame->GetAdapter().GetLayerColors();

    for( std::unique_ptr<APPEARANCE_SETTING_3D>& setting : m_layerSettings )
    {
        if( setting->m_Spacer )
            continue;

        if( setting->m_Ctl_visibility )
            setting->m_Ctl_visibility->SetValue( visibleLayers.test( setting->m_Id ) );

        if( setting->m_Ctl_color )
        {
            setting->m_Ctl_color->SetSwatchColor( colors[ setting->m_Id ], false );

            // if cfg->m_UseStackupColors is set, board colors cannot be modified locally, but
            // other colors can be
            if( alg::contains( inStackupColors, setting->m_Id ) && cfg )
                setting->m_Ctl_color->SetReadOnly( cfg->m_UseStackupColors );
        }
    }

    if( cfg )
    {
        m_cbUseBoardStackupColors->SetValue( cfg->m_UseStackupColors );
        m_cbUseBoardEditorCopperColors->SetValue( cfg->m_Render.use_board_editor_copper_colors );
    }
}


void APPEARANCE_CONTROLS_3D::rebuildLayerPresetsWidget( bool aReset )
{
    m_presetsLabel->SetLabel( wxString::Format( _( "Presets (%s+Tab):" ),
                                                KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) );

    m_cbLayerPresets->Clear();

    // Build the layers preset list.

    m_cbLayerPresets->Append( _( "Follow PCB Editor" ) );
    m_cbLayerPresets->Append( _( "Follow PCB Plot Settings" ) );

    if( !m_frame->GetAdapter().m_Cfg->m_LayerPresets.empty() )
        m_cbLayerPresets->Append( wxT( "---" ) );

    for( const LAYER_PRESET_3D& preset : m_frame->GetAdapter().m_Cfg->m_LayerPresets )
        m_cbLayerPresets->Append( preset.name );

    m_cbLayerPresets->Append( wxT( "---" ) );
    m_cbLayerPresets->Append( _( "Save preset..." ) );
    m_cbLayerPresets->Append( _( "Delete preset..." ) );

    if( aReset )
        updateLayerPresetWidget( m_frame->GetAdapter().m_Cfg->m_CurrentPreset );
}


void APPEARANCE_CONTROLS_3D::syncLayerPresetSelection()
{
    m_frame->GetAdapter().m_Cfg->m_CurrentPreset = wxEmptyString;

    std::vector<LAYER_PRESET_3D>& presets = m_frame->GetAdapter().m_Cfg->m_LayerPresets;
    std::bitset<LAYER_3D_END>     visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
    std::map<int, COLOR4D>        colors = m_frame->GetAdapter().GetLayerColors();

    auto it = std::find_if(
            presets.begin(), presets.end(),
            [&]( const LAYER_PRESET_3D& aPreset )
            {
                if( aPreset.name.Lower() == _( "legacy colors" ) && m_cbUseBoardStackupColors->GetValue() )
                    return false;

                for( int layer = LAYER_3D_BOARD; layer < LAYER_3D_END; ++layer )
                {
                    if( aPreset.layers.test( layer ) != visibleLayers.test( layer ) )
                        return false;
                }

                for( int layer : { LAYER_FP_REFERENCES, LAYER_FP_VALUES, LAYER_FP_TEXT } )
                {
                    if( aPreset.layers.test( layer ) != visibleLayers.test( layer ) )
                        return false;
                }

                for( int layer = LAYER_3D_START + 1; layer < LAYER_3D_END; ++layer )
                {
                    auto it1 = aPreset.colors.find( layer );
                    auto it2 = colors.find( layer );

                    if( it1 != aPreset.colors.end() && it2 != colors.end() && *it1 != *it2 )
                        return false;
                }

                return true;
            } );

    if( it != presets.end() )
    {
        m_frame->GetAdapter().m_Cfg->m_CurrentPreset = it->name;
        m_cbLayerPresets->SetStringSelection( it->name );
    }
    else
    {
        m_cbLayerPresets->SetSelection( m_cbLayerPresets->GetCount() - 3 ); // separator
    }
}


void APPEARANCE_CONTROLS_3D::updateLayerPresetWidget( const wxString& aName )
{
    if( aName == FOLLOW_PCB )
        m_cbLayerPresets->SetSelection( 0 );
    else if( aName == FOLLOW_PLOT_SETTINGS )
        m_cbLayerPresets->SetSelection( 1 );
    else if( !m_cbLayerPresets->SetStringSelection( aName ) )
        m_cbLayerPresets->SetSelection( m_cbLayerPresets->GetCount() - 3 ); // separator
}


void APPEARANCE_CONTROLS_3D::onLayerPresetChanged( wxCommandEvent& aEvent )
{
    EDA_3D_VIEWER_SETTINGS*   cfg = m_frame->GetAdapter().m_Cfg;
    int                       count = m_cbLayerPresets->GetCount();
    int                       index = m_cbLayerPresets->GetSelection();
    wxString                  name;

    auto resetSelection =
            [&]()
            {
                updateLayerPresetWidget( cfg->m_CurrentPreset );
            };

    if( index == 0 )
    {
        doApplyLayerPreset( FOLLOW_PCB );
    }
    else if( index == 1 )
    {
        doApplyLayerPreset( FOLLOW_PLOT_SETTINGS );
    }
    else if( index == count - 2 )
    {
        wxTextEntryDialog dlg( wxGetTopLevelParent( this ), _( "Layer preset name:" ),
                               _( "Save Layer Preset" ) );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        std::bitset<LAYER_3D_END> visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
        std::map<int, COLOR4D>    colors = m_frame->GetAdapter().GetLayerColors();

        name = dlg.GetValue();

        if( LAYER_PRESET_3D* preset = cfg->FindPreset( name ) )
        {
            if( !IsOK( wxGetTopLevelParent( this ), _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            preset->layers = visibleLayers;
            preset->colors = colors;
            m_cbLayerPresets->SetSelection( m_cbLayerPresets->FindString( name ) );
        }
        else
        {
            cfg->m_LayerPresets.emplace_back( name, visibleLayers, colors );
            rebuildLayerPresetsWidget( false );
            m_cbLayerPresets->SetStringSelection( name );
        }

        cfg->m_CurrentPreset = name;
        m_presetMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( LAYER_PRESET_3D& preset : cfg->m_LayerPresets )
        {
            wxArrayString item;
            item.Add( preset.name );
            items.emplace_back( item );
        }

        EDA_LIST_DIALOG dlg( m_frame, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            name = dlg.GetTextSelection();

            if( m_cbLayerPresets->FindString( name ) != wxNOT_FOUND )
                m_cbLayerPresets->Delete( m_cbLayerPresets->FindString( name ) );

            std::erase_if( cfg->m_LayerPresets,
                    [name]( const LAYER_PRESET_3D& preset )
                    {
                        return preset.name == name;
                    } );

            if( cfg->m_CurrentPreset == name )
                cfg->m_CurrentPreset = wxEmptyString;

            if( m_presetMRU.Index( name ) >= 0 )
                m_presetMRU.Remove( name );
        }

        resetSelection();
        return;
    }
    else if( m_cbLayerPresets->GetString( index ) == wxT( "---" ) )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else
    {
        doApplyLayerPreset( m_cbLayerPresets->GetStringSelection() );
    }

    // Move to front of MRU list
    if( m_presetMRU.Index( name ) != wxNOT_FOUND )
        m_presetMRU.Remove( name );

    m_presetMRU.Insert( name, 0 );

    passOnFocus();
}


void APPEARANCE_CONTROLS_3D::doApplyLayerPreset( const wxString& aPresetName )
{
    BOARD_ADAPTER& adapter = m_frame->GetAdapter();

    if( aPresetName == FOLLOW_PCB || aPresetName == FOLLOW_PLOT_SETTINGS )
    {
        adapter.m_Cfg->m_CurrentPreset = aPresetName;
        adapter.SetVisibleLayers( adapter.GetVisibleLayers() );
    }
    else if( LAYER_PRESET_3D* preset = adapter.m_Cfg->FindPreset( aPresetName ) )
    {
        adapter.m_Cfg->m_CurrentPreset = aPresetName;
        adapter.SetVisibleLayers( preset->layers );
        adapter.SetLayerColors( preset->colors );

        if( preset->name.Lower() == _( "legacy colors" ) )
            adapter.m_Cfg->m_UseStackupColors = false;
    }
    else
    {
        return;
    }

    UpdateLayerCtls();
    m_frame->NewDisplay( true );
}


void APPEARANCE_CONTROLS_3D::rebuildViewportsWidget()
{
    m_viewportsLabel->SetLabel( wxString::Format( _( "Viewports (%s+Tab):" ),
                                                  KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ) ) );

    m_cbViewports->Clear();

    for( std::pair<const wxString, VIEWPORT3D>& pair : m_viewports )
        m_cbViewports->Append( pair.first, static_cast<void*>( &pair.second ) );

    m_cbViewports->Append( wxT( "---" ) );
    m_cbViewports->Append( _( "Save viewport..." ) );
    m_cbViewports->Append( _( "Delete viewport..." ) );

    m_cbViewports->SetSelection( m_cbViewports->GetCount() - 3 );
    m_lastSelectedViewport = nullptr;
}


void APPEARANCE_CONTROLS_3D::onViewportChanged( wxCommandEvent& aEvent )
{
    int count = m_cbViewports->GetCount();
    int index = m_cbViewports->GetSelection();

    if( index >= 0 && index < count - 3 )
    {
        VIEWPORT3D* viewport = static_cast<VIEWPORT3D*>( m_cbViewports->GetClientData( index ) );

        wxCHECK( viewport, /* void */ );

        doApplyViewport( *viewport );

        if( !viewport->name.IsEmpty() )
        {
            if( m_viewportMRU.Index( viewport->name ) != wxNOT_FOUND )
                m_viewportMRU.Remove( viewport->name );

            m_viewportMRU.Insert( viewport->name, 0 );
        }
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        wxTextEntryDialog dlg( wxGetTopLevelParent( this ), _( "Viewport name:" ),
                               _( "Save Viewport" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            if( m_lastSelectedViewport )
                m_cbViewports->SetStringSelection( m_lastSelectedViewport->name );
            else
                m_cbViewports->SetSelection( m_cbViewports->GetCount() - 3 );

            return;
        }

        name = dlg.GetValue();
        bool exists = m_viewports.count( name );

        if( !exists )
        {
            m_viewports[name] = VIEWPORT3D( name, m_frame->GetCurrentCamera().GetViewMatrix() );

            index = m_cbViewports->Insert( name, index-1, static_cast<void*>( &m_viewports[name] ) );
        }
        else
        {
            m_viewports[name].matrix = m_frame->GetCurrentCamera().GetViewMatrix();
            index = m_cbViewports->FindString( name );

            if( m_viewportMRU.Index( name ) != wxNOT_FOUND )
                m_viewportMRU.Remove( name );
        }

        m_cbViewports->SetSelection( index );
        m_viewportMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete an existing viewport
        wxArrayString headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Viewports" ) );

        for( std::pair<const wxString, VIEWPORT3D>& pair : m_viewports )
        {
            wxArrayString item;
            item.Add( pair.first );
            items.emplace_back( item );
        }

        EDA_LIST_DIALOG dlg( m_frame, _( "Delete Viewport" ), headers, items );
        dlg.SetListLabel( _( "Select viewport:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString viewportName = dlg.GetTextSelection();
            int idx = m_cbViewports->FindString( viewportName );

            if( idx != wxNOT_FOUND )
            {
                m_viewports.erase( viewportName );
                m_cbViewports->Delete( idx );
            }

            if( m_viewportMRU.Index( viewportName ) != wxNOT_FOUND )
                m_viewportMRU.Remove( viewportName );
        }

        if( m_lastSelectedViewport )
            m_cbViewports->SetStringSelection( m_lastSelectedViewport->name );
        else
            m_cbViewports->SetSelection( m_cbViewports->GetCount() - 3 );

        return;
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS_3D::onUpdateViewportsCb( wxUpdateUIEvent& aEvent )
{
    int count = m_cbViewports->GetCount();
    int index = m_cbViewports->GetSelection();

    if( index >= 0 && index < count - 3 )
    {
        VIEWPORT3D* viewport = static_cast<VIEWPORT3D*>( m_cbViewports->GetClientData( index ) );

        wxCHECK( viewport, /* void */ );

        if( m_frame->GetCurrentCamera().GetViewMatrix() != viewport->matrix )
            m_cbViewports->SetSelection( -1 );
    }
}


void APPEARANCE_CONTROLS_3D::doApplyViewport( const VIEWPORT3D& aViewport )
{
    m_frame->GetCurrentCamera().SetViewMatrix( aViewport.matrix );

    if( m_frame->GetAdapter().m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
        m_frame->GetCanvas()->Request_refresh();
    else
        m_frame->GetCanvas()->RenderRaytracingRequest();
}


void APPEARANCE_CONTROLS_3D::passOnFocus()
{
    m_focusOwner->SetFocus();
}
