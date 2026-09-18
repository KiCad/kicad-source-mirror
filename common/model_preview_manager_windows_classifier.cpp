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

#include <model_preview_manager_windows_internal.h>


namespace
{
bool Uncertain( MODEL_PREVIEW_OBSERVATION aValue )
{
    return aValue == MODEL_PREVIEW_OBSERVATION::UNREADABLE;
}

bool Damaged( MODEL_PREVIEW_OBSERVATION aValue )
{
    return aValue == MODEL_PREVIEW_OBSERVATION::MISSING || aValue == MODEL_PREVIEW_OBSERVATION::MALFORMED;
}
} // namespace


MODEL_PREVIEW_STATUS ClassifyWindowsModelPreviewStatus( const WINDOWS_MODEL_PREVIEW_SNAPSHOT& aSnapshot )
{
    MODEL_PREVIEW_STATUS status;
    status.bundledVersion = aSnapshot.bundledVersion;
    status.registeredVersion = aSnapshot.registeredVersion;
    status.requirementId = aSnapshot.requirementId;

    if( !aSnapshot.bundledPresent )
    {
        status.component = MODEL_PREVIEW_COMPONENT_STATE::UNAVAILABLE;
        status.enablement = MODEL_PREVIEW_ENABLEMENT_STATE::SETUP_REQUIRED;
        status.guidance = MODEL_PREVIEW_GUIDANCE::REINSTALL_KICAD;
        status.detail = wxS( "The bundled Windows preview handler is missing." );
        return status;
    }

    if( Uncertain( aSnapshot.module ) || Uncertain( aSnapshot.associations ) || Uncertain( aSnapshot.surrogate )
        || Uncertain( aSnapshot.classMetadata ) || Uncertain( aSnapshot.previewList ) )
    {
        status.component = MODEL_PREVIEW_COMPONENT_STATE::AVAILABLE;
        status.enablement = MODEL_PREVIEW_ENABLEMENT_STATE::UNKNOWN;
        status.guidance = MODEL_PREVIEW_GUIDANCE::STATUS_UNAVAILABLE;
        status.detail = wxS( "Windows preview registration could not be read." );
        return status;
    }

    if( aSnapshot.foreignOwner || ( aSnapshot.module == MODEL_PREVIEW_OBSERVATION::PRESENT && !aSnapshot.sameModule ) )
    {
        status.component = MODEL_PREVIEW_COMPONENT_STATE::DAMAGED;
        status.enablement = MODEL_PREVIEW_ENABLEMENT_STATE::SETUP_REQUIRED;
        status.guidance = MODEL_PREVIEW_GUIDANCE::NONE;
        status.detail = wxS( "A different Windows preview handler owns this registration." );
        return status;
    }

    if( aSnapshot.module == MODEL_PREVIEW_OBSERVATION::MISSING )
    {
        status.component = MODEL_PREVIEW_COMPONENT_STATE::AVAILABLE;
        status.enablement = MODEL_PREVIEW_ENABLEMENT_STATE::SETUP_REQUIRED;
        status.guidance = MODEL_PREVIEW_GUIDANCE::REPAIR_REGISTRATION;
        status.detail = wxS( "Windows model previews are not registered for this user." );
        status.canRepairRegistration = true;
        return status;
    }

    if( aSnapshot.registeredVersion != aSnapshot.bundledVersion )
    {
        status.component = MODEL_PREVIEW_COMPONENT_STATE::VERSION_MISMATCH;
        status.enablement = MODEL_PREVIEW_ENABLEMENT_STATE::SETUP_REQUIRED;
        status.guidance = MODEL_PREVIEW_GUIDANCE::REPAIR_REGISTRATION;
        status.detail = wxS( "The registered Windows preview handler version does not match this KiCad installation." );
        status.canRepairRegistration = true;
        return status;
    }

    if( aSnapshot.module == MODEL_PREVIEW_OBSERVATION::MALFORMED || Damaged( aSnapshot.associations )
        || Damaged( aSnapshot.surrogate ) || Damaged( aSnapshot.classMetadata ) || Damaged( aSnapshot.previewList ) )
    {
        status.component = MODEL_PREVIEW_COMPONENT_STATE::DAMAGED;
        status.enablement = MODEL_PREVIEW_ENABLEMENT_STATE::SETUP_REQUIRED;
        status.guidance = MODEL_PREVIEW_GUIDANCE::REPAIR_REGISTRATION;
        status.detail = wxS( "Windows model preview registration is incomplete." );
        status.canRepairRegistration = true;
        return status;
    }

    status.component = MODEL_PREVIEW_COMPONENT_STATE::AVAILABLE;
    status.enablement = MODEL_PREVIEW_ENABLEMENT_STATE::ENABLED;
    status.detail = wxS( "Windows model previews are registered." );
    return status;
}
