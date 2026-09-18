/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 */

#ifndef MODEL_PREVIEW_MANAGER_H
#define MODEL_PREVIEW_MANAGER_H

#include <vector>

// Not just for wxArrayString: it is dllexported and derives from std::vector<wxString>, so this
// is what makes MSVC treat that instantiation as dllimport.  Dropping it for <wx/string.h> makes
// every translation unit emit its own copy and the link fails with LNK2005.
#include <wx/arrstr.h>

#include <kicommon.h>

class wxWindow;

enum class MODEL_PREVIEW_COMPONENT_STATE
{
    AVAILABLE,
    UNAVAILABLE,
    DAMAGED,
    VERSION_MISMATCH
};

enum class MODEL_PREVIEW_ENABLEMENT_STATE
{
    ENABLED,
    SETUP_REQUIRED,
    UNKNOWN
};

enum class MODEL_PREVIEW_GUIDANCE
{
    NONE,
    REPAIR_REGISTRATION,
    REINSTALL_KICAD,
    STATUS_UNAVAILABLE
};

struct MODEL_PREVIEW_STATUS
{
    MODEL_PREVIEW_COMPONENT_STATE  component = MODEL_PREVIEW_COMPONENT_STATE::UNAVAILABLE;
    MODEL_PREVIEW_ENABLEMENT_STATE enablement = MODEL_PREVIEW_ENABLEMENT_STATE::UNKNOWN;
    MODEL_PREVIEW_GUIDANCE         guidance = MODEL_PREVIEW_GUIDANCE::NONE;
    wxString                       bundledVersion;
    wxString                       registeredVersion;
    wxString                       requirementId;
    wxString                       detail; // Non-localized diagnostic for trace logging.
    bool                           canRepairRegistration = false;
};

KICOMMON_API bool ShouldPromptForModelPreviewSetup( const MODEL_PREVIEW_STATUS&  aStatus,
                                                    const std::vector<wxString>& aDeclinedRequirements );
KICOMMON_API void DeclineModelPreviewRequirement( const wxString&        aRequirement,
                                                  std::vector<wxString>& aDeclinedRequirements );

#if defined( KICAD_NATIVE_MODEL_PREVIEW ) && defined( __WINDOWS__ )
KICOMMON_API MODEL_PREVIEW_STATUS GetPlatformModelPreviewStatus();
KICOMMON_API bool                 RepairPlatformModelPreviewRegistration( wxString& aError );

KICOMMON_API void MaybeShowModelPreviewSetupPrompt( wxWindow* aParent );
#endif

#endif // MODEL_PREVIEW_MANAGER_H
