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

#pragma once

#include <bitmaps.h>
#include <widgets/bitmap_button.h>

#include <wx/dataview.h>
#include <wx/gbsizer.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/srchctrl.h>
#include <wx/string.h>

class EDA_BASE_FRAME;

/**
 * A base class used to implement docking net inspector panels.
 *
 * Provides a filter control, a settings button, and a data-driven wxDataViewCtrl
 */
class NET_INSPECTOR_PANEL : public wxPanel
{
public:
    NET_INSPECTOR_PANEL( wxWindow* parent, EDA_BASE_FRAME* aFrame, wxWindowID id = wxID_ANY,
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxSize( -1, -1 ), long style = wxTAB_TRAVERSAL,
                         const wxString& name = wxEmptyString );

    ~NET_INSPECTOR_PANEL();

    /**
     * Rebuild inspector data if project settings updated
     *
     * Called by the parent EDA_EDIT_FRAME on change of settings (e.g. stackup, netclass
     * definitions)
     */
    virtual void OnParentSetupChanged() {};

    /**
     * Save the net inspector settings - called from EDA_EDIT_FRAME when hiding the panel
     */
    virtual void SaveSettings() {};

    /**
     * Prepare the panel when (re-)shown in the editor
     */
    virtual void OnShowPanel() {}

    /**
     * Notification from file loader when board changed and connectivity rebuilt
     */
    virtual void OnBoardChanged() {}

protected:
    // User-driven UI events (override in derrived classes as required)
    virtual void OnSetFocus( wxFocusEvent& event ) { event.Skip(); }
    virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
    virtual void OnSearchTextChanged( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnConfigButton( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnLanguageChanged( wxCommandEvent& event );

    /**
     * Implementation-specific implementation of language update handling
     */
    virtual void OnLanguageChangedImpl() {};

protected:
    EDA_BASE_FRAME* m_frame;

    wxGridBagSizer* m_sizerOuter;
    wxSearchCtrl*   m_searchCtrl;
    BITMAP_BUTTON*  m_configureBtn;
    wxDataViewCtrl* m_netsList;
};
