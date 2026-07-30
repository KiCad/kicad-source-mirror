/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 */

#pragma once

#include <sch_io/pads/pads_sch_binary_builder.h>
#include <sch_io/pads/pads_sch_binary_model.h>

#include <cstdint>
#include <optional>
#include <vector>

#include <wx/string.h>

class SCHEMATIC;
class SCH_SHEET;

namespace PADS_SCH_BINARY
{

class PADS_SCH_BINARY_READER
{
public:
    static bool IsBinaryFamily( const std::vector<uint8_t>& aData );
    static bool IsSupportedVersion( uint16_t aVersion );
    static bool IsBinarySch( const std::vector<uint8_t>& aData );
    static bool ReadFile( const wxString& aFileName, std::vector<uint8_t>& aData );

    bool Parse( const std::vector<uint8_t>& aData, const wxString& aSourceName = {} );

    const PADS_SCH_MODEL& GetModel() const;

    BUILD_RESULT BuildSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aAppendToMe, const wxString& aSourcePath ) const;

private:
    std::optional<PADS_SCH_MODEL> m_model;
};

} // namespace PADS_SCH_BINARY
