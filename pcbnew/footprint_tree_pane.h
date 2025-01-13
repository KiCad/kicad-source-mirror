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

#ifndef FOOTPRINT_TREE_PANE_H
#define FOOTPRINT_TREE_PANE_H

#include <wx/panel.h>
#include <wx/dataview.h>
#include <vector>

class LIB_TREE;
class FOOTPRINT_EDIT_FRAME;
class wxBoxSizer;

/**
 * Footprint Editor pane with footprint library tree.
 */
class FOOTPRINT_TREE_PANE : public wxPanel
{
public:
    FOOTPRINT_TREE_PANE( FOOTPRINT_EDIT_FRAME* aParent );
    ~FOOTPRINT_TREE_PANE();

    LIB_TREE* GetLibTree() const
    {
        return m_tree;
    }

    /**
     * Focus the search widget if it exists
     */
    void FocusSearchFieldIfExists();

protected:

    /**
     * Handle parent menu events to block preview updates while the menu is open.
     */
    void onMenuOpen( wxMenuEvent& aEvent );
    void onMenuClose( wxMenuEvent& aEvent );

    void onComponentSelected( wxCommandEvent& aEvent );
    void onUpdateUI( wxUpdateUIEvent& aEvent );

    FOOTPRINT_EDIT_FRAME* m_frame;
    LIB_TREE*             m_tree;             ///< component search tree widget
};

#endif /* FOOTPRINT_TREE_PANE_H */
