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
#include <widgets/wx_combobox.h>
#include <wx/dc.h>
#include <wx/pen.h>
#include <wx/settings.h>


#define SEPARATOR wxT( "---" )


WX_COMBOBOX::WX_COMBOBOX( wxWindow* aParent, int aId, const wxString& aValue, const wxPoint& aPos,
                          const wxSize& aSize, int n, const wxString choices[], long style ) :
        wxOwnerDrawnComboBox( aParent, aId, aValue, aPos, aSize, n, choices, style ),
        m_lastSelection( 0 )
{
}


WX_COMBOBOX::~WX_COMBOBOX()
{
}


void WX_COMBOBOX::DoSetPopupControl( wxComboPopup* aPopup )
{
    using namespace std::placeholders;
    wxOwnerDrawnComboBox::DoSetPopupControl( aPopup );

    // Bind events to intercept selections, so the separator can be made nonselectable.

    GetVListBoxComboPopup()->Bind( wxEVT_MOTION, &WX_COMBOBOX::TryVetoMouse, this );
    GetVListBoxComboPopup()->Bind( wxEVT_LEFT_DOWN, &WX_COMBOBOX::TryVetoMouse, this );
    GetVListBoxComboPopup()->Bind( wxEVT_LEFT_UP, &WX_COMBOBOX::TryVetoMouse, this );
    GetVListBoxComboPopup()->Bind( wxEVT_LEFT_DCLICK, &WX_COMBOBOX::TryVetoMouse, this );
    GetVListBoxComboPopup()->Bind( wxEVT_LISTBOX, std::bind( &WX_COMBOBOX::TryVetoSelect,
                                                             this, _1, true ) );

    Bind( wxEVT_COMBOBOX, std::bind( &WX_COMBOBOX::TryVetoSelect, this, _1, false ) );
}


void WX_COMBOBOX::OnDrawItem( wxDC& aDC, wxRect const& aRect, int aItem, int aFlags ) const
{
    wxString text = GetMenuText( aItem );

    if( text == SEPARATOR )
    {
        wxPen pen( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ), 1 );

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

        aDC.DrawText( text, x, y );
    }
}


int WX_COMBOBOX::GetCharHeight() const
{
    return wxOwnerDrawnComboBox::GetCharHeight() + 2;
}


wxCoord WX_COMBOBOX::OnMeasureItem( size_t aItem ) const
{
    if( GetMenuText( aItem ) == SEPARATOR )
        return 11;
    else
        return wxOwnerDrawnComboBox::OnMeasureItem( aItem );
}


wxCoord WX_COMBOBOX::OnMeasureItemWidth( size_t aItem ) const
{
    if( GetMenuText( aItem ) == SEPARATOR )
        return GetTextRect().GetWidth() - 2;
    else
        return wxOwnerDrawnComboBox::OnMeasureItemWidth( aItem );
}


void WX_COMBOBOX::TryVetoMouse( wxMouseEvent& aEvent )
{
    int item = GetVListBoxComboPopup()->VirtualHitTest( aEvent.GetPosition().y );

    if( GetMenuText( item ) != SEPARATOR )
        aEvent.Skip();
}


void WX_COMBOBOX::TryVetoSelect( wxCommandEvent& aEvent, bool aInner )
{
    int sel = GetSelectionEither( aInner );

    if( sel >= 0 && sel < (int) GetCount() )
    {
        wxString text = GetMenuText( sel );

        if( text == SEPARATOR )
        {
            SetSelectionEither( aInner, m_lastSelection );
        }
        else
        {
            m_lastSelection = sel;
            aEvent.Skip();
        }
    }
}


void WX_COMBOBOX::Append( const wxString& aText, const wxString& aMenuText )
{
    if( !aMenuText.IsEmpty() )
        m_menuText[ GetCount() ] = aMenuText;

    wxOwnerDrawnComboBox::Append( aText );
}


wxString WX_COMBOBOX::GetMenuText( int aItem ) const
{
    if( m_menuText.count( aItem ) )
        return m_menuText.at( aItem );
    else if( aItem >= 0 && aItem < (int) GetCount() )
        return GetVListBoxComboPopup()->GetString( aItem );
    else
        return wxEmptyString;
}


int WX_COMBOBOX::GetSelectionEither( bool aInner ) const
{
    if( aInner )
        return GetVListBoxComboPopup()->wxVListBox::GetSelection();
    else
        return GetSelection();
}


void WX_COMBOBOX::SetSelectionEither( bool aInner, int aSel )
{
    if( aSel >= 0 && aSel < (int) GetCount() )
    {
        if( aInner )
            return GetVListBoxComboPopup()->wxVListBox::SetSelection( aSel );
        else
            return SetSelection( aSel );
    }
}
