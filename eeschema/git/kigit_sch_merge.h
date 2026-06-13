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

#ifndef KIGIT_SCH_MERGE_H
#define KIGIT_SCH_MERGE_H

#include <git/kigit_trivial_merge_base.h>


/**
 * libgit2 merge driver for `.kicad_sch` files.
 *
 * Resolves the trivial 3-way cases inline and defers complex merges to the
 * mergetool. Unlike PCB, schematics are page-structured: a single .kicad_sch
 * is one page of a hierarchy, so a single-blob driver cannot synthesize the
 * sheet-hierarchy context a structural merge needs. The mergetool recomputes
 * the plan from ancestor/ours/theirs page-by-page; cross-page consequences
 * (rename of a sub-sheet) flow through subsequent merges of the affected
 * sheet pages. Registered as "kicad-sch".
 */
class KIGIT_SCH_MERGE : public KIGIT_TRIVIAL_MERGE_BASE<KIGIT_SCH_MERGE>
{
public:
    using KIGIT_TRIVIAL_MERGE_BASE::KIGIT_TRIVIAL_MERGE_BASE;
};


#endif // KIGIT_SCH_MERGE_H
