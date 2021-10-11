/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <macros.h>             // arrayDim definition
#include <pcb_edit_frame.h>
#include <board.h>
#include <board_design_settings.h>
#include <dialogs/dialog_color_picker.h>
#include <widgets/paged_dialog.h>
#include <widgets/layer_box_selector.h>
#include <wx/log.h>
#include <wx/rawbmp.h>
#include <math/util.h>      // for KiROUND

#include "panel_board_stackup.h"
#include <panel_setup_layers.h>
#include "board_stackup_reporter.h"
#include <bitmaps.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include "dialog_dielectric_list_manager.h"
#include <wx/wupdlock.h>
#include <wx/richmsgdlg.h>
#include <wx/dcclient.h>
#include <wx/treebook.h>
#include <wx/textdlg.h>

#include <locale_io.h>
#include <eda_list_dialog.h>
#include <string_utils.h>               // for Double2Str()


// Some wx widget ID to know what widget has fired a event:
#define ID_INCREMENT 256    // space between 2 ID type. Bigger than the layer count max

// The actual widget IDs are the base id + the row index.
// they are used in events to know the row index of the control that fired the event
enum WIDGETS_IDS
{
    ID_ITEM_MATERIAL = 10000,       // Be sure it is higher than other IDs
                                    // used in the board setup dialog
    ID_ITEM_THICKNESS = ID_ITEM_MATERIAL + ID_INCREMENT,
    ID_ITEM_THICKNESS_LOCKED = ID_ITEM_THICKNESS + ID_INCREMENT,
    ID_ITEM_COLOR = ID_ITEM_THICKNESS_LOCKED + ID_INCREMENT,
};

// Default colors to draw icons:
static wxColor copperColor( 220, 180, 30 );
static wxColor dielectricColor( 75, 120, 75 );
static wxColor pasteColor( 200, 200, 200 );

static void drawBitmap( wxBitmap& aBitmap, wxColor aColor );


wxString getColourAsHexString( const wxColour aColour )
{
    // NB: wxWidgets 3.0's color.GetAsString( wxC2S_HTML_SYNTAX ) pukes on alpha
    return wxString::Format( wxT("#%02X%02X%02X%02X" ),
                             aColour.Red(),
                             aColour.Green(),
                             aColour.Blue(),
                             aColour.Alpha() );
}


PANEL_SETUP_BOARD_STACKUP::PANEL_SETUP_BOARD_STACKUP( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame,
                                                      PANEL_SETUP_LAYERS* aPanelLayers ):
        PANEL_SETUP_BOARD_STACKUP_BASE( aParent->GetTreebook() ),
        m_delectricMatList( DIELECTRIC_SUBSTRATE_LIST::DL_MATERIAL_DIELECTRIC ),
        m_solderMaskMatList( DIELECTRIC_SUBSTRATE_LIST::DL_MATERIAL_SOLDERMASK ),
        m_silkscreenMatList( DIELECTRIC_SUBSTRATE_LIST::DL_MATERIAL_SILKSCREEN )
{
    m_parentDialog = aParent;
    m_frame = aFrame;
    m_panelLayers = aPanelLayers;
    m_board = m_frame->GetBoard();
    m_brdSettings = &m_board->GetDesignSettings();
    m_units = aFrame->GetUserUnits();

    m_panelLayers->SetPhysicalStackupPanel( this );

    m_enabledLayers = m_board->GetEnabledLayers() & BOARD_STACKUP::StackupAllowedBrdLayers();

    // Calculates a good size for color swatches (icons) in this dialog
    wxClientDC dc( this );
    m_colorSwatchesSize = dc.GetTextExtent( "XX" );
    m_colorIconsSize = dc.GetTextExtent( "XXXX" );

    // Calculates a good size for wxTextCtrl to enter Epsilon R and Loss tan
    // ("0.0000000" + margins)
    m_numericFieldsSize = dc.GetTextExtent( "X.XXXXXXX" );
    m_numericFieldsSize.y = -1;     // Use default for the vertical size

    // Calculates a minimal size for wxTextCtrl to enter a dim with units
    // ("000.0000000 mils" + margins)
    m_numericTextCtrlSize = dc.GetTextExtent( "XXX.XXXXXXX mils" );
    m_numericTextCtrlSize.y = -1;     // Use default for the vertical size

    // The grid column containing the lock checkbox is kept to a minimal
    // size. So we use a wxStaticBitmap: set the bitmap itself
    m_bitmapLockThickness->SetBitmap( KiScaledBitmap( BITMAPS::locked, aFrame ) );

    // Gives a minimal size of wxTextCtrl showing dimensions+units
    m_tcCTValue->SetMinSize( m_numericTextCtrlSize );

    // Prepare dielectric layer type: layer type keyword is "core" or "prepreg"
    m_core_prepreg_choice.Add( _( "Core" ) );
    m_core_prepreg_choice.Add( _( "PrePreg" ) );

    buildLayerStackPanel( true );
    synchronizeWithBoard( true );
    computeBoardThickness();
}


PANEL_SETUP_BOARD_STACKUP::~PANEL_SETUP_BOARD_STACKUP()
{
    disconnectEvents();
}


void PANEL_SETUP_BOARD_STACKUP::onCopperLayersSelCount( wxCommandEvent& event )
{
    updateCopperLayerCount();
    showOnlyActiveLayers();
    computeBoardThickness();
    Layout();
}


void PANEL_SETUP_BOARD_STACKUP::onAdjustDielectricThickness( wxCommandEvent& event )
{
    // The list of items that can be modified:
    std::vector< BOARD_STACKUP_ROW_UI_ITEM* > items_candidate;

    // Some dielectric layers can have a locked thickness, so calculate the min
    // acceptable thickness
    int min_thickness = 0;

    for( BOARD_STACKUP_ROW_UI_ITEM& ui_item : m_rowUiItemsList )
    {
        BOARD_STACKUP_ITEM* item = ui_item.m_Item;

        if( !item->IsThicknessEditable() || !ui_item.m_isEnabled )
            continue;

        // We are looking for locked thickness items only:
        wxCheckBox* cb_box = dynamic_cast<wxCheckBox*> ( ui_item.m_ThicknessLockCtrl );

        if( cb_box && !cb_box->GetValue() )
        {
            items_candidate.push_back( &ui_item );
            continue;
        }

        wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( ui_item.m_ThicknessCtrl );
        wxString txt = textCtrl->GetValue();

        int item_thickness = ValueFromString( m_frame->GetUserUnits(), txt );
        min_thickness += item_thickness;
    }

    wxString title;

    if( min_thickness == 0 )
    {
        title.Printf( _( "Enter board thickness in %s:" ),
                      GetAbbreviatedUnitsLabel( m_frame->GetUserUnits() ).Trim( false ) );
    }
    else
    {
        title.Printf( _( "Enter expected board thickness (min value %s):" ),
                      StringFromValue( m_frame->GetUserUnits(), min_thickness, true ) );
    }

    wxTextEntryDialog dlg( this, title, _( "Adjust Unlocked Dielectric Layers" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString result = dlg.GetValue();

    int iu_thickness = ValueFromString( m_frame->GetUserUnits(), result );

    if( iu_thickness <= min_thickness )
    {
        wxMessageBox( wxString::Format( _("Value too small (min value %s)." ),
                      StringFromValue( m_frame->GetUserUnits(), min_thickness, true ) ) );
        return;
    }

    // Now adjust not locked dielectric thickness layers:

    if( items_candidate.size() )
    {
        int thickness_layer = ( iu_thickness - min_thickness ) / items_candidate.size();
        wxString txt = StringFromValue( m_frame->GetUserUnits(), thickness_layer );

        for( BOARD_STACKUP_ROW_UI_ITEM* ui_item : items_candidate )
        {
            wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( ui_item->m_ThicknessCtrl );
            textCtrl->SetValue( txt );
        }
    }
    else
    {
        wxMessageBox( _( "All dielectric  thickness layers are locked" ) );
    }

    computeBoardThickness();
}


void PANEL_SETUP_BOARD_STACKUP::disconnectEvents()
{
	// Disconnect Events connected to items in m_controlItemsList
    for( wxControl* item: m_controlItemsList )
    {
        wxBitmapComboBox* cb = dynamic_cast<wxBitmapComboBox*>( item );

        if( cb )
        {
            cb->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED,
                            wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onColorSelected ),
                            nullptr, this );
        }

        wxButton* matButt = dynamic_cast<wxButton*>( item );

        if( matButt )
        {
            matButt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                 wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onMaterialChange ),
                                 nullptr, this );
        }

        wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( item );

        if( textCtrl )
        {
            textCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED,
                                  wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onThicknessChange ),
                                  nullptr, this );
        }
    }
}


void PANEL_SETUP_BOARD_STACKUP::onAddDielectricLayer( wxCommandEvent& event )
{
    wxArrayString headers;
    headers.Add( _( "Layers" ) );

    // Build Dielectric layers list:
    std::vector<wxArrayString> d_list;
    std::vector<int>           rows;  // indexes of row values for each selectable item
    int                        row = -1;

    for( BOARD_STACKUP_ROW_UI_ITEM& item : m_rowUiItemsList )
    {
        row++;

        if( !item.m_isEnabled )
            continue;

        BOARD_STACKUP_ITEM* brd_stackup_item = item.m_Item;

        if( brd_stackup_item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        {
            wxArrayString d_item;

            if( brd_stackup_item->GetSublayersCount() > 1 )
            {
                d_item.Add( wxString::Format( _( "Layer '%s' (sublayer %d/%d)" ),
                                              brd_stackup_item->FormatDielectricLayerName(),
                                              item.m_SubItem+1,
                                              brd_stackup_item->GetSublayersCount() ) );
            }
            else
            {
                d_item.Add( brd_stackup_item->FormatDielectricLayerName() );
            }

            d_list.emplace_back( d_item );
            rows.push_back( row );
        }
    }

    EDA_LIST_DIALOG dlg( m_parentDialog, _( "Add Dielectric Layer" ), headers, d_list );
    dlg.SetListLabel( _( "Select layer to add:" ) );
    dlg.HideFilter();

    if( dlg.ShowModal() == wxID_OK && dlg.GetSelection() >= 0 )
    {
        row = rows[ dlg.GetSelection() ];

        BOARD_STACKUP_ITEM* brd_stackup_item = m_rowUiItemsList[row].m_Item;
        int new_sublayer = m_rowUiItemsList[row].m_SubItem;

        // Insert a new item after the selected item
        brd_stackup_item->AddDielectricPrms( new_sublayer+1 );

        rebuildLayerStackPanel();
        computeBoardThickness();
    }
}


void PANEL_SETUP_BOARD_STACKUP::onRemoveDielectricLayer( wxCommandEvent& event )
{
    wxArrayString headers;
    headers.Add( _( "Layers" ) );

    // Build deletable Dielectric layers list.
    // A layer can be deleted if there are 2 (or more) dielectric sub-layers
    // between 2 copper layers
    std::vector<wxArrayString> d_list;
    std::vector<int>           rows;      // indexes of row values for each selectable item
    int                        row = 0;   // row index in m_rowUiItemsList of items in choice list

    // Build the list of dielectric layers:
    for( BOARD_STACKUP_ITEM* item : m_stackup.GetList() )
    {
        if( !item->IsEnabled() || item->GetType() != BS_ITEM_TYPE_DIELECTRIC ||
            item->GetSublayersCount() <= 1 )
        {
            row++;
            continue;
        }

        for( int ii = 0; ii < item->GetSublayersCount(); ii++ )
        {
            wxArrayString d_item;

            d_item.Add( wxString::Format( _( "Layer '%s' sublayer %d/%d" ),
                                          item->FormatDielectricLayerName(),
                                          ii+1,
                                          item->GetSublayersCount() ) );

            d_list.emplace_back( d_item );
            rows.push_back( row++ );
        }
    }

    EDA_LIST_DIALOG dlg( m_parentDialog, _( "Remove Dielectric Layer" ), headers, d_list );
    dlg.SetListLabel( _( "Select layer to remove:" ) );
    dlg.HideFilter();

    if( dlg.ShowModal() == wxID_OK && dlg.GetSelection() >= 0 )
    {
        row = rows[ dlg.GetSelection() ];
        BOARD_STACKUP_ITEM* brd_stackup_item = m_rowUiItemsList[ row ].m_Item;
        int                 sublayer = m_rowUiItemsList[ row ].m_SubItem;

        // Remove the selected sub item for the selected dielectric layer
        brd_stackup_item->RemoveDielectricPrms( sublayer );

        rebuildLayerStackPanel();
        computeBoardThickness();
    }
}


void PANEL_SETUP_BOARD_STACKUP::onRemoveDielUI( wxUpdateUIEvent& event )
{
    // The m_buttonRemoveDielectricLayer wxButton is enabled only if a dielectric
    // layer can be removed, i.e. if dielectric layers have sublayers
    for( BOARD_STACKUP_ITEM* item : m_stackup.GetList() )
    {
        if( !item->IsEnabled() || item->GetType() != BS_ITEM_TYPE_DIELECTRIC )
           continue;

        if( item->GetSublayersCount() > 1 )
        {
            event.Enable( true );
            return;
        }
    }

    event.Enable( false );
}


void PANEL_SETUP_BOARD_STACKUP::onExportToClipboard( wxCommandEvent& event )
{
    if( !transferDataFromUIToStackup() )
        return;

    // Build a ASCII representation of stackup and copy it in the clipboard
    wxString report = BuildStackupReport( m_stackup, m_units );

    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( wxTheClipboard->Open() )
    {
        // This data objects are held by the clipboard,
        // so do not delete them in the app.
        wxTheClipboard->SetData( new wxTextDataObject( report ) );
        wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
        wxTheClipboard->Close();
    }
}


wxColor PANEL_SETUP_BOARD_STACKUP::GetSelectedColor( int aRow ) const
{
    const BOARD_STACKUP_ROW_UI_ITEM& row = m_rowUiItemsList[aRow];
    const BOARD_STACKUP_ITEM*        item = row.m_Item;
    const wxBitmapComboBox*          choice = dynamic_cast<wxBitmapComboBox*>( row.m_ColorCtrl );
    wxASSERT( choice );

    int idx = choice ? choice->GetSelection() : 0;

    if( idx != GetColorUserDefinedListIdx() ) // a standard color is selected
        return GetColorStandardList()[idx].GetColor( item->GetType() );
    else
        return m_rowUiItemsList[aRow].m_UserColor;
}


void PANEL_SETUP_BOARD_STACKUP::computeBoardThickness()
{
    int thickness = 0;

    for( BOARD_STACKUP_ROW_UI_ITEM& ui_item : m_rowUiItemsList )
    {
        BOARD_STACKUP_ITEM* item = ui_item.m_Item;

        if( !item->IsThicknessEditable() || !ui_item.m_isEnabled )
            continue;

        wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( ui_item.m_ThicknessCtrl );
        wxString txt = textCtrl->GetValue();

        int item_thickness = ValueFromString( m_frame->GetUserUnits(), txt );
        thickness += item_thickness;
    }

    wxString thicknessStr = StringFromValue( m_units, thickness, true );

    // The text in the event will translate to the value for the text control
    // and is only updated if it changed
    m_tcCTValue->SetValue( thicknessStr );
}


int PANEL_SETUP_BOARD_STACKUP::GetCopperLayerCount() const
{
    return ( m_choiceCopperLayers->GetSelection() + 1 ) * 2;
}


void PANEL_SETUP_BOARD_STACKUP::updateCopperLayerCount()
{
    int copperCount = GetCopperLayerCount();

    wxASSERT( copperCount >= 2 );

    m_enabledLayers |= LSET::ExternalCuMask();
    m_enabledLayers &= ~LSET::InternalCuMask();

    for( int i = 1; i < copperCount - 1; i++ )
        m_enabledLayers.set( F_Cu + i );
}


void PANEL_SETUP_BOARD_STACKUP::synchronizeWithBoard( bool aFullSync )
{
    const BOARD_STACKUP&   brd_stackup = m_brdSettings->GetStackupDescriptor();
    const FAB_LAYER_COLOR* color_list = GetColorStandardList();

    if( aFullSync )
    {
        m_choiceCopperLayers->SetSelection( ( m_board->GetCopperLayerCount() / 2 ) - 1 );
        m_impedanceControlled->SetValue( brd_stackup.m_HasDielectricConstrains );
    }

    for( BOARD_STACKUP_ROW_UI_ITEM& ui_row_item : m_rowUiItemsList )
    {
        BOARD_STACKUP_ITEM* item = ui_row_item.m_Item;
        int sub_item = ui_row_item.m_SubItem;

        if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        {
            wxChoice* choice = dynamic_cast<wxChoice*>( ui_row_item.m_LayerTypeCtrl );

            if( choice )
                choice->SetSelection( item->GetTypeName() == KEY_CORE ? 0 : 1 );
        }

        if( item->IsMaterialEditable() )
        {
            wxTextCtrl* matName = dynamic_cast<wxTextCtrl*>( ui_row_item.m_MaterialCtrl );

            if( matName )
            {
                if( IsPrmSpecified( item->GetMaterial( sub_item ) ) )
                    matName->SetValue( item->GetMaterial( sub_item ) );
                else
                    matName->SetValue( wxGetTranslation( NotSpecifiedPrm() ) );
            }
        }

        if( item->IsThicknessEditable() )
        {
            wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ui_row_item.m_ThicknessCtrl );

            if( textCtrl )
                textCtrl->SetValue( StringFromValue( m_units,
                                        item->GetThickness( sub_item ), true ) );

            if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
            {
                wxCheckBox* cb_box = dynamic_cast<wxCheckBox*> ( ui_row_item.m_ThicknessLockCtrl );

                if( cb_box )
                    cb_box->SetValue( item->IsThicknessLocked( sub_item ) );
            }
        }

        if( item->IsColorEditable() )
        {
            auto bm_combo = dynamic_cast<wxBitmapComboBox*>( ui_row_item.m_ColorCtrl );

            if( item->GetColor().StartsWith( "#" ) )  // User defined color
            {
                ui_row_item.m_UserColor = wxColour( item->GetColor() );

                if( bm_combo )      // Update user color shown in the wxBitmapComboBox
                {
                    bm_combo->SetString( GetColorUserDefinedListIdx(), item->GetColor() );
                    wxBitmap layerbmp( m_colorSwatchesSize.x, m_colorSwatchesSize.y );
                    LAYER_SELECTOR::DrawColorSwatch( layerbmp, COLOR4D(),
                                                     wxColour( item->GetColor() ) );
                    bm_combo->SetItemBitmap( GetColorUserDefinedListIdx(), layerbmp );
                    bm_combo->SetSelection( GetColorUserDefinedListIdx() );
                }
            }
            else
            {
                if( bm_combo )
                {
                    // Note: don't use bm_combo->FindString() because the combo strings are
                    // translated.
                    for( int ii = 0; ii < GetColorStandardListCount(); ii++ )
                    {
                        if( color_list[ii].GetName() == item->GetColor() )
                        {
                            bm_combo->SetSelection( ii );
                            break;
                        }
                    }
                }
            }

        }

        if( item->HasEpsilonRValue() )
        {
            wxString txt = Double2Str( item->GetEpsilonR( sub_item ) );
            wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ui_row_item.m_EpsilonCtrl );

            if( textCtrl )
                textCtrl->SetValue( txt );
        }

        if( item->HasLossTangentValue() )
        {
            wxString txt = Double2Str( item->GetLossTangent( sub_item ) );
            wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ui_row_item.m_LossTgCtrl );

            if( textCtrl )
                textCtrl->SetValue( txt );
        }
    }

    // Now enable/disable stackup items, according to the m_enabledLayers config
    showOnlyActiveLayers();

    updateIconColor();
}


void PANEL_SETUP_BOARD_STACKUP::showOnlyActiveLayers()
{

    // Now enable/disable stackup items, according to the m_enabledLayers config
    // Calculate copper layer count from m_enabledLayers, and *do not use* brd_stackup
    // for that, because it is not necessary up to date
    // (for instance after modifying the layer count from the panel layers in dialog)
    LSET copperMask = m_enabledLayers & ( LSET::ExternalCuMask() | LSET::InternalCuMask() );
    int copperLayersCount = copperMask.count();

    for( BOARD_STACKUP_ROW_UI_ITEM& ui_row_item: m_rowUiItemsList )
    {
        bool show_item;
        BOARD_STACKUP_ITEM* item = ui_row_item.m_Item;

        if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
            // the m_DielectricLayerId is not a copper layer id, it is a dielectric idx from 1
            show_item = item->GetDielectricLayerId() < copperLayersCount;
        else
            show_item = m_enabledLayers[item->GetBrdLayerId()];

        item->SetEnabled( show_item );

        ui_row_item.m_isEnabled = show_item;

        // Show or not items of this row:
        ui_row_item.m_Icon->Show( show_item );
        ui_row_item.m_LayerName->Show( show_item );
        ui_row_item.m_LayerTypeCtrl->Show( show_item );
        ui_row_item.m_MaterialCtrl->Show( show_item );

        if( ui_row_item.m_MaterialButt )
            ui_row_item.m_MaterialButt->Show( show_item );

        ui_row_item.m_ThicknessCtrl->Show( show_item );
        ui_row_item.m_ThicknessLockCtrl->Show( show_item );
        ui_row_item.m_ColorCtrl->Show( show_item );
        ui_row_item.m_EpsilonCtrl->Show( show_item );
        ui_row_item.m_LossTgCtrl->Show( show_item );
    }
}


void PANEL_SETUP_BOARD_STACKUP::addMaterialChooser( wxWindowID aId, const wxString* aMaterialName,
                                                    BOARD_STACKUP_ROW_UI_ITEM& aUiRowItem )
{
	wxBoxSizer* bSizerMat = new wxBoxSizer( wxHORIZONTAL );
	m_fgGridSizer->Add( bSizerMat, 1, wxRIGHT|wxEXPAND, 4 );
    wxTextCtrl* textCtrl = new wxTextCtrl( m_scGridWin, wxID_ANY );

    if( aMaterialName )
    {
        if( IsPrmSpecified( *aMaterialName ) )
            textCtrl->SetValue( *aMaterialName );
        else
            textCtrl->SetValue( wxGetTranslation( NotSpecifiedPrm() ) );
    }

    textCtrl->SetMinSize( m_numericTextCtrlSize );
	bSizerMat->Add( textCtrl, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxButton* m_buttonMat = new wxButton( m_scGridWin, aId, _( "..." ), wxDefaultPosition,
                                          wxDefaultSize, wxBU_EXACTFIT );
	bSizerMat->Add( m_buttonMat, 0, wxALIGN_CENTER_VERTICAL, 2 );

    m_buttonMat->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                          wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onMaterialChange ),
                          nullptr, this );
    m_controlItemsList.push_back( m_buttonMat );

    aUiRowItem.m_MaterialCtrl = textCtrl;
    aUiRowItem.m_MaterialButt = m_buttonMat;
}


wxControl* PANEL_SETUP_BOARD_STACKUP::addSpacer()
{
    wxStaticText* emptyText = new wxStaticText( m_scGridWin, wxID_ANY, wxEmptyString );
    m_fgGridSizer->Add( emptyText, 0, wxALIGN_CENTER_VERTICAL );
    return emptyText;
}


BOARD_STACKUP_ROW_UI_ITEM PANEL_SETUP_BOARD_STACKUP::createRowData( int aRow,
                                                                    BOARD_STACKUP_ITEM* aStackupItem,
                                                                    int aSublayerIdx )
{
    wxASSERT( aStackupItem );
    wxASSERT( aSublayerIdx >= 0 && aSublayerIdx < aStackupItem->GetSublayersCount() );

    BOARD_STACKUP_ROW_UI_ITEM ui_row_item( aStackupItem, aSublayerIdx );
    BOARD_STACKUP_ITEM* item = aStackupItem;
    int row = aRow;

    const FAB_LAYER_COLOR* color_list = GetColorStandardList();

    // Add color swatch icon. The color will be updated later,
    // when all widgets are initialized
    wxStaticBitmap* bitmap = new wxStaticBitmap( m_scGridWin, wxID_ANY, wxNullBitmap );
    m_fgGridSizer->Add( bitmap, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 4 );
    ui_row_item.m_Icon = bitmap;

    ui_row_item.m_isEnabled = true;

    if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
    {
        wxString lname = item->FormatDielectricLayerName();

        if( item->GetSublayersCount() > 1 )
        {
            lname <<  "  (" << aSublayerIdx+1 << "/" << item->GetSublayersCount() << ")";
        }

        wxStaticText* st_text = new wxStaticText( m_scGridWin, wxID_ANY, lname );
        m_fgGridSizer->Add( st_text, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
        ui_row_item.m_LayerName = st_text;

        // For a dielectric layer, the layer type choice is not for each sublayer,
        // only for the first (aSublayerIdx = 0), and is common to all sublayers
        if( aSublayerIdx == 0 )
        {
            wxChoice* choice = new wxChoice( m_scGridWin, wxID_ANY, wxDefaultPosition,
                                             wxDefaultSize, m_core_prepreg_choice );
            choice->SetSelection( item->GetTypeName() == KEY_CORE ? 0 : 1 );
            m_fgGridSizer->Add( choice, 1, wxEXPAND|wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );

            ui_row_item.m_LayerTypeCtrl = choice;
        }
        else
        {
            ui_row_item.m_LayerTypeCtrl = addSpacer();
        }
    }
    else
    {
        item->SetLayerName( m_board->GetLayerName( item->GetBrdLayerId() ) );
        wxStaticText* st_text =  new wxStaticText( m_scGridWin, wxID_ANY, item->GetLayerName() );
        m_fgGridSizer->Add( st_text, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 1 );
        st_text->Show( true );
        ui_row_item.m_LayerName = st_text;

        wxString lname;

        if( item->GetTypeName() == KEY_COPPER )
            lname = _( "Copper" );
        else
            lname = wxGetTranslation( item->GetTypeName() );

        st_text = new wxStaticText( m_scGridWin, wxID_ANY, lname );
        m_fgGridSizer->Add( st_text, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
        ui_row_item.m_LayerTypeCtrl = st_text;
    }

    if( item->IsMaterialEditable() )
    {
        wxString matName = item->GetMaterial( aSublayerIdx );
        addMaterialChooser( ID_ITEM_MATERIAL+row, &matName, ui_row_item );
    }
    else
    {
        ui_row_item.m_MaterialCtrl = addSpacer();
    }

    if( item->IsThicknessEditable() )
    {
        wxTextCtrl* textCtrl = new wxTextCtrl( m_scGridWin, ID_ITEM_THICKNESS+row );
        textCtrl->SetMinSize( m_numericTextCtrlSize );
        textCtrl->SetValue( StringFromValue( m_units, item->GetThickness( aSublayerIdx ), true ) );
        m_fgGridSizer->Add( textCtrl, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
        m_controlItemsList.push_back( textCtrl );
        textCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED,
                           wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onThicknessChange ),
                           nullptr, this );
        ui_row_item.m_ThicknessCtrl = textCtrl;

        if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        {
            wxCheckBox* cb_box = new wxCheckBox( m_scGridWin, ID_ITEM_THICKNESS_LOCKED+row,
                                                 wxEmptyString );
            cb_box->SetValue( item->IsThicknessLocked( aSublayerIdx ) );
            m_fgGridSizer->Add( cb_box, 0, wxALIGN_CENTER_VERTICAL, 2 );
            ui_row_item.m_ThicknessLockCtrl = cb_box;
        }
        else
        {
            ui_row_item.m_ThicknessLockCtrl = addSpacer();
        }
    }
    else
    {
        ui_row_item.m_ThicknessCtrl = addSpacer();
        ui_row_item.m_ThicknessLockCtrl = addSpacer();
    }

    if( item->IsColorEditable() )
    {
        if( item->GetColor().StartsWith( "#" ) )  // User defined color
            ui_row_item.m_UserColor = wxColour( item->GetColor() );
        else
            ui_row_item.m_UserColor = GetDefaultUserColor( item->GetType() );

        wxBitmapComboBox* bm_combo = createColorBox( item, row );
        m_fgGridSizer->Add( bm_combo, 1, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 2 );

        if( item->GetColor().StartsWith( "#" ) )
        {
            bm_combo->SetString( GetColorUserDefinedListIdx(), item->GetColor() );
            bm_combo->SetSelection( GetColorUserDefinedListIdx() );
        }
        else
        {
            // Note: don't use bm_combo->FindString() because the combo strings are translated.
            for( int ii = 0; ii < GetColorStandardListCount(); ii++ )
            {
                if( color_list[ii].GetName() == item->GetColor() )
                {
                    bm_combo->SetSelection( ii );
                    break;
                }
            }
        }

        ui_row_item.m_ColorCtrl = bm_combo;
    }
    else
    {
        ui_row_item.m_ColorCtrl = addSpacer();
    }

    if( item->HasEpsilonRValue() )
    {
        wxString txt = Double2Str( item->GetEpsilonR( aSublayerIdx ) );
        wxTextCtrl* textCtrl = new wxTextCtrl( m_scGridWin, wxID_ANY, wxEmptyString,
                                               wxDefaultPosition, m_numericFieldsSize );
        textCtrl->SetValue( txt );
        m_fgGridSizer->Add( textCtrl, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
        ui_row_item.m_EpsilonCtrl = textCtrl;
    }
    else
    {
        ui_row_item.m_EpsilonCtrl = addSpacer();
    }

    if( item->HasLossTangentValue() )
    {
        wxString txt = Double2Str( item->GetLossTangent( aSublayerIdx ) );;
        wxTextCtrl* textCtrl = new wxTextCtrl( m_scGridWin, wxID_ANY, wxEmptyString,
                                               wxDefaultPosition, m_numericFieldsSize );
        textCtrl->SetValue( txt );
        m_fgGridSizer->Add( textCtrl, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
        ui_row_item.m_LossTgCtrl = textCtrl;
    }
    else
    {
        ui_row_item.m_LossTgCtrl = addSpacer();
    }

    return ui_row_item;
}


void PANEL_SETUP_BOARD_STACKUP::rebuildLayerStackPanel()
{
    wxWindowUpdateLocker locker( m_scGridWin );
    m_scGridWin->Hide();

    // Rebuild the stackup for the dialog, after dielectric parameters list is modified
    // (added/removed):

    // First, delete all ui objects, because wxID values will be no longer valid for many widgets
    disconnectEvents();
    m_controlItemsList.clear();

    // Delete widgets (handled by the wxPanel parent)
    for( BOARD_STACKUP_ROW_UI_ITEM ui_item: m_rowUiItemsList )
    {
        // This remove and delete the current ui_item.m_MaterialCtrl sizer
        ui_item.m_MaterialCtrl->SetSizer( nullptr );

        // Delete other widgets
        delete ui_item.m_Icon;             // Color icon in first column (column 1)
        delete ui_item.m_LayerName;        // string shown in column 2
        delete ui_item.m_LayerTypeCtrl;    // control shown in column 3
        delete ui_item.m_MaterialCtrl;     // control shown in column 4, with m_MaterialButt
        delete ui_item.m_MaterialButt;     // control shown in column 4, with m_MaterialCtrl
        delete ui_item.m_ThicknessCtrl;    // control shown in column 5
        delete ui_item.m_ThicknessLockCtrl;// control shown in column 6
        delete ui_item.m_ColorCtrl;        // control shown in column 7
        delete ui_item.m_EpsilonCtrl;      // control shown in column 8
        delete ui_item.m_LossTgCtrl;       // control shown in column 9
    }

    m_rowUiItemsList.clear();

    // In order to recreate a clean grid layer list, we have to delete and
    // recreate the sizer m_fgGridSizer (just deleting items in this size is not enough)
    // therefore we also have to add the "old" title items to the newly recreated m_fgGridSizer:
	m_scGridWin->SetSizer( nullptr );   // This remove and delete the current m_fgGridSizer

    m_fgGridSizer = new wxFlexGridSizer( 0, 9, 0, 2 );
	m_fgGridSizer->SetFlexibleDirection( wxHORIZONTAL );
	m_fgGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	m_fgGridSizer->SetHGap( 6 );
	m_scGridWin->SetSizer( m_fgGridSizer );

    // Re-add "old" title items:
    const int sizer_flags = wxALIGN_CENTER_VERTICAL | wxALL | wxALIGN_CENTER_HORIZONTAL;
	m_fgGridSizer->Add( m_staticTextLayer, 0, sizer_flags, 2 );
	m_fgGridSizer->Add( m_staticTextType, 0, sizer_flags, 2 );
	m_fgGridSizer->Add( m_staticTextLayerId, 0, sizer_flags, 5 );
	m_fgGridSizer->Add( m_staticTextMaterial, 0, sizer_flags, 2 );
	m_fgGridSizer->Add( m_staticTextThickness, 0, sizer_flags, 2 );
	m_fgGridSizer->Add( m_bitmapLockThickness, 0, sizer_flags, 1 );
	m_fgGridSizer->Add( m_staticTextColor, 0, sizer_flags, 2 );
	m_fgGridSizer->Add( m_staticTextEpsilonR, 0, sizer_flags, 2 );
	m_fgGridSizer->Add( m_staticTextLossTg, 0, sizer_flags, 2 );


    // Now, rebuild the widget list from the new m_stackup items:
    buildLayerStackPanel( false );

    // Now enable/disable stackup items, according to the m_enabledLayers config
    showOnlyActiveLayers();

    m_scGridWin->Layout();
    m_scGridWin->Show();
}


void PANEL_SETUP_BOARD_STACKUP::buildLayerStackPanel( bool aCreatedInitialStackup )
{
    // Build a full stackup for the dialog, with a active copper layer count
    // = current board layer count to calculate a reasonable default stackup:
    if( aCreatedInitialStackup )
    {
        // Creates a full BOARD_STACKUP with 32 copper layers.
        // extra layers will be hidden later.
        // but if the number of layer is changed in the dialog, the corresponding
        // widgets will be available with their previous values.
        m_stackup.BuildDefaultStackupList( nullptr, m_brdSettings->GetCopperLayerCount() );
        const BOARD_STACKUP& brd_stackup = m_brdSettings->GetStackupDescriptor();

        // Now initialize all stackup items to the board values, when exist
        for( BOARD_STACKUP_ITEM* item: m_stackup.GetList() )
        {
            // Search for board settings:
            for( BOARD_STACKUP_ITEM* board_item: brd_stackup.GetList() )
            {
                if( item->GetBrdLayerId() != UNDEFINED_LAYER )
                {
                    if( item->GetBrdLayerId() == board_item->GetBrdLayerId() )
                    {
                        *item = *board_item;
                        break;
                    }
                }
                else    // dielectric layer: see m_DielectricLayerId for identification
                {
                    // Compare dielectric layer with dielectric layer
                    if( board_item->GetBrdLayerId() != UNDEFINED_LAYER )
                        continue;

                    if( item->GetDielectricLayerId() == board_item->GetDielectricLayerId() )
                    {
                        *item = *board_item;
                        break;
                    }
                }
            }
        }
    }

    int row = 0;

    for( BOARD_STACKUP_ITEM* item : m_stackup.GetList() )
    {
        for( int sub_idx = 0; sub_idx < item->GetSublayersCount(); sub_idx++ )
        {
            BOARD_STACKUP_ROW_UI_ITEM ui_row_item = createRowData( row, item, sub_idx );
            m_rowUiItemsList.emplace_back( ui_row_item );

            row++;
        }
    }

    updateIconColor();
}


// Transfer current UI settings to m_stackup but not to the board
bool PANEL_SETUP_BOARD_STACKUP::transferDataFromUIToStackup()
{
    wxString txt;
    wxString error_msg;
    bool success = true;
    double value;
    int row = 0;

    for( BOARD_STACKUP_ROW_UI_ITEM& ui_item : m_rowUiItemsList )
    {
        // Skip stackup items useless for the current board
        if( !ui_item.m_isEnabled )
        {
            row++;
            continue;
        }

        BOARD_STACKUP_ITEM* item = ui_item.m_Item;
        int sub_item = ui_item.m_SubItem;

        // Add sub layer if there is a new sub layer:
        while( item->GetSublayersCount() <= sub_item )
            item->AddDielectricPrms( item->GetSublayersCount() );

        if( sub_item == 0 )     // Name only main layer
            item->SetLayerName( ui_item.m_LayerName->GetLabel() );

        if( item->HasEpsilonRValue() )
        {
            wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( ui_item.m_EpsilonCtrl );
            txt = textCtrl->GetValue();

            if( txt.ToDouble( &value ) && value >= 0.0 )
                item->SetEpsilonR( value, sub_item );
            else if( txt.ToCDouble( &value ) && value >= 0.0 )
                item->SetEpsilonR( value, sub_item );
            else
            {
                success = false;
                error_msg << _( "Incorrect value for Epsilon R (Epsilon R must be positive or "
                                "null if not used)" );
            }
        }

        if( item->HasLossTangentValue() )
        {
            wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( ui_item.m_LossTgCtrl );
            txt = textCtrl->GetValue();

            if( txt.ToDouble( &value ) && value >= 0.0 )
                item->SetLossTangent( value, sub_item );
            else if( txt.ToCDouble( &value ) && value >= 0.0 )
                item->SetLossTangent( value, sub_item );
            else
            {
                success = false;

                if( !error_msg.IsEmpty() )
                    error_msg << "\n";

                error_msg << _( "Incorrect value for Loss tg (Loss tg must be positive or null "
                                "if not used)" );
            }
        }

        if( item->IsMaterialEditable() )
        {
            wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( ui_item.m_MaterialCtrl );
            item->SetMaterial( textCtrl->GetValue(), sub_item );

            // Ensure the not specified mat name is the keyword, not its translation
            // to avoid any issue is the language setting changes
            if( !IsPrmSpecified( item->GetMaterial( sub_item ) ) )
                item->SetMaterial( NotSpecifiedPrm(), sub_item );
        }

        if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        {
            // Choice is Core or Prepreg. Sublayers have no choice:
            wxChoice* choice = dynamic_cast<wxChoice*>( ui_item.m_LayerTypeCtrl );

            if( choice )
            {
                int idx = choice->GetSelection();

                if( idx == 0 )
                    item->SetTypeName( KEY_CORE );
                else
                    item->SetTypeName( KEY_PREPREG );
            }
        }

        if( item->IsThicknessEditable() )
        {
            wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( ui_item.m_ThicknessCtrl );
            txt = textCtrl->GetValue();

            int new_thickness = ValueFromString( m_frame->GetUserUnits(), txt );
            item->SetThickness( new_thickness, sub_item );

            if( new_thickness < 0 )
            {
                success = false;

                if( !error_msg.IsEmpty() )
                    error_msg << "\n";

                error_msg << _( "A layer thickness is < 0. Fix it" );
            }

            if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
            {
                // Dielectric thickness layer can have a locked thickness:
                wxCheckBox* cb_box = static_cast<wxCheckBox*>
                                        ( ui_item.m_ThicknessLockCtrl );
                item->SetThicknessLocked( cb_box && cb_box->GetValue(), sub_item );
            }
        }

        if( sub_item == 0 && item->IsColorEditable() )
        {
            const FAB_LAYER_COLOR* color_list = GetColorStandardList();

            wxBitmapComboBox* choice = dynamic_cast<wxBitmapComboBox*>( ui_item.m_ColorCtrl );

            if( choice )
            {
                int idx = choice->GetSelection();

                if( idx == GetColorUserDefinedListIdx() )
                {
                    // NB: wxWidgets 3.0's color.GetAsString( wxC2S_HTML_SYNTAX ) pukes on alpha
                    item->SetColor( getColourAsHexString( ui_item.m_UserColor ) );
                }
                else
                {
                    item->SetColor( color_list[idx].GetName() );
                }
            }
        }

        row++;
    }

    if( !success )
    {
        wxMessageBox( error_msg, _( "Errors" ) );
        return false;
    }

    m_stackup.m_HasDielectricConstrains = m_impedanceControlled->GetValue();

    return true;
}


bool PANEL_SETUP_BOARD_STACKUP::TransferDataFromWindow()
{
    if( !transferDataFromUIToStackup() )
        return false;

    // NOTE: Copper layer count is transferred via PANEL_SETUP_LAYERS even though it is configured
    // on this page, because the logic for confirming deletion of board items on deleted layers is
    // on that panel and it doesn't make sense to split it up.

    BOARD_STACKUP& brd_stackup = m_brdSettings->GetStackupDescriptor();

    STRING_FORMATTER old_stackup;

    // FormatBoardStackup() (using FormatInternalUnits()) expects a "C" locale
    // to execute some tests. So switch to the suitable locale
    LOCALE_IO dummy;
    brd_stackup.FormatBoardStackup( &old_stackup, m_board, 0 );

    brd_stackup.m_FinishType = m_stackup.m_FinishType;
    brd_stackup.m_HasDielectricConstrains = m_stackup.m_HasDielectricConstrains;
    brd_stackup.m_EdgeConnectorConstraints = m_stackup.m_EdgeConnectorConstraints;
    brd_stackup.m_CastellatedPads = m_stackup.m_CastellatedPads;
    brd_stackup.m_EdgePlating = m_stackup.m_EdgePlating;

    // copy enabled items to the new board stackup
    brd_stackup.RemoveAll();

    for( BOARD_STACKUP_ITEM* item : m_stackup.GetList() )
    {
        if( item->IsEnabled() )
            brd_stackup.Add( new BOARD_STACKUP_ITEM( *item ) );
    }

    STRING_FORMATTER new_stackup;
    brd_stackup.FormatBoardStackup( &new_stackup, m_board, 0 );

    bool modified = old_stackup.GetString() != new_stackup.GetString();
    int thickness = brd_stackup.BuildBoardThicknessFromStackup();

    if( m_brdSettings->GetBoardThickness() != thickness )
    {
        m_brdSettings->SetBoardThickness( thickness );
        modified = true;
    }

    if( !m_brdSettings->m_HasStackup )
    {
        m_brdSettings->m_HasStackup = true;
        modified = true;
    }

    if( modified )
        m_frame->OnModify();

    return true;
}


void PANEL_SETUP_BOARD_STACKUP::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD* savedBrd = m_board;
    BOARD_DESIGN_SETTINGS* savedSettings = m_brdSettings;
    m_brdSettings = &aBoard->GetDesignSettings();

    m_enabledLayers = m_panelLayers->GetUILayerMask() & BOARD_STACKUP::StackupAllowedBrdLayers();
    synchronizeWithBoard( true );

    rebuildLayerStackPanel();
    computeBoardThickness();

    m_brdSettings = savedSettings;
    m_board = savedBrd;
}


void PANEL_SETUP_BOARD_STACKUP::OnLayersOptionsChanged( LSET aNewLayerSet )
{
    // First, verify the list of layers currently in stackup:
    // if it does not mach the list of layers set in PANEL_SETUP_LAYERS
    // rebuild the panel

    // the current enabled layers in PANEL_SETUP_LAYERS
    // Note: the number of layer can change, but not the layers properties
    LSET layersList = m_panelLayers->GetUILayerMask() & BOARD_STACKUP::StackupAllowedBrdLayers();

    if( m_enabledLayers != layersList )
    {
        m_enabledLayers = layersList;

        synchronizeWithBoard( false );

        Layout();
        Refresh();
    }
}


void PANEL_SETUP_BOARD_STACKUP::onColorSelected( wxCommandEvent& event )
{
    int idx = event.GetSelection();
    int item_id = event.GetId();

    int row = item_id - ID_ITEM_COLOR;

    if( idx == GetColorStandardListCount() - 1 )   // Set user color is the last option in list
    {
        DIALOG_COLOR_PICKER dlg( this, m_rowUiItemsList[row].m_UserColor, true, nullptr,
                                 GetDefaultUserColor( m_rowUiItemsList[row].m_Item->GetType() ) );

#ifdef __WXGTK__
        // Give a time-slice to close the menu before opening the dialog.
        // (Only matters on some versions of GTK.)
        wxSafeYield();
#endif

        if( dlg.ShowModal() == wxID_OK )
        {
            wxBitmapComboBox* combo = static_cast<wxBitmapComboBox*>( FindWindowById( item_id ) );
            wxColour          color = dlg.GetColor().ToColour();

            m_rowUiItemsList[row].m_UserColor = color;

            // NB: wxWidgets 3.0's color.GetAsString( wxC2S_HTML_SYNTAX ) pukes on alpha
            combo->SetString( idx, getColourAsHexString( color ) );

            wxBitmap layerbmp( m_colorSwatchesSize.x, m_colorSwatchesSize.y );
            LAYER_SELECTOR::DrawColorSwatch( layerbmp, COLOR4D( 0, 0, 0, 0 ), COLOR4D( color ) );
            combo->SetItemBitmap( combo->GetCount()-1, layerbmp );

            combo->SetSelection( idx );
        }
    }

    updateIconColor( row );
}


void PANEL_SETUP_BOARD_STACKUP::onMaterialChange( wxCommandEvent& event )
{
    // Ensure m_materialList contains all materials already in use in stackup list
    // and add it is missing
    if( !transferDataFromUIToStackup() )
        return;

    for( BOARD_STACKUP_ITEM* item : m_stackup.GetList() )
    {
        DIELECTRIC_SUBSTRATE_LIST* mat_list = nullptr;

        if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
            mat_list = &m_delectricMatList;
        else if( item->GetType() == BS_ITEM_TYPE_SOLDERMASK )
            mat_list = &m_solderMaskMatList;
        else if( item->GetType() == BS_ITEM_TYPE_SILKSCREEN )
            mat_list = &m_silkscreenMatList;

        else
            continue;

        for( int ii = 0; ii < item->GetSublayersCount(); ii++ )
        {
            int idx = mat_list->FindSubstrate( item->GetMaterial( ii ),
                                               item->GetEpsilonR( ii ),
                                               item->GetLossTangent( ii ) );

            if( idx < 0 && !item->GetMaterial().IsEmpty() )
            {
                // This material is not in list: add it
                DIELECTRIC_SUBSTRATE new_mat;
                new_mat.m_Name = item->GetMaterial( ii );
                new_mat.m_EpsilonR = item->GetEpsilonR( ii );
                new_mat.m_LossTangent = item->GetLossTangent( ii );
                mat_list->AppendSubstrate( new_mat );
            }
        }
    }

    int row  = event.GetId() - ID_ITEM_MATERIAL;
    BOARD_STACKUP_ITEM* item = m_rowUiItemsList[row].m_Item;
    int sub_item = m_rowUiItemsList[row].m_SubItem;
    DIELECTRIC_SUBSTRATE_LIST* item_mat_list = nullptr;

    switch( item->GetType() )
    {
    case BS_ITEM_TYPE_DIELECTRIC: item_mat_list = &m_delectricMatList;  break;
    case BS_ITEM_TYPE_SOLDERMASK: item_mat_list = &m_solderMaskMatList; break;
    case BS_ITEM_TYPE_SILKSCREEN: item_mat_list = &m_silkscreenMatList; break;
    default:                      item_mat_list = nullptr;              break;
    }

    wxCHECK( item_mat_list, /* void */ );

    DIALOG_DIELECTRIC_MATERIAL dlg( this, *item_mat_list );

    if( dlg.ShowModal() != wxID_OK )
        return;

    DIELECTRIC_SUBSTRATE substrate = dlg.GetSelectedSubstrate();

    if( substrate.m_Name.IsEmpty() )    // No substrate specified
        return;

    // Update Name, Epsilon R and Loss tg
    item->SetMaterial( substrate.m_Name, sub_item );
    item->SetEpsilonR( substrate.m_EpsilonR, sub_item );
    item->SetLossTangent( substrate.m_LossTangent, sub_item );

    wxTextCtrl* textCtrl;
    textCtrl = static_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_MaterialCtrl );
    textCtrl->SetValue( item->GetMaterial( sub_item ) );

    // some layers have a material choice but not EpsilonR ctrl
    if( item->HasEpsilonRValue() )
    {
        textCtrl = dynamic_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_EpsilonCtrl );

        if( textCtrl )
            textCtrl->SetValue( item->FormatEpsilonR( sub_item ) );
    }

    // some layers have a material choice but not loss tg ctrl
    if( item->HasLossTangentValue() )
    {
        textCtrl = dynamic_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_LossTgCtrl );

        if( textCtrl )
            textCtrl->SetValue( item->FormatLossTangent( sub_item ) );
    }
}


void PANEL_SETUP_BOARD_STACKUP::onThicknessChange( wxCommandEvent& event )
{
    int row  = event.GetId() - ID_ITEM_THICKNESS;
    wxString value = event.GetString();

    BOARD_STACKUP_ITEM* item = GetStackupItem( row );
    int idx = GetSublayerId( row );

    item->SetThickness( ValueFromString( m_frame->GetUserUnits(), value ), idx );

    computeBoardThickness();
}


BOARD_STACKUP_ITEM* PANEL_SETUP_BOARD_STACKUP::GetStackupItem( int aRow )
{
    return m_rowUiItemsList[aRow].m_Item;
}


int PANEL_SETUP_BOARD_STACKUP::GetSublayerId( int aRow )
{
    return m_rowUiItemsList[aRow].m_SubItem;
}


wxColor PANEL_SETUP_BOARD_STACKUP::getColorIconItem( int aRow )
{
    BOARD_STACKUP_ITEM* st_item = dynamic_cast<BOARD_STACKUP_ITEM*>( GetStackupItem( aRow ) );

    wxASSERT( st_item );
    wxColor color;

    if( ! st_item )
        return color;

    switch( st_item->GetType() )
    {
    case BS_ITEM_TYPE_COPPER:      color = copperColor;              break;
    case BS_ITEM_TYPE_DIELECTRIC:  color = dielectricColor;          break;
    case BS_ITEM_TYPE_SOLDERMASK:  color = GetSelectedColor( aRow ); break;
    case BS_ITEM_TYPE_SILKSCREEN:  color = GetSelectedColor( aRow ); break;
    case BS_ITEM_TYPE_SOLDERPASTE: color = pasteColor;               break;

    default:
    case BS_ITEM_TYPE_UNDEFINED:
        wxFAIL_MSG( "PANEL_SETUP_BOARD_STACKUP::getColorIconItem: unrecognized item type" );
        break;
    }

    wxASSERT_MSG( color.IsOk(), "Invalid color in PCB stackup" );

    return color;
}


void PANEL_SETUP_BOARD_STACKUP::updateIconColor( int aRow )
{
    if( aRow >= 0 )
    {
        wxStaticBitmap* st_bitmap = m_rowUiItemsList[aRow].m_Icon;

        // explicit depth important under MSW
        wxBitmap bmp( m_colorIconsSize.x, m_colorIconsSize.y / 2, 28 );
        drawBitmap( bmp, getColorIconItem( aRow ) );
        st_bitmap->SetBitmap( bmp );
        return;
    }

    for( unsigned row = 0; row < m_rowUiItemsList.size(); row++ )
    {
        // explicit depth important under MSW
        wxBitmap bmp( m_colorIconsSize.x, m_colorIconsSize.y / 2, 28 );
        drawBitmap( bmp, getColorIconItem( row ) );
        m_rowUiItemsList[row].m_Icon->SetBitmap( bmp );
    }
}


wxBitmapComboBox* PANEL_SETUP_BOARD_STACKUP::createColorBox( BOARD_STACKUP_ITEM* aStackupItem,
                                                             int aRow )
{
    wxBitmapComboBox* combo = new wxBitmapComboBox( m_scGridWin, ID_ITEM_COLOR + aRow,
                                                    wxEmptyString, wxDefaultPosition,
                                                    wxDefaultSize, 0, nullptr, wxCB_READONLY );

    // Fills the combo box with choice list + bitmaps
    const FAB_LAYER_COLOR*  color_list = GetColorStandardList();
    BOARD_STACKUP_ITEM_TYPE itemType = aStackupItem ? aStackupItem->GetType()
                                                    : BS_ITEM_TYPE_SILKSCREEN;

    for( int ii = 0; ii < GetColorStandardListCount(); ii++ )
    {
        wxColor  curr_color;
        wxString label;

        // Defined colors have a name, the user color uses HTML notation ( i.e. #FF000080)
        if( ii == GetColorUserDefinedListIdx()
                && aStackupItem && aStackupItem->GetColor().StartsWith( "#" ) )
        {
            curr_color = wxColour( aStackupItem->GetColor() );

            // NB: wxWidgets 3.0's color.GetAsString( wxC2S_HTML_SYNTAX ) pukes on alpha
            label = getColourAsHexString( curr_color );
        }
        else
        {
            curr_color = color_list[ii].GetColor( itemType );
            label = _( color_list[ii].GetName() );
        }

        wxBitmap layerbmp( m_colorSwatchesSize.x, m_colorSwatchesSize.y );
        LAYER_SELECTOR::DrawColorSwatch( layerbmp, COLOR4D( 0, 0, 0, 0 ), COLOR4D( curr_color ) );

        combo->Append( label, layerbmp );
    }

    // Ensure the size of the widget is enough to show the text and the icon
    // We have to have a selected item when doing this, because otherwise GTK
    // will just choose a random size that might not fit the actual data
    // (such as in cases where the font size is very large). So we select
    // the longest item (which should be the last item), and size it that way.
    int sel = combo->GetSelection();
    combo->SetSelection( combo->GetCount() - 1 );

    combo->SetMinSize( wxSize( -1, -1 ) );
    wxSize bestSize = combo->GetBestSize();

    bestSize.x = bestSize.x + m_colorSwatchesSize.x;
    combo->SetMinSize( bestSize );
    combo->SetSelection( sel );

    // add the wxBitmapComboBox to wxControl list, to be able to disconnect the event
    // on exit
    m_controlItemsList.push_back( combo );

    combo->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED,
                    wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onColorSelected ),
                    nullptr, this );

    combo->Bind( wxEVT_COMBOBOX_DROPDOWN,
            [combo]( wxCommandEvent& aEvent )
            {
                combo->SetString( combo->GetCount() - 1, _( "Custom..." ) );
            } );

    return combo;
}


void drawBitmap( wxBitmap& aBitmap, wxColor aColor )
{
    wxNativePixelData data( aBitmap );
    wxNativePixelData::Iterator p( data );

    for( int yy = 0; yy < data.GetHeight(); yy++ )
    {
        wxNativePixelData::Iterator rowStart = p;

        for( int xx = 0; xx < data.GetWidth(); xx++ )
        {
            p.Red() = aColor.Red();
            p.Green() = aColor.Green();
            p.Blue() = aColor.Blue();
            ++p;
        }

        p = rowStart;
        p.OffsetY( data, 1 );
    }
}


