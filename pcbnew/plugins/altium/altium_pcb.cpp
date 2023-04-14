/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "plugins/altium/altium_parser.h"
#include <plugins/altium/altium_parser_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <pcb_dimension.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <string_utils.h>
#include <zone.h>

#include <board_stackup_manager/stackup_predefined_prms.h>

#include <compoundfilereader.h>
#include <convert_basic_shapes_to_polygon.h>
#include <project.h>
#include <trigo.h>
#include <utf.h>
#include <wx/docview.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include <progress_reporter.h>


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
        THROW_IO_ERROR( wxString::Format( wxT( "Component creator tries to access component id %d "
                                               "of %d existing components" ),
                                          aComponent, m_components.size() ) );
    }

    return m_components.at( aComponent );
}

PCB_SHAPE* ALTIUM_PCB::HelperCreateAndAddShape( uint16_t aComponent )
{
    if( aComponent == ALTIUM_COMPONENT_NONE )
    {
        PCB_SHAPE* shape = new PCB_SHAPE( m_board );
        m_board->Add( shape, ADD_MODE::APPEND );
        return shape;
    }
    else
    {
        if( m_components.size() <= aComponent )
        {
            THROW_IO_ERROR( wxString::Format( wxT( "Component creator tries to access component "
                                                   "id %d of %d existing components" ),
                                              aComponent,
                                              m_components.size() ) );
        }

        FOOTPRINT* footprint = m_components.at( aComponent );
        PCB_SHAPE* fpShape = new PCB_SHAPE( footprint );

        footprint->Add( fpShape, ADD_MODE::APPEND );
        return fpShape;
    }
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
            VECTOR2I arcStartOffset = VECTOR2I( KiROUND( std::cos( startradiant ) * vertex.radius ),
                                             -KiROUND( std::sin( startradiant ) * vertex.radius ) );

            VECTOR2I arcEndOffset = VECTOR2I( KiROUND( std::cos( endradiant ) * vertex.radius ),
                                           -KiROUND( std::sin( endradiant ) * vertex.radius ) );

            VECTOR2I arcStart = vertex.center + arcStartOffset;
            VECTOR2I arcEnd   = vertex.center + arcEndOffset;

            if( GetLineLength( arcStart, vertex.position )
                    < GetLineLength( arcEnd, vertex.position ) )
            {
                aLine.Append( SHAPE_ARC( vertex.center, arcStart, -angle ) );
            }
            else
            {
                aLine.Append( SHAPE_ARC( vertex.center, arcEnd, angle ) );
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
    case ALTIUM_LAYER::MECHANICAL_10:     return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_11:     return Eco2_User; //Eco1 is used for unknown elements
    case ALTIUM_LAYER::MECHANICAL_12:     return F_Fab;
    case ALTIUM_LAYER::MECHANICAL_13:     return B_Fab; // Don't use courtyard layers for other purposes
    case ALTIUM_LAYER::MECHANICAL_14:     return UNDEFINED_LAYER;
    case ALTIUM_LAYER::MECHANICAL_15:     return UNDEFINED_LAYER;
    case ALTIUM_LAYER::MECHANICAL_16:     return UNDEFINED_LAYER;

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

    if( aAltiumLayer == ALTIUM_LAYER::MULTI_LAYER )
    {
        std::vector<PCB_LAYER_ID> layers;
        layers.reserve( MAX_CU_LAYERS ); // TODO: only use Cu layers which are on the board
        for( PCB_LAYER_ID layer = PCB_LAYER_ID::F_Cu; layer <= PCB_LAYER_ID::B_Cu;
             layer = static_cast<PCB_LAYER_ID>( static_cast<int>( layer ) + 1 ) )
        {
            layers.emplace_back( layer );
        }

        return layers;
    }

    PCB_LAYER_ID klayer = GetKicadLayer( aAltiumLayer );

    if( klayer == UNDEFINED_LAYER )
    {
        wxLogWarning( _( "Altium layer (%d) has no KiCad equivalent. It has been moved to KiCad "
                         "layer Eco1_User." ),
                      aAltiumLayer );
        klayer = Eco1_User;
    }

    return { klayer };
}


ALTIUM_PCB::ALTIUM_PCB( BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter )
{
    m_board              = aBoard;
    m_progressReporter = aProgressReporter;
    m_doneCount = 0;
    m_lastProgressCount = 0;
    m_totalCount = 0;
    m_highest_pour_index = 0;
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
                THROW_IO_ERROR( _( "Open cancelled by user." ) );

            m_lastProgressCount = m_doneCount;
        }
    }
}

void ALTIUM_PCB::Parse( const ALTIUM_COMPOUND_FILE&                  altiumPcbFile,
                        const std::map<ALTIUM_PCB_DIR, std::string>& aFileMapping )
{
    // this vector simply declares in which order which functions to call.
    const std::vector<std::tuple<bool, ALTIUM_PCB_DIR, PARSE_FUNCTION_POINTER_fp>> parserOrder = {
        { true, ALTIUM_PCB_DIR::FILE_HEADER,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseFileHeader( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::BOARD6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseBoard6Data( aFile, fileHeader );
          } },
        { false, ALTIUM_PCB_DIR::EXTENDPRIMITIVEINFORMATION,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseExtendedPrimitiveInformationData( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::COMPONENTS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseComponents6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::MODELS,
          [this, aFileMapping]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              std::vector<std::string> dir{ aFileMapping.at( ALTIUM_PCB_DIR::MODELS ) };
              this->ParseModelsData( aFile, fileHeader, dir );
          } },
        { true, ALTIUM_PCB_DIR::COMPONENTBODIES6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseComponentsBodies6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::NETS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseNets6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::CLASSES6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseClasses6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::RULES6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseRules6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::DIMENSIONS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseDimensions6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::POLYGONS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParsePolygons6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::ARCS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseArcs6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::PADS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParsePads6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::VIAS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseVias6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::TRACKS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseTracks6Data( aFile, fileHeader );
          } },
        { false, ALTIUM_PCB_DIR::WIDESTRINGS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseWideStrings6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::TEXTS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseTexts6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::FILLS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseFills6Data( aFile, fileHeader );
          } },
        { false, ALTIUM_PCB_DIR::BOARDREGIONS,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseBoardRegionsData( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::SHAPEBASEDREGIONS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
          {
              this->ParseShapeBasedRegions6Data( aFile, fileHeader );
          } },
        { true, ALTIUM_PCB_DIR::REGIONS6,
          [this]( const ALTIUM_COMPOUND_FILE& aFile, auto fileHeader )
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

            ALTIUM_PARSER reader( altiumPcbFile, file );
            uint32_t      numOfRecords = reader.Read<uint32_t>();

            if( reader.HasParsingError() )
            {
                wxLogError( _( "'%s' was not parsed correctly." ), FormatPath( mappedFile ) );
                continue;
            }

            m_totalCount += numOfRecords;

            if( reader.GetRemainingBytes() != 0 )
            {
                wxLogError( _( "'%s' was not fully parsed." ), FormatPath( mappedFile ) );
                continue;
            }
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
            fp( altiumPcbFile, file );
        else if( isRequired )
            wxLogError( _( "File not found: '%s'." ), FormatPath( mappedFile ) );
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


FOOTPRINT* ALTIUM_PCB::ParseFootprint( const ALTIUM_COMPOUND_FILE& altiumLibFile,
                                       const wxString&             aFootprintName )
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

    const std::vector<std::string>  streamName{ aFootprintName.ToStdString(), "Data" };
    const CFB::COMPOUND_FILE_ENTRY* footprintData = altiumLibFile.FindStream( streamName );

    if( footprintData == nullptr )
    {
        THROW_IO_ERROR( wxString::Format( _( "File not found: '%s'." ),
                                          FormatPath( streamName ) ) );
    }

    ALTIUM_PARSER parser( altiumLibFile, footprintData );

    parser.ReadAndSetSubrecordLength();
    wxString footprintName = parser.ReadWxString();
    parser.SkipSubrecord();

    LIB_ID fpID = AltiumToKiCadLibID( "", footprintName ); // TODO: library name
    footprint->SetFPID( fpID );

    const std::vector<std::string>  parametersStreamName{ aFootprintName.ToStdString(),
                                                         "Parameters" };
    const CFB::COMPOUND_FILE_ENTRY* parametersData =
            altiumLibFile.FindStream( parametersStreamName );

    if( parametersData != nullptr )
    {
        ALTIUM_PARSER                parametersReader( altiumLibFile, parametersData );
        std::map<wxString, wxString> parameterProperties = parametersReader.ReadProperties();
        wxString                     description = ALTIUM_PARSER::ReadString( parameterProperties,
                                                                              wxT( "DESCRIPTION" ),
                                                                              wxT( "" ) );
        footprint->SetDescription( description );
    }
    else
    {
        wxLogError( _( "File not found: '%s'." ), FormatPath( parametersStreamName ) );
        footprint->SetDescription( wxT( "" ) );
    }

    const std::vector<std::string> extendedPrimitiveInformationStreamName{
        aFootprintName.ToStdString(), "ExtendedPrimitiveInformation", "Data"
    };
    const CFB::COMPOUND_FILE_ENTRY* extendedPrimitiveInformationData =
            altiumLibFile.FindStream( extendedPrimitiveInformationStreamName );

    if( extendedPrimitiveInformationData != nullptr )
        ParseExtendedPrimitiveInformationData( altiumLibFile, extendedPrimitiveInformationData );

    footprint->SetReference( wxT( "REF**" ) );
    footprint->SetValue( footprintName );
    footprint->Reference().SetVisible( true ); // TODO: extract visibility information
    footprint->Value().SetVisible( true );

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
            // TODO: implement
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
            ConvertShapeBasedRegions6ToFootprintItem( footprint.get(), region );
            break;
        }
        case ALTIUM_RECORD::MODEL:
        {
            ACOMPONENTBODY6 componentBody( parser );
            // Won't be supported for now, as we would need to extract the model
            break;
        }
        default:
            THROW_IO_ERROR( wxString::Format( _( "Record of unknown type: '%d'." ), recordtype ) );
        }
    }

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

void ALTIUM_PCB::ParseFileHeader( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    reader.ReadAndSetSubrecordLength();
    wxString header = reader.ReadWxString();

    //std::cout << "HEADER: " << header << std::endl;  // tells me: PCB 5.0 Binary File

    //reader.SkipSubrecord();

    // TODO: does not seem to work all the time at the moment
    //if( reader.GetRemainingBytes() != 0 )
    //    THROW_IO_ERROR( "FileHeader stream is not fully parsed" );
}

void ALTIUM_PCB::ParseExtendedPrimitiveInformationData( const ALTIUM_COMPOUND_FILE& aAltiumPcbFile,
                                                        const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading extended primitive information data..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

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

void ALTIUM_PCB::ParseBoard6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading board data..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

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

    auto curLayer = static_cast<int>( F_Cu );

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

        m_layermap.insert( { static_cast<ALTIUM_LAYER>( altiumLayerId ),
                static_cast<PCB_LAYER_ID>( curLayer++ ) } );

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
}

void ALTIUM_PCB::HelperCreateBoardOutline( const std::vector<ALTIUM_VERTICE>& aVertices )
{
    SHAPE_LINE_CHAIN lineChain;
    HelperShapeLineChainFromAltiumVertices( lineChain, aVertices );

    STROKE_PARAMS stroke( m_board->GetDesignSettings().GetLineThickness( Edge_Cuts ),
                          PLOT_DASH_TYPE::SOLID );

    for( int i = 0; i <= lineChain.PointCount() && i != -1; i = lineChain.NextShape( i ) )
    {
        if( lineChain.IsArcStart( i ) )
        {
            const SHAPE_ARC& currentArc = lineChain.Arc( lineChain.ArcIndex( i ) );
            int              nextShape = lineChain.NextShape( i );
            bool             isLastShape = nextShape < 0;

            PCB_SHAPE* shape = new PCB_SHAPE( m_board, SHAPE_T::ARC );
            m_board->Add( shape, ADD_MODE::APPEND );

            shape->SetStroke( stroke );
            shape->SetLayer( Edge_Cuts );
            shape->SetArcGeometry( currentArc.GetP0(), currentArc.GetArcMid(), currentArc.GetP1() );
        }
        else
        {
            const SEG& seg = lineChain.Segment( i );

            PCB_SHAPE* shape = new PCB_SHAPE( m_board, SHAPE_T::SEGMENT );
            m_board->Add( shape, ADD_MODE::APPEND );

            shape->SetStroke( stroke );
            shape->SetLayer( Edge_Cuts );
            shape->SetStart( seg.A );
            shape->SetEnd( seg.B );
        }
    }
}

void ALTIUM_PCB::ParseClasses6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                    const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading netclasses..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ACLASS6 elem( reader );

        if( elem.kind == ALTIUM_CLASS_KIND::NET_CLASS )
        {
            std::shared_ptr<NETCLASS> nc = std::make_shared<NETCLASS>( elem.name );

            for( const wxString& name : elem.names )
            {
                m_board->GetDesignSettings().m_NetSettings->m_NetClassPatternAssignments.push_back(
                        {
                            std::make_unique<EDA_COMBINED_MATCHER>( name, CTX_NETCLASS ),
                            nc->GetName()
                        } );
            }

            if( m_board->GetDesignSettings().m_NetSettings->m_NetClasses.count( nc->GetName() ) )
            {
                // Name conflict, this is likely a bad board file.
                // unique_ptr will delete nc on this code path
                THROW_IO_ERROR( wxString::Format( _( "Duplicate netclass name '%s'." ), elem.name ) );
            }
            else
            {
                m_board->GetDesignSettings().m_NetSettings->m_NetClasses[ nc->GetName() ] = nc;
            }
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Classes6 stream is not fully parsed" ) );

    m_board->m_LegacyNetclassesLoaded = true;
}

void ALTIUM_PCB::ParseComponents6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                       const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading components..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    uint16_t componentId = 0;

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ACOMPONENT6 elem( reader );

        FOOTPRINT* footprint = new FOOTPRINT( m_board );
        m_board->Add( footprint, ADD_MODE::APPEND );
        m_components.emplace_back( footprint );

        LIB_ID fpID = AltiumToKiCadLibID( elem.sourcefootprintlibrary, elem.pattern );

        footprint->SetFPID( fpID );

        footprint->SetPosition( elem.position );
        footprint->SetOrientationDegrees( elem.rotation );

        // KiCad netlisting requires parts to have non-digit + digit annotation.
        // If the reference begins with a number, we prepend 'UNK' (unknown) for the source designator
        wxString reference = elem.sourcedesignator;

        if( reference.find_first_not_of( "0123456789" ) == wxString::npos )
            reference.Prepend( wxT( "UNK" ) );

        footprint->SetReference( reference );

        footprint->SetLocked( elem.locked );
        footprint->Reference().SetVisible( elem.nameon );
        footprint->Value().SetVisible( elem.commenton );
        footprint->SetLayer( elem.layer == ALTIUM_LAYER::TOP_LAYER ? F_Cu : B_Cu );

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


void ALTIUM_PCB::ParseComponentsBodies6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                             const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading component 3D models..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ACOMPONENTBODY6 elem( reader ); // TODO: implement

        if( elem.component == ALTIUM_COMPONENT_NONE )
            continue; // TODO: we do not support components for the board yet

        if( m_components.size() <= elem.component )
        {
            THROW_IO_ERROR( wxString::Format( wxT( "ComponentsBodies6 stream tries to access "
                                                   "component id %d of %d existing components" ),
                                              elem.component,
                                              m_components.size() ) );
        }

        if( !elem.modelIsEmbedded )
            continue;

        auto modelTuple = m_models.find( elem.modelId );

        if( modelTuple == m_models.end() )
        {
            wxLogError( wxT( "ComponentsBodies6 stream tries to access model id %s which does not "
                             "exist" ),
                        elem.modelId );
            continue;
        }

        FOOTPRINT*     footprint  = m_components.at( elem.component );
        const VECTOR2I& fpPosition = footprint->GetPosition();

        FP_3DMODEL modelSettings;

        modelSettings.m_Filename = modelTuple->second;

        modelSettings.m_Offset.x = pcbIUScale.IUTomm((int) elem.modelPosition.x - fpPosition.x );
        modelSettings.m_Offset.y = -pcbIUScale.IUTomm((int) elem.modelPosition.y - fpPosition.y );
        modelSettings.m_Offset.z = pcbIUScale.IUTomm( (int) elem.modelPosition.z );

        EDA_ANGLE orientation = footprint->GetOrientation();

        if( footprint->IsFlipped() )
        {
            modelSettings.m_Offset.y = -modelSettings.m_Offset.y;
            orientation              = -orientation;
        }

        RotatePoint( &modelSettings.m_Offset.x, &modelSettings.m_Offset.y, orientation );

        modelSettings.m_Rotation.x = normalizeAngleDegrees( -elem.modelRotation.x, -180, 180 );
        modelSettings.m_Rotation.y = normalizeAngleDegrees( -elem.modelRotation.y, -180, 180 );
        modelSettings.m_Rotation.z = normalizeAngleDegrees( -elem.modelRotation.z
                                                                        + elem.rotation
                                                                        + orientation.AsDegrees(),
                                                            -180, 180 );
        modelSettings.m_Opacity = elem.bodyOpacity;

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
        wxLogWarning( _( "Dimension found on an Altium layer (%d) with no KiCad equivalent. "
                         "It has been moved to KiCad layer Eco1_User." ),
                      aElem.layer );
        klayer = Eco1_User;
    }

    VECTOR2I referencePoint0 = aElem.referencePoint.at( 0 );
    VECTOR2I referencePoint1 = aElem.referencePoint.at( 1 );

    PCB_DIM_ALIGNED* dimension = new PCB_DIM_ALIGNED( m_board, PCB_DIM_ALIGNED_T );
    m_board->Add( dimension, ADD_MODE::APPEND );

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
        VECTOR2I direction             = aElem.xy1 - referencePoint0;
        VECTOR2I directionNormalVector = VECTOR2I( -direction.y, direction.x );
        SEG     segm1( referencePoint0, referencePoint0 + directionNormalVector );
        SEG     segm2( referencePoint1, referencePoint1 + direction );
        OPT_VECTOR2I intersection( segm1.Intersect( segm2, true, true ) );

        if( !intersection )
            THROW_IO_ERROR( wxT( "Invalid dimension.  This should never happen." ) );

        dimension->SetEnd( *intersection );

        int height = static_cast<int>( EuclideanNorm( direction ) );

        if( direction.x <= 0 && direction.y <= 0 ) // TODO: I suspect this is not always correct
            height = -height;

        dimension->SetHeight( height );
    }
    else
    {
        dimension->SetEnd( referencePoint1 );
    }

    dimension->SetLineThickness( aElem.linewidth );

    dimension->SetPrefix( aElem.textprefix );

    // Suffix normally holds the units
    dimension->SetUnitsFormat( aElem.textsuffix.IsEmpty() ? DIM_UNITS_FORMAT::NO_SUFFIX
                                                          : DIM_UNITS_FORMAT::BARE_SUFFIX );

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
    case ALTIUM_UNIT::INCHES:
        dimension->SetUnits( EDA_UNITS::INCHES );
        break;
    case ALTIUM_UNIT::MILS:
        dimension->SetUnits( EDA_UNITS::MILS );
        break;
    case ALTIUM_UNIT::MILLIMETERS:
    case ALTIUM_UNIT::CENTIMETER:
        dimension->SetUnits( EDA_UNITS::MILLIMETRES );
        break;
    default:
        break;
    }
}


void ALTIUM_PCB::HelperParseDimensions6Radial(const ADIMENSION6 &aElem)
{
    if( aElem.referencePoint.size() < 2 )
        THROW_IO_ERROR( wxT( "Not enough reference points for radial dimension object" ) );

    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        wxLogWarning( _( "Dimension found on an Altium layer (%d) with no KiCad equivalent. "
                         "It has been moved to KiCad layer Eco1_User." ),
                      aElem.layer );
        klayer = Eco1_User;
    }

    VECTOR2I referencePoint0 = aElem.referencePoint.at( 0 );
    VECTOR2I referencePoint1 = aElem.referencePoint.at( 1 );

    PCB_DIM_RADIAL* dimension = new PCB_DIM_RADIAL( m_board );
    m_board->Add( dimension, ADD_MODE::APPEND );
    m_radialDimensions.push_back( dimension );

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
    case ALTIUM_UNIT::INCHES:
        dimension->SetUnits( EDA_UNITS::INCHES );
        break;
    case ALTIUM_UNIT::MILS:
        dimension->SetUnits( EDA_UNITS::MILS );
        break;
    case ALTIUM_UNIT::MILLIMETERS:
    case ALTIUM_UNIT::CENTIMETER:
        dimension->SetUnits( EDA_UNITS::MILLIMETRES );
        break;
    default:
        break;
    }

    if( aElem.textPoint.empty() )
    {
        wxLogError( wxT( "No text position present for leader dimension object" ) );
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

    int yAdjust = dimension->GetTextBox().GetCenter().y - dimension->GetTextPos().y;
    dimension->SetTextPos( dimension->GetTextPos() + VECTOR2I( 0, yAdjust + aElem.textgap ) );
    dimension->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
}


void ALTIUM_PCB::HelperParseDimensions6Leader( const ADIMENSION6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        wxLogWarning( _( "Dimension found on an Altium layer (%d) with no KiCad equivalent. "
                         "It has been moved to KiCad layer Eco1_User." ),
                      aElem.layer );
        klayer = Eco1_User;
    }

    if( !aElem.referencePoint.empty() )
    {
        VECTOR2I referencePoint0 = aElem.referencePoint.at( 0 );

        // line
        VECTOR2I last = referencePoint0;
        for( size_t i = 1; i < aElem.referencePoint.size(); i++ )
        {
            PCB_SHAPE* shape = new PCB_SHAPE( m_board, SHAPE_T::SEGMENT );
            m_board->Add( shape, ADD_MODE::APPEND );
            shape->SetLayer( klayer );
            shape->SetStroke( STROKE_PARAMS( aElem.linewidth, PLOT_DASH_TYPE::SOLID ) );
            shape->SetStart( last );
            shape->SetEnd( aElem.referencePoint.at( i ) );
            last = aElem.referencePoint.at( i );
        }

        // arrow
        if( aElem.referencePoint.size() >= 2 )
        {
            VECTOR2I dirVec = aElem.referencePoint.at( 1 ) - referencePoint0;

            if( dirVec.x != 0 || dirVec.y != 0 )
            {
                double   scaling = EuclideanNorm( dirVec ) / aElem.arrowsize;
                VECTOR2I arrVec = VECTOR2I( KiROUND( dirVec.x / scaling ),
                                            KiROUND( dirVec.y / scaling ) );
                RotatePoint( arrVec, EDA_ANGLE( 20.0, DEGREES_T ) );

                PCB_SHAPE* shape1 = new PCB_SHAPE( m_board, SHAPE_T::SEGMENT );
                m_board->Add( shape1, ADD_MODE::APPEND );
                shape1->SetLayer( klayer );
                shape1->SetStroke( STROKE_PARAMS( aElem.linewidth, PLOT_DASH_TYPE::SOLID ) );
                shape1->SetStart( referencePoint0 );
                shape1->SetEnd( referencePoint0 + arrVec );

                RotatePoint( arrVec, EDA_ANGLE( -40.0, DEGREES_T ) );

                PCB_SHAPE* shape2 = new PCB_SHAPE( m_board, SHAPE_T::SEGMENT );
                m_board->Add( shape2, ADD_MODE::APPEND );
                shape2->SetLayer( klayer );
                shape2->SetStroke( STROKE_PARAMS( aElem.linewidth, PLOT_DASH_TYPE::SOLID ) );
                shape2->SetStart( referencePoint0 );
                shape2->SetEnd( referencePoint0 + arrVec );
            }
        }
    }

    if( aElem.textPoint.empty() )
    {
        wxLogError( wxT( "No text position present for leader dimension object" ) );
        return;
    }

    PCB_TEXT* text = new PCB_TEXT( m_board );
    m_board->Add( text, ADD_MODE::APPEND );
    text->SetText( aElem.textformat );
    text->SetPosition( aElem.textPoint.at( 0 ) );
    text->SetLayer( klayer );
    text->SetTextSize( VECTOR2I( aElem.textheight, aElem.textheight ) ); // TODO: parse text width
    text->SetTextThickness( aElem.textlinewidth );
    text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
}


void ALTIUM_PCB::HelperParseDimensions6Datum( const ADIMENSION6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        wxLogWarning( _( "Dimension found on an Altium layer (%d) with no KiCad equivalent. "
                         "It has been moved to KiCad layer Eco1_User." ),
                      aElem.layer );
        klayer = Eco1_User;
    }

    for( size_t i = 0; i < aElem.referencePoint.size(); i++ )
    {
        PCB_SHAPE* shape = new PCB_SHAPE( m_board, SHAPE_T::SEGMENT );
        m_board->Add( shape, ADD_MODE::APPEND );
        shape->SetLayer( klayer );
        shape->SetStroke( STROKE_PARAMS( aElem.linewidth, PLOT_DASH_TYPE::SOLID ) );
        shape->SetStart( aElem.referencePoint.at( i ) );
        // shape->SetEnd( /* TODO: seems to be based on TEXTY */ );
    }
}


void ALTIUM_PCB::HelperParseDimensions6Center( const ADIMENSION6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        wxLogWarning( _( "Dimension found on an Altium layer (%d) with no KiCad equivalent. "
                         "It has been moved to KiCad layer Eco1_User." ),
                      aElem.layer );
        klayer = Eco1_User;
    }

    VECTOR2I vec = VECTOR2I( 0, aElem.height / 2 );
    RotatePoint( vec, EDA_ANGLE( aElem.angle, DEGREES_T ) );

    PCB_DIM_CENTER* dimension = new PCB_DIM_CENTER( m_board );
    m_board->Add( dimension, ADD_MODE::APPEND );
    dimension->SetLayer( klayer );
    dimension->SetLineThickness( aElem.linewidth );
    dimension->SetStart( aElem.xy1 );
    dimension->SetEnd( aElem.xy1 + vec );
}


void ALTIUM_PCB::ParseDimensions6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                       const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading dimension drawings..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ADIMENSION6 elem( reader );

        switch( elem.kind )
        {
        case ALTIUM_DIMENSION_KIND::LINEAR:
            HelperParseDimensions6Linear( elem );
            break;
        case ALTIUM_DIMENSION_KIND::RADIAL:
            HelperParseDimensions6Radial( elem );
            break;
        case ALTIUM_DIMENSION_KIND::LEADER:
            HelperParseDimensions6Leader( elem );
            break;
        case ALTIUM_DIMENSION_KIND::DATUM:
            wxLogError( _( "Ignored dimension of kind %d (not yet supported)." ), elem.kind );
            // HelperParseDimensions6Datum( elem );
            break;
        case ALTIUM_DIMENSION_KIND::CENTER:
            HelperParseDimensions6Center( elem );
            break;
        default:
            wxLogError( _( "Ignored dimension of kind %d (not yet supported)." ), elem.kind );
            break;
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Dimensions6 stream is not fully parsed" ) );
}


void ALTIUM_PCB::ParseModelsData( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry,
                                  const std::vector<std::string>& aRootDir )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading 3D models..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    if( reader.GetRemainingBytes() == 0 )
        return;

    wxString projectPath = wxPathOnly( m_board->GetFileName() );
    // TODO: set KIPRJMOD always after import (not only when loading project)?
    wxSetEnv( PROJECT_VAR_NAME, projectPath );

    // TODO: make this path configurable?
    const wxString altiumModelDir = wxT( "ALTIUM_EMBEDDED_MODELS" );

    wxFileName altiumModelsPath = wxFileName::DirName( projectPath );
    wxString   kicadModelPrefix = wxT( "${KIPRJMOD}/" ) + altiumModelDir + wxT( "/" );

    if( !altiumModelsPath.AppendDir( altiumModelDir ) )
        THROW_IO_ERROR( wxT( "Cannot construct directory path for step models" ) );

    // Create dir if it does not exist
    if( !altiumModelsPath.DirExists() )
    {
        if( !altiumModelsPath.Mkdir() )
        {
            wxLogError( _( "Failed to create folder '%s'." ) + wxS( " " )
                      + _( "No 3D-models will be imported." ),
                        altiumModelsPath.GetFullPath() );
            return;
        }
    }

    int      idx = 0;
    wxString invalidChars = wxFileName::GetForbiddenChars();

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AMODEL elem( reader );

        std::vector<std::string> stepPath = aRootDir;
        stepPath.emplace_back( std::to_string( idx ) );

        bool           validName = !elem.name.IsEmpty() && elem.name.IsAscii() &&
                                   wxString::npos == elem.name.find_first_of( invalidChars );
        wxString       storageName = !validName ? wxString::Format( wxT( "model_%d" ), idx )
                                                : elem.name;
        wxFileName     storagePath( altiumModelsPath.GetPath(), storageName );

        idx++;

        const CFB::COMPOUND_FILE_ENTRY* stepEntry = aAltiumPcbFile.FindStream( stepPath );

        if( stepEntry == nullptr )
        {
            wxLogError( _( "File not found: '%s'. 3D-model not imported." ),
                        FormatPath( stepPath ) );
            continue;
        }

        size_t            stepSize = static_cast<size_t>( stepEntry->size );
        std::vector<char> stepContent( stepSize );

        // read file into buffer
        aAltiumPcbFile.GetCompoundFileReader().ReadFile( stepEntry, 0, stepContent.data(),
                                                         stepSize );

        if( !storagePath.IsDirWritable() )
        {
            wxLogError( _( "Insufficient permissions to save file '%s'." ),
                        storagePath.GetFullPath() );
            continue;
        }

        wxMemoryInputStream stepStream( stepContent.data(), stepSize );
        wxZlibInputStream   zlibInputStream( stepStream );

        wxFFileOutputStream outputStream( storagePath.GetFullPath() );
        outputStream.Write( zlibInputStream );
        outputStream.Close();

        m_models.insert( { elem.id, kicadModelPrefix + storageName } );
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Models stream is not fully parsed" ) );
}


void ALTIUM_PCB::ParseNets6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading nets..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    wxASSERT( m_altiumToKicadNetcodes.empty() );
    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ANET6 elem( reader );

        NETINFO_ITEM* netInfo = new NETINFO_ITEM( m_board, elem.name, 0 );
        m_board->Add( netInfo, ADD_MODE::APPEND );

        m_altiumToKicadNetcodes.push_back( netInfo->GetNetCode() );
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Nets6 stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParsePolygons6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                     const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading polygons..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        APOLYGON6 elem( reader );

        SHAPE_LINE_CHAIN linechain;
        HelperShapeLineChainFromAltiumVertices( linechain, elem.vertices );

        if( linechain.PointCount() < 2 )
        {
            // We have found multiple Altium files with polygon records containing nothing but two
            // coincident vertices.  These polygons do not appear when opening the file in Altium.
            // https://gitlab.com/kicad/code/kicad/-/issues/8183
            //
            // wxLogError( _( "Polygon has only %d point extracted from %ld vertices. At least 2 "
            //                "points are required." ),
            //             linechain.PointCount(),
            //             elem.vertices.size() );

            m_polygons.emplace_back( nullptr );
            continue;
        }

        ZONE* zone = new ZONE( m_board );
        m_board->Add( zone, ADD_MODE::APPEND );
        m_polygons.emplace_back( zone );

        zone->SetNetCode( GetNetCode( elem.net ) );
        zone->SetPosition( elem.vertices.at( 0 ).position );
        zone->SetLocked( elem.locked );
        zone->SetAssignedPriority( elem.pourindex > 0 ? elem.pourindex : 0 );
        zone->Outline()->AddOutline( linechain );

        HelperSetZoneLayers( zone, elem.layer );

        if( elem.pourindex > m_highest_pour_index )
            m_highest_pour_index = elem.pourindex;

        // TODO: more flexible rule parsing
        const ARULE6* clearanceRule = GetRuleDefault( ALTIUM_RULE_KIND::PLANE_CLEARANCE );

        if( clearanceRule != nullptr )
            zone->SetLocalClearance( clearanceRule->planeclearanceClearance );

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
                m_outer_plane[elem.layer] = zone;
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
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Polygons6 stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParseRules6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading rules..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

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

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Rules6 stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParseBoardRegionsData( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                        const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading board regions..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AREGION6 elem( reader, false );

        // TODO: implement?
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "BoardRegions stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParseShapeBasedRegions6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                              const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading zones..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
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
            ConvertShapeBasedRegions6ToFootprintItem( footprint, elem );
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

        if( linechain.PointCount() < 2 )
        {
            // We have found multiple Altium files with polygon records containing nothing but
            // two coincident vertices.  These polygons do not appear when opening the file in
            // Altium.  https://gitlab.com/kicad/code/kicad/-/issues/8183
            return;
        }

        ZONE* zone = new ZONE( m_board );
        m_board->Add( zone, ADD_MODE::APPEND );

        zone->SetIsRuleArea( true );

        HelperSetZoneKeepoutRestrictions( zone, aElem.keepoutrestrictions );

        zone->SetPosition( aElem.outline.at( 0 ).position );
        zone->Outline()->AddOutline( linechain );

        HelperSetZoneLayers( zone, aElem.layer );

        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     ZONE::GetDefaultHatchPitch(), true );
    }
    else if( aElem.kind == ALTIUM_REGION_KIND::COPPER )
    {
        if( aElem.subpolyindex == ALTIUM_POLYGON_NONE )
        {
            for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            {
                ConvertShapeBasedRegions6ToBoardItemOnLayer( aElem, klayer );
            }
        }
    }
    else
    {
        wxLogError( _( "Ignored polygon shape of kind %d (not yet supported)." ), aElem.kind );
    }
}


void ALTIUM_PCB::ConvertShapeBasedRegions6ToFootprintItem( FOOTPRINT*      aFootprint,
                                                           const AREGION6& aElem )
{
    if( aElem.kind == ALTIUM_REGION_KIND::POLYGON_CUTOUT || aElem.is_keepout )
    {
        SHAPE_LINE_CHAIN linechain;
        HelperShapeLineChainFromAltiumVertices( linechain, aElem.outline );

        if( linechain.PointCount() < 2 )
        {
            // We have found multiple Altium files with polygon records containing nothing but
            // two coincident vertices. These polygons do not appear when opening the file in
            // Altium. https://gitlab.com/kicad/code/kicad/-/issues/8183
            return;
        }

        ZONE* zone = new ZONE( aFootprint );
        aFootprint->Add( zone, ADD_MODE::APPEND );

        zone->SetIsRuleArea( true );

        HelperSetZoneKeepoutRestrictions( zone, aElem.keepoutrestrictions );

        zone->SetPosition( aElem.outline.at( 0 ).position );
        zone->Outline()->AddOutline( linechain );

        HelperSetZoneLayers( zone, aElem.layer );

        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     ZONE::GetDefaultHatchPitch(), true );
    }
    else if( aElem.kind == ALTIUM_REGION_KIND::COPPER )
    {
        if( aElem.subpolyindex == ALTIUM_POLYGON_NONE )
        {
            for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
                ConvertShapeBasedRegions6ToFootprintItemOnLayer( aFootprint, aElem, klayer );
        }
    }
    else
    {
        wxLogError( _( "Ignored polygon shape of kind %d (not yet supported)." ), aElem.kind );
    }
}


void ALTIUM_PCB::ConvertShapeBasedRegions6ToBoardItemOnLayer( const AREGION6& aElem,
                                                              PCB_LAYER_ID    aLayer )
{
    SHAPE_LINE_CHAIN linechain;
    HelperShapeLineChainFromAltiumVertices( linechain, aElem.outline );

    if( linechain.PointCount() < 2 )
    {
        // We have found multiple Altium files with polygon records containing nothing
        // but two coincident vertices. These polygons do not appear when opening the
        // file in Altium. https://gitlab.com/kicad/code/kicad/-/issues/8183
        return;
    }

    PCB_SHAPE* shape = new PCB_SHAPE( m_board, SHAPE_T::POLY );

    shape->SetPolyShape( linechain );
    shape->SetFilled( true );
    shape->SetLayer( aLayer );
    shape->SetStroke( STROKE_PARAMS( 0 ) );

    m_board->Add( shape, ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertShapeBasedRegions6ToFootprintItemOnLayer( FOOTPRINT*      aFootprint,
                                                                  const AREGION6& aElem,
                                                                  PCB_LAYER_ID    aLayer )
{
    SHAPE_LINE_CHAIN linechain;
    HelperShapeLineChainFromAltiumVertices( linechain, aElem.outline );

    if( linechain.PointCount() < 2 )
    {
        // We have found multiple Altium files with polygon records containing nothing
        // but two coincident vertices. These polygons do not appear when opening the
        // file in Altium. https://gitlab.com/kicad/code/kicad/-/issues/8183
        return;
    }

    PCB_SHAPE* shape = new PCB_SHAPE( aFootprint, SHAPE_T::POLY );

    shape->SetPolyShape( linechain );
    shape->SetFilled( true );
    shape->SetLayer( aLayer );
    shape->SetStroke( STROKE_PARAMS( 0 ) );

    aFootprint->Add( shape, ADD_MODE::APPEND );
}


void ALTIUM_PCB::ParseRegions6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                    const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading zone fills..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    for( ZONE* zone : m_polygons )
    {
        if( zone )
            zone->UnFill(); // just to be sure
    }

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AREGION6 elem( reader, false );

        if( elem.subpolyindex != ALTIUM_POLYGON_NONE )
        {
            if( m_polygons.size() <= elem.subpolyindex )
            {
                THROW_IO_ERROR(  wxString::Format( "Region stream tries to access polygon id %d "
                                                   "of %d existing polygons.",
                                                   elem.subpolyindex,
                                                   m_polygons.size() ) );
            }

            ZONE *zone = m_polygons.at( elem.subpolyindex );

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
                fill.BooleanAdd( *zone->GetFill( klayer ), SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

            fill.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

            zone->SetFilledPolysList( klayer, fill );
            zone->SetIsFilled( true );
            zone->SetNeedRefill( false );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Regions6 stream is not fully parsed" ) );
}


void ALTIUM_PCB::ParseArcs6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading arcs..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

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
    if( aElem.is_polygonoutline || aElem.subpolyindex != ALTIUM_POLYGON_NONE )
        return;

    if( aElem.is_keepout || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER
        || IsAltiumLayerAPlane( aElem.layer ) )
    {
        // This is not the actual board item. We can use it to create the polygon for the region
        PCB_SHAPE shape( nullptr );

        ConvertArcs6ToPcbShape( aElem, &shape );
        shape.SetStroke( STROKE_PARAMS( aElem.width, PLOT_DASH_TYPE::SOLID ) );

        HelperPcpShapeAsBoardKeepoutRegion( shape, aElem.layer, aElem.keepoutrestrictions );
    }
    else
    {
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            ConvertArcs6ToBoardItemOnLayer( aElem, klayer );
    }

    for( const auto layerExpansionMask :
         HelperGetSolderAndPasteMaskExpansions( ALTIUM_RECORD::ARC, aPrimitiveIndex, aElem.layer ) )
    {
        int width = aElem.width + ( layerExpansionMask.second * 2 );

        if( width > 1 )
        {
            PCB_SHAPE* arc = new PCB_SHAPE( m_board );

            ConvertArcs6ToPcbShape( aElem, arc );
            arc->SetStroke( STROKE_PARAMS( width, PLOT_DASH_TYPE::SOLID ) );
            arc->SetLayer( layerExpansionMask.first );

            m_board->Add( arc, ADD_MODE::APPEND );
        }
    }
}


void ALTIUM_PCB::ConvertArcs6ToFootprintItem( FOOTPRINT* aFootprint, const AARC6& aElem,
                                              const int aPrimitiveIndex, const bool aIsBoardImport )
{
    if( aElem.is_polygonoutline || aElem.subpolyindex != ALTIUM_POLYGON_NONE )
        return;

    if( aElem.is_keepout || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER
        || IsAltiumLayerAPlane( aElem.layer ) )
    {
        // This is not the actual board item. We can use it to create the polygon for the region
        PCB_SHAPE shape( nullptr );

        ConvertArcs6ToPcbShape( aElem, &shape );
        shape.SetStroke( STROKE_PARAMS( aElem.width, PLOT_DASH_TYPE::SOLID ) );

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

    for( const auto layerExpansionMask :
         HelperGetSolderAndPasteMaskExpansions( ALTIUM_RECORD::ARC, aPrimitiveIndex, aElem.layer ) )
    {
        int width = aElem.width + ( layerExpansionMask.second * 2 );

        if( width > 1 )
        {
            PCB_SHAPE* arc = new PCB_SHAPE( aFootprint );

            ConvertArcs6ToPcbShape( aElem, arc );
            arc->SetStroke( STROKE_PARAMS( width, PLOT_DASH_TYPE::SOLID ) );
            arc->SetLayer( layerExpansionMask.first );

            aFootprint->Add( arc, ADD_MODE::APPEND );
        }
    }
}


void ALTIUM_PCB::ConvertArcs6ToBoardItemOnLayer( const AARC6& aElem, PCB_LAYER_ID aLayer )
{
    if( IsCopperLayer( aLayer ) && aElem.net != ALTIUM_NET_UNCONNECTED )
    {
        // TODO: This is not the actual board item. We use it for now to calculate the arc points. This could be improved!
        PCB_SHAPE shape( nullptr, SHAPE_T::ARC );

        EDA_ANGLE includedAngle( aElem.endangle - aElem.startangle, DEGREES_T );
        EDA_ANGLE startAngle( aElem.endangle, DEGREES_T );

        VECTOR2I startOffset = VECTOR2I( KiROUND( startAngle.Cos() * aElem.radius ),
                                         -KiROUND( startAngle.Sin() * aElem.radius ) );

        shape.SetCenter( aElem.center );
        shape.SetStart( aElem.center + startOffset );
        shape.SetArcAngleAndEnd( includedAngle.Normalize(), true );

        // Create actual arc
        SHAPE_ARC shapeArc( shape.GetCenter(), shape.GetStart(), shape.GetArcAngle(), aElem.width );
        PCB_ARC*  arc = new PCB_ARC( m_board, &shapeArc );

        arc->SetWidth( aElem.width );
        arc->SetLayer( aLayer );
        arc->SetNetCode( GetNetCode( aElem.net ) );

        m_board->Add( arc, ADD_MODE::APPEND );
    }
    else
    {
        PCB_SHAPE* arc = new PCB_SHAPE( m_board );

        ConvertArcs6ToPcbShape( aElem, arc );
        arc->SetStroke( STROKE_PARAMS( aElem.width, PLOT_DASH_TYPE::SOLID ) );
        arc->SetLayer( aLayer );

        m_board->Add( arc, ADD_MODE::APPEND );
    }
}


void ALTIUM_PCB::ConvertArcs6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const AARC6& aElem,
                                                     PCB_LAYER_ID aLayer )
{
    PCB_SHAPE* arc = new PCB_SHAPE( aFootprint );

    ConvertArcs6ToPcbShape( aElem, arc );
    arc->SetStroke( STROKE_PARAMS( aElem.width, PLOT_DASH_TYPE::SOLID ) );
    arc->SetLayer( aLayer );

    aFootprint->Add( arc, ADD_MODE::APPEND );
}


void ALTIUM_PCB::ParsePads6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading pads..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

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
        FOOTPRINT* footprint = new FOOTPRINT( m_board );
        footprint->SetPosition( aElem.position );

        ConvertPads6ToFootprintItemOnCopper( footprint, aElem );

        m_board->Add( footprint, ADD_MODE::APPEND );
    }
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
    PAD* pad = new PAD( aFootprint );

    pad->SetKeepTopBottom( false ); // TODO: correct? This seems to be KiCad default on import

    pad->SetNumber( aElem.name );
    pad->SetNetCode( GetNetCode( aElem.net ) );

    pad->SetPosition( aElem.position );
    pad->SetOrientationDegrees( aElem.direction );
    pad->SetSize( aElem.topsize );

    if( aElem.holesize == 0 )
    {
        pad->SetAttribute( PAD_ATTRIB::SMD );
    }
    else
    {
        if( aElem.layer != ALTIUM_LAYER::MULTI_LAYER )
        {
            // TODO: I assume other values are possible as well?
            wxLogError( _( "Footprint %s pad %s is not marked as multilayer, but is a TH pad." ),
                        aFootprint->GetReference(),
                        aElem.name );
        }

        pad->SetAttribute( aElem.plated ? PAD_ATTRIB::PTH : PAD_ATTRIB::NPTH );

        if( !aElem.sizeAndShape || aElem.sizeAndShape->holeshape == ALTIUM_PAD_HOLE_SHAPE::ROUND )
        {
            pad->SetDrillShape( PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_CIRCLE );
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
                wxLogWarning( _( "Footprint %s pad %s has a square hole (not yet supported)." ),
                              aFootprint->GetReference(),
                              aElem.name );

                pad->SetDrillShape( PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_CIRCLE );
                pad->SetDrillSize( VECTOR2I( aElem.holesize, aElem.holesize ) ); // Workaround
                // TODO: elem.sizeAndShape->slotsize was 0 in testfile. Either use holesize in
                //  this case or rect holes have a different id
                break;

            case ALTIUM_PAD_HOLE_SHAPE::SLOT:
            {
                pad->SetDrillShape( PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_OBLONG );
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
                    wxLogWarning( _( "Footprint %s pad %s has a hole-rotation of %f degrees. "
                                     "KiCad only supports 90 degree rotations." ),
                                  aFootprint->GetReference(),
                                  aElem.name,
                                  slotRotation.AsDegrees() );
                }

                break;
            }

            default:
            case ALTIUM_PAD_HOLE_SHAPE::UNKNOWN:
                wxLogError( _( "Footprint %s pad %s uses a hole of unknown kind %d." ),
                            aFootprint->GetReference(),
                            aElem.name,
                            aElem.sizeAndShape->holeshape );

                pad->SetDrillShape( PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_CIRCLE );
                pad->SetDrillSize( VECTOR2I( aElem.holesize, aElem.holesize ) ); // Workaround
                break;
            }
        }

        if( aElem.sizeAndShape )
            pad->SetOffset( aElem.sizeAndShape->holeoffset[0] );
    }

    if( aElem.padmode != ALTIUM_PAD_MODE::SIMPLE )
    {
        wxLogError( _( "Footprint %s pad %s uses a complex pad stack (not yet supported.)" ),
                    aFootprint->GetReference(), aElem.name );
    }

    switch( aElem.topshape )
    {
    case ALTIUM_PAD_SHAPE::RECT:
        pad->SetShape( PAD_SHAPE::RECT );
        break;

    case ALTIUM_PAD_SHAPE::CIRCLE:
        if( aElem.sizeAndShape
            && aElem.sizeAndShape->alt_shape[0] == ALTIUM_PAD_SHAPE_ALT::ROUNDRECT )
        {
            pad->SetShape( PAD_SHAPE::ROUNDRECT ); // 100 = round, 0 = rectangular
            double ratio = aElem.sizeAndShape->cornerradius[0] / 200.;
            pad->SetRoundRectRadiusRatio( ratio );
        }
        else if( aElem.topsize.x == aElem.topsize.y )
        {
            pad->SetShape( PAD_SHAPE::CIRCLE );
        }
        else
        {
            pad->SetShape( PAD_SHAPE::OVAL );
        }

        break;

    case ALTIUM_PAD_SHAPE::OCTAGONAL:
        pad->SetShape( PAD_SHAPE::CHAMFERED_RECT );
        pad->SetChamferPositions( RECT_CHAMFER_ALL );
        pad->SetChamferRectRatio( 0.25 );
        break;

    case ALTIUM_PAD_SHAPE::UNKNOWN:
    default:
        wxLogError( _( "Footprint %s pad %s uses an unknown pad-shape." ),
                    aFootprint->GetReference(), aElem.name );
        break;
    }

    if( pad->GetAttribute() == PAD_ATTRIB::NPTH && pad->HasHole() )
    {
        // KiCad likes NPTH pads to be the same size & shape as their holes
        pad->SetShape( pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE ? PAD_SHAPE::CIRCLE
                                                                      : PAD_SHAPE::OVAL );
        pad->SetSize( pad->GetDrillSize() );
    }

    switch( aElem.layer )
    {
    case ALTIUM_LAYER::TOP_LAYER:
        pad->SetLayer( F_Cu );
        pad->SetLayerSet( PAD::SMDMask() );
        break;

    case ALTIUM_LAYER::BOTTOM_LAYER:
        pad->SetLayer( B_Cu );
        pad->SetLayerSet( FlipLayerMask( PAD::SMDMask() ) );
        break;

    case ALTIUM_LAYER::MULTI_LAYER:
        pad->SetLayerSet( aElem.plated ? PAD::PTHMask() : PAD::UnplatedHoleMask() );
        break;

    default:
        PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );
        pad->SetLayer( klayer );
        pad->SetLayerSet( LSET( 1, klayer ) );
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

    aFootprint->Add( pad, ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertPads6ToBoardItemOnNonCopper( const APAD6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        wxLogWarning( _( "Non-copper pad %s found on an Altium layer (%d) with no KiCad "
                         "equivalent. It has been moved to KiCad layer Eco1_User." ),
                      aElem.name, aElem.layer );
        klayer = Eco1_User;
    }

    PCB_SHAPE* pad = new PCB_SHAPE( m_board );

    HelperParsePad6NonCopper( aElem, klayer, pad );

    m_board->Add( pad, ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertPads6ToFootprintItemOnNonCopper( FOOTPRINT* aFootprint, const APAD6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        wxLogWarning(
                _( "Non-copper pad %s found on an Altium layer (%d) with no KiCad equivalent. "
                   "It has been moved to KiCad layer Eco1_User." ),
                aElem.name, aElem.layer );
        klayer = Eco1_User;
    }

    PCB_SHAPE* pad = new PCB_SHAPE( aFootprint );

    HelperParsePad6NonCopper( aElem, klayer, pad );

    aFootprint->Add( pad, ADD_MODE::APPEND );
}


void ALTIUM_PCB::HelperParsePad6NonCopper( const APAD6& aElem, PCB_LAYER_ID aLayer,
                                           PCB_SHAPE* aShape )
{
    if( aElem.net != ALTIUM_NET_UNCONNECTED )
    {
        wxLogError( _( "Non-copper pad %s is connected to a net, which is not supported." ),
                    aElem.name );
    }

    if( aElem.holesize != 0 )
    {
        wxLogError( _( "Non-copper pad %s has a hole, which is not supported." ), aElem.name );
    }

    if( aElem.padmode != ALTIUM_PAD_MODE::SIMPLE )
    {
        wxLogWarning( _( "Non-copper pad %s has a complex pad stack (not yet supported)." ),
                      aElem.name );
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
            aShape->SetStroke( STROKE_PARAMS( offset * 2, PLOT_DASH_TYPE::SOLID ) );

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
                aShape->SetStroke( STROKE_PARAMS( aElem.topsize.x / 2, PLOT_DASH_TYPE::SOLID ) );
            }
            else if( aElem.topsize.x < aElem.topsize.y )
            {
                // short vertical line
                aShape->SetShape( SHAPE_T::SEGMENT );
                VECTOR2I pointOffset( 0, ( aElem.topsize.y - aElem.topsize.x ) / 2 );
                aShape->SetStart( aElem.position + pointOffset );
                aShape->SetEnd( aElem.position - pointOffset );
            }
            else
            {
                // short horizontal line
                aShape->SetShape( SHAPE_T::SEGMENT );
                VECTOR2I pointOffset( ( aElem.topsize.x - aElem.topsize.y ) / 2, 0 );
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
            aShape->SetStroke( STROKE_PARAMS( aElem.topsize.x / 2, PLOT_DASH_TYPE::SOLID ) );
        }
        else
        {
            // short line
            aShape->SetShape( SHAPE_T::SEGMENT );
            aShape->SetLayer( aLayer );
            aShape->SetStroke( STROKE_PARAMS( std::min( aElem.topsize.x, aElem.topsize.y ),
                                              PLOT_DASH_TYPE::SOLID ) );

            if( aElem.topsize.x < aElem.topsize.y )
            {
                VECTOR2I offset( 0, ( aElem.topsize.y - aElem.topsize.x ) / 2 );
                aShape->SetStart( aElem.position + offset );
                aShape->SetEnd( aElem.position - offset );
            }
            else
            {
                VECTOR2I offset( ( aElem.topsize.x - aElem.topsize.y ) / 2, 0 );
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
        wxLogError( _( "Non-copper pad %s uses an unknown pad-shape." ), aElem.name );
        break;
    }
}


void ALTIUM_PCB::ParseVias6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading vias..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AVIA6 elem( reader );

        PCB_VIA* via = new PCB_VIA( m_board );
        m_board->Add( via, ADD_MODE::APPEND );

        via->SetPosition( elem.position );
        via->SetWidth( elem.diameter );
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
            via->SetViaType( VIATYPE::BLIND_BURIED );
        }
        else
        {
            via->SetViaType( VIATYPE::MICROVIA ); // TODO: always a microvia?
        }

        PCB_LAYER_ID start_klayer = GetKicadLayer( elem.layer_start );
        PCB_LAYER_ID end_klayer   = GetKicadLayer( elem.layer_end );

        if( !IsCopperLayer( start_klayer ) || !IsCopperLayer( end_klayer ) )
        {
            wxLogError( _( "Via from layer %d to %d uses a non-copper layer, which is not "
                           "supported." ),
                        elem.layer_start,
                        elem.layer_end );
            continue; // just assume through-hole instead.
        }

        // we need VIATYPE set!
        via->SetLayerPair( start_klayer, end_klayer );
    }

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "Vias6 stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParseTracks6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                   const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading tracks..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

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
    if( aElem.is_polygonoutline || aElem.subpolyindex != ALTIUM_POLYGON_NONE )
        return;

    if( aElem.is_keepout || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER
        || IsAltiumLayerAPlane( aElem.layer ) )
    {
        // This is not the actual board item. We can use it to create the polygon for the region
        PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
        shape.SetStart( aElem.start );
        shape.SetEnd( aElem.end );
        shape.SetStroke( STROKE_PARAMS( aElem.width, PLOT_DASH_TYPE::SOLID ) );

        HelperPcpShapeAsBoardKeepoutRegion( shape, aElem.layer, aElem.keepoutrestrictions );
    }
    else
    {
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
        {
            ConvertTracks6ToBoardItemOnLayer( aElem, klayer );
        }
    }

    for( const auto layerExpansionMask : HelperGetSolderAndPasteMaskExpansions(
                 ALTIUM_RECORD::TRACK, aPrimitiveIndex, aElem.layer ) )
    {
        int width = aElem.width + ( layerExpansionMask.second * 2 );
        if( width > 1 )
        {
            PCB_SHAPE* seg = new PCB_SHAPE( m_board, SHAPE_T::SEGMENT );

            seg->SetStart( aElem.start );
            seg->SetEnd( aElem.end );
            seg->SetStroke( STROKE_PARAMS( width, PLOT_DASH_TYPE::SOLID ) );
            seg->SetLayer( layerExpansionMask.first );

            m_board->Add( seg, ADD_MODE::APPEND );
        }
    }
}


void ALTIUM_PCB::ConvertTracks6ToFootprintItem( FOOTPRINT* aFootprint, const ATRACK6& aElem,
                                                const int  aPrimitiveIndex,
                                                const bool aIsBoardImport )
{
    if( aElem.is_polygonoutline || aElem.subpolyindex != ALTIUM_POLYGON_NONE )
        return;

    if( aElem.is_keepout || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER
        || IsAltiumLayerAPlane( aElem.layer ) )
    {
        // This is not the actual board item. We can use it to create the polygon for the region
        PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
        shape.SetStart( aElem.start );
        shape.SetEnd( aElem.end );
        shape.SetStroke( STROKE_PARAMS( aElem.width, PLOT_DASH_TYPE::SOLID ) );

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

    for( const auto layerExpansionMask : HelperGetSolderAndPasteMaskExpansions(
                 ALTIUM_RECORD::TRACK, aPrimitiveIndex, aElem.layer ) )
    {
        int width = aElem.width + ( layerExpansionMask.second * 2 );
        if( width > 1 )
        {
            PCB_SHAPE* seg = new PCB_SHAPE( aFootprint, SHAPE_T::SEGMENT );

            seg->SetStart( aElem.start );
            seg->SetEnd( aElem.end );
            seg->SetStroke( STROKE_PARAMS( width, PLOT_DASH_TYPE::SOLID ) );
            seg->SetLayer( layerExpansionMask.first );

            aFootprint->Add( seg, ADD_MODE::APPEND );
        }
    }
}


void ALTIUM_PCB::ConvertTracks6ToBoardItemOnLayer( const ATRACK6& aElem, PCB_LAYER_ID aLayer )
{
    if( IsCopperLayer( aLayer ) && aElem.net != ALTIUM_NET_UNCONNECTED )
    {
        PCB_TRACK* track = new PCB_TRACK( m_board );

        track->SetStart( aElem.start );
        track->SetEnd( aElem.end );
        track->SetWidth( aElem.width );
        track->SetLayer( aLayer );
        track->SetNetCode( GetNetCode( aElem.net ) );

        m_board->Add( track, ADD_MODE::APPEND );
    }
    else
    {
        PCB_SHAPE* seg = new PCB_SHAPE( m_board, SHAPE_T::SEGMENT );

        seg->SetStart( aElem.start );
        seg->SetEnd( aElem.end );
        seg->SetStroke( STROKE_PARAMS( aElem.width, PLOT_DASH_TYPE::SOLID ) );
        seg->SetLayer( aLayer );

        m_board->Add( seg, ADD_MODE::APPEND );
    }
}


void ALTIUM_PCB::ConvertTracks6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const ATRACK6& aElem,
                                                       PCB_LAYER_ID aLayer )
{
    PCB_SHAPE* seg = new PCB_SHAPE( aFootprint, SHAPE_T::SEGMENT );

    seg->SetStart( aElem.start );
    seg->SetEnd( aElem.end );
    seg->SetStroke( STROKE_PARAMS( aElem.width, PLOT_DASH_TYPE::SOLID ) );
    seg->SetLayer( aLayer );

    aFootprint->Add( seg, ADD_MODE::APPEND );
}


void ALTIUM_PCB::ParseWideStrings6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                        const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading unicode strings..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

    m_unicodeStrings = reader.ReadWideStringTable();

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( wxT( "WideStrings6 stream is not fully parsed" ) );
}

void ALTIUM_PCB::ParseTexts6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading text..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

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
        wxLogError( _( "Ignored barcode on Altium layer %d (not yet supported)." ), aElem.layer );
        return;
    }

    for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
        ConvertTexts6ToBoardItemOnLayer( aElem, klayer );
}


void ALTIUM_PCB::ConvertTexts6ToFootprintItem( FOOTPRINT* aFootprint, const ATEXT6& aElem )
{
    if( aElem.fonttype == ALTIUM_TEXT_TYPE::BARCODE )
    {
        wxLogError( _( "Ignored barcode on Altium layer %d (not yet supported)." ), aElem.layer );
        return;
    }

    for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
        ConvertTexts6ToFootprintItemOnLayer( aFootprint, aElem, klayer );
}


void ALTIUM_PCB::ConvertTexts6ToBoardItemOnLayer( const ATEXT6& aElem, PCB_LAYER_ID aLayer )
{
    PCB_TEXT* pcbText = new PCB_TEXT( m_board );

    // TODO: improve parsing of variables
    wxString trimmedText = aElem.text;
    trimmedText.Trim();

    if( trimmedText.CmpNoCase( wxT( ".Layer_Name" ) ) == 0 )
        pcbText->SetText( wxT( "${LAYER}" ) );
    else
        pcbText->SetText( aElem.text );

    pcbText->SetLayer( aLayer );
    pcbText->SetPosition( aElem.position );
    pcbText->SetTextAngle( EDA_ANGLE( aElem.rotation, DEGREES_T ) );

    ConvertTexts6ToEdaTextSettings( aElem, pcbText );

    m_board->Add( pcbText, ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertTexts6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const ATEXT6& aElem,
                                                      PCB_LAYER_ID aLayer )
{
    PCB_TEXT* fpText;

    if( aElem.isDesignator )
    {
        fpText = &aFootprint->Reference(); // TODO: handle multiple layers
    }
    else if( aElem.isComment )
    {
        fpText = &aFootprint->Value(); // TODO: handle multiple layers
    }
    else
    {
        fpText = new PCB_TEXT( aFootprint, PCB_TEXT::TEXT_is_DIVERS );
        aFootprint->Add( fpText, ADD_MODE::APPEND );
    }

    // TODO: improve parsing of variables
    wxString trimmedText = aElem.text;
    trimmedText.Trim();

    if( !aElem.isDesignator && trimmedText.CmpNoCase( wxT( ".Designator" ) ) == 0 )
        fpText->SetText( wxT( "${REFERENCE}" ) );
    else if( !aElem.isComment && trimmedText.CmpNoCase( wxT( ".Comment" ) ) == 0 )
        fpText->SetText( wxT( "${VALUE}" ) );
    else if( trimmedText.CmpNoCase( wxT( ".Layer_Name" ) ) == 0 )
        fpText->SetText( wxT( "${LAYER}" ) );
    else
        fpText->SetText( aElem.text );

    fpText->SetKeepUpright( true );
    fpText->SetLayer( aLayer );
    fpText->SetPosition( aElem.position );
    fpText->SetTextAngle( EDA_ANGLE( aElem.rotation, DEGREES_T ) );

    ConvertTexts6ToEdaTextSettings( aElem, fpText );
}


void ALTIUM_PCB::ConvertTexts6ToEdaTextSettings( const ATEXT6& aElem, EDA_TEXT* aEdaText )
{
    if( aElem.fonttype == ALTIUM_TEXT_TYPE::TRUETYPE )
    {
        // TODO: why is this required? Somehow, truetype size is calculated differently
        aEdaText->SetTextSize( VECTOR2I( aElem.height / 2, aElem.height / 2 ) );
    }
    else
    {
        aEdaText->SetTextSize( VECTOR2I( aElem.height, aElem.height ) ); // TODO: parse text width
    }

    aEdaText->SetTextThickness( aElem.strokewidth );
    aEdaText->SetBold( aElem.isBold );
    aEdaText->SetItalic( aElem.isItalic );
    aEdaText->SetMirrored( aElem.isMirrored );

    if( aElem.isDesignator || aElem.isComment ) // That's just a bold assumption
    {
        aEdaText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aEdaText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    }
    else
    {
        switch( aElem.textposition )
        {
        case ALTIUM_TEXT_POSITION::LEFT_TOP:
        case ALTIUM_TEXT_POSITION::LEFT_CENTER:
        case ALTIUM_TEXT_POSITION::LEFT_BOTTOM:
            aEdaText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            break;
        case ALTIUM_TEXT_POSITION::CENTER_TOP:
        case ALTIUM_TEXT_POSITION::CENTER_CENTER:
        case ALTIUM_TEXT_POSITION::CENTER_BOTTOM:
            aEdaText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
            break;
        case ALTIUM_TEXT_POSITION::RIGHT_TOP:
        case ALTIUM_TEXT_POSITION::RIGHT_CENTER:
        case ALTIUM_TEXT_POSITION::RIGHT_BOTTOM:
            aEdaText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            break;
        default:
            wxLogError( wxT( "Unexpected horizontal Text Position. This should never happen." ) );
            break;
        }

        switch( aElem.textposition )
        {
        case ALTIUM_TEXT_POSITION::LEFT_TOP:
        case ALTIUM_TEXT_POSITION::CENTER_TOP:
        case ALTIUM_TEXT_POSITION::RIGHT_TOP:
            aEdaText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
            break;
        case ALTIUM_TEXT_POSITION::LEFT_CENTER:
        case ALTIUM_TEXT_POSITION::CENTER_CENTER:
        case ALTIUM_TEXT_POSITION::RIGHT_CENTER:
            aEdaText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
            break;
        case ALTIUM_TEXT_POSITION::LEFT_BOTTOM:
        case ALTIUM_TEXT_POSITION::CENTER_BOTTOM:
        case ALTIUM_TEXT_POSITION::RIGHT_BOTTOM:
            aEdaText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
            break;
        default:
            wxLogError( wxT( "Unexpected vertical text position. This should never happen." ) );
            break;
        }
    }
}


void ALTIUM_PCB::ParseFills6Data( const ALTIUM_COMPOUND_FILE&     aAltiumPcbFile,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading rectangles..." ) );

    ALTIUM_PARSER reader( aAltiumPcbFile, aEntry );

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
    if( aElem.is_keepout || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER
        || aElem.net != ALTIUM_NET_UNCONNECTED )
    {
        ConvertFills6ToBoardItemWithNet( aElem );
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
        PCB_SHAPE shape( nullptr, SHAPE_T::RECT );
        shape.SetStart( aElem.pos1 );
        shape.SetEnd( aElem.pos2 );
        shape.SetStroke( STROKE_PARAMS( 0, PLOT_DASH_TYPE::SOLID ) );

        if( aElem.rotation != 0. )
        {
            VECTOR2I center( ( aElem.pos1.x + aElem.pos2.x ) / 2,
                             ( aElem.pos1.y + aElem.pos2.y ) / 2 );
            shape.Rotate( center, EDA_ANGLE( aElem.rotation, DEGREES_T ) );
        }

        HelperPcpShapeAsFootprintKeepoutRegion( aFootprint, shape, aElem.layer,
                                                aElem.keepoutrestrictions );
    }
    else if( aIsBoardImport && IsAltiumLayerCopper( aElem.layer )
             && aElem.net != ALTIUM_NET_UNCONNECTED )
    {
        // Special case: do to not lose net connections in footprints
        ConvertFills6ToBoardItemWithNet( aElem );
    }
    else
    {
        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aElem.layer ) )
            ConvertFills6ToFootprintItemOnLayer( aFootprint, aElem, klayer );
    }
}


void ALTIUM_PCB::ConvertFills6ToBoardItemWithNet( const AFILL6& aElem )
{
    ZONE* zone = new ZONE( m_board );
    m_board->Add( zone, ADD_MODE::APPEND );

    zone->SetNetCode( GetNetCode( aElem.net ) );

    zone->SetPosition( aElem.pos1 );
    zone->SetAssignedPriority( 1000 );

    HelperSetZoneLayers( zone, aElem.layer );

    VECTOR2I p11( aElem.pos1.x, aElem.pos1.y );
    VECTOR2I p12( aElem.pos1.x, aElem.pos2.y );
    VECTOR2I p22( aElem.pos2.x, aElem.pos2.y );
    VECTOR2I p21( aElem.pos2.x, aElem.pos1.y );

    VECTOR2I center( ( aElem.pos1.x + aElem.pos2.x ) / 2, ( aElem.pos1.y + aElem.pos2.y ) / 2 );

    const int outlineIdx = -1; // this is the id of the copper zone main outline
    zone->AppendCorner( p11, outlineIdx );
    zone->AppendCorner( p12, outlineIdx );
    zone->AppendCorner( p22, outlineIdx );
    zone->AppendCorner( p21, outlineIdx );

    // should be correct?
    zone->SetLocalClearance( 0 );
    zone->SetPadConnection( ZONE_CONNECTION::FULL );

    if( aElem.is_keepout || aElem.layer == ALTIUM_LAYER::KEEP_OUT_LAYER )
    {
        zone->SetIsRuleArea( true );

        HelperSetZoneKeepoutRestrictions( zone, aElem.keepoutrestrictions );
    }

    if( aElem.rotation != 0. )
        zone->Rotate( center, EDA_ANGLE( aElem.rotation, DEGREES_T ) );

    zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                 ZONE::GetDefaultHatchPitch(), true );
}


void ALTIUM_PCB::ConvertFills6ToBoardItemOnLayer( const AFILL6& aElem, PCB_LAYER_ID aLayer )
{
    PCB_SHAPE* fill = new PCB_SHAPE( m_board, SHAPE_T::RECT );

    fill->SetFilled( true );
    fill->SetLayer( aLayer );
    fill->SetStroke( STROKE_PARAMS( 0 ) );

    fill->SetStart( aElem.pos1 );
    fill->SetEnd( aElem.pos2 );

    if( aElem.rotation != 0. )
    {
        // TODO: Do we need SHAPE_T::POLY for non 90° rotations?
        VECTOR2I center( ( aElem.pos1.x + aElem.pos2.x ) / 2, ( aElem.pos1.y + aElem.pos2.y ) / 2 );
        fill->Rotate( center, EDA_ANGLE( aElem.rotation, DEGREES_T ) );
    }

    m_board->Add( fill, ADD_MODE::APPEND );
}


void ALTIUM_PCB::ConvertFills6ToFootprintItemOnLayer( FOOTPRINT* aFootprint, const AFILL6& aElem,
                                                      PCB_LAYER_ID aLayer )
{
    PCB_SHAPE* fill = new PCB_SHAPE( aFootprint, SHAPE_T::RECT );

    fill->SetFilled( true );
    fill->SetLayer( aLayer );
    fill->SetStroke( STROKE_PARAMS( 0 ) );

    fill->SetStart( aElem.pos1 );
    fill->SetEnd( aElem.pos2 );

    if( aElem.rotation != 0. )
    {
        // TODO: Do we need SHAPE_T::POLY for non 90° rotations?
        VECTOR2I center( ( aElem.pos1.x + aElem.pos2.x ) / 2, ( aElem.pos1.y + aElem.pos2.y ) / 2 );
        fill->Rotate( center, EDA_ANGLE( aElem.rotation, DEGREES_T ) );
    }

    aFootprint->Add( fill, ADD_MODE::APPEND );
}


void ALTIUM_PCB::HelperSetZoneLayers( ZONE* aZone, const ALTIUM_LAYER aAltiumLayer )
{
    if( aAltiumLayer == ALTIUM_LAYER::MULTI_LAYER || aAltiumLayer == ALTIUM_LAYER::KEEP_OUT_LAYER )
    {
        aZone->SetLayerSet( LSET::AllCuMask() );
    }
    else
    {
        LSET layerSet;

        for( PCB_LAYER_ID klayer : GetKicadLayersToIterate( aAltiumLayer ) )
            layerSet.set( klayer );

        aZone->SetLayerSet( layerSet );
    }
}


void ALTIUM_PCB::HelperSetZoneKeepoutRestrictions( ZONE* aZone, const uint8_t aKeepoutRestrictions )
{
    bool keepoutRestrictionVia = ( aKeepoutRestrictions & 0x01 ) != 0;
    bool keepoutRestrictionTrack = ( aKeepoutRestrictions & 0x02 ) != 0;
    bool keepoutRestrictionCopper = ( aKeepoutRestrictions & 0x04 ) != 0;
    bool keepoutRestrictionSMDPad = ( aKeepoutRestrictions & 0x08 ) != 0;
    bool keepoutRestrictionTHPad = ( aKeepoutRestrictions & 0x10 ) != 0;

    aZone->SetDoNotAllowVias( keepoutRestrictionVia );
    aZone->SetDoNotAllowTracks( keepoutRestrictionTrack );
    aZone->SetDoNotAllowCopperPour( keepoutRestrictionCopper );
    aZone->SetDoNotAllowPads( keepoutRestrictionSMDPad && keepoutRestrictionTHPad );
    aZone->SetDoNotAllowFootprints( false );
}


void ALTIUM_PCB::HelperPcpShapeAsBoardKeepoutRegion( const PCB_SHAPE&   aShape,
                                                     const ALTIUM_LAYER aAltiumLayer,
                                                     const uint8_t      aKeepoutRestrictions )
{
    ZONE* zone = new ZONE( m_board );

    zone->SetIsRuleArea( true );

    HelperSetZoneLayers( zone, aAltiumLayer );
    HelperSetZoneKeepoutRestrictions( zone, aKeepoutRestrictions );

    aShape.EDA_SHAPE::TransformShapeToPolygon( *zone->Outline(), 0, ARC_HIGH_DEF, ERROR_INSIDE );

    zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                 ZONE::GetDefaultHatchPitch(), true );

    m_board->Add( zone, ADD_MODE::APPEND );
}


void ALTIUM_PCB::HelperPcpShapeAsFootprintKeepoutRegion( FOOTPRINT*         aFootprint,
                                                         const PCB_SHAPE&   aShape,
                                                         const ALTIUM_LAYER aAltiumLayer,
                                                         const uint8_t      aKeepoutRestrictions )
{
    ZONE* zone = new ZONE( aFootprint );

    zone->SetIsRuleArea( true );

    HelperSetZoneLayers( zone, aAltiumLayer );
    HelperSetZoneKeepoutRestrictions( zone, aKeepoutRestrictions );

    aShape.EDA_SHAPE::TransformShapeToPolygon( *zone->Outline(), 0, ARC_HIGH_DEF, ERROR_INSIDE );

    zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                 ZONE::GetDefaultHatchPitch(), true );

    // TODO: zone->SetLocalCoord(); missing?
    aFootprint->Add( zone, ADD_MODE::APPEND );
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
