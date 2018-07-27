/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
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

#ifndef CMP_TREE_PANE_H
#define CMP_TREE_PANE_H

#include <wx/panel.h>
#include <wx/dataview.h>
#include <vector>

class LIB_TREE;
class LIB_EDIT_FRAME;
class LIB_MANAGER;
class wxBoxSizer;

/**
 * Library Editor pane with component tree and symbol library table selector.
 */
class SYMBOL_TREE_PANE : public wxPanel
{
public:
    SYMBOL_TREE_PANE( LIB_EDIT_FRAME* aParent, LIB_MANAGER* aLibMgr );
    ~SYMBOL_TREE_PANE();

    LIB_TREE* GetLibTree() const
    {
        return m_tree;
    }

    ///> Updates the component tree
    void Regenerate();

protected:
    void onComponentSelected( wxCommandEvent& aEvent );

    LIB_EDIT_FRAME* m_libEditFrame;
    LIB_TREE* m_tree;             ///< component search tree widget
    LIB_MANAGER* m_libMgr;
};

#endif /* CMP_TREE_PANE_H */
