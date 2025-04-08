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

#include <wx/log.h>
#include <wx/regex.h>
#include <wx/string.h>

enum class EDA_SEARCH_MATCH_MODE
{
    PLAIN,
    WHOLEWORD,
    WILDCARD,
    REGEX,
    PERMISSIVE
};

struct EDA_SEARCH_DATA
{
    wxString         findString;
    wxString         replaceString;

    mutable wxRegEx  regex;
    mutable wxString regex_string;

    bool             searchAndReplace;
    bool             searchAllFields;
    bool             searchMetadata;

    bool                  matchCase;
    bool                  markersOnly;
    EDA_SEARCH_MATCH_MODE matchMode;

    EDA_SEARCH_DATA() :
            findString(),
            replaceString(),
            searchAndReplace( false ),
            searchAllFields( false ),
            searchMetadata( false ),
            matchCase( false ),
            markersOnly( false ),
            matchMode( EDA_SEARCH_MATCH_MODE::PLAIN )
    {
    }

    // Need an explicit copy constructor because wxRegEx is not copyable
    EDA_SEARCH_DATA( const EDA_SEARCH_DATA& other ) :
            findString( other.findString ),
            replaceString( other.replaceString ),
            regex_string( other.regex_string ),
            searchAndReplace( other.searchAndReplace ),
            searchAllFields( other.searchAllFields ),
            searchMetadata( other.searchMetadata ),
            matchCase( other.matchCase ),
            markersOnly( other.markersOnly ),
            matchMode( other.matchMode )
    {
        if( matchMode == EDA_SEARCH_MATCH_MODE::REGEX )
        {
            wxLogNull noLogs;
            regex.Compile( findString, matchCase ? wxRE_DEFAULT : wxRE_ICASE );
        }
    }

    virtual ~EDA_SEARCH_DATA() {}
};

struct SCH_SEARCH_DATA : public EDA_SEARCH_DATA
{
    bool searchAllPins;
    bool searchCurrentSheetOnly;
    bool searchSelectedOnly;
    bool searchNetNames;

    bool replaceReferences;

    SCH_SEARCH_DATA() :
            EDA_SEARCH_DATA(),
            searchAllPins( false ),
            searchCurrentSheetOnly( false ),
            searchSelectedOnly( false ),
            searchNetNames( false ),
            replaceReferences( false )
    {
    }
};

#endif
