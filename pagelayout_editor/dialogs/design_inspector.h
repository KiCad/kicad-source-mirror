/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 jp.charras at wanadoo.fr
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


/**
 * @file design_inspector.h
 */

#ifndef  _DESIGN_INSPECTOR_H
#define  _DESIGN_INSPECTOR_H

#include <vector>

class DIALOG_INSPECTOR_BASE;
class PL_EDITOR_FRAME;

/**
 * DESIGN_INSPECTOR is the left window showing the list of items
 */
class DIALOG_INSPECTOR : public DIALOG_INSPECTOR_BASE
{
public:
    DIALOG_INSPECTOR( PL_EDITOR_FRAME* aParent );
    ~DIALOG_INSPECTOR();

    void    ReCreateDesignList();

    /**
     * @return the DS_DATA_ITEM item managed by the grid row
     */
    DS_DATA_ITEM* GetDrawingSheetDataItem( int aRow ) const;

    // Select the tree item corresponding to the DS_DATA_ITEM aItem
    void SelectRow( DS_DATA_ITEM* aItem );

private:
    friend class PL_EDITOR_FRAME;

    wxGrid* GetGridList() const { return m_gridListItems; }
	void onCellClicked( wxGridEvent& event ) override;

    // The list of DS_DATA_ITEM found in drawing sheet
    std::vector<DS_DATA_ITEM*> m_itemsList;
    PL_EDITOR_FRAME* m_editorFrame;

};

#endif /* _DESIGN_INSPECTOR_H */
