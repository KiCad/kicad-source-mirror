/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <widgets/bitmap_toggle.h>

#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/timer.h>

wxDEFINE_EVENT( TOGGLE_CHANGED, wxCommandEvent );


BITMAP_TOGGLE::BITMAP_TOGGLE( wxWindow *aParent, wxWindowID aId,
                              const wxBitmapBundle& aCheckedBitmap,
                              const wxBitmapBundle& aUncheckedBitmap, bool aChecked ) :
        wxPanel( aParent, aId ),
        m_checked( aChecked ),
        m_unchecked_bitmap( aUncheckedBitmap ),
        m_checked_bitmap( aCheckedBitmap ),
        m_debounce( 0 )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( sizer );

    const wxBitmapBundle& bundle = aChecked ? m_checked_bitmap : m_unchecked_bitmap;

    m_bitmap = new wxStaticBitmap( this, aId, bundle, wxDefaultPosition );

    sizer->Add( m_bitmap, 0, 0 );

    m_bitmap->Bind( wxEVT_LEFT_UP,
                    [&]( wxMouseEvent& event )
                    {
                        wxLongLong now = wxGetLocalTimeMillis();

                        if( now - m_debounce < 200 )
                            return;
                        else
                            m_debounce = now;

                        SetValue( !GetValue() );

                        wxCommandEvent command( TOGGLE_CHANGED );
                        command.SetInt( m_checked );
                        command.SetEventObject( this );
                        wxPostEvent( this, command );
                    } );

    auto passOnEvent =
            [&]( wxEvent& aEvent )
            {
                wxPostEvent( this, aEvent );
            };

    m_bitmap->Bind( wxEVT_RIGHT_DOWN, passOnEvent );
    m_bitmap->Bind( wxEVT_RIGHT_UP, passOnEvent );
}


void BITMAP_TOGGLE::SetValue( bool aValue )
{
    m_checked = aValue;
    m_bitmap->SetBitmap( aValue ? m_checked_bitmap : m_unchecked_bitmap );
}
