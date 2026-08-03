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

#ifndef MDW_RELEASE_READINESS_H
#define MDW_RELEASE_READINESS_H

#include <kicommon.h>
#include <mdw/client/mdw_baseline.h>
#include <mdw/client/mdw_client.h>

#include <wx/string.h>

/// What still stands between the design on disk and its release, in the order it must be done
enum class MDW_RELEASE_STEP
{
    SAVE,          ///< An editor holds changes the release would not include
    LOCKED,        ///< Another user has the project checked out
    NOT_ON_BRANCH, ///< A commit made here would belong to no branch
    COMMIT,        ///< The files differ from the revision a release would name
    NO_REMOTE,     ///< The project has nowhere to share a release
    READY
};

/// The one thing the release dialog offers for a step
enum class MDW_RELEASE_ACTION
{
    NONE, ///< Only the user can take the step, outside the dialog
    SAVE_ALL,
    COMMIT,
    RELEASE
};

struct MDW_RELEASE_READINESS
{
    MDW_RELEASE_STEP   step = MDW_RELEASE_STEP::READY;
    MDW_RELEASE_ACTION action = MDW_RELEASE_ACTION::RELEASE;
    wxString           summary;
};

/**
 * The first step a release of the open design still needs and the action that takes it.
 *
 * @param aDirtyEditors The schematic or board editor holds unsaved changes.
 * @param aState Where the project stands against its design source.
 * @param aHold What the project source's baseline check said a release would be refused on.
 * @param aHolder Who holds a LOCKED project, by display name.
 * @param aModel How the project source takes changes, which names the step that puts them in a revision.
 */
KICOMMON_API MDW_RELEASE_READINESS MdwReleaseReadiness( bool aDirtyEditors, const MDW_PROJECT_STATE& aState,
                                                        MDW_BASELINE_HOLD aHold, const wxString& aHolder,
                                                        MDW_CHECKIN_MODEL aModel );

#endif // MDW_RELEASE_READINESS_H
