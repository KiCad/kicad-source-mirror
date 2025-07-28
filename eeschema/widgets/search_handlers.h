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

#ifndef SEARCH_HANDLERS_H
#define SEARCH_HANDLERS_H

#include <functional>
#include <widgets/search_pane.h>

class SCH_ITEM;
class SCH_EDIT_FRAME;

struct SCH_SEARCH_HIT
{
    SCH_ITEM*       item;
    SCH_SHEET_PATH* sheetPath;
};

class SCH_SEARCH_HANDLER : public SEARCH_HANDLER
{
public:
    SCH_SEARCH_HANDLER( const wxString& aName, SCH_EDIT_FRAME* aFrame ) :
            SEARCH_HANDLER( aName ),
            m_frame( aFrame )
    {}

    void ActivateItem( long aItemRow ) override;

    wxString GetResultCell( int aRow, int aCol ) override
    {
        if( m_frame->IsClosing() )
            return wxEmptyString;

        if( aRow >= static_cast<int>( m_hitlist.size() ) )
            return wxEmptyString;

        const SCH_SEARCH_HIT& hit = m_hitlist[aRow];

        if( !hit.item )
            return wxEmptyString;

        return getResultCell( hit, aCol );
    }

    void FindAll( const std::function<bool( SCH_ITEM*, SCH_SHEET_PATH* )>& aCollector );
    void Sort( int aCol, bool aAscending, std::vector<long>* aSelection ) override;
    void SelectItems( std::vector<long>& aItemRows ) override;

protected:
    virtual wxString getResultCell( const SCH_SEARCH_HIT& hit, int aCol ) = 0;

protected:
    SCH_EDIT_FRAME*             m_frame;
    std::vector<SCH_SEARCH_HIT> m_hitlist;
};

class SYMBOL_SEARCH_HANDLER : public SCH_SEARCH_HANDLER
{
public:
    SYMBOL_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame );

    int Search( const wxString& aQuery ) override;

protected:
    wxString getResultCell( const SCH_SEARCH_HIT& aHit, int aCol ) override;
};

class POWER_SEARCH_HANDLER : public SCH_SEARCH_HANDLER
{
public:
    POWER_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame );

    int Search( const wxString& aQuery ) override;

protected:
    wxString getResultCell( const SCH_SEARCH_HIT& aHit, int aCol ) override;
};

class TEXT_SEARCH_HANDLER : public SCH_SEARCH_HANDLER
{
public:
    TEXT_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame );

    int Search( const wxString& aQuery ) override;

protected:
    wxString getResultCell( const SCH_SEARCH_HIT& hit, int aCol ) override;
};

class LABEL_SEARCH_HANDLER : public SCH_SEARCH_HANDLER
{
public:
    LABEL_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame );

    int Search( const wxString& aQuery ) override;

protected:
    wxString getResultCell( const SCH_SEARCH_HIT& hit, int aCol ) override;
};

#endif
