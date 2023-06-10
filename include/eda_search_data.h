/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2023 KiCad Developers, see change_log.txt for contributors.
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

#ifndef EDA_ITEM_SEARCH_H
#define EDA_ITEM_SEARCH_H

#include <wx/string.h>

enum class EDA_SEARCH_MATCH_MODE
{
    PLAIN,
    WHOLEWORD,
    WILDCARD,
    PERMISSIVE
};

struct EDA_SEARCH_DATA
{
    wxString findString;
    wxString replaceString;

    bool     searchAndReplace;

    bool                  matchCase;
    bool                  markersOnly;
    EDA_SEARCH_MATCH_MODE matchMode;

    EDA_SEARCH_DATA() :
            findString(),
            replaceString(),
            searchAndReplace( false ),
            matchCase( false ),
            markersOnly( false ),
            matchMode( EDA_SEARCH_MATCH_MODE::PLAIN )
    {
    }

    virtual ~EDA_SEARCH_DATA() {}
};

struct SCH_SEARCH_DATA : public EDA_SEARCH_DATA
{
    bool searchAllFields;
    bool searchAllPins;
    bool searchCurrentSheetOnly;
    bool searchSelectedOnly;

    bool replaceReferences;

    SCH_SEARCH_DATA() :
            EDA_SEARCH_DATA(),
            searchAllFields( false ),
            searchAllPins( false ),
            searchCurrentSheetOnly( false ),
            searchSelectedOnly( false ),
            replaceReferences( false )
    {
    }
};

#endif
