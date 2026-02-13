/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
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
 * Parses a .brd header, taking care of any version-specific differences.
 *
 * Encapsulates any state and context needed to parse the header.
 */
class HEADER_PARSER
{
public:
    HEADER_PARSER( FILE_STREAM& aStream ) :
            m_stream( aStream ),
            m_fmtVer( FMT_VER::V_UNKNOWN )
    {
    }

    std::unique_ptr<FILE_HEADER> ParseHeader();

    /**
     * Get the parsed format version.
     *
     * This is only valid after ParseHeader() has been called successfully.
     */
    FMT_VER GetFormatVersion() const { return m_fmtVer; }

    /**
     * Determine the format version from the magic number.
     */
    static FMT_VER FormatFromMagic( uint32_t aMagic );

private:
    FILE_STREAM& m_stream;
    FMT_VER      m_fmtVer;
};


/**
 * The block parser is responsible for parsing individual blocks of data from the file stream.
 *
 * Most blocks don't need much context to parse in a binary sense, but most need to know the
 * version, and one (0x27) needs to know where the end of the block is.
 */
class BLOCK_PARSER
{
public:
    BLOCK_PARSER( FILE_STREAM& aStream, FMT_VER aVer, size_t aX27End = 0 ) :
            m_stream( aStream ),
            m_ver( aVer ),
            m_x27_end( aX27End )
    {
    }

    /**
     * Parse one block from the stream, returning a BLOCK_BASE representing the raw data of the block.
     *
     * @param aStream The stream to read from, positioned at the start of the block
     *                (i.e. the next byte to read is the block type).
     * @param aEndOfObjectsMarker This is set to true if we encounter the end of objects marker.
     */
    std::unique_ptr<BLOCK_BASE> ParseBlock( bool& aEndOfObjectsMarker );

private:
    FILE_STREAM&  m_stream;
    const FMT_VER m_ver;

    /**
     * To parse an 0x27 block, we need to know where the end of the block is in the stream.
     * In a .brd file, this is in the header.
     */
    const size_t m_x27_end;
};


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
