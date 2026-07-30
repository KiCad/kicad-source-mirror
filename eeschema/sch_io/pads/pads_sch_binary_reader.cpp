/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 */

#include <sch_io/pads/pads_sch_binary_reader.h>

#include <sch_io/pads/pads_sch_binary_parser.h>

#include <io/pads/pads_binary_utils.h>
#include <ki_exception.h>

namespace PADS_SCH_BINARY
{
namespace
{
    constexpr size_t   HEADER_SIZE = 32;
    constexpr uint8_t  MAGIC1 = 0xFE;
    constexpr uint16_t VERSION_12 = 0x000C;
    constexpr uint16_t VERSION_13 = 0x000D;
}


bool PADS_SCH_BINARY_READER::IsBinaryFamily( const std::vector<uint8_t>& aData )
{
    return aData.size() >= HEADER_SIZE && PADS_IO::HasSdbMagic( aData, MAGIC1 );
}


bool PADS_SCH_BINARY_READER::IsSupportedVersion( uint16_t aVersion )
{
    return aVersion == VERSION_12 || aVersion == VERSION_13;
}


bool PADS_SCH_BINARY_READER::IsBinarySch( const std::vector<uint8_t>& aData )
{
    return IsBinaryFamily( aData );
}


bool PADS_SCH_BINARY_READER::ReadFile( const wxString& aFileName, std::vector<uint8_t>& aData )
{
    return PADS_IO::ReadFileToBuffer( aFileName, aData );
}


bool PADS_SCH_BINARY_READER::Parse( const std::vector<uint8_t>& aData, const wxString& aSourceName )
{
    m_model.reset();

    if( !IsBinaryFamily( aData ) )
        return false;

    m_model = PADS_SCH_BINARY_PARSER().Parse( aData, aSourceName );
    return true;
}


const PADS_SCH_MODEL& PADS_SCH_BINARY_READER::GetModel() const
{
    if( !m_model )
        THROW_IO_ERROR( wxS( "PADS Logic binary schematic has not been parsed" ) );

    return *m_model;
}


BUILD_RESULT PADS_SCH_BINARY_READER::BuildSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aAppendToMe,
                                                     const wxString& aSourcePath ) const
{
    return PADS_SCH_BINARY_BUILDER().Build( GetModel(), aSchematic, aAppendToMe, aSourcePath );
}

} // namespace PADS_SCH_BINARY
