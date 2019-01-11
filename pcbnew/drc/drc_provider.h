/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see change_log.txt for contributors.
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


#ifndef DRC_PROVIDER__H
#define DRC_PROVIDER__H

#include <class_board.h>
#include <class_marker_pcb.h>

#include <drc/drc_marker_factory.h>

#include <functional>


/**
 * Class that represents a DRC "provider" which runs some
 * DRC functions over a #BOARD and spits out #PCB_MARKERs as needed.
 */
class DRC_PROVIDER
{
public:
    /**
     * A callable that can handle a single generated PCB_MARKER
     */
    using MARKER_HANDLER = std::function<void( MARKER_PCB* )>;

    // This class can be handled by base pointer
    virtual ~DRC_PROVIDER()
    {
    }

    /**
     * Runs this provider against the given PCB with
     * configured options (if any).
     *
     * Note: Board is non-const, as some DRC functions modify the board
     * (e.g. zone fill or polygon coalescing)
     */
    virtual bool RunDRC( BOARD& aBoard ) const = 0;

protected:
    DRC_PROVIDER( const DRC_MARKER_FACTORY& aMarkerMaker, MARKER_HANDLER aMarkerHandler );

    /**
     * Access to the stored reference to a marker constructor
     */
    const DRC_MARKER_FACTORY& GetMarkerFactory() const;

    /**
     * Pass a given marker to the marker handler
     */
    void HandleMarker( std::unique_ptr<MARKER_PCB> aMarker ) const;

private:

    /// A marker generator to make markers in the right context
    const DRC_MARKER_FACTORY& m_marker_factory;

    /// The handler for any generated markers
    MARKER_HANDLER m_marker_handler;
};

#endif // DRC_PROVIDER__H