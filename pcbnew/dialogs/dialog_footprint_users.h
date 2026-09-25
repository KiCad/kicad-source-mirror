/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/timer.h>

#include <lib_id.h>
#include <symbol_library_query.h>

#include <dialog_footprint_users_base.h>

class KIWAY;


/**
 * Shows the symbols that use a footprint with some switchable match categories.
 */
class DIALOG_FOOTPRINT_USERS : public DIALOG_FOOTPRINT_USERS_BASE
{
public:
    /**
     * @param aParent is the parent window of the dialog.
     * @param aKiway is used to reach the eeschema kiface.
     * @param aFootprint is the ID of the footprint whose users are to be shown.
     * @param aFootprintPinCount is the pin count of this footprint.
     */
    DIALOG_FOOTPRINT_USERS( wxWindow* aParent, KIWAY& aKiway, const LIB_ID& aFootprint, int aFootprintPinCount );

private:
    void onMatchTypeChanged( wxCommandEvent& aEvent );
    void onLoadTimer( wxTimerEvent& aEvent );
    void onUserActivated( wxDataViewEvent& aEvent );
    void onUserRightClick( wxDataViewEvent& aEvent );
    void onUserKeyDown( wxKeyEvent& aEvent );

    /// Show the users for the enabled match categories, querying them when they are not
    /// cached yet.
    void updateUsers();

    /// Return the enabled match categories, as the bit set of FOOTPRINT_USERS_MATCH_CATEGORY.
    int matchCategories() const;

    /// Return true once the symbol libraries are loaded, showing the progress when they are not.
    bool symbolLibrariesReady();

    void showUsers( const FOOTPRINT_USERS_RESULT& aResult );
    void showLoadIndication( bool aShow, float aProgress = 0.0f );

    /// Return the "library:symbol" name of a table row, or an empty string if there is none.
    wxString userSymbolName( int aRow ) const;

    void openSymbolInEditor( const wxString& aSymbolName );
    void copySymbolName( const wxString& aSymbolName );

    KIWAY&  m_kiway;
    LIB_ID  m_footprint;
    int     m_footprintPinCount;
    wxTimer m_loadTimer;
    bool    m_librariesReady;
};
