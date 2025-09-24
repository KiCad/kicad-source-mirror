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

#include "context_manager.h"
#include <cstddef>

using namespace U3D;

CONTEXT_MANAGER::CONTEXT_MANAGER() :
        m_symbolCount( CONSTANTS::StaticFull ),
        m_cumulativeCount( CONSTANTS::StaticFull ),
        m_elephant( 0x00001fff ),
        m_maximumSymbolInHistogram( 0x0000ffff ),
        m_arraySizeIncr( 32 )
{
}


void CONTEXT_MANAGER::AddSymbol( uint32_t aContext, uint32_t aSymbol )
{
    if( aContext < CONSTANTS::StaticFull && aContext != CONSTANTS::Context8
        && aSymbol < m_maximumSymbolInHistogram )
    {
        std::vector<uint32_t>& cumulativeCountRef = m_cumulativeCount[aContext];
        std::vector<uint32_t>& symbolCountRef = m_symbolCount[aContext];

        if( cumulativeCountRef.empty() || cumulativeCountRef.size() <= aSymbol )
        {
            std::vector<uint32_t> newCumulativeCount( aSymbol + m_arraySizeIncr, 0 );
            std::vector<uint32_t> newSymbolCount( aSymbol + m_arraySizeIncr, 0 );

            if( cumulativeCountRef.empty() )
            {
                cumulativeCountRef = newCumulativeCount;
                cumulativeCountRef[0] = 1;

                symbolCountRef = newSymbolCount;
                symbolCountRef[0] = 1;
            }
            else
            {
                std::copy( cumulativeCountRef.begin(), cumulativeCountRef.end(),
                            newCumulativeCount.begin() );
                std::copy( symbolCountRef.begin(), symbolCountRef.end(),
                            newSymbolCount.begin() );
                cumulativeCountRef = newCumulativeCount;
                symbolCountRef = newSymbolCount;
            }
        }

        if( cumulativeCountRef[0] >= m_elephant )
        {
            size_t len = cumulativeCountRef.size();
            uint32_t    tempAccum = 0;
            for( int i = (int32_t)len - 1; i >= 0; i-- )
            {
                symbolCountRef[i] >>= 1;
                tempAccum += symbolCountRef[i];
                cumulativeCountRef[i] = tempAccum;
            }
            symbolCountRef[0]++;
            cumulativeCountRef[0]++;
        }

        symbolCountRef[aSymbol]++;
        for( uint32_t i = 0; i <= aSymbol; i++ )
        {
            cumulativeCountRef[i]++;
        }
    }
}


uint32_t CONTEXT_MANAGER::GetSymbolFrequency( uint32_t aContext, uint32_t aSymbol )
{
    uint32_t rValue = 1;
    if( aContext < CONSTANTS::StaticFull && aContext != CONSTANTS::Context8 )
    {
        rValue = 0;
        if( !m_symbolCount[aContext].empty() && aSymbol < m_symbolCount[aContext].size() )
        {
            rValue = m_symbolCount[aContext][aSymbol];
        }
        else if( aSymbol == 0 )
        {
            rValue = 1;
        }
    }
    return rValue;
}


uint32_t CONTEXT_MANAGER::GetCumulativeSymbolFrequency( uint32_t aContext, uint32_t aSymbol )
{
    uint32_t rValue = aSymbol - 1;
    if( aContext < CONSTANTS::StaticFull && aContext != CONSTANTS::Context8 )
    {
        rValue = 0;
        if( !m_cumulativeCount[aContext].empty() )
        {
            if( aSymbol < m_cumulativeCount[aContext].size() )
            {
                rValue = m_cumulativeCount[aContext][0] - m_cumulativeCount[aContext][aSymbol];
            }
            else
            {
                rValue = m_cumulativeCount[aContext][0];
            }
        }
    }
    return rValue;
}


uint32_t CONTEXT_MANAGER::GetTotalSymbolFrequency( uint32_t aContext )
{
    if( aContext < CONSTANTS::StaticFull && aContext != CONSTANTS::Context8 )
    {
        uint32_t rValue = 1;
        if( !m_cumulativeCount[aContext].empty() )
        {
            rValue = m_cumulativeCount[aContext][0];
        }
        return rValue;
    }

    if( aContext == CONSTANTS::Context8 )
    {
        return 256;
    }

    return aContext - CONSTANTS::StaticFull;
}


uint32_t CONTEXT_MANAGER::GetSymbolFromFrequency( uint32_t aContext, uint32_t aSymbolFrequency )
{
    uint32_t rValue = 0;
    if( aContext < CONSTANTS::StaticFull && aContext != CONSTANTS::Context8 )
    {
        rValue = 0;
        if( !m_cumulativeCount[aContext].empty() && aSymbolFrequency != 0
            && m_cumulativeCount[aContext][0] >= aSymbolFrequency )
        {
            for( size_t i = 0; i < m_cumulativeCount[aContext].size(); i++ )
            {
                if( GetCumulativeSymbolFrequency( aContext, (uint32_t) i ) <= aSymbolFrequency )
                {
                    rValue = (uint32_t) i;
                }
                else
                {
                    break;
                }
            }
        }
    }
    else
    {
        rValue = aSymbolFrequency + 1;
    }
    return rValue;
}