/**
 * @file class_pl_editor_screen.h
 */

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

#ifndef CLASS_PL_EDITOR_SCREEN_H_
#define CLASS_PL_EDITOR_SCREEN_H_


#include <base_units.h>
#include <class_base_screen.h>

class WORKSHEET_DATAITEM;


/* Handle info to display a board */
class PL_EDITOR_SCREEN : public BASE_SCREEN
{
public:
    /**
     * Constructor
     * @param aPageSizeIU is the size of the initial paper page in internal units.
     */
    PL_EDITOR_SCREEN( const wxSize& aPageSizeIU );

    ~PL_EDITOR_SCREEN();

    virtual int MilsToIuScalar() override;

    /**
     * Function ClearUndoORRedoList
     * virtual pure in BASE_SCREEN, so it must be defined here
     */

    void ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount = -1 ) override;
    /**
     * Function GetCurItem
     * returns the currently selected WORKSHEET_DATAITEM, overriding
     * BASE_SCREEN::GetCurItem().
     * @return WORKSHEET_DATAITEM* - the one selected, or NULL.
     */

    WORKSHEET_DATAITEM* GetCurItem() const
    {
        return (WORKSHEET_DATAITEM*) BASE_SCREEN::GetCurItem();
    }

    /**
     * Function SetCurItem
     * sets the currently selected object, m_CurrentItem.
     * @param aItem Any object derived from WORKSHEET_DATAITEM
     */
    void SetCurItem( WORKSHEET_DATAITEM* aItem ) { BASE_SCREEN::SetCurItem( (EDA_ITEM*)aItem ); }
};


#endif  // CLASS_PL_EDITOR_SCREEN_H_
