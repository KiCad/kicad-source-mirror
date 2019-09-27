/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <convert_to_biu.h>
#include <macros.h>             // arrayDim definition
#include <pcb_edit_frame.h>
#include <class_board.h>
#include <widgets/paged_dialog.h>
#include <widgets/layer_box_selector.h>
#include <wx/colordlg.h>
#include <wx/rawbmp.h>

#include "panel_board_stackup.h"
#include <panel_setup_layers.h>
#include "board_stackup_reporter.h"
#include <bitmaps.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include "dialog_dielectric_list_manager.h"

// Some wx widget ID to know what widget has fired a event:
#define ID_INCREMENT 128    // space between 2 ID type. Bigger than the layer count max

// The actual widget IDs are the base id + the row index.
// they are used in events to know the row index of the control that fired the event
enum WIDGETS_IDS
{
    ID_ITEM_MATERIAL = 10000,     // Be sure it is higher than other IDs used in the board setup dialog
    ID_ITEM_THICKNESS = ID_ITEM_MATERIAL + ID_INCREMENT,
    ID_ITEM_THICKNESS_LOCKED = ID_ITEM_THICKNESS + ID_INCREMENT,
    ID_ITEM_COLOR = ID_ITEM_THICKNESS_LOCKED + ID_INCREMENT,
};

// Default colors to draw icons:
static wxColor copperColor( 220, 180, 30 );
static wxColor dielectricColor( 75, 120, 75 );
static wxColor pasteColor( 200, 200, 200 );

static void drawBitmap( wxBitmap& aBitmap, wxColor aColor );


PANEL_SETUP_BOARD_STACKUP::PANEL_SETUP_BOARD_STACKUP( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame,
                                                      PANEL_SETUP_LAYERS* aPanelLayers ):
        PANEL_SETUP_BOARD_STACKUP_BASE( aParent->GetTreebook() ),
        m_delectricMatList( DIELECTRIC_SUBSTRATE_LIST::DL_MATERIAL_DIELECTRIC ),
        m_solderMaskMatList( DIELECTRIC_SUBSTRATE_LIST::DL_MATERIAL_SOLDERMASK ),
        m_silkscreenMatList( DIELECTRIC_SUBSTRATE_LIST::DL_MATERIAL_SILKSCREEN )
{
    m_frame = aFrame;
    m_panelLayers = aPanelLayers;
    m_board = m_frame->GetBoard();
    m_brdSettings = &m_board->GetDesignSettings();
    m_units = aFrame->GetUserUnits();

    m_enabledLayers = m_board->GetEnabledLayers() & BOARD_STACKUP::StackupAllowedBrdLayers();

    // Calculates a good size for color swatches (icons) in this dialog
    wxClientDC dc( this );
    m_colorSwatchesSize = dc.GetTextExtent( "XX" );
    m_colorComboSize = dc.GetTextExtent( wxString::Format( "XXX %s XXX",
                                         wxGetTranslation( NotSpecifiedPrm() ) ) );
    m_colorIconsSize = dc.GetTextExtent( "XXXX" );

    // Calculates a good size for wxTextCtrl to enter Epsilon R and Loss tan
    // ("0.000000" + margins)
    m_numericFieldsSize = dc.GetTextExtent( "X.XXXXXXX" );
    m_numericFieldsSize.y = -1;     // Use default for the vertical size

    // Calculates a minimal size for wxTextCtrl to enter a dim with units
    // ("000.0000000 mils" + margins)
    m_numericTextCtrlSize = dc.GetTextExtent( "XXX.XXXXXXX mils" );
    m_numericTextCtrlSize.y = -1;     // Use default for the vertical size

    // The grid column containing the lock checkbox is kept to a minimal
    // size. So we use a wxStaticBitmap: set the bitmap itself
    m_bitmapLockThickness->SetBitmap( KiScaledBitmap( locked_xpm, aFrame ) );

    // Gives a minimal size of wxTextCtrl showing dimensions+units
    m_thicknessCtrl->SetMinSize( m_numericTextCtrlSize );
    m_tcCTValue->SetMinSize( m_numericTextCtrlSize );

    buildLayerStackPanel();
    synchronizeWithBoard( true );
}


PANEL_SETUP_BOARD_STACKUP::~PANEL_SETUP_BOARD_STACKUP()
{
    disconnectEvents();
}


void PANEL_SETUP_BOARD_STACKUP::disconnectEvents()
{
	// Disconnect Events connected to items in m_controlItemsList
    for( wxControl* item: m_controlItemsList )
    {
        wxBitmapComboBox* cb = dynamic_cast<wxBitmapComboBox*>( item );

        if( cb )
            cb->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED,
                            wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onColorSelected ),
                            NULL, this );

        wxButton* matButt = dynamic_cast<wxButton*>( item );

        if( matButt )
            matButt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                 wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onMaterialChange ),
                                 NULL, this );

        wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( item );

        if( textCtrl )
            textCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED,
                                  wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onThicknessChange ),
                                  NULL, this );
    }
}


void PANEL_SETUP_BOARD_STACKUP::onExportToClipboard( wxCommandEvent& event )
{
    if( !transferDataFromUIToStackup() )
        return;

    // Build a ascii representation of stackup and copy it in the clipboard
    wxString report = BuildStackupReport( m_stackup, m_units );

    if( wxTheClipboard->Open() )
    {
        // This data objects are held by the clipboard,
        // so do not delete them in the app.
        wxTheClipboard->SetData( new wxTextDataObject( report ) );
        wxTheClipboard->Close();
    }
}


wxColor PANEL_SETUP_BOARD_STACKUP::GetSelectedColor( int aRow ) const
{
    wxBitmapComboBox* choice = static_cast<wxBitmapComboBox*>( m_rowUiItemsList[aRow].m_ColorCtrl );
    wxASSERT( choice );

    int idx = choice->GetSelection();

    if( idx != GetColorUserDefinedListIdx() ) // a standard color is selected
        return GetColorStandardList()[idx].m_Color;
    else
        return m_UserColors[aRow];
}


void PANEL_SETUP_BOARD_STACKUP::onUpdateThicknessValue( wxUpdateUIEvent& event )
{
    int thickness = 0;

    for( auto item : m_stackup.GetList() )
    {
        if( item->IsThicknessEditable() && item->m_Enabled )
           thickness += item->m_Thickness;
    }

    m_tcCTValue->SetValue( StringFromValue( m_units, thickness, true, true ) );
}


int PANEL_SETUP_BOARD_STACKUP::GetPcbTickness()
{
    return ValueFromString( m_units, m_thicknessCtrl->GetValue(), true );
}


void PANEL_SETUP_BOARD_STACKUP::synchronizeWithBoard( bool aFullSync )
{
    BOARD_STACKUP& brd_stackup = m_brdSettings->GetStackupDescriptor();

    // Calculate copper layer count from m_enabledLayers, and *do not use* brd_stackup
    // for that, because it is not necessary up to date
    // (for instance after modifying the layer count from the panel layers in dialog)
    LSET copperMask = m_enabledLayers & ( LSET::ExternalCuMask() | LSET::InternalCuMask() );
    int copperLayersCount = copperMask.count();

    if( aFullSync )
    {
        int thickness = m_brdSettings->GetBoardThickness();
        m_thicknessCtrl->SetValue( StringFromValue( m_units, thickness, true, true ) );

        m_rbDielectricConstraint->SetSelection( brd_stackup.m_HasDielectricConstrains ? 1 : 0 );
        m_choiceEdgeConn->SetSelection( brd_stackup.m_EdgeConnectorConstraints );
        m_cbCastellatedPads->SetValue( brd_stackup.m_CastellatedPads );
        m_cbEgdesPlated->SetValue( brd_stackup.m_EdgePlating );

        // find the choice depending on the initial finish setting
        wxArrayString initial_finish_list = GetCopperFinishStandardList( false );
        unsigned idx;

        for( idx = 0; idx < initial_finish_list.GetCount(); idx++ )
        {
            if( initial_finish_list[idx] ==  brd_stackup.m_FinishType )
                break;
        }

        // Now init the choice (use last choice: "User defined" if not found )
        if( idx >= initial_finish_list.GetCount() )
            idx = initial_finish_list.GetCount()-1;

        m_choiceFinish->SetSelection( idx );
    }

    int row = 0;

    for( BOARD_STACKUP_ITEM* item : m_stackup.GetList() )
    {
        BOARD_STACKUP_ROW_UI_ITEM& ui_row_item = m_rowUiItemsList[row];
        BOARD_STACKUP_ITEM* brd_stack_item = nullptr;

        // test for existing stackup items in board:
        for( BOARD_STACKUP_ITEM* brd_item : brd_stackup.GetList() )
        {
            if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC )
            {
                // Compare only BS_ITEM_TYPE_DIELECTRIC items
                if( brd_item->m_Type != BS_ITEM_TYPE_DIELECTRIC )
                    continue;

                if( item->m_DielectricLayerId  == brd_item->m_DielectricLayerId )
                    brd_stack_item = brd_item;
            }
            else if( item->m_LayerId == brd_item->m_LayerId )
                brd_stack_item = brd_item;

            if( brd_stack_item )
                break;
        }

        // Update panel stackup info if needed. If the board stackup item is not found
        // just refresh the default values
        if( brd_stack_item != nullptr && aFullSync )
        {
            *item = *brd_stack_item;

            if( item->IsMaterialEditable() )
            {
                wxTextCtrl* matName = dynamic_cast<wxTextCtrl*>( ui_row_item.m_MaterialCtrl );

                if( matName )
                {
                    if( IsPrmSpecified( item->m_Material ) )
                        matName->SetValue( item->m_Material );
                    else
                        matName->SetValue( wxGetTranslation( NotSpecifiedPrm() ) );
                }
            }

            if( item->IsThicknessEditable() )
            {
                wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ui_row_item.m_ThicknessCtrl );

                if( textCtrl )
                    textCtrl->SetValue( StringFromValue( m_units, item->m_Thickness, true, true ) );

                if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC )
                {
                    wxCheckBox* cb_box = dynamic_cast<wxCheckBox*> ( ui_row_item.m_ThicknessLockCtrl );

                    if( cb_box )
                        cb_box->SetValue( item->m_ThicknessLocked );
                }
            }

            if( item->IsColorEditable() )
            {
                auto bm_combo = dynamic_cast<wxBitmapComboBox*>( ui_row_item.m_ColorCtrl );
                int  color_idx = 0;

                if( item->m_Color.StartsWith( "#" ) )  // User defined color
                {
                    wxColour color( item->m_Color );
                    m_UserColors[row] = color;
                    color_idx = GetColorUserDefinedListIdx();

                    if( bm_combo )      // Update user color shown in the wxBitmapComboBox
                    {
                        bm_combo->SetString( color_idx, color.GetAsString( wxC2S_HTML_SYNTAX ) );
                        wxBitmap layerbmp( m_colorSwatchesSize.x, m_colorSwatchesSize.y );
                        LAYER_SELECTOR::DrawColorSwatch( layerbmp, COLOR4D(), COLOR4D( color ) );
                        bm_combo->SetItemBitmap( color_idx, layerbmp );
                    }
                }
                else
                {
                    const FAB_LAYER_COLOR* color_list = GetColorStandardList();

                    for( int ii = 0; ii < GetColorStandardListCount(); ii++ )
                    {
                        if( color_list[ii].m_ColorName == item->m_Color )
                        {
                            color_idx = ii;
                            break;
                        }
                    }
                }

                if( bm_combo )
                    bm_combo->SetSelection( color_idx );
            }

            if( item->HasEpsilonRValue() )
            {
                wxString txt;
                txt.Printf( "%.1f", item->m_EpsilonR );
                wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ui_row_item.m_EpsilonCtrl );

                if( textCtrl )
                    textCtrl->SetValue( txt );
            }

            if( item->HasLossTangentValue() )
            {
                wxString txt;
                txt.Printf( "%g", item->m_LossTangent );
                wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ui_row_item.m_LossTgCtrl );

                if( textCtrl )
                    textCtrl->SetValue( txt );
            }
        }

        // Now enable/disable stackup items, according to the m_enabledLayers config
        bool show_item;

        if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC )
            // the m_DielectricLayerId is not a copper layer id, it is a dielectric idx from 1
            show_item = item->m_DielectricLayerId < copperLayersCount;
        else
            show_item = m_enabledLayers[item->m_LayerId];

        item->m_Enabled = show_item;

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

        row++;
    }

    updateIconColor();
}


void PANEL_SETUP_BOARD_STACKUP::addMaterialChooser( wxWindowID aId,
                                                    const wxString * aMaterialName,
                                                    BOARD_STACKUP_ROW_UI_ITEM& aUiRowItem )
{
	wxBoxSizer* bSizerMat = new wxBoxSizer( wxHORIZONTAL );
	m_fgGridSizer->Add( bSizerMat, 1, wxEXPAND, 2 );
    wxTextCtrl* textCtrl = new wxTextCtrl( m_scGridWin, wxID_ANY );

    if( aMaterialName )
    {
        if( IsPrmSpecified( *aMaterialName ) )
            textCtrl->SetValue( *aMaterialName );
        else
            textCtrl->SetValue( wxGetTranslation( NotSpecifiedPrm() ) );
    }

    textCtrl->SetMinSize( m_numericTextCtrlSize );
	bSizerMat->Add( textCtrl, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxButton* m_buttonMat = new wxButton( m_scGridWin, aId, _("..."),
                                          wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerMat->Add( m_buttonMat, 0, wxALIGN_CENTER_VERTICAL, 2 );

    m_buttonMat->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                       wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onMaterialChange ),
                       NULL, this );
    m_controlItemsList.push_back( m_buttonMat );

    aUiRowItem.m_MaterialCtrl = textCtrl;
    aUiRowItem.m_MaterialButt = m_buttonMat;
}


wxControl* PANEL_SETUP_BOARD_STACKUP::addSpacer()
{
    wxStaticText* emptyText = new wxStaticText( m_scGridWin, wxID_ANY, wxEmptyString );
    m_fgGridSizer->Add( emptyText, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND );
    return emptyText;
}


void PANEL_SETUP_BOARD_STACKUP::buildLayerStackPanel()
{
    // for dielectric: layer type keyword is "core"
    m_core_prepreg_choice.Add( _( "Core" ) );
    // for dielectric: layer type keyword is "prepreg"
    m_core_prepreg_choice.Add( _( "PrePreg" ) );

    // Build a full stackup for the dialog, with a active copper layer count
    // = current board layer count to calculate a reasonable default
    // dielectric thickness, for board having no stackup initalized:
    m_stackup.BuildDefaultStackupList( nullptr, m_brdSettings->GetCopperLayerCount() );

    int row = 0;

    const FAB_LAYER_COLOR* color_list = GetColorStandardList();

    for( auto item : m_stackup.GetList() )
    {
        BOARD_STACKUP_ROW_UI_ITEM ui_row_item;

        bool show_item = true;//false;
        ui_row_item.m_isEnabled = true;

        // Reserve room in m_UserColors to store usercolor definition
        m_UserColors.push_back( color_list[GetColorUserDefinedListIdx()].m_Color );

        // Add color swatch icon. The color will be updated later,
        // when all widgets are initialized
        wxStaticBitmap* bitmap = new wxStaticBitmap( m_scGridWin, wxID_ANY, wxNullBitmap );
        m_fgGridSizer->Add( bitmap, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT );
        ui_row_item.m_Icon = bitmap;

        if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC )
        {
            wxString lname;
            lname.Printf( _( "Dielectric %d" ), item->m_DielectricLayerId );
            wxStaticText* st_text = new wxStaticText( m_scGridWin, wxID_ANY, lname );
            m_fgGridSizer->Add( st_text, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
            ui_row_item.m_LayerName = st_text;

            wxChoice* choice = new wxChoice( m_scGridWin, wxID_ANY, wxDefaultPosition,
                                             wxDefaultSize, m_core_prepreg_choice );
            choice->SetSelection( item->m_TypeName == KEY_CORE ? 0 : 1 );
            m_fgGridSizer->Add( choice, 0, wxEXPAND|wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );

            ui_row_item.m_LayerTypeCtrl = choice;
        }
        else
        {
            item->m_LayerName = m_board->GetLayerName( item->m_LayerId );
            wxStaticText* st_text =  new wxStaticText( m_scGridWin, wxID_ANY, item->m_LayerName );
            m_fgGridSizer->Add( st_text, 0, wxALL|wxALIGN_CENTER_VERTICAL, 1 );
            st_text->Show( show_item );
            ui_row_item.m_LayerName = st_text;

            wxString lname;

            if( item->m_TypeName == KEY_COPPER )
                lname = _( "Copper" );
            else
                lname = wxGetTranslation( item->m_TypeName );

            st_text = new wxStaticText( m_scGridWin, wxID_ANY, lname );
            m_fgGridSizer->Add( st_text, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
            ui_row_item.m_LayerTypeCtrl = st_text;
        }

        if( item->IsMaterialEditable() )
        {
            addMaterialChooser( ID_ITEM_MATERIAL+row, &item->m_Material, ui_row_item );
        }
        else
        {
            ui_row_item.m_MaterialCtrl = addSpacer();
        }

        if( item->IsThicknessEditable() )
        {
            wxTextCtrl* textCtrl = new wxTextCtrl( m_scGridWin, ID_ITEM_THICKNESS+row );
            textCtrl->SetMinSize( m_numericTextCtrlSize );
            textCtrl->SetValue( StringFromValue( m_units, item->m_Thickness, true, true ) );
            m_fgGridSizer->Add( textCtrl, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
            m_controlItemsList.push_back( textCtrl );
            textCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED,
                               wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onThicknessChange ),
                               NULL, this );
            ui_row_item.m_ThicknessCtrl = textCtrl;

            if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC )
            {
                wxCheckBox* cb_box = new wxCheckBox( m_scGridWin, ID_ITEM_THICKNESS_LOCKED+row,
                                                     wxEmptyString );
                cb_box->SetValue( item->m_ThicknessLocked );
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
            int color_idx = 0;

            if( item->m_Color.StartsWith( "#" ) )  // User defined color
            {
                wxColour color( item->m_Color );
                m_UserColors[row] = color;
                color_idx = GetColorUserDefinedListIdx();
            }
            else
            {
                for( int ii = 0; ii < GetColorStandardListCount(); ii++ )
                {
                    if( color_list[ii].m_ColorName == item->m_Color )
                    {
                        color_idx = ii;
                        break;
                    }
                }
            }

            wxBitmapComboBox* bm_combo = createBmComboBox( item, row );
            m_colorComboSize.y = bm_combo->GetSize().y;
            bm_combo->SetMinSize( m_colorComboSize );
            m_fgGridSizer->Add( bm_combo, 0, wxEXPAND|wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 2 );
            bm_combo->SetSelection( color_idx );
            ui_row_item.m_ColorCtrl = bm_combo;
        }
        else
        {
            ui_row_item.m_ColorCtrl = addSpacer();
        }

        if( item->HasEpsilonRValue() )
        {
            wxString txt;
            txt.Printf( "%.1f", item->m_EpsilonR );
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
            wxString txt;
            txt.Printf( "%g", item->m_LossTangent );
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

        m_rowUiItemsList.push_back( ui_row_item );

        row++;
    }

    // Get the translated list of choices and init m_choiceFinish
    wxArrayString finish_list = GetCopperFinishStandardList( true );
    m_choiceFinish->Append( finish_list );
    m_choiceFinish->SetSelection( 0 );      // Will be correctly set later

    updateIconColor();
    m_scGridWin->Layout();
}



// Transfer current UI settings to m_stackup but not to the board
bool PANEL_SETUP_BOARD_STACKUP::transferDataFromUIToStackup()
{
    // First, verify the list of layers currently in stackup:
    // if it does not mach the list of layers set in PANEL_SETUP_LAYERS
    // prompt the user to update the stackup

    // the current enabled layers in PANEL_SETUP_LAYERS
    LSET layersList = m_panelLayers->GetUILayerMask() & BOARD_STACKUP::StackupAllowedBrdLayers();

    if( m_enabledLayers != layersList )
    {
        wxMessageBox( _( "Stackup not up to date. Verify it" ) );
        return false;
    }

    // The board thickness and the thickness from stackup settings should be compatible
    // so verify that compatibility
    int pcbTickness = GetPcbTickness() ;
    int stackup_thickness = 0;
    bool thickness_error = false;   // Set to true if a negative thickness value is found

    for( auto item : m_stackup.GetList() )
    {
        if( item->IsThicknessEditable() && item->m_Enabled )
        {
            stackup_thickness += item->m_Thickness;

            if( item->m_Thickness < 0 )
                thickness_error = true;
        }
    }

    if( thickness_error )
    {
        wxMessageBox( _( "A layer thickness is < 0. Fix it" ) );
        return false;
    }

    int delta = std::abs( stackup_thickness - pcbTickness );
    double relative_error = (double)delta/pcbTickness;

    // warn user if relative_error > 0.01
    const double relative_error_max = 0.01;

    if( relative_error > relative_error_max )
    {
        wxString msg;
        msg.Printf( _( "Board thickness %s differs from stackup thickness %s\n"
                       "Allowed max error %s" ),
                    StringFromValue( m_units, pcbTickness, true, true ),
                    StringFromValue( m_units, stackup_thickness, true, true ),
                    StringFromValue( m_units, KiROUND( relative_error_max*pcbTickness),
                                     true, true ) );

        wxMessageBox( msg );
        return false;
    }


    int row = 0;
    wxString txt;
    wxString error_msg;
    bool success = true;
    double value;

    for( auto item : m_stackup.GetList() )
    {
        // Skip stackup items useless for the current board
        if( !item->m_Enabled )
        {
            row++;
            continue;
        }

        item->m_LayerName = m_rowUiItemsList[row].m_LayerName->GetLabel();

        if( item->HasEpsilonRValue() )
        {
            wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_EpsilonCtrl );
            txt = textCtrl->GetValue();

            if( txt.ToDouble( &value ) && value >= 0.0 )
                item->m_EpsilonR = value;
            else if( txt.ToCDouble( &value ) && value >= 0.0 )
                item->m_EpsilonR = value;
            else
            {
                success = false;
                error_msg << _( "Incorrect value for Epsilon R (Epsilon R must be positive or null if not used)" );
            }
        }

        if( item->HasLossTangentValue() )
        {
            wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_LossTgCtrl );
            txt = textCtrl->GetValue();

            if( txt.ToDouble( &value ) && value >= 0.0 )
                item->m_LossTangent = value;
            else if( txt.ToCDouble( &value ) && value >= 0.0 )
                item->m_LossTangent = value;
            else
            {
                success = false;
                if( !error_msg.IsEmpty() )
                    error_msg << "\n";
                error_msg << _( "Incorrect value for Loss tg (Loss tg must be positive or null if not used)" );
            }
        }

        if( item->IsMaterialEditable() )
        {
            wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>(  m_rowUiItemsList[row].m_MaterialCtrl );
            item->m_Material = textCtrl->GetValue();

            // Ensure the not specified mat name is the keyword, not its translation
            // to avoid any issue is the language setting changes
            if( !IsPrmSpecified( item->m_Material ) )
                item->m_Material = NotSpecifiedPrm();
        }

        if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC )
        {
            // Choice is Core or Prepreg:
            wxChoice* choice = static_cast<wxChoice*>( m_rowUiItemsList[row].m_LayerTypeCtrl );

            int idx = choice->GetSelection();

            if( idx == 0 )
                item->m_TypeName = KEY_CORE;
            else
                item->m_TypeName = KEY_PREPREG;
        }

        if( item->IsThicknessEditable() )
        {
            wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_ThicknessCtrl );
            txt = textCtrl->GetValue();

            item->m_Thickness = ValueFromString( m_frame->GetUserUnits(), txt, true );

            if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC )
            {
                // Dielectric thickness layer can have a locked thickness:
                wxCheckBox* cb_box = static_cast<wxCheckBox*>
                                        ( m_rowUiItemsList[row].m_ThicknessLockCtrl );
                item->m_ThicknessLocked = cb_box && cb_box->GetValue();
            }
        }

        if( item->IsColorEditable() )
        {
            const FAB_LAYER_COLOR* color_list = GetColorStandardList();

            wxBitmapComboBox* choice = static_cast<wxBitmapComboBox*>( m_rowUiItemsList[row].m_ColorCtrl );
            int idx = choice->GetSelection();

            if( idx == GetColorUserDefinedListIdx() )
                item->m_Color = m_UserColors[row].GetAsString( wxC2S_HTML_SYNTAX );
            else
                item->m_Color = color_list[idx].m_ColorName;
        }
        row++;
    }

    if( !success )
    {
        wxMessageBox( error_msg, _( "Errors" ) );
        return false;
    }

    wxArrayString finish_list = GetCopperFinishStandardList( false );
    int finish = m_choiceFinish->GetSelection() >= 0 ? m_choiceFinish->GetSelection() : 0;
    m_stackup.m_FinishType = finish_list[finish];
    m_stackup.m_HasDielectricConstrains = m_rbDielectricConstraint->GetSelection() == 1;
    m_stackup.m_EdgeConnectorConstraints = (BS_EDGE_CONNECTOR_CONSTRAINTS)m_choiceEdgeConn->GetSelection();
    m_stackup.m_CastellatedPads = m_cbCastellatedPads->GetValue();
    m_stackup.m_EdgePlating = m_cbEgdesPlated->GetValue();

    return true;
}


bool PANEL_SETUP_BOARD_STACKUP::TransferDataFromWindow()
{
    if( !transferDataFromUIToStackup() )
        return false;

    BOARD_STACKUP& brd_stackup = m_brdSettings->GetStackupDescriptor();

    brd_stackup.m_FinishType = m_stackup.m_FinishType;
    brd_stackup.m_HasDielectricConstrains = m_stackup.m_HasDielectricConstrains;
    brd_stackup.m_EdgeConnectorConstraints = m_stackup.m_EdgeConnectorConstraints;
    brd_stackup.m_CastellatedPads = m_stackup.m_CastellatedPads;
    brd_stackup.m_EdgePlating = m_stackup.m_EdgePlating;

    // copy enabled items to the new board stackup
    brd_stackup.RemoveAll();

    for( auto item : m_stackup.GetList() )
    {
        if( item->m_Enabled )
        brd_stackup.Add( new BOARD_STACKUP_ITEM( *item ) );
    }

    m_brdSettings->SetBoardThickness( GetPcbTickness() );
    m_brdSettings->m_HasStackup = true;

    return true;
}


void PANEL_SETUP_BOARD_STACKUP::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD* savedBrd = m_board;
    BOARD_DESIGN_SETTINGS* savedSettings = m_brdSettings;
    m_brdSettings = &aBoard->GetDesignSettings();

    m_enabledLayers = m_panelLayers->GetUILayerMask() & BOARD_STACKUP::StackupAllowedBrdLayers();
    synchronizeWithBoard( true );

    m_brdSettings = savedSettings;
    m_board = savedBrd;

    Layout();
    Refresh();
}


void PANEL_SETUP_BOARD_STACKUP::OnLayersOptionsChanged( LSET aNewLayerSet )
{
    // First, verify the list of layers currently in stackup:
    // if it does not mach the list of layers set in PANEL_SETUP_LAYERS
    // rebuild the panel

    // the current enabled layers in PANEL_SETUP_LAYERS
    LSET layersList = m_panelLayers->GetUILayerMask() & BOARD_STACKUP::StackupAllowedBrdLayers();

    if( m_enabledLayers != layersList )
    {
        m_enabledLayers = layersList;

        synchronizeWithBoard( false );

        Layout();
        Refresh();
    }
}


void PANEL_SETUP_BOARD_STACKUP::onCalculateDielectricThickness( wxCommandEvent& event )
{
    // Collect thickness of all layers but dielectric
    int thickness = 0;
    int fixed_thickness_cnt = 0;
    bool thickness_error = false;   // True if a locked thickness value in list is < 0

    int row = 0;
    for( auto item : m_stackup.GetList() )
    {
        if( !item->IsThicknessEditable() || !item->m_Enabled )
        {
            row++;
            continue;
        }

        if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC )
        {
            wxCheckBox* checkBox = static_cast<wxCheckBox*>( m_rowUiItemsList[row].m_ThicknessLockCtrl );

            if( !checkBox->GetValue() )  // Only not locked dielectric thickness can be modified
            {
                row++;
                continue;
            }
            else
            {
                fixed_thickness_cnt++;

                if( item->m_Thickness < 0 )
                    thickness_error = true;
            }
        }

        thickness += item->m_Thickness;
        row++;
    }

    if( thickness_error )
    {
        wxMessageBox( _( "A locked dielectric thickness is < 0\n"
                         "Unlock it or change its thickness") );
        return;
    }

    LSET copperMask = m_enabledLayers & ( LSET::ExternalCuMask() | LSET::InternalCuMask() );
    int copperLayersCount = copperMask.count();

    // the number of adjustable dielectric layers must obvioulsly be > 0
    // So verify the user has at least one dielectric layer free
    int adjustableDielectricCount = copperLayersCount - 1 - fixed_thickness_cnt;

    if( adjustableDielectricCount <= 0 )
    {
        wxMessageBox( _( "Cannot calculate dielectric thickness\n"
                         "At least one dielectric layer must be not locked") );
        return;
    }

    int dielectric_thickness = GetPcbTickness() - thickness;

    if( dielectric_thickness <= 0 ) // fixed thickness is too big: cannot calculate free thickness
    {
        wxMessageBox( _( "Cannot calculate dielectric thickness\n"
                         "Fixed thickness too big or board thickness too small") );
        return;
    }

    dielectric_thickness /= adjustableDielectricCount;

    // Update items thickness, and the values displayed on screen
    row = 0;
    for( auto item : m_stackup.GetList() )
    {
        if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC && item->m_Enabled  )
        {
            wxCheckBox* checkBox = static_cast<wxCheckBox*>( m_rowUiItemsList[row].m_ThicknessLockCtrl );

            if( !checkBox->GetValue() )  // Not locked thickness: can be modified
            {
                item->m_Thickness = dielectric_thickness;
                wxTextCtrl* textCtrl = static_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_ThicknessCtrl );
                textCtrl->SetValue( StringFromValue( m_units, item->m_Thickness, true, true ) );
            }
        }

        row++;
    }
}


void PANEL_SETUP_BOARD_STACKUP::onColorSelected( wxCommandEvent& event )
{
    int idx = event.GetSelection();
    int item_id = event.GetId();

    int row = item_id - ID_ITEM_COLOR;
    wxASSERT( (int)m_UserColors.size() > row );

    if( GetColorStandardListCount()-1 == idx )   // Set user color is the last option in list
    {
        wxColourDialog dlg( this );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxBitmapComboBox* combo = static_cast<wxBitmapComboBox*>( FindWindowById( item_id ) );
            wxColour color = dlg.GetColourData().GetColour();
            m_UserColors[row] = color;

            combo->SetString( idx, color.GetAsString( wxC2S_HTML_SYNTAX ) );

            wxBitmap layerbmp( m_colorSwatchesSize.x, m_colorSwatchesSize.y );
            LAYER_SELECTOR::DrawColorSwatch( layerbmp, COLOR4D( 0, 0, 0, 0 ),
                                             COLOR4D( color ) );
            combo->SetItemBitmap( combo->GetCount()-1, layerbmp );
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

        if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC )
            mat_list = &m_delectricMatList;
        else if( item->m_Type == BS_ITEM_TYPE_SOLDERMASK )
            mat_list = &m_solderMaskMatList;
        else if( item->m_Type == BS_ITEM_TYPE_SILKSCREEN )
            mat_list = &m_silkscreenMatList;

        else
            continue;

        int idx = mat_list->FindSubstrate( item->m_Material,
                                           item->m_EpsilonR,
                                           item->m_LossTangent );

        if( idx < 0 && !item->m_Material.IsEmpty() )
        {
            // This material is not in list: add it
            DIELECTRIC_SUBSTRATE new_mat;
            new_mat.m_Name = item->m_Material;
            new_mat.m_EpsilonR = item->m_EpsilonR;
            new_mat.m_LossTangent = item->m_LossTangent;
            mat_list->AppendSubstrate( new_mat );
        }
    }

    int row  = event.GetId() - ID_ITEM_MATERIAL;
    BOARD_STACKUP_ITEM* item = GetStackupItem( row );
    DIELECTRIC_SUBSTRATE_LIST* item_mat_list = nullptr;

    switch( item->m_Type )
    {
    case BS_ITEM_TYPE_DIELECTRIC:
        item_mat_list = &m_delectricMatList;
        break;

    case BS_ITEM_TYPE_SOLDERMASK:
        item_mat_list = &m_solderMaskMatList;
        break;

    case BS_ITEM_TYPE_SILKSCREEN:
        item_mat_list = &m_silkscreenMatList;
        break;

    default:
        item_mat_list = nullptr;
        break;
    }

    DIALOG_DIELECTRIC_MATERIAL dlg( this, *item_mat_list );

    if( dlg.ShowModal() != wxID_OK )
        return;

    DIELECTRIC_SUBSTRATE substrate = dlg.GetSelectedSubstrate();

    if( substrate.m_Name.IsEmpty() )    // No substrate specified
        return;

    // Update Name, Epsilon R and Loss tg
    item->m_Material = substrate.m_Name;
    item->m_EpsilonR = substrate.m_EpsilonR;
    item->m_LossTangent = substrate.m_LossTangent;

    wxTextCtrl* textCtrl;
    textCtrl = static_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_MaterialCtrl );
    textCtrl->SetValue( item->m_Material );

    // some layers have a material choice but not EpsilonR ctrl
    if( item->HasEpsilonRValue() )
    {
        textCtrl = dynamic_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_EpsilonCtrl );

        if( textCtrl )
            textCtrl->SetValue( item->FormatEpsilonR() );
    }

    // some layers have a material choice but not loss tg ctrl
    if( item->HasLossTangentValue() )
    {
        textCtrl = dynamic_cast<wxTextCtrl*>( m_rowUiItemsList[row].m_LossTgCtrl );

        if( textCtrl )
            textCtrl->SetValue( item->FormatLossTangent() );
    }
}


void PANEL_SETUP_BOARD_STACKUP::onThicknessChange( wxCommandEvent& event )
{
    int row  = event.GetId() - ID_ITEM_THICKNESS;
    wxString value = event.GetString();

    BOARD_STACKUP_ITEM* item = GetStackupItem( row );
    item->m_Thickness = ValueFromString( m_frame->GetUserUnits(), value, true );
}


BOARD_STACKUP_ITEM* PANEL_SETUP_BOARD_STACKUP::GetStackupItem( int aIndex )
{
    return m_stackup.GetStackupLayer( aIndex );
}


wxColor PANEL_SETUP_BOARD_STACKUP::getColorIconItem( int aRow )
{
    BOARD_STACKUP_ITEM* layer = GetStackupItem( aRow );
    wxColor color;

    switch( layer->m_Type )
    {
    case BS_ITEM_TYPE_COPPER:
        color = copperColor;
        break;

    case BS_ITEM_TYPE_DIELECTRIC:
        color = dielectricColor;
        break;

    case BS_ITEM_TYPE_SOLDERMASK:
        color = GetSelectedColor( aRow );
        break;

    case BS_ITEM_TYPE_SILKSCREEN:
        color = GetSelectedColor( aRow );
        break;

    case BS_ITEM_TYPE_SOLDERPASTE:
        color = pasteColor;
        break;

    case BS_ITEM_TYPE_UNDEFINED:    // Should not happen
        wxASSERT( 0 );
        break;
    }

    return color;
}


void PANEL_SETUP_BOARD_STACKUP::updateIconColor( int aRow )
{
    if( aRow >= 0 )
    {
        wxStaticBitmap* st_bitmap = m_rowUiItemsList[aRow].m_Icon;
        // explicit depth important under MSW
        wxBitmap bmp(m_colorIconsSize.x, m_colorIconsSize.y / 2, 24);
        drawBitmap( bmp, getColorIconItem( aRow ) );
        st_bitmap->SetBitmap( bmp );
        return;
    }

    for( unsigned row = 0; row < m_rowUiItemsList.size(); row++ )
    {
        // explicit depth important under MSW
        wxBitmap bmp(m_colorIconsSize.x, m_colorIconsSize.y / 2, 24);
        drawBitmap( bmp, getColorIconItem( row ) );
        m_rowUiItemsList[row].m_Icon->SetBitmap( bmp );
    }
}


wxBitmapComboBox* PANEL_SETUP_BOARD_STACKUP::createBmComboBox( BOARD_STACKUP_ITEM* aStackupItem, int aRow )
{
    wxBitmapComboBox* combo = new wxBitmapComboBox( m_scGridWin, ID_ITEM_COLOR+aRow,
                                                    wxEmptyString, wxDefaultPosition,
                                                    wxDefaultSize, 0, nullptr, wxCB_READONLY );
    // Fills the combo box with choice list + bitmaps
    const FAB_LAYER_COLOR* color_list = GetColorStandardList();

    for( int ii = 0; ii < GetColorStandardListCount(); ii++ )
    {
        const FAB_LAYER_COLOR& item = color_list[ii];

        wxColor curr_color = item.m_Color;
        wxString label;

        // Defined colors have a name, the user color uses the HTML notation ( i.e. #FF0000)
        if( GetColorStandardListCount()-1 > (int)combo->GetCount() )
            label = wxGetTranslation( item.m_ColorName );
        else    // Append the user color, if specified, else add a default user color
        {
            if( aStackupItem && aStackupItem->m_Color.StartsWith( "#" ) )
            {
                curr_color = wxColour( aStackupItem->m_Color );
                label = aStackupItem->m_Color;
            }
            else
                label = curr_color.GetAsString( wxC2S_HTML_SYNTAX );
        }

        wxBitmap layerbmp( m_colorSwatchesSize.x, m_colorSwatchesSize.y );
        LAYER_SELECTOR::DrawColorSwatch( layerbmp, COLOR4D( 0, 0, 0, 0 ),
                                         COLOR4D( curr_color ) );

        combo->Append( label, layerbmp );
    }

#ifdef __WXGTK__
    // Adjust the minimal width. On GTK, the size calculated by wxWidgets is not good
    // bitmaps are ignored
    combo->Fit();
    int min_width = combo->GetSize().x;
    min_width += m_colorSwatchesSize.x;
    combo->SetMinSize( wxSize( min_width, -1 ) );
#endif

    // add the wxBitmapComboBox to wxControl list, to be able to disconnect the event
    // on exit
    m_controlItemsList.push_back( combo );

    combo->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED,
                    wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP::onColorSelected ),
                    NULL, this );

    return combo;
}


void drawBitmap( wxBitmap& aBitmap, wxColor aColor )
{
    wxNativePixelData data( aBitmap );
    wxNativePixelData::Iterator p(data);

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
        p.OffsetY(data, 1);
    }
}
