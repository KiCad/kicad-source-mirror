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

#ifndef MODEL_FILE_PREVIEW_REQUEST_SERVICE_H
#define MODEL_FILE_PREVIEW_REQUEST_SERVICE_H

#include "panel_model_file_preview.h"

#include <memory>

class MODEL_FILE_PREVIEW_REQUEST_SERVICE
{
public:
    MODEL_FILE_PREVIEW_REQUEST_SERVICE();
    ~MODEL_FILE_PREVIEW_REQUEST_SERVICE();

    MODEL_FILE_PREVIEW_REQUEST_SERVICE( const MODEL_FILE_PREVIEW_REQUEST_SERVICE& ) = delete;
    MODEL_FILE_PREVIEW_REQUEST_SERVICE& operator=( const MODEL_FILE_PREVIEW_REQUEST_SERVICE& ) = delete;

    PANEL_MODEL_FILE_PREVIEW::REQUEST MakeRequest();
    PANEL_MODEL_FILE_PREVIEW::CANCEL  MakeCancel();

    // The owning UI thread calls Cancel when its dialog closes and Shutdown before wxApp teardown.
    // Completion callbacks must not call either lifecycle method.
    void Cancel();
    void Shutdown();

private:
    struct STATE;

    std::shared_ptr<STATE> m_state;
};

#endif // MODEL_FILE_PREVIEW_REQUEST_SERVICE_H
