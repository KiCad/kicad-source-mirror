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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef WX_ELLIPSIZED_STATIC_TEXT_H_
#define WX_ELLIPSIZED_STATIC_TEXT_H_

#include <wx/stattext.h>

/**
 * A version of a wxStaticText control that will request a smaller size than the full string.
 *
 * This can be used with the ellipsization styles to ensure that the control will actually
 * ellipsize properly inside sizer elements (since they ask for the best size but GTK reports
 * the best size as being the full string length, even when ellipsization is enabled). This work
 * around is discussed in upstream ticket https://trac.wxwidgets.org/ticket/18992.
 */
class WX_ELLIPSIZED_STATIC_TEXT: public wxStaticText
{
public:
    WX_ELLIPSIZED_STATIC_TEXT( wxWindow* aParent, wxWindowID aID, const wxString& aLabel,
                               const wxPoint& aPos = wxDefaultPosition,
                               const wxSize& aSize = wxDefaultSize,
                               long aStyle = 0 );

    /**
     * Set the string that is used for determining the requested size of the control.
     *
     * The control will return this string length from GetBestSize(), regardless of what string
     * the control is displaying.
     *
     * @param aString is the smallest string to display without ellipses.
     */
    void SetMinimumStringLength( const wxString& aString )
    {
        m_minimumString = aString;
    }

protected:
    wxSize DoGetBestSize() const override;

private:
    wxString m_minimumString;  ///< The string that is used to set the minimum control width.
};


#endif // WX_ELLIPSIZED_STATIC_TEXT_H_
