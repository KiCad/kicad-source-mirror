/*
 * Copyright (C) 2025 Mark Roszko <mark.roszko@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2007 Ecma International (original Java source)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * License info found here:
 * https://www.loc.gov/preservation/digital/formats/fdd/fdd000491.shtml
 */

#pragma once

#include <cstdint>
#include <vector>
#include <exporters/u3d/context_manager.h>
#include <exporters/u3d/data_block.h>
#include <memory>
#include <string>

namespace U3D
{
/**
 * @brief Implements the bit stream writer functionality.
 *
 * @details
 * - All uncompressed writes are converted to unsigned integers and broken down into a sequence of U8 values
 *   that are written with the private method WriteSymbol in the static context Context8.
 * - All compressed writes are for unsigned integers and are passed through to the private method WriteSymbol
 *   with the associated context.
 */
class BIT_STREAM_WRITER
{
public:
    BIT_STREAM_WRITER();

    void WriteU8( uint8_t aValue );

    void WriteU16( uint16_t uValue );

    void WriteU32( uint32_t uValue );

    void WriteU64( uint64_t uValue );

    void WriteI32( int32_t iValue );

    void WriteF32( float fValue );

    void WriteF64( double aValue );

    void WriteCompressedU32( uint32_t aContext, uint32_t uValue );

    void WriteCompressedU16( uint32_t aContext, uint16_t uValue );

    void WriteCompressedU8( uint32_t aContext, uint8_t uValue );

    void WriteString( const std::string& aStr );

    void AlignToByte();

    void AlignTo4Byte();

    /**
     * @brief Returns the number of bits written.
     * @param rCount Reference to an integer where the bit count will be stored.
     */
    void GetBitCount( int32_t& rCount );

    void WriteDataBlock( std::shared_ptr<DATA_BLOCK> b );

    std::shared_ptr<DATA_BLOCK> GetDataBlock();

private:
    /*
     * Stores the local values of the data to the data array
     */
    void putLocal();

    /*
     * Write the given symbol to the datablock in the specified context.
     * rEscape returns as false if the symbol was written successfully.
     * rEscape will return true when writing in dynamically compressed
     * contexts when the symbol to write has not appeared yet in the
     * context's histogram. In this case, the escape symbol, 0, is
     * written.
     */
    void writeSymbol( uint32_t aContext, uint32_t aSymbol, bool& rEscape );

    /**
     * changes the ordering of an 8 bit value so that the first
     * 4 bits become the last 4 bits and the last 4 bits become
     * the first 4. E.g. abcdefgh -> efghabcd
     */
    void swapBits8( uint32_t& rValue );

    void writeBit( uint32_t aBit );

    /**
     * @brief Updates the values of the datablock stored in dataLocal and dataLocalNext
     * to the next values in the datablock.
     */
    void incrementPosition();

    /**
     * @brief Checks that the array allocated for writing is large enough.
     *        Reallocates if necessary.
     */
    void checkPosition();


    std::unique_ptr<CONTEXT_MANAGER> m_contextManager;

    /**
     * @brief high and low are the upper and lower limits on the probability
     */
    uint32_t              m_high;
    uint32_t              m_low;

    /**
     * @brief stores the number of bits of underflow cause dby the limited range of high and low
     */
    uint32_t              m_underflow;
    /**
     * @brief Indicates if a compressed value was written.
     *
     * When the datablock is retrieved, a 32-bit 0 is written to reset the values of
     * high, low, and underflow.
     */
    bool                  m_compressed;

    std::vector<uint32_t> m_data;
    int32_t               m_dataPosition;

    /**
     * The local value of the data corresponding to m_dataPosition
     */
    uint32_t              m_dataLocal;

    /**
     * The value of the data after m_dataLocal
     */
    uint32_t              m_dataLocalNext;

    /**
     * The offset into data local that the next write occur
     */
    int32_t               m_dataBitOffset;

    const int32_t         m_dataSizeIncrement = 0x000023F8;
};
} // namespace U3D