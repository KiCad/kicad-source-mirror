/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file design_tree_frame.h
 */

#ifndef  _DESIGN_TREE_FRAME_H
#define  _DESIGN_TREE_FRAME_H

#include <wx/treectrl.h>

#include <pl_editor_frame.h>

class WORKSHEET_DATAITEM;

/** class DESIGN_TREE_ITEM
 * Handle one item for the page layoiut design
 */
class DESIGN_TREE_ITEM_DATA : public wxTreeItemData
{
private:
    WORKSHEET_DATAITEM* m_wsItem;    // the page layout item owned by me

public:

    DESIGN_TREE_ITEM_DATA( WORKSHEET_DATAITEM* aItem = NULL )
    {
        m_wsItem = aItem;
    }

    /** @return the item managed by the cell
     */
    WORKSHEET_DATAITEM* GetItem() const
    {
        return m_wsItem;
    }

    /** Set the link to the item managed by the cell
     */
    void SetItem( WORKSHEET_DATAITEM* aItem )
    {
        m_wsItem = aItem;
    }
};


/**
 * Class DESIGN_TREE_FRAME is the left window showing the list of items
 */

class DESIGN_TREE_FRAME : protected wxTreeCtrl
{
    friend class PL_EDITOR_FRAME;

private:
    wxImageList* m_imageList;

public:
    DESIGN_TREE_FRAME( PL_EDITOR_FRAME* aParent );
    ~DESIGN_TREE_FRAME();

    void    ReCreateDesignTree();
    wxSize  GetMinSize() const override;

    /** @return the page layout item managed by the cell
     */
    WORKSHEET_DATAITEM* GetPageLayoutItem(wxTreeItemId aCell) const;

    /** @return the page layout item managed by the selected cell (or NULL)
     */
    WORKSHEET_DATAITEM* GetPageLayoutSelectedItem() const;

    /** @return the page layout item index managed by the selected cell (or -1)
     */
    int GetSelectedItemIndex();

    // Select the tree item corresponding to the WORKSHEET_DATAITEM aItem
    void SelectCell( WORKSHEET_DATAITEM* aItem );

    void SelectCell( const wxTreeItemId &aTreeItem, bool aSelect=true )
    {
        SelectItem( aTreeItem, aSelect );
    }
};

#endif /* _DESIGN_TREE_FRAME_H */
