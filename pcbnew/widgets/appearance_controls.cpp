/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#include <widgets/appearance_controls.h>

#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <pad.h>
#include <pcb_track.h>
#include <eda_list_dialog.h>
#include <string_utils.h>
#include <footprint_edit_frame.h>
#include <confirm.h>
#include <pcb_display_options.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <footprint_editor_settings.h>
#include <project.h>
#include <project/project_local_settings.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <widgets/bitmap_button.h>
#include <widgets/bitmap_toggle.h>
#include <widgets/wx_collapsible_pane.h>
#include <widgets/color_swatch.h>
#include <widgets/grid_bitmap_toggle.h>
#include <widgets/grid_color_swatch_helpers.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/indicator_icon.h>
#include <widgets/wx_infobar.h>
#include <widgets/wx_grid.h>
#include <dialogs/eda_view_switcher.h>
#include <wx/checkbox.h>
#include <wx/hyperlink.h>
#include <wx/msgdlg.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/statline.h>
#include <wx/textdlg.h>
#include <wx/bmpbuttn.h>        // needed on wxMSW for OnSetFocus()
#include <core/profile.h>
#include <pgm_base.h>


NET_GRID_TABLE::NET_GRID_TABLE( PCB_BASE_FRAME* aFrame, wxColor aBackgroundColor ) :
        wxGridTableBase(),
        m_frame( aFrame )
{
    m_defaultAttr = new wxGridCellAttr;
    m_defaultAttr->SetBackgroundColour( aBackgroundColor );

    m_labelAttr = new wxGridCellAttr;
    m_labelAttr->SetRenderer( new GRID_CELL_ESCAPED_TEXT_RENDERER );
    m_labelAttr->SetBackgroundColour( aBackgroundColor );
}


NET_GRID_TABLE::~NET_GRID_TABLE()
{
    m_defaultAttr->DecRef();
    m_labelAttr->DecRef();
}


wxGridCellAttr* NET_GRID_TABLE::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind )
{
    wxGridCellAttr* attr = nullptr;

    switch( aCol )
    {
    case COL_COLOR:      attr = m_defaultAttr; break;
    case COL_VISIBILITY: attr = m_defaultAttr; break;
    case COL_LABEL:      attr = m_labelAttr;   break;
    default:             wxFAIL;
    }

    if( attr )
        attr->IncRef();

    return attr;
}


wxString NET_GRID_TABLE::GetValue( int aRow, int aCol )
{
    wxASSERT( static_cast<size_t>( aRow ) < m_nets.size() );

    switch( aCol )
    {
    case COL_COLOR:      return m_nets[aRow].color.ToCSSString();
    case COL_VISIBILITY: return m_nets[aRow].visible ? wxT( "1" ) : wxT( "0" );
    case COL_LABEL:      return m_nets[aRow].name;
    default:             return wxEmptyString;
    }
}


void NET_GRID_TABLE::SetValue( int aRow, int aCol, const wxString& aValue )
{
    wxASSERT( static_cast<size_t>( aRow ) < m_nets.size() );

    NET_GRID_ENTRY& net = m_nets[aRow];

    switch( aCol )
    {
    case COL_COLOR:
        net.color.SetFromWxString( aValue );
        updateNetColor( net );
        break;

    case COL_VISIBILITY:
        net.visible = ( aValue != wxT( "0" ) );
        updateNetVisibility( net );
        break;

    case COL_LABEL:
        net.name = aValue;
        break;

    default:
        break;
    }
}


wxString NET_GRID_TABLE::GetTypeName( int aRow, int aCol )
{
    switch( aCol )
    {
    case COL_COLOR:      return wxT( "COLOR4D" );
    case COL_VISIBILITY: return wxGRID_VALUE_BOOL;
    case COL_LABEL:      return wxGRID_VALUE_STRING;
    default:             return wxGRID_VALUE_STRING;
    }
}


bool NET_GRID_TABLE::GetValueAsBool( int aRow, int aCol )
{
    wxASSERT( static_cast<size_t>( aRow ) < m_nets.size() );
    wxASSERT( aCol == COL_VISIBILITY );

    return m_nets[aRow].visible;
}


void NET_GRID_TABLE::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    wxASSERT( static_cast<size_t>( aRow ) < m_nets.size() );
    wxASSERT( aCol == COL_VISIBILITY );

    m_nets[aRow].visible = aValue;
    updateNetVisibility( m_nets[aRow] );
}


void* NET_GRID_TABLE::GetValueAsCustom( int aRow, int aCol, const wxString& aTypeName )
{
    wxASSERT( aCol == COL_COLOR );
    wxASSERT( aTypeName == wxT( "COLOR4D" ) );
    wxASSERT( static_cast<size_t>( aRow ) < m_nets.size() );

    return ColorToVoid( m_nets[aRow].color );
}


void NET_GRID_TABLE::SetValueAsCustom( int aRow, int aCol, const wxString& aTypeName, void* aValue )
{
    wxASSERT( aCol == COL_COLOR );
    wxASSERT( aTypeName == wxT( "COLOR4D" ) );
    wxASSERT( static_cast<size_t>( aRow ) < m_nets.size() );

    m_nets[aRow].color = VoidToColor( aValue );
    updateNetColor( m_nets[aRow] );
}


NET_GRID_ENTRY& NET_GRID_TABLE::GetEntry( int aRow )
{
    wxASSERT( static_cast<size_t>( aRow ) < m_nets.size() );
    return m_nets[aRow];
}


int NET_GRID_TABLE::GetRowByNetcode( int aCode ) const
{
    auto it = std::find_if( m_nets.cbegin(), m_nets.cend(),
            [aCode]( const NET_GRID_ENTRY& aEntry )
            {
                return aEntry.code == aCode;
            } );

    if( it == m_nets.cend() )
        return -1;

    return std::distance( m_nets.cbegin(), it );
}


void NET_GRID_TABLE::Rebuild()
{
    BOARD*                      board = m_frame->GetBoard();
    const NETNAMES_MAP&         nets  = board->GetNetInfo().NetsByName();
    KIGFX::RENDER_SETTINGS*     renderSettings = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( renderSettings );

    std::set<int>&                 hiddenNets = rs->GetHiddenNets();
    std::map<int, KIGFX::COLOR4D>& netColors  = rs->GetNetColorMap();

    int deleted = (int) m_nets.size();
    m_nets.clear();

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, deleted );
        GetView()->ProcessTableMessage( msg );
    }

    for( const std::pair<const wxString, NETINFO_ITEM*>& pair : nets )
    {
        int netCode = pair.second->GetNetCode();

        if( netCode > 0 && !pair.first.StartsWith( wxT( "unconnected-(" ) ) )
        {
            COLOR4D color = netColors.count( netCode ) ? netColors.at( netCode )
                                                       : COLOR4D::UNSPECIFIED;

            bool visible = hiddenNets.count( netCode ) == 0;

            m_nets.emplace_back( NET_GRID_ENTRY( netCode, pair.first, color, visible ) );
        }
    }

    // TODO(JE) move to ::Compare so we can re-sort easily
    std::sort( m_nets.begin(), m_nets.end(),
               []( const NET_GRID_ENTRY& a, const NET_GRID_ENTRY& b )
               {
                 return a.name < b.name;
               } );

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, (int) m_nets.size() );
        GetView()->ProcessTableMessage( msg );
    }
}


void NET_GRID_TABLE::ShowAllNets()
{
    for( NET_GRID_ENTRY& net : m_nets )
    {
        net.visible = true;
        updateNetVisibility( net );
    }

    if( GetView() )
        GetView()->ForceRefresh();
}


void NET_GRID_TABLE::HideOtherNets( const NET_GRID_ENTRY& aNet )
{
    for( NET_GRID_ENTRY& net : m_nets )
    {
        net.visible = ( net.code == aNet.code );
        updateNetVisibility( net );
    }

    if( GetView() )
        GetView()->ForceRefresh();
}


void NET_GRID_TABLE::updateNetVisibility( const NET_GRID_ENTRY& aNet )
{
    const TOOL_ACTION& action = aNet.visible ? PCB_ACTIONS::showNetInRatsnest
                                             : PCB_ACTIONS::hideNetInRatsnest;

    m_frame->GetToolManager()->RunAction( action, aNet.code );
}


void NET_GRID_TABLE::updateNetColor( const NET_GRID_ENTRY& aNet )
{
    KIGFX::RENDER_SETTINGS*     rs = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    KIGFX::PCB_RENDER_SETTINGS* renderSettings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( rs );

    std::map<int, KIGFX::COLOR4D>& netColors  = renderSettings->GetNetColorMap();

    if( aNet.color != COLOR4D::UNSPECIFIED )
        netColors[aNet.code] = aNet.color;
    else
        netColors.erase( aNet.code );

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->RedrawRatsnest();
    m_frame->GetCanvas()->Refresh();
}


/// Template for object appearance settings
const APPEARANCE_CONTROLS::APPEARANCE_SETTING APPEARANCE_CONTROLS::s_objectSettings[] = {

#define RR  APPEARANCE_CONTROLS::APPEARANCE_SETTING   // Render Row abbreviation to reduce source width

    // clang-format off

    //        text                           id                            tooltip                opacity slider   visibility checkbox
    RR( _HKI( "Tracks" ),               LAYER_TRACKS,             _HKI( "Show tracks" ),              true ),
    RR( _HKI( "Vias" ),                 LAYER_VIAS,               _HKI( "Show all vias" ),            true ),
    RR( _HKI( "Pads" ),                 LAYER_PADS,               _HKI( "Show all pads" ),            true ),
    RR( _HKI( "Zones" ),                LAYER_ZONES,              _HKI( "Show copper zones" ),        true ),
    RR( _HKI( "Filled Shapes" ),        LAYER_FILLED_SHAPES,      _HKI( "Opacity of filled shapes" ), true,             false ),
    RR( _HKI( "Images" ),               LAYER_DRAW_BITMAPS,       _HKI( "Show user images" ),         true ),
    RR(),
    RR( _HKI( "Footprints Front" ),     LAYER_FOOTPRINTS_FR,      _HKI( "Show footprints that are on board's front" ) ),
    RR( _HKI( "Footprints Back" ),      LAYER_FOOTPRINTS_BK,      _HKI( "Show footprints that are on board's back" ) ),
    RR( _HKI( "Values" ),               LAYER_FP_VALUES,          _HKI( "Show footprint values" ) ),
    RR( _HKI( "References" ),           LAYER_FP_REFERENCES,      _HKI( "Show footprint references" ) ),
    RR( _HKI( "Footprint Text" ),       LAYER_FP_TEXT,            _HKI( "Show all footprint text" ) ),
    RR(),
    RR(),
    RR( _HKI( "Ratsnest" ),             LAYER_RATSNEST,           _HKI( "Show unconnected nets as a ratsnest") ),
    RR( _HKI( "DRC Warnings" ),         LAYER_DRC_WARNING,        _HKI( "DRC violations with a Warning severity" ) ),
    RR( _HKI( "DRC Errors" ),           LAYER_DRC_ERROR,          _HKI( "DRC violations with an Error severity" ) ),
    RR( _HKI( "DRC Exclusions" ),       LAYER_DRC_EXCLUSION,      _HKI( "DRC violations which have been individually excluded" ) ),
    RR( _HKI( "Anchors" ),              LAYER_ANCHOR,             _HKI( "Show footprint and text origins as a cross" ) ),
    RR( _HKI( "Points" ),               LAYER_POINTS,             _HKI( "Show explicit snap points as crosses" ) ),
    RR( _HKI( "Locked Item Shadow" ),   LAYER_LOCKED_ITEM_SHADOW, _HKI( "Show a shadow on locked items" ) ),
    RR( _HKI( "Colliding Courtyards" ), LAYER_CONFLICTS_SHADOW,   _HKI( "Show colliding footprint courtyards" ) ),
    RR( _HKI( "Board Area Shadow" ),    LAYER_BOARD_OUTLINE_AREA, _HKI( "Show board area shadow" ) ),
    RR( _HKI( "Drawing Sheet" ),        LAYER_DRAWINGSHEET,       _HKI( "Show drawing sheet borders and title block" ) ),
    RR( _HKI( "Grid" ),                 LAYER_GRID,               _HKI( "Show the (x,y) grid dots" ) )
    // clang-format on
};

/// These GAL layers are shown in the Objects tab in the footprint editor
static std::set<int> s_allowedInFpEditor =
        {
            LAYER_TRACKS,
            LAYER_VIAS,
            LAYER_PADS,
            LAYER_ZONES,
            LAYER_FILLED_SHAPES,
            LAYER_FP_VALUES,
            LAYER_FP_REFERENCES,
            LAYER_FP_TEXT,
            LAYER_DRAW_BITMAPS,
            LAYER_GRID,
            LAYER_POINTS,
        };

// These are the built-in layer presets that cannot be deleted

LAYER_PRESET APPEARANCE_CONTROLS::presetNoLayers( _HKI( "No Layers" ), LSET(), false );

LAYER_PRESET APPEARANCE_CONTROLS::presetAllLayers( _HKI( "All Layers" ),
        LSET::AllLayersMask(), false );

LAYER_PRESET APPEARANCE_CONTROLS::presetAllCopper( _HKI( "All Copper Layers" ),
        LSET( LSET::AllCuMask() ).set( Edge_Cuts ), false );

LAYER_PRESET APPEARANCE_CONTROLS::presetInnerCopper( _HKI( "Inner Copper Layers" ),
        LSET( LSET::InternalCuMask() ).set( Edge_Cuts ), false );

LAYER_PRESET APPEARANCE_CONTROLS::presetFront( _HKI( "Front Layers" ),
        LSET( LSET::FrontMask() ).set( Edge_Cuts ), false );

LAYER_PRESET APPEARANCE_CONTROLS::presetFrontAssembly( _HKI( "Front Assembly View" ),
        LSET( LSET::FrontAssembly() ).set( Edge_Cuts ), GAL_SET::DefaultVisible(), F_SilkS, false );

LAYER_PRESET APPEARANCE_CONTROLS::presetBack( _HKI( "Back Layers" ),
        LSET( LSET::BackMask() ).set( Edge_Cuts ), true );

LAYER_PRESET APPEARANCE_CONTROLS::presetBackAssembly( _HKI( "Back Assembly View" ),
        LSET( LSET::BackAssembly() ).set( Edge_Cuts ), GAL_SET::DefaultVisible(), B_SilkS, true );

// this one is only used to store the object visibility settings  of the last used
// built-in layer preset
LAYER_PRESET APPEARANCE_CONTROLS::m_lastBuiltinPreset;


APPEARANCE_CONTROLS::APPEARANCE_CONTROLS( PCB_BASE_FRAME* aParent, wxWindow* aFocusOwner, bool aFpEditorMode ) :
        APPEARANCE_CONTROLS_BASE( aParent ),
        m_frame( aParent ),
        m_focusOwner( aFocusOwner ),
        m_board( nullptr ),
        m_isFpEditor( aFpEditorMode ),
        m_currentPreset( nullptr ),
        m_lastSelectedUserPreset( nullptr ),
        m_layerContextMenu( nullptr ),
        m_togglingNetclassRatsnestVisibility( false )
{
    // Correct the min size from wxformbuilder not using fromdip
    SetMinSize( FromDIP( GetMinSize() ) );

    // We pregenerate the visibility bundles to reuse to reduce gdi exhaustion on windows
    // We can get a crazy amount of nets and netclasses
    m_visibleBitmapBundle = KiBitmapBundle( BITMAPS::visibility );
    m_notVisibileBitmapBundle = KiBitmapBundle( BITMAPS::visibility_off );

    int screenHeight   = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );
    m_iconProvider     = new ROW_ICON_PROVIDER( KIUI::c_IndicatorSizeDIP, this );
    m_pointSize        = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ).GetPointSize();
    m_layerPanelColour = m_panelLayers->GetBackgroundColour().ChangeLightness( 110 );
    SetBorders( true, false, false, false );

    m_layersOuterSizer = new wxBoxSizer( wxVERTICAL );
    m_windowLayers->SetSizer( m_layersOuterSizer );
    m_windowLayers->SetScrollRate( 0, 5 );
    m_windowLayers->Bind( wxEVT_SET_FOCUS, &APPEARANCE_CONTROLS::OnSetFocus, this );

    m_objectsOuterSizer = new wxBoxSizer( wxVERTICAL );
    m_windowObjects->SetSizer( m_objectsOuterSizer );
    m_windowObjects->SetScrollRate( 0, 5 );
    m_windowObjects->Bind( wxEVT_SET_FOCUS, &APPEARANCE_CONTROLS::OnSetFocus, this );

    wxFont infoFont = KIUI::GetInfoFont( this );
    m_staticTextNets->SetFont( infoFont );
    m_staticTextNetClasses->SetFont( infoFont );
    m_panelLayers->SetFont( infoFont );
    m_windowLayers->SetFont( infoFont );
    m_windowObjects->SetFont( infoFont );
    m_presetsLabel->SetFont( infoFont );
    m_viewportsLabel->SetFont( infoFont );

    m_cbLayerPresets->SetToolTip( wxString::Format( _( "Save and restore layer visibility combinations.\n"
                                                       "Use %s+Tab to activate selector.\n"
                                                       "Successive Tabs while holding %s down will "
                                                       "cycle through presets in the popup." ),
                                                    KeyNameFromKeyCode( PRESET_SWITCH_KEY ),
                                                    KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) );

    m_cbViewports->SetToolTip( wxString::Format( _( "Save and restore view location and zoom.\n"
                                                    "Use %s+Tab to activate selector.\n"
                                                    "Successive Tabs while holding %s down will "
                                                    "cycle through viewports in the popup." ),
                                                 KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ),
                                                 KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ) ) );

    createControls();

    m_btnNetInspector->SetBitmap( KiBitmapBundle( BITMAPS::list_nets_16 ) );
    m_btnNetInspector->SetPadding( 2 );

    m_btnConfigureNetClasses->SetBitmap( KiBitmapBundle( BITMAPS::options_generic_16 ) );
    m_btnConfigureNetClasses->SetPadding( 2 );

    m_txtNetFilter->SetHint( _( "Filter nets" ) );

    if( screenHeight <= 900 && m_pointSize >= FromDIP( KIUI::c_IndicatorSizeDIP ) )
        m_pointSize = m_pointSize * 8 / 10;

    wxFont font = m_notebook->GetFont();

#ifdef __WXMAC__
    font.SetPointSize( m_pointSize );
    m_notebook->SetFont( font );
#endif

    auto setHighContrastMode =
            [&]( HIGH_CONTRAST_MODE aMode )
            {
                PCB_DISPLAY_OPTIONS opts   = m_frame->GetDisplayOptions();
                opts.m_ContrastModeDisplay = aMode;

                m_frame->SetDisplayOptions( opts );
                passOnFocus();
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
                m_frame->GetToolManager()->RunAction( PCB_ACTIONS::showNetInspector );
            } );

    m_btnConfigureNetClasses->Bind( wxEVT_BUTTON,
            [&]( wxCommandEvent& aEvent )
            {
                // This panel should only be visible in the PCB_EDIT_FRAME anyway
                if( PCB_EDIT_FRAME* editframe = dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) )
                    editframe->ShowBoardSetupDialog( _( "Net Classes" ) );

                passOnFocus();
            } );

    m_cbFlipBoard->SetValue( m_frame->GetCanvas()->GetView()->IsMirroredX() );
    m_cbFlipBoard->Bind( wxEVT_CHECKBOX,
            [&]( wxCommandEvent& aEvent )
            {
                m_frame->GetToolManager()->RunAction( PCB_ACTIONS::flipBoard );
                syncLayerPresetSelection();
            } );

    m_toggleGridRenderer = new GRID_BITMAP_TOGGLE_RENDERER( m_visibleBitmapBundle,
                                                            m_notVisibileBitmapBundle );

    m_netsGrid->RegisterDataType( wxT( "bool" ), m_toggleGridRenderer, new wxGridCellBoolEditor );

    m_netsGrid->RegisterDataType( wxT( "COLOR4D" ),
                                  new GRID_CELL_COLOR_RENDERER( m_frame, SWATCH_SMALL ),
                                  new GRID_CELL_COLOR_SELECTOR( m_frame, m_netsGrid ) );

    m_netsTable = new NET_GRID_TABLE( m_frame, m_panelNets->GetBackgroundColour() );
    m_netsGrid->SetTable( m_netsTable, true );
    m_netsGrid->SetColLabelSize( 0 );

    m_netsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_netsGrid->SetSelectionForeground( m_netsGrid->GetDefaultCellTextColour() );
    m_netsGrid->SetSelectionBackground( m_panelNets->GetBackgroundColour() );

    const int cellPadding      = 6;
#ifdef __WXMAC__
    const int rowHeightPadding = 5;
#else
    const int rowHeightPadding = 3;
#endif

    wxSize size = ConvertDialogToPixels( SWATCH_SIZE_SMALL_DU );
    m_netsGrid->SetColSize( NET_GRID_TABLE::COL_COLOR, size.x + cellPadding );

    size = m_visibleBitmapBundle.GetPreferredBitmapSizeFor( this );
    m_netsGrid->SetColSize( NET_GRID_TABLE::COL_VISIBILITY, size.x + cellPadding );

    m_netsGrid->SetDefaultCellFont( font );
    m_netsGrid->SetDefaultRowSize( font.GetPixelSize().y + rowHeightPadding );

    m_netsGrid->GetGridWindow()->Bind( wxEVT_MOTION, &APPEARANCE_CONTROLS::OnNetGridMouseEvent, this );

    // To handle middle click on color swatches
    m_netsGrid->GetGridWindow()->Bind( wxEVT_MIDDLE_UP, &APPEARANCE_CONTROLS::OnNetGridMouseEvent, this );

    m_netsGrid->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT );
    m_netclassScrolledWindow->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT );

    if( m_isFpEditor )
        m_notebook->RemovePage( 2 );

    if( PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings() )
    {
        if( cfg->m_AuiPanels.appearance_expand_layer_display )
            m_paneLayerDisplayOptions->Expand();

        if( cfg->m_AuiPanels.appearance_expand_net_display )
            m_paneNetDisplayOptions->Expand();
    }

    loadDefaultLayerPresets();
    rebuildLayerPresetsWidget( true );
    rebuildObjects();
    OnBoardChanged();

    // Grid visibility is loaded and set to the GAL before we are constructed
    SetObjectVisible( LAYER_GRID, m_frame->IsGridVisible() );

    Bind( wxEVT_COMMAND_MENU_SELECTED, &APPEARANCE_CONTROLS::OnLayerContextMenu, this,
          ID_CHANGE_COLOR, ID_LAST_VALUE );

    m_frame->Bind( EDA_LANG_CHANGED, &APPEARANCE_CONTROLS::OnLanguageChanged, this );
}


APPEARANCE_CONTROLS::~APPEARANCE_CONTROLS()
{
    m_frame->Unbind( EDA_LANG_CHANGED, &APPEARANCE_CONTROLS::OnLanguageChanged, this );

    delete m_iconProvider;
}


void APPEARANCE_CONTROLS::createControls()
{
    int      hotkey;
    wxString msg;
    wxFont   infoFont = KIUI::GetInfoFont( this );

    // Create layer display options
    m_paneLayerDisplayOptions = new WX_COLLAPSIBLE_PANE( m_panelLayers, wxID_ANY,
                                                         _( "Layer Display Options" ) );
    m_paneLayerDisplayOptions->Collapse();
    m_paneLayerDisplayOptions->SetBackgroundColour( m_notebook->GetThemeBackgroundColour() );

    wxWindow* layerDisplayPane = m_paneLayerDisplayOptions->GetPane();

    wxBoxSizer* layerDisplayOptionsSizer;
    layerDisplayOptionsSizer = new wxBoxSizer( wxVERTICAL );

    hotkey = PCB_ACTIONS::highContrastModeCycle.GetHotKey();

    if( hotkey )
        msg = wxString::Format( _( "Inactive layers (%s):" ), KeyNameFromKeyCode( hotkey ) );
    else
        msg = _( "Inactive layers:" );

    m_inactiveLayersLabel = new wxStaticText( layerDisplayPane, wxID_ANY, msg );
    m_inactiveLayersLabel->SetFont( infoFont );
    m_inactiveLayersLabel->Wrap( -1 );
    layerDisplayOptionsSizer->Add( m_inactiveLayersLabel, 0, wxEXPAND | wxBOTTOM, 2 );

    wxBoxSizer* contrastModeSizer;
    contrastModeSizer = new wxBoxSizer( wxHORIZONTAL );

    m_rbHighContrastNormal = new wxRadioButton( layerDisplayPane, wxID_ANY, _( "Normal" ),
                                                wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_rbHighContrastNormal->SetFont( infoFont );
    m_rbHighContrastNormal->SetValue( true );
    m_rbHighContrastNormal->SetToolTip( _( "Inactive layers will be shown in full color" ) );

    contrastModeSizer->Add( m_rbHighContrastNormal, 0, wxRIGHT, 5 );
    contrastModeSizer->AddStretchSpacer();

    m_rbHighContrastDim = new wxRadioButton( layerDisplayPane, wxID_ANY, _( "Dim" ) );
    m_rbHighContrastDim->SetFont( infoFont );
    m_rbHighContrastDim->SetToolTip( _( "Inactive layers will be dimmed" ) );

    contrastModeSizer->Add( m_rbHighContrastDim, 0, wxRIGHT, 5 );
    contrastModeSizer->AddStretchSpacer();

    m_rbHighContrastOff = new wxRadioButton( layerDisplayPane, wxID_ANY, _( "Hide" ) );
    m_rbHighContrastOff->SetFont( infoFont );
    m_rbHighContrastOff->SetToolTip( _( "Inactive layers will be hidden" ) );

    contrastModeSizer->Add( m_rbHighContrastOff, 0, 0, 5 );
    contrastModeSizer->AddStretchSpacer();

    layerDisplayOptionsSizer->Add( contrastModeSizer, 0, wxEXPAND, 5 );

    m_layerDisplaySeparator = new wxStaticLine( layerDisplayPane, wxID_ANY, wxDefaultPosition,
                                                wxDefaultSize, wxLI_HORIZONTAL );
    layerDisplayOptionsSizer->Add( m_layerDisplaySeparator, 0, wxEXPAND | wxTOP, 4 );

    m_cbFlipBoard = new wxCheckBox( layerDisplayPane, wxID_ANY, _( "Flip board view" ) );
    m_cbFlipBoard->SetFont( infoFont );
    layerDisplayOptionsSizer->Add( m_cbFlipBoard, 0, wxTOP | wxBOTTOM, 3 );

    layerDisplayPane->SetSizer( layerDisplayOptionsSizer );
    layerDisplayPane->Layout();
    layerDisplayOptionsSizer->Fit( layerDisplayPane );

    m_panelLayersSizer->Add( m_paneLayerDisplayOptions, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 5 );

    m_paneLayerDisplayOptions->Bind( WX_COLLAPSIBLE_PANE_CHANGED,
                                     [&]( wxCommandEvent& aEvent )
                                     {
                                         Freeze();
                                         m_panelLayers->Fit();
                                         m_sizerOuter->Layout();
                                         Thaw();
                                     } );

    // Create net display options

    m_paneNetDisplayOptions = new WX_COLLAPSIBLE_PANE( m_panelNetsAndClasses, wxID_ANY,
                                                       _( "Net Display Options" ) );
    m_paneNetDisplayOptions->Collapse();
    m_paneNetDisplayOptions->SetBackgroundColour( m_notebook->GetThemeBackgroundColour() );

    wxWindow* netDisplayPane = m_paneNetDisplayOptions->GetPane();
    wxBoxSizer* netDisplayOptionsSizer = new wxBoxSizer( wxVERTICAL );

    //// Net color mode

    hotkey = PCB_ACTIONS::netColorModeCycle.GetHotKey();

    if( hotkey )
        msg = wxString::Format( _( "Net colors (%s):" ), KeyNameFromKeyCode( hotkey ) );
    else
        msg = _( "Net colors:" );

    m_txtNetDisplayTitle = new wxStaticText( netDisplayPane, wxID_ANY, msg );
    m_txtNetDisplayTitle->SetFont( infoFont );
    m_txtNetDisplayTitle->Wrap( -1 );
    m_txtNetDisplayTitle->SetToolTip( _( "Choose when to show net and netclass colors" ) );

    netDisplayOptionsSizer->Add( m_txtNetDisplayTitle, 0, wxEXPAND | wxBOTTOM | wxLEFT, 2 );

    wxBoxSizer* netColorSizer = new wxBoxSizer( wxHORIZONTAL );

    m_rbNetColorAll = new wxRadioButton( netDisplayPane, wxID_ANY, _( "All" ), wxDefaultPosition,
                                         wxDefaultSize, wxRB_GROUP );
    m_rbNetColorAll->SetFont( infoFont );
    m_rbNetColorAll->SetToolTip( _( "Net and netclass colors are shown on all copper items" ) );

    netColorSizer->Add( m_rbNetColorAll, 0, wxRIGHT, 5 );
    netColorSizer->AddStretchSpacer();

    m_rbNetColorRatsnest = new wxRadioButton( netDisplayPane, wxID_ANY, _( "Ratsnest" ) );
    m_rbNetColorRatsnest->SetFont( infoFont );
    m_rbNetColorRatsnest->SetValue( true );
    m_rbNetColorRatsnest->SetToolTip( _( "Net and netclass colors are shown on the ratsnest only" ) );

    netColorSizer->Add( m_rbNetColorRatsnest, 0, wxRIGHT, 5 );
    netColorSizer->AddStretchSpacer();

    m_rbNetColorOff = new wxRadioButton( netDisplayPane, wxID_ANY, _( "None" ) );
    m_rbNetColorOff->SetFont( infoFont );
    m_rbNetColorOff->SetToolTip( _( "Net and netclass colors are not shown" ) );

    netColorSizer->Add( m_rbNetColorOff, 0, 0, 5 );

    netDisplayOptionsSizer->Add( netColorSizer, 0, wxEXPAND | wxBOTTOM, 5 );

    //// Ratsnest display

    hotkey = PCB_ACTIONS::ratsnestModeCycle.GetHotKey();

    if( hotkey )
        msg = wxString::Format( _( "Ratsnest display (%s):" ), KeyNameFromKeyCode( hotkey ) );
    else
        msg = _( "Ratsnest display:" );

    m_txtRatsnestVisibility = new wxStaticText( netDisplayPane, wxID_ANY, msg );
    m_txtRatsnestVisibility->SetFont( infoFont );
    m_txtRatsnestVisibility->Wrap( -1 );
    m_txtRatsnestVisibility->SetToolTip( _( "Choose which ratsnest lines to display" ) );

    netDisplayOptionsSizer->Add( m_txtRatsnestVisibility, 0, wxEXPAND | wxBOTTOM | wxLEFT, 2 );

    wxBoxSizer* ratsnestDisplayModeSizer = new wxBoxSizer( wxHORIZONTAL );

    m_rbRatsnestAllLayers = new wxRadioButton( netDisplayPane, wxID_ANY, _( "All" ),
                                               wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_rbRatsnestAllLayers->SetFont( infoFont );
    m_rbRatsnestAllLayers->SetValue( true );
    m_rbRatsnestAllLayers->SetToolTip( _( "Show ratsnest lines to items on all layers" ) );

    ratsnestDisplayModeSizer->Add( m_rbRatsnestAllLayers, 0, wxRIGHT, 5 );
    ratsnestDisplayModeSizer->AddStretchSpacer();

    m_rbRatsnestVisLayers = new wxRadioButton( netDisplayPane, wxID_ANY, _( "Visible layers" ) );
    m_rbRatsnestVisLayers->SetFont( infoFont );
    m_rbRatsnestVisLayers->SetToolTip( _( "Show ratsnest lines to items on visible layers" ) );

    ratsnestDisplayModeSizer->Add( m_rbRatsnestVisLayers, 0, wxRIGHT, 5 );
    ratsnestDisplayModeSizer->AddStretchSpacer();

    m_rbRatsnestNone = new wxRadioButton( netDisplayPane, wxID_ANY, _( "None" ) );
    m_rbRatsnestNone->SetFont( infoFont );
    m_rbRatsnestNone->SetToolTip( _( "Hide all ratsnest lines" ) );

    ratsnestDisplayModeSizer->Add( m_rbRatsnestNone, 0, 0, 5 );

    netDisplayOptionsSizer->Add( ratsnestDisplayModeSizer, 0, wxEXPAND | wxBOTTOM, 5 );

    ////

    netDisplayPane->SetSizer( netDisplayOptionsSizer );
    netDisplayPane->Layout();
    netDisplayOptionsSizer->Fit( netDisplayPane );

    m_netsTabOuterSizer->Add( m_paneNetDisplayOptions, 0, wxEXPAND | wxTOP, 5 );

    m_paneNetDisplayOptions->Bind( WX_COLLAPSIBLE_PANE_CHANGED,
                                   [&]( wxCommandEvent& aEvent )
                                   {
                                       Freeze();
                                       m_panelNetsAndClasses->Fit();
                                       m_sizerOuter->Layout();
                                       passOnFocus();
                                       Thaw();
                                   } );

    m_rbNetColorAll->Bind( wxEVT_RADIOBUTTON, &APPEARANCE_CONTROLS::onNetColorMode, this );
    m_rbNetColorOff->Bind( wxEVT_RADIOBUTTON, &APPEARANCE_CONTROLS::onNetColorMode, this );
    m_rbNetColorRatsnest->Bind( wxEVT_RADIOBUTTON, &APPEARANCE_CONTROLS::onNetColorMode, this );

    m_rbRatsnestAllLayers->Bind( wxEVT_RADIOBUTTON, &APPEARANCE_CONTROLS::onRatsnestMode, this );
    m_rbRatsnestVisLayers->Bind( wxEVT_RADIOBUTTON, &APPEARANCE_CONTROLS::onRatsnestMode, this );
    m_rbRatsnestNone->Bind( wxEVT_RADIOBUTTON, &APPEARANCE_CONTROLS::onRatsnestMode, this );
}


wxSize APPEARANCE_CONTROLS::GetBestSize() const
{
    DPI_SCALING_COMMON dpi( nullptr, m_frame );
    wxSize      size( 220 * dpi.GetScaleFactor(), 480 * dpi.GetScaleFactor() );
    return size;
}


bool APPEARANCE_CONTROLS::IsLayerOptionsExpanded()
{
    return m_paneLayerDisplayOptions->IsExpanded();
}


bool APPEARANCE_CONTROLS::IsNetOptionsExpanded()
{
    return m_paneNetDisplayOptions->IsExpanded();
}


void APPEARANCE_CONTROLS::OnNotebookPageChanged( wxNotebookEvent& aEvent )
{
    // Work around wxMac issue where the notebook pages are blank
#ifdef __WXMAC__
    int page = aEvent.GetSelection();

    if( page >= 0 )
        m_notebook->ChangeSelection( static_cast<unsigned>( page ) );
#endif

#ifndef __WXMSW__
    // Because wxWidgets is broken and will send click events to children of the collapsible
    // panes even if they are collapsed without this
    Freeze();
    m_panelLayers->Fit();
    m_panelNetsAndClasses->Fit();
    m_sizerOuter->Layout();
    Thaw();
#endif

    Bind( wxEVT_IDLE, &APPEARANCE_CONTROLS::idleFocusHandler, this );
}


void APPEARANCE_CONTROLS::idleFocusHandler( wxIdleEvent& aEvent )
{
    passOnFocus();
    Unbind( wxEVT_IDLE, &APPEARANCE_CONTROLS::idleFocusHandler, this );
}


void APPEARANCE_CONTROLS::OnSetFocus( wxFocusEvent& aEvent )
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


void APPEARANCE_CONTROLS::OnSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();
}


void APPEARANCE_CONTROLS::OnNetGridClick( wxGridEvent& event )
{
    int row = event.GetRow();
    int col = event.GetCol();

    switch( col )
    {
    case NET_GRID_TABLE::COL_VISIBILITY:
        m_netsTable->SetValueAsBool( row, col, !m_netsTable->GetValueAsBool( row, col ) );
        m_netsGrid->ForceRefresh();
        break;

    default:
        break;
    }
}


void APPEARANCE_CONTROLS::OnNetGridDoubleClick( wxGridEvent& event )
{
    int row = event.GetRow();
    int col = event.GetCol();

    switch( col )
    {
    case NET_GRID_TABLE::COL_COLOR:
        m_netsGrid->GetCellEditor( row, col )->BeginEdit( row, col, m_netsGrid );
        break;

    default:
        break;
    }
}


void APPEARANCE_CONTROLS::OnNetGridRightClick( wxGridEvent& event )
{
    m_netsGrid->SelectRow( event.GetRow() );

    wxString netName = UnescapeString( m_netsGrid->GetCellValue( event.GetRow(),
                                                                 NET_GRID_TABLE::COL_LABEL ) );
    wxMenu menu;

    menu.Append( new wxMenuItem( &menu, ID_SET_NET_COLOR, _( "Set Net Color" ), wxEmptyString,
                                 wxITEM_NORMAL ) );
    menu.Append( new wxMenuItem( &menu, ID_CLEAR_NET_COLOR, _( "Clear Net Color" ), wxEmptyString,
                                 wxITEM_NORMAL ) );

    menu.AppendSeparator();

    menu.Append( new wxMenuItem( &menu, ID_HIGHLIGHT_NET,
                                 wxString::Format( _( "Highlight %s" ), netName ), wxEmptyString,
                                 wxITEM_NORMAL ) );
    menu.Append( new wxMenuItem( &menu, ID_SELECT_NET,
                                 wxString::Format( _( "Select Tracks and Vias in %s" ), netName ),
                                 wxEmptyString, wxITEM_NORMAL ) );
    menu.Append( new wxMenuItem( &menu, ID_DESELECT_NET,
                                 wxString::Format( _( "Unselect Tracks and Vias in %s" ), netName ),
                                 wxEmptyString, wxITEM_NORMAL ) );

    menu.AppendSeparator();

    menu.Append( new wxMenuItem( &menu, ID_SHOW_ALL_NETS, _( "Show All Nets" ), wxEmptyString,
                                 wxITEM_NORMAL ) );
    menu.Append( new wxMenuItem( &menu, ID_HIDE_OTHER_NETS, _( "Hide All Other Nets" ),
                                 wxEmptyString, wxITEM_NORMAL ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &APPEARANCE_CONTROLS::onNetContextMenu, this );

    PopupMenu( &menu );
}


void APPEARANCE_CONTROLS::OnNetGridMouseEvent( wxMouseEvent& aEvent )
{
    wxPoint pos = m_netsGrid->CalcUnscrolledPosition( aEvent.GetPosition() );
    wxGridCellCoords cell = m_netsGrid->XYToCell( pos );

    if( aEvent.Moving() || aEvent.Entering() )
    {
        aEvent.Skip();

        if( !cell )
        {
            m_netsGrid->GetGridWindow()->UnsetToolTip();
            return;
        }

        if( cell == m_hoveredCell )
            return;

        m_hoveredCell = cell;

        NET_GRID_ENTRY& net = m_netsTable->GetEntry( cell.GetRow() );

        wxString name = net.name;
        wxString showOrHide = net.visible ? _( "Click to hide ratsnest for %s" )
                                          : _( "Click to show ratsnest for %s" );
        wxString tip;

        if( cell.GetCol() == NET_GRID_TABLE::COL_VISIBILITY )
            tip.Printf( showOrHide, name );
        else if( cell.GetCol() == NET_GRID_TABLE::COL_COLOR )
            tip = _( "Double click (or middle click) to change color; right click for more actions" );

        m_netsGrid->GetGridWindow()->SetToolTip( tip );
    }
    else if( aEvent.Leaving() )
    {
        m_netsGrid->UnsetToolTip();
        aEvent.Skip();
    }
    else if( aEvent.Dragging() )
    {
        // not allowed
        CallAfter( [this]()
                   {
                       m_netsGrid->ClearSelection();
                   } );
    }
    else if( aEvent.ButtonUp( wxMOUSE_BTN_MIDDLE ) && !!cell )
    {
        int row = cell.GetRow();
        int col = cell.GetCol();

        if(col == NET_GRID_TABLE::COL_COLOR )
            m_netsGrid->GetCellEditor( row, col )->BeginEdit( row, col, m_netsGrid );

        aEvent.Skip();
    }
    else
    {
        aEvent.Skip();
    }
}


void APPEARANCE_CONTROLS::OnLanguageChanged( wxCommandEvent& aEvent )
{
    m_notebook->SetPageText( 0, _( "Layers" ) );
    m_notebook->SetPageText( 1, _( "Objects" ) );

    if( m_notebook->GetPageCount() >= 3 )
        m_notebook->SetPageText( 2, _( "Nets" ) );

    m_netsGrid->ClearSelection();

    Freeze();
    rebuildLayers();
    rebuildLayerContextMenu();
    rebuildLayerPresetsWidget( false );
    rebuildViewportsWidget();
    rebuildObjects();
    rebuildNets();

    syncColorsAndVisibility();
    syncObjectSettings();
    syncLayerPresetSelection();

    UpdateDisplayOptions();

    Thaw();
    Refresh();

    aEvent.Skip();
}


void APPEARANCE_CONTROLS::OnBoardChanged()
{
    m_netsGrid->ClearSelection();

    Freeze();
    rebuildLayers();
    rebuildLayerContextMenu();
    syncColorsAndVisibility();
    syncObjectSettings();
    rebuildNets();
    rebuildLayerPresetsWidget( true );
    syncLayerPresetSelection();
    rebuildViewportsWidget();

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


void APPEARANCE_CONTROLS::OnNetVisibilityChanged( int aNetCode, bool aVisibility )
{
    if( m_togglingNetclassRatsnestVisibility )
        return;

    int row = m_netsTable->GetRowByNetcode( aNetCode );

    if( row >= 0 )
    {
        m_netsTable->SetValueAsBool( row, NET_GRID_TABLE::COL_VISIBILITY, aVisibility );
        m_netsGrid->ForceRefresh();
    }
}


bool APPEARANCE_CONTROLS::doesBoardItemNeedRebuild( BOARD_ITEM* aBoardItem )
{
    return aBoardItem->Type() == PCB_NETINFO_T;
}


bool APPEARANCE_CONTROLS::doesBoardItemNeedRebuild( std::vector<BOARD_ITEM*>& aBoardItems )
{
    bool rebuild = std::any_of( aBoardItems.begin(), aBoardItems.end(),
                                []( const BOARD_ITEM* a )
                                {
                                    return a->Type() == PCB_NETINFO_T;
                                } );

    return rebuild;
}


void APPEARANCE_CONTROLS::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aItem )
{
    if( doesBoardItemNeedRebuild( aItem ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems )
{
    if( doesBoardItemNeedRebuild( aItems ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aItem )
{
    if( doesBoardItemNeedRebuild( aItem ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems )
{
    if( doesBoardItemNeedRebuild( aItems ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aItem )
{
    if( doesBoardItemNeedRebuild( aItem ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems )
{
    if( doesBoardItemNeedRebuild( aItems ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardCompositeUpdate( BOARD&                    aBoard,
                                                  std::vector<BOARD_ITEM*>& aAddedItems,
                                                  std::vector<BOARD_ITEM*>& aRemovedItems,
                                                  std::vector<BOARD_ITEM*>& aChangedItems )
{
    if( doesBoardItemNeedRebuild( aAddedItems ) || doesBoardItemNeedRebuild( aRemovedItems )
        || doesBoardItemNeedRebuild( aChangedItems ) )
    {
        handleBoardItemsChanged();
    }
}


void APPEARANCE_CONTROLS::handleBoardItemsChanged()
{
    m_netsGrid->ClearSelection();

    Freeze();
    rebuildNets();
    Thaw();
}


void APPEARANCE_CONTROLS::OnColorThemeChanged()
{
    syncColorsAndVisibility();
    syncObjectSettings();
}


void APPEARANCE_CONTROLS::OnDarkModeToggle()
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

    // Easier than calling OnDarkModeToggle on all the GRID_CELL_COLOR_RENDERERs:
    m_netsGrid->RegisterDataType( wxT( "COLOR4D" ),
                                  new GRID_CELL_COLOR_RENDERER( m_frame, SWATCH_SMALL ),
                                  new GRID_CELL_COLOR_SELECTOR( m_frame, m_netsGrid ) );

    for( const std::pair<const wxString, APPEARANCE_SETTING*>& pair : m_netclassSettingsMap )
    {
        if( pair.second->ctl_color )
            pair.second->ctl_color->OnDarkModeToggle();
    }

    OnLayerChanged();       // Update selected highlighting
}


void APPEARANCE_CONTROLS::OnLayerChanged()
{
    for( const std::unique_ptr<APPEARANCE_SETTING>& setting : m_layerSettings )
    {
        setting->ctl_panel->SetBackgroundColour( m_layerPanelColour );
        setting->ctl_indicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::OFF );
    }

    wxChar r = m_layerPanelColour.Red();
    wxChar g = m_layerPanelColour.Green();
    wxChar b = m_layerPanelColour.Blue();

    if( r < 240 || g < 240 || b < 240 )
    {
        r = wxChar( std::min( (int) r + 15, 255 ) );
        g = wxChar( std::min( (int) g + 15, 255 ) );
        b = wxChar( std::min( (int) b + 15, 255 ) );
    }
    else
    {
        r = wxChar( std::max( (int) r - 15, 0 ) );
        g = wxChar( std::max( (int) g - 15, 0 ) );
        b = wxChar( std::max( (int) b - 15, 0 ) );
    }

    PCB_LAYER_ID current = m_frame->GetActiveLayer();

    if( !m_layerSettingsMap.count( current ) )
    {
        wxASSERT( m_layerSettingsMap.count( F_Cu ) );
        current = F_Cu;
    }

    APPEARANCE_SETTING* newSetting = m_layerSettingsMap[ current ];

    newSetting->ctl_panel->SetBackgroundColour( wxColour( r, g, b ) );
    newSetting->ctl_indicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::ON );

    Refresh();
}


void APPEARANCE_CONTROLS::SetLayerVisible( int aLayer, bool isVisible )
{
    LSET         visible = getVisibleLayers();
    PCB_LAYER_ID layer   = ToLAYER_ID( aLayer );

    if( visible.test( layer ) == isVisible )
        return;

    visible.set( layer, isVisible );
    setVisibleLayers( visible );

    m_frame->GetCanvas()->GetView()->SetLayerVisible( layer, isVisible );

    syncColorsAndVisibility();
}


void APPEARANCE_CONTROLS::SetObjectVisible( GAL_LAYER_ID aLayer, bool isVisible )
{
    if( m_objectSettingsMap.count( aLayer ) )
    {
        APPEARANCE_SETTING* setting = m_objectSettingsMap.at( aLayer );

        if( setting->can_control_visibility )
            setting->ctl_visibility->SetValue( isVisible );
    }

    m_frame->GetBoard()->SetElementVisibility( aLayer, isVisible );

    m_frame->Update3DView( true, m_frame->GetPcbNewSettings()->m_Display.m_Live3DRefresh );

    m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, isVisible );
    m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::setVisibleLayers( const LSET& aLayers )
{
    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();

    if( m_isFpEditor )
    {
        for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
            view->SetLayerVisible( layer, aLayers.Contains( layer ) );
    }
    else
    {
        m_frame->GetBoard()->SetVisibleLayers( aLayers );

        // Note: KIGFX::REPAINT isn't enough for things that go from invisible to visible as
        // they won't be found in the view layer's itemset for repainting.
        view->UpdateAllItemsConditionally( KIGFX::ALL,
                []( KIGFX::VIEW_ITEM* aItem ) -> bool
                {
                    // Items rendered to composite layers (such as LAYER_PAD_TH) must be redrawn
                    // whether they're optionally flashed or not (as the layer being hidden/shown
                    // might be the last layer the item is visible on).
                    return dynamic_cast<PCB_VIA*>( aItem ) || dynamic_cast<PAD*>( aItem );
                } );

        m_frame->Update3DView( true, m_frame->GetPcbNewSettings()->m_Display.m_Live3DRefresh );
    }
}


bool APPEARANCE_CONTROLS::isLayerEnabled( PCB_LAYER_ID aLayer ) const
{
    // This used to be used for disabling some layers in the footprint editor, but
    // now all layers are enabled in the footprint editor.
    // But this function is the place to add logic if you do need to grey out a layer
    // from the appearance panel for some reason.
    return true;
}


void APPEARANCE_CONTROLS::setVisibleObjects( GAL_SET aLayers )
{
    if( m_isFpEditor )
    {
        KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();

        for( size_t i = 0; i < GAL_LAYER_INDEX( LAYER_ZONE_START ); i++ )
            view->SetLayerVisible( GAL_LAYER_ID_START + GAL_LAYER_ID( i ), aLayers.test( i ) );
    }
    else
    {
        // Ratsnest visibility is controlled by the ratsnest option, and not by the preset
        if( m_frame->IsType( FRAME_PCB_EDITOR ) )
            aLayers.set( LAYER_RATSNEST, m_frame->GetPcbNewSettings()->m_Display.m_ShowGlobalRatsnest );

        m_frame->SetGridVisibility( aLayers.test( LAYER_GRID - GAL_LAYER_ID_START ) );
        m_frame->GetBoard()->SetVisibleElements( aLayers );

        m_frame->Update3DView( true, m_frame->GetPcbNewSettings()->m_Display.m_Live3DRefresh );
    }
}


LSET APPEARANCE_CONTROLS::getVisibleLayers()
{
    if( m_isFpEditor )
    {
        KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
        LSET set;

        for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
            set.set( layer, view->IsLayerVisible( layer ) );

        return set;
    }
    else
    {
        return m_frame->GetBoard()->GetVisibleLayers();
    }
}


GAL_SET APPEARANCE_CONTROLS::getVisibleObjects()
{
    if( m_isFpEditor )
    {
        KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
        GAL_SET set;
        set.reset();

        for( size_t i = 0; i < set.size(); i++ )
            set.set( i, view->IsLayerVisible( GAL_LAYER_ID_START + GAL_LAYER_ID( i ) ) );

        return set;
    }
    else
    {
        return m_frame->GetBoard()->GetVisibleElements();
    }
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

    m_cbFlipBoard->SetValue( m_frame->GetCanvas()->GetView()->IsMirroredX() );

    if( !m_isFpEditor )
    {
        if( PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings() )
        {
            if( !cfg->m_Display.m_ShowGlobalRatsnest )
                m_rbRatsnestNone->SetValue( true );
            else if( cfg->m_Display.m_RatsnestMode == RATSNEST_MODE::ALL )
                m_rbRatsnestAllLayers->SetValue( true );
            else
                m_rbRatsnestVisLayers->SetValue( true );

            wxASSERT( m_objectSettingsMap.count( LAYER_RATSNEST ) );
            APPEARANCE_SETTING* ratsnest = m_objectSettingsMap.at( LAYER_RATSNEST );
            ratsnest->ctl_visibility->SetValue( cfg->m_Display.m_ShowGlobalRatsnest );
        }
    }
}


std::vector<LAYER_PRESET> APPEARANCE_CONTROLS::GetUserLayerPresets() const
{
    std::vector<LAYER_PRESET> ret;

    for( const std::pair<const wxString, LAYER_PRESET>& pair : m_layerPresets )
    {
        if( !pair.second.readOnly )
            ret.emplace_back( pair.second );
    }

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

        m_presetMRU.Add( preset.name );
    }

    rebuildLayerPresetsWidget( true );
}


void APPEARANCE_CONTROLS::loadDefaultLayerPresets()
{
    m_layerPresets.clear();

    // Load the read-only defaults
    for( const LAYER_PRESET& preset : { presetAllLayers,
                                        presetNoLayers,
                                        presetAllCopper,
                                        presetInnerCopper,
                                        presetFront,
                                        presetFrontAssembly,
                                        presetBack,
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

    m_lastSelectedUserPreset = ( m_currentPreset && !m_currentPreset->readOnly ) ? m_currentPreset
                                                                                 : nullptr;

    updateLayerPresetSelection( aPreset.name );
    doApplyLayerPreset( aPreset );
}


std::vector<VIEWPORT> APPEARANCE_CONTROLS::GetUserViewports() const
{
    std::vector<VIEWPORT> ret;

    for( const std::pair<const wxString, VIEWPORT>& pair : m_viewports )
        ret.emplace_back( pair.second );

    return ret;
}


void APPEARANCE_CONTROLS::SetUserViewports( std::vector<VIEWPORT>& aViewportList )
{
    m_viewports.clear();

    for( const VIEWPORT& viewport : aViewportList )
    {
        if( m_viewports.count( viewport.name ) )
            continue;

        m_viewports[viewport.name] = viewport;

        m_viewportMRU.Add( viewport.name );
    }

    rebuildViewportsWidget();
}


void APPEARANCE_CONTROLS::ApplyViewport( const wxString& aViewportName )
{
    updateViewportSelection( aViewportName );

    wxCommandEvent dummy;
    onViewportChanged( dummy );
}


void APPEARANCE_CONTROLS::ApplyViewport( const VIEWPORT& aViewport )
{
    updateViewportSelection( aViewport.name );
    doApplyViewport( aViewport );
}


void APPEARANCE_CONTROLS::rebuildLayers()
{
    BOARD* board = m_frame->GetBoard();
    LSET enabled = board->GetEnabledLayers();
    LSET visible = getVisibleLayers();

    COLOR_SETTINGS* theme    = m_frame->GetColorSettings();
    COLOR4D         bgColor  = theme->GetColor( LAYER_PCB_BACKGROUND );
    bool            readOnly = theme->IsReadOnly();

    FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );

#ifdef __WXMAC__
    wxSizerItem* m_windowLayersSizerItem = m_panelLayersSizer->GetItem( m_windowLayers );
    m_windowLayersSizerItem->SetFlag( m_windowLayersSizerItem->GetFlag() & ~wxTOP );
#endif

    auto appendLayer =
            [&]( std::unique_ptr<APPEARANCE_SETTING>& aSetting )
            {
                int layer = aSetting->id;

                wxPanel*    panel = new wxPanel( m_windowLayers, layer );
                wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
                panel->SetSizer( sizer );

                panel->SetBackgroundColour( m_layerPanelColour );

                aSetting->visible = visible[layer];

                // TODO(JE) consider restyling this indicator
                INDICATOR_ICON* indicator = new INDICATOR_ICON( panel, *m_iconProvider,
                                                                ROW_ICON_PROVIDER::STATE::OFF, layer );

                COLOR_SWATCH* swatch = new COLOR_SWATCH( panel, COLOR4D::UNSPECIFIED, layer, bgColor,
                                                         theme->GetColor( layer ), SWATCH_SMALL );
                swatch->SetToolTip( _( "Double click or middle click for color change, right click for menu" ) );

                BITMAP_TOGGLE* btn_visible = new BITMAP_TOGGLE( panel, layer,
                                                                m_visibleBitmapBundle,
                                                                m_notVisibileBitmapBundle,
                                                                aSetting->visible );
                btn_visible->SetToolTip( _( "Show or hide this layer" ) );

                wxStaticText* label = new wxStaticText( panel, layer, aSetting->label );
                label->Wrap( -1 );
                label->SetToolTip( aSetting->tooltip );

                sizer->AddSpacer( 1 );
                sizer->Add( indicator, 0, wxALIGN_CENTER_VERTICAL | wxTOP, 2 );
                sizer->AddSpacer( 5 );
                sizer->Add( swatch, 0, wxALIGN_CENTER_VERTICAL | wxTOP, 2 );
                sizer->AddSpacer( 6 );
                sizer->Add( btn_visible, 0, wxALIGN_CENTER_VERTICAL | wxTOP, 2 );
                sizer->AddSpacer( 5 );
                sizer->Add( label, 1, wxALIGN_CENTER_VERTICAL | wxTOP, 2 );

                m_layersOuterSizer->Add( panel, 0, wxEXPAND, 0 );

                aSetting->ctl_panel      = panel;
                aSetting->ctl_indicator  = indicator;
                aSetting->ctl_visibility = btn_visible;
                aSetting->ctl_color      = swatch;
                aSetting->ctl_text       = label;

                panel->Bind( wxEVT_LEFT_DOWN, &APPEARANCE_CONTROLS::onLayerLeftClick, this );
                indicator->Bind( wxEVT_LEFT_DOWN, &APPEARANCE_CONTROLS::onLayerLeftClick, this );
                swatch->Bind( wxEVT_LEFT_DOWN, &APPEARANCE_CONTROLS::onLayerLeftClick, this );
                label->Bind( wxEVT_LEFT_DOWN, &APPEARANCE_CONTROLS::onLayerLeftClick, this );

                btn_visible->Bind( TOGGLE_CHANGED,
                        [&]( wxCommandEvent& aEvent )
                        {
                            wxObject* btn = aEvent.GetEventObject();
                            int       layerId = static_cast<wxWindow*>( btn )->GetId();

                            onLayerVisibilityToggled( static_cast<PCB_LAYER_ID>( layerId ) );
                        } );

                swatch->Bind( COLOR_SWATCH_CHANGED, &APPEARANCE_CONTROLS::OnColorSwatchChanged, this );
                swatch->SetReadOnlyCallback( std::bind( &APPEARANCE_CONTROLS::onReadOnlySwatch, this ) );
                swatch->SetReadOnly( readOnly );

                panel->Bind( wxEVT_RIGHT_DOWN, &APPEARANCE_CONTROLS::rightClickHandler, this );
                indicator->Bind( wxEVT_RIGHT_DOWN, &APPEARANCE_CONTROLS::rightClickHandler, this );
                swatch->Bind( wxEVT_RIGHT_DOWN, &APPEARANCE_CONTROLS::rightClickHandler, this );
                btn_visible->Bind( wxEVT_RIGHT_DOWN, &APPEARANCE_CONTROLS::rightClickHandler, this );
                label->Bind( wxEVT_RIGHT_DOWN, &APPEARANCE_CONTROLS::rightClickHandler, this );
            };

    auto updateLayer =
            [&]( std::unique_ptr<APPEARANCE_SETTING>& aSetting )
            {
                int layer = aSetting->id;
                aSetting->visible = visible[layer];
                aSetting->ctl_panel->Show();
                aSetting->ctl_panel->SetId( layer );
                aSetting->ctl_indicator->SetWindowID( layer );
                aSetting->ctl_color->SetWindowID( layer );
                aSetting->ctl_color->SetSwatchColor( theme->GetColor( layer ), false );
                aSetting->ctl_visibility->SetWindowID( layer );
                aSetting->ctl_text->SetLabelText( aSetting->label );
                aSetting->ctl_text->SetId( layer );
                aSetting->ctl_text->SetToolTip( aSetting->tooltip );
            };

    // technical layers are shown in this order:
    // Because they are static, wxGetTranslation must be explicitly
    // called for tooltips.
    static const struct {
        PCB_LAYER_ID layerId;
        wxString     tooltip;
    } non_cu_seq[] = {
        { F_Adhes,          _HKI( "Adhesive on board's front" ) },
        { B_Adhes,          _HKI( "Adhesive on board's back" ) },
        { F_Paste,          _HKI( "Solder paste on board's front" ) },
        { B_Paste,          _HKI( "Solder paste on board's back" ) },
        { F_SilkS,          _HKI( "Silkscreen on board's front" ) },
        { B_SilkS,          _HKI( "Silkscreen on board's back" ) },
        { F_Mask,           _HKI( "Solder mask on board's front" ) },
        { B_Mask,           _HKI( "Solder mask on board's back" ) },
        { Dwgs_User,        _HKI( "Explanatory drawings" ) },
        { Cmts_User,        _HKI( "Explanatory comments" ) },
        { Eco1_User,        _HKI( "User defined meaning" ) },
        { Eco2_User,        _HKI( "User defined meaning" ) },
        { Edge_Cuts,        _HKI( "Board's perimeter definition" ) },
        { Margin,           _HKI( "Board's edge setback outline" ) },
        { F_CrtYd,          _HKI( "Footprint courtyards on board's front" ) },
        { B_CrtYd,          _HKI( "Footprint courtyards on board's back" ) },
        { F_Fab,            _HKI( "Footprint assembly on board's front" ) },
        { B_Fab,            _HKI( "Footprint assembly on board's back" ) },
        { User_1,           _HKI( "User defined layer 1" ) },
        { User_2,           _HKI( "User defined layer 2" ) },
        { User_3,           _HKI( "User defined layer 3" ) },
        { User_4,           _HKI( "User defined layer 4" ) },
        { User_5,           _HKI( "User defined layer 5" ) },
        { User_6,           _HKI( "User defined layer 6" ) },
        { User_7,           _HKI( "User defined layer 7" ) },
        { User_8,           _HKI( "User defined layer 8" ) },
        { User_9,           _HKI( "User defined layer 9" ) },
        { User_10,           _HKI( "User defined layer 10" ) },
        { User_11,           _HKI( "User defined layer 11" ) },
        { User_12,           _HKI( "User defined layer 12" ) },
        { User_13,           _HKI( "User defined layer 13" ) },
        { User_14,           _HKI( "User defined layer 14" ) },
        { User_15,           _HKI( "User defined layer 15" ) },
        { User_16,           _HKI( "User defined layer 16" ) },
        { User_17,           _HKI( "User defined layer 17" ) },
        { User_18,           _HKI( "User defined layer 18" ) },
        { User_19,           _HKI( "User defined layer 19" ) },
        { User_20,           _HKI( "User defined layer 20" ) },
        { User_21,           _HKI( "User defined layer 21" ) },
        { User_22,           _HKI( "User defined layer 22" ) },
        { User_23,           _HKI( "User defined layer 23" ) },
        { User_24,           _HKI( "User defined layer 24" ) },
        { User_25,           _HKI( "User defined layer 25" ) },
        { User_26,           _HKI( "User defined layer 26" ) },
        { User_27,           _HKI( "User defined layer 27" ) },
        { User_28,           _HKI( "User defined layer 28" ) },
        { User_29,           _HKI( "User defined layer 29" ) },
        { User_30,           _HKI( "User defined layer 30" ) },
        { User_31,           _HKI( "User defined layer 31" ) },
        { User_32,           _HKI( "User defined layer 32" ) },
        { User_33,           _HKI( "User defined layer 33" ) },
        { User_34,           _HKI( "User defined layer 34" ) },
        { User_35,           _HKI( "User defined layer 35" ) },
        { User_36,           _HKI( "User defined layer 36" ) },
        { User_37,           _HKI( "User defined layer 37" ) },
        { User_38,           _HKI( "User defined layer 38" ) },
        { User_39,           _HKI( "User defined layer 39" ) },
        { User_40,           _HKI( "User defined layer 40" ) },
        { User_41,           _HKI( "User defined layer 41" ) },
        { User_42,           _HKI( "User defined layer 42" ) },
        { User_43,           _HKI( "User defined layer 43" ) },
        { User_44,           _HKI( "User defined layer 44" ) },
        { User_45,           _HKI( "User defined layer 45" ) },
    };

    // There is a spacer added to the end of the list that we need to remove and re-add
    // after possibly adding additional layers
    if( m_layersOuterSizer->GetItemCount() > 0 )
    {
        m_layersOuterSizer->Detach( m_layersOuterSizer->GetItemCount() - 1 );
    }
    // Otherwise, this is the first time we are updating the control, so we need to attach
    // the handler
    else
    {
        // Add right click handling to show the context menu when clicking to the free area in
        // m_windowLayers (below the layer items)
        m_windowLayers->Bind( wxEVT_RIGHT_DOWN, &APPEARANCE_CONTROLS::rightClickHandler, this );
    }

    std::size_t total_layers = enabled.CuStack().size();

    for( const auto& entry : non_cu_seq )
    {
        if( enabled[entry.layerId] )
            total_layers++;
    }

    // Adds layers to the panel until we have enough to hold our total count
    while( total_layers > m_layerSettings.size() )
        m_layerSettings.push_back( std::make_unique<APPEARANCE_SETTING>() );

    // We never delete layers from the panel, only hide them.  This saves us
    // having to recreate the (possibly) later with minimal overhead
    for( std::size_t ii = total_layers; ii < m_layerSettings.size(); ++ii )
    {
        if( m_layerSettings[ii]->ctl_panel )
            m_layerSettings[ii]->ctl_panel->Show( false );
    }

    auto layer_it = m_layerSettings.begin();

    // show all coppers first, with front on top, back on bottom, then technical layers
    for( PCB_LAYER_ID layer : enabled.CuStack() )
    {
        wxString dsc;

        switch( layer )
        {
            case F_Cu: dsc = _( "Front copper layer" ); break;
            case B_Cu: dsc = _( "Back copper layer" );  break;
            default:   dsc = _( "Inner copper layer" ); break;
        }

        std::unique_ptr<APPEARANCE_SETTING>& setting = *layer_it;

        setting->label = board->GetLayerName( layer );
        setting->id = layer;
        setting->tooltip = dsc;

        if( setting->ctl_panel == nullptr )
            appendLayer( setting );
        else
            updateLayer( setting );

        m_layerSettingsMap[layer] = setting.get();

        if( !isLayerEnabled( layer ) )
        {
            setting->ctl_text->Disable();
            setting->ctl_color->SetToolTip( wxEmptyString );
        }

        ++layer_it;
    }

    for( const auto& entry : non_cu_seq )
    {
        PCB_LAYER_ID layer = entry.layerId;

        if( !enabled[layer] )
            continue;

        std::unique_ptr<APPEARANCE_SETTING>& setting = *layer_it;

        if( m_isFpEditor )
        {
            wxString canonicalName = LSET::Name( static_cast<PCB_LAYER_ID>( layer ) );

            if( cfg->m_DesignSettings.m_UserLayerNames.contains( canonicalName.ToStdString() ) )
                setting->label = cfg->m_DesignSettings.m_UserLayerNames[canonicalName.ToStdString()];
            else
                setting->label = board->GetStandardLayerName( layer );
        }
        else
        {
            setting->label = board->GetLayerName( layer );
        }

        setting->id = layer;
        // Because non_cu_seq is created static, we must explicitly call wxGetTranslation for
        // texts which are internationalized
        setting->tooltip = wxGetTranslation( entry.tooltip );

        if( setting->ctl_panel == nullptr )
            appendLayer( setting );
        else
            updateLayer( setting );

        m_layerSettingsMap[layer] = setting.get();

        if( !isLayerEnabled( layer ) )
        {
            setting->ctl_text->Disable();
            setting->ctl_color->SetToolTip( wxEmptyString );
        }

        ++layer_it;
    }

    m_layersOuterSizer->AddSpacer( 10 );
    m_windowLayers->SetBackgroundColour( m_layerPanelColour );
    m_windowLayers->FitInside(); // Updates virtual size to fit subwindows, also auto-layouts.

    m_paneLayerDisplayOptions->SetLabel( _( "Layer Display Options" ) );

    int hotkey = PCB_ACTIONS::highContrastModeCycle.GetHotKey();
    wxString msg;

    if( hotkey )
        msg = wxString::Format( _( "Inactive layers (%s):" ), KeyNameFromKeyCode( hotkey ) );
    else
        msg = _( "Inactive layers:" );

    m_inactiveLayersLabel->SetLabel( msg );

    m_rbHighContrastNormal->SetLabel( _( "Normal" ) );
    m_rbHighContrastNormal->SetToolTip( _( "Inactive layers will be shown in full color" ) );

    m_rbHighContrastDim->SetLabel( _( "Dim" ) );
    m_rbHighContrastDim->SetToolTip( _( "Inactive layers will be dimmed" ) );

    m_rbHighContrastOff->SetLabel( _( "Hide" ) );
    m_rbHighContrastOff->SetToolTip( _( "Inactive layers will be hidden" ) );

    m_cbFlipBoard->SetLabel( _( "Flip board view" ) );
}


void APPEARANCE_CONTROLS::rebuildLayerContextMenu()
{
    delete m_layerContextMenu;
    m_layerContextMenu = new wxMenu;

    KIUI::AddMenuItem( m_layerContextMenu, ID_SHOW_ALL_COPPER_LAYERS, _( "Show All Copper Layers" ),
                       KiBitmap( BITMAPS::show_all_copper_layers ) );
    KIUI::AddMenuItem( m_layerContextMenu, ID_HIDE_ALL_COPPER_LAYERS, _( "Hide All Copper Layers" ),
                       KiBitmap( BITMAPS::show_no_copper_layers ) );

    m_layerContextMenu->AppendSeparator();

    KIUI::AddMenuItem( m_layerContextMenu, ID_HIDE_ALL_BUT_ACTIVE, _( "Hide All Layers But Active" ),
                       KiBitmap( BITMAPS::select_w_layer ) );

    m_layerContextMenu->AppendSeparator();

    KIUI::AddMenuItem( m_layerContextMenu, ID_SHOW_ALL_NON_COPPER, _( "Show All Non Copper Layers" ),
                       KiBitmap( BITMAPS::show_no_copper_layers ) );

    KIUI::AddMenuItem( m_layerContextMenu, ID_HIDE_ALL_NON_COPPER, _( "Hide All Non Copper Layers" ),
                       KiBitmap( BITMAPS::show_all_copper_layers ) );

    m_layerContextMenu->AppendSeparator();

    KIUI::AddMenuItem( m_layerContextMenu, ID_PRESET_ALL_LAYERS, _( "Show All Layers" ),
                       KiBitmap( BITMAPS::show_all_layers ) );

    KIUI::AddMenuItem( m_layerContextMenu, ID_PRESET_NO_LAYERS, _( "Hide All Layers" ),
                       KiBitmap( BITMAPS::show_no_layers ) );

    m_layerContextMenu->AppendSeparator();

    KIUI::AddMenuItem( m_layerContextMenu, ID_PRESET_FRONT_ASSEMBLY, _( "Show Only Front Assembly Layers" ),
                       KiBitmap( BITMAPS::show_front_assembly_layers ) );

    KIUI::AddMenuItem( m_layerContextMenu, ID_PRESET_FRONT, _( "Show Only Front Layers" ),
                       KiBitmap( BITMAPS::show_all_front_layers ) );

    // Only show the internal layer option if internal layers are enabled
    if( m_frame->GetBoard()->GetCopperLayerCount() > 2 )
    {
        KIUI::AddMenuItem( m_layerContextMenu, ID_PRESET_INNER_COPPER, _( "Show Only Inner Layers" ),
                           KiBitmap( BITMAPS::show_all_copper_layers ) );
    }

    KIUI::AddMenuItem( m_layerContextMenu, ID_PRESET_BACK, _( "Show Only Back Layers" ),
                       KiBitmap( BITMAPS::show_all_back_layers ) );

    KIUI::AddMenuItem( m_layerContextMenu, ID_PRESET_BACK_ASSEMBLY, _( "Show Only Back Assembly Layers" ),
                       KiBitmap( BITMAPS::show_back_assembly_layers ) );
}


void APPEARANCE_CONTROLS::OnLayerContextMenu( wxCommandEvent& aEvent )
{
    BOARD* board   = m_frame->GetBoard();
    LSET   visible = getVisibleLayers();

    PCB_LAYER_ID current = m_frame->GetActiveLayer();

    // The new preset. We keep the visibility state of objects:
    LAYER_PRESET preset;
    preset.renderLayers = getVisibleObjects();

    switch( aEvent.GetId() )
    {
    case ID_PRESET_NO_LAYERS:
        preset.layers = presetNoLayers.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_PRESET_ALL_LAYERS:
        preset.layers = presetAllLayers.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_SHOW_ALL_COPPER_LAYERS:
        visible |= presetAllCopper.layers;
        setVisibleLayers( visible );
        break;

    case ID_HIDE_ALL_BUT_ACTIVE:
        preset.layers = presetNoLayers.layers | LSET( { current } );
        ApplyLayerPreset( preset );
        break;

    case ID_HIDE_ALL_COPPER_LAYERS:
        visible &= ~presetAllCopper.layers;

        if( !visible.test( current ) && visible.count() > 0 )
            m_frame->SetActiveLayer( *visible.Seq().begin() );

        setVisibleLayers( visible );
        break;

    case ID_HIDE_ALL_NON_COPPER:
        visible &= presetAllCopper.layers;

        if( !visible.test( current ) && visible.count() > 0 )
            m_frame->SetActiveLayer( *visible.Seq().begin() );

        setVisibleLayers( visible );
        break;

    case ID_SHOW_ALL_NON_COPPER:
        visible |= ~presetAllCopper.layers;

        setVisibleLayers( visible );
        break;

    case ID_PRESET_FRONT_ASSEMBLY:
        preset.layers = presetFrontAssembly.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_PRESET_FRONT:
        preset.layers = presetFront.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_PRESET_INNER_COPPER:
        preset.layers = presetInnerCopper.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_PRESET_BACK:
        preset.layers = presetBack.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_PRESET_BACK_ASSEMBLY:
        preset.layers = presetBackAssembly.layers;
        ApplyLayerPreset( preset );
        return;
    }

    syncLayerPresetSelection();
    syncColorsAndVisibility();

    if( !m_isFpEditor )
        m_frame->GetCanvas()->SyncLayersVisibility( board );

    m_frame->GetCanvas()->Refresh();
}


int APPEARANCE_CONTROLS::GetTabIndex() const
{
    return m_notebook->GetSelection();
}


void APPEARANCE_CONTROLS::SetTabIndex( int aTab )
{
    size_t max = m_notebook->GetPageCount();

    if( aTab >= 0 && static_cast<size_t>( aTab ) < max )
        m_notebook->SetSelection( aTab );
}


void APPEARANCE_CONTROLS::syncColorsAndVisibility()
{
    COLOR_SETTINGS* theme    = m_frame->GetColorSettings();
    bool            readOnly = theme->IsReadOnly();
    LSET            visible  = getVisibleLayers();
    GAL_SET         objects  = getVisibleObjects();

    Freeze();

    for( std::unique_ptr<APPEARANCE_SETTING>& setting : m_layerSettings )
    {
        int layer = setting->id;

        if( setting->ctl_visibility )
            setting->ctl_visibility->SetValue( visible[layer] );

        if( setting->ctl_color )
        {
            const COLOR4D& color = theme->GetColor( layer );
            setting->ctl_color->SetSwatchColor( color, false );
            setting->ctl_color->SetReadOnly( readOnly );
        }
    }

    for( std::unique_ptr<APPEARANCE_SETTING>& setting : m_objectSettings )
    {
        GAL_LAYER_ID layer = static_cast<GAL_LAYER_ID>( setting->id );

        if( setting->ctl_visibility )
            setting->ctl_visibility->SetValue( objects.Contains( layer ) );

        if( setting->ctl_color )
        {
            const COLOR4D& color = theme->GetColor( layer );
            setting->ctl_color->SetSwatchColor( color, false );
            setting->ctl_color->SetReadOnly( readOnly );
        }
    }

    // Update indicators and panel background colors
    OnLayerChanged();

    Thaw();

    m_windowLayers->Refresh();
}


void APPEARANCE_CONTROLS::onLayerLeftClick( wxMouseEvent& aEvent )
{
    wxWindow* eventSource = static_cast<wxWindow*>( aEvent.GetEventObject() );

    PCB_LAYER_ID layer = ToLAYER_ID( eventSource->GetId() );

    if( !isLayerEnabled( layer ) )
        return;

    m_frame->SetActiveLayer( layer );
    passOnFocus();
}


void APPEARANCE_CONTROLS::rightClickHandler( wxMouseEvent& aEvent )
{
    wxASSERT( m_layerContextMenu );
    PopupMenu( m_layerContextMenu );
    passOnFocus();
};


void APPEARANCE_CONTROLS::onLayerVisibilityToggled( PCB_LAYER_ID aLayer )
{
    LSET visibleLayers = getVisibleLayers();

    visibleLayers.set( aLayer, !visibleLayers.test( aLayer ) );
    setVisibleLayers( visibleLayers );
    m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, visibleLayers.test( aLayer ) );

    syncLayerPresetSelection();
    m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::onObjectVisibilityChanged( GAL_LAYER_ID aLayer, bool isVisible, bool isFinal )
{
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
            m_frame->GetPcbNewSettings()->m_Display.m_ShowGlobalRatsnest = isVisible;
            m_frame->GetBoard()->SetElementVisibility( aLayer, isVisible );
            m_frame->OnDisplayOptionsChanged();
            m_frame->GetCanvas()->RedrawRatsnest();
        }

        break;
    }

    case LAYER_GRID:
        m_frame->SetGridVisibility( isVisible );
        m_frame->GetCanvas()->Refresh();
        syncLayerPresetSelection();
        break;

    case LAYER_FP_TEXT:
        // Because Footprint Text is a meta-control that also can disable values/references,
        // drag them along here so that the user is less likely to be confused.
        if( isFinal )
        {
            // Should only trigger when you actually click the Footprint Text button
            // Otherwise it goes into infinite recursive loop with the following case section
            onObjectVisibilityChanged( LAYER_FP_REFERENCES, isVisible, false );
            onObjectVisibilityChanged( LAYER_FP_VALUES, isVisible, false );
            m_objectSettingsMap[LAYER_FP_REFERENCES]->ctl_visibility->SetValue( isVisible );
            m_objectSettingsMap[LAYER_FP_VALUES]->ctl_visibility->SetValue( isVisible );
        }
        break;

    case LAYER_FP_REFERENCES:
    case LAYER_FP_VALUES:
        // In case that user changes Footprint Value/References when the Footprint Text
        // meta-control is disabled, we should put it back on.
        if( isVisible )
        {
            onObjectVisibilityChanged( LAYER_FP_TEXT, isVisible, false );
            m_objectSettingsMap[LAYER_FP_TEXT]->ctl_visibility->SetValue( isVisible );
        }
        break;

    default:
        break;
    }

    GAL_SET visible = getVisibleObjects();

    if( visible.Contains( aLayer ) != isVisible )
    {
        visible.set( aLayer, isVisible );
        setVisibleObjects( visible );
        m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, isVisible );
        syncLayerPresetSelection();
    }

    if( isFinal )
    {
        m_frame->GetCanvas()->Refresh();
        passOnFocus();
    }
}


void APPEARANCE_CONTROLS::rebuildObjects()
{
    COLOR_SETTINGS* theme   = m_frame->GetColorSettings();
    COLOR4D         bgColor = theme->GetColor( LAYER_PCB_BACKGROUND );
    GAL_SET         visible = getVisibleObjects();
    int             swatchWidth = m_windowObjects->ConvertDialogToPixels( wxSize( 8, 0 ) ).x;
    int             labelWidth = 0;

    int btnWidth = m_visibleBitmapBundle.GetPreferredLogicalSizeFor( m_windowObjects ).x;

    m_objectSettings.clear();
    m_objectsOuterSizer->Clear( true );
    m_objectsOuterSizer->AddSpacer( 5 );

    auto appendObject =
            [&]( const std::unique_ptr<APPEARANCE_SETTING>& aSetting )
            {
                wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
                int         layer = aSetting->id;

                aSetting->visible = visible.Contains( ToGalLayer( layer ) );
                COLOR4D color     = theme->GetColor( layer );
                COLOR4D defColor  = theme->GetDefaultColor( layer );

                if( color != COLOR4D::UNSPECIFIED || defColor != COLOR4D::UNSPECIFIED )
                {
                    COLOR_SWATCH* swatch = new COLOR_SWATCH( m_windowObjects, color, layer,
                                                             bgColor, defColor, SWATCH_SMALL );
                    swatch->SetToolTip( _( "Left double click or middle click for color change, "
                                           "right click for menu" ) );

                    sizer->Add( swatch, 0,  wxALIGN_CENTER_VERTICAL, 0 );
                    aSetting->ctl_color = swatch;

                    swatch->Bind( COLOR_SWATCH_CHANGED, &APPEARANCE_CONTROLS::OnColorSwatchChanged, this );

                    swatch->SetReadOnlyCallback( std::bind( &APPEARANCE_CONTROLS::onReadOnlySwatch, this ) );
                }
                else
                {
                    sizer->AddSpacer( swatchWidth );
                }

                BITMAP_TOGGLE* btn_visible = nullptr;
                wxString tip;

                if( aSetting->can_control_visibility )
                {
                    btn_visible = new BITMAP_TOGGLE( m_windowObjects, layer,
                                                     m_visibleBitmapBundle,
                                                     m_notVisibileBitmapBundle,
                                                     aSetting->visible );

                    tip.Printf( _( "Show or hide %s" ), aSetting->label.Lower() );
                    btn_visible->SetToolTip( tip );

                    aSetting->ctl_visibility = btn_visible;

                    btn_visible->Bind( TOGGLE_CHANGED,
                            [&]( wxCommandEvent& aEvent )
                            {
                                int id = static_cast<wxWindow*>( aEvent.GetEventObject() )->GetId();
                                bool isVisible = aEvent.GetInt();
                                onObjectVisibilityChanged( ToGalLayer( id ), isVisible, true );
                            } );
                }

                sizer->AddSpacer( 5 );

                wxStaticText* label = new wxStaticText( m_windowObjects, layer, aSetting->label );
                label->Wrap( -1 );
                label->SetToolTip( aSetting->tooltip );

                if( aSetting->can_control_opacity )
                {
                    label->SetMinSize( wxSize( labelWidth, -1 ) );
#ifdef __WXMAC__
                    if( btn_visible )
                        sizer->Add( btn_visible, 0, wxALIGN_CENTER_VERTICAL | wxBOTTOM, 10 );
                    else
                        sizer->AddSpacer( btnWidth );

                    sizer->AddSpacer( 5 );
                    sizer->Add( label, 0, wxALIGN_CENTER_VERTICAL | wxBOTTOM, 10 );
#else
                    if( btn_visible )
                        sizer->Add( btn_visible, 0, wxALIGN_CENTER_VERTICAL, 0 );
                    else
                        sizer->AddSpacer( btnWidth );

                    sizer->AddSpacer( 5 );
                    sizer->Add( label, 0, wxALIGN_CENTER_VERTICAL, 0 );
#endif

                    wxSlider* slider = new wxSlider( m_windowObjects, wxID_ANY, 100, 0, 100,
                                                     wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
#ifdef __WXMAC__
                    slider->SetMinSize( wxSize( 80, 16 ) );
#else
                    slider->SetMinSize( wxSize( 80, -1 ) );
#endif

                    tip.Printf( _( "Set opacity of %s" ), aSetting->label.Lower() );
                    slider->SetToolTip( tip );

                    sizer->Add( slider, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5 );
                    aSetting->ctl_opacity = slider;

                    auto opacitySliderHandler =
                            [this, layer]( wxCommandEvent& aEvent )
                            {
                                wxSlider* ctrl = static_cast<wxSlider*>( aEvent.GetEventObject() );
                                int value = ctrl->GetValue();
                                onObjectOpacitySlider( layer, value / 100.0f );
                            };

                    slider->Bind( wxEVT_SCROLL_CHANGED, opacitySliderHandler );
                    slider->Bind( wxEVT_SCROLL_THUMBTRACK, opacitySliderHandler );
                    slider->Bind( wxEVT_SET_FOCUS, &APPEARANCE_CONTROLS::OnSetFocus, this );
                }
                else
                {
                    if( btn_visible )
                        sizer->Add( btn_visible, 0, wxALIGN_CENTER_VERTICAL, 0 );
                    else
                        sizer->AddSpacer( btnWidth );

                    sizer->AddSpacer( 5 );
                    sizer->Add( label, 0, wxALIGN_CENTER_VERTICAL, 0 );
                }

                aSetting->ctl_text = label;
                m_objectsOuterSizer->Add( sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );

                if( !aSetting->can_control_opacity )
                    m_objectsOuterSizer->AddSpacer( 2 );
            };

    for( const APPEARANCE_SETTING& s_setting : s_objectSettings )
    {
        if( m_isFpEditor && !s_allowedInFpEditor.count( s_setting.id ) )
            continue;

        m_objectSettings.emplace_back( std::make_unique<APPEARANCE_SETTING>( s_setting ) );

        std::unique_ptr<APPEARANCE_SETTING>& setting = m_objectSettings.back();

        // Because s_render_rows is created static, we must explicitly call wxGetTranslation
        // for texts which are internationalized (tool tips and item names)
        setting->tooltip = wxGetTranslation( s_setting.tooltip );
        setting->label   = wxGetTranslation( s_setting.label );

        if( setting->can_control_opacity )
        {
            int width = m_windowObjects->GetTextExtent( setting->label ).x + 5;
            labelWidth = std::max( labelWidth, width );
        }

        if( !s_setting.spacer )
            m_objectSettingsMap[ToGalLayer( setting->id )] = setting.get();
    }

    for( const std::unique_ptr<APPEARANCE_SETTING>& setting : m_objectSettings )
    {
        if( setting->spacer )
            m_objectsOuterSizer->AddSpacer( m_pointSize / 2 );
        else
            appendObject( setting );
    }

    m_objectsOuterSizer->Layout();
}


void APPEARANCE_CONTROLS::syncObjectSettings()
{
    GAL_SET visible = getVisibleObjects();

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
              && m_objectSettingsMap.count( LAYER_ZONES )
              && m_objectSettingsMap.count( LAYER_DRAW_BITMAPS )
              && m_objectSettingsMap.count( LAYER_FILLED_SHAPES ) );

    m_objectSettingsMap[LAYER_TRACKS]->ctl_opacity->SetValue( opts.m_TrackOpacity * 100 );
    m_objectSettingsMap[LAYER_VIAS]->ctl_opacity->SetValue( opts.m_ViaOpacity * 100 );
    m_objectSettingsMap[LAYER_PADS]->ctl_opacity->SetValue( opts.m_PadOpacity * 100 );
    m_objectSettingsMap[LAYER_ZONES]->ctl_opacity->SetValue( opts.m_ZoneOpacity * 100 );
    m_objectSettingsMap[LAYER_DRAW_BITMAPS]->ctl_opacity->SetValue( opts.m_ImageOpacity * 100 );
    m_objectSettingsMap[LAYER_FILLED_SHAPES]->ctl_opacity->SetValue( opts.m_FilledShapeOpacity * 100 );
}


void APPEARANCE_CONTROLS::buildNetClassMenu( wxMenu& aMenu, bool isDefaultClass,
                                             const wxString& aName )
{
    BOARD*                         board = m_frame->GetBoard();
    std::shared_ptr<NET_SETTINGS>& netSettings = board->GetDesignSettings().m_NetSettings;

    if( !isDefaultClass)
    {
        aMenu.Append( new wxMenuItem( &aMenu, ID_SET_NET_COLOR, _( "Set Netclass Color" ),
                                      wxEmptyString, wxITEM_NORMAL ) );

        wxMenuItem* schematicColor = new wxMenuItem( &aMenu, ID_USE_SCHEMATIC_NET_COLOR,
                                                     _( "Use Color from Schematic" ),
                                                     wxEmptyString, wxITEM_NORMAL );
        std::shared_ptr<NETCLASS> nc = netSettings->GetNetClassByName( aName );
        const KIGFX::COLOR4D      ncColor = nc->GetSchematicColor();
        aMenu.Append( schematicColor );

        if( ncColor == KIGFX::COLOR4D::UNSPECIFIED )
            schematicColor->Enable( false );

        aMenu.Append( new wxMenuItem( &aMenu, ID_CLEAR_NET_COLOR, _( "Clear Netclass Color" ),
                                      wxEmptyString, wxITEM_NORMAL ) );
        aMenu.AppendSeparator();
    }

    wxString name = UnescapeString( aName );

    aMenu.Append( new wxMenuItem( &aMenu, ID_HIGHLIGHT_NET,
                                  wxString::Format( _( "Highlight Nets in %s" ), name ),
                                  wxEmptyString, wxITEM_NORMAL ) );
    aMenu.Append( new wxMenuItem( &aMenu, ID_SELECT_NET,
                                  wxString::Format( _( "Select Tracks and Vias in %s" ), name ),
                                  wxEmptyString, wxITEM_NORMAL ) );
    aMenu.Append( new wxMenuItem( &aMenu, ID_DESELECT_NET,
                                  wxString::Format( _( "Unselect Tracks and Vias in %s" ), name ),
                                  wxEmptyString, wxITEM_NORMAL ) );

    aMenu.AppendSeparator();

    aMenu.Append( new wxMenuItem( &aMenu, ID_SHOW_ALL_NETS, _( "Show All Netclasses" ),
                                  wxEmptyString, wxITEM_NORMAL ) );
    aMenu.Append( new wxMenuItem( &aMenu, ID_HIDE_OTHER_NETS, _( "Hide All Other Netclasses" ),
                                  wxEmptyString, wxITEM_NORMAL ) );

    aMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &APPEARANCE_CONTROLS::onNetclassContextMenu, this );

}


void APPEARANCE_CONTROLS::rebuildNets()
{
    BOARD*          board   = m_frame->GetBoard();
    COLOR_SETTINGS* theme   = m_frame->GetColorSettings();
    COLOR4D         bgColor = theme->GetColor( LAYER_PCB_BACKGROUND );

    // If the board isn't fully loaded, we can't yet rebuild
    if( !board->GetProject() )
        return;

    m_staticTextNets->SetLabel( _( "Nets" ) );
    m_staticTextNetClasses->SetLabel( _( "Net Classes" ) );

    std::shared_ptr<NET_SETTINGS>& netSettings = board->GetDesignSettings().m_NetSettings;

    const std::set<wxString>& hiddenClasses = m_frame->Prj().GetLocalSettings().m_HiddenNetclasses;

    m_netclassOuterSizer->Clear( true );

    auto appendNetclass =
            [&]( int aId, const std::shared_ptr<NETCLASS>& aClass, bool isDefaultClass = false )
            {
                wxString name = aClass->GetName();

                m_netclassSettings.emplace_back( std::make_unique<APPEARANCE_SETTING>() );
                APPEARANCE_SETTING* setting = m_netclassSettings.back().get();
                m_netclassSettingsMap[name] = setting;

                setting->ctl_panel = new wxPanel( m_netclassScrolledWindow, aId );
                wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
                setting->ctl_panel->SetSizer( sizer );

                COLOR4D color = netSettings->HasNetclass( name )
                                        ? netSettings->GetNetClassByName( name )->GetPcbColor()
                                        : COLOR4D::UNSPECIFIED;

                setting->ctl_color = new COLOR_SWATCH( setting->ctl_panel, color, aId, bgColor,
                                                       COLOR4D::UNSPECIFIED, SWATCH_SMALL );
                setting->ctl_color->SetToolTip( _( "Left double click or middle click for color "
                                                   "change, right click for menu" ) );

                setting->ctl_color->Bind( COLOR_SWATCH_CHANGED,
                                          &APPEARANCE_CONTROLS::onNetclassColorChanged, this );

                // Default netclass can't have an override color
                if( isDefaultClass )
                    setting->ctl_color->Hide();

                setting->ctl_visibility = new BITMAP_TOGGLE( setting->ctl_panel, aId,
                                                             m_visibleBitmapBundle,
                                                             m_notVisibileBitmapBundle,
                                                             !hiddenClasses.count( name ) );

                wxString tip;
                tip.Printf( _( "Show or hide ratsnest for nets in %s" ), name );
                setting->ctl_visibility->SetToolTip( tip );

                setting->ctl_text = new wxStaticText( setting->ctl_panel, aId, name );
                setting->ctl_text->Wrap( -1 );

                int flags = wxALIGN_CENTER_VERTICAL;

                sizer->Add( setting->ctl_color, 0, flags | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );
                sizer->AddSpacer( 7 );
                sizer->Add( setting->ctl_visibility, 0, flags, 5 );
                sizer->AddSpacer( 3 );
                sizer->Add( setting->ctl_text, 1, flags, 5 );

                m_netclassOuterSizer->Add( setting->ctl_panel, 0, wxEXPAND, 5 );
                m_netclassOuterSizer->AddSpacer( 2 );

                setting->ctl_visibility->Bind( TOGGLE_CHANGED,
                                               &APPEARANCE_CONTROLS::onNetclassVisibilityChanged,
                                               this );

                auto menuHandler =
                        [&, name, isDefaultClass]( wxMouseEvent& aEvent )
                        {
                            wxMenu menu;
                            buildNetClassMenu( menu, isDefaultClass, name );

                            m_contextMenuNetclass = name;
                            PopupMenu( &menu );
                        };

                setting->ctl_panel->Bind( wxEVT_RIGHT_DOWN, menuHandler );
                setting->ctl_visibility->Bind( wxEVT_RIGHT_DOWN, menuHandler );
                setting->ctl_color->Bind( wxEVT_RIGHT_DOWN, menuHandler );
                setting->ctl_text->Bind( wxEVT_RIGHT_DOWN, menuHandler );
            };

    std::vector<wxString> names;

    for( const auto& [name, netclass] : netSettings->GetNetclasses() )
        names.emplace_back( name );

    std::sort( names.begin(), names.end() );

    m_netclassIdMap.clear();

    int idx = wxID_HIGHEST;

    m_netclassIdMap[idx] = netSettings->GetDefaultNetclass()->GetName();
    appendNetclass( idx++, netSettings->GetDefaultNetclass(), true );

    for( const wxString& name : names )
    {
        m_netclassIdMap[idx] = name;
        appendNetclass( idx++, netSettings->GetNetclasses().at( name ) );
    }

    int      hotkey;
    wxString msg;

    m_paneNetDisplayOptions->SetLabel( _( "Net Display Options" ) );

    hotkey = PCB_ACTIONS::netColorModeCycle.GetHotKey();

    if( hotkey )
        msg = wxString::Format( _( "Net colors (%s):" ), KeyNameFromKeyCode( hotkey ) );
    else
        msg = _( "Net colors:" );

    m_txtNetDisplayTitle->SetLabel( msg );
    m_txtNetDisplayTitle->SetToolTip( _( "Choose when to show net and netclass colors" ) );

    m_rbNetColorAll->SetLabel( _( "All" ) );
    m_rbNetColorAll->SetToolTip( _( "Net and netclass colors are shown on all copper items" ) );

    m_rbNetColorRatsnest->SetLabel( _( "Ratsnest" ) );
    m_rbNetColorRatsnest->SetToolTip( _( "Net and netclass colors are shown on the ratsnest only" ) );

    m_rbNetColorOff->SetLabel( _( "None" ) );
    m_rbNetColorOff->SetToolTip( _( "Net and netclass colors are not shown" ) );

    hotkey = PCB_ACTIONS::ratsnestModeCycle.GetHotKey();

    if( hotkey )
        msg = wxString::Format( _( "Ratsnest display (%s):" ), KeyNameFromKeyCode( hotkey ) );
    else
        msg = _( "Ratsnest display:" );

    m_txtRatsnestVisibility->SetLabel( msg );
    m_txtRatsnestVisibility->SetToolTip( _( "Choose which ratsnest lines to display" ) );

    m_rbRatsnestAllLayers->SetLabel( _( "All" ) );
    m_rbRatsnestAllLayers->SetToolTip( _( "Show ratsnest lines to items on all layers" ) );

    m_rbRatsnestVisLayers->SetLabel( _( "Visible layers" ) );
    m_rbRatsnestVisLayers->SetToolTip( _( "Show ratsnest lines to items on visible layers" ) );

    m_rbRatsnestNone->SetLabel( _( "None" ) );
    m_rbRatsnestNone->SetToolTip( _( "Hide all ratsnest lines" ) );

    m_netclassOuterSizer->Layout();

    m_netsTable->Rebuild();
    m_panelNets->GetSizer()->Layout();
}


void APPEARANCE_CONTROLS::rebuildLayerPresetsWidget( bool aReset )
{
    m_viewportsLabel->SetLabel( wxString::Format( _( "Presets (%s+Tab):" ),
                                                  KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) );

    m_cbLayerPresets->Clear();

    if( aReset )
        m_presetMRU.clear();

    // Build the layers preset list.
    // By default, the presetAllLayers will be selected
    int idx = 0;
    int default_idx = 0;
    std::vector<std::pair<wxString, void*>> userPresets;

    // m_layerPresets is alphabetical: m_presetMRU should also be alphabetical, but m_cbLayerPresets
    // is split into build-in and user sections.
    for( auto& [name, preset] : m_layerPresets )
    {
        const wxString translatedName = wxGetTranslation( name );
        void*          userData = static_cast<void*>( &preset );

        if( preset.readOnly )
            m_cbLayerPresets->Append( translatedName, userData );
        else
            userPresets.push_back( { name, userData } );

        if( aReset )
            m_presetMRU.push_back( translatedName );

        if( name == presetAllLayers.name )
            default_idx = idx;

        idx++;
    }

    if( !userPresets.empty() )
    {
        m_cbLayerPresets->Append( wxT( "---" ) );

        for( auto& [name, userData] : userPresets )
            m_cbLayerPresets->Append( name, userData );
    }

    m_cbLayerPresets->Append( wxT( "---" ) );
    m_cbLayerPresets->Append( _( "Save preset..." ) );
    m_cbLayerPresets->Append( _( "Delete preset..." ) );

    // At least the built-in presets should always be present
    wxASSERT( !m_layerPresets.empty() );

    if( aReset )
    {
        // Default preset: all layers
        m_cbLayerPresets->SetSelection( default_idx );
        m_currentPreset = &m_layerPresets[presetAllLayers.name];
    }
}


void APPEARANCE_CONTROLS::syncLayerPresetSelection()
{
    LSET    visibleLayers  = getVisibleLayers();
    GAL_SET visibleObjects = getVisibleObjects();
    bool    flipBoard = m_cbFlipBoard->GetValue();

    auto it = std::find_if( m_layerPresets.begin(), m_layerPresets.end(),
                            [&]( const std::pair<const wxString, LAYER_PRESET>& aPair )
                            {
                                return ( aPair.second.layers == visibleLayers
                                         && aPair.second.renderLayers == visibleObjects
                                         && aPair.second.flipBoard == flipBoard );
                            } );

    if( it != m_layerPresets.end() )
    {
        // Select the right m_cbLayersPresets item.
        // but these items are translated if they are predefined items.
        bool do_translate = it->second.readOnly;
        wxString text = do_translate ? wxGetTranslation( it->first ) : it->first;

        m_cbLayerPresets->SetStringSelection( text );
    }
    else
    {
        m_cbLayerPresets->SetSelection( m_cbLayerPresets->GetCount() - 3 ); // separator
    }

    m_currentPreset = static_cast<LAYER_PRESET*>( m_cbLayerPresets->GetClientData( m_cbLayerPresets->GetSelection() ) );
}


void APPEARANCE_CONTROLS::updateLayerPresetSelection( const wxString& aName )
{
    // look at m_layerPresets to know if aName is a read only preset, or a user preset.
    // Read only presets have translated names in UI, so we have to use
    // a translated name in UI selection.
    // But for a user preset name we should search for aName (not translated)
    wxString ui_label = aName;

    for( std::pair<const wxString, LAYER_PRESET>& pair : m_layerPresets )
    {
        if( pair.first != aName )
            continue;

        if( pair.second.readOnly == true )
            ui_label = wxGetTranslation( aName );

        break;
    }

    int idx = m_cbLayerPresets->FindString( ui_label );

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
    int count = m_cbLayerPresets->GetCount();
    int index = m_cbLayerPresets->GetSelection();

    auto resetSelection =
            [&]()
            {
                if( m_currentPreset )
                    m_cbLayerPresets->SetStringSelection( m_currentPreset->name );
                else
                    m_cbLayerPresets->SetSelection( m_cbLayerPresets->GetCount() - 3 );
            };

    if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedUserPreset )
            name = m_lastSelectedUserPreset->name;

        wxTextEntryDialog dlg( wxGetTopLevelParent( this ), _( "Layer preset name:" ),
                               _( "Save Layer Preset" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        name = dlg.GetValue();
        bool exists = m_layerPresets.count( name );

        if( !exists )
        {
            m_layerPresets[name] = LAYER_PRESET( name, getVisibleLayers(), getVisibleObjects(),
                                                 UNSELECTED_LAYER, m_cbFlipBoard->GetValue() );
        }

        LAYER_PRESET* preset = &m_layerPresets[name];

        if( !exists )
        {
            rebuildLayerPresetsWidget( false );
            index = m_cbLayerPresets->FindString( name );
        }
        else if( preset->readOnly )
        {
            wxMessageBox( _( "Default presets cannot be modified.\nPlease use a different name." ),
                          _( "Error" ), wxOK | wxICON_ERROR, wxGetTopLevelParent( this ) );
            resetSelection();
            return;
        }
        else
        {
            // Ask the user if they want to overwrite the existing preset
            if( !IsOK( wxGetTopLevelParent( this ), _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            preset->layers       = getVisibleLayers();
            preset->renderLayers = getVisibleObjects();
            preset->flipBoard    = m_cbFlipBoard->GetValue();

            index = m_cbLayerPresets->FindString( name );

            if( m_presetMRU.Index( name ) != wxNOT_FOUND )
                m_presetMRU.Remove( name );
        }

        m_currentPreset = preset;
        m_cbLayerPresets->SetSelection( index );
        m_presetMRU.Insert( name, 0 );

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

        EDA_LIST_DIALOG dlg( m_frame, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();
            int idx = m_cbLayerPresets->FindString( presetName );

            if( idx != wxNOT_FOUND )
            {
                m_layerPresets.erase( presetName );

                m_cbLayerPresets->Delete( idx );
                m_currentPreset = nullptr;
            }

            if( m_presetMRU.Index( presetName ) != wxNOT_FOUND )
                m_presetMRU.Remove( presetName );
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

    // Store the objects visibility settings if the preset is not a user preset,
    // to be reused when selecting a new built-in layer preset, even if a previous
    // user preset has changed the object visibility
    if( !m_currentPreset || m_currentPreset->readOnly )
    {
        m_lastBuiltinPreset.renderLayers = getVisibleObjects();
    }

    LAYER_PRESET* preset = static_cast<LAYER_PRESET*>( m_cbLayerPresets->GetClientData( index ) );
    m_currentPreset = preset;

    m_lastSelectedUserPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
    {
        // Change board layers visibility, but do not change objects visibility
        LAYER_PRESET curr_layers_choice = *preset;

        // For predefined presets that do not manage objects visibility, use
        // the objects visibility settings of the last used predefined preset.
        if( curr_layers_choice.readOnly )
            curr_layers_choice.renderLayers = m_lastBuiltinPreset.renderLayers;

        doApplyLayerPreset( curr_layers_choice );
    }

    if( !m_currentPreset->name.IsEmpty() )
    {
        const wxString translatedName = wxGetTranslation( m_currentPreset->name );

        if( m_presetMRU.Index( translatedName ) != wxNOT_FOUND )
            m_presetMRU.Remove( translatedName );

        m_presetMRU.Insert( translatedName, 0 );
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS::doApplyLayerPreset( const LAYER_PRESET& aPreset )
{
    BOARD*           board = m_frame->GetBoard();
    KIGFX::PCB_VIEW* view = m_frame->GetCanvas()->GetView();

    setVisibleLayers( aPreset.layers );
    setVisibleObjects( aPreset.renderLayers );

    // If the preset doesn't have an explicit active layer to restore, we can at least
    // force the active layer to be something in the preset's layer set
    PCB_LAYER_ID activeLayer = UNSELECTED_LAYER;

    if( aPreset.activeLayer != UNSELECTED_LAYER )
        activeLayer = aPreset.activeLayer;
    else if( aPreset.layers.any() && !aPreset.layers.test( m_frame->GetActiveLayer() ) )
        activeLayer = *aPreset.layers.Seq().begin();

    LSET boardLayers = board->GetLayerSet();

    if( activeLayer != UNSELECTED_LAYER && boardLayers.Contains( activeLayer ) )
        m_frame->SetActiveLayer( activeLayer );

    if( !m_isFpEditor )
        m_frame->GetCanvas()->SyncLayersVisibility( board );

    if( aPreset.flipBoard != view->IsMirroredX() )
    {
        view->SetMirror( !view->IsMirroredX(), view->IsMirroredY() );
        view->RecacheAllItems();
    }

    m_frame->GetCanvas()->Refresh();

    syncColorsAndVisibility();
    UpdateDisplayOptions();
}


void APPEARANCE_CONTROLS::rebuildViewportsWidget()
{
    m_viewportsLabel->SetLabel( wxString::Format( _( "Viewports (%s+Tab):" ),
                                                  KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ) ) );

    m_cbViewports->Clear();

    for( std::pair<const wxString, VIEWPORT>& pair : m_viewports )
        m_cbViewports->Append( pair.first, static_cast<void*>( &pair.second ) );

    m_cbViewports->Append( wxT( "---" ) );
    m_cbViewports->Append( _( "Save viewport..." ) );
    m_cbViewports->Append( _( "Delete viewport..." ) );

    m_cbViewports->SetSelection( m_cbViewports->GetCount() - 3 );
    m_lastSelectedViewport = nullptr;
}


void APPEARANCE_CONTROLS::updateViewportSelection( const wxString& aName )
{
    int idx = m_cbViewports->FindString( aName );

    if( idx >= 0 && idx < (int)m_cbViewports->GetCount() - 3 /* separator */ )
    {
        m_cbViewports->SetSelection( idx );
        m_lastSelectedViewport = static_cast<VIEWPORT*>( m_cbViewports->GetClientData( idx ) );
    }
    else if( idx < 0 )
    {
        m_cbViewports->SetSelection( m_cbViewports->GetCount() - 3 ); // separator
        m_lastSelectedViewport = nullptr;
    }
}


void APPEARANCE_CONTROLS::onViewportChanged( wxCommandEvent& aEvent )
{
    int count = m_cbViewports->GetCount();
    int index = m_cbViewports->GetSelection();

    if( index >= 0 && index < count - 3 )
    {
        VIEWPORT* viewport = static_cast<VIEWPORT*>( m_cbViewports->GetClientData( index ) );

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

        wxTextEntryDialog dlg( wxGetTopLevelParent( this ), _( "Viewport name:" ), _( "Save Viewport" ), name );

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
            m_viewports[name] = VIEWPORT( name, m_frame->GetCanvas()->GetView()->GetViewport() );

            index = m_cbViewports->Insert( name, index-1, static_cast<void*>( &m_viewports[name] ) );
        }
        else
        {
            m_viewports[name].rect = m_frame->GetCanvas()->GetView()->GetViewport();
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

        for( std::pair<const wxString, VIEWPORT>& pair : m_viewports )
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


void APPEARANCE_CONTROLS::doApplyViewport( const VIEWPORT& aViewport )
{
    m_frame->GetCanvas()->GetView()->SetViewport( aViewport.rect );
    m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::OnColorSwatchChanged( wxCommandEvent& aEvent )
{
    COLOR_SWATCH* swatch   = static_cast<COLOR_SWATCH*>( aEvent.GetEventObject() );
    COLOR4D       newColor = swatch->GetSwatchColor();
    int           layer    = swatch->GetId();

    COLOR_SETTINGS* cs = m_frame->GetColorSettings();

    cs->SetColor( layer, newColor );
    m_frame->GetSettingsManager()->SaveColorSettings( cs, "board" );

    m_frame->GetCanvas()->UpdateColors();

    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
    view->UpdateLayerColor( layer );
    view->UpdateLayerColor( GetNetnameLayer( layer ) );

    if( IsCopperLayer( layer ) )
    {
        view->UpdateLayerColor( ZONE_LAYER_FOR( layer ) );
        view->UpdateLayerColor( VIA_COPPER_LAYER_FOR( layer ) );
        view->UpdateLayerColor( PAD_COPPER_LAYER_FOR( layer ) );
        view->UpdateLayerColor( CLEARANCE_LAYER_FOR( layer ) );
    }

    // Update the bitmap of the layer box
    if( m_frame->IsType( FRAME_PCB_EDITOR ) )
        static_cast<PCB_EDIT_FRAME*>( m_frame )->ReCreateLayerBox( false );

    m_frame->GetCanvas()->Refresh();

    if( layer == LAYER_PCB_BACKGROUND )
        m_frame->SetDrawBgColor( newColor );

    passOnFocus();
}


void APPEARANCE_CONTROLS::onObjectOpacitySlider( int aLayer, float aOpacity )
{
    PCB_DISPLAY_OPTIONS options = m_frame->GetDisplayOptions();

    switch( aLayer )
    {
    case static_cast<int>( LAYER_TRACKS ):        options.m_TrackOpacity       = aOpacity; break;
    case static_cast<int>( LAYER_VIAS ):          options.m_ViaOpacity         = aOpacity; break;
    case static_cast<int>( LAYER_PADS ):          options.m_PadOpacity         = aOpacity; break;
    case static_cast<int>( LAYER_ZONES ):         options.m_ZoneOpacity        = aOpacity; break;
    case static_cast<int>( LAYER_DRAW_BITMAPS ):  options.m_ImageOpacity       = aOpacity; break;
    case static_cast<int>( LAYER_FILLED_SHAPES ): options.m_FilledShapeOpacity = aOpacity; break;
    default: return;
    }

    m_frame->SetDisplayOptions( options );
    passOnFocus();
}


void APPEARANCE_CONTROLS::onNetContextMenu( wxCommandEvent& aEvent )
{
    wxASSERT( m_netsGrid->GetSelectedRows().size() == 1 );

    int row = m_netsGrid->GetSelectedRows()[0];
    NET_GRID_ENTRY& net = m_netsTable->GetEntry( row );

    m_netsGrid->ClearSelection();

    switch( aEvent.GetId() )
    {
    case ID_SET_NET_COLOR:
    {
        wxGridCellEditor* editor = m_netsGrid->GetCellEditor( row, NET_GRID_TABLE::COL_COLOR );
        editor->BeginEdit( row, NET_GRID_TABLE::COL_COLOR, m_netsGrid );
        break;
    }

    case ID_CLEAR_NET_COLOR:
        m_netsGrid->SetCellValue( row, NET_GRID_TABLE::COL_COLOR, wxS( "rgba(0,0,0,0)" ) );
        break;

    case ID_HIGHLIGHT_NET:
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::highlightNet, net.code );
        m_frame->GetCanvas()->Refresh();
        break;

    case ID_SELECT_NET:
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectNet, net.code );
        m_frame->GetCanvas()->Refresh();
        break;

    case ID_DESELECT_NET:
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::deselectNet, net.code );
        m_frame->GetCanvas()->Refresh();
        break;

    case ID_SHOW_ALL_NETS:
        m_netsTable->ShowAllNets();
        break;

    case ID_HIDE_OTHER_NETS:
        m_netsTable->HideOtherNets( net );
        break;

    default:
        break;
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS::onNetclassVisibilityChanged( wxCommandEvent& aEvent )
{
    wxString className = netclassNameFromEvent( aEvent );
    bool     show      = aEvent.GetInt();
    showNetclass( className, show );
    passOnFocus();
}


void APPEARANCE_CONTROLS::showNetclass( const wxString& aClassName, bool aShow )
{
    m_togglingNetclassRatsnestVisibility = true;

    for( NETINFO_ITEM* net : m_frame->GetBoard()->GetNetInfo() )
    {
        if( net->GetNetClass()->ContainsNetclassWithName( aClassName ) )
        {
            m_frame->GetToolManager()->RunAction( aShow ? PCB_ACTIONS::showNetInRatsnest
                                                        : PCB_ACTIONS::hideNetInRatsnest,
                                                  net->GetNetCode() );

            int row = m_netsTable->GetRowByNetcode( net->GetNetCode() );

            if( row >= 0 )
                m_netsTable->SetValueAsBool( row, NET_GRID_TABLE::COL_VISIBILITY, aShow );
        }
    }

    PROJECT_LOCAL_SETTINGS& localSettings = m_frame->Prj().GetLocalSettings();

    if( !aShow )
        localSettings.m_HiddenNetclasses.insert( aClassName );
    else
        localSettings.m_HiddenNetclasses.erase( aClassName );

    m_netsGrid->ForceRefresh();
    m_frame->GetCanvas()->RedrawRatsnest();
    m_frame->GetCanvas()->Refresh();
    m_togglingNetclassRatsnestVisibility = false;
}


void APPEARANCE_CONTROLS::onNetclassColorChanged( wxCommandEvent& aEvent )
{
    COLOR_SWATCH* swatch = static_cast<COLOR_SWATCH*>( aEvent.GetEventObject() );
    wxString      netclassName = netclassNameFromEvent( aEvent );

    BOARD*                         board = m_frame->GetBoard();
    std::shared_ptr<NET_SETTINGS>& netSettings = board->GetDesignSettings().m_NetSettings;
    std::shared_ptr<NETCLASS>      nc = netSettings->GetNetClassByName( netclassName );

    nc->SetPcbColor( swatch->GetSwatchColor() );
    netSettings->RecomputeEffectiveNetclasses();

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


void APPEARANCE_CONTROLS::onNetColorMode( wxCommandEvent& aEvent )
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
    passOnFocus();
}


void APPEARANCE_CONTROLS::onRatsnestMode( wxCommandEvent& aEvent )
{
    if( PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings() )
    {
        if( m_rbRatsnestAllLayers->GetValue() )
        {
            cfg->m_Display.m_ShowGlobalRatsnest = true;
            cfg->m_Display.m_RatsnestMode = RATSNEST_MODE::ALL;
        }
        else if( m_rbRatsnestVisLayers->GetValue() )
        {
            cfg->m_Display.m_ShowGlobalRatsnest = true;
            cfg->m_Display.m_RatsnestMode = RATSNEST_MODE::VISIBLE;
        }
        else
        {
            cfg->m_Display.m_ShowGlobalRatsnest = false;
        }
    }

    if( PCB_EDIT_FRAME* editframe = dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) )
    {
        if( PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings() )
            editframe->SetElementVisibility( LAYER_RATSNEST, cfg->m_Display.m_ShowGlobalRatsnest );

        editframe->OnDisplayOptionsChanged();
        editframe->GetCanvas()->RedrawRatsnest();
        editframe->GetCanvas()->Refresh();
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS::onNetclassContextMenu( wxCommandEvent& aEvent )
{
    KIGFX::VIEW*                   view = m_frame->GetCanvas()->GetView();
    KIGFX::PCB_RENDER_SETTINGS*    rs =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    BOARD*                         board = m_frame->GetBoard();
    std::shared_ptr<NET_SETTINGS>& netSettings = board->GetDesignSettings().m_NetSettings;
    APPEARANCE_SETTING*            setting = nullptr;

    auto it = m_netclassSettingsMap.find( m_contextMenuNetclass );

    if( it != m_netclassSettingsMap.end() )
        setting = it->second;

    auto runOnNetsOfClass =
            [&]( const wxString& netClassName, std::function<void( NETINFO_ITEM* )> aFunction )
            {
                for( NETINFO_ITEM* net : board->GetNetInfo() )
                {
                    if( net->GetNetClass()->ContainsNetclassWithName( netClassName ) )
                        aFunction( net );
                }
            };

    switch( aEvent.GetId() )
    {
        case ID_SET_NET_COLOR:
        {
            if( setting )
            {
                setting->ctl_color->GetNewSwatchColor();

                COLOR4D color = setting->ctl_color->GetSwatchColor();

                if( color != COLOR4D::UNSPECIFIED )
                {
                    netSettings->GetNetClassByName( m_contextMenuNetclass )->SetPcbColor( color );
                    netSettings->RecomputeEffectiveNetclasses();
                }

                view->UpdateAllLayersColor();
            }

            break;
        }

        case ID_CLEAR_NET_COLOR:
        {
            if( setting )
            {
                setting->ctl_color->SetSwatchColor( COLOR4D( 0, 0, 0, 0 ), true );

                netSettings->GetNetClassByName( m_contextMenuNetclass )->SetPcbColor( COLOR4D::UNSPECIFIED );
                netSettings->RecomputeEffectiveNetclasses();

                view->UpdateAllLayersColor();
            }

            break;
        }

        case ID_USE_SCHEMATIC_NET_COLOR:
        {
            if( setting )
            {
                std::shared_ptr<NETCLASS> nc = netSettings->GetNetClassByName( m_contextMenuNetclass );
                const KIGFX::COLOR4D ncColor = nc->GetSchematicColor();

                setting->ctl_color->SetSwatchColor( ncColor, true );

                netSettings->GetNetClassByName( m_contextMenuNetclass )->SetPcbColor( ncColor );
                netSettings->RecomputeEffectiveNetclasses();

                view->UpdateAllLayersColor();
            }

            break;
        }

        case ID_HIGHLIGHT_NET:
        {
            if( !m_contextMenuNetclass.IsEmpty() )
            {
                runOnNetsOfClass( m_contextMenuNetclass,
                        [&]( NETINFO_ITEM* aItem )
                        {
                            static bool first = true;
                            int code = aItem->GetNetCode();

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
                        } );

                view->UpdateAllLayersColor();
                board->HighLightON();
            }

            break;
        }

        case ID_SELECT_NET:
        case ID_DESELECT_NET:
        {
            if( !m_contextMenuNetclass.IsEmpty() )
            {
                TOOL_MANAGER* toolMgr = m_frame->GetToolManager();
                TOOL_ACTION&  action = aEvent.GetId() == ID_SELECT_NET ? PCB_ACTIONS::selectNet
                                                                       : PCB_ACTIONS::deselectNet;

                runOnNetsOfClass( m_contextMenuNetclass,
                        [&]( NETINFO_ITEM* aItem )
                        {
                            toolMgr->RunAction( action, aItem->GetNetCode() );
                        } );
            }
            break;
        }


        case ID_SHOW_ALL_NETS:
        {
            showNetclass( NETCLASS::Default );
            wxASSERT( m_netclassSettingsMap.count( NETCLASS::Default ) );
            m_netclassSettingsMap.at( NETCLASS::Default )->ctl_visibility->SetValue( true );

            for( const auto& [name, netclass] : netSettings->GetNetclasses() )
            {
                showNetclass( name );

                if( m_netclassSettingsMap.count( name ) )
                    m_netclassSettingsMap.at( name )->ctl_visibility->SetValue( true );
            }

            break;
        }

        case ID_HIDE_OTHER_NETS:
        {
            bool showDefault = m_contextMenuNetclass == NETCLASS::Default;
            showNetclass( NETCLASS::Default, showDefault );
            wxASSERT( m_netclassSettingsMap.count( NETCLASS::Default ) );
            m_netclassSettingsMap.at( NETCLASS::Default )->ctl_visibility->SetValue( showDefault );

            for( const auto& [name, netclass] : netSettings->GetNetclasses() )
            {
                bool show = ( name == m_contextMenuNetclass );

                showNetclass( name, show );

                if( m_netclassSettingsMap.count( name ) )
                    m_netclassSettingsMap.at( name )->ctl_visibility->SetValue( show );
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


void APPEARANCE_CONTROLS::passOnFocus()
{
    m_focusOwner->SetFocus();
}


void APPEARANCE_CONTROLS::onReadOnlySwatch()
{
    WX_INFOBAR* infobar = m_frame->GetInfoBar();

    wxHyperlinkCtrl* button = new wxHyperlinkCtrl( infobar, wxID_ANY, _( "Open Preferences" ), wxEmptyString );

    button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
            [&]( wxHyperlinkEvent& aEvent )
            {
                 m_frame->ShowPreferences( wxEmptyString, wxEmptyString );
            } ) );

    infobar->RemoveAllButtons();
    infobar->AddButton( button );
    infobar->AddCloseButton();

    infobar->ShowMessageFor( _( "The current color theme is read-only.  Create a new theme in Preferences to "
                                "enable color editing." ),
                             10000, wxICON_INFORMATION );
}


void APPEARANCE_CONTROLS::RefreshCollapsiblePanes()
{
    m_paneLayerDisplayOptions->Refresh();
}


bool APPEARANCE_CONTROLS::IsTogglingNetclassRatsnestVisibility()
{
    return m_togglingNetclassRatsnestVisibility;
}
