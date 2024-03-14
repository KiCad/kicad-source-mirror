/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef JSON_COMMON_H
#define JSON_COMMON_H

#include <nlohmann/json.hpp>

#include <kicommon.h>

/**
 * This is simply a "stub" meant to inform MSVC when compiling shared libraries that it can find
 * template instances in kicommon of nlohmann::json's various templates
 */
class KICOMMON_API JSON_COMMON_EXPORT_STUB final : public nlohmann::json
{
};

#endif