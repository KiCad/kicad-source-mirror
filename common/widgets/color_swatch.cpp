/*
 * This program source code file is part of KiCad, a free EDA CAD application.

 * Copyright (C) 2017 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <wx/colour.h>
#include <wx/colordlg.h>

wxDEFINE_EVENT(COLOR_SWATCH_CHANGED, wxCommandEvent);

using KIGFX::COLOR4D;


const static int SWATCH_SIZE_X = 14;
const static int SWATCH_SIZE_Y = 12;

// See selcolor.cpp:
extern COLOR4D DisplayColorFrame( wxWindow* aParent, COLOR4D aOldColor );


/**
 * Make a simple color swatch bitmap
 */
static wxBitmap makeBitmap( COLOR4D aColor )
{
    wxBitmap    bitmap( SWATCH_SIZE_X, SWATCH_SIZE_Y );
    wxBrush     brush;
    wxMemoryDC  iconDC;

    iconDC.SelectObject( bitmap );

    brush.SetColour( aColor.ToColour() );
    brush.SetStyle( wxBRUSHSTYLE_SOLID );

    iconDC.SetBrush( brush );

    iconDC.DrawRectangle( 0, 0, SWATCH_SIZE_X, SWATCH_SIZE_Y );

    return bitmap;
}


/**
 * Function makeColorButton
 * creates a wxStaticBitmap and assigns it a solid color and a control ID
 */
static std::unique_ptr<wxStaticBitmap> makeColorSwatch(
        wxWindow* aParent, COLOR4D aColor, int aID )
{
    // construct a bitmap of the right color and make the swatch from it
    wxBitmap bitmap = makeBitmap( aColor );
    auto ret = std::make_unique<wxStaticBitmap>( aParent, aID, bitmap );

    return ret;
}


COLOR_SWATCH::COLOR_SWATCH( wxWindow* aParent, COLOR4D aColor, int aID,
                            bool aArbitraryColors ):
        wxPanel( aParent, aID ),
        m_arbitraryColors( aArbitraryColors ),
        m_color( aColor )
{
    auto sizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( sizer );

    auto swatch = makeColorSwatch( this, m_color, aID );
    m_swatch = swatch.get(); // hold a handle

    sizer->Add( swatch.release(), 0, 0 );

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

    wxBitmap bm = makeBitmap( aColor );
    m_swatch->SetBitmap( bm );

    if( sendEvent )
    {
        sendSwatchChangeEvent( *this );
    }
}


COLOR4D COLOR_SWATCH::GetSwatchColor() const
{
    return m_color;
}


void COLOR_SWATCH::GetNewSwatchColor()
{
    COLOR4D newColor = COLOR4D::UNSPECIFIED;

    if( m_arbitraryColors )
    {
        wxColourData colourData;
        colourData.SetColour( m_color.ToColour() );
        wxColourDialog* dialog = new wxColourDialog( this, &colourData );

        if( dialog->ShowModal() == wxID_OK )
        {
            newColor = COLOR4D( dialog->GetColourData().GetColour() );
        }
    }
    else
    {
        newColor = DisplayColorFrame( this, m_color );
    }

    if( newColor != COLOR4D::UNSPECIFIED )
    {
        m_color = newColor;

        wxBitmap bm = makeBitmap( newColor );
        m_swatch->SetBitmap( bm );

        sendSwatchChangeEvent( *this );
    }
}
