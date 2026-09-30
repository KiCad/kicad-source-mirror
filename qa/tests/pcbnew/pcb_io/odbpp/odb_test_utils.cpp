/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "odb_test_utils.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>

#include <board.h>
#include <core/utf8.h>
#include <footprint.h>
#include <pad.h>
#include <reporter.h>
#include <pcbnew/pcb_io/odbpp/pcb_io_odbpp.h>
#include <richio.h>

namespace fs = std::filesystem;


fs::path ExportOdb( const BOARD& aBoard, const fs::path& aDir, const std::string& aUnits,
                    const std::string& aSigfig, REPORTER* aReporter )
{
    fs::create_directories( aDir );

    PCB_IO_ODBPP odbExporter;

    if( aReporter )
        odbExporter.SetReporter( aReporter );

    std::map<std::string, UTF8> props;
    props["units"] = aUnits;
    props["sigfig"] = aSigfig;

    odbExporter.SaveBoard( wxString::FromUTF8( aDir.string() ), const_cast<BOARD&>( aBoard ), &props );

    return aDir;
}


std::vector<std::string> ReadLines( const fs::path& aFile )
{
    std::vector<std::string> lines;
    std::ifstream            stream( aFile );
    std::string              line;

    while( std::getline( stream, line ) )
        lines.push_back( line );

    return lines;
}


const KI_TEST::SCOPED_TEMP_DIR& TempDir()
{
    static std::vector<std::unique_ptr<KI_TEST::SCOPED_TEMP_DIR>> dirs;
    dirs.emplace_back( std::make_unique<KI_TEST::SCOPED_TEMP_DIR>( wxS( "odb_attributes" ) ) );
    return *dirs.back();
}


size_t CountPads( const BOARD& aBoard, PAD_PROP aProperty, PCB_LAYER_ID aLayer )
{
    size_t count = 0;

    for( const FOOTPRINT* footprint : aBoard.Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( pad->GetProperty() == aProperty && pad->IsOnLayer( aLayer ) )
                ++count;
        }
    }

    return count;
}


OdbFeatureFile::OdbFeatureFile( const fs::path& aFile )
{
    for( const std::string& line : ReadLines( aFile ) )
    {
        if( line.rfind( "@", 0 ) == 0 )
        {
            size_t space = line.find( ' ' );

            if( space != std::string::npos )
                m_names.emplace( line.substr( space + 1 ), line.substr( 1, space - 1 ) );
        }
        else if( line.rfind( "CMP ", 0 ) == 0
                 || ( line.size() > 2 && line[1] == ' '
                      && ( line[0] == 'P' || line[0] == 'L' || line[0] == 'A' || line[0] == 'S' ) ) )
        {
            m_records.push_back( line );
        }
    }
}


size_t OdbFeatureFile::CountRecordsWithOption( const std::string& aName, const std::string& aOption ) const
{
    static const std::map<std::string, std::vector<std::string>> options = {
        { ".pad_usage", { "toeprint", "via", "g_fiducial", "l_fiducial", "tooling_hole", "bond_finger" } },
        { ".drill", { "plated", "non_plated", "via" } },
        { ".plated_type", { "standard", "press_fit" } },
        { ".via_type", { "drilled", "laser", "photo" } },
        { ".comp_mount_type", { "other", "smd", "tht", "pressfit" } }
    };

    auto found = options.find( aName );

    if( found == options.end() )
        return 0;

    const auto& values = found->second;
    auto        option = std::find( values.begin(), values.end(), aOption );

    if( option == values.end() )
        return 0;

    return CountRecordsWithValue( aName, std::to_string( std::distance( values.begin(), option ) ) );
}


size_t OdbFeatureFile::CountRecordsWithFlag( const std::string& aName ) const
{
    return CountRecordsWithValue( aName, std::string() );
}


size_t OdbFeatureFile::CountRecordsWithInteger( const std::string& aName, int aValue ) const
{
    return CountRecordsWithValue( aName, std::to_string( aValue ) );
}


size_t OdbFeatureFile::CountRecordsWithValue( const std::string& aName, const std::string& aValue ) const
{
    auto name = m_names.find( aName );

    if( name == m_names.end() )
        return 0;

    size_t count = 0;

    for( const std::string& record : m_records )
    {
        size_t start = record.find( ';' );

        if( start == std::string::npos )
            continue;

        std::stringstream attributes( record.substr( start + 1 ) );
        std::string       attribute;

        while( std::getline( attributes, attribute, ',' ) )
        {
            if( attribute == name->second + ( aValue.empty() ? "" : "=" + aValue ) )
            {
                ++count;
                break;
            }
        }
    }

    return count;
}


ODB_PRODUCT::ODB_PRODUCT( const fs::path& aRoot )
{
    wxString stepName;
    wxString block;
    ODB_MATRIX_ROW row;

    FILE_LINE_READER reader( wxString::FromUTF8( ( aRoot / "matrix" / "matrix" ).string() ) );

    while( const char* line = reader.ReadLine() )
    {
        wxString text = wxString::FromUTF8( line ).Strip( wxString::both );

        if( text.EndsWith( wxS( "{" ) ) )
        {
            block = text.BeforeLast( '{' ).Strip( wxString::both );
            row = ODB_MATRIX_ROW();
            continue;
        }

        if( text == wxS( "}" ) )
        {
            if( block == wxS( "LAYER" ) )
                m_matrix.push_back( row );

            block.clear();
            continue;
        }

        wxString key = text.BeforeFirst( '=' ).Strip( wxString::both );
        wxString value = text.AfterFirst( '=' ).Strip( wxString::both );

        if( block == wxS( "STEP" ) && key == wxS( "NAME" ) && stepName.IsEmpty() )
        {
            stepName = value;
        }
        else if( block == wxS( "LAYER" ) )
        {
            if( key == wxS( "ROW" ) )
                row.m_row = wxAtoi( value );
            else if( key == wxS( "CONTEXT" ) )
                row.m_context = value;
            else if( key == wxS( "TYPE" ) )
                row.m_type = value;
            else if( key == wxS( "NAME" ) )
                row.m_name = value;
            else if( key == wxS( "POLARITY" ) )
                row.m_polarity = value;
            else if( key == wxS( "START_NAME" ) )
                row.m_startName = value;
            else if( key == wxS( "END_NAME" ) )
                row.m_endName = value;
            else if( key == wxS( "ADD_TYPE" ) )
                row.m_addType = value;
            else if( key == wxS( "ID" ) )
                row.m_id = value;
            else if( key == wxS( "REF" ) )
                row.m_ref = value;
            else if( key == wxS( "CU_TOP" ) )
                row.m_cuTop = value;
            else if( key == wxS( "CU_BOTTOM" ) )
                row.m_cuBottom = value;
            else if( key == wxS( "DIELECTRIC_TYPE" ) )
                row.m_dielectricType = value;
        }
    }

    std::error_code ec;

    for( const fs::directory_entry& entry : fs::directory_iterator( aRoot / "steps", ec ) )
    {
        if( entry.is_directory() && wxString::FromUTF8( entry.path().filename().string() ).IsSameAs( stepName, false ) )
            m_stepDir = entry.path();
    }
}


wxString ODB_PRODUCT::LayerFile( const wxString& aLayer, const wxString& aFile ) const
{
    fs::path        layersRoot = m_stepDir / "layers";
    fs::path        layerDir = layersRoot / aLayer.Lower().ToStdString();
    std::error_code ec;

    for( const fs::directory_entry& entry : fs::directory_iterator( layersRoot, ec ) )
    {
        if( entry.is_directory() && wxString::FromUTF8( entry.path().filename().string() ).IsSameAs( aLayer, false ) )
            layerDir = entry.path();
    }

    return wxString::FromUTF8( ( layerDir / aFile.ToStdString() ).string() );
}
