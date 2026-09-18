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

#include <model_preview_manager.h>

#include <algorithm>


namespace
{
wxString canonicalRequirement( wxString aRequirement )
{
    aRequirement.Trim( true ).Trim( false ).MakeLower();
    return aRequirement;
}
} // namespace


bool ShouldPromptForModelPreviewSetup( const MODEL_PREVIEW_STATUS&  aStatus,
                                       const std::vector<wxString>& aDeclinedRequirements )
{
    if( aStatus.enablement != MODEL_PREVIEW_ENABLEMENT_STATE::SETUP_REQUIRED
        || aStatus.guidance != MODEL_PREVIEW_GUIDANCE::REPAIR_REGISTRATION
        || !aStatus.canRepairRegistration || aStatus.requirementId.IsEmpty() )
    {
        return false;
    }

    const wxString required = canonicalRequirement( aStatus.requirementId );

    return std::none_of( aDeclinedRequirements.begin(), aDeclinedRequirements.end(),
                         [&]( const wxString& aDeclined )
                         {
                             return canonicalRequirement( aDeclined ) == required;
                         } );
}


void DeclineModelPreviewRequirement( const wxString& aRequirement, std::vector<wxString>& aDeclinedRequirements )
{
    const wxString requirement = canonicalRequirement( aRequirement );

    if( requirement.IsEmpty()
        || std::any_of( aDeclinedRequirements.begin(), aDeclinedRequirements.end(),
                        [&]( const wxString& aDeclined )
                        {
                            return canonicalRequirement( aDeclined ) == requirement;
                        } ) )
    {
        return;
    }

    aDeclinedRequirements.push_back( requirement );
}

