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

#ifndef _BITMAP_TOGGLE_H
#define _BITMAP_TOGGLE_H

#include <wx/statbmp.h>
#include <wx/bmpbndl.h>
#include <wx/panel.h>

#include <gal/color4d.h>

class wxStaticBitmap;

wxDECLARE_EVENT( TOGGLE_CHANGED, wxCommandEvent );

/**
 * A checkbox control except with custom bitmaps for the checked and unchecked states.
 *
 * This is useful in space-constrained situations where native toggle button controls are too big.
 */

class BITMAP_TOGGLE : public wxPanel
{
public:
    BITMAP_TOGGLE() {}

    BITMAP_TOGGLE( wxWindow* aParent, wxWindowID aId, const wxBitmapBundle& aCheckedBitmap,
                   const wxBitmapBundle& aUncheckedBitmap, bool aChecked = false );

    /// Set the checkbox state
    void SetValue( bool aValue );

    /// Read the checkbox state
    bool GetValue() const { return m_checked; }

    /**
     * Update the window ID of this control and its children.
     *
     * @param aId new Window ID to set
     */
    void SetWindowID( wxWindowID aId )
    {
        SetId( aId );
        m_bitmap->SetId( aId );
    }

private:
    bool            m_checked;

    wxStaticBitmap* m_bitmap;
    wxBitmapBundle  m_unchecked_bitmap;
    wxBitmapBundle  m_checked_bitmap;

    wxLongLong      m_debounce;            ///< Timestamp for debouncing events.
};

#endif
