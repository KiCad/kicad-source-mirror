/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef HOTKEY_STORE__H
#define HOTKEY_STORE__H

#include <hotkeys_basic.h>
#include <tool/tool_action.h>

class TOOL_MANAGER;


struct HOTKEY
{
    std::vector<TOOL_ACTION*> m_Actions;
    int                       m_EditKeycode;

    HOTKEY() :
            m_EditKeycode( 0 )
    { }

    HOTKEY( TOOL_ACTION* aAction ) :
            m_EditKeycode( aAction->GetHotKey() )
    {
        m_Actions.push_back( aAction );
    }
};


struct HOTKEY_SECTION
{
    wxString            m_SectionName;   // The displayed, translated, name of the section
    std::vector<HOTKEY> m_HotKeys;
};


/**
 * A class that contains a set of hotkeys, arranged into "sections"
 * and provides some book-keeping functions for them.
 */
class HOTKEY_STORE
{
public:

    /**
     * Construct a HOTKEY_STORE from a list of hotkey sections
     *
     * @param aHotkeys the hotkey configs that will be managed by this store.
     */
    HOTKEY_STORE();

    void Init( std::vector<TOOL_MANAGER*> aToolManagerList, bool aIncludeGestures );

    static wxString GetAppName( TOOL_ACTION* aAction );
    static wxString GetSectionName( TOOL_ACTION* aAction );

    /**
     * Get the list of sections managed by this store
     */
    std::vector<HOTKEY_SECTION>& GetSections();

    /**
     * Persist all changes to hotkeys in the store to the underlying
     * data structures.
     */
    void SaveAllHotkeys();

    /**
     * Reset every hotkey in the store to the default values
     */
    void ResetAllHotkeysToDefault();

    /**
     * Resets every hotkey to the original values.
     */
    void ResetAllHotkeysToOriginal();

    /**
     * Check whether the given key conflicts with anything in this store.
     *
     * @param aAction - the action the key is proposed to be assigned to.  Only conflicts
     *                  within the same section will be flagged.
     * @param aKey - key to check
     * @param aConflict - outparam getting the section this one conflicts with
     */
    bool CheckKeyConflicts( TOOL_ACTION* aAction, long aKey, HOTKEY** aConflict );

private:
    std::vector<TOOL_MANAGER*>  m_toolManagers;
    std::vector<HOTKEY_SECTION> m_hk_sections;
};

#endif // HOTKEY_STORE__H