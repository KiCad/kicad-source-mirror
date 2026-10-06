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

#include <easypc/easypc_pcb_items.h>

#include <io/easypc/easypc_classes_connectivity.h>
#include <io/easypc/easypc_classes_library.h>
#include <io/easypc/easypc_text_metrics.h>

#include <board_item_container.h>
#include <convert_basic_shapes_to_polygon.h>
#include <font/font.h>
#include <footprint.h>
#include <ki_exception.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_segment.h>
#include <lib_id.h>
#include <pad.h>
#include <pcb_field.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <stroke_params.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <set>


using namespace EASYPC;

using KI_PAD_SHAPE = ::PAD_SHAPE;


namespace EASYPC_PCB
{

namespace
{

    constexpr int IU_PER_UNIT = NM_PER_DSU;

    /// Pad openings drawn as shapes are finer than the board default so the plot stays within a micron
    constexpr int OPENING_MAX_ERROR = 1000;


    /// aLength in IU, refused when KiCad cannot hold it
    int lengthOf( int64_t aLength )
    {
        if( std::abs( aLength ) > COORD_LIMIT / IU_PER_UNIT )
        {
            THROW_IO_ERROR( wxString::Format( _( "Easy-PC length %lld is outside KiCad's range" ),
                                              static_cast<long long>( aLength ) ) );
        }

        return static_cast<int>( aLength * IU_PER_UNIT );
    }


    /// A non-electrical layer's KiCad layer from its type name
    PCB_LAYER_ID vendorTechLayer( const SOURCE_LAYER* aLayer )
    {
        wxString name = ( aLayer->Type ? aLayer->Type->Name : aLayer->TypeName ).Upper();
        bool     back = aLayer->Side == LAYER_SIDE::BOTTOM;

        if( name.Contains( wxS( "PASTE" ) ) )
            return back ? B_Paste : F_Paste;

        if( name.Contains( wxS( "SILK" ) ) )
            return back ? B_SilkS : F_SilkS;

        if( name.Contains( wxS( "RESIST" ) ) || name.Contains( wxS( "SOLDER MASK" ) ) )
            return back ? B_Mask : F_Mask;

        return UNDEFINED_LAYER;
    }


    /// The centre of an arc span, native units
    VECTOR2D ArcCentre( const POINT32& aStart, const POINT32& aEnd, int32_t aArcAngle, bool aAnticlockwise )
    {
        double hx = ( aEnd.X - aStart.X ) * 0.5;
        double hy = ( aEnd.Y - aStart.Y ) * 0.5;
        double cx = aStart.X + hx;
        double cy = aStart.Y + hy;
        double t = std::tan( ( aArcAngle * M_PI / 180000.0 ) * ( aAnticlockwise ? 1 : -1 ) * 0.5 );

        if( t != 0.0 )
        {
            cx = cx - hy / t;
            cy = hx / t + cy;
        }

        auto closeTo = []( int32_t aA, double aB )
        {
            return std::abs( aA - static_cast<int32_t>( aB < 0.0 ? aB - 0.5 : aB + 0.5 ) ) < 3;
        };

        if( closeTo( aStart.X, cx ) && closeTo( aEnd.Y, cy ) )
            return VECTOR2D( aStart.X, aEnd.Y );

        if( closeTo( aEnd.X, cx ) && closeTo( aStart.Y, cy ) )
            return VECTOR2D( aEnd.X, aStart.Y );

        return VECTOR2D( cx, cy );
    }


    VECTOR2D arcPoint( const POINT32& aStart, const VECTOR2D& aCentre, int32_t aArcAngle, bool aAnticlockwise,
                       double aFraction )
    {
        double a0 = std::atan2( aStart.Y - aCentre.y, aStart.X - aCentre.x );
        double sweep = aArcAngle / 1000.0 * M_PI / 180.0 * ( aAnticlockwise ? 1.0 : -1.0 );
        double r = std::hypot( aStart.X - aCentre.x, aStart.Y - aCentre.y );
        double a = a0 + sweep * aFraction;
        return VECTOR2D( aCentre.x + r * std::cos( a ), aCentre.y + r * std::sin( a ) );
    }


    struct SPAN
    {
        POINT32 Start;
        POINT32 End;
        int32_t ArcAngle = 0;
        bool    Anticlockwise = true;
    };


    /// Every vertex to its successor, the last back to the first only when closed
    std::vector<SPAN> spansOf( const SOURCE_SHAPE& aShape )
    {
        std::vector<const SOURCE_SEGMENT*> verts;

        for( const OBJECT* obj : aShape.Segments )
        {
            if( const SOURCE_SEGMENT* seg = dynamic_cast<const SOURCE_SEGMENT*>( obj ) )
                verts.push_back( seg );
        }

        std::vector<SPAN> spans;
        size_t            n = verts.size();

        for( size_t i = 0; n > 0 && i < ( aShape.IsClosed() ? n : n - 1 ); ++i )
        {
            const SOURCE_SEGMENT* s = verts[i];
            const SOURCE_SEGMENT* e = verts[( i + 1 ) % n];
            spans.push_back( { POINT32{ s->X, s->Y }, POINT32{ e->X, e->Y }, s->ArcAngle, s->Anticlockwise } );
        }

        return spans;
    }


    SHAPE_LINE_CHAIN shapeToChain( const SOURCE_SHAPE& aShape, const FRAME& aFrame )
    {
        SHAPE_LINE_CHAIN  chain;
        std::vector<SPAN> spans = spansOf( aShape );

        for( size_t i = 0; i < spans.size(); ++i )
        {
            ARC_FORM form = ArcForm( spans[i].Start, spans[i].End, spans[i].ArcAngle, spans[i].Anticlockwise, aFrame );

            if( form.IsArc() )
            {
                chain.Append( SHAPE_ARC( form.Start, form.Mid, form.End, 0 ) );
                continue;
            }

            for( size_t k = i == 0 ? 0 : 1; k < form.Points.size(); ++k )
                chain.Append( form.Points[k] );
        }

        chain.SetClosed( aShape.IsClosed() );
        return chain;
    }


    /// The bullet outline in the pad frame, native Y up
    SHAPE_POLY_SET bulletOutline( int32_t aSize, int32_t aLength )
    {
        double radius = ( aSize + 1 ) / 2;
        double h = std::max( radius / 2, double( ( aLength + 1 ) / 2 ) );
        double k = h - radius;

        auto pt = [&]( double aX, double aY )
        {
            return VECTOR2I( KiROUND( aX * IU_PER_UNIT ), KiROUND( -aY * IU_PER_UNIT ) );
        };

        SHAPE_LINE_CHAIN chain;
        chain.Append( pt( -radius, -h ) );
        chain.Append( pt( radius, -h ) );
        chain.Append( pt( radius, k ) );
        chain.Append( SHAPE_ARC( pt( radius, k ), pt( 0, k + radius ), pt( -radius, k ), 0 ) );
        chain.SetClosed( true );

        SHAPE_POLY_SET poly;
        poly.AddOutline( chain );
        poly.Fracture();
        return poly;
    }


    /// aLayer's pad outline
    void setPadShape( PAD& aPad, PCB_LAYER_ID aLayer, EASYPC::PAD_SHAPE aShape, int32_t aSize, int32_t aLength,
                      int32_t aD3 )
    {
        VECTOR2I square( lengthOf( aSize ), lengthOf( aSize ) );
        VECTOR2I sized( lengthOf( aSize ), lengthOf( aLength ) );

        switch( aShape )
        {
        case EASYPC::PAD_SHAPE::ROUND:
            aPad.SetShape( aLayer, KI_PAD_SHAPE::CIRCLE );
            aPad.SetSize( aLayer, square );
            break;

        case EASYPC::PAD_SHAPE::SQUARE:
            aPad.SetShape( aLayer, KI_PAD_SHAPE::RECTANGLE );
            aPad.SetSize( aLayer, square );
            break;

        case EASYPC::PAD_SHAPE::RECTANGLE:
            aPad.SetShape( aLayer, KI_PAD_SHAPE::RECTANGLE );
            aPad.SetSize( aLayer, sized );
            break;

        case EASYPC::PAD_SHAPE::OVAL:
            aPad.SetShape( aLayer, KI_PAD_SHAPE::OVAL );
            aPad.SetSize( aLayer, sized );
            break;

        case EASYPC::PAD_SHAPE::ROUNDED_RECTANGLE:
            aPad.SetShape( aLayer, KI_PAD_SHAPE::ROUNDRECT );
            aPad.SetSize( aLayer, sized );
            aPad.SetRoundRectRadiusRatio( aLayer, std::clamp<double>( aD3, 0.0, std::min( aSize, aLength ) / 2.0 )
                                                          / std::min( aSize, aLength ) );
            break;

        case EASYPC::PAD_SHAPE::OCTAGON:
            // Regular, flats on the axes, chamfer leg round(S / (2 + sqrt 2))
            aPad.SetShape( aLayer, KI_PAD_SHAPE::CHAMFERED_RECT );
            aPad.SetSize( aLayer, square );
            aPad.SetChamferRectRatio( aLayer, std::round( aSize / ( 2.0 + std::sqrt( 2.0 ) ) ) / aSize );
            aPad.SetChamferPositions( aLayer, RECT_CHAMFER_ALL );
            break;

        case EASYPC::PAD_SHAPE::BULLET:
        {
            int anchor = std::max( 1, lengthOf( std::min( aSize, aLength ) ) / 4 );

            aPad.SetShape( aLayer, KI_PAD_SHAPE::CUSTOM );
            aPad.SetAnchorPadShape( aLayer, KI_PAD_SHAPE::CIRCLE );
            aPad.SetSize( aLayer, VECTOR2I( anchor, anchor ) );
            aPad.AddPrimitivePoly( aLayer, bulletOutline( aSize, aLength ), 0, true );
            break;
        }

        default:
            // A guessed outline would be wrong copper, so a shape no known file uses is refused
            THROW_IO_ERROR( wxString::Format( _( "Unsupported Easy-PC pad shape %d." ), static_cast<int>( aShape ) ) );
        }
    }


    /// An exception's outline, replaced by the hole when the hole is larger; size 0 draws nothing
    struct LAYER_OUTLINE
    {
        EASYPC::PAD_SHAPE Shape;
        int32_t           Size;
        int32_t           Length;
        int32_t           Dimension3;
    };


    LAYER_OUTLINE exceptionOutline( const SOURCE_PAD_STYLE& aStyle, const SOURCE_PAD_STYLE_EXCEPTION& aException )
    {
        LAYER_OUTLINE out{ aException.Shape, aException.Size, aException.Length ? aException.Length : aException.Size,
                           aException.Dimension3 };
        int32_t       holeX = aStyle.Drill;
        int32_t       holeY =
                aStyle.DrillShape != EASYPC::PAD_SHAPE::ROUND && aStyle.Drill ? aStyle.DrillLength : aStyle.Drill;

        if( holeX > out.Size && holeY > out.Length )
            out = { aStyle.DrillShape, holeX, holeY, aStyle.DrillCornerRadius };

        return out;
    }


    /// Per-layer outlines from aStyle's exceptions; a mask or paste opening grows by the layer's oversize
    void applyPadStyleExceptions( FOOTPRINT* aParent, PAD& aPad, const SOURCE_PAD_STYLE& aStyle,
                                  const LAYER_MAPPER& aLayers )
    {
        std::vector<const SOURCE_PAD_STYLE_EXCEPTION*> exceptions =
                TypedItems<const SOURCE_PAD_STYLE_EXCEPTION>( aStyle.Exceptions );
        LSET layers = aPad.GetLayerSet();

        for( PCB_LAYER_ID layer :
             LSET( layers & ( LSET::AllCuMask() | LSET( { F_Mask, B_Mask, F_Paste, B_Paste } ) ) ) )
        {
            const SOURCE_LAYER* native = aLayers.Native( layer );
            auto                it = std::find_if( exceptions.begin(), exceptions.end(),
                                                   [&]( const SOURCE_PAD_STYLE_EXCEPTION* aEx )
                                                   {
                                        return aEx->Layer == native;
                                    } );

            if( !native || it == exceptions.end() )
                continue;

            LAYER_OUTLINE outline = exceptionOutline( aStyle, **it );
            bool          present = ( ( int64_t( outline.Size ) + 1 ) >> 1 ) >= 1;

            if( IsCopperLayer( layer ) )
            {
                if( !present )
                {
                    layers.reset( layer );
                    continue;
                }

                aPad.Padstack().SetMode( PADSTACK::MODE::CUSTOM );
                setPadShape( aPad, layer, outline.Shape, outline.Size, outline.Length, outline.Dimension3 );
                continue;
            }

            // A KiCad pad has one opening per side, so the exception's opening is drawn on its own
            layers.reset( layer );

            if( !present || !aParent )
                continue;

            PAD opening( aPad );
            opening.Padstack().SetMode( PADSTACK::MODE::NORMAL );
            setPadShape( opening, PADSTACK::ALL_LAYERS, outline.Shape, aLayers.GrowByLayer( native, outline.Size ),
                         aLayers.GrowByLayer( native, outline.Length ), outline.Dimension3 );

            SHAPE_POLY_SET poly;
            opening.TransformShapeToPolygon( poly, F_Cu, 0, OPENING_MAX_ERROR, ERROR_INSIDE );

            PCB_SHAPE* shape = new PCB_SHAPE( aParent, SHAPE_T::POLY );
            shape->SetPolyShape( poly );
            shape->SetFilled( true );
            shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
            shape->SetLayer( layer );
            aParent->Add( shape );
        }

        aPad.SetLayerSet( layers );
    }


    /// Extents of a text's stroke centre lines as KiCad lays it out
    BOX2I strokeBox( const PCB_TEXT& aText )
    {
        BOX2I                           box;
        std::shared_ptr<SHAPE_COMPOUND> shapes = aText.GetEffectiveTextShape( false );

        for( SHAPE* shape : shapes->Shapes() )
        {
            if( shape->Type() == SH_SEGMENT )
            {
                box.Merge( static_cast<SHAPE_SEGMENT*>( shape )->GetSeg().A );
                box.Merge( static_cast<SHAPE_SEGMENT*>( shape )->GetSeg().B );
            }
            else
            {
                box.Merge( shape->BBox() );
            }
        }

        return box;
    }


    /// The pin number pad aPadNumber of a placed component shows
    wxString PcbPadNumber( const SOURCE_PCB_COMPONENT& aComponent, int aPadNumber )
    {
        const wxString     pad = wxString::Format( wxS( "%d" ), aPadNumber );
        std::set<wxString> used;

        // A pad named by a gate pin takes that pin's component pin number
        for( const SOURCE_GATE_MAP* map : TypedItems<const SOURCE_GATE_MAP>( aComponent.GateMaps ) )
        {
            for( const SOURCE_GATE_PIN* pin : TypedItems<const SOURCE_GATE_PIN>( map->Pins ) )
            {
                if( pin->PcbSymbolPin == pad )
                    return pin->PinNumber;
            }
        }

        for( const SOURCE_GATE_MAP* map : TypedItems<const SOURCE_GATE_MAP>( aComponent.GateMaps ) )
        {
            for( const SOURCE_GATE_PIN* pin : TypedItems<const SOURCE_GATE_PIN>( map->Pins ) )
            {
                for( const wxString& n : ExpandPinNumberList( pin->PinNumber ) )
                    used.insert( n );
            }

            // Before format 9000 a gate map is a pad number list, read as gate pins numbered alike
            for( int32_t number : map->PcbSymbolPins )
            {
                if( number == -1 )
                    continue;

                if( number == aPadNumber )
                    return pad;

                used.insert( wxString::Format( wxS( "%d" ), number ) );
            }
        }

        // Any other pad keeps its number unless a pin number has it, then takes the lowest free number
        wxString number = pad;

        for( int n = 1; used.count( number ); ++n )
            number = wxString::Format( wxS( "%d" ), n );

        return number;
    }


    /// Place a field as AddText places a free text
    void placeField( FOOTPRINT& aFootprint, PCB_FIELD& aField, const SOURCE_VALUE_POSITION& aPos, const FRAME& aFrame,
                     const LAYER_MAPPER& aLayers )
    {
        if( PCB_TEXT* text = AddText( aFootprint, aPos, aField.GetText(), aFrame, aLayers ) )
        {
            aField.SetAttributes( *text );
            aField.SetPosition( text->GetPosition() );
            aField.SetLayer( text->GetLayer() );
            aFootprint.Remove( text );
            delete text;
        }

        aField.SetVisible( aPos.Displayed );
    }

} // namespace


VECTOR2I FRAME::ToKiCad( const VECTOR2D& aPoint ) const
{
    double x = Offset.x + ( aPoint.x - Origin.X ) * IU_PER_UNIT;
    double y = Offset.y - ( aPoint.y - Origin.Y ) * IU_PER_UNIT;

    if( std::abs( x ) > COORD_LIMIT || std::abs( y ) > COORD_LIMIT )
    {
        wxString msg = _( "Easy-PC coordinate (%.0f, %.0f) is outside KiCad's range" ); //format:allow
        THROW_IO_ERROR( wxString::Format( msg, aPoint.x, aPoint.y ) );
    }

    return VECTOR2I( KiROUND( x ), KiROUND( y ) );
}


int FRAME::Length( int64_t aLength ) const
{
    return lengthOf( aLength );
}


EDA_ANGLE FRAME::Angle( int32_t aMilliDegrees )
{
    return EDA_ANGLE( AngleToDegrees( aMilliDegrees ), DEGREES_T ).Normalize();
}


LAYER_MAPPER::LAYER_MAPPER( const std::vector<const SOURCE_LAYER*>& aLayers ) :
        m_layers( aLayers )
{
    // Copper in list order; KiCad has no room past In30.Cu
    constexpr int maxInner = MAX_CU_LAYERS - 2;
    int           inner = 0;

    for( const SOURCE_LAYER* layer : m_layers )
    {
        if( layer->Usage != LAYER_USAGE_ELECTRICAL )
            continue;

        if( layer->Side == LAYER_SIDE::TOP )
            m_map[layer] = F_Cu;
        else if( layer->Side == LAYER_SIDE::BOTTOM )
            m_map[layer] = B_Cu;
        else
            m_map[layer] = static_cast<PCB_LAYER_ID>( In1_Cu + 2 * ( std::min( ++inner, maxInner ) - 1 ) );
    }

    // KiCad stacks copper in pairs
    m_copperCount = std::min( ( inner + 3 ) / 2 * 2, MAX_CU_LAYERS );

    int user = 0;

    for( const SOURCE_LAYER* layer : m_layers )
    {
        if( layer->Usage != LAYER_USAGE_NON_ELECTRICAL )
            continue;

        PCB_LAYER_ID id = vendorTechLayer( layer );

        if( id == UNDEFINED_LAYER && user <= ( User_45 - User_1 ) / 2 )
            id = static_cast<PCB_LAYER_ID>( User_1 + 2 * user++ );

        if( id != UNDEFINED_LAYER )
            m_map[layer] = id;
    }
}


std::vector<INPUT_LAYER_DESC> LAYER_MAPPER::Describe() const
{
    std::vector<INPUT_LAYER_DESC> descs;

    for( const SOURCE_LAYER* layer : m_layers )
    {
        auto it = m_map.find( layer );

        if( it == m_map.end() )
            continue;

        INPUT_LAYER_DESC desc;
        desc.Name = layer->Name;
        desc.AutoMapLayer = it->second;
        desc.Required = layer->Usage == LAYER_USAGE_ELECTRICAL;
        desc.PermittedLayers = desc.Required ? LSET::AllCuMask() : LSET::AllNonCuMask();
        descs.push_back( desc );
    }

    return descs;
}


void LAYER_MAPPER::ApplyUserMapping( const std::map<wxString, PCB_LAYER_ID>& aMapping )
{
    for( auto& [layer, id] : m_map )
    {
        if( auto it = aMapping.find( layer->Name ); it != aMapping.end() )
            id = it->second;
    }
}


PCB_LAYER_ID LAYER_MAPPER::Map( const OBJECT* aLayer ) const
{
    auto it = m_map.find( dynamic_cast<const SOURCE_LAYER*>( aLayer ) );
    return it == m_map.end() ? UNDEFINED_LAYER : it->second;
}


LSET LAYER_MAPPER::PadLayers( const OBJECT* aLayer, bool aComponentPad ) const
{
    const SOURCE_LAYER* ref = dynamic_cast<const SOURCE_LAYER*>( aLayer );
    bool                all = !ref || ( ref->Usage == LAYER_USAGE_SET && ref->Side == LAYER_SIDE::ALL );
    LSET                set;

    // No known file puts a pad on a layer span, and treating one as a single layer would drop its copper
    if( ref && ref->Usage == LAYER_USAGE_SPAN )
        THROW_IO_ERROR( wxString::Format( _( "Unsupported Easy-PC pad on layer span '%s'." ), ref->Name ) );

    for( const SOURCE_LAYER* layer : m_layers )
    {
        if( layer->Usage != LAYER_USAGE_ELECTRICAL )
            continue;

        if( all || ( ref->Usage == LAYER_USAGE_SET ? layer->Side == ref->Side : layer == ref ) )
            set.set( Map( layer ) );
    }

    // Mask and paste by the layer type flags, on the side the pad reaches
    for( const SOURCE_LAYER* layer : m_layers )
    {
        const SOURCE_LAYER_TYPE* type = layer->Type;
        PCB_LAYER_ID             id = Map( layer );

        if( layer->Usage != LAYER_USAGE_NON_ELECTRICAL || !type
            || ( id != F_Mask && id != B_Mask && id != F_Paste && id != B_Paste ) )
        {
            continue;
        }

        bool enabled = aComponentPad ? ( all ? type->AllLayerPadInstancesEnabled : type->SurfacePadInstancesEnabled )
                                     : ( all ? type->AllLayerPadsEnabled : type->SurfacePadsEnabled );

        if( enabled && ( all || ref->Side == layer->Side ) )
            set.set( id );
    }

    return set;
}


const SOURCE_LAYER* LAYER_MAPPER::Native( PCB_LAYER_ID aLayer ) const
{
    for( const SOURCE_LAYER* layer : m_layers )
    {
        if( Map( layer ) == aLayer )
            return layer;
    }

    return nullptr;
}


int32_t LAYER_MAPPER::GrowByLayer( const SOURCE_LAYER* aLayer, int32_t aSize ) const
{
    const SOURCE_LAYER_TYPE* type = aLayer ? aLayer->Type : nullptr;

    if( !type || type->OversizeType == LAYER_OVERSIZE_TYPE::NONE )
        return aSize;

    int64_t grow = type->Oversize;

    if( type->OversizeType == LAYER_OVERSIZE_TYPE::PERCENT )
        grow = grow * aSize / 100;

    // The caller's lengthOf refuses what KiCad cannot hold
    return static_cast<int32_t>( std::clamp<int64_t>( aSize + grow * 2, 0, std::numeric_limits<int32_t>::max() ) );
}


ARC_FORM ArcForm( const POINT32& aStart, const POINT32& aEnd, int32_t aArcAngle, bool aAnticlockwise,
                  const FRAME& aFrame )
{
    ARC_FORM form{ aFrame.ToKiCad( aStart ), {}, aFrame.ToKiCad( aEnd ), {} };

    if( aArcAngle == 0 )
    {
        form.Points = { form.Start, form.End };
        return form;
    }

    VECTOR2D c = ArcCentre( aStart, aEnd, aArcAngle, aAnticlockwise );
    double   r = std::hypot( aStart.X - c.x, aStart.Y - c.y ) * IU_PER_UNIT;
    double   cx = aFrame.Offset.x + ( c.x - aFrame.Origin.X ) * IU_PER_UNIT;
    double   cy = aFrame.Offset.y - ( c.y - aFrame.Origin.Y ) * IU_PER_UNIT;

    if( r <= COORD_LIMIT && std::abs( cx ) <= COORD_LIMIT && std::abs( cy ) <= COORD_LIMIT )
    {
        form.Mid = aFrame.ToKiCad( arcPoint( aStart, c, aArcAngle, aAnticlockwise, 0.5 ) );
        return form;
    }

    // A nearly straight arc of huge radius is kept as points on the true arc
    double sweep = std::abs( aArcAngle / 1000.0 * M_PI / 180.0 );
    int    count = 1;

    if( r * ( 1.0 - std::cos( sweep / 2.0 ) ) >= 1.0 )
    {
        double step = 2.0 * std::acos( std::max( 0.0, 1.0 - ARC_HIGH_DEF / r ) );
        count = std::max( 1, static_cast<int>( std::ceil( sweep / std::max( step, 1e-12 ) ) ) );
    }

    form.Points.push_back( form.Start );

    for( int i = 1; i < count; ++i )
    {
        VECTOR2D p = arcPoint( aStart, c, aArcAngle, aAnticlockwise, double( i ) / count );
        form.Points.push_back( aFrame.ToKiCad( p ) );
    }

    form.Points.push_back( form.End );
    return form;
}


SHAPE_POLY_SET ShapeToPolySet( const SOURCE_DESIGN_SHAPE& aShape, const FRAME& aFrame )
{
    SHAPE_POLY_SET poly;
    poly.AddOutline( shapeToChain( aShape, aFrame ) );

    for( const OBJECT* obj : aShape.Cutouts )
    {
        if( const SOURCE_SHAPE* cutout = dynamic_cast<const SOURCE_SHAPE*>( obj ) )
            poly.AddHole( shapeToChain( *cutout, aFrame ) );
    }

    return poly;
}


SHAPE_POLY_SET ShapeItemArea( const SOURCE_SHAPE_ITEM& aItem, const FRAME& aFrame, int aMaxError )
{
    SHAPE_POLY_SET             area;
    const SOURCE_DESIGN_SHAPE* shape = dynamic_cast<const SOURCE_DESIGN_SHAPE*>( aItem.Shape );

    if( !shape )
        return area;

    if( shape->IsClosed() && aItem.Filled )
    {
        area = ShapeToPolySet( *shape, aFrame );

        // Boolean operations need straight segments
        area.ClearArcs();
    }

    const SOURCE_LINE_STYLE* style = dynamic_cast<const SOURCE_LINE_STYLE*>( aItem.Style );
    int                      width = style ? lengthOf( style->Width ) : 0;

    std::vector<const SOURCE_SHAPE*> contours{ shape };

    for( const OBJECT* obj : shape->Cutouts )
        contours.push_back( dynamic_cast<const SOURCE_SHAPE*>( obj ) );

    for( const SOURCE_SHAPE* contour : contours )
    {
        for( const SPAN& span : contour&& width > 0 ? spansOf( *contour ) : std::vector<SPAN>() )
        {
            ARC_FORM f = ArcForm( span.Start, span.End, span.ArcAngle, span.Anticlockwise, aFrame );

            // Approximate inside the source outline to preserve clearance.
            if( f.IsArc() )
                TransformArcToPolygon( area, f.Start, f.Mid, f.End, width, aMaxError, ERROR_INSIDE );

            for( size_t i = 1; i < f.Points.size(); ++i )
                TransformOvalToPolygon( area, f.Points[i - 1], f.Points[i], width, aMaxError, ERROR_INSIDE );
        }
    }

    area.Simplify();
    return area;
}


std::vector<PCB_SHAPE*> AddShapeItem( BOARD_ITEM_CONTAINER& aContainer, const SOURCE_SHAPE_ITEM& aItem,
                                      const FRAME& aFrame, const LAYER_MAPPER& aLayers, PCB_LAYER_ID aLayer )
{
    std::vector<PCB_SHAPE*>          out;
    const SOURCE_DESIGN_SHAPE*       shape = dynamic_cast<const SOURCE_DESIGN_SHAPE*>( aItem.Shape );
    const SOURCE_LAYERED_SHAPE_ITEM* layered = dynamic_cast<const SOURCE_LAYERED_SHAPE_ITEM*>( &aItem );
    PCB_LAYER_ID layer = aLayer == UNDEFINED_LAYER && layered ? aLayers.Map( layered->Layer ) : aLayer;

    if( !shape || layer == UNDEFINED_LAYER )
        return out;

    const SOURCE_LINE_STYLE* style = dynamic_cast<const SOURCE_LINE_STYLE*>( aItem.Style );
    STROKE_PARAMS            stroke( style ? lengthOf( style->Width ) : 0, LINE_STYLE::SOLID );
    std::vector<SPAN>        spans = spansOf( *shape );
    BOARD_ITEM*              parent = dynamic_cast<BOARD_ITEM*>( &aContainer );

    auto add = [&]( SHAPE_T aType ) -> PCB_SHAPE*
    {
        PCB_SHAPE* s = new PCB_SHAPE( parent, aType );
        s->SetLayer( layer );
        s->SetStroke( stroke );
        aContainer.Add( s );
        out.push_back( s );
        return s;
    };

    if( shape->IsClosed() && aItem.Filled )
    {
        PCB_SHAPE* poly = add( SHAPE_T::POLY );
        poly->SetPolyShape( ShapeToPolySet( *shape, aFrame ) );
        poly->SetFilled( true );
        return out;
    }

    // Two half arcs in the same direction are a circle
    if( shape->IsClosed() && spans.size() == 2 && spans[0].ArcAngle == 180000 && spans[1].ArcAngle == 180000
        && spans[0].Anticlockwise == spans[1].Anticlockwise )
    {
        VECTOR2I   a = aFrame.ToKiCad( spans[0].Start );
        PCB_SHAPE* circle = add( SHAPE_T::CIRCLE );
        circle->SetCenter( ( a + aFrame.ToKiCad( spans[1].Start ) ) / 2 );
        circle->SetEnd( a );
        return out;
    }

    for( const SPAN& span : spans )
    {
        ARC_FORM form = ArcForm( span.Start, span.End, span.ArcAngle, span.Anticlockwise, aFrame );

        if( form.IsArc() )
            add( SHAPE_T::ARC )->SetArcGeometry( form.Start, form.Mid, form.End );

        for( size_t i = 1; i < form.Points.size(); ++i )
        {
            PCB_SHAPE* seg = add( SHAPE_T::SEGMENT );
            seg->SetStart( form.Points[i - 1] );
            seg->SetEnd( form.Points[i] );
        }
    }

    return out;
}


std::unique_ptr<PAD> CreatePad( FOOTPRINT* aParent, const SOURCE_FREE_PAD& aPad, const SOURCE_PAD_STYLE& aStyle,
                                const FRAME& aFrame, const LAYER_MAPPER& aLayers )
{
    std::unique_ptr<PAD> pad = std::make_unique<PAD>( aParent );
    int32_t              size = aStyle.Size;
    int32_t              length = aStyle.Length ? aStyle.Length : aStyle.Size;
    EASYPC::PAD_SHAPE    shape = aStyle.Shape;
    bool                 slot = aStyle.Drill != 0 && aStyle.DrillShape != EASYPC::PAD_SHAPE::ROUND;
    int32_t              drillX = aStyle.Drill;
    int32_t              drillY = slot ? aStyle.DrillLength : aStyle.Drill;

    pad->SetNumber( aPad.Number ? wxString::Format( wxS( "%d" ), aPad.Number ) : wxString() );
    pad->SetPosition( aFrame.ToKiCad( aPad.Position ) );
    pad->SetOrientation( FRAME::Angle( aPad.Angle ) );

    // Unplated holes use the hole for mask; plated holes use the land.
    bool holeOnly = aStyle.Drill != 0 && drillX > size && drillY > length;

    // A plated hole with no land plots on no layer; KiCad needs a land, so it gets one inside the hole and no mask
    bool barePlatedHole = holeOnly && aStyle.Plated && aStyle.Size == 0;

    if( holeOnly && ( !aStyle.Plated || barePlatedHole ) )
    {
        size = drillX;
        length = drillY;
        shape = slot ? EASYPC::PAD_SHAPE::OVAL : EASYPC::PAD_SHAPE::ROUND;
    }

    setPadShape( *pad, PADSTACK::ALL_LAYERS, shape, size, length, aStyle.Dimension3 );

    LSET layers = aLayers.PadLayers( aPad.Layer, aParent != nullptr );

    if( aStyle.Drill != 0 )
    {
        pad->SetAttribute( aStyle.Plated ? PAD_ATTRIB::PTH : PAD_ATTRIB::NPTH );
        pad->SetDrillShape( slot ? PAD_DRILL_SHAPE::OBLONG : PAD_DRILL_SHAPE::CIRCLE );
        pad->SetDrillSize( VECTOR2I( lengthOf( drillX ), lengthOf( drillY ) ) );
    }
    else
    {
        pad->SetAttribute( PAD_ATTRIB::SMD );
    }

    if( barePlatedHole )
        layers &= LSET::AllCuMask();

    pad->SetLayerSet( layers );

    if( aStyle.Exceptions )
        applyPadStyleExceptions( aParent, *pad, aStyle, aLayers );

    return pad;
}


bool ApplyTextPosition( PCB_TEXT& aText, const SOURCE_TEXT_POSITION& aPosition, const FRAME& aFrame,
                        const LAYER_MAPPER& aLayers )
{
    PCB_LAYER_ID layer = aLayers.Map( aPosition.Layer );

    if( layer == UNDEFINED_LAYER )
        return false;

    const SOURCE_TEXT_STYLE*    style = dynamic_cast<const SOURCE_TEXT_STYLE*>( aPosition.TextStyle );
    int                         height = lengthOf( style ? style->Height : 0 );
    EDA_ANGLE                   angle = FRAME::Angle( NormalizeAngle( aPosition.Rotation ) );
    wxString                    text = aText.GetText();
    std::optional<TEXT_METRICS> metrics;

    if( style )
        metrics.emplace( *style );

    bool measurable = metrics && metrics->CanMeasure();

    text.Replace( wxS( "\r" ), wxEmptyString );

    // Stored text marks overbars with __; KiCad draws and measures its own markup
    wxString shown = ToKiCadMarkup( text );
    aText.SetText( shown );

    int glyph = static_cast<int>( int64_t( height ) * 3 / 4 );
    int glyphWidth = glyph;

    aText.SetLayer( layer );
    aText.SetTextThickness( style ? lengthOf( style->LineWidth ) : 0 );
    aText.SetKeepUpright( false );

    // Easy-PC advances every character by its pitch while KiCad's font is proportional, so the glyphs are
    // widened until KiCad's advance over the longest line equals the Easy-PC line width
    if( measurable )
    {
        int32_t  widest = 0;
        wxString widestLine;

        for( const wxString& line : wxSplit( text, '\n', '\0' ) )
        {
            std::string bytes = TEXT_METRICS::Encode( line );
            int32_t     width = bytes.empty() ? 0 : metrics->RunWidth( bytes, 0, bytes.size() );

            if( width > widest )
            {
                widest = width;
                widestLine = line;
            }
        }

        int advance = widest > 0 ? KIFONT::FONT::GetFont()
                                           ->StringBoundaryLimits( ToKiCadMarkup( widestLine ),
                                                                   VECTOR2I( glyph, glyph ), aText.GetTextThickness(),
                                                                   false, false, KIFONT::METRICS::Default() )
                                           .x
                                 : 0;

        if( advance > 0 )
            glyphWidth = KiROUND( static_cast<double>( glyph ) * lengthOf( widest ) / advance );
    }

    aText.SetTextSize( VECTOR2I( glyphWidth, glyph ) );

    switch( aPosition.Alignment )
    {
    case TEXT_ALIGN_RIGHT: aText.SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT ); break;
    case TEXT_ALIGN_CENTRE: aText.SetHorizJustify( GR_TEXT_H_ALIGN_CENTER ); break;
    default: aText.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT ); break;
    }

    // The first line hangs from a top-justified anchor, as the stored anchor is the first baseline
    aText.SetVertJustify( GR_TEXT_V_ALIGN_TOP );

    // Lay the text out unrotated at the origin to measure where KiCad's own font puts it
    PCB_TEXT probe( static_cast<BOARD_ITEM*>( nullptr ) );
    probe.SetTextSize( aText.GetTextSize() );
    probe.SetTextThickness( aText.GetTextThickness() );
    probe.SetHorizJustify( aText.GetHorizJustify() );
    probe.SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    probe.SetTextPos( VECTOR2I( 0, 0 ) );

    wxString firstLine = text.BeforeFirst( '\n' );

    if( text.Contains( wxS( "\n" ) ) && style )
    {
        probe.SetText( shown );
        probe.SetLineSpacing( 1.0 );
        aText.SetLineSpacing( height * style->InterlineHeightPercent / 100.0 / probe.GetInterline( nullptr ) );
    }

    // Capital baseline of the first line, from a capital without descenders
    probe.SetText( wxS( "H" ) );
    int baseline = strokeBox( probe ).GetBottom();

    probe.SetText( firstLine.IsEmpty() ? wxString( wxS( " " ) ) : ToKiCadMarkup( firstLine ) );
    BOX2I ink = strokeBox( probe );
    int   dx = 0;

    // The first glyph starts at its line cell's left edge; KiCad's font adds its own side bearing.
    // Without the Easy-PC width, KiCad's own justification stands and only the ink's bearing is removed
    if( measurable )
    {
        std::string bytes = TEXT_METRICS::Encode( firstLine );
        int32_t     width = metrics->RunWidth( bytes, 0, bytes.size() );
        int         cellLeft = 0;

        if( aPosition.Alignment == TEXT_ALIGN_RIGHT )
            cellLeft = -lengthOf( width );
        else if( aPosition.Alignment == TEXT_ALIGN_CENTRE )
            cellLeft = -lengthOf( width / 2 );

        dx = cellLeft - ink.GetLeft();
    }
    else if( aPosition.Alignment == TEXT_ALIGN_RIGHT )
    {
        dx = -ink.GetRight();
    }
    else if( aPosition.Alignment == TEXT_ALIGN_CENTRE )
    {
        dx = -ink.Centre().x;
    }
    else
    {
        dx = -ink.GetLeft();
    }

    // Capitals sit 3/16 of the height above the anchor
    VECTOR2I shift( aPosition.Mirrored ? -dx : dx, static_cast<int>( -int64_t( height ) * 3 / 16 ) - baseline );

    RotatePoint( shift, angle );

    aText.SetTextAngle( angle );
    aText.SetMirrored( aPosition.Mirrored );
    aText.SetPosition( aFrame.ToKiCad( aPosition.Position ) + shift );
    return true;
}


PCB_TEXT* AddText( BOARD_ITEM_CONTAINER& aContainer, const SOURCE_TEXT_POSITION& aPosition, const wxString& aText,
                   const FRAME& aFrame, const LAYER_MAPPER& aLayers )
{
    std::unique_ptr<PCB_TEXT> text = std::make_unique<PCB_TEXT>( dynamic_cast<BOARD_ITEM*>( &aContainer ) );
    text->SetText( aText );

    if( !ApplyTextPosition( *text, aPosition, aFrame, aLayers ) )
        return nullptr;

    aContainer.Add( text.get() );
    return text.release();
}


std::unique_ptr<FOOTPRINT> ConvertFootprint( const SOURCE_SYMBOL& aSymbol, const LAYER_MAPPER& aLayers,
                                             const LIB_ID& aId, const SOURCE_PCB_COMPONENT* aComponent,
                                             std::map<const SOURCE_FREE_PAD*, PAD*>* aPadMap )
{
    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( nullptr );
    const FRAME                frame{ POINT32{ aSymbol.OriginX, aSymbol.OriginY } };

    footprint->SetFPID( aId );
    footprint->Reference().SetText( wxS( "REF**" ) );
    footprint->Value().SetText( aComponent ? aComponent->Package : aSymbol.Name );

    for( const SOURCE_SHAPE_ITEM* item : TypedItems<const SOURCE_SHAPE_ITEM>( aSymbol.Shapes ) )
        AddShapeItem( *footprint, *item, frame, aLayers );

    for( const SOURCE_FREE_TEXT* text : TypedItems<const SOURCE_FREE_TEXT>( aSymbol.Texts ) )
        AddText( *footprint, *text, text->Text, frame, aLayers );

    for( const SOURCE_FREE_PAD* pad : TypedItems<const SOURCE_FREE_PAD>( aSymbol.Pads ) )
    {
        const SOURCE_PAD_STYLE* style = dynamic_cast<const SOURCE_PAD_STYLE*>( pad->Style );

        if( !style )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "Easy-PC symbol '%s' pad %d has no pad style" ), aSymbol.Name, pad->Number ) );
        }

        std::unique_ptr<PAD> kpad = CreatePad( footprint.get(), *pad, *style, frame, aLayers );
        wxString             number =
                aComponent ? PcbPadNumber( *aComponent, pad->Number ) : wxString::Format( wxS( "%d" ), pad->Number );

        kpad->SetNumber( number );

        for( const SOURCE_VALUE_POSITION* pos : TypedItems<const SOURCE_VALUE_POSITION>( pad->ValuePositions ) )
        {
            if( pos->Displayed && ( pos->Types & VALUE_PIN_NUMBER ) )
                AddText( *footprint, *pos, number, frame, aLayers );
        }

        if( aPadMap )
            ( *aPadMap )[pad] = kpad.get();

        footprint->Add( kpad.release(), ADD_MODE::APPEND );
    }

    const SOURCE_VALUE_POSITION* refPos = nullptr;
    const SOURCE_VALUE_POSITION* valuePos = nullptr;

    for( const SOURCE_VALUE_POSITION* pos : TypedItems<const SOURCE_VALUE_POSITION>( aSymbol.ValuePositions ) )
    {
        if( !pos->Displayed )
            continue;

        if( !refPos && ( pos->Types & VALUE_REFERENCE_NAME ) )
            refPos = pos;
        else if( !valuePos && ( pos->Types & ( VALUE_COMPONENT_NAME | VALUE_VALUES ) ) )
            valuePos = pos;
    }

    if( refPos )
        placeField( *footprint, footprint->Reference(), *refPos, frame, aLayers );

    if( valuePos )
        placeField( *footprint, footprint->Value(), *valuePos, frame, aLayers );
    else
        footprint->Value().SetVisible( false );

    return footprint;
}

} // namespace EASYPC_PCB
