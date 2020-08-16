/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/appearance_controls.h>

#include <bitmaps.h>
#include <class_board.h>
#include <menus_helpers.h>
#include <pcb_display_options.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <settings/color_settings.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <widgets/bitmap_toggle.h>
#include <widgets/color_swatch.h>
#include <widgets/indicator_icon.h>
#include <dialog_helpers.h>

/// Template for object appearance settings
const APPEARANCE_CONTROLS::APPEARANCE_SETTING APPEARANCE_CONTROLS::s_objectSettings[] = {

#define RR  APPEARANCE_CONTROLS::APPEARANCE_SETTING   // Render Row abbreviation to reduce source width

    //     text                  id                        tooltip                   opacity slider
    RR( _( "Tracks" ),           LAYER_TRACKS,             _( "Show tracks" ),       true ),
    RR( _( "Vias" ),             LAYER_VIAS,               _( "Show all vias" ),     true ),
    RR( _( "Pads" ),             LAYER_PADS,               _( "Show all pads" ),     true ),
    RR( _( "Zones" ),            LAYER_ZONES,              _( "Show copper zones" ), true ),
    RR(),
    RR( _( "Footprints Front" ), LAYER_MOD_FR,             _( "Show footprints that are on board's front" ) ),
    RR( _( "Footprints Back" ),  LAYER_MOD_BK,             _( "Show footprints that are on board's back" ) ),
    RR( _( "Values" ),           LAYER_MOD_VALUES,         _( "Show footprint values" ) ),
    RR( _( "References" ),       LAYER_MOD_REFERENCES,     _( "Show footprint references" ) ),
    RR( _( "Footprint Text" ),   LAYER_MOD_TEXT_FR,        _( "Show footprint text" ) ),
    RR( _( "Hidden Text" ),      LAYER_MOD_TEXT_INVISIBLE, _( "Show footprint text marked as invisible" ) ),
    RR(),
    RR( _( "Ratsnest" ),         LAYER_RATSNEST,           _( "Show unconnected nets as a ratsnest") ),
    RR( _( "No-Connects" ),      LAYER_NO_CONNECTS,        _( "Show a marker on pads which have no net connected" ) ),
    RR( _( "DRC Warnings" ),     LAYER_DRC_WARNING,        _( "DRC violations with a Warning severity" ) ),
    RR( _( "DRC Errors" ),       LAYER_DRC_ERROR,          _( "DRC violations with an Error severity" ) ),
    RR( _( "DRC Exclusions" ),   LAYER_DRC_EXCLUSION,      _( "DRC violations which have been individually excluded" ) ),
    RR( _( "Anchors" ),          LAYER_ANCHOR,             _( "Show footprint and text origins as a cross" ) ),
    RR( _( "Worksheet" ),        LAYER_WORKSHEET,          _( "Show worksheet" ) ),
    RR( _( "Grid" ),             LAYER_GRID,               _( "Show the (x,y) grid dots" ) )
};

// These are the built-in layer presets that cannot be deleted

LAYER_PRESET APPEARANCE_CONTROLS::presetNoLayers( _( "No Layers" ), LSET() );

LAYER_PRESET APPEARANCE_CONTROLS::presetAllLayers( _( "All Layers" ), LSET::AllLayersMask() );

LAYER_PRESET APPEARANCE_CONTROLS::presetAllCopper( _( "All Copper Layers" ), LSET::AllCuMask() );

LAYER_PRESET APPEARANCE_CONTROLS::presetInnerCopper( _( "Inner Copper Layers" ),
        LSET::InternalCuMask().set( Edge_Cuts ) );

LAYER_PRESET APPEARANCE_CONTROLS::presetFront( _( "Front Layers" ),
        LSET::FrontMask().set( Edge_Cuts ) );

LAYER_PRESET APPEARANCE_CONTROLS::presetFrontAssembly( _( "Front Assembly View" ),
        LSET::FrontAssembly().set( Edge_Cuts ), GAL_SET::DefaultVisible(), F_SilkS );

LAYER_PRESET APPEARANCE_CONTROLS::presetBack( _( "Back Layers" ),
        LSET::BackMask().set( Edge_Cuts ) );

LAYER_PRESET APPEARANCE_CONTROLS::presetBackAssembly( _( "Back Assembly View" ),
        LSET::BackAssembly().set( Edge_Cuts ), GAL_SET::DefaultVisible(), B_SilkS );


APPEARANCE_CONTROLS::APPEARANCE_CONTROLS( PCB_BASE_FRAME* aParent, wxWindow* aFocusOwner,
                                          bool aFpEditorMode ) :
        APPEARANCE_CONTROLS_BASE( aParent ),
        m_frame( aParent ),
        m_board( nullptr ),
        m_currentPreset( nullptr ),
        m_layerContextMenu( nullptr ),
        m_contextMenuNetCode( 0 )
{
    int indicatorSize = ConvertDialogToPixels( wxSize( 6, 6 ) ).x;
    m_iconProvider    = new ROW_ICON_PROVIDER( indicatorSize );
    int pointSize     = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ).GetPointSize();
    int screenHeight  = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );

    m_btnNetInspector->SetBitmapLabel( KiBitmap( list_nets_xpm ) );
    m_btnConfigureNetClasses->SetBitmapLabel( KiBitmap( options_generic_xpm ) );

    m_txtNetFilter->SetHint( _( "Filter nets" ) );

    if( screenHeight <= 900 && pointSize >= indicatorSize )
        pointSize = pointSize * 8 / 10;

    m_pointSize = pointSize;

#ifdef __WXMAC__
    wxFont font = m_notebook->GetFont();
    font.SetPointSize( m_pointSize );
    m_notebook->SetFont( font );
#endif

    auto setHighContrastMode =
            [&]( HIGH_CONTRAST_MODE aMode )
            {
                PCB_DISPLAY_OPTIONS opts   = m_frame->GetDisplayOptions();
                opts.m_ContrastModeDisplay = aMode;

                m_frame->SetDisplayOptions( opts );
                m_frame->GetCanvas()->SetHighContrastLayer( m_frame->GetActiveLayer() );
                m_frame->GetCanvas()->Refresh();
            };

    m_rbHighContrastNormal->Bind( wxEVT_RADIOBUTTON,
            [=]( wxCommandEvent& aEvent )
            {
                setHighContrastMode( HIGH_CONTRAST_MODE::NORMAL );
            } );

    m_rbHighContrastDim->Bind( wxEVT_RADIOBUTTON,
            [=]( wxCommandEvent& aEvent )
            {
                setHighContrastMode( HIGH_CONTRAST_MODE::DIMMED );
            } );

    m_rbHighContrastOff->Bind( wxEVT_RADIOBUTTON,
            [=]( wxCommandEvent& aEvent )
            {
                setHighContrastMode( HIGH_CONTRAST_MODE::HIDDEN );
            } );

    m_cbLayerPresets->Bind( wxEVT_CHOICE, &APPEARANCE_CONTROLS::onLayerPresetChanged, this );

    m_btnNetInspector->Bind( wxEVT_BUTTON,
            [&]( wxCommandEvent& aEvent )
            {
                m_frame->GetToolManager()->RunAction( PCB_ACTIONS::listNets, true );
            } );

    m_btnConfigureNetClasses->Bind( wxEVT_BUTTON,
            [&]( wxCommandEvent& aEvent )
            {
                // This panel should only be visible in the PCB_EDIT_FRAME anyway
                if( PCB_EDIT_FRAME* editframe = dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) )
                    editframe->ShowBoardSetupDialog( _( "Net Classes" ) );
            } );

    m_cbFlipBoard->SetValue( m_frame->GetCanvas()->GetView()->IsMirroredX() );
    m_cbFlipBoard->Bind( wxEVT_CHECKBOX,
            [&]( wxCommandEvent& aEvent )
            {
                m_frame->GetToolManager()->RunAction( PCB_ACTIONS::flipBoard, true );
            } );

    m_paneLayerDisplay->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    loadDefaultLayerPresets();
    rebuildObjects();
    OnBoardChanged();

    Bind( wxEVT_COMMAND_MENU_SELECTED, &APPEARANCE_CONTROLS::OnLayerContextMenu, this,
          ID_CHANGE_COLOR, ID_LAST_VALUE );

    m_rbNetColorAll->Bind( wxEVT_RADIOBUTTON, &APPEARANCE_CONTROLS::onNetColorModeChanged, this );
    m_rbNetColorOff->Bind( wxEVT_RADIOBUTTON, &APPEARANCE_CONTROLS::onNetColorModeChanged, this );
    m_rbNetColorRatsnest->Bind( wxEVT_RADIOBUTTON,
                                &APPEARANCE_CONTROLS::onNetColorModeChanged, this );
}


APPEARANCE_CONTROLS::~APPEARANCE_CONTROLS()
{
    delete m_iconProvider;
}


wxSize APPEARANCE_CONTROLS::GetBestSize() const
{
    wxSize size( 220, 480 );
    // TODO(JE) appropriate logic
    return size;
}


void APPEARANCE_CONTROLS::OnLayerDisplayPaneChanged( wxCollapsiblePaneEvent& event )
{
    // Because wxWidgets is broken and will not properly lay these out automatically
    Freeze();
    m_panelLayers->Fit();
    m_sizerOuter->Layout();
    Thaw();
}


void APPEARANCE_CONTROLS::OnNetDisplayPaneChanged( wxCollapsiblePaneEvent& event )
{
    // Because wxWidgets is broken and will not properly lay these out automatically
    Freeze();
    m_panelNetsAndClasses->Fit();
    m_sizerOuter->Layout();
    Thaw();
}


void APPEARANCE_CONTROLS::OnNotebookPageChanged( wxNotebookEvent& aEvent )
{
    // Work around wxMac issue where the notebook pages are blank
#ifdef __WXMAC__
    int page = aEvent.GetSelection();

    if( page >= 0 )
        m_notebook->ChangeSelection( static_cast<unsigned>( page ) );
#endif

    // Because wxWidgets is broken and will send click events to children of the collapsible
    // panes even if they are collapsed without this
    Freeze();
    m_panelLayers->Fit();
    m_panelNetsAndClasses->Fit();
    m_sizerOuter->Layout();
    Thaw();
}


void APPEARANCE_CONTROLS::OnBoardChanged()
{
    Freeze();
    rebuildLayers();
    rebuildLayerContextMenu();
    syncColorsAndVisibility();
    syncObjectSettings();
    rebuildNets();
    rebuildLayerPresetsWidget();
    syncLayerPresetSelection();

    UpdateDisplayOptions();

    m_board = m_frame->GetBoard();

    if( m_board )
        m_board->AddListener( this );

    Thaw();
    Refresh();
}


void APPEARANCE_CONTROLS::OnBoardNetSettingsChanged( BOARD& aBoard )
{
    handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( aBoardItem->Type() == PCB_NETINFO_T )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( aBoardItem->Type() == PCB_NETINFO_T )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( aBoardItem->Type() == PCB_NETINFO_T )
        handleBoardItemsChanged();
}



void APPEARANCE_CONTROLS::handleBoardItemsChanged()
{
    Freeze();
    rebuildNets();
    Thaw();
}


void APPEARANCE_CONTROLS::OnColorThemeChanged()
{
    syncColorsAndVisibility();
    syncObjectSettings();
}


void APPEARANCE_CONTROLS::OnLayerChanged()
{
    static wxColour normalColor    = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    static wxColour highlightColor = wxSystemSettings::GetColour( wxSYS_COLOUR_SCROLLBAR );

    PCB_LAYER_ID current = m_frame->GetActiveLayer();

    for( std::unique_ptr<APPEARANCE_SETTING>& setting : m_layerSettings )
    {
        LAYER_NUM layer = setting->id;

        if( setting->ctl_panel )
        {
            setting->ctl_panel->SetBackgroundColour( current == layer ?
                                                     highlightColor : normalColor );
        }

        if( setting->ctl_indicator )
        {
            setting->ctl_indicator->SetIndicatorState( current == layer ?
                                                       ROW_ICON_PROVIDER::STATE::ON :
                                                       ROW_ICON_PROVIDER::STATE::OFF );
        }
    }

#if defined( __WXMAC__ ) || defined( __WXMSW__ )
    Refresh();
#endif
}


void APPEARANCE_CONTROLS::SetLayerVisible( LAYER_NUM aLayer, bool isVisible )
{
    BOARD*       board   = m_frame->GetBoard();
    LSET         visible = board->GetVisibleLayers();
    PCB_LAYER_ID layer   = ToLAYER_ID( aLayer );

    if( visible.test( layer ) == isVisible )
        return;

    visible.set( layer, isVisible );
    board->SetVisibleLayers( visible );

    m_frame->GetCanvas()->GetView()->SetLayerVisible( layer, isVisible );

    syncColorsAndVisibility();
}


void APPEARANCE_CONTROLS::SetObjectVisible( GAL_LAYER_ID aLayer, bool isVisible )
{
    // Currently only used for the grid, so we just update the ui
    if( m_objectSettingsMap.count( aLayer ) )
    {
        APPEARANCE_SETTING* setting = m_objectSettingsMap.at( aLayer );
        setting->ctl_visibility->SetValue( isVisible );
    }
}


void APPEARANCE_CONTROLS::OnLayerAlphaChanged()
{
    // TODO(JE) Is this even needed if the layer alphas are getting directly updated?
    // Maybe we just need the "down" arrow to indicate if the alpha is below 1

#if 0
    static constexpr double alphaEpsilon = 0.04;

    PCB_LAYER_ID        current = m_frame->GetActiveLayer();
    COLOR_SETTINGS*     theme   = m_frame->GetColorSettings();
    KIGFX::PCB_PAINTER* painter =
            static_cast<KIGFX::PCB_PAINTER*>( m_frame->GetCanvas()->GetView()->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* rs = painter->GetSettings();

    for( APPEARANCE_SETTING& setting : m_layerSettings )
    {
        if( !setting.ctl_indicator )
            continue;

        COLOR4D layerColor  = theme->GetColor( setting.id );
        COLOR4D screenColor = rs->GetLayerColor( setting.id );

        if( std::abs( screenColor.a - layerColor.a ) > alphaEpsilon )
        {
            if( screenColor.a < layerColor.a )
                setting.ctl_indicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::DOWN );
            else
                setting.ctl_indicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::UP );
        }
        else
        {
            setting.ctl_indicator->SetIndicatorState( setting.id == current ?
                                                      ROW_ICON_PROVIDER::STATE::ON :
                                                      ROW_ICON_PROVIDER::STATE::OFF );
        }
    }
#endif
}


void APPEARANCE_CONTROLS::UpdateDisplayOptions()
{
    const PCB_DISPLAY_OPTIONS& options = m_frame->GetDisplayOptions();

    switch( options.m_ContrastModeDisplay )
    {
    case HIGH_CONTRAST_MODE::NORMAL: m_rbHighContrastNormal->SetValue( true ); break;
    case HIGH_CONTRAST_MODE::DIMMED: m_rbHighContrastDim->SetValue( true );    break;
    case HIGH_CONTRAST_MODE::HIDDEN: m_rbHighContrastOff->SetValue( true );    break;
    }

    switch( options.m_NetColorMode )
    {
    case NET_COLOR_MODE::ALL:      m_rbNetColorAll->SetValue( true );      break;
    case NET_COLOR_MODE::RATSNEST: m_rbNetColorRatsnest->SetValue( true ); break;
    case NET_COLOR_MODE::OFF:      m_rbNetColorOff->SetValue( true );      break;
    }

    wxASSERT( m_objectSettingsMap.count( LAYER_RATSNEST ) );
    APPEARANCE_SETTING* ratsnest = m_objectSettingsMap.at( LAYER_RATSNEST );
    ratsnest->ctl_visibility->SetValue( options.m_ShowGlobalRatsnest );
}


std::vector<LAYER_PRESET> APPEARANCE_CONTROLS::GetUserLayerPresets() const
{
    std::vector<LAYER_PRESET> ret;

    for( const auto& pair : m_layerPresets )
        if( !pair.second.readOnly )
            ret.emplace_back( pair.second );

    return ret;
}


void APPEARANCE_CONTROLS::SetUserLayerPresets( std::vector<LAYER_PRESET>& aPresetList )
{
    // Reset to defaults
    loadDefaultLayerPresets();

    for( const LAYER_PRESET& preset : aPresetList )
    {
        if( m_layerPresets.count( preset.name ) )
            continue;

        m_layerPresets[preset.name] = preset;
    }

    rebuildLayerPresetsWidget();
}


void APPEARANCE_CONTROLS::loadDefaultLayerPresets()
{
    m_layerPresets.clear();

    // Load the read-only defaults
    for( const LAYER_PRESET& preset : { presetAllLayers, presetAllCopper, presetInnerCopper,
                                        presetFront, presetFrontAssembly, presetBack,
                                        presetBackAssembly } )
    {
        m_layerPresets[preset.name]          = preset;
        m_layerPresets[preset.name].readOnly = true;
    }
}


void APPEARANCE_CONTROLS::ApplyLayerPreset( const wxString& aPresetName )
{
    updateLayerPresetSelection( aPresetName );

    wxCommandEvent dummy;
    onLayerPresetChanged( dummy );
}


void APPEARANCE_CONTROLS::ApplyLayerPreset( const LAYER_PRESET& aPreset )
{
    if( m_layerPresets.count( aPreset.name ) )
        m_currentPreset = &m_layerPresets[aPreset.name];
    else
        m_currentPreset = nullptr;

    updateLayerPresetSelection( aPreset.name );
    doApplyLayerPreset( aPreset );
}


void APPEARANCE_CONTROLS::rebuildLayers()
{
    BOARD* board = m_frame->GetBoard();
    LSET enabled = board->GetEnabledLayers();
    LSET visible = board->GetVisibleLayers();

    COLOR_SETTINGS* theme      = m_frame->GetColorSettings();
    COLOR4D         bgColor    = theme->GetColor( LAYER_PCB_BACKGROUND );
    bool            firstLayer = true;

#ifdef __WXMAC__
    wxSizerItem* m_windowLayersSizerItem = m_panelLayersSizer->GetItem( m_windowLayers );
    m_windowLayersSizerItem->SetFlag( m_windowLayersSizerItem->GetFlag() & ~wxTOP );
#endif

    m_layerSettings.clear();
    m_layers_outer_sizer->Clear( true );

    auto appendLayer =
            [&]( std::unique_ptr<APPEARANCE_SETTING>& aSetting )
            {
                int layer = aSetting->id;

                wxPanel*    panel = new wxPanel( m_windowLayers, layer );
                wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
                panel->SetSizer( sizer );

                aSetting->visible = visible[layer];

                // TODO(JE) consider restyling this indicator
                INDICATOR_ICON* indicator = new INDICATOR_ICON( panel, *m_iconProvider,
                        ROW_ICON_PROVIDER::STATE::OFF, layer );

                COLOR_SWATCH* swatch = new COLOR_SWATCH( panel, COLOR4D::UNSPECIFIED, layer,
                                                         bgColor, theme->GetColor( layer ), false );
                swatch->SetToolTip( _( "Left double click or middle click for color change, "
                                       "right click for menu" ) );

                BITMAP_TOGGLE* btn_visible =
                        new BITMAP_TOGGLE( panel, layer, KiBitmap( visibility_xpm ),
                                           KiBitmap( visibility_off_xpm ), aSetting->visible );
                btn_visible->SetToolTip( _( "Show or hide this layer" ) );

                wxStaticText* label = new wxStaticText( panel, layer, aSetting->label );
                label->Wrap( -1 );
                label->SetToolTip( aSetting->tooltip );

                int topMargin = firstLayer ? 2 : 1;
                firstLayer = false;

                sizer->AddSpacer( 1 );
                sizer->Add( indicator, 0, wxALIGN_CENTER_VERTICAL | wxTOP, topMargin );
                sizer->AddSpacer( 5 );
                sizer->Add( swatch, 0, wxALIGN_CENTER_VERTICAL | wxTOP, topMargin );
                sizer->AddSpacer( 6 );
                sizer->Add( btn_visible, 0, wxALIGN_CENTER_VERTICAL | wxTOP, topMargin );
                sizer->AddSpacer( 5 );
                sizer->Add( label, 1, wxALIGN_CENTER_VERTICAL | wxTOP, topMargin );

                m_layers_outer_sizer->Add( panel, 0, wxEXPAND, 0 );

                aSetting->ctl_panel      = panel;
                aSetting->ctl_indicator  = indicator;
                aSetting->ctl_visibility = btn_visible;
                aSetting->ctl_color      = swatch;
                aSetting->ctl_text       = label;

                panel->Bind( wxEVT_LEFT_DOWN, &APPEARANCE_CONTROLS::onLayerClick, this );
                indicator->Bind( wxEVT_LEFT_DOWN, &APPEARANCE_CONTROLS::onLayerClick, this );
                swatch->Bind( wxEVT_LEFT_DOWN, &APPEARANCE_CONTROLS::onLayerClick, this );
                label->Bind( wxEVT_LEFT_DOWN, &APPEARANCE_CONTROLS::onLayerClick, this );

                btn_visible->Bind( TOGGLE_CHANGED,
                        [&]( wxCommandEvent& aEvent )
                        {
                            int layId = static_cast<wxWindow*>( aEvent.GetEventObject() )->GetId();
                            bool isVisible = aEvent.GetInt();

                            wxASSERT( layId >= 0 && layId < PCB_LAYER_ID_COUNT );

                            onLayerVisibilityChanged( static_cast<PCB_LAYER_ID>( layId ),
                                                      isVisible, true );
                        } );

                swatch->Bind( COLOR_SWATCH_CHANGED,
                              &APPEARANCE_CONTROLS::OnColorSwatchChanged, this );

                auto rightClickHandler =
                        [&]( wxMouseEvent& aEvent )
                        {
                            wxASSERT( m_layerContextMenu );
                            PopupMenu( m_layerContextMenu );
                        };

                panel->Bind( wxEVT_RIGHT_DOWN, rightClickHandler );
                indicator->Bind( wxEVT_RIGHT_DOWN, rightClickHandler );
                swatch->Bind( wxEVT_RIGHT_DOWN, rightClickHandler );
                btn_visible->Bind( wxEVT_RIGHT_DOWN, rightClickHandler );
                label->Bind( wxEVT_RIGHT_DOWN, rightClickHandler );
            };

    wxString dsc;

    // show all coppers first, with front on top, back on bottom, then technical layers
    for( LSEQ cu_stack = enabled.CuStack(); cu_stack; ++cu_stack )
    {
        PCB_LAYER_ID layer = *cu_stack;

        switch( layer )
        {
            case F_Cu: dsc = _( "Front copper layer" ); break;
            case B_Cu: dsc = _( "Back copper layer" );  break;
            default:   dsc = _( "Inner copper layer" ); break;
        }

        m_layerSettings.emplace_back(
                std::make_unique<APPEARANCE_SETTING>( board->GetLayerName( layer ), layer, dsc ) );

        std::unique_ptr<APPEARANCE_SETTING>& setting = m_layerSettings.back();

        m_layerSettingsMap[layer] = setting.get();

        appendLayer( setting );

        // TODO(JE)
#ifdef NOTYET
        if( m_fp_editor_mode && LSET::ForbiddenFootprintLayers().test( layer ) )
        {
            getLayerComp( GetLayerRowCount()-1, COLUMN_COLOR_LYRNAME )->Enable( false );
            getLayerComp( GetLayerRowCount()-1, COLUMN_COLORBM )->SetToolTip( wxEmptyString );
        }
#endif
    }

    // technical layers are shown in this order:
    // Because they are static, wxGetTranslation must be explicitly
    // called for tooltips.
    static const struct {
        PCB_LAYER_ID layerId;
        wxString     tooltip;
    } non_cu_seq[] = {
        { F_Adhes,          _( "Adhesive on board's front" ) },
        { B_Adhes,          _( "Adhesive on board's back" ) },
        { F_Paste,          _( "Solder paste on board's front" ) },
        { B_Paste,          _( "Solder paste on board's back" ) },
        { F_SilkS,          _( "Silkscreen on board's front" ) },
        { B_SilkS,          _( "Silkscreen on board's back" ) },
        { F_Mask,           _( "Solder mask on board's front" ) },
        { B_Mask,           _( "Solder mask on board's back" ) },
        { Dwgs_User,        _( "Explanatory drawings" ) },
        { Cmts_User,        _( "Explanatory comments" ) },
        { Eco1_User,        _( "User defined meaning" ) },
        { Eco2_User,        _( "User defined meaning" ) },
        { Edge_Cuts,        _( "Board's perimeter definition" ) },
        { Margin,           _( "Board's edge setback outline" ) },
        { F_CrtYd,          _( "Footprint courtyards on board's front" ) },
        { B_CrtYd,          _( "Footprint courtyards on board's back" ) },
        { F_Fab,            _( "Footprint assembly on board's front" ) },
        { B_Fab,            _( "Footprint assembly on board's back" ) }
    };

    for( const auto& entry : non_cu_seq )
    {
        PCB_LAYER_ID layer = entry.layerId;

        if( !enabled[layer] )
            continue;

        m_layerSettings.emplace_back( std::make_unique<APPEARANCE_SETTING>(
                board->GetLayerName( layer ), layer, wxGetTranslation( entry.tooltip ) ) );

        std::unique_ptr<APPEARANCE_SETTING>& setting = m_layerSettings.back();

        m_layerSettingsMap[layer] = setting.get();

        appendLayer( setting );

        // TODO(JE)
#ifdef NOTYET
        if( m_fp_editor_mode && LSET::ForbiddenFootprintLayers().test( layer ) )
        {
            getLayerComp( GetLayerRowCount()-1, COLUMN_COLOR_LYRNAME )->Enable( false );
            getLayerComp( GetLayerRowCount()-1, COLUMN_COLORBM )->SetToolTip( wxEmptyString );
        }
#endif
    }

    m_layers_outer_sizer->Layout();
}


void APPEARANCE_CONTROLS::rebuildLayerContextMenu()
{
    delete m_layerContextMenu;
    m_layerContextMenu = new wxMenu;

    AddMenuItem( m_layerContextMenu, ID_SHOW_ALL_COPPER_LAYERS,
                 _( "Show All Copper Layers" ),
                 KiBitmap( select_layer_pair_xpm ) );
    AddMenuItem( m_layerContextMenu, ID_HIDE_ALL_COPPER_LAYERS,
                 _( "Hide All Copper Layers" ),
                 KiBitmap( show_no_copper_layers_xpm ) );

    m_layerContextMenu->AppendSeparator();

    AddMenuItem( m_layerContextMenu, ID_HIDE_ALL_BUT_ACTIVE,
                 _( "Hide All Layers But Active" ),
                 KiBitmap( select_w_layer_xpm ) );

    m_layerContextMenu->AppendSeparator();

    AddMenuItem( m_layerContextMenu, ID_SHOW_ALL_NON_COPPER, _( "Show All Non Copper Layers" ),
                 KiBitmap( select_w_layer_xpm ) );

    AddMenuItem( m_layerContextMenu, ID_HIDE_ALL_NON_COPPER, _( "Hide All Non Copper Layers" ),
                 KiBitmap( show_no_copper_layers_xpm ) );

    m_layerContextMenu->AppendSeparator();

    AddMenuItem( m_layerContextMenu, ID_PRESET_ALL_LAYERS, _( "Show All Layers" ),
                 KiBitmap( show_all_layers_xpm ) );

    AddMenuItem( m_layerContextMenu, ID_PRESET_NO_LAYERS, _( "Hide All Layers" ),
                 KiBitmap( show_no_layers_xpm ) );

    m_layerContextMenu->AppendSeparator();

    AddMenuItem( m_layerContextMenu, ID_PRESET_FRONT_ASSEMBLY,
                 _( "Show Only Front Assembly Layers" ), KiBitmap( shape_3d_xpm ) );

    AddMenuItem( m_layerContextMenu, ID_PRESET_FRONT, _( "Show Only Front Layers" ),
                 KiBitmap( show_all_front_layers_xpm ) );

    // Only show the internal layer option if internal layers are enabled
    if( m_frame->GetBoard()->GetCopperLayerCount() > 2 )
    {
        AddMenuItem( m_layerContextMenu, ID_PRESET_INNER_COPPER, _( "Show Only Inner Layers" ),
                     KiBitmap( show_all_copper_layers_xpm ) );
    }

    AddMenuItem( m_layerContextMenu, ID_PRESET_BACK, _( "Show Only Back Layers" ),
                 KiBitmap( show_all_back_layers_xpm ) );

    AddMenuItem( m_layerContextMenu, ID_PRESET_BACK_ASSEMBLY, _( "Show Only Back Assembly Layers" ),
                 KiBitmap( shape_3d_back_xpm ) );
}


void APPEARANCE_CONTROLS::OnLayerContextMenu( wxCommandEvent& aEvent )
{
    BOARD* board   = m_frame->GetBoard();
    LSET   visible = board->GetVisibleLayers();

    PCB_LAYER_ID current = m_frame->GetActiveLayer();

    switch( aEvent.GetId() )
    {
    case ID_PRESET_NO_LAYERS:
        ApplyLayerPreset( presetNoLayers );
        return;

    case ID_PRESET_ALL_LAYERS:
        ApplyLayerPreset( presetAllLayers );
        return;

    case ID_SHOW_ALL_COPPER_LAYERS:
    {
        visible |= presetAllCopper.layers;
        board->SetVisibleLayers( visible );
        break;
    }

    case ID_HIDE_ALL_BUT_ACTIVE:
        ApplyLayerPreset( presetNoLayers );
        SetLayerVisible( current, true );
        break;

    case ID_HIDE_ALL_COPPER_LAYERS:
    {
        visible &= ~presetAllCopper.layers;

        if( !visible.test( current ) )
            m_frame->SetActiveLayer( *visible.Seq().begin() );

        board->SetVisibleLayers( visible );
        break;
    }

    case ID_HIDE_ALL_NON_COPPER:
    {
        visible &= presetAllCopper.layers;

        if( !visible.test( current ) )
            m_frame->SetActiveLayer( *visible.Seq().begin() );

        board->SetVisibleLayers( visible );
        break;
    }

    case ID_SHOW_ALL_NON_COPPER:
    {
        visible |= ~presetAllCopper.layers;

        board->SetVisibleLayers( visible );
        break;
    }

    case ID_PRESET_FRONT_ASSEMBLY:
        ApplyLayerPreset( presetFrontAssembly );
        return;

    case ID_PRESET_FRONT:
        ApplyLayerPreset( presetFront );
        return;

    case ID_PRESET_INNER_COPPER:
        ApplyLayerPreset( presetInnerCopper );
        return;

    case ID_PRESET_BACK:
        ApplyLayerPreset( presetBack );
        return;

    case ID_PRESET_BACK_ASSEMBLY:
        ApplyLayerPreset( presetBackAssembly );
        return;
    }

    syncLayerPresetSelection();
    syncColorsAndVisibility();
    m_frame->GetCanvas()->SyncLayersVisibility( board );
    m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::syncColorsAndVisibility()
{
    BOARD*  board   = m_frame->GetBoard();
    LSET    visible = board->GetVisibleLayers();
    GAL_SET objects = board->GetVisibleElements();

    Freeze();

    for( std::unique_ptr<APPEARANCE_SETTING>& setting : m_layerSettings )
    {
        LAYER_NUM layer = setting->id;

        if( setting->ctl_visibility )
            setting->ctl_visibility->SetValue( visible[layer] );

        if( setting->ctl_color )
        {
            const COLOR4D& color = m_frame->GetColorSettings()->GetColor( layer );
            setting->ctl_color->SetSwatchColor( color, false );
        }
    }

    for( std::unique_ptr<APPEARANCE_SETTING>& setting : m_objectSettings )
    {
        GAL_LAYER_ID layer = static_cast<GAL_LAYER_ID>( setting->id );

        if( setting->ctl_visibility )
            setting->ctl_visibility->SetValue( objects.Contains( layer ) );

        if( setting->ctl_color )
        {
            const COLOR4D& color = m_frame->GetColorSettings()->GetColor( layer );
            setting->ctl_color->SetSwatchColor( color, false );
        }
    }

    // Update indicators and panel background colors
    OnLayerChanged();

    Thaw();

    m_windowLayers->Refresh();
}


void APPEARANCE_CONTROLS::onLayerClick( wxMouseEvent& aEvent )
{
    auto eventSource = static_cast<wxWindow*>( aEvent.GetEventObject() );

    PCB_LAYER_ID layer = ToLAYER_ID( eventSource->GetId() );

    // TODO(JE)
#ifdef NOTYET
    if( m_fp_editor_mode && LSET::ForbiddenFootprintLayers().test( layer ) )
        return false;
#endif

    m_frame->SetActiveLayer( layer );
}


void APPEARANCE_CONTROLS::onLayerVisibilityChanged( PCB_LAYER_ID aLayer, bool isVisible,
                                                    bool isFinal )
{
    BOARD* board = m_frame->GetBoard();

    LSET visibleLayers = board->GetVisibleLayers();

    if( visibleLayers.test( aLayer ) != isVisible )
    {
        visibleLayers.set( aLayer, isVisible );

        board->SetVisibleLayers( visibleLayers );

        m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, isVisible );
    }

    syncLayerPresetSelection();

    if( isFinal )
        m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::onObjectVisibilityChanged( GAL_LAYER_ID aLayer, bool isVisible,
                                                     bool isFinal )
{
    BOARD*  board   = m_frame->GetBoard();
    GAL_SET visible = board->GetVisibleElements();

    // Special-case controls
    switch( aLayer )
    {
    case LAYER_RATSNEST:
    {
        // don't touch the layers. ratsnest is enabled on per-item basis.
        m_frame->GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
        m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, true );

        if( m_frame->IsType( FRAME_PCB_EDITOR ) )
        {
            PCB_DISPLAY_OPTIONS opt  = m_frame->GetDisplayOptions();
            opt.m_ShowGlobalRatsnest = isVisible;
            m_frame->SetDisplayOptions( opt );
        }

        break;
    }

    case LAYER_GRID:
        m_frame->SetGridVisibility( isVisible );
        m_frame->GetCanvas()->Refresh();
        syncLayerPresetSelection();
        break;

    default:
        break;
    }

    if( visible.Contains( aLayer ) != isVisible )
    {
        visible.set( aLayer, isVisible );
        board->SetVisibleElements( visible );
        m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, isVisible );
        syncLayerPresetSelection();
    }

    if( isFinal )
        m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::rebuildObjects()
{
    BOARD*          board   = m_frame->GetBoard();
    COLOR_SETTINGS* theme   = m_frame->GetColorSettings();
    COLOR4D         bgColor = theme->GetColor( LAYER_PCB_BACKGROUND );
    GAL_SET         visible = board->GetVisibleElements();
    int             swatchWidth = m_windowObjects->ConvertDialogToPixels( wxSize( 8, 0 ) ).x;
    int             labelWidth = 0;

    m_objectSettings.clear();
    m_objectsSizer->Clear( true );
    m_objectsSizer->AddSpacer( 5 );

    auto appendObject =
            [&]( const std::unique_ptr<APPEARANCE_SETTING>& aSetting )
            {
                wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
                int         layer = aSetting->id;

                aSetting->visible = visible.Contains( ToGalLayer( layer ) );
                COLOR4D color     = theme->GetColor( layer );
                COLOR4D defColor  = theme->GetDefaultColor( layer );

                if( color != COLOR4D::UNSPECIFIED )
                {
                    COLOR_SWATCH* swatch = new COLOR_SWATCH( m_windowObjects, color, layer,
                                                             bgColor, defColor, false );
                    swatch->SetToolTip( _( "Left double click or middle click for color change, "
                                           "right click for menu" ) );

                    sizer->Add( swatch, 0,  wxALIGN_CENTER_VERTICAL, 0 );
                    aSetting->ctl_color = swatch;

                    swatch->Bind( COLOR_SWATCH_CHANGED,
                                  &APPEARANCE_CONTROLS::OnColorSwatchChanged, this );
                }
                else
                {
                    sizer->AddSpacer( swatchWidth  );
                }

                BITMAP_TOGGLE* btn_visible = new BITMAP_TOGGLE( m_windowObjects, layer,
                                                                KiBitmap( visibility_xpm ),
                                                                KiBitmap( visibility_off_xpm ),
                                                                aSetting->visible );

                wxString tip;
                tip.Printf( _( "Show or hide %s" ), aSetting->label.Lower() );
                btn_visible->SetToolTip( tip );

                aSetting->ctl_visibility = btn_visible;

                sizer->AddSpacer( 5 );

                btn_visible->Bind( TOGGLE_CHANGED,
                        [&]( wxCommandEvent& aEvent )
                        {
                            int id = static_cast<wxWindow*>( aEvent.GetEventObject() )->GetId();
                            bool isVisible = aEvent.GetInt();
                            onObjectVisibilityChanged( ToGalLayer( id ), isVisible, true );
                        } );

                wxStaticText* label = new wxStaticText( m_windowObjects, layer, aSetting->label );
                label->Wrap( -1 );
                label->SetToolTip( aSetting->tooltip );

                if( aSetting->can_control_opacity )
                {
                    label->SetMinSize( wxSize( labelWidth, -1 ) );
#ifdef __WXMAC__
                    sizer->Add( btn_visible, 0, wxALIGN_CENTER_VERTICAL | wxBOTTOM, 10 );
                    sizer->AddSpacer( 5 );
                    sizer->Add( label, 0, wxALIGN_CENTER_VERTICAL | wxBOTTOM, 10 );
#else
                    sizer->Add( btn_visible, 0, wxALIGN_CENTER_VERTICAL, 0 );
                    sizer->AddSpacer( 5 );
                    sizer->Add( label, 0, wxALIGN_CENTER_VERTICAL, 0 );
#endif

                    wxSlider* slider = new wxSlider( m_windowObjects, wxID_ANY, 100, 0, 100,
                                                     wxDefaultPosition, wxDefaultSize,
                                                     wxSL_HORIZONTAL );
#ifdef __WXMAC__
                    slider->SetMinSize( wxSize( 80, 22 ) );
#else
                    slider->SetMinSize( wxSize( 80, -1 ) );
#endif

                    tip.Printf( _( "Set opacity of %s" ), aSetting->label.Lower() );
                    slider->SetToolTip( tip );

                    sizer->Add( slider, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5 );
                    aSetting->ctl_opacity = slider;

                    auto opacitySliderHandler =
                            [=]( wxCommandEvent& aEvent )
                            {
                                wxSlider* ctrl = static_cast<wxSlider*>( aEvent.GetEventObject() );
                                int value = ctrl->GetValue();
                                onObjectOpacitySlider( layer, value / 100.0f );
                            };

                    slider->Bind( wxEVT_SCROLL_CHANGED, opacitySliderHandler );
                    slider->Bind( wxEVT_SCROLL_THUMBTRACK, opacitySliderHandler );
                }
                else
                {
                    sizer->Add( btn_visible, 0, wxALIGN_CENTER_VERTICAL, 0 );
                    sizer->AddSpacer( 5 );
                    sizer->Add( label, 0, wxALIGN_CENTER_VERTICAL, 0 );
                }

                aSetting->ctl_text = label;
                m_objectsSizer->Add( sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );
                m_objectsSizer->AddSpacer( 1 );
            };

    for( const APPEARANCE_SETTING& s_setting : s_objectSettings )
    {
        // TODO(JE)
#ifdef NOTYET
        if( m_fp_editor_mode && !isAllowedInFpMode( setting.id ) )
            continue;
#endif

        if( !s_setting.spacer )
        {
            m_objectSettings.emplace_back( std::make_unique<APPEARANCE_SETTING>( s_setting ) );

            std::unique_ptr<APPEARANCE_SETTING>& setting = m_objectSettings.back();

            // Because s_render_rows is created static, we must explicitly call
            // wxGetTranslation for texts which are internationalized (tool tips
            // and item names)
            setting->tooltip = wxGetTranslation( s_setting.tooltip );
            setting->label   = wxGetTranslation( s_setting.label );

            if( setting->can_control_opacity )
            {
                int width = m_windowObjects->GetTextExtent( setting->label ).x;
                labelWidth = std::max( labelWidth, width );
            }

            m_objectSettingsMap[ToGalLayer( setting->id )] = setting.get();
        }
    }

    for( const std::unique_ptr<APPEARANCE_SETTING>& setting : m_objectSettings )
    {
        if( setting->spacer )
            m_objectsSizer->AddSpacer( m_pointSize );
        else
            appendObject( setting );
    }

    m_objectsSizer->Layout();
}


void APPEARANCE_CONTROLS::syncObjectSettings()
{
    BOARD*  board   = m_frame->GetBoard();
    GAL_SET visible = board->GetVisibleElements();

    const PCB_DISPLAY_OPTIONS& opts = m_frame->GetDisplayOptions();

    for( std::unique_ptr<APPEARANCE_SETTING>& setting : m_objectSettings )
    {
        if( setting->spacer )
            continue;

        GAL_LAYER_ID layer = ToGalLayer( setting->id );

        if( setting->ctl_visibility )
            setting->ctl_visibility->SetValue( visible.Contains( layer ) );

        if( setting->ctl_color )
        {
            COLOR4D color = m_frame->GetColorSettings()->GetColor( setting->id );
            setting->ctl_color->SetSwatchColor( color, false );
        }
    }

    wxASSERT( m_objectSettingsMap.count( LAYER_TRACKS )
              && m_objectSettingsMap.count( LAYER_VIAS )
              && m_objectSettingsMap.count( LAYER_PADS )
              && m_objectSettingsMap.count( LAYER_ZONES ) );

    m_objectSettingsMap[LAYER_TRACKS]->ctl_opacity->SetValue( opts.m_TrackOpacity * 100 );
    m_objectSettingsMap[LAYER_VIAS]->ctl_opacity->SetValue( opts.m_ViaOpacity * 100 );
    m_objectSettingsMap[LAYER_PADS]->ctl_opacity->SetValue( opts.m_PadOpacity * 100 );
    m_objectSettingsMap[LAYER_ZONES]->ctl_opacity->SetValue( opts.m_ZoneOpacity * 100 );
}


void APPEARANCE_CONTROLS::rebuildNets()
{
    BOARD*          board   = m_frame->GetBoard();
    COLOR_SETTINGS* theme   = m_frame->GetColorSettings();
    COLOR4D         bgColor = theme->GetColor( LAYER_PCB_BACKGROUND );

    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings() );

    std::set<int>&                      hiddenNets     = rs->GetHiddenNets();
    std::map<int, KIGFX::COLOR4D>&      netColors      = rs->GetNetColorMap();
    std::map<wxString, KIGFX::COLOR4D>& netclassColors = rs->GetNetclassColorMap();

    m_netsOuterSizer->Clear( true );
    m_netclassOuterSizer->Clear( true );

    auto appendNet =
            [&]( NETINFO_ITEM* aNet )
            {
                int netCode = aNet->GetNet();
                int id      = netCode + wxID_HIGHEST;

                if( netCode == 0 )
                    return;

                m_netSettings.emplace_back( std::make_unique<APPEARANCE_SETTING>() );
                APPEARANCE_SETTING* setting = m_netSettings.back().get();
                m_netSettingsMap[netCode]   = setting;

                setting->ctl_panel = new wxPanel( m_netsScrolledWindow, id );
                wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
                setting->ctl_panel->SetSizer( sizer );

                COLOR4D color = netColors.count( netCode ) ? netColors.at( netCode ) :
                                                             COLOR4D::UNSPECIFIED;

                setting->ctl_color = new COLOR_SWATCH( setting->ctl_panel, color, id, bgColor,
                                                       COLOR4D::UNSPECIFIED, false );
                setting->ctl_color->SetToolTip( _( "Left double click or middle click for color "
                                                   "change, right click for menu" ) );

                if( color == COLOR4D::UNSPECIFIED )
                    setting->ctl_color->Hide();

                setting->ctl_color->Bind( COLOR_SWATCH_CHANGED,
                        [&]( wxCommandEvent& aEvent )
                        {
                            COLOR_SWATCH* s = static_cast<COLOR_SWATCH*>( aEvent.GetEventObject() );
                            int net = s->GetId();
                            net -= wxID_HIGHEST;

                            netColors[net] = s->GetSwatchColor();

                            if( s->GetSwatchColor() == COLOR4D::UNSPECIFIED )
                                s->Hide();

                            m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();

                            m_frame->GetCanvas()->RedrawRatsnest();
                            m_frame->GetCanvas()->Refresh();
                        } );

                bool visible = hiddenNets.count( netCode ) == 0;

                setting->ctl_visibility =
                        new BITMAP_TOGGLE( setting->ctl_panel, id, KiBitmap( visibility_xpm ),
                                           KiBitmap( visibility_off_xpm ), visible );

                wxString tip;
                tip.Printf( _( "Show or hide ratsnest for %s" ), aNet->GetShortNetname() );
                setting->ctl_visibility->SetToolTip( tip );

                setting->ctl_text = new wxStaticText( setting->ctl_panel, id,
                                                      aNet->GetShortNetname() );
                setting->ctl_text->Wrap( -1 );

                int flags = wxALIGN_CENTER_VERTICAL | wxRIGHT;

                sizer->Add( setting->ctl_color,      0, flags | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );
                sizer->Add( setting->ctl_visibility, 0, flags, 5 );
                sizer->Add( setting->ctl_text,       1, flags, 5 );

                m_netsOuterSizer->Add( setting->ctl_panel, 0, wxEXPAND, 0 );

                setting->ctl_visibility->Bind( TOGGLE_CHANGED,
                        [&]( wxCommandEvent& aEvent )
                        {
                            int net = static_cast<wxWindow*>( aEvent.GetEventObject() )->GetId();
                            net -= wxID_HIGHEST;
                            const TOOL_ACTION& action = aEvent.GetInt() ? PCB_ACTIONS::showNet :
                                                                          PCB_ACTIONS::hideNet;

                            m_frame->GetToolManager()->RunAction( action, true, net );
                        } );

                const wxString& netName = aNet->GetShortNetname();

                auto menuHandler =
                        [&, netCode, netName]( wxMouseEvent& aEvent )
                        {
                            m_contextMenuNetCode = netCode;

                            wxMenu menu;

                            menu.Append( new wxMenuItem( &menu, ID_SET_NET_COLOR,
                                         _( "Set net color" ), wxEmptyString, wxITEM_NORMAL ) );
                            menu.Append( new wxMenuItem( &menu, ID_HIGHLIGHT_NET,
                                         wxString::Format( _( "Highlight %s" ), netName ),
                                            wxEmptyString, wxITEM_NORMAL ) );
                            menu.Append( new wxMenuItem( &menu, ID_SELECT_NET,
                                         wxString::Format( _( "Select %s" ), netName ),
                                         wxEmptyString, wxITEM_NORMAL ) );

                            menu.AppendSeparator();

                            menu.Append( new wxMenuItem( &menu, ID_SHOW_ALL_NETS,
                                         _( "Show all nets" ), wxEmptyString, wxITEM_NORMAL ) );
                            menu.Append( new wxMenuItem( &menu, ID_HIDE_OTHER_NETS,
                                         _( "Hide all other nets" ), wxEmptyString,
                                         wxITEM_NORMAL ) );

                            menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
                                       &APPEARANCE_CONTROLS::onNetContextMenu, this );

                            PopupMenu( &menu );
                        };

                setting->ctl_panel->Bind( wxEVT_RIGHT_DOWN, menuHandler );
                setting->ctl_visibility->Bind( wxEVT_RIGHT_DOWN, menuHandler );
                setting->ctl_color->Bind( wxEVT_RIGHT_DOWN, menuHandler );
                setting->ctl_text->Bind( wxEVT_RIGHT_DOWN, menuHandler );
            };

    auto appendNetclass =
            [&]( int aId, const NETCLASSPTR& aClass, bool isDefault = false )
            {
                wxString name = aClass->GetName();

                m_netclassSettings.emplace_back( std::make_unique<APPEARANCE_SETTING>() );
                APPEARANCE_SETTING* setting = m_netclassSettings.back().get();
                m_netclassSettingsMap[name] = setting;

                setting->ctl_panel = new wxPanel( m_netclassScrolledWindow, aId );
                wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
                setting->ctl_panel->SetSizer( sizer );
                COLOR4D color = netclassColors.count( name ) ? netclassColors.at( name ) :
                                                                COLOR4D::UNSPECIFIED;

                setting->ctl_color = new COLOR_SWATCH( setting->ctl_panel, color, aId, bgColor,
                                                        COLOR4D::UNSPECIFIED, false );
                setting->ctl_color->SetToolTip( _( "Left double click or middle click for color "
                                                    "change, right click for menu" ) );

                if( !isDefault || color == COLOR4D::UNSPECIFIED )
                    setting->ctl_color->Hide();

                if( !isDefault )
                    setting->ctl_color->Bind( COLOR_SWATCH_CHANGED,
                                              &APPEARANCE_CONTROLS::onNetclassColorChanged, this );

                setting->ctl_visibility =
                        new BITMAP_TOGGLE( setting->ctl_panel, aId, KiBitmap( visibility_xpm ),
                                           KiBitmap( visibility_off_xpm ), true );

                wxString tip;
                tip.Printf( _( "Show or hide ratsnest for nets in %s" ), name );
                setting->ctl_visibility->SetToolTip( tip );

                setting->ctl_text = new wxStaticText( setting->ctl_panel, aId, name );
                setting->ctl_text->Wrap( -1 );

                int flags = wxALIGN_CENTER_VERTICAL | wxRIGHT;

                sizer->Add( setting->ctl_color,      0, flags | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );
                sizer->Add( setting->ctl_visibility, 0, flags, 5 );
                sizer->Add( setting->ctl_text,       1, flags, 5 );

                m_netclassOuterSizer->Add( setting->ctl_panel, 0, wxEXPAND, 0 );

                setting->ctl_visibility->Bind( TOGGLE_CHANGED,
                                               &APPEARANCE_CONTROLS::onNetclassVisibilityChanged,
                                               this );

                auto menuHandler =
                        [&, name, isDefault]( wxMouseEvent& aEvent )
                        {
                            m_contextMenuNetclass = name;

                            wxMenu menu;

                            if( !isDefault )
                            {
                                menu.Append( new wxMenuItem( &menu, ID_SET_NET_COLOR,
                                             _( "Set netclass color" ), wxEmptyString,
                                             wxITEM_NORMAL ) );
                            }

                            menu.Append( new wxMenuItem( &menu, ID_HIGHLIGHT_NET,
                                         wxString::Format( _( "Highlight nets in %s" ), name ),
                                                         wxEmptyString, wxITEM_NORMAL ) );
                            menu.Append( new wxMenuItem( &menu, ID_SELECT_NET,
                                         wxString::Format( _( "Select nets in %s" ), name ),
                                         wxEmptyString, wxITEM_NORMAL ) );

                            menu.AppendSeparator();

                            menu.Append( new wxMenuItem( &menu, ID_SHOW_ALL_NETS,
                                         _( "Show all netclasses" ), wxEmptyString, wxITEM_NORMAL ) );
                            menu.Append( new wxMenuItem( &menu, ID_HIDE_OTHER_NETS,
                                         _( "Hide all other netclasses" ), wxEmptyString,
                                         wxITEM_NORMAL ) );

                            menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
                                       &APPEARANCE_CONTROLS::onNetclassContextMenu, this );

                            PopupMenu( &menu );
                        };

                setting->ctl_panel->Bind( wxEVT_RIGHT_DOWN, menuHandler );
                setting->ctl_visibility->Bind( wxEVT_RIGHT_DOWN, menuHandler );

                if( !isDefault )
                    setting->ctl_color->Bind( wxEVT_RIGHT_DOWN, menuHandler );

                setting->ctl_text->Bind( wxEVT_RIGHT_DOWN, menuHandler );
            };

    const NETNAMES_MAP& nets = board->GetNetInfo().NetsByName();

    std::vector<wxString> names;

    for( const auto& pair : nets )
        names.emplace_back( pair.first );

    std::sort( names.begin(), names.end() );

    m_netSettings.clear();
    m_netSettingsMap.clear();

    for( const wxString& name : names )
        appendNet( nets.at( name ) );

    const NETCLASS_MAP& classes = board->GetDesignSettings().GetNetClasses().NetClasses();

    names.clear();

    for( const auto& pair : classes )
        names.emplace_back( pair.first );

    std::sort( names.begin(), names.end() );

    m_netclassIdMap.clear();

    int idx = wxID_HIGHEST + nets.size();

    NETCLASSPTR defaultClass = board->GetDesignSettings().GetNetClasses().GetDefault();

    m_netclassIdMap[idx] = defaultClass->GetName();
    appendNetclass( idx++, defaultClass, true );

    for( const wxString& name : names )
    {
        m_netclassIdMap[idx] = name;
        appendNetclass( idx++, classes.at( name ) );
    }

    m_netsOuterSizer->Layout();
    m_netclassOuterSizer->Layout();
    //m_netsTabSplitter->Layout();
}


void APPEARANCE_CONTROLS::rebuildLayerPresetsWidget()
{
    m_cbLayerPresets->Clear();

    for( std::pair<const wxString, LAYER_PRESET>& pair : m_layerPresets )
        m_cbLayerPresets->Append( pair.first, static_cast<void*>( &pair.second ) );

    m_cbLayerPresets->Append( wxT( "-----" ) );
    m_cbLayerPresets->Append( _( "Save new preset..." ) );
    m_cbLayerPresets->Append( _( "Delete preset..." ) );

    m_cbLayerPresets->SetSelection( 0 );

    // At least the build in presets should always be present
    wxASSERT( !m_layerPresets.empty() );

    // Default preset: all layers
    m_currentPreset = &m_layerPresets[presetAllLayers.name];
}


void APPEARANCE_CONTROLS::syncLayerPresetSelection()
{
    BOARD*  board          = m_frame->GetBoard();
    LSET    visibleLayers  = board->GetVisibleLayers();
    GAL_SET visibleObjects = board->GetVisibleElements();

    auto it = std::find_if( m_layerPresets.begin(), m_layerPresets.end(),
                            [&]( const std::pair<const wxString, LAYER_PRESET>& aPair )
                            {
                                return ( aPair.second.layers == visibleLayers
                                         && aPair.second.renderLayers == visibleObjects );
                            } );

    if( it != m_layerPresets.end() )
        m_cbLayerPresets->SetStringSelection( it->first  );
    else
        m_cbLayerPresets->SetSelection( m_cbLayerPresets->GetCount() - 3 ); // separator

    m_currentPreset = static_cast<LAYER_PRESET*>(
            m_cbLayerPresets->GetClientData( m_cbLayerPresets->GetSelection() ) );
}


void APPEARANCE_CONTROLS::updateLayerPresetSelection( const wxString& aName )
{
    int idx = m_cbLayerPresets->FindString( aName );

    if( idx >= 0 && m_cbLayerPresets->GetSelection() != idx )
    {
        m_cbLayerPresets->SetSelection( idx );
        m_currentPreset = static_cast<LAYER_PRESET*>( m_cbLayerPresets->GetClientData( idx ) );
    }
    else if( idx < 0 )
    {
        m_cbLayerPresets->SetSelection( m_cbLayerPresets->GetCount() - 3 ); // separator
    }
}


void APPEARANCE_CONTROLS::onLayerPresetChanged( wxCommandEvent& aEvent )
{
    BOARD* board = m_frame->GetBoard();

    int count = m_cbLayerPresets->GetCount();
    int index = m_cbLayerPresets->GetSelection();

    auto resetSelection =
            [&]()
            {
                if( m_currentPreset )
                    m_cbLayerPresets->SetStringSelection( m_currentPreset->name );
                else
                    m_cbLayerPresets->SetSelection( count - 3 );
            };

    if( index == count - 3 )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxTextEntryDialog dlg( this, _( "New layer preset name:" ), _( "Save Layer Preset" ) );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        wxString name = dlg.GetValue();

        if( m_layerPresets.count( name ) )
        {
            wxMessageBox( _( "Preset already exists!" ) );
            resetSelection();
            return;
        }

        m_layerPresets[name] = LAYER_PRESET( name, board->GetVisibleLayers(),
                                             board->GetVisibleElements(), UNSELECTED_LAYER );

        LAYER_PRESET* preset = &m_layerPresets[name];
        m_currentPreset      = preset;

        index = m_cbLayerPresets->Insert( name, index - 1, static_cast<void*>( preset ) );
        m_cbLayerPresets->SetSelection( index );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete a preset
        wxArrayString headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( std::pair<const wxString, LAYER_PRESET>& pair : m_layerPresets )
        {
            if( !pair.second.readOnly )
            {
                wxArrayString item;
                item.Add( pair.first );
                items.emplace_back( item );
            }
        }

        EDA_LIST_DIALOG dlg( m_frame, _( "Delete Preset" ), headers, items, wxEmptyString );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();

            m_layerPresets.erase( presetName );

            m_cbLayerPresets->Delete( m_cbLayerPresets->FindString( presetName ) );
            m_cbLayerPresets->SetSelection( m_cbLayerPresets->GetCount() - 3 );
            m_currentPreset = nullptr;
        }

        resetSelection();
        return;
    }

    LAYER_PRESET* preset = static_cast<LAYER_PRESET*>( m_cbLayerPresets->GetClientData( index ) );
    m_currentPreset      = preset;

    doApplyLayerPreset( *preset );
}


void APPEARANCE_CONTROLS::doApplyLayerPreset( const LAYER_PRESET& aPreset )
{
    BOARD* board = m_frame->GetBoard();

    board->SetVisibleLayers( aPreset.layers );
    board->SetVisibleElements( aPreset.renderLayers );

    // If the preset doesn't have an explicit active layer to restore, we can at least
    // force the active layer to be something in the preset's layer set
    if( aPreset.activeLayer != UNSELECTED_LAYER )
        m_frame->SetActiveLayer( aPreset.activeLayer );
    else if( aPreset.layers.any() && !aPreset.layers.test( m_frame->GetActiveLayer() ) )
        m_frame->SetActiveLayer( *aPreset.layers.Seq().begin() );

    m_frame->GetCanvas()->SyncLayersVisibility( board );
    m_frame->GetCanvas()->Refresh();

    syncColorsAndVisibility();
}


void APPEARANCE_CONTROLS::OnColorSwatchChanged( wxCommandEvent& aEvent )
{
    COLOR_SWATCH* swatch   = static_cast<COLOR_SWATCH*>( aEvent.GetEventObject() );
    COLOR4D       newColor = swatch->GetSwatchColor();
    LAYER_NUM     layer    = swatch->GetId();

    COLOR_SETTINGS* cs = m_frame->GetColorSettings();
    cs->SetColor( layer, newColor );

    m_frame->GetCanvas()->UpdateColors();

    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
    view->UpdateLayerColor( layer );
    view->UpdateLayerColor( GetNetnameLayer( layer ) );

    m_frame->ReCreateHToolbar();
    m_frame->GetCanvas()->Refresh();

    if( layer == LAYER_PCB_BACKGROUND )
        m_frame->SetDrawBgColor( newColor );
}


void APPEARANCE_CONTROLS::onObjectOpacitySlider( int aLayer, float aOpacity )
{
    PCB_DISPLAY_OPTIONS options = m_frame->GetDisplayOptions();

    switch( aLayer )
    {
    case static_cast<int>( LAYER_TRACKS ): options.m_TrackOpacity = aOpacity; break;
    case static_cast<int>( LAYER_VIAS ):   options.m_ViaOpacity = aOpacity;   break;
    case static_cast<int>( LAYER_PADS ):   options.m_PadOpacity = aOpacity;   break;
    case static_cast<int>( LAYER_ZONES ):  options.m_ZoneOpacity = aOpacity;  break;
    default: return;
    }

    m_frame->SetDisplayOptions( options );
}


void APPEARANCE_CONTROLS::onNetContextMenu( wxCommandEvent& aEvent )
{
    switch( aEvent.GetId() )
    {
    case ID_SET_NET_COLOR:
    {
        if( m_netSettingsMap.count( m_contextMenuNetCode ) )
        {
            APPEARANCE_SETTING* setting = m_netSettingsMap.at( m_contextMenuNetCode );
            setting->ctl_color->GetNewSwatchColor();

            COLOR4D color = setting->ctl_color->GetSwatchColor();

            setting->ctl_color->Show( color != COLOR4D::UNSPECIFIED );

            KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
                    m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings() );
            std::map<int, KIGFX::COLOR4D>& netColors = rs->GetNetColorMap();

            if( color != COLOR4D::UNSPECIFIED )
                netColors[m_contextMenuNetCode] = color;
            else
                netColors.erase( m_contextMenuNetCode );

            m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
            m_frame->GetCanvas()->Refresh();
        }

        break;
    }

    case ID_HIGHLIGHT_NET:
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::highlightNet, true,
                                              m_contextMenuNetCode );
        break;

    case ID_SELECT_NET:
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectNet, true,
                                              m_contextMenuNetCode );
        break;

    case ID_SHOW_ALL_NETS:
    {
        for( const std::pair<const int, APPEARANCE_SETTING*>& pair : m_netSettingsMap )
        {
            pair.second->ctl_visibility->SetValue( true );
            m_frame->GetToolManager()->RunAction( PCB_ACTIONS::showNet, true, pair.first );
        }

        break;
    }

    case ID_HIDE_OTHER_NETS:
    {
        TOOL_MANAGER* manager = m_frame->GetToolManager();

        for( const std::pair<const int, APPEARANCE_SETTING*>& pair : m_netSettingsMap )
        {
            bool show = pair.first == m_contextMenuNetCode;
            pair.second->ctl_visibility->SetValue( show );
            manager->RunAction( show ? PCB_ACTIONS::showNet : PCB_ACTIONS::hideNet,
                                true, pair.first );
        }

        break;
    }

    default:
        break;
    }

    m_frame->GetCanvas()->RedrawRatsnest();
    m_frame->GetCanvas()->Refresh();

    m_contextMenuNetCode = 0;
}


void APPEARANCE_CONTROLS::onNetclassVisibilityChanged( wxCommandEvent& aEvent )
{
    wxString className = netclassNameFromEvent( aEvent );
    bool     show      = aEvent.GetInt();
    showNetclass( className, show );
}


void APPEARANCE_CONTROLS::showNetclass( const wxString& aClassName, bool aShow )
{
    BOARD*        board    = m_frame->GetBoard();
    NETINFO_LIST& nets     = board->GetNetInfo();
    NETCLASSES&   classes  = board->GetDesignSettings().GetNetClasses();
    NETCLASSPTR   netclass = classes.Find( aClassName );
    TOOL_MANAGER* manager  = m_frame->GetToolManager();

    if( !netclass )
        return;

    NETCLASS* defaultClass = classes.GetDefaultPtr();

    auto updateWidget =
            [&]( int aCode )
            {
                if( m_netSettingsMap.count( aCode ) )
                {
                    APPEARANCE_SETTING* setting = m_netSettingsMap.at( aCode );
                    setting->ctl_visibility->SetValue( aShow );
                }
            };

    if( netclass == classes.GetDefault() )
    {
        const TOOL_ACTION& action = aShow ? PCB_ACTIONS::showNet : PCB_ACTIONS::hideNet;

        for( NETINFO_ITEM* net : nets )
        {
            if( net->GetNetClass() == defaultClass )
            {
                manager->RunAction( action, true, net->GetNet() );
                updateWidget( net->GetNet() );
            }
        }
    }
    else
    {
        const TOOL_ACTION& action = aShow ? PCB_ACTIONS::showNet : PCB_ACTIONS::hideNet;

        for( const wxString& member : *netclass )
        {
            int code = nets.GetNetItem( member )->GetNet();
            manager->RunAction( action, true, code );
            updateWidget( code );
        }
    }
}


void APPEARANCE_CONTROLS::onNetclassColorChanged( wxCommandEvent& aEvent )
{
    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings() );

    std::map<wxString, KIGFX::COLOR4D>& netclassColors = rs->GetNetclassColorMap();

    COLOR_SWATCH* swatch    = static_cast<COLOR_SWATCH*>( aEvent.GetEventObject() );
    wxString      className = netclassNameFromEvent( aEvent );

    netclassColors[className] = swatch->GetSwatchColor();

    if( swatch->GetSwatchColor() == COLOR4D::UNSPECIFIED )
        swatch->Hide();

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->RedrawRatsnest();
    m_frame->GetCanvas()->Refresh();
}


wxString APPEARANCE_CONTROLS::netclassNameFromEvent( wxEvent& aEvent )
{
    COLOR_SWATCH* s = static_cast<COLOR_SWATCH*>( aEvent.GetEventObject() );
    int classId = s->GetId();

    wxASSERT( m_netclassIdMap.count( classId ) );
    return m_netclassIdMap.at( classId );
}


void APPEARANCE_CONTROLS::onNetColorModeChanged( wxCommandEvent& aEvent )
{
    PCB_DISPLAY_OPTIONS options = m_frame->GetDisplayOptions();

    if( m_rbNetColorAll->GetValue() )
        options.m_NetColorMode = NET_COLOR_MODE::ALL;
    else if( m_rbNetColorRatsnest->GetValue() )
        options.m_NetColorMode = NET_COLOR_MODE::RATSNEST;
    else
        options.m_NetColorMode = NET_COLOR_MODE::OFF;

    m_frame->SetDisplayOptions( options );
    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
}


void APPEARANCE_CONTROLS::onNetclassContextMenu( wxCommandEvent& aEvent )
{
    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
    KIGFX::PCB_RENDER_SETTINGS* rs =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    BOARD*        board    = m_frame->GetBoard();
    NETINFO_LIST& nets     = board->GetNetInfo();
    NETCLASSES&   classes  = board->GetDesignSettings().GetNetClasses();
    NETCLASSPTR   netclass = classes.Find( m_contextMenuNetclass );

    APPEARANCE_SETTING* setting = m_netclassSettingsMap.count( m_contextMenuNetclass ) ?
                                  m_netclassSettingsMap.at( m_contextMenuNetclass ) : nullptr;

    switch( aEvent.GetId() )
    {
        case ID_SET_NET_COLOR:
        {
            if( setting )
            {
                setting->ctl_color->GetNewSwatchColor();

                COLOR4D color = setting->ctl_color->GetSwatchColor();

                setting->ctl_color->Show( color != COLOR4D::UNSPECIFIED );

                std::map<wxString, KIGFX::COLOR4D>& netclassColors = rs->GetNetclassColorMap();

                if( color != COLOR4D::UNSPECIFIED )
                    netclassColors[m_contextMenuNetclass] = color;
                else
                    netclassColors.erase( m_contextMenuNetclass );

                view->UpdateAllLayersColor();
            }

            break;
        }

        case ID_HIGHLIGHT_NET:
        {
            if( netclass )
            {
                bool first = true;

                for( const wxString& member : *netclass )
                {
                    if( NETINFO_ITEM* net = nets.GetNetItem( member ) )
                    {
                        int code = net->GetNet();

                        if( first )
                        {
                            board->SetHighLightNet( code );
                            rs->SetHighlight( true, code );
                            first = false;
                        }
                        else
                        {
                            board->SetHighLightNet( code, true );
                            rs->SetHighlight( true, code, true );
                        }
                    }
                }

                view->UpdateAllLayersColor();
                board->HighLightON();
            }
            break;
        }

        case ID_SELECT_NET:
        {
            if( netclass )
            {
                for( const wxString& member : *netclass )
                {
                    if( NETINFO_ITEM* net = nets.GetNetItem( member ) )
                    {
                        int code = net->GetNet();
                        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectNet, true, code );
                    }
                }
            }
            break;
        }

        case ID_SHOW_ALL_NETS:
        {
            for( const auto& pair : classes.NetClasses() )
            {
                showNetclass( pair.first );

                if( m_netclassSettingsMap.count( pair.first ) )
                    m_netclassSettingsMap.at( pair.first )->ctl_visibility->SetValue( true );
            }

            break;
        }

        case ID_HIDE_OTHER_NETS:
        {
            for( const auto& pair : classes.NetClasses() )
            {
                bool show = pair.first == m_contextMenuNetclass;

                showNetclass( pair.first, show );

                if( m_netclassSettingsMap.count( pair.first ) )
                    m_netclassSettingsMap.at( pair.first )->ctl_visibility->SetValue( show );
            }

            break;
        }

        default:
            break;
    }

    m_frame->GetCanvas()->RedrawRatsnest();
    m_frame->GetCanvas()->Refresh();

    m_contextMenuNetclass.clear();
}
