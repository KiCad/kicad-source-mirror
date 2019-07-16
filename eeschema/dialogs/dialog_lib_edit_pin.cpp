/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016 - 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <fctsys.h>
#include <macros.h>
#include <gr_basic.h>
#include <bitmaps.h>
#include <sch_painter.h>
#include <lib_edit_frame.h>
#include <class_libentry.h>
#include <lib_pin.h>

#include <dialog_lib_edit_pin.h>
#include <confirm.h>


DIALOG_LIB_EDIT_PIN::DIALOG_LIB_EDIT_PIN( LIB_EDIT_FRAME* parent, LIB_PIN* aPin ) :
    DIALOG_LIB_EDIT_PIN_BASE( parent ),
    m_frame( parent ),
    m_pin( aPin ),
    m_posX( parent, m_posXLabel, m_posXCtrl, m_posXUnits, true ),
    m_posY( parent, m_posYLabel, m_posYCtrl, m_posYUnits, true ),
    m_pinLength( parent, m_pinLengthLabel, m_pinLengthCtrl, m_pinLengthUnits, true ),
    m_nameSize( parent, m_nameSizeLabel, m_nameSizeCtrl, m_nameSizeUnits, true ),
    m_numberSize( parent, m_numberSizeLabel, m_numberSizeCtrl, m_numberSizeUnits, true )
{
    // Creates a dummy pin to show on a panel, inside this dialog:
    m_dummyPin = new LIB_PIN( *m_pin );

    // m_dummyPin changes do not propagate to other pins of the current lib component,
    // so set parent to null and clear flags
    m_dummyPin->SetParent( nullptr );
    m_dummyPin->ClearFlags();

    COLOR4D bgColor = parent->GetRenderSettings()->GetLayerColor( LAYER_SCHEMATIC_BACKGROUND );
    m_panelShowPin->SetBackgroundColour( bgColor.ToColour() );

    const wxArrayString& orientationNames = LIB_PIN::GetOrientationNames();
    const BITMAP_DEF*    orientationBitmaps = LIB_PIN::GetOrientationSymbols();

    for ( unsigned ii = 0; ii < orientationNames.GetCount(); ii++ )
        m_choiceOrientation->Insert( orientationNames[ii], KiBitmap( orientationBitmaps[ii] ), ii );

    m_sdbSizerButtonsOK->SetDefault();
    SetInitialFocus( m_textPinName );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();

    // On some windows manager (Unity, XFCE), this dialog is
    // not always raised, depending on this dialog is run.
    // Force it to be raised
    Raise();
}


DIALOG_LIB_EDIT_PIN::~DIALOG_LIB_EDIT_PIN()
{
    delete m_dummyPin;
}


bool DIALOG_LIB_EDIT_PIN::TransferDataToWindow()
{
    if( !DIALOG_SHIM::TransferDataToWindow() )
        return false;

    m_choiceOrientation->SetSelection( LIB_PIN::GetOrientationIndex( m_pin->GetOrientation() ) );
    m_choiceStyle->SetSelection( m_pin->GetShape() );
    m_choiceElectricalType->SetSelection( m_pin->GetType() );
    m_textPinName->SetValue( m_pin->GetName() );
    m_nameSize.SetValue( m_pin->GetNameTextSize() );
    m_posX.SetValue( m_pin->GetPosition().x );
    m_posY.SetValue( -m_pin->GetPosition().y );
    m_textPinNumber->SetValue( m_pin->GetNumber() );
    m_numberSize.SetValue( m_pin->GetNumberTextSize() );
    m_pinLength.SetValue( m_pin->GetLength() );
    m_checkApplyToAllParts->SetValue( m_pin->GetUnit() == 0 );
    m_checkApplyToAllConversions->SetValue( m_pin->GetConvert() == 0 );
    m_checkShow->SetValue( m_pin->IsVisible() );

    m_dummyPin->SetVisible( m_pin->IsVisible() );

    return true;
}


bool DIALOG_LIB_EDIT_PIN::TransferDataFromWindow()
{
    if( !DIALOG_SHIM::TransferDataFromWindow() )
        return false;

    const int acceptable_mingrid = 50;

    if( ( m_posX.GetValue() % acceptable_mingrid ) || ( m_posY.GetValue() % acceptable_mingrid ) )
    {
        auto msg = wxString::Format( _( "This pin is not on a %d mils grid which will make it\n"
                                        "difficult to connect to in the schematic.\n"
                                        "Do you want to continue?" ),
                                     acceptable_mingrid );
        if( !IsOK( this, msg ) )
            return false;
    }

    if( m_pin->GetEditFlags() == 0 )
        m_frame->SaveCopyInUndoList( m_pin->GetParent() );

    m_pin->SetName( m_textPinName->GetValue() );
    m_pin->SetNumber( m_textPinNumber->GetValue() );
    m_pin->SetNameTextSize( m_nameSize.GetValue() );
    m_pin->SetNumberTextSize( m_numberSize.GetValue() );
    m_pin->SetOrientation( LIB_PIN::GetOrientationCode( m_choiceOrientation->GetSelection() ) );
    m_pin->SetLength( m_pinLength.GetValue() );
    m_pin->SetPinPosition( wxPoint( m_posX.GetValue(), -m_posY.GetValue() ) );
    m_pin->SetType( m_choiceElectricalType->GetPinTypeSelection() );
    m_pin->SetShape( m_choiceStyle->GetPinShapeSelection() );
    m_pin->SetConversion( m_checkApplyToAllConversions->GetValue() ? 0 : m_frame->GetConvert() );
    m_pin->SetPartNumber( m_checkApplyToAllParts->GetValue() ? 0 : m_frame->GetUnit() );
    m_pin->SetVisible( m_checkShow->GetValue() );

    return true;
}


/*
 * Draw (on m_panelShowPin) the pin currently edited accroding to current settings in dialog
 */
void DIALOG_LIB_EDIT_PIN::OnPaintShowPanel( wxPaintEvent& event )
{
    wxPaintDC dc( m_panelShowPin );
    wxSize    dc_size = dc.GetSize();
    dc.SetDeviceOrigin( dc_size.x / 2, dc_size.y / 2 );

    // Give a parent to m_dummyPin only from draw purpose.
    // In fact m_dummyPin should not have a parent, but draw functions need a parent
    // to know some options, about pin texts
    LIB_EDIT_FRAME* libframe = (LIB_EDIT_FRAME*) GetParent();
    m_dummyPin->SetParent( libframe->GetCurPart() );

    // Calculate a suitable scale to fit the available draw area
    EDA_RECT bBox = m_dummyPin->GetBoundingBox( true );
    double   xscale = (double) dc_size.x / bBox.GetWidth();
    double   yscale = (double) dc_size.y / bBox.GetHeight();
    double   scale = std::min( xscale, yscale );

    // Give a 10% margin and limit to no more than 100% zoom
    scale = std::min( scale * 0.9, 1.0 );
    dc.SetUserScale( scale, scale );
    GRResetPenAndBrush( &dc );

    PART_DRAW_OPTIONS opts = PART_DRAW_OPTIONS::Default();
    opts.draw_hidden_fields = true;

    m_dummyPin->Print( &dc, -bBox.Centre(), (void*) &opts, DefaultTransform );

    m_dummyPin->SetParent( nullptr );

    event.Skip();
}


void DIALOG_LIB_EDIT_PIN::OnPropertiesChange( wxCommandEvent& event )
{
    if( !IsShown() )   // do nothing at init time
        return;

    m_dummyPin->SetName( m_textPinName->GetValue() );
    m_dummyPin->SetNumber( m_textPinNumber->GetValue() );
    m_dummyPin->SetNameTextSize( m_nameSize.GetValue() );
    m_dummyPin->SetNumberTextSize( m_numberSize.GetValue() );
    m_dummyPin->SetOrientation( LIB_PIN::GetOrientationCode( m_choiceOrientation->GetSelection() ) );
    m_dummyPin->SetLength( m_pinLength.GetValue() );
    m_dummyPin->SetType( m_choiceElectricalType->GetPinTypeSelection() );
    m_dummyPin->SetShape( m_choiceStyle->GetPinShapeSelection() );
    m_dummyPin->SetVisible( m_checkShow->GetValue() );

    m_panelShowPin->Refresh();
}
