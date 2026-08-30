/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_io_pads_binary.h"
#include "pads_binary_parser.h"
#include "pads_layer_mapper.h"
#include "pads_pcb_shapes.h"

#include <algorithm>
#include <memory>
#include <cmath>
#include <functional>
#include <set>

#include <board.h>
#include <pcb_track.h>
#include <footprint.h>
#include <pcb_group.h>
#include <zone.h>

#include <io/pads/pads_unit_converter.h>
#include <io/pads/pads_common.h>

#include <netinfo.h>
#include <wx/log.h>
#include <pad.h>
#include <pcb_shape.h>
#include <board_design_settings.h>
#include <netclass.h>
#include <project/net_settings.h>
#include <geometry/eda_angle.h>
#include <string_utils.h>
#include <progress_reporter.h>
#include <reporter.h>
#include <locale_io.h>
#include <advanced_config.h>
#include <geometry/shape_arc.h>


PCB_IO_PADS_BINARY::PCB_IO_PADS_BINARY() :
        PCB_IO( "PADS Binary" )
{
    m_minObjectSize = ADVANCED_CFG::GetCfg().m_PcbImportMinObjectSizeNm;

    LAYER_MAPPABLE_PLUGIN::RegisterCallback(
            std::bind( &PCB_IO_PADS_BINARY::DefaultLayerMappingCallback, this, std::placeholders::_1 ) );
}


PCB_IO_PADS_BINARY::~PCB_IO_PADS_BINARY() = default;


const IO_BASE::IO_FILE_DESC PCB_IO_PADS_BINARY::GetBoardFileDesc() const
{
    IO_FILE_DESC desc;
    desc.m_FileExtensions.emplace_back( "pcb" );
    desc.m_Description = "PADS Binary";
    return desc;
}


const IO_BASE::IO_FILE_DESC PCB_IO_PADS_BINARY::GetLibraryDesc() const
{
    return IO_FILE_DESC( "PADS Binary Library", { "pcb" } );
}


long long PCB_IO_PADS_BINARY::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    return 0;
}


bool PCB_IO_PADS_BINARY::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    return PADS_IO::BINARY_PARSER::IsBinaryPadsFile( aFileName );
}


bool PCB_IO_PADS_BINARY::CanReadLibrary( const wxString& aFileName ) const
{
    // The .pcb extension is shared with other tools, so a non-PADS .pcb must not be
    // mis-identified; content-check the magic rather than trusting the extension.
    if( !PCB_IO::CanReadLibrary( aFileName ) )
        return false;

    return PADS_IO::BINARY_PARSER::IsBinaryPadsFile( aFileName );
}


BOARD* PCB_IO_PADS_BINARY::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                      const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    LOCALE_IO setlocale;

    // Never own the caller's board; a throw below would delete it while the caller still uses it
    std::unique_ptr<BOARD> ownedBoard;
    BOARD*                 board = aAppendToMe;

    if( !board )
    {
        ownedBoard = std::make_unique<BOARD>();
        board = ownedBoard.get();
    }

    if( m_reporter )
        m_reporter->Report( _( "Starting PADS binary PCB import" ), RPT_SEVERITY_INFO );

    if( m_progressReporter )
        m_progressReporter->SetNumPhases( 3 );

    PADS_IO::BINARY_PARSER parser;

    try
    {
        parser.Parse( aFileName );
    }
    catch( const std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( "Error parsing PADS binary file: %s", e.what() ) );
    }

    m_loadBoard = board;
    m_parser = &parser;
    m_converter = std::make_unique<PADS_PCB_CONVERTER>( m_loadBoard, m_reporter );

    try
    {
        if( m_progressReporter )
            m_progressReporter->BeginPhase( 1 );

        loadBoardSetup();

        // Nets are created before the geometry that references them; loadTracksAndVias resolves
        // each track and via net against this table.
        loadNets();

        if( m_progressReporter )
            m_progressReporter->BeginPhase( 2 );

        loadFootprints();

        // Cluster groups reference the footprints loadFootprints just added, so they follow it.
        loadClusterGroups();
        loadBoardOutline();
        loadGraphicLines();
        loadTracksAndVias();
        m_converter->LoadTexts( m_parser->GetTexts() );
        loadCopperShapes();
        loadZones();
        m_converter->LoadKeepouts( m_parser->GetKeepouts() );
        m_converter->LoadDimensions( m_parser->GetDimensions() );

        // Appending merges into a board that already has its own rule file, so nothing would
        // ever load rules written beside the PADS source
        if( !aAppendToMe )
            m_converter->WriteDiffPairRules( aFileName, m_parser->GetDiffPairs() );

        m_converter->ReportStatistics();
    }
    catch( ... )
    {
        clearLoadingState();
        throw;
    }

    clearLoadingState();
    return aAppendToMe ? aAppendToMe : ownedBoard.release();
}


void PCB_IO_PADS_BINARY::loadBoardSetup()
{
    std::vector<PADS_IO::LAYER_INFO> padsLayerInfos = m_parser->GetLayerInfos();

    // Binary layer records often carry no function code, so the layer name is the only
    // remaining clue to what the layer is for.
    m_converter->SetupLayers( padsLayerInfos, m_parser->GetParameters().layer_count, m_layer_mapping_handler, true );

    // Binary files always use BASIC units, and the stackup thicknesses are BASIC too, so the
    // units have to be settled before anything is scaled.
    m_converter->UnitConverter().SetBasicUnitsMode( true );
    m_converter->SetScaleFactor( PADS_UNIT_CONVERTER::BASIC_TO_NM );

    m_converter->BuildStackup( padsLayerInfos );

    const PADS_IO::POINT& origin = m_parser->GetParameters().origin;

    m_converter->SetOrigin( origin.x, origin.y );

    // Fall back to the board-outline center only when there is no DFT origin; the DFT origin
    // gives exact coordinates and overriding it would shift all parts.
    if( origin.x == 0.0 && origin.y == 0.0 )
        m_converter->SetOriginFromOutlines( m_parser->GetBoardOutlines() );
}


void PCB_IO_PADS_BINARY::loadNets()
{
    const auto& nets = m_parser->GetNets();

    for( const auto& padsNet : nets )
        m_converter->EnsureNet( padsNet.name );

    // Key on the decoded text so buildPad() can match against the reference it already built.
    // ToStdString() would re-encode through the locale, which LOCALE_IO has pinned to "C"
    for( const auto& padsNet : nets )
    {
        for( const PADS_IO::NET_PIN& pin : padsNet.pins )
        {
            wxString key = PADS_COMMON::ConvertText( pin.ref_des ) + wxT( "." )
                           + PADS_COMMON::ConvertText( pin.pin_name );
            m_pinToNetMap[key] = padsNet.name;
        }
    }

    // One KiCad NETCLASS per PADS net class, with its member nets. Empty on boards with no
    // net classes (such as the v0x2021 dialect), where this is a no-op.
    const std::vector<PADS_IO::BIN_NET_CLASS_DEF>& netClasses = m_parser->GetNetClasses();
    const std::vector<PADS_IO::DIFF_PAIR_DEF>& diffPairs = m_parser->GetDiffPairs();

    if( netClasses.empty() && diffPairs.empty() )
        return;

    std::shared_ptr<NET_SETTINGS> netSettings = m_loadBoard->GetDesignSettings().m_NetSettings;

    for( const PADS_IO::BIN_NET_CLASS_DEF& nc : netClasses )
    {
        if( nc.name.empty() || nc.nets.empty() )
            continue;

        wxString className = PADS_COMMON::ConvertText( nc.name );

        if( !netSettings->HasNetclass( className ) )
        {
            std::shared_ptr<NETCLASS> kicadClass = std::make_shared<NETCLASS>( className );

            // KiCad's netclass carries one clearance and one track width; the PADS min/max
            // values are advisory and dropped.
            if( nc.hasRuleValues )
            {
                if( nc.clearance > 0 )
                    kicadClass->SetClearance( scaleSize( nc.clearance ) );

                if( nc.trackWidth > 0 )
                    kicadClass->SetTrackWidth( scaleSize( nc.trackWidth ) );
            }

            netSettings->SetNetclass( className, kicadClass );
        }

        // Label assignments are rebuilt from schematic labels on every connectivity pass, so a
        // board-side membership has to be a pattern assignment to survive
        for( const std::string& net : nc.nets )
            netSettings->SetNetclassPatternAssignment( PADS_COMMON::ConvertInvertedNetName( net ), className );
    }

    // Each differential pair becomes one DiffPair_<name> net class.
    for( const PADS_IO::DIFF_PAIR_DEF& dp : diffPairs )
    {
        if( dp.name.empty() )
            continue;

        wxString dpClassName = wxString::Format( wxT( "DiffPair_%s" ), PADS_COMMON::ConvertText( dp.name ) );
        std::shared_ptr<NETCLASS> dpNetclass = std::make_shared<NETCLASS>( dpClassName );

        if( dp.gap > 0 )
            dpNetclass->SetDiffPairGap( scaleSize( dp.gap ) );

        if( dp.width > 0 )
        {
            dpNetclass->SetDiffPairWidth( scaleSize( dp.width ) );
            dpNetclass->SetTrackWidth( scaleSize( dp.width ) );
        }

        netSettings->SetNetclass( dpClassName, dpNetclass );

        if( !dp.positive_net.empty() )
            netSettings->SetNetclassPatternAssignment( PADS_COMMON::ConvertInvertedNetName( dp.positive_net ),
                                                       dpClassName );

        if( !dp.negative_net.empty() )
            netSettings->SetNetclassPatternAssignment( PADS_COMMON::ConvertInvertedNetName( dp.negative_net ),
                                                       dpClassName );
    }
}


void PCB_IO_PADS_BINARY::loadFootprints()
{
    const auto& decals = m_parser->GetPartDecals();
    const auto& parts = m_parser->GetParts();

    for( const auto& padsPart : parts )
    {
        FOOTPRINT* footprint = new FOOTPRINT( m_loadBoard );
        footprint->SetReference( PADS_COMMON::ConvertText( padsPart.name ) );

        KIID symbolUuid = PADS_COMMON::GenerateDeterministicUuid( padsPart.name );
        KIID_PATH path;
        path.push_back( symbolUuid );
        footprint->SetPath( path );

        std::string decalName = padsPart.decal;

        LIB_ID fpid;

        if( !decalName.empty() )
            fpid.SetLibItemName( PADS_COMMON::ConvertText( decalName ) );
        else
            fpid.SetLibItemName( PADS_COMMON::ConvertText( padsPart.name ) );

        footprint->SetFPID( fpid );

        // A part referencing a *PARTTYPE alias (often a manufacturer part number) shows that
        // alias as its value, not the physical decal it resolves to; see linkPartsToDecals().
        footprint->SetValue( PADS_COMMON::ConvertText( padsPart.value.empty() ? padsPart.decal
                                                                              : padsPart.value ) );

        footprint->SetPosition( scalePoint( padsPart.location.x, padsPart.location.y ) );
        footprint->SetOrientation( EDA_ANGLE( padsPart.rotation, DEGREES_T ) );
        footprint->SetLayer( F_Cu );
        footprint->Reference().SetLayer( F_Fab );
        footprint->Reference().SetVisible( false );
        footprint->Value().SetLayer( F_Fab );
        footprint->Value().SetVisible( false );

        auto decalIt = decals.find( decalName );

        if( decalIt == decals.end() )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "Part '%s': decal '%s' not found, no pads created" ),
                                                      padsPart.name, decalName ),
                                    RPT_SEVERITY_WARNING );
            }
        }

        if( decalIt != decals.end() )
        {
            const PADS_IO::PART_DECAL& decal = decalIt->second;
            EDA_ANGLE                  partOrient( padsPart.rotation, DEGREES_T );

            for( size_t termIdx = 0; termIdx < decal.terminals.size(); ++termIdx )
                buildPad( footprint, decal, termIdx, partOrient );
        }

        for( const PADS_IO::ATTRIBUTE& attribute : padsPart.attributes )
        {
            PCB_FIELD* field = nullptr;

            if( attribute.name == "Ref.Des." )
                field = &footprint->Reference();
            else if( attribute.name == "Part Type" )
                field = &footprint->Value();

            if( !field )
                continue;

            int scaledSize = scaleSize( attribute.height );
            int charHeight = static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextHeightScale );
            int charWidth = static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsPcbTextWidthScale );
            field->SetTextSize( VECTOR2I( charWidth, charHeight ) );

            if( attribute.width > 0 )
                field->SetTextThickness( scaleSize( attribute.width ) );

            VECTOR2I  offset( scaleSize( attribute.x ), -scaleSize( attribute.y ) );
            EDA_ANGLE partAngle( padsPart.rotation, DEGREES_T );
            RotatePoint( offset, partAngle );

            EDA_ANGLE textAngle( attribute.orientation + padsPart.rotation, DEGREES_T );
            VECTOR2I  textShift( -ADVANCED_CFG::GetCfg().m_PadsTextAnchorOffsetNm, 0 );
            RotatePoint( textShift, textAngle );

            field->SetPosition( footprint->GetPosition() + offset + textShift );
            field->SetTextAngle( textAngle );
            field->SetKeepUpright( false );
            field->SetVisible( attribute.visible );
            field->SetLayer( F_Fab );

            if( attribute.hjust == "LEFT" )
                field->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            else if( attribute.hjust == "RIGHT" )
                field->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            else
                field->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );

            if( attribute.vjust == "UP" )
                field->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
            else if( attribute.vjust == "DOWN" )
                field->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
            else
                field->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        }

        m_loadBoard->Add( footprint );

        if( padsPart.bottom_layer )
        {
            footprint->Flip( footprint->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );

            for( PAD* pad : footprint->Pads() )
            {
                if( pad->GetAttribute() == PAD_ATTRIB::PTH && pad->Padstack().Mode() == PADSTACK::MODE::NORMAL )
                {
                    pad->SetLayerSet( PAD::PTHMask() | ( pad->GetLayerSet() & LSET( { F_Paste, B_Paste } ) ) );
                }
            }
        }

        m_partFootprints.push_back( footprint );
    }
}


void PCB_IO_PADS_BINARY::buildPad( FOOTPRINT* aFootprint, const PADS_IO::PART_DECAL& aDecal, size_t aTermIdx,
                                   const EDA_ANGLE& aPartOrient )
{
    const auto& term = aDecal.terminals[aTermIdx];
    PAD* pad = new PAD( aFootprint );
    aFootprint->Add( pad );

    pad->SetNumber( PADS_COMMON::ConvertText( term.name ) );

    VECTOR2I padPos( scaleSize( term.x ), -scaleSize( term.y ) );
    RotatePoint( padPos, aPartOrient );
    pad->SetPosition( aFootprint->GetPosition() + padPos );

    int pinNum = static_cast<int>( aTermIdx + 1 );
    auto stackIt = aDecal.pad_stacks.find( pinNum );

    if( stackIt == aDecal.pad_stacks.end() )
        stackIt = aDecal.pad_stacks.find( 0 );

    if( stackIt != aDecal.pad_stacks.end() && !stackIt->second.empty() )
    {
        const std::vector<PADS_IO::PAD_STACK_LAYER>& stack = stackIt->second;
        const PADS_IO::PAD_STACK_LAYER& layerDef = stack[0];

        std::set<int> serializedLayers;
        bool          ambiguousLayers = false;

        // A relief or anti-pad row repeats the layer ordinal of the copper row it qualifies, so
        // counting it here would read every plane-relief padstack as ambiguous
        for( auto it = std::next( stack.begin() ); it != stack.end(); ++it )
        {
            if( PADS_IO::IsCopperPadRow( *it ) )
                ambiguousLayers |= !serializedLayers.insert( it->layer ).second;
        }

        bool modernZeroDefaultSmd = m_parser->GetVersion() == 0x2024 && layerDef.drill == 0 && layerDef.sizeA <= 0;
        bool legacyZeroDefaultSmd = m_parser->GetVersion() == 0x2022 && layerDef.drill == 0
                                    && layerDef.sizeA <= 0;
        bool zeroDefaultSmd = modernZeroDefaultSmd || legacyZeroDefaultSmd;
        bool hasExplicitLayers = stack.size() > 1 && ( layerDef.drill > 0 || zeroDefaultSmd ) && !ambiguousLayers;
        LSET layerSet;

        if( hasExplicitLayers )
        {
            pad->Padstack().SetMode( PADSTACK::MODE::FRONT_INNER_BACK );
            bool appliedExplicitLayer = false;
            const PADS_IO::PAD_STACK_LAYER* inheritedLegacyShape = nullptr;

            if( legacyZeroDefaultSmd )
            {
                auto inherited = std::find_if( stack.begin(), stack.end(),
                                               []( const PADS_IO::PAD_STACK_LAYER& aLayer )
                                               {
                                                   return aLayer.layer == 0 && aLayer.sizeA > 0;
                                               } );

                if( inherited != stack.end() )
                    inheritedLegacyShape = &*inherited;
            }

            for( const PADS_IO::PAD_STACK_LAYER& stackLayer : stack )
            {
                if( stackLayer.sizeA <= 0 )
                    continue;

                // RT/ST are plane thermal-relief spoke patterns and RA/SA are anti-pad
                // clearances. Neither is pad copper, so they must not reach the shape below.
                // KiCad derives the relief geometry from the zone, so an RT/ST row only says
                // that this pad connects through spokes rather than solid copper.
                if( PADS_IO::IsThermalReliefPadRow( stackLayer ) )
                {
                    pad->SetLocalZoneConnection( ZONE_CONNECTION::THERMAL );
                    continue;
                }

                if( PADS_IO::IsAntiPadRow( stackLayer ) )
                    continue;

                if( modernZeroDefaultSmd && stackLayer.layer == 0 )
                    continue;

                if( legacyZeroDefaultSmd && stackLayer.layer <= 0 )
                    continue;

                PCB_LAYER_ID kicadLayer = UNDEFINED_LAYER;

                if( modernZeroDefaultSmd && stackLayer.layer == -1 )
                {
                    kicadLayer = B_Paste;
                    layerSet.set( B_Paste );
                }
                else if( stackLayer.layer == -2 )
                {
                    kicadLayer = F_Cu;
                    layerSet.set( F_Cu );
                }
                else if( stackLayer.layer == -1 )
                {
                    kicadLayer = B_Cu;
                    layerSet.set( B_Cu );
                }
                else if( stackLayer.layer == 0 )
                {
                    if( m_loadBoard->GetCopperLayerCount() > 2 )
                    {
                        kicadLayer = In1_Cu;

                        for( int layer = 2; layer < m_parser->GetParameters().layer_count; ++layer )
                        {
                            PCB_LAYER_ID inner = getMappedLayer( layer );

                            if( IsCopperLayer( inner ) )
                                layerSet.set( inner );
                        }
                    }
                }
                else
                {
                    int padsLayer = stackLayer.layer;
                    kicadLayer = getMappedLayer( padsLayer );

                    if( kicadLayer != UNDEFINED_LAYER )
                        layerSet.set( kicadLayer );
                }

                if( kicadLayer != UNDEFINED_LAYER )
                {
                    PADS_IO::PAD_STACK_LAYER appliedLayer = stackLayer;

                    if( modernZeroDefaultSmd )
                    {
                        appliedLayer.sizeA *= 1.5;
                        appliedLayer.sizeB *= 1.5;
                        appliedLayer.corner_radius *= 1.5;
                    }
                    else if( inheritedLegacyShape )
                    {
                        appliedLayer.shape = inheritedLegacyShape->shape;
                        appliedLayer.sizeA = inheritedLegacyShape->sizeA * 1.5;
                        appliedLayer.sizeB = inheritedLegacyShape->sizeB * 1.5;
                    }

                    applyPadShape( pad, appliedLayer, kicadLayer );
                    appliedExplicitLayer = true;

                    if( zeroDefaultSmd && appliedLayer.shape == "R" )
                        pad->SetRoundRectRadiusRatio( kicadLayer, 0.25 );
                }
            }

            if( zeroDefaultSmd && !appliedExplicitLayer )
            {
                applyPadShape( pad, layerDef, F_Cu );
                applyPadShape( pad, layerDef, F_Mask );
                applyPadShape( pad, layerDef, F_Paste );
                layerSet = LSET( { F_Cu, F_Mask, F_Paste } );
            }
        }
        else
        {
            applyPadShape( pad, layerDef, F_Cu );
        }

        pad->SetOrientation( aPartOrient + EDA_ANGLE( layerDef.rotation, DEGREES_T ) );

        int drill = scaleSize( layerDef.drill );

        if( layerDef.slot_length > 0 && layerDef.slot_length != layerDef.drill )
        {
            pad->SetDrillShape( PAD_DRILL_SHAPE::OBLONG );

            int    drillMajor = scaleSize( layerDef.slot_length );
            double relativeAngle = std::fmod( layerDef.slot_orientation - layerDef.rotation, 360.0 );

            if( relativeAngle < 0 )
                relativeAngle += 360.0;

            bool vertical = ( relativeAngle > 45.0 && relativeAngle < 135.0 )
                            || ( relativeAngle > 225.0 && relativeAngle < 315.0 );
            pad->SetDrillSize( vertical ? VECTOR2I( drill, drillMajor ) : VECTOR2I( drillMajor, drill ) );
        }
        else
        {
            pad->SetDrillSize( VECTOR2I( drill, drill ) );
        }

        if( drill == 0 )
        {
            pad->SetAttribute( PAD_ATTRIB::SMD );

            if( hasExplicitLayers )
            {
                if( !zeroDefaultSmd )
                {
                    if( layerSet.test( F_Cu ) )
                        layerSet |= LSET( { F_Mask, F_Paste } );

                    if( layerSet.test( B_Cu ) )
                        layerSet |= LSET( { B_Mask, B_Paste } );
                }

                pad->SetLayerSet( layerSet );
            }
            else
            {
                pad->SetLayerSet( LSET( { F_Cu, F_Mask, F_Paste } ) );
            }
        }
        else
        {
            // Keep whatever mask and paste rows the stack carried, the way the ASCII path does
            LSET maskPasteBits = layerSet & LSET( { F_Mask, B_Mask, F_Paste, B_Paste } );

            pad->Padstack().SetMode( PADSTACK::MODE::NORMAL );
            applyPadShape( pad, layerDef, F_Cu );

            if( layerDef.plated )
            {
                pad->SetAttribute( PAD_ATTRIB::PTH );
                pad->SetLayerSet( PAD::PTHMask() | maskPasteBits );
            }
            else
            {
                pad->SetAttribute( PAD_ATTRIB::NPTH );
                pad->SetNumber( wxString() );
                pad->SetLayerSet( PAD::UnplatedHoleMask() | maskPasteBits );
            }
        }
    }
    else
    {
        // 38100 basic units = 1 mil; default 60 mil pad.
        int defaultPad = scaleSize( 60.0 * PADS_IO::SDB_BASIC_PER_MIL );
        pad->SetSize( F_Cu, VECTOR2I( defaultPad, defaultPad ) );
        pad->SetShape( F_Cu, PAD_SHAPE::CIRCLE );
        pad->SetAttribute( PAD_ATTRIB::PTH );
        pad->SetLayerSet( PAD::PTHMask() );
    }

    // An unplated hole is mechanical; joining it to a net pulls ratsnest to a mounting hole
    if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
        return;

    auto netIt = m_pinToNetMap.find( aFootprint->GetReference() + wxT( "." )
                                     + PADS_COMMON::ConvertText( term.name ) );

    if( netIt != m_pinToNetMap.end() )
    {
        NETINFO_ITEM* net = m_loadBoard->FindNet( PADS_COMMON::ConvertInvertedNetName( netIt->second ) );

        if( net )
            pad->SetNet( net );
    }
}


void PCB_IO_PADS_BINARY::applyPadShape( PAD* aPad, const PADS_IO::PAD_STACK_LAYER& aLayer,
                                        PCB_LAYER_ID aKiCadLayer )
{
    auto scaledPadSize = [&]( double aValue )
    {
        return std::max( m_minObjectSize, scaleSize( aValue ) );
    };

    VECTOR2I size( scaledPadSize( aLayer.sizeA ), scaledPadSize( aLayer.sizeA ) );

    auto applyCornerRadius = [&]( const VECTOR2I& aSize, bool aDefaultRound )
    {
        if( aLayer.corner_radius > 0 )
        {
            int    radius = scaledPadSize( aLayer.corner_radius );
            int    minimumDimension = std::min( aSize.x, aSize.y );
            double ratio = minimumDimension > 0
                                   ? std::min( static_cast<double>( radius ) / minimumDimension, 0.5 )
                                   : 0.25;

            aPad->SetShape( aKiCadLayer, PAD_SHAPE::ROUNDRECT );
            aPad->SetRoundRectRadiusRatio( aKiCadLayer, ratio );
        }
        else if( aDefaultRound )
        {
            aPad->SetShape( aKiCadLayer, PAD_SHAPE::ROUNDRECT );
            aPad->SetRoundRectRadiusRatio( aKiCadLayer, 0.25 );
        }
    };

    aPad->SetShape( aKiCadLayer, PADS_PCB::PadsShapeToKiCad( aLayer.shape ) );

    if( aLayer.shape == "O" || aLayer.shape == "OF" )
    {
        VECTOR2I ovalSize( scaledPadSize( aLayer.sizeB ), scaledPadSize( aLayer.sizeA ) );
        aPad->SetSize( aKiCadLayer, ovalSize );
    }
    else if( aLayer.shape == "RF" )
    {
        VECTOR2I rectSize( scaledPadSize( aLayer.sizeB ), scaledPadSize( aLayer.sizeA ) );
        aPad->SetSize( aKiCadLayer, rectSize );
        applyCornerRadius( rectSize, false );

        if( aLayer.finger_offset != 0 )
        {
            int offset = scaleSize( aLayer.finger_offset );
            VECTOR2I padOffset( offset, 0 );
            RotatePoint( padOffset, EDA_ANGLE( aLayer.rotation, DEGREES_T ) );
            aPad->SetOffset( aKiCadLayer, padOffset );
        }
    }
    else if( aLayer.shape == "RC" || aLayer.shape == "OC" )
    {
        VECTOR2I rrSize( scaledPadSize( aLayer.sizeB ), scaledPadSize( aLayer.sizeA ) );
        aPad->SetSize( aKiCadLayer, rrSize );
        applyCornerRadius( rrSize, true );
    }
    else
    {
        aPad->SetSize( aKiCadLayer, size );

        if( aLayer.shape == "S" || aLayer.shape == "ST" )
            applyCornerRadius( size, false );
    }
}


void PCB_IO_PADS_BINARY::loadClusterGroups()
{
    const std::vector<PADS_IO::PART_CLUSTER>& clusters = m_parser->GetClusters();

    if( clusters.empty() )
        return;

    // One PCB_GROUP per cluster, keyed by the 1-based CLSTID the membership field references.
    std::map<int, PCB_GROUP*> clusterGroups;

    for( const PADS_IO::PART_CLUSTER& cluster : clusters )
    {
        PCB_GROUP* group = new PCB_GROUP( m_loadBoard );
        group->SetName( PADS_COMMON::ConvertText( cluster.name ) );
        m_loadBoard->Add( group );
        clusterGroups[cluster.id] = group;
    }

    // m_partFootprints is indexed by part index, so the part-index-keyed membership map
    // resolves directly to a footprint.
    const std::map<size_t, int>& partClusterIds = m_parser->GetPartClusterIds();

    for( size_t i = 0; i < m_partFootprints.size(); ++i )
    {
        auto idIt = partClusterIds.find( i );

        if( idIt == partClusterIds.end() || idIt->second <= 0 )
            continue;

        auto groupIt = clusterGroups.find( idIt->second );

        if( groupIt != clusterGroups.end() )
            groupIt->second->AddItem( m_partFootprints[i] );
    }
}


void PCB_IO_PADS_BINARY::setBoardOutlineArc( PCB_SHAPE* aShape, const PADS_IO::ARC_POINT& aPrev,
                                             const PADS_IO::ARC_POINT& aCurr )
{
    aShape->SetShape( SHAPE_T::ARC );

    // A start/end/center triple is ambiguous (minor vs major arc) for shallow arcs, so
    // sample a midpoint at the sweep midpoint and pass start/mid/end; scaleCoord applies the
    // Y-axis flip uniformly.
    double midAngle = ( aCurr.arc.start_angle + aCurr.arc.delta_angle / 2.0 ) * M_PI / 180.0;
    double midX = aCurr.arc.cx + aCurr.arc.radius * std::cos( midAngle );
    double midY = aCurr.arc.cy + aCurr.arc.radius * std::sin( midAngle );

    VECTOR2I start = scalePoint( aPrev.x, aPrev.y );
    VECTOR2I mid = scalePoint( midX, midY );
    VECTOR2I end = scalePoint( aCurr.x, aCurr.y );

    aShape->SetArcGeometry( start, mid, end );
}


void PCB_IO_PADS_BINARY::loadBoardOutline()
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
                setBoardOutlineArc( shape, p1, p2 );
            }
            else
            {
                shape->SetShape( SHAPE_T::SEGMENT );
                shape->SetStart( scalePoint( p1.x, p1.y ) );
                shape->SetEnd( scalePoint( p2.x, p2.y ) );
            }

            shape->SetWidth( scaleSize( polyline.width ) );
            shape->SetLayer( Edge_Cuts );
            m_loadBoard->Add( shape );
        }

        if( polyline.closed && pts.size() > 2 )
        {
            const PADS_IO::ARC_POINT& pLast = pts.back();
            const PADS_IO::ARC_POINT& pFirst = pts.front();

            bool needsClosing = ( std::abs( pLast.x - pFirst.x ) > 0.001 || std::abs( pLast.y - pFirst.y ) > 0.001 );

            if( needsClosing )
            {
                PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard );

                if( pFirst.is_arc )
                {
                    setBoardOutlineArc( shape, pLast, pFirst );
                }
                else
                {
                    shape->SetShape( SHAPE_T::SEGMENT );
                    shape->SetStart( scalePoint( pLast.x, pLast.y ) );
                    shape->SetEnd( scalePoint( pFirst.x, pFirst.y ) );
                }

                shape->SetWidth( scaleSize( polyline.width ) );
                shape->SetLayer( Edge_Cuts );
                m_loadBoard->Add( shape );
            }
        }
    }
}


void PCB_IO_PADS_BINARY::loadGraphicLines()
{
    for( const PADS_IO::GRAPHIC_LINE& graphic : m_parser->GetGraphicLines() )
    {
        PCB_LAYER_ID layer = getMappedLayer( graphic.layer );

        if( layer == UNDEFINED_LAYER )
            continue;

        const std::vector<PADS_IO::ARC_POINT>& points = graphic.points;

        if( points.size() == 1 && points[0].is_arc && std::abs( points[0].arc.delta_angle - 360.0 ) < 0.1 )
        {
            PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard );
            VECTOR2I   center = scalePoint( points[0].arc.cx, points[0].arc.cy );
            int        radius = std::max( scaleSize( points[0].arc.radius ), m_minObjectSize );
            shape->SetShape( SHAPE_T::CIRCLE );
            shape->SetCenter( center );
            shape->SetEnd( VECTOR2I( center.x + radius, center.y ) );
            shape->SetWidth( scaleSize( graphic.width ) );
            shape->SetLayer( layer );
            m_loadBoard->Add( shape );
            continue;
        }

        if( points.size() < 2 )
            continue;

        for( size_t index = 1; index < points.size(); ++index )
        {
            const PADS_IO::ARC_POINT& previous = points[index - 1];
            const PADS_IO::ARC_POINT& current = points[index];

            if( std::abs( previous.x - current.x ) < 0.001 && std::abs( previous.y - current.y ) < 0.001 )
                continue;

            PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard );

            if( current.is_arc )
            {
                setBoardOutlineArc( shape, previous, current );
            }
            else
            {
                shape->SetShape( SHAPE_T::SEGMENT );
                shape->SetStart( scalePoint( previous.x, previous.y ) );
                shape->SetEnd( scalePoint( current.x, current.y ) );
            }

            shape->SetWidth( scaleSize( graphic.width ) );
            shape->SetLayer( layer );
            m_loadBoard->Add( shape );
        }
    }
}


void PCB_IO_PADS_BINARY::loadTracksAndVias()
{
    const auto& routes = m_parser->GetRoutes();
    for( const auto& route : routes )
    {
        NETINFO_ITEM* net = nullptr;

        if( !route.net_name.empty() )
        {
            net = m_loadBoard->FindNet( PADS_COMMON::ConvertInvertedNetName( route.net_name ) );

            if( !net )
            {
                if( m_reporter )
                {
                    m_reporter->Report( wxString::Format( _( "Route net '%s' not found, skipping" ), route.net_name ),
                            RPT_SEVERITY_WARNING );
                }

                continue;
            }
        }

        for( const auto& track_def : route.tracks )
        {
            if( track_def.points.size() < 2 )
                continue;

            PCB_LAYER_ID track_layer = getMappedLayer( track_def.layer );

            // An importer is best effort, and a route layer the user mapped to a non-copper
            // layer is ordinary input, so drop the track rather than the whole board
            if( !IsCopperLayer( track_layer ) )
            {
                if( m_reporter )
                {
                    m_reporter->Report( wxString::Format( _( "Skipped a route on non-copper or unmapped "
                                                             "PADS layer %d." ),
                                                          track_def.layer ),
                                        RPT_SEVERITY_WARNING );
                }

                continue;
            }

            int track_width = scaleSize( track_def.width );

            if( track_width <= 0 )
            {
                if( m_reporter )
                {
                    m_reporter->Report( wxString::Format( _( "Skipped a non-positive-width route on PADS layer %d." ),
                                                          track_def.layer ),
                                        RPT_SEVERITY_WARNING );
                }

                continue;
            }

            for( size_t i = 0; i < track_def.points.size() - 1; ++i )
            {
                const PADS_IO::ARC_POINT& p1 = track_def.points[i];
                const PADS_IO::ARC_POINT& p2 = track_def.points[i + 1];

                VECTOR2I start = scalePoint( p1.x, p1.y );
                VECTOR2I end = scalePoint( p2.x, p2.y );

                // Route-cell endpoints retain quantization residue that the PADS writer
                // collapses when exporting the same route to ASCII.
                constexpr int MIN_ROUTE_LENGTH = 500;
                int64_t deltaX = static_cast<int64_t>( end.x ) - start.x;
                int64_t deltaY = static_cast<int64_t>( end.y ) - start.y;

                if( deltaX * deltaX + deltaY * deltaY < MIN_ROUTE_LENGTH * MIN_ROUTE_LENGTH )
                    continue;

                if( p2.is_arc )
                {
                    VECTOR2I center = scalePoint( p2.arc.cx, p2.arc.cy );

                    bool clockwise = ( p2.arc.delta_angle < 0 );

                    SHAPE_ARC shapeArc;
                    shapeArc.ConstructFromStartEndCenter( start, end, center, clockwise, track_width );

                    PCB_ARC* arc = new PCB_ARC( m_loadBoard, &shapeArc );

                    if( net )
                        arc->SetNet( net );

                    arc->SetWidth( track_width );
                    arc->SetLayer( track_layer );
                    m_loadBoard->Add( arc );
                }
                else
                {
                    PCB_TRACK* track = new PCB_TRACK( m_loadBoard );

                    if( net )
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
            VECTOR2I pos = scalePoint( via_def.location.x, via_def.location.y );

            std::unique_ptr<PCB_VIA> via = std::make_unique<PCB_VIA>( m_loadBoard );

            if( net )
                via->SetNet( net );

            via->SetPosition( pos );

            const double viaDrill = via_def.stack.front().drill;
            via->Padstack().SetMode( PADSTACK::MODE::FRONT_INNER_BACK );

            for( const PADS_IO::PAD_STACK_LAYER& layer : via_def.stack )
            {
                if( layer.sizeA <= 0 || !PADS_IO::IsCopperPadRow( layer ) )
                    continue;

                int width = scaleSize( layer.sizeA );

                if( layer.layer == -2 )
                    via->SetWidth( F_Cu, width );
                else if( layer.layer == 0 )
                    via->SetWidth( PADSTACK::INNER_LAYERS, width );
                else if( layer.layer == -1 )
                    via->SetWidth( B_Cu, width );
            }

            via->SetDrill( scaleSize( viaDrill ) );
            PCB_LAYER_ID startLayer = getMappedLayer( via_def.start_layer );
            PCB_LAYER_ID endLayer = getMappedLayer( via_def.end_layer );

            // A one-layer file defaults every via to the same span, so a whole board must not be
            // lost to a via the layer table cannot place
            if( !IsCopperLayer( startLayer ) || !IsCopperLayer( endLayer ) || startLayer == endLayer )
            {
                if( m_reporter )
                {
                    m_reporter->Report( wxString::Format( _( "Skipped a via on net '%s' with an unusable "
                                                             "PADS layer span %d,%d." ),
                                                          PADS_COMMON::ConvertText( route.net_name ),
                                                          via_def.start_layer, via_def.end_layer ),
                                        RPT_SEVERITY_WARNING );
                }

                continue;
            }

            // SetLayerPair sanitizes the span against the current via type and PCB_VIA is
            // constructed as THROUGH, so the type has to be set first or the decoded span is
            // overwritten with F_Cu/B_Cu and every blind or buried via becomes full stack
            if( ( startLayer == F_Cu && endLayer == B_Cu )
                || ( startLayer == B_Cu && endLayer == F_Cu ) )
                via->SetViaType( VIATYPE::THROUGH );
            else if( std::abs( via_def.start_layer - via_def.end_layer ) == 1
                     && ( startLayer == F_Cu || startLayer == B_Cu || endLayer == F_Cu || endLayer == B_Cu ) )
                via->SetViaType( VIATYPE::MICROVIA );
            else if( startLayer == F_Cu || startLayer == B_Cu || endLayer == F_Cu || endLayer == B_Cu )
                via->SetViaType( VIATYPE::BLIND );
            else
                via->SetViaType( VIATYPE::BURIED );

            via->SetLayerPair( startLayer, endLayer );

            m_loadBoard->Add( via.release() );
        }
    }
}


void PCB_IO_PADS_BINARY::loadCopperShapes()
{
    const auto& copperShapes = m_parser->GetCopperShapes();

    for( const PADS_IO::COPPER_SHAPE& copper : copperShapes )
    {
        if( !copper.filled || copper.is_cutout || copper.outline.size() < 3 )
            continue;

        PCB_LAYER_ID layer = getMappedLayer( copper.layer );

        if( layer == UNDEFINED_LAYER )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "COPPER item on unmapped layer %d" ), copper.layer ),
                                    RPT_SEVERITY_WARNING );
            }

            continue;
        }

        SHAPE_LINE_CHAIN outline;

        for( const PADS_IO::ARC_POINT& pt : copper.outline )
            outline.Append( scalePoint( pt.x, pt.y ) );

        outline.SetClosed( true );

        if( !IsCopperLayer( layer ) )
        {
            PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard, SHAPE_T::POLY );
            shape->SetLayer( layer );
            shape->SetFilled( true );
            shape->SetPolyShape( SHAPE_POLY_SET( outline ) );
            m_loadBoard->Add( shape );
            continue;
        }

        ZONE* zone = new ZONE( m_loadBoard );
        zone->SetLayer( layer );
        zone->SetIsRuleArea( false );
        zone->Outline()->AddOutline( outline );
        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE, ZONE::GetDefaultHatchPitch(), true );

        m_loadBoard->Add( zone );
    }
}


void PCB_IO_PADS_BINARY::loadZones()
{
    const auto& pours = m_parser->GetPours();
    const auto& params = m_parser->GetParameters();

    int maxPriority = 0;

    for( const auto& pour_def : pours )
    {
        if( pour_def.priority > maxPriority )
            maxPriority = pour_def.priority;
    }

    for( const auto& pour_def : pours )
    {
        if( pour_def.points.size() < 3 )
            continue;

        PCB_LAYER_ID pourLayer = getMappedLayer( pour_def.layer );

        if( pourLayer == UNDEFINED_LAYER )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "Skipping pour on unmapped layer %d" ), pour_def.layer ),
                        RPT_SEVERITY_WARNING );
            }

            continue;
        }

        if( !IsCopperLayer( pourLayer ) )
        {
            SHAPE_LINE_CHAIN outline;

            for( const auto& point : pour_def.points )
                outline.Append( scalePoint( point.x, point.y ) );

            outline.SetClosed( true );

            PCB_SHAPE* shape = new PCB_SHAPE( m_loadBoard, SHAPE_T::POLY );
            shape->SetLayer( pourLayer );
            shape->SetFilled( true );
            shape->SetPolyShape( SHAPE_POLY_SET( outline ) );
            m_loadBoard->Add( shape );
            continue;
        }

        ZONE* zone = new ZONE( m_loadBoard );
        zone->SetLayer( pourLayer );

        zone->Outline()->NewOutline();

        for( const auto& pt : pour_def.points )
        {
            zone->Outline()->Append( scalePoint( pt.x, pt.y ) );
        }

        if( zone->GetNumCorners() == 0 )
        {
            delete zone;
            continue;
        }

        m_converter->ApplyPourSettings( zone, pour_def, maxPriority, params );

        m_loadBoard->Add( zone );
    }
}


std::map<wxString, PCB_LAYER_ID>
PCB_IO_PADS_BINARY::DefaultLayerMappingCallback( const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector )
{
    std::map<wxString, PCB_LAYER_ID> layerMap;

    for( const INPUT_LAYER_DESC& layer : aInputLayerDescriptionVector )
        layerMap[layer.Name] = layer.AutoMapLayer;

    return layerMap;
}


void PCB_IO_PADS_BINARY::clearLoadingState()
{
    m_loadBoard = nullptr;
    m_parser = nullptr;
    m_converter.reset();
    m_pinToNetMap.clear();
    m_partFootprints.clear();
}
