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

#ifndef PCB_COMPONENT_H
#define PCB_COMPONENT_H

#include <netlist_reader/netlist.h>
#include <footprint.h>
#include <memory>


/**
 * PCB-specific extension of COMPONENT that adds footprint support.
 */
class PCB_COMPONENT : public COMPONENT
{
public:
    PCB_COMPONENT( const LIB_ID&            aFPID,
                   const wxString&          aReference,
                   const wxString&          aValue,
                   const KIID_PATH&         aPath,
                   const std::vector<KIID>& aKiids )
        : COMPONENT( aFPID, aReference, aValue, aPath, aKiids )
    {
    }

    virtual ~PCB_COMPONENT() { };

    FOOTPRINT* GetFootprint( bool aRelease = false )
    {
        return ( aRelease ) ? m_footprint.release() : m_footprint.get();
    }

    void SetFootprint( FOOTPRINT* aFootprint );

private:
    /// The #FOOTPRINT loaded for #m_FPID.
    std::unique_ptr<FOOTPRINT>   m_footprint;
};


typedef boost::ptr_vector< PCB_COMPONENT > PCB_COMPONENTS;

#endif  // PCB_COMPONENT_H
