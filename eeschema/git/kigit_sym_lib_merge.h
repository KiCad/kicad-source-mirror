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

#ifndef KIGIT_SYM_LIB_MERGE_H
#define KIGIT_SYM_LIB_MERGE_H

#include <git/kigit_trivial_merge_base.h>


/**
 * libgit2 merge driver for `.kicad_sym` library files.
 *
 * Trivial-case 3-way merge inline; non-trivial cases return
 * GIT_EMERGECONFLICT and defer to the mergetool. The output blob always
 * contains a parseable .kicad_sym so opening the library shows a usable
 * state (ours by default). Registered as "kicad-sym-lib".
 */
class KIGIT_SYM_LIB_MERGE : public KIGIT_TRIVIAL_MERGE_BASE<KIGIT_SYM_LIB_MERGE>
{
public:
    using KIGIT_TRIVIAL_MERGE_BASE::KIGIT_TRIVIAL_MERGE_BASE;
};


#endif // KIGIT_SYM_LIB_MERGE_H
