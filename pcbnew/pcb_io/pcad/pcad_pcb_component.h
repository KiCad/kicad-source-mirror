/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PCB_COMPONENT_H_
#define PCB_COMPONENT_H_

#include <pcad/pcad2kicad_common.h>
#include <pcad/pcad_item_types.h>
#include <pcad/pcad_callbacks.h>

#include <kiid.h>
#include <layer_ids.h>
#include <wx/object.h>

class BOARD;
class FOOTPRINT;
class wxString;
class wxRealPoint;

namespace PCAD2KICAD {

// basic parent class for PCB objects
class PCAD_PCB_COMPONENT : public wxObject
{
public:
    PCAD_PCB_COMPONENT( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard );
    ~PCAD_PCB_COMPONENT();

    virtual void SetPosOffset( int aX_offs, int aY_offs );
    virtual void Flip();
    virtual void AddToBoard( FOOTPRINT* aFootprint = nullptr ) = 0;

    PCB_LAYER_ID GetKiCadLayer() const { return m_callbacks->GetKiCadLayer( m_PCadLayer ); }

    int          GetNetCode( const wxString& aNetName ) const
    {
        return m_callbacks->GetNetCode( aNetName );
    }

public:
    char         m_ObjType;
    int          m_PCadLayer;
    PCB_LAYER_ID m_KiCadLayer;
    KIID         m_Uuid;
    int          m_PositionX;
    int          m_PositionY;
    EDA_ANGLE    m_Rotation;
    TTEXTVALUE   m_Name;             // name has also private positions, rotations and so on....
    wxString     m_Net;
    int          m_NetCode;
    wxString     m_CompRef;          // internal usage for XL parsing
    wxString     m_PatGraphRefName;  // internal usage for XL parsing

protected:
    PCAD_CALLBACKS* m_callbacks;
    BOARD*          m_board;
};

} // namespace PCAD2KICAD

#endif    // PCB_COMPONENT_H_
