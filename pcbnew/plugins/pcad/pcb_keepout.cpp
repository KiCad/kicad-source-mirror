/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <pcad/pcb_keepout.h>

#include <common.h>
#include <xnode.h>

#include <wx/gdicmn.h>
#include <wx/string.h>

namespace PCAD2KICAD {

PCB_KEEPOUT::PCB_KEEPOUT( PCB_CALLBACKS*    aCallbacks,
                          BOARD*            aBoard,
                          int               aPCadLayer ) :
    PCB_POLYGON( aCallbacks, aBoard, aPCadLayer )
{
    m_objType   = wxT( 'K' );
}


PCB_KEEPOUT::~PCB_KEEPOUT()
{
}


bool PCB_KEEPOUT::Parse( XNODE*          aNode,
                         const wxString& aDefaultMeasurementUnit,
                         const wxString& aActualConversion )
{
    XNODE*          lNode;

    lNode = FindNode( aNode, wxT( "pcbPoly" ) );

    if( lNode )
    {
        // retrieve keepOut outline
        FormPolygon( lNode, &m_outline, aDefaultMeasurementUnit, aActualConversion );

        m_positionX = m_outline[0]->x;
        m_positionY = m_outline[0]->y;
    }
    else
    {
        return false;
    }

    return true;
}

} // namespace PCAD2KICAD
