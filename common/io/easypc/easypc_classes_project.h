/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file easypc_classes_project.h
 * @brief The project file (.prj), a bare archive stream. Every gate is on the project format.
 */

#ifndef EASYPC_CLASSES_PROJECT_H
#define EASYPC_CLASSES_PROJECT_H

#include <vector>

#include <wx/string.h>

#include <io/easypc/easypc_archive.h>


namespace EASYPC
{

/// One file of a project
struct SOURCE_PROJECT_ITEM : OBJECT
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString Name; ///< Windows path relative to the project directory
};


/// The document root
struct SOURCE_PROJECT : OBJECT
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    int32_t                           Format = 0;
    std::vector<SOURCE_PROJECT_ITEM*> Boards;     ///< at most one
    std::vector<SOURCE_PROJECT_ITEM*> Schematics; ///< in sheet order
};

} // namespace EASYPC

#endif // EASYPC_CLASSES_PROJECT_H
