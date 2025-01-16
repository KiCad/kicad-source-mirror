/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <functional>
#include <widgets/footprint_choice.h>
#include <wx/dc.h>
#include <wx/pen.h>

wxColour FOOTPRINT_CHOICE::m_grey( 0x808080 );


FOOTPRINT_CHOICE::FOOTPRINT_CHOICE( wxWindow* aParent, int aId ) :
        wxOwnerDrawnComboBox( aParent, aId, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                              /* n */ 0, /* choices */ nullptr, wxCB_READONLY ),
        m_last_selection( 0 )
{
}


FOOTPRINT_CHOICE::~FOOTPRINT_CHOICE()
{
}


void FOOTPRINT_CHOICE::DoSetPopupControl( wxComboPopup* aPopup )
{
    using namespace std::placeholders;
    wxOwnerDrawnComboBox::DoSetPopupControl( aPopup );

    // Bind events to intercept selections, so the separator can be made nonselectable.

    GetVListBoxComboPopup()->Bind( wxEVT_MOTION, &FOOTPRINT_CHOICE::TryVetoMouse, this );
    GetVListBoxComboPopup()->Bind( wxEVT_LEFT_DOWN, &FOOTPRINT_CHOICE::TryVetoMouse, this );
    GetVListBoxComboPopup()->Bind( wxEVT_LEFT_UP, &FOOTPRINT_CHOICE::TryVetoMouse, this );
    GetVListBoxComboPopup()->Bind( wxEVT_LEFT_DCLICK, &FOOTPRINT_CHOICE::TryVetoMouse, this );
    GetVListBoxComboPopup()->Bind( wxEVT_LISTBOX, std::bind( &FOOTPRINT_CHOICE::TryVetoSelect,
                                                             this, _1, true ) );
    Bind( wxEVT_COMBOBOX, std::bind( &FOOTPRINT_CHOICE::TryVetoSelect, this, _1, false ) );
}


void FOOTPRINT_CHOICE::OnDrawItem( wxDC& aDC, wxRect const& aRect, int aItem, int aFlags ) const
{
    wxString text = SafeGetString( aItem );

    if( text == wxEmptyString )
    {
        wxPen pen( m_grey, 1, wxPENSTYLE_SOLID );

        aDC.SetPen( pen );
        aDC.DrawLine( aRect.x, aRect.y + aRect.height / 2, aRect.x + aRect.width,
                      aRect.y + aRect.height / 2 );
    }
    else
    {
        wxCoord x, y;

        if( aFlags & wxODCB_PAINTING_CONTROL )
        {
            x = aRect.x + GetMargins().x;
            y = ( aRect.height - aDC.GetCharHeight() ) / 2 + aRect.y;
        }
        else
        {
            x = aRect.x + 2;
            y = aRect.y;
        }

        // If this item has a footprint and that footprint has a ":" delimiter, find the
        // library component, then find that in the display string and grey it out.
        size_t start_grey = 0;
        size_t end_grey = 0;

        wxString lib = static_cast<wxStringClientData*>( GetClientObject( aItem ) )->GetData();
        size_t   colon_index = lib.rfind( ':' );

        if( colon_index != wxString::npos )
        {
            wxString library_part = lib.SubString( 0, colon_index );
            size_t   library_index = text.rfind( library_part );

            if( library_index != wxString::npos )
            {
                start_grey = library_index;
                end_grey = start_grey + library_part.Length();
            }
        }

        if( start_grey != end_grey && !( aFlags & wxODCB_PAINTING_SELECTED ) )
        {
            x = DrawTextFragment( aDC, x, y, text.SubString( 0, start_grey - 1 ) );

            wxColour standard_color = aDC.GetTextForeground();

            aDC.SetTextForeground( m_grey );
            x = DrawTextFragment( aDC, x, y, text.SubString( start_grey, end_grey - 1 ) );

            aDC.SetTextForeground( standard_color );
            x = DrawTextFragment( aDC, x, y, text.SubString( end_grey, text.Length() - 1 ) );
        }
        else
        {
            aDC.DrawText( text, x, y );
        }
    }
}


wxCoord FOOTPRINT_CHOICE::OnMeasureItem( size_t aItem ) const
{
    if( SafeGetString( aItem ) == wxS( "" ) )
        return 11;
    else
        return wxOwnerDrawnComboBox::OnMeasureItem( aItem );
}


wxCoord FOOTPRINT_CHOICE::OnMeasureItemWidth( size_t aItem ) const
{
    if( SafeGetString( aItem ) == wxS( "" ) )
        return GetTextRect().GetWidth() - 2;
    else
        return wxOwnerDrawnComboBox::OnMeasureItemWidth( aItem );
}


wxCoord FOOTPRINT_CHOICE::DrawTextFragment( wxDC& aDC, wxCoord x, wxCoord y, wxString const& aText )
{
    aDC.DrawText( aText, x, y );
    return x + aDC.GetTextExtent( aText ).GetWidth();
}


void FOOTPRINT_CHOICE::TryVetoMouse( wxMouseEvent& aEvent )
{
    int item = GetVListBoxComboPopup()->VirtualHitTest( aEvent.GetPosition().y );

    if( SafeGetString( item ) != wxS( "" ) )
        aEvent.Skip();
}


void FOOTPRINT_CHOICE::TryVetoSelect( wxCommandEvent& aEvent, bool aInner )
{
    int sel = GetSelectionEither( aInner );

    if( sel >= 0 && sel < (int) GetCount() )
    {
        wxString text = SafeGetString( sel );

        if( text == "" )
        {
            SetSelectionEither( aInner, m_last_selection );
        }
        else
        {
            m_last_selection = sel;
            aEvent.Skip();
        }
    }
}


wxString FOOTPRINT_CHOICE::SafeGetString( int aItem ) const
{
    if( aItem >= 0 && aItem < (int) GetCount() )
        return GetVListBoxComboPopup()->GetString( aItem );
    else
        return wxEmptyString;
}


int FOOTPRINT_CHOICE::GetSelectionEither( bool aInner ) const
{
    if( aInner )
        return GetVListBoxComboPopup()->wxVListBox::GetSelection();
    else
        return GetSelection();
}


void FOOTPRINT_CHOICE::SetSelectionEither( bool aInner, int aSel )
{
    if( aSel >= 0 && aSel < (int) GetCount() )
    {
        if( aInner )
            return GetVListBoxComboPopup()->wxVListBox::SetSelection( aSel );
        else
            return SetSelection( aSel );
    }
}
