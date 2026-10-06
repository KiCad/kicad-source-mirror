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

#include <sch_io/easypc/easypc_sch_builder.h>
#include <sch_io/easypc/easypc_sch_items.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <numeric>
#include <utility>

#include <io/easypc/easypc_classes_connectivity.h>
#include <io/easypc/easypc_classes_geometry.h>
#include <io/easypc/easypc_classes_geometry_pad.h>
#include <io/easypc/easypc_classes_library.h>
#include <io/easypc/easypc_classes_root.h>
#include <io/easypc/easypc_classes_styles.h>
#include <io/easypc/easypc_design_settings.h>
#include <io/easypc/easypc_document.h>
#include <io/easypc/easypc_text_metrics.h>
#include <io/easypc/easypc_units.h>

#include <bus_alias.h>
#include <geometry/seg.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <page_info.h>
#include <reporter.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <schematic.h>
#include <string_utils.h>
#include <transform.h>

#include <wx/regex.h>


namespace EASYPC
{

/// The sheet origin snaps to 0.1 inch, keeping items on Easy-PC grids on KiCad's 50 mil grid
static constexpr int32_t SHEET_ORIGIN_GRID = 25400;

static constexpr int32_t PAGE_MARGIN = static_cast<int32_t>( PAGE_MARGIN_MM * 10000 );

/// Offset that keeps a wire off another net's connection point, far below drawing resolution
static constexpr int32_t DETOUR = 10;


bool IsNamedNet( const wxString& aName )
{
    // Automatic names are N or n and digits, optionally with a "!sheet!" suffix
    static wxRegEx automatic( wxS( "^[Nn][0-9]+(![^!]*!)?$" ), wxRE_ADVANCED );

    return !automatic.Matches( aName );
}


void SCH_NET_NAMES::AddSheet( const DESIGN_DOCUMENT& aDoc )
{
    std::set<wxString> names;

    for( const std::unique_ptr<OBJECT>& obj : aDoc.Objects )
    {
        const SOURCE_NET* net = dynamic_cast<const SOURCE_NET*>( obj.get() );

        if( net && IsNamedNet( net->Name ) )
        {
            m_canonical.emplace( net->Name.Upper(), net->Name );
            names.insert( net->Name.Upper() );
        }
    }

    for( const wxString& name : names )
        m_sheetCount[name]++;
}


bool SCH_NET_NAMES::Shared( const wxString& aName ) const
{
    auto it = m_sheetCount.find( aName.Upper() );
    return IsNamedNet( aName ) && it != m_sheetCount.end() && it->second > 1;
}


wxString SCH_NET_NAMES::Name( const wxString& aStoredName ) const
{
    auto it = m_canonical.find( aStoredName.Upper() );
    return !IsNamedNet( aStoredName ) || it == m_canonical.end() ? aStoredName : it->second;
}


/// Symbol point to design point: subtract the symbol origin, mirror in X, rotate anticlockwise, add the position
static POINT32 placePoint( const SOURCE_SYMBOL_INSTANCE& aInst, const SOURCE_SYMBOL& aSym, const POINT32& aLocal )
{
    int64_t x = static_cast<int64_t>( aLocal.X ) - aSym.OriginX;
    int64_t y = static_cast<int64_t>( aLocal.Y ) - aSym.OriginY;
    int32_t rx = 0;
    int32_t ry = 0;

    if( aInst.Mirrored )
        x = -x;

    // A rotated offset keeps its length, which must fit the file's 32-bit coordinates
    if( std::hypot( static_cast<double>( x ), static_cast<double>( y ) ) > INT32_MAX )
        THROW_IO_ERROR( wxString::Format( _( "Easy-PC symbol %s has a point too far from its origin" ), aSym.Name ) );

    PointRotate( static_cast<double>( x ), static_cast<double>( y ), aInst.Angle, 0.0, 0.0, rx, ry );

    int64_t px = static_cast<int64_t>( aInst.Position.X ) + rx;
    int64_t py = static_cast<int64_t>( aInst.Position.Y ) + ry;

    if( px < INT32_MIN || px > INT32_MAX || py < INT32_MIN || py > INT32_MAX )
        THROW_IO_ERROR( wxString::Format( _( "Easy-PC symbol %s has a point outside the design" ), aSym.Name ) );

    return POINT32{ static_cast<int32_t>( px ), static_cast<int32_t>( py ) };
}


/// Gates the import draws: placed ones, and unplaced ones whose pins are still on nets
static bool isDrawn( const SOURCE_SYMBOL_INSTANCE& aInst )
{
    if( aInst.Placed )
        return true;

    for( SOURCE_PAD_INSTANCE* pad : TypedItems<SOURCE_PAD_INSTANCE>( aInst.Pads ) )
    {
        if( pad->Node )
            return true;
    }

    return false;
}


static const SOURCE_NET* netOf( const OBJECT* aNode )
{
    const SOURCE_NODE* node = dynamic_cast<const SOURCE_NODE*>( aNode );
    return node ? dynamic_cast<const SOURCE_NET*>( node->Parent ) : nullptr;
}


static const SOURCE_TEXT_STYLE* defaultTextStyle( const SOURCE_DESIGN& aDesign )
{
    return aDesign.Defaults ? dynamic_cast<const SOURCE_TEXT_STYLE*>( aDesign.Defaults->TextStyle ) : nullptr;
}


/// Package attributes then the instance's own, an instance attribute overriding the package's of the same name
static std::vector<const SOURCE_ATTRIBUTE*> partAttributes( const SOURCE_COMPONENT_INSTANCE& aComp,
                                                            const SOURCE_COMPONENT&          aPart )
{
    std::vector<const SOURCE_ATTRIBUTE*> out;
    std::vector<SOURCE_ATTRIBUTE*>       own = TypedItems<SOURCE_ATTRIBUTE>( aComp.Attributes );

    for( SOURCE_ATTRIBUTE* attr : TypedItems<SOURCE_ATTRIBUTE>( aPart.Board ? aPart.Board->Attributes : nullptr ) )
    {
        auto over = std::find_if( own.begin(), own.end(),
                                  [&]( const SOURCE_ATTRIBUTE* aMine )
                                  {
                                      return aMine->AttributeName && attr->AttributeName
                                             && aMine->AttributeName->Name.CmpNoCase( attr->AttributeName->Name ) == 0;
                                  } );

        out.push_back( over != own.end() ? *over : attr );
    }

    for( const SOURCE_ATTRIBUTE* mine : own )
    {
        if( std::find( out.begin(), out.end(), mine ) == out.end() )
            out.push_back( mine );
    }

    return out;
}


/// The value of the part's first attribute named aName, or empty
static wxString attributeValue( const SOURCE_COMPONENT_INSTANCE& aComp, const SOURCE_COMPONENT& aPart,
                                const wxString& aName )
{
    for( const SOURCE_ATTRIBUTE* attr : partAttributes( aComp, aPart ) )
    {
        if( attr->AttributeName && attr->AttributeName->Name.CmpNoCase( aName ) == 0 )
            return attr->Value;
    }

    return wxEmptyString;
}


/// One line of the text a value position shows
struct VALUE_LINE
{
    FIELD_T  Field;
    wxString Name;
    wxString Text;
};


static std::vector<VALUE_LINE> valueLines( const SOURCE_VALUE_POSITION& aVp, const SOURCE_COMPONENT_INSTANCE& aComp,
                                           const SOURCE_COMPONENT& aPart, const SOURCE_SYMBOL_INSTANCE& aInst )
{
    std::vector<VALUE_LINE> lines;
    uint64_t                shown = aVp.Types & aComp.ValueTypes;
    const SOURCE_SYMBOL*    sym = dynamic_cast<const SOURCE_SYMBOL*>( aInst.Symbol );

    auto add = [&]( FIELD_T aField, const wxString& aName, const wxString& aText )
    {
        if( !aText.IsEmpty() )
            lines.push_back( { aField, aName, aText } );
    };

    if( shown & VALUE_REFERENCE_NAME )
        add( FIELD_T::REFERENCE, wxEmptyString, aComp.Name );

    if( shown & VALUE_COMPONENT_NAME )
        add( FIELD_T::USER, wxS( "Component" ), aPart.Schematic ? aPart.Schematic->Name : wxString() );

    if( shown & VALUE_PACKAGE_NAME )
        add( FIELD_T::USER, wxS( "Package" ), aPart.Board ? aPart.Board->Package : wxString() );

    if( shown & VALUE_SYMBOL_NAME )
        add( FIELD_T::USER, wxS( "Symbol" ), sym ? sym->Name : wxString() );

    if( shown & VALUE_DESCRIPTION )
        add( FIELD_T::USER, wxS( "Description" ), aPart.Schematic ? aPart.Schematic->Description : wxString() );

    // Values is not masked by the part's value types; each displayed attribute adds its value
    for( uint64_t kind : { VALUE_VALUES, VALUE_ATTRIBUTE } )
    {
        if( !( aVp.Types & kind ) )
            continue;

        for( const SOURCE_ATTRIBUTE* attr : partAttributes( aComp, aPart ) )
        {
            if( !attr->AttributeName
                || ( kind == VALUE_VALUES ? !attr->Displayed
                                          : attr->AttributeName->Name.CmpNoCase( aVp.Attribute ) != 0 ) )
            {
                continue;
            }

            bool isValue = attr->AttributeName->Name.CmpNoCase( wxS( "Value" ) ) == 0;
            add( isValue ? FIELD_T::VALUE : FIELD_T::USER, attr->AttributeName->Name, attr->Value );
        }
    }

    return lines;
}


static wxString joinLines( const std::vector<VALUE_LINE>& aLines )
{
    wxString whole;

    for( const VALUE_LINE& line : aLines )
        whole += ( whole.IsEmpty() ? wxString() : wxString( wxS( "\n" ) ) ) + line.Text;

    return whole;
}


SCH_BUILDER::SCH_BUILDER( SCHEMATIC* aSchematic, const SCH_NET_NAMES& aNames, REPORTER* aReporter ) :
        m_schematic( aSchematic ),
        m_names( aNames ),
        m_reporter( aReporter )
{
}


void SCH_BUILDER::SetProjectSymbols( const std::vector<const DESIGN_DOCUMENT*>& aDocs )
{
    m_projectSymbols.clear();

    for( const DESIGN_DOCUMENT* doc : aDocs )
    {
        for( SOURCE_SYMBOL* sym :
             TypedItems<SOURCE_SYMBOL>( static_cast<const SOURCE_DESIGN&>( *doc->Design ).Symbols ) )
            m_projectSymbols.emplace( sym->Name, sym );
    }
}


VECTOR2I SCH_BUILDER::toSheet( int32_t aX, int32_t aY ) const
{
    return SCH_FRAME{ m_originX, m_originY }.Map( aX, aY );
}


VECTOR2I SCH_BUILDER::toSheet( const POINT32& aPoint ) const
{
    return toSheet( aPoint.X, aPoint.Y );
}


static int32_t floorTo( int64_t aValue, int32_t aGrid )
{
    int64_t q = aValue / aGrid;

    if( aValue % aGrid < 0 )
        q--;

    return static_cast<int32_t>( q * aGrid );
}


void SCH_BUILDER::computeOffset( const SOURCE_DESIGN& aDesign )
{
    // A design's stored area is usually unbounded, so the page is fitted to the drawing
    int64_t minX = INT64_MAX, minY = INT64_MAX, maxX = INT64_MIN, maxY = INT64_MIN;

    auto add = [&]( int64_t aX, int64_t aY )
    {
        minX = std::min( minX, aX );
        minY = std::min( minY, aY );
        maxX = std::max( maxX, aX );
        maxY = std::max( maxY, aY );
    };

    auto addShape = [&]( const OBJECT* aShape )
    {
        if( const SOURCE_SHAPE* shape = dynamic_cast<const SOURCE_SHAPE*>( aShape ) )
        {
            for( const SOURCE_SEGMENT* seg : ShapeVertices( *shape ) )
                add( seg->X, seg->Y );
        }
    };

    // Texts reach past the items they belong to
    auto addText = [&]( const SOURCE_TEXT_POSITION& aPos, const wxString& aText )
    {
        const SOURCE_TEXT_STYLE* style = dynamic_cast<const SOURCE_TEXT_STYLE*>( aPos.TextStyle );

        if( !style || aText.IsEmpty() || !TEXT_METRICS( *style ).CanMeasure() )
            return;

        BOX2I box = TEXT_METRICS( *style ).TextBox( aText, VECTOR2I( aPos.Position.X, aPos.Position.Y ), aPos.Rotation,
                                                    aPos.Mirrored, aPos.Alignment );
        add( box.GetLeft(), box.GetTop() );
        add( box.GetRight(), box.GetBottom() );
    };

    for( SOURCE_NET* net : TypedItems<SOURCE_NET>( aDesign.Nets ) )
    {
        for( SOURCE_NODE* node : TypedItems<SOURCE_NODE>( net->Nodes ) )
        {
            const SOURCE_NET_NAME* name = dynamic_cast<const SOURCE_NET_NAME*>( node->NetName );

            add( node->Position.X, node->Position.Y );

            if( name && name->Displayed )
                addText( *name, net->Name );
        }

        for( SOURCE_CONNECTION* conn : TypedItems<SOURCE_CONNECTION>( net->Connections ) )
            addShape( conn->Track );
    }

    for( SOURCE_COMPONENT_INSTANCE* comp : TypedItems<SOURCE_COMPONENT_INSTANCE>( aDesign.ComponentInstances ) )
    {
        const SOURCE_COMPONENT* part = dynamic_cast<const SOURCE_COMPONENT*>( comp->Component );

        for( SOURCE_SYMBOL_INSTANCE* inst : TypedItems<SOURCE_SYMBOL_INSTANCE>( comp->SymbolInstances ) )
        {
            const SOURCE_SYMBOL* sym = dynamic_cast<const SOURCE_SYMBOL*>( inst->Symbol );

            if( !isDrawn( *inst ) )
                continue;

            add( inst->Position.X, inst->Position.Y );

            // A drawing frame is a symbol whose outline reaches far from its origin
            for( SOURCE_FREE_COPPER* copper : TypedItems<SOURCE_FREE_COPPER>( sym ? sym->Shapes : nullptr ) )
            {
                if( const SOURCE_SHAPE* shape = dynamic_cast<const SOURCE_SHAPE*>( copper->Shape ) )
                {
                    for( const SOURCE_SEGMENT* seg : ShapeVertices( *shape ) )
                    {
                        POINT32 p = placePoint( *inst, *sym, POINT32{ seg->X, seg->Y } );
                        add( p.X, p.Y );
                    }
                }
            }

            for( SOURCE_FREE_TEXT* text : TypedItems<SOURCE_FREE_TEXT>( sym ? sym->Texts : nullptr ) )
            {
                POINT32 p = placePoint( *inst, *sym, text->Position );
                add( p.X, p.Y );
            }

            for( SOURCE_VALUE_POSITION* vp :
                 TypedItems<SOURCE_VALUE_POSITION>( part ? inst->ValuePositions : nullptr ) )
            {
                if( vp->Displayed )
                    addText( *vp, joinLines( valueLines( *vp, *comp, *part, *inst ) ) );
            }
        }
    }

    for( SOURCE_FREE_TEXT* text : TypedItems<SOURCE_FREE_TEXT>( aDesign.Texts ) )
    {
        add( text->Position.X, text->Position.Y );
        addText( *text, text->Text );
    }

    for( SOURCE_FREE_COPPER* copper : TypedItems<SOURCE_FREE_COPPER>( aDesign.Coppers ) )
        addShape( copper->Shape );

    for( SOURCE_BUS* bus : TypedItems<SOURCE_BUS>( aDesign.Buses ) )
    {
        addShape( bus->Shape );

        for( SOURCE_BUS_TERMINAL* term : TypedItems<SOURCE_BUS_TERMINAL>( bus->Terminals ) )
        {
            add( term->Position.X, term->Position.Y );
            add( term->BusEndPosition.X, term->BusEndPosition.Y );
        }
    }

    if( minX > maxX )
        minX = maxX = minY = maxY = 0;

    m_originX = floorTo( minX - PAGE_MARGIN, SHEET_ORIGIN_GRID );
    m_originY = -floorTo( -( maxY + PAGE_MARGIN ), SHEET_ORIGIN_GRID );

    // The grown extents start at the sheet origin, so the page holds the grid rounding as well
    BOX2L extents( VECTOR2L( ( m_originX + PAGE_MARGIN ) * 100LL, minY * 100LL ),
                   VECTOR2L( ( maxX - m_originX - PAGE_MARGIN ) * 100LL, ( m_originY - PAGE_MARGIN - minY ) * 100LL ) );

    m_screen->SetPageSettings( PageForExtents( extents ) );
}


void SCH_BUILDER::BuildSheet( const DESIGN_DOCUMENT& aDoc, SCH_SHEET* aSheet, const SCH_SHEET_PATH& aSheetPath )
{
    const SOURCE_DESIGN& design = static_cast<const SOURCE_DESIGN&>( *aDoc.Design );

    m_screen = aSheet->GetScreen();
    m_design = &design;
    m_powerNodes.clear();
    m_powerNets.clear();
    m_labelledNets.clear();

    computeOffset( design );
    m_keepUpright = TextStaysUpright( design, true );

    TITLE_BLOCK titleBlock;
    FillTitleBlock( aDoc, titleBlock );
    m_screen->SetTitleBlock( titleBlock );

    buildSymbols( design, aSheetPath );
    buildWires( design );
    buildLabels( design );
    buildCommonPinCopies( design );
    buildBuses( design );

    SCH_FRAME frame{ m_originX, m_originY };

    for( SOURCE_FREE_TEXT* text : TypedItems<SOURCE_FREE_TEXT>( design.Texts ) )
        m_screen->Append( ConvertFreeText( *text, frame, LAYER_NOTES, 0, m_keepUpright ).release() );

    for( SOURCE_FREE_COPPER* copper : TypedItems<SOURCE_FREE_COPPER>( design.Coppers ) )
    {
        for( std::unique_ptr<SCH_SHAPE>& shape : ConvertShapeItem( *copper, frame, LAYER_NOTES ) )
            m_screen->Append( shape.release() );
    }
}


/// KiCad transform for a stored placement: mirror in X, then rotate anticlockwise (Y up)
static TRANSFORM placementTransform( int32_t aAngle, bool aMirrored )
{
    int32_t angle = NormalizeAngle( aAngle );
    int     c = angle == 180000 ? -1 : ( angle == 90000 || angle == 270000 ? 0 : 1 );
    int     s = angle == 90000 ? 1 : ( angle == 270000 ? -1 : 0 );

    // Library items are Y down, so the anticlockwise rotation becomes R(-angle) and the mirror keeps its sign
    return aMirrored ? TRANSFORM( -c, s, s, c ) : TRANSFORM( c, s, -s, c );
}


/// Schematic-only part: no package, an empty package name, or no gate pin mapped to a pad
static bool isScmOnly( const SOURCE_COMPONENT& aPart )
{
    if( !aPart.Board || aPart.Board->Package.IsEmpty() )
        return true;

    for( SOURCE_GATE_MAP* map : TypedItems<SOURCE_GATE_MAP>( aPart.Board->GateMaps ) )
    {
        for( SOURCE_GATE_PIN* pin : TypedItems<SOURCE_GATE_PIN>( map->Pins ) )
        {
            if( !pin->PcbSymbolPin.IsEmpty() && pin->PcbSymbolPin != wxS( "-1" ) )
                return false;
        }

        for( int32_t pad : map->PcbSymbolPins )
        {
            if( pad != -1 )
                return false;
        }
    }

    return true;
}


void SCH_BUILDER::buildSymbols( const SOURCE_DESIGN& aDesign, const SCH_SHEET_PATH& aSheetPath )
{
    SCH_FRAME                                   frame{ m_originX, m_originY };
    std::map<wxString, const SOURCE_COMPONENT*> libNames;
    std::map<wxString, const SOURCE_SYMBOL*>    designSymbols;

    // The first symbol of each name wins, as a front-to-back search would find it
    for( SOURCE_SYMBOL* sym : TypedItems<SOURCE_SYMBOL>( aDesign.Symbols ) )
        designSymbols.emplace( sym->Name, sym );

    for( SOURCE_COMPONENT_INSTANCE* comp : TypedItems<SOURCE_COMPONENT_INSTANCE>( aDesign.ComponentInstances ) )
    {
        const SOURCE_COMPONENT* part = dynamic_cast<const SOURCE_COMPONENT*>( comp->Component );

        if( !part || !part->Schematic )
            THROW_IO_ERROR( wxString::Format( _( "Component instance %s has no schematic component" ), comp->Name ) );

        std::vector<SOURCE_SYMBOL_INSTANCE*> instances = TypedItems<SOURCE_SYMBOL_INSTANCE>( comp->SymbolInstances );
        std::vector<SOURCE_GATE*>            gates = TypedItems<SOURCE_GATE>( part->Schematic->Gates );
        SCH_COMPONENT_SOURCE                 source;

        source.Schematic = part->Schematic;
        source.Board = part->Board;
        source.PinTexts = false;
        source.Gates.assign( gates.size(), nullptr );

        if( part->Board && !part->Board->Symbol.IsEmpty() )
            source.Footprint = part->Board->Symbol;

        for( SOURCE_SYMBOL_INSTANCE* inst : instances )
        {
            if( inst->Gate >= 0 && inst->Gate < static_cast<int>( gates.size() ) )
                source.Gates[inst->Gate] = dynamic_cast<const SOURCE_SYMBOL*>( inst->Symbol );
        }

        // A gate not placed here is drawn from this design's symbol of the gate's name, else another sheet's
        for( size_t g = 0; g < gates.size(); ++g )
        {
            if( source.Gates[g] )
                continue;

            if( auto it = designSymbols.find( gates[g]->Symbol ); it != designSymbols.end() )
                source.Gates[g] = it->second;
            else if( auto pit = m_projectSymbols.find( gates[g]->Symbol ); pit != m_projectSymbols.end() )
                source.Gates[g] = pit->second;
        }

        int               pinCount = 0;
        const SOURCE_NET* signalNet = nullptr;

        for( SOURCE_SYMBOL_INSTANCE* inst : instances )
        {
            for( SOURCE_PAD_INSTANCE* pad : TypedItems<SOURCE_PAD_INSTANCE>( inst->Pads ) )
            {
                pinCount++;

                if( dynamic_cast<const SOURCE_NODE*>( pad->Node ) )
                    signalNet = netOf( pad->Node );
            }
        }

        bool scmOnly = isScmOnly( *part );

        // A schematic-only single gate with at most one pin is a signal symbol naming its net
        bool power = scmOnly && gates.size() == 1 && pinCount < 2 && signalNet && IsNamedNet( signalNet->Name );

        wxString netName = power ? m_names.Name( signalNet->Name ) : wxString();
        wxString libName = power ? netName : part->Schematic->Name;
        wxString base = libName;

        // Different parts may share a component name or a power net in one design; each keeps its own symbol
        for( int n = 2; libNames.count( libName ) && libNames[libName] != part; ++n )
            libName = wxString::Format( wxS( "%s_%d" ), base, n );

        libNames[libName] = part;

        LIB_ID                      libId( wxS( "easypc" ), EscapeString( libName, CTX_LIBID ) );
        std::unique_ptr<LIB_SYMBOL> libSymbol = ConvertComponentSymbol( source, libId );

        if( power )
        {
            libSymbol->SetGlobalPower();

            for( SOURCE_SYMBOL_INSTANCE* inst : instances )
            {
                for( SOURCE_PAD_INSTANCE* pad : TypedItems<SOURCE_PAD_INSTANCE>( inst->Pads ) )
                {
                    if( const SOURCE_NODE* node = dynamic_cast<const SOURCE_NODE*>( pad->Node ) )
                    {
                        m_powerNodes.insert( node );
                        m_powerNets.insert( netName.Upper() );
                    }
                }
            }

            libSymbol->GetValueField().SetText( ToKiCadMarkup( netName ) );

            // KiCad names a net from a power symbol only through a power input pin
            for( SCH_PIN* pin : libSymbol->GetGraphicalPins( 0, 0 ) )
            {
                pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
                pin->SetName( ToKiCadMarkup( netName ) );
            }
        }

        for( SOURCE_SYMBOL_INSTANCE* inst : instances )
        {
            // KiCad has no unplaced unit; a gate whose pins are still on nets is drawn where it is stored
            if( !isDrawn( *inst ) )
                continue;

            if( !inst->Placed )
            {
                m_reporter->Report( wxString::Format( _( "Unplaced gate %d of %s has connected pins and is placed "
                                                         "at its stored position" ),
                                                      inst->Gate + 1, comp->Name ),
                                    RPT_SEVERITY_WARNING );
            }

            int         unit = inst->Gate >= 0 ? inst->Gate + 1 : 1;
            SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &aSheetPath, unit, 0, toSheet( inst->Position ) );
            wxString    ref = power ? wxString::Format( wxS( "#PWR%04d" ), ++m_powerCount ) : comp->Name;
            wxString    value = power ? netName : attributeValue( *comp, *part, wxS( "Value" ) );

            symbol->SetLibSymbol( new LIB_SYMBOL( *libSymbol ) );
            symbol->SetTransform( placementTransform( inst->Angle, inst->Mirrored ) );
            symbol->GetField( FIELD_T::REFERENCE )->SetText( ref );
            symbol->SetRef( &aSheetPath, ref );
            symbol->SetValueFieldText( ToKiCadMarkup( power || !value.IsEmpty() ? value : part->Schematic->Name ) );

            if( scmOnly || ( comp->Suppress & SUPPRESS_SCM_PCB_TRANSLATION ) )
                symbol->SetExcludedFromBoard( true );

            if( scmOnly || ( comp->Suppress & SUPPRESS_PARTS_LIST ) )
                symbol->SetExcludedFromBOM( true );

            placeFields( *symbol, *comp, *inst, power, frame );
            m_screen->Append( symbol );
            placePinItems( *part, *inst, frame );
        }
    }
}


/// A symbol-local text position placed on the sheet with the instance transform
static SOURCE_TEXT_POSITION placedText( const SOURCE_SYMBOL_INSTANCE& aInst, const SOURCE_SYMBOL& aSym,
                                        const SOURCE_TEXT_POSITION& aLocal )
{
    SOURCE_TEXT_POSITION out;
    out.TextStyle = aLocal.TextStyle;
    out.Alignment = aLocal.Alignment;
    out.Position = placePoint( aInst, aSym, aLocal.Position );
    out.Mirrored = aLocal.Mirrored != aInst.Mirrored;
    out.Rotation = NormalizeAngle( ( aInst.Mirrored ? -aLocal.Rotation : aLocal.Rotation ) + aInst.Angle );
    return out;
}


void SCH_BUILDER::placePinItems( const SOURCE_COMPONENT& aPart, const SOURCE_SYMBOL_INSTANCE& aInst,
                                 const SCH_FRAME& aFrame )
{
    const SOURCE_SYMBOL*      sym = dynamic_cast<const SOURCE_SYMBOL*>( aInst.Symbol );
    std::vector<SOURCE_GATE*> gates = TypedItems<SOURCE_GATE>( aPart.Schematic ? aPart.Schematic->Gates : nullptr );
    const SOURCE_GATE*        gate =
            aInst.Gate >= 0 && aInst.Gate < static_cast<int>( gates.size() ) ? gates[aInst.Gate] : nullptr;

    auto addText = [&]( const wxString& aText, const SOURCE_TEXT_POSITION& aPos )
    {
        if( aText.IsEmpty() )
            return;

        SCH_TEXT* text = new SCH_TEXT( VECTOR2I( 0, 0 ), ToKiCadMarkup( aText ), LAYER_NOTES );
        ApplyTextPosition( *text, aPos, aFrame, m_keepUpright, aText );
        m_screen->Append( text );
    };

    for( SOURCE_PAD_INSTANCE* pad : TypedItems<SOURCE_PAD_INSTANCE>( aInst.Pads ) )
    {
        const SOURCE_FREE_PAD* freePad = dynamic_cast<const SOURCE_FREE_PAD*>( pad->Pad );

        if( !freePad )
            continue;

        int               terminal = freePad->Number - 1;
        GATE_TERMINAL     resolved = ResolveTerminal( aPart.Board, aInst.Gate, terminal );
        const SOURCE_NET* net = netOf( pad->Node );
        wxString          pinName;

        if( gate && terminal >= 0 && terminal < static_cast<int>( gate->PinNames.size() ) )
            pinName = gate->PinNames[terminal];

        // A no-connect gate pin left unwired; crosses on other unconnected pins are their pad style
        if( resolved.NoConnect && !dynamic_cast<const SOURCE_NODE*>( pad->Node ) && sym )
            m_screen->Append( new SCH_NO_CONNECT( toSheet( placePoint( aInst, *sym, freePad->Position ) ) ) );

        for( SOURCE_VALUE_POSITION* vp : TypedItems<SOURCE_VALUE_POSITION>( pad->ValuePositions ) )
        {
            if( !vp->Displayed )
                continue;

            if( vp->Types & VALUE_PIN_NAME )
                addText( pinName, *vp );
            else if( vp->Types & VALUE_PIN_NUMBER )
                addText( resolved.Display, *vp );
            else if( ( vp->Types & VALUE_NET_NAME ) && net )
                addText( m_names.Name( net->Name ), *vp );
        }

        // Below 10003 the pin texts are instances of the symbol's pin name and number
        const SOURCE_PIN_NAME_INSTANCE* name = dynamic_cast<const SOURCE_PIN_NAME_INSTANCE*>( pad->PinName );
        const SOURCE_PIN_NAME_INSTANCE* number = dynamic_cast<const SOURCE_PIN_NAME_INSTANCE*>( pad->PinNumber );

        if( sym && name && name->Visible )
        {
            if( const SOURCE_TEXT_POSITION* def = dynamic_cast<const SOURCE_TEXT_POSITION*>( name->Definition ) )
                addText( pinName, placedText( aInst, *sym, *def ) );
        }

        if( sym && number && number->Visible )
        {
            if( const SOURCE_TEXT_POSITION* def = dynamic_cast<const SOURCE_TEXT_POSITION*>( number->Definition ) )
                addText( resolved.Display, placedText( aInst, *sym, *def ) );
        }
    }
}


void SCH_BUILDER::placeFields( SCH_SYMBOL& aSymbol, const SOURCE_COMPONENT_INSTANCE& aComp,
                               const SOURCE_SYMBOL_INSTANCE& aInst, bool aPower, const SCH_FRAME& aFrame )
{
    const SOURCE_COMPONENT* part = dynamic_cast<const SOURCE_COMPONENT*>( aComp.Component );
    std::set<SCH_FIELD*>    placed;

    for( SCH_FIELD& field : aSymbol.GetFields() )
        field.SetVisible( false );

    for( SOURCE_VALUE_POSITION* vp : TypedItems<SOURCE_VALUE_POSITION>( part ? aInst.ValuePositions : nullptr ) )
    {
        if( !vp->Displayed )
            continue;

        const SOURCE_TEXT_STYLE* style = dynamic_cast<const SOURCE_TEXT_STYLE*>( vp->TextStyle );
        int                      pitch = style ? style->Height * style->InterlineHeightPercent / 100 : 0;
        std::vector<VALUE_LINE>  lines = valueLines( *vp, aComp, *part, aInst );
        wxString                 whole = joinLines( lines );

        // Text turned upright continues downwards from its turned first line
        int32_t drawnAngle = DrawsUpright( *vp, m_keepUpright ) ? UprightRotation( vp->Rotation, true ) : vp->Rotation;
        double  angle = drawnAngle * M_PI / 180000.0;
        bool    hasOther = std::any_of( lines.begin(), lines.end(),
                                        []( const VALUE_LINE& aLine )
                                        {
                                         return aLine.Field != FIELD_T::REFERENCE;
                                     } );

        for( size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex )
        {
            const VALUE_LINE& line = lines[lineIndex];
            SCH_FIELD*        field = nullptr;

            // A power symbol's value is drawn where its first text other than the reference is, else the reference
            if( aPower )
                field = line.Field != FIELD_T::REFERENCE || !hasOther ? aSymbol.GetField( FIELD_T::VALUE ) : nullptr;
            else if( line.Field != FIELD_T::USER )
                field = aSymbol.GetField( line.Field );
            else if( !( field = aSymbol.GetField( line.Name ) ) )
                field = aSymbol.AddField( SCH_FIELD( &aSymbol, FIELD_T::USER, line.Name ) );

            if( !field || placed.count( field ) )
                continue;

            if( line.Field == FIELD_T::USER && !aPower )
                field->SetText( ToKiCadMarkup( line.Text ) );

            ApplyTextPosition( *field, *vp, aFrame, m_keepUpright, whole );

            // Each later line sits one interline height below the first
            VECTOR2D          down( std::sin( angle ) * pitch * lineIndex, -std::cos( angle ) * pitch * lineIndex );
            VECTOR2I          pos = field->GetTextPos() + VECTOR2I( KiROUND( down.x ), KiROUND( -down.y ) );
            GR_TEXT_H_ALIGN_T hJustify = field->GetHorizJustify();
            GR_TEXT_V_ALIGN_T vJustify = field->GetVertJustify();

            // KiCad draws a field through its symbol's transform, so store the inverse of the sheet placement
            if( aSymbol.GetTransform().y1 )
                field->SetTextAngle( field->GetTextAngle().IsHorizontal() ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );

            field->SetPosition( pos );
            field->SetEffectiveHorizJustify( hJustify );
            field->SetEffectiveVertJustify( vJustify );
            field->SetVisible( true );
            placed.insert( field );
        }
    }
}


/// Two segments on one line sharing more than a point
static bool collinearOverlap( const SEG& aA, const SEG& aB )
{
    if( aA.A == aA.B || aB.A == aB.B )
        return false;

    VECTOR2L d = VECTOR2L( aA.B - aA.A );

    auto cross = [&]( const VECTOR2I& aPoint )
    {
        VECTOR2L r = VECTOR2L( aPoint - aA.A );
        return d.x * r.y - d.y * r.x;
    };

    auto param = [&]( const VECTOR2I& aPoint )
    {
        VECTOR2L r = VECTOR2L( aPoint - aA.A );
        return d.x * r.x + d.y * r.y;
    };

    if( cross( aB.A ) != 0 || cross( aB.B ) != 0 )
        return false;

    int64_t lo = std::max<int64_t>( 0, std::min( param( aB.A ), param( aB.B ) ) );
    int64_t hi = std::min<int64_t>( d.x * d.x + d.y * d.y, std::max( param( aB.A ), param( aB.B ) ) );

    return hi > lo;
}


void SCH_BUILDER::buildWires( const SOURCE_DESIGN& aDesign )
{
    // Every connection point with its net, null for an unconnected pin, which KiCad also connects
    std::vector<std::pair<VECTOR2I, const SOURCE_NET*>> points;
    std::map<VECTOR2I, std::set<const SOURCE_NET*>>     netsAt;
    std::map<int, std::vector<size_t>>                  pointsByX;
    std::map<int, std::vector<size_t>>                  pointsByY;

    for( SOURCE_NET* net : TypedItems<SOURCE_NET>( aDesign.Nets ) )
    {
        for( SOURCE_NODE* node : TypedItems<SOURCE_NODE>( net->Nodes ) )
            points.emplace_back( toSheet( node->Position ), net );
    }

    for( SOURCE_COMPONENT_INSTANCE* comp : TypedItems<SOURCE_COMPONENT_INSTANCE>( aDesign.ComponentInstances ) )
    {
        for( SOURCE_SYMBOL_INSTANCE* inst : TypedItems<SOURCE_SYMBOL_INSTANCE>( comp->SymbolInstances ) )
        {
            const SOURCE_SYMBOL* sym = dynamic_cast<const SOURCE_SYMBOL*>( inst->Symbol );

            for( SOURCE_PAD_INSTANCE* pad :
                 TypedItems<SOURCE_PAD_INSTANCE>( sym && isDrawn( *inst ) ? inst->Pads : nullptr ) )
            {
                if( const SOURCE_FREE_PAD* freePad = dynamic_cast<const SOURCE_FREE_PAD*>( pad->Pad );
                    freePad && !pad->Node )
                    points.emplace_back( toSheet( placePoint( *inst, *sym, freePad->Position ) ), nullptr );
            }
        }
    }

    for( size_t i = 0; i < points.size(); ++i )
    {
        netsAt[points[i].first].insert( points[i].second );
        pointsByX[points[i].first.x].push_back( i );
        pointsByY[points[i].first.y].push_back( i );
    }

    m_sharedPoints.clear();

    for( const auto& [pt, nets] : netsAt )
    {
        if( nets.size() > 1 )
            m_sharedPoints.insert( pt );
    }

    // Points SEG::Contains can accept lie within one unit of the line, so an axis-aligned wire looks in three rows
    auto candidates = [&]( const SEG& aSeg )
    {
        std::vector<size_t>                       out;
        const std::map<int, std::vector<size_t>>* index = aSeg.A.x == aSeg.B.x   ? &pointsByX
                                                          : aSeg.A.y == aSeg.B.y ? &pointsByY
                                                                                 : nullptr;
        int                                       line = index == &pointsByX ? aSeg.A.x : aSeg.A.y;

        if( !index )
        {
            out.resize( points.size() );
            std::iota( out.begin(), out.end(), 0 );
            return out;
        }

        for( int c = line - 1; c <= line + 1; ++c )
        {
            if( auto it = index->find( c ); it != index->end() )
                out.insert( out.end(), it->second.begin(), it->second.end() );
        }

        std::sort( out.begin(), out.end() );
        return out;
    };

    // Spans of earlier nets by the line they lie on, for wires of two nets drawn over each other
    using SPANS = std::vector<std::pair<SEG, int>>;

    std::map<const SOURCE_NET*, int>     netIndex;
    std::map<std::pair<int, int>, SPANS> spansOnLine;
    SPANS                                allSpans;
    std::set<VECTOR2I>                   bareEnds;

    for( SOURCE_NET* net : TypedItems<SOURCE_NET>( aDesign.Nets ) )
    {
        int index = static_cast<int>( netIndex.size() );
        netIndex[net] = index;

        for( SOURCE_NODE* node : TypedItems<SOURCE_NODE>( net->Nodes ) )
        {
            const SOURCE_JUNCTION* junction = dynamic_cast<const SOURCE_JUNCTION*>( node->Item );

            // A junction without a dot is only the end or bend of a wire
            if( junction && !junction->Style )
                bareEnds.insert( toSheet( node->Position ) );
        }

        for( SOURCE_CONNECTION* conn : TypedItems<SOURCE_CONNECTION>( net->Connections ) )
        {
            const SOURCE_TRACK*                track = dynamic_cast<const SOURCE_TRACK*>( conn->Track );
            std::vector<const SOURCE_SEGMENT*> verts =
                    track ? ShapeVertices( *track ) : std::vector<const SOURCE_SEGMENT*>();

            for( size_t i = 0; i + 1 < verts.size(); ++i )
            {
                SEG seg( toSheet( verts[i]->X, verts[i]->Y ), toSheet( verts[i + 1]->X, verts[i + 1]->Y ) );

                allSpans.emplace_back( seg, index );

                if( seg.A.x == seg.B.x )
                    spansOnLine[{ 0, seg.A.x }].emplace_back( seg, index );
                else if( seg.A.y == seg.B.y )
                    spansOnLine[{ 1, seg.A.y }].emplace_back( seg, index );
            }
        }
    }

    auto overlapsEarlierNet = [&]( const SEG& aSeg, int aNet )
    {
        static const SPANS none;
        const SPANS*       spans = &allSpans;

        // A span collinear with an axis-aligned one lies on its line exactly
        if( aSeg.A.x == aSeg.B.x || aSeg.A.y == aSeg.B.y )
        {
            auto it = spansOnLine.find( aSeg.A.x == aSeg.B.x ? std::make_pair( 0, aSeg.A.x )
                                                             : std::make_pair( 1, aSeg.A.y ) );
            spans = it == spansOnLine.end() ? &none : &it->second;
        }

        return std::any_of( spans->begin(), spans->end(),
                            [&]( const std::pair<SEG, int>& aSpan )
                            {
                                return aSpan.second < aNet && collinearOverlap( aSeg, aSpan.first );
                            } );
    };

    for( SOURCE_NET* net : TypedItems<SOURCE_NET>( aDesign.Nets ) )
    {
        for( SOURCE_CONNECTION* conn : TypedItems<SOURCE_CONNECTION>( net->Connections ) )
        {
            const SOURCE_TRACK*                track = dynamic_cast<const SOURCE_TRACK*>( conn->Track );
            std::vector<const SOURCE_SEGMENT*> verts =
                    track ? ShapeVertices( *track ) : std::vector<const SOURCE_SEGMENT*>();

            for( size_t i = 0; i + 1 < verts.size(); ++i )
            {
                const SOURCE_TRACK_SEGMENT* seg = dynamic_cast<const SOURCE_TRACK_SEGMENT*>( verts[i] );
                const SOURCE_TRACK_STYLE* style = seg ? dynamic_cast<const SOURCE_TRACK_STYLE*>( seg->Style ) : nullptr;
                VECTOR2I                  start = toSheet( verts[i]->X, verts[i]->Y );
                VECTOR2I                  end = toSheet( verts[i + 1]->X, verts[i + 1]->Y );
                VECTOR2D                  dir = VECTOR2D( end - start ).Resize( 1.0 );
                VECTOR2I                  along( KiROUND( dir.x * DETOUR ), KiROUND( dir.y * DETOUR ) );
                VECTOR2I                  across( -along.y, along.x );

                // A bare wire end on another net's point stops short of it, since the file keeps the nets apart
                auto retract = [&]( VECTOR2I& aPt, const VECTOR2I& aInward, bool aIsTrackEnd )
                {
                    const std::set<const SOURCE_NET*>& nets = netsAt[aPt];

                    if( !aIsTrackEnd || !bareEnds.count( aPt ) || nets.empty()
                        || ( nets.size() == 1 && *nets.begin() == net ) )
                    {
                        return;
                    }

                    wxString msg = _( "A wire of net %s ends on a connection of another net at " //format:allow
                                      "(%.4f, %.4f) mm; it stops %d nm short" );                 //format:allow

                    m_reporter->Report(
                            wxString::Format( msg, net->Name, aPt.x / 10000.0, aPt.y / 10000.0, DETOUR * 100 ),
                            RPT_SEVERITY_WARNING );
                    aPt += aInward;
                };

                retract( start, along, i == 0 );
                retract( end, -along, i + 2 == verts.size() );

                std::vector<VECTOR2I> path = { start };
                bool                  overlaps = overlapsEarlierNet( SEG( start, end ), netIndex.at( net ) );

                // Wires of two nets drawn over each other: the later net's wire runs alongside instead
                if( overlaps )
                {
                    m_reporter->Report( wxString::Format( _( "A wire of net %s is drawn over a wire of another "
                                                             "net; it is offset by %d nm" ),
                                                          net->Name, DETOUR * 100 ),
                                        RPT_SEVERITY_WARNING );
                    path.push_back( start + across );
                    path.push_back( end + across );
                }

                // A point lying on a wire is not connected in the file, KiCad always connects it
                std::vector<VECTOR2I> crossings;

                for( size_t k : overlaps ? std::vector<size_t>() : candidates( SEG( start, end ) ) )
                {
                    const auto& [pt, other] = points[k];

                    if( other != net && pt != start && pt != end && SEG( start, end ).Contains( pt ) )
                        crossings.push_back( pt );
                }

                std::sort( crossings.begin(), crossings.end(),
                           [&]( const VECTOR2I& l, const VECTOR2I& r )
                           {
                               return ( l - start ).SquaredEuclideanNorm() < ( r - start ).SquaredEuclideanNorm();
                           } );

                for( const VECTOR2I& pt : crossings )
                {
                    path.insert( path.end(), { pt - along, pt - along + across, pt + along + across, pt + along } );

                    wxString msg = _( "A wire of net %s passes over a connection of another net at " //format:allow
                                      "(%.4f, %.4f) mm; it detours by %d nm to stay unconnected" );  //format:allow

                    m_reporter->Report(
                            wxString::Format( msg, net->Name, pt.x / 10000.0, pt.y / 10000.0, DETOUR * 100 ),
                            RPT_SEVERITY_WARNING );
                }

                path.push_back( end );

                for( size_t k = 0; k + 1 < path.size(); ++k )
                {
                    SCH_LINE* wire = new SCH_LINE( path[k], LAYER_WIRE );
                    wire->SetEndPoint( path[k + 1] );

                    if( style )
                        wire->SetLineWidth( style->Width );

                    m_screen->Append( wire );
                }
            }
        }

        // A junction with a pad style is a drawn junction dot
        for( SOURCE_NODE* node : TypedItems<SOURCE_NODE>( net->Nodes ) )
        {
            const SOURCE_JUNCTION*  junction = dynamic_cast<const SOURCE_JUNCTION*>( node->Item );
            const SOURCE_PAD_STYLE* style =
                    junction ? dynamic_cast<const SOURCE_PAD_STYLE*>( junction->Style ) : nullptr;

            if( junction && junction->Style )
                m_screen->Append( new SCH_JUNCTION( toSheet( junction->Position ), style ? style->Size : 0 ) );
        }
    }
}


SCH_LABEL_BASE* SCH_BUILDER::addLabel( const wxString& aName, const VECTOR2I& aAt )
{
    // A name shared across a project's sheets needs a global label, and so does a
    // net a global power symbol names, since a local label of that name would be another net
    SCH_LABEL_BASE* label = nullptr;

    if( m_names.Shared( aName ) || m_powerNets.count( aName.Upper() ) )
        label = new SCH_GLOBALLABEL( aAt, ToKiCadMarkup( aName ) );
    else
        label = new SCH_LABEL( aAt, ToKiCadMarkup( aName ) );

    if( const SOURCE_TEXT_STYLE* style = defaultTextStyle( *m_design ) )
        label->SetTextSize( TextSize( *style ) );

    m_labelledNets.insert( aName.Upper() );
    return label;
}


/// Label orientation from the stored text rotation and alignment, judged by the middle of the text
static SPIN_STYLE labelSpin( const SOURCE_NET_NAME& aName, const SOURCE_NODE& aNode, const wxString& aText )
{
    const SOURCE_TEXT_STYLE* style = dynamic_cast<const SOURCE_TEXT_STYLE*>( aName.TextStyle );
    int32_t                  rot = NormalizeAngle( aName.Rotation );
    bool                     vertical = ( rot >= 45000 && rot < 135000 ) || ( rot >= 225000 && rot < 315000 );
    int                      reading = rot >= 135000 && rot < 315000 ? -1 : 1;

    // Stroke glyphs average about 0.6 of the text height in width; only the side of the node is decided by it
    double halfWidth = style ? style->Height * 0.6 * aText.length() / 2.0 : 0.0;

    if( aName.Alignment == TEXT_ALIGN_RIGHT )
        reading = -reading;
    else if( aName.Alignment == TEXT_ALIGN_CENTRE )
        halfWidth = 0.0;

    if( vertical )
        return aName.Position.Y + reading * halfWidth - aNode.Position.Y >= 0 ? SPIN_STYLE::UP : SPIN_STYLE::BOTTOM;

    return aName.Position.X + reading * halfWidth - aNode.Position.X >= 0 ? SPIN_STYLE::RIGHT : SPIN_STYLE::LEFT;
}


void SCH_BUILDER::buildLabels( const SOURCE_DESIGN& aDesign )
{
    auto place = [&]( const wxString& aName, const SOURCE_NODE& aNode, const SOURCE_NET_NAME* aStyle )
    {
        SCH_LABEL_BASE*          label = addLabel( aName, toSheet( aNode.Position ) );
        const SOURCE_TEXT_STYLE* style = dynamic_cast<const SOURCE_TEXT_STYLE*>( aStyle ? aStyle->TextStyle : nullptr );

        if( aStyle )
            label->SetSpinStyle( labelSpin( *aStyle, aNode, aName ) );

        if( style )
            label->SetTextSize( TextSize( *style ) );

        m_screen->Append( label );
    };

    for( SOURCE_NET* net : TypedItems<SOURCE_NET>( aDesign.Nets ) )
    {
        wxString                                         name = m_names.Name( net->Name );
        std::vector<SOURCE_NODE*>                        nodes = TypedItems<SOURCE_NODE>( net->Nodes );
        std::map<const SOURCE_NODE*, const SOURCE_NODE*> root;

        // A stored net is one object however it is drawn; KiCad needs every drawn piece named
        auto find = [&]( const SOURCE_NODE* aNode )
        {
            const SOURCE_NODE* r = aNode;

            while( root[r] != r )
                r = root[r];

            for( const SOURCE_NODE* n = aNode; n != r; )
                n = std::exchange( root[n], r );

            return r;
        };

        for( SOURCE_NODE* node : nodes )
            root[node] = node;

        for( SOURCE_CONNECTION* conn : TypedItems<SOURCE_CONNECTION>( net->Connections ) )
        {
            const SOURCE_NODE* a = dynamic_cast<const SOURCE_NODE*>( conn->Node1 );
            const SOURCE_NODE* b = dynamic_cast<const SOURCE_NODE*>( conn->Node2 );

            if( a && b && conn->Track && root.count( a ) && root.count( b ) )
                root[find( a )] = find( b );
        }

        std::set<const SOURCE_NODE*>                     named;
        std::map<const SOURCE_NODE*, const SOURCE_NODE*> hiddenLabelOf;
        std::vector<const SOURCE_NODE*>                  pieces;

        for( SOURCE_NODE* node : nodes )
        {
            const SOURCE_NODE*     piece = find( node );
            const SOURCE_NET_NAME* label = dynamic_cast<const SOURCE_NET_NAME*>( node->NetName );
            bool                   free = !m_sharedPoints.count( toSheet( node->Position ) );

            if( std::find( pieces.begin(), pieces.end(), piece ) == pieces.end() )
                pieces.push_back( piece );

            if( m_powerNodes.count( node ) )
                named.insert( piece );

            // A label on a point another net shares would join the two nets in KiCad
            if( label && label->Displayed && free )
            {
                place( name, *node, label );
                named.insert( piece );
            }
            else if( label && free && !hiddenLabelOf.count( piece ) )
            {
                hiddenLabelOf[piece] = node;
            }
        }

        // A single automatic piece is named by KiCad itself; anything else must carry the stored name
        if( pieces.size() < 2 && !IsNamedNet( name ) )
            continue;

        // The source draws no name here, so keep the label off pins: a junction, then a free wire end
        auto rank = []( const SOURCE_NODE* aNode )
        {
            return dynamic_cast<const SOURCE_JUNCTION*>( aNode->Item )       ? 1
                   : dynamic_cast<const SOURCE_PAD_INSTANCE*>( aNode->Item ) ? 3
                                                                             : 2;
        };

        for( const SOURCE_NODE* piece : pieces )
        {
            auto               hidden = hiddenLabelOf.find( piece );
            const SOURCE_NODE* at = hidden != hiddenLabelOf.end() ? hidden->second : nullptr;

            if( named.count( piece ) )
                continue;

            for( SOURCE_NODE* node : nodes )
            {
                if( find( node ) == piece && !m_sharedPoints.count( toSheet( node->Position ) )
                    && ( !at || rank( node ) < rank( at ) ) )
                {
                    at = node;
                }
            }

            if( at )
                place( name, *at, dynamic_cast<const SOURCE_NET_NAME*>( at->NetName ) );
        }
    }
}


void SCH_BUILDER::buildCommonPinCopies( const SOURCE_DESIGN& aDesign )
{
    for( SOURCE_COMPONENT_INSTANCE* comp : TypedItems<SOURCE_COMPONENT_INSTANCE>( aDesign.ComponentInstances ) )
    {
        const SOURCE_COMPONENT* part = dynamic_cast<const SOURCE_COMPONENT*>( comp->Component );

        if( !part || !part->Board )
            continue;

        // KiCad draws a pin common to several units once per unit and never joins the copies; the file has one
        // pin, on the net of whichever gate wires it
        std::map<wxString, const SOURCE_NET*> netOfPin;

        for( SOURCE_SYMBOL_INSTANCE* inst : TypedItems<SOURCE_SYMBOL_INSTANCE>( comp->SymbolInstances ) )
        {
            for( SOURCE_PAD_INSTANCE* pad : TypedItems<SOURCE_PAD_INSTANCE>( isDrawn( *inst ) ? inst->Pads : nullptr ) )
            {
                const SOURCE_FREE_PAD* freePad = dynamic_cast<const SOURCE_FREE_PAD*>( pad->Pad );
                const SOURCE_NET*      net = netOf( pad->Node );

                if( freePad && net )
                {
                    if( GATE_TERMINAL pin = ResolveTerminal( part->Board, inst->Gate, freePad->Number - 1 );
                        pin.Mapped )
                        netOfPin.emplace( pin.Number, net );
                }
            }
        }

        for( SOURCE_SYMBOL_INSTANCE* inst : TypedItems<SOURCE_SYMBOL_INSTANCE>( comp->SymbolInstances ) )
        {
            const SOURCE_SYMBOL* sym = dynamic_cast<const SOURCE_SYMBOL*>( inst->Symbol );

            for( SOURCE_PAD_INSTANCE* pad :
                 TypedItems<SOURCE_PAD_INSTANCE>( sym && isDrawn( *inst ) ? inst->Pads : nullptr ) )
            {
                const SOURCE_FREE_PAD* freePad = dynamic_cast<const SOURCE_FREE_PAD*>( pad->Pad );

                if( !freePad || pad->Node )
                    continue;

                GATE_TERMINAL pin = ResolveTerminal( part->Board, inst->Gate, freePad->Number - 1 );
                auto          it = pin.Mapped ? netOfPin.find( pin.Number ) : netOfPin.end();

                if( it == netOfPin.end() )
                    continue;

                // A copy on another net's point stays unconnected
                if( VECTOR2I at = toSheet( placePoint( *inst, *sym, freePad->Position ) ); !m_sharedPoints.count( at ) )
                    m_screen->Append( addLabel( m_names.Name( it->second->Name ), at ) );
            }
        }
    }
}


void SCH_BUILDER::buildBuses( const SOURCE_DESIGN& aDesign )
{
    for( SOURCE_BUS* bus : TypedItems<SOURCE_BUS>( aDesign.Buses ) )
    {
        const SOURCE_SHAPE*                shape = dynamic_cast<const SOURCE_SHAPE*>( bus->Shape );
        const SOURCE_LINE_STYLE*           style = dynamic_cast<const SOURCE_LINE_STYLE*>( bus->Style );
        std::vector<const SOURCE_SEGMENT*> verts =
                shape ? ShapeVertices( *shape ) : std::vector<const SOURCE_SEGMENT*>();

        for( size_t i = 0; i + 1 < verts.size(); ++i )
        {
            SCH_LINE* line = new SCH_LINE( toSheet( verts[i]->X, verts[i]->Y ), LAYER_BUS );
            line->SetEndPoint( toSheet( verts[i + 1]->X, verts[i + 1]->Y ) );

            if( style )
                line->SetLineWidth( style->Width );

            m_screen->Append( line );
        }

        for( SOURCE_BUS_TERMINAL* term : TypedItems<SOURCE_BUS_TERMINAL>( bus->Terminals ) )
        {
            SCH_BUS_WIRE_ENTRY* entry = new SCH_BUS_WIRE_ENTRY( toSheet( term->BusEndPosition ) );
            entry->SetSize( toSheet( term->Position ) - toSheet( term->BusEndPosition ) );
            m_screen->Append( entry );
        }

        if( shape )
            addBusAlias( *bus, *shape );
    }
}


void SCH_BUILDER::addBusAlias( const SOURCE_BUS& aBus, const SOURCE_SHAPE& aShape )
{
    // KiCad knows a bus's members only through its label, an alias of the nets a label or power symbol names
    std::vector<wxString> members;

    for( OBJECT* obj : aBus.Nets )
    {
        const SOURCE_NET* net = dynamic_cast<const SOURCE_NET*>( obj );
        wxString          name = net ? m_names.Name( net->Name ) : wxString();

        if( net && ( m_labelledNets.count( name.Upper() ) || m_powerNets.count( name.Upper() ) ) )
            members.push_back( ToKiCadMarkup( name ) );
    }

    std::vector<const SOURCE_SEGMENT*> verts = ShapeVertices( aShape );

    if( members.empty() || verts.empty() )
        return;

    wxString base;

    for( wxUniChar ch : aBus.Name )
        base += wxIsalnum( ch ) ? ch : wxUniChar( '_' );

    if( base.IsEmpty() )
        base = wxS( "BUS" );

    wxString aliasName = base;

    for( int i = 2; m_schematic->GetBusAlias( aliasName ); ++i )
        aliasName = wxString::Format( wxS( "%s_%d" ), base, i );

    std::shared_ptr<BUS_ALIAS> alias = std::make_shared<BUS_ALIAS>();
    alias->SetName( aliasName );
    alias->SetMembers( members );
    m_screen->AddBusAlias( alias );

    SCH_LABEL* label = new SCH_LABEL( toSheet( verts[0]->X, verts[0]->Y ), wxS( "{" ) + aliasName + wxS( "}" ) );

    if( const SOURCE_TEXT_STYLE* style = defaultTextStyle( *m_design ) )
        label->SetTextSize( TextSize( *style ) );

    m_screen->Append( label );
}

} // namespace EASYPC
