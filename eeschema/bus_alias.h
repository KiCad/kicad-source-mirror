/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _BUS_ALIAS_H
#define _BUS_ALIAS_H

#include <memory>
#include <wx/string.h>
#include <wx/arrstr.h>


class SCH_SCREEN;


class BUS_ALIAS
{
public:
    BUS_ALIAS( SCH_SCREEN* aParent = nullptr );

    ~BUS_ALIAS();

    std::shared_ptr< BUS_ALIAS > Clone() const
    {
        return std::make_shared< BUS_ALIAS >( *this );
    }

    wxString GetName()
    {
        return m_name;
    }

    void SetName( const wxString& aName )
    {
        m_name = aName;
    }

    void ClearMembers()
    {
        m_members.clear();
    }

    void AddMember( const wxString& aName )
    {
        m_members.push_back( aName );
    }

    int GetMemberCount()
    {
        return m_members.size();
    }

    wxArrayString& Members()
    {
        return m_members;
    }

    bool Contains( const wxString& aName );

    SCH_SCREEN* GetParent()
    {
        return m_parent;
    }

    void SetParent( SCH_SCREEN* aParent )
    {
        m_parent = aParent;
    }

protected:

    wxString m_name;

    wxArrayString m_members;

    /**
     * The bus alias editor dialog can edit aliases from all open sheets.
     * This means we have to store a reference back to our parent so that
     * the dialog can update the parent if aliases are changed or removed.
     */
    SCH_SCREEN* m_parent;
};

#endif
