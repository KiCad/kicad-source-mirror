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
 */

#ifndef KIGIT_DRIVER_REGISTRY_H
#define KIGIT_DRIVER_REGISTRY_H

#include <kicommon.h>

#include <git2/buffer.h>
#include <git2/sys/merge.h>


namespace KIGIT
{

/**
 * Signature for a KiCad merge-driver apply function. Mirrors the libgit2
 * merge driver `apply` callback, but with simpler args because we only need
 * the source + a target buffer.
 *
 *   src        - libgit2 merge driver source (ancestor/ours/theirs blobs)
 *   path_out   - destination for the resulting path
 *   mode_out   - destination for the resulting file mode
 *   merged_out - destination buffer for the merged blob
 *
 * Returns 0 on success, GIT_EMERGECONFLICT (-13) on unresolved conflicts,
 * or any other negative git error code on failure.
 */
/// uint32_t for mode_out per libgit2; included via <cstdint> by callers.
using MERGE_APPLY_FN = int ( * )( const git_merge_driver_source* src,
                                  const char**                   path_out,
                                  unsigned int*                  mode_out,
                                  git_buf*                       merged_out );


/**
 * Register a KiCad merge driver with libgit2. The driver name must match the
 * `merge=<name>` attribute set in `.gitattributes` (e.g. "kicad-pcb",
 * "kicad-sch", "kicad-sym", "kicad-fp-lib").
 *
 * Idempotent: calling twice with the same name is a no-op. Thread-safety:
 * libgit2's git_merge_driver_register is NOT thread-safe; this function
 * holds an internal mutex around the call, but callers should still ensure
 * it runs once at process init, before any thread fan-out (libgit2 may not
 * tolerate concurrent registration even with our local mutex if its global
 * driver table is read by a parallel merge).
 *
 * Returns true on success.
 */
KICOMMON_API bool RegisterMergeDriver( const char* aName, MERGE_APPLY_FN aApply );

} // namespace KIGIT

#endif // KIGIT_DRIVER_REGISTRY_H
