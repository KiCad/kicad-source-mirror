/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Rivos
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

extern void to_json( nlohmann::json& aJson, const wxPoint& aPoint );
extern void from_json( const nlohmann::json& aJson, wxPoint& aPoint );
extern bool operator<( const wxPoint& aLhs, const wxPoint& aRhs );

extern void to_json( nlohmann::json& aJson, const wxSize& aPoint );
extern void from_json( const nlohmann::json& aJson, wxSize& aPoint );
extern bool operator<( const wxSize& aLhs, const wxSize& aRhs );

extern void to_json( nlohmann::json& aJson, const wxRect& aRect );
extern void from_json( const nlohmann::json& aJson, wxRect& aRect );
extern bool operator<( const wxRect& aLhs, const wxRect& aRhs );

extern void to_json( nlohmann::json& aJson, const wxAuiPaneInfo& aPaneInfo );
extern void from_json( const nlohmann::json& aJson, wxAuiPaneInfo& aPaneInfo );
extern bool operator<( const wxAuiPaneInfo& aLhs, const wxAuiPaneInfo& aRhs );
extern bool operator==( const wxAuiPaneInfo& aLhs, const wxAuiPaneInfo& aRhs );

#endif // _AUI_SETTINGS_H_
