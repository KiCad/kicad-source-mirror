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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PADS_ATTRIBUTE_MAPPER_H
#define PADS_ATTRIBUTE_MAPPER_H

#include <map>
#include <string>

/**
 * Maps PADS attribute names to KiCad field names and identifies which attributes
 * are standard KiCad fields (Reference, Value, Footprint) versus custom fields.
 */
class PADS_ATTRIBUTE_MAPPER
{
public:
    PADS_ATTRIBUTE_MAPPER();

    /**
     * Get the KiCad field name for a PADS attribute, or the original name
     * unchanged when no mapping exists.
     */
    std::string GetKiCadFieldName( const std::string& aPadsAttr ) const;

    /**
     * Check if a PADS attribute maps to a standard KiCad field (Reference, Value,
     * or Footprint).
     */
    bool IsStandardField( const std::string& aPadsAttr ) const;

    bool IsReferenceField( const std::string& aPadsAttr ) const;

    bool IsValueField( const std::string& aPadsAttr ) const;

    bool IsFootprintField( const std::string& aPadsAttr ) const;

    /**
     * Add or override a custom attribute mapping.
     */
    void AddMapping( const std::string& aPadsAttr, const std::string& aKiCadField );

    const std::map<std::string, std::string>& GetMappings() const { return m_customMappings; }

    static constexpr const char* FIELD_REFERENCE = "Reference";
    static constexpr const char* FIELD_VALUE = "Value";
    static constexpr const char* FIELD_FOOTPRINT = "Footprint";
    static constexpr const char* FIELD_DATASHEET = "Datasheet";
    static constexpr const char* FIELD_MPN = "MPN";
    static constexpr const char* FIELD_MANUFACTURER = "Manufacturer";

private:
    std::string normalizeAttrName( const std::string& aName ) const;

    std::map<std::string, std::string> m_standardMappings;
    std::map<std::string, std::string> m_customMappings;
};

#endif // PADS_ATTRIBUTE_MAPPER_H
