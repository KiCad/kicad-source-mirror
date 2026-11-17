/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#pragma once

#include <wx/textentry.h>
#include <wx/weakref.h>
#include <wx/window.h>

/**
 * Remember which window has the keyboard focus, so focus can be put back after code that moves
 * it as a side effect, such as building a page or showing a wxSimplebook page.
 *
 * Refocusing a text field selects all of it on some platforms, so Restore() puts back the
 * field's selection, including any keys that reached the field in the meantime.
 */
class FOCUS_SNAPSHOT
{
public:
    FOCUS_SNAPSHOT() :
            m_focus( wxWindow::FindFocus() )
    {
        if( dynamic_cast<wxTextEntry*>( m_focus.get() ) )
        {
            captureSelection();
            m_focus->Bind( wxEVT_TEXT, &FOCUS_SNAPSHOT::onText, this );
        }
    }

    ~FOCUS_SNAPSHOT()
    {
        if( m_focus && dynamic_cast<wxTextEntry*>( m_focus.get() ) )
            m_focus->Unbind( wxEVT_TEXT, &FOCUS_SNAPSHOT::onText, this );
    }

    FOCUS_SNAPSHOT( const FOCUS_SNAPSHOT& ) = delete;
    FOCUS_SNAPSHOT& operator=( const FOCUS_SNAPSHOT& ) = delete;

    /// @return true when the remembered window is aParent or one of its descendants.
    bool IsIn( const wxWindow* aParent ) const { return m_focus && aParent->IsDescendant( m_focus.get() ); }

    /// Give the focus back to the remembered window if something else took it.
    void Restore()
    {
        if( !m_focus || wxWindow::FindFocus() == m_focus )
            return;

#ifndef __WXOSX__
        // Elsewhere the field keeps its selection while unfocused, so read what it holds now
        captureSelection();
#endif

        m_focus->SetFocus();

        if( wxTextEntry* text = dynamic_cast<wxTextEntry*>( m_focus.get() ) )
            text->SetSelection( m_from, m_to );
    }

private:
    void captureSelection()
    {
        if( wxTextEntry* text = dynamic_cast<wxTextEntry*>( m_focus.get() ) )
            text->GetSelection( &m_from, &m_to );
    }

    // wxOSX reports a field's selection only while it has focus, so follow it as keys arrive
    void onText( wxCommandEvent& aEvent )
    {
        if( wxWindow::FindFocus() == m_focus )
            captureSelection();

        aEvent.Skip();
    }

    wxWeakRef<wxWindow> m_focus;
    long                m_from = 0;
    long                m_to = 0;
};
