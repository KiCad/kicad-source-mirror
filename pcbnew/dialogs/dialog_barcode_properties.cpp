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
// For BOX2D viewport checks
#include <math/box2.h>


DIALOG_BARCODE_PROPERTIES::DIALOG_BARCODE_PROPERTIES( PCB_BASE_FRAME* aParent, PCB_BARCODE* aBarcode ) :
        DIALOG_BARCODE_PROPERTIES_BASE( aParent ),
        m_parent( aParent ),
        m_orientation( aParent, m_orientationLabel, m_orientationCtrl, nullptr )
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
    delete m_axisOrigin;
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
    // Initialize the canvas to display the pad

    // Show the X and Y axis. It is usefull because pad shape can have an offset
    // or be a complex shape.
    KIGFX::COLOR4D axis_color = LIGHTBLUE;

    m_axisOrigin = new KIGFX::ORIGIN_VIEWITEM( axis_color, KIGFX::ORIGIN_VIEWITEM::CROSS, pcbIUScale.mmToIU( 0.2 ),
                                               VECTOR2D( m_dummyBarcode->GetPosition() ) );
    m_axisOrigin->SetDrawAtZero( true );

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
    view->Add( m_axisOrigin );

    m_panelShowBarcodeGal->StartDrawing();
    Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_BARCODE_PROPERTIES::OnResize ) );
}


void DIALOG_BARCODE_PROPERTIES::initValues()
{
    // Copy current barcode into our working copy
    if( m_currentBarcode )
        *m_dummyBarcode = *m_currentBarcode;

    m_dummyBarcode->AssembleBarcode( true, true );

    // Set units labels to current user units
    wxString unitsLabel = EDA_UNIT_UTILS::GetText( GetUserUnits(), EDA_DATA_TYPE::DISTANCE );
    m_posXUnits->SetLabel( unitsLabel );
    m_posYUnits->SetLabel( unitsLabel );
    m_sizeXUnits->SetLabel( unitsLabel );
    m_sizeYUnits->SetLabel( unitsLabel );
    m_staticText18->SetLabel( unitsLabel );
    m_offsetXUnits->SetLabel( unitsLabel );
    m_offsetYUnits->SetLabel( unitsLabel );

    m_orientation.SetUnits( EDA_UNITS::DEGREES );

    // Populate layer list
    m_Layer->Clear();

    for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        wxString name = m_board->GetLayerName( static_cast<PCB_LAYER_ID>( layer ) );
        if( name.IsEmpty() )
            continue;

        m_Layer->Append( name );
    }
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

    bool showText = m_cbShowText->GetValue();
    m_staticText17->Enable( showText );
    m_textCtrl8->Enable( showText );
    m_staticText18->Enable( showText );

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

    // Set text
    m_textInput->ChangeValue( m_dummyBarcode->GetText() );

    // Set layer selection
    wxString layerName = m_board->GetLayerName( m_dummyBarcode->GetLayer() );
    int      idx = m_Layer->FindString( layerName );

    if( idx != wxNOT_FOUND )
        m_Layer->SetSelection( idx );

    // Position
    VECTOR2I pos = m_dummyBarcode->GetPosition();
    m_posXCtrl->ChangeValue( m_parent->StringFromValue( pos.x ) );
    m_posYCtrl->ChangeValue( m_parent->StringFromValue( pos.y ) );

    // Size
    m_sizeXCtrl->ChangeValue( m_parent->StringFromValue( m_dummyBarcode->GetWidth() ) );
    m_sizeYCtrl->ChangeValue( m_parent->StringFromValue( m_dummyBarcode->GetHeight() ) );

    m_textCtrl8->ChangeValue( m_parent->StringFromValue( m_dummyBarcode->GetTextHeight() ) );

    // Orientation
    m_orientation.SetAngleValue( m_dummyBarcode->GetAngle() );

    // Show text option
    m_cbShowText->SetValue( m_dummyBarcode->Text().IsVisible() );

    m_inverted->SetValue( m_dummyBarcode->IsKnockout() );

    m_marginXCtrl->ChangeValue( m_parent->StringFromValue( m_dummyBarcode->GetMargin().x ) );
    m_marginYCtrl->ChangeValue( m_parent->StringFromValue( m_dummyBarcode->GetMargin().y ) );

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

    // Layer
    wxString     layerName = m_Layer->GetStringSelection();
    PCB_LAYER_ID layer = m_board->GetLayerID( layerName );
    aBarcode->SetLayer( layer );

    // Position
    int posX = m_parent->ValueFromString( m_posXCtrl->GetValue() );
    int posY = m_parent->ValueFromString( m_posYCtrl->GetValue() );
    aBarcode->SetPosition( VECTOR2I( posX, posY ) );

    // Size
    int width = m_parent->ValueFromString( m_sizeXCtrl->GetValue() );
    int height = m_parent->ValueFromString( m_sizeYCtrl->GetValue() );
    aBarcode->SetWidth( width );
    aBarcode->SetHeight( height );

    int textHeight = m_parent->ValueFromString( m_textCtrl8->GetValue() );
    aBarcode->SetTextHeight( textHeight );

    // Orientation
    EDA_ANGLE oldAngle = aBarcode->GetAngle();
    EDA_ANGLE newAngle = m_orientation.GetAngleValue();

    if( newAngle != oldAngle )
        aBarcode->Rotate( aBarcode->GetPosition(), newAngle - oldAngle );

    aBarcode->SetIsKnockout( m_inverted->GetValue() );

    // Margins
    int marginX = m_parent->ValueFromString( m_marginXCtrl->GetValue() );
    int marginY = m_parent->ValueFromString( m_marginYCtrl->GetValue() );
    aBarcode->SetMargin( VECTOR2I( marginX, marginY ) );

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

    aBarcode->AssembleBarcode( true, true );

    return true;
}


void DIALOG_BARCODE_PROPERTIES::OnValuesChanged( wxCommandEvent& event )
{
    if( transferDataToBarcode( m_dummyBarcode ) )
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
        OnModify();
    }
}


void PCB_BASE_EDIT_FRAME::ShowBarcodePropertiesDialog( PCB_BARCODE* aBarcode )
{
    DIALOG_BARCODE_PROPERTIES dlg( this, aBarcode );
    dlg.ShowModal();
}
