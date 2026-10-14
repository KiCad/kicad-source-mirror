/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 */

#ifndef RHI_METAL_VIEW_H
#define RHI_METAL_VIEW_H

#include <kicommon.h>

struct RHI_METAL_HOST_VIEW
{
    void* view  = nullptr;
    void* layer = nullptr;
};

KICOMMON_API RHI_METAL_HOST_VIEW RhiMetalAttachHostView( void* aCanvasView );
KICOMMON_API void RhiMetalDetachHostView( void* aHostView );

#endif
