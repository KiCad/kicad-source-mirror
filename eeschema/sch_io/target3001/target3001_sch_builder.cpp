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

#include <sch_io/target3001/target3001_sch_builder.h>

#include <algorithm>
#include <cmath>
#include <set>

#include <wx/filename.h>
#include <wx/image.h>
#include <wx/mstream.h>

#include <base_units.h>
#include <geometry/seg.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <page_info.h>
#include <reporter.h>
#include <bitmap_base.h>
#include <sch_bitmap.h>
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
#include <trigo.h>
#include <wildcards_and_files_ext.h>

using TARGET3001::ELEMENT;
using TARGET3001::ELEMENT_TYPE;
using TARGET3001::TEXT_FUNCTION;
using TARGET3001::ToWx;

namespace
{

constexpr int PAGE_MARGIN_MM = 10;

// Keeps imported content on the standard connection grid when it is moved onto the page
constexpr int PLACEMENT_GRID = 12700;

// Signal of schematic buses, which join their member wires by name rather than by touching
constexpr int BUS_SIGNAL = 65502;

// Gap that parts touching items of different signals; one mil stays on the imperial grid
constexpr int SIGNAL_GAP = 254;

// Diagonal steps tried for a free spot before the last one is taken anyway
constexpr int MAX_DIAGONAL_STEPS = 8;

// Ends of one signal closer than this are one connection drawn off grid
constexpr int NEAR_MISS = 1000;

// Farthest a signal name is drawn from its wire and still names it
constexpr int LABEL_REACH = 50800;

const wxString LIB_NAME = wxS( "target3001" );


PIN_ORIENTATION pinOrientation( float aDirection )
{
    int quadrant = ( (int) std::lround( aDirection / 90.0f ) % 4 + 4 ) % 4;

    switch( quadrant )
    {
    case 1:  return PIN_ORIENTATION::PIN_UP;
    case 2:  return PIN_ORIENTATION::PIN_LEFT;
    case 3:  return PIN_ORIENTATION::PIN_DOWN;
    default: return PIN_ORIENTATION::PIN_RIGHT;
    }
}


ELECTRICAL_PINTYPE pinType( const ELEMENT& aPin )
{
    if( aPin.pin.supplyPin )
        return ELECTRICAL_PINTYPE::PT_POWER_IN;

    switch( aPin.pin.electricalType )
    {
    // An unconnected pin wired to a signal still joins it, which KiCad allows only when not internally connected
    case 0:  return aPin.base.signal ? ELECTRICAL_PINTYPE::PT_NIC : ELECTRICAL_PINTYPE::PT_NC;
    case 1:  return ELECTRICAL_PINTYPE::PT_INPUT;
    case 2:  return ELECTRICAL_PINTYPE::PT_OUTPUT;
    case 3:  return ELECTRICAL_PINTYPE::PT_BIDI;
    case 4:  return ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR;
    case 5:  return ELECTRICAL_PINTYPE::PT_POWER_IN;
    case 6:  return ELECTRICAL_PINTYPE::PT_PASSIVE;
    case 7:  return ELECTRICAL_PINTYPE::PT_TRISTATE;
    default: return ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
    }
}


GRAPHIC_PINSHAPE pinShape( const TARGET3001::PIN& aPin )
{
    if( aPin.inverted && aPin.clock )
        return GRAPHIC_PINSHAPE::INVERTED_CLOCK;

    if( aPin.inverted )
        return GRAPHIC_PINSHAPE::INVERTED;

    if( aPin.clock )
        return GRAPHIC_PINSHAPE::CLOCK;

    return GRAPHIC_PINSHAPE::LINE;
}


LINE_STYLE lineStyle( uint8_t aDash )
{
    switch( aDash )
    {
    case 1:  return LINE_STYLE::DASH;
    case 2:  return LINE_STYLE::DASHDOT;
    case 3:  return LINE_STYLE::DASHDOTDOT;
    case 4:  return LINE_STYLE::DOT;
    default: return LINE_STYLE::SOLID;
    }
}


/// Quarter turns of a text direction, 0 to 3
int textQuadrant( float aDirection )
{
    return ( (int) std::lround( aDirection / 90.0f ) % 4 + 4 ) % 4;
}


/// Hidden by flag bit 0, or by a zero weight
bool isHiddenText( const ELEMENT& aElem )
{
    return ( aElem.base.flags & 1 ) || aElem.text.weight == 0;
}


/// An unplaced gate stays in the library symbol but is drawn on no sheet
bool isUnplacedGate( const ELEMENT& aGate )
{
    return aGate.base.flags & 1;
}


VECTOR2I wireEnd( const SCH_LINE* aWire, int aEnd )
{
    return aEnd ? aWire->GetEndPoint() : aWire->GetStartPoint();
}


void setWireEnd( SCH_LINE* aWire, int aEnd, const VECTOR2I& aPos )
{
    if( aEnd )
        aWire->SetEndPoint( aPos );
    else
        aWire->SetStartPoint( aPos );
}


/// Point one gap in from a wire end, or the end itself when the wire is too short to shorten
VECTOR2I stepIn( const SCH_LINE* aWire, int aEnd )
{
    VECTOR2I end = wireEnd( aWire, aEnd );
    VECTOR2I span = wireEnd( aWire, 1 - aEnd ) - end;

    if( span.EuclideanNorm() <= 2 * SIGNAL_GAP )
        return end;

    return end + span.Resize( SIGNAL_GAP );
}


bool passesThrough( const SCH_LINE* aLine, const VECTOR2I& aPos )
{
    return !aLine->IsEndPoint( aPos ) && IsPointOnSegment( aLine->GetStartPoint(), aLine->GetEndPoint(), aPos );
}


/// Label spin that runs text along @a aDir
SPIN_STYLE spinAlong( const VECTOR2I& aDir )
{
    if( std::abs( aDir.x ) >= std::abs( aDir.y ) )
        return aDir.x < 0 ? SPIN_STYLE::LEFT : SPIN_STYLE::RIGHT;

    return aDir.y < 0 ? SPIN_STYLE::UP : SPIN_STYLE::BOTTOM;
}


/// True when two lines lie on one line and share more than a point
bool overlaps( const SCH_LINE* aFirst, const SCH_LINE* aSecond )
{
    VECTOR2I a = aFirst->GetStartPoint();
    VECTOR2I b = aFirst->GetEndPoint();
    VECTOR2I c = aSecond->GetStartPoint();
    VECTOR2I d = aSecond->GetEndPoint();
    VECTOR2I dir = b - a;

    if( dir.Cross( c - a ) || dir.Cross( d - a ) )
        return false;

    auto along = [&]( const VECTOR2I& aPos )
    {
        return dir.Dot( aPos - a );
    };

    return std::min( std::max( along( a ), along( b ) ), std::max( along( c ), along( d ) ) )
           > std::max( std::min( along( a ), along( b ) ), std::min( along( c ), along( d ) ) );
}


/**
 * Set size, angle and justification, and return the file position the text belongs at.
 *
 * Schematic text is always upright and never mirrored, so text turned past 90 degrees or mirrored keeps
 * its extent by justifying from the opposite side of its anchor.
 */
std::pair<int32_t, int32_t> configureText( EDA_TEXT* aText, const ELEMENT& aElem, TARGET3001::FONTS& aFonts,
                                           bool aUseRightEnd = true )
{
    const TARGET3001::TEXT& src = aElem.text;

    aFonts.Apply( aText, aElem, 100 );

    int  quadrant = textQuadrant( src.direction );
    bool upsideDown = quadrant >= 2;
    bool fromEnd = upsideDown != ( src.mirrored != 0 );

    aText->SetTextAngle( quadrant % 2 ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
    aText->SetVertJustify( upsideDown ? GR_TEXT_V_ALIGN_TOP : GR_TEXT_V_ALIGN_BOTTOM );

    // A stored right end lies on the far side of the anchor, so the justification flips with it
    std::optional<std::pair<int32_t, int32_t>> end = aElem.base.TextRightEnd();

    if( end && aUseRightEnd )
    {
        aText->SetHorizJustify( fromEnd ? GR_TEXT_H_ALIGN_LEFT : GR_TEXT_H_ALIGN_RIGHT );
        return *end;
    }

    aText->SetHorizJustify( fromEnd ? GR_TEXT_H_ALIGN_RIGHT : GR_TEXT_H_ALIGN_LEFT );
    return { aElem.base.x, aElem.base.y };
}

} // namespace


TARGET3001_SCH_BUILDER::TARGET3001_SCH_BUILDER( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet,
                                                const TARGET3001::PROJECT& aProject, const wxString& aFileName,
                                                REPORTER* aReporter ) :
        m_schematic( aSchematic ),
        m_rootSheet( aRootSheet ),
        m_project( aProject ),
        m_fileName( aFileName ),
        m_reporter( aReporter )
{
}


VECTOR2I TARGET3001_SCH_BUILDER::toSch( int32_t aX, int32_t aY )
{
    return VECTOR2I( KiROUND( aX / 100.0 ), KiROUND( -aY / 100.0 ) );
}


int TARGET3001_SCH_BUILDER::toSch( int32_t aLength )
{
    return KiROUND( aLength / 100.0 );
}


int TARGET3001_SCH_BUILDER::sheetOf( const ELEMENT& aElem )
{
    int sheet = aElem.base.layer - TARGET3001::SCHEMATIC_LAYER_BASE;

    if( sheet < 1 || sheet > TARGET3001::SCHEMATIC_SHEET_COUNT )
        return 0;

    return sheet;
}


void TARGET3001_SCH_BUILDER::Build()
{
    // Create sheets in order so the lowest numbered one becomes the root sheet
    std::set<int> used;

    for( const ELEMENT& e : m_project.components )
    {
        if( e.type == ELEMENT_TYPE::COMPONENT && e.component )
        {
            for( const ELEMENT& s : e.component->symbols )
            {
                if( !isUnplacedGate( s ) )
                    used.insert( sheetOf( s ) );
            }
        }
    }

    for( const ELEMENT& e : m_project.signals )
    {
        if( e.type == ELEMENT_TYPE::SIGNAL && e.signal )
        {
            for( const ELEMENT& c : e.signal->connections )
                used.insert( sheetOf( c ) );
        }
    }

    for( const ELEMENT& e : m_project.schematicItems )
        used.insert( sheetOf( e ) );

    used.erase( 0 );

    for( int sheet : used )
        screen( sheet );

    for( const ELEMENT& e : m_project.components )
    {
        if( e.type == ELEMENT_TYPE::COMPONENT && e.component )
            addComponent( e );
    }

    for( const ELEMENT& e : m_project.signals )
    {
        if( e.type == ELEMENT_TYPE::SIGNAL && e.signal )
            addConnections( e );
    }

    for( const ELEMENT& e : m_project.schematicItems )
        addFreeItem( e );

    for( const auto& [sheet, items] : m_netItems )
        separateSignals( sheet );

    // Parting signals can split one, so its pieces are joined on the final geometry
    joinSignalPieces();

    finishSheets();
}


void TARGET3001_SCH_BUILDER::addToSheet( int aSheet, SCH_ITEM* aItem )
{
    if( !aItem )
        return;

    BOX2I bbox = aItem->GetBoundingBox();
    auto  it = m_extents.find( aSheet );

    if( it == m_extents.end() )
        m_extents[aSheet] = bbox;
    else
        it->second.Merge( bbox );

    m_pending[aSheet].push_back( aItem );
}


SCH_ITEM* TARGET3001_SCH_BUILDER::makeGraphic( const ELEMENT& aElem, const VECTOR2I& aOrigin, bool aInSymbol ) const
{
    const TARGET3001::ELEMENT_BASE& b = aElem.base;
    SCH_LAYER_ID                    layer = aInSymbol ? LAYER_DEVICE : LAYER_NOTES;
    VECTOR2I                        p0 = toSch( b.x, b.y ) - aOrigin;

    switch( aElem.type )
    {
    case ELEMENT_TYPE::LINE:
    {
        const TARGET3001::LINE& l = aElem.line;
        VECTOR2I                p1 = toSch( l.x2, l.y2 ) - aOrigin;

        if( !aInSymbol )
        {
            SCH_LINE* line = new SCH_LINE( p0, LAYER_NOTES );
            line->SetEndPoint( p1 );
            line->SetStroke( STROKE_PARAMS( toSch( l.width ), lineStyle( l.dashStyle ) ) );
            return line;
        }

        SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, layer, toSch( l.width ) );
        shape->AddPoint( p0 );
        shape->AddPoint( p1 );
        shape->SetStroke( STROKE_PARAMS( toSch( l.width ), lineStyle( l.dashStyle ) ) );
        return shape;
    }

    case ELEMENT_TYPE::RECTANGLE:
    {
        if( !aInSymbol )
        {
            if( SCH_BITMAP* picture = makePicture( aElem ) )
                return picture;
        }

        const TARGET3001::RECTANGLE& r = aElem.rectangle;
        EDA_ANGLE                    rot( double( r.direction ), DEGREES_T );
        SCH_SHAPE*                   shape = new SCH_SHAPE( SHAPE_T::POLY, layer, 0, FILL_T::FILLED_SHAPE );
        VECTOR2D corners[4] = { { 0.0, 0.0 }, { double( r.width ), 0.0 },
                                { double( r.width ), double( r.height ) }, { 0.0, double( r.height ) } };

        for( VECTOR2D c : corners )
        {
            RotatePoint( c, -rot );
            shape->AddPoint( toSch( b.x + KiROUND( c.x ), b.y + KiROUND( c.y ) ) - aOrigin );
        }

        shape->AddPoint( p0 );
        return shape;
    }

    case ELEMENT_TYPE::TRIANGLE:
    {
        const TARGET3001::TRIANGLE& t = aElem.triangle;
        SCH_SHAPE*                  shape = new SCH_SHAPE( SHAPE_T::POLY, layer, 0, FILL_T::FILLED_SHAPE );
        shape->AddPoint( p0 );
        shape->AddPoint( toSch( t.x2, t.y2 ) - aOrigin );
        shape->AddPoint( toSch( t.x3, t.y3 ) - aOrigin );
        shape->AddPoint( p0 );
        return shape;
    }

    case ELEMENT_TYPE::DISC:
    {
        SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::CIRCLE, layer, 0, FILL_T::FILLED_SHAPE );
        shape->SetCenter( p0 );
        shape->SetEnd( p0 + VECTOR2I( toSch( aElem.disc.radius ), 0 ) );
        return shape;
    }

    case ELEMENT_TYPE::ARC:
    {
        const TARGET3001::ARC& a = aElem.arc;
        VECTOR2I               center = toSch( a.centerX, a.centerY ) - aOrigin;
        int                    radius = toSch( a.radius );
        double                 sweep = a.Sweep();

        if( sweep >= 360.0 - 1e-6 )
        {
            SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::CIRCLE, layer, toSch( a.width ) );
            shape->SetCenter( center );
            shape->SetEnd( center + VECTOR2I( radius, 0 ) );
            return shape;
        }

        auto at = [&]( double aDeg )
        {
            return center + VECTOR2I( KiROUND( radius * cos( DEG2RAD( aDeg ) ) ),
                                      -KiROUND( radius * sin( DEG2RAD( aDeg ) ) ) );
        };

        SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::ARC, layer, toSch( a.width ) );
        shape->SetArcGeometry( at( a.startAngle ), at( a.startAngle + sweep / 2.0 ),
                               at( a.startAngle + sweep ) );
        return shape;
    }

    case ELEMENT_TYPE::TEXT:
    {
        if( isHiddenText( aElem ) )
            return nullptr;

        wxString content = m_project.FunctionText( aElem.text, true ).value_or( ToWx( aElem.text.text ) );

        SCH_TEXT*                   text = new SCH_TEXT( p0, content, layer );
        std::pair<int32_t, int32_t> pos = configureText( text, aElem, m_fonts );
        text->SetPosition( toSch( pos.first, pos.second ) - aOrigin + TARGET3001::FONTS::BaselineShift( *text ) );
        return text;
    }

    default:
        return nullptr;
    }
}


SCH_BITMAP* TARGET3001_SCH_BUILDER::makePicture( const ELEMENT& aElem ) const
{
    if( aElem.type != ELEMENT_TYPE::RECTANGLE )
        return nullptr;

    const TARGET3001::RECTANGLE& r = aElem.rectangle;
    std::vector<uint8_t>         data = aElem.base.Picture();
    VECTOR2I                     size( std::abs( toSch( r.width ) ), std::abs( toSch( r.height ) ) );
    wxImage                      image;

    if( data.empty() || size.x == 0 || size.y == 0 )
        return nullptr;

    wxMemoryInputStream stream( data.data(), data.size() );

    if( !image.LoadFile( stream, wxBITMAP_TYPE_ANY ) )
        return nullptr;

    // The picture is stretched over its rectangle; KiCad only scales images uniformly
    int w = image.GetWidth();
    int h = image.GetHeight();
    int stretchedW = std::max( w, KiROUND( double( h ) * size.x / size.y ) );
    int stretchedH = std::max( h, KiROUND( double( w ) * size.y / size.x ) );

    if( stretchedW > w + 1 || stretchedH > h + 1 )
        image.Rescale( stretchedW, stretchedH, wxIMAGE_QUALITY_HIGH );

    EDA_ANGLE rot( double( r.direction ), DEGREES_T );
    VECTOR2D  half( r.width / 2.0, r.height / 2.0 );
    RotatePoint( half, -rot );

    SCH_BITMAP*      bitmap = new SCH_BITMAP();
    REFERENCE_IMAGE& ref = bitmap->GetReferenceImage();

    if( !ref.SetImage( image ) )
    {
        delete bitmap;
        return nullptr;
    }

    ref.MutableImage().SetImageType( image.GetType() );
    ref.SetWidth( size.x );

    VECTOR2I center = toSch( aElem.base.x + KiROUND( half.x ), aElem.base.y + KiROUND( half.y ) );
    ref.SetPosition( center );

    if( !rot.IsZero() )
        ref.Rotate( center, rot );

    return bitmap;
}


void TARGET3001_SCH_BUILDER::addComponent( const ELEMENT& aComponent )
{
    const TARGET3001::COMPONENT& comp = *aComponent.component;
    std::vector<const ELEMENT*>  gates;

    for( const ELEMENT& s : comp.symbols )
    {
        if( s.type == ELEMENT_TYPE::SYMBOL && s.symbol && sheetOf( s ) > 0 )
            gates.push_back( &s );
    }

    if( gates.empty() )
        return;

    std::stable_sort( gates.begin(), gates.end(),
                      []( const ELEMENT* a, const ELEMENT* b )
                      {
                          return a->symbol->gateOrdinal < b->symbol->gateOrdinal;
                      } );

    bool hasPackage = std::any_of( comp.packageItems.begin(), comp.packageItems.end(),
                                   []( const ELEMENT& e ) { return e.type == ELEMENT_TYPE::PACKAGE; } );
    bool allSupply = true;
    int  supplySignal = 0;

    for( const ELEMENT* gate : gates )
    {
        for( const ELEMENT& child : gate->symbol->children )
        {
            if( child.type != ELEMENT_TYPE::PIN )
                continue;

            if( !child.pin.supplyPin )
                allSupply = false;
            else if( !supplySignal )
                supplySignal = child.base.signal;
        }
    }

    bool isPower = !hasPackage && allSupply && supplySignal;

    wxString reference = ToWx( comp.prefix ).Trim().Trim( false );
    uint16_t number = comp.referenceExtension.value_or( comp.referenceNumber );

    if( isPower )
        reference = wxString::Format( wxS( "#PWR%03d" ), ++m_powerCount );
    else if( number )
        reference << number;

    wxString value = ToWx( comp.value );

    const ELEMENT* sig = isPower ? m_project.FindSignal( supplySignal ) : nullptr;

    if( sig && !sig->signal->name.empty() )
        value = ToWx( sig->signal->name );

    // Each placed component gets its own library symbol because gate geometry is stored as drawn
    wxString symName = ToWx( comp.name );

    if( symName.IsEmpty() )
        symName = reference;

    symName = wxString::Format( wxS( "%s_%d" ), symName, int( aComponent.base.component ) );
    symName = LIB_ID::FixIllegalChars( symName, false ).wx_str();
    symName.Replace( wxS( "/" ), wxS( "_" ) );

    LIB_SYMBOL* lib = new LIB_SYMBOL( symName );
    lib->SetLibId( LIB_ID( LIB_NAME, symName ) );
    lib->SetUnitCount( int( gates.size() ), false );
    lib->GetReferenceField().SetText( reference );
    lib->GetValueField().SetText( value );

    if( isPower )
        lib->SetGlobalPower();

    std::map<uint16_t, wxString> padNumbers;

    for( const ELEMENT& item : comp.packageItems )
    {
        if( item.type == ELEMENT_TYPE::PAD )
            padNumbers.emplace( item.pad.number, PadNumber( item ) );
    }

    addGateBodies( lib, gates, true, padNumbers );

    for( size_t g = 0; g < gates.size(); g++ )
    {
        const ELEMENT& gate = *gates[g];

        if( isUnplacedGate( gate ) )
            continue;

        SCH_SHEET_PATH path;
        path.push_back( m_sheets.at( sheetOf( gate ) ) );

        SCH_SYMBOL* symbol = new SCH_SYMBOL( *lib, lib->GetLibId(), &path, int( g ) + 1, 0,
                                             toSch( gate.base.x, gate.base.y ) );
        symbol->SetLibSymbol( new LIB_SYMBOL( *lib ) );
        symbol->SetRef( &path, reference );
        symbol->GetField( FIELD_T::REFERENCE )->SetText( reference );
        symbol->GetField( FIELD_T::VALUE )->SetText( value );

        if( !comp.package.empty() )
            symbol->SetFootprintFieldText( LIB_NAME + wxS( ":" ) + ToWx( comp.package ) );

        bool refShown = false;
        bool valueShown = false;

        for( const ELEMENT& child : gate.symbol->children )
        {
            if( child.type != ELEMENT_TYPE::TEXT )
                continue;

            FIELD_T id;

            if( child.text.function == uint8_t( TEXT_FUNCTION::REFERENCE ) )
                id = FIELD_T::REFERENCE;
            else if( child.text.function == uint8_t( TEXT_FUNCTION::VALUE ) )
                id = FIELD_T::VALUE;
            else
                continue;

            SCH_FIELD* field = symbol->GetField( id );
            std::pair<int32_t, int32_t> pos = configureText( field, child, m_fonts );
            field->SetPosition( toSch( pos.first, pos.second ) + TARGET3001::FONTS::BaselineShift( *field ) );
            field->SetVisible( !isHiddenText( child ) );
            ( id == FIELD_T::REFERENCE ? refShown : valueShown ) = !isHiddenText( child );
        }

        if( !refShown )
            symbol->GetField( FIELD_T::REFERENCE )->SetVisible( false );

        if( !valueShown )
            symbol->GetField( FIELD_T::VALUE )->SetVisible( false );

        addToSheet( sheetOf( gate ), symbol );

        if( isPower )
            m_powerSheets[supplySignal].insert( sheetOf( gate ) );

        for( const ELEMENT& child : gate.symbol->children )
        {
            if( SCH_BITMAP* picture = makePicture( child ) )
                addToSheet( sheetOf( gate ), picture );

            // An unconnected pin belongs to no signal, so it gets one of its own
            if( child.type == ELEMENT_TYPE::PIN )
            {
                int signal = child.base.signal ? int( child.base.signal ) : -( ++m_unconnectedPins );
                m_pinSignals[sheetOf( gate )].emplace_back( toSch( child.base.x, child.base.y ), signal );

                if( child.base.signal )
                {
                    m_pinEnds[signal].push_back(
                            { sheetOf( gate ), toSch( child.base.x, child.base.y ), child.pin.direction, isPower } );
                }
            }
        }
    }

    delete lib;
}


void TARGET3001_SCH_BUILDER::addGateBodies( LIB_SYMBOL* aLib, const std::vector<const ELEMENT*>& aGates,
                                           bool aFieldTexts, const std::map<uint16_t, wxString>& aPadNumbers ) const
{
    bool                  showNumbers = false;
    bool                  showNames = false;
    std::vector<SCH_PIN*> hiddenNames;

    for( size_t g = 0; g < aGates.size(); g++ )
    {
        const ELEMENT& gate = *aGates[g];
        VECTOR2I       origin = toSch( gate.base.x, gate.base.y );
        int            unit = int( g ) + 1;

        for( const ELEMENT& child : gate.symbol->children )
        {
            if( child.type == ELEMENT_TYPE::PIN )
            {
                const TARGET3001::PIN& src = child.pin;
                SCH_PIN*               pin = new SCH_PIN( aLib );
                wxString               name;
                bool                   nameShown = false;

                for( const ELEMENT& pc : child.children )
                {
                    if( pc.type == ELEMENT_TYPE::TEXT && pc.text.function == uint8_t( TEXT_FUNCTION::PIN_NAME ) )
                    {
                        name = ToWx( pc.text.text );
                        nameShown = !isHiddenText( pc ) && !name.IsEmpty();

                        if( nameShown )
                        {
                            SCH_TEXT probe( VECTOR2I( 0, 0 ), name );
                            m_fonts.Apply( &probe, pc, 100 );
                            pin->SetNameTextSize( probe.GetTextHeight() );
                        }
                    }
                }

                pin->SetPosition( toSch( child.base.x, child.base.y ) - origin );
                pin->SetOrientation( pinOrientation( src.direction ) );
                pin->SetLength( toSch( src.length ) );
                pin->SetType( pinType( child ) );
                pin->SetShape( pinShape( src ) );
                pin->SetName( name.IsEmpty() ? wxString( wxS( "~" ) ) : name );
                pin->SetUnit( unit );

                // The footprint pad decides the number so pins and pads connect; the pin's PADNAME is what it shows
                auto               pad = aPadNumbers.find( src.pinNumber );
                const std::string* padName = child.base.FindProp( "PADNAME" );

                if( pad != aPadNumbers.end() )
                    pin->SetNumber( pad->second );
                else if( padName && !padName->empty() )
                    pin->SetNumber( ToWx( *padName ) );
                else
                    pin->SetNumber( wxString::Format( wxS( "%u" ), src.pinNumber ) );

                if( !nameShown && !name.IsEmpty() )
                    hiddenNames.push_back( pin );

                // KiCad shows names and numbers per symbol, so one shown pin shows them all
                showNumbers |= src.showNumber != 0;
                showNames |= nameShown;
                aLib->AddDrawItem( pin, false );
                continue;
            }

            if( aFieldTexts && child.type == ELEMENT_TYPE::TEXT
                && ( child.text.function == uint8_t( TEXT_FUNCTION::REFERENCE )
                     || child.text.function == uint8_t( TEXT_FUNCTION::VALUE ) ) )
            {
                continue;
            }

            // Library symbols cannot hold images; placed gates put the picture on their sheet
            if( child.type == ELEMENT_TYPE::RECTANGLE && child.base.FindProp( "JPEG_PICTURE" ) )
                continue;

            if( SCH_ITEM* item = makeGraphic( child, origin, true ) )
            {
                item->SetUnit( unit );
                aLib->AddDrawItem( item, false );
            }
        }
    }

    // Names are hidden per pin in the source but per symbol in KiCad
    if( showNames )
    {
        for( SCH_PIN* pin : hiddenNames )
            pin->SetName( wxS( "~" ) );
    }

    aLib->GetDrawItems().sort();
    aLib->SetShowPinNumbers( showNumbers );
    aLib->SetShowPinNames( showNames );
}


void TARGET3001_SCH_BUILDER::addConnections( const ELEMENT& aSignal )
{
    wxString name = ToWx( aSignal.signal->name );
    bool     isBus = aSignal.base.signal == BUS_SIGNAL;
    bool     global = !isBus && joinsGlobally( aSignal );

    if( global )
        m_globalSignals.insert( aSignal.base.signal );

    for( const ELEMENT& e : aSignal.signal->connections )
    {
        int sheet = sheetOf( e );

        if( sheet == 0 )
            continue;

        const TARGET3001::ELEMENT_BASE& b = e.base;

        switch( e.type )
        {
        case ELEMENT_TYPE::LINE:
        {
            SCH_LINE* wire = new SCH_LINE( toSch( b.x, b.y ), isBus ? LAYER_BUS : LAYER_WIRE );
            wire->SetEndPoint( toSch( e.line.x2, e.line.y2 ) );
            addNetItem( sheet, wire, aSignal.base.signal );
            break;
        }

        case ELEMENT_TYPE::DISC:
            addNetItem( sheet, new SCH_JUNCTION( toSch( b.x, b.y ), toSch( 2 * e.disc.radius ) ),
                        aSignal.base.signal );
            break;

        case ELEMENT_TYPE::TEXT:
        {
            bool signalName = e.text.function == uint8_t( TEXT_FUNCTION::SIGNAL_NAME );

            // Labels carry connectivity between wire groups, so a hidden signal name still becomes one
            if( isHiddenText( e ) && !signalName )
                break;

            // The name is drawn beside its wire; a label joins only where it touches
            std::optional<VECTOR2I> anchor;

            if( signalName )
                anchor = labelAnchor( aSignal, sheet, toSch( b.x, b.y ) );

            if( anchor )
            {
                SCH_LABEL_BASE* label = nullptr;

                if( global )
                    label = new SCH_GLOBALLABEL( *anchor, name );
                else
                    label = new SCH_LABEL( *anchor, name );

                configureText( label, e, m_fonts, false );
                addNetItem( sheet, label, aSignal.base.signal );
            }
            else
            {
                addToSheet( sheet, makeGraphic( e, VECTOR2I( 0, 0 ), false ) );
            }

            break;
        }

        default:
            addToSheet( sheet, makeGraphic( e, VECTOR2I( 0, 0 ), false ) );
            break;
        }
    }
}


bool TARGET3001_SCH_BUILDER::joinsGlobally( const ELEMENT& aSignal ) const
{
    if( aSignal.signal->name.empty() )
        return false;

    // Supply symbols name their net globally, which a local label would not join
    if( m_powerSheets.count( aSignal.base.signal ) )
        return true;

    std::set<int> sheets;

    for( const ELEMENT& e : aSignal.signal->connections )
    {
        if( sheetOf( e ) )
            sheets.insert( sheetOf( e ) );
    }

    if( auto it = m_pinEnds.find( aSignal.base.signal ); it != m_pinEnds.end() )
    {
        for( const PIN_END& pin : it->second )
            sheets.insert( pin.sheet );
    }

    return sheets.size() > 1;
}


std::optional<VECTOR2I> TARGET3001_SCH_BUILDER::labelAnchor( const ELEMENT& aSignal, int aSheet,
                                                             const VECTOR2I& aPos ) const
{
    std::optional<VECTOR2I> best;

    auto consider = [&]( const VECTOR2I& aCandidate )
    {
        SEG::ecoord dist = ( aCandidate - aPos ).SquaredEuclideanNorm();

        if( dist <= SEG::Square( LABEL_REACH ) && ( !best || dist < ( *best - aPos ).SquaredEuclideanNorm() ) )
            best = aCandidate;
    };

    for( const ELEMENT& e : aSignal.signal->connections )
    {
        if( e.type != ELEMENT_TYPE::LINE || sheetOf( e ) != aSheet )
            continue;

        SEG      wire( toSch( e.base.x, e.base.y ), toSch( e.line.x2, e.line.y2 ) );
        VECTOR2I nearest = wire.NearestPoint( aPos );

        // Rounding can leave the point beside a slanted wire, so it falls back to the nearer end
        if( ( nearest - wire.A ).Cross( wire.B - wire.A ) != 0 )
        {
            consider( wire.A );
            consider( wire.B );
        }
        else
        {
            consider( nearest );
        }
    }

    if( auto it = m_pinEnds.find( aSignal.base.signal ); it != m_pinEnds.end() )
    {
        for( const PIN_END& pin : it->second )
        {
            if( pin.sheet == aSheet )
                consider( pin.pos );
        }
    }

    return best;
}


void TARGET3001_SCH_BUILDER::joinSignalPieces()
{
    std::map<int, std::map<int, std::vector<SCH_ITEM*>>> bySignal;

    for( const auto& [sheet, items] : m_netItems )
    {
        for( const auto& [item, signal] : items )
            bySignal[signal][sheet].push_back( item );
    }

    for( const auto& [signal, pins] : m_pinEnds )
    {
        if( signal != BUS_SIGNAL )
            joinPieces( signal, bySignal[signal] );
    }
}


void TARGET3001_SCH_BUILDER::joinPieces( int aSignal, const std::map<int, std::vector<SCH_ITEM*>>& aItems )
{
    const std::vector<PIN_END>& pins = m_pinEnds.at( aSignal );
    bool                        global = m_globalSignals.count( aSignal ) > 0;
    bool                        anyNamed = false;
    std::set<int>               sheets;

    // An unnamed piece takes its label on a free wire end, else on its pin
    struct PIECE
    {
        const PIN_END*                             pin;
        std::vector<std::pair<VECTOR2I, VECTOR2I>> ends;   ///< open wire ends and their outward direction
    };

    std::vector<PIECE> unnamed;

    // Only pieces holding a pin matter to the netlist
    for( const PIN_END& pin : pins )
        sheets.insert( pin.sheet );

    auto isFree = [&]( int aSheet, const VECTOR2I& aPos )
    {
        for( const auto& [pos, pinSignal] : m_pinSignals[aSheet] )
        {
            if( pos == aPos && pinSignal != aSignal )
                return false;
        }

        for( const auto& [item, itemSignal] : m_netItems[aSheet] )
        {
            if( itemSignal == aSignal )
                continue;

            if( item->Type() != SCH_LINE_T )
            {
                if( item->GetPosition() == aPos )
                    return false;

                continue;
            }

            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), aPos ) )
                return false;
        }

        return true;
    };

    for( int sheet : sheets )
    {
        std::vector<SEG>      wires;
        std::vector<VECTOR2I> points;
        std::set<VECTOR2I>    names;
        std::set<VECTOR2I>    pinPoints;

        if( auto it = aItems.find( sheet ); it != aItems.end() )
        {
            for( SCH_ITEM* item : it->second )
            {
                if( item->Type() == SCH_LINE_T )
                {
                    SCH_LINE* line = static_cast<SCH_LINE*>( item );
                    wires.emplace_back( line->GetStartPoint(), line->GetEndPoint() );
                }
                else if( dynamic_cast<SCH_LABEL_BASE*>( item ) )
                {
                    names.insert( item->GetPosition() );
                }
                else
                {
                    points.push_back( item->GetPosition() );
                }
            }
        }

        for( const PIN_END& pin : pins )
        {
            if( pin.sheet == sheet )
                pinPoints.insert( pin.pos );
        }

        std::map<VECTOR2I, int> index;
        std::vector<int>        parent;

        auto node = [&]( const VECTOR2I& aPos )
        {
            auto [it, inserted] = index.emplace( aPos, int( parent.size() ) );

            if( inserted )
                parent.push_back( it->second );

            return it->second;
        };

        auto find = [&]( int aNode )
        {
            while( parent[aNode] != aNode )
                aNode = parent[aNode] = parent[parent[aNode]];

            return aNode;
        };

        auto unite = [&]( int aFirst, int aSecond )
        {
            parent[find( aFirst )] = find( aSecond );
        };

        std::vector<VECTOR2I> ends( pinPoints.begin(), pinPoints.end() );

        for( const SEG& wire : wires )
        {
            int start = node( wire.A );
            unite( start, node( wire.B ) );
            ends.push_back( wire.A );
            ends.push_back( wire.B );
        }

        for( const VECTOR2I& pos : points )
            node( pos );

        for( const VECTOR2I& pos : names )
            node( pos );

        for( const VECTOR2I& pos : pinPoints )
            node( pos );

        // A point lying inside a wire joins it
        for( const auto& [pos, id] : index )
        {
            for( const SEG& wire : wires )
            {
                if( IsPointOnSegment( wire.A, wire.B, pos ) )
                    unite( id, index.at( wire.A ) );
            }
        }

        std::sort( ends.begin(), ends.end(),
                   []( const VECTOR2I& a, const VECTOR2I& b )
                   {
                       return a.x < b.x || ( a.x == b.x && a.y < b.y );
                   } );

        ends.erase( std::unique( ends.begin(), ends.end() ), ends.end() );

        for( size_t i = 0; i < ends.size(); i++ )
        {
            for( size_t j = i + 1; j < ends.size() && ends[j].x - ends[i].x < NEAR_MISS; j++ )
            {
                int first = index.at( ends[i] );
                int second = index.at( ends[j] );

                if( ( ends[j] - ends[i] ).EuclideanNorm() >= NEAR_MISS || find( first ) == find( second ) )
                    continue;

                SCH_LINE* stub = new SCH_LINE( ends[i], LAYER_WIRE );
                stub->SetEndPoint( ends[j] );
                addNetItem( sheet, stub, aSignal );
                unite( first, second );
            }
        }

        std::set<int> named;

        for( const VECTOR2I& pos : names )
            named.insert( find( index.at( pos ) ) );

        for( const PIN_END& pin : pins )
        {
            if( global && pin.power && pin.sheet == sheet )
                named.insert( find( index.at( pin.pos ) ) );
        }

        std::map<VECTOR2I, int>      endCount;
        std::map<VECTOR2I, VECTOR2I> outward;

        for( const SEG& wire : wires )
        {
            endCount[wire.A]++;
            endCount[wire.B]++;
            outward[wire.A] = wire.A - wire.B;
            outward[wire.B] = wire.B - wire.A;
        }

        std::set<int> seen;

        for( const PIN_END& pin : pins )
        {
            if( pin.sheet != sheet )
                continue;

            int root = find( index.at( pin.pos ) );

            if( named.count( root ) )
            {
                anyNamed = true;
                continue;
            }

            if( !seen.insert( root ).second )
                continue;

            PIECE piece{ &pin, {} };

            for( const auto& [pos, count] : endCount )
            {
                if( count == 1 && !pinPoints.count( pos ) && find( index.at( pos ) ) == root )
                    piece.ends.emplace_back( pos, outward.at( pos ) );
            }

            unnamed.push_back( std::move( piece ) );
        }
    }

    const ELEMENT* sig = m_project.FindSignal( aSignal );

    if( unnamed.size() + ( anyNamed ? 1 : 0 ) < 2 || !sig || sig->signal->name.empty() )
        return;

    // A name another signal shares would join the two
    for( const ELEMENT& e : m_project.signals )
    {
        if( &e != sig && e.type == ELEMENT_TYPE::SIGNAL && e.signal && e.signal->name == sig->signal->name )
            return;
    }

    wxString name = ToWx( sig->signal->name );

    for( const PIECE& piece : unnamed )
    {
        int        sheet = piece.pin->sheet;
        VECTOR2I   pos = piece.pin->pos;
        SPIN_STYLE spin = SPIN_STYLE::RIGHT;

        // Text on a pin runs away from the body
        switch( pinOrientation( piece.pin->direction ) )
        {
        case PIN_ORIENTATION::PIN_RIGHT: spin = SPIN_STYLE::LEFT;   break;
        case PIN_ORIENTATION::PIN_UP:    spin = SPIN_STYLE::BOTTOM; break;
        case PIN_ORIENTATION::PIN_DOWN:  spin = SPIN_STYLE::UP;     break;
        default:                         spin = SPIN_STYLE::RIGHT;  break;
        }

        for( const auto& [end, dir] : piece.ends )
        {
            if( isFree( sheet, end ) )
            {
                pos = end;
                spin = spinAlong( dir );
                break;
            }
        }

        SCH_LABEL_BASE* label = nullptr;

        if( global )
            label = new SCH_GLOBALLABEL( pos, name );
        else
            label = new SCH_LABEL( pos, name );

        label->SetSpinStyle( spin );
        addNetItem( sheet, label, aSignal );
    }
}


void TARGET3001_SCH_BUILDER::addNetItem( int aSheet, SCH_ITEM* aItem, int aSignal )
{
    addToSheet( aSheet, aItem );
    m_netItems[aSheet].emplace_back( aItem, aSignal );
}


void TARGET3001_SCH_BUILDER::separateSignals( int aSheet )
{
    std::vector<std::pair<SCH_LINE*, int>> wires;
    std::vector<SCH_LINE*>                 buses;
    std::vector<std::pair<SCH_ITEM*, int>> points;

    for( const auto& [item, signal] : m_netItems[aSheet] )
    {
        if( item->Type() != SCH_LINE_T )
            points.emplace_back( item, signal );
        else if( signal == BUS_SIGNAL )
            buses.push_back( static_cast<SCH_LINE*>( item ) );
        else
            wires.emplace_back( static_cast<SCH_LINE*>( item ), signal );
    }

    // A bus joins its members by name, so each wire leaving it needs a label of its signal
    for( const auto& [wire, signal] : wires )
    {
        for( int end : { 0, 1 } )
        {
            VECTOR2I pos = wireEnd( wire, end );
            VECTOR2I moved = stepIn( wire, end );

            bool onBus = std::any_of( buses.begin(), buses.end(),
                                      [&]( SCH_LINE* bus )
                                      {
                                          return IsPointOnSegment( bus->GetStartPoint(), bus->GetEndPoint(), pos );
                                      } );

            if( !onBus || moved == pos )
                continue;

            setWireEnd( wire, end, moved );
            bool labeled = false;

            for( const auto& [item, itemSignal] : points )
            {
                if( itemSignal != signal )
                    continue;

                if( item->GetPosition() == pos )
                    item->SetPosition( moved );

                labeled |= dynamic_cast<SCH_LABEL_BASE*>( item )
                           && IsPointOnSegment( wire->GetStartPoint(), wire->GetEndPoint(), item->GetPosition() );
            }

            const ELEMENT* sig = m_project.FindSignal( signal );

            if( labeled || !sig || sig->signal->name.empty() )
                continue;

            wxString        name = ToWx( sig->signal->name );
            SCH_LABEL_BASE* label = nullptr;

            if( m_globalSignals.count( signal ) )
                label = new SCH_GLOBALLABEL( moved, name );
            else
                label = new SCH_LABEL( moved, name );

            // The text runs along the wire, away from the bus
            label->SetSpinStyle( spinAlong( wireEnd( wire, 1 - end ) - moved ) );

            addNetItem( aSheet, label, signal );
            points.emplace_back( label, signal );
        }
    }

    // A junction or label inside a wire joins a branch to it, which a shifted wire would lose
    auto hasBranch = [&]( const std::pair<SCH_LINE*, int>& aWire )
    {
        return std::any_of( points.begin(), points.end(),
                            [&]( const std::pair<SCH_ITEM*, int>& aPoint )
                            {
                                return aPoint.second == aWire.second
                                       && passesThrough( aWire.first, aPoint.first->GetPosition() );
                            } );
    };

    // Overlapping wires of different signals would be merged into one, so one of them steps aside; ends left
    // inside the other wire are parted below with everything else
    size_t drawn = wires.size();

    for( size_t i = 0; i < drawn; i++ )
    {
        for( size_t j = i + 1; j < drawn; j++ )
        {
            if( wires[i].second == wires[j].second || !overlaps( wires[i].first, wires[j].first ) )
                continue;

            bool                      firstShorter = wires[i].first->GetLength() <= wires[j].first->GetLength();
            std::pair<SCH_LINE*, int> pick = firstShorter ? wires[i] : wires[j];

            if( hasBranch( pick ) )
                pick = firstShorter ? wires[j] : wires[i];

            SCH_LINE* wire = pick.first;

            if( hasBranch( pick ) || wire->GetLength() <= 2 * SIGNAL_GAP )
                continue;

            // Stubs join the shifted run back to its ends, which stay where they connect
            VECTOR2I start = wire->GetStartPoint();
            VECTOR2I end = wire->GetEndPoint();
            VECTOR2I side = VECTOR2I( start.y - end.y, end.x - start.x ).Resize( SIGNAL_GAP );

            for( const auto& [from, to] : { std::pair( start, start + side ), std::pair( end + side, end ) } )
            {
                SCH_LINE* stub = new SCH_LINE( *wire );
                stub->SetStartPoint( from );
                stub->SetEndPoint( to );
                addNetItem( aSheet, stub, pick.second );
                wires.emplace_back( stub, pick.second );
            }

            wire->SetStartPoint( start + side );
            wire->SetEndPoint( end + side );
        }
    }

    // What KiCad joins at each point: everything there and every line passing through. A fixed member is a pin,
    // a bus or a line passing through; only pins and buses have no item
    struct MEMBER
    {
        int       signal;
        SCH_ITEM* item;
        int       end;
        bool      fixed;
    };

    std::map<VECTOR2I, std::vector<MEMBER>> at;
    std::set<VECTOR2I>                      busEnds;

    for( const auto& [wire, signal] : wires )
    {
        for( int end : { 0, 1 } )
            at[wireEnd( wire, end )].push_back( { signal, wire, end, false } );
    }

    for( const auto& [pos, signal] : m_pinSignals[aSheet] )
        at[pos].push_back( { signal, nullptr, -1, true } );

    for( const auto& [item, signal] : points )
        at[item->GetPosition()].push_back( { signal, item, -1, false } );

    for( SCH_LINE* bus : buses )
    {
        busEnds.insert( bus->GetStartPoint() );
        busEnds.insert( bus->GetEndPoint() );
    }

    for( auto& [pos, members] : at )
    {
        // Wires never join buses, but pins, junctions and labels join both
        bool joinsBus = std::any_of( members.begin(), members.end(),
                                     []( const MEMBER& m )
                                     {
                                         return m.end < 0;
                                     } );

        if( joinsBus && busEnds.count( pos ) )
            members.push_back( { BUS_SIGNAL, nullptr, -1, true } );

        for( const auto& [wire, signal] : wires )
        {
            if( passesThrough( wire, pos ) )
                members.push_back( { signal, wire, -1, true } );
        }

        for( SCH_LINE* bus : buses )
        {
            if( joinsBus && passesThrough( bus, pos ) )
                members.push_back( { BUS_SIGNAL, bus, -1, true } );
        }
    }

    // A side is free when nothing of any signal lies there
    auto isFree = [&]( const VECTOR2I& aPos )
    {
        auto onLine = [&]( SCH_LINE* aLine )
        {
            return IsPointOnSegment( aLine->GetStartPoint(), aLine->GetEndPoint(), aPos );
        };

        return !at.count( aPos ) && std::none_of( buses.begin(), buses.end(), onLine )
               && std::none_of( wires.begin(), wires.end(),
                                [&]( const std::pair<SCH_LINE*, int>& aWire )
                                {
                                    return onLine( aWire.first );
                                } );
    };

    // A wire bent earlier may have handed this end to one of its new pieces
    auto endAt = [&]( const MEMBER* aMember, const VECTOR2I& aPos ) -> std::pair<SCH_LINE*, int>
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( aMember->item );

        if( wireEnd( line, aMember->end ) == aPos )
            return { line, aMember->end };

        for( const auto& [wire, signal] : wires )
        {
            for( int end : { 0, 1 } )
            {
                if( signal == aMember->signal && wireEnd( wire, end ) == aPos )
                    return { wire, end };
            }
        }

        return { line, aMember->end };
    };

    for( const auto& [pos, members] : at )
    {
        std::map<int, std::vector<const MEMBER*>> groups;
        std::set<int>                             fixed;
        std::set<int>                             pinned;

        for( const MEMBER& m : members )
        {
            groups[m.signal].push_back( &m );

            if( m.fixed )
                fixed.insert( m.signal );

            if( m.fixed && !m.item )
                pinned.insert( m.signal );
        }

        // Pins of two signals on one spot cannot be parted without moving a symbol
        if( groups.size() < 2 || pinned.size() > 1 )
            continue;

        int keeper = groups.begin()->first;

        for( const auto& [signal, group] : groups )
        {
            if( fixed.count( signal ) > fixed.count( keeper )
                || ( fixed.count( signal ) == fixed.count( keeper ) && group.size() > groups.at( keeper ).size() ) )
            {
                keeper = signal;
            }
        }

        if( !pinned.empty() )
            keeper = *pinned.begin();

        int step = 0;

        for( const auto& [signal, group] : groups )
        {
            if( signal == keeper )
                continue;

            VECTOR2I moved = pos;

            if( fixed.count( signal ) )
            {
                // Lines of this signal passing through bend around the point, taking its other items along
                std::optional<VECTOR2I> side = bendAround( aSheet, signal, pos, wires, isFree );

                if( !side )
                    continue;

                moved = pos + *side;
            }
            else
            {
                // A wire end steps back along its wire, taking the rest along; otherwise all shift diagonally
                auto end = std::find_if( group.begin(), group.end(),
                                         []( const MEMBER* m )
                                         {
                                             return m->end >= 0;
                                         } );

                bool oneEnd = std::count_if( group.begin(), group.end(),
                                             []( const MEMBER* m )
                                             {
                                                 return m->end >= 0;
                                             } )
                              == 1;

                if( oneEnd )
                {
                    auto [line, lineEnd] = endAt( *end, pos );
                    moved = stepIn( line, lineEnd );
                }

                // The diagonal step skips spots another item already holds
                if( moved == pos )
                {
                    do
                    {
                        ++step;
                        moved = pos + VECTOR2I( step * SIGNAL_GAP, step * SIGNAL_GAP );
                    } while( !isFree( moved ) && step < MAX_DIAGONAL_STEPS );
                }
            }

            for( const MEMBER* m : group )
            {
                if( m->end >= 0 )
                {
                    auto [line, lineEnd] = endAt( m, pos );
                    setWireEnd( line, lineEnd, moved );
                }
                else if( !m->fixed )
                {
                    m->item->SetPosition( moved );
                }
            }
        }
    }
}


std::optional<VECTOR2I> TARGET3001_SCH_BUILDER::bendAround( int aSheet, int aSignal, const VECTOR2I& aPos,
                                                            std::vector<std::pair<SCH_LINE*, int>>&       aWires,
                                                            const std::function<bool( const VECTOR2I& )>& aIsFree )
{
    std::vector<SCH_LINE*> lines;

    for( const auto& [wire, signal] : aWires )
    {
        if( signal == aSignal && passesThrough( wire, aPos ) )
            lines.push_back( wire );
    }

    if( lines.empty() )
        return std::nullopt;

    VECTOR2I dir = lines[0]->GetEndPoint() - lines[0]->GetStartPoint();

    for( SCH_LINE* line : lines )
    {
        VECTOR2I span = line->GetEndPoint() - line->GetStartPoint();

        // The bend needs room on both sides, and the point's other items must sit on every bent line
        if( span.Cross( dir ) || ( aPos - line->GetStartPoint() ).EuclideanNorm() <= 2 * SIGNAL_GAP
            || ( line->GetEndPoint() - aPos ).EuclideanNorm() <= 2 * SIGNAL_GAP )
        {
            return std::nullopt;
        }
    }

    VECTOR2I normal = VECTOR2I( -dir.y, dir.x ).Resize( SIGNAL_GAP );
    VECTOR2I side = normal;

    for( int k : { 1, -1, 2, -2, 3, -3 } )
    {
        if( aIsFree( aPos + normal * k ) )
        {
            side = normal * k;
            break;
        }
    }

    for( SCH_LINE* line : lines )
    {
        VECTOR2I along = ( line->GetEndPoint() - line->GetStartPoint() ).Resize( SIGNAL_GAP );
        VECTOR2I before = aPos - along;
        VECTOR2I after = aPos + along;
        VECTOR2I end = line->GetEndPoint();

        line->SetEndPoint( before );

        for( const auto& [from, to] : { std::pair( before, before + side ), std::pair( before + side, after + side ),
                                        std::pair( after + side, after ), std::pair( after, end ) } )
        {
            SCH_LINE* piece = new SCH_LINE( *line );
            piece->SetStartPoint( from );
            piece->SetEndPoint( to );
            addNetItem( aSheet, piece, aSignal );
            aWires.emplace_back( piece, aSignal );
        }
    }

    return side;
}


void TARGET3001_SCH_BUILDER::addFreeItem( const ELEMENT& aElem )
{
    int sheet = sheetOf( aElem );

    if( sheet == 0 )
        return;

    if( aElem.type == ELEMENT_TYPE::SYMBOL && aElem.symbol )
    {
        addFreeSymbol( aElem );
        return;
    }

    addToSheet( sheet, makeGraphic( aElem, VECTOR2I( 0, 0 ), false ) );
}


void TARGET3001_SCH_BUILDER::addFreeSymbol( const ELEMENT& aSymbol )
{
    // A symbol without a component draws its placeholder texts as stored and is not a part
    wxString reference = wxString::Format( wxS( "#SYM%d" ), ++m_freeSymbolCount );
    wxString symName = LIB_ID::FixIllegalChars( reference.Mid( 1 ), false ).wx_str();

    LIB_SYMBOL lib( symName );
    lib.SetLibId( LIB_ID( LIB_NAME, symName ) );
    lib.GetReferenceField().SetText( reference );
    addGateBodies( &lib, { &aSymbol }, false );

    SCH_SHEET_PATH path;
    path.push_back( m_sheets.at( sheetOf( aSymbol ) ) );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( lib, lib.GetLibId(), &path, 1, 0, toSch( aSymbol.base.x, aSymbol.base.y ) );
    symbol->SetLibSymbol( new LIB_SYMBOL( lib ) );
    symbol->SetRef( &path, reference );
    symbol->GetField( FIELD_T::REFERENCE )->SetVisible( false );
    symbol->GetField( FIELD_T::VALUE )->SetVisible( false );
    symbol->SetExcludedFromBOM( true );
    symbol->SetExcludedFromBoard( true );
    symbol->SetExcludedFromSim( true );
    addToSheet( sheetOf( aSymbol ), symbol );

    for( const ELEMENT& child : aSymbol.symbol->children )
    {
        if( SCH_BITMAP* picture = makePicture( child ) )
            addToSheet( sheetOf( aSymbol ), picture );
    }
}


SCH_SCREEN* TARGET3001_SCH_BUILDER::screen( int aSheet )
{
    auto it = m_sheets.find( aSheet );

    if( it != m_sheets.end() )
        return it->second->GetScreen();

    SCH_SHEET* sheet;

    // The first sheet in use holds its content in the root sheet
    if( m_sheets.empty() )
    {
        sheet = m_rootSheet;
    }
    else
    {
        sheet = new SCH_SHEET( &m_schematic->Root() );
        SCH_SCREEN* newScreen = new SCH_SCREEN( m_schematic );

        wxFileName fn( m_fileName );
        fn.SetName( fn.GetName() + wxString::Format( wxS( "_%d" ), aSheet ) );
        fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

        newScreen->SetFileName( fn.GetFullPath() );
        sheet->SetScreen( newScreen );
        sheet->SetFileName( fn.GetFullName() );
    }

    wxString name;

    if( aSheet - 1 < int( m_project.settings.sheetNames.size() ) )
        name = ToWx( m_project.settings.sheetNames[aSheet - 1] );

    if( name.IsEmpty() )
        name = wxString::Format( wxS( "Sheet %d" ), aSheet );

    if( sheet != m_rootSheet )
        sheet->SetName( name );

    m_sheets[aSheet] = sheet;
    return sheet->GetScreen();
}


void TARGET3001_SCH_BUILDER::finishSheets()
{
    const int margin = schIUScale.mmToIU( PAGE_MARGIN_MM );

    for( auto& [sheet, items] : m_pending )
    {
        SCH_SCREEN* scr = screen( sheet );
        BOX2I       extent = m_extents[sheet];

        // Target sheets are centred on their origin; KiCad pages start at the top left corner
        auto snap = []( int v )
        {
            return KiROUND( double( v ) / PLACEMENT_GRID ) * PLACEMENT_GRID;
        };

        VECTOR2I offset( snap( margin - extent.GetLeft() ), snap( margin - extent.GetTop() ) );

        PAGE_INFO page( PAGE_SIZE_TYPE::User );
        page.SetWidthMM( std::max( 297.0, schIUScale.IUTomm( extent.GetWidth() + 2 * margin ) ) );
        page.SetHeightMM( std::max( 210.0, schIUScale.IUTomm( extent.GetHeight() + 2 * margin ) ) );
        scr->SetPageSettings( page );

        for( SCH_ITEM* item : items )
        {
            item->Move( offset );
            scr->Append( item );
        }
    }

    if( m_sheets.size() > 1 )
    {
        std::vector<SCH_SHEET*> sheets;

        for( const auto& [number, sheet] : m_sheets )
            sheets.push_back( sheet );

        m_schematic->SetTopLevelSheets( sheets );
    }

    int page = 1;

    for( const auto& [number, sheet] : m_sheets )
        sheet->GetScreen()->SetPageNumber( wxString::Format( wxS( "%d" ), page++ ) );
}
