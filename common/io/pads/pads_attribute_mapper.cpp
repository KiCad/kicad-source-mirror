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

#include <io/pads/pads_attribute_mapper.h>

#include <algorithm>
#include <cctype>


PADS_ATTRIBUTE_MAPPER::PADS_ATTRIBUTE_MAPPER()
{
    // Standard PADS attribute mappings to KiCad fields
    // Reference designator variations
    m_standardMappings["ref.des."] = FIELD_REFERENCE;
    m_standardMappings["ref des"] = FIELD_REFERENCE;
    m_standardMappings["ref-des"] = FIELD_REFERENCE;
    m_standardMappings["refdes"] = FIELD_REFERENCE;
    m_standardMappings["reference"] = FIELD_REFERENCE;

    // Value/Part type variations
    m_standardMappings["part type"] = FIELD_VALUE;
    m_standardMappings["part-type"] = FIELD_VALUE;
    m_standardMappings["parttype"] = FIELD_VALUE;
    m_standardMappings["value"] = FIELD_VALUE;
    m_standardMappings["part_type"] = FIELD_VALUE;

    // Footprint/Decal variations
    m_standardMappings["pcb decal"] = FIELD_FOOTPRINT;
    m_standardMappings["pcb_decal"] = FIELD_FOOTPRINT;
    m_standardMappings["decal"] = FIELD_FOOTPRINT;
    m_standardMappings["footprint"] = FIELD_FOOTPRINT;
    m_standardMappings["pattern"] = FIELD_FOOTPRINT;

    // Datasheet variations
    m_standardMappings["datasheet"] = FIELD_DATASHEET;
    m_standardMappings["data sheet"] = FIELD_DATASHEET;
    m_standardMappings["spec"] = FIELD_DATASHEET;
    m_standardMappings["specification"] = FIELD_DATASHEET;

    // MPN (Manufacturer Part Number) variations
    m_standardMappings["part number"] = FIELD_MPN;
    m_standardMappings["part_number"] = FIELD_MPN;
    m_standardMappings["partnumber"] = FIELD_MPN;
    m_standardMappings["pn"] = FIELD_MPN;
    m_standardMappings["mpn"] = FIELD_MPN;
    m_standardMappings["mfr part number"] = FIELD_MPN;
    m_standardMappings["mfr_part_number"] = FIELD_MPN;

    // Manufacturer variations
    m_standardMappings["manufacturer"] = FIELD_MANUFACTURER;
    m_standardMappings["mfr"] = FIELD_MANUFACTURER;
    m_standardMappings["mfg"] = FIELD_MANUFACTURER;
    m_standardMappings["vendor"] = FIELD_MANUFACTURER;
}


std::string PADS_ATTRIBUTE_MAPPER::normalizeAttrName( const std::string& aName ) const
{
    std::string normalized;
    normalized.reserve( aName.size() );

    for( char c : aName )
    {
        normalized += static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
    }

    return normalized;
}


std::string PADS_ATTRIBUTE_MAPPER::GetKiCadFieldName( const std::string& aPadsAttr ) const
{
    std::string normalized = normalizeAttrName( aPadsAttr );

    // Check custom mappings first (they take precedence)
    auto customIt = m_customMappings.find( normalized );

    if( customIt != m_customMappings.end() )
        return customIt->second;

    // Then check standard mappings
    auto standardIt = m_standardMappings.find( normalized );

    if( standardIt != m_standardMappings.end() )
        return standardIt->second;

    // Return original name if no mapping exists
    return aPadsAttr;
}


bool PADS_ATTRIBUTE_MAPPER::IsStandardField( const std::string& aPadsAttr ) const
{
    return IsReferenceField( aPadsAttr ) || IsValueField( aPadsAttr ) || IsFootprintField( aPadsAttr );
}


bool PADS_ATTRIBUTE_MAPPER::IsReferenceField( const std::string& aPadsAttr ) const
{
    std::string fieldName = GetKiCadFieldName( aPadsAttr );
    return fieldName == FIELD_REFERENCE;
}


bool PADS_ATTRIBUTE_MAPPER::IsValueField( const std::string& aPadsAttr ) const
{
    std::string fieldName = GetKiCadFieldName( aPadsAttr );
    return fieldName == FIELD_VALUE;
}


bool PADS_ATTRIBUTE_MAPPER::IsFootprintField( const std::string& aPadsAttr ) const
{
    std::string fieldName = GetKiCadFieldName( aPadsAttr );
    return fieldName == FIELD_FOOTPRINT;
}


void PADS_ATTRIBUTE_MAPPER::AddMapping( const std::string& aPadsAttr, const std::string& aKiCadField )
{
    std::string normalized = normalizeAttrName( aPadsAttr );
    m_customMappings[normalized] = aKiCadField;
}
