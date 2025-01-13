/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SYM_TREE_PANE_H
#define SYM_TREE_PANE_H

#include <wx/panel.h>
#include <wx/dataview.h>
#include <vector>

class LIB_TREE;
class SYMBOL_EDIT_FRAME;
class LIB_SYMBOL_LIBRARY_MANAGER;
class wxBoxSizer;

/**
 * Library Editor pane with symbol tree and symbol library table selector.
 */
class SYMBOL_TREE_PANE : public wxPanel
{
public:
    SYMBOL_TREE_PANE( SYMBOL_EDIT_FRAME* aParent, LIB_SYMBOL_LIBRARY_MANAGER* aLibMgr );
    ~SYMBOL_TREE_PANE();

    LIB_TREE* GetLibTree() const
    {
        return m_tree;
    }

protected:
    void onSymbolSelected( wxCommandEvent& aEvent );
    void onUpdateUI( wxUpdateUIEvent& aEvent );

    /**
     * Handle parent menu events to block preview updates while the menu is open.
     */
    void onMenuOpen( wxMenuEvent& aEvent );
    void onMenuClose( wxMenuEvent& aEvent );

    SYMBOL_EDIT_FRAME*          m_symbolEditFrame;
    LIB_TREE*                   m_tree;             ///< symbol search tree widget
    LIB_SYMBOL_LIBRARY_MANAGER* m_libMgr;
};

#endif /* SYM_TREE_PANE_H */
