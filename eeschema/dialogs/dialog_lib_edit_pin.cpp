/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <base_units.h>

#include <libeditframe.h>
#include <class_libentry.h>
#include <lib_pin.h>

#include <dialog_lib_edit_pin.h>

DIALOG_LIB_EDIT_PIN::DIALOG_LIB_EDIT_PIN( EDA_DRAW_FRAME* parent, LIB_PIN* aPin ) :
    DIALOG_LIB_EDIT_PIN_BASE( parent )
{
    // Creates a dummy pin to show on a panel, inside this dialog:
    m_dummyPin = new LIB_PIN( *aPin );

    // m_dummyPin changes do not propagate to other pins of the current lib component,
    // so set parent to null and clear flags
    m_dummyPin->SetParent( NULL );
    m_dummyPin->ClearFlags();

    m_panelShowPin->SetBackgroundColour( MakeColour( parent->GetDrawBgColor() ) );

    // Set tab order
    m_textPadName->MoveAfterInTabOrder(m_textPinName);
    m_sdbSizerButtonsOK->SetDefault();

    GetSizer()->SetSizeHints( this );

    // On some windows manager (Unity, XFCE), this dialog is
    // not always raised, depending on this dialog is run.
    // Force it to be raised
    Raise();
}


DIALOG_LIB_EDIT_PIN::~DIALOG_LIB_EDIT_PIN()
{
    delete m_dummyPin;
}


/*
 * Draw (on m_panelShowPin) the pin currently edited
 * accroding to current settings in dialog
 */
void DIALOG_LIB_EDIT_PIN::OnPaintShowPanel( wxPaintEvent& event )
{
    wxPaintDC    dc( m_panelShowPin );
    wxSize dc_size = dc.GetSize();
    dc.SetDeviceOrigin( dc_size.x / 2, dc_size.y / 2 );

    // Give a parent to m_dummyPin only from draw purpose.
    // In fact m_dummyPin should not have a parent, but draw functions need a parent
    // to know some options, about pin texts
    LIB_EDIT_FRAME* libframe = (LIB_EDIT_FRAME*) GetParent();
    m_dummyPin->SetParent( libframe->GetCurPart() );

    // Calculate a suitable scale to fit the available draw area
    EDA_RECT bBox = m_dummyPin->GetBoundingBox();
    double xscale    = (double) dc_size.x / bBox.GetWidth();
    double yscale = (double) dc_size.y / bBox.GetHeight();
    double scale = std::min( xscale, yscale );

    // Give a 10% margin
    scale *= 0.9;
    dc.SetUserScale( scale, scale );

    wxPoint offset = -bBox.Centre();

    GRResetPenAndBrush( &dc );
    bool drawpinTexts = true;   // this is a dummy param. We use its reference
                                // as non null value for m_dummyPin->Draw
    m_dummyPin->Draw( NULL, &dc, offset, UNSPECIFIED_COLOR, GR_COPY,
                      &drawpinTexts, DefaultTransform );

    m_dummyPin->SetParent(NULL);

    event.Skip();
}

void DIALOG_LIB_EDIT_PIN::OnCloseDialog( wxCloseEvent& event )
{
    EndModal( wxID_CANCEL );
}

void DIALOG_LIB_EDIT_PIN::OnCancelButtonClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}

void DIALOG_LIB_EDIT_PIN::OnOKButtonClick( wxCommandEvent& event )
{
    EndModal( wxID_OK );
}

// Called when a pin properties changes
void DIALOG_LIB_EDIT_PIN::OnPropertiesChange( wxCommandEvent& event )
{
    if( ! IsShown() )   // do nothing at init time
        return;

    int pinNameSize = ValueFromString( g_UserUnit, GetPinNameTextSize() );
    int pinNumSize = ValueFromString( g_UserUnit, GetPadNameTextSize());
    int pinOrient = LIB_PIN::GetOrientationCode( GetOrientation() );
    int pinLength = ValueFromString( g_UserUnit, GetLength() );
    int pinShape = LIB_PIN::GetStyleCode( GetStyle() );
    int pinType = GetElectricalType();

    m_dummyPin->SetName( GetPinName() );
    m_dummyPin->SetNameTextSize( pinNameSize );
    m_dummyPin->SetNumber( GetPadName() );
    m_dummyPin->SetNumberTextSize( pinNumSize );
    m_dummyPin->SetOrientation( pinOrient );
    m_dummyPin->SetLength( pinLength );
    m_dummyPin->SetShape( pinShape );
    m_dummyPin->SetVisible( GetVisible() );
    m_dummyPin->SetType( pinType );

    m_panelShowPin->Refresh();
}


void DIALOG_LIB_EDIT_PIN::SetOrientationList( const wxArrayString& list,
                                              const BITMAP_DEF* aBitmaps )
{
    for ( unsigned ii = 0; ii < list.GetCount(); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceOrientation->Append( list[ii] );
        else
            m_choiceOrientation->Insert( list[ii], KiBitmap( aBitmaps[ii] ), ii );
    }
}


void DIALOG_LIB_EDIT_PIN::SetElectricalTypeList( const wxArrayString& list,
                                                 const BITMAP_DEF* aBitmaps )
{
    for ( unsigned ii = 0; ii < list.GetCount(); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceElectricalType->Append( list[ii] );
        else
            m_choiceElectricalType->Insert( list[ii], KiBitmap( aBitmaps[ii] ), ii );
    }
}


void DIALOG_LIB_EDIT_PIN::SetStyleList( const wxArrayString& list, const BITMAP_DEF* aBitmaps )
{
    for ( unsigned ii = 0; ii < list.GetCount(); ii++ )
    {
        if( aBitmaps == NULL )
            m_choiceStyle->Append( list[ii] );
        else
            m_choiceStyle->Insert( list[ii], KiBitmap( aBitmaps[ii] ), ii );
    }
}
