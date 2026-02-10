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

#ifndef PADS_ATTRIBUTE_MAPPER_H
#define PADS_ATTRIBUTE_MAPPER_H

#include <map>
#include <string>

/**
 * Maps PADS attribute names to KiCad field names.
 *
 * PADS uses different attribute names than KiCad for standard fields.
 * This class provides mapping from PADS names to KiCad-compatible names,
 * and identifies which attributes correspond to standard KiCad fields
 * (Reference, Value, Footprint) versus custom user fields.
 */
class PADS_ATTRIBUTE_MAPPER
{
public:
    PADS_ATTRIBUTE_MAPPER();

    /**
     * Get the KiCad field name for a PADS attribute.
     *
     * For known PADS attributes (like "Ref.Des.", "Part Type"), returns
     * the corresponding KiCad field name. For unknown attributes, returns
     * the original name unchanged.
     *
     * @param aPadsAttr The PADS attribute name.
     * @return The corresponding KiCad field name.
     */
    std::string GetKiCadFieldName( const std::string& aPadsAttr ) const;

    /**
     * Check if a PADS attribute maps to a standard KiCad field.
     *
     * Standard fields are Reference, Value, and Footprint. These are
     * handled specially in KiCad and exist on every footprint.
     *
     * @param aPadsAttr The PADS attribute name.
     * @return True if this is a standard field (Reference, Value, Footprint).
     */
    bool IsStandardField( const std::string& aPadsAttr ) const;

    /**
     * Check if a PADS attribute maps to the Reference field.
     *
     * @param aPadsAttr The PADS attribute name.
     * @return True if this maps to the Reference field.
     */
    bool IsReferenceField( const std::string& aPadsAttr ) const;

    /**
     * Check if a PADS attribute maps to the Value field.
     *
     * @param aPadsAttr The PADS attribute name.
     * @return True if this maps to the Value field.
     */
    bool IsValueField( const std::string& aPadsAttr ) const;

    /**
     * Check if a PADS attribute maps to the Footprint field.
     *
     * @param aPadsAttr The PADS attribute name.
     * @return True if this maps to the Footprint field.
     */
    bool IsFootprintField( const std::string& aPadsAttr ) const;

    /**
     * Add or override a custom attribute mapping.
     *
     * @param aPadsAttr The PADS attribute name.
     * @param aKiCadField The KiCad field name to map to.
     */
    void AddMapping( const std::string& aPadsAttr, const std::string& aKiCadField );

    /**
     * Get all custom mappings.
     *
     * @return Map of PADS attribute names to KiCad field names.
     */
    const std::map<std::string, std::string>& GetMappings() const { return m_customMappings; }

    // Standard KiCad field names
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
