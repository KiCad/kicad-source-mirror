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

#pragma once

#include <memory>

#include <allegro_pcb_structs.h>

#include <board.h>
#include <reporter.h>
#include <progress_reporter.h>


class BOARD;

namespace ALLEGRO
{

/**
 * Class that builds a KiCad board from a RAW_BOARD
 * (= FILE_HEADER + STRINGS + OBJECTS + bookkeeping)
 */
class BOARD_BUILDER
{
public:
    BOARD_BUILDER(const RAW_BOARD& aRawBoard, BOARD& aBoard, REPORTER& aReporter, PROGRESS_REPORTER* aProgressReporter );

    bool BuildBoard();

private:
    std::unique_ptr<FOOTPRINT> buildFootprint( const BLK_0x2D& aFpInstance );

    const RAW_BOARD&   m_rawBoard;
    BOARD&             m_board;
    REPORTER&          m_reporter;
    PROGRESS_REPORTER* m_progressReporter;
};

} // namespace ALLEGRO
