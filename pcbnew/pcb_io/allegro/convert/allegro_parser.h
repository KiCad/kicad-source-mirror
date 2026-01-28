/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <memory>

#include <convert/allegro_db.h>
#include <convert/allegro_stream.h>
#include <convert/allegro_pcb_structs.h>

#include <progress_reporter.h>

namespace ALLEGRO
{

/**
 * Class that parses a single FILE_STREAM into a RAW_BOARD,
 * and handles any state involved in that parsing
 *
 * This only handles converting rawfile stream data into
 * structs that represent a near-verbatim representation of the
 * data, with a few small conversions and conveniences.
 */
class PARSER
{
public:
    PARSER( FILE_STREAM& aStream, PROGRESS_REPORTER* aProgressReporter ) :
        m_stream( aStream ),
        m_progressReporter( aProgressReporter )
    {
    }

    /**
     * When set to true, the parser will stop at the first unknown block, rather
     * than throwing an error.
     *
     * This is mostly useful for debugging, as at least you can dump the blocks
     * and see what they are. But in real life, this would result in a very incomplete
     * board state.
     */
    void EndAtUnknownBlock( bool aEndAtUnknownBlock )
    {
        m_endAtUnknownBlock = aEndAtUnknownBlock;
    }

    std::unique_ptr<BRD_DB> Parse();

private:
    void readObjects( BRD_DB& aDb );

    FILE_STREAM& m_stream;

    bool               m_endAtUnknownBlock = false;
    PROGRESS_REPORTER* m_progressReporter = nullptr;
};

} // namespace ALLEGRO
