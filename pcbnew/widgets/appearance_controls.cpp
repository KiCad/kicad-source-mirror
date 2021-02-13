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
#include <board.h>
#include <dialog_helpers.h>
#include <footprint_edit_frame.h>
#include <menus_helpers.h>
#include <pcb_display_options.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <settings/color_settings.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <widgets/bitmap_button.h>
#include <widgets/bitmap_toggle.h>
#include <widgets/collapsible_pane.h>
#include <widgets/color_swatch.h>
#include <widgets/grid_bitmap_toggle.h>
#include <widgets/grid_color_swatch_helpers.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/indicator_icon.h>
#include <widgets/infobar.h>
#include <widgets/wx_grid.h>
#include <wx/hyperlink.h>
#include <wx/statline.h>


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
    switch( aCol )
    {
    case COL_COLOR:
        m_defaultAttr->IncRef();
        return m_defaultAttr;

    case COL_VISIBILITY:
        m_defaultAttr->IncRef();
        return m_defaultAttr;

    case COL_LABEL:
        m_labelAttr->IncRef();
        return m_labelAttr;

    default:
        wxFAIL;
        return nullptr;
    }
}


wxString NET_GRID_TABLE::GetValue( int aRow, int aCol )
{
    wxASSERT( static_cast<size_t>( aRow ) < m_nets.size() );

    switch( aCol )
    {
    case COL_COLOR:
        return m_nets[aRow].color.ToWxString( wxC2S_CSS_SYNTAX );

    case COL_VISIBILITY:
        return m_nets[aRow].visible ? "1" : "0";

    case COL_LABEL:
        return m_nets[aRow].name;

    default:
        return wxEmptyString;
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
        net.visible = ( aValue != "0" );
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
    case COL_COLOR:
        return wxT( "COLOR4D" );

    case COL_VISIBILITY:
        return wxGRID_VALUE_BOOL;

    case COL_LABEL:
    default:
        return wxGRID_VALUE_STRING;
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

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_REQUEST_VIEW_GET_VALUES );
        GetView()->ProcessTableMessage( msg );
    }
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
    BOARD*              board = m_frame->GetBoard();
    const NETNAMES_MAP& nets  = board->GetNetInfo().NetsByName();

    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings() );

    std::set<int>&                 hiddenNets = rs->GetHiddenNets();
    std::map<int, KIGFX::COLOR4D>& netColors  = rs->GetNetColorMap();

    int deleted = m_nets.size();
    m_nets.clear();

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, deleted );
        GetView()->ProcessTableMessage( msg );
    }

    for( const std::pair<const wxString, NETINFO_ITEM*>& pair : nets )
    {
        int netCode = pair.second->GetNetCode();

        if( netCode > 0 && !pair.first.StartsWith( "unconnected-(" ) )
        {
            COLOR4D color = netColors.count( netCode ) ? netColors.at( netCode ) :
                            COLOR4D::UNSPECIFIED;

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
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_nets.size() );
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
    const TOOL_ACTION& action = aNet.visible ? PCB_ACTIONS::showNet : PCB_ACTIONS::hideNet;
    m_frame->GetToolManager()->RunAction( action, true, aNet.code );
}


void NET_GRID_TABLE::updateNetColor( const NET_GRID_ENTRY& aNet )
{
    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings() );

    std::map<int, KIGFX::COLOR4D>& netColors  = rs->GetNetColorMap();

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

    //     text                  id                        tooltip                   opacity slider
    RR( _( "Tracks" ),           LAYER_TRACKS,             _( "Show tracks" ),       true ),
    RR( _( "Vias" ),             LAYER_VIAS,               _( "Show all vias" ),     true ),
    RR( _( "Pads" ),             LAYER_PADS,               _( "Show all pads" ),     true ),
    RR( _( "Zones" ),            LAYER_ZONES,              _( "Show copper zones" ), true ),
    RR(),
    RR( _( "Footprints Front" ), LAYER_MOD_FR,             _( "Show footprints that are on board's front" ) ),
    RR( _( "Footprints Back" ),  LAYER_MOD_BK,             _( "Show footprints that are on board's back" ) ),
    RR( _( "Through-hole Pads" ),LAYER_PADS_TH,            _( "Show through-hole pads" ) ),
    RR( _( "Values" ),           LAYER_MOD_VALUES,         _( "Show footprint values" ) ),
    RR( _( "References" ),       LAYER_MOD_REFERENCES,     _( "Show footprint references" ) ),
    RR( _( "Footprint Text" ),   LAYER_MOD_TEXT_FR,        _( "Show all footprint text" ) ),
    RR( _( "Hidden Text" ),      LAYER_MOD_TEXT_INVISIBLE, _( "Show footprint text marked as invisible" ) ),
    RR(),
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

/// These GAL layers are shown in the Objects tab in the footprint editor
static std::set<int> s_allowedInFpEditor =
        {
            LAYER_TRACKS,
            LAYER_VIAS,
            LAYER_PADS,
            LAYER_ZONES,
            LAYER_PADS_TH,
            LAYER_MOD_VALUES,
            LAYER_MOD_REFERENCES,
            LAYER_MOD_TEXT_INVISIBLE,
            LAYER_GRID
        };

// These are the built-in layer presets that cannot be deleted

LAYER_PRESET APPEARANCE_CONTROLS::presetNoLayers( _( "No Layers" ), LSET() );

LAYER_PRESET APPEARANCE_CONTROLS::presetAllLayers( _( "All Layers" ), LSET::AllLayersMask() );

LAYER_PRESET APPEARANCE_CONTROLS::presetAllCopper( _( "All Copper Layers" ),
        LSET::AllCuMask().set( Edge_Cuts ) );

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
        m_focusOwner( aFocusOwner ),
        m_board( nullptr ),
        m_isFpEditor( aFpEditorMode ),
        m_currentPreset( nullptr ),
        m_lastSelectedUserPreset( nullptr ),
        m_layerContextMenu( nullptr )
{
    int indicatorSize = ConvertDialogToPixels( wxSize( 6, 6 ) ).x;
    m_iconProvider    = new ROW_ICON_PROVIDER( indicatorSize );
    int pointSize     = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ).GetPointSize();
    int screenHeight  = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );

    m_layerPanelColour = m_panelLayers->GetBackgroundColour().ChangeLightness( 110 );

    m_layersOuterSizer = new wxBoxSizer( wxVERTICAL );
    m_windowLayers->SetSizer( m_layersOuterSizer );
    m_windowLayers->SetScrollRate( 0, 5 );
    m_windowLayers->Bind( wxEVT_SET_FOCUS, &APPEARANCE_CONTROLS::OnSetFocus, this );

    m_objectsOuterSizer = new wxBoxSizer( wxVERTICAL );
    m_windowObjects->SetSizer( m_objectsOuterSizer );
    m_windowObjects->SetScrollRate( 0, 5 );
    m_windowObjects->Bind( wxEVT_SET_FOCUS, &APPEARANCE_CONTROLS::OnSetFocus, this );

    createControls();

    m_btnNetInspector->SetBitmap( KiBitmap( list_nets_16_xpm ) );
    m_btnNetInspector->SetPadding( 2 );

    m_btnConfigureNetClasses->SetBitmap( KiBitmap( options_generic_16_xpm ) );
    m_btnConfigureNetClasses->SetPadding( 2 );

    m_txtNetFilter->SetHint( _( "Filter nets" ) );

    if( screenHeight <= 900 && pointSize >= indicatorSize )
        pointSize = pointSize * 8 / 10;

    m_pointSize = pointSize;
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
                m_frame->GetToolManager()->RunAction( PCB_ACTIONS::listNets, true );
                passOnFocus();
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
                m_frame->GetToolManager()->RunAction( PCB_ACTIONS::flipBoard, true );
            } );

    m_toggleGridRenderer = new GRID_BITMAP_TOGGLE_RENDERER( KiBitmap( visibility_xpm ),
                                                            KiBitmap( visibility_off_xpm ) );

    m_netsGrid->RegisterDataType( wxT( "bool" ), m_toggleGridRenderer, new wxGridCellBoolEditor );

    // TODO(JE) Update background color of swatch renderer when theme changes
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

    size = KiBitmap( visibility_xpm ).GetSize();
    m_netsGrid->SetColSize( NET_GRID_TABLE::COL_VISIBILITY, size.x + cellPadding );

    m_netsGrid->SetDefaultCellFont( font );
    m_netsGrid->SetDefaultRowSize( font.GetPixelSize().y + rowHeightPadding );

    m_netsGrid->GetGridWindow()->Bind( wxEVT_MOTION,
                                       &APPEARANCE_CONTROLS::OnNetGridMouseEvent, this );

    // To handle middle click on color swatches
    m_netsGrid->GetGridWindow()->Bind( wxEVT_MIDDLE_UP,
                                       &APPEARANCE_CONTROLS::OnNetGridMouseEvent, this );

    m_netsGrid->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT );
    m_netclassScrolledWindow->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT );

    if( m_isFpEditor )
        m_notebook->RemovePage( 2 );

    loadDefaultLayerPresets();
    rebuildObjects();
    OnBoardChanged();

    Bind( wxEVT_COMMAND_MENU_SELECTED, &APPEARANCE_CONTROLS::OnLayerContextMenu, this,
          ID_CHANGE_COLOR, ID_LAST_VALUE );
}


APPEARANCE_CONTROLS::~APPEARANCE_CONTROLS()
{
    delete m_iconProvider;
}


void APPEARANCE_CONTROLS::createControls()
{
    // Create layer display options
    m_paneLayerDisplayOptions = new WX_COLLAPSIBLE_PANE( m_panelLayers, wxID_ANY,
                                                         _( "Layer Display Options" ) );
    m_paneLayerDisplayOptions->Collapse();
    m_paneLayerDisplayOptions->SetBackgroundColour( m_notebook->GetThemeBackgroundColour() );

    wxWindow* layerDisplayPane = m_paneLayerDisplayOptions->GetPane();

    wxBoxSizer* layerDisplayOptionsSizer;
    layerDisplayOptionsSizer = new wxBoxSizer( wxVERTICAL );

    m_staticTextContrastModeTitle = new wxStaticText( layerDisplayPane, wxID_ANY,
                                                      _( "Non-active layers:" ), wxDefaultPosition,
                                                      wxDefaultSize, 0 );
    m_staticTextContrastModeTitle->Wrap( -1 );
    layerDisplayOptionsSizer->Add( m_staticTextContrastModeTitle, 0,
                                   wxEXPAND | wxBOTTOM | wxLEFT, 2 );

    wxBoxSizer* contrastModeSizer;
    contrastModeSizer = new wxBoxSizer( wxHORIZONTAL );

    m_rbHighContrastNormal = new wxRadioButton( layerDisplayPane, wxID_ANY, _( "Normal" ),
                                                wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_rbHighContrastNormal->SetValue( true );
    m_rbHighContrastNormal->SetToolTip( _( "Non-active layers will be shown in full color" ) );

    contrastModeSizer->Add( m_rbHighContrastNormal, 0, wxRIGHT, 4 );

    m_rbHighContrastDim = new wxRadioButton( layerDisplayPane, wxID_ANY, _( "Dim" ),
                                             wxDefaultPosition, wxDefaultSize, 0 );
    m_rbHighContrastDim->SetToolTip( _( "Non-active layers will be dimmed" ) );

    contrastModeSizer->Add( m_rbHighContrastDim, 0, wxRIGHT | wxLEFT, 10 );

    m_rbHighContrastOff = new wxRadioButton( layerDisplayPane, wxID_ANY, _( "Hide" ),
                                             wxDefaultPosition, wxDefaultSize, 0 );
    m_rbHighContrastOff->SetToolTip( _( "Non-active layers will be hidden" ) );

    contrastModeSizer->Add( m_rbHighContrastOff, 0, 0, 5 );

    layerDisplayOptionsSizer->Add( contrastModeSizer, 0, wxEXPAND, 5 );

    m_layerDisplaySeparator = new wxStaticLine( layerDisplayPane, wxID_ANY, wxDefaultPosition,
                                                wxDefaultSize, wxLI_HORIZONTAL );
    layerDisplayOptionsSizer->Add( m_layerDisplaySeparator, 0, wxEXPAND | wxTOP | wxBOTTOM, 5 );

    m_cbFlipBoard = new wxCheckBox( layerDisplayPane, wxID_ANY, _( "Flip board view" ),
                                    wxDefaultPosition, wxDefaultSize, 0 );
    layerDisplayOptionsSizer->Add( m_cbFlipBoard, 0, 0, 5 );

    layerDisplayPane->SetSizer( layerDisplayOptionsSizer );
    layerDisplayPane->Layout();
    layerDisplayOptionsSizer->Fit( layerDisplayPane );

    m_panelLayersSizer->Add( m_paneLayerDisplayOptions, 0, wxEXPAND | wxTOP, 5 );

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

    m_txtNetDisplayTitle = new wxStaticText( netDisplayPane, wxID_ANY, _( "Net colors:" ),
                                             wxDefaultPosition, wxDefaultSize, 0 );
    m_txtNetDisplayTitle->Wrap( -1 );
    m_txtNetDisplayTitle->SetToolTip( _( "Choose when to show net and netclass colors" ) );

    netDisplayOptionsSizer->Add( m_txtNetDisplayTitle, 0, wxEXPAND | wxBOTTOM | wxLEFT, 2 );

    wxBoxSizer* netColorSizer = new wxBoxSizer( wxHORIZONTAL );

    m_rbNetColorAll = new wxRadioButton( netDisplayPane, wxID_ANY, _( "All" ), wxDefaultPosition,
                                         wxDefaultSize, wxRB_GROUP );
    m_rbNetColorAll->SetToolTip( _( "Net and netclass colors are shown on all copper items" ) );

    netColorSizer->Add( m_rbNetColorAll, 0, wxRIGHT, 10 );

    m_rbNetColorRatsnest = new wxRadioButton( netDisplayPane, wxID_ANY, _( "Ratsnest" ),
                                              wxDefaultPosition, wxDefaultSize, 0 );
    m_rbNetColorRatsnest->SetValue( true );
    m_rbNetColorRatsnest->SetToolTip( _( "Net and netclass colors are shown on the ratsnest only" ) );

    netColorSizer->Add( m_rbNetColorRatsnest, 0, wxRIGHT, 4 );

    m_rbNetColorOff = new wxRadioButton( netDisplayPane, wxID_ANY, _( "None" ), wxDefaultPosition,
                                         wxDefaultSize, 0 );
    m_rbNetColorOff->SetToolTip( _( "Net and netclass colors are not shown" ) );

    netColorSizer->Add( m_rbNetColorOff, 0, 0, 5 );

    netDisplayOptionsSizer->Add( netColorSizer, 0, wxEXPAND | wxBOTTOM, 5 );

    //// Ratsnest display

    m_txtRatsnestVisibility = new wxStaticText( netDisplayPane, wxID_ANY, _( "Ratsnest display:" ),
                                                wxDefaultPosition, wxDefaultSize, 0 );
    m_txtRatsnestVisibility->Wrap( -1 );
    m_txtRatsnestVisibility->SetToolTip( _( "Choose what ratsnest lines to display" ) );

    netDisplayOptionsSizer->Add( m_txtRatsnestVisibility, 0, wxEXPAND | wxBOTTOM | wxLEFT, 2 );

    wxBoxSizer* ratsnestDisplayModeSizer = new wxBoxSizer( wxHORIZONTAL );

    m_rbRatsnestAllLayers = new wxRadioButton( netDisplayPane, wxID_ANY, _( "All layers" ),
                                               wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_rbRatsnestAllLayers->SetToolTip( _( "Ratsnest lines are shown to items on all layers" ) );
    m_rbRatsnestAllLayers->SetValue( true );

    ratsnestDisplayModeSizer->Add( m_rbRatsnestAllLayers, 0, wxRIGHT, 10 );

    m_rbRatsnestVisibleLayers = new wxRadioButton( netDisplayPane, wxID_ANY, _( "Visible layers" ),
                                                   wxDefaultPosition, wxDefaultSize, 0 );
    m_rbRatsnestVisibleLayers->SetToolTip( _( "Ratsnest lines are shown to items on visible layers" ) );

    ratsnestDisplayModeSizer->Add( m_rbRatsnestVisibleLayers, 0, wxRIGHT, 4 );

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

    m_rbNetColorAll->Bind( wxEVT_RADIOBUTTON, &APPEARANCE_CONTROLS::onNetColorModeChanged, this );
    m_rbNetColorOff->Bind( wxEVT_RADIOBUTTON, &APPEARANCE_CONTROLS::onNetColorModeChanged, this );
    m_rbNetColorRatsnest->Bind( wxEVT_RADIOBUTTON,
                                &APPEARANCE_CONTROLS::onNetColorModeChanged, this );

    m_rbRatsnestAllLayers->Bind( wxEVT_RADIOBUTTON,
                                 &APPEARANCE_CONTROLS::onRatsnestModeChanged, this );
    m_rbRatsnestVisibleLayers->Bind( wxEVT_RADIOBUTTON,
                                     &APPEARANCE_CONTROLS::onRatsnestModeChanged, this );
}


wxSize APPEARANCE_CONTROLS::GetBestSize() const
{
    wxSize size( 220, 480 );
    // TODO(JE) appropriate logic
    return size;
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

    wxString netName = m_netsGrid->GetCellValue( event.GetRow(), NET_GRID_TABLE::COL_LABEL );
    wxMenu menu;

    menu.Append( new wxMenuItem( &menu, ID_SET_NET_COLOR,
                                 _( "Set net color" ), wxEmptyString, wxITEM_NORMAL ) );
    menu.Append( new wxMenuItem( &menu, ID_HIGHLIGHT_NET,
                                 wxString::Format( _( "Highlight %s" ), netName ),
                                 wxEmptyString, wxITEM_NORMAL ) );
    menu.Append( new wxMenuItem( &menu, ID_SELECT_NET,
                                 wxString::Format( _( "Select tracks and vias in %s" ), netName ),
                                 wxEmptyString, wxITEM_NORMAL ) );
    menu.Append( new wxMenuItem( &menu, ID_DESELECT_NET,
                                 wxString::Format( _( "Deselect tracks and vias in %s" ), netName ),
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
            tip = _( "Left double click or middle click for color change, "
                     "right click for menu" );

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
        CallAfter( [&]()
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


void APPEARANCE_CONTROLS::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( doesBoardItemNeedRebuild( aBoardItem ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    if( doesBoardItemNeedRebuild( aBoardItems ) )
    {
        handleBoardItemsChanged();
    }
}


void APPEARANCE_CONTROLS::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( doesBoardItemNeedRebuild( aBoardItem ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemsRemoved(
        BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    if( doesBoardItemNeedRebuild( aBoardItems ) )
    {
        handleBoardItemsChanged();
    }
}


void APPEARANCE_CONTROLS::OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( doesBoardItemNeedRebuild( aBoardItem ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemsChanged(
        BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    if( doesBoardItemNeedRebuild( aBoardItems ) )
    {
        handleBoardItemsChanged();
    }
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
    for( const std::unique_ptr<APPEARANCE_SETTING>& setting : m_layerSettings )
    {
        setting->ctl_panel->SetBackgroundColour( m_layerPanelColour );
        setting->ctl_indicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::OFF );
    }

    wxChar r, g, b;

    r = m_layerPanelColour.Red();
    g = m_layerPanelColour.Green();
    b = m_layerPanelColour.Blue();

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


void APPEARANCE_CONTROLS::SetLayerVisible( LAYER_NUM aLayer, bool isVisible )
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
        setting->ctl_visibility->SetValue( isVisible );
    }

    m_frame->GetBoard()->SetElementVisibility( aLayer, isVisible );

    m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, isVisible );
    m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::setVisibleLayers( LSET aLayers )
{
    if( m_isFpEditor )
    {
        KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();

        for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
            view->SetLayerVisible( layer, aLayers.Contains( layer ) );
    }
    else
    {
        m_frame->GetBoard()->SetVisibleLayers( aLayers );
    }
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
        {
            PCB_DISPLAY_OPTIONS opt = m_frame->GetDisplayOptions();
            aLayers.set( LAYER_RATSNEST, opt.m_ShowGlobalRatsnest );
        }

        m_frame->GetBoard()->SetVisibleElements( aLayers );
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

    m_cbFlipBoard->SetValue( m_frame->GetCanvas()->GetView()->IsMirroredX() );

    if( !m_isFpEditor )
    {
        if( options.m_RatsnestMode == RATSNEST_MODE::ALL )
            m_rbRatsnestAllLayers->SetValue( true );
        else
            m_rbRatsnestVisibleLayers->SetValue( true );

        wxASSERT( m_objectSettingsMap.count( LAYER_RATSNEST ) );
        APPEARANCE_SETTING* ratsnest = m_objectSettingsMap.at( LAYER_RATSNEST );
        ratsnest->ctl_visibility->SetValue( options.m_ShowGlobalRatsnest );
    }
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

        m_presetMRU.Add( preset.name );
    }

    rebuildLayerPresetsWidget();
}


void APPEARANCE_CONTROLS::loadDefaultLayerPresets()
{
    m_layerPresets.clear();
    m_presetMRU.clear();

    // Load the read-only defaults
    for( const LAYER_PRESET& preset : { presetAllLayers, presetAllCopper, presetInnerCopper,
                                        presetFront, presetFrontAssembly, presetBack,
                                        presetBackAssembly } )
    {
        m_layerPresets[preset.name]          = preset;
        m_layerPresets[preset.name].readOnly = true;

        m_presetMRU.Add( preset.name );
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


void APPEARANCE_CONTROLS::rebuildLayers()
{
    BOARD* board = m_frame->GetBoard();
    LSET enabled = board->GetEnabledLayers();
    LSET visible = getVisibleLayers();

    COLOR_SETTINGS* theme      = m_frame->GetColorSettings();
    COLOR4D         bgColor    = theme->GetColor( LAYER_PCB_BACKGROUND );
    bool            firstLayer = true;
    bool            readOnly   = theme->IsReadOnly();

#ifdef __WXMAC__
    wxSizerItem* m_windowLayersSizerItem = m_panelLayersSizer->GetItem( m_windowLayers );
    m_windowLayersSizerItem->SetFlag( m_windowLayersSizerItem->GetFlag() & ~wxTOP );
#endif

    m_layerSettings.clear();
    m_layerSettingsMap.clear();
    m_layersOuterSizer->Clear( true );

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
                                                                ROW_ICON_PROVIDER::STATE::OFF,
                                                                layer );

                COLOR_SWATCH* swatch = new COLOR_SWATCH( panel, COLOR4D::UNSPECIFIED, layer,
                                                         bgColor, theme->GetColor( layer ),
                                                         SWATCH_SMALL );
                swatch->SetToolTip( _( "Double click or middle click for color change, "
                                       "right click for menu" ) );

                BITMAP_TOGGLE* btn_visible = new BITMAP_TOGGLE( panel, layer,
                                                                KiBitmap( visibility_xpm ),
                                                                KiBitmap( visibility_off_xpm ),
                                                                aSetting->visible );
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

                m_layersOuterSizer->Add( panel, 0, wxEXPAND, 0 );

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
                            wxObject* btn = aEvent.GetEventObject();
                            int layId = static_cast<wxWindow*>( btn )->GetId();
                            bool isVisible = aEvent.GetInt();

                            wxASSERT( layId >= 0 && layId < PCB_LAYER_ID_COUNT );

                            if( m_isFpEditor && LSET::ForbiddenFootprintLayers().test( layId ) )
                            {
                                static_cast<BITMAP_TOGGLE*>( btn )->SetValue( !isVisible );
                                return;
                            }

                            onLayerVisibilityChanged( static_cast<PCB_LAYER_ID>( layId ),
                                                      isVisible, true );
                        } );

                swatch->Bind( COLOR_SWATCH_CHANGED, &APPEARANCE_CONTROLS::OnColorSwatchChanged,
                              this );
                swatch->SetReadOnlyCallback(std::bind( &APPEARANCE_CONTROLS::onReadOnlySwatch,
                                                       this ) );
                swatch->SetReadOnly( readOnly );

                auto rightClickHandler =
                        [&]( wxMouseEvent& aEvent )
                        {
                            wxASSERT( m_layerContextMenu );
                            PopupMenu( m_layerContextMenu );
                            passOnFocus();
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

        if( m_isFpEditor && LSET::ForbiddenFootprintLayers().test( layer ) )
        {
            setting->ctl_text->Disable();
            setting->ctl_color->SetToolTip( wxEmptyString );
        }
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
        { B_Fab,            _( "Footprint assembly on board's back" ) },
        { User_1,           _( "User defined layer 1" ) },
        { User_2,           _( "User defined layer 2" ) },
        { User_3,           _( "User defined layer 3" ) },
        { User_4,           _( "User defined layer 4" ) },
        { User_5,           _( "User defined layer 5" ) },
        { User_6,           _( "User defined layer 6" ) },
        { User_7,           _( "User defined layer 7" ) },
        { User_8,           _( "User defined layer 8" ) },
        { User_9,           _( "User defined layer 9" ) },
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

        if( m_isFpEditor && LSET::ForbiddenFootprintLayers().test( layer ) )
        {
            setting->ctl_text->Disable();
            setting->ctl_color->SetToolTip( wxEmptyString );
        }
    }

    m_layersOuterSizer->AddSpacer( 10 );
    m_windowLayers->SetBackgroundColour( m_layerPanelColour );
    m_windowLayers->Layout();
}


void APPEARANCE_CONTROLS::rebuildLayerContextMenu()
{
    delete m_layerContextMenu;
    m_layerContextMenu = new wxMenu;

    AddMenuItem( m_layerContextMenu, ID_SHOW_ALL_COPPER_LAYERS,
                 _( "Show All Copper Layers" ),
                 KiBitmap( show_all_copper_layers_xpm ) );
    AddMenuItem( m_layerContextMenu, ID_HIDE_ALL_COPPER_LAYERS,
                 _( "Hide All Copper Layers" ),
                 KiBitmap( show_no_copper_layers_xpm ) );

    m_layerContextMenu->AppendSeparator();

    AddMenuItem( m_layerContextMenu, ID_HIDE_ALL_BUT_ACTIVE,
                 _( "Hide All Layers But Active" ),
                 KiBitmap( select_w_layer_xpm ) );

    m_layerContextMenu->AppendSeparator();

    AddMenuItem( m_layerContextMenu, ID_SHOW_ALL_NON_COPPER, _( "Show All Non Copper Layers" ),
                 KiBitmap( show_no_copper_layers_xpm ) );

    AddMenuItem( m_layerContextMenu, ID_HIDE_ALL_NON_COPPER, _( "Hide All Non Copper Layers" ),
                 KiBitmap( show_all_copper_layers_xpm ) );

    m_layerContextMenu->AppendSeparator();

    AddMenuItem( m_layerContextMenu, ID_PRESET_ALL_LAYERS, _( "Show All Layers" ),
                 KiBitmap( show_all_layers_xpm ) );

    AddMenuItem( m_layerContextMenu, ID_PRESET_NO_LAYERS, _( "Hide All Layers" ),
                 KiBitmap( show_no_layers_xpm ) );

    m_layerContextMenu->AppendSeparator();

    AddMenuItem( m_layerContextMenu, ID_PRESET_FRONT_ASSEMBLY,
                 _( "Show Only Front Assembly Layers" ), KiBitmap( show_front_assembly_layers_xpm ) );

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
                 KiBitmap( show_back_assembly_layers_xpm ) );
}


void APPEARANCE_CONTROLS::OnLayerContextMenu( wxCommandEvent& aEvent )
{
    BOARD* board   = m_frame->GetBoard();
    LSET   visible = getVisibleLayers();

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
        setVisibleLayers( visible );
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

        setVisibleLayers( visible );
        break;
    }

    case ID_HIDE_ALL_NON_COPPER:
    {
        visible &= presetAllCopper.layers;

        if( !visible.test( current ) )
            m_frame->SetActiveLayer( *visible.Seq().begin() );

        setVisibleLayers( visible );
        break;
    }

    case ID_SHOW_ALL_NON_COPPER:
    {
        visible |= ~presetAllCopper.layers;

        setVisibleLayers( visible );
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
        LAYER_NUM layer = setting->id;

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


void APPEARANCE_CONTROLS::onLayerClick( wxMouseEvent& aEvent )
{
    auto eventSource = static_cast<wxWindow*>( aEvent.GetEventObject() );

    PCB_LAYER_ID layer = ToLAYER_ID( eventSource->GetId() );

    if( m_isFpEditor && LSET::ForbiddenFootprintLayers().test( layer ) )
        return;

    m_frame->SetActiveLayer( layer );
    passOnFocus();
}


void APPEARANCE_CONTROLS::onLayerVisibilityChanged( PCB_LAYER_ID aLayer, bool isVisible,
                                                    bool isFinal )
{
    LSET visibleLayers = getVisibleLayers();

    if( visibleLayers.test( aLayer ) != isVisible )
    {
        visibleLayers.set( aLayer, isVisible );

        setVisibleLayers( visibleLayers );

        m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, isVisible );
    }

    syncLayerPresetSelection();

    if( isFinal )
        m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::onObjectVisibilityChanged( GAL_LAYER_ID aLayer, bool isVisible,
                                                     bool isFinal )
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
            PCB_DISPLAY_OPTIONS opt  = m_frame->GetDisplayOptions();
            opt.m_ShowGlobalRatsnest = isVisible;
            m_frame->SetDisplayOptions( opt );
            m_frame->GetBoard()->SetElementVisibility( aLayer, isVisible );
            m_frame->GetCanvas()->RedrawRatsnest();
        }

        break;
    }

    case LAYER_GRID:
        m_frame->SetGridVisibility( isVisible );
        m_frame->GetCanvas()->Refresh();
        syncLayerPresetSelection();
        break;

    case LAYER_MOD_TEXT_FR:
        // Because Footprint Text is a meta-control that also can disable values/references,
        // drag them along here so that the user is less likely to be confused.
        onObjectVisibilityChanged( LAYER_MOD_REFERENCES, isVisible, false );
        onObjectVisibilityChanged( LAYER_MOD_VALUES, isVisible, false );
        m_objectSettingsMap[LAYER_MOD_REFERENCES]->ctl_visibility->SetValue( isVisible );
        m_objectSettingsMap[LAYER_MOD_VALUES]->ctl_visibility->SetValue( isVisible );
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

                if( color != COLOR4D::UNSPECIFIED )
                {
                    COLOR_SWATCH* swatch = new COLOR_SWATCH( m_windowObjects, color, layer,
                                                             bgColor, defColor, SWATCH_SMALL );
                    swatch->SetToolTip( _( "Left double click or middle click for color change, "
                                           "right click for menu" ) );

                    sizer->Add( swatch, 0,  wxALIGN_CENTER_VERTICAL, 0 );
                    aSetting->ctl_color = swatch;

                    swatch->Bind( COLOR_SWATCH_CHANGED,
                                  &APPEARANCE_CONTROLS::OnColorSwatchChanged, this );

                    swatch->SetReadOnlyCallback( std::bind( &APPEARANCE_CONTROLS::onReadOnlySwatch,
                                                            this ) );
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
                    slider->Bind( wxEVT_SET_FOCUS, &APPEARANCE_CONTROLS::OnSetFocus, this );
                }
                else
                {
                    sizer->Add( btn_visible, 0, wxALIGN_CENTER_VERTICAL, 0 );
                    sizer->AddSpacer( 5 );
                    sizer->Add( label, 0, wxALIGN_CENTER_VERTICAL, 0 );
                }

                aSetting->ctl_text = label;
                m_objectsOuterSizer->Add( sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );
                m_objectsOuterSizer->AddSpacer( 1 );
            };

    for( const APPEARANCE_SETTING& s_setting : s_objectSettings )
    {
        if( m_isFpEditor && !s_allowedInFpEditor.count( s_setting.id ) )
            continue;

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
            m_objectsOuterSizer->AddSpacer( m_pointSize );
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

    // If the board isn't fully loaded, we can't yet rebuild
    if( !board->GetProject() )
        return;

    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings() );

    std::map<wxString, KIGFX::COLOR4D>& netclassColors = rs->GetNetclassColorMap();

    m_netclassOuterSizer->Clear( true );

    auto appendNetclass =
            [&]( int aId, const NETCLASSPTR& aClass, bool isDefaultClass = false )
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
                                                        COLOR4D::UNSPECIFIED, SWATCH_SMALL );
                setting->ctl_color->SetToolTip( _( "Left double click or middle click for color "
                                                   "change, right click for menu" ) );

                setting->ctl_color->Bind( COLOR_SWATCH_CHANGED,
                                          &APPEARANCE_CONTROLS::onNetclassColorChanged, this );

                // Default netclass can't have an override color
                if( isDefaultClass )
                    setting->ctl_color->Hide();

                setting->ctl_visibility =
                        new BITMAP_TOGGLE( setting->ctl_panel, aId, KiBitmap( visibility_xpm ),
                                           KiBitmap( visibility_off_xpm ), true );

                wxString tip;
                tip.Printf( _( "Show or hide ratsnest for nets in %s" ), name );
                setting->ctl_visibility->SetToolTip( tip );

                setting->ctl_text = new wxStaticText( setting->ctl_panel, aId, name );
                setting->ctl_text->Wrap( -1 );

                int flags = wxALIGN_CENTER_VERTICAL;

                sizer->Add( setting->ctl_color,      0, flags | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );
                sizer->AddSpacer( 7 );
                sizer->Add( setting->ctl_visibility, 0, flags,                                  5 );
                sizer->AddSpacer( 3 );
                sizer->Add( setting->ctl_text,       1, flags,                                  5 );

                m_netclassOuterSizer->Add( setting->ctl_panel, 0, wxEXPAND, 5 );
                m_netclassOuterSizer->AddSpacer( 1 );

                setting->ctl_visibility->Bind( TOGGLE_CHANGED,
                                               &APPEARANCE_CONTROLS::onNetclassVisibilityChanged,
                                               this );

                auto menuHandler =
                        [&, name, isDefaultClass]( wxMouseEvent& aEvent )
                        {
                            m_contextMenuNetclass = name;

                            wxMenu menu;

                            if( !isDefaultClass)
                            {
                                menu.Append( new wxMenuItem( &menu, ID_SET_NET_COLOR,
                                             _( "Set netclass color" ), wxEmptyString,
                                             wxITEM_NORMAL ) );
                            }

                            menu.Append( new wxMenuItem( &menu, ID_HIGHLIGHT_NET,
                                         wxString::Format( _( "Highlight nets in %s" ), name ),
                                                         wxEmptyString, wxITEM_NORMAL ) );
                            menu.Append( new wxMenuItem( &menu, ID_SELECT_NET,
                                         wxString::Format( _( "Select tracks and vias in %s" ),
                                                           name ),
                                         wxEmptyString, wxITEM_NORMAL ) );
                            menu.Append( new wxMenuItem( &menu, ID_DESELECT_NET,
                                         wxString::Format( _( "Deselect tracks and vias in %s" ),
                                                           name ),
                                         wxEmptyString, wxITEM_NORMAL ) );

                            menu.AppendSeparator();

                            menu.Append( new wxMenuItem( &menu, ID_SHOW_ALL_NETS,
                                         _( "Show all netclasses" ), wxEmptyString,
                                         wxITEM_NORMAL ) );
                            menu.Append( new wxMenuItem( &menu, ID_HIDE_OTHER_NETS,
                                         _( "Hide all other netclasses" ), wxEmptyString,
                                         wxITEM_NORMAL ) );

                            menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
                                       &APPEARANCE_CONTROLS::onNetclassContextMenu, this );

                            PopupMenu( &menu );
                        };

                setting->ctl_panel->Bind( wxEVT_RIGHT_DOWN, menuHandler );
                setting->ctl_visibility->Bind( wxEVT_RIGHT_DOWN, menuHandler );
                setting->ctl_color->Bind( wxEVT_RIGHT_DOWN, menuHandler );
                setting->ctl_text->Bind( wxEVT_RIGHT_DOWN, menuHandler );
            };

    const NETCLASS_MAP& classes = board->GetDesignSettings().GetNetClasses().NetClasses();

    std::vector<wxString> names;

    for( const auto& pair : classes )
        names.emplace_back( pair.first );

    std::sort( names.begin(), names.end() );

    m_netclassIdMap.clear();

    int idx = wxID_HIGHEST;

    NETCLASSPTR defaultClass = board->GetDesignSettings().GetNetClasses().GetDefault();

    m_netclassIdMap[idx] = defaultClass->GetName();
    appendNetclass( idx++, defaultClass, true );

    for( const wxString& name : names )
    {
        m_netclassIdMap[idx] = name;
        appendNetclass( idx++, classes.at( name ) );
    }

    m_netclassOuterSizer->Layout();

    m_netsTable->Rebuild();
    m_panelNets->GetSizer()->Layout();
}


void APPEARANCE_CONTROLS::rebuildLayerPresetsWidget()
{
    m_cbLayerPresets->Clear();

    for( std::pair<const wxString, LAYER_PRESET>& pair : m_layerPresets )
        m_cbLayerPresets->Append( pair.first, static_cast<void*>( &pair.second ) );

    m_cbLayerPresets->Append( wxT( "-----" ) );
    m_cbLayerPresets->Append( _( "Save preset..." ) );
    m_cbLayerPresets->Append( _( "Delete preset..." ) );

    m_cbLayerPresets->SetSelection( 0 );

    // At least the build in presets should always be present
    wxASSERT( !m_layerPresets.empty() );

    // Default preset: all layers
    m_currentPreset = &m_layerPresets[presetAllLayers.name];
}


void APPEARANCE_CONTROLS::syncLayerPresetSelection()
{
    LSET    visibleLayers  = getVisibleLayers();
    GAL_SET visibleObjects = getVisibleObjects();

    auto it = std::find_if( m_layerPresets.begin(), m_layerPresets.end(),
                            [&]( const std::pair<const wxString, LAYER_PRESET>& aPair )
                            {
                                return ( aPair.second.layers == visibleLayers
                                         && aPair.second.renderLayers == visibleObjects );
                            } );

    if( it != m_layerPresets.end() )
        m_cbLayerPresets->SetStringSelection( it->first );
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

    if( index == count - 3 )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedUserPreset )
            name = m_lastSelectedUserPreset->name;

        wxTextEntryDialog dlg( this, _( "Layer preset name:" ), _( "Save Layer Preset" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        name = dlg.GetValue();
        bool exists = m_layerPresets.count( name );

        if( !exists )
            m_layerPresets[name] = LAYER_PRESET( name, getVisibleLayers(),
                                                 getVisibleObjects(), UNSELECTED_LAYER );

        LAYER_PRESET* preset = &m_layerPresets[name];
        m_currentPreset      = preset;

        if( !exists )
        {
            index = m_cbLayerPresets->Insert( name, index - 1, static_cast<void*>( preset ) );
        }
        else
        {
            index = m_cbLayerPresets->FindString( name );
            m_presetMRU.Remove( name );
        }

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

        EDA_LIST_DIALOG dlg( m_frame, _( "Delete Preset" ), headers, items, wxEmptyString );
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

                m_presetMRU.Remove( presetName );
            }
        }

        resetSelection();
        return;
    }

    LAYER_PRESET* preset = static_cast<LAYER_PRESET*>( m_cbLayerPresets->GetClientData( index ) );
    m_currentPreset      = preset;

    m_lastSelectedUserPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
        doApplyLayerPreset( *preset );

    if( !m_currentPreset->name.IsEmpty() )
    {
        m_presetMRU.Remove( m_currentPreset->name );
        m_presetMRU.Insert( m_currentPreset->name, 0 );
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS::doApplyLayerPreset( const LAYER_PRESET& aPreset )
{
    BOARD* board = m_frame->GetBoard();

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

    if( IsCopperLayer( layer ) )
        view->UpdateLayerColor( ZONE_LAYER_FOR( layer ) );

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
    case static_cast<int>( LAYER_TRACKS ): options.m_TrackOpacity = aOpacity; break;
    case static_cast<int>( LAYER_VIAS ):   options.m_ViaOpacity = aOpacity;   break;
    case static_cast<int>( LAYER_PADS ):   options.m_PadOpacity = aOpacity;   break;
    case static_cast<int>( LAYER_ZONES ):  options.m_ZoneOpacity = aOpacity;  break;
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

    case ID_HIGHLIGHT_NET:
    {
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::highlightNet, true, net.code );
        m_frame->GetCanvas()->Refresh();
        break;
    }

    case ID_SELECT_NET:
    {
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectNet, true, net.code );
        m_frame->GetCanvas()->Refresh();
        break;
    }

    case ID_DESELECT_NET:
    {
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::deselectNet, true, net.code );
        m_frame->GetCanvas()->Refresh();
        break;
    }

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
    BOARD*        board    = m_frame->GetBoard();
    NETINFO_LIST& nets     = board->GetNetInfo();
    NETCLASSES&   classes  = board->GetDesignSettings().GetNetClasses();
    NETCLASSPTR   netclass = classes.Find( aClassName );
    TOOL_MANAGER* manager  = m_frame->GetToolManager();

    if( !netclass )
        return;

    NETCLASS* defaultClass = classes.GetDefaultPtr();

    if( netclass == classes.GetDefault() )
    {
        const TOOL_ACTION& action = aShow ? PCB_ACTIONS::showNet : PCB_ACTIONS::hideNet;

        for( NETINFO_ITEM* net : nets )
        {
            if( net->GetNetClass() == defaultClass )
            {
                manager->RunAction( action, true, net->GetNetCode() );

                int row = m_netsTable->GetRowByNetcode( net->GetNetCode() );

                if( row >= 0 )
                    m_netsTable->SetValueAsBool( row, NET_GRID_TABLE::COL_VISIBILITY, aShow );
            }
        }
    }
    else
    {
        const TOOL_ACTION& action = aShow ? PCB_ACTIONS::showNet : PCB_ACTIONS::hideNet;

        for( const wxString& member : *netclass )
        {
            if( NETINFO_ITEM* net = nets.GetNetItem( member ) )
            {
                int code = net->GetNetCode();
                manager->RunAction( action, true, code );

                int row = m_netsTable->GetRowByNetcode( code );

                if( row >= 0 )
                    m_netsTable->SetValueAsBool( row, NET_GRID_TABLE::COL_VISIBILITY, aShow );
            }
        }
    }

    m_netsGrid->ForceRefresh();
}


void APPEARANCE_CONTROLS::onNetclassColorChanged( wxCommandEvent& aEvent )
{
    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings() );

    std::map<wxString, KIGFX::COLOR4D>& netclassColors = rs->GetNetclassColorMap();

    COLOR_SWATCH* swatch    = static_cast<COLOR_SWATCH*>( aEvent.GetEventObject() );
    wxString      className = netclassNameFromEvent( aEvent );

    netclassColors[className] = swatch->GetSwatchColor();

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
    passOnFocus();
}


void APPEARANCE_CONTROLS::onRatsnestModeChanged( wxCommandEvent& aEvent )
{
    PCB_DISPLAY_OPTIONS options = m_frame->GetDisplayOptions();

    if( m_rbRatsnestAllLayers->GetValue() )
        options.m_RatsnestMode = RATSNEST_MODE::ALL;
    else
        options.m_RatsnestMode = RATSNEST_MODE::VISIBLE;

    m_frame->SetDisplayOptions( options );
    m_frame->GetCanvas()->RedrawRatsnest();
    passOnFocus();
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

    NETCLASSPTR defaultClass     = classes.GetDefault();
    wxString    defaultClassName = defaultClass->GetName();

    auto runOnNetsOfClass =
            [&]( NETCLASSPTR aClass, std::function<void( NETINFO_ITEM* )> aFunction )
            {
                if( aClass == defaultClass )
                {
                    for( NETINFO_ITEM* net : nets )
                        if( net->GetNetClass() == defaultClass.get() )
                            aFunction( net );
                }
                else
                {
                    for( const wxString& netName : *aClass )
                        aFunction( nets.GetNetItem( netName ) );
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
                runOnNetsOfClass( netclass,
                        [&]( NETINFO_ITEM* aItem )
                        {
                            if( !aItem )
                                return;

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
            if( netclass )
            {
                TOOL_ACTION& action = aEvent.GetId() == ID_SELECT_NET ? PCB_ACTIONS::selectNet :
                                                                        PCB_ACTIONS::deselectNet;
                runOnNetsOfClass( netclass,
                        [&]( NETINFO_ITEM* aItem )
                        {
                            if( !aItem )
                                return;

                            int code = aItem->GetNetCode();
                            m_frame->GetToolManager()->RunAction( action, true, code );
                        } );
            }
            break;
        }

        case ID_SHOW_ALL_NETS:
        {
            showNetclass( defaultClassName );
            wxASSERT( m_netclassSettingsMap.count( defaultClassName ) );
            m_netclassSettingsMap.at( defaultClassName )->ctl_visibility->SetValue( true );

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
            bool showDefault = m_contextMenuNetclass == defaultClassName;
            showNetclass( defaultClassName, showDefault );
            wxASSERT( m_netclassSettingsMap.count( defaultClassName ) );
            m_netclassSettingsMap.at( defaultClassName )->ctl_visibility->SetValue( showDefault );

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


void APPEARANCE_CONTROLS::passOnFocus()
{
    m_focusOwner->SetFocus();
}


void APPEARANCE_CONTROLS::onReadOnlySwatch()
{
    WX_INFOBAR* infobar = m_frame->GetInfoBar();

    wxHyperlinkCtrl* button = new wxHyperlinkCtrl( infobar, wxID_ANY, _( "Open Preferences" ),
                                                   wxEmptyString );

    button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
            [&]( wxHyperlinkEvent& aEvent )
            {
                 wxCommandEvent dummy;
                 m_frame->OnPreferences( dummy );
            } ) );

    infobar->RemoveAllButtons();
    infobar->AddButton( button );
    infobar->AddCloseButton();

    infobar->ShowMessageFor( _( "The current color theme is read-only.  Create a new theme in "
                                "Preferences to enable color editing." ),
                             10000, wxICON_INFORMATION );
}
