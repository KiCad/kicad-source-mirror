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

#include <sch_io/pads/pads_sch_binary_model.h>

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

class SCHEMATIC;
class SCH_SHEET;

namespace PADS_SCH_BINARY
{

struct BUILD_COUNTS
{
    size_t sheets = 0;
    size_t symbols = 0;
    size_t wires = 0;
    size_t buses = 0;
    size_t busEntries = 0;
    size_t junctions = 0;
    size_t labels = 0;
    size_t texts = 0;
    size_t graphics = 0;

    bool operator==( const BUILD_COUNTS& ) const = default;
};


struct BUILD_RESULT
{
    BUILD_COUNTS                   counts;
    std::vector<PARSER_DIAGNOSTIC> diagnostics;
};


class PADS_SCH_BINARY_BUILDER
{
public:
    explicit PADS_SCH_BINARY_BUILDER( std::function<void()> aBeforeCommit = {} ) :
            m_beforeCommit( std::move( aBeforeCommit ) )
    {
    }

    BUILD_RESULT Build( const PADS_SCH_MODEL& aModel, SCHEMATIC* aSchematic, SCH_SHEET* aAppendToMe,
                        const wxString& aSourcePath );

private:
    std::function<void()> m_beforeCommit;
};

} // namespace PADS_SCH_BINARY
