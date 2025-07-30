/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef SCH_FIND_REPLACE_TOOL_H
#define SCH_FIND_REPLACE_TOOL_H

#include <sch_base_frame.h>
#include <tools/sch_tool_base.h>


/**
 * Handle actions specific to the schematic editor.
 */
class SCH_FIND_REPLACE_TOOL : public wxEvtHandler, public SCH_TOOL_BASE<SCH_BASE_FRAME>
{
public:
    SCH_FIND_REPLACE_TOOL()  :
            SCH_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.FindReplace" ),
            m_foundItemHighlighted( false )
    { }

    ~SCH_FIND_REPLACE_TOOL() { }

    int FindAndReplace( const TOOL_EVENT& aEvent );

    int FindNext( const TOOL_EVENT& aEvent );
    bool HasMatch();
    int ReplaceAndFindNext( const TOOL_EVENT& aEvent );
    int ReplaceAll( const TOOL_EVENT& aEvent );

    int UpdateFind( const TOOL_EVENT& aEvent );

private:
    ///< Set up handlers for various events.
    void setTransitions() override;

    /**
     * Advance the search and returns the next matching item after \a aAfter.
     *
     * @param aScreen Pointer to the screen used for searching
     * @param aAfter Starting match to compare
     * @param aData Search data to compare against or NULL to match the first item found
     * @param reverse Search in reverse (find previous)
     * @return pointer to the next search item found or NULL if nothing found
     */
    SCH_ITEM* nextMatch( SCH_SCREEN* aScreen, SCH_SHEET_PATH* aSheet, SCH_ITEM* aAfter,
                         EDA_SEARCH_DATA& aData, bool reverse );
    EDA_ITEM* getCurrentMatch();

private:
    SCH_ITEM*   m_afterItem = nullptr;
    bool        m_foundItemHighlighted;
    wxTimer     m_wrapAroundTimer;          // A timer during which a subsequent FindNext will
                                            //  result in a wrap-around
};


#endif // SCH_FIND_REPLACE_TOOL_H
