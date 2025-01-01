/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Rivos
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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

#ifndef _AUI_SETTINGS_H_
#define _AUI_SETTINGS_H_

#include <settings/json_settings.h>

class wxAuiPaneInfo;
class wxPoint;
class wxRect;
class wxSize;

KICOMMON_API void to_json( nlohmann::json& aJson, const wxPoint& aPoint );
KICOMMON_API void from_json( const nlohmann::json& aJson, wxPoint& aPoint );
KICOMMON_API bool operator<( const wxPoint& aLhs, const wxPoint& aRhs );

KICOMMON_API void to_json( nlohmann::json& aJson, const wxSize& aPoint );
KICOMMON_API void from_json( const nlohmann::json& aJson, wxSize& aPoint );
KICOMMON_API bool operator<( const wxSize& aLhs, const wxSize& aRhs );

KICOMMON_API void to_json( nlohmann::json& aJson, const wxRect& aRect );
KICOMMON_API void from_json( const nlohmann::json& aJson, wxRect& aRect );
KICOMMON_API bool operator<( const wxRect& aLhs, const wxRect& aRhs );

KICOMMON_API void to_json( nlohmann::json& aJson, const wxAuiPaneInfo& aPaneInfo );
KICOMMON_API void from_json( const nlohmann::json& aJson, wxAuiPaneInfo& aPaneInfo );
KICOMMON_API bool operator<( const wxAuiPaneInfo& aLhs, const wxAuiPaneInfo& aRhs );
KICOMMON_API bool operator==( const wxAuiPaneInfo& aLhs, const wxAuiPaneInfo& aRhs );

#endif // _AUI_SETTINGS_H_
