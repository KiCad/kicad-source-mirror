
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

#pragma once

#include <memory>
#include <optional>

#include <wx/panel.h>
#include <wx/combo.h>
#include <wx/vlbox.h>

#include <font/font.h>


class wxTextValidator;
class wxTextCtrl;
class wxListBox;


class FILTER_COMBOPOPUP_LISTBOX : public wxVListBox
{
public:
    FILTER_COMBOPOPUP_LISTBOX( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos, const wxSize& aSize,
                               int aFlags ) :
            wxVListBox( aParent, aId, aPos, aSize, aFlags | wxLB_OWNERDRAW )
    {
        m_displayStyleCallback =
                []( const wxString& aItem ) -> int
                {
                    return 0;
                };
    }

    void Set( const wxArrayString& aChoices )
    {
        m_choices = aChoices;
        SetItemCount( m_choices.size() );

        RefreshAll();
    }

    void SetDisplayStyleCallback( const std::function<int( const wxString& aItem )>& aCallback );

    size_t GetCount() const
    {
        return m_choices.size();
    }

    wxString GetString( size_t aIdx ) const
    {
        if( aIdx < m_choices.size() )
            return m_choices[aIdx];
        else
            return wxEmptyString;
    }

    void SetStringSelection( const wxString& aString )
    {
        for( int ii = 0; ii < (int) m_choices.size(); ++ii )
        {
            if( m_choices[ii] == aString )
            {
                SetSelection( ii );
                return;
            }
        }

        SetSelection( -1 );
    }

    int HitTest( const wxPoint& aPoint ) const;

protected:
    wxCoord OnMeasureItem( size_t aItem ) const override;

    void OnDrawItem( wxDC& aDC, const wxRect& aRect, size_t aItem ) const override;

    void OnDrawBackground( wxDC& aDC, const wxRect& aRect, size_t aItem ) const override;

private:
    wxArrayString                               m_choices;
    std::function<int( const wxString& aItem )> m_displayStyleCallback;
};


class FILTER_COMBOPOPUP : public wxPanel, public wxComboPopup
{
public:
    FILTER_COMBOPOPUP();

    bool Create( wxWindow* aParent ) override;

    wxWindow* GetControl() override { return this; }

    void SetStringList( const wxArrayString& aStringList );

    wxString GetStringValue() const override;
    void SetStringValue( const wxString& aNetName ) override;

    void SetSelectedString( const wxString& aString );

    void SetDisplayStyleCallback( const std::function<int( const wxString& aItem )>& aCallback );

    void OnPopup() override;

    void OnStartingKey( wxKeyEvent& aEvent );

    wxSize GetAdjustedSize( int aMinWidth, int aPrefHeight, int aMaxHeight ) override;

    virtual void Accept();

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
     * Fill the combobox list
     */
    virtual void getListContent( wxArrayString& aStringList );

    /**
     * Call this to rebuild the list from the getListContent() method.
     */
    void rebuildList();

private:
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
    wxTextValidator*           m_filterValidator;
    wxTextCtrl*                m_filterCtrl;
    FILTER_COMBOPOPUP_LISTBOX* m_listBox;
    int                        m_minPopupWidth;
    int                        m_maxPopupHeight;

    wxEvtHandler*              m_focusHandler;

    wxString                   m_selectedString;
    wxArrayString              m_stringList;

    std::function<int( const wxString& aItem )> m_displayStyleCallback;
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
    // C'tor matching wxFormBuilder's Custom Control
    FILTER_COMBOBOX( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize, long style = 0 );

    // C'tor matching wxFormBuilder's ComboxBox.
    FILTER_COMBOBOX( wxWindow* parent, wxWindowID id, const wxString& value,
                     const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                     int count = 0, wxString strings[] = nullptr, long style = 0 );

    ~FILTER_COMBOBOX();

    virtual void SetStringList( const wxArrayString& aStringList );

    void SetDisplayStyleCallback( const std::function<int( const wxString& aItem )>& aCallback );

    virtual void SetSelectedString( const wxString& aString );

protected:
    void setFilterPopup( FILTER_COMBOPOPUP* aPopup );

    void onKeyDown( wxKeyEvent& aEvt );

protected:
    FILTER_COMBOPOPUP* m_filterPopup;
};
