/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Marco Mattila <marcom99@gmail.com>
 * Copyright (C) 2006 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 1992-2021 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_FIND_BASE_H
#define DIALOG_FIND_BASE_H

#include <functional>
#include <sys/types.h>
#include <wx/event.h>
#include <deque>

#include <board_item.h>

#include <dialog_find_base.h>

using namespace std;

class DIALOG_FIND : public DIALOG_FIND_BASE
{
public:
    DIALOG_FIND( PCB_BASE_FRAME* aParent );

    /**
     * Return the currently found item or nullptr in the case of no items found.
     */
    inline BOARD_ITEM* GetItem() const
    {
        if( m_it != m_hitList.end() )
            return *m_it;
        else
            return nullptr;
    }

    /**
     * Function to be called on each found event.
     *
     * The callback function must be able to handle nullptr in the case where no item is found.
     */
    void SetCallback( std::function<void( BOARD_ITEM* )> aCallback )
    {
        m_highlightCallback = aCallback;
    }

    /**
     * Finds the next item
     */
    void FindNext() { search( true ); }

    /**
     * The Show method is overridden to make the search combobox
     * focused by default.
     * wxShowEvent is not suitable here because it supports MSW
     * and GTK only.
     */
    bool Show( bool show = true ) override;

protected:
    void OnClose( wxCloseEvent& event ) override;
    void OnCloseButtonClick( wxCommandEvent& aEvent ) override;

private:
    void onTextEnter( wxCommandEvent& event ) override;
    void onFindNextClick( wxCommandEvent& event ) override;
    void onFindPreviousClick( wxCommandEvent& event ) override;
    void onSearchAgainClick( wxCommandEvent& event ) override;
    void search( bool direction );

    PCB_BASE_FRAME*                     m_frame;
    std::deque<BOARD_ITEM*>             m_hitList;
    std::deque<BOARD_ITEM*>::iterator   m_it;
    bool                                m_upToDate;

    std::function<void( BOARD_ITEM* )> m_highlightCallback;
};

#endif /* DIALOG_FIND_BASE_H */
