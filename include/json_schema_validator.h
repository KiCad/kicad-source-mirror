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

#ifndef JSON_SCHEMA_VALIDATOR_H
#define JSON_SCHEMA_VALIDATOR_H

#include <wx/filename.h>
#include <kicommon.h>
#include <json_common.h>
#include <nlohmann/json-schema.hpp>

class KICOMMON_API JSON_SCHEMA_VALIDATOR
{
public:
    JSON_SCHEMA_VALIDATOR( const wxFileName& aSchemaFile );

    ~JSON_SCHEMA_VALIDATOR() = default;

    nlohmann::json Validate( const nlohmann::json& aJson,
                             nlohmann::json_schema::error_handler& aErrorHandler,
                             const nlohmann::json_uri& aInitialUri = nlohmann::json_uri("#") ) const;

private:
    nlohmann::json_schema::json_validator m_validator;
};

#endif //JSON_SCHEMA_VALIDATOR_H
