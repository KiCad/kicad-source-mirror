/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_SCHEMATIC_SETTINGS_H
#define KICAD_SCHEMATIC_SETTINGS_H

#include <convert_to_biu.h>
#include <default_values.h>

/**
 * These settings were stored in SCH_BASE_FRAME previously.
 * The backing store is currently the project file.
 * They should likely move to a project settings file (JSON) once that framework exists.
 *
 * These are loaded from eeschema settings but then overwritten by the project settings.
 * All of the values are stored in IU, but the backing file stores in mils.
 */
struct SCHEMATIC_SETTINGS
{
    SCHEMATIC_SETTINGS() :
            m_DefaultLineWidth( DEFAULT_LINE_THICKNESS * IU_PER_MILS ),
            m_DefaultWireThickness( DEFAULT_WIRE_THICKNESS * IU_PER_MILS ),
            m_DefaultBusThickness( DEFAULT_BUS_THICKNESS * IU_PER_MILS ),
            m_DefaultTextSize( DEFAULT_TEXT_SIZE * IU_PER_MILS ),
            m_TextOffsetRatio( 0.08 ),
            m_PinSymbolSize( DEFAULT_TEXT_SIZE * IU_PER_MILS / 2 ),
            m_JunctionSize( DEFAULT_JUNCTION_DIAM * IU_PER_MILS )
    {}

    int    m_DefaultLineWidth;
    int    m_DefaultWireThickness;
    int    m_DefaultBusThickness;
    int    m_DefaultTextSize;
    double m_TextOffsetRatio;
    int    m_PinSymbolSize;
    int    m_JunctionSize;
};

#endif
