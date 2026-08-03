/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <mdw/client/mdw_release_readiness.h>

#include <wx/intl.h>


MDW_RELEASE_READINESS MdwReleaseReadiness( bool aDirtyEditors, const MDW_PROJECT_STATE& aState, MDW_BASELINE_HOLD aHold,
                                           const wxString& aHolder, MDW_CHECKIN_MODEL aModel )
{
    const bool workingCopy = aModel == MDW_CHECKIN_MODEL::WORKING_COPY;

    // Saving first, since a save is itself a change the later steps must carry
    if( aDirtyEditors )
    {
        return { MDW_RELEASE_STEP::SAVE, MDW_RELEASE_ACTION::SAVE_ALL,
                 workingCopy ? _( "The design has unsaved changes, so it is not committed yet. Save it, then commit "
                                  "and push it." )
                             : _( "The design has unsaved changes, so it is not checked in yet. Save it, then check it "
                                  "in." ) };
    }

    // No step here clears another user's checkout, so it comes before them all
    if( aHold == MDW_BASELINE_HOLD::LOCKED )
    {
        return { MDW_RELEASE_STEP::LOCKED, MDW_RELEASE_ACTION::NONE,
                 wxString::Format( _( "Locked by %s." ), aHolder ) };
    }

    // A commit made on no branch could not be shared, so the branch comes before the commit
    if( aHold == MDW_BASELINE_HOLD::DETACHED )
    {
        return { MDW_RELEASE_STEP::NOT_ON_BRANCH, MDW_RELEASE_ACTION::NONE,
                 _( "The project is not on a branch. Switch to a branch to release the design." ) };
    }

    // Without a base the files on disk are not a revision the source holds
    if( aState.base_revision_id.IsEmpty() )
    {
        return { MDW_RELEASE_STEP::COMMIT, MDW_RELEASE_ACTION::COMMIT,
                 workingCopy ? _( "The design is not committed yet. Commit and push it before releasing." )
                             : _( "The design is not checked in yet. Check it in before releasing." ) };
    }

    if( aHold == MDW_BASELINE_HOLD::UNCOMMITTED || aState.locally_modified )
    {
        return { MDW_RELEASE_STEP::COMMIT, MDW_RELEASE_ACTION::COMMIT,
                 workingCopy ? _( "The design has changes that are not committed yet. Commit and push them "
                                  "before releasing." )
                             : _( "The design has changes that are not checked in yet. Check them in before "
                                  "releasing." ) };
    }

    if( aHold == MDW_BASELINE_HOLD::NO_REMOTE )
    {
        return { MDW_RELEASE_STEP::NO_REMOTE, MDW_RELEASE_ACTION::NONE,
                 _( "The project has no remote to share a release through. Add one to release the design." ) };
    }

    return { MDW_RELEASE_STEP::READY, MDW_RELEASE_ACTION::RELEASE, _( "The design is ready to release." ) };
}
