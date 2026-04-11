/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Outline font class
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

#ifndef VERSION_INFO_H_
#define VERSION_INFO_H_

#include <kicommon.h>
#include <mutex>
#include <wx/string.h>

namespace KIFONT
{
/**
 * Container for library version helpers.
 */
class KICOMMON_API VERSION_INFO
{
public:
    static wxString FontConfig();

    static wxString FreeType();

    static wxString HarfBuzz();

    static wxString FontLibrary();

private:
    // we are a static helper
    VERSION_INFO() {}
};

} //namespace KIFONT

#endif // VERSION_INFO_H_
