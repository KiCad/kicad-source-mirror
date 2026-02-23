/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pads_sch_parser.h"

#include <io/pads/pads_common.h>
#include <reporter.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <regex>

namespace PADS_SCH
{

PADS_SCH_PARSER::PADS_SCH_PARSER() :
    m_reporter( nullptr ),
    m_lineNumber( 0 ),
    m_currentSheet( 0 )
{
}


PADS_SCH_PARSER::~PADS_SCH_PARSER()
{
}


bool PADS_SCH_PARSER::isSectionMarker( const std::string& aLine ) const
{
    if( aLine.size() < 3 || aLine[0] != '*' )
        return false;

    size_t endPos = aLine.find( '*', 1 );
    return endPos != std::string::npos && endPos > 1;
}


std::string PADS_SCH_PARSER::extractSectionName( const std::string& aLine ) const
{
    if( aLine.size() < 3 || aLine[0] != '*' )
        return "";

    size_t endPos = aLine.find( '*', 1 );

    if( endPos == std::string::npos || endPos <= 1 )
        return "";

    return aLine.substr( 1, endPos - 1 );
}


bool PADS_SCH_PARSER::Parse( const std::string& aFileName )
{
    m_header = FILE_HEADER();
    m_parameters = PARAMETERS();
    m_symbolDefs.clear();
    m_partPlacements.clear();
    m_signals.clear();
    m_offPageConnectors.clear();
    m_partTypes.clear();
    m_tiedDots.clear();
    m_sheetHeaders.clear();
    m_textItems.clear();
    m_linesItems.clear();
    m_netNameLabels.clear();
    m_lineNumber = 0;
    m_currentSheet = 0;

    std::ifstream file( aFileName );

    if( !file.is_open() )
    {
        if( m_reporter )
            m_reporter->Report( wxString::Format( "Cannot open file: %s", aFileName ), RPT_SEVERITY_ERROR );

        return false;
    }

    std::vector<std::string> lines;
    std::string line;

    while( std::getline( file, line ) )
    {
        if( !line.empty() && line.back() == '\r' )
            line.pop_back();

        lines.push_back( line );
    }

    file.close();

    if( lines.empty() )
    {
        if( m_reporter )
            m_reporter->Report( "File is empty", RPT_SEVERITY_ERROR );

        return false;
    }

    if( !parseHeader( lines[0] ) )
    {
        if( m_reporter )
            m_reporter->Report( "Invalid PADS Logic file header", RPT_SEVERITY_ERROR );

        return false;
    }

    m_header.valid = true;

    for( size_t i = 1; i < lines.size(); i++ )
    {
        m_lineNumber = static_cast<int>( i + 1 );
        const std::string& currentLine = lines[i];

        if( currentLine.empty() )
            continue;

        // Skip remark lines
        if( currentLine.find( "*REMARK*" ) == 0 )
            continue;

        if( !isSectionMarker( currentLine ) )
            continue;

        std::string sectionName = extractSectionName( currentLine );

        if( sectionName == "SCH" )
        {
            i = parseSectionSCH( lines, i );
        }
        else if( sectionName == "CAM" || sectionName == "MISC" )
        {
            i = skipBraceDelimitedSection( lines, i );
        }
        else if( sectionName == "FIELDS" )
        {
            i = parseSectionFIELDS( lines, i );
        }
        else if( sectionName == "SHT" )
        {
            i = parseSectionSHT( lines, i );
        }
        else if( sectionName == "CAE" )
        {
            i = parseSectionCAE( lines, i );
        }
        else if( sectionName == "TEXT" )
        {
            i = parseSectionTEXT( lines, i );
        }
        else if( sectionName == "LINES" )
        {
            i = parseSectionLINES( lines, i );
        }
        else if( sectionName == "CAEDECAL" )
        {
            i = parseSectionCAEDECAL( lines, i );
        }
        else if( sectionName == "PARTTYPE" )
        {
            i = parseSectionPARTTYPE( lines, i );
        }
        else if( sectionName == "PART" )
        {
            i = parseSectionPART( lines, i );
        }
        else if( sectionName == "BUSSES" )
        {
            // Empty in all observed files, skip to next section
            continue;
        }
        else if( sectionName == "OFFPAGE REFS" )
        {
            i = parseSectionOFFPAGEREFS( lines, i );
        }
        else if( sectionName == "TIEDOTS" )
        {
            i = parseSectionTIEDOTS( lines, i );
        }
        else if( sectionName == "CONNECTION" )
        {
            i = parseSectionCONNECTION( lines, i );
        }
        else if( sectionName == "NETNAMES" )
        {
            i = parseSectionNETNAMES( lines, i );
        }
        else if( sectionName == "END" )
        {
            break;
        }
    }

    mergePartTypeData();

    return true;
}


void PADS_SCH_PARSER::mergePartTypeData()
{
    // Pin data from PARTTYPEs is applied at symbol build time via GATE_DEF::pins,
    // not mutated on shared SYMBOL_DEF objects. Multiple PARTTYPEs can reference
    // the same CAEDECAL with different pin mappings.

    for( auto& part : m_partPlacements )
    {
        for( auto& attr : part.attributes )
        {
            if( attr.name == "Ref.Des." && attr.value.empty() )
                attr.value = part.reference;

            auto ovr = part.attr_overrides.find( attr.name );

            if( ovr != part.attr_overrides.end() && attr.value.empty() )
                attr.value = ovr->second;
        }
    }
}


bool PADS_SCH_PARSER::CheckFileHeader( const std::string& aFileName )
{
    std::ifstream file( aFileName );

    if( !file.is_open() )
        return false;

    std::string firstLine;

    if( !std::getline( file, firstLine ) )
        return false;

    if( !firstLine.empty() && firstLine.back() == '\r' )
        firstLine.pop_back();

    if( firstLine.find( "*PADS-LOGIC" ) == 0 )
        return true;

    if( firstLine.find( "*PADS-POWERLOGIC" ) == 0 )
        return true;

    return false;
}


bool PADS_SCH_PARSER::parseHeader( const std::string& aLine )
{
    if( aLine.empty() || aLine[0] != '*' )
        return false;

    size_t endPos = aLine.find( '*', 1 );

    if( endPos == std::string::npos )
        return false;

    std::string headerTag = aLine.substr( 1, endPos - 1 );

    std::regex headerRegex( R"(PADS-(POWER)?LOGIC-V(\d+\.\d+))" );
    std::smatch match;

    if( !std::regex_match( headerTag, match, headerRegex ) )
        return false;

    if( match[1].matched )
        m_header.product = "PADS-POWERLOGIC";
    else
        m_header.product = "PADS-LOGIC";

    m_header.version = "V" + match[2].str();

    if( endPos + 1 < aLine.size() )
    {
        std::string desc = aLine.substr( endPos + 1 );
        size_t start = desc.find_first_not_of( ' ' );

        if( start != std::string::npos )
            m_header.description = desc.substr( start );
    }

    return true;
}


size_t PADS_SCH_PARSER::parseSectionSCH( const std::vector<std::string>& aLines, size_t aStartLine )
{
    size_t i = aStartLine + 1;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( isSectionMarker( line ) )
            return i - 1;

        if( line.empty() )
        {
            i++;
            continue;
        }

        std::istringstream iss( line );
        std::string keyword;
        iss >> keyword;

        if( keyword == "UNITS" )
        {
            int unitsVal = 0;
            iss >> unitsVal;

            switch( unitsVal )
            {
            case 0: m_parameters.units = UNIT_TYPE::MILS; break;
            case 1: m_parameters.units = UNIT_TYPE::METRIC; break;
            case 2: m_parameters.units = UNIT_TYPE::INCHES; break;
            default: m_parameters.units = UNIT_TYPE::MILS; break;
            }
        }
        else if( keyword == "CUR" )
        {
            // Compound keyword: CUR SHEET
            std::string second;
            iss >> second;

            if( second == "SHEET" )
                iss >> m_parameters.cur_sheet;
        }
        else if( keyword == "SHEET" )
        {
            // Compound keyword: SHEET SIZE
            std::string second;
            iss >> second;

            if( second == "SIZE" )
            {
                std::string sizeCode;
                iss >> sizeCode;

                m_parameters.sheet_size.name = sizeCode;

                if( sizeCode == "A" )
                {
                    m_parameters.sheet_size.width = 11000.0;
                    m_parameters.sheet_size.height = 8500.0;
                }
                else if( sizeCode == "B" )
                {
                    m_parameters.sheet_size.width = 17000.0;
                    m_parameters.sheet_size.height = 11000.0;
                }
                else if( sizeCode == "C" )
                {
                    m_parameters.sheet_size.width = 22000.0;
                    m_parameters.sheet_size.height = 17000.0;
                }
                else if( sizeCode == "D" )
                {
                    m_parameters.sheet_size.width = 34000.0;
                    m_parameters.sheet_size.height = 22000.0;
                }
                else if( sizeCode == "E" )
                {
                    m_parameters.sheet_size.width = 44000.0;
                    m_parameters.sheet_size.height = 34000.0;
                }
            }
        }
        else if( keyword == "USERGRID" )
        {
            iss >> m_parameters.grid_x >> m_parameters.grid_y;
        }
        else if( keyword == "LINEWIDTH" )
        {
            iss >> m_parameters.line_width;
        }
        else if( keyword == "CONNWIDTH" )
        {
            iss >> m_parameters.conn_width;
        }
        else if( keyword == "BUSWIDTH" )
        {
            iss >> m_parameters.bus_width;
        }
        else if( keyword == "BUSANGLE" )
        {
            iss >> m_parameters.bus_angle;
        }
        else if( keyword == "TEXTSIZE" )
        {
            iss >> m_parameters.text_h >> m_parameters.text_w;
            m_parameters.text_size = m_parameters.text_h;
        }
        else if( keyword == "PINNAMESIZE" )
        {
            iss >> m_parameters.pin_name_h >> m_parameters.pin_name_w;
        }
        else if( keyword == "REFNAMESIZE" )
        {
            iss >> m_parameters.ref_name_h >> m_parameters.ref_name_w;
        }
        else if( keyword == "PARTNAMESIZE" )
        {
            iss >> m_parameters.part_name_h >> m_parameters.part_name_w;
        }
        else if( keyword == "PINNOSIZE" )
        {
            iss >> m_parameters.pin_no_h >> m_parameters.pin_no_w;
        }
        else if( keyword == "NETNAMESIZE" )
        {
            iss >> m_parameters.net_name_h >> m_parameters.net_name_w;
        }
        else if( keyword == "DOTGRID" )
        {
            iss >> m_parameters.dot_grid;
        }
        else if( keyword == "TIEDOTSIZE" )
        {
            iss >> m_parameters.tied_dot_size;
        }
        else if( keyword == "REAL" )
        {
            // Compound keyword: REAL WIDTH
            std::string second;
            iss >> second;

            if( second == "WIDTH" )
                iss >> m_parameters.real_width;
        }
        else if( keyword == "FONT" )
        {
            // Compound keyword: FONT MODE
            std::string second;
            iss >> second;

            if( second == "MODE" )
                iss >> m_parameters.font_mode;
        }
        else if( keyword == "DEFAULT" )
        {
            // Compound keyword: DEFAULT FONT
            std::string second;
            iss >> second;

            if( second == "FONT" )
            {
                std::string rest;
                std::getline( iss, rest );
                size_t start = rest.find( '"' );

                if( start != std::string::npos )
                {
                    size_t end = rest.find( '"', start + 1 );

                    if( end != std::string::npos )
                        m_parameters.default_font = rest.substr( start + 1, end - start - 1 );
                }
            }
        }
        else if( keyword == "BORDER" )
        {
            // Compound keyword: BORDER NAME
            std::string second;
            iss >> second;

            if( second == "NAME" )
            {
                std::string name;
                iss >> name;
                m_parameters.border_template = name;
            }
            else
            {
                // Might just be "BORDER value" in simplified test format
                m_parameters.border_template = second;
            }
        }
        else if( keyword == "JOBNAME" )
        {
            std::string rest;
            std::getline( iss, rest );
            size_t start = rest.find_first_not_of( " \t" );

            if( start != std::string::npos )
            {
                rest = rest.substr( start );

                if( rest.size() >= 2 && rest.front() == '"' && rest.back() == '"' )
                    rest = rest.substr( 1, rest.size() - 2 );

                m_parameters.job_name = rest;
            }
        }
        // Color keywords and other numeric keywords -- just skip without error
        else if( keyword == "NTXCOL" || keyword == "HIRCOL" || keyword == "LINCOL" ||
                 keyword == "TXTCOL" || keyword == "CONCOL" || keyword == "BUSCOL" ||
                 keyword == "PTXCOL" || keyword == "COMCOL" || keyword == "NMCOL" ||
                 keyword == "PNMCOL" || keyword == "PINCOL" || keyword == "NETCOL" ||
                 keyword == "FBGCOL" || keyword == "FIELDCOL" ||
                 keyword == "NNVISPWRGND" || keyword == "PCBFLAGS" ||
                 keyword == "JOBTIME" || keyword == "BACKUPTIME" ||
                 keyword == "OFFREFVIEW" || keyword == "OFFREFNUM" ||
                 keyword == "SHEETNUMSEP" )
        {
            // Skip these, values consumed by getline
        }

        i++;
    }

    return aLines.size() - 1;
}


size_t PADS_SCH_PARSER::parseSectionFIELDS( const std::vector<std::string>& aLines, size_t aStartLine )
{
    size_t i = aStartLine + 1;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( isSectionMarker( line ) )
            return i - 1;

        if( line.empty() || line[0] != '"' )
        {
            i++;
            continue;
        }

        // Format: "Field Name" [value text]
        size_t closeQuote = line.find( '"', 1 );

        if( closeQuote != std::string::npos )
        {
            std::string fieldName = line.substr( 1, closeQuote - 1 );
            std::string fieldValue;

            if( closeQuote + 1 < line.size() )
            {
                fieldValue = line.substr( closeQuote + 1 );
                size_t start = fieldValue.find_first_not_of( " \t" );

                if( start != std::string::npos )
                    fieldValue = fieldValue.substr( start );
                else
                    fieldValue.clear();
            }

            m_parameters.fields[fieldName] = fieldValue;
        }

        i++;
    }

    return aLines.size() - 1;
}


size_t PADS_SCH_PARSER::parseSectionSHT( const std::vector<std::string>& aLines, size_t aStartLine )
{
    // Format: *SHT*   sheet_num sheet_name parent_num parent_name
    const std::string& line = aLines[aStartLine];
    std::string afterMarker = line.substr( line.find( '*', 1 ) + 1 );

    std::istringstream iss( afterMarker );
    SHEET_HEADER header;

    iss >> header.sheet_num >> header.sheet_name >> header.parent_num >> header.parent_name;

    m_currentSheet = header.sheet_num;
    m_sheetHeaders.push_back( header );

    return aStartLine;
}


size_t PADS_SCH_PARSER::parseSectionCAE( const std::vector<std::string>& aLines, size_t aStartLine )
{
    // *CAE* just contains viewport parameters, skip until next section
    size_t i = aStartLine + 1;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( isSectionMarker( line ) )
            return i - 1;

        i++;
    }

    return aLines.size() - 1;
}


size_t PADS_SCH_PARSER::parseSectionTEXT( const std::vector<std::string>& aLines, size_t aStartLine )
{
    size_t i = aStartLine + 1;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( line.empty() )
        {
            i++;
            continue;
        }

        if( isSectionMarker( line ) )
            return i - 1;

        // Each text item is two lines: attribute line + content line
        TEXT_ITEM item;
        std::istringstream iss( line );

        int x = 0, y = 0;
        iss >> x >> y >> item.rotation >> item.justification >> item.height >> item.width_factor
            >> item.attr_flag;

        item.position.x = x;
        item.position.y = y;

        // Parse quoted font name from remainder
        std::string rest;
        std::getline( iss, rest );
        size_t qStart = rest.find( '"' );

        if( qStart != std::string::npos )
        {
            size_t qEnd = rest.find( '"', qStart + 1 );

            if( qEnd != std::string::npos )
                item.font_name = rest.substr( qStart + 1, qEnd - qStart - 1 );
        }

        // Next line is content
        i++;

        if( i < aLines.size() )
            item.content = aLines[i];

        m_textItems.push_back( item );
        i++;
    }

    return aLines.size() - 1;
}


size_t PADS_SCH_PARSER::parseSectionLINES( const std::vector<std::string>& aLines, size_t aStartLine )
{
    size_t i = aStartLine + 1;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( line.empty() )
        {
            i++;
            continue;
        }

        if( isSectionMarker( line ) )
            return i - 1;

        // Look for LINES item header: name LINES x y param1 param2
        if( line.find( "LINES" ) != std::string::npos )
        {
            LINES_ITEM item;
            std::istringstream iss( line );
            std::string name, keyword;
            int x = 0, y = 0;

            iss >> name >> keyword >> x >> y >> item.param1 >> item.param2;

            if( keyword == "LINES" )
            {
                item.name = name;
                item.origin.x = x;
                item.origin.y = y;

                i++;

                // Parse graphic primitives and embedded text until next LINES item or section
                while( i < aLines.size() )
                {
                    const std::string& pline = aLines[i];

                    if( pline.empty() )
                    {
                        i++;
                        continue;
                    }

                    if( isSectionMarker( pline ) )
                        break;

                    // Check if this is a new LINES item header
                    if( pline.find( "LINES" ) != std::string::npos )
                    {
                        std::istringstream tiss( pline );
                        std::string tname, tkw;
                        tiss >> tname >> tkw;

                        if( tkw == "LINES" )
                            break;
                    }

                    std::istringstream piss( pline );
                    std::string firstToken;
                    piss >> firstToken;

                    if( firstToken == "OPEN" || firstToken == "CLOSED" ||
                        firstToken == "CIRCLE" || firstToken == "COPCLS" )
                    {
                        SYMBOL_GRAPHIC graphic;
                        i = parseGraphicPrimitive( aLines, i, graphic );
                        item.primitives.push_back( graphic );
                        i++;
                        continue;
                    }

                    // Try to parse as text (attribute line + content) if it starts with
                    // a number that could be a coordinate
                    bool isNumber = !firstToken.empty()
                                    && ( std::isdigit( firstToken[0] )
                                         || firstToken[0] == '-'
                                         || firstToken[0] == '+' );

                    if( isNumber && pline.find( '"' ) != std::string::npos )
                    {
                        // Text attribute line
                        TEXT_ITEM text;
                        std::istringstream tiss( pline );
                        int tx = 0, ty = 0;

                        tiss >> tx >> ty >> text.rotation >> text.justification >> text.height
                             >> text.width_factor;

                        text.position.x = tx;
                        text.position.y = ty;

                        std::string trest;
                        std::getline( tiss, trest );
                        size_t qStart = trest.find( '"' );

                        if( qStart != std::string::npos )
                        {
                            size_t qEnd = trest.find( '"', qStart + 1 );

                            if( qEnd != std::string::npos )
                                text.font_name = trest.substr( qStart + 1, qEnd - qStart - 1 );
                        }

                        i++;

                        if( i < aLines.size() )
                            text.content = aLines[i];

                        item.texts.push_back( text );
                    }

                    i++;
                }

                m_linesItems.push_back( item );
                continue;
            }
        }

        i++;
    }

    return aLines.size() - 1;
}


size_t PADS_SCH_PARSER::parseSectionCAEDECAL( const std::vector<std::string>& aLines,
                                               size_t aStartLine )
{
    size_t i = aStartLine + 1;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( line.empty() )
        {
            i++;
            continue;
        }

        if( isSectionMarker( line ) )
            break;

        // Each entry starts with a header line of 13 fields
        SYMBOL_DEF symbol;
        i = parseSymbolDef( aLines, i, symbol );

        if( !symbol.name.empty() )
            m_symbolDefs.push_back( std::move( symbol ) );

        i++;
    }

    // Resolve pin lengths from pin decal OPEN line geometry. The name-based
    // heuristic (SHORT/LONG) only covers standard pin decals; custom names
    // like PIN_ADI_300 need their length computed from the actual graphics.
    std::map<std::string, double> pinDecalLengths;

    for( const auto& sym : m_symbolDefs )
    {
        if( !sym.is_pin_decal )
            continue;

        for( const auto& graphic : sym.graphics )
        {
            if( graphic.type != GRAPHIC_TYPE::LINE && graphic.type != GRAPHIC_TYPE::POLYLINE )
                continue;

            if( graphic.points.size() < 2 )
                continue;

            const auto& first = graphic.points.front().coord;
            const auto& last = graphic.points.back().coord;
            double dx = last.x - first.x;
            double dy = last.y - first.y;
            pinDecalLengths[sym.name] = std::sqrt( dx * dx + dy * dy );
            break;
        }
    }

    for( auto& sym : m_symbolDefs )
    {
        if( sym.is_pin_decal )
            continue;

        for( auto& pin : sym.pins )
        {
            if( pin.pin_decal_name.empty() )
                continue;

            auto it = pinDecalLengths.find( pin.pin_decal_name );

            if( it != pinDecalLengths.end() )
                pin.length = it->second;
        }
    }

    return i > 0 ? i - 1 : aLines.size() - 1;
}


size_t PADS_SCH_PARSER::parseSymbolDef( const std::vector<std::string>& aLines, size_t aStartLine,
                                         SYMBOL_DEF& aSymbol )
{
    if( aStartLine >= aLines.size() )
        return aStartLine;

    const std::string& headerLine = aLines[aStartLine];

    // Parse header: name f1 f2 height width h2 w2 num_attrs num_pieces has_polarity num_pins
    //               pin_origin_code is_pin_decal
    std::istringstream iss( headerLine );
    std::string name;
    iss >> name;

    if( name.empty() )
        return aStartLine;

    aSymbol.name = name;

    // Try parsing as full 13-field CAEDECAL format
    std::vector<std::string> tokens;
    std::string token;

    while( iss >> token )
        tokens.push_back( token );

    if( tokens.size() >= 12 )
    {
        // Full CAEDECAL format
        aSymbol.f1 = PADS_COMMON::ParseInt( tokens[0], 0, "CAEDECAL header" );
        aSymbol.f2 = PADS_COMMON::ParseInt( tokens[1], 0, "CAEDECAL header" );
        aSymbol.height = PADS_COMMON::ParseInt( tokens[2], 0, "CAEDECAL header" );
        aSymbol.width = PADS_COMMON::ParseInt( tokens[3], 0, "CAEDECAL header" );
        aSymbol.h2 = PADS_COMMON::ParseInt( tokens[4], 0, "CAEDECAL header" );
        aSymbol.w2 = PADS_COMMON::ParseInt( tokens[5], 0, "CAEDECAL header" );
        aSymbol.num_attrs = PADS_COMMON::ParseInt( tokens[6], 0, "CAEDECAL header" );
        aSymbol.num_pieces = PADS_COMMON::ParseInt( tokens[7], 0, "CAEDECAL header" );
        aSymbol.has_polarity = PADS_COMMON::ParseInt( tokens[8], 0, "CAEDECAL header" );
        aSymbol.num_pins = PADS_COMMON::ParseInt( tokens[9], 0, "CAEDECAL header" );
        aSymbol.pin_origin_code = PADS_COMMON::ParseInt( tokens[10], 0, "CAEDECAL header" );
        aSymbol.is_pin_decal = PADS_COMMON::ParseInt( tokens[11], 0, "CAEDECAL header" );
    }
    else
    {
        // Simplified test format: name num_pieces num_pins gate_count
        if( tokens.size() >= 3 )
        {
            aSymbol.num_pieces = PADS_COMMON::ParseInt( tokens[0], 0, "CAEDECAL simplified" );
            aSymbol.num_pins = PADS_COMMON::ParseInt( tokens[1], 0, "CAEDECAL simplified" );
            aSymbol.gate_count = PADS_COMMON::ParseInt( tokens[2], 0, "CAEDECAL simplified" );
        }
        else
        {
            return aStartLine;
        }

        // Parse simplified format graphics and pins inline
        size_t idx = aStartLine + 1;

        for( int p = 0; p < aSymbol.num_pieces && idx < aLines.size(); p++ )
        {
            const std::string& gline = aLines[idx];

            if( gline.empty() || isSectionMarker( gline ) )
                break;

            SYMBOL_GRAPHIC graphic;
            std::istringstream giss( gline );
            std::string typeStr;
            giss >> typeStr;

            if( typeStr == "OPEN" || typeStr == "LINE" )
            {
                graphic.type = GRAPHIC_TYPE::LINE;
                POINT p1, p2;
                giss >> p1.x >> p1.y >> p2.x >> p2.y;
                graphic.points.push_back( { p1, std::nullopt } );
                graphic.points.push_back( { p2, std::nullopt } );
                giss >> graphic.line_width;
            }
            else if( typeStr == "CLOSED" || typeStr == "RECT" )
            {
                graphic.type = GRAPHIC_TYPE::RECTANGLE;
                POINT p1, p2;
                giss >> p1.x >> p1.y >> p2.x >> p2.y;
                graphic.points.push_back( { p1, std::nullopt } );
                graphic.points.push_back( { p2, std::nullopt } );
                giss >> graphic.line_width;
            }
            else if( typeStr == "CIRCLE" )
            {
                graphic.type = GRAPHIC_TYPE::CIRCLE;
                giss >> graphic.center.x >> graphic.center.y >> graphic.radius;
                giss >> graphic.line_width;
            }

            aSymbol.graphics.push_back( graphic );
            idx++;
        }

        for( int p = 0; p < aSymbol.num_pins && idx < aLines.size(); p++ )
        {
            const std::string& pline = aLines[idx];

            if( pline.empty() || isSectionMarker( pline ) )
                break;

            SYMBOL_PIN pin;
            std::istringstream piss( pline );
            double orientation = 0;

            piss >> pin.position.x >> pin.position.y >> orientation >> pin.length;
            piss >> pin.number >> pin.name;

            pin.rotation = orientation;

            std::string typeStr;

            if( piss >> typeStr )
                pin.type = parsePinType( typeStr );

            aSymbol.pins.push_back( pin );
            idx++;
        }

        return idx - 1;
    }

    // Full CAEDECAL format parsing
    size_t idx = aStartLine + 1;

    // TIMESTAMP line
    if( idx < aLines.size() && aLines[idx].find( "TIMESTAMP" ) == 0 )
    {
        std::istringstream tiss( aLines[idx] );
        std::string kw;
        tiss >> kw >> aSymbol.timestamp;
        idx++;
    }

    // Two font name lines (optional, not present in older formats like V5.2)
    if( idx < aLines.size() && aLines[idx].size() >= 2 && aLines[idx][0] == '"' )
    {
        size_t qEnd = aLines[idx].find( '"', 1 );

        if( qEnd != std::string::npos )
            aSymbol.font1 = aLines[idx].substr( 1, qEnd - 1 );

        idx++;
    }

    if( idx < aLines.size() && aLines[idx].size() >= 2 && aLines[idx][0] == '"' )
    {
        size_t qEnd = aLines[idx].find( '"', 1 );

        if( qEnd != std::string::npos )
            aSymbol.font2 = aLines[idx].substr( 1, qEnd - 1 );

        idx++;
    }

    // Attribute label pairs (num_attrs pairs of position+name lines)
    for( int a = 0; a < aSymbol.num_attrs && idx + 1 < aLines.size(); a++ )
    {
        CAEDECAL_ATTR attr;
        std::istringstream aiss( aLines[idx] );
        int x = 0, y = 0;

        aiss >> x >> y >> attr.angle >> attr.justification >> attr.height >> attr.width;
        attr.position.x = x;
        attr.position.y = y;

        // Parse quoted font name
        std::string rest;
        std::getline( aiss, rest );
        size_t qStart = rest.find( '"' );

        if( qStart != std::string::npos )
        {
            size_t qEnd = rest.find( '"', qStart + 1 );

            if( qEnd != std::string::npos )
                attr.font_name = rest.substr( qStart + 1, qEnd - qStart - 1 );
        }

        idx++;

        if( idx < aLines.size() )
            attr.attr_name = aLines[idx];

        aSymbol.attrs.push_back( attr );
        idx++;
    }

    // Graphic primitives (num_pieces)
    for( int p = 0; p < aSymbol.num_pieces && idx < aLines.size(); p++ )
    {
        if( aLines[idx].empty() )
        {
            idx++;
            p--;
            continue;
        }

        SYMBOL_GRAPHIC graphic;
        idx = parseGraphicPrimitive( aLines, idx, graphic );
        aSymbol.graphics.push_back( graphic );
        idx++;
    }

    // Embedded text labels (between graphics and pins)
    // Scan forward looking for T-prefixed pin lines. A blank line marks the end of
    // the current CAEDECAL entry (important for entries with num_pins=0 like pin decals).
    while( idx < aLines.size() )
    {
        const std::string& tline = aLines[idx];

        if( tline.empty() )
            break;

        if( isSectionMarker( tline ) )
            break;

        // Pin T-lines start with T immediately followed by a digit or minus sign
        if( tline.size() > 1 && tline[0] == 'T' &&
            ( std::isdigit( static_cast<unsigned char>( tline[1] ) ) || tline[1] == '-' ) )
        {
            break;
        }

        // This is an embedded text label (two-line format)
        SYMBOL_TEXT text;
        std::istringstream tiss( tline );
        int tx = 0, ty = 0;

        tiss >> tx >> ty >> text.rotation >> text.justification;

        int height = 0, width = 0;
        tiss >> height >> width;
        text.size = height;
        text.width_factor = width;

        text.position.x = tx;
        text.position.y = ty;

        idx++;

        if( idx < aLines.size() )
        {
            text.content = aLines[idx];
            idx++;
        }

        aSymbol.texts.push_back( text );
    }

    // Pin T/P line pairs (num_pins)
    for( int p = 0; p < aSymbol.num_pins && idx < aLines.size(); p++ )
    {
        const std::string& tLine = aLines[idx];

        if( tLine.empty() )
        {
            idx++;
            p--;
            continue;
        }

        if( isSectionMarker( tLine ) )
            break;

        SYMBOL_PIN pin;

        // T line: T<x> y angle side pn_h pn_w pn_angle pn_just pl_h pl_w pl_angle pl_just
        //         pin_decal_name
        if( tLine.size() > 1 && tLine[0] == 'T' )
        {
            // T immediately followed by x coordinate
            std::string tContent = tLine.substr( 1 );
            std::istringstream tiss( tContent );
            int tx = 0, ty = 0, angle = 0;

            tiss >> tx >> ty >> angle >> pin.side >> pin.pn_h >> pin.pn_w >> pin.pn_angle
                 >> pin.pn_just >> pin.pl_h >> pin.pl_w >> pin.pl_angle >> pin.pl_just
                 >> pin.pin_decal_name;

            pin.position.x = tx;
            pin.position.y = ty;
            pin.rotation = angle;

            // Determine inverted/clock from pin decal name
            if( pin.pin_decal_name == "PINB" || pin.pin_decal_name == "PINORB" ||
                pin.pin_decal_name == "PCLKB" || pin.pin_decal_name == "PINIEB" ||
                pin.pin_decal_name == "PINCLKB" )
            {
                pin.inverted = true;
            }

            if( pin.pin_decal_name == "PCLK" || pin.pin_decal_name == "PCLKB" ||
                pin.pin_decal_name == "PINCLK" || pin.pin_decal_name == "PINCLKB" )
            {
                pin.clock = true;
            }

            // Pin stub length from pin decal name. Self-contained pin decals
            // (no pin_decal_name) draw the pin graphics themselves so the
            // KiCad stub length should be zero.
            if( pin.pin_decal_name.empty() )
                pin.length = 0.0;
            else if( pin.pin_decal_name.find( "SHORT" ) != std::string::npos )
                pin.length = 100.0;
            else if( pin.pin_decal_name.find( "LONG" ) != std::string::npos )
                pin.length = 300.0;
        }

        idx++;

        // P line: P<x1> y1 angle1 just1 x2 y2 angle2 just2 flags
        if( idx < aLines.size() )
        {
            const std::string& pLine = aLines[idx];

            if( pLine.size() > 1 && pLine[0] == 'P' )
            {
                std::string pContent = pLine.substr( 1 );
                std::istringstream piss( pContent );
                int px1 = 0, py1 = 0, px2 = 0, py2 = 0;

                piss >> px1 >> py1 >> pin.pn_off_angle >> pin.pn_off_just
                     >> px2 >> py2 >> pin.pl_off_angle >> pin.pl_off_just >> pin.p_flags;

                pin.pn_offset.x = px1;
                pin.pn_offset.y = py1;
                pin.pl_offset.x = px2;
                pin.pl_offset.y = py2;

                // Pin name hidden if flags bit 128 set
                if( pin.p_flags & 128 )
                    pin.name = "";
            }

            idx++;
        }

        // Pin number is derived from index+1 by default. The actual assignment
        // comes from the PARTTYPE section, so we use a placeholder.
        pin.number = std::to_string( p + 1 );

        aSymbol.pins.push_back( pin );
    }

    return idx > 0 ? idx - 1 : 0;
}


size_t PADS_SCH_PARSER::parseGraphicPrimitive( const std::vector<std::string>& aLines,
                                                size_t aStartLine, SYMBOL_GRAPHIC& aGraphic )
{
    if( aStartLine >= aLines.size() )
        return aStartLine;

    const std::string& headerLine = aLines[aStartLine];
    std::istringstream iss( headerLine );
    std::string typeStr;
    int pointCount = 0, lineWidth = 0, lineStyle = 255;

    iss >> typeStr >> pointCount >> lineWidth >> lineStyle;

    aGraphic.line_width = lineWidth;
    aGraphic.line_style = lineStyle;

    if( typeStr == "OPEN" )
    {
        aGraphic.type = GRAPHIC_TYPE::POLYLINE;
        aGraphic.filled = false;
    }
    else if( typeStr == "CLOSED" )
    {
        aGraphic.type = GRAPHIC_TYPE::RECTANGLE;
        aGraphic.filled = false;
    }
    else if( typeStr == "CIRCLE" )
    {
        aGraphic.type = GRAPHIC_TYPE::CIRCLE;
        aGraphic.filled = false;
    }
    else if( typeStr == "COPCLS" )
    {
        aGraphic.type = GRAPHIC_TYPE::RECTANGLE;
        aGraphic.filled = true;
    }

    // Parse point data
    size_t idx = aStartLine + 1;

    for( int p = 0; p < pointCount && idx < aLines.size(); p++ )
    {
        const std::string& ptLine = aLines[idx];

        if( ptLine.empty() || isSectionMarker( ptLine ) )
            break;

        std::istringstream piss( ptLine );
        GRAPHIC_POINT gpt;
        piss >> gpt.coord.x >> gpt.coord.y;

        std::vector<std::string> extraTokens;
        std::string tok;

        while( piss >> tok )
            extraTokens.push_back( tok );

        if( extraTokens.size() >= 6 )
        {
            ARC_DATA arcData;
            arcData.bulge = PADS_COMMON::ParseDouble( extraTokens[0], 0.0, "arc data" );
            arcData.angle = PADS_COMMON::ParseDouble( extraTokens[1], 0.0, "arc data" );
            arcData.bbox_x1 = PADS_COMMON::ParseDouble( extraTokens[2], 0.0, "arc data" );
            arcData.bbox_y1 = PADS_COMMON::ParseDouble( extraTokens[3], 0.0, "arc data" );
            arcData.bbox_x2 = PADS_COMMON::ParseDouble( extraTokens[4], 0.0, "arc data" );
            arcData.bbox_y2 = PADS_COMMON::ParseDouble( extraTokens[5], 0.0, "arc data" );
            gpt.arc = arcData;
        }

        aGraphic.points.push_back( gpt );

        idx++;
    }

    // For CLOSED/COPCLS polygons, determine whether the points form an axis-aligned rectangle
    // or a general polygon. Rectangles are reduced to 2 corner points (min, max) for the builder.
    // Non-rectangular shapes (triangles, arbitrary polygons) become POLYLINE primitives.
    if( aGraphic.type == GRAPHIC_TYPE::RECTANGLE && aGraphic.points.size() >= 4 )
    {
        std::set<double> uniqueX, uniqueY;

        for( const auto& pt : aGraphic.points )
        {
            uniqueX.insert( pt.coord.x );
            uniqueY.insert( pt.coord.y );
        }

        bool isRect = ( uniqueX.size() == 2 && uniqueY.size() == 2 );

        if( isRect )
        {
            double minX = *uniqueX.begin();
            double maxX = *uniqueX.rbegin();
            double minY = *uniqueY.begin();
            double maxY = *uniqueY.rbegin();

            aGraphic.points.clear();
            aGraphic.points.push_back( { { minX, minY }, std::nullopt } );
            aGraphic.points.push_back( { { maxX, maxY }, std::nullopt } );
        }
        else
        {
            aGraphic.type = GRAPHIC_TYPE::POLYLINE;
        }
    }

    // For CIRCLE with 2 points, compute center and radius
    if( aGraphic.type == GRAPHIC_TYPE::CIRCLE && aGraphic.points.size() == 2 )
    {
        aGraphic.center.x = ( aGraphic.points[0].coord.x + aGraphic.points[1].coord.x ) / 2.0;
        aGraphic.center.y = ( aGraphic.points[0].coord.y + aGraphic.points[1].coord.y ) / 2.0;
        double dx = aGraphic.points[1].coord.x - aGraphic.points[0].coord.x;
        double dy = aGraphic.points[1].coord.y - aGraphic.points[0].coord.y;
        aGraphic.radius = std::sqrt( dx * dx + dy * dy ) / 2.0;
    }

    return idx > 0 ? idx - 1 : aStartLine;
}


size_t PADS_SCH_PARSER::parseSectionPARTTYPE( const std::vector<std::string>& aLines,
                                               size_t aStartLine )
{
    size_t i = aStartLine + 1;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( line.empty() )
        {
            i++;
            continue;
        }

        if( isSectionMarker( line ) )
            return i - 1;

        // Part type header: name category num_physical num_sigpins unused num_swap_groups
        PARTTYPE_DEF pt;
        std::istringstream iss( line );
        iss >> pt.name >> pt.category >> pt.num_physical >> pt.num_sigpins
            >> pt.unused >> pt.num_swap_groups;

        if( pt.name.empty() )
        {
            i++;
            continue;
        }

        i++;

        // TIMESTAMP line
        if( i < aLines.size() && aLines[i].find( "TIMESTAMP" ) == 0 )
        {
            std::istringstream tiss( aLines[i] );
            std::string kw;
            tiss >> kw >> pt.timestamp;
            i++;
        }

        // Check for special symbols ($GND_SYMS, $PWR_SYMS, $OSR_SYMS)
        bool isSpecial = ( pt.name == "$GND_SYMS" || pt.name == "$PWR_SYMS" ||
                           pt.name == "$OSR_SYMS" );

        // V5.2 uses "G:decalname swap num_pins" for gate definitions while V9.0+ uses
        // "GATE num_variants num_pins swap" followed by decal name lines. Detect by
        // checking whether the first content line starts with "G:".
        bool isV52Gates = ( i < aLines.size() && aLines[i].size() >= 3
                            && aLines[i][0] == 'G' && aLines[i][1] == ':' );

        if( isSpecial && !isV52Gates )
        {
            // V9.0+ special symbol format: keyword num_variants, then variant lines
            if( i < aLines.size() )
            {
                std::istringstream siss( aLines[i] );
                int numVariants = 0;
                siss >> pt.special_keyword >> numVariants;
                i++;

                for( int v = 0; v < numVariants && i < aLines.size(); v++ )
                {
                    PARTTYPE_DEF::SPECIAL_VARIANT sv;
                    std::istringstream viss( aLines[i] );
                    viss >> sv.decal_name >> sv.pin_type;

                    std::string rest;

                    if( viss >> rest )
                        sv.net_suffix = rest;

                    pt.special_variants.push_back( sv );
                    i++;
                }
            }
        }
        else if( i < aLines.size() )
        {
            if( isSpecial )
            {
                if( pt.name == "$GND_SYMS" )
                    pt.special_keyword = "GND";
                else if( pt.name == "$PWR_SYMS" )
                    pt.special_keyword = "PWR";
                else
                    pt.special_keyword = "OSR";
            }

            // Standard parts have GATE, CONN, or V5.2 G: blocks
            while( i < aLines.size() )
            {
                const std::string& gline = aLines[i];

                if( gline.empty() )
                {
                    // Blank line ends this parttype entry
                    break;
                }

                if( isSectionMarker( gline ) )
                    break;

                std::istringstream giss( gline );
                std::string keyword;
                giss >> keyword;

                if( keyword == "GATE" )
                {
                    GATE_DEF gate;
                    giss >> gate.num_decal_variants >> gate.num_pins >> gate.swap_flag;
                    i++;

                    // Decal name lines
                    for( int d = 0; d < gate.num_decal_variants && i < aLines.size(); d++ )
                    {
                        if( aLines[i].empty() || isSectionMarker( aLines[i] ) )
                            break;

                        gate.decal_names.push_back( aLines[i] );
                        i++;
                    }

                    // Pin definition lines
                    for( int p = 0; p < gate.num_pins && i < aLines.size(); p++ )
                    {
                        if( aLines[i].empty() || isSectionMarker( aLines[i] ) )
                            break;

                        PARTTYPE_PIN pin;
                        std::istringstream piss( aLines[i] );
                        std::string pinType;

                        piss >> pin.pin_id >> pin.swap_group >> pinType;

                        if( !pinType.empty() )
                            pin.pin_type = pinType[0];

                        std::string pinName;

                        if( piss >> pinName )
                            pin.pin_name = pinName;

                        gate.pins.push_back( pin );
                        i++;
                    }

                    pt.gates.push_back( gate );
                    continue;
                }
                else if( keyword.size() >= 3 && keyword[0] == 'G' && keyword[1] == ':' )
                {
                    // V5.2 gate format: G:decal1[:decal2:...] swap_flag num_pins
                    // Pin lines use dot-separated fields with multiple pins per line.
                    if( pt.category == "CON" )
                        pt.is_connector = true;

                    GATE_DEF gate;

                    std::string decalStr = keyword.substr( 2 );
                    std::istringstream diss( decalStr );
                    std::string decalName;

                    while( std::getline( diss, decalName, ':' ) )
                    {
                        if( !decalName.empty() )
                            gate.decal_names.push_back( decalName );
                    }

                    gate.num_decal_variants = static_cast<int>( gate.decal_names.size() );
                    giss >> gate.swap_flag >> gate.num_pins;
                    i++;

                    int pinsRead = 0;

                    while( pinsRead < gate.num_pins && i < aLines.size() )
                    {
                        const std::string& pline = aLines[i];

                        if( pline.empty() || isSectionMarker( pline ) )
                            break;

                        if( ( pline[0] == 'G' && pline.size() >= 2 && pline[1] == ':' )
                            || pline.find( "SIGPIN" ) == 0 )
                        {
                            break;
                        }

                        std::istringstream piss( pline );
                        std::string pinToken;

                        while( piss >> pinToken && pinsRead < gate.num_pins )
                        {
                            PARTTYPE_PIN pin;
                            std::vector<std::string> fields;
                            std::istringstream fiss( pinToken );
                            std::string field;

                            while( std::getline( fiss, field, '.' ) )
                                fields.push_back( field );

                            if( fields.size() >= 1 )
                                pin.pin_id = fields[0];

                            if( fields.size() >= 2 )
                            {
                                pin.swap_group =
                                        PADS_COMMON::ParseInt( fields[1], 0, "V5.2 pin" );
                            }

                            if( fields.size() >= 3 && !fields[2].empty() )
                                pin.pin_type = fields[2][0];

                            if( fields.size() >= 4 )
                                pin.pin_name = fields[3];

                            gate.pins.push_back( pin );
                            pinsRead++;
                        }

                        i++;
                    }

                    if( isSpecial )
                    {
                        PARTTYPE_DEF::SPECIAL_VARIANT sv;
                        sv.decal_name =
                                gate.decal_names.empty() ? "" : gate.decal_names[0];

                        if( !gate.pins.empty() )
                            sv.pin_type = std::string( 1, gate.pins[0].pin_type );

                        pt.special_variants.push_back( sv );
                    }

                    pt.gates.push_back( gate );
                    continue;
                }
                else if( keyword == "CONN" )
                {
                    pt.is_connector = true;
                    GATE_DEF gate;
                    int numPins = 0;
                    giss >> gate.num_decal_variants >> numPins;
                    gate.num_pins = numPins;
                    i++;

                    // Decal name + pin_type lines
                    for( int d = 0; d < gate.num_decal_variants && i < aLines.size(); d++ )
                    {
                        if( aLines[i].empty() || isSectionMarker( aLines[i] ) )
                            break;

                        std::istringstream diss( aLines[i] );
                        std::string decalName, pinType;
                        diss >> decalName >> pinType;
                        gate.decal_names.push_back( decalName );
                        i++;
                    }

                    // Pin definition lines
                    for( int p = 0; p < numPins && i < aLines.size(); p++ )
                    {
                        if( aLines[i].empty() || isSectionMarker( aLines[i] ) )
                            break;

                        PARTTYPE_PIN pin;
                        std::istringstream piss( aLines[i] );
                        std::string pinType;

                        piss >> pin.pin_id >> pin.swap_group >> pinType;

                        if( !pinType.empty() )
                            pin.pin_type = pinType[0];

                        gate.pins.push_back( pin );
                        i++;
                    }

                    pt.gates.push_back( gate );
                    continue;
                }
                else if( keyword == "SIGPIN" )
                {
                    PARTTYPE_DEF::SIGPIN sp;
                    std::string token;
                    giss >> token;

                    // V5.2 uses dot-separated fields (e.g. "1.50.DGND") while
                    // V9.0+ uses space-separated "pin_number net_name".
                    if( token.find( '.' ) != std::string::npos )
                    {
                        std::vector<std::string> fields;
                        std::istringstream fiss( token );
                        std::string field;

                        while( std::getline( fiss, field, '.' ) )
                            fields.push_back( field );

                        if( fields.size() >= 1 )
                            sp.pin_number = fields[0];

                        if( fields.size() >= 3 )
                            sp.net_name = fields.back();
                    }
                    else
                    {
                        sp.pin_number = token;
                        giss >> sp.net_name;
                    }

                    pt.sigpins.push_back( sp );
                    i++;
                    continue;
                }
                else
                {
                    // Swap group line or unknown, store and continue
                    pt.swap_lines.push_back( gline );
                    i++;
                    continue;
                }
            }
        }

        m_partTypes[pt.name] = pt;
    }

    return aLines.size() - 1;
}


size_t PADS_SCH_PARSER::parseSectionPART( const std::vector<std::string>& aLines,
                                           size_t aStartLine )
{
    size_t i = aStartLine + 1;

    // Skip blank lines after header
    while( i < aLines.size() && aLines[i].empty() )
        i++;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( line.empty() )
        {
            i++;
            continue;
        }

        if( isSectionMarker( line ) )
            return i - 1;

        // Part header starts with reference designator (alpha character)
        if( std::isalpha( static_cast<unsigned char>( line[0] ) ) )
        {
            PART_PLACEMENT part;
            i = parsePartPlacement( aLines, i, part );

            if( !part.reference.empty() )
            {
                part.sheet_number = m_currentSheet;
                m_partPlacements.push_back( std::move( part ) );
            }

            i++;
            continue;
        }

        i++;
    }

    return aLines.size() - 1;
}


size_t PADS_SCH_PARSER::parsePartPlacement( const std::vector<std::string>& aLines,
                                             size_t aStartLine, PART_PLACEMENT& aPart )
{
    if( aStartLine >= aLines.size() )
        return aStartLine;

    const std::string& headerLine = aLines[aStartLine];
    std::istringstream iss( headerLine );

    // Two PART header formats:
    //   Normal:  ref part_type x y angle mirror h1 w1 h2 w2 attrs disp pins u1 gate u2
    //   Power:   ref net_name $part_type x y angle mirror variant_index
    // Detect power format by checking whether the third token is numeric.
    std::string refdes, partType;
    int x = 0, y = 0, angleCode = 0, mirrorFlag = 0;

    iss >> refdes >> partType;

    if( !( iss >> x ) )
    {
        // Third field is not a number (e.g. "$PWR_SYMS"), so this is a power symbol entry.
        iss.clear();
        std::string actualPartType;
        iss >> actualPartType >> x >> y >> angleCode >> mirrorFlag;

        aPart.power_net_name = partType;
        partType = actualPartType;
    }
    else
    {
        iss >> y >> angleCode >> mirrorFlag;
    }

    aPart.reference = refdes;
    aPart.part_type = partType;
    aPart.symbol_name = partType;
    aPart.position.x = x;
    aPart.position.y = y;

    switch( angleCode )
    {
    case 0: aPart.rotation = 0.0; break;
    case 1: aPart.rotation = 90.0; break;
    case 2: aPart.rotation = 180.0; break;
    case 3: aPart.rotation = 270.0; break;
    default: aPart.rotation = angleCode; break;
    }

    aPart.mirror_flags = mirrorFlag;

    if( !aPart.power_net_name.empty() )
    {
        // Power symbol: remaining field is the variant index
        int variantIdx = 0;

        if( iss >> variantIdx )
            aPart.gate_index = variantIdx;
    }
    else
    {
        // Try to read remaining header fields for normal parts
        int numAttrs = 0, numDisplayedValues = 0, numPins = 0, unused1 = 0, gateIdx = 0;
        int unused2 = 0;

        if( iss >> aPart.h1 >> aPart.w1 >> aPart.h2 >> aPart.w2 >> numAttrs >> numDisplayedValues
                >> numPins >> unused1 >> gateIdx >> unused2 )
        {
            aPart.num_attrs = numAttrs;
            aPart.num_displayed_values = numDisplayedValues;
            aPart.num_pins = numPins;
            aPart.gate_index = gateIdx;
            aPart.gate_number = gateIdx + 1;
        }
        else
        {
            // Simplified test format: ref part_type x y angle mirror sheet gate
            std::istringstream iss2( headerLine );
            std::string dummy;
            iss2 >> dummy >> dummy >> x >> y;

            double rotDeg = 0;
            iss2 >> rotDeg;
            aPart.rotation = rotDeg;

            std::string mirrorStr;

            if( iss2 >> mirrorStr )
            {
                if( mirrorStr == "M" || mirrorStr == "Y" || mirrorStr == "1" )
                    aPart.mirror_flags = 1;
            }

            iss2 >> aPart.sheet_number >> aPart.gate_number;
        }
    }

    // Extract gate index from reference designator suffix. PADS multi-gate components
    // use "REFDES-LETTER" or "REFDES.LETTER" (e.g., U17-A, U1.B).
    size_t sepPos = refdes.rfind( '-' );

    if( sepPos == std::string::npos )
        sepPos = refdes.rfind( '.' );

    if( sepPos != std::string::npos && sepPos + 1 < refdes.size() )
    {
        char gateLetter = refdes[sepPos + 1];

        if( std::isalpha( static_cast<unsigned char>( gateLetter ) ) )
        {
            // Only derive gate index from the letter when the header didn't provide one
            if( aPart.gate_index == 0 )
            {
                aPart.gate_index = std::toupper( static_cast<unsigned char>( gateLetter ) ) - 'A';
                aPart.gate_number = aPart.gate_index + 1;
            }
        }
    }

    size_t i = aStartLine + 1;

    // If full format, parse font lines, attribute labels, overrides, and pin overrides
    if( aPart.num_attrs > 0 || aPart.num_displayed_values > 0 )
    {
        // Two font lines
        if( i < aLines.size() )
        {
            const std::string& fl = aLines[i];

            if( fl.size() >= 2 && fl[0] == '"' )
            {
                size_t qEnd = fl.find( '"', 1 );

                if( qEnd != std::string::npos )
                    aPart.font1 = fl.substr( 1, qEnd - 1 );

                i++;
            }
        }

        if( i < aLines.size() )
        {
            const std::string& fl = aLines[i];

            if( fl.size() >= 2 && fl[0] == '"' )
            {
                size_t qEnd = fl.find( '"', 1 );

                if( qEnd != std::string::npos )
                    aPart.font2 = fl.substr( 1, qEnd - 1 );

                i++;
            }
        }

        // Attribute label pairs (num_attrs pairs)
        for( int a = 0; a < aPart.num_attrs && i + 1 < aLines.size(); a++ )
        {
            PART_ATTRIBUTE attr;
            std::istringstream aiss( aLines[i] );
            int ax = 0, ay = 0, angle = 0, disp = 0, h = 0, w = 0, vis = 0;

            aiss >> ax >> ay >> angle >> disp >> h >> w >> vis;

            attr.position.x = ax;
            attr.position.y = ay;
            attr.rotation = angle;
            attr.justification = disp;
            attr.height = h;
            attr.width = w;
            attr.size = h;
            attr.visibility = vis;
            attr.visible = ( vis == 0 );

            // Parse quoted font name
            std::string rest;
            std::getline( aiss, rest );
            size_t qStart = rest.find( '"' );

            if( qStart != std::string::npos )
            {
                size_t qEnd = rest.find( '"', qStart + 1 );

                if( qEnd != std::string::npos )
                    attr.font_name = rest.substr( qStart + 1, qEnd - qStart - 1 );
            }

            i++;

            if( i < aLines.size() )
                attr.name = aLines[i];

            aPart.attributes.push_back( attr );
            i++;
        }

        // Displayed value overrides: each has a position line then a "name" value line
        for( int d = 0; d < aPart.num_displayed_values && i < aLines.size(); d++ )
        {
            // Skip the position/formatting line (starts with digit)
            if( !aLines[i].empty()
                && std::isdigit( static_cast<unsigned char>( aLines[i][0] ) ) )
            {
                i++;
            }

            if( i >= aLines.size() )
                break;

            const std::string& valLine = aLines[i];

            if( valLine.size() > 2 && valLine[0] == '"' )
            {
                size_t closeQ = valLine.find( '"', 1 );

                if( closeQ != std::string::npos )
                {
                    std::string attrName = valLine.substr( 1, closeQ - 1 );
                    std::string attrValue;

                    if( closeQ + 1 < valLine.size() )
                    {
                        attrValue = valLine.substr( closeQ + 1 );
                        size_t start = attrValue.find_first_not_of( " \t" );

                        if( start != std::string::npos )
                            attrValue = attrValue.substr( start );
                        else
                            attrValue.clear();
                    }

                    aPart.attr_overrides[attrName] = attrValue;
                }
            }

            i++;
        }

        // Apply overrides back to the attribute vector so attr.value is populated
        for( auto& attr : aPart.attributes )
        {
            auto it = aPart.attr_overrides.find( attr.name );

            if( it != aPart.attr_overrides.end() )
                attr.value = it->second;
        }

        // Pin override lines
        while( i < aLines.size() )
        {
            const std::string& pline = aLines[i];

            if( pline.empty() )
                break;

            if( isSectionMarker( pline ) )
                return i - 1;

            // New part (alpha at start)
            if( std::isalpha( static_cast<unsigned char>( pline[0] ) ) )
                return i - 1;

            // Pin override: index height width angle justification
            if( std::isdigit( static_cast<unsigned char>( pline[0] ) ) )
            {
                PART_PLACEMENT::PIN_OVERRIDE po;
                std::istringstream poiss( pline );
                int pinIdx = 0;
                poiss >> pinIdx >> po.height >> po.width >> po.angle >> po.justification;
                aPart.pin_overrides.push_back( po );
            }

            i++;
        }
    }
    else
    {
        // Simplified test format: @-prefixed attribute lines
        while( i < aLines.size() )
        {
            const std::string& attrLine = aLines[i];

            if( attrLine.empty() )
                break;

            if( isSectionMarker( attrLine ) )
                return i - 1;

            if( attrLine[0] == '@' )
            {
                PART_ATTRIBUTE attr;
                std::istringstream aiss( attrLine.substr( 1 ) );
                aiss >> attr.name;

                std::string rest;
                std::getline( aiss, rest );
                size_t start = rest.find_first_not_of( " \t" );

                if( start != std::string::npos )
                {
                    rest = rest.substr( start );

                    if( !rest.empty() && rest[0] == '"' )
                    {
                        size_t endQuote = rest.find( '"', 1 );

                        if( endQuote != std::string::npos )
                        {
                            attr.value = rest.substr( 1, endQuote - 1 );
                            rest = rest.substr( endQuote + 1 );
                        }
                    }
                    else
                    {
                        std::istringstream viss( rest );
                        viss >> attr.value;
                        std::getline( viss, rest );
                    }

                    std::istringstream piss( rest );
                    piss >> attr.position.x >> attr.position.y >> attr.rotation >> attr.size;

                    std::string visStr;

                    if( piss >> visStr )
                        attr.visible = ( visStr != "N" && visStr != "0" && visStr != "H" );
                }

                aPart.attributes.push_back( attr );
                i++;
            }
            else if( std::isalpha( static_cast<unsigned char>( attrLine[0] ) ) )
            {
                return i - 1;
            }
            else
            {
                i++;
            }
        }
    }

    return i - 1;
}


size_t PADS_SCH_PARSER::parseSectionOFFPAGEREFS( const std::vector<std::string>& aLines,
                                                   size_t aStartLine )
{
    size_t i = aStartLine + 1;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( line.empty() )
        {
            i++;
            continue;
        }

        if( isSectionMarker( line ) )
            return i - 1;

        // Format: @@@O<id>    net_name symbol_lib x y rotation flags1 flags2
        if( line.find( "@@@O" ) == 0 )
        {
            OFF_PAGE_CONNECTOR opc;
            std::istringstream iss( line );
            std::string idToken;
            iss >> idToken;

            // Extract numeric ID from @@@O<id>
            if( idToken.size() > 4 )
                opc.id = PADS_COMMON::ParseInt( idToken.substr( 4 ), 0, "OPC id" );

            int x = 0, y = 0;
            iss >> opc.signal_name >> opc.symbol_lib >> x >> y >> opc.rotation
                >> opc.flags1 >> opc.flags2;

            opc.position.x = x;
            opc.position.y = y;
            opc.source_sheet = m_currentSheet;

            m_offPageConnectors.push_back( opc );
        }

        i++;
    }

    return aLines.size() - 1;
}


size_t PADS_SCH_PARSER::parseSectionTIEDOTS( const std::vector<std::string>& aLines,
                                              size_t aStartLine )
{
    size_t i = aStartLine + 1;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( line.empty() )
        {
            i++;
            continue;
        }

        if( isSectionMarker( line ) )
            return i - 1;

        // Format: @@@D<id>    x y
        if( line.find( "@@@D" ) == 0 )
        {
            TIED_DOT dot;
            std::istringstream iss( line );
            std::string idToken;
            iss >> idToken;

            if( idToken.size() > 4 )
                dot.id = PADS_COMMON::ParseInt( idToken.substr( 4 ), 0, "TIEDOT id" );

            int x = 0, y = 0;
            iss >> x >> y;
            dot.position.x = x;
            dot.position.y = y;
            dot.sheet_number = m_currentSheet;

            m_tiedDots.push_back( dot );
        }

        i++;
    }

    return aLines.size() - 1;
}


size_t PADS_SCH_PARSER::parseSectionCONNECTION( const std::vector<std::string>& aLines,
                                                  size_t aStartLine )
{
    // *CONNECTION* is the section marker. *SIGNAL* blocks follow within.
    size_t i = aStartLine + 1;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( line.empty() )
        {
            i++;
            continue;
        }

        // A non-SIGNAL section marker ends the CONNECTION section
        if( isSectionMarker( line ) )
        {
            std::string secName = extractSectionName( line );

            if( secName == "SIGNAL" )
            {
                SCH_SIGNAL signal;
                i = parseSignalDef( aLines, i, signal );

                if( !signal.name.empty() )
                {
                    // Set sheet number on all wire segments
                    for( auto& wire : signal.wires )
                        wire.sheet_number = m_currentSheet;

                    // Build connections from endpoint references
                    for( const auto& wire : signal.wires )
                    {
                        for( const auto& ep : { wire.endpoint_a, wire.endpoint_b } )
                        {
                            if( ep.find( '.' ) != std::string::npos &&
                                ep.find( "@@@" ) == std::string::npos )
                            {
                                size_t dotPos = ep.find( '.' );
                                PIN_CONNECTION conn;
                                conn.reference = ep.substr( 0, dotPos );
                                conn.pin_number = ep.substr( dotPos + 1 );
                                conn.sheet_number = m_currentSheet;

                                // Avoid duplicates
                                bool found = false;

                                for( const auto& existing : signal.connections )
                                {
                                    if( existing.reference == conn.reference &&
                                        existing.pin_number == conn.pin_number )
                                    {
                                        found = true;
                                        break;
                                    }
                                }

                                if( !found )
                                    signal.connections.push_back( conn );
                            }
                        }
                    }

                    m_signals.push_back( std::move( signal ) );
                }

                i++;
                continue;
            }
            else
            {
                return i - 1;
            }
        }

        i++;
    }

    return aLines.size() - 1;
}


size_t PADS_SCH_PARSER::parseSignalDef( const std::vector<std::string>& aLines, size_t aStartLine,
                                          SCH_SIGNAL& aSignal )
{
    if( aStartLine >= aLines.size() )
        return aStartLine;

    // Header: *SIGNAL* net_name flags1 flags2
    const std::string& headerLine = aLines[aStartLine];
    std::string secName = extractSectionName( headerLine );

    if( secName != "SIGNAL" )
        return aStartLine;

    // Extract everything after *SIGNAL*
    size_t afterMarker = headerLine.find( '*', 1 );

    if( afterMarker == std::string::npos )
        return aStartLine;

    std::string rest = headerLine.substr( afterMarker + 1 );
    std::istringstream iss( rest );

    iss >> aSignal.name >> aSignal.flags1 >> aSignal.flags2;

    size_t i = aStartLine + 1;

    // Optional FUNCTION line
    if( aSignal.flags2 == 1 && i < aLines.size() )
    {
        const std::string& funcLine = aLines[i];

        if( funcLine.find( "\"FUNCTION\"" ) != std::string::npos ||
            funcLine.find( "FUNCTION" ) == 0 )
        {
            size_t qStart = funcLine.find( '"' );

            if( qStart != std::string::npos )
            {
                size_t qEnd = funcLine.find( '"', qStart + 1 );

                if( qEnd != std::string::npos )
                {
                    size_t afterQ = funcLine.find_first_not_of( " \t", qEnd + 1 );

                    if( afterQ != std::string::npos )
                        aSignal.function = funcLine.substr( afterQ );
                }
            }

            i++;
        }
    }

    // Wire segments: endpoint_a endpoint_b vertex_count flags
    //                x1 y1
    //                x2 y2 ...
    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( line.empty() )
        {
            i++;
            continue;
        }

        if( isSectionMarker( line ) )
            return i - 1;

        // Wire segment header line: endpoint_a endpoint_b vertex_count flags
        WIRE_SEGMENT wire;
        std::istringstream wiss( line );
        wiss >> wire.endpoint_a >> wire.endpoint_b >> wire.vertex_count >> wire.flags;

        if( wire.endpoint_a.empty() || wire.endpoint_b.empty() )
        {
            i++;
            continue;
        }

        i++;

        // Read vertex coordinates
        for( int v = 0; v < wire.vertex_count && i < aLines.size(); v++ )
        {
            const std::string& ptLine = aLines[i];

            if( ptLine.empty() || isSectionMarker( ptLine ) )
                break;

            POINT pt;
            std::istringstream piss( ptLine );
            piss >> pt.x >> pt.y;
            wire.vertices.push_back( pt );
            i++;
        }

        // Set start/end from first/last vertex for backward compat
        if( !wire.vertices.empty() )
        {
            wire.start = wire.vertices.front();
            wire.end = wire.vertices.back();
        }

        aSignal.wires.push_back( wire );
    }

    return i > 0 ? i - 1 : aStartLine;
}


size_t PADS_SCH_PARSER::parseSectionNETNAMES( const std::vector<std::string>& aLines,
                                               size_t aStartLine )
{
    size_t i = aStartLine + 1;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        if( line.empty() )
        {
            i++;
            continue;
        }

        if( isSectionMarker( line ) )
            return i - 1;

        // Format: net_name anchor_ref x_offset y_offset rotation justification f3 f4 f5 f6 f7
        //         height width_pct "font_name"
        NETNAME_LABEL label;
        std::istringstream iss( line );

        iss >> label.net_name >> label.anchor_ref >> label.x_offset >> label.y_offset
            >> label.rotation >> label.justification >> label.f3 >> label.f4 >> label.f5
            >> label.f6 >> label.f7 >> label.height >> label.width_pct;

        // Parse quoted font name
        std::string rest;
        std::getline( iss, rest );
        size_t qStart = rest.find( '"' );

        if( qStart != std::string::npos )
        {
            size_t qEnd = rest.find( '"', qStart + 1 );

            if( qEnd != std::string::npos )
                label.font_name = rest.substr( qStart + 1, qEnd - qStart - 1 );
        }

        m_netNameLabels.push_back( label );
        i++;
    }

    return aLines.size() - 1;
}


size_t PADS_SCH_PARSER::skipBraceDelimitedSection( const std::vector<std::string>& aLines,
                                                     size_t aStartLine )
{
    size_t i = aStartLine + 1;
    int braceDepth = 0;
    bool foundFirstBrace = false;

    while( i < aLines.size() )
    {
        const std::string& line = aLines[i];

        for( char c : line )
        {
            if( c == '{' )
            {
                braceDepth++;
                foundFirstBrace = true;
            }
            else if( c == '}' )
            {
                braceDepth--;
            }
        }

        if( foundFirstBrace && braceDepth <= 0 )
            return i;

        // If we hit another section marker without finding any braces, this section was empty
        if( !foundFirstBrace && isSectionMarker( line ) )
            return i - 1;

        i++;
    }

    return aLines.size() - 1;
}


const SYMBOL_DEF* PADS_SCH_PARSER::GetSymbolDef( const std::string& aName ) const
{
    for( const auto& sym : m_symbolDefs )
    {
        if( sym.name == aName )
            return &sym;
    }

    return nullptr;
}


const PART_PLACEMENT* PADS_SCH_PARSER::GetPartPlacement( const std::string& aReference ) const
{
    for( const auto& part : m_partPlacements )
    {
        if( part.reference == aReference )
            return &part;
    }

    return nullptr;
}


const SCH_SIGNAL* PADS_SCH_PARSER::GetSignal( const std::string& aName ) const
{
    for( const auto& signal : m_signals )
    {
        if( signal.name == aName )
            return &signal;
    }

    return nullptr;
}


int PADS_SCH_PARSER::GetSheetCount() const
{
    std::set<int> sheets = GetSheetNumbers();

    if( sheets.empty() )
        return 1;

    return *sheets.rbegin();
}


std::set<int> PADS_SCH_PARSER::GetSheetNumbers() const
{
    std::set<int> sheets;

    for( const auto& header : m_sheetHeaders )
        sheets.insert( header.sheet_num );

    for( const auto& part : m_partPlacements )
        sheets.insert( part.sheet_number );

    for( const auto& signal : m_signals )
    {
        for( const auto& wire : signal.wires )
            sheets.insert( wire.sheet_number );

        for( const auto& conn : signal.connections )
            sheets.insert( conn.sheet_number );
    }

    if( sheets.empty() )
        sheets.insert( 1 );

    return sheets;
}


std::vector<SCH_SIGNAL> PADS_SCH_PARSER::GetSignalsOnSheet( int aSheetNumber ) const
{
    std::vector<SCH_SIGNAL> result;

    for( const auto& signal : m_signals )
    {
        SCH_SIGNAL filteredSignal;
        filteredSignal.name = signal.name;

        for( const auto& wire : signal.wires )
        {
            if( wire.sheet_number == aSheetNumber )
                filteredSignal.wires.push_back( wire );
        }

        for( const auto& conn : signal.connections )
        {
            if( conn.sheet_number == aSheetNumber )
                filteredSignal.connections.push_back( conn );
        }

        if( !filteredSignal.wires.empty() || !filteredSignal.connections.empty() )
            result.push_back( filteredSignal );
    }

    return result;
}


std::vector<PART_PLACEMENT> PADS_SCH_PARSER::GetPartsOnSheet( int aSheetNumber ) const
{
    std::vector<PART_PLACEMENT> result;

    for( const auto& part : m_partPlacements )
    {
        if( part.sheet_number == aSheetNumber )
            result.push_back( part );
    }

    return result;
}


PIN_TYPE PADS_SCH_PARSER::parsePinType( const std::string& aTypeStr )
{
    std::string upper = aTypeStr;
    std::transform( upper.begin(), upper.end(), upper.begin(), ::toupper );

    if( upper == "I" || upper == "IN" || upper == "INPUT" || upper == "L" )
        return PIN_TYPE::INPUT;

    if( upper == "O" || upper == "OUT" || upper == "OUTPUT" || upper == "S" )
        return PIN_TYPE::OUTPUT;

    if( upper == "B" || upper == "BI" || upper == "BIDIR" || upper == "BIDIRECTIONAL" )
        return PIN_TYPE::BIDIRECTIONAL;

    if( upper == "T" || upper == "TRI" || upper == "TRISTATE" )
        return PIN_TYPE::TRISTATE;

    if( upper == "OC" || upper == "OPENCOLLECTOR" )
        return PIN_TYPE::OPEN_COLLECTOR;

    if( upper == "OE" || upper == "OPENEMITTER" )
        return PIN_TYPE::OPEN_EMITTER;

    if( upper == "P" || upper == "PWR" || upper == "POWER" || upper == "G" )
        return PIN_TYPE::POWER;

    if( upper == "PAS" || upper == "PASSIVE" )
        return PIN_TYPE::PASSIVE;

    return PIN_TYPE::UNSPECIFIED;
}


PIN_TYPE PADS_SCH_PARSER::ParsePinTypeChar( char aTypeChar )
{
    switch( std::toupper( aTypeChar ) )
    {
    case 'L': return PIN_TYPE::INPUT;
    case 'S': return PIN_TYPE::OUTPUT;
    case 'B': return PIN_TYPE::BIDIRECTIONAL;
    case 'P': return PIN_TYPE::POWER;
    case 'G': return PIN_TYPE::POWER;
    case 'U': return PIN_TYPE::UNSPECIFIED;
    default:  return PIN_TYPE::UNSPECIFIED;
    }
}

} // namespace PADS_SCH
