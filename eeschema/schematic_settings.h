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
#include <settings/nested_settings.h>
#include <template_fieldnames.h>

/**
 * These settings were stored in SCH_BASE_FRAME previously.
 * The backing store is currently the project file.
 * They should likely move to a project settings file (JSON) once that framework exists.
 *
 * These are loaded from eeschema settings but then overwritten by the project settings.
 * All of the values are stored in IU, but the backing file stores in mils.
 */
class SCHEMATIC_SETTINGS : public NESTED_SETTINGS
{
public:
    SCHEMATIC_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~SCHEMATIC_SETTINGS();

    // Default sizes are all stored in IU here, and in mils in the JSON file

    int       m_DefaultLineWidth;
    int       m_DefaultWireThickness;
    int       m_DefaultBusThickness;
    int       m_DefaultTextSize;
    double    m_TextOffsetRatio;
    int       m_PinSymbolSize;
    int       m_JunctionSize; // Size of junction dot in mils
    // User choice for junction dot size ( e.g. none = 0, smallest = 1, small = 2, etc )
    int       m_JunctionSizeChoice;

    bool      m_IntersheetRefsShow;
    bool      m_IntersheetRefsListOwnPage;
    bool      m_IntersheetRefsFormatShort;
    wxString  m_IntersheetRefsPrefix;
    wxString  m_IntersheetRefsSuffix;

    wxString  m_PageLayoutDescrFile;

    wxString  m_PlotDirectoryName;

    wxString  m_NetFormatName;

    bool      m_SpiceAdjustPassiveValues;
    wxString  m_SpiceCommandString;      // A command string to run external spice

    TEMPLATES m_TemplateFieldNames;
};

#endif
