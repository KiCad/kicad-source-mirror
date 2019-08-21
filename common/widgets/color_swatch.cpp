/*
 * This program source code file is part of KiCad, a free EDA CAD application.

 * Copyright (C) 2017-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/color_swatch.h>

#include "dialog_color_picker.h"
#include <memory>

wxDEFINE_EVENT(COLOR_SWATCH_CHANGED, wxCommandEvent);

using KIGFX::COLOR4D;


const static wxSize SWATCH_SIZE_DU( 8, 6 );

// See selcolor.cpp:
extern COLOR4D DisplayColorFrame( wxWindow* aParent, COLOR4D aOldColor );


/**
 * Make a simple color swatch bitmap
 *
 * @param aWindow - window used as context for device-independent size
 */
wxBitmap COLOR_SWATCH::MakeBitmap( COLOR4D aColor, COLOR4D aBackground, wxSize aSize )
{
    wxBitmap    bitmap( aSize );
    wxBrush     brush;
    wxMemoryDC  iconDC;

    iconDC.SelectObject( bitmap );

    brush.SetStyle( wxBRUSHSTYLE_SOLID );
    brush.SetColour( aBackground.WithAlpha(1.0).ToColour() );
    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, aSize.x, aSize.y );

    brush.SetColour( aColor.ToColour() );
    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, aSize.x, aSize.y );

    return bitmap;
}


/**
 * Function makeColorButton
 * creates a wxStaticBitmap and assigns it a solid color and a control ID
 */
static std::unique_ptr<wxStaticBitmap> makeColorSwatch( wxWindow* aParent, COLOR4D aColor,
                                                        COLOR4D aBackground, int aID )
{
    static wxSize size = aParent->ConvertDialogToPixels( SWATCH_SIZE_DU );

    wxBitmap bitmap = COLOR_SWATCH::MakeBitmap( aColor, aBackground, size );
    auto ret = std::make_unique<wxStaticBitmap>( aParent, aID, bitmap );

    return ret;
}


COLOR_SWATCH::COLOR_SWATCH( wxWindow* aParent, COLOR4D aColor, int aID, COLOR4D aBackground,
        const COLOR4D aDefault ) :
        wxPanel( aParent, aID ),
        m_color( aColor ),
        m_background( aBackground ),
        m_default( aDefault )
{
    auto sizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( sizer );

    auto swatch = makeColorSwatch( this, m_color, m_background, aID );
    m_swatch = swatch.release(); // hold a handle

    sizer->Add( m_swatch, 0, 0 );

    // forward click to any other listeners, since we don't want them
    m_swatch->Bind( wxEVT_LEFT_DOWN, &COLOR_SWATCH::rePostEvent, this );
    m_swatch->Bind( wxEVT_RIGHT_DOWN, &COLOR_SWATCH::rePostEvent, this );

    // bind the events that trigger the dialog
    m_swatch->Bind( wxEVT_LEFT_DCLICK, [this] ( wxMouseEvent& aEvt ) {
        GetNewSwatchColor();
    } );

    m_swatch->Bind( wxEVT_MIDDLE_DOWN, [this] ( wxMouseEvent& aEvt ) {
        GetNewSwatchColor();
    } );
}


void COLOR_SWATCH::rePostEvent( wxEvent& aEvt )
{
    wxPostEvent( this, aEvt );
}


static void sendSwatchChangeEvent( COLOR_SWATCH& aSender )
{
    wxCommandEvent changeEvt( COLOR_SWATCH_CHANGED );

    // use this class as the object (alternative might be to
    // set a custom event class but that's more work)
    changeEvt.SetEventObject( &aSender );

    wxPostEvent( &aSender, changeEvt );
}


void COLOR_SWATCH::SetSwatchColor( COLOR4D aColor, bool sendEvent )
{
    m_color = aColor;

    wxBitmap bm = MakeBitmap( m_color, m_background, ConvertDialogToPixels( SWATCH_SIZE_DU ) );
    m_swatch->SetBitmap( bm );

    if( sendEvent )
    {
        sendSwatchChangeEvent( *this );
    }
}


void COLOR_SWATCH::SetSwatchBackground( COLOR4D aBackground )
{
    m_background = aBackground;
    wxBitmap bm = MakeBitmap( m_color, m_background, ConvertDialogToPixels( SWATCH_SIZE_DU ) );
    m_swatch->SetBitmap( bm );
}


COLOR4D COLOR_SWATCH::GetSwatchColor() const
{
    return m_color;
}


void COLOR_SWATCH::GetNewSwatchColor()
{
    COLOR4D newColor = COLOR4D::UNSPECIFIED;

    DIALOG_COLOR_PICKER dialog( ::wxGetTopLevelParent( this ), m_color, true, nullptr, m_default );

    if( dialog.ShowModal() == wxID_OK )
        newColor = dialog.GetColor();

    if( newColor != COLOR4D::UNSPECIFIED )
    {
        m_color = newColor;

        wxBitmap bm = MakeBitmap( newColor, m_background, ConvertDialogToPixels( SWATCH_SIZE_DU ) );
        m_swatch->SetBitmap( bm );

        sendSwatchChangeEvent( *this );
    }
}
