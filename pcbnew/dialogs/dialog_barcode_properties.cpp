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

#include <base_units.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <class_barcode.h>
#include <confirm.h>
#include <dialog_barcode_properties.h>
#include <pcb_base_frame.h>
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <settings/color_settings.h>
#include <tool/tool_manager.h>
#include <tools/footprint_editor_tools.h>
#include <view/view_controls.h>


DIALOG_BARCODE_PROPERTIES::DIALOG_BARCODE_PROPERTIES(
        PCB_BASE_FRAME* aParent, PCB_BARCODE* aBarcode )
        : DIALOG_BARCODE_PROPERTIES_BASE( aParent ), m_parent( aParent )
{
    m_currentBarcode = aBarcode; // aBarcode can not be NULL (no FOOTPRINT editor?)

    m_board = m_parent->GetBoard();

    m_dummyBarcode = new PCB_BARCODE( nullptr );

    initValues();

    // Usually, TransferDataToWindow is called by OnInitDialog
    // calling it here fixes all widget sizes so FinishDialogSettings can safely fix minsizes
    TransferDataToWindow();

    // Initialize canvas to be able to display the dummy pad:
    prepareCanvas();

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


DIALOG_BARCODE_PROPERTIES::~DIALOG_BARCODE_PROPERTIES()
{
    delete m_dummyBarcode;
    delete m_axisOrigin;
}


void DIALOG_BARCODE_PROPERTIES::OnInitDialog( wxInitDialogEvent& event )
{
    // Needed on some WM to be sure the pad is redrawn according to the final size
    // of the canvas, with the right zoom factor
    // TODO redraw();
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

    m_axisOrigin = new KIGFX::ORIGIN_VIEWITEM( axis_color, KIGFX::ORIGIN_VIEWITEM::CROSS,
            Millimeter2iu( 0.2 ), VECTOR2D( m_dummyBarcode->GetPosition() ) );
    m_axisOrigin->SetDrawAtZero( true );

    m_panelShowBarcodeGal->UpdateColors();
    m_panelShowBarcodeGal->SwitchBackend( m_parent->GetCanvas()->GetBackend() );
    m_panelShowBarcodeGal->SetStealsFocus( false );

    bool mousewheelPan = m_parent->GetCanvas()->GetViewControls()->IsMousewheelPanEnabled();
    m_panelShowBarcodeGal->GetViewControls()->EnableMousewheelPan( mousewheelPan );

    m_panelShowBarcodeGal->Show();

    KIGFX::VIEW* view = m_panelShowBarcodeGal->GetView();

    // fix the pad render mode (filled/not filled)
    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );
    settings->SetSketchModeGraphicItems( false );

    // gives a non null grid size (0.001mm) because GAL layer does not like a 0 size grid:
    double gridsize = 0.001 * IU_PER_MM;
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
    // TODO
}

// A small helper function, to display coordinates:
static wxString formatCoord( EDA_UNITS aUnits, wxPoint aCoord )
{
    return wxString::Format( "(X:%s Y:%s)", MessageTextFromValue( aUnits, aCoord.x, true ),
            MessageTextFromValue( aUnits, aCoord.y, true ) );
}


void DIALOG_BARCODE_PROPERTIES::OnResize( wxSizeEvent& event )
{
    // TODO
}


void DIALOG_BARCODE_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    // TODO
}


bool DIALOG_BARCODE_PROPERTIES::TransferDataToWindow()
{
    // TODO

    return true;
}


bool DIALOG_BARCODE_PROPERTIES::TransferDataFromWindow()
{
    BOARD_COMMIT commit( m_parent );

    //if( !wxDialog::TransferDataFromWindow() )
    //    return false;

    // redraw the area where the pad was
    m_parent->GetCanvas()->Refresh();

    commit.Push( _( "Modify barcode" ) );

    return true;
}


bool DIALOG_BARCODE_PROPERTIES::transferDataToBarcode( PCB_BARCODE* aPad )
{
    wxString msg;

    //if( !Validate() )
    //    return true;

    // TODO

    return false;
}


void DIALOG_BARCODE_PROPERTIES::OnValuesChanged( wxCommandEvent& event )
{
    // TODO
}
