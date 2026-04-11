/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCAD_NETS_H
#define PCAD_NETS_H


#include <pcad/pcad2kicad_common.h>
#include <pcad/pcad_item_types.h>

class wxString;
class XNODE;

namespace PCAD2KICAD {

wxString ConvertNetName( const wxString& aName );

class PCAD_NET_NODE : public wxObject
{
public:
    PCAD_NET_NODE();
    ~PCAD_NET_NODE();

public:
    wxString    m_CompRef;
    wxString    m_PinRef;
};

class PCAD_NET : public wxObject
{
public:
    PCAD_NET( int aNetCode );
    ~PCAD_NET();

    void Parse( XNODE* aNode );

public:
    wxString             m_Name;
    int                  m_NetCode;
    PCAD_NET_NODES_ARRAY m_NetNodes;
};

} // namespace PCAD2KICAD

#endif    // PCAD_NETS_H
