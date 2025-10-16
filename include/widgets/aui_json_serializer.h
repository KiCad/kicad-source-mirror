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

#ifndef KICAD_WIDGETS_AUI_JSON_SERIALIZER_H
#define KICAD_WIDGETS_AUI_JSON_SERIALIZER_H

#include <nlohmann/json_fwd.hpp>
#include <kicommon.h>

class wxAuiManager;

class KICOMMON_API WX_AUI_JSON_SERIALIZER
{
public:
    explicit WX_AUI_JSON_SERIALIZER( wxAuiManager& aManager );

    nlohmann::json Serialize() const;
    bool Deserialize( const nlohmann::json& aState ) const;

private:
    [[maybe_unused]] wxAuiManager& m_manager;
};

#endif
