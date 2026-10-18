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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <widgets/field_warning_icon.h>

#include <algorithm>
#include <utility>
#include <vector>

#include <bitmaps.h>
#include <i18n_utility.h>
#include <wx/choice.h>
#include <wx/combo.h>
#include <wx/combobox.h>
#include <wx/settings.h>


FIELD_WARNING_ICON::FIELD_WARNING_ICON( wxWindow* aField ) :
        wxGenericStaticBitmap( aField->GetParent(), wxID_ANY, KiBitmapBundle( BITMAPS::small_warning ) ),
        m_field( aField )
{
    Hide();

    // Without a background of its own the icon shows the page behind the field, or on some ports the text under it
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    m_field->Bind( wxEVT_SIZE, &FIELD_WARNING_ICON::onFieldChanged, this );
    m_field->Bind( wxEVT_MOVE, &FIELD_WARNING_ICON::onFieldChanged, this );
    m_field->Bind( wxEVT_DESTROY, &FIELD_WARNING_ICON::onFieldDestroyed, this );
}


FIELD_WARNING_ICON::~FIELD_WARNING_ICON()
{
    // The parent destroys the field first, since it was created first
    if( m_field )
    {
        m_field->Unbind( wxEVT_SIZE, &FIELD_WARNING_ICON::onFieldChanged, this );
        m_field->Unbind( wxEVT_MOVE, &FIELD_WARNING_ICON::onFieldChanged, this );
        m_field->Unbind( wxEVT_DESTROY, &FIELD_WARNING_ICON::onFieldDestroyed, this );
    }
}


void FIELD_WARNING_ICON::SetWarnings( const wxArrayString& aMessages )
{
    // Identical messages collapse into one line with a count, in the order they first appear
    std::vector<std::pair<wxString, size_t>> lines;

    for( const wxString& message : aMessages )
    {
        auto it = std::find_if( lines.begin(), lines.end(),
                                [&]( const std::pair<wxString, size_t>& aLine )
                                {
                                    return aLine.first == message;
                                } );

        if( it != lines.end() )
            it->second++;
        else
            lines.emplace_back( message, 1 );
    }

    wxString text;

    for( size_t ii = 0; ii < lines.size() && ii < MAX_LINES; ++ii )
    {
        if( ii > 0 )
            text += wxS( "\n" );

        text += lines[ii].first;

        if( lines[ii].second > 1 )
            text += wxString::Format( _( " (%zu times)" ), lines[ii].second );
    }

    if( lines.size() > MAX_LINES )
        text += wxString::Format( _( "\n... and %zu more" ), lines.size() - MAX_LINES );

    SetToolTip( text );
    Show( !text.IsEmpty() );

    Reposition();
}


void FIELD_WARNING_ICON::SetWarning( const wxString& aMessage )
{
    wxArrayString messages;

    if( !aMessage.IsEmpty() )
        messages.Add( aMessage );

    SetWarnings( messages );
}


void FIELD_WARNING_ICON::Reposition()
{
    if( !m_field )
        return;

    const wxSize fieldSize = m_field->GetSize();
    int          buttonWidth = 0;

    // A dropdown button is about as wide as the field is tall, except on Windows where it has the width of a
    // scrollbar
    if( dynamic_cast<wxComboBox*>( m_field ) || dynamic_cast<wxChoice*>( m_field )
        || dynamic_cast<wxComboCtrl*>( m_field ) )
    {
#ifdef __WXMSW__
        buttonWidth = wxSystemSettings::GetMetric( wxSYS_VSCROLL_X, m_field );
#else
        buttonWidth = fieldSize.y;
#endif
    }

    const wxSize iconSize = GetSize();
    const int    gap = m_field->FromDIP( 3 );

    SetPosition( m_field->GetPosition()
                 + wxPoint( fieldSize.x - buttonWidth - iconSize.x - gap, ( fieldSize.y - iconSize.y ) / 2 ) );
    Raise();
}


void FIELD_WARNING_ICON::onFieldChanged( wxEvent& aEvent )
{
    aEvent.Skip();

    // The field's new geometry is only final once the sizer is done with it
    CallAfter(
            [this]()
            {
                Reposition();
            } );
}


void FIELD_WARNING_ICON::onFieldDestroyed( wxWindowDestroyEvent& aEvent )
{
    // Only the field's own destruction counts, not a child's that bubbles up
    if( aEvent.GetEventObject() == m_field )
        m_field = nullptr;

    aEvent.Skip();
}
