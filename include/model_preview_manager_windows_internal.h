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

#ifndef MODEL_PREVIEW_MANAGER_WINDOWS_INTERNAL_H
#define MODEL_PREVIEW_MANAGER_WINDOWS_INTERNAL_H

#include <model_preview_manager.h>

/**
 * Registration facts read from the Windows registry.
 *
 * Collected by model_preview_manager_windows.cpp and classified separately so the state machine
 * can be exercised without a registry.
 */

enum class MODEL_PREVIEW_OBSERVATION
{
    PRESENT,
    MISSING,
    UNREADABLE,
    MALFORMED
};

struct WINDOWS_MODEL_PREVIEW_SNAPSHOT
{
    bool                      bundledPresent = false;
    bool                      foreignOwner = false;
    bool                      sameModule = false;
    MODEL_PREVIEW_OBSERVATION module = MODEL_PREVIEW_OBSERVATION::MISSING;
    MODEL_PREVIEW_OBSERVATION associations = MODEL_PREVIEW_OBSERVATION::MISSING;
    MODEL_PREVIEW_OBSERVATION surrogate = MODEL_PREVIEW_OBSERVATION::MISSING;
    MODEL_PREVIEW_OBSERVATION classMetadata = MODEL_PREVIEW_OBSERVATION::MISSING;
    MODEL_PREVIEW_OBSERVATION previewList = MODEL_PREVIEW_OBSERVATION::MISSING;
    wxString                  bundledVersion;
    wxString                  registeredVersion;
    wxString                  requirementId;
};

KICOMMON_API MODEL_PREVIEW_STATUS ClassifyWindowsModelPreviewStatus( const WINDOWS_MODEL_PREVIEW_SNAPSHOT& aSnapshot );

#endif
