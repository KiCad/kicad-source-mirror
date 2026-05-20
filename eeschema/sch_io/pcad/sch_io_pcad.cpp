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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_io/pcad/sch_io_pcad.h>
#include <sch_io/pcad/pcad_sch_parser.h>

#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <lib_symbol.h>
#include <sch_pin.h>
#include <sch_shape.h>
#include <sch_field.h>
#include <stroke_params.h>
#include <pin_type.h>
#include <layer_ids.h>
#include <wildcards_and_files_ext.h>
#include <string_utils.h>
#include <sch_io/sch_io_mgr.h>
#include <page_info.h>
#include <lib_id.h>
#include <kiid.h>

#include <wx/filename.h>

#include <cmath>
#include <memory>


static const wxString PCAD_IMPORT_LIB = wxT( "pcad_import" );


namespace
{

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

static int toIU( double aMils )
{
    return schIUScale.MilsToIU( aMils );
}

static int schX( double aPcadX )
{
    return toIU( aPcadX );
}

// P-Cad uses Y-up; KiCad uses Y-down.  Flip relative to page height.
static int schY( double aPcadY, double aPageHeightMils )
{
    return toIU( aPageHeightMils - aPcadY );
}

// For symbol-local coordinates, just negate Y.
static int symY( double aPcadY )
{
    return -toIU( aPcadY );
}

// ---------------------------------------------------------------------------
// Map P-Cad pin rotation to KiCad PIN_ORIENTATION.
// P-Cad rotation is the direction the pin line exits the body (CCW, Y-up).
// After Y-flip the mapping is identical to Eagle.
// ---------------------------------------------------------------------------

static PIN_ORIENTATION pinOrientation( double aRotDeg )
{
    int rot = static_cast<int>( std::round( aRotDeg ) ) % 360;

    if( rot < 0 )
        rot += 360;

    // In KiCad PIN_X means the pin LINE goes in direction X from connection toward body.
    // P-Cad rotation is the direction the pin exits the body toward the connection.
    // So P-Cad 0° (exits right) → connection right, body left → line goes LEFT → PIN_LEFT.
    // After Y-flip: P-Cad 90° (exits up) → connection top → PIN_DOWN, 270° → PIN_UP.
    switch( rot )
    {
    case 90:  return PIN_ORIENTATION::PIN_DOWN;
    case 180: return PIN_ORIENTATION::PIN_RIGHT;
    case 270: return PIN_ORIENTATION::PIN_UP;
    default:  return PIN_ORIENTATION::PIN_LEFT;   // 0°
    }
}


// ---------------------------------------------------------------------------
// Build a LIB_SYMBOL from a PCAD_SCH::SYMBOL_DEF.
// Caller owns the returned pointer.
// ---------------------------------------------------------------------------

static LIB_SYMBOL* buildLibSymbol( const PCAD_SCH::SYMBOL_DEF& aDef )
{
    auto* sym = new LIB_SYMBOL( aDef.name );

    LIB_ID libId( PCAD_IMPORT_LIB, aDef.name );
    sym->SetLibId( libId );

    const int lineWidth = toIU( 10.0 );

    // Graphical lines
    for( const PCAD_SCH::LINE& ln : aDef.lines )
    {
        auto* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        shape->AddPoint( VECTOR2I( toIU( ln.x1 ), symY( ln.y1 ) ) );
        shape->AddPoint( VECTOR2I( toIU( ln.x2 ), symY( ln.y2 ) ) );
        shape->SetFillMode( FILL_T::NO_FILL );
        shape->SetStroke( STROKE_PARAMS( lineWidth, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( shape );
    }

    // Arcs  (centre/radius/startAngle/sweepAngle)
    for( const PCAD_SCH::ARC& arc : aDef.arcs )
    {
        auto* shape = new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE );
        int   radius  = toIU( arc.radius );
        VECTOR2I center( toIU( arc.x ), symY( arc.y ) );
        // Negate start angle and sweep to account for Y-flip
        double startRad = -arc.startAngle * M_PI / 180.0;
        VECTOR2I start( center.x + (int)( radius * std::cos( startRad ) ),
                        center.y + (int)( radius * std::sin( startRad ) ) );
        shape->SetCenter( center );
        shape->SetStart( start );
        shape->SetArcAngleAndEnd( EDA_ANGLE( -arc.sweepAngle, DEGREES_T ) );
        shape->SetFillMode( FILL_T::NO_FILL );
        shape->SetStroke( STROKE_PARAMS( lineWidth, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( shape );
    }

    // Pins
    // In P-Cad, pin.x/y is the BODY ATTACHMENT.  The wire connection point is
    // body_attachment + direction * pinLength.  We store that as the KiCad pin
    // position so wires connect correctly.  KiCad pin length = 0 because the
    // P-Cad body lines already draw the visual stub.
    const int pinTextSize = toIU( 50.0 );

    for( const PCAD_SCH::PIN& pin : aDef.pins )
    {
        // Compute external connection tip: body_attachment + direction * pinLength
        int   rot = static_cast<int>( std::round( pin.rotation ) ) % 360;
        if( rot < 0 )
            rot += 360;

        double extX = pin.x, extY = pin.y;
        switch( rot )
        {
        case 0:   extX += pin.pinLength; break;
        case 90:  extY += pin.pinLength; break;   // +Y = up in P-Cad Y-up
        case 180: extX -= pin.pinLength; break;
        case 270: extY -= pin.pinLength; break;
        default:
            extX += pin.pinLength * std::cos( rot * M_PI / 180.0 );
            extY += pin.pinLength * std::sin( rot * M_PI / 180.0 );
            break;
        }

        auto* schPin = new SCH_PIN( sym );
        schPin->SetNumber( pin.number );
        schPin->SetName( pin.name );
        schPin->SetPosition( VECTOR2I( toIU( extX ), symY( extY ) ) );
        schPin->SetLength( toIU( pin.pinLength ) );
        schPin->SetOrientation( pinOrientation( pin.rotation ) );
        schPin->SetType( ELECTRICAL_PINTYPE::PT_UNSPECIFIED );
        schPin->SetNumberTextSize( pinTextSize );
        schPin->SetNameTextSize( pinTextSize );
        sym->AddDrawItem( schPin );
    }

    // Place reference and value fields from (attr "RefDes" ...) and (attr "Type" ...)
    for( const PCAD_SCH::ATTR& attr : aDef.attrs )
    {
        SCH_FIELD* field = nullptr;

        if( attr.name == wxT( "RefDes" ) )
            field = &sym->GetReferenceField();
        else if( attr.name == wxT( "Type" ) )
            field = &sym->GetValueField();

        if( field )
        {
            field->SetPosition( VECTOR2I( toIU( attr.x ), symY( attr.y ) ) );
            field->SetVisible( attr.isVisible );

            // Apply horizontal and vertical justification
            using J = PCAD_SCH::JUSTIFY;
            switch( attr.justify )
            {
            case J::LOWER_LEFT:  case J::LEFT:  case J::UPPER_LEFT:
                field->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );   break;
            case J::LOWER_RIGHT: case J::RIGHT: case J::UPPER_RIGHT:
                field->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );  break;
            default:
                field->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER ); break;
            }
            switch( attr.justify )
            {
            case J::LOWER_LEFT:  case J::LOWER_CENTER:  case J::LOWER_RIGHT:
                field->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );  break;
            case J::UPPER_LEFT:  case J::UPPER_CENTER:  case J::UPPER_RIGHT:
                field->SetVertJustify( GR_TEXT_V_ALIGN_TOP );     break;
            default:
                field->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );  break;
            }
        }
    }

    sym->SetShowPinNumbers( false );
    sym->SetShowPinNames( false );

    return sym;
}


// ---------------------------------------------------------------------------
// Map P-Cad CCW rotation (Y-up) to KiCad SYMBOL_ORIENTATION_T (same as Eagle)
// ---------------------------------------------------------------------------

static SYMBOL_ORIENTATION_T symOrientation( double aRotDeg )
{
    int rot = static_cast<int>( std::round( aRotDeg ) ) % 360;

    if( rot < 0 )
        rot += 360;

    // P-Cad uses clockwise rotation convention; KiCad SYM_ORIENT matches directly.
    switch( rot )
    {
    case 90:  return SYM_ORIENT_90;
    case 180: return SYM_ORIENT_180;
    case 270: return SYM_ORIENT_270;
    default:  return SYM_ORIENT_0;
    }
}

} // anonymous namespace


// ---------------------------------------------------------------------------
// SCH_IO_PCAD
// ---------------------------------------------------------------------------

SCH_IO_PCAD::SCH_IO_PCAD() : SCH_IO( wxS( "P-CAD" ) )
{
}


SCH_IO_PCAD::~SCH_IO_PCAD()
{
}


bool SCH_IO_PCAD::CanReadSchematicFile( const wxString& aFileName ) const
{
    wxFileName fn( aFileName );

    if( fn.GetExt().Upper() != wxT( "SCH" ) )
        return false;

    FILE* fp = wxFopen( aFileName, wxT( "rt" ) );

    if( !fp )
        return false;

    char line[16];
    bool ok = ( fgets( line, sizeof( line ), fp ) != nullptr )
              && ( memcmp( line, "ACCEL_ASCII", 11 ) == 0 );
    fclose( fp );
    return ok;
}


SCH_SHEET* SCH_IO_PCAD::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                           SCH_SHEET*             aAppendToMe,
                                           const std::map<std::string, UTF8>* aProperties )
{
    wxASSERT( !aFileName.IsEmpty() );

    // --- Parse the P-Cad file -----------------------------------------------

    PCAD_SCH::SCHEMATIC pcad;
    PCAD_SCH::PCAD_SCH_PARSER parser;
    parser.LoadFromFile( aFileName, pcad );

    // --- Set up sheet / screen ----------------------------------------------
    // We must NOT use aSchematic->Root() here — that returns the virtual root
    // (nil UUID) which SetTopLevelSheets() will skip. Create a real sheet instead.

    SCH_SHEET* rootSheet;
    SCH_SCREEN* screen;

    if( aAppendToMe )
    {
        rootSheet = aAppendToMe;
        screen    = rootSheet->GetScreen();

        if( !screen )
        {
            screen = new SCH_SCREEN( aSchematic );
            rootSheet->SetScreen( screen );
        }
    }
    else
    {
        rootSheet = new SCH_SHEET( aSchematic );
        screen    = new SCH_SCREEN( aSchematic );
        rootSheet->SetScreen( screen );
    }

    // Set page size from workspace dimensions
    PAGE_INFO page;
    double    wIn = pcad.workspaceWidth / 1000.0;
    double    hIn = pcad.workspaceHeight / 1000.0;

    if( std::abs( wIn - 17.0 ) < 0.5 && std::abs( hIn - 11.0 ) < 0.5 )
        page.SetType( wxT( "B" ) );
    else if( std::abs( wIn - 11.0 ) < 0.5 && std::abs( hIn - 8.5 ) < 0.5 )
        page.SetType( wxT( "A" ) );
    else
    {
        page.SetWidthMils( (int) pcad.workspaceWidth );
        page.SetHeightMils( (int) pcad.workspaceHeight );
    }

    screen->SetPageSettings( page );

    const double pageH = pcad.workspaceHeight;

    // --- Build LIB_SYMBOL cache from symbolDef entries ---------------------
    // Each symbol is added to the screen's lib symbol map so that the schematic
    // saves correctly in KiCad sexpr format.

    std::map<wxString, LIB_SYMBOL*> libSymbols;

    for( const PCAD_SCH::SYMBOL_DEF& sd : pcad.symbolDefs )
    {
        LIB_SYMBOL* ls = buildLibSymbol( sd );
        libSymbols[sd.name] = ls;
        screen->AddLibSymbol( new LIB_SYMBOL( *ls ) );
    }

    // --- Populate the screen from schematic sheets -------------------------
    // Multi-sheet P-Cad schematics will be handled in future work; for now
    // all content is placed on the single root screen.

    // TODO: multi-sheet support via SCH_SHEET hierarchy
    if( !pcad.sheets.empty() )
    {
        const PCAD_SCH::SHEET& sheet = pcad.sheets[0];

        // Wires
        for( const PCAD_SCH::WIRE& w : sheet.wires )
        {
            auto* line = new SCH_LINE( VECTOR2I( schX( w.x1 ), schY( w.y1, pageH ) ),
                                       LAYER_WIRE );
            line->SetEndPoint( VECTOR2I( schX( w.x2 ), schY( w.y2, pageH ) ) );
            screen->Append( line );
        }

        // Junctions
        for( const PCAD_SCH::JUNCTION& j : sheet.junctions )
        {
            auto* junc = new SCH_JUNCTION(
                    VECTOR2I( schX( j.x ), schY( j.y, pageH ) ) );
            screen->Append( junc );
        }

        // Net labels for named (non-auto) nets at junction points.
        // Auto-named nets start with "NET" followed by digits.
        for( const PCAD_SCH::JUNCTION& j : sheet.junctions )
        {
            if( j.netName.IsEmpty() )
                continue;

            if( j.netName.StartsWith( wxT( "NET" ) ) && j.netName.length() > 3
                && wxIsdigit( j.netName[3] ) )
                continue;

            auto* label = new SCH_LABEL(
                    VECTOR2I( schX( j.x ), schY( j.y, pageH ) ), j.netName );
            label->SetTextSize(
                    VECTOR2I( toIU( 50.0 ), toIU( 50.0 ) ) );
            screen->Append( label );
        }

        // Component instances
        for( const PCAD_SCH::SYMBOL_INST& inst : sheet.symbols )
        {
            auto libIt = libSymbols.find( inst.symbolRef );

            if( libIt == libSymbols.end() )
                continue;

            const LIB_SYMBOL* libSym = libIt->second;
            LIB_ID libId( PCAD_IMPORT_LIB, inst.symbolRef );

            auto* symbol = new SCH_SYMBOL();
            symbol->SetLibId( libId );
            symbol->SetUnit( 1 );
            symbol->SetLibSymbol( new LIB_SYMBOL( *libSym ) );
            symbol->SetPosition( VECTOR2I( schX( inst.x ), schY( inst.y, pageH ) ) );
            symbol->SetOrientation( symOrientation( inst.rotation ) );

            if( inst.isFlipped )
                symbol->SetMirrorY( true );

            // Initialise field positions from the lib symbol's attr positions
            // (UpdateFields sets each field to symbol_pos + lib_field_offset).
            symbol->UpdateFields( nullptr,
                                  true,  /* update style */
                                  false, /* update ref text */
                                  false, /* update other field text */
                                  false, /* reset ref */
                                  false  /* reset other fields */ );

            // Apply reference designator and value from the netlist compInst
            auto ciIt = pcad.compInstsByRef.find( inst.refDesRef );

            if( ciIt != pcad.compInstsByRef.end() )
            {
                const PCAD_SCH::COMP_INST* ci = ciIt->second;
                symbol->GetField( FIELD_T::REFERENCE )->SetText( ci->refDes );
                wxString val = ci->value.IsEmpty() ? ci->originalName : ci->value;

                if( !val.IsEmpty() )
                    symbol->GetField( FIELD_T::VALUE )->SetText( val );
            }
            else
            {
                symbol->GetField( FIELD_T::REFERENCE )->SetText( inst.refDesRef );
            }

            screen->Append( symbol );
        }
    }

    // Free the local lib symbol copies (the screen owns its own copies)
    for( auto& [name, ls] : libSymbols )
        delete ls;


    return rootSheet;
}


void SCH_IO_PCAD::EnumerateSymbolLib( wxArrayString& aSymbolNameList,
                                      const wxString& aLibraryPath,
                                      const std::map<std::string, UTF8>* aProperties )
{
    PCAD_SCH::SCHEMATIC pcad;
    PCAD_SCH::PCAD_SCH_PARSER parser;
    parser.LoadFromFile( aLibraryPath, pcad );

    for( const PCAD_SCH::SYMBOL_DEF& sd : pcad.symbolDefs )
        aSymbolNameList.Add( sd.name );
}


void SCH_IO_PCAD::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                      const wxString& aLibraryPath,
                                      const std::map<std::string, UTF8>* aProperties )
{
    PCAD_SCH::SCHEMATIC pcad;
    PCAD_SCH::PCAD_SCH_PARSER parser;
    parser.LoadFromFile( aLibraryPath, pcad );

    for( const PCAD_SCH::SYMBOL_DEF& sd : pcad.symbolDefs )
        aSymbolList.push_back( buildLibSymbol( sd ) );
}


LIB_SYMBOL* SCH_IO_PCAD::LoadSymbol( const wxString& aLibraryPath, const wxString& aPartName,
                                     const std::map<std::string, UTF8>* aProperties )
{
    PCAD_SCH::SCHEMATIC pcad;
    PCAD_SCH::PCAD_SCH_PARSER parser;
    parser.LoadFromFile( aLibraryPath, pcad );

    for( const PCAD_SCH::SYMBOL_DEF& sd : pcad.symbolDefs )
    {
        if( sd.name == aPartName )
            return buildLibSymbol( sd );
    }

    return nullptr;
}
