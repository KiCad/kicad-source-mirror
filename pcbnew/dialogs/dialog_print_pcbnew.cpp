/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2016 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2018-2023 CERN
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <kiface_base.h>
#include <pcbnew_settings.h>
#include <pcbplot.h>
#include <board.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_control.h>
#include <dialogs/dialog_print_generic.h>
#include <pcbnew_printout.h>
#include <wx/checklst.h>


class DIALOG_PRINT_PCBNEW : public DIALOG_PRINT_GENERIC
{
public:
    DIALOG_PRINT_PCBNEW( PCB_BASE_EDIT_FRAME* aParent, PCBNEW_PRINTOUT_SETTINGS* aSettings );
    ~DIALOG_PRINT_PCBNEW();

private:
    enum
    {
        ID_SELECT_FIRST = 4100,
        ID_SELECT_FAB_LAYERS = ID_SELECT_FIRST,
        ID_SELECT_COPPER_LAYERS,
        ID_DESELECT_COPPER_LAYERS,
        ID_SELECT_ALL_LAYERS,
        ID_DESELECT_ALL_LAYERS,
        ID_SELECT_LAST
    };

    PCBNEW_PRINTOUT_SETTINGS* settings() const
    {
        wxASSERT( dynamic_cast<PCBNEW_PRINTOUT_SETTINGS*>( m_settings ) );
        return static_cast<PCBNEW_PRINTOUT_SETTINGS*>( m_settings );
    }

    bool TransferDataToWindow() override;

    void createExtraOptions();
    void createLeftPanel();

    void onUseThemeClicked( wxCommandEvent& event );
    void onPagePerLayerClicked( wxCommandEvent& event );
    void onColorModeClicked( wxCommandEvent& event );
    void onPopUpLayers( wxCommandEvent& event );

    ///< Update layerset basing on the selected layers.
    int setLayerSetFromList();

    void saveSettings() override;

    wxPrintout* createPrintout( const wxString& aTitle ) override
    {
        return new PCBNEW_PRINTOUT( m_parent->GetBoard(), *settings(),
                                    m_parent->GetCanvas()->GetView(), aTitle );
    }

    PCB_BASE_EDIT_FRAME* m_parent;
    LSEQ                 m_layerList;                // List to hold CheckListBox layer numbers
    wxMenu*              m_popMenu;

    wxCheckListBox*      m_layerCheckListBox;
    wxCheckBox*          m_checkboxMirror;
    wxChoice*            m_drillMarksChoice;
    wxCheckBox*          m_checkboxPagePerLayer;
    wxCheckBox*          m_checkboxEdgesOnAllPages;
    wxCheckBox*          m_checkAsItems;
    wxCheckBox*          m_checkBackground;
    wxCheckBox*          m_checkUseTheme;
    wxChoice*            m_colorTheme;
};


DIALOG_PRINT_PCBNEW::DIALOG_PRINT_PCBNEW( PCB_BASE_EDIT_FRAME* aParent, PCBNEW_PRINTOUT_SETTINGS* aSettings ) :
        DIALOG_PRINT_GENERIC( aParent, aSettings ),
        m_parent( aParent )
{
    createExtraOptions();
    createLeftPanel();

    BOARD* board = m_parent->GetBoard();

    // Create layer list
    // Could devote a PlotOrder() function in place of UIOrder().
    m_layerList = board->GetEnabledLayers().UIOrder();

    // Populate the check list box by all enabled layers names. They will be enabled later
    // when the dlg settings are loaded (i.e. after DIALOG_PRINT_GENERIC::TransferDataToWindow()
    // is called
    for( PCB_LAYER_ID layer : m_layerList )
        m_layerCheckListBox->Append( board->GetLayerName( layer ) );

    m_infoText->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );
    m_infoText->SetLabel( _( "Right-click for layer selection commands." ) );
    m_infoText->Show( true );

    finishDialogSettings();

    m_popMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW::onPopUpLayers ),
                     this, ID_SELECT_FIRST, ID_SELECT_LAST );

    m_outputMode->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &DIALOG_PRINT_PCBNEW::onColorModeClicked, this );
}


DIALOG_PRINT_PCBNEW::~DIALOG_PRINT_PCBNEW()
{
    m_popMenu->Unbind( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW::onPopUpLayers ),
                       this, ID_SELECT_FIRST, ID_SELECT_LAST );

    m_outputMode->Unbind( wxEVT_COMMAND_CHOICE_SELECTED, &DIALOG_PRINT_PCBNEW::onColorModeClicked, this );
}


bool DIALOG_PRINT_PCBNEW::TransferDataToWindow()
{
    if( !DIALOG_PRINT_GENERIC::TransferDataToWindow() )
        return false;

    BOARD* board = m_parent->GetBoard();

    // Enable layers from previous dlg settings
    m_layerList = board->GetEnabledLayers().UIOrder();
    int choice_ly_id = 0;

    for( PCB_LAYER_ID layer : m_layerList )
    {
        if( settings()->m_LayerSet.test( layer ) )
            m_layerCheckListBox->Check( choice_ly_id );

        choice_ly_id++;
    }

    m_checkAsItems->SetValue( settings()->m_AsItemCheckboxes );
    m_checkboxMirror->SetValue( settings()->m_Mirror );
    m_titleBlock->SetValue( settings()->m_titleBlock );

    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

    m_checkBackground->SetValue( cfg->m_Printing.background );
    m_checkUseTheme->SetValue( cfg->m_Printing.use_theme );

    m_colorTheme->Clear();

    int width    = 0;
    int height   = 0;
    int minwidth = width;

    wxString target = cfg->m_Printing.use_theme ? cfg->m_Printing.color_theme : cfg->m_ColorTheme;

    for( COLOR_SETTINGS* settings : m_parent->GetSettingsManager()->GetColorSettingsList() )
    {
        int pos = m_colorTheme->Append( settings->GetName(), static_cast<void*>( settings ) );

        if( settings->GetFilename() == target )
            m_colorTheme->SetSelection( pos );

        m_colorTheme->GetTextExtent( settings->GetName(), &width, &height );
        minwidth = std::max( minwidth, width );
    }

    m_colorTheme->SetMinSize( wxSize( minwidth + 50, -1 ) );

    wxCommandEvent dummy;
    onColorModeClicked( dummy );

    // Options to plot pads and vias holes
    m_drillMarksChoice->SetSelection( (int)settings()->m_DrillMarks );

    // Print all layers one one page or separately
    m_checkboxPagePerLayer->SetValue( settings()->m_Pagination == PCBNEW_PRINTOUT_SETTINGS::LAYER_PER_PAGE );
    onPagePerLayerClicked( dummy );

    // Update the dialog layout when layers are added
    GetSizer()->Fit( this );

    return true;
}


void DIALOG_PRINT_PCBNEW::createExtraOptions()
{
    wxGridBagSizer* optionsSizer = getOptionsSizer();
    wxStaticBox*    box = getOptionsBox();
    int             rows = optionsSizer->GetEffectiveRowsCount();

    m_checkAsItems = new wxCheckBox( box, wxID_ANY, _( "Print according to objects tab of appearance manager" ) );
    optionsSizer->Add( m_checkAsItems, wxGBPosition( rows++, 0 ), wxGBSpan( 1, 2 ), wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    m_checkBackground = new wxCheckBox( box, wxID_ANY, _( "Print background color" ) );
    optionsSizer->Add( m_checkBackground, wxGBPosition( rows++, 0 ), wxGBSpan( 1, 2 ), wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    m_checkUseTheme = new wxCheckBox( box, wxID_ANY, _( "Use a different color theme for printing:" ) );
    optionsSizer->Add( m_checkUseTheme, wxGBPosition( rows++, 0 ), wxGBSpan( 1, 2 ), wxLEFT|wxRIGHT, 5 );

    m_checkUseTheme->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &DIALOG_PRINT_PCBNEW::onUseThemeClicked, this );

    wxArrayString choices;
    m_colorTheme = new wxChoice( box, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices, 0 );
    m_colorTheme->SetSelection( 0 );
    m_colorTheme->SetMinSize( { 200, -1 } );
    optionsSizer->Add( m_colorTheme, wxGBPosition( rows++, 0 ), wxGBSpan( 1, 2 ), wxLEFT, 28 );

    rows++;

    // Drill marks option
    auto drillMarksLabel = new wxStaticText( box, wxID_ANY, _( "Drill marks:" ) );
    std::vector<wxString> drillMarkChoices = { _( "No drill mark" ),
                                               _( "Small mark" ),
                                               _( "Real drill" ) };
    m_drillMarksChoice = new wxChoice( box, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       drillMarkChoices.size(), drillMarkChoices.data(), 0 );
    m_drillMarksChoice->SetSelection( 0 );

    optionsSizer->Add( drillMarksLabel, wxGBPosition( rows, 0 ), wxGBSpan( 1, 1 ),
                       wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5 );
    optionsSizer->Add( m_drillMarksChoice, wxGBPosition( rows++, 1 ), wxGBSpan( 1, 1 ),
                       wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5 );

    // Print mirrored
    m_checkboxMirror = new wxCheckBox( box, wxID_ANY, _( "Print mirrored" ) );

    optionsSizer->Add( m_checkboxMirror, wxGBPosition( rows++, 0 ), wxGBSpan( 1, 2 ),
                       wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    // Pagination
    m_checkboxPagePerLayer = new wxCheckBox( box, wxID_ANY, _( "Print one page per layer" ) );

    m_checkboxPagePerLayer->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &DIALOG_PRINT_PCBNEW::onPagePerLayerClicked, this );

    m_checkboxEdgesOnAllPages = new wxCheckBox( box, wxID_ANY, _( "Print board edges on all pages" ) );

    optionsSizer->Add( m_checkboxPagePerLayer, wxGBPosition( rows++, 0 ), wxGBSpan( 1, 2 ), wxLEFT|wxRIGHT, 5 );
    optionsSizer->Add( m_checkboxEdgesOnAllPages, wxGBPosition( rows++, 0 ), wxGBSpan( 1, 2 ), wxLEFT, 28 );
}


void DIALOG_PRINT_PCBNEW::createLeftPanel()
{
    wxStaticBox* box = new wxStaticBox( this, wxID_ANY, _( "Include Layers" ) );
    wxStaticBoxSizer* sbLayersSizer = new wxStaticBoxSizer( box, wxVERTICAL );

   	m_layerCheckListBox = new wxCheckListBox( sbLayersSizer->GetStaticBox(), wxID_ANY );
   	m_layerCheckListBox->SetMinSize( wxSize( 180, -1 ) );

    sbLayersSizer->Add( m_layerCheckListBox, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

    getMainSizer()->Insert( 0, sbLayersSizer, 1, wxEXPAND | wxALL, 5 );

    m_popMenu = new wxMenu();
   	m_popMenu->Append( new wxMenuItem( m_popMenu, ID_SELECT_FAB_LAYERS,      _( "Select Fab Layers" ) ) );
   	m_popMenu->Append( new wxMenuItem( m_popMenu, ID_SELECT_COPPER_LAYERS,   _( "Select all Copper Layers" ) ) );
   	m_popMenu->Append( new wxMenuItem( m_popMenu, ID_DESELECT_COPPER_LAYERS, _( "Deselect all Copper Layers" ) ) );
   	m_popMenu->Append( new wxMenuItem( m_popMenu, ID_SELECT_ALL_LAYERS,      _( "Select all Layers" ) ) );
   	m_popMenu->Append( new wxMenuItem( m_popMenu, ID_DESELECT_ALL_LAYERS,    _( "Deselect all Layers" ) ) );

    this->Bind( wxEVT_RIGHT_DOWN,
                [&]( wxMouseEvent& aEvent )
                {
                    this->PopupMenu( m_popMenu, aEvent.GetPosition() );
                } );

    m_layerCheckListBox->Bind( wxEVT_RIGHT_DOWN,
                [&]( wxMouseEvent& aEvent )
                {
                    this->PopupMenu( m_popMenu, aEvent.GetPosition() );
                } );
}


void DIALOG_PRINT_PCBNEW::onUseThemeClicked( wxCommandEvent& event )
{
    m_colorTheme->Enable( m_checkUseTheme->GetValue() );
}


void DIALOG_PRINT_PCBNEW::onPagePerLayerClicked( wxCommandEvent& event )
{
    if( m_checkboxPagePerLayer->GetValue() )
    {
        m_checkboxEdgesOnAllPages->Enable( true );
        m_checkboxEdgesOnAllPages->SetValue( settings()->m_PrintEdgeCutsOnAllPages );
    }
    else
    {
        m_checkboxEdgesOnAllPages->Enable( false );
        m_checkboxEdgesOnAllPages->SetValue( false );
    }
}


void DIALOG_PRINT_PCBNEW::onColorModeClicked( wxCommandEvent& event )
{
    m_settings->m_blackWhite = m_outputMode->GetSelection();

    m_checkBackground->Enable( !m_settings->m_blackWhite );
    m_checkUseTheme->Enable( !m_settings->m_blackWhite );

    if( PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings() )
        m_colorTheme->Enable( !m_settings->m_blackWhite && cfg->m_Printing.use_theme );
}


// Select or deselect groups of layers in the layers list:
void DIALOG_PRINT_PCBNEW::onPopUpLayers( wxCommandEvent& event )
{
    // Build a list of layers for usual fabrication: copper layers + tech layers without courtyard
    LSET fab_layer_set = ( LSET::AllCuMask() | LSET::AllTechMask() ) & ~LSET( { B_CrtYd, F_CrtYd } );

    switch( event.GetId() )
    {
    case ID_SELECT_FAB_LAYERS: // Select layers usually needed to build a board
        for( unsigned i = 0; i < m_layerList.size(); i++ )
        {
            LSET layermask( { m_layerList[ i ] } );

            if( ( layermask & fab_layer_set ).any() )
                m_layerCheckListBox->Check( i, true );
            else
                m_layerCheckListBox->Check( i, false );
        }

        break;

    case ID_SELECT_COPPER_LAYERS:
        for( unsigned i = 0; i < m_layerList.size(); i++ )
        {
            if( IsCopperLayer( m_layerList[i] ) )
                m_layerCheckListBox->Check( i, true );
        }

        break;

    case ID_DESELECT_COPPER_LAYERS:
        for( unsigned i = 0; i < m_layerList.size(); i++ )
        {
            if( IsCopperLayer( m_layerList[i] ) )
                m_layerCheckListBox->Check( i, false );
        }

        break;

    case ID_SELECT_ALL_LAYERS:
        for( unsigned i = 0; i < m_layerList.size(); i++ )
            m_layerCheckListBox->Check( i, true );

        break;

    case ID_DESELECT_ALL_LAYERS:
        for( unsigned i = 0; i < m_layerList.size(); i++ )
            m_layerCheckListBox->Check( i, false );

        break;

    default:
        break;
    }
}


int DIALOG_PRINT_PCBNEW::setLayerSetFromList()
{
    settings()->m_LayerSet = LSET();
    int& pageCount = settings()->m_pageCount;
    pageCount = 0;

    for( unsigned i = 0; i < m_layerList.size(); i++ )
    {
        if( m_layerCheckListBox->IsChecked( i ) )
        {
            ++pageCount;
            settings()->m_LayerSet.set( m_layerList[i] );
        }
    }

    // In Pcbnew force the EDGE layer to be printed or not with the other layers
    settings()->m_PrintEdgeCutsOnAllPages = m_checkboxEdgesOnAllPages->IsChecked();

    // All layers on one page (only if there is at least one layer selected)
    if( !m_checkboxPagePerLayer->GetValue() && pageCount > 0 )
        pageCount = 1;

    return pageCount;
}


void DIALOG_PRINT_PCBNEW::saveSettings()
{
    setLayerSetFromList();

    settings()->m_AsItemCheckboxes = m_checkAsItems->GetValue();

    settings()->m_DrillMarks = static_cast<DRILL_MARKS>( m_drillMarksChoice->GetSelection() );

    if( m_checkboxPagePerLayer->GetValue() )
    {
        settings()->m_Pagination = PCBNEW_PRINTOUT_SETTINGS::LAYER_PER_PAGE;
        settings()->m_PrintEdgeCutsOnAllPages = m_checkboxEdgesOnAllPages->GetValue();
    }
    else
    {
        settings()->m_Pagination = PCBNEW_PRINTOUT_SETTINGS::ALL_LAYERS;
    }

    settings()->m_Mirror = m_checkboxMirror->GetValue();

    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

    cfg->m_Printing.background = m_checkBackground->GetValue();
    settings()->m_background   = cfg->m_Printing.background;
    cfg->m_Printing.use_theme  = m_checkUseTheme->GetValue();

    int sel = m_colorTheme->GetSelection();
    COLOR_SETTINGS* theme = nullptr;

    if( sel >= 0 && sel < (int) m_colorTheme->GetCount() )
        theme = static_cast<COLOR_SETTINGS*>( m_colorTheme->GetClientData( sel ) );

    if( theme && m_checkUseTheme->IsChecked() )
    {
        cfg->m_Printing.color_theme = theme->GetFilename();
        settings()->m_colorSettings = theme;
    }
    else
    {
        settings()->m_colorSettings = m_parent->GetColorSettings();
    }

    DIALOG_PRINT_GENERIC::saveSettings();

    settings()->Save( cfg );
}


int PCB_CONTROL::Print( const TOOL_EVENT& aEvent )
{
    // Selection affects the origin item visibility
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    PCBNEW_PRINTOUT_SETTINGS settings( m_frame->GetPageSettings() );

    // Load saved settings
    PCBNEW_SETTINGS* cfg = static_cast<PCB_BASE_EDIT_FRAME*>( m_frame )->GetPcbNewSettings();
    settings.Load( cfg );

    DIALOG_PRINT_PCBNEW dlg( (PCB_BASE_EDIT_FRAME*) m_frame, &settings );

    if( m_isFootprintEditor )
        dlg.ForcePrintBorder( false );

    dlg.ShowModal();

    return 0;
}


