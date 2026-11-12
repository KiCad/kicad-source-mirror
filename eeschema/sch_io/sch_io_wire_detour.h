/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>

#include <geometry/seg.h>
#include <math/vector2d.h>

/**
 * Keeps wires of one net from touching pins, junctions and wire ends of other nets on import.
 *
 * Other EDA tools join a pin or wire end to a wire only where the file says so, whereas a pin or wire end lying on a
 * wire always joins it here.  The importer describes one sheet with plain geometry and an opaque net identity per
 * item, then asks for each wire to be replaced by a detour around the foreign items lying on it.
 */
class SCH_IO_WIRE_DETOUR
{
public:
    struct RESULT
    {
        /// The wire itself when nothing lies on it, otherwise the pieces of the detoured wire in order
        std::vector<SEG> Pieces;

        /// A foreign item lies on the wire and no safe detour was found, the wire is left as it was
        bool Failed = false;
    };

    /**
     * @return a net identity that no other item shares, for an item that belongs to no net.
     */
    int AnonymousNet() { return --m_anonymousNets; }

    /**
     * Register a wire as an obstacle for the wires of other nets.
     */
    void AddWire( const SEG& aWire, int aNet );

    /**
     * Register a pin or junction.
     */
    void AddContact( const VECTOR2I& aPos, int aNet );

    /**
     * Detour a wire around foreign items lying on it and register the result as the wire's geometry.
     *
     * Wire ends count as items.  A detour never lands on another net's item or wire, and is not made where a branch
     * of the same net joins the replaced stretch.
     */
    RESULT Route( const SEG& aWire, int aNet );

    /**
     * @return true if a pin, junction or wire end of another net sits exactly at a point.
     */
    bool TouchesOtherNet( const VECTOR2I& aPos, int aNet ) const;

private:
    struct WIRE
    {
        SEG Seg;
        int Net;
    };

    struct CONTACT
    {
        VECTOR2I Pos;
        int      Net;
    };

    std::vector<CONTACT> contacts() const;

    /// True if a detour piece would join another net
    bool joinsOtherNet( const SEG& aPiece, const SEG& aWire, int aNet, const std::vector<CONTACT>& aContacts ) const;

    /// Detour the stretch of a wire between two points, or an empty vector if there is no safe place for it
    std::vector<SEG> detour( const SEG& aWire, const VECTOR2I& aEnter, const VECTOR2I& aLeave, int aNet,
                             const std::vector<CONTACT>& aContacts ) const;

    std::vector<WIRE>    m_wires;
    std::vector<CONTACT> m_contacts;
    int                  m_anonymousNets = 0;
};
