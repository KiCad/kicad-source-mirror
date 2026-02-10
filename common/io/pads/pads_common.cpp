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

#include <io/pads/pads_common.h>

#include <cstdint>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <exception>


#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/log.h>

namespace PADS_COMMON
{

KIID GenerateDeterministicUuid( const std::string& aIdentifier )
{
    // Use FNV-1a hash algorithm to generate a 128-bit hash from the identifier string.
    // This produces deterministic output for the same input, enabling cross-probe
    // linking between schematic and PCB imports.

    // FNV-1a parameters for 64-bit
    const uint64_t FNV_PRIME = 0x00000100000001B3ULL;
    const uint64_t FNV_OFFSET = 0xcbf29ce484222325ULL;

    // Hash the identifier twice with different salts to get 128 bits
    uint64_t hash1 = FNV_OFFSET;
    uint64_t hash2 = FNV_OFFSET;

    // First hash with "PADS1:" prefix
    std::string salt1 = "PADS1:" + aIdentifier;

    for( char c : salt1 )
    {
        hash1 ^= static_cast<uint8_t>( c );
        hash1 *= FNV_PRIME;
    }

    // Second hash with "PADS2:" prefix
    std::string salt2 = "PADS2:" + aIdentifier;

    for( char c : salt2 )
    {
        hash2 ^= static_cast<uint8_t>( c );
        hash2 *= FNV_PRIME;
    }

    // Format as UUID string (xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx)
    // Version 4 UUID format with deterministic values
    std::ostringstream ss;
    ss << std::hex << std::setfill( '0' );

    // First 8 hex chars from hash1
    ss << std::setw( 8 ) << ( hash1 >> 32 );
    ss << '-';

    // Next 4 hex chars from hash1
    ss << std::setw( 4 ) << ( ( hash1 >> 16 ) & 0xFFFF );
    ss << '-';

    // Version 4 UUID: set version bits
    uint16_t version = ( ( hash1 & 0xFFFF ) & 0x0FFF ) | 0x4000;
    ss << std::setw( 4 ) << version;
    ss << '-';

    // Variant bits: set to 10xx (RFC 4122 variant)
    uint16_t variant = ( ( hash2 >> 48 ) & 0x3FFF ) | 0x8000;
    ss << std::setw( 4 ) << variant;
    ss << '-';

    // Last 12 hex chars from hash2
    ss << std::setw( 12 ) << ( hash2 & 0xFFFFFFFFFFFFULL );

    return KIID( ss.str() );
}


PADS_FILE_TYPE DetectPadsFileType( const wxString& aFilePath )
{
    std::ifstream file( aFilePath.fn_str() );

    if( !file.is_open() )
        return PADS_FILE_TYPE::UNKNOWN;

    std::string line;

    if( !std::getline( file, line ) )
        return PADS_FILE_TYPE::UNKNOWN;

    // PADS PowerPCB (PCB) files start with *PADS-POWERPCB* or !PADS-POWERPCB*
    if( line.find( "*PADS-POWERPCB*" ) != std::string::npos
        || line.find( "!PADS-POWERPCB" ) != std::string::npos )
        return PADS_FILE_TYPE::PCB_ASCII;

    // PADS Router PCB files start with *PADS2000* or !PADS2000*
    if( line.find( "*PADS2000*" ) != std::string::npos
        || line.find( "!PADS2000" ) != std::string::npos )
        return PADS_FILE_TYPE::PCB_ASCII;

    // PADS Logic schematic files start with *PADS-POWERLOGIC* or !PADS-POWERLOGIC*
    if( line.find( "*PADS-POWERLOGIC*" ) != std::string::npos
        || line.find( "!PADS-POWERLOGIC" ) != std::string::npos )
        return PADS_FILE_TYPE::SCHEMATIC_ASCII;

    if( line.find( "*PADS-LOGIC*" ) != std::string::npos
        || line.find( "!PADS-LOGIC" ) != std::string::npos )
        return PADS_FILE_TYPE::SCHEMATIC_ASCII;

    return PADS_FILE_TYPE::UNKNOWN;
}


RELATED_FILES FindRelatedPadsFiles( const wxString& aFilePath )
{
    RELATED_FILES result;

    wxFileName sourceFn( aFilePath );

    if( !sourceFn.IsOk() || !sourceFn.FileExists() )
        return result;

    PADS_FILE_TYPE sourceType = DetectPadsFileType( aFilePath );

    if( sourceType == PADS_FILE_TYPE::UNKNOWN )
        return result;

    // Set the source file in the appropriate field
    if( sourceType == PADS_FILE_TYPE::PCB_ASCII )
        result.pcbFile = aFilePath;
    else if( sourceType == PADS_FILE_TYPE::SCHEMATIC_ASCII )
        result.schematicFile = aFilePath;

    wxString sourceDir = sourceFn.GetPath();
    wxString sourceBase = sourceFn.GetName();

    // Get list of potential related files in the same directory
    wxDir dir( sourceDir );

    if( !dir.IsOpened() )
        return result;

    // Common PADS file extensions to check
    static const std::vector<wxString> extensions = { wxS( "asc" ), wxS( "ASC" ), wxS( "txt" ),
                                                      wxS( "TXT" ) };

    wxString filename;
    bool cont = dir.GetFirst( &filename );

    while( cont )
    {
        wxFileName candidateFn( sourceDir, filename );
        wxString candidatePath = candidateFn.GetFullPath();

        // Skip the source file itself
        if( candidatePath == aFilePath )
        {
            cont = dir.GetNext( &filename );
            continue;
        }

        // Check if this file matches any of our extensions
        wxString ext = candidateFn.GetExt().Lower();
        bool validExt = false;

        for( const wxString& validExtension : extensions )
        {
            if( ext == validExtension.Lower() )
            {
                validExt = true;
                break;
            }
        }

        if( !validExt )
        {
            cont = dir.GetNext( &filename );
            continue;
        }

        // Prioritize files with matching base names
        bool matchingBase = ( candidateFn.GetName() == sourceBase );

        PADS_FILE_TYPE candidateType = DetectPadsFileType( candidatePath );

        // If we imported PCB, look for schematic
        if( sourceType == PADS_FILE_TYPE::PCB_ASCII
            && candidateType == PADS_FILE_TYPE::SCHEMATIC_ASCII )
        {
            if( matchingBase || result.schematicFile.IsEmpty() )
            {
                result.schematicFile = candidatePath;

                // If we found a matching base name, stop looking
                if( matchingBase )
                {
                    cont = dir.GetNext( &filename );
                    continue;
                }
            }
        }
        // If we imported schematic, look for PCB
        else if( sourceType == PADS_FILE_TYPE::SCHEMATIC_ASCII
                 && candidateType == PADS_FILE_TYPE::PCB_ASCII )
        {
            if( matchingBase || result.pcbFile.IsEmpty() )
            {
                result.pcbFile = candidatePath;

                // If we found a matching base name, stop looking
                if( matchingBase )
                {
                    cont = dir.GetNext( &filename );
                    continue;
                }
            }
        }

        cont = dir.GetNext( &filename );
    }

    return result;
}

int ParseInt( const std::string& aStr, int aDefault, const std::string& aContext )
{
    try
    {
        return std::stoi( aStr );
    }
    catch( const std::exception& )
    {
        if( !aContext.empty() )
        {
            wxLogTrace( wxT( "PADS" ), wxT( "Parse error in %s: '%s' is not a valid integer" ),
                        wxString::FromUTF8( aContext ), wxString::FromUTF8( aStr ) );
        }

        return aDefault;
    }
}


double ParseDouble( const std::string& aStr, double aDefault, const std::string& aContext )
{
    try
    {
        return std::stod( aStr );
    }
    catch( const std::exception& )
    {
        if( !aContext.empty() )
        {
            wxLogTrace( wxT( "PADS" ), wxT( "Parse error in %s: '%s' is not a valid number" ),
                        wxString::FromUTF8( aContext ), wxString::FromUTF8( aStr ) );
        }

        return aDefault;
    }
}


wxString ConvertInvertedNetName( const std::string& aNetName )
{
    if( aNetName.empty() )
        return wxString();

    if( aNetName[0] == '/' )
        return wxT( "~{" ) + wxString::FromUTF8( aNetName.substr( 1 ) ) + wxT( "}" );

    return wxString::FromUTF8( aNetName );
}


LINE_STYLE PadsLineStyleToKiCad( int aPadsStyle )
{
    int8_t s = static_cast<int8_t>( aPadsStyle & 0xFF );

    switch( s )
    {
    case 0:   return LINE_STYLE::DASH;
    case 1:   return LINE_STYLE::SOLID;
    case -1:  return LINE_STYLE::SOLID;
    case -2:  return LINE_STYLE::DASH;
    case -3:  return LINE_STYLE::DOT;
    case -4:  return LINE_STYLE::DASHDOT;
    case -5:  return LINE_STYLE::DASHDOTDOT;
    default:  return LINE_STYLE::SOLID;
    }
}

} // namespace PADS_COMMON
