/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/dcmemory.h>

#include <gal/dpi_scaling.h>
#include <dialogs/dialog_color_picker.h>
#include <memory>

wxDEFINE_EVENT( COLOR_SWATCH_CHANGED, wxCommandEvent );

using KIGFX::COLOR4D;


// See selcolor.cpp:
extern COLOR4D DisplayColorFrame( wxWindow* aParent, COLOR4D aOldColor );


wxBitmap COLOR_SWATCH::MakeBitmap( const COLOR4D& aColor, const COLOR4D& aBackground,
                                   const wxSize& aSize, const wxSize& aCheckerboardSize,
                                   const COLOR4D& aCheckerboardBackground )
{
    wxBitmap   bitmap( aSize );
    wxMemoryDC iconDC;

    iconDC.SelectObject( bitmap );

    RenderToDC( &iconDC, aColor, aBackground, aSize, aCheckerboardSize, aCheckerboardBackground );

    return bitmap;
}


void COLOR_SWATCH::RenderToDC( wxDC* aDC, const KIGFX::COLOR4D& aColor,
                               const KIGFX::COLOR4D& aBackground, const wxRect& aRect,
                               const wxSize&         aCheckerboardSize,
                               const KIGFX::COLOR4D& aCheckerboardBackground )
{
    wxBrush     brush;
    wxPen       pen;

    brush.SetStyle( wxBRUSHSTYLE_SOLID );

    if( aColor == COLOR4D::UNSPECIFIED )
    {
        // Draw a checkerboard
        COLOR4D white;
        COLOR4D black;
        bool    rowCycle;

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

        for( int x = aRect.GetLeft(); x < aRect.GetRight(); x += aCheckerboardSize.x )
        {
            bool colCycle = rowCycle;

            for( int y = aRect.GetTop(); y < aRect.GetBottom(); y += aCheckerboardSize.y )
            {
                COLOR4D color = colCycle ? black : white;
                brush.SetColour( color.ToColour() );
                pen.SetColour( color.ToColour() );

                aDC->SetBrush( brush );
                aDC->SetPen( pen );
                aDC->DrawRectangle( x, y, aCheckerboardSize.x, aCheckerboardSize.y );

                colCycle = !colCycle;
            }

            rowCycle = !rowCycle;
        }
    }
    else
    {
        brush.SetColour( aBackground.WithAlpha(1.0).ToColour() );
        pen.SetColour( aBackground.WithAlpha(1.0).ToColour() );

        aDC->SetBrush( brush );
        aDC->SetPen( pen );
        aDC->DrawRectangle( aRect );

        brush.SetColour( aColor.ToColour() );
        pen.SetColour( aColor.ToColour() );

        aDC->SetBrush( brush );
        aDC->SetPen( pen );
        aDC->DrawRectangle( aRect );
    }
}


COLOR_SWATCH::COLOR_SWATCH( wxWindow* aParent, const COLOR4D& aColor, int aID,
                            const COLOR4D& aBackground, const COLOR4D& aDefault,
                            SWATCH_SIZE aSwatchSize, bool aTriggerWithSingleClick ) :
        wxPanel( aParent, aID ),
        m_color( aColor ),
        m_background( aBackground ),
        m_default( aDefault ),
        m_userColors( nullptr ),
        m_readOnly( false ),
        m_supportsOpacity( true )
{
    wxASSERT_MSG( aSwatchSize != SWATCH_EXPAND, wxS( "SWATCH_EXPAND not supported in COLOR_SWATCH" ) );

    switch( aSwatchSize )
    {
    case SWATCH_MEDIUM: m_size = ConvertDialogToPixels( SWATCH_SIZE_MEDIUM_DU ); break;
    case SWATCH_SMALL:  m_size = ConvertDialogToPixels( SWATCH_SIZE_SMALL_DU );  break;
    case SWATCH_LARGE:  m_size = ConvertDialogToPixels( SWATCH_SIZE_LARGE_DU );  break;
    case SWATCH_EXPAND: m_size = ConvertDialogToPixels( SWATCH_SIZE_LARGE_DU );  break;
    }

    m_checkerboardSize = ConvertDialogToPixels( CHECKERBOARD_SIZE_DU );
    m_checkerboardBg = aParent->GetBackgroundColour();

#ifdef __WXMSW__
    // These need additional scaling on Windows because of some discrepancy between pixel and
    // content scaling that only affects certain widgets on Windows HiDPI.  On other platforms, the
    // value returned by ConvertDialogToPixels appears to be correct.
    DPI_SCALING dpi( nullptr, aParent );

    m_size /= dpi.GetContentScaleFactor();
    m_checkerboardSize /= dpi.GetContentScaleFactor();
#endif

    auto sizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( sizer );

    wxBitmap bitmap = COLOR_SWATCH::MakeBitmap( aColor, aBackground, m_size,
                                                m_checkerboardSize, m_checkerboardBg );
    m_swatch = new wxStaticBitmap( this, aID, bitmap );

    sizer->Add( m_swatch, 0, 0 );

    setupEvents( aTriggerWithSingleClick );
}


COLOR_SWATCH::COLOR_SWATCH( wxWindow* aParent, wxWindowID aID, const wxPoint& aPos,
                            const wxSize& aSize, long aStyle ) :
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

    SetSize( m_size );

    auto sizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( sizer );

    wxBitmap bitmap = COLOR_SWATCH::MakeBitmap( COLOR4D::UNSPECIFIED, COLOR4D::UNSPECIFIED,
                                                m_size, m_checkerboardSize, m_checkerboardBg );
    m_swatch = new wxStaticBitmap( this, aID, bitmap );

    sizer->Add( m_swatch, 0, 0 );

    setupEvents( false );
}


void COLOR_SWATCH::setupEvents( bool aTriggerWithSingleClick )
{
    wxWindow* topLevelParent = GetParent();

    while( topLevelParent && !topLevelParent->IsTopLevel() )
        topLevelParent = topLevelParent->GetParent();

    if( topLevelParent && dynamic_cast<DIALOG_SHIM*>( topLevelParent ) )
    {
        m_swatch->Bind( wxEVT_LEFT_DOWN,
                        [this] ( wxMouseEvent& aEvt )
                        {
                            GetNewSwatchColor();
                        } );
    }
    else
    {
        // forward click to any other listeners, since we don't want them
        m_swatch->Bind( wxEVT_LEFT_DOWN, &COLOR_SWATCH::rePostEvent, this );

        // bind the events that trigger the dialog
        m_swatch->Bind( wxEVT_LEFT_DCLICK,
                        [this] ( wxMouseEvent& aEvt )
                        {
                            GetNewSwatchColor();
                        } );

        if( aTriggerWithSingleClick )
        {
            m_swatch->Bind( wxEVT_LEFT_UP,
                        [this] ( wxMouseEvent& aEvt )
                        {
                            GetNewSwatchColor();
                        } );
        }
    }

    m_swatch->Bind( wxEVT_MIDDLE_DOWN,
                    [this] ( wxMouseEvent& aEvt )
                    {
                        GetNewSwatchColor();
                    } );

    m_swatch->Bind( wxEVT_RIGHT_DOWN, &COLOR_SWATCH::rePostEvent, this );
}


void COLOR_SWATCH::rePostEvent( wxEvent& aEvent )
{
    wxPostEvent( this, aEvent );
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

    wxBitmap bm = MakeBitmap( m_color, m_background, m_size, m_checkerboardSize, m_checkerboardBg );
    m_swatch->SetBitmap( bm );

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
    wxBitmap bm = MakeBitmap( m_color, m_background, m_size, m_checkerboardSize, m_checkerboardBg );
    m_swatch->SetBitmap( bm );
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

    DIALOG_COLOR_PICKER dialog( ::wxGetTopLevelParent( this ), m_color, m_supportsOpacity,
                                m_userColors, m_default );

    if( dialog.ShowModal() == wxID_OK )
    {
        COLOR4D newColor = dialog.GetColor();

        if( newColor != COLOR4D::UNSPECIFIED || m_default == COLOR4D::UNSPECIFIED )
        {
            m_color = newColor;

            wxBitmap bm = MakeBitmap( newColor, m_background, m_size, m_checkerboardSize,
                                      m_checkerboardBg );
            m_swatch->SetBitmap( bm );

            sendSwatchChangeEvent( *this );
        }
    }
}


void COLOR_SWATCH::OnDarkModeToggle()
{
    m_checkerboardBg = m_parent->GetBackgroundColour();
    wxBitmap bm = MakeBitmap( m_color, m_background, m_size, m_checkerboardSize, m_checkerboardBg );
    m_swatch->SetBitmap( bm );
}
