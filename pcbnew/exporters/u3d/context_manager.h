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
#include <exporters/u3d/constants.h>

namespace U3D
{
/**
 * @brief Class for managing static and dynamic contexts used in reading and writing compressed data.
 *
 * @details
 * - **Dynamic Context:** Dynamic contexts are specified as 0x0001 through 0x3FFF.
 *   Dynamic contexts keep a histogram that stores
 *   the number of occurrences of symbols that are added through the AddSymbol method.
 * - **Static Context:** Static contexts are specified as 0x4000 through 0x7FFF.
 *   Static contexts represent histograms where each value between 0 and (context - 0x4000)
 *   are equally likely. Static context histograms are not changed by the AddSymbol method.
 * - **Context 0 or Context8:** Context 0 is a shortcut to context 0x40FF, which corresponds
 *   to values from 0 through 255.
 * - When a histogram for a dynamic context is initialized, the symbol frequency of the
 *   escape symbol is initialized to 1.
 * - Symbols larger than 0xFFFF are treated as static.
 * - The ContextManager class implements this interface.
 */
class CONTEXT_MANAGER
{
public:
    CONTEXT_MANAGER();

    /**
     * @brief Add an occurrence of the symbol to the specified context.
     * @param aContext Add the occurrence to this context's histogram.
     * @param aSymbol Add an occurrence of this symbol to the histogram.
     */
    void AddSymbol( uint32_t aContext, uint32_t aSymbol );

    /**
     * @brief Get the number of occurrences of the given symbol in the context.
     */
    uint32_t GetSymbolFrequency( uint32_t aContext, uint32_t aSymbol );

    /**
     * @brief Get the total number of occurrences for all symbols less than the given symbol in the context.
     * @param aContext Use this context's histogram.
     * @param aSymbol Use this symbol.
     * @return Sum of all symbol frequencies for symbols less than the given symbol in the given context.
     */
    uint32_t GetCumulativeSymbolFrequency( uint32_t aContext, uint32_t aSymbol );

    /**
     * @brief Get the total occurrences of all the symbols in this context.
     * @param aContext Use this context's histogram.
     * @return Total occurrences of all symbols for the given context.
     */
    uint32_t GetTotalSymbolFrequency( uint32_t aContext );

    /**
     * @brief Find the symbol in a histogram that has the specified cumulative frequency.
     * @param aContext Use this context's histogram.
     * @param symbolFrequency Use this frequency.
     * @return The symbol that corresponds to the given cumulative frequency and context.
     */
    uint32_t GetSymbolFromFrequency( uint32_t aContext, uint32_t symbolFrequency );

private:
    std::vector<std::vector<std::uint32_t>> m_symbolCount;
    std::vector<std::vector<std::uint32_t>> m_cumulativeCount;

    /**
     * Elephant determines the number of symbol occurences that are stored in each histogram
     */
    const uint32_t m_elephant;
    /**
     * the maximum value that is stored in a histogram
     */
    const uint32_t m_maximumSymbolInHistogram;
    /**
     * the ammount to increase the size of an array when reallocating an array.
     */
    const uint32_t m_arraySizeIncr;
};

} // namespace U3D