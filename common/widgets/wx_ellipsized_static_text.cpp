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

#include <widgets/ui_common.h>
#include <widgets/wx_ellipsized_static_text.h>

WX_ELLIPSIZED_STATIC_TEXT::WX_ELLIPSIZED_STATIC_TEXT( wxWindow* aParent, wxWindowID aID,
                                                      const wxString& aLabel, const wxPoint& aPos,
                                                      const wxSize& aSize, long aStyle )
    : wxStaticText( aParent, aID, aLabel, aPos, aSize, aStyle ),
      m_minimumString( "M...M" )
{
}


wxSize WX_ELLIPSIZED_STATIC_TEXT::DoGetBestSize() const
{
    // This is slightly awkward because DoGetBestSize is const upstream, but we need the non-const
    // object to measure the text size in this function.
    return KIUI::GetTextSize( m_minimumString, const_cast<WX_ELLIPSIZED_STATIC_TEXT*>( this ) );
}
