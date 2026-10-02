/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WORKSPACE_TREE_MODEL_H
#define WORKSPACE_TREE_MODEL_H

#include <dialogs/git/workspace_tree.h>

#include <wx/dataview.h>

#include <functional>
#include <map>


/**
 * The Workspace page's tree as a wxDataViewModel. Each item is a node of the tree last passed to Reset(), which
 * stays valid until the next Reset(). The panel turns a node into column text through the cell callback.
 */
class WORKSPACE_TREE_MODEL : public wxDataViewModel
{
public:
    enum COLUMN
    {
        COL_NAME = 0,
        COL_STATE,
        COL_EDITING,
        COL_LAST_CHANGE,
        COL_COUNT
    };

    using CELL = std::function<wxString( const WORKSPACE_TREE_NODE&, unsigned aColumn )>;

    void SetCellText( CELL aCell ) { m_cell = std::move( aCell ); }

    /// Replace the whole tree; every item handed out before is invalid afterwards.
    void Reset( WORKSPACE_TREE_NODE aRoot );

    const WORKSPACE_TREE_NODE& Root() const { return m_root; }

    /// The node behind aItem, or null for the invisible root.
    const WORKSPACE_TREE_NODE* NodeOf( const wxDataViewItem& aItem ) const;

    /// The item showing aNode, which must belong to Root().
    wxDataViewItem ItemOf( const WORKSPACE_TREE_NODE& aNode ) const;

    /// Draw every row in the grey of stale data, or back in the normal colour.
    void SetGreyed( bool aGreyed );

    /// Ask the control to read every cell again, after a change the model was not told about.
    void RefreshCells();

    void GetValue( wxVariant& aVariant, const wxDataViewItem& aItem, unsigned int aCol ) const override;
    bool SetValue( const wxVariant&, const wxDataViewItem&, unsigned int ) override { return false; }
    bool GetAttr( const wxDataViewItem& aItem, unsigned int aCol, wxDataViewItemAttr& aAttr ) const override;

    wxDataViewItem GetParent( const wxDataViewItem& aItem ) const override;
    bool           IsContainer( const wxDataViewItem& aItem ) const override;
    unsigned int   GetChildren( const wxDataViewItem& aItem, wxDataViewItemArray& aChildren ) const override;

    /// Folders and libraries leave their cells to the callback rather than to wxWidgets' container default
    bool HasContainerColumns( const wxDataViewItem& ) const override { return true; }

private:
    void index( const WORKSPACE_TREE_NODE& aNode, const WORKSPACE_TREE_NODE* aParent );

    WORKSPACE_TREE_NODE                                              m_root;
    std::map<const WORKSPACE_TREE_NODE*, const WORKSPACE_TREE_NODE*> m_parents; ///< Null for a top level node
    CELL                                                             m_cell;
    bool                                                             m_greyed = false;
};

#endif // WORKSPACE_TREE_MODEL_H
