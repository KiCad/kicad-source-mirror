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

#include <cmath>
#include <cstring>
#include "bit_stream_writer.h"
#include "constants.h"

using namespace U3D;

BIT_STREAM_WRITER::BIT_STREAM_WRITER() :
        m_contextManager( nullptr ),
        m_high( 0x0000FFFF ),
        m_low( 0 ),
        m_underflow( 0 ),
        m_compressed( false ),
        m_data(),
        m_dataPosition( 0 ),
        m_dataLocal( 0 ),
        m_dataLocalNext( 0 ),
        m_dataBitOffset( 0 )
{
    m_contextManager = std::make_unique<CONTEXT_MANAGER>();
    m_data = std::vector<uint32_t>( m_dataSizeIncrement );
}


void BIT_STREAM_WRITER::WriteU8( uint8_t aValue )
{
    uint32_t symbol = (uint32_t) aValue;
    swapBits8( symbol );
    bool escape = false;
    writeSymbol( CONSTANTS::Context8, symbol, escape );
}


void BIT_STREAM_WRITER::WriteU16( uint16_t uValue )
{
    WriteU8( (uint8_t) ( 0x00FF & uValue ) );
    WriteU8( (uint8_t) ( 0x00FF & ( uValue >> 8 ) ) );
}


void BIT_STREAM_WRITER::WriteU32( uint32_t uValue )
{
    WriteU16( (uint16_t) ( 0x0000FFFF & uValue ) );
    WriteU16( (uint16_t) ( 0x0000FFFF & ( uValue >> 16 ) ) );
}


void BIT_STREAM_WRITER::WriteU64( uint64_t uValue )
{
    WriteU32( (uint32_t) ( 0x00000000FFFFFFFF & uValue ) );
    WriteU32( (uint32_t) ( 0x00000000FFFFFFFF & ( uValue >> 32 ) ) );
}


void BIT_STREAM_WRITER::WriteI32( int32_t iValue )
{
    WriteU32( (uint32_t) iValue );
}


void BIT_STREAM_WRITER::WriteF32( float fValue )
{
    uint32_t uValue;
    std::memcpy( &uValue, &fValue, sizeof( fValue ) );
    WriteU32( uValue );
}


void BIT_STREAM_WRITER::WriteF64( double aValue )
{
    uint64_t uValue;
    std::memcpy( &uValue, &aValue, sizeof( aValue ) );
    WriteU64( uValue );
}


void BIT_STREAM_WRITER::WriteCompressedU32( uint32_t aContext, uint32_t uValue )
{
    m_compressed = true;
    bool escape = false;
    if( ( aContext != 0 ) && ( aContext < CONSTANTS::MaxRange ) )
    {
        writeSymbol( aContext, uValue, escape );
        if( escape == true )
        {
            WriteU32( uValue );
            //Assuming symbol is a class member, and is in scope.
            //If symbol is not in scope, you will have to find the correct
            //symbol value to add to the context manager.
            m_contextManager->AddSymbol( aContext, uValue + 1 );
        }
    }
    else
    {
        WriteU32( uValue );
    }
}


void BIT_STREAM_WRITER::WriteCompressedU16( uint32_t aContext, uint16_t uValue )
{
    m_compressed = true;
    bool escape = false;
    if( ( aContext != 0 ) && ( aContext < CONSTANTS::MaxRange ) )
    {
        writeSymbol( aContext, uValue, escape );
        if( escape == true )
        {
            WriteU16( uValue );
            //Assuming symbol is a class member, and is in scope.
            //If symbol is not in scope, you will have to find the correct
            //symbol value to add to the context manager.
            m_contextManager->AddSymbol( aContext, uValue + 1 );
        }
    }
    else
    {
        WriteU16( uValue );
    }
}


void BIT_STREAM_WRITER::WriteCompressedU8( uint32_t aContext, uint8_t uValue )
{
    m_compressed = true;
    bool escape = false;
    if( ( aContext != 0 ) && ( aContext < CONSTANTS::MaxRange ) )
    {
        writeSymbol( aContext, uValue, escape );
        if( escape == true )
        {
            WriteU8( uValue );
            //Assuming symbol is a class member, and is in scope.
            //If symbol is not in scope, you will have to find the correct
            //symbol value to add to the context manager.
            m_contextManager->AddSymbol( aContext, uValue + 1 );
        }
    }
    else
    {
        WriteU8( uValue );
    }
}


void BIT_STREAM_WRITER::WriteString( const std::string& aStr )
{
    WriteU16( (uint16_t) aStr.length() );

    for( size_t i = 0; i < aStr.length(); i++ )
        WriteU8( aStr[i] );
}


void BIT_STREAM_WRITER::AlignToByte()
{
    int32_t uBitCount = 0;
    GetBitCount( uBitCount );

    uBitCount = ( 8 - ( uBitCount & 7 ) ) & 7;
    m_dataBitOffset += uBitCount;
    if( m_dataBitOffset >= 32 )
    {
        m_dataBitOffset -= 32;
        incrementPosition();
    }
}


void BIT_STREAM_WRITER::AlignTo4Byte()
{
    if( m_dataBitOffset > 0 )
    {
        m_dataBitOffset = 0;
        incrementPosition();
    }
}

void BIT_STREAM_WRITER::GetBitCount( int32_t& rCount )
{
    rCount = ( m_dataPosition << 5 ) + m_dataBitOffset;
}


void BIT_STREAM_WRITER::WriteDataBlock( std::shared_ptr<DATA_BLOCK> b )
{
    int dataSize = static_cast<int>( std::ceil( static_cast<double>( b->GetDataSize() ) / 4.0 ) );
    int metaDataSize =
            static_cast<int>( std::ceil( static_cast<double>( b->GetMetaDataSize() ) / 4.0 ) );

    WriteU32( b->GetBlockType() );
    WriteU32( b->GetDataSize() );
    WriteU32( b->GetMetaDataSize() );

    for( int i = 0; i < dataSize; i++ )
    {
        WriteU32( b->GetData()[i] );
    }

    for( int i = 0; i < metaDataSize; i++ )
    {
        WriteU32( b->GetMetaData()[i] );
    }
}


std::shared_ptr<DATA_BLOCK> BIT_STREAM_WRITER::GetDataBlock()
{
    if( m_compressed )
    {
        WriteU32( 0 );
    }
    AlignToByte();
    uint32_t                      numBytes = ( m_dataPosition << 2 ) + ( m_dataBitOffset >> 3 );
    std::shared_ptr<DATA_BLOCK> ret = std::make_shared<DATA_BLOCK>();
    putLocal();
    ret->SetDataSize( numBytes );
    ret->SetData( m_data );

    return ret;
}


void BIT_STREAM_WRITER::putLocal()
{
    m_data[m_dataPosition] = m_dataLocal;
    m_data[m_dataPosition + 1] = m_dataLocalNext;
}


void BIT_STREAM_WRITER::writeSymbol( uint32_t aContext, uint32_t aSymbol, bool& rEscape )
{
    aSymbol++;
    rEscape = false;
    uint32_t totalCumFreq = 0;
    uint32_t symbolCumFreq = 0;
    uint32_t symbolFreq = 0;

    totalCumFreq = m_contextManager->GetTotalSymbolFrequency( aContext );
    symbolCumFreq = m_contextManager->GetCumulativeSymbolFrequency( aContext, aSymbol );
    symbolFreq = m_contextManager->GetSymbolFrequency( aContext, aSymbol );

    if( 0 == symbolFreq )
    {
        aSymbol = 0;
        symbolCumFreq = m_contextManager->GetCumulativeSymbolFrequency( aContext, aSymbol );
        symbolFreq = m_contextManager->GetSymbolFrequency( aContext, aSymbol );
    }

    if( 0 == aSymbol )
    {
        rEscape = true;
    }

    uint32_t range = m_high + 1 - m_low;
    m_high = m_low - 1 + range * ( symbolCumFreq + symbolFreq ) / totalCumFreq;
    m_low = m_low + range * symbolCumFreq / totalCumFreq;

    m_contextManager->AddSymbol( aContext, aSymbol );


    //write bits
    uint32_t bit = m_low >> 15;

    //uint32_t highmask = m_high & U3DConstants::HalfMask;
    //uint32_t lowmask = m_low & U3DConstants::HalfMask;

    while( ( m_high & CONSTANTS::HalfMask ) == ( m_low & CONSTANTS::HalfMask ) )
    {
        m_high &= ~CONSTANTS::HalfMask;
        m_high += m_high + 1;
        writeBit( bit );
        while( m_underflow > 0 )
        {
            m_underflow--;
            writeBit( ( ~bit ) & 1 );
        }
        m_low &= ~CONSTANTS::HalfMask;
        m_low += m_low;
        bit = m_low >> 15;
    }

    //check for underflow
    // Underflow occurs when the values stored in this.high and
    // this.low differ only in the most significant bit.
    // The precision of the variables is not large enough to predict
    // the next symbol.
    while( ( 0 == ( m_high & CONSTANTS::QuarterMask ) )
           && ( CONSTANTS::QuarterMask == ( m_low & CONSTANTS::QuarterMask ) ) )
    {
        m_high &= ~CONSTANTS::HalfMask;
        m_high <<= 1;
        m_low <<= 1;
        m_high |= CONSTANTS::HalfMask;
        m_high |= 1;
        m_low &= ~CONSTANTS::HalfMask;
        m_underflow++;
    }
}


void BIT_STREAM_WRITER::swapBits8( uint32_t& rValue )
{
    rValue = ( CONSTANTS::Swap8[( rValue ) & 0xf] << 4 ) | ( CONSTANTS::Swap8[( rValue ) >> 4] );
}


void BIT_STREAM_WRITER::writeBit( uint32_t aBit )
{
    uint32_t mask = 1;
    aBit &= mask;
    m_dataLocal &= ~( mask << m_dataBitOffset );
    m_dataLocal |= ( aBit << m_dataBitOffset );
    m_dataBitOffset += 1;
    if( m_dataBitOffset >= 32 )
    {
        m_dataBitOffset -= 32;
        incrementPosition();
    }
}


void BIT_STREAM_WRITER::incrementPosition()
{
    m_dataPosition++;
    checkPosition();
    m_data[m_dataPosition - 1] = m_dataLocal;
    m_dataLocal = m_dataLocalNext;
    m_dataLocalNext = m_data[m_dataPosition + 1];
}


void BIT_STREAM_WRITER::checkPosition()
{
    if( static_cast<size_t>( m_dataPosition + 2 ) > m_data.size() )
    {
        m_data.resize( m_dataPosition + 2 + m_dataSizeIncrement );
    }
}