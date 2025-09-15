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

namespace U3D
{
class CONSTANTS
{
public:
    /**
     * Uncompressed U8 Context
     */
    static const uint32_t Context8 = 0;

    /**
     * Bit masks for reading and writing symbols.
     * masks all but the most significan bit
     */
    static const uint32_t HalfMask = 0x00008000;
    /**
     *  Masks the most significant bit
     */
    static const uint32_t NotHalfMask = 0x00007FFF;
    /**
     *  Masks all but the 2nd most significan bit
     */
    static const uint32_t QuarterMask = 0x00004000;
    /**
     *  Masks the 2 most significant bits
     */
    static const uint32_t NotThreeQuarterMask = 0x00003FFF;
    /**
     * Contexts greater than this are static contexts
     */
    static const uint32_t StaticFull = 0x00000400;
    /**
     * The largest allowable static context. values written to contexts > MaxRange are
     * written as uncompressed.
     */
    static const uint32_t MaxRange = StaticFull + 0x00003FFF;

    static const uint32_t Swap8[];
};
} // namespace U3D