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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
