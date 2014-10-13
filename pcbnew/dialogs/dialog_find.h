/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Marco Mattila <marcom99@gmail.com>
 * Copyright (C) 2006 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 1992-2012 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_find_base.h>
#include <boost/function.hpp>

class DIALOG_FIND : public DIALOG_FIND_BASE
{
public:
    DIALOG_FIND( PCB_BASE_FRAME* aParent );
    inline BOARD_ITEM* GetItem() const { return foundItem; }
    void EnableWarp( bool aEnabled );
    void SetCallback( boost::function<void (BOARD_ITEM*)> aCallback ) { callback = aCallback; }

private:
    PCB_BASE_FRAME* parent;

    int itemCount, markerCount;
    static wxString prevSearchString;
    static bool warpMouse;
    BOARD_ITEM* foundItem;

    // Function called when an item is found
    boost::function<void (BOARD_ITEM*)> callback;

    void onButtonFindItemClick( wxCommandEvent& event );
    void onButtonFindMarkerClick( wxCommandEvent& event );
    void onButtonCloseClick( wxCommandEvent& event );
    void onClose( wxCloseEvent& event );
};

#endif /* DIALOG_FIND_BASE_H */
