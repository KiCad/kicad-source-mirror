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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KIGIT_FP_MERGE_H
#define KIGIT_FP_MERGE_H

#include <git/kigit_trivial_merge_base.h>


/**
 * libgit2 merge driver for `.kicad_mod` footprint files (members of a
 * `.pretty` directory).
 *
 * Same trivial-case 3-way pattern as KIGIT_SCH_MERGE. Complex 3-way merges
 * defer to the mergetool app. Registered as "kicad-fp".
 */
class KIGIT_FP_MERGE : public KIGIT_TRIVIAL_MERGE_BASE<KIGIT_FP_MERGE>
{
public:
    using KIGIT_TRIVIAL_MERGE_BASE::KIGIT_TRIVIAL_MERGE_BASE;
};


#endif // KIGIT_FP_MERGE_H
