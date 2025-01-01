/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PLUGIN_COMMON_CHOOSE_PROJECT_H
#define PLUGIN_COMMON_CHOOSE_PROJECT_H

#include <functional>
#include <map>
#include <wx/string.h>

/**
 * @brief Describes how non-KiCad boards and schematics should be imported as KiCad projects
 */
struct IMPORT_PROJECT_DESC
{
    wxString ComboName;
    wxString PCBName;
    wxString SchematicName;

    wxString ComboId;
    wxString PCBId;
    wxString SchematicId;

    IMPORT_PROJECT_DESC() {}
};

/**
 * @brief Pointer to a function that takes descriptions of the source projects
 * and removes the ones that are not needed, or clears their ID fields.
 */
using CHOOSE_PROJECT_HANDLER = std::function<std::vector<IMPORT_PROJECT_DESC>( const std::vector<IMPORT_PROJECT_DESC>& )>;


/**
 * @brief Plugin class for import plugins that support choosing a project
 */
class PROJECT_CHOOSER_PLUGIN
{
public:
    /**
     * @brief Register a different handler to be called when a non-KiCad project
     * contains multiple PCB+Schematic combinations.
     *
     * The function is marked as virtual, so the plugins can implement extra
     * logic (e.g., enable warnings or checks)
     */
    virtual void RegisterCallback( CHOOSE_PROJECT_HANDLER aChooseProjectHandler )
    {
        m_choose_project_handler = aChooseProjectHandler;
    }

    virtual ~PROJECT_CHOOSER_PLUGIN() = default;

protected:
    CHOOSE_PROJECT_HANDLER m_choose_project_handler; ///< Callback to choose projects to import
};

#endif // PLUGIN_COMMON_CHOOSE_PROJECT_H
