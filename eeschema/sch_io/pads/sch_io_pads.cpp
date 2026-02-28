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

#include <sch_io/pads/sch_io_pads.h>
#include <sch_io/pads/pads_sch_parser.h>
#include <sch_io/pads/pads_sch_symbol_builder.h>
#include <sch_io/pads/pads_sch_schematic_builder.h>

#include <lib_symbol.h>
#include <page_info.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <schematic.h>
#include <schematic_settings.h>
#include <wildcards_and_files_ext.h>

#include <math/util.h>
#include <stroke_params.h>

#include <advanced_config.h>
#include <io/pads/pads_common.h>
#include <locale_io.h>
#include <progress_reporter.h>
#include <reporter.h>

#include <fstream>
#include <map>
#include <set>
#include <wx/log.h>


/**
 * Extract the numeric connector pin suffix from a reference designator.
 * For "J12-15" returns "15", for "J12-1" returns "1".
 * Returns empty string if no numeric suffix is found.
 */
static std::string extractConnectorPinNumber( const std::string& aRef )
{
    size_t sepPos = aRef.rfind( '-' );

    if( sepPos == std::string::npos )
        sepPos = aRef.rfind( '.' );

    if( sepPos != std::string::npos && sepPos + 1 < aRef.size()
        && std::isdigit( static_cast<unsigned char>( aRef[sepPos + 1] ) ) )
    {
        return aRef.substr( sepPos + 1 );
    }

    return "";
}


/**
 * Extract the base reference from a connector reference designator.
 * For "J12-15" returns "J12", for "J12-1" returns "J12".
 * Returns the full reference if no numeric suffix is found.
 */
static std::string extractConnectorBaseRef( const std::string& aRef )
{
    size_t sepPos = aRef.rfind( '-' );

    if( sepPos == std::string::npos )
        sepPos = aRef.rfind( '.' );

    if( sepPos != std::string::npos && sepPos + 1 < aRef.size()
        && std::isdigit( static_cast<unsigned char>( aRef[sepPos + 1] ) ) )
    {
        return aRef.substr( 0, sepPos );
    }

    return aRef;
}


/**
 * Strip any alphabetic gate suffix (e.g. "-A", ".B") from a PADS reference designator,
 * returning the base refdes that matches the PCB footprint naming convention.
 */
static std::string stripGateSuffix( const std::string& aRef )
{
    size_t sepPos = aRef.rfind( '-' );

    if( sepPos == std::string::npos )
        sepPos = aRef.rfind( '.' );

    if( sepPos != std::string::npos && sepPos + 1 < aRef.size()
        && std::isalpha( static_cast<unsigned char>( aRef[sepPos + 1] ) ) )
    {
        return aRef.substr( 0, sepPos );
    }

    return aRef;
}


static SCH_TEXT* createSchText( const PADS_SCH::TEXT_ITEM& aText, const VECTOR2I& aPos )
{
    SCH_TEXT* schText = new SCH_TEXT( aPos, wxString::FromUTF8( aText.content ) );

    if( aText.height > 0 )
    {
        int scaledSize = schIUScale.MilsToIU( aText.height );
        int charHeight = static_cast<int>( scaledSize
                                           * ADVANCED_CFG::GetCfg().m_PadsSchTextHeightScale );
        int charWidth = static_cast<int>( scaledSize
                                          * ADVANCED_CFG::GetCfg().m_PadsSchTextWidthScale );
        schText->SetTextSize( VECTOR2I( charWidth, charHeight ) );
    }

    if( aText.width_factor > 0 )
        schText->SetTextThickness( schIUScale.MilsToIU( aText.width_factor ) );

    // PADS justification: value = vertical_offset + horizontal_code
    // Vertical offsets: bottom=0, top=2, middle=8
    // Horizontal codes: left=0, right=1, center=4
    int justVal = aText.justification;
    int hCode = 0;
    int vGroup = 0;

    if( justVal >= 8 )
    {
        vGroup = 2;  // middle
        hCode = justVal - 8;
    }
    else if( justVal >= 2 )
    {
        vGroup = 1;  // top
        hCode = justVal - 2;
    }
    else
    {
        vGroup = 0;  // bottom
        hCode = justVal;
    }

    switch( hCode )
    {
    default:
    case 0: schText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );   break;
    case 1: schText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );  break;
    case 4: schText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER ); break;
    }

    switch( vGroup )
    {
    default:
    case 0: schText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM ); break;
    case 1: schText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );    break;
    case 2: schText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER ); break;
    }

    if( aText.rotation != 0 )
        schText->SetTextAngleDegrees( aText.rotation * 90.0 );

    return schText;
}


/**
 * Determine the orientation for a power symbol at an OPC position based on
 * the wire direction at that point. All power symbols are drawn with their
 * pin at (0,0). Ground-style symbols have their body below the pin (pin_up=false),
 * while VCC-style symbols have their body above the pin (pin_up=true).
 * We orient the symbol so its body faces away from the wire.
 */
static int computePowerOrientation( const std::string& aOpcId,
                                    const std::vector<PADS_SCH::SCH_SIGNAL>& aSignals,
                                    const VECTOR2I& aOpcPos, bool aPinUp, int aPageHeightIU )
{
    // Find the wire endpoint matching this OPC and get the adjacent vertex
    std::string opcRef = "@@@O" + aOpcId;
    VECTOR2I adjPos = aOpcPos;
    bool found = false;

    for( const auto& signal : aSignals )
    {
        for( const auto& wire : signal.wires )
        {
            if( wire.vertices.size() < 2 )
                continue;

            if( wire.endpoint_a == opcRef )
            {
                adjPos = VECTOR2I(
                        schIUScale.MilsToIU( KiROUND( wire.vertices[1].x ) ),
                        aPageHeightIU
                                - schIUScale.MilsToIU( KiROUND( wire.vertices[1].y ) ) );
                found = true;
                break;
            }

            if( wire.endpoint_b == opcRef )
            {
                size_t last = wire.vertices.size() - 1;
                adjPos = VECTOR2I(
                        schIUScale.MilsToIU( KiROUND( wire.vertices[last - 1].x ) ),
                        aPageHeightIU
                                - schIUScale.MilsToIU(
                                        KiROUND( wire.vertices[last - 1].y ) ) );
                found = true;
                break;
            }
        }

        if( found )
            break;
    }

    if( !found )
        return SYMBOL_ORIENTATION_T::SYM_ORIENT_0;

    // Wire goes from aOpcPos toward adjPos
    int dx = adjPos.x - aOpcPos.x;
    int dy = adjPos.y - aOpcPos.y;

    // Determine which direction the wire approaches from (relative to OPC position).
    // The symbol body should face AWAY from the wire.
    // In KiCad Y-down coordinates: dy > 0 means wire goes down from OPC.

    if( std::abs( dx ) >= std::abs( dy ) )
    {
        // Horizontal wire
        if( dx > 0 )
        {
            // Wire goes right → body should face left
            return aPinUp ? SYMBOL_ORIENTATION_T::SYM_ORIENT_90
                          : SYMBOL_ORIENTATION_T::SYM_ORIENT_270;
        }
        else
        {
            // Wire goes left → body should face right
            return aPinUp ? SYMBOL_ORIENTATION_T::SYM_ORIENT_270
                          : SYMBOL_ORIENTATION_T::SYM_ORIENT_90;
        }
    }
    else
    {
        // Vertical wire
        if( dy > 0 )
        {
            // Wire goes down → body should face up
            return aPinUp ? SYMBOL_ORIENTATION_T::SYM_ORIENT_0
                          : SYMBOL_ORIENTATION_T::SYM_ORIENT_180;
        }
        else
        {
            // Wire goes up → body should face down
            return aPinUp ? SYMBOL_ORIENTATION_T::SYM_ORIENT_180
                          : SYMBOL_ORIENTATION_T::SYM_ORIENT_0;
        }
    }
}


SCH_IO_PADS::SCH_IO_PADS() : SCH_IO( wxS( "PADS Logic" ) )
{
}


SCH_IO_PADS::~SCH_IO_PADS()
{
}


bool SCH_IO_PADS::CanReadSchematicFile( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadSchematicFile( aFileName ) )
        return false;

    return checkFileHeader( aFileName );
}


bool SCH_IO_PADS::CanReadLibrary( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadLibrary( aFileName ) )
        return false;

    return checkFileHeader( aFileName );
}


SCH_SHEET* SCH_IO_PADS::LoadSchematicFile( const wxString&                    aFileName,
                                           SCHEMATIC*                         aSchematic,
                                           SCH_SHEET*                         aAppendToMe,
                                           const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK( !aFileName.IsEmpty() && aSchematic, nullptr );

    LOCALE_IO setlocale;

    SCH_SHEET* rootSheet = nullptr;

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr, "Can't append to a schematic with no root!" );
        rootSheet = aAppendToMe;
    }
    else
    {
        rootSheet = new SCH_SHEET( aSchematic );
        rootSheet->SetFileName( aFileName );
        aSchematic->SetTopLevelSheets( { rootSheet } );
    }

    if( !rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );
        screen->SetFileName( aFileName );
        rootSheet->SetScreen( screen );

        const_cast<KIID&>( rootSheet->m_Uuid ) = screen->GetUuid();
    }

    SCH_SHEET_PATH rootPath;
    rootPath.push_back( rootSheet );

    SCH_SCREEN* rootScreen = rootSheet->GetScreen();
    wxCHECK( rootScreen, nullptr );

    SCH_SHEET_INSTANCE sheetInstance;
    sheetInstance.m_Path = rootPath.Path();
    sheetInstance.m_PageNumber = wxT( "#" );
    rootScreen->m_sheetInstances.emplace_back( sheetInstance );

    if( m_progressReporter )
        m_progressReporter->SetNumPhases( 3 );

    PADS_SCH::PADS_SCH_PARSER parser;
    std::string filename( aFileName.ToUTF8() );

    if( !parser.Parse( filename ) )
    {
        THROW_IO_ERROR( wxString::Format( wxT( "Failed to parse PADS file: %s" ), aFileName ) );
    }

    if( m_progressReporter )
        m_progressReporter->BeginPhase( 1 );

    const PADS_SCH::PARAMETERS& params = parser.GetParameters();
    PADS_SCH::PADS_SCH_SYMBOL_BUILDER symbolBuilder( params );
    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER schBuilder( params, aSchematic );

    // Detect gate suffix separator from multi-gate part references (e.g. U17-A → '-')
    for( const auto& part : parser.GetPartPlacements() )
    {
        const std::string& ref = part.reference;
        size_t dashPos = ref.rfind( '-' );
        size_t dotPos = ref.rfind( '.' );
        size_t sepPos = std::string::npos;

        if( dashPos != std::string::npos )
            sepPos = dashPos;
        else if( dotPos != std::string::npos )
            sepPos = dotPos;

        if( sepPos != std::string::npos && sepPos + 1 < ref.size()
            && std::isalpha( static_cast<unsigned char>( ref[sepPos + 1] ) ) )
        {
            aSchematic->Settings().m_SubpartIdSeparator = static_cast<int>( ref[sepPos] );
            aSchematic->Settings().m_SubpartFirstId = 'A';
            break;
        }
    }

    // Set KiCad page size to match the PADS drawing sheet
    PAGE_INFO pageInfo;

    if( !params.sheet_size.name.empty() )
        pageInfo.SetType( wxString::FromUTF8( params.sheet_size.name ) );
    else
        pageInfo.SetType( PAGE_SIZE_TYPE::A );

    // PADS Y-up to KiCad Y-down: Y_kicad = pageHeight - Y_pads
    const int pageHeightIU = pageInfo.GetHeightIU( schIUScale.IU_PER_MILS );

    // Build LIB_SYMBOL objects from all CAEDECAL definitions
    for( const PADS_SCH::SYMBOL_DEF& symDef : parser.GetSymbolDefs() )
        symbolBuilder.GetOrCreateSymbol( symDef );

    std::set<int> sheetNumbers = parser.GetSheetNumbers();

    if( sheetNumbers.empty() )
        sheetNumbers.insert( 1 );

    bool isSingleSheet = ( sheetNumbers.size() == 1 );

    // Map sheet number -> (SCH_SHEET*, SCH_SCREEN*, SCH_SHEET_PATH)
    struct SheetContext
    {
        SCH_SHEET*      sheet = nullptr;
        SCH_SCREEN*     screen = nullptr;
        SCH_SHEET_PATH  path;
    };

    std::map<int, SheetContext> sheetContexts;

    if( isSingleSheet )
    {
        int sheetNum = *sheetNumbers.begin();
        SheetContext ctx;
        ctx.sheet = rootSheet;
        ctx.screen = rootScreen;
        ctx.path = rootPath;
        ctx.screen->SetPageSettings( pageInfo );
        sheetContexts[sheetNum] = ctx;
    }
    else
    {
        // Multi-sheet: root is a container with sub-sheets
        int totalSheets = static_cast<int>( sheetNumbers.size() );

        for( int sheetNum : sheetNumbers )
        {
            SCH_SHEET* subSheet = schBuilder.CreateHierarchicalSheet(
                    sheetNum, totalSheets, rootSheet, aFileName );

            if( !subSheet )
                continue;

            // Find the sheet name from parser headers
            for( const PADS_SCH::SHEET_HEADER& hdr : parser.GetSheetHeaders() )
            {
                if( hdr.sheet_num == sheetNum && !hdr.sheet_name.empty() )
                {
                    subSheet->GetField( FIELD_T::SHEET_NAME )->SetText(
                            wxString::FromUTF8( hdr.sheet_name ) );

                    break;
                }
            }

            SCH_SHEET_PATH subPath;
            subPath.push_back( rootSheet );
            subPath.push_back( subSheet );

            wxString pageNo = wxString::Format( wxT( "%d" ), sheetNum );
            subPath.SetPageNumber( pageNo );

            SCH_SHEET_INSTANCE subInstance;
            subInstance.m_Path = subPath.Path();
            subInstance.m_PageNumber = pageNo;
            subSheet->GetScreen()->m_sheetInstances.emplace_back( subInstance );

            SheetContext ctx;
            ctx.sheet = subSheet;
            ctx.screen = subSheet->GetScreen();
            ctx.path = subPath;
            ctx.screen->SetPageSettings( pageInfo );
            sheetContexts[sheetNum] = ctx;
        }
    }

    if( m_progressReporter )
        m_progressReporter->BeginPhase( 2 );

    // Track connector base references for wire-endpoint label creation
    std::set<std::string> connectorBaseRefs;

    // Place symbols on each sheet
    for( auto& [sheetNum, ctx] : sheetContexts )
    {
        std::vector<PADS_SCH::PART_PLACEMENT> parts = parser.GetPartsOnSheet( sheetNum );

        for( const PADS_SCH::PART_PLACEMENT& part : parts )
        {
            auto ptIt = parser.GetPartTypes().find( part.part_type );

            LIB_SYMBOL*  libSymbol = nullptr;
            bool         isMultiGate = false;
            bool         isConnector = false;
            bool         isPower = false;
            std::string  libItemName;
            std::string  connectorPinNumber;

            if( ptIt != parser.GetPartTypes().end() )
            {
                const PADS_SCH::PARTTYPE_DEF& ptDef = ptIt->second;

                if( ptDef.gates.size() > 1 )
                {
                    // Multi-gate PARTTYPE: composite multi-unit symbol
                    libSymbol = symbolBuilder.GetOrCreateMultiUnitSymbol(
                            ptDef, parser.GetSymbolDefs() );
                    libItemName = ptDef.name;
                    isMultiGate = true;
                }
                else if( !ptDef.gates.empty() )
                {
                    const PADS_SCH::GATE_DEF& gate = ptDef.gates[0];
                    int idx = std::max( 0, part.gate_index );
                    std::string decalName;

                    if( idx < static_cast<int>( gate.decal_names.size() ) )
                        decalName = gate.decal_names[idx];
                    else if( !gate.decal_names.empty() )
                        decalName = gate.decal_names[0];

                    const PADS_SCH::SYMBOL_DEF* symDef = parser.GetSymbolDef( decalName );

                    connectorPinNumber = ptDef.is_connector
                                                ? extractConnectorPinNumber( part.reference )
                                                : std::string();

                    if( symDef && !connectorPinNumber.empty() )
                    {
                        // Single-pin connector placement (e.g. J12-15).
                        // Each placement gets a symbol variant with the correct pin number.
                        libSymbol = symbolBuilder.GetOrCreateConnectorPinSymbol(
                                ptDef, *symDef, connectorPinNumber );
                        libItemName = decalName + "_pin" + connectorPinNumber;
                        isConnector = true;

                        connectorBaseRefs.insert(
                                extractConnectorBaseRef( part.reference ) );
                    }
                    else if( symDef )
                    {
                        libSymbol = symbolBuilder.GetOrCreatePartTypeSymbol( ptDef, *symDef );
                        libItemName = decalName;
                    }
                }
                else if( !ptDef.special_variants.empty() )
                {
                    // Power/ground symbols
                    int idx = std::max( 0, part.gate_index );
                    idx = std::min( idx, static_cast<int>( ptDef.special_variants.size() ) - 1 );
                    std::string decalName = ptDef.special_variants[idx].decal_name;

                    const PADS_SCH::SYMBOL_DEF* symDef = parser.GetSymbolDef( decalName );

                    if( symDef )
                    {
                        libSymbol = symbolBuilder.GetOrCreateSymbol( *symDef );
                        libItemName = decalName;
                    }
                }

                if( !ptDef.special_keyword.empty() && ptDef.special_keyword != "OFF" )
                    isPower = true;
            }

            // Fallback: resolve directly by CAEDECAL name
            if( !libSymbol )
            {
                const PADS_SCH::SYMBOL_DEF* symDef = parser.GetSymbolDef( part.symbol_name );

                if( !symDef )
                {
                    m_errorMessages.emplace(
                            wxString::Format( wxT( "PADS Import: symbol '%s' not found,"
                                                   " part '%s' skipped" ),
                                              wxString::FromUTF8( part.symbol_name ),
                                              wxString::FromUTF8( part.reference ) ),
                            RPT_SEVERITY_WARNING );
                    continue;
                }

                libSymbol = symbolBuilder.GetOrCreateSymbol( *symDef );
                libItemName = symDef->name;
            }

            if( !libSymbol )
                continue;

            if( ptIt != parser.GetPartTypes().end() && !ptIt->second.sigpins.empty() )
                symbolBuilder.AddHiddenPowerPins( libSymbol, ptIt->second.sigpins );

            if( !isPower )
                isPower = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( part.part_type );

            // Resolve power symbol style. Prefer the PARTTYPE variant decal style
            // (e.g. +BUBBLE → +VDC) which preserves the original PADS symbol shape,
            // falling back to net-name matching (e.g. GND → GND, +5V → +5V).
            std::string powerStyle;

            if( isPower && ptIt != parser.GetPartTypes().end()
                && !ptIt->second.special_variants.empty() )
            {
                int varIdx = std::max( 0, part.gate_index );
                varIdx = std::min(
                        varIdx,
                        static_cast<int>( ptIt->second.special_variants.size() ) - 1 );
                const auto& variant = ptIt->second.special_variants[varIdx];

                powerStyle = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant(
                        variant.decal_name, variant.pin_type );
            }

            if( isPower && powerStyle.empty() )
            {
                std::string rawNetName = part.power_net_name.empty()
                                                 ? part.symbol_name
                                                 : part.power_net_name;

                auto powerLibId =
                        PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( rawNetName );

                if( powerLibId )
                    powerStyle = std::string( powerLibId->GetLibItemName().c_str() );
            }

            auto symbolPtr = std::make_unique<SCH_SYMBOL>();
            SCH_SYMBOL* symbol = symbolPtr.get();
            LIB_SYMBOL* instanceSymbol = nullptr;

            if( isPower && !powerStyle.empty() )
            {
                instanceSymbol = symbolBuilder.BuildKiCadPowerSymbol( powerStyle );

                LIB_ID libId;
                libId.SetLibNickname( wxT( "power" ) );
                libId.SetLibItemName( wxString::FromUTF8( powerStyle ) );
                symbol->SetLibId( libId );
            }
            else
            {
                LIB_ID libId;
                libId.SetLibNickname( wxT( "pads_import" ) );
                libId.SetLibItemName( wxString::FromUTF8( libItemName ) );
                symbol->SetLibId( libId );

                instanceSymbol = new LIB_SYMBOL( *libSymbol );

                if( isPower )
                    instanceSymbol->SetGlobalPower();
            }

            symbol->SetLibSymbol( instanceSymbol );
            symbol->SetPosition( VECTOR2I(
                    schIUScale.MilsToIU( KiROUND( part.position.x ) ),
                    pageHeightIU - schIUScale.MilsToIU( KiROUND( part.position.y ) ) ) );

            int orientation = SYMBOL_ORIENTATION_T::SYM_ORIENT_0;

            if( part.rotation == 90.0 )
                orientation = SYMBOL_ORIENTATION_T::SYM_ORIENT_90;
            else if( part.rotation == 180.0 )
                orientation = SYMBOL_ORIENTATION_T::SYM_ORIENT_180;
            else if( part.rotation == 270.0 )
                orientation = SYMBOL_ORIENTATION_T::SYM_ORIENT_270;

            if( part.mirror_flags & 1 )
                orientation |= SYMBOL_ORIENTATION_T::SYM_MIRROR_Y;

            if( part.mirror_flags & 2 )
                orientation |= SYMBOL_ORIENTATION_T::SYM_MIRROR_X;

            symbol->SetOrientation( orientation );

            if( isMultiGate )
                symbol->SetUnit( part.gate_index + 1 );
            else
                symbol->SetUnit( 1 );

            // Assign deterministic UUID so PCB cross-probe can match footprints
            // to symbols. Only the primary gate (index 0) gets the deterministic
            // UUID since one footprint maps to one symbol instance.
            if( !isPower && ( !isMultiGate || part.gate_index == 0 ) )
            {
                std::string baseRef = stripGateSuffix( part.reference );

                const_cast<KIID&>( symbol->m_Uuid ) =
                        PADS_COMMON::GenerateDeterministicUuid( baseRef );
            }

            symbol->SetRef( &ctx.path, wxString::FromUTF8( part.reference ) );

            schBuilder.ApplyPartAttributes( symbol, part );
            schBuilder.CreateCustomFields( symbol, part );

            // For passive components, override Value with VALUE1 parametric value
            // so that e.g. C10 shows "0.1uF" instead of the generic "CAPMF0805".
            // Also apply the VALUE1 attribute position.
            if( ptIt != parser.GetPartTypes().end() )
            {
                const std::string& cat = ptIt->second.category;

                if( cat == "CAP" || cat == "RES" || cat == "IND" )
                {
                    auto valIt = part.attr_overrides.find( "VALUE" );

                    if( valIt == part.attr_overrides.end() )
                        valIt = part.attr_overrides.find( "VALUE1" );

                    if( valIt != part.attr_overrides.end() && !valIt->second.empty() )
                    {
                        symbol->SetValueFieldText( wxString::FromUTF8( valIt->second ) );

                        for( const auto& attr : part.attributes )
                        {
                            if( attr.name == "VALUE" || attr.name == "VALUE1"
                                || attr.name == "Value1" )
                            {
                                SCH_FIELD* valField = symbol->GetField( FIELD_T::VALUE );
                                int fx = schIUScale.MilsToIU( KiROUND( attr.position.x ) );

                                if( part.mirror_flags & 1 )
                                    fx = -fx;

                                VECTOR2I fieldPos( fx,
                                                   -schIUScale.MilsToIU(
                                                           KiROUND( attr.position.y ) ) );
                                valField->SetPosition( symbol->GetPosition() + fieldPos );

                                int fieldTextSize = schIUScale.MilsToIU( 50 );
                                valField->SetTextSize(
                                        VECTOR2I( fieldTextSize, fieldTextSize ) );
                                valField->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
                                valField->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
                                break;
                            }
                        }
                    }
                }
            }

            if( isPower )
            {
                symbol->GetField( FIELD_T::REFERENCE )->SetVisible( false );

                wxString netName = part.power_net_name.empty()
                                           ? wxString::FromUTF8( part.symbol_name )
                                           : wxString::FromUTF8( part.power_net_name );

                if( netName.StartsWith( wxT( "/" ) ) )
                    netName = wxT( "~{" ) + netName.Mid( 1 ) + wxT( "}" );

                symbol->GetField( FIELD_T::VALUE )->SetText( netName );
                symbol->GetField( FIELD_T::VALUE )->SetVisible( true );
            }

            symbol->AddHierarchicalReference( ctx.path.Path(),
                                              wxString::FromUTF8( part.reference ),
                                              symbol->GetUnit() );

            symbol->ClearFlags();

            // For connector pins, create a local label at the pin position
            // before transferring ownership to the screen.
            // The matching label at the wire endpoint creates the electrical connection.
            if( isConnector && !connectorPinNumber.empty() )
            {
                std::string baseRef = extractConnectorBaseRef( part.reference );
                wxString labelText = wxString::Format( wxT( "%s.%s" ),
                        wxString::FromUTF8( baseRef ),
                        wxString::FromUTF8( connectorPinNumber ) );

                VECTOR2I pinPos = symbol->GetPosition();
                std::vector<SCH_PIN*> pins = symbol->GetPins();

                if( !pins.empty() )
                    pinPos = pins[0]->GetPosition();

                SCH_LABEL* label = new SCH_LABEL( pinPos, labelText );
                int labelSize = schIUScale.MilsToIU( 50 );
                label->SetTextSize( VECTOR2I( labelSize, labelSize ) );
                label->SetSpinStyle( SPIN_STYLE::RIGHT );
                label->SetFlags( IS_NEW );
                ctx.screen->Append( label );
            }

            ctx.screen->Append( symbolPtr.release() );
        }
    }

    // Build set of power signal names so we can suppress duplicate global labels
    // where a power symbol is placed instead.  Non-power signal labels are handled
    // by CreateNetLabels which places them at dangling wire endpoints.
    std::set<std::string> powerSignalNames;

    for( const PADS_SCH::OFF_PAGE_CONNECTOR& opc : parser.GetOffPageConnectors() )
    {
        if( opc.signal_name.empty() )
            continue;

        auto ptIt = parser.GetPartTypes().find( opc.symbol_lib );

        if( ptIt != parser.GetPartTypes().end()
            && !ptIt->second.special_keyword.empty() && ptIt->second.special_keyword != "OFF"
            && !ptIt->second.special_variants.empty() )
        {
            int idx = std::max( 0, opc.flags2 );
            idx = std::min( idx,
                            static_cast<int>( ptIt->second.special_variants.size() ) - 1 );
            const auto& variant = ptIt->second.special_variants[idx];

            if( !PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant(
                        variant.decal_name, variant.pin_type ).empty() )
            {
                powerSignalNames.insert( opc.signal_name );
            }
        }
    }

    // Build set of OPC reference IDs for non-power signal OPCs. Each entry
    // corresponds to a wire endpoint reference like "@@@O48" that should receive
    // its own global label with orientation derived from the wire direction.
    std::set<std::string> signalOpcIds;

    for( const PADS_SCH::OFF_PAGE_CONNECTOR& opc : parser.GetOffPageConnectors() )
    {
        if( opc.signal_name.empty() || powerSignalNames.count( opc.signal_name ) )
            continue;

        signalOpcIds.insert( "@@@O" + std::to_string( opc.id ) );
    }

    // Create wires and connectivity on each sheet
    for( auto& [sheetNum, ctx] : sheetContexts )
    {
        std::vector<PADS_SCH::SCH_SIGNAL> sheetSignals = parser.GetSignalsOnSheet( sheetNum );

        // Create wire segments from vertex data
        for( const PADS_SCH::SCH_SIGNAL& signal : sheetSignals )
        {
            for( const PADS_SCH::WIRE_SEGMENT& wire : signal.wires )
            {
                if( wire.vertices.size() < 2 )
                    continue;

                // Each consecutive pair of vertices becomes a wire segment
                for( size_t v = 0; v + 1 < wire.vertices.size(); v++ )
                {
                    VECTOR2I start(
                            schIUScale.MilsToIU( KiROUND( wire.vertices[v].x ) ),
                            pageHeightIU
                                    - schIUScale.MilsToIU( KiROUND( wire.vertices[v].y ) ) );
                    VECTOR2I end(
                            schIUScale.MilsToIU( KiROUND( wire.vertices[v + 1].x ) ),
                            pageHeightIU
                                    - schIUScale.MilsToIU( KiROUND( wire.vertices[v + 1].y ) ) );

                    if( start == end )
                        continue;

                    SCH_LINE* line = new SCH_LINE( start, SCH_LAYER_ID::LAYER_WIRE );
                    line->SetEndPoint( end );
                    line->SetConnectivityDirty();
                    ctx.screen->Append( line );
                }
            }
        }

        // Create local labels at wire endpoints that reference connector pins
        for( const PADS_SCH::SCH_SIGNAL& signal : sheetSignals )
        {
            for( const PADS_SCH::WIRE_SEGMENT& wire : signal.wires )
            {
                if( wire.vertices.size() < 2 )
                    continue;

                // Check endpoint_a for connector pin reference
                if( wire.endpoint_a.find( '.' ) != std::string::npos
                    && wire.endpoint_a.find( "@@@" ) == std::string::npos )
                {
                    size_t dotPos = wire.endpoint_a.find( '.' );
                    std::string ref = wire.endpoint_a.substr( 0, dotPos );

                    if( connectorBaseRefs.count( ref ) )
                    {
                        const auto& vtx = wire.vertices.front();
                        VECTOR2I pos(
                                schIUScale.MilsToIU( KiROUND( vtx.x ) ),
                                pageHeightIU - schIUScale.MilsToIU( KiROUND( vtx.y ) ) );

                        // Compute label orientation from adjacent vertex
                        const auto& adj = wire.vertices[1];
                        VECTOR2I adjPos(
                                schIUScale.MilsToIU( KiROUND( adj.x ) ),
                                pageHeightIU - schIUScale.MilsToIU( KiROUND( adj.y ) ) );

                        int dx = adjPos.x - pos.x;
                        int dy = adjPos.y - pos.y;
                        SPIN_STYLE orient = SPIN_STYLE::RIGHT;

                        if( std::abs( dx ) >= std::abs( dy ) )
                            orient = ( dx > 0 ) ? SPIN_STYLE::LEFT : SPIN_STYLE::RIGHT;
                        else
                            orient = ( dy > 0 ) ? SPIN_STYLE::UP : SPIN_STYLE::BOTTOM;

                        wxString labelText = wxString::FromUTF8( wire.endpoint_a );
                        SCH_LABEL* label = new SCH_LABEL( pos, labelText );
                        int labelSize = schIUScale.MilsToIU( 50 );
                        label->SetTextSize( VECTOR2I( labelSize, labelSize ) );
                        label->SetSpinStyle( orient );
                        label->SetFlags( IS_NEW );
                        ctx.screen->Append( label );
                    }
                }

                // Check endpoint_b for connector pin reference
                if( wire.endpoint_b.find( '.' ) != std::string::npos
                    && wire.endpoint_b.find( "@@@" ) == std::string::npos )
                {
                    size_t dotPos = wire.endpoint_b.find( '.' );
                    std::string ref = wire.endpoint_b.substr( 0, dotPos );

                    if( connectorBaseRefs.count( ref ) )
                    {
                        const auto& vtx = wire.vertices.back();
                        VECTOR2I pos(
                                schIUScale.MilsToIU( KiROUND( vtx.x ) ),
                                pageHeightIU - schIUScale.MilsToIU( KiROUND( vtx.y ) ) );

                        // Compute label orientation from adjacent vertex
                        size_t lastIdx = wire.vertices.size() - 1;
                        const auto& adj = wire.vertices[lastIdx - 1];
                        VECTOR2I adjPos(
                                schIUScale.MilsToIU( KiROUND( adj.x ) ),
                                pageHeightIU - schIUScale.MilsToIU( KiROUND( adj.y ) ) );

                        int dx = adjPos.x - pos.x;
                        int dy = adjPos.y - pos.y;
                        SPIN_STYLE orient = SPIN_STYLE::RIGHT;

                        if( std::abs( dx ) >= std::abs( dy ) )
                            orient = ( dx > 0 ) ? SPIN_STYLE::LEFT : SPIN_STYLE::RIGHT;
                        else
                            orient = ( dy > 0 ) ? SPIN_STYLE::UP : SPIN_STYLE::BOTTOM;

                        wxString labelText = wxString::FromUTF8( wire.endpoint_b );
                        SCH_LABEL* label = new SCH_LABEL( pos, labelText );
                        int labelSize = schIUScale.MilsToIU( 50 );
                        label->SetTextSize( VECTOR2I( labelSize, labelSize ) );
                        label->SetSpinStyle( orient );
                        label->SetFlags( IS_NEW );
                        ctx.screen->Append( label );
                    }
                }
            }
        }

        // Create junctions from TIEDOTS for this sheet
        for( const PADS_SCH::TIED_DOT& dot : parser.GetTiedDots() )
        {
            if( dot.sheet_number != sheetNum )
                continue;

            VECTOR2I pos( schIUScale.MilsToIU( KiROUND( dot.position.x ) ),
                          pageHeightIU - schIUScale.MilsToIU( KiROUND( dot.position.y ) ) );

            SCH_JUNCTION* junction = new SCH_JUNCTION( pos );
            ctx.screen->Append( junction );
        }

        // Create net labels, skipping power nets that get dedicated symbols
        schBuilder.CreateNetLabels( sheetSignals, ctx.screen, signalOpcIds,
                                    powerSignalNames );

        // Place off-page connectors: power/ground types become SCH_SYMBOL with
        // KiCad standard power graphics; signal types become SCH_GLOBALLABEL.
        int pwrIndex = 1;

        for( const PADS_SCH::OFF_PAGE_CONNECTOR& opc : parser.GetOffPageConnectors() )
        {
            if( opc.source_sheet != sheetNum )
                continue;

            if( opc.signal_name.empty() )
                continue;

            VECTOR2I pos( schIUScale.MilsToIU( KiROUND( opc.position.x ) ),
                          pageHeightIU
                                  - schIUScale.MilsToIU( KiROUND( opc.position.y ) ) );

            // Resolve power style from the PARTTYPE variant definition
            std::string powerStyle;
            auto opcPtIt = parser.GetPartTypes().find( opc.symbol_lib );

            if( opcPtIt != parser.GetPartTypes().end()
                && !opcPtIt->second.special_keyword.empty() && opcPtIt->second.special_keyword != "OFF"
                && !opcPtIt->second.special_variants.empty() )
            {
                int idx = std::max( 0, opc.flags2 );
                idx = std::min( idx,
                                static_cast<int>(
                                        opcPtIt->second.special_variants.size() ) - 1 );
                const auto& variant = opcPtIt->second.special_variants[idx];

                powerStyle = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant(
                        variant.decal_name, variant.pin_type );
            }

            if( !powerStyle.empty() )
            {
                LIB_SYMBOL* pwrSym = symbolBuilder.BuildKiCadPowerSymbol( powerStyle );

                if( pwrSym )
                {
                    auto symbolPtr = std::make_unique<SCH_SYMBOL>();
                    SCH_SYMBOL* symbol = symbolPtr.get();

                    LIB_ID libId;
                    libId.SetLibNickname( wxT( "power" ) );
                    libId.SetLibItemName( wxString::FromUTF8( powerStyle ) );
                    symbol->SetLibId( libId );
                    symbol->SetLibSymbol( pwrSym );
                    symbol->SetPosition( pos );
                    symbol->SetUnit( 1 );

                    // VCC and PWR_TRIANGLE have pin pointing up (body above pin).
                    // All others (GND, GNDD, PWR_BAR, VEE, Earth) have pin pointing down.
                    bool pinUp = ( powerStyle == "VCC" || powerStyle == "PWR_TRIANGLE" );
                    int orient = computePowerOrientation(
                            std::to_string( opc.id ), sheetSignals, pos, pinUp,
                            pageHeightIU );

                    symbol->SetOrientation( orient );

                    wxString netName = wxString::FromUTF8( opc.signal_name );

                    if( netName.StartsWith( wxT( "/" ) ) )
                        netName = wxT( "~{" ) + netName.Mid( 1 ) + wxT( "}" );

                    symbol->GetField( FIELD_T::VALUE )->SetText( netName );
                    symbol->GetField( FIELD_T::VALUE )->SetVisible( true );

                    wxString pwrRef = wxString::Format( wxT( "#PWR%03d" ), pwrIndex++ );
                    symbol->SetRef( &ctx.path, pwrRef );
                    symbol->GetField( FIELD_T::REFERENCE )->SetVisible( false );

                    symbol->ClearFlags();
                    ctx.screen->Append( symbolPtr.release() );
                    continue;
                }
            }

            // Non-power signal OPCs don't create labels here.  CreateNetLabels
            // handles all signal net labels, placing them at dangling wire endpoints
            // rather than at OPC positions (which may not land on a wire).
        }
    }

    // Place free text items from *TEXT* section on the first (or only) sheet
    if( !sheetContexts.empty() )
    {
        SCH_SCREEN* textScreen = sheetContexts.begin()->second.screen;

        for( const PADS_SCH::TEXT_ITEM& textItem : parser.GetTextItems() )
        {
            if( textItem.content.empty() )
                continue;

            VECTOR2I pos( schIUScale.MilsToIU( KiROUND( textItem.position.x ) ),
                          pageHeightIU
                                  - schIUScale.MilsToIU( KiROUND( textItem.position.y ) ) );

            textScreen->Append( createSchText( textItem, pos ) );
        }
    }

    // Place graphic lines from *LINES* section (skip the border template)
    if( !sheetContexts.empty() )
    {
        SCH_SCREEN* linesScreen = sheetContexts.begin()->second.screen;

        for( const PADS_SCH::LINES_ITEM& linesItem : parser.GetLinesItems() )
        {
            if( linesItem.name == params.border_template )
                continue;

            double ox = linesItem.origin.x;
            double oy = linesItem.origin.y;

            for( const PADS_SCH::SYMBOL_GRAPHIC& prim : linesItem.primitives )
            {
                int strokeWidth = prim.line_width > 0.0
                                          ? schIUScale.MilsToIU( KiROUND( prim.line_width ) )
                                          : 0;

                LINE_STYLE lineStyle = PADS_COMMON::PadsLineStyleToKiCad( prim.line_style );

                if( prim.type == PADS_SCH::GRAPHIC_TYPE::CIRCLE )
                {
                    VECTOR2I center(
                            schIUScale.MilsToIU( KiROUND( ox + prim.center.x ) ),
                            pageHeightIU
                                    - schIUScale.MilsToIU( KiROUND( oy + prim.center.y ) ) );
                    int radius = schIUScale.MilsToIU( KiROUND( prim.radius ) );

                    SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE );
                    circle->SetStart( center );
                    circle->SetEnd( VECTOR2I( center.x + radius, center.y ) );
                    circle->SetStroke( STROKE_PARAMS( strokeWidth, lineStyle ) );

                    if( prim.filled )
                        circle->SetFillMode( FILL_T::FILLED_SHAPE );

                    linesScreen->Append( circle );
                }
                else if( prim.type == PADS_SCH::GRAPHIC_TYPE::RECTANGLE
                         && prim.points.size() == 2 )
                {
                    VECTOR2I pos(
                            schIUScale.MilsToIU( KiROUND( ox + prim.points[0].coord.x ) ),
                            pageHeightIU
                                    - schIUScale.MilsToIU(
                                            KiROUND( oy + prim.points[0].coord.y ) ) );
                    VECTOR2I end(
                            schIUScale.MilsToIU( KiROUND( ox + prim.points[1].coord.x ) ),
                            pageHeightIU
                                    - schIUScale.MilsToIU(
                                            KiROUND( oy + prim.points[1].coord.y ) ) );

                    SCH_SHAPE* rect = new SCH_SHAPE( SHAPE_T::RECTANGLE );
                    rect->SetPosition( pos );
                    rect->SetEnd( end );
                    rect->SetStroke( STROKE_PARAMS( strokeWidth, lineStyle ) );

                    if( prim.filled )
                        rect->SetFillMode( FILL_T::FILLED_SHAPE );

                    linesScreen->Append( rect );
                }
                else if( prim.points.size() >= 2 )
                {
                    for( size_t p = 0; p + 1 < prim.points.size(); p++ )
                    {
                        VECTOR2I start(
                                schIUScale.MilsToIU(
                                        KiROUND( ox + prim.points[p].coord.x ) ),
                                pageHeightIU
                                        - schIUScale.MilsToIU(
                                                KiROUND( oy + prim.points[p].coord.y ) ) );
                        VECTOR2I end(
                                schIUScale.MilsToIU(
                                        KiROUND( ox + prim.points[p + 1].coord.x ) ),
                                pageHeightIU
                                        - schIUScale.MilsToIU(
                                                KiROUND( oy + prim.points[p + 1].coord.y ) ) );

                        if( start == end )
                            continue;

                        if( prim.points[p].arc.has_value() )
                        {
                            const PADS_SCH::ARC_DATA& ad = *prim.points[p].arc;
                            double cx = ( ad.bbox_x1 + ad.bbox_x2 ) / 2.0;
                            double cy = ( ad.bbox_y1 + ad.bbox_y2 ) / 2.0;
                            VECTOR2I center(
                                    schIUScale.MilsToIU( KiROUND( ox + cx ) ),
                                    pageHeightIU
                                            - schIUScale.MilsToIU( KiROUND( oy + cy ) ) );

                            double sx = start.x - center.x;
                            double sy = start.y - center.y;
                            double ex = end.x - center.x;
                            double ey = end.y - center.y;
                            double radius = std::sqrt( sx * sx + sy * sy );

                            double mx = sx + ex;
                            double my = sy + ey;
                            double mlen = std::sqrt( mx * mx + my * my );

                            VECTOR2I midPt;

                            if( mlen > 0.001 )
                            {
                                midPt.x = center.x
                                           + static_cast<int>( radius * mx / mlen );
                                midPt.y = center.y
                                           + static_cast<int>( radius * my / mlen );
                            }
                            else
                            {
                                midPt.x = center.x
                                           + static_cast<int>( -sy * radius
                                                               / std::max( radius, 1.0 ) );
                                midPt.y = center.y
                                           + static_cast<int>( sx * radius
                                                               / std::max( radius, 1.0 ) );
                            }

                            if( ad.angle < 0 )
                            {
                                midPt.x = 2 * center.x - midPt.x;
                                midPt.y = 2 * center.y - midPt.y;
                            }

                            SCH_SHAPE* arc = new SCH_SHAPE( SHAPE_T::ARC );
                            arc->SetArcGeometry( start, midPt, end );
                            arc->SetStroke( STROKE_PARAMS( strokeWidth, lineStyle ) );

                            if( prim.filled )
                                arc->SetFillMode( FILL_T::FILLED_SHAPE );

                            linesScreen->Append( arc );
                        }
                        else
                        {
                            SCH_LINE* line = new SCH_LINE(
                                    start, SCH_LAYER_ID::LAYER_NOTES );
                            line->SetEndPoint( end );
                            line->SetStroke( STROKE_PARAMS( strokeWidth, lineStyle ) );
                            linesScreen->Append( line );
                        }
                    }
                }
            }

            // Render text items within this LINES group
            for( const PADS_SCH::TEXT_ITEM& textItem : linesItem.texts )
            {
                if( textItem.content.empty() )
                    continue;

                VECTOR2I pos(
                        schIUScale.MilsToIU( KiROUND( ox + textItem.position.x ) ),
                        pageHeightIU
                                - schIUScale.MilsToIU( KiROUND( oy + textItem.position.y ) ) );

                linesScreen->Append( createSchText( textItem, pos ) );
            }
        }
    }

    // Set title block from parsed parameters
    schBuilder.CreateTitleBlock( rootScreen );

    // Finalize all sheets
    SCH_SCREENS allSheets( rootSheet );
    allSheets.UpdateSymbolLinks();
    allSheets.ClearEditFlags();

    if( m_reporter )
    {
        for( const auto& [msg, severity] : m_errorMessages )
            m_reporter->Report( msg, severity );
    }

    m_errorMessages.clear();

    return rootSheet;
}


bool SCH_IO_PADS::checkFileHeader( const wxString& aFileName ) const
{
    try
    {
        std::ifstream file( aFileName.fn_str() );

        if( !file.is_open() )
            return false;

        std::string line;

        if( std::getline( file, line ) )
        {
            if( line.find( "*PADS-POWERLOGIC" ) != std::string::npos )
                return true;

            if( line.find( "*PADS-LOGIC" ) != std::string::npos )
                return true;
        }
    }
    catch( ... )
    {
    }

    return false;
}
