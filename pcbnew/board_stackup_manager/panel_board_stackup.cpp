/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <macros.h>             // arrayDim definition
#include <pcb_edit_frame.h>
#include <board.h>
#include <board_design_settings.h>
#include <dialogs/dialog_color_picker.h>
#include <widgets/paged_dialog.h>
#include <widgets/layer_presentation.h>
#include <widgets/wx_panel.h>
#include <wx/bmpcbox.h>
#include <wx/log.h>
#include <wx/rawbmp.h>
#include <wx/clipbrd.h>
#include <wx/wupdlock.h>
#include <wx/richmsgdlg.h>
#include <math/util.h>      // for KiROUND

#include "panel_board_stackup.h"
#include "panel_board_finish.h"
#include <panel_setup_layers.h>
#include "board_stackup_reporter.h"
#include <bitmaps.h>
#include "dialog_dielectric_list_manager.h"
#include <wx/textdlg.h>

#include <locale_io.h>
#include <eda_list_dialog.h>
#include <richio.h>
#include <string_utils.h>               // for UIDouble2Str()


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


PANEL_SETUP_BOARD_STACKUP::PANEL_SETUP_BOARD_STACKUP( wxWindow* aParentWindow,
                                                      PCB_EDIT_FRAME* aFrame,
                                                      PANEL_SETUP_LAYERS* aPanelLayers,
                                                      PANEL_SETUP_BOARD_FINISH* aPanelFinish ):
        PANEL_SETUP_BOARD_STACKUP_BASE( aParentWindow ),
        m_delectricMatList( DIELECTRIC_SUBSTRATE_LIST::DL_MATERIAL_DIELECTRIC ),
        m_solderMaskMatList( DIELECTRIC_SUBSTRATE_LIST::DL_MATERIAL_SOLDERMASK ),
        m_silkscreenMatList( DIELECTRIC_SUBSTRATE_LIST::DL_MATERIAL_SILKSCREEN ),
        m_board( aFrame->GetBoard() ),
        m_frame( aFrame ),
        m_lastUnits( aFrame->GetUserUnits() )
{
    m_panelLayers = aPanelLayers;
    m_panelFinish = aPanelFinish;
    m_brdSettings = &m_board->GetDesignSettings();

    m_panel1->SetBorders( false, false, true, true );

    m_panelLayers->SetPhysicalStackupPanel( this );

    m_enabledLayers = m_board->GetEnabledLayers() & BOARD_STACKUP::StackupAllowedBrdLayers();

    // Use a good size for color swatches (icons) in this dialog
    m_colorSwatchesSize = wxSize( 14, 14 );
    m_colorIconsSize = wxSize( 24, 14 );

    // Calculates a good size for wxTextCtrl to enter Epsilon R and Loss tan
    // ("0.0000000" + margins)
    m_numericFieldsSize = GetTextExtent( wxT( "X.XXXXXXX" ) );
    m_numericFieldsSize.y = -1;     // Use default for the vertical size

    // Calculates a minimal size for wxTextCtrl to enter a dim with units
    // ("000.0000000 mils" + margins)
    m_numericTextCtrlSize = GetTextExtent( wxT( "XXX.XXXXXXX mils" ) );
    m_numericTextCtrlSize.y = -1;     // Use default for the vertical size

    // The grid column containing the lock checkbox is kept to a minimal
    // size. So we use a wxStaticBitmap: set the bitmap itself
    m_bitmapLockThickness->SetBitmap( KiBitmapBundle( BITMAPS::locked ) );

    // Gives a minimal size of wxTextCtrl showing dimensions+units
    m_tcCTValue->SetMinSize( m_numericTextCtrlSize );

    // Prepare dielectric layer type: layer type keyword is "core" or "prepreg"
    m_core_prepreg_choice.Add( _( "Core" ) );
    m_core_prepreg_choice.Add( _( "PrePreg" ) );

    buildLayerStackPanel( true );
    synchronizeWithBoard( true );
    computeBoardThickness();

    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &PANEL_SETUP_BOARD_STACKUP::onUnitsChanged, this );
}


PANEL_SETUP_BOARD_STACKUP::~PANEL_SETUP_BOARD_STACKUP()
{
    disconnectEvents();
}


void PANEL_SETUP_BOARD_STACKUP::onUnitsChanged( wxCommandEvent& event )
{
    EDA_UNITS    newUnits = m_frame->GetUserUnits();
    EDA_IU_SCALE scale = m_frame->GetIuScale();

    auto convert =
            [&]( wxTextCtrl* aTextCtrl )
            {
                wxString str = aTextCtrl->GetValue();
                long long int temp = EDA_UNIT_UTILS::UI::ValueFromString( scale, m_lastUnits, str );
                str = EDA_UNIT_UTILS::UI::StringFromValue( scale, newUnits, temp, true );

                // Don't use SetValue(); we don't want a bunch of event propagation as the actual
                // value hasn't changed, only its presentation.
                aTextCtrl->ChangeValue( str );
            };

    for( BOARD_STACKUP_ROW_UI_ITEM& ui_item : m_rowUiItemsList )
    {
        BOARD_STACKUP_ITEM* item = ui_item.m_Item;

        if( item->IsThicknessEditable() && item->IsEnabled() )
            convert( static_cast<wxTextCtrl*>( ui_item.m_ThicknessCtrl ) );
    }

    convert( m_tcCTValue );

    m_lastUnits = newUnits;

    event.Skip();
}


void PANEL_SETUP_BOARD_STACKUP::onCopperLayersSelCount( wxCommandEvent& event )
{
    int oldBoardWidth = static_cast<int>( m_frame->ValueFromString( m_tcCTValue->GetValue() ) );
    updateCopperLayerCount();
    showOnlyActiveLayers();
    updateIconColor();
    setDefaultLayerWidths( oldBoardWidth );
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

        int item_thickness = m_frame->ValueFromString( textCtrl->GetValue() );
        min_thickness += item_thickness;
    }

    wxString title;

    if( min_thickness == 0 )
    {
        title.Printf( _( "Enter board thickness in %s:" ),
                      EDA_UNIT_UTILS::GetText( m_frame->GetUserUnits() ).Trim( false ) );
    }
    else
    {
        title.Printf( _( "Enter expected board thickness (min value %s):" ),
                      m_frame->StringFromValue( min_thickness, true ) );
    }

    wxTextEntryDialog dlg( this, title, _( "Adjust Unlocked Dielectric Layers" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    int iu_thickness = m_frame->ValueFromString( dlg.GetValue() );

    if( iu_thickness < min_thickness )
    {
        wxMessageBox( wxString::Format( _("Value too small (min value %s)." ),
                                        m_frame->StringFromValue( min_thickness, true ) ) );
        return;
    }

    // Now adjust not locked dielectric thickness layers:

    if( items_candidate.size() )
        setDefaultLayerWidths( iu_thickness );
    else
        wxMessageBox( _( "All dielectric  thickness layers are locked" ) );

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

    EDA_LIST_DIALOG dlg( PAGED_DIALOG::GetDialog( this ), _( "Add Dielectric Layer" ),
                         headers, d_list, wxEmptyString,
                         false /* do not sort the list: it is **expected** in stackup order */ );
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

    EDA_LIST_DIALOG dlg( PAGED_DIALOG::GetDialog( this ), _( "Remove Dielectric Layer" ),
                         headers, d_list, wxEmptyString,
                         false /* do not sort the list: it is **expected** in stackup order */ );
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

    m_panelFinish->TransferDataFromWindow( m_stackup );

    // Build a ASCII representation of stackup and copy it in the clipboard
    wxString report = BuildStackupReport( m_stackup, m_frame->GetUserUnits() );

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
    int                              idx = choice ? choice->GetSelection() : 0;

    if( IsCustomColorIdx( item->GetType(), idx ) )
        return m_rowUiItemsList[aRow].m_UserColor.ToColour();
    else
        return GetStandardColor( item->GetType(), idx ).ToColour();
}


void PANEL_SETUP_BOARD_STACKUP::setDefaultLayerWidths( int targetThickness )
{
    // This function tries to set the PCB thickness to the parameter and uses a fixed prepreg thickness
    // of 0.1 mm. The core thickness is calculated accordingly as long as it also stays above 0.1mm.
    // If the core thickness would be smaller than the default pregreg thickness given here,
    // both are reduced towards zero to arrive at the correct PCB width
    const int prePregDefaultThickness = pcbIUScale.mmToIU( 0.1 );

    int copperLayerCount = GetCopperLayerCount();

    // This code is for a symmetrical PCB stackup with even copper layer count
    // If asymmetric stackups were to be implemented, the following layer count calculations
    // for dielectric/core layers might need adjustments.
    wxASSERT( copperLayerCount % 2 == 0 );

    int  dielectricLayerCount = copperLayerCount - 1;
    int  coreLayerCount = copperLayerCount / 2 - 1;

    wxASSERT( dielectricLayerCount > 0 );

    bool currentLayerIsCore = false;

    // start with prepreg layer on the outside, except when creating two-layer-board
    if( copperLayerCount == 2 )
    {
        coreLayerCount = 1;
        currentLayerIsCore = true;
    }

    wxASSERT( coreLayerCount > 0 );

    int prePregLayerCount = dielectricLayerCount - coreLayerCount;

    int totalWidthOfFixedItems = 0;

    for( BOARD_STACKUP_ROW_UI_ITEM& ui_item : m_rowUiItemsList )
    {
        BOARD_STACKUP_ITEM* item = ui_item.m_Item;

        if( !item->IsThicknessEditable() || !ui_item.m_isEnabled )
            continue;

        wxCheckBox* cbLock = dynamic_cast<wxCheckBox*>( ui_item.m_ThicknessLockCtrl );
        wxChoice*   layerType = dynamic_cast<wxChoice*>( ui_item.m_LayerTypeCtrl );

        if( ( item->GetType() == BS_ITEM_TYPE_DIELECTRIC && !layerType )
            || item->GetType() == BS_ITEM_TYPE_SOLDERMASK
            || item->GetType() == BS_ITEM_TYPE_COPPER
            || ( cbLock && cbLock->GetValue() ) )
        {
            // secondary dielectric layers, mask and copper layers and locked layers will be
            // counted as fixed width
            wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( ui_item.m_ThicknessCtrl );
            int         item_thickness = m_frame->ValueFromString( textCtrl->GetValue() );

            totalWidthOfFixedItems += item_thickness;
        }
    }

    // Width that hasn't been allocated by fixed items
    int remainingWidth = targetThickness
                            - totalWidthOfFixedItems
                            - ( prePregDefaultThickness * prePregLayerCount );

    int prePregThickness = prePregDefaultThickness;
    int coreThickness = remainingWidth / coreLayerCount;

    if( coreThickness < prePregThickness )
    {
        // There's not enough room for prepreg and core layers of at least 0.1 mm, so adjust both down
        remainingWidth = targetThickness - totalWidthOfFixedItems;
        prePregThickness = coreThickness = std::max( 0, remainingWidth / dielectricLayerCount );
    }

    for( BOARD_STACKUP_ROW_UI_ITEM& ui_item : m_rowUiItemsList )
    {
        BOARD_STACKUP_ITEM* item = ui_item.m_Item;

        if( item->GetType() != BS_ITEM_TYPE_DIELECTRIC || !ui_item.m_isEnabled )
            continue;

        wxChoice* layerType = dynamic_cast<wxChoice*>( ui_item.m_LayerTypeCtrl );

        if( !layerType )
        {
            // ignore secondary dielectric layers
            continue;
        }

        wxCheckBox* cbLock = dynamic_cast<wxCheckBox*>( ui_item.m_ThicknessLockCtrl );

        if( cbLock && cbLock->GetValue() )
        {
            currentLayerIsCore = !currentLayerIsCore;

            // Don't override width of locked layer
            continue;
        }

        int layerThickness = currentLayerIsCore ? coreThickness : prePregThickness;

        wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( ui_item.m_ThicknessCtrl );
        layerType->SetSelection( currentLayerIsCore ? 0 : 1 );
        textCtrl->SetValue( m_frame->StringFromValue( layerThickness ) );

        currentLayerIsCore = !currentLayerIsCore;
    }
}


int PANEL_SETUP_BOARD_STACKUP::computeBoardThickness()
{
    int thickness = 0;

    for( BOARD_STACKUP_ROW_UI_ITEM& ui_item : m_rowUiItemsList )
    {
        BOARD_STACKUP_ITEM* item = ui_item.m_Item;

        if( !item->IsThicknessEditable() || !ui_item.m_isEnabled )
            continue;

        wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( ui_item.m_ThicknessCtrl );
        int         item_thickness = m_frame->ValueFromString( textCtrl->GetValue() );

        thickness += item_thickness;
    }

    wxString thicknessStr = m_frame->StringFromValue( thickness, true );

    // The text in the event will translate to the value for the text control
    // and is only updated if it changed
    m_tcCTValue->ChangeValue( thicknessStr );

    return thickness;
}


int PANEL_SETUP_BOARD_STACKUP::GetCopperLayerCount() const
{
    return ( m_choiceCopperLayers->GetSelection() + 1 ) * 2;
}


void PANEL_SETUP_BOARD_STACKUP::updateCopperLayerCount()
{
    const int copperCount = GetCopperLayerCount();

    wxASSERT( copperCount >= 2 );

    m_enabledLayers.ClearCopperLayers();
    m_enabledLayers |= LSET::AllCuMask( copperCount );
}


void PANEL_SETUP_BOARD_STACKUP::synchronizeWithBoard( bool aFullSync )
{
    const BOARD_STACKUP&   brd_stackup = m_brdSettings->GetStackupDescriptor();

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
                    matName->ChangeValue( item->GetMaterial( sub_item ) );
                else
                    matName->ChangeValue( wxGetTranslation( NotSpecifiedPrm() ) );
            }
        }

        if( item->IsThicknessEditable() )
        {
            wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ui_row_item.m_ThicknessCtrl );

            if( textCtrl )
                textCtrl->ChangeValue( m_frame->StringFromValue( item->GetThickness( sub_item ), true ) );

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
            int  selected = 0;  // The "not specified" item

            if( item->GetColor( sub_item ).StartsWith( wxT( "#" ) ) )  // User defined color
            {
                COLOR4D custom_color( item->GetColor( sub_item ) );

                ui_row_item.m_UserColor = custom_color;

                selected = GetColorUserDefinedListIdx( item->GetType() );

                if( bm_combo )      // Update user color shown in the wxBitmapComboBox
                {
                    bm_combo->SetString( selected, item->GetColor( sub_item ) );
                    wxBitmap layerbmp( m_colorSwatchesSize.x, m_colorSwatchesSize.y );
                    LAYER_PRESENTATION::DrawColorSwatch( layerbmp, COLOR4D(), custom_color );
                    bm_combo->SetItemBitmap( selected, layerbmp );
                }
            }
            else
            {
                if( bm_combo )
                {
                    // Note: don't use bm_combo->FindString() because the combo strings are
                    // translated.
                    for( size_t ii = 0; ii < GetStandardColors( item->GetType() ).size(); ii++ )
                    {
                        if( GetStandardColorName( item->GetType(), ii ) == item->GetColor( sub_item ) )
                        {
                            selected = ii;
                            break;
                        }
                    }
                }
            }

            if( bm_combo )
                bm_combo->SetSelection( selected );
        }

        if( item->HasEpsilonRValue() )
        {
            wxString txt = UIDouble2Str( item->GetEpsilonR( sub_item ) );
            wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ui_row_item.m_EpsilonCtrl );

            if( textCtrl )
                textCtrl->ChangeValue( txt );
        }

        if( item->HasLossTangentValue() )
        {
            wxString txt = UIDouble2Str( item->GetLossTangent( sub_item ) );
            wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ui_row_item.m_LossTgCtrl );

            if( textCtrl )
                textCtrl->ChangeValue( txt );
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
    int  pos = 0;

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

        if( show_item )
        {
            // pre-increment (ie: before calling lazyBuildRowUI) to account for header row
            pos += 9;
        }

        if( show_item && !ui_row_item.m_Icon )
            lazyBuildRowUI( ui_row_item, pos );

        if( ui_row_item.m_Icon )
        {
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
}


wxControl* PANEL_SETUP_BOARD_STACKUP::addSpacer( int aPos )
{
    wxStaticText* emptyText = new wxStaticText( m_scGridWin, wxID_ANY, wxEmptyString );
    m_fgGridSizer->Insert( aPos, emptyText, 0, wxALIGN_CENTER_VERTICAL );
    return emptyText;
}


void PANEL_SETUP_BOARD_STACKUP::lazyBuildRowUI( BOARD_STACKUP_ROW_UI_ITEM& ui_row_item,
                                                int aPos )
{
    BOARD_STACKUP_ITEM* item = ui_row_item.m_Item;
    int                 sublayerIdx = ui_row_item.m_SubItem;
    int                 row = ui_row_item.m_Row;

    // Add color swatch icon. The color will be updated later,
    // when all widgets are initialized
    wxStaticBitmap* bitmap = new wxStaticBitmap( m_scGridWin, wxID_ANY, wxNullBitmap );
    m_fgGridSizer->Insert( aPos++, bitmap, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 4 );
    ui_row_item.m_Icon = bitmap;

    if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
    {
        wxString lname = item->FormatDielectricLayerName();

        if( item->GetSublayersCount() > 1 )
        {
            lname <<  wxT( "  (" ) << sublayerIdx +1 << wxT( "/" )
                                   << item->GetSublayersCount() << wxT( ")" );
        }

        wxStaticText* st_text = new wxStaticText( m_scGridWin, wxID_ANY, lname );
        m_fgGridSizer->Insert( aPos++, st_text, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
        ui_row_item.m_LayerName = st_text;

        // For a dielectric layer, the layer type choice is not for each sublayer,
        // only for the first (sublayerIdx = 0), and is common to all sublayers
        if( sublayerIdx == 0 )
        {
            wxChoice* choice = new wxChoice( m_scGridWin, wxID_ANY, wxDefaultPosition,
                                             wxDefaultSize, m_core_prepreg_choice );
            choice->SetSelection( item->GetTypeName() == KEY_CORE ? 0 : 1 );
            m_fgGridSizer->Insert( aPos++, choice, 1, wxEXPAND|wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );

            ui_row_item.m_LayerTypeCtrl = choice;
        }
        else
        {
            ui_row_item.m_LayerTypeCtrl = addSpacer( aPos++ );
        }
    }
    else
    {
        item->SetLayerName( m_board->GetLayerName( item->GetBrdLayerId() ) );
        wxStaticText* st_text =  new wxStaticText( m_scGridWin, wxID_ANY, item->GetLayerName() );
        m_fgGridSizer->Insert( aPos++, st_text, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 1 );
        st_text->Show( true );
        ui_row_item.m_LayerName = st_text;

        wxString lname;

        if( item->GetTypeName() == KEY_COPPER )
            lname = _( "Copper" );
        else
            lname = wxGetTranslation( item->GetTypeName() );

        st_text = new wxStaticText( m_scGridWin, wxID_ANY, lname );
        m_fgGridSizer->Insert( aPos++, st_text, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
        ui_row_item.m_LayerTypeCtrl = st_text;
    }

    if( item->IsMaterialEditable() )
    {
        wxString matName = item->GetMaterial( sublayerIdx );

        wxBoxSizer* bSizerMat = new wxBoxSizer( wxHORIZONTAL );
       	m_fgGridSizer->Insert( aPos++, bSizerMat, 1, wxRIGHT|wxEXPAND, 4 );
        wxTextCtrl* textCtrl = new wxTextCtrl( m_scGridWin, wxID_ANY );

        if( IsPrmSpecified( matName ) )
            textCtrl->ChangeValue( matName );
        else
            textCtrl->ChangeValue( wxGetTranslation( NotSpecifiedPrm() ) );

        textCtrl->SetMinSize( m_numericTextCtrlSize );
       	bSizerMat->Add( textCtrl, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

       	wxButton* m_buttonMat = new wxButton( m_scGridWin, ID_ITEM_MATERIAL+row, _( "..." ),
                                              wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
       	bSizerMat->Add( m_buttonMat, 0, wxALIGN_CENTER_VERTICAL, 2 );

        m_buttonMat->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                              wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onMaterialChange ),
                              nullptr, this );
        m_controlItemsList.push_back( m_buttonMat );

        ui_row_item.m_MaterialCtrl = textCtrl;
        ui_row_item.m_MaterialButt = m_buttonMat;

    }
    else
    {
        ui_row_item.m_MaterialCtrl = addSpacer( aPos++ );
    }

    if( item->IsThicknessEditable() )
    {
        wxTextCtrl* textCtrl = new wxTextCtrl( m_scGridWin, ID_ITEM_THICKNESS+row );
        textCtrl->SetMinSize( m_numericTextCtrlSize );
        textCtrl->ChangeValue( m_frame->StringFromValue( item->GetThickness( sublayerIdx ), true ) );
        m_fgGridSizer->Insert( aPos++, textCtrl, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
        m_controlItemsList.push_back( textCtrl );
        textCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED,
                           wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onThicknessChange ),
                           nullptr, this );
        ui_row_item.m_ThicknessCtrl = textCtrl;

        if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        {
            wxCheckBox* cb_box = new wxCheckBox( m_scGridWin, ID_ITEM_THICKNESS_LOCKED+row,
                                                 wxEmptyString );
            cb_box->SetValue( item->IsThicknessLocked( sublayerIdx ) );

            m_fgGridSizer->Insert( aPos++, cb_box, 0,
                                   wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL, 2 );

            ui_row_item.m_ThicknessLockCtrl = cb_box;
        }
        else
        {
            ui_row_item.m_ThicknessLockCtrl = addSpacer( aPos++);
        }
    }
    else
    {
        ui_row_item.m_ThicknessCtrl = addSpacer( aPos++ );
        ui_row_item.m_ThicknessLockCtrl = addSpacer( aPos++ );
    }

    if( item->IsColorEditable() )
    {
        if( item->GetColor( sublayerIdx ).StartsWith( wxT( "#" ) ) )  // User defined color
        {
            ui_row_item.m_UserColor = COLOR4D( item->GetColor( sublayerIdx ) ).ToColour();
        }
        else
            ui_row_item.m_UserColor = GetDefaultUserColor( item->GetType() );

        wxBitmapComboBox* bm_combo = createColorBox( item, row );
        int               selected = 0;     // The "not specified" item

        m_fgGridSizer->Insert( aPos++, bm_combo, 1, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 2 );

        if( item->GetColor( sublayerIdx ).StartsWith( wxT( "#" ) ) )
        {
            selected = GetColorUserDefinedListIdx( item->GetType() );
            bm_combo->SetString( selected, item->GetColor( sublayerIdx ) );
        }
        else
        {
            // Note: don't use bm_combo->FindString() because the combo strings are translated.
            for( size_t ii = 0; ii < GetStandardColors( item->GetType() ).size(); ii++ )
            {
                if( GetStandardColorName( item->GetType(), ii ) == item->GetColor( sublayerIdx ) )
                {
                    selected = ii;
                    break;
                }
            }
        }

        bm_combo->SetSelection( selected );
        ui_row_item.m_ColorCtrl = bm_combo;
    }
    else
    {
        ui_row_item.m_ColorCtrl = addSpacer( aPos++ );
    }

    if( item->HasEpsilonRValue() )
    {
        wxString txt = UIDouble2Str( item->GetEpsilonR( sublayerIdx ) );
        wxTextCtrl* textCtrl = new wxTextCtrl( m_scGridWin, wxID_ANY, wxEmptyString,
                                               wxDefaultPosition, m_numericFieldsSize );
        textCtrl->ChangeValue( txt );
        m_fgGridSizer->Insert( aPos++, textCtrl, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
        ui_row_item.m_EpsilonCtrl = textCtrl;
    }
    else
    {
        ui_row_item.m_EpsilonCtrl = addSpacer( aPos++ );
    }

    if( item->HasLossTangentValue() )
    {
        wxString txt = UIDouble2Str( item->GetLossTangent( sublayerIdx ) );;
        wxTextCtrl* textCtrl = new wxTextCtrl( m_scGridWin, wxID_ANY, wxEmptyString,
                                               wxDefaultPosition, m_numericFieldsSize );
        textCtrl->ChangeValue( txt );
        m_fgGridSizer->Insert( aPos++, textCtrl, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
        ui_row_item.m_LossTgCtrl = textCtrl;
    }
    else
    {
        ui_row_item.m_LossTgCtrl = addSpacer( aPos++ );
    }
}


void PANEL_SETUP_BOARD_STACKUP::rebuildLayerStackPanel( bool aRelinkItems )
{
    wxWindowUpdateLocker locker( m_scGridWin );
    m_scGridWin->Hide();

    // Rebuild the stackup for the dialog, after dielectric parameters list is modified
    // (added/removed):

    // First, delete all ui objects, because wxID values will be no longer valid for many widgets
    disconnectEvents();
    m_controlItemsList.clear();

    // Delete widgets (handled by the wxPanel parent)
    for( BOARD_STACKUP_ROW_UI_ITEM& ui_item: m_rowUiItemsList )
    {
        // This remove and delete the current ui_item.m_MaterialCtrl sizer
        if( ui_item.m_MaterialCtrl )
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
    buildLayerStackPanel( false, aRelinkItems );

    // Now enable/disable stackup items, according to the m_enabledLayers config
    showOnlyActiveLayers();

    updateIconColor();

    m_scGridWin->Layout();
    m_scGridWin->Show();
}


void PANEL_SETUP_BOARD_STACKUP::buildLayerStackPanel( bool aCreateInitialStackup,
                                                      bool aRelinkStackup )
{
    // Build a full stackup for the dialog, with a active copper layer count
    // = current board layer count to calculate a reasonable default stackup:
    if( aCreateInitialStackup || aRelinkStackup )
    {
        if( aCreateInitialStackup )
        {
            // Creates a BOARD_STACKUP with 32 copper layers.
            // extra layers will be hidden later.
            // but if the number of layer is changed in the dialog, the corresponding
            // widgets will be available with their previous values.
            m_stackup.BuildDefaultStackupList( nullptr, m_brdSettings->GetCopperLayerCount() );
        }

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
            m_rowUiItemsList.emplace_back( item, sub_idx, row );
            row++;
        }
    }
}


// Transfer current UI settings to m_stackup but not to the board
bool PANEL_SETUP_BOARD_STACKUP::transferDataFromUIToStackup()
{
    wxString error_msg;
    bool success = true;
    double value;

    for( BOARD_STACKUP_ROW_UI_ITEM& ui_item : m_rowUiItemsList )
    {
        // Skip stackup items useless for the current board
        if( !ui_item.m_isEnabled )
        {
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
            wxString    txt = textCtrl->GetValue();

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
            wxString    txt = textCtrl->GetValue();

            if( txt.ToDouble( &value ) && value >= 0.0 )
                item->SetLossTangent( value, sub_item );
            else if( txt.ToCDouble( &value ) && value >= 0.0 )
                item->SetLossTangent( value, sub_item );
            else
            {
                success = false;

                if( !error_msg.IsEmpty() )
                    error_msg << wxT( "\n" );

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
            int         new_thickness = m_frame->ValueFromString( textCtrl->GetValue() );

            item->SetThickness( new_thickness, sub_item );

            if( new_thickness < 0 )
            {
                success = false;

                if( !error_msg.IsEmpty() )
                    error_msg << wxT( "\n" );

                error_msg << _( "A layer thickness is < 0. Fix it" );
            }

            if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
            {
                // Dielectric thickness layer can have a locked thickness:
                wxCheckBox* cb_box = static_cast<wxCheckBox*>( ui_item.m_ThicknessLockCtrl );
                item->SetThicknessLocked( cb_box && cb_box->GetValue(), sub_item );
            }
        }

        if( item->IsColorEditable() )
        {
            wxBitmapComboBox* choice = dynamic_cast<wxBitmapComboBox*>( ui_item.m_ColorCtrl );

            if( choice )
            {
                int idx = choice->GetSelection();

                if( IsCustomColorIdx( item->GetType(), idx ) )
                    item->SetColor( ui_item.m_UserColor.ToHexString(), sub_item );
                else
                    item->SetColor( GetStandardColorName( item->GetType(), idx ), sub_item );
            }
        }
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
    brd_stackup.FormatBoardStackup( &old_stackup, m_board );

    // copy enabled items to the new board stackup
    brd_stackup.RemoveAll();

    for( BOARD_STACKUP_ITEM* item : m_stackup.GetList() )
    {
        if( item->IsEnabled() )
            brd_stackup.Add( new BOARD_STACKUP_ITEM( *item ) );
    }

    STRING_FORMATTER new_stackup;
    brd_stackup.FormatBoardStackup( &new_stackup, m_board );

    bool modified = old_stackup.GetString() != new_stackup.GetString();
    int thickness = brd_stackup.BuildBoardThicknessFromStackup();

    if( m_brdSettings->GetBoardThickness() != thickness )
    {
        m_brdSettings->SetBoardThickness( thickness );
        modified = true;
    }

    if( brd_stackup.m_HasDielectricConstrains != m_impedanceControlled->GetValue() )
    {
        brd_stackup.m_HasDielectricConstrains = m_impedanceControlled->GetValue();
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
    m_board = aBoard;

    BOARD_DESIGN_SETTINGS* savedSettings = m_brdSettings;
    m_brdSettings = &aBoard->GetDesignSettings();

    m_enabledLayers = m_board->GetEnabledLayers() & BOARD_STACKUP::StackupAllowedBrdLayers();

    rebuildLayerStackPanel( true );
    synchronizeWithBoard( true );
    computeBoardThickness();

    m_brdSettings = savedSettings;
    m_board = savedBrd;
}


void PANEL_SETUP_BOARD_STACKUP::OnLayersOptionsChanged( const LSET& aNewLayerSet )
{
    // Can be called spuriously from events before the layers page is even created
    if( !m_panelLayers->IsInitialized() )
        return;

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
    int                 idx = event.GetSelection();
    int                 item_id = event.GetId();
    int                 row = item_id - ID_ITEM_COLOR;
    BOARD_STACKUP_ITEM* item = m_rowUiItemsList[row].m_Item;

    if( IsCustomColorIdx( item->GetType(), idx ) )   // user color is the last option in list
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
            COLOR4D           color = dlg.GetColor();

            m_rowUiItemsList[row].m_UserColor = color;

            combo->SetString( idx, color.ToHexString() );

            wxBitmap layerbmp( m_colorSwatchesSize.x, m_colorSwatchesSize.y );
            LAYER_PRESENTATION::DrawColorSwatch( layerbmp, COLOR4D( 0, 0, 0, 0 ), color );
            combo->SetItemBitmap( combo->GetCount() - 1, layerbmp );

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

    wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_MaterialCtrl );
    textCtrl->ChangeValue( item->GetMaterial( sub_item ) );

    if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC
            && !item->GetColor( sub_item ).StartsWith( "#" ) /* User defined color */ )
    {
        if( substrate.m_Name.IsSameAs( "PTFE" )
              || substrate.m_Name.IsSameAs( "Teflon" ) )
        {
            item->SetColor( "PTFE natural", sub_item );
        }
        else if( substrate.m_Name.IsSameAs( "Polyimide" )
              || substrate.m_Name.IsSameAs( "Kapton" ) )
        {
            item->SetColor( "Polyimide", sub_item );
        }
        else if( substrate.m_Name.IsSameAs( "Al" ) )
        {
            item->SetColor( "Aluminum", sub_item );
        }
        else
        {
            item->SetColor( "FR4 natural", sub_item );
        }
    }

    wxBitmapComboBox* picker = static_cast<wxBitmapComboBox*>( m_rowUiItemsList[row].m_ColorCtrl );

    for( size_t ii = 0; ii < GetStandardColors( item->GetType() ).size(); ii++ )
    {
        if( GetStandardColorName( item->GetType(), ii ) == item->GetColor( sub_item ) )
        {
            picker->SetSelection( ii );
            break;
        }
    }

    // some layers have a material choice but not EpsilonR ctrl
    if( item->HasEpsilonRValue() )
    {
        textCtrl = dynamic_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_EpsilonCtrl );

        if( textCtrl )
            textCtrl->ChangeValue( item->FormatEpsilonR( sub_item ) );
    }

    // some layers have a material choice but not loss tg ctrl
    if( item->HasLossTangentValue() )
    {
        textCtrl = dynamic_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_LossTgCtrl );

        if( textCtrl )
            textCtrl->ChangeValue( item->FormatLossTangent( sub_item ) );
    }
}


void PANEL_SETUP_BOARD_STACKUP::onThicknessChange( wxCommandEvent& event )
{
    int row  = event.GetId() - ID_ITEM_THICKNESS;
    wxString value = event.GetString();

    BOARD_STACKUP_ITEM* item = GetStackupItem( row );
    int idx = GetSublayerId( row );

    item->SetThickness( m_frame->ValueFromString( value ), idx );

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
        wxFAIL_MSG( wxT( "PANEL_SETUP_BOARD_STACKUP::getColorIconItem: unrecognized item type" ) );
        break;
    }

    wxASSERT_MSG( color.IsOk(), wxT( "Invalid color in PCB stackup" ) );

    return color;
}


void PANEL_SETUP_BOARD_STACKUP::updateIconColor( int aRow )
{
    // explicit depth important under MSW. We use R,V,B 24 bits/pixel bitmap
    const int bitmap_depth = 24;

    if( aRow >= 0 )
    {
        wxStaticBitmap* st_bitmap = m_rowUiItemsList[aRow].m_Icon;

        wxBitmap bmp( m_colorIconsSize.x, m_colorIconsSize.y / 2, bitmap_depth );
        drawBitmap( bmp, getColorIconItem( aRow ) );
        st_bitmap->SetBitmap( bmp );
        return;
    }

    for( unsigned row = 0; row < m_rowUiItemsList.size(); row++ )
    {
        if( m_rowUiItemsList[row].m_Icon )
        {
            wxBitmap bmp( m_colorIconsSize.x, m_colorIconsSize.y / 2, bitmap_depth );
            drawBitmap( bmp, getColorIconItem( row ) );
            m_rowUiItemsList[row].m_Icon->SetBitmap( bmp );
        }
    }
}


wxBitmapComboBox* PANEL_SETUP_BOARD_STACKUP::createColorBox( BOARD_STACKUP_ITEM* aStackupItem,
                                                             int aRow )
{
    wxBitmapComboBox* combo = new wxBitmapComboBox( m_scGridWin, ID_ITEM_COLOR + aRow,
                                                    wxEmptyString, wxDefaultPosition,
                                                    wxDefaultSize, 0, nullptr, wxCB_READONLY );

    // Fills the combo box with choice list + bitmaps
    BOARD_STACKUP_ITEM_TYPE itemType = aStackupItem ? aStackupItem->GetType()
                                                    : BS_ITEM_TYPE_SILKSCREEN;

    for( size_t ii = 0; ii < GetStandardColors( itemType ).size(); ii++ )
    {
        wxString label;
        COLOR4D  curr_color;

        // Defined colors have a name, the user color uses HTML notation ( i.e. #FF000080)
        if( IsCustomColorIdx( itemType, ii )
                && aStackupItem && aStackupItem->GetColor().StartsWith( wxT( "#" ) ) )
        {
            label = aStackupItem->GetColor();
            curr_color = COLOR4D( label );
        }
        else
        {
            label = wxGetTranslation( GetStandardColorName( itemType, ii ) );
            curr_color = GetStandardColor( itemType, ii );
        }

        wxBitmap layerbmp( m_colorSwatchesSize.x, m_colorSwatchesSize.y );
        LAYER_PRESENTATION::DrawColorSwatch( layerbmp, COLOR4D( 0, 0, 0, 0 ), curr_color );

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


