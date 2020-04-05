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
#include "altium_parser.h"
#include "altium_parser_pcb.h"

#include <class_board.h>
#include <class_dimension.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_track.h>

#include <class_edge_mod.h>
#include <class_text_mod.h>

#include <board_stackup_manager/stackup_predefined_prms.h>

#include <compoundfilereader.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <utf.h>


void ParseAltiumPcb( BOARD* aBoard, const wxString& aFileName,
        const std::map<ALTIUM_PCB_DIR, std::string>& aFileMapping )
{
    // Open file
    FILE* fp = wxFopen( aFileName, "rb" );
    if( fp == nullptr )
    {
        wxLogError( wxString::Format( _( "Cannot open file '%s'" ), aFileName ) );
        return;
    }

    fseek( fp, 0, SEEK_END );
    long len = ftell( fp );
    if( len < 0 )
    {
        fclose( fp );
        THROW_IO_ERROR( "Reading error, cannot determine length of file" );
    }

    std::unique_ptr<unsigned char[]> buffer( new unsigned char[len] );
    fseek( fp, 0, SEEK_SET );

    size_t bytesRead = fread( buffer.get(), sizeof( unsigned char ), len, fp );
    fclose( fp );
    if( static_cast<size_t>( len ) != bytesRead )
    {
        THROW_IO_ERROR( "Reading error" );
    }

    try
    {
        CFB::CompoundFileReader reader( buffer.get(), bytesRead );

        // Parse File
        ALTIUM_PCB pcb( aBoard );
        pcb.Parse( reader, aFileMapping );
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }
}


bool IsAltiumLayerAPlane( ALTIUM_LAYER aLayer )
{
    return aLayer >= ALTIUM_LAYER::INTERNAL_PLANE_1 && aLayer <= ALTIUM_LAYER::INTERNAL_PLANE_16;
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
    case ALTIUM_LAYER::UNKNOWN:
        return UNDEFINED_LAYER;

    case ALTIUM_LAYER::TOP_LAYER:
        return F_Cu;
    case ALTIUM_LAYER::MID_LAYER_1:
        return In1_Cu; // TODO: stackup same as in KiCad?
    case ALTIUM_LAYER::MID_LAYER_2:
        return In2_Cu;
    case ALTIUM_LAYER::MID_LAYER_3:
        return In3_Cu;
    case ALTIUM_LAYER::MID_LAYER_4:
        return In4_Cu;
    case ALTIUM_LAYER::MID_LAYER_5:
        return In5_Cu;
    case ALTIUM_LAYER::MID_LAYER_6:
        return In6_Cu;
    case ALTIUM_LAYER::MID_LAYER_7:
        return In7_Cu;
    case ALTIUM_LAYER::MID_LAYER_8:
        return In8_Cu;
    case ALTIUM_LAYER::MID_LAYER_9:
        return In9_Cu;
    case ALTIUM_LAYER::MID_LAYER_10:
        return In10_Cu;
    case ALTIUM_LAYER::MID_LAYER_11:
        return In11_Cu;
    case ALTIUM_LAYER::MID_LAYER_12:
        return In12_Cu;
    case ALTIUM_LAYER::MID_LAYER_13:
        return In13_Cu;
    case ALTIUM_LAYER::MID_LAYER_14:
        return In14_Cu;
    case ALTIUM_LAYER::MID_LAYER_15:
        return In15_Cu;
    case ALTIUM_LAYER::MID_LAYER_16:
        return In16_Cu;
    case ALTIUM_LAYER::MID_LAYER_17:
        return In17_Cu;
    case ALTIUM_LAYER::MID_LAYER_18:
        return In18_Cu;
    case ALTIUM_LAYER::MID_LAYER_19:
        return In19_Cu;
    case ALTIUM_LAYER::MID_LAYER_20:
        return In20_Cu;
    case ALTIUM_LAYER::MID_LAYER_21:
        return In21_Cu;
    case ALTIUM_LAYER::MID_LAYER_22:
        return In22_Cu;
    case ALTIUM_LAYER::MID_LAYER_23:
        return In23_Cu;
    case ALTIUM_LAYER::MID_LAYER_24:
        return In24_Cu;
    case ALTIUM_LAYER::MID_LAYER_25:
        return In25_Cu;
    case ALTIUM_LAYER::MID_LAYER_26:
        return In26_Cu;
    case ALTIUM_LAYER::MID_LAYER_27:
        return In27_Cu;
    case ALTIUM_LAYER::MID_LAYER_28:
        return In28_Cu;
    case ALTIUM_LAYER::MID_LAYER_29:
        return In29_Cu;
    case ALTIUM_LAYER::MID_LAYER_30:
        return In30_Cu;
    case ALTIUM_LAYER::BOTTOM_LAYER:
        return B_Cu;

    case ALTIUM_LAYER::TOP_OVERLAY:
        return F_SilkS;
    case ALTIUM_LAYER::BOTTOM_OVERLAY:
        return B_SilkS;
    case ALTIUM_LAYER::TOP_PASTE:
        return F_Paste;
    case ALTIUM_LAYER::BOTTOM_PASTE:
        return B_Paste;
    case ALTIUM_LAYER::TOP_SOLDER:
        return F_Mask;
    case ALTIUM_LAYER::BOTTOM_SOLDER:
        return B_Mask;

    case ALTIUM_LAYER::INTERNAL_PLANE_1:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_2:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_3:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_4:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_5:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_6:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_7:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_8:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_9:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_10:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_11:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_12:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_13:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_14:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_15:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::INTERNAL_PLANE_16:
        return UNDEFINED_LAYER;

    case ALTIUM_LAYER::DRILL_GUIDE:
        return Dwgs_User;
    case ALTIUM_LAYER::KEEP_OUT_LAYER:
        return UNDEFINED_LAYER;

    case ALTIUM_LAYER::MECHANICAL_1:
        return Dwgs_User; //Edge_Cuts;
    case ALTIUM_LAYER::MECHANICAL_2:
        return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_3:
        return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_4:
        return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_5:
        return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_6:
        return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_7:
        return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_8:
        return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_9:
        return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_10:
        return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_11:
        return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_12:
        return Dwgs_User;
    case ALTIUM_LAYER::MECHANICAL_13:
        return F_Fab;
    case ALTIUM_LAYER::MECHANICAL_14:
        return B_Fab;
    case ALTIUM_LAYER::MECHANICAL_15:
        return F_CrtYd;
    case ALTIUM_LAYER::MECHANICAL_16:
        return B_CrtYd;

    case ALTIUM_LAYER::DRILL_DRAWING:
        return Dwgs_User;
    case ALTIUM_LAYER::MULTI_LAYER:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::CONNECTIONS:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::BACKGROUND:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::DRC_ERROR_MARKERS:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::SELECTIONS:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::VISIBLE_GRID_1:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::VISIBLE_GRID_2:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::PAD_HOLES:
        return UNDEFINED_LAYER;
    case ALTIUM_LAYER::VIA_HOLES:
        return UNDEFINED_LAYER;

    default:
        return UNDEFINED_LAYER;
    }
}


ALTIUM_PCB::ALTIUM_PCB( BOARD* aBoard )
{
    m_board = aBoard;
    m_num_nets = 0;
}

ALTIUM_PCB::~ALTIUM_PCB()
{
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
            wxASSERT_MSG( !isRequired,
                    wxString::Format(
                            "Altium Directory of kind %d was expected, but no mapping is present in the code",
                            directory ) );
            continue;
        }

        const CFB::COMPOUND_FILE_ENTRY* file =
                FindStream( aReader, mappedDirectory->second.c_str() );
        if( file != nullptr )
        {
            fp( aReader, file );
        }
        else if( isRequired )
        {
            wxLogError( wxString::Format( _( "File not found: '%s'" ), mappedDirectory->second ) );
        }
    }

    // change priority of outer zone to zero
    for( auto& zone : m_outer_plane )
    {
        zone.second->SetPriority( 0 );
    }

    // Finish Board by recalculating module boundingboxes
    for( auto& module : m_board->Modules() )
    {
        module->CalculateBoundingBox();
    }
}

int ALTIUM_PCB::GetNetCode( uint16_t aId ) const
{
    if( aId == ALTIUM_NET_UNCONNECTED )
    {
        return NETINFO_LIST::UNCONNECTED;
    }
    else if( m_num_nets < aId )
    {
        THROW_IO_ERROR(
                wxString::Format( "Netcode with id %d does not exist. Only %d nets are known",
                        aId, m_num_nets ) );
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

void ALTIUM_PCB::ParseFileHeader(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
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

void ALTIUM_PCB::ParseBoard6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    ABOARD6 elem( reader );

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Board6 stream is not fully parsed" );
    }

    m_board->SetAuxOrigin( elem.sheetpos );
    m_board->SetGridOrigin( elem.sheetpos );

    // read layercount from stackup, because LAYERSETSCOUNT is not always correct?!
    size_t layercount = 0;
    for( size_t i                                = static_cast<size_t>( ALTIUM_LAYER::TOP_LAYER );
            i < elem.stackup.size() && i != 0; i = elem.stackup[i - 1].nextId, layercount++ )
        ;
    m_board->SetCopperLayerCount( layercount );

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
    for( size_t i                                = static_cast<size_t>( ALTIUM_LAYER::TOP_LAYER );
            i < elem.stackup.size() && i != 0; i = elem.stackup[i - 1].nextId, layercount++ )
    {
        auto layer = elem.stackup.at( i - 1 ); // array starts with 0, but stackup with 1
        m_layermap.insert(
                { static_cast<ALTIUM_LAYER>( i ), static_cast<PCB_LAYER_ID>( curLayer++ ) } );

        if( ( *it )->GetType() != BS_ITEM_TYPE_COPPER )
        {
            THROW_IO_ERROR( "Board6 stream, unexpected item while parsing stackup" );
        }
        ( *it )->SetThickness( layer.copperthick );

        if( ( *it )->GetBrdLayerId() == B_Cu )
        {
            if( layer.nextId != 0 )
            {
                THROW_IO_ERROR(
                        "Board6 stream, unexpected id while parsing last stackup layer" );
            }
            // overwrite entry from internal -> bottom
            m_layermap[static_cast<ALTIUM_LAYER>( i )] = B_Cu;
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

            DRAWSEGMENT* ds = new DRAWSEGMENT( m_board );
            m_board->Add( ds, ADD_MODE::APPEND );

            ds->SetWidth( m_board->GetDesignSettings().GetLineThickness( Edge_Cuts ) );
            ds->SetLayer( Edge_Cuts );

            if( !last->isRound && !cur->isRound )
            {
                ds->SetShape( STROKE_T::S_SEGMENT );
                ds->SetStart( last->position );
                ds->SetEnd( cur->position );
            }
            else if( cur->isRound )
            {
                ds->SetShape( STROKE_T::S_ARC );
                ds->SetAngle( -NormalizeAngleDegreesPos( cur->endangle - cur->startangle ) * 10. );

                double  startradiant   = DEG2RAD( cur->startangle );
                wxPoint arcStartOffset = wxPoint( KiROUND( std::cos( startradiant ) * cur->radius ),
                        -KiROUND( std::sin( startradiant ) * cur->radius ) );
                wxPoint arcStart       = cur->center + arcStartOffset;
                ds->SetCenter( cur->center );
                ds->SetArcStart( arcStart );

                if( !last->isRound )
                {
                    double  endradiant   = DEG2RAD( cur->endangle );
                    wxPoint arcEndOffset = wxPoint( KiROUND( std::cos( endradiant ) * cur->radius ),
                            -KiROUND( std::sin( endradiant ) * cur->radius ) );
                    wxPoint arcEnd       = cur->center + arcEndOffset;

                    DRAWSEGMENT* ds2 = new DRAWSEGMENT( m_board );
                    ds2->SetShape( STROKE_T::S_SEGMENT );
                    m_board->Add( ds2, ADD_MODE::APPEND );
                    ds2->SetWidth( m_board->GetDesignSettings().GetLineThickness( Edge_Cuts ) );
                    ds2->SetLayer( Edge_Cuts );
                    ds2->SetStart( last->position );

                    // TODO: this is more of a hack than the real solution
                    double lineLengthStart = GetLineLength( last->position, arcStart );
                    double lineLengthEnd   = GetLineLength( last->position, arcEnd );
                    if( lineLengthStart > lineLengthEnd )
                    {
                        ds2->SetEnd( cur->center + arcEndOffset );
                    }
                    else
                    {
                        ds2->SetEnd( cur->center + arcStartOffset );
                    }
                }
            }
            last = cur;
        }
    }
}

void ALTIUM_PCB::ParseClasses6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    BOARD_DESIGN_SETTINGS& designSettings = m_board->GetDesignSettings();

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        ACLASS6 elem( reader );

        if( elem.kind == ALTIUM_CLASS_KIND::NET_CLASS )
        {
            const NETCLASSPTR& netclass = std::make_shared<NETCLASS>( elem.name );
            designSettings.m_NetClasses.Add( netclass );

            for( const auto& name : elem.names )
            {
                netclass->Add(
                        name ); // TODO: it seems it can happen that we have names not attached to any net.
            }
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Classes6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseComponents6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    uint16_t componentId = 0;
    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        ACOMPONENT6 elem( reader );

        MODULE* module = new MODULE( m_board );
        m_board->Add( module, ADD_MODE::APPEND );
        m_components.emplace_back( module );

        wxString pack_ref = elem.sourcelibreference;
        wxString lib_ref  = elem.sourcefootprintlibrary; // TODO: remove ".PcbLib" part
        ReplaceIllegalFileNameChars( lib_ref, '_' );
        ReplaceIllegalFileNameChars( pack_ref, '_' );

        wxString key = !lib_ref.empty() ? lib_ref + ":" + pack_ref : pack_ref;

        LIB_ID fpID;
        fpID.Parse( key, LIB_ID::ID_PCB, true );
        module->SetFPID( fpID );

        module->SetPosition( elem.position );
        module->SetOrientationDegrees( elem.rotation );

        // KiCad netlisting requires parts to have non-digit + digit annotation.
        // If the reference begins with a number, we prepend 'UNK' (unknown) for the source designator
        wxString reference = elem.sourcedesignator;
        if( reference.find_first_not_of( "0123456789" ) == wxString::npos )
            reference.Prepend( "UNK" );
        module->SetReference( reference );

        module->SetLocked( elem.locked );
        module->Reference().SetVisible( elem.nameon );
        module->Value().SetVisible( elem.commenton );
        module->SetLayer( elem.layer == ALTIUM_LAYER::TOP_LAYER ? F_Cu : B_Cu );

        componentId++;
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Components6 stream is not fully parsed" );
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
        wxLogInfo( wxString::Format(
                _( "Dimension on Altium layer %d has no KiCad equivalent. Put it on Eco1_User instead" ),
                aElem.layer ) );
        klayer = Eco1_User;
    }

    wxPoint referencePoint0 = aElem.referencePoint.at( 0 );
    wxPoint referencePoint1 = aElem.referencePoint.at( 1 );

    DIMENSION* dimension = new DIMENSION( m_board );
    m_board->Add( dimension, ADD_MODE::APPEND );

    dimension->SetLayer( klayer );
    dimension->SetOrigin( referencePoint0, aElem.textprecission );
    if( referencePoint0 != aElem.xy1 )
    {
        /**
         * Basically REFERENCE0POINT and REFERENCE1POINT are the two end points of the dimension.
         * XY1 is the position of the arrow above REFERENCE0POINT. those three points are not necesarily
         * in 90degree angle, but KiCad requires this to show the correct measurements.
         *
         * Therefore, we take the vector of REFERENCE0POINT -> XY1, calculate the normal, and intersect it with
         * REFERENCE1POINT pointing the same direction as REFERENCE0POINT -> XY1. This should give us a valid
         * measurement point where we can place the drawsegment.
         */
        wxPoint direction = aElem.xy1 - referencePoint0;
        wxPoint directionNormalVector = wxPoint( -direction.y, direction.x );
        SEG     segm1( referencePoint0, referencePoint0 + directionNormalVector );
        SEG     segm2( referencePoint1, referencePoint1 + direction );
        wxPoint intersection( segm1.Intersect( segm2, true, true ).get() );
        dimension->SetEnd( intersection, aElem.textprecission );

        int height = static_cast<int>( EuclideanNorm( direction ) );
        if( direction.x <= 0 && direction.y <= 0 ) // TODO: I suspect this is not always correct
        {
            height = -height;
        }
        dimension->SetHeight( height, aElem.textprecission );
    }
    else
    {
        dimension->SetEnd( referencePoint1, aElem.textprecission );
    }

    dimension->SetWidth( aElem.linewidth );

    dimension->Text().SetThickness( aElem.textlinewidth );
    dimension->Text().SetTextSize( wxSize( aElem.textheight, aElem.textheight ) );
    dimension->Text().SetBold( aElem.textbold );
    dimension->Text().SetItalic( aElem.textitalic );

    switch( aElem.textunit )
    {
    case ALTIUM_UNIT::INCHES:
        dimension->SetUnits( EDA_UNITS::INCHES, false );
        break;
    case ALTIUM_UNIT::MILS:
        dimension->SetUnits( EDA_UNITS::INCHES, true );
        break;
    case ALTIUM_UNIT::MILLIMETERS:
    case ALTIUM_UNIT::CENTIMETER:
        dimension->SetUnits( EDA_UNITS::MILLIMETRES, false );
        break;
    default:
        break;
    }

    dimension->AdjustDimensionDetails( aElem.textprecission );
}

void ALTIUM_PCB::HelperParseDimensions6Leader( const ADIMENSION6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );
    if( klayer == UNDEFINED_LAYER )
    {
        wxLogInfo( wxString::Format(
                _( "Dimension on Altium layer %d has no KiCad equivalent. Put it on Eco1_User instead" ),
                aElem.layer ) );
        klayer = Eco1_User;
    }

    if( !aElem.referencePoint.empty() )
    {
        wxPoint referencePoint0 = aElem.referencePoint.at( 0 );

        // line
        wxPoint last = referencePoint0;
        for( size_t i = 1; i < aElem.referencePoint.size(); i++ )
        {
            DRAWSEGMENT* ds = new DRAWSEGMENT( m_board );
            m_board->Add( ds, ADD_MODE::APPEND );
            ds->SetShape( STROKE_T::S_SEGMENT );
            ds->SetLayer( klayer );
            ds->SetWidth( aElem.linewidth );
            ds->SetStart( last );
            ds->SetEnd( aElem.referencePoint.at( i ) );
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

                DRAWSEGMENT* ds1 = new DRAWSEGMENT( m_board );
                m_board->Add( ds1, ADD_MODE::APPEND );
                ds1->SetShape( STROKE_T::S_SEGMENT );
                ds1->SetLayer( klayer );
                ds1->SetWidth( aElem.linewidth );
                ds1->SetStart( referencePoint0 );
                ds1->SetEnd( referencePoint0 + arrVec );

                RotatePoint( &arrVec, -400. );

                DRAWSEGMENT* ds2 = new DRAWSEGMENT( m_board );
                m_board->Add( ds2, ADD_MODE::APPEND );
                ds2->SetShape( STROKE_T::S_SEGMENT );
                ds2->SetLayer( klayer );
                ds2->SetWidth( aElem.linewidth );
                ds2->SetStart( referencePoint0 );
                ds2->SetEnd( referencePoint0 + arrVec );
            }
        }
    }

    if( aElem.textPoint.empty() )
    {
        wxLogError( "No text position present for leader dimension object" );
        return;
    }

    TEXTE_PCB* text = new TEXTE_PCB( m_board );
    m_board->Add( text, ADD_MODE::APPEND );
    text->SetText( aElem.textformat );
    text->SetPosition( aElem.textPoint.at( 0 ) );
    text->SetLayer( klayer );
    text->SetTextSize( wxSize( aElem.textheight, aElem.textheight ) ); // TODO: parse text width
    text->SetThickness( aElem.textlinewidth );
    text->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT );
    text->SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_BOTTOM );
}

void ALTIUM_PCB::HelperParseDimensions6Datum( const ADIMENSION6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );
    if( klayer == UNDEFINED_LAYER )
    {
        wxLogInfo( wxString::Format(
                _( "Dimension on Altium layer %d has no KiCad equivalent. Put it on Eco1_User instead" ),
                aElem.layer ) );
        klayer = Eco1_User;
    }

    for( size_t i = 0; i < aElem.referencePoint.size(); i++ )
    {
        DRAWSEGMENT* ds1 = new DRAWSEGMENT( m_board );
        m_board->Add( ds1, ADD_MODE::APPEND );
        ds1->SetShape( STROKE_T::S_SEGMENT );
        ds1->SetLayer( klayer );
        ds1->SetWidth( aElem.linewidth );
        ds1->SetStart( aElem.referencePoint.at( i ) );
        // ds1->SetEnd( /* TODO: seems to be based on TEXTY */ );
    }
}

void ALTIUM_PCB::HelperParseDimensions6Center( const ADIMENSION6& aElem )
{
    PCB_LAYER_ID klayer = GetKicadLayer( aElem.layer );
    if( klayer == UNDEFINED_LAYER )
    {
        wxLogInfo( wxString::Format(
                _( "Dimension on Altium layer %d has no KiCad equivalent. Put it on Eco1_User instead" ),
                aElem.layer ) );
        klayer = Eco1_User;
    }

    DRAWSEGMENT* ds1 = new DRAWSEGMENT( m_board );
    m_board->Add( ds1, ADD_MODE::APPEND );
    ds1->SetShape( STROKE_T::S_SEGMENT );
    ds1->SetLayer( klayer );
    ds1->SetWidth( aElem.linewidth );

    wxPoint vec1 = wxPoint( 0, aElem.height / 2 );
    RotatePoint( &vec1, aElem.angle * 10. );
    ds1->SetStart( aElem.xy1 + vec1 );
    ds1->SetEnd( aElem.xy1 - vec1 );

    DRAWSEGMENT* ds2 = new DRAWSEGMENT( m_board );
    m_board->Add( ds2, ADD_MODE::APPEND );
    ds2->SetShape( STROKE_T::S_SEGMENT );
    ds2->SetLayer( klayer );
    ds2->SetWidth( aElem.linewidth );

    wxPoint vec2 = wxPoint( aElem.height / 2, 0 );
    RotatePoint( &vec2, aElem.angle * 10. );
    ds2->SetStart( aElem.xy1 + vec2 );
    ds2->SetEnd( aElem.xy1 - vec2 );
}


void ALTIUM_PCB::ParseDimensions6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
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
            wxLogInfo( wxString::Format( "Ignore dimension object of kind %d", elem.kind ) );
            // HelperParseDimensions6Datum( elem );
            break;
        case ALTIUM_DIMENSION_KIND::CENTER:
            HelperParseDimensions6Center( elem );
            break;
        default:
            wxLogInfo( wxString::Format( "Ignore dimension object of kind %d", elem.kind ) );
            break;
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Dimensions6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseNets6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    wxASSERT( m_num_nets == 0 );
    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        ANET6 elem( reader );

        m_board->Add( new NETINFO_ITEM( m_board, elem.name, ++m_num_nets ), ADD_MODE::APPEND );
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Nets6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParsePolygons6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        APOLYGON6 elem( reader );

        PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );
        if( klayer == UNDEFINED_LAYER )
        {
            wxLogWarning( wxString::Format(
                    _( "Polygon on Altium layer %d has no KiCad equivalent. Ignore it instead" ),
                    elem.layer ) );
            m_polygons.emplace_back( nullptr );
            continue;
        }

        ZONE_CONTAINER* zone = new ZONE_CONTAINER( m_board );
        m_board->Add( zone, ADD_MODE::APPEND );
        m_polygons.emplace_back( zone );

        zone->SetNetCode( GetNetCode( elem.net ) );
        zone->SetLayer( klayer );
        zone->SetPosition( elem.vertices.at( 0 ).position );
        zone->SetLocked( elem.locked );

        for( auto& vertice : elem.vertices )
        {
            zone->AppendCorner( vertice.position, -1 ); // TODO: arcs
        }

        // TODO: more flexible rule parsing
        const ARULE6* clearanceRule = GetRuleDefault( ALTIUM_RULE_KIND::PLANE_CLEARANCE );
        if( clearanceRule != nullptr )
        {
            zone->SetZoneClearance( clearanceRule->planeclearanceClearance );
        }
        const ARULE6* polygonConnectRule = GetRuleDefault( ALTIUM_RULE_KIND::POLYGON_CONNECT );
        if( clearanceRule != nullptr )
        {
            // TODO: correct variables?
            zone->SetThermalReliefCopperBridge(
                    polygonConnectRule->polygonconnectReliefconductorwidth );
            zone->SetThermalReliefGap( polygonConnectRule->polygonconnectAirgapwidth );
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

        if( elem.hatchstyle != ALTIUM_POLYGON_HATCHSTYLE::SOLID )
        {
            zone->SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
            zone->SetHatchFillTypeThickness( elem.trackwidth );
            if( elem.hatchstyle == ALTIUM_POLYGON_HATCHSTYLE::NONE )
            {
                // use a small hack to get us only an outline (hopefully)
                const EDA_RECT& bbox = zone->GetBoundingBox();
                zone->SetHatchFillTypeGap( std::max( bbox.GetHeight(), bbox.GetWidth() ) );
            }
            else
            {
                zone->SetHatchFillTypeGap( elem.gridsize - elem.trackwidth );
            }
            zone->SetHatchFillTypeOrientation(
                    elem.hatchstyle == ALTIUM_POLYGON_HATCHSTYLE::DEGREE_45 ? 45 : 0 );
        }

        zone->SetHatch(
                ZONE_HATCH_STYLE::DIAGONAL_EDGE, ZONE_CONTAINER::GetDefaultHatchPitch(), true );
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Polygons6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseRules6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        ARULE6 elem( reader );

        m_rules[elem.kind].emplace_back( elem );
    }

    // sort rules by priority
    for( auto&& val : m_rules )
    {
        std::sort( val.second.begin(), val.second.end(),
                []( const auto& lhs, const auto& rhs ) { return lhs.priority < rhs.priority; } );
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Rules6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseBoardRegionsData(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        AREGION6 elem( reader, false );

        // TODO: implement?
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "BoardRegions stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseShapeBasedRegions6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        AREGION6 elem( reader, true );

        if( elem.kind == ALTIUM_REGION_KIND::BOARD_CUTOUT )
        {
            HelperCreateBoardOutline( elem.vertices );
        }
        else if( elem.kind == ALTIUM_REGION_KIND::POLYGON_CUTOUT )
        {
            ZONE_CONTAINER* zone = new ZONE_CONTAINER( m_board );
            m_board->Add( zone, ADD_MODE::APPEND );

            if( elem.kind == ALTIUM_REGION_KIND::POLYGON_CUTOUT || elem.is_keepout )
            {
                zone->SetIsKeepout( true );
                zone->SetDoNotAllowTracks( false );
                zone->SetDoNotAllowVias( false );
                zone->SetDoNotAllowCopperPour( true );
            }
            else
            {
                zone->SetNetCode( GetNetCode( elem.net ) );
            }

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
                    wxLogInfo( wxString::Format(
                            _( "Zone on Altium layer %d has no KiCad equivalent. Put it on Eco1_User instead" ),
                            elem.layer ) );
                    klayer = Eco1_User;
                }
                zone->SetLayer( klayer );
            }

            zone->SetPosition( elem.vertices.at( 0 ).position );

            for( auto& vertice : elem.vertices )
            {
                zone->AppendCorner( vertice.position, -1 );
            }

            zone->SetHatch(
                    ZONE_HATCH_STYLE::DIAGONAL_EDGE, ZONE_CONTAINER::GetDefaultHatchPitch(), true );
        }
        else if( elem.kind == ALTIUM_REGION_KIND::COPPER )
        {
            if( elem.subpolyindex == ALTIUM_POLYGON_NONE )
            {
                PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );
                if( klayer == UNDEFINED_LAYER )
                {
                    wxLogInfo( wxString::Format(
                            _( "Polygon on Altium layer %d has no KiCad equivalent. Put it on Eco1_User instead" ),
                            elem.layer ) );
                    klayer = Eco1_User;
                }

                DRAWSEGMENT* ds = new DRAWSEGMENT( m_board );
                m_board->Add( ds, ADD_MODE::APPEND );
                ds->SetShape( STROKE_T::S_POLYGON );
                ds->SetLayer( klayer );
                ds->SetWidth( 0 );

                std::vector<wxPoint> pts;
                for( auto& vertice : elem.vertices )
                {
                    pts.push_back( vertice.position );
                }
                ds->SetPolyPoints( pts );
            }
        }
        else
        {
            wxLogError( wxString::Format(
                    "Ignore polygon shape of kind %d on layer %s, because not implemented yet",
                    elem.kind, LSET::Name( GetKicadLayer( elem.layer ) ) ) );
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "ShapeBasedRegions6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseRegions6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    for( ZONE_CONTAINER* zone : m_polygons )
    {
        if( zone != nullptr )
        {
            zone->UnFill(); // just to be sure
        }
    }

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        AREGION6 elem( reader, false );

#if 0 // TODO: it seems this code has multiple issues right now, and we can manually fill anyways
        if( elem.subpolyindex != ALTIUM_POLYGON_NONE )
        {
            if( m_polygons.size() <= elem.subpolyindex )
            {
                THROW_IO_ERROR(  wxString::Format(
                        "Region stream tries to access polygon id %d of %d existing polygons",
                        elem.subpolyindex, m_polygons.size() ) );
            }

            ZONE_CONTAINER *zone = m_polygons.at(elem.subpolyindex);

            if( zone == nullptr )
            {
                continue; // we know the zone id, but because we do not know the layer we did not add it!
            }

            SHAPE_LINE_CHAIN linechain;
            for( auto& vertice : elem.vertices )
            {
                linechain.Append( vertice.position );
            }
            linechain.Append( elem.vertices.at( 0 ).position );
            linechain.SetClosed( true );

            SHAPE_POLY_SET polyset;
            polyset.AddOutline( linechain );
            polyset.BooleanAdd( zone->GetFilledPolysList(), SHAPE_POLY_SET::POLYGON_MODE::PM_STRICTLY_SIMPLE );

            zone->SetFilledPolysList( polyset );
            zone->SetIsFilled( true );
        }
#endif
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Regions6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseArcs6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        AARC6 elem( reader );

        if( elem.is_polygonoutline || elem.subpolyindex != ALTIUM_POLYGON_NONE )
        {
            continue;
        }

        // element in plane is in fact substracted from the plane. Should be already done by Altium?
        /*if( IsAltiumLayerAPlane( elem.layer ) )
        {
            continue;
        }*/

        PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );
        if( klayer == UNDEFINED_LAYER )
        {
            wxLogInfo( wxString::Format(
                    _( "Arc on Altium layer %d has no KiCad equivalent. Put it on Eco1_User instead" ),
                    elem.layer ) );
            klayer = Eco1_User;
        }

        if( elem.is_keepout || IsAltiumLayerAPlane( elem.layer ) )
        {
            DRAWSEGMENT ds( nullptr ); // just a helper to get the graphic
            ds.SetWidth( elem.width );
            ds.SetCenter( elem.center );
            if( elem.startangle == 0. && elem.endangle == 360. )
            { // TODO: other variants to define circle?
                ds.SetShape( STROKE_T::S_CIRCLE );
                ds.SetArcStart( elem.center - wxPoint( 0, elem.radius ) );
            }
            else
            {
                ds.SetShape( STROKE_T::S_ARC );
                ds.SetAngle( -NormalizeAngleDegreesPos( elem.endangle - elem.startangle ) * 10. );

                double  startradiant   = DEG2RAD( elem.startangle );
                wxPoint arcStartOffset = wxPoint( KiROUND( std::cos( startradiant ) * elem.radius ),
                        -KiROUND( std::sin( startradiant ) * elem.radius ) );
                ds.SetArcStart( elem.center + arcStartOffset );
            }

            ZONE_CONTAINER* zone = new ZONE_CONTAINER( m_board );
            m_board->Add( zone, ADD_MODE::APPEND );

            zone->SetLayer( klayer );
            zone->SetIsKeepout( true );
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowCopperPour( true );

            ds.TransformShapeWithClearanceToPolygon( *zone->Outline(), 0, ARC_HIGH_DEF, false );
            zone->Outline()->Simplify(
                    SHAPE_POLY_SET::PM_STRICTLY_SIMPLE ); // the outline is not a single polygon!

            zone->SetHatch(
                    ZONE_HATCH_STYLE::DIAGONAL_EDGE, ZONE_CONTAINER::GetDefaultHatchPitch(), true );
            continue;
        }

        if( klayer >= F_Cu && klayer <= B_Cu )
        {
            double  angle          = -NormalizeAngleDegreesPos( elem.endangle - elem.startangle );
            double  startradiant   = DEG2RAD( elem.startangle );
            wxPoint arcStartOffset = wxPoint( KiROUND( std::cos( startradiant ) * elem.radius ),
                    -KiROUND( std::sin( startradiant ) * elem.radius ) );

            SHAPE_ARC shapeArc( elem.center, elem.center + arcStartOffset, angle, elem.width );
            ARC*      arc = new ARC( m_board, &shapeArc );
            m_board->Add( arc, ADD_MODE::APPEND );

            arc->SetWidth( elem.width );
            arc->SetLayer( klayer );
            arc->SetNetCode( GetNetCode( elem.net ) );
        }
        else
        {
            // TODO: better approach to select if item belongs to a MODULE
            DRAWSEGMENT* ds = nullptr;
            if( elem.component == ALTIUM_COMPONENT_NONE )
            {
                ds = new DRAWSEGMENT( m_board );
                m_board->Add( ds, ADD_MODE::APPEND );
            }
            else
            {
                if( m_components.size() <= elem.component )
                {
                    THROW_IO_ERROR( wxString::Format(
                            "Arcs6 stream tries to access component id %d of %d existing components",
                            elem.component, m_components.size() ) );
                }
                MODULE* module = m_components.at( elem.component );
                ds             = new EDGE_MODULE( module );
                module->Add( ds, ADD_MODE::APPEND );
            }

            ds->SetCenter( elem.center );
            ds->SetWidth( elem.width );
            ds->SetLayer( klayer );

            if( elem.startangle == 0. && elem.endangle == 360. )
            { // TODO: other variants to define circle?
                ds->SetShape( STROKE_T::S_CIRCLE );
                ds->SetArcStart( elem.center - wxPoint( 0, elem.radius ) );
            }
            else
            {
                ds->SetShape( STROKE_T::S_ARC );
                ds->SetAngle( -NormalizeAngleDegreesPos( elem.endangle - elem.startangle ) * 10. );

                double  startradiant   = DEG2RAD( elem.startangle );
                wxPoint arcStartOffset = wxPoint( KiROUND( std::cos( startradiant ) * elem.radius ),
                        -KiROUND( std::sin( startradiant ) * elem.radius ) );
                ds->SetArcStart( elem.center + arcStartOffset );
            }

            if( elem.component != ALTIUM_COMPONENT_NONE )
            {
                auto em = dynamic_cast<EDGE_MODULE*>( ds );

                if( em )
                    em->SetLocalCoord();
            }
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Arcs6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParsePads6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        APAD6 elem( reader );

        // Create Pad
        MODULE* module = nullptr;
        if( elem.component == ALTIUM_COMPONENT_NONE )
        {
            module = new MODULE( m_board ); // We cannot add a pad directly into the PCB
            m_board->Add( module, ADD_MODE::APPEND );
            module->SetPosition( elem.position );
        }
        else
        {
            if( m_components.size() <= elem.component )
            {
                THROW_IO_ERROR( wxString::Format(
                        "Pads6 stream tries to access component id %d of %d existing components",
                        elem.component, m_components.size() ) );
            }
            module = m_components.at( elem.component );
        }

        D_PAD*  pad    = new D_PAD( module );
        module->Add( pad, ADD_MODE::APPEND );

        pad->SetName( elem.name );
        pad->SetNetCode( GetNetCode( elem.net ) );
        pad->SetLocked( elem.is_locked );

        pad->SetPosition( elem.position );
        pad->SetOrientationDegrees( elem.direction );
        pad->SetLocalCoord();

        pad->SetSize( elem.topsize );

        if( elem.holesize == 0 )
        {
            if( elem.layer == ALTIUM_LAYER::MULTI_LAYER )
            {
                wxLogError( wxString::Format(
                        _( "Pad '%s' of Footprint %s marked as multilayer, but it is an SMT pad" ),
                        elem.name, module->GetReference() ) );
            }
            pad->SetAttribute( PAD_ATTR_T::PAD_ATTRIB_SMD );
        }
        else
        {
            if( elem.layer != ALTIUM_LAYER::MULTI_LAYER )
            {
                // TODO: I assume other values are possible as well?
                wxLogError( wxString::Format(
                        "Pad '%s' of Footprint %s is not marked as multilayer, but it is an THT pad",
                        elem.name, module->GetReference() ) );
            }
            pad->SetAttribute( elem.plated ? PAD_ATTR_T::PAD_ATTRIB_STANDARD :
                                             PAD_ATTR_T::PAD_ATTRIB_HOLE_NOT_PLATED );
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
                    wxLogWarning( wxString::Format(
                            _( "Pad '%s' of Footprint %s has a square hole. KiCad does not support this yet" ),
                            elem.name, module->GetReference() ) );
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
                            wxLogWarning( wxString::Format(
                                    _( "Pad '%s' of Footprint %s has a hole-rotation of %f degree. KiCad only supports 90 degree angles" ),
                                    elem.name, module->GetReference(), normalizedSlotrotation ) );
                        }

                        pad->SetDrillSize( wxSize( elem.holesize, elem.sizeAndShape->slotsize ) );
                    }
                }
                break;

                default:
                case ALTIUM_PAD_HOLE_SHAPE::UNKNOWN:
                    wxLogError( wxString::Format(
                           "Pad '%s' of Footprint %s uses a hole of unknown kind %d",
                            elem.name, module->GetReference(), elem.sizeAndShape->holeshape ) );
                    pad->SetDrillShape( PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_CIRCLE );
                    pad->SetDrillSize( wxSize( elem.holesize, elem.holesize ) ); // Workaround
                    break;
                }
            }
        }

        if( elem.padmode != ALTIUM_PAD_MODE::SIMPLE )
        {
            wxLogWarning( wxString::Format(
                    _( "Pad '%s' of Footprint %s uses a complex pad stack (kind %d), which is not supported yet" ),
                    elem.name, module->GetReference(), elem.padmode ) );
        }

        switch( elem.topshape )
        {
        case ALTIUM_PAD_SHAPE::RECT:
            pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_RECT );
            break;
        case ALTIUM_PAD_SHAPE::CIRCLE:
            if( elem.sizeAndShape
                    && elem.sizeAndShape->alt_shape[0] == ALTIUM_PAD_SHAPE_ALT::ROUNDRECT )
            {
                pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_ROUNDRECT ); // 100 = round, 0 = rectangular
                double ratio = elem.sizeAndShape->cornerradius[0] / 200.;
                pad->SetRoundRectRadiusRatio( ratio );
            }
            else if( elem.topsize.x == elem.topsize.y )
            {
                pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_CIRCLE );
            }
            else
            {
                pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_OVAL );
            }
            break;
        case ALTIUM_PAD_SHAPE::OCTAGONAL:
            pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_CHAMFERED_RECT );
            pad->SetChamferPositions( RECT_CHAMFER_ALL );
            pad->SetChamferRectRatio( 0.25 );
            break;
        case ALTIUM_PAD_SHAPE::UNKNOWN:
        default:
            wxLogError( wxString::Format( "Pad '%s' of Footprint %s uses a unknown pad-shape",
                    elem.name, module->GetReference() ) );
            break;
        }

        switch( elem.layer )
        {
        case ALTIUM_LAYER::TOP_LAYER:
            pad->SetLayer( F_Cu );
            pad->SetLayerSet( D_PAD::SMDMask() );
            break;
        case ALTIUM_LAYER::BOTTOM_LAYER:
            pad->SetLayer( B_Cu );
            pad->SetLayerSet( FlipLayerMask( D_PAD::SMDMask() ) );
            break;
        case ALTIUM_LAYER::MULTI_LAYER:
        default:
            pad->SetLayerSet( elem.plated ? D_PAD::StandardMask() : D_PAD::UnplatedHoleMask() );
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

void ALTIUM_PCB::ParseVias6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        AVIA6 elem( reader );

        VIA* via = new VIA( m_board );
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
            wxLogError( wxString::Format(
                    "Via from layer %d <-> %d uses non-copper layer. This should not happen.",
                    elem.layer_start, elem.layer_end ) );
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

void ALTIUM_PCB::ParseTracks6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        ATRACK6 elem( reader );

        if( elem.is_polygonoutline || elem.subpolyindex != ALTIUM_POLYGON_NONE )
        {
            continue;
        }

        // element in plane is in fact substracted from the plane. Already done by Altium?
        /*if( IsAltiumLayerAPlane( elem.layer ) )
        {
            continue;
        }*/

        PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );
        if( klayer == UNDEFINED_LAYER )
        {
            wxLogInfo( wxString::Format(
                    _( "Track on Altium layer %d has no KiCad equivalent. Put it on Eco1_User instead" ),
                    elem.layer ) );
            klayer = Eco1_User;
        }

        if( elem.is_keepout || IsAltiumLayerAPlane( elem.layer ) )
        {
            DRAWSEGMENT ds( nullptr ); // just a helper to get the graphic
            ds.SetShape( STROKE_T::S_SEGMENT );
            ds.SetStart( elem.start );
            ds.SetEnd( elem.end );
            ds.SetWidth( elem.width );

            ZONE_CONTAINER* zone = new ZONE_CONTAINER( m_board );
            m_board->Add( zone, ADD_MODE::APPEND );
            zone->SetLayer( klayer );
            zone->SetIsKeepout( true );
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowCopperPour( true );

            ds.TransformShapeWithClearanceToPolygon( *zone->Outline(), 0, ARC_HIGH_DEF, false );

            zone->SetHatch(
                    ZONE_HATCH_STYLE::DIAGONAL_EDGE, ZONE_CONTAINER::GetDefaultHatchPitch(), true );
            continue;
        }

        if( klayer >= F_Cu && klayer <= B_Cu )
        {
            TRACK* track = new TRACK( m_board );
            m_board->Add( track, ADD_MODE::APPEND );

            track->SetStart( elem.start );
            track->SetEnd( elem.end );
            track->SetWidth( elem.width );
            track->SetLayer( klayer );
            track->SetNetCode( GetNetCode( elem.net ) );
        }
        else
        {
            DRAWSEGMENT* ds = nullptr;

            if( elem.component == ALTIUM_COMPONENT_NONE )
            {
                ds = new DRAWSEGMENT( m_board );
                ds->SetShape( STROKE_T::S_SEGMENT );
                m_board->Add( ds, ADD_MODE::APPEND );

                ds->SetStart( elem.start );
                ds->SetEnd( elem.end );
            }
            else
            {
                if( m_components.size() <= elem.component )
                {
                    THROW_IO_ERROR( wxString::Format(
                            "Tracks6 stream tries to access component id %d of %d existing components",
                            elem.component, m_components.size() ) );
                }
                MODULE*      module = m_components.at( elem.component );
                EDGE_MODULE* em     = new EDGE_MODULE( module, STROKE_T::S_SEGMENT );
                module->Add( em, ADD_MODE::APPEND );

                em->SetStart( elem.start );
                em->SetEnd( elem.end );
                em->SetLocalCoord();

                ds = em;
            }

            ds->SetWidth( elem.width );

            ds->SetLayer( klayer );
        }

        reader.SkipSubrecord();
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Tracks6 stream is not fully parsed" );
    }
}

void ALTIUM_PCB::ParseTexts6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        ATEXT6 elem( reader );

        // TODO: better approach to select if item belongs to a MODULE
        EDA_TEXT*   tx  = nullptr;
        BOARD_ITEM* itm = nullptr;
        if( elem.component == ALTIUM_COMPONENT_NONE )
        {
            TEXTE_PCB* txp = new TEXTE_PCB( m_board );
            tx             = txp;
            itm            = txp;
            m_board->Add( txp, ADD_MODE::APPEND );
        }
        else
        {
            if( m_components.size() <= elem.component )
            {
                THROW_IO_ERROR( wxString::Format(
                        "Texts6 stream tries to access component id %d of %d existing components",
                        elem.component, m_components.size() ) );
            }
            MODULE*       module = m_components.at( elem.component );
            TEXTE_MODULE* txm;
            if( elem.isDesignator )
            {
                txm = &module->Reference();
            }
            else if( elem.isComment )
            {
                txm = &module->Value();
            }
            else
            {
                txm = new TEXTE_MODULE( module );
                module->Add( txm, ADD_MODE::APPEND );
            }

            tx  = txm;
            itm = txm;
        }

        if( !elem.isDesignator && elem.text == ".Designator" )
        {
            tx->SetText( "%R" );
        }
        else
        {
            tx->SetText( elem.text );
        }

        itm->SetPosition( elem.position );
        tx->SetTextAngle( elem.rotation * 10. );

        if( elem.component != ALTIUM_COMPONENT_NONE )
        {
            TEXTE_MODULE* txm = dynamic_cast<TEXTE_MODULE*>( tx );

            if( txm )
            {
                if( elem.isDesignator || elem.isComment )
                {
                    double orientation =
                            static_cast<const MODULE*>( txm->GetParent() )->GetOrientation();
                    txm->SetTextAngle( orientation + txm->GetTextAngle() );
                }
                txm->SetLocalCoord();
            }
        }

        PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );
        if( klayer == UNDEFINED_LAYER )
        {
            wxLogInfo( wxString::Format(
                    _( "Text on Altium layer %d has no KiCad equivalent. Put it on Eco1_User instead" ),
                    elem.layer ) );
            klayer = Eco1_User;
        }
        itm->SetLayer( klayer );

        tx->SetTextSize( wxSize( elem.height, elem.height ) ); // TODO: parse text width
        tx->SetThickness( elem.strokewidth );
        tx->SetMirrored( elem.mirrored );
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

void ALTIUM_PCB::ParseFills6Data(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    ALTIUM_PARSER reader( aReader, aEntry );

    while( reader.GetRemainingBytes() >= 4 /* TODO: use Header section of file */ )
    {
        AFILL6 elem( reader );

        wxPoint p11( elem.pos1.x, elem.pos1.y );
        wxPoint p12( elem.pos1.x, elem.pos2.y );
        wxPoint p22( elem.pos2.x, elem.pos2.y );
        wxPoint p21( elem.pos2.x, elem.pos1.y );

        wxPoint center( ( elem.pos1.x + elem.pos2.x ) / 2, ( elem.pos1.y + elem.pos2.y ) / 2 );

        PCB_LAYER_ID klayer = GetKicadLayer( elem.layer );
        if( klayer == UNDEFINED_LAYER )
        {
            wxLogInfo( wxString::Format(
                    _( "Fill on Altium layer %d has no KiCad equivalent. Put it on Eco1_User instead" ),
                    elem.layer ) );
            klayer = Eco1_User;
        }

        if( elem.is_keepout || elem.net != ALTIUM_NET_UNCONNECTED )
        {
            ZONE_CONTAINER* zone = new ZONE_CONTAINER( m_board );
            m_board->Add( zone, ADD_MODE::APPEND );

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
            zone->SetZoneClearance( 0 );
            zone->SetPadConnection( ZONE_CONNECTION::FULL );

            if( elem.is_keepout )
            {
                zone->SetIsKeepout( true );
                zone->SetDoNotAllowTracks( false );
                zone->SetDoNotAllowVias( false );
                zone->SetDoNotAllowCopperPour( true );
            }

            if( elem.rotation != 0. )
            {
                zone->Rotate( center, elem.rotation * 10 );
            }

            zone->SetHatch(
                    ZONE_HATCH_STYLE::DIAGONAL_EDGE, ZONE_CONTAINER::GetDefaultHatchPitch(), true );
        }
        else
        {
            DRAWSEGMENT* ds = new DRAWSEGMENT( m_board );
            m_board->Add( ds, ADD_MODE::APPEND );

            ds->SetShape( STROKE_T::S_POLYGON );
            ds->SetLayer( klayer );

            ds->SetPolyPoints( { p11, p12, p22, p21 } );

            if( elem.rotation != 0. )
            {
                ds->Rotate( center, elem.rotation * 10 );
            }
        }
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "Fills6 stream is not fully parsed" );
    }
}
