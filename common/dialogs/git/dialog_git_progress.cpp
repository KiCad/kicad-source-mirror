/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <wx/progdlg.h>
#include <wx/statusbr.h>

#include <dialog_shim.h>

class GIT_PROGRESS : public DIALOG_SHIM {
public:
    GIT_PROGRESS(wxWindow* aParent, int aMaxValue1, int aMaxValue2);
    ~GIT_PROGRESS();

    void SetStatusText(const wxString& aText);
    bool UpdateProgressBar1(int aValue);
    bool UpdateProgressBar2(int aValue);

private:
    void OnCancel(wxCommandEvent& aEvent);

    wxGauge* m_progBar1;
    wxGauge* m_progBar2;
    wxButton* m_cancelBtn;
    wxStatusBar* m_statusBar;
    bool m_cancelled;

    DECLARE_EVENT_TABLE()
};
