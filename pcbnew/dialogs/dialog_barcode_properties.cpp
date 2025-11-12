/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <core/type_helpers.h>
#include <base_units.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <pcb_barcode.h>
#include <pcb_text.h>
#include <layer_ids.h>
#include <math/util.h>
#include <dialog_barcode_properties.h>
#include <pcb_base_frame.h>
#include <pcb_base_edit_frame.h>
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <settings/color_settings.h>
#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcb_layer_box_selector.h>
// For BOX2D viewport checks
#include <math/box2.h>


DIALOG_BARCODE_PROPERTIES::DIALOG_BARCODE_PROPERTIES( PCB_BASE_FRAME* aParent, PCB_BARCODE* aBarcode ) :
        DIALOG_BARCODE_PROPERTIES_BASE( aParent ),
        m_parent( aParent ),
        m_posX( aParent, m_posXLabel, m_posXCtrl, m_posXUnits ),
        m_posY( aParent, m_posYLabel, m_posYCtrl, m_posYUnits ),
        m_sizeX( aParent, m_sizeXLabel, m_sizeXCtrl, m_sizeXUnits ),
        m_sizeY( aParent, m_sizeYLabel, m_sizeYCtrl, m_sizeYUnits ),
        m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits ),
        m_orientation( aParent, m_orientationLabel, m_orientationCtrl, m_orientationUnits ),
        m_knockoutMarginX( aParent, m_marginXLabel, m_marginXCtrl, m_marginXUnits ),
        m_knockoutMarginY( aParent, m_marginYLabel, m_marginYCtrl, m_marginYUnits )
{
    m_currentBarcode = aBarcode; // aBarcode can not be NULL (no FOOTPRINT editor?)

    m_board = m_parent->GetBoard();

    m_dummyBarcode = new PCB_BARCODE( nullptr );

    // Initialize canvas to be able to display the dummy barcode:
    prepareCanvas();

    m_sdbSizerOK->SetDefault();
}


DIALOG_BARCODE_PROPERTIES::~DIALOG_BARCODE_PROPERTIES()
{
    delete m_dummyBarcode;
}


void DIALOG_BARCODE_PROPERTIES::OnCancel( wxCommandEvent& event )
{
    // Mandatory to avoid m_panelShowPadGal trying to draw something
    // in a non valid context during closing process:
    m_panelShowBarcodeGal->StopDrawing();

    // Now call default handler for wxID_CANCEL command event
    event.Skip();
}


void DIALOG_BARCODE_PROPERTIES::prepareCanvas()
{
    // Initialize the canvas to display the barcode

    m_panelShowBarcodeGal->UpdateColors();
    m_panelShowBarcodeGal->SwitchBackend( m_parent->GetCanvas()->GetBackend() );
    m_panelShowBarcodeGal->SetStealsFocus( false );

    m_panelShowBarcodeGal->Show();

    KIGFX::VIEW* view = m_panelShowBarcodeGal->GetView();

    // gives a non null grid size (0.001mm) because GAL layer does not like a 0 size grid:
    double gridsize = pcbIUScale.mmToIU( 0.001 );
    view->GetGAL()->SetGridSize( VECTOR2D( gridsize, gridsize ) );
    // And do not show the grid:
    view->GetGAL()->SetGridVisibility( false );
    view->Add( m_dummyBarcode );

    m_panelShowBarcodeGal->StartDrawing();
    Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_BARCODE_PROPERTIES::OnResize ) );
}


void DIALOG_BARCODE_PROPERTIES::initValues()
{
    // Copy current barcode into our working copy
    if( m_currentBarcode )
        *m_dummyBarcode = *m_currentBarcode;

    m_dummyBarcode->AssembleBarcode();

    m_orientation.SetUnits( EDA_UNITS::DEGREES );

    // Configure the layers list selector.  Note that footprints are built outside the current
    // board and so we may need to show all layers if the barcode is on an unactivated layer.
    if( !m_parent->GetBoard()->IsLayerEnabled( m_dummyBarcode->GetLayer() ) )
        m_cbLayer->ShowNonActivatedLayers( true );

    m_cbLayer->SetLayersHotkeys( false );
    m_cbLayer->SetBoardFrame( m_parent );
    m_cbLayer->Resync();
}


void DIALOG_BARCODE_PROPERTIES::OnResize( wxSizeEvent& event )
{
    m_panelShowBarcodeGal->Refresh();
    event.Skip();
}


void DIALOG_BARCODE_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    // Error correction options are only meaningful for QR codes
    bool enableEC = m_barcode->GetSelection() >= to_underlying( BARCODE_T::QR_CODE );
    m_errorCorrection->Enable( enableEC );

    m_textSize.Enable( m_cbShowText->GetValue() );

    m_knockoutMarginX.Enable( m_cbKnockout->GetValue() );
    m_knockoutMarginY.Enable( m_cbKnockout->GetValue() );

    if( enableEC )
    {
        // Micro QR codes do not support High (H) error correction level
        bool isMicroQR = ( m_barcode->GetSelection() == to_underlying( BARCODE_T::MICRO_QR_CODE ) );

        // Enable/disable the High option (index 3)
        m_errorCorrection->Enable( 3, !isMicroQR );

        // If currently High is selected and we switched to Micro QR, change to a valid option
        if( isMicroQR && m_errorCorrection->GetSelection() == 3 )
        {
            m_errorCorrection->SetSelection( 2 ); // Default to Q (Quartile), consistent with SetErrorCorrection
        }
    }
}


bool DIALOG_BARCODE_PROPERTIES::TransferDataToWindow()
{
    initValues();

    if( m_currentBarcode )
        *m_dummyBarcode = *m_currentBarcode;

    m_textInput->ChangeValue( m_dummyBarcode->GetText() );
    m_cbLocked->SetValue( m_dummyBarcode->IsLocked() );
    m_cbLayer->SetLayerSelection( m_dummyBarcode->GetLayer() );

    // Position
    m_posX.ChangeValue( m_dummyBarcode->GetPosition().x );
    m_posY.ChangeValue( m_dummyBarcode->GetPosition().y );

    // Size
    m_sizeX.ChangeValue( m_dummyBarcode->GetWidth() );
    m_sizeY.ChangeValue( m_dummyBarcode->GetHeight() );
    m_textSize.ChangeValue( m_dummyBarcode->GetTextSize() );

    // Orientation
    m_orientation.ChangeAngleValue( m_dummyBarcode->GetAngle() );

    // Show text option
    m_cbShowText->SetValue( m_dummyBarcode->Text().IsVisible() );

    m_cbKnockout->SetValue( m_dummyBarcode->IsKnockout() );
    m_knockoutMarginX.ChangeValue( m_dummyBarcode->GetMargin().x );
    m_knockoutMarginY.ChangeValue( m_dummyBarcode->GetMargin().y );

    // Barcode type
    switch( m_dummyBarcode->GetKind() )
    {
    case BARCODE_T::CODE_39:       m_barcode->SetSelection( 0 ); break;
    case BARCODE_T::CODE_128:      m_barcode->SetSelection( 1 ); break;
    case BARCODE_T::DATA_MATRIX:   m_barcode->SetSelection( 2 ); break;
    case BARCODE_T::QR_CODE:       m_barcode->SetSelection( 3 ); break;
    case BARCODE_T::MICRO_QR_CODE: m_barcode->SetSelection( 4 ); break;
    default:                       m_barcode->SetSelection( 0 ); break;
    }

    // Error correction level
    switch( m_dummyBarcode->GetErrorCorrection() )
    {
    case BARCODE_ECC_T::L: m_errorCorrection->SetSelection( 0 ); break;
    case BARCODE_ECC_T::M: m_errorCorrection->SetSelection( 1 ); break;
    case BARCODE_ECC_T::Q: m_errorCorrection->SetSelection( 2 ); break;
    case BARCODE_ECC_T::H: m_errorCorrection->SetSelection( 3 ); break;
    default:               m_errorCorrection->SetSelection( 0 ); break;
    }

    // Now all widgets have the size fixed, call finishDialogSettings
    finishDialogSettings();

    refreshPreview();

    wxUpdateUIEvent dummy;
    OnUpdateUI( dummy );

    return true;
}


bool DIALOG_BARCODE_PROPERTIES::TransferDataFromWindow()
{
    BOARD_COMMIT commit( m_parent );
    commit.Modify( m_currentBarcode );

    if( !transferDataToBarcode( m_currentBarcode ) )
        return false;

    m_parent->GetCanvas()->Refresh();

    commit.Push( _( "Modify barcode" ) );

    return true;
}


bool DIALOG_BARCODE_PROPERTIES::transferDataToBarcode( PCB_BARCODE* aBarcode )
{
    if( !aBarcode )
        return false;

    aBarcode->SetText( m_textInput->GetValue() );
    aBarcode->SetLocked( m_cbLocked->GetValue() );
    aBarcode->SetLayer( ToLAYER_ID( m_cbLayer->GetLayerSelection() ) );

    // Position
    aBarcode->SetPosition( VECTOR2I( m_posX.GetIntValue(), m_posY.GetIntValue() ) );

    // Size
    aBarcode->SetWidth( m_sizeX.GetIntValue() );
    aBarcode->SetHeight( m_sizeY.GetIntValue() );
    aBarcode->SetTextSize( m_textSize.GetIntValue() );

    // Orientation
    EDA_ANGLE oldAngle = aBarcode->GetAngle();
    EDA_ANGLE newAngle = m_orientation.GetAngleValue();

    if( newAngle != oldAngle )
        aBarcode->Rotate( aBarcode->GetPosition(), newAngle - oldAngle );

    // Knockout
    aBarcode->SetIsKnockout( m_cbKnockout->GetValue() );
    aBarcode->SetMargin( VECTOR2I( m_knockoutMarginX.GetIntValue(), m_knockoutMarginY.GetIntValue() ) );

    // Show text
    aBarcode->Text().SetVisible( m_cbShowText->GetValue() );

    // Barcode kind
    switch( m_barcode->GetSelection() )
    {
    case 0:  aBarcode->SetKind( BARCODE_T::CODE_39 );       break;
    case 1:  aBarcode->SetKind( BARCODE_T::CODE_128 );      break;
    case 2:  aBarcode->SetKind( BARCODE_T::DATA_MATRIX );   break;
    case 3:  aBarcode->SetKind( BARCODE_T::QR_CODE );       break;
    case 4:  aBarcode->SetKind( BARCODE_T::MICRO_QR_CODE ); break;
    default: aBarcode->SetKind( BARCODE_T::QR_CODE );       break;
    }

    switch( m_errorCorrection->GetSelection() )
    {
    case 0:  aBarcode->SetErrorCorrection( BARCODE_ECC_T::L ); break;
    case 1:  aBarcode->SetErrorCorrection( BARCODE_ECC_T::M ); break;
    case 2:  aBarcode->SetErrorCorrection( BARCODE_ECC_T::Q ); break;
    case 3:  aBarcode->SetErrorCorrection( BARCODE_ECC_T::H ); break;
    default: aBarcode->SetErrorCorrection( BARCODE_ECC_T::L ); break;
    }

    aBarcode->AssembleBarcode();

    return true;
}


void DIALOG_BARCODE_PROPERTIES::refreshPreview()
{
    KIGFX::VIEW* view = m_panelShowBarcodeGal->GetView();

    // Compute the polygon bbox for the current dummy barcode (symbol + text/knockout as applicable)
    const SHAPE_POLY_SET& poly = m_dummyBarcode->GetPolyShape();

    if( poly.OutlineCount() > 0 )
    {
        BOX2I bbI = poly.BBox();

        // Autozoom
        view->SetViewport( BOX2D( bbI.GetOrigin(), bbI.GetSize() ) );

        // Add a margin
        view->SetScale( view->GetScale() * 0.7 );
    }

    view->SetCenter( VECTOR2D( m_dummyBarcode->GetPosition() ) );
    view->Update( m_dummyBarcode );
    m_panelShowBarcodeGal->Refresh();
}


void DIALOG_BARCODE_PROPERTIES::OnValuesChanged( wxCommandEvent& event )
{
    if( transferDataToBarcode( m_dummyBarcode ) )
    {
        refreshPreview();
        OnModify();
    }
}


void DIALOG_BARCODE_PROPERTIES::OnTextValueChanged( wxKeyEvent& event )
{
    if( transferDataToBarcode( m_dummyBarcode ) )
    {
        refreshPreview();
        OnModify();
    }
}


void PCB_BASE_EDIT_FRAME::ShowBarcodePropertiesDialog( PCB_BARCODE* aBarcode )
{
    DIALOG_BARCODE_PROPERTIES dlg( this, aBarcode );
    dlg.ShowModal();
}
