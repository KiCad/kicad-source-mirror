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

#include <wx/panel.h>
#include <functional>

class BOARD;
class NET_SELECTOR;
class NETCLASS_SELECTOR;
class AREA_SELECTOR;
class wxChoice;
class wxBitmapButton;
class wxStyledTextCtrl;
class wxBoxSizer;

/**
 * A single condition row in the condition group panel.
 *
 * Each row contains:
 * - Object selector (A/B) for two-object constraints
 * - Condition type dropdown (Any, Net, Netclass, Area, Custom)
 * - Value selector appropriate for the condition type
 * - Delete button
 */
class DRC_RE_CONDITION_ROW_PANEL : public wxPanel
{
public:
    enum class OBJECT_TARGET
    {
        OBJECT_A,
        OBJECT_B
    };

    enum class CONDITION_TYPE
    {
        ANY = 0,
        NET,
        NETCLASS,
        WITHIN_AREA,
        CUSTOM
    };

    DRC_RE_CONDITION_ROW_PANEL( wxWindow* aParent, BOARD* aBoard, bool aShowObjectSelector );

    void SetObjectTarget( OBJECT_TARGET aTarget );
    OBJECT_TARGET GetObjectTarget() const;

    void SetConditionType( CONDITION_TYPE aType );
    CONDITION_TYPE GetConditionType() const;

    void SetValue( const wxString& aValue );
    wxString GetValue() const;

    /**
     * Parse a condition expression and set the row's state.
     *
     * @param aExpr The expression to parse (e.g., "A.NetName == 'VCC'")
     * @return true if the expression was successfully parsed
     */
    bool ParseExpression( const wxString& aExpr );

    /**
     * Build a condition expression from the row's current state.
     *
     * @return The condition expression (e.g., "A.NetName == 'VCC'")
     */
    wxString BuildExpression() const;

    void SetDeleteCallback( std::function<void()> aCallback ) { m_deleteCallback = aCallback; }
    void SetChangeCallback( std::function<void()> aCallback ) { m_changeCallback = aCallback; }

    void ShowDeleteButton( bool aShow );
    bool HasCustomQuerySelected() const;

private:
    void onObjectChoice( wxCommandEvent& aEvent );
    void onConditionChoice( wxCommandEvent& aEvent );
    void onDeleteClicked( wxCommandEvent& aEvent );
    void updateVisibility();

    bool               m_showObjectSelector;

    wxChoice*          m_objectChoice;
    wxChoice*          m_conditionChoice;
    NET_SELECTOR*      m_netSelector;
    NETCLASS_SELECTOR* m_netclassSelector;
    AREA_SELECTOR*     m_areaSelector;
    wxStyledTextCtrl*  m_customQueryCtrl;
    wxBitmapButton*    m_deleteBtn;
    wxBoxSizer*        m_mainSizer;
    wxBoxSizer*        m_contentSizer;
    wxBoxSizer*        m_rowSizer;

    std::function<void()> m_deleteCallback;
    std::function<void()> m_changeCallback;
};
