
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <memory>
#include <optional>

#include <wx/panel.h>
#include <wx/combo.h>
#include <wx/listbox.h>


class wxTextValidator;
class wxTextCtrl;
class wxListBox;


class FILTER_COMBOPOPUP : public wxPanel, public wxComboPopup
{
public:
    FILTER_COMBOPOPUP();

    bool Create( wxWindow* aParent ) override;

    wxWindow* GetControl() override { return this; }

    void SetStringValue( const wxString& aNetName ) override;

    void OnPopup() override;

    void OnStartingKey( wxKeyEvent& aEvent );

    wxSize GetAdjustedSize( int aMinWidth, int aPrefHeight, int aMaxHeight ) override;

    virtual void Accept() = 0;

protected:
    /**
     * Get the currently selected value in the list, or std::nullopt
     */
    std::optional<wxString> getSelectedValue() const;

    /**
     * Get the current value of the filter control. Can be empty.
     */
    wxString getFilterValue() const;

    /**
     * Call this to rebuild the list from the getListContent() method.
     */
    void rebuildList();

private:
    /**
     * Implement this to fill in the given list.
     */
    virtual void getListContent( wxArrayString& aListToFill ) = 0;

    wxSize updateSize();

    void onIdle( wxIdleEvent& aEvent );

    // Hot-track the mouse (for focus and listbox selection)
    void onMouseMoved( const wxPoint aScreenPos );
    void onMouseClick( wxMouseEvent& aEvent );
    void onKeyDown( wxKeyEvent& aEvent );
    void onEnter( wxCommandEvent& aEvent );
    void onFilterEdit( wxCommandEvent& aEvent );
    void doStartingKey( wxKeyEvent& aEvent );
    void doSetFocus( wxWindow* aWindow );

protected:
    wxTextValidator* m_filterValidator;
    wxTextCtrl*      m_filterCtrl;
    wxListBox*       m_listBox;
    int              m_minPopupWidth;
    int              m_maxPopupHeight;

    wxEvtHandler* m_focusHandler;
};


wxDECLARE_EVENT( FILTERED_ITEM_SELECTED, wxCommandEvent );

/**
 * A combobox that has a filterable popup.
 *
 * Useful when the list of items is long and you want the user to
 * be able to filter it by typing.
 */
class FILTER_COMBOBOX : public wxComboCtrl
{
public:
    FILTER_COMBOBOX( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize, long style = 0 );

    ~FILTER_COMBOBOX();

protected:
    void setFilterPopup( FILTER_COMBOPOPUP* aPopup );

    void onKeyDown( wxKeyEvent& aEvt );

    FILTER_COMBOPOPUP* m_filterPopup;
};
