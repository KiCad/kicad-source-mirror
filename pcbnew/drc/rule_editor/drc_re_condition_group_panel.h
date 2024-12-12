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
#include <vector>

class BOARD;
class DRC_RE_CONDITION_ROW_PANEL;
class wxChoice;
class wxBitmapButton;
class wxBoxSizer;
class wxStaticText;

/**
 * Container panel that manages multiple condition rows with boolean operators.
 *
 * Supports AND, OR, AND NOT, OR NOT operators between conditions.
 * Evaluates left-to-right without parentheses.
 */
class DRC_RE_CONDITION_GROUP_PANEL : public wxPanel
{
public:
    enum class BOOL_OPERATOR
    {
        AND = 0,
        OR,
        AND_NOT,
        OR_NOT
    };

    DRC_RE_CONDITION_GROUP_PANEL( wxWindow* aParent, BOARD* aBoard, bool aTwoObjectConstraint );

    /**
     * Parse a condition string and populate the UI.
     *
     * @param aConditionExpr The complete condition expression
     */
    void ParseCondition( const wxString& aConditionExpr );

    /**
     * Build a condition string from the current UI state.
     *
     * @return The complete condition expression
     */
    wxString BuildCondition() const;

    /**
     * Set a callback to be invoked when any condition changes.
     */
    void SetChangeCallback( std::function<void()> aCallback ) { m_changeCallback = aCallback; }

    /**
     * @return true if any row has custom query selected.
     */
    bool HasCustomQuerySelected() const;

private:
    struct CONDITION_ENTRY
    {
        BOOL_OPERATOR               boolOp;
        wxChoice*                   operatorChoice;
        DRC_RE_CONDITION_ROW_PANEL* panel;
        wxBoxSizer*                 rowSizer;
    };

    void addConditionRow( BOOL_OPERATOR aOp = BOOL_OPERATOR::AND );
    void removeConditionRow( int aIndex );
    void onAddClicked( wxCommandEvent& aEvent );
    void rebuildLayout();
    void updateDeleteButtonVisibility();

    /**
     * Tokenize a condition expression by boolean operators.
     *
     * @param aExpr The expression to tokenize
     * @param aParts Output vector of (operator, expression) pairs
     * @return true if tokenization succeeded
     */
    bool tokenizeCondition( const wxString& aExpr,
                            std::vector<std::pair<BOOL_OPERATOR, wxString>>& aParts );

    BOARD*                       m_board;
    bool                         m_twoObjectConstraint;
    bool                         m_suppressCallbacks;
    std::vector<CONDITION_ENTRY> m_conditions;
    wxBoxSizer*                  m_mainSizer;
    wxBitmapButton*              m_addBtn;
    wxStaticText*                m_labelText;

    std::function<void()>        m_changeCallback;
};
