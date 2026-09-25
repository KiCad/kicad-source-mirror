/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file mdw_revision_copy.h
 * Saving one revision of a workspace item as a file of its own.
 */

#ifndef MDW_CLIENT_REVISION_COPY_H
#define MDW_CLIENT_REVISION_COPY_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_revision.h>

#include <chrono>
#include <memory>

class MDW_SOURCE;

/**
 * Download aRevisionId of aItem to aTarget, which an empty id takes as the latest revision. The revision goes
 * to a file beside aTarget first, so aDone, which runs at most once, finds aTarget either holding it in full or
 * as it was. Cancelling suppresses aDone, and leaves aTarget as it was unless the copy already won the race to
 * commit, in which case aTarget may still be replaced.
 *
 * aReplace says the user agreed to replace a file at aTarget. Without it a file found there at the end, as one
 * another program made during the download, fails the copy and is kept.
 */
KICOMMON_API MDW_OP MdwSaveRevisionCopy( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_ITEM_REF& aItem,
                                         const wxString& aRevisionId, const wxString& aTarget, bool aReplace,
                                         MDW_DONE<MDW_REVISION> aDone );

/**
 * Remove the partial and backend staging files of copies whose download never settled, as when a backend kept
 * writing past a timeout or stopping the client workers dropped the cleanup. A file that cannot be removed stays
 * listed for a later sweep. Call only once the backends and every client worker have stopped, since a running
 * copy's partial file would go too.
 *
 * @return false when the sweep was still running after aTimeout, as on an unresponsive share, when an earlier
 *         sweep still runs, or when no thread could be started. A sweep left running finishes on its own thread.
 */
KICOMMON_API bool MdwSweepRevisionCopies( std::chrono::milliseconds aTimeout );

/// True while a sweep that outlived its wait still runs, when process teardown is unsafe
KICOMMON_API bool MdwRevisionSweepRunning();

#endif // MDW_CLIENT_REVISION_COPY_H
