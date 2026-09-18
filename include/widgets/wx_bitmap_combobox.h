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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef WX_BITMAP_COMBOBOX_H
#define WX_BITMAP_COMBOBOX_H

#include <wx/bmpcbox.h>
#include <wx/weakref.h>

class WX_BITMAP_COMBOBOX : public wxBitmapComboBox
{
public:
    WX_BITMAP_COMBOBOX( wxWindow *parent, wxWindowID id = wxID_ANY, const wxString& value = wxEmptyString,
                        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                        int n = 0, const wxString choices[] = NULL, long style = 0,
                        const wxValidator& validator = wxDefaultValidator,
                        const wxString& name = wxASCII_STR( wxBitmapComboBoxNameStr ) );

    wxSize DoGetBestSize() const override;

#ifdef __WXMAC__
protected:
    void DoShowPopup( const wxRect& aRect, int aFlags ) override;

private:
    /**
     * Stop wxPopupTransientWindow from releasing the drop-down list's mouse capture.
     *
     * The popup is never the key window, so macOS hands its mouse-moved events to the controls it
     * covers and the list tracks the pointer only while it holds the capture.  Releasing it, as
     * wxPopupTransientWindow does from idle once the pointer is inside, freezes the highlight.
     *
     * Called from DoShowPopup() because the popup window does not exist yet at dropdown time.
     */
    void holdPopupMouseCapture();

    wxWindowRef m_hookedPopupWindow;
#endif
};


#endif //WX_BITMAP_COMBOBOX_H
