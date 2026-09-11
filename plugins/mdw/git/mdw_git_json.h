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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file mdw_git_json.h
 * @brief nlohmann::json for the backend sources that a kicommon consumer also compiles.
 *
 * kicommon exports json's members, so a target linking it must take them from there or MSVC defines them twice
 * (LNK2005). Such a target sets MDW_GIT_JSON_FROM_KICOMMON. kicad-mdw-git does not link kicommon and uses json
 * directly.
 */

#ifndef MDW_GIT_JSON_H
#define MDW_GIT_JSON_H

#ifdef MDW_GIT_JSON_FROM_KICOMMON
#include <json_common.h>
#else
#include <nlohmann/json.hpp>
#endif

#endif // MDW_GIT_JSON_H
