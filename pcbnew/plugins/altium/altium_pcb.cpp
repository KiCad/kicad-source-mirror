/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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
#include <kicad_string.h>

#include <fp_shape.h>
#include <fp_text.h>
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
#include <widgets/progress_reporter.h>


void ParseAltiumPcb( BOARD* aBoard, const wxString& aFileName, PROGRESS_REPORTER* aProgressReporter,
                     const std::map<ALTIUM_PCB_DIR, std::string>& aFileMapping )
{
    // Open file
    FILE* fp = wxFopen( aFileName, "rb" );

    if( fp == nullptr )
    {
        wxLogError( _( "Cannot open file '%s'." ), aFileName );
        return;
    }

    fseek( fp, 0, SEEK_END );
    long len = ftell( fp );

    if( len < 0 )
    {
        fclose( fp );
        THROW_IO_ERROR( _( "Error reading file: cannot determine length." ) );
    }

    std::unique_ptr<unsigned char[]> buffer( new unsigned char[len] );
    fseek( fp, 0, SEEK_SET );

    size_t bytesRead = fread( buffer.get(), sizeof( unsigned char ), len, fp );
    fclose( fp );

    if( static_cast<size_t>( len ) != bytesRead )
    {
        THROW_IO_ERROR( _( "Error reading file." ) );
    }

    try
    {
        CFB::CompoundFileReader reader( buffer.get(), bytesRead );

        // Parse File
        ALTIUM_PCB pcb( aBoard, aProgressReporter );
        pcb.Parse( reader, aFileMapping );
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }
}


bool IsAltiumLayerCopper( ALTIUM_LAYER aLayer )
{
    return aLayer >= ALTIUM_LAYER::TOP_LAYER && aLayer <= ALTIUM_LAYER::BOTTOM_LAYER;
}


bool IsAltiumLayerAPlane( ALTIUM_LAYER aLayer )
{
    return aLayer >= ALTIUM_LAYER::INTERNAL_PLANE_1 && aLayer <= ALTIUM_LAYER::INTERNAL_PLANE_16;
}


PCB_SHAPE* ALTIUM_PCB::HelperCreateAndAddDrawsegment( uint16_t aComponent )
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
            THROW_IO_ERROR( wxString::Format( "Component creator tries to access component id %d "
                                              "of %d existing components",
                                              aComponent,
                                              m_components.size() ) );
        }

        FOOTPRINT* footprint = m_components.at( aComponent );
        PCB_SHAPE* fpShape = new FP_SHAPE( footprint );

        footprint->Add( fpShape, ADD_MODE::APPEND );
        return fpShape;
    }
}


void ALTIUM_PCB::HelperDrawsegmentSetLocalCoord( PCB_SHAPE* aShape, uint16_t aComponent )
{
    if( aComponent != ALTIUM_COMPONENT_NONE )
    {
        FP_SHAPE* fpShape = dynamic_cast<FP_SHAPE*>( aShape );

        if( fpShape )
        {
            fpShape->SetLocalCoord();

            // TODO: SetLocalCoord() does not update the polygon shape!
            // This workaround converts the poly shape into the local coordinates
            SHAPE_POLY_SET& polyShape = fpShape->GetPolyShape();
            if( !polyShape.IsEmpty() )
            {
                FOOTPRINT* fp = m_components.at( aComponent );

                polyShape.Move( -fp->GetPosition() );
                polyShape.Rotate( -fp->GetOrientationRadians() );
            }
        }
    }
}


void HelperShapeLineChainFromAltiumVertices(
        SHAPE_LINE_CHAIN& aLine, const std::vector<ALTIUM_VERTICE>& aVertices )
{
    for( auto& vertice : aVertices )
    {
        if( vertice.isRound )
        {
            double angle = NormalizeAngleDegreesPos( vertice.endangle - vertice.startangle );

            double  startradiant   = DEG2RAD( vertice.startangle );
            double  endradiant     = DEG2RAD( vertice.endangle );
            wxPoint arcStartOffset = wxPoint( KiROUND( std::cos( startradiant ) * vertice.radius ),
                    -KiROUND( std::sin( startradiant ) * vertice.radius ) );

            wxPoint arcEndOffset = wxPoint( KiROUND( std::cos( endradiant ) * vertice.radius ),
                    -KiROUND( std::sin( endradiant ) * vertice.radius ) );

            wxPoint arcStart = vertice.center + arcStartOffset;
            wxPoint arcEnd   = vertice.center + arcEndOffset;

            if( GetLineLength( arcStart, vertice.position )
                    < GetLineLength( arcEnd, vertice.position ) )
            {
                aLine.Append( SHAPE_ARC( vertice.center, arcStart, -angle ) );
            }
            else
            {
                aLine.Append( SHAPE_ARC( vertice.center, arcEnd, angle ) );
            }
        }
        else
        {
            aLine.Append( vertice.position );
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
    case ALTIUM_LAYER::MECHANICAL_11:     return Eco1_User;
    case ALTIUM_LAYER::MECHANICAL_12:     return Eco2_User;
    case ALTIUM_LAYER::MECHANICAL_13:     return F_Fab;
    case ALTIUM_LAYER::MECHANICAL_14:     return B_Fab;
    case ALTIUM_LAYER::MECHANICAL_15:     return F_CrtYd;
    case ALTIUM_LAYER::MECHANICAL_16:     return B_CrtYd;

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


ALTIUM_PCB::ALTIUM_PCB( BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter )
{
    m_board              = aBoard;
    m_progressReporter = aProgressReporter;
    m_doneCount = 0;
    m_lastProgressCount = 0;
    m_totalCount = 0;
    m_num_nets           = 0;
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
                THROW_IO_ERROR( ( "Open cancelled by user." ) );

            m_lastProgressCount = m_doneCount;
        }
    }
}

void ALTIUM_PCB::Parse( const CFB::CompoundFileReader& aReader,
                        const std::map<ALTIUM_PCB_DIR, std::string>&   aFileMapping )
{
    // this vector simply declares in which order which functions to call.
    const std::vector<std::tuple<bool, ALTIUM_PCB_DIR, PARSE_FUNCTION_POINTER_fp>> parserOrder = {
        { true, ALTIUM_PCB_DIR::FILE_HEADER,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseFileHeader( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::BOARD6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseBoard6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::COMPONENTS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseComponents6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::MODELS,
                [this, aFileMapping]( auto aReader, auto fileHeader ) {
                    wxString dir( aFileMapping.at( ALTIUM_PCB_DIR::MODELS ) );
                    this->ParseModelsData( aReader, fileHeader, dir );
                } },
        { true, ALTIUM_PCB_DIR::COMPONENTBODIES6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseComponentsBodies6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::NETS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseNets6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::CLASSES6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseClasses6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::RULES6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseRules6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::DIMENSIONS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseDimensions6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::POLYGONS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParsePolygons6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::ARCS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseArcs6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::PADS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParsePads6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::VIAS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseVias6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::TRACKS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseTracks6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::TEXTS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseTexts6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::FILLS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseFills6Data( aReader, fileHeader );
                } },
        { false, ALTIUM_PCB_DIR::BOARDREGIONS,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseBoardRegionsData( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::SHAPEBASEDREGIONS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseShapeBasedRegions6Data( aReader, fileHeader );
                } },
        { true, ALTIUM_PCB_DIR::REGIONS6,
                [this]( auto aReader, auto fileHeader ) {
                    this->ParseRegions6Data( aReader, fileHeader );
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
            {
                continue;
            }

            const auto& mappedDirectory = aFileMapping.find( directory );
            if( mappedDirectory == aFileMapping.end() )
            {
                continue;
            }

            std::string mappedFile = mappedDirectory->second + "Header";

            const CFB::COMPOUND_FILE_ENTRY* file = FindStream( aReader, mappedFile.c_str() );
            if( file == nullptr )
            {
                continue;
            }

            ALTIUM_PARSER reader( aReader, file );
            uint32_t      numOfRecords = reader.Read<uint32_t>();

            if( reader.HasParsingError() )
            {
                wxLogError( _( "'%s' was not parsed correctly." ), mappedFile );
                continue;
            }

            m_totalCount += numOfRecords;

            if( reader.GetRemainingBytes() != 0 )
            {
                wxLogError( _( "'%s' was not fully parsed." ), mappedFile );
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
            wxASSERT_MSG( !isRequired, wxString::Format( "Altium Directory of kind %d was expected, "
                                                         "but no mapping is present in the code",
                                                         directory ) );
            continue;
        }

        std::string mappedFile = mappedDirectory->second;
        if( directory != ALTIUM_PCB_DIR::FILE_HEADER )
        {
            mappedFile += "Data";
        }

        const CFB::COMPOUND_FILE_ENTRY* file = FindStream( aReader, mappedFile.c_str() );
        if( file != nullptr )
        {
            fp( aReader, file );
        }
        else if( isRequired )
        {
            wxLogError( _( "File not found: '%s'." ), mappedFile );
        }
    }

    // fixup zone priorities since Altium stores them in the opposite order
    for( auto& zone : m_polygons )
    {
        if( !zone )
            continue;

        // Altium "fills" - not poured in Altium
        if( zone->GetPriority() == 1000 )
        {
            // Unlikely, but you never know
            if( m_highest_pour_index >= 1000 )
                zone->SetPriority( m_highest_pour_index + 1 );

            continue;
        }

        int priority = m_highest_pour_index - zone->GetPriority();

        zone->SetPriority( priority >= 0 ? priority : 0 );
    }

    // change priority of outer zone to zero
    for( auto& zone : m_outer_plane )
    {
        zone.second->SetPriority( 0 );
    }

    // center board
    EDA_RECT bbbox = m_board->GetBoardEdgesBoundingBox();

    int w = m_board->GetPageSettings().GetWidthIU();
    int h = m_board->GetPageSettings().GetHeightIU();

    int desired_x = ( w - bbbox.GetWidth() ) / 2;
    int desired_y = ( h - bbbox.GetHeight() ) / 2;

    wxPoint movementVector( desired_x - bbbox.GetX(), desired_y - bbbox.GetY() );
    m_board->Move( movementVector );
    m_board->GetDesignSettings().m_AuxOrigin += movementVector;
    m_board->GetDesignSettings().m_GridOrigin += movementVector;

    m_board->SetModified();
}

int ALTIUM_PCB::GetNetCode( uint16_t aId ) const
{
    if( aId == ALTIUM_NET_UNCONNECTED )
    {
        return NETINFO_LIST::UNCONNECTED;
    }
    else if( m_num_nets < aId )
    {
        THROW_IO_ERROR( wxString::Format(
                "Netcode with id %d does not exist. Only %d nets are known", aId, m_num_nets ) );
    }
    else
    {
        return aId + 1;
    }
}

const ARULE6* ALTIUM_PCB::GetRule( ALTIUM_RULE_KIND aKind, const wxString& aName ) const
{
    const auto rules = m_rules.find( aKind );
    if( rules == m_rules.end() )
    {
        return nullptr;
    }
    for( const ARULE6& rule : rules->second )
    {
        if( rule.name == aName )
        {
            return &rule;
        }
    }
    return nullptr;
}

const ARULE6* ALTIUM_PCB::GetRuleDefault( ALTIUM_RULE_KIND aKind ) const
{
    const auto rules = m_rules.find( aKind );
    if( rules == m_rules.end() )
    {
        return nullptr;
    }
    for( const ARULE6& rule : rules->second )
    {
        if( rule.scope1expr == "All" && rule.scope2expr == "All" )
        {
            return &rule;
        }
    }
    return nullptr;
}

void ALTIUM_PCB::ParseFileHeader( const CFB::CompoundFileReader& aReader,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    reader.ReadAndSetSubrecordLength();
    wxString header = reader.ReadWxString();

    //std::cout << "HEADER: " << header << std::endl;  // tells me: PCB 5.0 Binary File

    //reader.SkipSubrecord();

    // TODO: does not seem to work all the time at the moment
    //if( reader.GetRemainingBytes() != 0 )
    //{
    //    THROW_IO_ERROR( "FileHeader stream is not fully parsed" );
    //}
}

void ALTIUM_PCB::ParseBoard6Data( const CFB::CompoundFileReader& aReader,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( "Loading board data..." );

    ALTIUM_PARSER reader( aReader, aEntry );

    checkpoint();
    ABOARD6 elem( reader );

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Board6 stream is not fully parsed" );
    }

    m_board->GetDesignSettings().m_AuxOrigin = elem.sheetpos;
    m_board->GetDesignSettings().m_GridOrigin = elem.sheetpos;

    // read layercount from stackup, because LAYERSETSCOUNT is not always correct?!
    size_t layercount = 0;
    for( size_t i                                = static_cast<size_t>( ALTIUM_LAYER::TOP_LAYER );
            i < elem.stackup.size() && i != 0; i = elem.stackup[i - 1].nextId, layercount++ )
        ;
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
            m_board->SetLayerName( ( *it )->GetBrdLayerId(), "[unused]" );

            if( ( *it )->GetType() != BS_ITEM_TYPE_COPPER )
            {
                THROW_IO_ERROR( "Board6 stream, unexpected item while parsing stackup" );
            }
            ( *it )->SetThickness( 0 );

            ++it;
            if( ( *it )->GetType() != BS_ITEM_TYPE_DIELECTRIC )
            {
                THROW_IO_ERROR( "Board6 stream, unexpected item while parsing stackup" );
            }
            ( *it )->SetThickness( 0, 0 );
            ( *it )->SetThicknessLocked( true, 0 );
            ++it;
        }

        m_layermap.insert( { static_cast<ALTIUM_LAYER>( altiumLayerId ),
                static_cast<PCB_LAYER_ID>( curLayer++ ) } );

        if( ( *it )->GetType() != BS_ITEM_TYPE_COPPER )
        {
            THROW_IO_ERROR( "Board6 stream, unexpected item while parsing stackup" );
        }
        ( *it )->SetThickness( layer.copperthick );

        ALTIUM_LAYER alayer = static_cast<ALTIUM_LAYER>( altiumLayerId );
        PCB_LAYER_ID klayer = ( *it )->GetBrdLayerId();

        m_board->SetLayerName( klayer, layer.name );

        if( layer.copperthick == 0 )
        {
            m_board->SetLayerType( klayer, LAYER_T::LT_JUMPER ); // used for things like wirebonding
        }
        else if( IsAltiumLayerAPlane( alayer ) )
        {
            m_board->SetLayerType( klayer, LAYER_T::LT_POWER );
        }

        if( klayer == B_Cu )
        {
            if( layer.nextId != 0 )
            {
                THROW_IO_ERROR( "Board6 stream, unexpected id while parsing last stackup layer" );
            }
            // overwrite entry from internal -> bottom
            m_layermap[alayer] = B_Cu;
            break;
        }

        ++it;
        if( ( *it )->GetType() != BS_ITEM_TYPE_DIELECTRIC )
        {
            THROW_IO_ERROR( "Board6 stream, unexpected item while parsing stackup" );
        }
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
    if( !aVertices.empty() )
    {
        const ALTIUM_VERTICE* last = &aVertices.at( 0 );
        for( size_t i = 0; i < aVertices.size(); i++ )
        {
            const ALTIUM_VERTICE* cur = &aVertices.at( ( i + 1 ) % aVertices.size() );

            PCB_SHAPE* shape = new PCB_SHAPE( m_board );
            m_board->Add( shape, ADD_MODE::APPEND );

            shape->SetWidth( m_board->GetDesignSettings().GetLineThickness( Edge_Cuts ) );
            shape->SetLayer( Edge_Cuts );

            if( !last->isRound && !cur->isRound )
            {
                shape->SetShape( PCB_SHAPE_TYPE::SEGMENT );
                shape->SetStart( last->position );
                shape->SetEnd( cur->position );
            }
            else if( cur->isRound )
            {
                shape->SetShape( PCB_SHAPE_TYPE::ARC );
                shape->SetAngle( -NormalizeAngleDegreesPos( cur->endangle - cur->startangle ) * 10. );

                double  startradiant   = DEG2RAD( cur->startangle );
                wxPoint arcStartOffset = wxPoint( KiROUND( std::cos( startradiant ) * cur->radius ),
                        -KiROUND( std::sin( startradiant ) * cur->radius ) );
                wxPoint arcStart       = cur->center + arcStartOffset;
                shape->SetCenter( cur->center );
                shape->SetArcStart( arcStart );

                if( !last->isRound )
                {
                    double  endradiant   = DEG2RAD( cur->endangle );
                    wxPoint arcEndOffset = wxPoint( KiROUND( std::cos( endradiant ) * cur->radius ),
                            -KiROUND( std::sin( endradiant ) * cur->radius ) );
                    wxPoint arcEnd       = cur->center + arcEndOffset;

                    PCB_SHAPE* shape2 = new PCB_SHAPE( m_board );
                    shape2->SetShape( PCB_SHAPE_TYPE::SEGMENT );
                    m_board->Add( shape2, ADD_MODE::APPEND );
                    shape2->SetWidth( m_board->GetDesignSettings().GetLineThickness( Edge_Cuts ) );
                    shape2->SetLayer( Edge_Cuts );
                    shape2->SetStart( last->position );

                    // TODO: this is more of a hack than the real solution
                    double lineLengthStart = GetLineLength( last->position, arcStart );
                    double lineLengthEnd   = GetLineLength( last->position, arcEnd );
                    if( lineLengthStart > lineLengthEnd )
                    {
                        shape2->SetEnd( cur->center + arcEndOffset );
                    }
                    else
                    {
                        shape2->SetEnd( cur->center + arcStartOffset );
                    }
                }
            }
            last = cur;
        }
    }
}

void ALTIUM_PCB::ParseClasses6Data( const CFB::CompoundFileReader& aReader,
                                    const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( "Loading netclasses..." );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ACLASS6 elem( reader );

        if( elem.kind == ALTIUM_CLASS_KIND::NET_CLASS )
        {
            NETCLASSPTR nc = std::make_shared<NETCLASS>( elem.name );

            for( const auto& name : elem.names )
            {
                // TODO: it seems it can happen that we have names not attached to any net.
                nc->Add( name );
            }

            if( !m_board->GetDesignSettings().GetNetClasses().Add( nc ) )
            {
                // Name conflict, this is likely a bad board file.
                // unique_ptr will delete nc on this code path
                THROW_IO_ERROR( wxString::Format( _( "Duplicated Netclass name \"%s\"" ), elem.name ) );
            }
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Classes6 stream is not fully parsed" );
    }

    m_board->m_LegacyNetclassesLoaded = true;
}

void ALTIUM_PCB::ParseComponents6Data( const CFB::CompoundFileReader& aReader,
                                       const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( "Loading components..." );

    ALTIUM_PARSER reader( aReader, aEntry );

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
            reference.Prepend( "UNK" );
        footprint->SetReference( reference );

        footprint->SetLocked( elem.locked );
        footprint->Reference().SetVisible( elem.nameon );
        footprint->Value().SetVisible( elem.commenton );
        footprint->SetLayer( elem.layer == ALTIUM_LAYER::TOP_LAYER ? F_Cu : B_Cu );

        componentId++;
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Components6 stream is not fully parsed" );
    }
}


void ALTIUM_PCB::ParseComponentsBodies6Data( const CFB::CompoundFileReader& aReader,
                                             const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( "Loading component 3D models..." );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ACOMPONENTBODY6 elem( reader ); // TODO: implement

        if( elem.component == ALTIUM_COMPONENT_NONE )
        {
            continue; // TODO: we do not support components for the board yet
        }

        if( m_components.size() <= elem.component )
        {
            THROW_IO_ERROR( wxString::Format(
                    "ComponentsBodies6 stream tries to access component id %d of %d existing components",
                    elem.component, m_components.size() ) );
        }

        if( !elem.modelIsEmbedded )
        {
            continue;
        }

        auto modelTuple = m_models.find( elem.modelId );
        if( modelTuple == m_models.end() )
        {
            THROW_IO_ERROR( wxString::Format(
                    "ComponentsBodies6 stream tries to access model id %s which does not exist",
                    elem.modelId ) );
        }

        FOOTPRINT*     footprint  = m_components.at( elem.component );
        const wxPoint& fpPosition = footprint->GetPosition();

        FP_3DMODEL modelSettings;

        modelSettings.m_Filename = modelTuple->second;

        modelSettings.m_Offset.x = Iu2Millimeter((int) elem.modelPosition.x - fpPosition.x );
        modelSettings.m_Offset.y = -Iu2Millimeter((int) elem.modelPosition.y - fpPosition.y );
        modelSettings.m_Offset.z = Iu2Millimeter( (int) elem.modelPosition.z );

        double orientation = footprint->GetOrientation();

        if( footprint->IsFlipped() )
        {
            modelSettings.m_Offset.y = -modelSettings.m_Offset.y;
            orientation              = -orientation;
        }

        RotatePoint( &modelSettings.m_Offset.x, &modelSettings.m_Offset.y, orientation );

        modelSettings.m_Rotation.x = NormalizeAngleDegrees( -elem.modelRotation.x, -180, 180 );
        modelSettings.m_Rotation.y = NormalizeAngleDegrees( -elem.modelRotation.y, -180, 180 );
        modelSettings.m_Rotation.z = NormalizeAngleDegrees(
                -elem.modelRotation.z + elem.rotation + orientation / 10, -180, 180 );

        modelSettings.m_Opacity = elem.bodyOpacity;

        footprint->Models().push_back( modelSettings );
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "ComponentsBodies6 stream is not fully parsed" );
    }
}


void ALTIUM_PCB::HelperParseDimensions6Linear( const ADIMENSION6& aElem )
{
    if( aElem.referencePoint.size() != 2 )
    {
        THROW_IO_ERROR( "Incorrect number of reference points for linear dimension object" );
    }

    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        wxLogWarning( _( "Dimension found on an Altium layer (%d) with no KiCad equivalent. "
                         "It has been moved to KiCad layer Eco1_User." ),
                      aElem.layer );
        klayer = Eco1_User;
    }

    wxPoint referencePoint0 = aElem.referencePoint.at( 0 );
    wxPoint referencePoint1 = aElem.referencePoint.at( 1 );

    PCB_DIM_ALIGNED* dimension = new PCB_DIM_ALIGNED( m_board );
    m_board->Add( dimension, ADD_MODE::APPEND );

    dimension->SetPrecision( aElem.textprecision );
    dimension->SetLayer( klayer );
    dimension->SetStart( referencePoint0 );

    if( referencePoint0 != aElem.xy1 )
    {
        /**
         * Basically REFERENCE0POINT and REFERENCE1POINT are the two end points of the dimension.
         * XY1 is the position of the arrow above REFERENCE0POINT. those three points are not necessarily
         * in 90degree angle, but KiCad requires this to show the correct measurements.
         *
         * Therefore, we take the vector of REFERENCE0POINT -> XY1, calculate the normal, and intersect it with
         * REFERENCE1POINT pointing the same direction as REFERENCE0POINT -> XY1. This should give us a valid
         * measurement point where we can place the drawsegment.
         */
        wxPoint direction             = aElem.xy1 - referencePoint0;
        wxPoint directionNormalVector = wxPoint( -direction.y, direction.x );
        SEG     segm1( referencePoint0, referencePoint0 + directionNormalVector );
        SEG     segm2( referencePoint1, referencePoint1 + direction );
        wxPoint intersection( segm1.Intersect( segm2, true, true ).get() );
        dimension->SetEnd( intersection );

        int height = static_cast<int>( EuclideanNorm( direction ) );

        if( direction.x <= 0 && direction.y <= 0 ) // TODO: I suspect this is not always correct
        {
            height = -height;
        }

        dimension->SetHeight( height );
    }
    else
    {
        dimension->SetEnd( referencePoint1 );
    }

    dimension->SetLineThickness( aElem.linewidth );

    dimension->Text().SetTextThickness( aElem.textlinewidth );
    dimension->Text().SetTextSize( wxSize( aElem.textheight, aElem.textheight ) );
    dimension->Text().SetBold( aElem.textbold );
    dimension->Text().SetItalic( aElem.textitalic );

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
        wxPoint referencePoint0 = aElem.referencePoint.at( 0 );

        // line
        wxPoint last = referencePoint0;
        for( size_t i = 1; i < aElem.referencePoint.size(); i++ )
        {
            PCB_SHAPE* shape = new PCB_SHAPE( m_board );
            m_board->Add( shape, ADD_MODE::APPEND );
            shape->SetShape( PCB_SHAPE_TYPE::SEGMENT );
            shape->SetLayer( klayer );
            shape->SetWidth( aElem.linewidth );
            shape->SetStart( last );
            shape->SetEnd( aElem.referencePoint.at( i ) );
            last = aElem.referencePoint.at( i );
        }

        // arrow
        if( aElem.referencePoint.size() >= 2 )
        {
            wxPoint dirVec = aElem.referencePoint.at( 1 ) - referencePoint0;
            if( dirVec.x != 0 || dirVec.y != 0 )
            {
                double  scaling = EuclideanNorm( dirVec ) / aElem.arrowsize;
                wxPoint arrVec =
                        wxPoint( KiROUND( dirVec.x / scaling ), KiROUND( dirVec.y / scaling ) );
                RotatePoint( &arrVec, 200. );

                PCB_SHAPE* shape1 = new PCB_SHAPE( m_board );
                m_board->Add( shape1, ADD_MODE::APPEND );
                shape1->SetShape( PCB_SHAPE_TYPE::SEGMENT );
                shape1->SetLayer( klayer );
                shape1->SetWidth( aElem.linewidth );
                shape1->SetStart( referencePoint0 );
                shape1->SetEnd( referencePoint0 + arrVec );

                RotatePoint( &arrVec, -400. );

                PCB_SHAPE* shape2 = new PCB_SHAPE( m_board );
                m_board->Add( shape2, ADD_MODE::APPEND );
                shape2->SetShape( PCB_SHAPE_TYPE::SEGMENT );
                shape2->SetLayer( klayer );
                shape2->SetWidth( aElem.linewidth );
                shape2->SetStart( referencePoint0 );
                shape2->SetEnd( referencePoint0 + arrVec );
            }
        }
    }

    if( aElem.textPoint.empty() )
    {
        wxLogError( "No text position present for leader dimension object" );
        return;
    }

    PCB_TEXT* text = new PCB_TEXT( m_board );
    m_board->Add( text, ADD_MODE::APPEND );
    text->SetText( aElem.textformat );
    text->SetPosition( aElem.textPoint.at( 0 ) );
    text->SetLayer( klayer );
    text->SetTextSize( wxSize( aElem.textheight, aElem.textheight ) ); // TODO: parse text width
    text->SetTextThickness( aElem.textlinewidth );
    text->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT );
    text->SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_BOTTOM );
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
        PCB_SHAPE* shape = new PCB_SHAPE( m_board );
        m_board->Add( shape, ADD_MODE::APPEND );
        shape->SetShape( PCB_SHAPE_TYPE::SEGMENT );
        shape->SetLayer( klayer );
        shape->SetWidth( aElem.linewidth );
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

    wxPoint vec = wxPoint( 0, aElem.height / 2 );
    RotatePoint( &vec, aElem.angle * 10. );

    PCB_DIM_CENTER* dimension = new PCB_DIM_CENTER( m_board );
    m_board->Add( dimension, ADD_MODE::APPEND );
    dimension->SetLayer( klayer );
    dimension->SetLineThickness( aElem.linewidth );
    dimension->SetStart( aElem.xy1 );
    dimension->SetEnd( aElem.xy1 + vec );
}


void ALTIUM_PCB::ParseDimensions6Data( const CFB::CompoundFileReader& aReader,
                                       const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( "Loading dimension drawings..." );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ADIMENSION6 elem( reader );

        switch( elem.kind )
        {
        case ALTIUM_DIMENSION_KIND::LINEAR:
            HelperParseDimensions6Linear( elem );
            break;
        case ALTIUM_DIMENSION_KIND::LEADER:
            HelperParseDimensions6Leader( elem );
            break;
        case ALTIUM_DIMENSION_KIND::DATUM:
            wxLogError( _( "Ignored dimension of kind %d (not yet supported)." ),
                        elem.kind );
            // HelperParseDimensions6Datum( elem );
            break;
        case ALTIUM_DIMENSION_KIND::CENTER:
            HelperParseDimensions6Center( elem );
            break;
        default:
            wxLogError( _( "Ignored dimension of kind %d (not yet supported)." ),
                        elem.kind );
            break;
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Dimensions6 stream is not fully parsed" );
    }
}


void ALTIUM_PCB::ParseModelsData( const CFB::CompoundFileReader& aReader,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry, const wxString aRootDir )
{
    if( m_progressReporter )
        m_progressReporter->Report( "Loading 3D models..." );

    ALTIUM_PARSER reader( aReader, aEntry );

    if( reader.GetRemainingBytes() == 0 )
    {
        return; // fast path: no 3d-models present which need to be imported -> no directory needs to be created
    }

    wxString projectPath = wxPathOnly( m_board->GetFileName() );
    // TODO: set KIPRJMOD always after import (not only when loading project)?
    wxSetEnv( PROJECT_VAR_NAME, projectPath );

    // TODO: make this path configurable?
    const wxString altiumModelDir = "ALTIUM_EMBEDDED_MODELS";

    wxFileName altiumModelsPath = wxFileName::DirName( projectPath );
    wxString   kicadModelPrefix = "${KIPRJMOD}/" + altiumModelDir + "/";
    if( !altiumModelsPath.AppendDir( altiumModelDir ) )
    {
        THROW_IO_ERROR( "Cannot construct directory path for step models" );
    }

    // Create dir if it does not exist
    if( !altiumModelsPath.DirExists() )
    {
        if( !altiumModelsPath.Mkdir() )
        {
            wxLogError( _( "Cannot create directory '%s'. No 3D-models will be imported." ),
                        altiumModelsPath.GetFullPath() );
            return;
        }
    }

    int idx = 0;
    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AMODEL elem( reader );

        wxString stepPath = aRootDir + std::to_string( idx++ );

        const CFB::COMPOUND_FILE_ENTRY* stepEntry = FindStream( aReader, stepPath.c_str() );
        if( stepEntry == nullptr )
        {
            wxLogError( _( "File not found: '%s'. 3D-model not imported." ), stepPath );
            continue;
        }

        size_t                  stepSize = static_cast<size_t>( stepEntry->size );
        std::unique_ptr<char[]> stepContent( new char[stepSize] );

        // read file into buffer
        aReader.ReadFile( stepEntry, 0, stepContent.get(), stepSize );

        wxFileName storagePath( altiumModelsPath.GetPath(), elem.name );

        if( !storagePath.IsDirWritable() )
        {
            wxLogError( _( "Insufficient permissions to save file '%s'." ),
                        storagePath.GetFullPath() );
            continue;
        }

        wxMemoryInputStream stepStream( stepContent.get(), stepSize );
        wxZlibInputStream   zlibInputStream( stepStream );

        wxFFileOutputStream outputStream( storagePath.GetFullPath() );
        outputStream.Write( zlibInputStream );
        outputStream.Close();

        m_models.insert( { elem.id, kicadModelPrefix + elem.name } );
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Models stream is not fully parsed" );
    }
}


void ALTIUM_PCB::ParseNets6Data( const CFB::CompoundFileReader& aReader,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading nets..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

    wxASSERT( m_num_nets == 0 );
    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ANET6 elem( reader );

        m_board->Add( new NETINFO_ITEM( m_board, elem.name, ++m_num_nets ), ADD_MODE::APPEND );
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Nets6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParsePolygons6Data( const CFB::CompoundFileReader& aReader,
                                     const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading polygons..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        APOLYGON6 elem( reader );

        PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );

        if( klayer == UNDEFINED_LAYER )
        {
            wxLogWarning( _( "Polygon found on an Altium layer (%d) with no KiCad equivalent. "
                             "It has been moved to KiCad layer Eco1_User." ),
                          elem.layer );
            klayer = Eco1_User;
        }

        SHAPE_LINE_CHAIN linechain;
        HelperShapeLineChainFromAltiumVertices( linechain, elem.vertices );

        if( linechain.PointCount() < 2 )
        {
            wxLogError( _( "Polygon has only %d point extracted from %ld vertices. At least 2 "
                           "points are required." ),
                        linechain.PointCount(),
                        elem.vertices.size() );
            m_polygons.emplace_back( nullptr );
            continue;
        }

        ZONE* zone = new ZONE( m_board );
        m_board->Add( zone, ADD_MODE::APPEND );
        m_polygons.emplace_back( zone );

        zone->SetFillVersion( 6 );
        zone->SetNetCode( GetNetCode( elem.net ) );
        zone->SetLayer( klayer );
        zone->SetPosition( elem.vertices.at( 0 ).position );
        zone->SetLocked( elem.locked );
        zone->SetPriority( elem.pourindex > 0 ? elem.pourindex : 0 );
        zone->Outline()->AddOutline( linechain );

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
            zone->SetPriority( 1 );

            // check if this is the outer zone by simply comparing the BBOX
            const auto& cur_outer_plane = m_outer_plane.find( elem.layer );
            if( cur_outer_plane == m_outer_plane.end()
                    || zone->GetBoundingBox().Contains(
                            cur_outer_plane->second->GetBoundingBox() ) )
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
                const EDA_RECT& bbox = zone->GetBoundingBox();
                zone->SetHatchGap( std::max( bbox.GetHeight(), bbox.GetWidth() ) );
            }
            else
            {
                zone->SetHatchGap( elem.gridsize - elem.trackwidth );
            }

            zone->SetHatchOrientation( elem.hatchstyle == ALTIUM_POLYGON_HATCHSTYLE::DEGREE_45 ? 45 : 0 );
        }

        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     ZONE::GetDefaultHatchPitch(), true );
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Polygons6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseRules6Data( const CFB::CompoundFileReader& aReader,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading rules..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ARULE6 elem( reader );

        m_rules[elem.kind].emplace_back( elem );
    }

    // sort rules by priority
    for( auto&& val : m_rules )
    {
        std::sort( val.second.begin(), val.second.end(),
                []( const auto& lhs, const auto& rhs )
                {
                    return lhs.priority < rhs.priority;
                } );
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Rules6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseBoardRegionsData( const CFB::CompoundFileReader& aReader,
                                        const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading board regions..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AREGION6 elem( reader, false );

        // TODO: implement?
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "BoardRegions stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseShapeBasedRegions6Data( const CFB::CompoundFileReader& aReader,
                                              const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading zones..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AREGION6 elem( reader, true );

        if( elem.kind == ALTIUM_REGION_KIND::BOARD_CUTOUT )
        {
            HelperCreateBoardOutline( elem.outline );
        }
        else if( elem.kind == ALTIUM_REGION_KIND::POLYGON_CUTOUT || elem.is_keepout )
        {
            SHAPE_LINE_CHAIN linechain;
            HelperShapeLineChainFromAltiumVertices( linechain, elem.outline );

            if( linechain.PointCount() < 2 )
            {
                wxLogError(  _( "ShapeBasedRegion has only %d point extracted from %ld vertices. "
                                "At least 2 points are required." ),
                             linechain.PointCount(),
                             elem.outline.size() );
                continue;
            }

            ZONE* zone = new ZONE( m_board );
            m_board->Add( zone, ADD_MODE::APPEND );

            zone->SetFillVersion( 6 );
            zone->SetIsRuleArea( true );
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowPads( false );
            zone->SetDoNotAllowFootprints( false );
            zone->SetDoNotAllowCopperPour( true );

            zone->SetPosition( elem.outline.at( 0 ).position );
            zone->Outline()->AddOutline( linechain );

            if( elem.layer == ALTIUM_LAYER::MULTI_LAYER )
            {
                zone->SetLayer( F_Cu );
                zone->SetLayerSet( LSET::AllCuMask() );
            }
            else
            {
                PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );

                if( klayer == UNDEFINED_LAYER )
                {
                    wxLogWarning( _( "Zone found on an Altium layer (%d) with no KiCad equivalent. "
                                     "It has been moved to KiCad layer Eco1_User." ),
                                  elem.layer );
                    klayer = Eco1_User;
                }
                zone->SetLayer( klayer );
            }

            zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                         ZONE::GetDefaultHatchPitch(), true );
        }
        else if( elem.kind == ALTIUM_REGION_KIND::COPPER )
        {
            if( elem.subpolyindex == ALTIUM_POLYGON_NONE )
            {
                PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );

                if( klayer == UNDEFINED_LAYER )
                {
                    wxLogWarning( _( "Polygon found on an Altium layer (%d) with no KiCad equivalent. "
                                     "It has been moved to KiCad layer Eco1_User." ),
                                  elem.layer );
                    klayer = Eco1_User;
                }

                SHAPE_LINE_CHAIN linechain;
                HelperShapeLineChainFromAltiumVertices( linechain, elem.outline );

                if( linechain.PointCount() < 2 )
                {
                    wxLogError( _( "Polygon has only %d point extracted from %ld vertices. At "
                                   "least 2 points are required." ),
                                linechain.PointCount(),
                                elem.outline.size() );
                    continue;
                }

                PCB_SHAPE* shape = new PCB_SHAPE( m_board );
                m_board->Add( shape, ADD_MODE::APPEND );
                shape->SetShape( PCB_SHAPE_TYPE::POLYGON );
                shape->SetFilled( true );
                shape->SetLayer( klayer );
                shape->SetWidth( 0 );

                shape->SetPolyShape( linechain );
            }
        }
        else
        {
            wxLogError( _( "Ignored polygon shape of kind %d (not yet supported)." ), elem.kind );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "ShapeBasedRegions6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseRegions6Data( const CFB::CompoundFileReader& aReader,
                                    const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading zone fills..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

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
                continue; // we know the zone id, but because we do not know the layer we did not add it!
            }

            PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );
            if( klayer == UNDEFINED_LAYER )
            {
                continue; // Just skip it for now. Users can fill it themselves.
            }

            SHAPE_LINE_CHAIN linechain;
            for( const ALTIUM_VERTICE& vertice : elem.outline )
            {
                linechain.Append( vertice.position );
            }
            linechain.Append( elem.outline.at( 0 ).position );
            linechain.SetClosed( true );

            SHAPE_POLY_SET rawPolys;
            rawPolys.AddOutline( linechain );

            for( const std::vector<ALTIUM_VERTICE>& hole : elem.holes )
            {
                SHAPE_LINE_CHAIN hole_linechain;
                for( const ALTIUM_VERTICE& vertice : hole )
                {
                    hole_linechain.Append( vertice.position );
                }
                hole_linechain.Append( hole.at( 0 ).position );
                hole_linechain.SetClosed( true );
                rawPolys.AddHole( hole_linechain );
            }

            if( zone->GetFilledPolysUseThickness() )
                rawPolys.Deflate( zone->GetMinThickness() / 2, 32 );

            if( zone->HasFilledPolysForLayer( klayer ) )
                rawPolys.BooleanAdd( zone->RawPolysList( klayer ),
                                     SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

            SHAPE_POLY_SET finalPolys = rawPolys;
            finalPolys.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

            zone->SetRawPolysList( klayer, rawPolys );
            zone->SetFilledPolysList( klayer, finalPolys );
            zone->SetIsFilled( true );
            zone->SetNeedRefill( false );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Regions6 stream is not fully parsed" );
    }
}


void ALTIUM_PCB::ParseArcs6Data( const CFB::CompoundFileReader& aReader,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading arcs..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AARC6 elem( reader );

        if( elem.is_polygonoutline || elem.subpolyindex != ALTIUM_POLYGON_NONE )
            continue;

        // element in plane is in fact substracted from the plane. Should be already done by Altium?
        //if( IsAltiumLayerAPlane( elem.layer ) )
        //    continue;

        PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );

        if( elem.is_keepout || IsAltiumLayerAPlane( elem.layer ) )
        {
            PCB_SHAPE shape( nullptr ); // just a helper to get the graphic
            shape.SetWidth( elem.width );
            shape.SetCenter( elem.center );

            if( elem.startangle == 0. && elem.endangle == 360. )
            { // TODO: other variants to define circle?
                shape.SetShape( PCB_SHAPE_TYPE::CIRCLE );
                shape.SetArcStart( elem.center - wxPoint( 0, elem.radius ) );
            }
            else
            {
                shape.SetShape( PCB_SHAPE_TYPE::ARC );
                shape.SetAngle( -NormalizeAngleDegreesPos( elem.endangle - elem.startangle ) * 10. );

                double  startradiant   = DEG2RAD( elem.startangle );
                wxPoint arcStartOffset = wxPoint( KiROUND( std::cos( startradiant ) * elem.radius ),
                        -KiROUND( std::sin( startradiant ) * elem.radius ) );
                shape.SetArcStart( elem.center + arcStartOffset );
            }

            ZONE* zone = new ZONE( m_board );
            m_board->Add( zone, ADD_MODE::APPEND );

            zone->SetFillVersion( 6 );
            zone->SetIsRuleArea( true );
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowPads( false );
            zone->SetDoNotAllowFootprints( false );
            zone->SetDoNotAllowCopperPour( true );

            if( elem.layer == ALTIUM_LAYER::MULTI_LAYER )
            {
                zone->SetLayer( F_Cu );
                zone->SetLayerSet( LSET::AllCuMask() );
            }
            else
            {
                PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );

                if( klayer == UNDEFINED_LAYER )
                {
                    wxLogWarning( _( "Arc keepout found on an Altium layer (%d) with no KiCad "
                                     "equivalent. It has been moved to KiCad layer Eco1_User." ),
                                  elem.layer );
                    klayer = Eco1_User;
                }
                zone->SetLayer( klayer );
            }

            shape.TransformShapeWithClearanceToPolygon( *zone->Outline(), klayer, 0, ARC_HIGH_DEF,
                                                        ERROR_INSIDE );
            zone->Outline()->Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE ); // the outline is not a single polygon!

            zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                         ZONE::GetDefaultHatchPitch(), true );
            continue;
        }

        if( klayer == UNDEFINED_LAYER )
        {
            wxLogWarning( _( "Arc found on an Altium layer (%d) with no KiCad equivalent. "
                             "It has been moved to KiCad layer Eco1_User." ),
                          elem.layer );
            klayer = Eco1_User;
        }

        if( klayer >= F_Cu && klayer <= B_Cu )
        {
            double  angle          = -NormalizeAngleDegreesPos( elem.endangle - elem.startangle );
            double  startradiant   = DEG2RAD( elem.startangle );
            wxPoint arcStartOffset = wxPoint( KiROUND( std::cos( startradiant ) * elem.radius ),
                    -KiROUND( std::sin( startradiant ) * elem.radius ) );

            SHAPE_ARC shapeArc( elem.center, elem.center + arcStartOffset, angle, elem.width );
            PCB_ARC*  arc = new PCB_ARC( m_board, &shapeArc );
            m_board->Add( arc, ADD_MODE::APPEND );

            arc->SetWidth( elem.width );
            arc->SetLayer( klayer );
            arc->SetNetCode( GetNetCode( elem.net ) );
        }
        else
        {
            PCB_SHAPE* shape = HelperCreateAndAddDrawsegment( elem.component );
            shape->SetCenter( elem.center );
            shape->SetWidth( elem.width );
            shape->SetLayer( klayer );

            if( elem.startangle == 0. && elem.endangle == 360. )
            { // TODO: other variants to define circle?
                shape->SetShape( PCB_SHAPE_TYPE::CIRCLE );
                shape->SetArcStart( elem.center - wxPoint( 0, elem.radius ) );
            }
            else
            {
                shape->SetShape( PCB_SHAPE_TYPE::ARC );
                shape->SetAngle( -NormalizeAngleDegreesPos( elem.endangle - elem.startangle ) * 10. );

                double  startradiant   = DEG2RAD( elem.startangle );
                wxPoint arcStartOffset = wxPoint( KiROUND( std::cos( startradiant ) * elem.radius ),
                        -KiROUND( std::sin( startradiant ) * elem.radius ) );
                shape->SetArcStart( elem.center + arcStartOffset );
            }

            HelperDrawsegmentSetLocalCoord( shape, elem.component );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Arcs6 stream is not fully parsed" );
    }
}


void ALTIUM_PCB::ParsePads6Data( const CFB::CompoundFileReader& aReader,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading pads..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        APAD6 elem( reader );

        // It is possible to place altium pads on non-copper layers -> we need to interpolate them using drawings!
        if( !IsAltiumLayerCopper( elem.layer ) && !IsAltiumLayerAPlane( elem.layer )
                && elem.layer != ALTIUM_LAYER::MULTI_LAYER )
        {
            HelperParsePad6NonCopper( elem );
            continue;
        }

        // Create Pad
        FOOTPRINT* footprint = nullptr;

        if( elem.component == ALTIUM_COMPONENT_NONE )
        {
            footprint = new FOOTPRINT( m_board ); // We cannot add a pad directly into the PCB
            m_board->Add( footprint, ADD_MODE::APPEND );
            footprint->SetPosition( elem.position );
        }
        else
        {
            if( m_components.size() <= elem.component )
            {
                THROW_IO_ERROR( wxString::Format( "Pads6 stream tries to access component id %d "
                                                  "of %d existing components",
                                                  elem.component,
                                                  m_components.size() ) );
            }
            footprint = m_components.at( elem.component );
        }

        PAD* pad = new PAD( footprint );
        footprint->Add( pad, ADD_MODE::APPEND );

        pad->SetName( elem.name );
        pad->SetNetCode( GetNetCode( elem.net ) );
        pad->SetLocked( elem.is_locked );

        pad->SetPosition( elem.position );
        pad->SetOrientationDegrees( elem.direction );
        pad->SetLocalCoord();

        pad->SetSize( elem.topsize );

        if( elem.holesize == 0 )
        {
            pad->SetAttribute( PAD_ATTRIB::SMD );
        }
        else
        {
            if( elem.layer != ALTIUM_LAYER::MULTI_LAYER )
            {
                // TODO: I assume other values are possible as well?
                wxLogError( _( "Footprint %s pad %s is not marked as multilayer, but is a TH pad." ),
                            footprint->GetReference(),
                            elem.name );
            }
            pad->SetAttribute( elem.plated ? PAD_ATTRIB::PTH :
                                             PAD_ATTRIB::NPTH );
            if( !elem.sizeAndShape || elem.sizeAndShape->holeshape == ALTIUM_PAD_HOLE_SHAPE::ROUND )
            {
                pad->SetDrillShape( PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_CIRCLE );
                pad->SetDrillSize( wxSize( elem.holesize, elem.holesize ) );
            }
            else
            {
                switch( elem.sizeAndShape->holeshape )
                {
                case ALTIUM_PAD_HOLE_SHAPE::ROUND:
                    wxFAIL_MSG( "Round holes are handled before the switch" );
                    break;

                case ALTIUM_PAD_HOLE_SHAPE::SQUARE:
                    wxLogWarning( _( "Footprint %s pad %s has a square hole (not yet supported)." ),
                                  footprint->GetReference(),
                                  elem.name );
                    pad->SetDrillShape( PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_CIRCLE );
                    pad->SetDrillSize( wxSize( elem.holesize, elem.holesize ) ); // Workaround
                    // TODO: elem.sizeAndShape->slotsize was 0 in testfile. Either use holesize in this case or rect holes have a different id
                    break;

                case ALTIUM_PAD_HOLE_SHAPE::SLOT:
                {
                    pad->SetDrillShape( PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_OBLONG );
                    double normalizedSlotrotation =
                            NormalizeAngleDegreesPos( elem.sizeAndShape->slotrotation );

                    if( normalizedSlotrotation == 0. || normalizedSlotrotation == 180. )
                    {
                        pad->SetDrillSize( wxSize( elem.sizeAndShape->slotsize, elem.holesize ) );
                    }
                    else
                    {
                        if( normalizedSlotrotation != 90. && normalizedSlotrotation != 270. )
                        {
                            wxLogWarning( _( "Footprint %s pad %s has a hole-rotation of %f "
                                             "degrees. KiCad only supports 90 degree rotations." ),
                                          footprint->GetReference(),
                                          elem.name,
                                          normalizedSlotrotation );
                        }

                        pad->SetDrillSize( wxSize( elem.holesize, elem.sizeAndShape->slotsize ) );
                    }
                }
                break;

                default:
                case ALTIUM_PAD_HOLE_SHAPE::UNKNOWN:
                    wxLogError( _( "Footprint %s pad %s uses a hole of unknown kind %d." ),
                                footprint->GetReference(),
                                elem.name,
                                elem.sizeAndShape->holeshape );
                    pad->SetDrillShape( PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_CIRCLE );
                    pad->SetDrillSize( wxSize( elem.holesize, elem.holesize ) ); // Workaround
                    break;
                }
            }

            if( elem.sizeAndShape )
            {
                pad->SetOffset( elem.sizeAndShape->holeoffset[0] );
            }
        }

        if( elem.padmode != ALTIUM_PAD_MODE::SIMPLE )
        {
            wxLogError( _( "Footprint %s pad %s uses a complex pad stack (not yet supported.)" ),
                        footprint->GetReference(),
                        elem.name );
        }

        switch( elem.topshape )
        {
        case ALTIUM_PAD_SHAPE::RECT:
            pad->SetShape( PAD_SHAPE::RECT );
            break;
        case ALTIUM_PAD_SHAPE::CIRCLE:
            if( elem.sizeAndShape
                    && elem.sizeAndShape->alt_shape[0] == ALTIUM_PAD_SHAPE_ALT::ROUNDRECT )
            {
                pad->SetShape( PAD_SHAPE::ROUNDRECT ); // 100 = round, 0 = rectangular
                double ratio = elem.sizeAndShape->cornerradius[0] / 200.;
                pad->SetRoundRectRadiusRatio( ratio );
            }
            else if( elem.topsize.x == elem.topsize.y )
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
                        footprint->GetReference(),
                        elem.name );
            break;
        }

        switch( elem.layer )
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
            pad->SetLayerSet( elem.plated ? PAD::PTHMask() : PAD::UnplatedHoleMask() );
            break;
        default:
            PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );
            pad->SetLayer( klayer );
            pad->SetLayerSet( LSET( 1, klayer ) );
            break;
        }

        if( elem.pastemaskexpansionmode == ALTIUM_PAD_RULE::MANUAL )
        {
            pad->SetLocalSolderPasteMargin( elem.pastemaskexpansionmanual );
        }

        if( elem.soldermaskexpansionmode == ALTIUM_PAD_RULE::MANUAL )
        {
            pad->SetLocalSolderMaskMargin( elem.soldermaskexpansionmanual );
        }

        if( elem.is_tent_top )
        {
            pad->SetLayerSet( pad->GetLayerSet().reset( F_Mask ) );
        }
        if( elem.is_tent_bottom )
        {
            pad->SetLayerSet( pad->GetLayerSet().reset( B_Mask ) );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Pads6 stream is not fully parsed" );
    }
}


void ALTIUM_PCB::HelperParsePad6NonCopper( const APAD6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );

    if( klayer == UNDEFINED_LAYER )
    {
        wxLogWarning( _( "Non-copper pad %s found on an Altium layer (%d) with no KiCad equivalent. "
                         "It has been moved to KiCad layer Eco1_User." ),
                      aElem.name,
                      aElem.layer );
        klayer = Eco1_User;
    }

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
        PCB_SHAPE* shape = HelperCreateAndAddDrawsegment( aElem.component );
        shape->SetShape( PCB_SHAPE_TYPE::POLYGON );
        shape->SetFilled( true );
        shape->SetLayer( klayer );
        shape->SetWidth( 0 );

        shape->SetPolyPoints( { aElem.position + wxPoint( aElem.topsize.x / 2, aElem.topsize.y / 2 ),
                aElem.position + wxPoint( aElem.topsize.x / 2, -aElem.topsize.y / 2 ),
                aElem.position + wxPoint( -aElem.topsize.x / 2, -aElem.topsize.y / 2 ),
                aElem.position + wxPoint( -aElem.topsize.x / 2, aElem.topsize.y / 2 ) } );

        if( aElem.direction != 0 )
            shape->Rotate( aElem.position, aElem.direction * 10 );

        HelperDrawsegmentSetLocalCoord( shape, aElem.component );
    }
    break;

    case ALTIUM_PAD_SHAPE::CIRCLE:
        if( aElem.sizeAndShape
                && aElem.sizeAndShape->alt_shape[0] == ALTIUM_PAD_SHAPE_ALT::ROUNDRECT )
        {
            // filled roundrect
            int cornerradius = aElem.sizeAndShape->cornerradius[0];
            int offset = ( std::min( aElem.topsize.x, aElem.topsize.y ) * cornerradius ) / 200;

            PCB_SHAPE* shape = HelperCreateAndAddDrawsegment( aElem.component );
            shape->SetLayer( klayer );
            shape->SetWidth( offset * 2 );

            if( cornerradius < 100 )
            {
                int offsetX = aElem.topsize.x / 2 - offset;
                int offsetY = aElem.topsize.y / 2 - offset;

                wxPoint p11 = aElem.position + wxPoint( offsetX, offsetY );
                wxPoint p12 = aElem.position + wxPoint( offsetX, -offsetY );
                wxPoint p22 = aElem.position + wxPoint( -offsetX, -offsetY );
                wxPoint p21 = aElem.position + wxPoint( -offsetX, offsetY );

                shape->SetShape( PCB_SHAPE_TYPE::POLYGON );
                shape->SetFilled( true );
                shape->SetPolyPoints( { p11, p12, p22, p21 } );
            }
            else if( aElem.topsize.x == aElem.topsize.y )
            {
                // circle
                shape->SetShape( PCB_SHAPE_TYPE::CIRCLE );
                shape->SetFilled( true );
                shape->SetCenter( aElem.position );
                shape->SetWidth( aElem.topsize.x / 2 );
                shape->SetArcStart( aElem.position - wxPoint( 0, aElem.topsize.x / 4 ) );
            }
            else if( aElem.topsize.x < aElem.topsize.y )
            {
                // short vertical line
                shape->SetShape( PCB_SHAPE_TYPE::SEGMENT );
                wxPoint pointOffset( 0, ( aElem.topsize.y - aElem.topsize.x ) / 2 );
                shape->SetStart( aElem.position + pointOffset );
                shape->SetEnd( aElem.position - pointOffset );
            }
            else
            {
                // short horizontal line
                shape->SetShape( PCB_SHAPE_TYPE::SEGMENT );
                wxPoint pointOffset( ( aElem.topsize.x - aElem.topsize.y ) / 2, 0 );
                shape->SetStart( aElem.position + pointOffset );
                shape->SetEnd( aElem.position - pointOffset );
            }

            if( aElem.direction != 0 )
                shape->Rotate( aElem.position, aElem.direction * 10 );

            HelperDrawsegmentSetLocalCoord( shape, aElem.component );
        }
        else if( aElem.topsize.x == aElem.topsize.y )
        {
            // filled circle
            PCB_SHAPE* shape = HelperCreateAndAddDrawsegment( aElem.component );
            shape->SetShape( PCB_SHAPE_TYPE::CIRCLE );
            shape->SetFilled( true );
            shape->SetLayer( klayer );
            shape->SetCenter( aElem.position );
            shape->SetWidth( aElem.topsize.x / 2 );
            shape->SetArcStart( aElem.position - wxPoint( 0, aElem.topsize.x / 4 ) );
            HelperDrawsegmentSetLocalCoord( shape, aElem.component );
        }
        else
        {
            // short line
            PCB_SHAPE* shape = HelperCreateAndAddDrawsegment( aElem.component );
            shape->SetShape( PCB_SHAPE_TYPE::SEGMENT );
            shape->SetLayer( klayer );
            shape->SetWidth( std::min( aElem.topsize.x, aElem.topsize.y ) );

            if( aElem.topsize.x < aElem.topsize.y )
            {
                wxPoint offset( 0, ( aElem.topsize.y - aElem.topsize.x ) / 2 );
                shape->SetStart( aElem.position + offset );
                shape->SetEnd( aElem.position - offset );
            }
            else
            {
                wxPoint offset( ( aElem.topsize.x - aElem.topsize.y ) / 2, 0 );
                shape->SetStart( aElem.position + offset );
                shape->SetEnd( aElem.position - offset );
            }

            if( aElem.direction != 0 )
                shape->Rotate( aElem.position, aElem.direction * 10. );

            HelperDrawsegmentSetLocalCoord( shape, aElem.component );
        }
        break;

    case ALTIUM_PAD_SHAPE::OCTAGONAL:
    {
        // filled octagon
        PCB_SHAPE* shape = HelperCreateAndAddDrawsegment( aElem.component );
        shape->SetShape( PCB_SHAPE_TYPE::POLYGON );
        shape->SetFilled( true );
        shape->SetLayer( klayer );
        shape->SetWidth( 0 );

        wxPoint p11 = aElem.position + wxPoint( aElem.topsize.x / 2, aElem.topsize.y / 2 );
        wxPoint p12 = aElem.position + wxPoint( aElem.topsize.x / 2, -aElem.topsize.y / 2 );
        wxPoint p22 = aElem.position + wxPoint( -aElem.topsize.x / 2, -aElem.topsize.y / 2 );
        wxPoint p21 = aElem.position + wxPoint( -aElem.topsize.x / 2, aElem.topsize.y / 2 );

        int     chamfer = std::min( aElem.topsize.x, aElem.topsize.y ) / 4;
        wxPoint chamferX( chamfer, 0 );
        wxPoint chamferY( 0, chamfer );

        shape->SetPolyPoints( { p11 - chamferX, p11 - chamferY, p12 + chamferY, p12 - chamferX,
                                p22 + chamferX, p22 + chamferY, p21 - chamferY, p21 + chamferX } );

        if( aElem.direction != 0. )
            shape->Rotate( aElem.position, aElem.direction * 10 );

        HelperDrawsegmentSetLocalCoord( shape, aElem.component );
    }
        break;

    case ALTIUM_PAD_SHAPE::UNKNOWN:
    default:
        wxLogError( _( "Non-copper pad %s uses an unknown pad-shape." ), aElem.name );
        break;
    }
}

void ALTIUM_PCB::ParseVias6Data( const CFB::CompoundFileReader& aReader,
                                 const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading vias..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

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
    {
        THROW_IO_ERROR( "Vias6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseTracks6Data( const CFB::CompoundFileReader& aReader,
                                   const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading tracks..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ATRACK6 elem( reader );

        if( elem.is_polygonoutline || elem.subpolyindex != ALTIUM_POLYGON_NONE )
            continue;

        // element in plane is in fact substracted from the plane. Already done by Altium?
        //if( IsAltiumLayerAPlane( elem.layer ) )
        //    continue;

        PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );

        if( elem.is_keepout || IsAltiumLayerAPlane( elem.layer ) )
        {
            PCB_SHAPE shape( nullptr ); // just a helper to get the graphic
            shape.SetShape( PCB_SHAPE_TYPE::SEGMENT );
            shape.SetStart( elem.start );
            shape.SetEnd( elem.end );
            shape.SetWidth( elem.width );

            ZONE* zone = new ZONE( m_board );
            m_board->Add( zone, ADD_MODE::APPEND );

            zone->SetFillVersion( 6 );
            zone->SetIsRuleArea( true );
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowPads( false );
            zone->SetDoNotAllowFootprints( false );
            zone->SetDoNotAllowCopperPour( true );

            if( elem.layer == ALTIUM_LAYER::MULTI_LAYER )
            {
                zone->SetLayer( F_Cu );
                zone->SetLayerSet( LSET::AllCuMask() );
            }
            else
            {
                PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );

                if( klayer == UNDEFINED_LAYER )
                {
                    wxLogWarning( _( "Track keepout found on an Altium layer (%d) with no KiCad "
                                     "equivalent. It has been moved to KiCad layer Eco1_User." ),
                                  elem.layer );
                    klayer = Eco1_User;
                }
                zone->SetLayer( klayer );
            }

            shape.TransformShapeWithClearanceToPolygon( *zone->Outline(), klayer, 0, ARC_HIGH_DEF,
                                                        ERROR_INSIDE );

            zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                         ZONE::GetDefaultHatchPitch(), true );
            continue;
        }

        if( klayer == UNDEFINED_LAYER )
        {
            wxLogWarning( _( "Track found on an Altium layer (%d) with no KiCadequivalent. "
                             "It has been moved to KiCad layer Eco1_User." ),
                          elem.layer );
            klayer = Eco1_User;
        }

        if( klayer >= F_Cu && klayer <= B_Cu )
        {
            PCB_TRACK* track = new PCB_TRACK( m_board );
            m_board->Add( track, ADD_MODE::APPEND );

            track->SetStart( elem.start );
            track->SetEnd( elem.end );
            track->SetWidth( elem.width );
            track->SetLayer( klayer );
            track->SetNetCode( GetNetCode( elem.net ) );
        }
        else
        {
            PCB_SHAPE* shape = HelperCreateAndAddDrawsegment( elem.component );
            shape->SetShape( PCB_SHAPE_TYPE::SEGMENT );
            shape->SetStart( elem.start );
            shape->SetEnd( elem.end );
            shape->SetWidth( elem.width );
            shape->SetLayer( klayer );
            HelperDrawsegmentSetLocalCoord( shape, elem.component );
        }

        reader.SkipSubrecord();
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Tracks6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseTexts6Data( const CFB::CompoundFileReader& aReader,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading text..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        ATEXT6 elem( reader );

        if( elem.fonttype == ALTIUM_TEXT_TYPE::BARCODE )
        {
            wxLogError( _( "Ignored barcode on Altium layer %d (not yet supported)." ),
                        elem.layer );
            continue;
        }

        // TODO: better approach to select if item belongs to a FOOTPRINT
        EDA_TEXT*   tx  = nullptr;
        BOARD_ITEM* itm = nullptr;

        if( elem.component == ALTIUM_COMPONENT_NONE )
        {
            PCB_TEXT* pcbText = new PCB_TEXT( m_board );
            tx = pcbText;
            itm = pcbText;
            m_board->Add( pcbText, ADD_MODE::APPEND );
        }
        else
        {
            if( m_components.size() <= elem.component )
            {
                THROW_IO_ERROR( wxString::Format( "Texts6 stream tries to access component id %d "
                                                  "of %d existing components",
                                                  elem.component,
                                                  m_components.size() ) );
            }

            FOOTPRINT* footprint = m_components.at( elem.component );
            FP_TEXT*   fpText;

            if( elem.isDesignator )
            {
                fpText = &footprint->Reference();
            }
            else if( elem.isComment )
            {
                fpText = &footprint->Value();
            }
            else
            {
                fpText = new FP_TEXT( footprint );
                footprint->Add( fpText, ADD_MODE::APPEND );
            }

            fpText->SetKeepUpright( false );

            tx  = fpText;
            itm = fpText;
        }

        wxString trimmedText = elem.text.Trim();
        if( !elem.isDesignator && trimmedText.CmpNoCase( ".Designator" ) == 0 )
        {
            tx->SetText( "${REFERENCE}" );
        }
        else if( !elem.isComment && trimmedText.CmpNoCase( ".Comment" ) == 0 )
        {
            tx->SetText( "${VALUE}" );
        }
        else if( trimmedText.CmpNoCase( ".Layer_Name" ) == 0 )
        {
            tx->SetText( "${LAYER}" );
        }
        else
        {
            tx->SetText( elem.text );
        }

        itm->SetPosition( elem.position );
        tx->SetTextAngle( elem.rotation * 10. );

        if( elem.component != ALTIUM_COMPONENT_NONE )
        {
            FP_TEXT* fpText = dynamic_cast<FP_TEXT*>( tx );

            if( fpText )
            {
                FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( fpText->GetParent() );
                double     orientation     = parentFootprint->GetOrientation();

                fpText->SetTextAngle( fpText->GetTextAngle() - orientation );
                fpText->SetLocalCoord();
            }
        }

        PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );

        if( klayer == UNDEFINED_LAYER )
        {
            wxLogWarning( _( "Text found on an Altium layer (%d) with no KiCad equivalent. "
                             "It has been moved to KiCad layer Eco1_User." ),
                          elem.layer );
            klayer = Eco1_User;
        }

        itm->SetLayer( klayer );

        if( elem.fonttype == ALTIUM_TEXT_TYPE::TRUETYPE )
        {
            // TODO: why is this required? Somehow, truetype size is calculated differently
            tx->SetTextSize( wxSize( elem.height / 2, elem.height / 2 ) );
        }
        else
        {
            tx->SetTextSize( wxSize( elem.height, elem.height ) ); // TODO: parse text width
        }

        tx->SetTextThickness( elem.strokewidth );
        tx->SetBold( elem.isBold );
        tx->SetItalic( elem.isItalic );
        tx->SetMirrored( elem.isMirrored );

        if( elem.isDesignator || elem.isComment ) // That's just a bold assumption
        {
            tx->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT );
            tx->SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_BOTTOM );
        }
        else
        {
            switch( elem.textposition )
            {
            case ALTIUM_TEXT_POSITION::LEFT_TOP:
            case ALTIUM_TEXT_POSITION::LEFT_CENTER:
            case ALTIUM_TEXT_POSITION::LEFT_BOTTOM:
                tx->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT );
                break;
            case ALTIUM_TEXT_POSITION::CENTER_TOP:
            case ALTIUM_TEXT_POSITION::CENTER_CENTER:
            case ALTIUM_TEXT_POSITION::CENTER_BOTTOM:
                tx->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_CENTER );
                break;
            case ALTIUM_TEXT_POSITION::RIGHT_TOP:
            case ALTIUM_TEXT_POSITION::RIGHT_CENTER:
            case ALTIUM_TEXT_POSITION::RIGHT_BOTTOM:
                tx->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_RIGHT );
                break;
            default:
                wxLogError( "Unexpected horizontal Text Position. This should never happen." );
                break;
            }

            switch( elem.textposition )
            {
            case ALTIUM_TEXT_POSITION::LEFT_TOP:
            case ALTIUM_TEXT_POSITION::CENTER_TOP:
            case ALTIUM_TEXT_POSITION::RIGHT_TOP:
                tx->SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_TOP );
                break;
            case ALTIUM_TEXT_POSITION::LEFT_CENTER:
            case ALTIUM_TEXT_POSITION::CENTER_CENTER:
            case ALTIUM_TEXT_POSITION::RIGHT_CENTER:
                tx->SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_CENTER );
                break;
            case ALTIUM_TEXT_POSITION::LEFT_BOTTOM:
            case ALTIUM_TEXT_POSITION::CENTER_BOTTOM:
            case ALTIUM_TEXT_POSITION::RIGHT_BOTTOM:
                tx->SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_BOTTOM );
                break;
            default:
                wxLogError( "Unexpected vertical text position. This should never happen." );
                break;
            }
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Texts6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseFills6Data( const CFB::CompoundFileReader& aReader,
                                  const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    if( m_progressReporter )
        m_progressReporter->Report( _( "Loading rectangles..." ) );

    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        checkpoint();
        AFILL6 elem( reader );

        wxPoint p11( elem.pos1.x, elem.pos1.y );
        wxPoint p12( elem.pos1.x, elem.pos2.y );
        wxPoint p22( elem.pos2.x, elem.pos2.y );
        wxPoint p21( elem.pos2.x, elem.pos1.y );

        wxPoint center( ( elem.pos1.x + elem.pos2.x ) / 2, ( elem.pos1.y + elem.pos2.y ) / 2 );

        PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );

        if( klayer == UNDEFINED_LAYER )
        {
            wxLogWarning( _( "Fill found on an Altium layer (%d) with no KiCad equivalent. "
                             "It has been moved to KiCad layer Eco1_User." ),
                          elem.layer );
            klayer = Eco1_User;
        }

        if( elem.is_keepout || elem.net != ALTIUM_NET_UNCONNECTED )
        {
            ZONE* zone = new ZONE( m_board );
            m_board->Add( zone, ADD_MODE::APPEND );

            zone->SetFillVersion( 6 );
            zone->SetNetCode( GetNetCode( elem.net ) );
            zone->SetLayer( klayer );
            zone->SetPosition( elem.pos1 );
            zone->SetPriority( 1000 );

            const int outlineIdx = -1; // this is the id of the copper zone main outline
            zone->AppendCorner( p11, outlineIdx );
            zone->AppendCorner( p12, outlineIdx );
            zone->AppendCorner( p22, outlineIdx );
            zone->AppendCorner( p21, outlineIdx );

            // should be correct?
            zone->SetLocalClearance( 0 );
            zone->SetPadConnection( ZONE_CONNECTION::FULL );

            if( elem.is_keepout )
            {
                zone->SetIsRuleArea( true );
                zone->SetDoNotAllowTracks( false );
                zone->SetDoNotAllowVias( false );
                zone->SetDoNotAllowPads( false );
                zone->SetDoNotAllowFootprints( false );
                zone->SetDoNotAllowCopperPour( true );
            }

            if( elem.rotation != 0. )
                zone->Rotate( center, elem.rotation * 10 );

            zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                         ZONE::GetDefaultHatchPitch(), true );
        }
        else
        {
            PCB_SHAPE* shape = new PCB_SHAPE( m_board );
            m_board->Add( shape, ADD_MODE::APPEND );

            shape->SetShape( PCB_SHAPE_TYPE::POLYGON );
            shape->SetFilled( true );
            shape->SetLayer( klayer );
            shape->SetWidth( 0 );

            shape->SetPolyPoints( { p11, p12, p22, p21 } );

            if( elem.rotation != 0. )
                shape->Rotate( center, elem.rotation * 10 );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Fills6 stream is not fully parsed" );
    }
}
