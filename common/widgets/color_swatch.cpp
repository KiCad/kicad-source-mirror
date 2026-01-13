/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <kiplatform/ui.h>
#include <widgets/color_swatch.h>
#include <wx/dcmemory.h>

#include <dpi_scaling_common.h>
#include <dialogs/dialog_color_picker.h>
#include <memory>

wxDEFINE_EVENT( COLOR_SWATCH_CHANGED, wxCommandEvent );

using KIGFX::COLOR4D;


wxBitmap COLOR_SWATCH::MakeBitmap( const COLOR4D& aColor, const COLOR4D& aBackground, const wxSize& aSize,
                                   const wxSize& aCheckerboardSize, const COLOR4D& aCheckerboardBackground,
                                   const std::vector<int>& aMargins )
{
    wxBitmap   bitmap( aSize );
    wxMemoryDC iconDC;

    iconDC.SelectObject( bitmap );

    RenderToDC( &iconDC, aColor, aBackground, aSize, aCheckerboardSize, aCheckerboardBackground, aMargins );

    return bitmap;
}


wxBitmap COLOR_SWATCH::makeBitmap()
{
    wxBitmap bitmap = COLOR_SWATCH::MakeBitmap( m_color, m_background, ToPhys( m_size ),
                                                ToPhys( m_checkerboardSize ), m_checkerboardBg );

    bitmap.SetScaleFactor( GetDPIScaleFactor() );
    return bitmap;
}


void COLOR_SWATCH::RenderToDC( wxDC* aDC, const KIGFX::COLOR4D& aColor, const KIGFX::COLOR4D& aBackground,
                               const wxRect& aRect, const wxSize& aCheckerboardSize,
                               const KIGFX::COLOR4D& aCheckerboardBackground, const std::vector<int>& aMargins )
{
    wxColor fg = aColor.m_text.has_value() ? COLOR4D::UNSPECIFIED.ToColour() : aColor.ToColour();

    wxBrush brush;
    brush.SetStyle( wxBRUSHSTYLE_SOLID );

    aDC->SetPen( *wxTRANSPARENT_PEN );

    // Draw a checkerboard
    COLOR4D white;
    COLOR4D black;
    bool    rowCycle;

    if( aColor.m_text.has_value() || aColor == COLOR4D::UNSPECIFIED )
    {
        if( aCheckerboardBackground.GetBrightness() > 0.4 )
        {
            white = COLOR4D::WHITE;
            black = white.Darkened( 0.15 );
            rowCycle = true;
        }
        else
        {
            black = COLOR4D::BLACK;
            white = black.Brightened( 0.15 );
            rowCycle = false;
        }
    }
    else
    {
        if( aBackground.GetBrightness() > 0.4 )
        {
            white = aBackground;
            black = white.Darkened( 0.15 );
            rowCycle = true;
        }
        else
        {
            black = COLOR4D::BLACK;
            white = black.Brightened( 0.15 );
            rowCycle = false;
        }
    }

    for( int x = aRect.GetLeft(); x <= aRect.GetRight(); x += aCheckerboardSize.x )
    {
        bool colCycle = rowCycle;

        for( int y = aRect.GetTop(); y <= aRect.GetBottom(); y += aCheckerboardSize.y )
        {
            wxColor bg = colCycle ? black.ToColour() : white.ToColour();

            // Blend fg bg with the checkerboard
            double alpha = (double)fg.Alpha() / 255.0;
            unsigned char r = wxColor::AlphaBlend( fg.Red(), bg.Red(), alpha );
            unsigned char g = wxColor::AlphaBlend( fg.Green(), bg.Green(), alpha );
            unsigned char b = wxColor::AlphaBlend( fg.Blue(), bg.Blue(), alpha );

            brush.SetColour( r, g, b );

            aDC->SetBrush( brush );
            aDC->DrawRectangle( x, y, aCheckerboardSize.x, aCheckerboardSize.y );

            colCycle = !colCycle;
        }

        rowCycle = !rowCycle;
    }

    aDC->SetBrush( *wxWHITE_BRUSH );

    if( aMargins[0] )
        aDC->DrawRectangle( 0, 0, aMargins[0], aRect.GetHeight() );

    if( aMargins[1] )
        aDC->DrawRectangle( 0, 0, aRect.GetWidth(), aMargins[1] );

    if( aMargins[2] )
        aDC->DrawRectangle( aRect.GetWidth() - aMargins[2], 0, aMargins[2], aRect.GetHeight() );

    if( aMargins[3] )
        aDC->DrawRectangle( 0, aRect.GetHeight() - aMargins[3], aRect.GetWidth(), aMargins[3] );
}


COLOR_SWATCH::COLOR_SWATCH( wxWindow* aParent, const COLOR4D& aColor, int aID, const COLOR4D& aBackground,
                            const COLOR4D& aDefault, SWATCH_SIZE aSwatchSize, bool aTriggerWithSingleClick ) :
        wxPanel( aParent, aID ),
        m_color( aColor ),
        m_background( aBackground ),
        m_default( aDefault ),
        m_userColors( nullptr ),
        m_readOnly( false ),
        m_supportsOpacity( true )
{
    wxASSERT_MSG( aSwatchSize != SWATCH_EXPAND,
                  wxS( "SWATCH_EXPAND not supported in COLOR_SWATCH" ) );

    switch( aSwatchSize )
    {
    case SWATCH_MEDIUM: m_size = ConvertDialogToPixels( SWATCH_SIZE_MEDIUM_DU ); break;
    case SWATCH_SMALL:  m_size = ConvertDialogToPixels( SWATCH_SIZE_SMALL_DU );  break;
    case SWATCH_LARGE:  m_size = ConvertDialogToPixels( SWATCH_SIZE_LARGE_DU );  break;
    case SWATCH_EXPAND: m_size = ConvertDialogToPixels( SWATCH_SIZE_LARGE_DU );  break;
    }

    m_checkerboardSize = ConvertDialogToPixels( CHECKERBOARD_SIZE_DU );
    m_checkerboardBg = aParent->GetBackgroundColour();

    auto sizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( sizer );

    m_swatch = new wxStaticBitmap( this, aID, makeBitmap() );

    sizer->Add( m_swatch, 0, 0 );

    setupEvents( aTriggerWithSingleClick );
}


COLOR_SWATCH::COLOR_SWATCH( wxWindow* aParent, wxWindowID aID, const wxPoint& aPos, const wxSize& aSize,
                            long aStyle ) :
        wxPanel( aParent, aID, aPos, aSize, aStyle ),
        m_userColors( nullptr ),
        m_readOnly( false ),
        m_supportsOpacity( true )
{
    if( aSize == wxDefaultSize )
        m_size = ConvertDialogToPixels( SWATCH_SIZE_MEDIUM_DU );
    else
        m_size = aSize;

    m_checkerboardSize = ConvertDialogToPixels( CHECKERBOARD_SIZE_DU );
    m_checkerboardBg = aParent->GetBackgroundColour();

#ifdef __WXMAC__
    // Adjust for border
    m_size.x -= 2;
    m_size.y -= 2;
#endif

    SetSize( m_size );

    auto sizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( sizer );

    m_swatch = new wxStaticBitmap( this, aID, makeBitmap() );

    sizer->Add( m_swatch, 0, 0 );

    setupEvents( false );
}


void COLOR_SWATCH::setupEvents( bool aTriggerWithSingleClick )
{
    if( dynamic_cast<DIALOG_SHIM*>( wxGetTopLevelParent( this ) ) )
    {
        m_swatch->Bind( wxEVT_LEFT_DOWN, &COLOR_SWATCH::onMouseEvent, this );
    }
    else
    {
        // forward click to any other listeners, since we don't want them
        m_swatch->Bind( wxEVT_LEFT_DOWN, &COLOR_SWATCH::rePostEvent, this );

        // bind the events that trigger the dialog
        m_swatch->Bind( wxEVT_LEFT_DCLICK, &COLOR_SWATCH::onMouseEvent, this );

        if( aTriggerWithSingleClick )
        {
            m_swatch->Bind( wxEVT_LEFT_UP, &COLOR_SWATCH::onMouseEvent, this );
        }
    }

    m_swatch->Bind( wxEVT_MIDDLE_DOWN, &COLOR_SWATCH::onMouseEvent, this );

    m_swatch->Bind( wxEVT_RIGHT_DOWN, &COLOR_SWATCH::rePostEvent, this );
}


void COLOR_SWATCH::rePostEvent( wxEvent& aEvent )
{
    wxPostEvent( this, aEvent );
}


void COLOR_SWATCH::onMouseEvent( wxEvent& )
{
    GetNewSwatchColor();
}


static void sendSwatchChangeEvent( COLOR_SWATCH& aSender )
{
    wxCommandEvent changeEvt( COLOR_SWATCH_CHANGED, aSender.GetId() );

    // use this class as the object (alternative might be to
    // set a custom event class but that's more work)
    changeEvt.SetEventObject( &aSender );

    wxPostEvent( &aSender, changeEvt );
}


void COLOR_SWATCH::SetSwatchColor( const COLOR4D& aColor, bool aSendEvent )
{
    m_color = aColor;

    m_swatch->SetBitmap( makeBitmap() );

    if( aSendEvent )
        sendSwatchChangeEvent( *this );
}


void COLOR_SWATCH::SetDefaultColor( const COLOR4D& aColor )
{
    m_default = aColor;
}


void COLOR_SWATCH::SetSwatchBackground( const COLOR4D& aBackground )
{
    m_background = aBackground;

    m_swatch->SetBitmap( makeBitmap() );
}


COLOR4D COLOR_SWATCH::GetSwatchColor() const
{
    return m_color;
}


void COLOR_SWATCH::GetNewSwatchColor()
{
    if( m_readOnly )
    {
        if( m_readOnlyCallback )
            m_readOnlyCallback();

        return;
    }

    DIALOG_COLOR_PICKER dialog( ::wxGetTopLevelParent( this ), m_color, m_supportsOpacity, m_userColors, m_default );

    if( dialog.ShowModal() == wxID_OK )
    {
        COLOR4D newColor = dialog.GetColor();

        if( newColor != COLOR4D::UNSPECIFIED || m_default == COLOR4D::UNSPECIFIED )
        {
            m_color = newColor;

            m_swatch->SetBitmap( makeBitmap() );

            sendSwatchChangeEvent( *this );
        }
    }
}


void COLOR_SWATCH::OnDarkModeToggle()
{
    wxWindow* parent = GetParent();
    m_checkerboardBg = parent ? parent->GetBackgroundColour() : wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );

    if( m_swatch )
        m_swatch->SetBitmap( makeBitmap() );
}
