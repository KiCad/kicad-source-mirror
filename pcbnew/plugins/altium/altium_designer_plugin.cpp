/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file altium_designer_plugin.cpp
 * @brief Pcbnew PLUGIN for Altium *.PcbDoc format.
 */

#include <wx/string.h>

#include <altium_designer_plugin.h>
#include <altium_pcb.h>
#include "plugins/altium/altium_parser.h"

#include <board.h>

#include <compoundfilereader.h>
#include <utf.h>

ALTIUM_DESIGNER_PLUGIN::ALTIUM_DESIGNER_PLUGIN()
{
    m_board = nullptr;
    m_props = nullptr;
}


ALTIUM_DESIGNER_PLUGIN::~ALTIUM_DESIGNER_PLUGIN()
{
}


const wxString ALTIUM_DESIGNER_PLUGIN::PluginName() const
{
    return wxT( "Altium Designer" );
}


const wxString ALTIUM_DESIGNER_PLUGIN::GetFileExtension() const
{
    return wxT( "PcbDoc" );
}


BOARD* ALTIUM_DESIGNER_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe,
                                     const STRING_UTF8_MAP* aProperties, PROJECT* aProject,
                                     PROGRESS_REPORTER* aProgressReporter )
{
    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    // clang-format off
    const std::map<ALTIUM_PCB_DIR, std::string> mapping = {
            { ALTIUM_PCB_DIR::FILE_HEADER, "FileHeader" },
            { ALTIUM_PCB_DIR::ARCS6, "Arcs6" },
            { ALTIUM_PCB_DIR::BOARD6, "Board6" },
            { ALTIUM_PCB_DIR::BOARDREGIONS, "BoardRegions" },
            { ALTIUM_PCB_DIR::CLASSES6, "Classes6" },
            { ALTIUM_PCB_DIR::COMPONENTS6, "Components6" },
            { ALTIUM_PCB_DIR::COMPONENTBODIES6, "ComponentBodies6" },
            { ALTIUM_PCB_DIR::DIMENSIONS6, "Dimensions6" },
            { ALTIUM_PCB_DIR::EXTENDPRIMITIVEINFORMATION, "ExtendedPrimitiveInformation" },
            { ALTIUM_PCB_DIR::FILLS6, "Fills6" },
            { ALTIUM_PCB_DIR::MODELS, "Models" },
            { ALTIUM_PCB_DIR::NETS6, "Nets6" },
            { ALTIUM_PCB_DIR::PADS6, "Pads6" },
            { ALTIUM_PCB_DIR::POLYGONS6, "Polygons6" },
            { ALTIUM_PCB_DIR::REGIONS6, "Regions6" },
            { ALTIUM_PCB_DIR::RULES6, "Rules6" },
            { ALTIUM_PCB_DIR::SHAPEBASEDREGIONS6, "ShapeBasedRegions6" },
            { ALTIUM_PCB_DIR::TEXTS6, "Texts6" },
            { ALTIUM_PCB_DIR::TRACKS6, "Tracks6" },
            { ALTIUM_PCB_DIR::VIAS6, "Vias6" },
            { ALTIUM_PCB_DIR::WIDESTRINGS6, "WideStrings6" }
    };
    // clang-format on

    ALTIUM_COMPOUND_FILE altiumPcbFile( aFileName );

    try
    {
        // Parse File
        ALTIUM_PCB pcb( m_board, aProgressReporter );
        pcb.Parse( altiumPcbFile, mapping );
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }

    return m_board;
}

long long ALTIUM_DESIGNER_PLUGIN::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    // File hasn't been loaded yet.
    if( aLibraryPath.IsEmpty() )
    {
        return 0;
    }

    wxFileName fn( aLibraryPath );

    if( fn.IsFileReadable() )
    {
        return fn.GetModificationTime().GetValue().GetValue();
    }
    else
    {
        return 0;
    }
}

void ALTIUM_DESIGNER_PLUGIN::FootprintEnumerate( wxArrayString&  aFootprintNames,
                                                 const wxString& aLibraryPath, bool aBestEfforts,
                                                 const STRING_UTF8_MAP* aProperties )
{
    ALTIUM_COMPOUND_FILE altiumLibFile( aLibraryPath );

    try
    {
        const std::vector<std::string>  streamName = { "Library", "Data" };
        const CFB::COMPOUND_FILE_ENTRY* libraryData = altiumLibFile.FindStream( streamName );
        if( libraryData == nullptr )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "File not found: '%s'." ), FormatPath( streamName ) ) );
        }

        ALTIUM_PARSER parser( altiumLibFile, libraryData );

        std::map<wxString, wxString> properties = parser.ReadProperties();

        uint32_t numberOfFootprints = parser.Read<uint32_t>();
        aFootprintNames.Alloc( numberOfFootprints );
        for( size_t i = 0; i < numberOfFootprints; i++ )
        {
            parser.ReadAndSetSubrecordLength();
            wxString footprintName = parser.ReadWxString();
            aFootprintNames.Add( footprintName );
            parser.SkipSubrecord();
        }

        if( parser.HasParsingError() )
        {
            THROW_IO_ERROR( wxString::Format( "%s stream was not parsed correctly",
                                              FormatPath( streamName ) ) );
        }

        if( parser.GetRemainingBytes() != 0 )
        {
            THROW_IO_ERROR(
                    wxString::Format( "%s stream is not fully parsed", FormatPath( streamName ) ) );
        }
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }
}

FOOTPRINT* ALTIUM_DESIGNER_PLUGIN::FootprintLoad( const wxString& aLibraryPath,
                                                  const wxString& aFootprintName, bool aKeepUUID,
                                                  const STRING_UTF8_MAP* aProperties )
{
    ALTIUM_COMPOUND_FILE altiumLibFile( aLibraryPath );

    try
    {
        // Parse File
        ALTIUM_PCB pcb( m_board, nullptr );
        return pcb.ParseFootprint( altiumLibFile, aFootprintName );
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }
}
