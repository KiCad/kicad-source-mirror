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

#include <widgets/indicator_icon.h>
#include <wx/event.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>

INDICATOR_ICON::INDICATOR_ICON( wxWindow* aParent, ICON_PROVIDER& aIconProvider,
                                ICON_ID aInitialIcon, int aID ):
        wxPanel( aParent, aID ),
        m_iconProvider( aIconProvider ),
        m_currentId( aInitialIcon )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( sizer );

    const wxBitmap& icon = m_iconProvider.GetIndicatorIcon( m_currentId );

    m_bitmap = new wxStaticBitmap( this, aID, icon, wxDefaultPosition, icon.GetSize() );

    sizer->Add( m_bitmap, 0, 0 );

    auto evtSkipper = [this] ( wxEvent& aEvent )
                      {
                          wxPostEvent( this, aEvent );
                      };

    m_bitmap->Bind( wxEVT_LEFT_DOWN, evtSkipper );
}


void INDICATOR_ICON::SetIndicatorState( ICON_ID aIconId )
{
    if( aIconId == m_currentId )
        return;

    m_currentId = aIconId;

    const wxBitmap& icon = m_iconProvider.GetIndicatorIcon( m_currentId );
    m_bitmap->SetBitmap( icon );
    m_bitmap->SetSize( icon.GetSize() );
}


INDICATOR_ICON::ICON_ID INDICATOR_ICON::GetIndicatorState() const
{
    return m_currentId;
}


wxImage createBlankImage( int size )
{
    wxImage image( size, size );

    image.InitAlpha();
    for( int y = 0; y < size; ++y )
        for( int x = 0; x < size; ++x )
            image.SetAlpha( x, y, wxIMAGE_ALPHA_TRANSPARENT );

#ifdef __WXMSW__
    // wxWidgets on Windows chokes on an empty fully transparent bitmap and draws it
    // as a black box
    image.SetRGB( size / 2, size / 2, 128, 128, 128 );
    image.SetAlpha( size / 2, size / 2, 10 );
#endif

    return image;
}


// Create an arrow icon of a particular size, colour and direction.  0 points up, 1 points
// right, and so forth.
wxBitmap createArrow( int size, int aDirection, wxColour aColour )
{
    wxImage image = createBlankImage( size );

    int startX = size / 2 - 1;
    int len = 1;

    int startY = aDirection % 2;

    for( int y = startY; y < startY + ( size / 2 ); ++y )
    {
        for( int x = startX; x < startX + len; ++x )
        {
            image.SetRGB( x, y, aColour.Red(), aColour.Green(), aColour.Blue() );
            image.SetAlpha( x, y, wxIMAGE_ALPHA_OPAQUE );
        }

        // Next row will start one pixel back and be two pixels longer
        startX -= 1;
        len += 2;
    }

    for( int i = 0; i < aDirection; ++i )
        image = image.Rotate90();

    return wxBitmap( image );
}


// Create a diamond icon of a particular size and colour.
wxBitmap createDiamond( int size, wxColour aColour )
{
    wxImage image = createBlankImage( size );

    int startX = size / 2 - 1;
    int len = 1;

    int startY = 2;

    for( int y = startY; y < size && len > 0; ++y )
    {
        for( int x = startX; x < startX + len; ++x )
        {
            image.SetRGB( x, y, aColour.Red(), aColour.Green(), aColour.Blue() );
            image.SetAlpha( x, y, wxIMAGE_ALPHA_OPAQUE );
        }

        // Next row will start one pixel back and be two pixels longer
        if( y <  ( size / 2) - 1  )
        {
            startX -= 1;
            len += 2;
        }
        else
        {
            startX += 1;
            len -= 2;
        }
    }

    return wxBitmap( image );
}


ROW_ICON_PROVIDER::ROW_ICON_PROVIDER( int aSize )
{
    m_blankBitmap = wxBitmap( createBlankImage( aSize ) );
    m_rightArrowBitmap = createArrow( aSize, 1, wxColour( 64, 72, 255 ) );
    m_upArrowBitmap = createArrow( aSize - 2, 0, wxSystemSettings().GetColour( wxSYS_COLOUR_3DDKSHADOW ) );
    m_downArrowBitmap = createArrow( aSize - 2, 2, wxSystemSettings().GetColour( wxSYS_COLOUR_3DDKSHADOW ) );
    m_dotBitmap = createDiamond( aSize, wxColour( 128, 144, 255 ) );
}


const wxBitmap& ROW_ICON_PROVIDER::GetIndicatorIcon( INDICATOR_ICON::ICON_ID aId ) const
{
    switch( aId )
    {
    case STATE::UP:     return m_upArrowBitmap;
    case STATE::DOWN:   return m_downArrowBitmap;
    case STATE::ON:     return m_rightArrowBitmap;
    case STATE::DIMMED: return m_dotBitmap;
    case STATE::OFF:    return m_blankBitmap;
    default:            return m_blankBitmap;
    }
}
