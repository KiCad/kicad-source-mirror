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

#include <widgets/indicator_icon.h>


INDICATOR_ICON::INDICATOR_ICON( wxWindow* aParent,
                              ICON_PROVIDER& aIconProvider,
                              ICON_ID aInitialIcon, int aID ):
        wxPanel( aParent, aID ),
        m_iconProvider( aIconProvider ),
        m_currentId( aInitialIcon )
{
    auto sizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( sizer );

    const wxBitmap& initBitmap = m_iconProvider.GetIndicatorIcon( m_currentId );

    m_bitmap = new wxStaticBitmap( this, aID,
                                   initBitmap, wxDefaultPosition,
                                   initBitmap.GetSize() );

    sizer->Add( m_bitmap, 0, 0 );

    auto evtSkipper = [this] ( wxEvent& aEvent ) {
        wxPostEvent( this, aEvent );
    };

    m_bitmap->Bind( wxEVT_LEFT_DOWN, evtSkipper );
}


void INDICATOR_ICON::SetIndicatorState( ICON_ID aIconId )
{
    if( aIconId == m_currentId )
        return;

    m_currentId = aIconId;

    m_bitmap->SetBitmap( m_iconProvider.GetIndicatorIcon( m_currentId ) );
}


INDICATOR_ICON::ICON_ID INDICATOR_ICON::GetIndicatorState() const
{
    return m_currentId;
}

// ====================================================================
// Common icon providers

/* XPM
 * This bitmap is used for not selected layers
 */
static const char * clear_xpm[] = {
"10 14 1 1",
" 	c None",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          "};

/* XPM
 * This bitmap can be used to show a not selected layer
 * with special property (mainly not selected layers not in use in GerbView)
 */
static const char * clear_alternate_xpm[] = {
"10 14 4 1",
"       c None",
"X      c #008080",
"o      c GREEN",
"O      c #00B080",
"          ",
"          ",
"          ",
"          ",
"    X     ",
"   XXX    ",
"  XXXXX   ",
" OOOOOOO  ",
"  ooooo   ",
"   ooo    ",
"    o     ",
"          ",
"          ",
"          "};


/* XPM
 * This bitmap  is used for a normale selected layer
 */
static const char * rightarrow_xpm[] = {
"10 14 4 1",
"       c None",
"X      c #8080ff",
"o      c BLUE",
"O      c gray56",
"  X       ",
"  XX      ",
"  XXX     ",
"  XXXX    ",
"  XXXXX   ",
"  XXXXXX  ",
"  XXXXXXX ",
"  oooooooO",
"  ooooooO ",
"  oooooO  ",
"  ooooO   ",
"  oooO    ",
"  ooO     ",
"  oO      "};

/* XPM
 * This bitmap can be used to show the selected layer
 * with special property (mainly a layer in use in GerbView)
 */
static const char * rightarrow_alternate_xpm[] = {
"10 14 5 1",
"       c None",
".      c #00B000",
"X      c #8080ff",
"o      c BLUE",
"O      c gray56",
"..X       ",
"..XX      ",
"..XXX     ",
"..XXXX    ",
"..XXXXX   ",
"..XXXXXX  ",
"..XXXXXXX ",
"..oooooooO",
"..ooooooO ",
"..oooooO  ",
"..ooooO   ",
"..oooO    ",
"..ooO     ",
"..oO      "};


ROW_ICON_PROVIDER::ROW_ICON_PROVIDER( bool aAlt ):
        m_alt( aAlt )
{}


const wxBitmap& ROW_ICON_PROVIDER::GetIndicatorIcon(
            INDICATOR_ICON::ICON_ID aIconId ) const
{
    // need to wait until UI is ready before construction
    // so can't go in the global scope
    static const wxBitmap rightArrowBitmap( rightarrow_xpm );
    static const wxBitmap rightArrowAlternateBitmap( rightarrow_alternate_xpm );
    static const wxBitmap blankBitmap( clear_xpm );
    static const wxBitmap blankAlternateBitmap( clear_alternate_xpm );

    const bool on = ( aIconId == STATE::ON );

    if( m_alt )
        return ( on ? rightArrowAlternateBitmap : blankAlternateBitmap );

    return ( on ? rightArrowBitmap : blankBitmap );
}
