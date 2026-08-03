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

#ifndef ORCAD_OLE_H_
#define ORCAD_OLE_H_

#include <sch_io/ole_image.h>

using ORCAD_OLE_PREVIEW_TYPE = OLE_IMAGE_TYPE;
using ORCAD_OLE_PREVIEW = OLE_IMAGE_PAYLOAD;

ORCAD_OLE_PREVIEW OrcadExtractOlePreview( const std::vector<uint8_t>& aPayload );

bool OrcadRenderWmf( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight, wxImage& aImage );

#endif
