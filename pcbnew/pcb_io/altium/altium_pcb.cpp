/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "altium_pcb.h"
#include "altium_parser_pcb.h"
#include <altium_pcb_compound_file.h>
#include <io/altium/altium_binary_parser.h>
#include <io/altium/altium_parser_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <layer_range.h>
#include <pcb_dimension.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <pcb_barcode.h>
#include <core/profile.h>
#include <string_utils.h>
#include <tools/pad_tool.h>
#include <zone.h>

#include <board_stackup_manager/stackup_predefined_prms.h>

#include <advanced_config.h>
#include <compoundfilereader.h>
#include <convert_basic_shapes_to_polygon.h>
#include <font/outline_font.h>
#include <project.h>
#include <reporter.h>
#include <trigo.h>
#include <utf.h>
#include <wx/docview.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include <progress_reporter.h>
#include <magic_enum.hpp>


constexpr double BOLD_FACTOR = 1.75;    // CSS font-weight-normal is 400; bold is 700


bool IsAltiumLayerCopper( ALTIUM_LAYER aLayer )
{
    return ( aLayer >= ALTIUM_LAYER::TOP_LAYER && aLayer <= ALTIUM_LAYER::BOTTOM_LAYER )
           || aLayer == ALTIUM_LAYER::MULTI_LAYER; // TODO: add IsAltiumLayerAPlane?
}


bool IsAltiumLayerAPlane( ALTIUM_LAYER aLayer )
{
    return aLayer >= ALTIUM_LAYER::INTERNAL_PLANE_1 && aLayer <= ALTIUM_LAYER::INTERNAL_PLANE_16;
}

FOOTPRINT* ALTIUM_PCB::HelperGetFootprint( uint16_t aComponent ) const
{
    if( aComponent == ALTIUM_COMPONENT_NONE || m_components.size() <= aComponent )
    {
        THROW_IO_ERROR( wxString::Format( wxT( "Component creator tries to access component id %u "
                                               "of %u existing components" ),
                                          (unsigned)aComponent, (unsigned)m_components.size() ) );
    }

    return m_components.at( aComponent );
}


void HelperShapeLineChainFromAltiumVertices( SHAPE_LINE_CHAIN& aLine,
                                             const std::vector<ALTIUM_VERTICE>& aVertices )
{
    for( const ALTIUM_VERTICE& vertex : aVertices )
    {
        if( vertex.isRound )
        {
            EDA_ANGLE angle( vertex.endangle - vertex.startangle, DEGREES_T );
            angle.Normalize();

            double  startradiant   = DEG2RAD( vertex.startangle );
            double  endradiant     = DEG2RAD( vertex.endangle );
            VECTOR2I arcStartOffset = KiROUND( std::cos( startradiant ) * vertex.radius,
                                               -std::sin( startradiant ) * vertex.radius );

            VECTOR2I arcEndOffset = KiROUND( std::cos( endradiant ) * vertex.radius,
                                             -std::sin( endradiant ) * vertex.radius );

            VECTOR2I arcStart = vertex.center + arcStartOffset;
            VECTOR2I arcEnd   = vertex.center + arcEndOffset;

            bool isShort = arcStart.Distance( arcEnd ) < pcbIUScale.mmToIU( 0.001 )
                           || angle.AsDegrees() < 0.2;

            if( arcStart.Distance( vertex.position )
                    < arcEnd.Distance( vertex.position ) )
            {
                if( !isShort )
                {
                    aLine.Append( SHAPE_ARC( vertex.center, arcStart, -angle ) );
                }
                else
                {
                    aLine.Append( arcStart );
                    aLine.Append( arcEnd );
                }
            }
            else
            {
                if( !isShort )
                {
                    aLine.Append( SHAPE_ARC( vertex.center, arcEnd, angle ) );
                }
                else
                {
                    aLine.Append( arcEnd );
                    aLine.Append( arcStart );
                }
            }
        }
        else
        {
            aLine.Append( vertex.position );
        }
    }

    aLine.SetClosed( true );
}


PCB_LAYER_ID ALTIUM_PCB::GetKicadLayer( ALTIUM_LAYER aAltiumLayer ) const
{
    auto override = m_layermap.find( aAltiumLayer );

    if( override != m_layermap.end() )
    {
        return override->second;
    }

    switch( aAltiumLayer )
    {
    case ALTIUM_LAYER::UNKNOWN:           return UNDEFINED_LAYER;

    case ALTIUM_LAYER::TOP_LAYER:         return F_Cu;
    case ALTIUM_LAYER::MID_LAYER_1:       return In1_Cu;
    case ALTIUM_LAYER::MID_LAYER_2:       return In2_Cu;
    case ALTIUM_LAYER::MID_LAYER_3:       return In3_Cu;
    case ALTIUM_LAYER::MID_LAYER_4:       return In4_Cu;
    case ALTIUM_LAYER::MID_LAYER_5:       return In5_Cu;
    case ALTIUM_LAYER::MID_LAYER_6:       return In6_Cu;
    case ALTIUM_LAYER::MID_LAYER_7:       return In7_Cu;
    case ALTIUM_LAYER::MID_LAYER_8:       return In8_Cu;
    case ALTIUM_LAYER::MID_LAYER_9:       return In9_Cu;
    case ALTIUM_LAYER::MID_LAYER_10:      return In10_Cu;
    case ALTIUM_LAYER::MID_LAYER_11:      return In11_Cu;
    case ALTIUM_LAYER::MID_LAYER_12:      return In12_Cu;
    case ALTIUM_LAYER::MID_LAYER_13:      return In13_Cu;
    case ALTIUM_LAYER::MID_LAYER_14:      return In14_Cu;
    case ALTIUM_LAYER::MID_LAYER_15:      return In15_Cu;
    case ALTIUM_LAYER::MID_LAYER_16:      return In16_Cu;
    case ALTIUM_LAYER::MID_LAYER_17:      return In17_Cu;
    case ALTIUM_LAYER::MID_LAYER_18:      return In18_Cu;
    case ALTIUM_LAYER::MID_LAYER_19:      return In19_Cu;
    case ALTIUM_LAYER::MID_LAYER_20:      return In20_Cu;
    case ALTIUM_LAYER::MID_LAYER_21:      return In21_Cu;
    case ALTIUM_LAYER::MID_LAYER_22:      return In22_Cu;
    case ALTIUM_LAYER::MID_LAYER_23:      return In23_Cu;
    case ALTIUM_LAYER::MID_LAYER_24:      return In24_Cu;
    case ALTIUM_LAYER::MID_LAYER_25:      return In25_Cu;
    case ALTIUM_LAYER::MID_LAYER_26:      return In26_Cu;
    case ALTIUM_LAYER::MID_LAYER_27:      return In27_Cu;
    case ALTIUM_LAYER::MID_LAYER_28:      return In28_Cu;
    case ALTIUM_LAYER::MID_LAYER_29:      return In29_Cu;
    case ALTIUM_LAYER::MID_LAYER_30:      return In30_Cu;
    case ALTIUM_LAYER::BOTTOM_LAYER:      return B_Cu;

    case ALTIUM_LAYER::TOP_OVERLAY:       return F_SilkS;
    case ALTIUM_LAYER::BOTTOM_OVERLAY:    return B_SilkS;
    case ALTIUM_LAYER::TOP_PASTE:         return F_Paste;
    case ALTIUM_LAYER::BOTTOM_PASTE:      return B_Paste;
    case ALTIUM_LAYER::TOP_SOLDER:        return F_Mask;
    case ALTIUM_LAYER::BOTTOM_SOLDER:     return B_Mask;

    case ALTIUM_LAYER::INTERNAL_PLANE_1:  return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_2:  return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_3:  return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_4:  return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_5:  return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_6:  return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_7:  return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_8:  return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_9:  return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_10: return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_11: return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_12: return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_13: return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_14: return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_15: return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_16: return UNDEFINED_LAYER;

    case ALTIUM_LAYER::DRILL_GUIDE:       return Dwgs_User;
    case ALTIUM_LAYER::KEEP_OUT_LAYER:    return Margin;

    case ALTIUM_LAYER::MECHANICAL_1:      return User_1; //Edge_Cuts;
    case ALTIUM_LAYER::MECHANICAL_2:      return User_2;
    case ALTIUM_LAYER::MECHANICAL_3:      return User_3;
    case ALTIUM_LAYER::MECHANICAL_4:      return User_4;
    case ALTIUM_LAYER::MECHANICAL_5:      return User_5;
    case ALTIUM_LAYER::MECHANICAL_6:      return User_6;
    case ALTIUM_LAYER::MECHANICAL_7:      return User_7;
    case ALTIUM_LAYER::MECHANICAL_8:      return User_8;
    case ALTIUM_LAYER::MECHANICAL_9:      return User_9;
    case ALTIUM_LAYER::MECHANICAL_10:     return User_10;
    case ALTIUM_LAYER::MECHANICAL_11:     return User_11; //Eco1 is used for unknown elements
    case ALTIUM_LAYER::MECHANICAL_12:     return F_Fab;
    case ALTIUM_LAYER::MECHANICAL_13:     return B_Fab; // Don't use courtyard layers for other purposes
    case ALTIUM_LAYER::MECHANICAL_14:     return User_12;
    case ALTIUM_LAYER::MECHANICAL_15:     return User_13;
    case ALTIUM_LAYER::MECHANICAL_16:     return User_14;

    case ALTIUM_LAYER::DRILL_DRAWING:     return Dwgs_User;
    case ALTIUM_LAYER::MULTI_LAYER:       return UNDEFINED_LAYER;
    case ALTIUM_LAYER::CONNECTIONS:       return UNDEFINED_LAYER;
    case ALTIUM_LAYER::BACKGROUND:        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::DRC_ERROR_MARKERS: return UNDEFINED_LAYER;
    case ALTIUM_LAYER::SELECTIONS:        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::VISIBLE_GRID_1:    return UNDEFINED_LAYER;
    case ALTIUM_LAYER::VISIBLE_GRID_2:    return UNDEFINED_LAYER;
    case ALTIUM_LAYER::PAD_HOLES:         return UNDEFINED_LAYER;
    case ALTIUM_LAYER::VIA_HOLES:         return UNDEFINED_LAYER;

    default:                              return UNDEFINED_LAYER;
    }
}


std::vector<PCB_LAYER_ID> ALTIUM_PCB::GetKicadLayersToIterate( ALTIUM_LAYER aAltiumLayer ) const
{
    static std::set<ALTIUM_LAYER> altiumLayersWithWarning;

    if( aAltiumLayer == ALTIUM_LAYER::MULTI_LAYER || aAltiumLayer == ALTIUM_LAYER::KEEP_OUT_LAYER )
    {
        int layerCount = m_board ? m_board->GetCopperLayerCount() : 32;
        std::vector<PCB_LAYER_ID> layers;
        layers.reserve( layerCount );

        for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, layerCount ) )
        {
            if( !m_board || m_board->IsLayerEnabled( layer ) )
                layers.emplace_back( layer );
        }

        return layers;
    }

    PCB_LAYER_ID klayer = GetKicadLayer( aAltiumLayer );

    if( klayer == UNDEFINED_LAYER )
    {
        auto it = m_layerNames.find( aAltiumLayer );
        wxString layerName = it != m_layerNames.end() ? it->second : wxString::Format( wxT( "(%d)" ),
                                                                                      (int) aAltiumLayer );

        if( m_reporter && altiumLayersWithWarning.insert( aAltiumLayer ).second )
        {
            m_reporter->Report( wxString::Format(
                    _( "Altium layer %s has no KiCad equivalent. It has been moved to KiCad "
                       "layer Eco1_User." ), layerName ), RPT_SEVERITY_INFO );
        }

        klayer = Eco1_User;

        if( m_board )
            m_board->SetEnabledLayers( m_board->GetEnabledLayers() | LSET( { klayer } ) );
    }

    return { klayer };
}


ALTIUM_PCB::ALTIUM_PCB( BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter,
                        LAYER_MAPPING_HANDLER& aHandler, REPORTER* aReporter,
                        const wxString& aLibrary, const wxString& aFootprintName )
{
    m_board = aBoard;
    m_progressReporter = aProgressReporter;
    m_layerMappingHandler = aHandler;
    m_reporter = aReporter;
    m_doneCount = 0;
    m_lastProgressCount = 0;
    m_totalCount = 0;
    m_highest_pour_index = 0;
    m_library = aLibrary;
    m_footprintName = aFootprintName;
}

ALTIUM_PCB::~ALTIUM_PCB()
{
}

void ALTIUM_PCB::checkpoint()
{
    const unsigned PROGRESS_DELTA = 250;

    if( m_progressReporter )
    {
        if( ++m_doneCount > m_lastProgressCount + PROGRESS_DELTA )
        {
            m_progressReporter->SetCurrentProgress( ( (double) m_doneCount )
                                                    / std::max( 1U, m_totalCount ) );

            if( !m_progressReporter->KeepRefreshing() )
                THROW_IO_ERROR( _( "File import canceled by user." ) );

            m_lastProgressCount = m_doneCount;
        }
    }
}

void ALTIUM_PCB::Parse( const ALTIUM_PCB_COMPOUND_FILE&                  altiumPcbFile,
                        const std::map<ALTIUM_PCB_DIR, std::string>& aFileMapping )
{
    // this vector simply declares in which order which functions to call.
    const std::vector<std::tuple<bool, ALTIUM_PCB_DIR, PARSE_FUNCTION_POINTER_fp>> parserOrder = {
        { true, ALTIUM_PCB_DIR::FILE_HEADER,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseFileHeader( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::BOARD6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseBoard6Data( aFile, fileHeader );
          } },
        { false, ALTIUM_PCB_DIR::EXTENDPRIMITIVEINFORMATION,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseExtendedPrimitiveInformationData( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::COMPONENTS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseComponents6Data( aFile, fileHeader );
          } },
        { false, ALTIUM_PCB_DIR::MODELS,
          [this, aFileMapping]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              std::vector<std::string> dir{ aFileMapping.at( ALTIUM_PCB_DIR::MODELS ) };
              this->ParseModelsData( aFile, fileHeader, dir );
          } },
        { true, ALTIUM_PCB_DIR::COMPONENTBODIES6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseComponentsBodies6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::NETS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseNets6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::CLASSES6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseClasses6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::RULES6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseRules6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::DIMENSIONS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseDimensions6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::POLYGONS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParsePolygons6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::ARCS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseArcs6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::PADS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParsePads6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::VIAS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseVias6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::TRACKS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseTracks6Data( aFile, fileHeader );
          } },
        { false, ALTIUM_PCB_DIR::WIDESTRINGS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseWideStrings6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::TEXTS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseTexts6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::FILLS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseFills6Data( aFile, fileHeader );
          } },
        { false, ALTIUM_PCB_DIR::BOARDREGIONS,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseBoardRegionsData( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::SHAPEBASEDREGIONS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseShapeBasedRegions6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::REGIONS6,
          [this]( const ALTIUM_PCB_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseRegions6Data( aFile, fileHeader );
          } }
    };

    if( m_progressReporter != nullptr )
    {
        // Count number of records we will read for the progress reporter
        for( const std::tuple<bool, ALTIUM_PCB_DIR, PARSE_FUNCTION_POINTER_fp>& cur : parserOrder )
        {
            bool                      isRequired;
            ALTIUM_PCB_DIR            directory;
            PARSE_FUNCTION_POINTER_fp fp;
            std::tie( isRequired, directory, fp ) = cur;

            if( directory == ALTIUM_PCB_DIR::FILE_HEADER )
                continue;

            const auto& mappedDirectory = aFileMapping.find( directory );

            if( mappedDirectory == aFileMapping.end() )
                continue;

            const std::vector<std::string>  mappedFile{ mappedDirectory->second, "Header" };
            const CFB::COMPOUND_FILE_ENTRY* file = altiumPcbFile.FindStream( mappedFile );

            if( file == nullptr )
                continue;

            ALTIUM_BINARY_PARSER reader( altiumPcbFile, file );
            uint32_t             numOfRecords = reader.Read<uint32_t>();

            if( reader.HasParsingError() )
            {
                if( m_reporter )
                {
                    m_reporter->Report( wxString::Format( _( "'%s' was not parsed correctly." ),
                                                          FormatPath( mappedFile ) ),
                                        RPT_SEVERITY_ERROR );
                }

                continue;
            }

            m_totalCount += numOfRecords;

            if( reader.GetRemainingBytes() != 0 )
            {
                if( m_reporter )
                {
                    m_reporter->Report( wxString::Format( _( "'%s' was not fully parsed." ),
                                                          FormatPath( mappedFile ) ),
                                        RPT_SEVERITY_ERROR );
                }

                continue;
            }
        }
    }

    const auto& boardDirectory = aFileMapping.find( ALTIUM_PCB_DIR::BOARD6 );

    if( boardDirectory != aFileMapping.end() )
    {
        std::vector<std::string> mappedFile{ boardDirectory->second, "Data" };

        const CFB::COMPOUND_FILE_ENTRY* file = altiumPcbFile.FindStream( mappedFile );

        if( !file )
        {
            THROW_IO_ERROR( _(
                    "This file does not appear to be in a valid PCB Binary Version 6.0 format. In "
                    "Altium Designer, "
                    "make sure to save as \"PCB Binary Files (*.PcbDoc)\"." ) );
        }
    }

    // Parse data in specified order
    for( const std::tuple<bool, ALTIUM_PCB_DIR, PARSE_FUNCTION_POINTER_fp>& cur : parserOrder )
    {
        bool                      isRequired;
        ALTIUM_PCB_DIR            directory;
        PARSE_FUNCTION_POINTER_fp fp;
        std::tie( isRequired, directory, fp ) = cur;

        const auto& mappedDirectory = aFileMapping.find( directory );

        if( mappedDirectory == aFileMapping.end() )
        {
            wxASSERT_MSG( !isRequired, wxString::Format( wxT( "Altium Directory of kind %d was "
                                                              "expected, but no mapping is "
                                                              "present in the code" ),
                                                         directory ) );
            continue;
        }

        std::vector<std::string> mappedFile{ mappedDirectory->second };

        if( directory != ALTIUM_PCB_DIR::FILE_HEADER )
            mappedFile.emplace_back( "Data" );

        const CFB::COMPOUND_FILE_ENTRY* file = altiumPcbFile.FindStream( mappedFile );

        if( file != nullptr )
        {
            fp( altiumPcbFile, file );
        }
        else if( isRequired )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "File not found: '%s' for directory '%s'." ),
                                                      FormatPath( mappedFile ),
                                                      magic_enum::enum_name( directory ) ),
                                    RPT_SEVERITY_ERROR );
            }
        }
    }

    // fixup zone priorities since Altium stores them in the opposite order
    for( ZONE* zone : m_polygons )
    {
        if( !zone )
            continue;

        // Altium "fills" - not poured in Altium
        if( zone->GetAssignedPriority() == 1000 )
        {
            // Unlikely, but you never know
            if( m_highest_pour_index >= 1000 )
                zone->SetAssignedPriority( m_highest_pour_index + 1 );

            continue;
        }

        int priority = m_highest_pour_index - zone->GetAssignedPriority();

        zone->SetAssignedPriority( priority >= 0 ? priority : 0 );
    }

    // change priority of outer zone to zero
    for( std::pair<const ALTIUM_LAYER, ZONE*>& zone : m_outer_plane )
        zone.second->SetAssignedPriority( 0 );

    // Simplify and fracture zone fills in case we constructed them from tracks (hatched fill)
    for( ZONE* zone : m_polygons )
    {
        if( !zone )
            continue;

        for( PCB_LAYER_ID layer : zone->GetLayerSet() )
        {
            if( !zone->HasFilledPolysForLayer( layer ) )
                continue;

            zone->GetFilledPolysList( layer )->Fracture();
        }
    }

    // Altium doesn't appear to store either the dimension value nor the dimensioned object in
    // the dimension record.  (Yes, there is a REFERENCE0OBJECTID, but it doesn't point to the
    // dimensioned object.)  We attempt to plug this gap by finding a colocated arc or circle
    // and using its radius.  If there are more than one such arcs/circles, well, :shrug:.
    for( PCB_DIM_RADIAL* dim : m_radialDimensions )
    {
        int radius = 0;

        for( BOARD_ITEM* item : m_board->Drawings() )
        {
            if( item->Type() != PCB_SHAPE_T )
                continue;

            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

            if( shape->GetShape() != SHAPE_T::ARC && shape->GetShape() != SHAPE_T::CIRCLE )
                continue;

            if( shape->GetPosition() == dim->GetPosition() )
            {
                radius = shape->GetRadius();
                break;
            }
        }

        if( radius == 0 )
        {
            for( PCB_TRACK* track : m_board->Tracks() )
            {
                if( track->Type() != PCB_ARC_T )
                    continue;

                PCB_ARC* arc = static_cast<PCB_ARC*>( track );

                if( arc->GetCenter() == dim->GetPosition() )
                {
                    radius = arc->GetRadius();
                    break;
                }
            }
        }

        // Move the radius point onto the circumference
        VECTOR2I radialLine = dim->GetEnd() - dim->GetStart();
        int      totalLength = radialLine.EuclideanNorm();

        // Enforce a minimum on the radialLine else we won't have enough precision to get the
        // angle from it.
        radialLine = radialLine.Resize( std::max( radius, 2 ) );
        dim->SetEnd( dim->GetStart() + (VECTOR2I) radialLine );
        dim->SetLeaderLength( totalLength - radius );
        dim->Update();
    }

    // center board
    BOX2I bbbox = m_board->GetBoardEdgesBoundingBox();

    int w = m_board->GetPageSettings().GetWidthIU( pcbIUScale.IU_PER_MILS );
    int h = m_board->GetPageSettings().GetHeightIU( pcbIUScale.IU_PER_MILS );

    int desired_x = ( w - bbbox.GetWidth() ) / 2;
    int desired_y = ( h - bbbox.GetHeight() ) / 2;

    VECTOR2I movementVector( desired_x - bbbox.GetX(), desired_y - bbbox.GetY() );
    m_board->Move( movementVector );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.SetAuxOrigin( bds.GetAuxOrigin() + movementVector );
    bds.SetGridOrigin( bds.GetGridOrigin() + movementVector );

    m_board->SetModified();
}


FOOTPRINT* ALTIUM_PCB::ParseFootprint( ALTIUM_PCB_COMPOUND_FILE& altiumLibFile,
                                       const wxString&       aFootprintName )
{
    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( m_board );

    // TODO: what should we do with those layers?
    m_layermap.emplace( ALTIUM_LAYER::MECHANICAL_14, Eco2_User );
    m_layermap.emplace( ALTIUM_LAYER::MECHANICAL_15, Eco2_User );
    m_layermap.emplace( ALTIUM_LAYER::MECHANICAL_16, Eco2_User );

    m_unicodeStrings.clear();
    m_extendedPrimitiveInformationMaps.clear();

    // TODO: WideStrings are stored as parameterMap in the case of footprints, not as binary
    //    std::string                     unicodeStringsStreamName = aFootprintName.ToStdString() + "\\WideStrings";
    //    const CFB::COMPOUND_FILE_ENTRY* unicodeStringsData = altiumLibFile.FindStream( unicodeStringsStreamName );
    //    if( unicodeStringsData != nullptr )
    //    {
    //        ParseWideStrings6Data( altiumLibFile, unicodeStringsData );
    //    }

    std::tuple<wxString, const CFB::COMPOUND_FILE_ENTRY*> ret =
            altiumLibFile.FindLibFootprintDirName( aFootprintName );

    wxString fpDirName = std::get<0>( ret );
    const CFB::COMPOUND_FILE_ENTRY* footprintStream = std::get<1>( ret );

    if( fpDirName.IsEmpty() )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Footprint directory not found: '%s'." ), aFootprintName ) );
    }

    const std::vector<std::string>  streamName{ fpDirName.ToStdString(), "Data" };
    const CFB::COMPOUND_FILE_ENTRY* footprintData = altiumLibFile.FindStream( footprintStream, { "Data" } );

    if( footprintData == nullptr )
    {
        THROW_IO_ERROR( wxString::Format( _( "File not found: '%s'." ),
                                          FormatPath( streamName ) ) );
    }

    ALTIUM_BINARY_PARSER parser( altiumLibFile, footprintData );

    parser.ReadAndSetSubrecordLength();
    //wxString footprintName = parser.ReadWxString(); // Not used (single-byte char set)
    parser.SkipSubrecord();

    LIB_ID fpID = AltiumToKiCadLibID( "", aFootprintName ); // TODO: library name
    footprint->SetFPID( fpID );

    const std::vector<std::string>  parametersStreamName{ fpDirName.ToStdString(),
                                                         "Parameters" };
    const CFB::COMPOUND_FILE_ENTRY* parametersData =
            altiumLibFile.FindStream( footprintStream, { "Parameters" } );

    if( parametersData != nullptr )
    {
        ALTIUM_BINARY_PARSER         parametersReader( altiumLibFile, parametersData );
        std::map<wxString, wxString> parameterProperties = parametersReader.ReadProperties();
        wxString description = ALTIUM_PROPS_UTILS::ReadString( parameterProperties,
                                                               wxT( "DESCRIPTION" ), wxT( "" ) );
        footprint->SetLibDescription( description );
    }
    else
    {
        if( m_reporter )
        {
            m_reporter->Report( wxString::Format( _( "File not found: '%s'." ),
                                                  FormatPath( parametersStreamName ) ),
                                RPT_SEVERITY_ERROR );
        }

        footprint->SetLibDescription( wxT( "" ) );
    }

    const std::vector<std::string> extendedPrimitiveInformationStreamName{
        "ExtendedPrimitiveInformation", "Data"
    };
    const CFB::COMPOUND_FILE_ENTRY* extendedPrimitiveInformationData =
            altiumLibFile.FindStream( footprintStream, extendedPrimitiveInformationStreamName );

    if( extendedPrimitiveInformationData != nullptr )
        ParseExtendedPrimitiveInformationData( altiumLibFile, extendedPrimitiveInformationData );

    footprint->SetReference( wxT( "REF**" ) );
    footprint->SetValue( aFootprintName );
    footprint->Reference().SetVisible( true ); // TODO: extract visibility information
    footprint->Value().SetVisible( true );

    const VECTOR2I defaultTextSize( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) );
    const int      defaultTextThickness( pcbIUScale.mmToIU( 0.15 ) );

    for( PCB_FIELD* field : footprint->GetFields() )
    {
        field->SetTextSize( defaultTextSize );
        field->SetTextThickness( defaultTextThickness );
    }

    for( int primitiveIndex = 0; parser.GetRemainingBytes() >= 4; primitiveIndex++ )
    {
        ALTIUM_RECORD recordtype = static_cast<ALTIUM_RECORD>( parser.Peek<uint8_t>() );

        switch( recordtype )
        {
        case ALTIUM_RECORD::ARC:
        {
            AARC6 arc( parser );
            ConvertArcs6ToFootprintItem( footprint.get(), arc, primitiveIndex, false );
            break;
        }
        case ALTIUM_RECORD::PAD:
        {
            APAD6 pad( parser );
            ConvertPads6ToFootprintItem( footprint.get(), pad );
            break;
        }
        case ALTIUM_RECORD::VIA:
        {
            AVIA6 via( parser );
            ConvertVias6ToFootprintItem( footprint.get(), via );
            break;
        }
        case ALTIUM_RECORD::TRACK:
        {
            ATRACK6 track( parser );
            ConvertTracks6ToFootprintItem( footprint.get(), track, primitiveIndex, false );
            break;
        }
        case ALTIUM_RECORD::TEXT:
        {
            ATEXT6 text( parser, m_unicodeStrings );
            ConvertTexts6ToFootprintItem( footprint.get(), text );
            break;
        }
        case ALTIUM_RECORD::FILL:
        {
            AFILL6 fill( parser );
            ConvertFills6ToFootprintItem( footprint.get(), fill, false );
            break;
        }
        case ALTIUM_RECORD::REGION:
        {
            AREGION6 region( parser, false );
            ConvertShapeBasedRegions6ToFootprintItem( footprint.get(), region, primitiveIndex );
            break;
        }
        case ALTIUM_RECORD::MODEL:
        {
            ACOMPONENTBODY6 componentBody( parser );
            ConvertComponentBody6ToFootprintItem( altiumLibFile, footprint.get(), componentBody );
            break;
        }
        default:
            THROW_IO_ERROR( wxString::Format( _( "Record of unknown type: '%d'." ), recordtype ) );
        }
    }


    // Loop over this multiple times to catch pads that are jumpered to each other by multiple shapes
    for( bool changes = true; changes; )
    {
        changes = false;

        alg::for_all_pairs( footprint->Pads().begin(), footprint->Pads().end(),
                [&changes]( PAD* aPad1, PAD* aPad2 )
                {
                    if( !( aPad1->GetNumber().IsEmpty() ^ aPad2->GetNumber().IsEmpty() ) )
                        return;

                    for( PCB_LAYER_ID layer : aPad1->GetLayerSet() )
                    {
                        std::shared_ptr<SHAPE> shape1 = aPad1->GetEffectiveShape( layer );
                        std::shared_ptr<SHAPE> shape2 = aPad2->GetEffectiveShape( layer );

                        if( shape1->Collide( shape2.get() ) )
                        {
                            if( aPad1->GetNumber().IsEmpty() )
                                aPad1->SetNumber( aPad2->GetNumber() );
                            else
                                aPad2->SetNumber( aPad1->GetNumber() );

                            changes = true;
                        }
                    }
                } );
    }

    // Auto-position reference and value
    footprint->AutoPositionFields();

    if( parser.HasParsingError() )
    {
        THROW_IO_ERROR( wxString::Format( wxT( "%s stream was not parsed correctly" ),
                                          FormatPath( streamName ) ) );
    }

    if( parser.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( wxString::Format( wxT( "%s stream is not fully parsed" ),
                                          FormatPath( streamName ) ) );
    }

    return footprint.release();
}

int ALTIUM_PCB::GetNetCode( uint16_t aId ) const
{
    if( aId == ALTIUM_NET_UNCONNECTED )
    {
        return NETINFO_LIST::UNCONNECTED;
    }
    else if( m_altiumToKicadNetcodes.size() < aId )
    {
        THROW_IO_ERROR( wxString::Format( wxT( "Netcode with id %d does not exist. Only %d nets "
                                               "are known" ),
                                          aId, m_altiumToKicadNetcodes.size() ) );
    }
    else
    {
        return m_altiumToKicadNetcodes[ aId ];
    }
}

const ARULE6* ALTIUM_PCB::GetRule( ALTIUM_RULE_KIND aKind, const wxString& aName ) const
{
    const auto rules = m_rules.find( aKind );

    if( rules == m_rules.end() )
        return nullptr;

    for( const ARULE6& rule : rules->second )
    {
        if( rule.name == aName )
            return &rule;
    }

    return nullptr;
}

const ARULE6* ALTIUM_PCB::GetRuleDefault( ALTIUM_RULE_KIND aKind ) const
{
    const auto rules = m_rules.find( aKind );

    if( rules == m_rules.end() )
        return nullptr;

    for( const ARULE6& rule : rules->second )
    {
        if( rule.scope1expr == wxT( "All" ) && rule.scope2expr == wxT( "All" ) )
            return &rule;
    }

    return nullptr;
}

void ALTIUM_PCB::ParseFileHeader( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    reader.ReadAndSetSubrecordLength();
    wxString header = reader.ReadWxString();

    //std::cout << "HEADER: " << header << std::endl;  // tells me: PCB 5.0 Binary File

    //reader.SkipSubrecord();

    // TODO: does not seem to work all the time at the moment
    //if( reader.GetRemainingBytes() != 0 )
    //    THROW_IO_ERROR( "FileHeader stream is not fully parsed" );
}


void ALTIUM_PCB::ParseExtendedPrimitiveInformationData( const ALTIUM_PCB_COMPOUND_FILE& aAltiumPcbFile,
                                                        const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading extended primitive information data..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AEXTENDED_PRIMITIVE_INFORMATION elem( reader );

        m_extendedPrimitiveInformationMaps[elem.primitiveObjectId].emplace( elem.primitiveIndex,
                                                                            std::move( elem ) );
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "ExtendedPrimitiveInformation stream is not fully parsed" ) );
}


void ALTIUM_PCB::ParseBoard6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading board data..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    checkpoint();
    ABOARD6 elem( reader );

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Board6 stream is not fully parsed" ) );

    m_board->GetDesignSettings().SetAuxOrigin( elem.sheetpos );
    m_board->GetDesignSettings().SetGridOrigin( elem.sheetpos );

    // read layercount from stackup, because LAYERSETSCOUNT is not always correct?!
    size_t layercount = 0;
    size_t layerid = static_cast<size_t>( ALTIUM_LAYER::TOP_LAYER );

    while( layerid < elem.stackup.size() && layerid != 0 )
    {
        layerid = elem.stackup[ layerid - 1 ].nextId;
        layercount++;
    }

    size_t kicadLayercount = ( layercount % 2 == 0 ) ? layercount : layercount + 1;
    m_board->SetCopperLayerCount( kicadLayercount );

    BOARD_DESIGN_SETTINGS& designSettings = m_board->GetDesignSettings();
    BOARD_STACKUP&         stackup        = designSettings.GetStackupDescriptor();

    // create board stackup
    stackup.RemoveAll(); // Just to be sure
    stackup.BuildDefaultStackupList( &designSettings, layercount );

    auto it = stackup.GetList().begin();

    // find first copper layer
    for( ; it != stackup.GetList().end() && ( *it )->GetType() != BS_ITEM_TYPE_COPPER; ++it )
        ;

    auto cuLayer = LAYER_RANGE( F_Cu, B_Cu, 32 ).begin();

    for( size_t altiumLayerId = static_cast<size_t>( ALTIUM_LAYER::TOP_LAYER );
            altiumLayerId < elem.stackup.size() && altiumLayerId != 0;
            altiumLayerId = elem.stackup[altiumLayerId - 1].nextId )
    {
        // array starts with 0, but stackup with 1
        ABOARD6_LAYER_STACKUP& layer = elem.stackup.at( altiumLayerId - 1 );

        // handle unused layer in case of odd layercount
        if( layer.nextId == 0 && layercount != kicadLayercount )
        {
            m_board->SetLayerName( ( *it )->GetBrdLayerId(), wxT( "[unused]" ) );

            if( ( *it )->GetType() != BS_ITEM_TYPE_COPPER )
                THROW_IO_ERROR( wxT( "Board6 stream, unexpected item while parsing stackup" ) );

            ( *it )->SetThickness( 0 );

            ++it;

            if( ( *it )->GetType() != BS_ITEM_TYPE_DIELECTRIC )
                THROW_IO_ERROR( wxT( "Board6 stream, unexpected item while parsing stackup" ) );

            ( *it )->SetThickness( 0, 0 );
            ( *it )->SetThicknessLocked( true, 0 );
            ++it;
        }

        m_layermap.insert( { static_cast<ALTIUM_LAYER>( altiumLayerId ), *cuLayer } );
        ++cuLayer;

        if( ( *it )->GetType() != BS_ITEM_TYPE_COPPER )
            THROW_IO_ERROR( wxT( "Board6 stream, unexpected item while parsing stackup" ) );

        ( *it )->SetThickness( layer.copperthick );

        ALTIUM_LAYER alayer = static_cast<ALTIUM_LAYER>( altiumLayerId );
        PCB_LAYER_ID klayer = ( *it )->GetBrdLayerId();

        m_board->SetLayerName( klayer, layer.name );

        if( layer.copperthick == 0 )
            m_board->SetLayerType( klayer, LAYER_T::LT_JUMPER ); // used for things like wirebonding
        else if( IsAltiumLayerAPlane( alayer ) )
            m_board->SetLayerType( klayer, LAYER_T::LT_POWER );

        if( klayer == B_Cu )
        {
            if( layer.nextId != 0 )
                THROW_IO_ERROR( wxT( "Board6 stream, unexpected id while parsing last stackup layer" ) );

            // overwrite entry from internal -> bottom
            m_layermap[alayer] = B_Cu;
            break;
        }

        ++it;

        if( ( *it )->GetType() != BS_ITEM_TYPE_DIELECTRIC )
            THROW_IO_ERROR( wxT( "Board6 stream, unexpected item while parsing stackup" ) );

        ( *it )->SetThickness( layer.dielectricthick, 0 );
        ( *it )->SetMaterial( layer.dielectricmaterial.empty() ?
                                              NotSpecifiedPrm() :
                                              wxString( layer.dielectricmaterial ) );
        ( *it )->SetEpsilonR( layer.dielectricconst, 0 );

        ++it;
    }

    remapUnsureLayers( elem.stackup );

    // Set name of all non-cu layers
    for( size_t altiumLayerId = static_cast<size_t>( ALTIUM_LAYER::TOP_OVERLAY );
         altiumLayerId <= static_cast<size_t>( ALTIUM_LAYER::BOTTOM_SOLDER ); altiumLayerId++ )
    {
        // array starts with 0, but stackup with 1
        ABOARD6_LAYER_STACKUP& layer = elem.stackup.at( altiumLayerId - 1 );

        ALTIUM_LAYER alayer = static_cast<ALTIUM_LAYER>( altiumLayerId );
        PCB_LAYER_ID klayer = GetKicadLayer( alayer );

        m_board->SetLayerName( klayer, layer.name );
    }

    for( size_t altiumLayerId = static_cast<size_t>( ALTIUM_LAYER::MECHANICAL_1 );
         altiumLayerId <= static_cast<size_t>( ALTIUM_LAYER::MECHANICAL_16 ); altiumLayerId++ )
    {
        // array starts with 0, but stackup with 1
        ABOARD6_LAYER_STACKUP& layer = elem.stackup.at( altiumLayerId - 1 );

        ALTIUM_LAYER alayer = static_cast<ALTIUM_LAYER>( altiumLayerId );
        PCB_LAYER_ID klayer = GetKicadLayer( alayer );

        m_board->SetLayerName( klayer, layer.name );
    }

    HelperCreateBoardOutline( elem.board_vertices );
    m_board->GetDesignSettings().SetBoardThickness( stackup.BuildBoardThicknessFromStackup() );
}


void ALTIUM_PCB::remapUnsureLayers( std::vector<ABOARD6_LAYER_STACKUP>& aStackup )
{
    LSET enabledLayers        = m_board->GetEnabledLayers();
    LSET validRemappingLayers = enabledLayers    | LSET::AllBoardTechMask() |
                                LSET::UserMask() | LSET::UserDefinedLayersMask();

    if( aStackup.size() == 0 )
        return;

    std::vector<INPUT_LAYER_DESC> inputLayers;
    std::map<wxString, ALTIUM_LAYER>  altiumLayerNameMap;

    ABOARD6_LAYER_STACKUP& curLayer = aStackup[0];
    ALTIUM_LAYER           layer_num;
    INPUT_LAYER_DESC       iLdesc;

    auto next =
            [&]( size_t ii ) -> size_t
            {
                // Within the copper stack, the nextId can be used to hop over unused layers in
                // a particular Altium board.  The IDs start with ALTIUM_LAYER::UNKNOWN but the
                // first copper layer in the array will be ALTIUM_LAYER::TOP_LAYER.
                if( layer_num < ALTIUM_LAYER::BOTTOM_LAYER )
                    return curLayer.nextId - 1;
                else
                    return ii + 1;
            };

    for( size_t ii = 0; ii < aStackup.size(); ii = next( ii ) )
    {
        curLayer = aStackup[ii];
        layer_num = static_cast<ALTIUM_LAYER>( ii + 1 );

        if( m_layermap.find( layer_num ) != m_layermap.end() )
            continue;

        if( ii >= m_board->GetCopperLayerCount() && layer_num != ALTIUM_LAYER::BOTTOM_LAYER
            && !( layer_num >= ALTIUM_LAYER::TOP_OVERLAY
                   && layer_num <= ALTIUM_LAYER::BOTTOM_SOLDER )
            && !( layer_num >= ALTIUM_LAYER::MECHANICAL_1
                   && layer_num <= ALTIUM_LAYER::MECHANICAL_16 ) )
        {
            if( layer_num < ALTIUM_LAYER::BOTTOM_LAYER )
                continue;

            iLdesc.AutoMapLayer = PCB_LAYER_ID::UNDEFINED_LAYER;
        }
        else
        {
            iLdesc.AutoMapLayer = GetKicadLayer( layer_num );
        }

        iLdesc.Name            = curLayer.name;
        iLdesc.PermittedLayers = validRemappingLayers;
        iLdesc.Required        = ii < m_board->GetCopperLayerCount() || layer_num == ALTIUM_LAYER::BOTTOM_LAYER;

        inputLayers.push_back( iLdesc );
        altiumLayerNameMap.insert( { curLayer.name, layer_num } );
        m_layerNames.insert( { layer_num, curLayer.name } );
    }

    if( inputLayers.size() == 0 )
        return;

    // Callback:
    std::map<wxString, PCB_LAYER_ID> reMappedLayers = m_layerMappingHandler( inputLayers );

    for( std::pair<wxString, PCB_LAYER_ID> layerPair : reMappedLayers )
    {
        if( layerPair.second == PCB_LAYER_ID::UNDEFINED_LAYER )
        {
            // Layer mapping handler returned UNDEFINED_LAYER - skip this layer
            // This can happen for layers that don't have a KiCad equivalent
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "Layer '%s' could not be mapped and "
                                                         "will be skipped." ),
                                                      layerPair.first ),
                                    RPT_SEVERITY_WARNING );
            }

            continue;
        }

        ALTIUM_LAYER altiumID     = altiumLayerNameMap.at( layerPair.first );
        m_layermap.insert_or_assign( altiumID, layerPair.second );
        enabledLayers |= LSET( { layerPair.second } );
    }

    m_board->SetEnabledLayers( enabledLayers );
    m_board->SetVisibleLayers( enabledLayers );
}


void ALTIUM_PCB::HelperCreateBoardOutline( const std::vector<ALTIUM_VERTICE>& aVertices )
{
    SHAPE_LINE_CHAIN lineChain;
    HelperShapeLineChainFromAltiumVertices( lineChain, aVertices );

    STROKE_PARAMS stroke( m_board->GetDesignSettings().GetLineThickness( Edge_Cuts ),
                          LINE_STYLE::SOLID );

    for( int i = 0; i <= lineChain.PointCount() && i != -1; i = lineChain.NextShape( i ) )
    {
        if( lineChain.IsArcStart( i ) )
        {
            const SHAPE_ARC& currentArc = lineChain.Arc( lineChain.ArcIndex( i ) );
            int              nextShape = lineChain.NextShape( i );
            bool             isLastShape = nextShape < 0;

            std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( m_board, SHAPE_T::ARC );

            shape->SetStroke( stroke );
            shape->SetLayer( Edge_Cuts );
            shape->SetArcGeometry( currentArc.GetP0(), currentArc.GetArcMid(), currentArc.GetP1() );

            m_board->Add( shape.release(), ADD_MODE::APPEND );
        }
        else
        {
            const SEG& seg = lineChain.Segment( i );

            std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( m_board, SHAPE_T::SEGMENT );

            shape->SetStroke( stroke );
            shape->SetLayer( Edge_Cuts );
            shape->SetStart( seg.A );
            shape->SetEnd( seg.B );

            m_board->Add( shape.release(), ADD_MODE::APPEND );
        }
    }
}


void ALTIUM_PCB::ParseClasses6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                    const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading netclasses..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ACLASS6 elem( reader );

        if( elem.kind == ALTIUM_CLASS_KIND::NET_CLASS )
        {
            std::shared_ptr<NETCLASS> nc = std::make_shared<NETCLASS>( elem.name );

            for( const wxString& name : elem.names )
            {
                m_board->GetDesignSettings().m_NetSettings->SetNetclassPatternAssignment(
                        name, nc->GetName() );
            }

            if( m_board->GetDesignSettings().m_NetSettings->HasNetclass( nc->GetName() ) )
            {
                // Name conflict, happens in some unknown circumstances
                // unique_ptr will delete nc on this code path
                if( m_reporter )
                {
                    wxString msg;
                    msg.Printf( _( "More than one Altium netclass with name '%s' found. "
                                   "Only the first one will be imported." ), elem.name );
                    m_reporter->Report( msg, RPT_SEVERITY_ERROR );
                }
            }
            else
            {
                m_board->GetDesignSettings().m_NetSettings->SetNetclass( nc->GetName(), nc );
            }
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Classes6 stream is not fully parsed" ) );

    // Now that all netclasses and pattern assignments are set up, resolve the pattern
    // assignments to direct netclass assignments on each net.
    std::shared_ptr<NET_SETTINGS> netSettings = m_board->GetDesignSettings().m_NetSettings;

    for( NETINFO_ITEM* net : m_board->GetNetInfo() )
    {
        if( net->GetNetCode() > 0 )
        {
            std::shared_ptr<NETCLASS> netclass = netSettings->GetEffectiveNetClass( net->GetNetname() );

            if( netclass )
                net->SetNetClass( netclass );
        }
    }

    m_board->m_LegacyNetclassesLoaded = true;
}


void ALTIUM_PCB::ParseComponents6Data( const ALTIUM_PCB_COMPOUND_FILE& aAltiumPcbFile,
                                       const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading components..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    uint16_t componentId = 0;

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ACOMPONENT6 elem( reader );

        std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( m_board );

        // Altium stores the footprint library information needed to find the footprint in the
        // source library in the sourcefootprintlibrary field.  Since Altium is a Windows-only
        // program, the path separator is always a backslash.  We need strip the extra path information
        // here to prevent overly-long LIB_IDs because KiCad doesn't store the full path to the
        // footprint library in the design file, only in a library table.
        wxFileName libName( elem.sourcefootprintlibrary, wxPATH_WIN );

        // The pattern field may also contain a path when Altium stores it with a full library path.
        // Extract just the footprint name portion to avoid creating invalid filenames.
        wxString fpName = elem.pattern;

        if( fpName.Contains( wxT( "\\" ) ) || fpName.Contains( wxT( "/" ) ) )
        {
            wxFileName fpPath( fpName, wxPATH_WIN );
            fpName = fpPath.GetFullName();
        }

        LIB_ID fpID = AltiumToKiCadLibID( libName.GetName(), fpName );

        footprint->SetFPID( fpID );

        footprint->SetPosition( elem.position );
        footprint->SetOrientationDegrees( elem.rotation );

        // KiCad netlisting requires parts to have non-digit + digit annotation.
        // If the reference begins with a number, we prepend 'UNK' (unknown) for the source designator
        wxString reference = elem.sourcedesignator;

        if( reference.find_first_not_of( "0123456789" ) == wxString::npos )
            reference.Prepend( wxT( "UNK" ) );

        footprint->SetReference( reference );

        KIID id( elem.sourceUniqueID );
        KIID pathid( elem.sourceHierachicalPath );
        KIID_PATH path;
        path.push_back( pathid );
        path.push_back( id );

        footprint->SetPath( path );
        footprint->SetSheetname( elem.sourceHierachicalPath );
        footprint->SetSheetfile( elem.sourceHierachicalPath + wxT( ".kicad_sch" ));

        footprint->SetLocked( elem.locked );
        footprint->Reference().SetVisible( elem.nameon );
        footprint->Value().SetVisible( elem.commenton );
        footprint->SetLayer( elem.layer == ALTIUM_LAYER::TOP_LAYER ? F_Cu : B_Cu );

        m_components.emplace_back( footprint.get() );
        m_board->Add( footprint.release(), ADD_MODE::APPEND );

        componentId++;
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Components6 stream is not fully parsed" ) );
}


/// Normalize angle to be aMin < angle <= aMax angle is in degrees.
double normalizeAngleDegrees( double Angle, double aMin, double aMax )
{
    while( Angle < aMin )
        Angle += 360.0;

    while( Angle >= aMax )
        Angle -= 360.0;

    return Angle;
}


void ALTIUM_PCB::ConvertComponentBody6ToFootprintItem( const ALTIUM_PCB_COMPOUND_FILE& aAltiumPcbFile,
                                                       FOOTPRINT* aFootprint,
                                                       const ACOMPONENTBODY6& aElem )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading component 3D models..." ) );

    if( !aElem.modelIsEmbedded )
        return;

    auto model = aAltiumPcbFile.GetLibModel( aElem.modelId );

    if( !model )
    {
        if( m_reporter )
        {
            m_reporter->Report( wxString::Format( wxT( "Model %s not found for footprint %s" ),
                                                  aElem.modelId, aFootprint->GetReference() ),
                                RPT_SEVERITY_ERROR );
        }

        return;
    }

    const VECTOR2I& fpPosition = aFootprint->GetPosition();

    EMBEDDED_FILES::EMBEDDED_FILE* file = new EMBEDDED_FILES::EMBEDDED_FILE();
    file->name = aElem.modelName;

    if( file->name.IsEmpty() )
        file->name = model->first.name;

    // Decompress the model data before assigning
    std::vector<char>   decompressedData;
    wxMemoryInputStream compressedStream( model->second.data(), model->second.size() );
    wxZlibInputStream   zlibStream( compressedStream );

    // Reserve some space, assuming decompressed data is larger -- STEP file
    // compression is typically 5:1 using zlib like Altium does
    decompressedData.resize( model->second.size() * 6 );
    size_t offset = 0;

    while( !zlibStream.Eof() )
    {
        zlibStream.Read( decompressedData.data() + offset, decompressedData.size() - offset );
        size_t bytesRead = zlibStream.LastRead();

        if( !bytesRead )
            break;

        offset += bytesRead;

        if( offset >= decompressedData.size() )
            decompressedData.resize( 2 * decompressedData.size() ); // Resizing is expensive, avoid if we can
    }

    decompressedData.resize( offset );

    file->decompressedData = std::move( decompressedData );
    file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::MODEL;

    EMBEDDED_FILES::CompressAndEncode( *file );
    aFootprint->GetEmbeddedFiles()->AddFile( file );

    FP_3DMODEL modelSettings;

    modelSettings.m_Filename = aFootprint->GetEmbeddedFiles()->GetEmbeddedFileLink( *file );

    modelSettings.m_Offset.x = pcbIUScale.IUTomm( (int) aElem.modelPosition.x );
    modelSettings.m_Offset.y = -pcbIUScale.IUTomm( (int) aElem.modelPosition.y );
    modelSettings.m_Offset.z = pcbIUScale.IUTomm( (int) aElem.modelPosition.z );

    EDA_ANGLE orientation = aFootprint->GetOrientation();

    if( aFootprint->IsFlipped() )
    {
        modelSettings.m_Offset.y = -modelSettings.m_Offset.y;
        orientation              = -orientation;
    }

    VECTOR3D modelRotation( aElem.modelRotation );

    if( ( aElem.body_projection == 1 ) != aFootprint->IsFlipped() )
    {
        modelRotation.x += 180;
        modelRotation.z = -modelRotation.z;

        modelSettings.m_Offset.z = -DEFAULT_BOARD_THICKNESS_MM - modelSettings.m_Offset.z;
    }

    RotatePoint( &modelSettings.m_Offset.x, &modelSettings.m_Offset.y, orientation );

    modelSettings.m_Rotation.x = normalizeAngleDegrees( -modelRotation.x, -180, 180 );
    modelSettings.m_Rotation.y = normalizeAngleDegrees( -modelRotation.y, -180, 180 );
    modelSettings.m_Rotation.z = normalizeAngleDegrees( -modelRotation.z + aElem.rotation
                                                                         + orientation.AsDegrees(),
                                                                           -180, 180 );
    modelSettings.m_Opacity = aElem.body_opacity_3d;

    aFootprint->Models().push_back( modelSettings );
}


void ALTIUM_PCB::ParseComponentsBodies6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                             const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading component 3D models..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ACOMPONENTBODY6 elem( reader );

        static const bool skipComponentBodies = ADVANCED_CFG::GetCfg().m_ImportSkipComponentBodies;

        if( skipComponentBodies )
            continue;

        if( elem.component == ALTIUM_COMPONENT_NONE )
            continue; // TODO: we do not support components for the board yet

        if( m_components.size() <= elem.component )
        {
            THROW_IO_ERROR( wxString::Format( wxT( "ComponentsBodies6 stream tries to access "
                                                   "component id %d of %zu existing components" ),
                                              elem.component,
                                              m_components.size() ) );
        }

        if( !elem.modelIsEmbedded )
            continue;

        auto modelTuple = m_EmbeddedModels.find( elem.modelId );

        if( modelTuple == m_EmbeddedModels.end() )
        {
            if( m_reporter )
            {
                wxString msg;
                msg.Printf( wxT( "ComponentsBodies6 stream tries to access model id %s which does "
                                 "not exist" ), elem.modelId );
                m_reporter->Report( msg, RPT_SEVERITY_ERROR );
            }

            continue;
        }

        const ALTIUM_EMBEDDED_MODEL_DATA& modelData = modelTuple->second;
        FOOTPRINT*                        footprint = m_components.at( elem.component );

        EMBEDDED_FILES::EMBEDDED_FILE* file = new EMBEDDED_FILES::EMBEDDED_FILE();
        file->name = modelData.m_modelname;

        wxMemoryInputStream  compressedStream( modelData.m_data.data(), modelData.m_data.size() );
        wxZlibInputStream    zlibStream( compressedStream );
        wxMemoryOutputStream decompressedStream;

        zlibStream.Read( decompressedStream );
        file->decompressedData.resize( decompressedStream.GetSize() );
        decompressedStream.CopyTo( file->decompressedData.data(), file->decompressedData.size() );

        EMBEDDED_FILES::CompressAndEncode( *file );
        footprint->GetEmbeddedFiles()->AddFile( file );

        FP_3DMODEL modelSettings;

        modelSettings.m_Filename = footprint->GetEmbeddedFiles()->GetEmbeddedFileLink( *file );
        VECTOR2I fpPosition = footprint->GetPosition();

        modelSettings.m_Offset.x =
                pcbIUScale.IUTomm( KiROUND( elem.modelPosition.x - fpPosition.x ) );
        modelSettings.m_Offset.y =
                -pcbIUScale.IUTomm( KiROUND( elem.modelPosition.y - fpPosition.y ) );
        modelSettings.m_Offset.z = pcbIUScale.IUTomm( KiROUND( elem.modelPosition.z ) );

        EDA_ANGLE orientation = footprint->GetOrientation();

        if( footprint->IsFlipped() )
        {
            modelSettings.m_Offset.y = -modelSettings.m_Offset.y;
            orientation              = -orientation;
        }

        if( ( elem.body_projection == 1 ) != footprint->IsFlipped() )
        {
            elem.modelRotation.x += 180;
            elem.modelRotation.z = -elem.modelRotation.z;

            modelSettings.m_Offset.z =
                    -pcbIUScale.IUTomm( m_board->GetDesignSettings().GetBoardThickness() )
                    - modelSettings.m_Offset.z;
        }

        RotatePoint( &modelSettings.m_Offset.x, &modelSettings.m_Offset.y, orientation );

        modelSettings.m_Rotation.x = normalizeAngleDegrees( -elem.modelRotation.x, -180, 180 );
        modelSettings.m_Rotation.y = normalizeAngleDegrees( -elem.modelRotation.y, -180, 180 );
        modelSettings.m_Rotation.z = normalizeAngleDegrees( -elem.modelRotation.z + elem.rotation
                                                                                  + orientation.AsDegrees(),
                                                                                   -180, 180 );

        modelSettings.m_Opacity = elem.body_opacity_3d;

        footprint->Models().push_back( modelSettings );
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "ComponentsBodies6 stream is not fully parsed" ) );
}


void ALTIUM_PCB::HelperParseDimensions6Linear( const ADIMENSION6& aElem )
{
    if( aElem.referencePoint.size() != 2 )
        THROW_IO_ERROR( wxT( "Incorrect number of reference points for linear dimension object" ) );

    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        if( m_reporter )
        {
            m_reporter->Report( wxString::Format(
                    _( "Dimension found on an Altium layer (%d) with no KiCad equivalent. "
                           "It has been moved to KiCad layer Eco1_User." ), aElem.layer ),
                        RPT_SEVERITY_INFO );
        }

        klayer = Eco1_User;
    }

    VECTOR2I referencePoint0 = aElem.referencePoint.at( 0 );
    VECTOR2I referencePoint1 = aElem.referencePoint.at( 1 );

    std::unique_ptr<PCB_DIM_ALIGNED> dimension = std::make_unique<PCB_DIM_ALIGNED>( m_board, PCB_DIM_ALIGNED_T );

    dimension->SetPrecision( static_cast<DIM_PRECISION>( aElem.textprecision ) );
    dimension->SetLayer( klayer );
    dimension->SetStart( referencePoint0 );

    if( referencePoint0 != aElem.xy1 )
    {
        /**
         * Basically REFERENCE0POINT and REFERENCE1POINT are the two end points of the dimension.
         * XY1 is the position of the arrow above REFERENCE0POINT. those three points are not
         * necessarily in 90degree angle, but KiCad requires this to show the correct measurements.
         *
         * Therefore, we take the vector of REFERENCE0POINT -> XY1, calculate the normal, and
         * intersect it with REFERENCE1POINT pointing the same direction as REFERENCE0POINT -> XY1.
         * This should give us a valid measurement point where we can place the drawsegment.
         */
        VECTOR2I     direction             = aElem.xy1 - referencePoint0;
        VECTOR2I     referenceDiff         = referencePoint1 - referencePoint0;
        VECTOR2I     directionNormalVector = direction.Perpendicular();
        SEG          segm1( referencePoint0, referencePoint0 + directionNormalVector );
        SEG          segm2( referencePoint1, referencePoint1 + direction );
        OPT_VECTOR2I intersection( segm1.Intersect( segm2, true, true ) );

        if( !intersection )
            THROW_IO_ERROR( wxT( "Invalid dimension.  This should never happen." ) );

        dimension->SetEnd( *intersection );

        int height = direction.EuclideanNorm();

        if( direction.Cross( referenceDiff ) > 0 )
            height = -height;

        dimension->SetHeight( height );
    }
    else
    {
        dimension->SetEnd( referencePoint1 );
    }

    dimension->SetLineThickness( aElem.linewidth );

    dimension->SetUnitsFormat( DIM_UNITS_FORMAT::NO_SUFFIX );
    dimension->SetPrefix( aElem.textprefix );


    int dist = ( dimension->GetEnd() - dimension->GetStart() ).EuclideanNorm();

    if( dist < 3 * dimension->GetArrowLength() )
        dimension->SetArrowDirection( DIM_ARROW_DIRECTION::INWARD );

    // Suffix normally (but not always) holds the units
    wxRegEx units( wxS( "(mm)|(in)|(mils)|(thou)|(')|(\")" ), wxRE_ADVANCED );

    if( units.Matches( aElem.textsuffix ) )
        dimension->SetUnitsFormat( DIM_UNITS_FORMAT::BARE_SUFFIX );
    else
        dimension->SetSuffix( aElem.textsuffix );

    dimension->SetTextThickness( aElem.textlinewidth );
    dimension->SetTextSize( VECTOR2I( aElem.textheight, aElem.textheight ) );
    dimension->SetItalic( aElem.textitalic );

#if 0  // we don't currently support bold; map to thicker text
    dimension->Text().SetBold( aElem.textbold );
#else
    if( aElem.textbold )
        dimension->SetTextThickness( dimension->GetTextThickness() * BOLD_FACTOR );
#endif

    switch( aElem.textunit )
    {
    case ALTIUM_UNIT::INCH: dimension->SetUnits( EDA_UNITS::INCH );   break;
    case ALTIUM_UNIT::MILS: dimension->SetUnits( EDA_UNITS::MILS ); break;
    case ALTIUM_UNIT::MM:   dimension->SetUnits( EDA_UNITS::MM );   break;
    case ALTIUM_UNIT::CM:   dimension->SetUnits( EDA_UNITS::MM );   break;
    default:                                                        break;
    }

    m_board->Add( dimension.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::HelperParseDimensions6Radial(const ADIMENSION6 &aElem)
{
    if( aElem.referencePoint.size() < 2 )
        THROW_IO_ERROR( wxT( "Not enough reference points for radial dimension object" ) );

    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        if( m_reporter )
        {
            m_reporter->Report( wxString::Format(
                _( "Dimension found on an Altium layer (%d) with no KiCad equivalent. "
                   "It has been moved to KiCad layer Eco1_User." ),
                aElem.layer ), RPT_SEVERITY_INFO );
        }

        klayer = Eco1_User;
    }

    VECTOR2I referencePoint0 = aElem.referencePoint.at( 0 );
    VECTOR2I referencePoint1 = aElem.referencePoint.at( 1 );

    std::unique_ptr<PCB_DIM_RADIAL> dimension = std::make_unique<PCB_DIM_RADIAL>( m_board );

    dimension->SetPrecision( static_cast<DIM_PRECISION>( aElem.textprecision ) );
    dimension->SetLayer( klayer );
    dimension->SetStart( referencePoint0 );
    dimension->SetEnd( aElem.xy1 );
    dimension->SetLineThickness( aElem.linewidth );
    dimension->SetKeepTextAligned( false );

    dimension->SetPrefix( aElem.textprefix );

    // Suffix normally holds the units
    dimension->SetUnitsFormat( aElem.textsuffix.IsEmpty() ? DIM_UNITS_FORMAT::NO_SUFFIX
                                                          : DIM_UNITS_FORMAT::BARE_SUFFIX );

    switch( aElem.textunit )
    {
    case ALTIUM_UNIT::INCH: dimension->SetUnits( EDA_UNITS::INCH );   break;
    case ALTIUM_UNIT::MILS: dimension->SetUnits( EDA_UNITS::MILS ); break;
    case ALTIUM_UNIT::MM:   dimension->SetUnits( EDA_UNITS::MM );   break;
    case ALTIUM_UNIT::CM:   dimension->SetUnits( EDA_UNITS::MM );   break;
    default:                                                        break;
    }

    if( aElem.textPoint.empty() )
    {
        if( m_reporter )
        {
            m_reporter->Report( wxT( "No text position present for leader dimension object" ),
                                RPT_SEVERITY_ERROR );
        }

        return;
    }

    dimension->SetTextPos( aElem.textPoint.at( 0 ) );
    dimension->SetTextThickness( aElem.textlinewidth );
    dimension->SetTextSize( VECTOR2I( aElem.textheight, aElem.textheight ) );
    dimension->SetItalic( aElem.textitalic );

#if 0  // we don't currently support bold; map to thicker text
    dimension->SetBold( aElem.textbold );
#else
    if( aElem.textbold )
        dimension->SetTextThickness( dimension->GetTextThickness() * BOLD_FACTOR );
#endif

    // It's unclear exactly how Altium figures it's text positioning, but this gets us reasonably
    // close.
    dimension->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    dimension->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

    int yAdjust = dimension->GetTextBox( nullptr ).GetCenter().y - dimension->GetTextPos().y;
    dimension->SetTextPos( dimension->GetTextPos() + VECTOR2I( 0, yAdjust + aElem.textgap ) );
    dimension->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

    m_radialDimensions.push_back( dimension.get() );
    m_board->Add( dimension.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::HelperParseDimensions6Leader( const ADIMENSION6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        if( m_reporter )
        {
            wxString msg;
            msg.Printf( _( "Dimension found on an Altium layer (%d) with no KiCad equivalent. "
                          "It has been moved to KiCad layer Eco1_User." ), aElem.layer );
            m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        }

        klayer = Eco1_User;
    }

    if( !aElem.referencePoint.empty() )
    {
        VECTOR2I referencePoint0 = aElem.referencePoint.at( 0 );

        // line
        VECTOR2I last = referencePoint0;
        for( size_t i = 1; i < aElem.referencePoint.size(); i++ )
        {
            std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( m_board, SHAPE_T::SEGMENT );

            shape->SetLayer( klayer );
            shape->SetStroke( STROKE_PARAMS( aElem.linewidth, LINE_STYLE::SOLID ) );
            shape->SetStart( last );
            shape->SetEnd( aElem.referencePoint.at( i ) );
            last = aElem.referencePoint.at( i );

            m_board->Add( shape.release(), ADD_MODE::APPEND );
        }

        // arrow
        if( aElem.referencePoint.size() >= 2 )
        {
            VECTOR2I dirVec = aElem.referencePoint.at( 1 ) - referencePoint0;

            if( dirVec.x != 0 || dirVec.y != 0 )
            {
                double   scaling = (double) dirVec.EuclideanNorm() / aElem.arrowsize;
                VECTOR2I arrVec = KiROUND( dirVec.x / scaling, dirVec.y / scaling );
                RotatePoint( arrVec, EDA_ANGLE( 20.0, DEGREES_T ) );

                {
                    std::unique_ptr<PCB_SHAPE> shape1 = std::make_unique<PCB_SHAPE>( m_board, SHAPE_T::SEGMENT );

                    shape1->SetLayer( klayer );
                    shape1->SetStroke( STROKE_PARAMS( aElem.linewidth, LINE_STYLE::SOLID ) );
                    shape1->SetStart( referencePoint0 );
                    shape1->SetEnd( referencePoint0 + arrVec );

                    m_board->Add( shape1.release(), ADD_MODE::APPEND );
                }

                RotatePoint( arrVec, EDA_ANGLE( -40.0, DEGREES_T ) );

                {
                    std::unique_ptr<PCB_SHAPE> shape2 = std::make_unique<PCB_SHAPE>( m_board, SHAPE_T::SEGMENT );

                    shape2->SetLayer( klayer );
                    shape2->SetStroke( STROKE_PARAMS( aElem.linewidth, LINE_STYLE::SOLID ) );
                    shape2->SetStart( referencePoint0 );
                    shape2->SetEnd( referencePoint0 + arrVec );

                    m_board->Add( shape2.release(), ADD_MODE::APPEND );
                }
            }
        }
    }

    if( aElem.textPoint.empty() )
    {
        if( m_reporter )
        {
            m_reporter->Report( wxT( "No text position present for leader dimension object" ),
                                RPT_SEVERITY_ERROR );
        }

        return;
    }

    std::unique_ptr<PCB_TEXT> text = std::make_unique<PCB_TEXT>( m_board );

    text->SetText( aElem.textformat );
    text->SetPosition( aElem.textPoint.at( 0 ) );
    text->SetLayer( klayer );
    text->SetTextSize( VECTOR2I( aElem.textheight, aElem.textheight ) ); // TODO: parse text width
    text->SetTextThickness( aElem.textlinewidth );
    text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );

    m_board->Add( text.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::HelperParseDimensions6Datum( const ADIMENSION6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        if( m_reporter )
        {
            wxString msg;
            msg.Printf( _( "Dimension found on an Altium layer (%d) with no KiCad equivalent. "
                           "It has been moved to KiCad layer Eco1_User." ), aElem.layer );
            m_reporter->Report( msg, RPT_SEVERITY_INFO );
        }

        klayer = Eco1_User;
    }

    for( size_t i = 0; i < aElem.referencePoint.size(); i++ )
    {
        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( m_board, SHAPE_T::SEGMENT );

        shape->SetLayer( klayer );
        shape->SetStroke( STROKE_PARAMS( aElem.linewidth, LINE_STYLE::SOLID ) );
        shape->SetStart( aElem.referencePoint.at( i ) );
        // shape->SetEnd( /* TODO: seems to be based on TEXTY */ );

        m_board->Add( shape.release(), ADD_MODE::APPEND );
    }
}


void ALTIUM_PCB::HelperParseDimensions6Center( const ADIMENSION6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        if( m_reporter )
        {
            wxString msg;
            msg.Printf( _( "Dimension found on an Altium layer (%d) with no KiCad equivalent. "
                           "It has been moved to KiCad layer Eco1_User." ), aElem.layer );
            m_reporter->Report( msg, RPT_SEVERITY_INFO );
        }

        klayer = Eco1_User;
    }

    VECTOR2I vec = VECTOR2I( 0, aElem.height / 2 );
    RotatePoint( vec, EDA_ANGLE( aElem.angle, DEGREES_T ) );

    std::unique_ptr<PCB_DIM_CENTER> dimension = std::make_unique<PCB_DIM_CENTER>( m_board );

    dimension->SetLayer( klayer );
    dimension->SetLineThickness( aElem.linewidth );
    dimension->SetStart( aElem.xy1 );
    dimension->SetEnd( aElem.xy1 + vec );

    m_board->Add( dimension.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::ParseDimensions6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                       const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading dimension drawings..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ADIMENSION6 elem( reader );

        switch( elem.kind )
        {
        case ALTIUM_DIMENSION_KIND::LINEAR:
            HelperParseDimensions6Linear( elem );
            break;
        case ALTIUM_DIMENSION_KIND::ANGULAR:
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "Ignored Angular dimension (not yet supported)." ) ),
                                    RPT_SEVERITY_INFO );
            }
            break;
        case ALTIUM_DIMENSION_KIND::RADIAL:
            HelperParseDimensions6Radial( elem );
            break;
        case ALTIUM_DIMENSION_KIND::LEADER:
            HelperParseDimensions6Leader( elem );
            break;
        case ALTIUM_DIMENSION_KIND::DATUM:
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "Ignored Datum dimension (not yet supported)." ) ),
                                    RPT_SEVERITY_INFO );
            }
            // HelperParseDimensions6Datum( elem );
            break;
        case ALTIUM_DIMENSION_KIND::BASELINE:
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "Ignored Baseline dimension (not yet supported)." ) ),
                                    RPT_SEVERITY_INFO );
            }
            break;
        case ALTIUM_DIMENSION_KIND::CENTER:
            HelperParseDimensions6Center( elem );
            break;
        case ALTIUM_DIMENSION_KIND::LINEAR_DIAMETER:
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "Ignored Linear dimension (not yet supported)." ) ),
                                    RPT_SEVERITY_INFO );
            }
            break;
        case ALTIUM_DIMENSION_KIND::RADIAL_DIAMETER:
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "Ignored Radial dimension (not yet supported)." ) ),
                                    RPT_SEVERITY_INFO );
            }
            break;
        default:
            if( m_reporter )
            {
                wxString msg;
                msg.Printf( _( "Ignored dimension of kind %d (not yet supported)." ), elem.kind );
                m_reporter->Report( msg, RPT_SEVERITY_INFO );
            }
            break;
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Dimensions6 stream is not fully parsed" ) );
}


void ALTIUM_PCB::ParseModelsData( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry,
                                  const std::vector<std::string>& aRootDir )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading 3D models..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    if( reader.GetRemainingBytes() == 0 )
        return;

    int      idx = 0;
    wxString invalidChars = wxFileName::GetForbiddenChars();

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AMODEL elem( reader );

        std::vector<std::string> stepPath = aRootDir;
        stepPath.emplace_back( std::to_string( idx ) );

        bool     validName = !elem.name.IsEmpty() && elem.name.IsAscii()
                                && wxString::npos == elem.name.find_first_of( invalidChars );
        wxString storageName = validName ? elem.name : wxString::Format( wxT( "model_%d" ), idx );

        idx++;

        const CFB::COMPOUND_FILE_ENTRY* stepEntry = aAltiumPcbFile.FindStream( stepPath );

        if( stepEntry == nullptr )
        {
            if( m_reporter )
            {
                wxString msg;
                msg.Printf( _( "File not found: '%s'. 3D-model not imported." ), FormatPath( stepPath ) );
                m_reporter->Report( msg, RPT_SEVERITY_ERROR );
            }

            continue;
        }

        size_t            stepSize = static_cast<size_t>( stepEntry->size );
        std::vector<char> stepContent( stepSize );

        // read file into buffer
        aAltiumPcbFile.GetCompoundFileReader().ReadFile( stepEntry, 0, stepContent.data(),
                                                         stepSize );

        m_EmbeddedModels.insert( std::make_pair(
                elem.id, ALTIUM_EMBEDDED_MODEL_DATA( storageName, elem.rotation, elem.z_offset,
                                                     std::move( stepContent ) ) ) );
    }

    // Append _<index> to duplicate filenames
    std::map<wxString, std::vector<wxString>> nameIdMap;

    for( auto& [id, data] : m_EmbeddedModels )
        nameIdMap[data.m_modelname].push_back( id );

    for( auto& [name, ids] : nameIdMap )
    {
        for( size_t i = 1; i < ids.size(); i++ )
        {
            const wxString& id = ids[i];

            auto modelTuple = m_EmbeddedModels.find( id );

            if( modelTuple == m_EmbeddedModels.end() )
                continue;

            wxString modelName = modelTuple->second.m_modelname;

            if( modelName.Contains( "." ) )
            {
                wxString ext;
                wxString baseName = modelName.BeforeLast( '.', &ext );

                modelTuple->second.m_modelname = baseName + '_' + std::to_string( i ) + '.' + ext;
            }
            else
            {
                modelTuple->second.m_modelname = modelName + '_' + std::to_string( i );
            }
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Models stream is not fully parsed" ) );
}


void ALTIUM_PCB::ParseNets6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading nets..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    wxASSERT( m_altiumToKicadNetcodes.empty() );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ANET6 elem( reader );

        NETINFO_ITEM* netInfo = new NETINFO_ITEM( m_board, elem.name, -1 );
        m_board->Add( netInfo, ADD_MODE::APPEND );

        // needs to be called after m_board->Add() as assign us the NetCode
        m_altiumToKicadNetcodes.push_back( netInfo->GetNetCode() );
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Nets6 stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParsePolygons6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                     const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading polygons..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        APOLYGON6 elem( reader );

        SHAPE_LINE_CHAIN linechain;
        HelperShapeLineChainFromAltiumVertices( linechain, elem.vertices );

        if( linechain.PointCount() < 3 )
        {
            // We have found multiple Altium files with polygon records containing nothing but two
            // coincident vertices.  These polygons do not appear when opening the file in Altium.
            // https://gitlab.com/kicad/code/kicad/-/issues/8183
            // Also, polygons with less than 3 points are not supported in KiCad.
            //
            // wxLogError( _( "Polygon has only %d point extracted from %ld vertices. At least 2 "
            //                "points are required." ),
            //             linechain.PointCount(),
            //             elem.vertices.size() );

            m_polygons.emplace_back( nullptr );
            continue;
        }

        SHAPE_POLY_SET outline( linechain );

        if( elem.hatchstyle != ALTIUM_POLYGON_HATCHSTYLE::SOLID )
        {
            // Altium "Hatched" or "None" polygon outlines have thickness, convert it to KiCad's representation.
            outline.Inflate( elem.trackwidth / 2, CORNER_STRATEGY::CHAMFER_ACUTE_CORNERS,
                             ARC_HIGH_DEF, true );
        }

        if( outline.OutlineCount() != 1 && m_reporter )
        {
            wxString msg;
            msg.Printf( _( "Polygon outline count is %d, expected 1." ), outline.OutlineCount() );

            m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        }

        if( outline.OutlineCount() == 0 )
            continue;

        std::unique_ptr<ZONE> zone = std::make_unique<ZONE>(m_board);

        // Be sure to set the zone layer before setting the net code
        // so that we know that this is a copper zone and so needs a valid net code.
        HelperSetZoneLayers( *zone, elem.layer );
        zone->SetNetCode( GetNetCode( elem.net ) );
        zone->SetPosition( elem.vertices.at( 0 ).position );
        zone->SetLocked( elem.locked );
        zone->SetAssignedPriority( elem.pourindex > 0 ? elem.pourindex : 0 );
        zone->Outline()->AddOutline( outline.Outline( 0 ) );

        if( elem.pourindex > m_highest_pour_index )
            m_highest_pour_index = elem.pourindex;

        const ARULE6* planeClearanceRule = GetRuleDefault( ALTIUM_RULE_KIND::PLANE_CLEARANCE );
        const ARULE6* zoneClearanceRule = GetRule( ALTIUM_RULE_KIND::CLEARANCE, wxT( "PolygonClearance" ) );
        int planeLayers = 0;
        int signalLayers = 0;
        int clearance = 0;

        for( PCB_LAYER_ID layer : zone->GetLayerSet() )
        {
            LAYER_T layerType = m_board->GetLayerType( layer );

            if( layerType == LT_POWER || layerType == LT_MIXED )
                planeLayers++;

            if( layerType == LT_SIGNAL || layerType == LT_MIXED )
                signalLayers++;
        }

        if( planeLayers > 0 && planeClearanceRule )
            clearance = std::max( clearance, planeClearanceRule->planeclearanceClearance );

        if( signalLayers > 0 && zoneClearanceRule )
            clearance = std::max( clearance, zoneClearanceRule->clearanceGap );

        if( clearance > 0 )
            zone->SetLocalClearance( clearance );

        const ARULE6* polygonConnectRule = GetRuleDefault( ALTIUM_RULE_KIND::POLYGON_CONNECT );

        if( polygonConnectRule != nullptr )
        {
            switch( polygonConnectRule->polygonconnectStyle )
            {
            case ALTIUM_CONNECT_STYLE::DIRECT:
                zone->SetPadConnection( ZONE_CONNECTION::FULL );
                break;

            case ALTIUM_CONNECT_STYLE::NONE:
                zone->SetPadConnection( ZONE_CONNECTION::NONE );
                break;

            default:
            case ALTIUM_CONNECT_STYLE::RELIEF:
                zone->SetPadConnection( ZONE_CONNECTION::THERMAL );
                break;
            }

            // TODO: correct variables?
            zone->SetThermalReliefSpokeWidth(
                    polygonConnectRule->polygonconnectReliefconductorwidth );
            zone->SetThermalReliefGap( polygonConnectRule->polygonconnectAirgapwidth );

            if( polygonConnectRule->polygonconnectReliefconductorwidth < zone->GetMinThickness() )
                zone->SetMinThickness( polygonConnectRule->polygonconnectReliefconductorwidth );
        }

        if( IsAltiumLayerAPlane( elem.layer ) )
        {
            // outer zone will be set to priority 0 later.
            zone->SetAssignedPriority( 1 );

            // check if this is the outer zone by simply comparing the BBOX
            const auto& outer_plane = m_outer_plane.find( elem.layer );
            if( outer_plane == m_outer_plane.end()
                    || zone->GetBoundingBox().Contains( outer_plane->second->GetBoundingBox() ) )
            {
                m_outer_plane[elem.layer] = zone.get();
            }
        }

        if( elem.hatchstyle != ALTIUM_POLYGON_HATCHSTYLE::SOLID
                && elem.hatchstyle != ALTIUM_POLYGON_HATCHSTYLE::UNKNOWN )
        {
            zone->SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
            zone->SetHatchThickness( elem.trackwidth );

            if( elem.hatchstyle == ALTIUM_POLYGON_HATCHSTYLE::NONE )
            {
                // use a small hack to get us only an outline (hopefully)
                const BOX2I& bbox = zone->GetBoundingBox();
                zone->SetHatchGap( std::max( bbox.GetHeight(), bbox.GetWidth() ) );
            }
            else
            {
                zone->SetHatchGap( elem.gridsize - elem.trackwidth );
            }

            if( elem.hatchstyle == ALTIUM_POLYGON_HATCHSTYLE::DEGREE_45 )
                zone->SetHatchOrientation( ANGLE_45 );
        }

        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     ZONE::GetDefaultHatchPitch(), true );

        m_polygons.emplace_back( zone.get() );
        m_board->Add( zone.release(), ADD_MODE::APPEND );
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Polygons6 stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParseRules6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading rules..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ARULE6 elem( reader );

        m_rules[elem.kind].emplace_back( elem );
    }

    // sort rules by priority
    for( std::pair<const ALTIUM_RULE_KIND, std::vector<ARULE6>>& val : m_rules )
    {
        std::sort( val.second.begin(), val.second.end(),
                []( const ARULE6& lhs, const ARULE6& rhs )
                {
                    return lhs.priority < rhs.priority;
                } );
    }

    const ARULE6* clearanceRule = GetRuleDefault( ALTIUM_RULE_KIND::CLEARANCE );
    const ARULE6* trackWidthRule = GetRuleDefault( ALTIUM_RULE_KIND::WIDTH );
    const ARULE6* routingViasRule = GetRuleDefault( ALTIUM_RULE_KIND::ROUTING_VIAS );
    const ARULE6* holeSizeRule = GetRuleDefault( ALTIUM_RULE_KIND::HOLE_SIZE );
    const ARULE6* holeToHoleRule = GetRuleDefault( ALTIUM_RULE_KIND::HOLE_TO_HOLE_CLEARANCE );

    if( clearanceRule )
        m_board->GetDesignSettings().m_MinClearance = clearanceRule->clearanceGap;

    if( trackWidthRule )
    {
        m_board->GetDesignSettings().m_TrackMinWidth = trackWidthRule->minLimit;
        // TODO: construct a custom rule for preferredWidth and maxLimit values
    }

    if( routingViasRule )
    {
        m_board->GetDesignSettings().m_ViasMinSize = routingViasRule->minWidth;
        m_board->GetDesignSettings().m_MinThroughDrill = routingViasRule->minHoleWidth;
    }

    if( holeSizeRule )
    {
        // TODO: construct a custom rule for minLimit / maxLimit values
    }

    if( holeToHoleRule )
        m_board->GetDesignSettings().m_HoleToHoleMin = holeToHoleRule->clearanceGap;

    const ARULE6* soldermaskRule = GetRuleDefault( ALTIUM_RULE_KIND::SOLDER_MASK_EXPANSION );
    const ARULE6* pastemaskRule = GetRuleDefault( ALTIUM_RULE_KIND::PASTE_MASK_EXPANSION );

    if( soldermaskRule )
        m_board->GetDesignSettings().m_SolderMaskExpansion = soldermaskRule->soldermaskExpansion;

    if( pastemaskRule )
        m_board->GetDesignSettings().m_SolderPasteMargin = pastemaskRule->pastemaskExpansion;

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Rules6 stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParseBoardRegionsData( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                        const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading board regions..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AREGION6 elem( reader, false );

        // TODO: implement?
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "BoardRegions stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParseShapeBasedRegions6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                              const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading polygons..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    /* TODO: use Header section of file */
    for( int primitiveIndex = 0; reader.GetRemainingBytes() >= 4; primitiveIndex++ )
    {
        checkpoint();
        AREGION6 elem( reader, true );

        if( elem.component == ALTIUM_COMPONENT_NONE
            || elem.kind == ALTIUM_REGION_KIND::BOARD_CUTOUT )
        {
            // TODO: implement all different types for footprints
            ConvertShapeBasedRegions6ToBoardItem( elem );
        }
        else
        {
            FOOTPRINT* footprint = HelperGetFootprint( elem.component );
            ConvertShapeBasedRegions6ToFootprintItem( footprint, elem, primitiveIndex );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( "ShapeBasedRegions6 stream is not fully parsed" );
}


void ALTIUM_PCB::ConvertShapeBasedRegions6ToBoardItem( const AREGION6& aElem )
{
    if( aElem.kind == ALTIUM_REGION_KIND::BOARD_CUTOUT )
    {
        HelperCreateBoardOutline( aElem.outline );
    }
    else if( aElem.kind == ALTIUM_REGION_KIND::POLYGON_CUTOUT || aElem.is_keepout )
    {
        SHAPE_LINE_CHAIN linechain;
        HelperShapeLineChainFromAltiumVertices( linechain, aElem.outline );

        if( linechain.PointCount() < 3 )
        {
            // We have found multiple Altium files with polygon records containing nothing but
            // two coincident vertices.  These polygons do not appear when opening the file in
            // Altium.  https://gitlab.com/kicad/code/kicad/-/issues/8183
            // Also, polygons with less than 3 points are not supported in KiCad.
            return;
        }

        std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( m_board );

        zone->SetIsRuleArea( true );

        if( aElem.is_keepout )
        {
            HelperSetZoneKeepoutRestrictions( *zone, aElem.keepoutrestrictions );
        }
        else if( aElem.kind == ALTIUM_REGION_KIND::POLYGON_CUTOUT )
        {
            zone->SetDoNotAllowZoneFills( true );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowPads( false );
            zone->SetDoNotAllowFootprints( false );
        }

        zone->SetPosition( aElem.outline.at( 0 ).position );
        zone->Outline()->AddOutline( linechain );

        HelperSetZoneLayers( *zone, aElem.layer );

        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     ZONE::GetDefaultHatchPitch(), true );

        m_board->Add( zone.release(), ADD_MODE::APPEND );
    }
    else if( aElem.is_teardrop )
    {
        SHAPE_LINE_CHAIN linechain;
        HelperShapeLineChainFromAltiumVertices( linechain, aElem.outline );

        if( linechain.PointCount() < 3 )
        {
            // Polygons with less than 3 points are not supported in KiCad.
            return;
        }

        std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( m_board );

        zone->SetPosition( aElem.outline.at( 0 ).position );
        zone->Outline()->AddOutline( linechain );

        HelperSetZoneLayers( *zone, aElem.layer );
        zone->SetNetCode( GetNetCode( aElem.net ) );
        zone->SetTeardropAreaType( TEARDROP_TYPE::TD_UNSPECIFIED );
        zone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER );

        SHAPE_POLY_SET fill;
        fill.Append( linechain );
        fill.Fracture();

        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            zone->SetFilledPolysList( klayer, fill );

        zone->SetIsFilled( true );
        zone->SetNeedRefill( false );

        m_board->Add( zone.release(), ADD_MODE::APPEND );
    }
    else if( aElem.kind == ALTIUM_REGION_KIND::DASHED_OUTLINE )
    {
        PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

        if( klayer == UNDEFINED_LAYER )
        {
            if( m_reporter )
            {
                wxString msg;
                msg.Printf( _( "Dashed outline found on an Altium layer (%d) with no KiCad equivalent. "
                               "It has been moved to KiCad layer Eco1_User." ), aElem.layer );
                m_reporter->Report( msg, RPT_SEVERITY_ERROR );
            }

            klayer = Eco1_User;
        }

        SHAPE_LINE_CHAIN linechain;
        HelperShapeLineChainFromAltiumVertices( linechain, aElem.outline );

        if( linechain.PointCount() < 3 )
        {
            // We have found multiple Altium files with polygon records containing nothing but
            // two coincident vertices. These polygons do not appear when opening the file in
            // Altium. https://gitlab.com/kicad/code/kicad/-/issues/8183
            // Also, polygons with less than 3 points are not supported in KiCad.
            return;
        }

        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( m_board, SHAPE_T::POLY );

        shape->SetPolyShape( linechain );
        shape->SetFilled( false );
        shape->SetLayer( klayer );
        shape->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.1 ), LINE_STYLE::DASH ) );

        m_board->Add( shape.release(), ADD_MODE::APPEND );
    }
    else if( aElem.kind == ALTIUM_REGION_KIND::COPPER )
    {
        if( aElem.polygon == ALTIUM_POLYGON_NONE )
        {
            for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
                ConvertShapeBasedRegions6ToBoardItemOnLayer( aElem, klayer );
        }
    }
    else if( aElem.kind == ALTIUM_REGION_KIND::BOARD_CUTOUT )
    {
        ConvertShapeBasedRegions6ToBoardItemOnLayer( aElem, Edge_Cuts );
    }
    else
    {
        if( m_reporter )
        {
            wxString msg;
            msg.Printf( _( "Ignored polygon shape of kind %d (not yet supported)." ), aElem.kind );
            m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        }
    }
}


void ALTIUM_PCB::ConvertShapeBasedRegions6ToFootprintItem( FOOTPRINT*      aFootprint,
                                                           const AREGION6& aElem,
                                                           const int       aPrimitiveIndex )
{
    if( aElem.kind == ALTIUM_REGION_KIND::POLYGON_CUTOUT || aElem.is_keepout )
    {
        SHAPE_LINE_CHAIN linechain;
        HelperShapeLineChainFromAltiumVertices( linechain, aElem.outline );

        if( linechain.PointCount() < 3 )
        {
            // We have found multiple Altium files with polygon records containing nothing but
            // two coincident vertices. These polygons do not appear when opening the file in
            // Altium. https://gitlab.com/kicad/code/kicad/-/issues/8183
            // Also, polygons with less than 3 points are not supported in KiCad.
            return;
        }

        std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( aFootprint );

        zone->SetIsRuleArea( true );

        if( aElem.is_keepout )
        {
            HelperSetZoneKeepoutRestrictions( *zone, aElem.keepoutrestrictions );
        }
        else if( aElem.kind == ALTIUM_REGION_KIND::POLYGON_CUTOUT )
        {
            zone->SetDoNotAllowZoneFills( true );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowPads( false );
            zone->SetDoNotAllowFootprints( false );
        }

        zone->SetPosition( aElem.outline.at( 0 ).position );
        zone->Outline()->AddOutline( linechain );

        HelperSetZoneLayers( *zone, aElem.layer );

        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     ZONE::GetDefaultHatchPitch(), true );

        aFootprint->Add( zone.release(), ADD_MODE::APPEND );
    }
    else if( aElem.kind == ALTIUM_REGION_KIND::COPPER )
    {
        if( aElem.polygon == ALTIUM_POLYGON_NONE )
        {
            for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            {
                ConvertShapeBasedRegions6ToFootprintItemOnLayer( aFootprint, aElem, klayer,
                                                                 aPrimitiveIndex );
            }
        }
    }
    else if( aElem.kind == ALTIUM_REGION_KIND::DASHED_OUTLINE
             || aElem.kind == ALTIUM_REGION_KIND::BOARD_CUTOUT )
    {
        PCB_LAYER_ID klayer = aElem.kind == ALTIUM_REGION_KIND::BOARD_CUTOUT
                                      ? Edge_Cuts
                                      : GetKicadLayer( aElem.layer );

        if( klayer == UNDEFINED_LAYER )
        {
            if( !m_footprintName.IsEmpty() )
            {
                if( m_reporter )
                {
                    wxString msg;
                    msg.Printf( _( "Loading library '%s':\n"
                                 "Footprint %s contains a dashed outline on Altium layer (%d) with "
                                 "no KiCad equivalent. It has been moved to KiCad layer Eco1_User." ),
                              m_library,
                              m_footprintName,
                              aElem.layer );
                    m_reporter->Report( msg, RPT_SEVERITY_ERROR );
                }
            }
            else
            {
                if( m_reporter )
                {
                    wxString msg;
                    msg.Printf( _( "Footprint %s contains a dashed outline on Altium layer (%d) with "
                                 "no KiCad equivalent. It has been moved to KiCad layer Eco1_User." ),
                              aFootprint->GetReference(),
                              aElem.layer );
                    m_reporter->Report( msg, RPT_SEVERITY_ERROR );
                }
            }

            klayer = Eco1_User;
        }

        SHAPE_LINE_CHAIN linechain;
        HelperShapeLineChainFromAltiumVertices( linechain, aElem.outline );

        if( linechain.PointCount() < 3 )
        {
            // We have found multiple Altium files with polygon records containing nothing but
            // two coincident vertices. These polygons do not appear when opening the file in
            // Altium. https://gitlab.com/kicad/code/kicad/-/issues/8183
            // Also, polygons with less than 3 points are not supported in KiCad.
            return;
        }

        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( aFootprint, SHAPE_T::POLY );

        shape->SetPolyShape( linechain );
        shape->SetFilled( false );
        shape->SetLayer( klayer );

        if( aElem.kind == ALTIUM_REGION_KIND::DASHED_OUTLINE )
            shape->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.1 ), LINE_STYLE::DASH ) );
        else
            shape->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.1 ), LINE_STYLE::SOLID ) );

        aFootprint->Add( shape.release(), ADD_MODE::APPEND );
    }
    else
    {
        if( !m_footprintName.IsEmpty() )
        {
            if( m_reporter )
            {
                wxString msg;
                msg.Printf( _( "Error loading library '%s':\n"
                           "Footprint %s contains polygon shape of kind %d (not yet supported)." ),
                        m_library,
                        m_footprintName,
                        aElem.kind );
                m_reporter->Report( msg, RPT_SEVERITY_ERROR );
            }
        }
        else
        {
            if( m_reporter )
            {
                wxString msg;
                msg.Printf( _( "Footprint %s contains polygon shape of kind %d (not yet supported)." ),
                        aFootprint->GetReference(),
                        aElem.kind );
                m_reporter->Report( msg, RPT_SEVERITY_ERROR );
            }
        }
    }
}


void ALTIUM_PCB::ConvertShapeBasedRegions6ToBoardItemOnLayer( const AREGION6& aElem,
                                                              PCB_LAYER_ID    aLayer )
{
    SHAPE_LINE_CHAIN linechain;
    HelperShapeLineChainFromAltiumVertices( linechain, aElem.outline );

    if( linechain.PointCount() < 3 )
    {
        // We have found multiple Altium files with polygon records containing nothing
        // but two coincident vertices. These polygons do not appear when opening the
        // file in Altium. https://gitlab.com/kicad/code/kicad/-/issues/8183
        // Also, polygons with less than 3 points are not supported in KiCad.
        return;
    }

    SHAPE_POLY_SET polySet;
    polySet.AddOutline( linechain );

    for( const std::vector<ALTIUM_VERTICE>& hole : aElem.holes )
    {
        SHAPE_LINE_CHAIN hole_linechain;
        HelperShapeLineChainFromAltiumVertices( hole_linechain, hole );

        if( hole_linechain.PointCount() < 3 )
            continue;

        polySet.AddHole( hole_linechain );
    }

    std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( m_board, SHAPE_T::POLY );

    shape->SetPolyShape( polySet );
    shape->SetFilled( true );
    shape->SetLayer( aLayer );
    shape->SetStroke( STROKE_PARAMS( 0 ) );

    if( IsCopperLayer( aLayer ) && aElem.net != ALTIUM_NET_UNCONNECTED )
    {
        shape->SetNetCode( GetNetCode( aElem.net ) );
    }

    m_board->Add( shape.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertShapeBasedRegions6ToFootprintItemOnLayer( FOOTPRINT*      aFootprint,
                                                                  const AREGION6& aElem,
                                                                  PCB_LAYER_ID    aLayer,
                                                                  const int       aPrimitiveIndex )
{
    SHAPE_LINE_CHAIN linechain;
    HelperShapeLineChainFromAltiumVertices( linechain, aElem.outline );

    if( linechain.PointCount() < 3 )
    {
        // We have found multiple Altium files with polygon records containing nothing
        // but two coincident vertices. These polygons do not appear when opening the
        // file in Altium. https://gitlab.com/kicad/code/kicad/-/issues/8183
        // Also, polygons with less than 3 points are not supported in KiCad.
        return;
    }

    SHAPE_POLY_SET polySet;
    polySet.AddOutline( linechain );

    for( const std::vector<ALTIUM_VERTICE>& hole : aElem.holes )
    {
        SHAPE_LINE_CHAIN hole_linechain;
        HelperShapeLineChainFromAltiumVertices( hole_linechain, hole );

        if( hole_linechain.PointCount() < 3 )
            continue;

        polySet.AddHole( hole_linechain );
    }

    if( aLayer == F_Cu || aLayer == B_Cu )
    {
        // TODO(JE) padstacks -- not sure what should happen here yet
        std::unique_ptr<PAD> pad = std::make_unique<PAD>( aFootprint );

        LSET padLayers;
        padLayers.set( aLayer );

        pad->SetAttribute( PAD_ATTRIB::SMD );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
        pad->SetThermalSpokeAngle( ANGLE_90 );

        int      anchorSize = 1;
        VECTOR2I anchorPos = linechain.CPoint( 0 );

        pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, { anchorSize, anchorSize } );
        pad->SetPosition( anchorPos );

        SHAPE_POLY_SET shapePolys = polySet;
        shapePolys.Move( -anchorPos );
        pad->AddPrimitivePoly( PADSTACK::ALL_LAYERS, shapePolys, 0, true );

        auto& map = m_extendedPrimitiveInformationMaps[ALTIUM_RECORD::REGION];
        auto  it = map.find( aPrimitiveIndex );

        if( it != map.end() )
        {
            const AEXTENDED_PRIMITIVE_INFORMATION& info = it->second;

            if( info.pastemaskexpansionmode == ALTIUM_MODE::MANUAL )
            {
                pad->SetLocalSolderPasteMargin( info.pastemaskexpansionmanual );
            }

            if( info.soldermaskexpansionmode == ALTIUM_MODE::MANUAL )
            {
                pad->SetLocalSolderMaskMargin( info.soldermaskexpansionmanual );
            }

            if( info.pastemaskexpansionmode != ALTIUM_MODE::NONE )
                padLayers.set( aLayer == F_Cu ? F_Paste : B_Paste );

            if( info.soldermaskexpansionmode != ALTIUM_MODE::NONE )
                padLayers.set( aLayer == F_Cu ? F_Mask : B_Mask );
        }

        pad->SetLayerSet( padLayers );

        aFootprint->Add( pad.release(), ADD_MODE::APPEND );
    }
    else
    {
        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( aFootprint, SHAPE_T::POLY );

        shape->SetPolyShape( polySet );
        shape->SetFilled( true );
        shape->SetLayer( aLayer );
        shape->SetStroke( STROKE_PARAMS( 0 ) );

        aFootprint->Add( shape.release(), ADD_MODE::APPEND );
    }
}


void ALTIUM_PCB::ParseRegions6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                    const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading zone fills..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AREGION6 elem( reader, false );

        if( elem.polygon != ALTIUM_POLYGON_NONE )
        {
            if( m_polygons.size() <= elem.polygon )
            {
                THROW_IO_ERROR(  wxString::Format( "Region stream tries to access polygon id %d "
                                                   "of %d existing polygons.",
                                                  elem.polygon,
                                                   m_polygons.size() ) );
            }

            ZONE* zone = m_polygons.at( elem.polygon );

            if( zone == nullptr )
            {
                continue; // we know the zone id, but because we do not know the layer we did not
                          // add it!
            }

            PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );

            if( klayer == UNDEFINED_LAYER )
                continue; // Just skip it for now. Users can fill it themselves.

            SHAPE_LINE_CHAIN linechain;

            for( const ALTIUM_VERTICE& vertice : elem.outline )
                linechain.Append( vertice.position );

            linechain.Append( elem.outline.at( 0 ).position );
            linechain.SetClosed( true );

            SHAPE_POLY_SET fill;
            fill.AddOutline( linechain );

            for( const std::vector<ALTIUM_VERTICE>& hole : elem.holes )
            {
                SHAPE_LINE_CHAIN hole_linechain;

                for( const ALTIUM_VERTICE& vertice : hole )
                    hole_linechain.Append( vertice.position );

                hole_linechain.Append( hole.at( 0 ).position );
                hole_linechain.SetClosed( true );
                fill.AddHole( hole_linechain );
            }

            if( zone->HasFilledPolysForLayer( klayer ) )
                fill.BooleanAdd( *zone->GetFill( klayer ) );

            fill.Fracture();

            zone->SetFilledPolysList( klayer, fill );
            zone->SetIsFilled( true );
            zone->SetNeedRefill( false );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Regions6 stream is not fully parsed" ) );
}


void ALTIUM_PCB::ParseArcs6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading arcs..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    for( int primitiveIndex = 0; reader.GetRemainingBytes() >= 4; primitiveIndex++ )
    {
        checkpoint();
        AARC6 elem( reader );

        if( elem.component == ALTIUM_COMPONENT_NONE )
        {
            ConvertArcs6ToBoardItem( elem, primitiveIndex );
        }
        else
        {
            FOOTPRINT* footprint = HelperGetFootprint( elem.component );
            ConvertArcs6ToFootprintItem( footprint, elem, primitiveIndex, true );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( "Arcs6 stream is not fully parsed" );
}


void ALTIUM_PCB::ConvertArcs6ToPcbShape( const AARC6& aElem, PCB_SHAPE* aShape )
{
    if( aElem.startangle == 0. && aElem.endangle == 360. )
    {
        aShape->SetShape( SHAPE_T::CIRCLE );

        // TODO: other variants to define circle?
        aShape->SetStart( aElem.center );
        aShape->SetEnd( aElem.center - VECTOR2I( 0, aElem.radius ) );
    }
    else
    {
        aShape->SetShape( SHAPE_T::ARC );

        EDA_ANGLE includedAngle( aElem.endangle - aElem.startangle, DEGREES_T );
        EDA_ANGLE startAngle( aElem.endangle, DEGREES_T );

        VECTOR2I startOffset = VECTOR2I( KiROUND( startAngle.Cos() * aElem.radius ),
                                         -KiROUND( startAngle.Sin() * aElem.radius ) );

        aShape->SetCenter( aElem.center );
        aShape->SetStart( aElem.center + startOffset );
        aShape->SetArcAngleAndEnd( includedAngle.Normalize(), true );
    }
}


void ALTIUM_PCB::ConvertArcs6ToBoardItem( const AARC6& aElem, const int aPrimitiveIndex )
{
    if( aElem.polygon != ALTIUM_POLYGON_NONE && aElem.polygon != ALTIUM_POLYGON_BOARD )
    {
        if( m_polygons.size() <= aElem.polygon )
        {
            THROW_IO_ERROR( wxString::Format( "Tracks stream tries to access polygon id %u "
                                              "of %zu existing polygons.",
                                              aElem.polygon, m_polygons.size() ) );
        }

        ZONE* zone = m_polygons.at( aElem.polygon );

        if( zone == nullptr )
        {
            return; // we know the zone id, but because we do not know the layer we did not
                    // add it!
        }

        PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

        if( klayer == UNDEFINED_LAYER )
            return; // Just skip it for now. Users can fill it themselves.

        if( !zone->HasFilledPolysForLayer( klayer ) )
            return;

        SHAPE_POLY_SET* fill = zone->GetFill( klayer );

        // This is not the actual board item. We can use it to create the polygon for the region
        PCB_SHAPE shape( nullptr );

        ConvertArcs6ToPcbShape( aElem, &shape );
        shape.SetStroke( STROKE_PARAMS( aElem.width, LINE_STYLE::SOLID ) );

        shape.EDA_SHAPE::TransformShapeToPolygon( *fill, 0, ARC_HIGH_DEF, ERROR_INSIDE );
        // Will be simplified and fractured later

        zone->SetIsFilled( true );
        zone->SetNeedRefill( false );

        return;
    }

    if( aElem.is_keepout || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER
        || IsAltiumLayerAPlane( aElem.layer ) )
    {
        // This is not the actual board item. We can use it to create the polygon for the region
        PCB_SHAPE shape( nullptr );

        ConvertArcs6ToPcbShape( aElem, &shape );
        shape.SetStroke( STROKE_PARAMS( aElem.width, LINE_STYLE::SOLID ) );

        HelperPcpShapeAsBoardKeepoutRegion( shape, aElem.layer, aElem.keepoutrestrictions );
    }
    else
    {
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            ConvertArcs6ToBoardItemOnLayer( aElem, klayer );
    }

    for( const auto& layerExpansionMask :
         HelperGetSolderAndPasteMaskExpansions( ALTIUM_RECORD::ARC, aPrimitiveIndex, aElem.layer ) )
    {
        int width = aElem.width + ( layerExpansionMask.second * 2 );

        if( width > 1 )
        {
            std::unique_ptr<PCB_SHAPE> arc = std::make_unique<PCB_SHAPE>( m_board );

            ConvertArcs6ToPcbShape( aElem, arc.get() );
            arc->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );
            arc->SetLayer( layerExpansionMask.first );

            m_board->Add( arc.release(), ADD_MODE::APPEND );
        }
    }
}


void ALTIUM_PCB::ConvertArcs6ToFootprintItem( FOOTPRINT* aFootprint, const AARC6& aElem,
                                              const int aPrimitiveIndex, const bool aIsBoardImport )
{
    if( aElem.polygon != ALTIUM_POLYGON_NONE )
    {
        wxFAIL_MSG( wxString::Format( "Altium: Unexpected footprint Arc with polygon id %d",
                                      aElem.polygon ) );
        return;
    }

    if( aElem.is_keepout || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER
        || IsAltiumLayerAPlane( aElem.layer ) )
    {
        // This is not the actual board item. We can use it to create the polygon for the region
        PCB_SHAPE shape( nullptr );

        ConvertArcs6ToPcbShape( aElem, &shape );
        shape.SetStroke( STROKE_PARAMS( aElem.width, LINE_STYLE::SOLID ) );

        HelperPcpShapeAsFootprintKeepoutRegion( aFootprint, shape, aElem.layer,
                                                aElem.keepoutrestrictions );
    }
    else
    {
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
        {
            if( aIsBoardImport && IsCopperLayer( klayer ) && aElem.net != ALTIUM_NET_UNCONNECTED )
            {
                // Special case: do to not lose net connections in footprints
                ConvertArcs6ToBoardItemOnLayer( aElem, klayer );
            }
            else
            {
                ConvertArcs6ToFootprintItemOnLayer( aFootprint, aElem, klayer );
            }
        }
    }

    for( const auto& layerExpansionMask :
         HelperGetSolderAndPasteMaskExpansions( ALTIUM_RECORD::ARC, aPrimitiveIndex, aElem.layer ) )
    {
        int width = aElem.width + ( layerExpansionMask.second * 2 );

        if( width > 1 )
        {
            std::unique_ptr<PCB_SHAPE> arc = std::make_unique<PCB_SHAPE>( aFootprint );

            ConvertArcs6ToPcbShape( aElem, arc.get() );
            arc->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );
            arc->SetLayer( layerExpansionMask.first );

            aFootprint->Add( arc.release(), ADD_MODE::APPEND );
        }
    }
}


void ALTIUM_PCB::ConvertArcs6ToBoardItemOnLayer( const AARC6& aElem, PCB_LAYER_ID aLayer )
{
    if( IsCopperLayer( aLayer ) && aElem.net != ALTIUM_NET_UNCONNECTED )
    {
        EDA_ANGLE includedAngle( aElem.endangle - aElem.startangle, DEGREES_T );
        EDA_ANGLE startAngle( aElem.endangle, DEGREES_T );

        includedAngle.Normalize();

        VECTOR2I startOffset = VECTOR2I( KiROUND( startAngle.Cos() * aElem.radius ),
                                         -KiROUND( startAngle.Sin() * aElem.radius ) );

        if( includedAngle.AsDegrees() >= 0.1 )
        {
            // TODO: This is not the actual board item. We use it for now to calculate the arc points. This could be improved!
            PCB_SHAPE shape( nullptr, SHAPE_T::ARC );

            shape.SetCenter( aElem.center );
            shape.SetStart( aElem.center + startOffset );
            shape.SetArcAngleAndEnd( includedAngle, true );

            // Create actual arc
            SHAPE_ARC shapeArc( shape.GetCenter(), shape.GetStart(), shape.GetArcAngle(),
                                aElem.width );
            std::unique_ptr<PCB_ARC> arc = std::make_unique<PCB_ARC>( m_board, &shapeArc );

            arc->SetWidth( aElem.width );
            arc->SetLayer( aLayer );
            arc->SetNetCode( GetNetCode( aElem.net ) );

            m_board->Add( arc.release(), ADD_MODE::APPEND );
        }
    }
    else
    {
        std::unique_ptr<PCB_SHAPE> arc = std::make_unique<PCB_SHAPE>(m_board);

        ConvertArcs6ToPcbShape( aElem, arc.get() );
        arc->SetStroke( STROKE_PARAMS( aElem.width, LINE_STYLE::SOLID ) );
        arc->SetLayer( aLayer );

        m_board->Add( arc.release(), ADD_MODE::APPEND );
    }
}


void ALTIUM_PCB::ConvertArcs6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const AARC6& aElem,
                                                     PCB_LAYER_ID aLayer )
{
    std::unique_ptr<PCB_SHAPE> arc = std::make_unique<PCB_SHAPE>( aFootprint );

    ConvertArcs6ToPcbShape( aElem, arc.get() );
    arc->SetStroke( STROKE_PARAMS( aElem.width, LINE_STYLE::SOLID ) );
    arc->SetLayer( aLayer );

    aFootprint->Add( arc.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::ParsePads6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading pads..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        APAD6 elem( reader );

        if( elem.component == ALTIUM_COMPONENT_NONE )
        {
            ConvertPads6ToBoardItem( elem );
        }
        else
        {
            FOOTPRINT* footprint = HelperGetFootprint( elem.component );
            ConvertPads6ToFootprintItem( footprint, elem );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Pads6 stream is not fully parsed" ) );
}


void ALTIUM_PCB::ConvertPads6ToBoardItem( const APAD6& aElem )
{
    // It is possible to place altium pads on non-copper layers -> we need to interpolate them using drawings!
    if( !IsAltiumLayerCopper( aElem.layer ) && !IsAltiumLayerAPlane( aElem.layer )
        && aElem.layer != ALTIUM_LAYER::MULTI_LAYER )
    {
        ConvertPads6ToBoardItemOnNonCopper( aElem );
    }
    else
    {
        // We cannot add a pad directly into the PCB
        std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( m_board );
        footprint->SetPosition( aElem.position );

        ConvertPads6ToFootprintItemOnCopper( footprint.get(), aElem );

        m_board->Add( footprint.release(), ADD_MODE::APPEND );
    }
}


void ALTIUM_PCB::ConvertVias6ToFootprintItem( FOOTPRINT* aFootprint, const AVIA6& aElem )
{
    std::unique_ptr<PAD> pad = std::make_unique<PAD>( aFootprint );

    pad->SetNumber( "" );
    pad->SetNetCode( GetNetCode( aElem.net ) );

    pad->SetPosition( aElem.position );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( aElem.diameter, aElem.diameter ) );
    pad->SetDrillSize( VECTOR2I( aElem.holesize, aElem.holesize ) );
    pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetAttribute( PAD_ATTRIB::PTH );

    // Pads are always through holes in KiCad
    pad->SetLayerSet( LSET().AllCuMask() );

    if( aElem.viamode == ALTIUM_PAD_MODE::SIMPLE )
    {
        pad->Padstack().SetMode( PADSTACK::MODE::NORMAL );
    }
    else if( aElem.viamode == ALTIUM_PAD_MODE::TOP_MIDDLE_BOTTOM )
    {
        pad->Padstack().SetMode( PADSTACK::MODE::FRONT_INNER_BACK );
        pad->Padstack().SetSize( VECTOR2I( aElem.diameter_by_layer[1], aElem.diameter_by_layer[1] ),
                                 PADSTACK::INNER_LAYERS );
    }
    else
    {
        pad->Padstack().SetMode( PADSTACK::MODE::CUSTOM );
        int altiumIdx = 0;

        for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, 32 ) )
        {
            pad->Padstack().SetSize( VECTOR2I( aElem.diameter_by_layer[altiumIdx],
                                               aElem.diameter_by_layer[altiumIdx] ), layer );
            altiumIdx++;
        }
    }

    if( aElem.is_tent_top )
    {
        pad->Padstack().FrontOuterLayers().has_solder_mask = true;
    }
    else
    {
        pad->Padstack().FrontOuterLayers().has_solder_mask = false;
        pad->SetLayerSet( pad->GetLayerSet().set( F_Mask ) );
    }

    if( aElem.is_tent_bottom )
    {
        pad->Padstack().BackOuterLayers().has_solder_mask = true;
    }
    else
    {
        pad->Padstack().BackOuterLayers().has_solder_mask = false;
        pad->SetLayerSet( pad->GetLayerSet().set( B_Mask ) );
    }

    if( aElem.is_locked )
        pad->SetLocked( true );

    if( aElem.soldermask_expansion_manual )
    {
        pad->Padstack().FrontOuterLayers().solder_mask_margin = aElem.soldermask_expansion_front;
        pad->Padstack().BackOuterLayers().solder_mask_margin = aElem.soldermask_expansion_back;
    }


    aFootprint->Add( pad.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertPads6ToFootprintItem( FOOTPRINT* aFootprint, const APAD6& aElem )
{
    // It is possible to place altium pads on non-copper layers -> we need to interpolate them using drawings!
    if( !IsAltiumLayerCopper( aElem.layer ) && !IsAltiumLayerAPlane( aElem.layer )
        && aElem.layer != ALTIUM_LAYER::MULTI_LAYER )
    {
        ConvertPads6ToFootprintItemOnNonCopper( aFootprint, aElem );
    }
    else
    {
        ConvertPads6ToFootprintItemOnCopper( aFootprint, aElem );
    }
}


void ALTIUM_PCB::ConvertPads6ToFootprintItemOnCopper( FOOTPRINT* aFootprint, const APAD6& aElem )
{
    std::unique_ptr<PAD> pad = std::make_unique<PAD>( aFootprint );

    pad->SetNumber( aElem.name );
    pad->SetNetCode( GetNetCode( aElem.net ) );

    pad->SetPosition( aElem.position );
    pad->SetOrientationDegrees( aElem.direction );
    pad->SetThermalSpokeAngle( ANGLE_90 );

    if( aElem.holesize == 0 )
    {
        pad->SetAttribute( PAD_ATTRIB::SMD );
    }
    else
    {
        if( aElem.layer != ALTIUM_LAYER::MULTI_LAYER )
        {
            // TODO: I assume other values are possible as well?
            if( !m_footprintName.IsEmpty() )
            {
                if( m_reporter )
                {
                    wxString msg;
                    msg.Printf( _( "Error loading library '%s':\n"
                               "Footprint %s pad %s is not marked as multilayer, but is a TH pad." ),
                            m_library,
                            m_footprintName,
                            aElem.name );
                    m_reporter->Report( msg, RPT_SEVERITY_ERROR );
                }
            }
            else
            {
                if( m_reporter )
                {
                    wxString msg;
                    msg.Printf( _( "Footprint %s pad %s is not marked as multilayer, but is a TH pad." ),
                            aFootprint->GetReference(),
                            aElem.name );
                    m_reporter->Report( msg, RPT_SEVERITY_ERROR );
                }
            }
        }

        pad->SetAttribute( aElem.plated ? PAD_ATTRIB::PTH : PAD_ATTRIB::NPTH );

        if( !aElem.sizeAndShape || aElem.sizeAndShape->holeshape == ALTIUM_PAD_HOLE_SHAPE::ROUND )
        {
            pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
            pad->SetDrillSize( VECTOR2I( aElem.holesize, aElem.holesize ) );
        }
        else
        {
            switch( aElem.sizeAndShape->holeshape )
            {
            case ALTIUM_PAD_HOLE_SHAPE::ROUND:
                wxFAIL_MSG( wxT( "Round holes are handled before the switch" ) );
                break;

            case ALTIUM_PAD_HOLE_SHAPE::SQUARE:
                if( !m_footprintName.IsEmpty() )
                {
                    if( m_reporter )
                    {
                        wxString msg;
                        msg.Printf( _( "Loading library '%s':\n"
                                     "Footprint %s pad %s has a square hole (not yet supported)." ),
                                  m_library,
                                  m_footprintName,
                                  aElem.name );
                        m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
                    }
                }
                else
                {
                    if( m_reporter )
                    {
                        wxString msg;
                        msg.Printf( _( "Footprint %s pad %s has a square hole (not yet supported)." ),
                                  aFootprint->GetReference(),
                                  aElem.name );
                        m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
                    }
                }

                pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
                pad->SetDrillSize( VECTOR2I( aElem.holesize, aElem.holesize ) ); // Workaround
                // TODO: elem.sizeAndShape->slotsize was 0 in testfile. Either use holesize in
                //  this case or rect holes have a different id
                break;

            case ALTIUM_PAD_HOLE_SHAPE::SLOT:
            {
                pad->SetDrillShape( PAD_DRILL_SHAPE::OBLONG );
                EDA_ANGLE slotRotation( aElem.sizeAndShape->slotrotation, DEGREES_T );

                slotRotation.Normalize();

                if( slotRotation.IsHorizontal() )
                {
                    pad->SetDrillSize( VECTOR2I( aElem.sizeAndShape->slotsize, aElem.holesize ) );
                }
                else if( slotRotation.IsVertical() )
                {
                    pad->SetDrillSize( VECTOR2I( aElem.holesize, aElem.sizeAndShape->slotsize ) );
                }
                else
                {
                    if( !m_footprintName.IsEmpty() )
                    {
                        if( m_reporter )
                        {
                            wxString msg;
                            msg.Printf( _( "Loading library '%s':\n"
                                         "Footprint %s pad %s has a hole-rotation of %d degrees. "
                                         "KiCad only supports 90 degree rotations." ),
                                      m_library,
                                      m_footprintName,
                                      aElem.name,
                                      KiROUND( slotRotation.AsDegrees() ) );
                            m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
                        }
                    }
                    else
                    {
                        if( m_reporter )
                        {
                            wxString msg;
                            msg.Printf( _( "Footprint %s pad %s has a hole-rotation of %d degrees. "
                                         "KiCad only supports 90 degree rotations." ),
                                      aFootprint->GetReference(),
                                      aElem.name,
                                      KiROUND( slotRotation.AsDegrees() ) );
                            m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
                        }
                    }
                }

                break;
            }

            default:
            case ALTIUM_PAD_HOLE_SHAPE::UNKNOWN:
                if( !m_footprintName.IsEmpty() )
                {
                    if( m_reporter )
                    {
                        wxString msg;
                        msg.Printf( _( "Error loading library '%s':\n"
                                   "Footprint %s pad %s uses a hole of unknown kind %d." ),
                                m_library,
                                m_footprintName,
                                aElem.name,
                                aElem.sizeAndShape->holeshape );
                        m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
                    }
                }
                else
                {
                    if( m_reporter )
                    {
                        wxString msg;
                        msg.Printf( _( "Footprint %s pad %s uses a hole of unknown kind %d." ),
                                aFootprint->GetReference(),
                                aElem.name,
                                aElem.sizeAndShape->holeshape );
                        m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
                    }
                }

                pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
                pad->SetDrillSize( VECTOR2I( aElem.holesize, aElem.holesize ) ); // Workaround
                break;
            }
        }

        if( aElem.sizeAndShape )
            pad->SetOffset( PADSTACK::ALL_LAYERS, aElem.sizeAndShape->holeoffset[0] );
    }

    PADSTACK& ps = pad->Padstack();

    auto setCopperGeometry =
        [&]( PCB_LAYER_ID aLayer, ALTIUM_PAD_SHAPE aShape, const VECTOR2I& aSize )
        {
            int altLayer = CopperLayerToOrdinal( aLayer );

            ps.SetSize( aSize, aLayer );

            switch( aShape )
            {
            case ALTIUM_PAD_SHAPE::RECT:
                ps.SetShape( PAD_SHAPE::RECTANGLE, aLayer );
                break;

            case ALTIUM_PAD_SHAPE::CIRCLE:
                if( aElem.sizeAndShape
                    && aElem.sizeAndShape->alt_shape[altLayer] == ALTIUM_PAD_SHAPE_ALT::ROUNDRECT )
                {
                    ps.SetShape( PAD_SHAPE::ROUNDRECT, aLayer ); // 100 = round, 0 = rectangular
                    double ratio = aElem.sizeAndShape->cornerradius[altLayer] / 200.;
                    ps.SetRoundRectRadiusRatio( ratio, aLayer );
                }
                else if( aElem.topsize.x == aElem.topsize.y )
                {
                    ps.SetShape( PAD_SHAPE::CIRCLE, aLayer );
                }
                else
                {
                    ps.SetShape( PAD_SHAPE::OVAL, aLayer );
                }

                break;

            case ALTIUM_PAD_SHAPE::OCTAGONAL:
                ps.SetShape( PAD_SHAPE::CHAMFERED_RECT, aLayer );
                ps.SetChamferPositions( RECT_CHAMFER_ALL, aLayer );
                ps.SetChamferRatio( 0.25, aLayer );
                break;

            case ALTIUM_PAD_SHAPE::UNKNOWN:
            default:
                if( !m_footprintName.IsEmpty() )
                {
                    if( m_reporter )
                    {
                        wxString msg;
                        msg.Printf( _( "Error loading library '%s':\n"
                                   "Footprint %s pad %s uses an unknown pad shape." ),
                                m_library,
                                m_footprintName,
                                aElem.name );
                        m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
                    }
                }
                else
                {
                    if( m_reporter )
                    {
                        wxString msg;
                        msg.Printf( _( "Footprint %s pad %s uses an unknown pad shape." ),
                                aFootprint->GetReference(),
                                aElem.name );
                        m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
                    }
                }
                break;
            }
        };

    switch( aElem.padmode )
    {
    case ALTIUM_PAD_MODE::SIMPLE:
        ps.SetMode( PADSTACK::MODE::NORMAL );
        setCopperGeometry( PADSTACK::ALL_LAYERS, aElem.topshape, aElem.topsize );
        break;

    case ALTIUM_PAD_MODE::TOP_MIDDLE_BOTTOM:
        ps.SetMode( PADSTACK::MODE::FRONT_INNER_BACK );
        setCopperGeometry( F_Cu, aElem.topshape, aElem.topsize );
        setCopperGeometry( PADSTACK::INNER_LAYERS, aElem.midshape, aElem.midsize );
        setCopperGeometry( B_Cu, aElem.botshape, aElem.botsize );
        break;

    case ALTIUM_PAD_MODE::FULL_STACK:
        ps.SetMode( PADSTACK::MODE::CUSTOM );

        setCopperGeometry( F_Cu, aElem.topshape, aElem.topsize );
        setCopperGeometry( B_Cu, aElem.botshape, aElem.botsize );
        setCopperGeometry( In1_Cu, aElem.midshape, aElem.midsize );

        if( aElem.sizeAndShape )
        {
            size_t i = 0;

            LSET intLayers = aFootprint->BoardLayerSet();
            intLayers &= LSET::InternalCuMask();
            intLayers.set( In1_Cu, false ); // Already handled above

            for( PCB_LAYER_ID layer : intLayers )
            {
                setCopperGeometry( layer, aElem.sizeAndShape->inner_shape[i],
                                   VECTOR2I( aElem.sizeAndShape->inner_size[i].x,
                                             aElem.sizeAndShape->inner_size[i].y ) );
                i++;
            }
        }

        break;
    }

    switch( aElem.layer )
    {
    case ALTIUM_LAYER::TOP_LAYER:
        pad->SetLayer( F_Cu );
        pad->SetLayerSet( PAD::SMDMask() );
        break;

    case ALTIUM_LAYER::BOTTOM_LAYER:
        pad->SetLayer( B_Cu );
        pad->SetLayerSet( PAD::SMDMask().FlipStandardLayers() );
        break;

    case ALTIUM_LAYER::MULTI_LAYER:
        pad->SetLayerSet( aElem.plated ? PAD::PTHMask() : PAD::UnplatedHoleMask() );
        break;

    default:
        PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );
        pad->SetLayer( klayer );
        pad->SetLayerSet( LSET( { klayer } ) );
        break;
    }

    if( aElem.pastemaskexpansionmode == ALTIUM_MODE::MANUAL )
        pad->SetLocalSolderPasteMargin( aElem.pastemaskexpansionmanual );

    if( aElem.soldermaskexpansionmode == ALTIUM_MODE::MANUAL )
        pad->SetLocalSolderMaskMargin( aElem.soldermaskexpansionmanual );

    if( aElem.is_tent_top )
        pad->SetLayerSet( pad->GetLayerSet().reset( F_Mask ) );

    if( aElem.is_tent_bottom )
        pad->SetLayerSet( pad->GetLayerSet().reset( B_Mask ) );

    pad->SetPadToDieLength( aElem.pad_to_die_length );
    pad->SetPadToDieDelay( aElem.pad_to_die_delay );

    aFootprint->Add( pad.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertPads6ToBoardItemOnNonCopper( const APAD6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        if( m_reporter )
        {
            wxString msg;
            msg.Printf( _( "Non-copper pad %s found on an Altium layer (%d) with no KiCad "
                         "equivalent. It has been moved to KiCad layer Eco1_User." ),
                      aElem.name, aElem.layer );
            m_reporter->Report( msg, RPT_SEVERITY_INFO );
        }

        klayer = Eco1_User;
    }

    std::unique_ptr<PCB_SHAPE> pad = std::make_unique<PCB_SHAPE>( m_board );

    HelperParsePad6NonCopper( aElem, klayer, pad.get() );

    m_board->Add( pad.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertPads6ToFootprintItemOnNonCopper( FOOTPRINT* aFootprint, const APAD6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        if( !m_footprintName.IsEmpty() )
        {
            if( m_reporter )
            {
                wxString msg;
                msg.Printf( _( "Loading library '%s':\n"
                             "Footprint %s non-copper pad %s found on an Altium layer (%d) with no "
                             "KiCad equivalent. It has been moved to KiCad layer Eco1_User." ),
                          m_library,
                          m_footprintName,
                          aElem.name,
                          aElem.layer );
                m_reporter->Report( msg, RPT_SEVERITY_INFO );
            }
        }
        else
        {
            if( m_reporter )
            {
                wxString msg;
                msg.Printf( _( "Footprint %s non-copper pad %s found on an Altium layer (%d) with no "
                             "KiCad equivalent. It has been moved to KiCad layer Eco1_User." ),
                          aFootprint->GetReference(),
                          aElem.name,
                          aElem.layer );
                m_reporter->Report( msg, RPT_SEVERITY_INFO );
            }
        }

        klayer = Eco1_User;
    }

    std::unique_ptr<PCB_SHAPE> pad = std::make_unique<PCB_SHAPE>( aFootprint );

    HelperParsePad6NonCopper( aElem, klayer, pad.get() );

    aFootprint->Add( pad.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::HelperParsePad6NonCopper( const APAD6& aElem, PCB_LAYER_ID aLayer,
                                           PCB_SHAPE* aShape )
{
    if( aElem.net != ALTIUM_NET_UNCONNECTED )
    {
        if( m_reporter )
        {
            wxString msg;
            msg.Printf( _( "Non-copper pad %s is connected to a net, which is not supported." ),
                    aElem.name );
            m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
        }
    }

    if( aElem.holesize != 0 )
    {
        if( m_reporter )
        {
            wxString msg;
            msg.Printf( _( "Non-copper pad %s has a hole, which is not supported." ), aElem.name );
            m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
        }
    }

    if( aElem.padmode != ALTIUM_PAD_MODE::SIMPLE )
    {
        if( m_reporter )
        {
            wxString msg;
            msg.Printf( _( "Non-copper pad %s has a complex pad stack (not yet supported)." ),
                        aElem.name );
            m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
        }
    }

    switch( aElem.topshape )
    {
    case ALTIUM_PAD_SHAPE::RECT:
    {
        // filled rect
        aShape->SetShape( SHAPE_T::POLY );
        aShape->SetFilled( true );
        aShape->SetLayer( aLayer );
        aShape->SetStroke( STROKE_PARAMS( 0 ) );

        aShape->SetPolyPoints(
                { aElem.position + VECTOR2I( aElem.topsize.x / 2, aElem.topsize.y / 2 ),
                  aElem.position + VECTOR2I( aElem.topsize.x / 2, -aElem.topsize.y / 2 ),
                  aElem.position + VECTOR2I( -aElem.topsize.x / 2, -aElem.topsize.y / 2 ),
                  aElem.position + VECTOR2I( -aElem.topsize.x / 2, aElem.topsize.y / 2 ) } );

        if( aElem.direction != 0 )
            aShape->Rotate( aElem.position, EDA_ANGLE( aElem.direction, DEGREES_T ) );
    }
    break;

    case ALTIUM_PAD_SHAPE::CIRCLE:
        if( aElem.sizeAndShape
                && aElem.sizeAndShape->alt_shape[0] == ALTIUM_PAD_SHAPE_ALT::ROUNDRECT )
        {
            // filled roundrect
            int cornerradius = aElem.sizeAndShape->cornerradius[0];
            int offset = ( std::min( aElem.topsize.x, aElem.topsize.y ) * cornerradius ) / 200;

            aShape->SetLayer( aLayer );
            aShape->SetStroke( STROKE_PARAMS( offset * 2, LINE_STYLE::SOLID ) );

            if( cornerradius < 100 )
            {
                int offsetX = aElem.topsize.x / 2 - offset;
                int offsetY = aElem.topsize.y / 2 - offset;

                VECTOR2I p11 = aElem.position + VECTOR2I( offsetX, offsetY );
                VECTOR2I p12 = aElem.position + VECTOR2I( offsetX, -offsetY );
                VECTOR2I p22 = aElem.position + VECTOR2I( -offsetX, -offsetY );
                VECTOR2I p21 = aElem.position + VECTOR2I( -offsetX, offsetY );

                aShape->SetShape( SHAPE_T::POLY );
                aShape->SetFilled( true );
                aShape->SetPolyPoints( { p11, p12, p22, p21 } );
            }
            else if( aElem.topsize.x == aElem.topsize.y )
            {
                // circle
                aShape->SetShape( SHAPE_T::CIRCLE );
                aShape->SetFilled( true );
                aShape->SetStart( aElem.position );
                aShape->SetEnd( aElem.position - VECTOR2I( 0, aElem.topsize.x / 4 ) );
                aShape->SetStroke( STROKE_PARAMS( aElem.topsize.x / 2, LINE_STYLE::SOLID ) );
            }
            else if( aElem.topsize.x < aElem.topsize.y )
            {
                // short vertical line
                aShape->SetShape( SHAPE_T::SEGMENT );
                VECTOR2I pointOffset( 0, ( aElem.topsize.y / 2 - aElem.topsize.x / 2 ) );
                aShape->SetStart( aElem.position + pointOffset );
                aShape->SetEnd( aElem.position - pointOffset );
            }
            else
            {
                // short horizontal line
                aShape->SetShape( SHAPE_T::SEGMENT );
                VECTOR2I pointOffset( ( aElem.topsize.x / 2 - aElem.topsize.y / 2 ), 0 );
                aShape->SetStart( aElem.position + pointOffset );
                aShape->SetEnd( aElem.position - pointOffset );
            }

            if( aElem.direction != 0 )
                aShape->Rotate( aElem.position, EDA_ANGLE( aElem.direction, DEGREES_T ) );
        }
        else if( aElem.topsize.x == aElem.topsize.y )
        {
            // filled circle
            aShape->SetShape( SHAPE_T::CIRCLE );
            aShape->SetFilled( true );
            aShape->SetLayer( aLayer );
            aShape->SetStart( aElem.position );
            aShape->SetEnd( aElem.position - VECTOR2I( 0, aElem.topsize.x / 4 ) );
            aShape->SetStroke( STROKE_PARAMS( aElem.topsize.x / 2, LINE_STYLE::SOLID ) );
        }
        else
        {
            // short line
            aShape->SetShape( SHAPE_T::SEGMENT );
            aShape->SetLayer( aLayer );
            aShape->SetStroke( STROKE_PARAMS( std::min( aElem.topsize.x, aElem.topsize.y ),
                                              LINE_STYLE::SOLID ) );

            if( aElem.topsize.x < aElem.topsize.y )
            {
                VECTOR2I offset( 0, ( aElem.topsize.y / 2 - aElem.topsize.x / 2 ) );
                aShape->SetStart( aElem.position + offset );
                aShape->SetEnd( aElem.position - offset );
            }
            else
            {
                VECTOR2I offset( ( aElem.topsize.x / 2 - aElem.topsize.y / 2 ), 0 );
                aShape->SetStart( aElem.position + offset );
                aShape->SetEnd( aElem.position - offset );
            }

            if( aElem.direction != 0 )
                aShape->Rotate( aElem.position, EDA_ANGLE( aElem.direction, DEGREES_T ) );
        }
        break;

    case ALTIUM_PAD_SHAPE::OCTAGONAL:
    {
        // filled octagon
        aShape->SetShape( SHAPE_T::POLY );
        aShape->SetFilled( true );
        aShape->SetLayer( aLayer );
        aShape->SetStroke( STROKE_PARAMS( 0 ) );

        VECTOR2I p11 = aElem.position + VECTOR2I( aElem.topsize.x / 2, aElem.topsize.y / 2 );
        VECTOR2I p12 = aElem.position + VECTOR2I( aElem.topsize.x / 2, -aElem.topsize.y / 2 );
        VECTOR2I p22 = aElem.position + VECTOR2I( -aElem.topsize.x / 2, -aElem.topsize.y / 2 );
        VECTOR2I p21 = aElem.position + VECTOR2I( -aElem.topsize.x / 2, aElem.topsize.y / 2 );

        int     chamfer = std::min( aElem.topsize.x, aElem.topsize.y ) / 4;
        VECTOR2I chamferX( chamfer, 0 );
        VECTOR2I chamferY( 0, chamfer );

        aShape->SetPolyPoints( { p11 - chamferX, p11 - chamferY, p12 + chamferY, p12 - chamferX,
                                 p22 + chamferX, p22 + chamferY, p21 - chamferY, p21 + chamferX } );

        if( aElem.direction != 0. )
            aShape->Rotate( aElem.position, EDA_ANGLE( aElem.direction, DEGREES_T ) );
    }
        break;

    case ALTIUM_PAD_SHAPE::UNKNOWN:
    default:
        if( m_reporter )
        {
            wxString msg;
            msg.Printf( _( "Non-copper pad %s uses an unknown pad shape." ), aElem.name );
            m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
        }

        break;
    }
}


void ALTIUM_PCB::ParseVias6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading vias..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AVIA6 elem( reader );

        std::unique_ptr<PCB_VIA> via = std::make_unique<PCB_VIA>( m_board );

        via->SetPosition( elem.position );
        via->SetDrill( elem.holesize );
        via->SetNetCode( GetNetCode( elem.net ) );
        via->SetLocked( elem.is_locked );

        bool start_layer_outside = elem.layer_start == ALTIUM_LAYER::TOP_LAYER
                                   || elem.layer_start == ALTIUM_LAYER::BOTTOM_LAYER;
        bool end_layer_outside = elem.layer_end == ALTIUM_LAYER::TOP_LAYER
                                 || elem.layer_end == ALTIUM_LAYER::BOTTOM_LAYER;

        if( start_layer_outside && end_layer_outside )
        {
            via->SetViaType( VIATYPE::THROUGH );
        }
        else if( ( !start_layer_outside ) && ( !end_layer_outside ) )
        {
            via->SetViaType( VIATYPE::BURIED );
        }
        else
        {
            via->SetViaType( VIATYPE::BLIND );
        }

        // TODO: Altium has a specific flag for microvias, independent of start/end layer
#if 0
        if( something )
            via->SetViaType( VIATYPE::MICROVIA );
#endif

        PCB_LAYER_ID start_klayer = GetKicadLayer( elem.layer_start );
        PCB_LAYER_ID end_klayer   = GetKicadLayer( elem.layer_end );

        if( !IsCopperLayer( start_klayer ) || !IsCopperLayer( end_klayer ) )
        {
            if( m_reporter )
            {
                wxString msg;
                msg.Printf( _( "Via from layer %d to %d uses a non-copper layer, which is not "
                               "supported." ),
                        elem.layer_start,
                        elem.layer_end );
                m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
            }

            continue; // just assume through-hole instead.
        }

        // we need VIATYPE set!
        via->SetLayerPair( start_klayer, end_klayer );

        switch( elem.viamode )
        {
        default:
        case ALTIUM_PAD_MODE::SIMPLE:
            via->SetWidth( PADSTACK::ALL_LAYERS, elem.diameter );
            break;

        case ALTIUM_PAD_MODE::TOP_MIDDLE_BOTTOM:
            via->Padstack().SetMode( PADSTACK::MODE::FRONT_INNER_BACK );
            via->SetWidth( F_Cu, elem.diameter_by_layer[0] );
            via->SetWidth( PADSTACK::INNER_LAYERS, elem.diameter_by_layer[1] );
            via->SetWidth( B_Cu, elem.diameter_by_layer[31] );
            break;

        case ALTIUM_PAD_MODE::FULL_STACK:
        {
            via->Padstack().SetMode( PADSTACK::MODE::CUSTOM );

            for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, MAX_CU_LAYERS ) )
            {
                int altiumLayer = CopperLayerToOrdinal( layer );
                wxCHECK2_MSG( altiumLayer < 32, break,
                              "Altium importer expects 32 or fewer copper layers" );

                via->SetWidth( layer, elem.diameter_by_layer[altiumLayer] );
            }

            break;
        }
        }

        if( elem.soldermask_expansion_manual )
        {
            via->SetFrontTentingMode( elem.is_tent_top ? TENTING_MODE::TENTED
                                                       : TENTING_MODE::NOT_TENTED );
            via->SetBackTentingMode( elem.is_tent_bottom ? TENTING_MODE::TENTED
                                                         : TENTING_MODE::NOT_TENTED );
        }

        m_board->Add( via.release(), ADD_MODE::APPEND );
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Vias6 stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParseTracks6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                   const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading tracks..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    for( int primitiveIndex = 0; reader.GetRemainingBytes() >= 4; primitiveIndex++ )
    {
        checkpoint();
        ATRACK6 elem( reader );

        if( elem.component == ALTIUM_COMPONENT_NONE )
        {
            ConvertTracks6ToBoardItem( elem, primitiveIndex );
        }
        else
        {
            FOOTPRINT* footprint = HelperGetFootprint( elem.component );
            ConvertTracks6ToFootprintItem( footprint, elem, primitiveIndex, true );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( "Tracks6 stream is not fully parsed" );
}


void ALTIUM_PCB::ConvertTracks6ToBoardItem( const ATRACK6& aElem, const int aPrimitiveIndex )
{
    if( aElem.polygon != ALTIUM_POLYGON_NONE && aElem.polygon != ALTIUM_POLYGON_BOARD )
    {
        if( m_polygons.size() <= aElem.polygon )
        {
            // Can happen when reading old Altium files: just skip this item
            if( m_reporter )
            {
                wxString msg;
                msg.Printf( wxT( "ATRACK6 stream tries to access polygon id %u "
                                 "of %u existing polygons; skipping it" ),
                            static_cast<unsigned>( aElem.polygon ),
                            static_cast<unsigned>( m_polygons.size() ) );
                m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
            }

            return;
        }

        ZONE* zone = m_polygons.at( aElem.polygon );

        if( zone == nullptr )
        {
            return; // we know the zone id, but because we do not know the layer we did not
                    // add it!
        }

        PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

        if( klayer == UNDEFINED_LAYER )
            return; // Just skip it for now. Users can fill it themselves.

        if( !zone->HasFilledPolysForLayer( klayer ) )
            return;

        SHAPE_POLY_SET* fill = zone->GetFill( klayer );

        PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
        shape.SetStart( aElem.start );
        shape.SetEnd( aElem.end );
        shape.SetStroke( STROKE_PARAMS( aElem.width, LINE_STYLE::SOLID ) );

        shape.EDA_SHAPE::TransformShapeToPolygon( *fill, 0, ARC_HIGH_DEF, ERROR_INSIDE );
        // Will be simplified and fractured later

        zone->SetIsFilled( true );
        zone->SetNeedRefill( false );

        return;
    }

    if( aElem.is_keepout || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER
        || IsAltiumLayerAPlane( aElem.layer ) )
    {
        // This is not the actual board item. We can use it to create the polygon for the region
        PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
        shape.SetStart( aElem.start );
        shape.SetEnd( aElem.end );
        shape.SetStroke( STROKE_PARAMS( aElem.width, LINE_STYLE::SOLID ) );

        HelperPcpShapeAsBoardKeepoutRegion( shape, aElem.layer, aElem.keepoutrestrictions );
    }
    else
    {
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            ConvertTracks6ToBoardItemOnLayer( aElem, klayer );
    }

    for( const auto& layerExpansionMask : HelperGetSolderAndPasteMaskExpansions(
                 ALTIUM_RECORD::TRACK, aPrimitiveIndex, aElem.layer ) )
    {
        int width = aElem.width + ( layerExpansionMask.second * 2 );
        if( width > 1 )
        {
            std::unique_ptr<PCB_SHAPE> seg = std::make_unique<PCB_SHAPE>( m_board, SHAPE_T::SEGMENT );

            seg->SetStart( aElem.start );
            seg->SetEnd( aElem.end );
            seg->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );
            seg->SetLayer( layerExpansionMask.first );

            m_board->Add( seg.release(), ADD_MODE::APPEND );
        }
    }
}


void ALTIUM_PCB::ConvertTracks6ToFootprintItem( FOOTPRINT* aFootprint, const ATRACK6& aElem,
                                                const int  aPrimitiveIndex,
                                                const bool aIsBoardImport )
{
    if( aElem.polygon != ALTIUM_POLYGON_NONE )
    {
        wxFAIL_MSG( wxString::Format( "Altium: Unexpected footprint Track with polygon id %u",
                                      (unsigned)aElem.polygon ) );
        return;
    }

    if( aElem.is_keepout || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER
        || IsAltiumLayerAPlane( aElem.layer ) )
    {
        // This is not the actual board item. We can use it to create the polygon for the region
        PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
        shape.SetStart( aElem.start );
        shape.SetEnd( aElem.end );
        shape.SetStroke( STROKE_PARAMS( aElem.width, LINE_STYLE::SOLID ) );

        HelperPcpShapeAsFootprintKeepoutRegion( aFootprint, shape, aElem.layer,
                                                aElem.keepoutrestrictions );
    }
    else
    {
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
        {
            if( aIsBoardImport && IsCopperLayer( klayer ) && aElem.net != ALTIUM_NET_UNCONNECTED )
            {
                // Special case: do to not lose net connections in footprints
                ConvertTracks6ToBoardItemOnLayer( aElem, klayer );
            }
            else
            {
                ConvertTracks6ToFootprintItemOnLayer( aFootprint, aElem, klayer );
            }
        }
    }

    for( const auto& layerExpansionMask : HelperGetSolderAndPasteMaskExpansions(
                 ALTIUM_RECORD::TRACK, aPrimitiveIndex, aElem.layer ) )
    {
        int width = aElem.width + ( layerExpansionMask.second * 2 );
        if( width > 1 )
        {
            std::unique_ptr<PCB_SHAPE> seg = std::make_unique<PCB_SHAPE>( aFootprint, SHAPE_T::SEGMENT );

            seg->SetStart( aElem.start );
            seg->SetEnd( aElem.end );
            seg->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );
            seg->SetLayer( layerExpansionMask.first );

            aFootprint->Add( seg.release(), ADD_MODE::APPEND );
        }
    }
}


void ALTIUM_PCB::ConvertTracks6ToBoardItemOnLayer( const ATRACK6& aElem, PCB_LAYER_ID aLayer )
{
    if( IsCopperLayer( aLayer ) && aElem.net != ALTIUM_NET_UNCONNECTED )
    {
        std::unique_ptr<PCB_TRACK> track = std::make_unique<PCB_TRACK>( m_board );

        track->SetStart( aElem.start );
        track->SetEnd( aElem.end );
        track->SetWidth( aElem.width );
        track->SetLayer( aLayer );
        track->SetNetCode( GetNetCode( aElem.net ) );

        m_board->Add( track.release(), ADD_MODE::APPEND );
    }
    else
    {
        std::unique_ptr<PCB_SHAPE> seg = std::make_unique<PCB_SHAPE>( m_board, SHAPE_T::SEGMENT );

        seg->SetStart( aElem.start );
        seg->SetEnd( aElem.end );
        seg->SetStroke( STROKE_PARAMS( aElem.width, LINE_STYLE::SOLID ) );
        seg->SetLayer( aLayer );

        m_board->Add( seg.release(), ADD_MODE::APPEND );
    }
}


void ALTIUM_PCB::ConvertTracks6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const ATRACK6& aElem,
                                                       PCB_LAYER_ID aLayer )
{
    std::unique_ptr<PCB_SHAPE> seg = std::make_unique<PCB_SHAPE>( aFootprint, SHAPE_T::SEGMENT );

    seg->SetStart( aElem.start );
    seg->SetEnd( aElem.end );
    seg->SetStroke( STROKE_PARAMS( aElem.width, LINE_STYLE::SOLID ) );
    seg->SetLayer( aLayer );

    aFootprint->Add( seg.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::ParseWideStrings6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                        const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading unicode strings..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    m_unicodeStrings = reader.ReadWideStringTable();

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "WideStrings6 stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParseTexts6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading text..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ATEXT6 elem( reader, m_unicodeStrings );

        if( elem.component == ALTIUM_COMPONENT_NONE )
        {
            ConvertTexts6ToBoardItem( elem );
        }
        else
        {
            FOOTPRINT* footprint = HelperGetFootprint( elem.component );
            ConvertTexts6ToFootprintItem( footprint, elem );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Texts6 stream is not fully parsed" ) );
}


void ALTIUM_PCB::ConvertTexts6ToBoardItem( const ATEXT6& aElem )
{
    if( aElem.fonttype == ALTIUM_TEXT_TYPE::BARCODE )
    {
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            ConvertBarcodes6ToBoardItemOnLayer( aElem, klayer );
        return;
    }

    for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
        ConvertTexts6ToBoardItemOnLayer( aElem, klayer );
}


void ALTIUM_PCB::ConvertTexts6ToFootprintItem( FOOTPRINT* aFootprint, const ATEXT6& aElem )
{
    if( aElem.fonttype == ALTIUM_TEXT_TYPE::BARCODE )
    {
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            ConvertBarcodes6ToFootprintItemOnLayer( aFootprint, aElem, klayer );
        return;
    }

    for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
        ConvertTexts6ToFootprintItemOnLayer( aFootprint, aElem, klayer );
}


void ALTIUM_PCB::ConvertTexts6ToBoardItemOnLayer( const ATEXT6& aElem, PCB_LAYER_ID aLayer )
{
    std::unique_ptr<PCB_TEXTBOX> pcbTextbox = std::make_unique<PCB_TEXTBOX>( m_board );
    std::unique_ptr<PCB_TEXT>    pcbText = std::make_unique<PCB_TEXT>( m_board );

    bool isTextbox = aElem.isFrame && !aElem.isInverted; // Textbox knockout is not supported

    static const std::map<wxString, wxString> variableMap = {
        { "LAYER_NAME", "LAYER" },
        { "PRINT_DATE", "CURRENT_DATE"},
    };

    wxString    kicadText = AltiumPcbSpecialStringsToKiCadStrings( aElem.text, variableMap );
    BOARD_ITEM* item = pcbText.get();
    EDA_TEXT*   text = pcbText.get();
    int         margin = aElem.isOffsetBorder ? aElem.text_offset_width : aElem.margin_border_width;

    if( isTextbox )
    {
        item = pcbTextbox.get();
        text = pcbTextbox.get();

        ConvertTexts6ToEdaTextSettings( aElem, *text );
        HelperSetTextboxAlignmentAndPos( aElem, pcbTextbox.get() );
    }
    else
    {
        ConvertTexts6ToEdaTextSettings( aElem, *text );
        HelperSetTextAlignmentAndPos( aElem, text );
    }

    text->SetText( kicadText );
    item->SetLayer( aLayer );
    item->SetIsKnockout( aElem.isInverted );

    if( isTextbox )
        m_board->Add( pcbTextbox.release(), ADD_MODE::APPEND );
    else
        m_board->Add( pcbText.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertTexts6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const ATEXT6& aElem,
                                                      PCB_LAYER_ID aLayer )
{
    std::unique_ptr<PCB_TEXTBOX> fpTextbox = std::make_unique<PCB_TEXTBOX>( aFootprint );
    std::unique_ptr<PCB_TEXT> fpText = std::make_unique<PCB_TEXT>( aFootprint );

    BOARD_ITEM* item = fpText.get();
    EDA_TEXT*   text = fpText.get();
    PCB_FIELD*  field = nullptr;

    bool isTextbox = aElem.isFrame && !aElem.isInverted; // Textbox knockout is not supported
    bool toAdd = false;

    if( aElem.isDesignator )
    {
        item = &aFootprint->Reference(); // TODO: handle multiple layers
        text = &aFootprint->Reference();
        field = &aFootprint->Reference();
    }
    else if( aElem.isComment )
    {
        item = &aFootprint->Value(); // TODO: handle multiple layers
        text = &aFootprint->Value();
        field = &aFootprint->Value();
    }
    else
    {
        item = fpText.get();
        text = fpText.get();
        toAdd = true;
    }

    static const std::map<wxString, wxString> variableMap = {
        { "DESIGNATOR", "REFERENCE" },
        { "COMMENT",    "VALUE" },
        { "VALUE",      "ALTIUM_VALUE" },
        { "LAYER_NAME", "LAYER" },
        { "PRINT_DATE", "CURRENT_DATE"},
    };

    if( isTextbox )
    {
        item = fpTextbox.get();
        text = fpTextbox.get();

        ConvertTexts6ToEdaTextSettings( aElem, *text );
        HelperSetTextboxAlignmentAndPos( aElem, fpTextbox.get() );
    }
    else
    {
        ConvertTexts6ToEdaTextSettings( aElem, *text );
        HelperSetTextAlignmentAndPos( aElem, text );
    }

    wxString kicadText = AltiumPcbSpecialStringsToKiCadStrings( aElem.text, variableMap );

    text->SetText( kicadText );
    text->SetKeepUpright( false );
    item->SetLayer( aLayer );
    item->SetIsKnockout( aElem.isInverted );

    if( toAdd )
    {
        if( isTextbox )
            aFootprint->Add( fpTextbox.release(), ADD_MODE::APPEND );
        else
            aFootprint->Add( fpText.release(), ADD_MODE::APPEND );
    }
}


void ALTIUM_PCB::ConvertBarcodes6ToBoardItemOnLayer( const ATEXT6& aElem, PCB_LAYER_ID aLayer )
{
    std::unique_ptr<PCB_BARCODE> pcbBarcode = std::make_unique<PCB_BARCODE>( m_board );

    pcbBarcode->SetLayer( aLayer );
    pcbBarcode->SetPosition( aElem.position );
    pcbBarcode->SetWidth( aElem.textbox_rect_width );
    pcbBarcode->SetHeight( aElem.textbox_rect_height );
    pcbBarcode->SetMargin( aElem.barcode_margin );
    pcbBarcode->SetText( aElem.text );

    switch( aElem.barcode_type )
    {
    case ALTIUM_BARCODE_TYPE::CODE39: pcbBarcode->SetKind( BARCODE_T::CODE_39 ); break;
    case ALTIUM_BARCODE_TYPE::CODE128: pcbBarcode->SetKind( BARCODE_T::CODE_128 ); break;
    default: pcbBarcode->SetKind( BARCODE_T::CODE_39 ); break;
    }

    pcbBarcode->SetIsKnockout( aElem.barcode_inverted );
    pcbBarcode->AssembleBarcode();

    m_board->Add( pcbBarcode.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertBarcodes6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const ATEXT6& aElem,
                                                         PCB_LAYER_ID aLayer )
{
    std::unique_ptr<PCB_BARCODE> fpBarcode = std::make_unique<PCB_BARCODE>( aFootprint );

    fpBarcode->SetLayer( aLayer );
    fpBarcode->SetPosition( aElem.position );
    fpBarcode->SetWidth( aElem.textbox_rect_width );
    fpBarcode->SetHeight( aElem.textbox_rect_height );
    fpBarcode->SetMargin( aElem.barcode_margin );
    fpBarcode->SetText( aElem.text );

    switch( aElem.barcode_type )
    {
    case ALTIUM_BARCODE_TYPE::CODE39: fpBarcode->SetKind( BARCODE_T::CODE_39 ); break;
    case ALTIUM_BARCODE_TYPE::CODE128: fpBarcode->SetKind( BARCODE_T::CODE_128 ); break;
    default: fpBarcode->SetKind( BARCODE_T::CODE_39 ); break;
    }

    fpBarcode->SetIsKnockout( aElem.barcode_inverted );
    fpBarcode->AssembleBarcode();

    aFootprint->Add( fpBarcode.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::HelperSetTextboxAlignmentAndPos( const ATEXT6& aElem, PCB_TEXTBOX* aTextbox )
{
    int margin = aElem.isOffsetBorder ? aElem.text_offset_width : aElem.margin_border_width;

    // Altium textboxes do not have borders
    aTextbox->SetBorderEnabled( false );

    // Calculate position
    VECTOR2I kposition = aElem.position;

    if( aElem.isMirrored )
        kposition.x -= aElem.textbox_rect_width;

    kposition.y -= aElem.textbox_rect_height;

#if 0
    // Compensate for KiCad's textbox margin
    int charWidth = aTextbox->GetTextWidth();
    int charHeight = aTextbox->GetTextHeight();

    VECTOR2I kicadMargin;

    if( !aTextbox->GetFont() || aTextbox->GetFont()->IsStroke() )
        kicadMargin = VECTOR2I( charWidth * 0.933, charHeight * 0.67 );
    else
        kicadMargin = VECTOR2I( charWidth * 0.808, charHeight * 0.844 );

    aTextbox->SetEnd( VECTOR2I( aElem.textbox_rect_width, aElem.textbox_rect_height )
                        + kicadMargin * 2 - margin * 2 );

    kposition = kposition - kicadMargin + margin;
#else
    aTextbox->SetMarginBottom( margin );
    aTextbox->SetMarginLeft( margin );
    aTextbox->SetMarginRight( margin );
    aTextbox->SetMarginTop( margin );

    aTextbox->SetEnd( VECTOR2I( aElem.textbox_rect_width, aElem.textbox_rect_height ) );
#endif

    RotatePoint( kposition, aElem.position, EDA_ANGLE( aElem.rotation, DEGREES_T ) );

    aTextbox->SetPosition( kposition );

    ALTIUM_TEXT_POSITION justification = aElem.isJustificationValid
                                                 ? aElem.textbox_rect_justification
                                                 : ALTIUM_TEXT_POSITION::LEFT_BOTTOM;

    switch( justification )
    {
    case ALTIUM_TEXT_POSITION::LEFT_TOP:
    case ALTIUM_TEXT_POSITION::LEFT_CENTER:
    case ALTIUM_TEXT_POSITION::LEFT_BOTTOM:
        aTextbox->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aTextbox->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        break;
    case ALTIUM_TEXT_POSITION::CENTER_TOP:
    case ALTIUM_TEXT_POSITION::CENTER_CENTER:
    case ALTIUM_TEXT_POSITION::CENTER_BOTTOM:
        aTextbox->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        aTextbox->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        break;
    case ALTIUM_TEXT_POSITION::RIGHT_TOP:
    case ALTIUM_TEXT_POSITION::RIGHT_CENTER:
    case ALTIUM_TEXT_POSITION::RIGHT_BOTTOM:
        aTextbox->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        aTextbox->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        break;
    default:
        if( m_reporter )
        {
            wxString msg;
            msg.Printf( _( "Unknown textbox justification %d, aText %s" ), justification,
                        aElem.text );
            m_reporter->Report( msg, RPT_SEVERITY_DEBUG );
        }

        aTextbox->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aTextbox->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        break;
    }

    aTextbox->SetTextAngle( EDA_ANGLE( aElem.rotation, DEGREES_T ) );
}


void ALTIUM_PCB::HelperSetTextAlignmentAndPos( const ATEXT6& aElem, EDA_TEXT* aText )
{
    VECTOR2I kposition = aElem.position;

    int margin = aElem.isOffsetBorder ? aElem.text_offset_width : aElem.margin_border_width;
    int rectWidth = aElem.textbox_rect_width - margin * 2;
    int rectHeight = aElem.height;

    if( aElem.isMirrored )
        rectWidth = -rectWidth;

    ALTIUM_TEXT_POSITION justification = aElem.isJustificationValid
                                                 ? aElem.textbox_rect_justification
                                                 : ALTIUM_TEXT_POSITION::LEFT_BOTTOM;

    switch( justification )
    {
    case ALTIUM_TEXT_POSITION::LEFT_TOP:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

        kposition.y -= rectHeight;
        break;
    case ALTIUM_TEXT_POSITION::LEFT_CENTER:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

        kposition.y -= rectHeight / 2;
        break;
    case ALTIUM_TEXT_POSITION::LEFT_BOTTOM:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        break;
    case ALTIUM_TEXT_POSITION::CENTER_TOP:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

        kposition.x += rectWidth / 2;
        kposition.y -= rectHeight;
        break;
    case ALTIUM_TEXT_POSITION::CENTER_CENTER:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

        kposition.x += rectWidth / 2;
        kposition.y -= rectHeight / 2;
        break;
    case ALTIUM_TEXT_POSITION::CENTER_BOTTOM:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );

        kposition.x += rectWidth / 2;
        break;
    case ALTIUM_TEXT_POSITION::RIGHT_TOP:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

        kposition.x += rectWidth;
        kposition.y -= rectHeight;
        break;
    case ALTIUM_TEXT_POSITION::RIGHT_CENTER:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

        kposition.x += rectWidth;
        kposition.y -= rectHeight / 2;
        break;
    case ALTIUM_TEXT_POSITION::RIGHT_BOTTOM:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );

        kposition.x += rectWidth;
        break;
    default:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        break;
    }

    int charWidth = aText->GetTextWidth();
    int charHeight = aText->GetTextHeight();

    // Correct for KiCad's baseline offset.
    // Text height and font must be set correctly before calling.
    if( !aText->GetFont() || aText->GetFont()->IsStroke() )
    {
        switch( aText->GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP: kposition.y -= charHeight * 0.0407; break;
        case GR_TEXT_V_ALIGN_CENTER: kposition.y += charHeight * 0.0355; break;
        case GR_TEXT_V_ALIGN_BOTTOM: kposition.y += charHeight * 0.1225; break;
        default: break;
        }
    }
    else
    {
        switch( aText->GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP: kposition.y -= charWidth * 0.016; break;
        case GR_TEXT_V_ALIGN_CENTER: kposition.y += charWidth * 0.085; break;
        case GR_TEXT_V_ALIGN_BOTTOM: kposition.y += charWidth * 0.17; break;
        default: break;
        }
    }

    RotatePoint( kposition, aElem.position, EDA_ANGLE( aElem.rotation, DEGREES_T ) );

    aText->SetTextPos( kposition );
    aText->SetTextAngle( EDA_ANGLE( aElem.rotation, DEGREES_T ) );
}


void ALTIUM_PCB::ConvertTexts6ToEdaTextSettings( const ATEXT6& aElem, EDA_TEXT& aEdaText )
{
    aEdaText.SetTextSize( VECTOR2I( aElem.height, aElem.height ) );

    if( aElem.fonttype == ALTIUM_TEXT_TYPE::TRUETYPE )
    {
        KIFONT::FONT* font = KIFONT::FONT::GetFont( aElem.fontname, aElem.isBold, aElem.isItalic );
        aEdaText.SetFont( font );

        if( font->IsOutline() )
        {
            // TODO: why is this required? Somehow, truetype size is calculated differently
            if( font->GetName().Contains( wxS( "Arial" ) ) )
                aEdaText.SetTextSize( VECTOR2I( aElem.height * 0.63, aElem.height * 0.63 ) );
            else
                aEdaText.SetTextSize( VECTOR2I( aElem.height * 0.5, aElem.height * 0.5 ) );
        }
    }

    aEdaText.SetTextThickness( aElem.strokewidth );
    aEdaText.SetBoldFlag( aElem.isBold );
    aEdaText.SetItalic( aElem.isItalic );
    aEdaText.SetMirrored( aElem.isMirrored );
}


void ALTIUM_PCB::ParseFills6Data( const ALTIUM_PCB_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading rectangles..." ) );

    ALTIUM_BINARY_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AFILL6 elem( reader );

        if( elem.component == ALTIUM_COMPONENT_NONE )
        {
            ConvertFills6ToBoardItem( elem );
        }
        else
        {
            FOOTPRINT* footprint = HelperGetFootprint( elem.component );
            ConvertFills6ToFootprintItem( footprint, elem, true );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( "Fills6 stream is not fully parsed" );
}


void ALTIUM_PCB::ConvertFills6ToBoardItem( const AFILL6& aElem )
{
    if( aElem.is_keepout || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER )
    {
        // This is not the actual board item. We can use it to create the polygon for the region
        PCB_SHAPE shape( nullptr, SHAPE_T::RECTANGLE );

        shape.SetStart( aElem.pos1 );
        shape.SetEnd( aElem.pos2 );
        shape.SetFilled( true );
        shape.SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );

        if( aElem.rotation != 0. )
        {
            VECTOR2I center( aElem.pos1.x / 2 + aElem.pos2.x / 2,
                             aElem.pos1.y / 2 + aElem.pos2.y / 2 );
            shape.Rotate( center, EDA_ANGLE( aElem.rotation, DEGREES_T ) );
        }

        HelperPcpShapeAsBoardKeepoutRegion( shape, aElem.layer, aElem.keepoutrestrictions );
    }
    else
    {
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            ConvertFills6ToBoardItemOnLayer( aElem, klayer );
    }
}


void ALTIUM_PCB::ConvertFills6ToFootprintItem( FOOTPRINT* aFootprint, const AFILL6& aElem,
                                               const bool aIsBoardImport )
{
    if( aElem.is_keepout
        || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER ) // TODO: what about plane layers?
    {
        // This is not the actual board item. We can use it to create the polygon for the region
        PCB_SHAPE shape( nullptr, SHAPE_T::RECTANGLE );

        shape.SetStart( aElem.pos1 );
        shape.SetEnd( aElem.pos2 );
        shape.SetFilled( true );
        shape.SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );

        if( aElem.rotation != 0. )
        {
            VECTOR2I center( aElem.pos1.x / 2 + aElem.pos2.x / 2,
                             aElem.pos1.y / 2 + aElem.pos2.y / 2 );
            shape.Rotate( center, EDA_ANGLE( aElem.rotation, DEGREES_T ) );
        }

        HelperPcpShapeAsFootprintKeepoutRegion( aFootprint, shape, aElem.layer,
                                                aElem.keepoutrestrictions );
    }
    else if( aIsBoardImport && IsAltiumLayerCopper( aElem.layer )
             && aElem.net != ALTIUM_NET_UNCONNECTED )
    {
        // Special case: do to not lose net connections in footprints
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            ConvertFills6ToBoardItemOnLayer( aElem, klayer );
    }
    else
    {
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            ConvertFills6ToFootprintItemOnLayer( aFootprint, aElem, klayer );
    }
}


void ALTIUM_PCB::ConvertFills6ToBoardItemOnLayer( const AFILL6& aElem, PCB_LAYER_ID aLayer )
{
    std::unique_ptr<PCB_SHAPE> fill = std::make_unique<PCB_SHAPE>( m_board, SHAPE_T::RECTANGLE );

    fill->SetFilled( true );
    fill->SetLayer( aLayer );
    fill->SetStroke( STROKE_PARAMS( 0 ) );

    fill->SetStart( aElem.pos1 );
    fill->SetEnd( aElem.pos2 );

    if( IsCopperLayer( aLayer ) && aElem.net != ALTIUM_NET_UNCONNECTED )
    {
        fill->SetNetCode( GetNetCode( aElem.net ) );
    }

    if( aElem.rotation != 0. )
    {
        // TODO: Do we need SHAPE_T::POLY for non 90° rotations?
        VECTOR2I center( aElem.pos1.x / 2 + aElem.pos2.x / 2,
                         aElem.pos1.y / 2 + aElem.pos2.y / 2 );
        fill->Rotate( center, EDA_ANGLE( aElem.rotation, DEGREES_T ) );
    }

    m_board->Add( fill.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertFills6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const AFILL6& aElem,
                                                      PCB_LAYER_ID aLayer )
{
    if( aLayer == F_Cu || aLayer == B_Cu )
    {
        std::unique_ptr<PAD> pad = std::make_unique<PAD>( aFootprint );

        LSET padLayers;
        padLayers.set( aLayer );

        pad->SetAttribute( PAD_ATTRIB::SMD );
        EDA_ANGLE rotation( aElem.rotation, DEGREES_T );

        // Handle rotation multiples of 90 degrees
        if( rotation.IsCardinal() )
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );

            int width = std::abs( aElem.pos2.x - aElem.pos1.x );
            int height = std::abs( aElem.pos2.y - aElem.pos1.y );

            // Swap width and height for 90 or 270 degree rotations
            if( rotation.IsCardinal90() )
                std::swap( width, height );

            pad->SetSize( PADSTACK::ALL_LAYERS, { width, height } );
            pad->SetPosition( aElem.pos1 / 2 + aElem.pos2 / 2 );
        }
        else
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );

            int      anchorSize = std::min( std::abs( aElem.pos2.x - aElem.pos1.x ),
                                            std::abs( aElem.pos2.y - aElem.pos1.y ) );
            VECTOR2I anchorPos = aElem.pos1;

            pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
            pad->SetSize( PADSTACK::ALL_LAYERS, { anchorSize, anchorSize } );
            pad->SetPosition( anchorPos );

            SHAPE_POLY_SET shapePolys;
            shapePolys.NewOutline();
            shapePolys.Append( aElem.pos1.x - anchorPos.x, aElem.pos1.y - anchorPos.y );
            shapePolys.Append( aElem.pos2.x - anchorPos.x, aElem.pos1.y - anchorPos.y );
            shapePolys.Append( aElem.pos2.x - anchorPos.x, aElem.pos2.y - anchorPos.y );
            shapePolys.Append( aElem.pos1.x - anchorPos.x, aElem.pos2.y - anchorPos.y );
            shapePolys.Outline( 0 ).SetClosed( true );

            VECTOR2I center( aElem.pos1.x / 2 + aElem.pos2.x / 2 - anchorPos.x,
                             aElem.pos1.y / 2 + aElem.pos2.y / 2 - anchorPos.y );
            shapePolys.Rotate( EDA_ANGLE( aElem.rotation, DEGREES_T ), center );
            pad->AddPrimitivePoly( F_Cu, shapePolys, 0, true );
        }

        pad->SetThermalSpokeAngle( ANGLE_90 );
        pad->SetLayerSet( padLayers );

        aFootprint->Add( pad.release(), ADD_MODE::APPEND );
    }
    else
    {
        std::unique_ptr<PCB_SHAPE> fill =
                std::make_unique<PCB_SHAPE>( aFootprint, SHAPE_T::RECTANGLE );

        fill->SetFilled( true );
        fill->SetLayer( aLayer );
        fill->SetStroke( STROKE_PARAMS( 0 ) );

        fill->SetStart( aElem.pos1 );
        fill->SetEnd( aElem.pos2 );

        if( aElem.rotation != 0. )
        {
            VECTOR2I center( aElem.pos1.x / 2 + aElem.pos2.x / 2,
                             aElem.pos1.y / 2 + aElem.pos2.y / 2 );
            fill->Rotate( center, EDA_ANGLE( aElem.rotation, DEGREES_T ) );
        }

        aFootprint->Add( fill.release(), ADD_MODE::APPEND );
    }
}


void ALTIUM_PCB::HelperSetZoneLayers( ZONE& aZone, const ALTIUM_LAYER aAltiumLayer )
{
    LSET layerSet;

    for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aAltiumLayer ) )
        layerSet.set( klayer );

    aZone.SetLayerSet( layerSet );
}


void ALTIUM_PCB::HelperSetZoneKeepoutRestrictions( ZONE& aZone, const uint8_t aKeepoutRestrictions )
{
    bool keepoutRestrictionVia = ( aKeepoutRestrictions & 0x01 ) != 0;
    bool keepoutRestrictionTrack = ( aKeepoutRestrictions & 0x02 ) != 0;
    bool keepoutRestrictionCopper = ( aKeepoutRestrictions & 0x04 ) != 0;
    bool keepoutRestrictionSMDPad = ( aKeepoutRestrictions & 0x08 ) != 0;
    bool keepoutRestrictionTHPad = ( aKeepoutRestrictions & 0x10 ) != 0;

    aZone.SetDoNotAllowVias( keepoutRestrictionVia );
    aZone.SetDoNotAllowTracks( keepoutRestrictionTrack );
    aZone.SetDoNotAllowZoneFills( keepoutRestrictionCopper );
    aZone.SetDoNotAllowPads( keepoutRestrictionSMDPad && keepoutRestrictionTHPad );
    aZone.SetDoNotAllowFootprints( false );
}


void ALTIUM_PCB::HelperPcpShapeAsBoardKeepoutRegion( const PCB_SHAPE&   aShape,
                                                     const ALTIUM_LAYER aAltiumLayer,
                                                     const uint8_t      aKeepoutRestrictions )
{
    std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( m_board );

    zone->SetIsRuleArea( true );

    HelperSetZoneLayers( *zone, aAltiumLayer );
    HelperSetZoneKeepoutRestrictions( *zone, aKeepoutRestrictions );

    aShape.EDA_SHAPE::TransformShapeToPolygon( *zone->Outline(), 0, ARC_HIGH_DEF, ERROR_INSIDE );

    zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                 ZONE::GetDefaultHatchPitch(), true );

    m_board->Add( zone.release(), ADD_MODE::APPEND );
}


void ALTIUM_PCB::HelperPcpShapeAsFootprintKeepoutRegion( FOOTPRINT*         aFootprint,
                                                         const PCB_SHAPE&   aShape,
                                                         const ALTIUM_LAYER aAltiumLayer,
                                                         const uint8_t      aKeepoutRestrictions )
{
    std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( aFootprint );

    zone->SetIsRuleArea( true );

    HelperSetZoneLayers( *zone, aAltiumLayer );
    HelperSetZoneKeepoutRestrictions( *zone, aKeepoutRestrictions );

    aShape.EDA_SHAPE::TransformShapeToPolygon( *zone->Outline(), 0, ARC_HIGH_DEF, ERROR_INSIDE );

    zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                 ZONE::GetDefaultHatchPitch(), true );

    // TODO: zone->SetLocalCoord(); missing?
    aFootprint->Add( zone.release(), ADD_MODE::APPEND );
}


std::vector<std::pair<PCB_LAYER_ID, int>> ALTIUM_PCB::HelperGetSolderAndPasteMaskExpansions(
        const ALTIUM_RECORD aType, const int aPrimitiveIndex, const ALTIUM_LAYER aAltiumLayer )
{
    if( m_extendedPrimitiveInformationMaps.count( aType ) == 0 )
        return {}; // there is nothing to parse

    auto elems =
            m_extendedPrimitiveInformationMaps[ALTIUM_RECORD::TRACK].equal_range( aPrimitiveIndex );

    if( elems.first == elems.second )
        return {}; // there is nothing to parse

    std::vector<std::pair<PCB_LAYER_ID, int>> layerExpansionPairs;

    for( auto it = elems.first; it != elems.second; ++it )
    {
        const AEXTENDED_PRIMITIVE_INFORMATION& pInf = it->second;

        if( pInf.type == AEXTENDED_PRIMITIVE_INFORMATION_TYPE::MASK )
        {
            if( pInf.soldermaskexpansionmode == ALTIUM_MODE::MANUAL
                || pInf.soldermaskexpansionmode == ALTIUM_MODE::RULE )
            {
                // TODO: what layers can lead to solder or paste mask usage? E.g. KEEP_OUT_LAYER and other top/bottom layers
                if( aAltiumLayer == ALTIUM_LAYER::TOP_LAYER
                    || aAltiumLayer == ALTIUM_LAYER::MULTI_LAYER )
                {
                    layerExpansionPairs.emplace_back( F_Mask, pInf.soldermaskexpansionmanual );
                }

                if( aAltiumLayer == ALTIUM_LAYER::BOTTOM_LAYER
                    || aAltiumLayer == ALTIUM_LAYER::MULTI_LAYER )
                {
                    layerExpansionPairs.emplace_back( B_Mask, pInf.soldermaskexpansionmanual );
                }
            }
            if( pInf.pastemaskexpansionmode == ALTIUM_MODE::MANUAL
                || pInf.pastemaskexpansionmode == ALTIUM_MODE::RULE )
            {
                if( aAltiumLayer == ALTIUM_LAYER::TOP_LAYER
                    || aAltiumLayer == ALTIUM_LAYER::MULTI_LAYER )
                {
                    layerExpansionPairs.emplace_back( F_Paste, pInf.pastemaskexpansionmanual );
                }

                if( aAltiumLayer == ALTIUM_LAYER::BOTTOM_LAYER
                    || aAltiumLayer == ALTIUM_LAYER::MULTI_LAYER )
                {
                    layerExpansionPairs.emplace_back( B_Paste, pInf.pastemaskexpansionmanual );
                }
            }
        }
    }

    return layerExpansionPairs;
}
