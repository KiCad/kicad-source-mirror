/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_io_pads.h"
#include "pads_layer_mapper.h"

#include <algorithm>
#include <cmath>
#include <functional>

#include <board.h>
#include <pcb_track.h>
#include <pcb_text.h>
#include <footprint.h>
#include <zone.h>

#include "pads_parser.h"
#include <io/pads/pads_unit_converter.h>
#include <io/pads/pads_common.h>

#include <netinfo.h>
#include <wx/log.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <core/mirror.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_dimension.h>
#include <board_design_settings.h>
#include <netclass.h>
#include <geometry/eda_angle.h>
#include <geometry/shape_arc.h>
#include <pcb_group.h>
#include <string_utils.h>
#include <progress_reporter.h>
#include <reporter.h>
#include <advanced_config.h>
#include <locale_io.h>

PCB_IO_PADS::PCB_IO_PADS() : PCB_IO( "PADS ASCII" )
{
    LAYER_MAPPABLE_PLUGIN::RegisterCallback(
            std::bind( &PCB_IO_PADS::DefaultLayerMappingCallback, this, std::placeholders::_1 ) );
}


PCB_IO_PADS::~PCB_IO_PADS()
{
}


const IO_BASE::IO_FILE_DESC PCB_IO_PADS::GetBoardFileDesc() const
{
    IO_FILE_DESC desc;
    desc.m_FileExtensions.push_back( "asc" );
    desc.m_Description = "PADS ASCII";
    return desc;
}


const IO_BASE::IO_FILE_DESC PCB_IO_PADS::GetLibraryDesc() const
{
    // PADS ASCII doesn't really support libraries in the KiCad sense,
    // but we must implement this.
    return IO_FILE_DESC( "PADS ASCII Library", { "asc" } );
}


long long PCB_IO_PADS::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    return 0;
}


bool PCB_IO_PADS::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    std::ifstream file( aFileName.fn_str() );

    if( !file.is_open() )
        return false;

    std::string line;

    if( std::getline( file, line ) )
    {
        if( line.find( "!PADS-" ) != std::string::npos )
            return true;
    }

    return false;
}


BOARD* PCB_IO_PADS::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                               const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    LOCALE_IO setlocale;

    std::unique_ptr<BOARD> board( aAppendToMe ? aAppendToMe : new BOARD() );

    if( m_reporter )
        m_reporter->Report( _( "Starting PADS PCB import" ), RPT_SEVERITY_INFO );

    if( m_progressReporter )
        m_progressReporter->SetNumPhases( 4 );

    PADS_IO::PARSER parser;

    try
    {
        parser.Parse( aFileName );
    }
    catch( const std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( "Error parsing PADS file: %s", e.what() ) );
    }

    m_loadBoard = board.get();
    m_parser = &parser;
    m_testPointIndex = 1;

    try
    {
        if( m_progressReporter )
            m_progressReporter->BeginPhase( 1 );

        loadBoardSetup();
        loadNets();

        if( m_progressReporter )
            m_progressReporter->BeginPhase( 2 );

        loadFootprints();
        loadReuseBlockGroups();
        loadTestPoints();
        loadTexts();

        if( m_progressReporter )
            m_progressReporter->BeginPhase( 3 );

        loadTracksAndVias();
        loadClusterGroups();
        loadZones();
        loadBoardOutline();
        loadDimensions();
        loadKeepouts();
        loadGraphicLines();
        generateDrcRules( aFileName );
        reportStatistics();
    }
    catch( ... )
    {
        clearLoadingState();
        throw;
    }

    clearLoadingState();
    return board.release();
}


void PCB_IO_PADS::loadNets()
{
    const auto& nets = m_parser->GetNets();

    for( const auto& pads_net : nets )
        ensureNet( pads_net.name );

    for( const auto& pads_net : nets )
    {
        for( const auto& pin : pads_net.pins )
        {
            std::string key = pin.ref_des + "." + pin.pin_name;
            m_pinToNetMap[key] = pads_net.name;
        }
    }

    const auto& route_nets = m_parser->GetRoutes();

    for( const auto& route : route_nets )
    {
        for( const auto& pin : route.pins )
        {
            std::string key = pin.ref_des + "." + pin.pin_name;

            if( m_pinToNetMap.find( key ) == m_pinToNetMap.end() )
                m_pinToNetMap[key] = route.net_name;
        }
    }

    for( const auto& route : route_nets )
        ensureNet( route.net_name );

    for( const auto& pour_def : m_parser->GetPours() )
        ensureNet( pour_def.net_name );

    const auto& reuse_blocks = m_parser->GetReuseBlocks();

    for( const auto& [blockName, block] : reuse_blocks )
    {
        for( const std::string& partName : block.part_names )
        {
            m_partToBlockMap[partName] = blockName;
        }
    }
}


void PCB_IO_PADS::loadFootprints()
{
    const auto& decals = m_parser->GetPartDecals();
    const auto& part_types = m_parser->GetPartTypes();
    const auto& partInstanceAttrs = m_parser->GetPartInstanceAttrs();
    const auto& parts = m_parser->GetParts();

    for( const auto& pads_part : parts )
    {
        FOOTPRINT* footprint = new FOOTPRINT( m_loadBoard );
        footprint->SetReference( pads_part.name );

        // Generate deterministic UUID for cross-probe linking between schematic and PCB.
        // The schematic importer uses the same algorithm, enabling selection sync.
        KIID symbolUuid = PADS_COMMON::GenerateDeterministicUuid( pads_part.name );
        KIID_PATH path;
        path.push_back( symbolUuid );
        footprint->SetPath( path );

        // Resolve Decal Name
        std::string decal_name = pads_part.decal;

        // When explicit_decal is true, pads_part.decal already contains the direct decal name
        if( !pads_part.explicit_decal && decals.find( decal_name ) == decals.end() )
        {
            auto part_type_it = part_types.find( decal_name );

            if( part_type_it != part_types.end() )
                decal_name = part_type_it->second.decal_name;
        }

        // Handle Alternate Decals (separated by :)
        // The part's alt_decal_index specifies which alternate to use (0-based).
        std::stringstream ss( decal_name );
        std::string segment;
        std::vector<std::string> decal_list;

        while( std::getline( ss, segment, ':' ) )
        {
            decal_list.push_back( segment );
        }

        std::string actual_decal_name;
        bool found_valid_decal = false;

        if( pads_part.alt_decal_index >= 0
            && static_cast<size_t>( pads_part.alt_decal_index ) < decal_list.size() )
        {
            const std::string& alt_decal = decal_list[pads_part.alt_decal_index];

            if( decals.find( alt_decal ) != decals.end() )
            {
                actual_decal_name = alt_decal;
                found_valid_decal = true;
            }
        }

        if( !found_valid_decal )
        {
            for( const std::string& decal : decal_list )
            {
                if( decals.find( decal ) != decals.end() )
                {
                    actual_decal_name = decal;
                    found_valid_decal = true;
                    break;
                }
            }
        }

        if( found_valid_decal )
        {
            decal_name = actual_decal_name;
        }

        LIB_ID fpid;
        fpid.SetLibItemName( wxString::FromUTF8( decal_name ) );
        footprint->SetFPID( fpid );

        footprint->SetValue( pads_part.decal );

        if( !pads_part.alternate_decals.empty() )
        {
            wxString alternates;

            for( size_t i = 0; i < pads_part.alternate_decals.size(); ++i )
            {
                if( i > 0 )
                    alternates += wxT( ", " );

                alternates += wxString::FromUTF8( pads_part.alternate_decals[i] );
            }

            PCB_FIELD* field = new PCB_FIELD( footprint, FIELD_T::USER, wxT( "PADS_Alternate_Decals" ) );
            field->SetLayer( Cmts_User );
            field->SetVisible( false );
            field->SetText( alternates );
            footprint->Add( field );
        }

        auto partCoordScaler = [&]( double val, bool is_x ) {
            double origin = is_x ? m_originX : m_originY;

            double part_factor = m_scaleFactor;

            if( !m_parser->IsBasicUnits() )
            {
                if( pads_part.units == "M" ) part_factor = PADS_UNIT_CONVERTER::MILS_TO_NM;
                else if( pads_part.units == "MM" ) part_factor = PADS_UNIT_CONVERTER::MM_TO_NM;
                else if( pads_part.units == "I" ) part_factor = PADS_UNIT_CONVERTER::INCHES_TO_NM;
                else if( pads_part.units == "D" ) part_factor = PADS_UNIT_CONVERTER::MILS_TO_NM;
            }

            long long origin_nm = static_cast<long long>( std::round( origin * m_scaleFactor ) );
            long long val_nm = static_cast<long long>( std::round( val * part_factor ) );

            long long res_nm = val_nm - origin_nm;

            if( !is_x ) res_nm = -res_nm;

            return static_cast<int>( res_nm );
        };

        footprint->SetPosition( VECTOR2I( partCoordScaler( pads_part.location.x, true ),
                                           partCoordScaler( pads_part.location.y, false ) ) );

        // Both PADS and KiCad use counter-clockwise positive rotation convention.
        // The Y-axis flip (PADS Y-up vs KiCad Y-down) does not affect rotation direction,
        // so we use the PADS rotation value directly for both top and bottom layer parts.
        // For bottom-layer parts, the subsequent Flip() call handles the layer change and
        // adjusts the orientation appropriately.
        footprint->SetOrientation( EDA_ANGLE( pads_part.rotation, DEGREES_T ) );

        footprint->SetLayer( F_Cu );

        // Look up custom attribute values from part type and per-instance overrides.
        // Per-instance attributes (from PART <refdes> {...} in *PARTTYPE*) take priority.
        const PADS_IO::PART_TYPE* partType = nullptr;
        auto ptIt = part_types.find( pads_part.decal );

        if( ptIt != part_types.end() )
            partType = &ptIt->second;

        const std::map<std::string, std::string>* instanceAttrs = nullptr;
        auto iaIt = partInstanceAttrs.find( pads_part.name );

        if( iaIt != partInstanceAttrs.end() )
            instanceAttrs = &iaIt->second;

        auto applyAttributes = [&]( const std::vector<PADS_IO::ATTRIBUTE>& attrs,
                                    std::function<int(double)> scaler )
        {
            for( const auto& attr : attrs )
            {
                PCB_FIELD* field = nullptr;
                bool ownsField = false;

                if( attr.name == "Ref.Des." )
                {
                    field = &footprint->Reference();
                }
                else if( attr.name == "Part Type" || attr.name == "VALUE" )
                {
                    field = &footprint->Value();
                }
                else
                {
                    std::string attrValue;

                    if( instanceAttrs )
                    {
                        auto valIt = instanceAttrs->find( attr.name );

                        if( valIt != instanceAttrs->end() )
                            attrValue = valIt->second;
                    }

                    if( attrValue.empty() && partType )
                    {
                        auto valIt = partType->attributes.find( attr.name );

                        if( valIt != partType->attributes.end() )
                            attrValue = valIt->second;
                    }

                    if( !attrValue.empty() )
                    {
                        field = new PCB_FIELD( footprint, FIELD_T::USER,
                                               wxString::FromUTF8( attr.name ) );
                        field->SetText( wxString::FromUTF8( attrValue ) );

                        // Footprint text fields on copper layers are almost always documentation
                        // labels. Redirect to the corresponding silkscreen layer.
                        PCB_LAYER_ID fieldLayer = getMappedLayer( attr.level );

                        if( IsCopperLayer( fieldLayer ) )
                            fieldLayer = IsBackLayer( fieldLayer ) ? B_SilkS : F_SilkS;

                        field->SetLayer( fieldLayer );
                        ownsField = true;
                    }
                }

                if( !field )
                    continue;

                int scaledSize = scaler( attr.height );
                int charHeight =
                        static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextHeightScale );
                int charWidth =
                        static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextWidthScale );
                field->SetTextSize( VECTOR2I( charWidth, charHeight ) );

                if( attr.width > 0 )
                    field->SetTextThickness( scaler( attr.width ) );

                // Position is relative to part origin, rotated by part orientation.
                // Y is negated for coordinate system conversion.
                VECTOR2I offset( scaler( attr.x ), -scaler( attr.y ) );
                EDA_ANGLE part_orient( pads_part.rotation, DEGREES_T );
                RotatePoint( offset, part_orient );

                // PADS text anchor differs from KiCad by a small offset along the
                // reading direction. Shift left (toward text start) to compensate.
                EDA_ANGLE textAngle = EDA_ANGLE( attr.orientation, DEGREES_T ) + part_orient;
                VECTOR2I textShift( -ADVANCED_CFG::GetCfg().m_PadsTextAnchorOffsetNm, 0 );
                RotatePoint( textShift, textAngle );
                offset += textShift;

                field->SetPosition( footprint->GetPosition() + offset );
                field->SetTextAngle( textAngle );
                field->SetKeepUpright( false );
                field->SetVisible( attr.visible );

                if( attr.hjust == "LEFT" )
                    field->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                else if( attr.hjust == "RIGHT" )
                    field->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
                else
                    field->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );

                if( attr.vjust == "UP" )
                    field->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
                else if( attr.vjust == "DOWN" )
                    field->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
                else
                    field->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

                if( ownsField )
                    footprint->Add( field );
            }
        };

        auto decal_it = decals.find( decal_name );

        if( decal_it != decals.end() )
        {
            auto decalScaler = [&]( double val ) {
                if( m_parser->IsBasicUnits() )
                    return scaleSize( val );

                const std::string& units = decal_it->second.units;

                if( units == "I" || units == "MILS" || units == "MIL" )
                    return KiROUND( val * PADS_UNIT_CONVERTER::MILS_TO_NM );
                else if( units == "M" || units == "METRIC" || units == "MM" )
                    return KiROUND( val * PADS_UNIT_CONVERTER::MM_TO_NM );
                else if( units == "INCHES" || units == "INCH" )
                    return KiROUND( val * PADS_UNIT_CONVERTER::INCHES_TO_NM );
                else
                    return scaleSize( val );
            };

            applyAttributes( decal_it->second.attributes, decalScaler );
        }
        else
        {
             if( m_reporter )
             {
                 m_reporter->Report(
                         wxString::Format( _( "Footprint '%s' not found in decal list, part skipped" ),
                                           decal_name ),
                         RPT_SEVERITY_WARNING );
             }
        }

        auto partScaler = [&]( double val ) {
            if( !m_parser->IsBasicUnits() )
            {
                if( pads_part.units == "M" ) return KiROUND( val * PADS_UNIT_CONVERTER::MILS_TO_NM );
            }

            if( pads_part.units == "M" ) return KiROUND( val );

            return scaleSize( val );
        };

        applyAttributes( pads_part.attributes, partScaler );

        // PADS "Part Type" maps to KiCad Value field. Hide it since it typically
        // shows the part type name which is not useful on fabrication layers.
        footprint->Value().SetVisible( false );

        m_loadBoard->Add( footprint );

        auto blockIt = m_partToBlockMap.find( pads_part.name );

        if( blockIt != m_partToBlockMap.end() )
        {
            PCB_FIELD* blockField =
                    new PCB_FIELD( footprint, FIELD_T::USER, wxT( "PADS_Reuse_Block" ) );
            blockField->SetLayer( Cmts_User );
            blockField->SetVisible( false );
            blockField->SetText( wxString::FromUTF8( blockIt->second ) );
            footprint->Add( blockField );
        }

        if( decal_it == decals.end() )
        {
            continue;
        }

        // Add Pads and Graphics from Decal
        {
            const PADS_IO::PART_DECAL& decal = decal_it->second;

            auto decalScaler = [&]( double val ) {
                if( m_parser->IsBasicUnits() )
                    return scaleSize( val );

                if( decal.units == "I" || decal.units == "MILS" || decal.units == "MIL" )
                    return KiROUND( val * PADS_UNIT_CONVERTER::MILS_TO_NM );
                else if( decal.units == "M" || decal.units == "METRIC" || decal.units == "MM" )
                    return KiROUND( val * PADS_UNIT_CONVERTER::MM_TO_NM );
                else if( decal.units == "INCHES" || decal.units == "INCH" )
                    return KiROUND( val * PADS_UNIT_CONVERTER::INCHES_TO_NM );
                else
                    return scaleSize( val );
            };

            auto convertPadShape = [&]( const PADS_IO::PAD_STACK_LAYER& layer_def,
                                        PAD* pad, PCB_LAYER_ID kicad_layer,
                                        const EDA_ANGLE& part_orient ) {
                const std::string& shape = layer_def.shape;
                // In PADS, sizeA is height (Y) and sizeB is width (X), opposite of KiCad convention
                VECTOR2I size( decalScaler( layer_def.sizeB ), decalScaler( layer_def.sizeA ) );

                if( shape == "R" || shape == "C" || shape == "A" || shape == "RT" )
                {
                    pad->SetShape( kicad_layer, PAD_SHAPE::CIRCLE );
                    pad->SetSize( kicad_layer, VECTOR2I( size.x, size.x ) );
                }
                else if( shape == "S" || shape == "ST" )
                {
                    pad->SetShape( kicad_layer, PAD_SHAPE::RECTANGLE );
                    pad->SetSize( kicad_layer, VECTOR2I( size.x, size.x ) );
                }
                else if( shape == "O" || shape == "OT" )
                {
                    pad->SetShape( kicad_layer, PAD_SHAPE::OVAL );
                    pad->SetSize( kicad_layer, size );
                }
                else if( shape == "RF" )
                {
                    pad->SetShape( kicad_layer, PAD_SHAPE::RECTANGLE );
                    pad->SetSize( kicad_layer, size );

                    if( layer_def.finger_offset != 0 )
                    {
                        int offset = decalScaler( layer_def.finger_offset );
                        VECTOR2I pad_offset( offset, 0 );
                        RotatePoint( pad_offset, EDA_ANGLE( layer_def.rotation, DEGREES_T ) );
                        pad->SetOffset( kicad_layer, pad_offset );
                    }
                }
                else if( shape == "OF" )
                {
                    pad->SetShape( kicad_layer, PAD_SHAPE::OVAL );
                    pad->SetSize( kicad_layer, size );

                    if( layer_def.finger_offset != 0 )
                    {
                        int offset = decalScaler( layer_def.finger_offset );
                        VECTOR2I pad_offset( offset, 0 );
                        RotatePoint( pad_offset, EDA_ANGLE( layer_def.rotation, DEGREES_T ) );
                        pad->SetOffset( kicad_layer, pad_offset );
                    }
                }
                else if( shape == "RC" || shape == "OC" )
                {
                    pad->SetShape( kicad_layer, PAD_SHAPE::ROUNDRECT );
                    pad->SetSize( kicad_layer, size );

                    if( layer_def.corner_radius > 0 && size.x > 0 )
                    {
                        double min_dim = std::min( size.x, size.y );
                        double radius = decalScaler( layer_def.corner_radius );
                        double ratio = ( min_dim > 0 ) ? ( radius / min_dim ) : 0.25;
                        ratio = std::min( ratio, 0.5 );
                        pad->SetRoundRectRadiusRatio( kicad_layer, ratio );
                    }
                    else
                    {
                        pad->SetRoundRectRadiusRatio( kicad_layer, 0.25 );
                    }

                    if( layer_def.finger_offset != 0 )
                    {
                        int offset = decalScaler( layer_def.finger_offset );
                        VECTOR2I pad_offset( offset, 0 );
                        RotatePoint( pad_offset, EDA_ANGLE( layer_def.rotation, DEGREES_T ) );
                        pad->SetOffset( kicad_layer, pad_offset );
                    }
                }
                else
                {
                    pad->SetShape( kicad_layer, PAD_SHAPE::CIRCLE );
                    pad->SetSize( kicad_layer, VECTOR2I( size.x, size.x ) );
                }

                pad->SetOrientation( part_orient + EDA_ANGLE( layer_def.rotation, DEGREES_T ) );
            };

            EDA_ANGLE part_orient( pads_part.rotation, DEGREES_T );

            for( size_t term_idx = 0; term_idx < decal.terminals.size(); ++term_idx )
            {
                const auto& term = decal.terminals[term_idx];
                PAD* pad = new PAD( footprint );
                footprint->Add( pad );

                pad->SetNumber( term.name );

                VECTOR2I pad_pos( decalScaler( term.x ), -decalScaler( term.y ) );
                RotatePoint( pad_pos, part_orient );
                pad->SetPosition( footprint->GetPosition() + pad_pos );

                // Look up pad stack by terminal index (1-based). PAD 0 is the default for
                // terminals without explicit definitions. PAD N is for terminal index N.
                int pin_num = static_cast<int>( term_idx + 1 );

                auto stack_it = decal.pad_stacks.find( pin_num );

                if( stack_it == decal.pad_stacks.end() )
                    stack_it = decal.pad_stacks.find( 0 );

                if( stack_it != decal.pad_stacks.end() && !stack_it->second.empty() )
                {
                    const std::vector<PADS_IO::PAD_STACK_LAYER>& stack = stack_it->second;

                    double drill = 0.0;
                    bool plated = true;

                    for( const auto& layer_def : stack )
                    {
                        if( layer_def.drill > 0 )
                        {
                            drill = layer_def.drill;
                            plated = layer_def.plated;
                            break;
                        }
                    }

                    LSET layer_set;

                    auto mapPadsLayer = [&]( int pads_layer ) -> PCB_LAYER_ID {
                        if( pads_layer == -2 || pads_layer == 1 )
                            return F_Cu;
                        else if( pads_layer == -1
                                 || pads_layer == m_parser->GetParameters().layer_count )
                            return B_Cu;
                        else if( pads_layer > 1
                                 && pads_layer < m_parser->GetParameters().layer_count )
                        {
                            int inner_idx = pads_layer - 2;

                            if( inner_idx >= 0 && inner_idx < 30 )
                                return static_cast<PCB_LAYER_ID>( In1_Cu + inner_idx * 2 );
                        }

                        return UNDEFINED_LAYER;
                    };

                    bool has_explicit_layers = false;

                    for( const auto& layer_def : stack )
                    {
                        if( layer_def.layer == -2 || layer_def.layer == -1
                            || layer_def.layer == 1
                            || layer_def.layer == m_parser->GetParameters().layer_count )
                        {
                            has_explicit_layers = true;
                            break;
                        }
                    }

                    for( const auto& layer_def : stack )
                    {
                        if( layer_def.layer == 0 )
                        {
                            if( !has_explicit_layers )
                            {
                                layer_set = ( drill > 0 ) ? LSET::AllCuMask()
                                                          : LSET( { F_Cu, B_Cu } );
                                convertPadShape( layer_def, pad, F_Cu, part_orient );

                                if( drill == 0 )
                                {
                                    pad->SetShape( B_Cu, pad->GetShape( F_Cu ) );
                                    pad->SetSize( B_Cu, pad->GetSize( F_Cu ) );
                                }
                            }

                            continue;
                        }

                        // Skip layers with size 0 - "no pad on this layer" in PADS.
                        // We must not call SetSize with 0 since in PADSTACK NORMAL mode all
                        // layers write to the same ALL_LAYERS slot, overwriting valid sizes.
                        if( layer_def.sizeA <= 0 )
                            continue;

                        PCB_LAYER_ID kicad_layer = mapPadsLayer( layer_def.layer );

                        if( kicad_layer != UNDEFINED_LAYER )
                        {
                            layer_set.set( kicad_layer );
                            convertPadShape( layer_def, pad, kicad_layer, part_orient );
                        }
                    }

                    if( layer_set.none() )
                    {
                        layer_set.set( F_Cu );
                        convertPadShape( stack[0], pad, F_Cu, part_orient );
                    }

                    pad->SetDrillSize( VECTOR2I( decalScaler( drill ), decalScaler( drill ) ) );

                    if( drill == 0 )
                    {
                        pad->SetAttribute( PAD_ATTRIB::SMD );
                    }
                    else
                    {
                        if( plated )
                            pad->SetAttribute( PAD_ATTRIB::PTH );
                        else
                            pad->SetAttribute( PAD_ATTRIB::NPTH );

                        layer_set = LSET::AllCuMask();
                    }

                    pad->SetLayerSet( layer_set );
                }
                else
                {
                    pad->SetSize( F_Cu, VECTOR2I( decalScaler( 1.5 ), decalScaler( 1.5 ) ) );
                    pad->SetShape( F_Cu, PAD_SHAPE::CIRCLE );
                    pad->SetAttribute( PAD_ATTRIB::PTH );
                    pad->SetLayerSet( LSET::AllCuMask() );
                }

                std::string pinKey = pads_part.name + "." + term.name;
                auto netIt = m_pinToNetMap.find( pinKey );

                if( netIt != m_pinToNetMap.end() )
                {
                    NETINFO_ITEM* net =
                            m_loadBoard->FindNet( PADS_COMMON::ConvertInvertedNetName( netIt->second ) );

                    if( net )
                        pad->SetNet( net );
                }
            }

            for( const auto& item : decal.items )
            {
                if( item.points.empty() )
                    continue;

                // Decal graphics layers work differently from routing layers in PADS.
                // Layer 0 and layer 1 are typically footprint outlines, not copper.
                PCB_LAYER_ID shape_layer = F_SilkS;

                if( item.layer == 0 )
                {
                    shape_layer = F_SilkS;
                }
                else
                {
                    PCB_LAYER_ID mapped_layer = getMappedLayer( item.layer );

                    if( IsCopperLayer( mapped_layer ) )
                    {
                        if( mapped_layer == B_Cu )
                            shape_layer = B_SilkS;
                        else
                            shape_layer = F_SilkS;
                    }
                    else
                    {
                        shape_layer = mapped_layer;
                    }
                }

                if( shape_layer == UNDEFINED_LAYER )
                {
                    if( m_reporter )
                    {
                        m_reporter->Report( wxString::Format(
                                _( "Skipping decal item on unmapped layer %d" ), item.layer ),
                                RPT_SEVERITY_WARNING );
                    }
                    continue;
                }

                bool is_circle = ( item.type == "CIRCLE" );
                bool is_closed = ( item.type == "CLOSED" || is_circle );

                // Per PADS spec: CIRCLE pieces have 2 corners representing ends of
                // horizontal diameter.
                if( is_circle && item.points.size() >= 2 )
                {
                    PCB_SHAPE* shape = new PCB_SHAPE( footprint, SHAPE_T::CIRCLE );
                    shape->SetLayer( shape_layer );

                    double x1 = item.points[0].x;
                    double y1 = item.points[0].y;
                    double x2 = item.points[1].x;
                    double y2 = item.points[1].y;

                    double cx = ( x1 + x2 ) / 2.0;
                    double cy = ( y1 + y2 ) / 2.0;

                    double radius = std::sqrt( ( x2 - x1 ) * ( x2 - x1 )
                                               + ( y2 - y1 ) * ( y2 - y1 ) )
                                    / 2.0;

                    VECTOR2I center( decalScaler( cx ), -decalScaler( cy ) );
                    VECTOR2I pt_on_circle( decalScaler( cx + radius ), -decalScaler( cy ) );

                    RotatePoint( center, part_orient );
                    RotatePoint( pt_on_circle, part_orient );

                    VECTOR2I fp_pos = footprint->GetPosition();
                    shape->SetCenter( fp_pos + center );
                    shape->SetEnd( fp_pos + pt_on_circle );
                    shape->SetStroke(
                            STROKE_PARAMS( decalScaler( item.width ), LINE_STYLE::SOLID ) );

                    footprint->Add( shape );

                    continue;
                }

                if( item.points.size() < 2 )
                    continue;

                for( size_t i = 0; i < item.points.size() - 1; ++i )
                {
                    const PADS_IO::ARC_POINT& p1 = item.points[i];
                    const PADS_IO::ARC_POINT& p2 = item.points[i + 1];

                    PCB_SHAPE* shape = new PCB_SHAPE( footprint );
                    shape->SetLayer( shape_layer );
                    shape->SetStroke(
                            STROKE_PARAMS( decalScaler( item.width ), LINE_STYLE::SOLID ) );

                    if( p2.is_arc )
                    {
                        shape->SetShape( SHAPE_T::ARC );
                        VECTOR2I center( decalScaler( p2.arc.cx ), -decalScaler( p2.arc.cy ) );
                        VECTOR2I start( decalScaler( p1.x ), -decalScaler( p1.y ) );
                        VECTOR2I end( decalScaler( p2.x ), -decalScaler( p2.y ) );

                        // Y-axis flip reverses arc winding; swap endpoints for CCW arcs
                        if( p2.arc.delta_angle > 0 )
                            std::swap( start, end );

                        RotatePoint( center, part_orient );
                        RotatePoint( start, part_orient );
                        RotatePoint( end, part_orient );

                        VECTOR2I fp_pos = footprint->GetPosition();
                        shape->SetCenter( fp_pos + center );
                        shape->SetStart( fp_pos + start );
                        shape->SetEnd( fp_pos + end );
                    }
                    else
                    {
                        shape->SetShape( SHAPE_T::SEGMENT );
                        VECTOR2I start( decalScaler( p1.x ), -decalScaler( p1.y ) );
                        VECTOR2I end( decalScaler( p2.x ), -decalScaler( p2.y ) );

                        RotatePoint( start, part_orient );
                        RotatePoint( end, part_orient );

                        VECTOR2I fp_pos = footprint->GetPosition();
                        shape->SetStart( fp_pos + start );
                        shape->SetEnd( fp_pos + end );
                    }

                    footprint->Add( shape );
                }

                if( is_closed && item.points.size() > 2 )
                {
                    const PADS_IO::ARC_POINT& pLast = item.points.back();
                    const PADS_IO::ARC_POINT& pFirst = item.points.front();

                    PCB_SHAPE* shape = new PCB_SHAPE( footprint );
                    shape->SetLayer( shape_layer );
                    shape->SetStroke(
                            STROKE_PARAMS( decalScaler( item.width ), LINE_STYLE::SOLID ) );

                    if( pFirst.is_arc )
                    {
                        shape->SetShape( SHAPE_T::ARC );
                        VECTOR2I center( decalScaler( pFirst.arc.cx ),
                                         -decalScaler( pFirst.arc.cy ) );
                        VECTOR2I start( decalScaler( pLast.x ), -decalScaler( pLast.y ) );
                        VECTOR2I end( decalScaler( pFirst.x ), -decalScaler( pFirst.y ) );

                        if( pFirst.arc.delta_angle > 0 )
                            std::swap( start, end );

                        RotatePoint( center, part_orient );
                        RotatePoint( start, part_orient );
                        RotatePoint( end, part_orient );

                        VECTOR2I fp_pos = footprint->GetPosition();
                        shape->SetCenter( fp_pos + center );
                        shape->SetStart( fp_pos + start );
                        shape->SetEnd( fp_pos + end );
                    }
                    else
                    {
                        shape->SetShape( SHAPE_T::SEGMENT );
                        VECTOR2I start( decalScaler( pLast.x ), -decalScaler( pLast.y ) );
                        VECTOR2I end( decalScaler( pFirst.x ), -decalScaler( pFirst.y ) );

                        RotatePoint( start, part_orient );
                        RotatePoint( end, part_orient );

                        VECTOR2I fp_pos = footprint->GetPosition();
                        shape->SetStart( fp_pos + start );
                        shape->SetEnd( fp_pos + end );
                    }

                    footprint->Add( shape );
                }
            }
        }

        if( pads_part.bottom_layer )
        {
            footprint->Flip( footprint->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );
        }
    }
}


void PCB_IO_PADS::loadReuseBlockGroups()
{
    const auto& reuse_blocks = m_parser->GetReuseBlocks();

    if( reuse_blocks.empty() )
        return;

    std::map<std::string, PCB_GROUP*> blockGroups;

    for( const auto& [blockName, block] : reuse_blocks )
    {
        if( !block.instances.empty() || !block.part_names.empty() )
        {
            PCB_GROUP* group = new PCB_GROUP( m_loadBoard );
            group->SetName( wxString::FromUTF8( blockName ) );
            m_loadBoard->Add( group );
            blockGroups[blockName] = group;
        }
    }

    for( FOOTPRINT* fp : m_loadBoard->Footprints() )
    {
        for( PCB_FIELD* field : fp->GetFields() )
        {
            if( field->GetName() == wxT( "PADS_Reuse_Block" ) )
            {
                std::string blockName = field->GetText().ToStdString();
                auto groupIt = blockGroups.find( blockName );

                if( groupIt != blockGroups.end() )
                {
                    groupIt->second->AddItem( fp );
                }

                break;
            }
        }
    }
}


void PCB_IO_PADS::loadTestPoints()
{
    const auto& test_points = m_parser->GetTestPoints();

    for( const auto& tp : test_points )
    {
        FOOTPRINT* footprint = new FOOTPRINT( m_loadBoard );

        wxString refDes = wxString::Format( wxT( "TP%d" ), m_testPointIndex++ );
        footprint->SetReference( refDes );
        footprint->SetValue( wxString::FromUTF8( tp.symbol_name ) );

        VECTOR2I pos( scaleCoord( tp.x, true ), scaleCoord( tp.y, false ) );
        footprint->SetPosition( pos );

        PCB_LAYER_ID layer = F_Cu;

        if( tp.side == 2 )
            layer = B_Cu;

        footprint->SetLayer( ( layer == B_Cu ) ? B_Cu : F_Cu );

        PAD* pad = new PAD( footprint );
        pad->SetNumber( wxT( "1" ) );
        pad->SetPosition( pos );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( scaleSize( 50.0 ), scaleSize( 50.0 ) ) );
        pad->SetAttribute( PAD_ATTRIB::SMD );
        pad->SetLayerSet( layer == B_Cu ? LSET( { B_Cu } ) : LSET( { F_Cu } ) );

        if( !tp.net_name.empty() )
        {
            NETINFO_ITEM* net = m_loadBoard->FindNet( PADS_COMMON::ConvertInvertedNetName( tp.net_name ) );

            if( net )
                pad->SetNet( net );
        }

        footprint->Add( pad );

        footprint->SetBoardOnly( true );

        PCB_FIELD* tpField = new PCB_FIELD( footprint, FIELD_T::USER, wxT( "Test_Point" ) );
        tpField->SetLayer( Cmts_User );
        tpField->SetVisible( false );
        tpField->SetText( wxString::FromUTF8( tp.type ) );
        footprint->Add( tpField );

        m_loadBoard->Add( footprint );
    }
}


void PCB_IO_PADS::loadTexts()
{
    const auto& texts = m_parser->GetTexts();

    for( const auto& pads_text : texts )
    {
        PCB_LAYER_ID textLayer = getMappedLayer( pads_text.layer );

        if( textLayer == UNDEFINED_LAYER )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format(
                        _( "Text on unmapped layer %d assigned to Comments layer" ),
                        pads_text.layer ), RPT_SEVERITY_WARNING );
            }
            textLayer = Cmts_User;
        }

        PCB_TEXT* text = new PCB_TEXT( m_loadBoard );
        text->SetText( pads_text.content );

        // PADS text cell height includes internal leading and descender space.
        // Scale factors calibrated to match PADS rendered character dimensions.
        int scaledSize = scaleSize( pads_text.height );
        int charHeight =
                static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextHeightScale );
        int charWidth =
                static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextWidthScale );
        text->SetTextSize( VECTOR2I( charWidth, charHeight ) );

        if( pads_text.width > 0 )
            text->SetTextThickness( scaleSize( pads_text.width ) );

        EDA_ANGLE textAngle( pads_text.rotation, DEGREES_T );
        text->SetTextAngle( textAngle );

        // PADS text anchor differs from KiCad by a small offset along the
        // reading direction. Shift left (toward text start) to compensate.
        VECTOR2I pos( scaleCoord( pads_text.location.x, true ),
                      scaleCoord( pads_text.location.y, false ) );
        VECTOR2I textShift( -ADVANCED_CFG::GetCfg().m_PadsTextAnchorOffsetNm, 0 );
        RotatePoint( textShift, textAngle );
        text->SetPosition( pos + textShift );

        if( pads_text.hjust == "LEFT" )
            text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        else if( pads_text.hjust == "RIGHT" )
            text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else
            text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );

        if( pads_text.vjust == "UP" )
            text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        else if( pads_text.vjust == "DOWN" )
            text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        else
            text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

        text->SetKeepUpright( false );
        text->SetLayer( textLayer );
        m_loadBoard->Add( text );
    }
}


void PCB_IO_PADS::loadTracksAndVias()
{
    const auto& routes = m_parser->GetRoutes();
    std::set<std::pair<int, int>> placedThroughVias;

    for( const auto& route : routes )
    {
        NETINFO_ITEM* net = m_loadBoard->FindNet( PADS_COMMON::ConvertInvertedNetName( route.net_name ) );

        if( !net )
            continue;

        for( const auto& track_def : route.tracks )
        {
            if( track_def.points.size() < 2 )
                continue;

            PCB_LAYER_ID track_layer = getMappedLayer( track_def.layer );

            if( !IsCopperLayer( track_layer ) )
            {
                if( m_reporter )
                {
                    m_reporter->Report( wxString::Format(
                            _( "Skipping track on non-copper layer %d" ), track_def.layer ),
                            RPT_SEVERITY_WARNING );
                }
                continue;
            }

            int track_width = scaleSize( track_def.width );

            for( size_t i = 0; i < track_def.points.size() - 1; ++i )
            {
                const PADS_IO::ARC_POINT& p1 = track_def.points[i];
                const PADS_IO::ARC_POINT& p2 = track_def.points[i + 1];

                VECTOR2I start( scaleCoord( p1.x, true ), scaleCoord( p1.y, false ) );
                VECTOR2I end( scaleCoord( p2.x, true ), scaleCoord( p2.y, false ) );

                // Skip near-zero-length segments (can occur at via points with width changes).
                // Tolerance of 1000nm accounts for floating point precision in coordinate
                // transformation.
                if( ( start - end ).EuclideanNorm() < 1000 )
                    continue;

                if( p2.is_arc )
                {
                    VECTOR2I center( scaleCoord( p2.arc.cx, true ),
                                     scaleCoord( p2.arc.cy, false ) );

                    bool clockwise = ( p2.arc.delta_angle < 0 );

                    SHAPE_ARC shapeArc;
                    shapeArc.ConstructFromStartEndCenter( start, end, center, clockwise,
                                                         track_width );

                    PCB_ARC* arc = new PCB_ARC( m_loadBoard, &shapeArc );
                    arc->SetNet( net );
                    arc->SetWidth( track_width );
                    arc->SetLayer( track_layer );
                    m_loadBoard->Add( arc );
                }
                else
                {
                    PCB_TRACK* track = new PCB_TRACK( m_loadBoard );
                    track->SetNet( net );
                    track->SetWidth( track_width );
                    track->SetLayer( track_layer );
                    track->SetStart( start );
                    track->SetEnd( end );
                    m_loadBoard->Add( track );
                }
            }
        }

        for( const auto& via_def : route.vias )
        {
            VECTOR2I pos( scaleCoord( via_def.location.x, true ),
                          scaleCoord( via_def.location.y, false ) );

            VIATYPE viaType = VIATYPE::THROUGH;
            auto it = m_parser->GetViaDefs().find( via_def.name );

            if( it != m_parser->GetViaDefs().end() )
            {
                switch( it->second.via_type )
                {
                case PADS_IO::VIA_TYPE::THROUGH:  viaType = VIATYPE::THROUGH;  break;
                case PADS_IO::VIA_TYPE::BLIND:    viaType = VIATYPE::BLIND;    break;
                case PADS_IO::VIA_TYPE::BURIED:   viaType = VIATYPE::BURIED;   break;
                case PADS_IO::VIA_TYPE::MICROVIA: viaType = VIATYPE::MICROVIA; break;
                }
            }

            // Through-hole vias shared across multiple SIGNAL blocks for the same net
            // produce duplicates. Skip if we already placed one at this position.
            if( viaType == VIATYPE::THROUGH )
            {
                auto key = std::make_pair( pos.x, pos.y );

                if( placedThroughVias.count( key ) )
                    continue;

                placedThroughVias.insert( key );
            }

            PCB_VIA* via = new PCB_VIA( m_loadBoard );
            via->SetNet( net );
            via->SetPosition( pos );

            if( it != m_parser->GetViaDefs().end() )
            {
                const PADS_IO::VIA_DEF& def = it->second;

                via->SetWidth( scaleSize( def.size ) );
                via->SetDrill( scaleSize( def.drill ) );

                if( def.start_layer > 0 && def.end_layer > 0 )
                {
                    via->SetLayerPair( getMappedLayer( def.start_layer ),
                                       getMappedLayer( def.end_layer ) );
                    via->SetViaType( viaType );
                }
                else
                {
                    via->SetLayerPair( F_Cu, B_Cu );
                    via->SetViaType( VIATYPE::THROUGH );
                }
            }
            else
            {
                via->SetWidth( scaleSize( 20.0 ) );
                via->SetDrill( scaleSize( 10.0 ) );
                via->SetLayerPair( F_Cu, B_Cu );
                via->SetViaType( VIATYPE::THROUGH );
            }

            m_loadBoard->Add( via );
        }
    }
}


void PCB_IO_PADS::loadClusterGroups()
{
    const auto& clusters = m_parser->GetClusters();

    if( clusters.empty() )
        return;

    std::map<std::string, const PADS_IO::CLUSTER*> netToClusterMap;

    for( const auto& cluster : clusters )
    {
        for( const std::string& netName : cluster.net_names )
        {
            std::string converted = PADS_COMMON::ConvertInvertedNetName( netName ).ToStdString();
            netToClusterMap[converted] = &cluster;
        }
    }

    std::map<int, PCB_GROUP*> clusterGroups;

    for( const auto& cluster : clusters )
    {
        PCB_GROUP* group = new PCB_GROUP( m_loadBoard );
        group->SetName( wxString::FromUTF8( cluster.name ) );
        m_loadBoard->Add( group );
        clusterGroups[cluster.id] = group;
    }

    for( PCB_TRACK* track : m_loadBoard->Tracks() )
    {
        NETINFO_ITEM* net = track->GetNet();

        if( net )
        {
            std::string netName = net->GetNetname().ToStdString();
            auto clusterIt = netToClusterMap.find( netName );

            if( clusterIt != netToClusterMap.end() )
            {
                int clusterId = clusterIt->second->id;
                auto groupIt = clusterGroups.find( clusterId );

                if( groupIt != clusterGroups.end() )
                {
                    groupIt->second->AddItem( track );
                }
            }
        }
    }
}


void PCB_IO_PADS::loadZones()
{
    const auto& pours = m_parser->GetPours();
    const auto& params = m_parser->GetParameters();

    // PADS uses lower numbers = higher priority (priority 1 fills on top),
    // while KiCad uses higher numbers = higher priority.
    int maxPriority = 0;

    for( const auto& pour_def : pours )
    {
        if( pour_def.priority > maxPriority )
            maxPriority = pour_def.priority;
    }

    for( const auto& pour_def : pours )
    {
        PCB_LAYER_ID pourLayer = getMappedLayer( pour_def.layer );

        if( pourLayer == UNDEFINED_LAYER )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format(
                        _( "Skipping pour on unmapped layer %d" ), pour_def.layer ),
                        RPT_SEVERITY_WARNING );
            }
            continue;
        }

        ZONE* zone = new ZONE( m_loadBoard );
        zone->SetLayer( pourLayer );

        zone->Outline()->NewOutline();

        for( const auto& pt : pour_def.points )
        {
            zone->Outline()->Append( scaleCoord( pt.x, true ), scaleCoord( pt.y, false ) );
        }

        if( pour_def.is_cutout )
        {
            zone->SetIsRuleArea( true );
            zone->SetDoNotAllowZoneFills( true );
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowPads( false );
            zone->SetDoNotAllowFootprints( false );
            zone->SetZoneName( wxString::Format( wxT( "Cutout_%s" ), pour_def.owner_pour ) );
        }
        else
        {
            NETINFO_ITEM* net = m_loadBoard->FindNet( PADS_COMMON::ConvertInvertedNetName( pour_def.net_name ) );

            if( net )
            {
                zone->SetNet( net );
            }

            int kicadPriority = maxPriority - pour_def.priority + 1;
            zone->SetAssignedPriority( kicadPriority );
            zone->SetMinThickness( scaleSize( pour_def.width ) );

            zone->SetThermalReliefGap( scaleSize( params.thermal_min_clearance ) );
            zone->SetThermalReliefSpokeWidth( scaleSize( params.thermal_line_width ) );

            zone->SetPadConnection( ZONE_CONNECTION::THERMAL );
        }

        m_loadBoard->Add( zone );
    }
}


void PCB_IO_PADS::loadBoardOutline()
{
    for( const PADS_IO::POLYLINE& polyline : m_parser->GetBoardOutlines() )
    {
        const auto& pts = polyline.points;

        if( pts.size() < 2 )
            continue;

        for( size_t i = 0; i < pts.size() - 1; ++i )
        {
            const PADS_IO::ARC_POINT& p1 = pts[i];
            const PADS_IO::ARC_POINT& p2 = pts[i + 1];

            if( std::abs( p1.x - p2.x ) < 0.001 && std::abs( p1.y - p2.y ) < 0.001 )
                continue;

            PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard );

            if( p2.is_arc )
            {
                shape->SetShape( SHAPE_T::ARC );
                VECTOR2I center( scaleCoord( p2.arc.cx, true ), scaleCoord( p2.arc.cy, false ) );
                VECTOR2I start( scaleCoord( p1.x, true ), scaleCoord( p1.y, false ) );
                VECTOR2I end( scaleCoord( p2.x, true ), scaleCoord( p2.y, false ) );

                if( p2.arc.delta_angle > 0 )
                    std::swap( start, end );

                shape->SetCenter( center );
                shape->SetStart( start );
                shape->SetEnd( end );
            }
            else
            {
                shape->SetShape( SHAPE_T::SEGMENT );
                shape->SetStart( VECTOR2I( scaleCoord( p1.x, true ),
                                           scaleCoord( p1.y, false ) ) );
                shape->SetEnd( VECTOR2I( scaleCoord( p2.x, true ),
                                         scaleCoord( p2.y, false ) ) );
            }

            shape->SetWidth( scaleSize( polyline.width ) );
            shape->SetLayer( Edge_Cuts );
            m_loadBoard->Add( shape );
        }

        // PADS format repeats the first point at the end for closed polygons, so check
        // if pLast already equals pFirst to avoid creating a zero-length closing segment
        if( polyline.closed && pts.size() > 2 )
        {
            const PADS_IO::ARC_POINT& pLast = pts.back();
            const PADS_IO::ARC_POINT& pFirst = pts.front();

            bool needsClosing = ( std::abs( pLast.x - pFirst.x ) > 0.001
                                  || std::abs( pLast.y - pFirst.y ) > 0.001 );

            if( needsClosing )
            {
                PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard );

                if( pFirst.is_arc )
                {
                    shape->SetShape( SHAPE_T::ARC );
                    VECTOR2I center( scaleCoord( pFirst.arc.cx, true ),
                                     scaleCoord( pFirst.arc.cy, false ) );
                    VECTOR2I start( scaleCoord( pLast.x, true ),
                                    scaleCoord( pLast.y, false ) );
                    VECTOR2I end( scaleCoord( pFirst.x, true ),
                                  scaleCoord( pFirst.y, false ) );

                    if( pFirst.arc.delta_angle > 0 )
                        std::swap( start, end );

                    shape->SetCenter( center );
                    shape->SetStart( start );
                    shape->SetEnd( end );
                }
                else
                {
                    shape->SetShape( SHAPE_T::SEGMENT );
                    shape->SetStart( VECTOR2I( scaleCoord( pLast.x, true ),
                                               scaleCoord( pLast.y, false ) ) );
                    shape->SetEnd( VECTOR2I( scaleCoord( pFirst.x, true ),
                                             scaleCoord( pFirst.y, false ) ) );
                }

                shape->SetWidth( scaleSize( polyline.width ) );
                shape->SetLayer( Edge_Cuts );
                m_loadBoard->Add( shape );
            }
        }
    }
}


void PCB_IO_PADS::loadDimensions()
{
    const auto& dimensions = m_parser->GetDimensions();

    for( const auto& dim : dimensions )
    {
        if( dim.points.size() < 2 )
            continue;

        PCB_DIM_ALIGNED* dimension = new PCB_DIM_ALIGNED( m_loadBoard, PCB_DIM_ALIGNED_T );

        VECTOR2I start( scaleCoord( dim.points[0].x, true ),
                        scaleCoord( dim.points[0].y, false ) );
        VECTOR2I end( scaleCoord( dim.points[1].x, true ),
                      scaleCoord( dim.points[1].y, false ) );

        dimension->SetStart( start );
        dimension->SetEnd( end );

        // The crossbar_pos is the absolute coordinate of the crossbar. We compute
        // height as the offset from the start point to the crossbar.
        if( dim.is_horizontal )
        {
            double heightOffset = dim.crossbar_pos - dim.points[0].y;
            int height = -scaleSize( heightOffset );
            dimension->SetHeight( height );
        }
        else
        {
            double heightOffset = dim.crossbar_pos - dim.points[0].x;
            int height = scaleSize( heightOffset );
            dimension->SetHeight( height );
        }

        PCB_LAYER_ID dimLayer = getMappedLayer( dim.layer );

        if( dimLayer == UNDEFINED_LAYER || IsCopperLayer( dimLayer ) )
            dimLayer = Cmts_User;

        dimension->SetLayer( dimLayer );

        // PADS text_width is stroke thickness, not character width.
        // Calculate character dimensions from height.
        if( dim.text_height > 0 )
        {
            int scaledSize = scaleSize( dim.text_height );
            int charHeight =
                    static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextHeightScale );
            int charWidth =
                    static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextWidthScale );
            dimension->SetTextSize( VECTOR2I( charWidth, charHeight ) );

            if( dim.text_width > 0 )
                dimension->SetTextThickness( scaleSize( dim.text_width ) );
        }

        if( !dim.text.empty() )
        {
            dimension->SetOverrideTextEnabled( true );
            dimension->SetOverrideText( wxString::FromUTF8( dim.text ) );
        }

        dimension->SetLineThickness( scaleSize( 5.0 ) );

        if( dim.rotation != 0.0 )
            dimension->SetTextAngle( EDA_ANGLE( dim.rotation, DEGREES_T ) );

        dimension->Update();
        m_loadBoard->Add( dimension );
    }
}


void PCB_IO_PADS::loadKeepouts()
{
    const auto& keepouts = m_parser->GetKeepouts();
    int keepoutIndex = 0;

    for( const auto& ko : keepouts )
    {
        if( ko.outline.empty() )
            continue;

        ZONE* zone = new ZONE( m_loadBoard );
        zone->SetIsRuleArea( true );

        if( ko.layers.empty() )
        {
            zone->SetLayerSet( LSET::AllCuMask() );
        }
        else if( ko.layers.size() == 1 )
        {
            PCB_LAYER_ID koLayer = getMappedLayer( ko.layers[0] );

            if( koLayer == UNDEFINED_LAYER )
            {
                if( m_reporter )
                {
                    m_reporter->Report( wxString::Format(
                            _( "Skipping keepout on unmapped layer %d" ), ko.layers[0] ),
                            RPT_SEVERITY_WARNING );
                }
                delete zone;
                continue;
            }

            zone->SetLayer( koLayer );
        }
        else
        {
            LSET layerSet;

            for( int layer : ko.layers )
            {
                PCB_LAYER_ID mappedLayer = getMappedLayer( layer );

                if( mappedLayer != UNDEFINED_LAYER )
                    layerSet.set( mappedLayer );
            }

            if( layerSet.none() )
            {
                if( m_reporter )
                    m_reporter->Report( _( "Skipping keepout with no valid layers" ), RPT_SEVERITY_WARNING );
                delete zone;
                continue;
            }

            zone->SetLayerSet( layerSet );
        }

        zone->SetDoNotAllowTracks( ko.no_traces );
        zone->SetDoNotAllowVias( ko.no_vias );
        zone->SetDoNotAllowZoneFills( ko.no_copper );
        zone->SetDoNotAllowFootprints( ko.no_components );
        zone->SetDoNotAllowPads( false );

        wxString typeName;

        switch( ko.type )
        {
        case PADS_IO::KEEPOUT_TYPE::ALL:       typeName = wxT( "Keepout" ); break;
        case PADS_IO::KEEPOUT_TYPE::ROUTE:     typeName = wxT( "RouteKeepout" ); break;
        case PADS_IO::KEEPOUT_TYPE::VIA:       typeName = wxT( "ViaKeepout" ); break;
        case PADS_IO::KEEPOUT_TYPE::COPPER:    typeName = wxT( "CopperKeepout" ); break;
        case PADS_IO::KEEPOUT_TYPE::PLACEMENT: typeName = wxT( "PlacementKeepout" ); break;
        }

        zone->SetZoneName( wxString::Format( wxT( "%s_%d" ), typeName, ++keepoutIndex ) );

        zone->Outline()->NewOutline();

        for( const auto& pt : ko.outline )
        {
            zone->Outline()->Append( scaleCoord( pt.x, true ), scaleCoord( pt.y, false ) );
        }

        if( !ko.outline.empty() && ko.outline.size() > 2 )
        {
            const auto& first = ko.outline.front();
            const auto& last = ko.outline.back();

            if( std::abs( first.x - last.x ) > 0.001 || std::abs( first.y - last.y ) > 0.001 )
            {
                zone->Outline()->Append( scaleCoord( first.x, true ),
                                         scaleCoord( first.y, false ) );
            }
        }

        m_loadBoard->Add( zone );
    }
}


void PCB_IO_PADS::loadGraphicLines()
{
    for( const PADS_IO::GRAPHIC_LINE& graphic : m_parser->GetGraphicLines() )
    {
        const auto& pts = graphic.points;

        PCB_LAYER_ID graphicLayer = getMappedLayer( graphic.layer );

        if( graphicLayer == UNDEFINED_LAYER )
            continue;

        if( pts.size() == 1 && pts[0].is_arc
            && std::abs( pts[0].arc.delta_angle - 360.0 ) < 0.1 )
        {
            PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard );
            shape->SetShape( SHAPE_T::CIRCLE );
            VECTOR2I center( scaleCoord( pts[0].arc.cx, true ),
                             scaleCoord( pts[0].arc.cy, false ) );
            int radius = scaleSize( pts[0].arc.radius );
            shape->SetCenter( center );
            shape->SetEnd( VECTOR2I( center.x + radius, center.y ) );
            shape->SetWidth( scaleSize( graphic.width ) );
            shape->SetLayer( graphicLayer );
            m_loadBoard->Add( shape );
            continue;
        }

        if( pts.size() < 2 )
            continue;

        for( size_t i = 0; i < pts.size() - 1; ++i )
        {
            const PADS_IO::ARC_POINT& p1 = pts[i];
            const PADS_IO::ARC_POINT& p2 = pts[i + 1];

            if( std::abs( p1.x - p2.x ) < 0.001 && std::abs( p1.y - p2.y ) < 0.001 )
                continue;

            PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard );

            if( p2.is_arc )
            {
                shape->SetShape( SHAPE_T::ARC );
                VECTOR2I center( scaleCoord( p2.arc.cx, true ), scaleCoord( p2.arc.cy, false ) );
                VECTOR2I start( scaleCoord( p1.x, true ), scaleCoord( p1.y, false ) );
                VECTOR2I end( scaleCoord( p2.x, true ), scaleCoord( p2.y, false ) );

                if( p2.arc.delta_angle > 0 )
                    std::swap( start, end );

                shape->SetCenter( center );
                shape->SetStart( start );
                shape->SetEnd( end );
            }
            else
            {
                shape->SetShape( SHAPE_T::SEGMENT );
                shape->SetStart( VECTOR2I( scaleCoord( p1.x, true ),
                                           scaleCoord( p1.y, false ) ) );
                shape->SetEnd( VECTOR2I( scaleCoord( p2.x, true ),
                                         scaleCoord( p2.y, false ) ) );
            }

            shape->SetWidth( scaleSize( graphic.width ) );
            shape->SetLayer( graphicLayer );
            m_loadBoard->Add( shape );
        }

        if( graphic.closed && pts.size() > 2 )
        {
            const PADS_IO::ARC_POINT& pLast = pts.back();
            const PADS_IO::ARC_POINT& pFirst = pts.front();

            bool needsClosing = ( std::abs( pLast.x - pFirst.x ) > 0.001
                                  || std::abs( pLast.y - pFirst.y ) > 0.001 );

            if( needsClosing )
            {
                PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard );

                if( pFirst.is_arc )
                {
                    shape->SetShape( SHAPE_T::ARC );
                    VECTOR2I center( scaleCoord( pFirst.arc.cx, true ),
                                     scaleCoord( pFirst.arc.cy, false ) );
                    VECTOR2I start( scaleCoord( pLast.x, true ),
                                    scaleCoord( pLast.y, false ) );
                    VECTOR2I end( scaleCoord( pFirst.x, true ),
                                  scaleCoord( pFirst.y, false ) );

                    if( pFirst.arc.delta_angle > 0 )
                        std::swap( start, end );

                    shape->SetCenter( center );
                    shape->SetStart( start );
                    shape->SetEnd( end );
                }
                else
                {
                    shape->SetShape( SHAPE_T::SEGMENT );
                    shape->SetStart( VECTOR2I( scaleCoord( pLast.x, true ),
                                               scaleCoord( pLast.y, false ) ) );
                    shape->SetEnd( VECTOR2I( scaleCoord( pFirst.x, true ),
                                             scaleCoord( pFirst.y, false ) ) );
                }

                shape->SetWidth( scaleSize( graphic.width ) );
                shape->SetLayer( graphicLayer );
                m_loadBoard->Add( shape );
            }
        }
    }
}


void PCB_IO_PADS::generateDrcRules( const wxString& aFileName )
{
    wxFileName fn( aFileName );
    fn.SetExt( wxT( "kicad_dru" ) );

    wxString customRules = wxT( "(version 1)\n" );

    const auto& diffPairs = m_parser->GetDiffPairs();

    for( const auto& dp : diffPairs )
    {
        if( dp.name.empty() || ( dp.gap <= 0 && dp.width <= 0 ) )
            continue;

        wxString ruleName = wxString::Format( wxT( "DiffPair_%s" ), wxString::FromUTF8( dp.name ) );

        if( dp.gap > 0 && !dp.positive_net.empty() && !dp.negative_net.empty() )
        {
            wxString posNet = PADS_COMMON::ConvertInvertedNetName( dp.positive_net );
            wxString negNet = PADS_COMMON::ConvertInvertedNetName( dp.negative_net );
            double gapMm = dp.gap * m_scaleFactor / PADS_UNIT_CONVERTER::MM_TO_NM;
            wxString gapStr = wxString::FromUTF8( FormatDouble2Str( gapMm ) ) + wxT( "mm" );

            customRules += wxString::Format(
                wxT( "\n(rule \"%s_gap\"\n" )
                wxT( "  (condition \"A.NetName == '%s' && B.NetName == '%s'\")\n" )
                wxT( "  (constraint clearance (min %s)))\n" ),
                ruleName, posNet, negNet, gapStr );
        }
    }

    if( customRules.length() > 15 )
    {
        wxFile rulesFile( fn.GetFullPath(), wxFile::write );

        if( rulesFile.IsOpened() )
            rulesFile.Write( customRules );
    }
}


void PCB_IO_PADS::reportStatistics()
{
    if( !m_reporter )
        return;

    size_t trackCount = 0;
    size_t viaCount = 0;

    for( PCB_TRACK* track : m_loadBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            viaCount++;
        else
            trackCount++;
    }

    m_reporter->Report( wxString::Format( _( "Imported %zu footprints, %d nets, %zu tracks,"
                                              " %zu vias, %zu zones" ),
                                           m_loadBoard->Footprints().size(),
                                           m_loadBoard->GetNetCount(),
                                           trackCount, viaCount,
                                           m_loadBoard->Zones().size() ),
                         RPT_SEVERITY_INFO );
}


std::map<wxString, PCB_LAYER_ID> PCB_IO_PADS::DefaultLayerMappingCallback(
        const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector )
{
    std::map<wxString, PCB_LAYER_ID> layer_map;

    for( const INPUT_LAYER_DESC& layer : aInputLayerDescriptionVector )
    {
        layer_map[layer.Name] = layer.AutoMapLayer;
    }

    return layer_map;
}


int PCB_IO_PADS::scaleSize( double aVal ) const
{
    return static_cast<int>( m_unitConverter.ToNanometersSize( aVal ) );
}


int PCB_IO_PADS::scaleCoord( double aVal, bool aIsX ) const
{
    double origin = aIsX ? m_originX : m_originY;

    long long origin_nm = static_cast<long long>( std::round( origin * m_scaleFactor ) );
    long long val_nm = static_cast<long long>( std::round( aVal * m_scaleFactor ) );

    if( aIsX )
        return static_cast<int>( val_nm - origin_nm );
    else
        return static_cast<int>( origin_nm - val_nm );
}


PCB_LAYER_ID PCB_IO_PADS::getMappedLayer( int aPadsLayer ) const
{
    for( const auto& info : m_layerInfos )
    {
        if( info.padsLayerNum == aPadsLayer )
        {
            auto it = m_layer_map.find( wxString::FromUTF8( info.name ) );

            if( it != m_layer_map.end() && it->second != UNDEFINED_LAYER )
                return it->second;

            return m_layerMapper.GetAutoMapLayer( aPadsLayer, info.type );
        }
    }

    return m_layerMapper.GetAutoMapLayer( aPadsLayer );
}


void PCB_IO_PADS::ensureNet( const std::string& aNetName )
{
    if( aNetName.empty() )
        return;

    wxString wxName = PADS_COMMON::ConvertInvertedNetName( aNetName );

    if( m_loadBoard->FindNet( wxName ) == nullptr )
    {
        NETINFO_ITEM* net = new NETINFO_ITEM( m_loadBoard, wxName,
                                               m_loadBoard->GetNetCount() + 1 );
        m_loadBoard->Add( net );
    }
}


void PCB_IO_PADS::clearLoadingState()
{
    m_loadBoard = nullptr;
    m_parser = nullptr;
    m_unitConverter = PADS_UNIT_CONVERTER();
    m_layerMapper = PADS_LAYER_MAPPER();
    m_layerInfos.clear();
    m_scaleFactor = 0.0;
    m_originX = 0.0;
    m_originY = 0.0;
    m_pinToNetMap.clear();
    m_partToBlockMap.clear();
    m_testPointIndex = 1;
}


void PCB_IO_PADS::loadBoardSetup()
{
    m_layerMapper.SetCopperLayerCount( m_parser->GetParameters().layer_count );

    std::vector<PADS_IO::LAYER_INFO> padsLayerInfos = m_parser->GetLayerInfos();

    auto convertLayerType = []( PADS_IO::PADS_LAYER_FUNCTION func ) -> PADS_LAYER_TYPE {
        switch( func )
        {
        case PADS_IO::PADS_LAYER_FUNCTION::ROUTING:
        case PADS_IO::PADS_LAYER_FUNCTION::PLANE:
        case PADS_IO::PADS_LAYER_FUNCTION::MIXED:
            return PADS_LAYER_TYPE::COPPER_INNER;
        case PADS_IO::PADS_LAYER_FUNCTION::SOLDER_MASK:
            return PADS_LAYER_TYPE::SOLDERMASK_TOP;
        case PADS_IO::PADS_LAYER_FUNCTION::PASTE_MASK:
            return PADS_LAYER_TYPE::PASTE_TOP;
        case PADS_IO::PADS_LAYER_FUNCTION::SILK_SCREEN:
            return PADS_LAYER_TYPE::SILKSCREEN_TOP;
        case PADS_IO::PADS_LAYER_FUNCTION::ASSEMBLY:
            return PADS_LAYER_TYPE::ASSEMBLY_TOP;
        case PADS_IO::PADS_LAYER_FUNCTION::DOCUMENTATION:
            return PADS_LAYER_TYPE::DOCUMENTATION;
        case PADS_IO::PADS_LAYER_FUNCTION::DRILL:
            return PADS_LAYER_TYPE::DRILL_DRAWING;
        default:
            return PADS_LAYER_TYPE::UNKNOWN;
        }
    };

    for( const auto& padsInfo : padsLayerInfos )
    {
        PADS_LAYER_INFO info;
        info.padsLayerNum = padsInfo.number;
        info.name = padsInfo.name;

        if( padsInfo.layer_type != PADS_IO::PADS_LAYER_FUNCTION::UNKNOWN )
        {
            info.type = convertLayerType( padsInfo.layer_type );

            std::string lowerName = padsInfo.name;
            std::transform( lowerName.begin(), lowerName.end(), lowerName.begin(),
                            []( unsigned char c ){ return std::tolower( c ); } );

            bool isBottom = lowerName.find( "bottom" ) != std::string::npos
                            || lowerName.find( "bot" ) != std::string::npos;

            if( info.type == PADS_LAYER_TYPE::SOLDERMASK_TOP && isBottom )
                info.type = PADS_LAYER_TYPE::SOLDERMASK_BOTTOM;
            else if( info.type == PADS_LAYER_TYPE::PASTE_TOP && isBottom )
                info.type = PADS_LAYER_TYPE::PASTE_BOTTOM;
            else if( info.type == PADS_LAYER_TYPE::SILKSCREEN_TOP && isBottom )
                info.type = PADS_LAYER_TYPE::SILKSCREEN_BOTTOM;
            else if( info.type == PADS_LAYER_TYPE::ASSEMBLY_TOP && isBottom )
                info.type = PADS_LAYER_TYPE::ASSEMBLY_BOTTOM;
            else if( info.type == PADS_LAYER_TYPE::COPPER_INNER )
            {
                if( padsInfo.number == 1 )
                    info.type = PADS_LAYER_TYPE::COPPER_TOP;
                else if( padsInfo.number == m_parser->GetParameters().layer_count )
                    info.type = PADS_LAYER_TYPE::COPPER_BOTTOM;
            }
        }
        else
        {
            info.type = m_layerMapper.GetLayerType( padsInfo.number );
        }

        info.required = padsInfo.required;
        m_layerInfos.push_back( info );
    }

    std::vector<INPUT_LAYER_DESC> inputDescs =
            m_layerMapper.BuildInputLayerDescriptions( m_layerInfos );

    if( m_layer_mapping_handler )
        m_layer_map = m_layer_mapping_handler( inputDescs );

    int copperLayerCount = m_parser->GetParameters().layer_count;

    if( copperLayerCount < 1 )
        copperLayerCount = 2;

    m_loadBoard->SetCopperLayerCount( copperLayerCount );

    if( m_parser->IsBasicUnits() )
    {
        m_unitConverter.SetBasicUnitsMode( true );
    }
    else
    {
        switch( m_parser->GetParameters().units )
        {
        case PADS_IO::UNIT_TYPE::MILS:
            m_unitConverter.SetBaseUnits( PADS_UNIT_TYPE::MILS );
            break;
        case PADS_IO::UNIT_TYPE::METRIC:
            m_unitConverter.SetBaseUnits( PADS_UNIT_TYPE::METRIC );
            break;
        case PADS_IO::UNIT_TYPE::INCHES:
            m_unitConverter.SetBaseUnits( PADS_UNIT_TYPE::INCHES );
            break;
        }
    }

    m_scaleFactor = m_parser->IsBasicUnits()
            ? PADS_UNIT_CONVERTER::BASIC_TO_NM
            : ( m_parser->GetParameters().units == PADS_IO::UNIT_TYPE::MILS
                    ? PADS_UNIT_CONVERTER::MILS_TO_NM
                    : m_parser->GetParameters().units == PADS_IO::UNIT_TYPE::METRIC
                            ? PADS_UNIT_CONVERTER::MM_TO_NM
                            : PADS_UNIT_CONVERTER::INCHES_TO_NM );

    const auto& designRules = m_parser->GetDesignRules();
    BOARD_DESIGN_SETTINGS& bds = m_loadBoard->GetDesignSettings();

    bds.m_MinClearance = scaleSize( designRules.min_clearance );
    bds.m_TrackMinWidth = scaleSize( designRules.min_track_width );
    bds.m_ViasMinSize = scaleSize( designRules.min_via_size );
    bds.m_MinThroughDrill = scaleSize( designRules.min_via_drill );
    bds.m_HoleToHoleMin = scaleSize( designRules.hole_to_hole );
    bds.m_SilkClearance = scaleSize( designRules.silk_clearance );
    bds.m_SolderMaskExpansion = scaleSize( designRules.mask_clearance );

    bds.GetDefaultZoneSettings().m_ZoneClearance = scaleSize( designRules.default_clearance );

    bds.SetCustomTrackWidth( scaleSize( designRules.default_track_width ) );
    bds.SetCustomViaSize( scaleSize( designRules.default_via_size ) );
    bds.SetCustomViaDrill( scaleSize( designRules.default_via_drill ) );

    std::shared_ptr<NETCLASS> defaultNetclass = bds.m_NetSettings->GetDefaultNetclass();

    if( defaultNetclass )
    {
        defaultNetclass->SetClearance( scaleSize( designRules.default_clearance ) );
        defaultNetclass->SetTrackWidth( scaleSize( designRules.default_track_width ) );
        defaultNetclass->SetViaDiameter( scaleSize( designRules.default_via_size ) );
        defaultNetclass->SetViaDrill( scaleSize( designRules.default_via_drill ) );
    }

    const auto& netClasses = m_parser->GetNetClasses();

    for( const auto& nc : netClasses )
    {
        if( nc.name.empty() )
            continue;

        wxString ncName = wxString::FromUTF8( nc.name );
        std::shared_ptr<NETCLASS> netclass = std::make_shared<NETCLASS>( ncName );

        if( nc.clearance > 0 )
            netclass->SetClearance( scaleSize( nc.clearance ) );

        if( nc.track_width > 0 )
            netclass->SetTrackWidth( scaleSize( nc.track_width ) );

        if( nc.via_size > 0 )
            netclass->SetViaDiameter( scaleSize( nc.via_size ) );

        if( nc.via_drill > 0 )
            netclass->SetViaDrill( scaleSize( nc.via_drill ) );

        if( nc.diff_pair_width > 0 )
            netclass->SetDiffPairWidth( scaleSize( nc.diff_pair_width ) );

        if( nc.diff_pair_gap > 0 )
            netclass->SetDiffPairGap( scaleSize( nc.diff_pair_gap ) );

        bds.m_NetSettings->SetNetclass( ncName, netclass );

        for( const std::string& netName : nc.net_names )
        {
            wxString wxNetName = PADS_COMMON::ConvertInvertedNetName( netName );
            bds.m_NetSettings->SetNetclassPatternAssignment( wxNetName, ncName );
        }
    }

    const auto& diffPairs = m_parser->GetDiffPairs();

    for( const auto& dp : diffPairs )
    {
        if( dp.name.empty() )
            continue;

        wxString dpClassName =
                wxString::Format( wxT( "DiffPair_%s" ), wxString::FromUTF8( dp.name ) );
        std::shared_ptr<NETCLASS> dpNetclass = std::make_shared<NETCLASS>( dpClassName );

        if( dp.gap > 0 )
            dpNetclass->SetDiffPairGap( scaleSize( dp.gap ) );

        if( dp.width > 0 )
        {
            dpNetclass->SetDiffPairWidth( scaleSize( dp.width ) );
            dpNetclass->SetTrackWidth( scaleSize( dp.width ) );
        }

        bds.m_NetSettings->SetNetclass( dpClassName, dpNetclass );

        if( !dp.positive_net.empty() )
        {
            wxString wxPosNet = PADS_COMMON::ConvertInvertedNetName( dp.positive_net );
            bds.m_NetSettings->SetNetclassPatternAssignment( wxPosNet, dpClassName );
        }

        if( !dp.negative_net.empty() )
        {
            wxString wxNegNet = PADS_COMMON::ConvertInvertedNetName( dp.negative_net );
            bds.m_NetSettings->SetNetclassPatternAssignment( wxNegNet, dpClassName );
        }
    }

    m_originX = m_parser->GetParameters().origin.x;
    m_originY = m_parser->GetParameters().origin.y;

    const auto& boardOutlines = m_parser->GetBoardOutlines();

    if( !boardOutlines.empty() )
    {
        double min_x = std::numeric_limits<double>::max();
        double max_x = std::numeric_limits<double>::lowest();
        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::lowest();

        for( const auto& outline : boardOutlines )
        {
            for( const auto& pt : outline.points )
            {
                min_x = std::min( min_x, pt.x );
                max_x = std::max( max_x, pt.x );
                min_y = std::min( min_y, pt.y );
                max_y = std::max( max_y, pt.y );
            }
        }

        if( min_x < max_x && min_y < max_y )
        {
            m_originX = ( min_x + max_x ) / 2.0;
            m_originY = ( min_y + max_y ) / 2.0;
        }
    }
}
