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

#include <sch_io/easypc/easypc_sch_items.h>

#include <cmath>
#include <map>
#include <set>

#include <io/easypc/easypc_classes_connectivity.h>
#include <io/easypc/easypc_classes_geometry.h>
#include <io/easypc/easypc_classes_geometry_pad.h>
#include <io/easypc/easypc_classes_library.h>
#include <io/easypc/easypc_classes_styles.h>
#include <io/easypc/easypc_text_metrics.h>
#include <io/easypc/easypc_units.h>

#include <base_units.h>
#include <eda_text.h>
#include <font/font.h>
#include <ki_exception.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <sch_field.h>
#include <sch_pin.h>
#include <sch_shape.h>
#include <sch_text.h>
#include <string_utils.h>


namespace EASYPC
{

std::vector<const SOURCE_SEGMENT*> ShapeVertices( const SOURCE_SHAPE& aShape )
{
    std::vector<const SOURCE_SEGMENT*> out;
    const SOURCE_SEGMENT*              first =
            aShape.Segments.empty() ? nullptr : dynamic_cast<const SOURCE_SEGMENT*>( aShape.Segments[0] );

    if( first && first->HasNextRef )
    {
        // Formats 1 and 2 link the vertices; a closed outline links the last back to the first
        std::set<const SOURCE_SEGMENT*> seen;

        for( const SOURCE_SEGMENT* seg = first; seg && seen.insert( seg ).second; seg = seg->Next )
            out.push_back( seg );
    }
    else if( first )
    {
        for( OBJECT* obj : aShape.Segments )
        {
            if( const SOURCE_SEGMENT* seg = dynamic_cast<const SOURCE_SEGMENT*>( obj ) )
                out.push_back( seg );
        }
    }

    return out;
}


VECTOR2I SCH_FRAME::Map( int64_t aX, int64_t aY ) const
{
    int64_t x = aX - OriginX;
    int64_t y = OriginY - aY;

    if( std::abs( x ) > COORD_LIMIT || std::abs( y ) > COORD_LIMIT )
    {
        THROW_IO_ERROR( wxString::Format( _( "Easy-PC point (%lld, %lld) is outside KiCad's coordinate range" ),
                                          static_cast<long long>( aX ), static_cast<long long>( aY ) ) );
    }

    return VECTOR2I( static_cast<int>( x ), static_cast<int>( y ) );
}


static VECTOR2D rotateAbout( const VECTOR2D& aPt, const VECTOR2D& aCentre, double aRadians )
{
    VECTOR2D r = aPt - aCentre;
    double   c = std::cos( aRadians );
    double   s = std::sin( aRadians );

    return aCentre + VECTOR2D( r.x * c - r.y * s, r.x * s + r.y * c );
}


/// Centre of an arc span from its ends and swept angle
static VECTOR2D arcCentre( const VECTOR2D& aStart, const VECTOR2D& aEnd, int32_t aAngle, bool aAnticlockwise )
{
    VECTOR2D mid = ( aStart + aEnd ) / 2.0;
    double   t = std::tan( ( aAngle * M_PI / 180000.0 ) * ( aAnticlockwise ? 1 : -1 ) / 2.0 );

    if( std::abs( t ) < 1e-12 )
        return mid;

    return VECTOR2D( mid.x - ( aEnd.y - aStart.y ) / ( 2 * t ), mid.y + ( aEnd.x - aStart.x ) / ( 2 * t ) );
}


static VECTOR2I mapD( const SCH_FRAME& aFrame, const VECTOR2D& aPt )
{
    return aFrame.Map( KiROUND<double, int64_t>( aPt.x ), KiROUND<double, int64_t>( aPt.y ) );
}


/// Points after aStart of an arc span at ARC_HIGH_DEF, at most 16384 of them
static std::vector<VECTOR2D> arcAsPolyline( const VECTOR2D& aStart, const VECTOR2D& aEnd, const VECTOR2D& aCentre,
                                            int32_t aAngle, bool aAnticlockwise )
{
    double theta = std::abs( aAngle * M_PI / 180000.0 );
    double radius = ( aStart - aCentre ).EuclideanNorm();

    if( radius * ( 1.0 - std::cos( theta / 2.0 ) ) < 1.0 )
        return { aEnd };

    double step = 2.0 * std::acos( std::max( -1.0, 1.0 - schIUScale.mmToIU( ARC_HIGH_DEF_MM ) / radius ) );
    double wanted = std::ceil( theta / step );
    int    count = wanted < 16384 ? std::max( 2, static_cast<int>( wanted ) ) : 16384;

    std::vector<VECTOR2D> out;

    for( int k = 1; k < count; ++k )
        out.push_back( rotateAbout( aStart, aCentre, ( aAnticlockwise ? theta : -theta ) * k / count ) );

    out.push_back( aEnd );
    return out;
}


std::vector<std::unique_ptr<SCH_SHAPE>> ConvertShape( const SOURCE_SHAPE& aShape, const SCH_FRAME& aFrame,
                                                      SCH_LAYER_ID aLayer, int aStrokeWidth, bool aFilled, int aUnit )
{
    std::vector<std::unique_ptr<SCH_SHAPE>> out;
    std::vector<const SOURCE_SEGMENT*>      verts = ShapeVertices( aShape );
    bool                                    closed = aShape.IsClosed();

    auto make = [&]( SHAPE_T aType )
    {
        std::unique_ptr<SCH_SHAPE> shape = std::make_unique<SCH_SHAPE>( aType, aLayer, aStrokeWidth );
        shape->SetUnit( aUnit );

        if( aFilled )
            shape->SetFillMode( FILL_T::FILLED_SHAPE );

        return shape;
    };

    if( verts.size() < 2 )
        return out;

    // A closed outline of two half circles in one direction is a circle
    if( closed && verts.size() == 2 && verts[0]->ArcAngle == 180000 && verts[1]->ArcAngle == 180000
        && verts[0]->Anticlockwise == verts[1]->Anticlockwise )
    {
        std::unique_ptr<SCH_SHAPE> circle = make( SHAPE_T::CIRCLE );

        circle->SetCenter(
                mapD( aFrame, ( VECTOR2D( verts[0]->X, verts[0]->Y ) + VECTOR2D( verts[1]->X, verts[1]->Y ) ) / 2.0 ) );
        circle->SetEnd( aFrame.Map( verts[0]->X, verts[0]->Y ) );
        out.push_back( std::move( circle ) );
        return out;
    }

    size_t spans = closed ? verts.size() : verts.size() - 1;
    bool   hasArc = false;

    for( size_t i = 0; i < spans; ++i )
        hasArc |= verts[i]->ArcAngle != 0;

    // KiCad cannot fill an outline made of separate arc and polyline shapes
    bool                       onePolygon = aFilled && hasArc;
    std::unique_ptr<SCH_SHAPE> poly;

    for( size_t i = 0; i < spans; ++i )
    {
        const SOURCE_SEGMENT* a = verts[i];
        const SOURCE_SEGMENT* b = verts[( i + 1 ) % verts.size()];
        VECTOR2D              s( a->X, a->Y );
        VECTOR2D              e( b->X, b->Y );

        if( a->ArcAngle != 0 )
        {
            VECTOR2D c = arcCentre( s, e, a->ArcAngle, a->Anticlockwise );
            double   reach = ( s - c ).EuclideanNorm();

            // KiCad's arc code works in integers around the centre; a nearly straight arc puts it out of range
            bool arcFits = std::abs( c.x - aFrame.OriginX ) + reach < COORD_LIMIT
                           && std::abs( aFrame.OriginY - c.y ) + reach < COORD_LIMIT;

            if( arcFits && !onePolygon )
            {
                if( poly )
                    out.push_back( std::move( poly ) );

                double half = ( a->ArcAngle * M_PI / 180000.0 ) / 2.0 * ( a->Anticlockwise ? 1 : -1 );
                std::unique_ptr<SCH_SHAPE> arc = make( SHAPE_T::ARC );

                arc->SetArcGeometry( aFrame.Map( a->X, a->Y ), mapD( aFrame, rotateAbout( s, c, half ) ),
                                     aFrame.Map( b->X, b->Y ) );
                out.push_back( std::move( arc ) );
                continue;
            }

            if( !poly )
            {
                poly = make( SHAPE_T::POLY );
                poly->AddPoint( aFrame.Map( a->X, a->Y ) );
            }

            for( const VECTOR2D& pt : arcAsPolyline( s, e, c, a->ArcAngle, a->Anticlockwise ) )
                poly->AddPoint( mapD( aFrame, pt ) );

            continue;
        }

        if( !poly )
        {
            poly = make( SHAPE_T::POLY );
            poly->AddPoint( aFrame.Map( a->X, a->Y ) );
        }

        poly->AddPoint( aFrame.Map( b->X, b->Y ) );
    }

    if( poly )
        out.push_back( std::move( poly ) );

    return out;
}


std::vector<std::unique_ptr<SCH_SHAPE>> ConvertShapeItem( const SOURCE_SHAPE_ITEM& aItem, const SCH_FRAME& aFrame,
                                                          SCH_LAYER_ID aLayer, int aUnit )
{
    const SOURCE_SHAPE*      shape = dynamic_cast<const SOURCE_SHAPE*>( aItem.Shape );
    const SOURCE_LINE_STYLE* style = dynamic_cast<const SOURCE_LINE_STYLE*>( aItem.Style );

    if( !shape )
        return {};

    return ConvertShape( *shape, aFrame, aLayer, style ? style->Width : 0, aItem.Filled, aUnit );
}


VECTOR2I TextSize( const SOURCE_TEXT_STYLE& aStyle )
{
    // Easy-PC glyphs sit in a 240 unit cell whose capitals are 180 tall; a KiCad size is its capital height
    return VECTOR2I( aStyle.Height, KiROUND( aStyle.Height * 180.0 / 240.0 ) );
}


bool DrawsUpright( const SOURCE_TEXT_POSITION& aPos, bool aKeepUpright )
{
    const SOURCE_TEXT_STYLE* style = dynamic_cast<const SOURCE_TEXT_STYLE*>( aPos.TextStyle );

    return aKeepUpright && style && UprightRotation( aPos.Rotation, true ) != aPos.Rotation
           && TEXT_METRICS( *style ).CanMeasure();
}


void ApplyTextPosition( EDA_TEXT& aText, const SOURCE_TEXT_POSITION& aPos, const SCH_FRAME& aFrame, bool aKeepUpright,
                        const wxString& aMeasured )
{
    const SOURCE_TEXT_STYLE* style = dynamic_cast<const SOURCE_TEXT_STYLE*>( aPos.TextStyle );

    if( style )
    {
        aText.SetTextSize( TextSize( *style ) );
        aText.SetTextThickness( style->LineWidth );
    }

    GR_TEXT_H_ALIGN_T h = GR_TEXT_H_ALIGN_LEFT;

    if( aPos.Alignment == TEXT_ALIGN_RIGHT )
        h = GR_TEXT_H_ALIGN_RIGHT;
    else if( aPos.Alignment == TEXT_ALIGN_CENTRE )
        h = GR_TEXT_H_ALIGN_CENTER;

    auto flip = [&]()
    {
        if( h != GR_TEXT_H_ALIGN_CENTER )
            h = h == GR_TEXT_H_ALIGN_LEFT ? GR_TEXT_H_ALIGN_RIGHT : GR_TEXT_H_ALIGN_LEFT;
    };

    // Mirrored text reads the other way from its anchor
    if( aPos.Mirrored )
        flip();

    GR_TEXT_V_ALIGN_T v = GR_TEXT_V_ALIGN_BOTTOM;
    int32_t           rot = NormalizeAngle( aPos.Rotation );
    VECTOR2I          anchor( aPos.Position.X, aPos.Position.Y );
    wxString          text = aText.GetText();

    text.Replace( wxS( "\r" ), wxEmptyString );
    aText.SetText( text );

    if( DrawsUpright( aPos, aKeepUpright ) )
    {
        // Upright text is turned about the far end of its first line
        anchor = TEXT_METRICS( *style ).UprightAnchor( aMeasured.IsEmpty() ? text : aMeasured, anchor, rot,
                                                       aPos.Mirrored, aPos.Alignment, true );
        rot = NormalizeAngle( UprightRotation( rot, true ) );
    }
    else if( UprightRotation( rot, true ) != rot )
    {
        // KiCad never draws text upside down; turned back, the text keeps its box by flipping its justification
        rot -= 180000;
        flip();
        v = GR_TEXT_V_ALIGN_TOP;
    }

    bool vertical = rot >= 45000 && rot < 135000;

    aText.SetTextAngle( vertical ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
    aText.SetHorizJustify( h );
    aText.SetVertJustify( v );

    VECTOR2I pos = aFrame.Map( anchor.x, anchor.y );
    int      lines = static_cast<int>( text.Freq( '\n' ) ) + 1;

    // Each stored line is an interline height below the last, while KiCad stacks a bottom block upwards
    if( lines > 1 && style )
    {
        double pitch = style->Height * style->InterlineHeightPercent / 100;
        double kicadPitch = KIFONT::FONT::GetFont()->GetInterline( TextSize( *style ).y, KIFONT::METRICS::Default() );
        int    shift = KiROUND( ( v == GR_TEXT_V_ALIGN_BOTTOM ? 1 : -1 ) * ( lines - 1 ) * pitch );

        aText.SetLineSpacing( pitch / kicadPitch );
        pos += vertical ? VECTOR2I( shift, 0 ) : VECTOR2I( 0, shift );
    }

    aText.SetTextPos( pos );
}


std::unique_ptr<SCH_TEXT> ConvertFreeText( const SOURCE_FREE_TEXT& aText, const SCH_FRAME& aFrame, SCH_LAYER_ID aLayer,
                                           int aUnit, bool aKeepUpright )
{
    std::unique_ptr<SCH_TEXT> text =
            std::make_unique<SCH_TEXT>( VECTOR2I( 0, 0 ), ToKiCadMarkup( aText.Text ), aLayer );
    ApplyTextPosition( *text, aText, aFrame, aKeepUpright, aText.Text );
    text->SetUnit( aUnit );
    return text;
}


GATE_TERMINAL ResolveTerminal( const SOURCE_PCB_COMPONENT* aPcb, int aGate, int aTerminal )
{
    GATE_TERMINAL              result;
    const SOURCE_DESIGN_ARRAY* maps = aPcb ? dynamic_cast<const SOURCE_DESIGN_ARRAY*>( aPcb->GateMaps ) : nullptr;

    // Serialized object arrays may hold null elements, so the raw items are indexed
    if( !maps || aGate < 0 || aGate >= static_cast<int>( maps->Items.size() ) || aTerminal < 0 )
        return result;

    const SOURCE_GATE_MAP*     map = dynamic_cast<const SOURCE_GATE_MAP*>( maps->Items[aGate] );
    const SOURCE_DESIGN_ARRAY* pins = map ? dynamic_cast<const SOURCE_DESIGN_ARRAY*>( map->Pins ) : nullptr;

    if( pins )
    {
        const SOURCE_GATE_PIN* pin = aTerminal < static_cast<int>( pins->Items.size() )
                                             ? dynamic_cast<const SOURCE_GATE_PIN*>( pins->Items[aTerminal] )
                                             : nullptr;

        if( !pin )
            return result;

        result.Mapped = true;
        result.NoConnect = pin->IsNoConnect();
        result.Number = pin->PinNumber;
        ExpandPinNumberList( pin->PinNumber, &result.Display );

        if( result.Display.IsEmpty() )
            result.Display = pin->PinNumber;

        if( IsPadList( pin->PcbSymbolPin ) )
        {
            wxArrayString stacked;

            for( const wxString& number : ExpandPinNumberList( pin->PinNumber ) )
                stacked.Add( EscapeStackedPinItem( number ) );

            result.Number = wxS( "[" ) + wxJoin( stacked, ',', 0 ) + wxS( "]" );
        }

        return result;
    }

    // Before format 9000 a gate map stores only a pad number per terminal
    if( map && !map->Pins && aTerminal < static_cast<int>( map->PcbSymbolPins.size() )
        && map->PcbSymbolPins[aTerminal] != -1 )
    {
        result.Mapped = true;
        result.Number = wxString::Format( wxS( "%d" ), map->PcbSymbolPins[aTerminal] );
        result.Display = result.Number;
    }

    return result;
}


namespace
{

    void addText( LIB_SYMBOL& aSymbol, const SOURCE_TEXT_POSITION& aPos, const wxString& aText, const SCH_FRAME& aFrame,
                  int aUnit )
    {
        if( aText.IsEmpty() )
            return;

        SCH_TEXT* text = new SCH_TEXT( VECTOR2I(), ToKiCadMarkup( aText ), LAYER_DEVICE );
        ApplyTextPosition( *text, aPos, aFrame );
        text->SetUnit( aUnit );
        aSymbol.AddDrawItem( text, false );
    }


    void placeField( SCH_FIELD& aField, const SOURCE_VALUE_POSITION& aPos, const SCH_FRAME& aFrame )
    {
        ApplyTextPosition( aField, aPos, aFrame );
        aField.SetVisible( aPos.Displayed );
    }


    /// Fields take the first gate symbol's value positions: the reference one, then one naming the component or values
    void placeFields( LIB_SYMBOL& aSymbol, const SOURCE_SYMBOL& aGate0, const SCH_FRAME& aFrame,
                      const std::vector<const SOURCE_ATTRIBUTE*>& aAttributes )
    {
        std::vector<const SOURCE_VALUE_POSITION*> positions;

        for( const SOURCE_VALUE_POSITION* pos : TypedItems<const SOURCE_VALUE_POSITION>( aGate0.ValuePositions ) )
        {
            if( pos->Displayed )
                positions.push_back( pos );
        }

        const SOURCE_VALUE_POSITION* refPos = nullptr;
        const SOURCE_VALUE_POSITION* valuePos = nullptr;

        for( const SOURCE_VALUE_POSITION* pos : positions )
        {
            if( !refPos && ( pos->Types & VALUE_REFERENCE_NAME ) )
                refPos = pos;
        }

        for( uint64_t wanted : { VALUE_COMPONENT_NAME, VALUE_VALUES } )
        {
            for( const SOURCE_VALUE_POSITION* pos : positions )
            {
                if( !valuePos && pos != refPos && ( pos->Types & wanted ) )
                    valuePos = pos;
            }
        }

        if( refPos )
            placeField( aSymbol.GetReferenceField(), *refPos, aFrame );

        if( valuePos )
            placeField( aSymbol.GetValueField(), *valuePos, aFrame );
        else
            aSymbol.GetValueField().SetVisible( false );

        aSymbol.GetFootprintField().SetVisible( false );
        aSymbol.GetDescriptionField().SetVisible( false );

        for( const SOURCE_VALUE_POSITION* pos : positions )
        {
            if( ( pos->Types & VALUE_DESCRIPTION ) && pos != refPos && pos != valuePos )
                placeField( aSymbol.GetDescriptionField(), *pos, aFrame );
        }

        for( const SOURCE_ATTRIBUTE* attr : aAttributes )
        {
            if( !attr->AttributeName || attr->AttributeName->Name.IsEmpty() )
                continue;

            SCH_FIELD* field = new SCH_FIELD( &aSymbol, FIELD_T::USER, attr->AttributeName->Name );
            field->SetText( ToKiCadMarkup( attr->Value ) );
            field->SetVisible( false );

            for( const SOURCE_VALUE_POSITION* pos : positions )
            {
                if( ( pos->Types & VALUE_ATTRIBUTE ) && pos->Attribute == attr->AttributeName->Name )
                {
                    placeField( *field, *pos, aFrame );
                    field->SetVisible( pos->Displayed && attr->Displayed );
                    break;
                }
            }

            aSymbol.AddField( field );
        }
    }


    /// One gate's graphics and pins; without a gate definition pins are numbered by pad number
    void convertGate( LIB_SYMBOL& aSymbol, const SOURCE_SYMBOL& aGateSymbol, int aGate,
                      const SOURCE_PCB_COMPONENT* aPcb, const SOURCE_GATE* aGateDef, bool aPinTexts )
    {
        const int unit = aGate + 1;
        SCH_FRAME frame{ aGateSymbol.OriginX, aGateSymbol.OriginY };

        for( const SOURCE_SHAPE_ITEM* item : TypedItems<const SOURCE_SHAPE_ITEM>( aGateSymbol.Shapes ) )
        {
            for( std::unique_ptr<SCH_SHAPE>& shape : ConvertShapeItem( *item, frame, LAYER_DEVICE, unit ) )
                aSymbol.AddDrawItem( shape.release(), false );
        }

        for( const SOURCE_FREE_TEXT* text : TypedItems<const SOURCE_FREE_TEXT>( aGateSymbol.Texts ) )
            aSymbol.AddDrawItem( ConvertFreeText( *text, frame, LAYER_DEVICE, unit ).release(), false );

        for( const SOURCE_FREE_PAD* pad : TypedItems<const SOURCE_FREE_PAD>( aGateSymbol.Pads ) )
        {
            const int           terminal = pad->Number - 1;
            const GATE_TERMINAL gate = ResolveTerminal( aPcb, aGate, terminal );
            wxString            name;

            if( aGateDef && terminal >= 0 && terminal < static_cast<int>( aGateDef->PinNames.size() ) )
                name = aGateDef->PinNames[terminal];

            // A symbol pad is only the connection point; the leg is part of the symbol's shapes
            SCH_PIN* pin = new SCH_PIN( &aSymbol );
            pin->SetPosition( frame.Map( pad->Position.X, pad->Position.Y ) );
            pin->SetLength( 0 );
            pin->SetName( ToKiCadMarkup( name ) );
            pin->SetNumber( aGateDef ? gate.Number : wxString::Format( wxS( "%d" ), pad->Number ) );
            pin->SetType( gate.NoConnect ? ELECTRICAL_PINTYPE::PT_NC : ELECTRICAL_PINTYPE::PT_PASSIVE );
            pin->SetUnit( unit );

            switch( NormalizeAngle( pad->Angle ) / 90000 )
            {
            case 1: pin->SetOrientation( PIN_ORIENTATION::PIN_UP ); break;
            case 2: pin->SetOrientation( PIN_ORIENTATION::PIN_LEFT ); break;
            case 3: pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN ); break;
            default: pin->SetOrientation( PIN_ORIENTATION::PIN_RIGHT ); break;
            }

            aSymbol.AddDrawItem( pin, false );

            if( !aPinTexts )
                continue;

            const wxString number = aGateDef ? gate.Display : pin->GetNumber();

            for( const SOURCE_VALUE_POSITION* pos : TypedItems<const SOURCE_VALUE_POSITION>( pad->ValuePositions ) )
            {
                if( pos->Displayed && ( pos->Types & VALUE_PIN_NAME ) )
                    addText( aSymbol, *pos, name, frame, unit );
                else if( pos->Displayed && ( pos->Types & VALUE_PIN_NUMBER ) )
                    addText( aSymbol, *pos, number, frame, unit );
            }

            if( const SOURCE_PIN_NAME* old = dynamic_cast<const SOURCE_PIN_NAME*>( pad->OldPinName );
                old && old->Visible )
                addText( aSymbol, *old, name, frame, unit );

            if( const SOURCE_PIN_NAME* old = dynamic_cast<const SOURCE_PIN_NAME*>( pad->OldPinNumber );
                old && old->Visible )
                addText( aSymbol, *old, number, frame, unit );
        }
    }


    /// A pin number drawn alike in several gates is one physical pin, common to all units in KiCad
    void mergeCommonPins( LIB_SYMBOL& aSymbol )
    {
        std::map<wxString, std::vector<SCH_PIN*>> byNumber;

        for( SCH_PIN* pin : aSymbol.GetGraphicalPins() )
        {
            if( !pin->GetNumber().IsEmpty() )
                byNumber[pin->GetNumber()].push_back( pin );
        }

        for( auto& [number, pins] : byNumber )
        {
            const SCH_PIN* first = pins.front();
            bool           alike = pins.size() > 1;

            for( size_t i = 1; i < pins.size(); ++i )
            {
                alike &= pins[i]->GetPosition() == first->GetPosition()
                         && pins[i]->GetOrientation() == first->GetOrientation()
                         && pins[i]->GetName() == first->GetName() && pins[i]->GetType() == first->GetType()
                         && pins[i]->GetUnit() != first->GetUnit();
            }

            if( !alike )
                continue;

            pins.front()->SetUnit( 0 );

            for( size_t i = 1; i < pins.size(); ++i )
                aSymbol.RemoveDrawItem( pins[i] );
        }
    }


    std::unique_ptr<LIB_SYMBOL> newSymbol( const LIB_ID& aId )
    {
        std::unique_ptr<LIB_SYMBOL> symbol = std::make_unique<LIB_SYMBOL>( aId.GetLibItemName().wx_str() );
        symbol->SetLibId( aId );

        // Pin names and numbers have their own stored positions, so they are added as texts
        symbol->SetShowPinNames( false );
        symbol->SetShowPinNumbers( false );
        return symbol;
    }

} // namespace


std::unique_ptr<LIB_SYMBOL> ConvertComponentSymbol( const SCH_COMPONENT_SOURCE& aSrc, const LIB_ID& aId )
{
    if( !aSrc.Schematic )
        THROW_IO_ERROR( _( "Easy-PC component has no schematic part" ) );

    std::vector<const SOURCE_GATE*> gateDefs;

    if( const SOURCE_DESIGN_ARRAY* gates = dynamic_cast<const SOURCE_DESIGN_ARRAY*>( aSrc.Schematic->Gates ) )
    {
        for( const OBJECT* obj : gates->Items )
        {
            gateDefs.push_back( dynamic_cast<const SOURCE_GATE*>( obj ) );

            if( !gateDefs.back() )
            {
                THROW_IO_ERROR( wxString::Format( _( "Easy-PC component '%s' gate %zu is empty" ), aSrc.Schematic->Name,
                                                  gateDefs.size() ) );
            }
        }
    }

    if( aSrc.Gates.size() != gateDefs.size() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Easy-PC component '%s' has %zu gates but %zu gate symbols" ),
                                          aSrc.Schematic->Name, gateDefs.size(), aSrc.Gates.size() ) );
    }

    std::unique_ptr<LIB_SYMBOL> symbol = newSymbol( aId );
    symbol->SetUnitCount( std::max<int>( 1, static_cast<int>( aSrc.Gates.size() ) ), false );
    symbol->GetReferenceField().SetText( aSrc.Schematic->DefaultReference );
    symbol->GetValueField().SetText( ToKiCadMarkup( aSrc.Schematic->Name ) );
    symbol->SetDescription( ToKiCadMarkup( aSrc.Schematic->Description ) );
    symbol->GetFootprintField().SetText( aSrc.Footprint );
    symbol->SetFPFilters( aSrc.FootprintFilters );

    std::vector<const SOURCE_ATTRIBUTE*> attributes;

    if( aSrc.Board )
        attributes = TypedItems<const SOURCE_ATTRIBUTE>( aSrc.Board->Attributes );

    // Fields sit where the first gate symbol present puts them
    for( const SOURCE_SYMBOL* gateSymbol : aSrc.Gates )
    {
        if( gateSymbol )
        {
            placeFields( *symbol, *gateSymbol, SCH_FRAME{ gateSymbol->OriginX, gateSymbol->OriginY }, attributes );
            break;
        }
    }

    for( size_t g = 0; g < aSrc.Gates.size(); ++g )
    {
        if( aSrc.Gates[g] )
            convertGate( *symbol, *aSrc.Gates[g], static_cast<int>( g ), aSrc.Board, gateDefs[g], aSrc.PinTexts );
    }

    mergeCommonPins( *symbol );

    // Items are added unsorted, since LIB_SYMBOL sorts the whole list on every sorted insertion
    symbol->GetDrawItems().sort();
    return symbol;
}


std::unique_ptr<LIB_SYMBOL> ConvertSymbol( const SOURCE_SYMBOL& aSymbol, const LIB_ID& aId )
{
    std::unique_ptr<LIB_SYMBOL> symbol = newSymbol( aId );
    symbol->GetValueField().SetText( ToKiCadMarkup( aSymbol.Name ) );

    placeFields( *symbol, aSymbol, SCH_FRAME{ aSymbol.OriginX, aSymbol.OriginY }, {} );
    convertGate( *symbol, aSymbol, 0, nullptr, nullptr, true );
    symbol->GetDrawItems().sort();
    return symbol;
}

} // namespace EASYPC
