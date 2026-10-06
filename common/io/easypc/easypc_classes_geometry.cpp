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

#include <io/easypc/easypc_classes_geometry.h>
#include <io/easypc/easypc_classes_geometry_pad.h>

#include <climits>


namespace EASYPC
{

namespace
{

    LOAD_TASK readUntilNull( ARCHIVE& aAr, std::vector<OBJECT*>& aItems )
    {
        while( OBJECT* obj = co_await aAr.Object() )
            aItems.push_back( obj );
    }


    LOAD_TASK skipUntilNull( ARCHIVE& aAr )
    {
        while( co_await aAr.Object() )
        {
        }
    }


    bool isDesignItem( const OBJECT* aObject )
    {
        return dynamic_cast<const SOURCE_DESIGN_ITEM*>( aObject ) != nullptr;
    }


    // Classes whose fields no importer reads; dimensions import from their free copper parts
    constexpr FIELD_ROW FREE_PAD_ARRAY[] = { { 0, INT_MAX, "b2 i" } };
    constexpr FIELD_ROW RULER_STOP_SET[] = { { 0, INT_MAX, "s o2" } };
    constexpr FIELD_ROW ERROR_MARK[] = { { 0, INT_MAX, "i o i4" }, { 8002, INT_MAX, "s" }, { 13000, INT_MAX, "1" } };

    /// A dimension after its group: units, geometry, angle units and attachments
    constexpr FIELD_ROW DIMENSION[] = { { 0, INT_MAX, "o i13" },
                                        { 14003, INT_MAX, "s 12" },
                                        { 18000, INT_MAX, "i5 1 i s2 i b2 i8 1" },
                                        { 29000, INT_MAX, "i4" },
                                        { 29002, INT_MAX, "o i o i f2 1 f" } };

} // namespace


// ---- vertices and shapes -----------------------------------------------------------------------------------------

LOAD_TASK SOURCE_SEGMENT::Load( ARCHIVE& aAr )
{
    X = aAr.I32();
    Y = aAr.I32();
    ArcAngle = aAr.I32();
    Anticlockwise = aAr.InBool();

    // The shape format in force decides, after the flag, whether a next vertex follows
    HasNextRef = aAr.ShapeFormat() < 3;

    if( HasNextRef )
        Next = co_await aAr.ObjectAs<SOURCE_SEGMENT>();

    co_await LoadTail( aAr );
}


LOAD_TASK SOURCE_SEGMENT::LoadTail( ARCHIVE& aAr )
{
    co_return;
}


LOAD_TASK SOURCE_COMMON_SEGMENT::LoadTail( ARCHIVE& aAr )
{
    Shape = co_await aAr.Object();
}


int SOURCE_COMMON_SEGMENT::ItemFormat( const ARCHIVE& aAr ) const
{
    const SOURCE_COMMON_SHAPE* owner = dynamic_cast<const SOURCE_COMMON_SHAPE*>( Shape );

    return owner && isDesignItem( owner->Parent ) ? owner->Parent->ItemFormat( aAr ) : 0;
}


LOAD_TASK SOURCE_SHAPE::LoadShape( ARCHIVE& aAr, int aFormat )
{
    Format = aFormat;

    // The shape format persists across later shapes, nested ones included
    aAr.SetShapeFormat( aFormat );

    if( aFormat < 2 )
    {
        for( uint32_t i = aAr.Count(); i > 0; --i )
            Segments.push_back( co_await aAr.Object() );

        co_return;
    }

    SOURCE_SEGMENT* first = nullptr;
    SOURCE_SEGMENT* prev = nullptr;

    while( OBJECT* obj = co_await aAr.Object() )
    {
        Segments.push_back( obj );
        SOURCE_SEGMENT* cur = dynamic_cast<SOURCE_SEGMENT*>( obj );

        // Format 3 stores no next vertex, so the shape links them itself
        if( prev && cur && aAr.ShapeFormat() > 2 )
            prev->Next = cur;

        if( !first )
            first = cur;

        prev = cur;
    }

    if( aAr.ShapeFormat() > 2 )
    {
        HasClosed = true;
        Closed = aAr.U8() != 0;

        if( Closed && prev && first )
            prev->Next = first;
    }
}


LOAD_TASK SOURCE_SHAPE::Load( ARCHIVE& aAr )
{
    co_await LoadShape( aAr, aAr.ShapeFormat() );
}


bool SOURCE_SHAPE::IsClosed() const
{
    if( HasClosed )
        return Closed;

    const SOURCE_SEGMENT* last = Segments.empty() ? nullptr : dynamic_cast<const SOURCE_SEGMENT*>( Segments.back() );
    return last && last->Next;
}


LOAD_TASK SOURCE_COMMON_SHAPE::Load( ARCHIVE& aAr )
{
    const bool early = Schema > 1;
    int        fmt = 1;

    if( early )
        Parent = co_await aAr.Object();

    if( isDesignItem( Parent ) )
    {
        int v = Parent->ItemFormat( aAr );
        fmt = v >= 27001 ? 3 : ( v >= 2106 ? 2 : 1 );
    }

    co_await LoadShape( aAr, fmt );

    if( !early )
        Parent = co_await aAr.Object();
}


int SOURCE_COMMON_SHAPE::ItemFormat( const ARCHIVE& aAr ) const
{
    return isDesignItem( Parent ) ? Parent->ItemFormat( aAr ) : 0;
}


LOAD_TASK SOURCE_DESIGN_SHAPE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_COMMON_SHAPE::Load( aAr );

    if( isDesignItem( Parent ) && Parent->ItemFormat( aAr ) > 12001 )
        co_await readUntilNull( aAr, Cutouts );
}


// ---- shape items -------------------------------------------------------------------------------------------------

LOAD_TASK SOURCE_SHAPE_ITEM::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    Style = co_await aAr.Object();
    Filled = aAr.InBool();
    Shape = co_await aAr.Object();

    if( v > 12000 )
        co_await aAr.Object(); // hatch style

    if( v >= 8000 )
        aAr.Skip( 16 ); // fill type and colours
}


LOAD_TASK SOURCE_LAYERED_SHAPE_ITEM::Load( ARCHIVE& aAr )
{
    co_await         SOURCE_SHAPE_ITEM::Load( aAr );
    Layer = co_await aAr.Object();
}


LOAD_TASK SOURCE_AREA::Load( ARCHIVE& aAr )
{
    co_await SOURCE_LAYERED_SHAPE_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    if( v >= 4000 && v < 8000 )
    {
        aAr.SkipString();
        Type = aAr.I32();
    }
    else if( v >= 8000 )
    {
        Type = aAr.I32();
        Net = co_await    aAr.Object();
        Copper = co_await aAr.Object();

        if( v > 11000 )
            Flags = aAr.I32();

        if( v > 8999 )
        {
            Height = aAr.I32();

            if( v >= 12008 )
                aAr.Skip( 1 );

            if( v > 12999 )
                Name = aAr.ReadString();
        }
    }

    // Nested loads may update the archive's current item format.
    if( ItemFormat( aAr ) > 17999 )
        co_await SkipFields( aAr, THERMAL_RULE_SET );

    if( v > 26000 )
        aAr.Skip( 1 );

    // The special-highlight bit is runtime state, cleared on load
    Flags &= 0xbfffffff;
}


LOAD_TASK SOURCE_FREE_COPPER::Load( ARCHIVE& aAr )
{
    co_await SOURCE_LAYERED_SHAPE_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    Terminals = co_await aAr.Object();

    if( v > 3999 )
        Type = aAr.I32();

    if( v > 5999 )
        aAr.InBool();

    if( v > 10000 )
        PinNumber = aAr.I32();

    if( v > 17999 )
        aAr.InBool();

    if( v > 19999 )
        aAr.InBool();
}


LOAD_TASK SOURCE_FREE_PAD::Load( ARCHIVE& aAr )
{
    co_await SOURCE_CONNECT_POINT::Load( aAr );

    const uint32_t v = static_cast<uint32_t>( ItemFormat( aAr ) );

    Layer = co_await aAr.Object();

    if( v < 10003 )
    {
        OldPinName = co_await   aAr.Object();
        OldPinNumber = co_await aAr.Object();
    }
    else
    {
        ValuePositions = co_await aAr.Object();
    }

    Number = aAr.I32();

    if( v >= 6000 && v <= 8002 )
        aAr.InBool();

    // Flags, a BOOL before 11000
    if( v > 8999 && v < 11000 )
        aAr.InBool();
    else if( v > 8999 )
        aAr.Skip( 4 );
}


// ---- text --------------------------------------------------------------------------------------------------------

LOAD_TASK SOURCE_TEXT_POSITION::LoadRemainder( ARCHIVE& aAr )
{
    TextStyle = co_await aAr.Object();
    Mirrored = aAr.InBool();
    Position = ReadPoint( aAr );
    Rotation = aAr.I32();
    Layer = co_await aAr.Object();
    Alignment = aAr.I32();

    if( ItemFormat( aAr ) > 18999 )
    {
        aAr.Skip( 9 ); // box type and colour
        co_await aAr.Object();
    }

    // A schematic never shows mirrored text
    if( aAr.Kind() == DOC_KIND::SCHEMATIC )
        Mirrored = false;
}


LOAD_TASK SOURCE_TEXT_POSITION::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );
    co_await LoadRemainder( aAr );
}


LOAD_TASK SOURCE_FREE_TEXT::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    if( v > 16000 )
    {
        co_await LoadRemainder( aAr );
        Text = aAr.ReadString();
        aAr.InBool();

        if( v > 27999 )
        {
            aAr.InBool();
            aAr.Skip( 4 );
        }

        co_return;
    }

    Text = aAr.ReadString();
    Position = ReadPoint( aAr );
    Mirrored = aAr.InBool() && aAr.Kind() != DOC_KIND::SCHEMATIC;
    Rotation = aAr.I32();
    TextStyle = co_await aAr.Object();
    Layer = co_await     aAr.Object();

    if( ItemFormat( aAr ) > 5999 )
        aAr.InBool();
}


LOAD_TASK SOURCE_VALUE_POSITION::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );
    co_await LoadRemainder( aAr );

    const uint32_t v = static_cast<uint32_t>( ItemFormat( aAr ) );

    Types = v < 19000 ? aAr.U32() : aAr.U64();
    Displayed = aAr.Bool();

    if( v >= 11002 )
        Attribute = aAr.ReadString();

    // An all-ones mask from these versions means reference only
    if( v >= 19000 && v <= 20999 && Types == ~uint64_t( 0 ) )
        Types = 0x80000000;
}


LOAD_TASK SOURCE_TEXT_INSTANCE::Load( ARCHIVE& aAr )
{
    co_await        SOURCE_DESIGN_ITEM::Load( aAr );
    Text = co_await aAr.Object();
}


// ---- groups and origins ------------------------------------------------------------------------------------------

LOAD_TASK SOURCE_GROUP::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    if( v <= 10999 )
    {
        // A temporary design list serialized in place
        SOURCE_DESIGN_LIST list;
        co_await           aAr.Embedded( list, "CDesignList" );
        Members = list.Items;
        co_return;
    }

    Name = aAr.ReadString();
    Flags = v < 11003 ? ( aAr.Bool() ? 3 : 2 ) : aAr.I32();
    co_await readUntilNull( aAr, Members );

    if( v > 28999 )
    {
        co_await skipUntilNull( aAr );
        co_await skipUntilNull( aAr );
    }
}


LOAD_TASK SOURCE_ORIGIN::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Position = ReadPoint( aAr );
    TextStyle = co_await aAr.Object();
    Layer = co_await     aAr.Object();

    if( ItemFormat( aAr ) > 2001 )
    {
        Rotation = aAr.I32();
        Mirrored = aAr.InBool();
    }
}


void RegisterGeometryClasses( REGISTRY& aReg )
{
    Register<SOURCE_SEGMENT>( aReg, { "CSegment" } );
    Register<SOURCE_COMMON_SEGMENT>( aReg, { "CCommonSegment", "CShapeSegment" } );
    Register<SOURCE_SHAPE>( aReg, { "CShape" } );
    Register<SOURCE_COMMON_SHAPE>( aReg, { "CCommonShape" } );
    Register<SOURCE_DESIGN_SHAPE>( aReg, { "CDesignShape", "CDesignCutout" } );
    Register<SOURCE_SHAPE_ITEM>( aReg, { "CShapeItem" } );
    Register<SOURCE_LAYERED_SHAPE_ITEM>( aReg, { "CLayeredShapeItem" } );
    Register<SOURCE_AREA>( aReg, { "CArea" } );
    Register<SOURCE_FREE_COPPER>( aReg, { "CFreeCopper" } );
    Register<SOURCE_FREE_PAD>( aReg, { "CFreePad" } );
    RegisterSkipped<SOURCE_DESIGN_LIST>( aReg, "CFreePadArray", FREE_PAD_ARRAY );
    Register<SOURCE_TEXT_POSITION>( aReg, { "CTextPosition" } );
    Register<SOURCE_FREE_TEXT>( aReg, { "CFreeText" } );
    Register<SOURCE_VALUE_POSITION>( aReg, { "CValuePosition" } );
    Register<SOURCE_TEXT_INSTANCE>( aReg, { "CTextInstance" } );
    Register<SOURCE_GROUP>( aReg, { "CGroup" } );
    RegisterSkipped<SOURCE_GROUP>( aReg, "CDimension", DIMENSION );
    Register<SOURCE_ORIGIN>( aReg, { "COrigin", "CPlaceOrigin" } );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CError", ERROR_MARK );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CRulerStopSet", RULER_STOP_SET );
    Register<SOURCE_DESIGN_LIST>( aReg, { "CAreaArray", "CFreeCopperArray", "CFreeTextArray", "COverlayArray",
                                          "CTextInstanceArray", "CValuePositionArray", "CRulerStopArray",
                                          "CRulerStopSetArray", "COriginList", "CErrorArray", "CGroupList" } );
}

} // namespace EASYPC
