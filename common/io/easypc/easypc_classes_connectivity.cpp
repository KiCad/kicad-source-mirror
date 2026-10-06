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

#include <io/easypc/easypc_classes_connectivity.h>
#include <io/easypc/easypc_classes_library.h>

#include <climits>


namespace EASYPC
{

namespace
{

    // Classes whose fields no importer reads
    constexpr FIELD_ROW NET_ARRAY[] = { { 0, INT_MAX, "i" } };
    constexpr FIELD_ROW SHEET[] = { { 0, INT_MAX, "s" } };
    constexpr FIELD_ROW NAME_CHANGE[] = { { 0, INT_MAX, "i s2" } };

} // namespace


LOAD_TASK SOURCE_SYMBOL_INSTANCE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    Position = ReadPoint( aAr );
    Symbol = co_await  aAr.Object();
    Coppers = co_await aAr.Object();
    Pads = co_await    aAr.Object();
    Texts = co_await   aAr.Object();

    if( v < 10003 )
    {
        OBJECT* symbolName = co_await aAr.Object();

        // Before format 10003 the symbol name stands for the value positions; the component instance sets the types
        ConvertedValuePositions = std::make_unique<SOURCE_DESIGN_LIST>();
        ConvertedSymbolName = std::make_unique<SOURCE_VALUE_POSITION>();
        ConvertedSymbolName->Types = 0x1f;

        if( const SOURCE_TEXT_POSITION* name = dynamic_cast<const SOURCE_TEXT_POSITION*>( symbolName ) )
        {
            SOURCE_VALUE_POSITION& vp = *ConvertedSymbolName;
            vp.TextStyle = name->TextStyle;
            vp.Mirrored = name->Mirrored;
            vp.Position = name->Position;
            vp.Rotation = name->Rotation;
            vp.Layer = name->Layer;
            vp.Alignment = name->Alignment;
        }

        ConvertedValuePositions->Items.push_back( ConvertedSymbolName.get() );
        ValuePositions = ConvertedValuePositions.get();
    }
    else
    {
        ValuePositions = co_await aAr.Object();
    }

    Angle = aAr.I32();
    Gate = aAr.I32();
    Mirrored = aAr.InBool();

    if( v < 3000 )
    {
        Placed = true;
        co_return;
    }

    Placed = aAr.InBool();
    aAr.InBool(); // fixed

    if( v > 17999 )
        aAr.Skip( 8 ); // scale

    if( v > 23999 )
        aAr.Skip( 4 );
}


LOAD_TASK SOURCE_COMPONENT_INSTANCE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Component = co_await aAr.Object();
    Name = aAr.ReadString();
    SymbolInstances = co_await aAr.Object();

    const int v = ItemFormat( aAr );

    if( v < 10003 )
    {
        ValueTypes = VALUE_VALUES;

        // Four display flags in stream order
        for( uint64_t bit : { VALUE_COMPONENT_NAME, VALUE_REFERENCE_NAME, VALUE_PACKAGE_NAME, VALUE_SYMBOL_NAME } )
        {
            if( aAr.InBool() )
                ValueTypes |= bit;
        }

        // They become the types of each gate's first value position
        for( SOURCE_SYMBOL_INSTANCE* inst : TypedItems<SOURCE_SYMBOL_INSTANCE>( SymbolInstances ) )
        {
            const std::vector<OBJECT*>& vps = ListItems( inst->ValuePositions );

            if( SOURCE_VALUE_POSITION* first =
                        vps.empty() ? nullptr : dynamic_cast<SOURCE_VALUE_POSITION*>( vps.front() ) )
                first->Types = ValueTypes;
        }
    }
    else if( v >= 10005 )
    {
        ValueTypes = v < 19000 ? aAr.U32() : aAr.U64();
    }

    if( v >= 12000 )
        Suppress = aAr.I32();

    if( v > 12999 )
        co_await aAr.Object(); // sheet

    // Variants and associated parts are null tags in every known file
    if( v > 16999 )
        co_await aAr.Object();

    if( v > 21999 )
        co_await aAr.Object();
}


LOAD_TASK SOURCE_PAD_INSTANCE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    Node = co_await aAr.Object();
    Pad = co_await  aAr.Object();

    if( v < 10003 )
    {
        PinName = co_await   aAr.Object();
        PinNumber = co_await aAr.Object();

        if( v < 2103 )
            co_return;

        StyleException = co_await aAr.Object();

        if( v < 9000 )
            co_return;
    }
    else
    {
        ValuePositions = co_await aAr.Object();
        StyleException = co_await aAr.Object();
    }

    aAr.Skip( 8 ); // teardrop, testland

    if( v > 15000 )
        aAr.Skip( 4 );

    if( v > 22999 )
        aAr.Skip( 4 );

    if( v > 23999 )
        aAr.Skip( 12 );
}


LOAD_TASK SOURCE_COPPER_INSTANCE::Load( ARCHIVE& aAr )
{
    co_await          SOURCE_DESIGN_ITEM::Load( aAr );
    Copper = co_await aAr.Object();
}


LOAD_TASK SOURCE_NET::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Name = aAr.ReadString();
    Connections = co_await aAr.Object();
    Nodes = co_await       aAr.Object();
    NetClass = co_await    aAr.Object();
    aAr.InBool();
    GuardSpacing = aAr.I32();

    const int v = ItemFormat( aAr );

    if( v > 5999 )
    {
        aAr.InBool();
        aAr.Skip( 4 );
    }

    // Connection display, a BOOL before 15003
    if( v > 8999 && v < 15003 )
        aAr.InBool();
    else if( v > 8999 )
        aAr.Skip( 4 );
}


LOAD_TASK SOURCE_NET_NAME::Load( ARCHIVE& aAr )
{
    // The item fields, then the text position fields without their item fields
    co_await SOURCE_DESIGN_ITEM::Load( aAr );
    co_await LoadRemainder( aAr );
    Displayed = aAr.InBool();
}


LOAD_TASK SOURCE_NODE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Item = co_await    aAr.Object();
    NetName = co_await aAr.Object();
    Position = ReadPoint( aAr );
}


LOAD_TASK SOURCE_CONNECTION::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Node1 = co_await aAr.Object();
    Node2 = co_await aAr.Object();
    Track = co_await aAr.Object();
}


LOAD_TASK SOURCE_CONNECT_POINT::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    Position = ReadPoint( aAr );
    Style = co_await aAr.Object();
    Node = co_await  aAr.Object();
    Angle = aAr.I32();

    if( v > 8002 )
        aAr.InBool(); // fixed

    if( v > 8999 )
        aAr.Skip( v > 15000 ? 12 : 8 ); // teardrop, testland, connect type
}


LOAD_TASK SOURCE_VIA::Load( ARCHIVE& aAr )
{
    co_await SOURCE_CONNECT_POINT::Load( aAr );

    const int v = ItemFormat( aAr );

    if( v <= 8999 )
        co_return;

    aAr.InBool(); // locked

    if( v > 10000 )
        LayerSpan = co_await aAr.Object();

    if( v > 17000 )
        aAr.Skip( 1 );

    if( v >= 25000 )
        Tented = aAr.Bool();
}


LOAD_TASK SOURCE_BUS_TERMINAL::Load( ARCHIVE& aAr )
{
    co_await SOURCE_CONNECT_POINT::Load( aAr );
    BusEndPosition = ReadPoint( aAr );
}


LOAD_TASK SOURCE_TRACK::Load( ARCHIVE& aAr )
{
    co_await SOURCE_COMMON_SHAPE::Load( aAr );

    co_await         aAr.Object(); // default style
    Layer = co_await aAr.Object();

    // Both flags gate on the format of the shape's parent, and only when it exists
    if( !dynamic_cast<const SOURCE_DESIGN_ITEM*>( Parent ) )
        co_return;

    const int v = Parent->ItemFormat( aAr );

    if( v > 8002 )
        aAr.InBool();

    if( v > 8999 )
        aAr.Skip( 1 );
}


LOAD_TASK SOURCE_TRACK_SEGMENT::LoadTail( ARCHIVE& aAr )
{
    co_await SOURCE_COMMON_SEGMENT::LoadTail( aAr );

    Style = co_await aAr.Object();
    aAr.InBool();
}


LOAD_TASK SOURCE_BUS::Load( ARCHIVE& aAr )
{
    co_await SOURCE_SHAPE_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    if( v > 16000 )
        Name = aAr.ReadString();

    for( int32_t i = aAr.I32(); i > 0; --i )
        Nets.push_back( co_await aAr.Object() );

    Terminals = co_await aAr.Object();

    if( v > 16001 )
        ValuePositions = co_await aAr.Object();
}


LOAD_TASK SOURCE_BOARD::Load( ARCHIVE& aAr )
{
    co_await SOURCE_SHAPE_ITEM::Load( aAr );

    if( ItemFormat( aAr ) > 5999 )
        aAr.InBool();

    if( ItemFormat( aAr ) > 11000 )
        Plated = aAr.I32();
}


void RegisterConnectivityClasses( REGISTRY& aReg )
{
    Register<SOURCE_SYMBOL_INSTANCE>( aReg, { "CSymbolInstance" } );
    Register<SOURCE_COMPONENT_INSTANCE>( aReg, { "CComponentInstance" } );
    Register<SOURCE_PAD_INSTANCE>( aReg, { "CPadInstance" } );
    Register<SOURCE_COPPER_INSTANCE>( aReg, { "CCopperInstance" } );
    Register<SOURCE_NET>( aReg, { "CNet" } );
    RegisterSkipped<SOURCE_DESIGN_LIST>( aReg, "CNetArray", NET_ARRAY );
    Register<SOURCE_NET_NAME>( aReg, { "CNetName" } );
    Register<SOURCE_NODE>( aReg, { "CNode" } );
    Register<SOURCE_CONNECTION>( aReg, { "CConnection" } );
    Register<SOURCE_CONNECT_POINT>( aReg, { "CConnectPoint" } );
    Register<SOURCE_JUNCTION>( aReg, { "CJunction" } );
    Register<SOURCE_COPPER_TERMINAL>( aReg, { "CCopperTerminal" } );
    Register<SOURCE_VIA>( aReg, { "CVia" } );
    Register<SOURCE_BUS_TERMINAL>( aReg, { "CBusTerminal" } );
    Register<SOURCE_TRACK>( aReg, { "CTrack" } );
    Register<SOURCE_TRACK_SEGMENT>( aReg, { "CTrackSegment" } );
    Register<SOURCE_BUS>( aReg, { "CBus" } );
    Register<SOURCE_BOARD>( aReg, { "CBoard" } );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CSheet", SHEET );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CNameChange", NAME_CHANGE );
    Register<SOURCE_DESIGN_LIST>( aReg,
                                  { "CSymbolInstanceArray", "CComponentInstanceArray", "CPadInstanceArray",
                                    "CCopperInstanceArray", "CCopperTerminalArray", "CNodeArray", "CConnectionArray",
                                    "CBusArray", "CBusTerminalArray", "CBoardArray", "CSheetArray",
                                    "CComponentColoursArray", "CComponentRulesArray", "CNameChangeList" } );
}

} // namespace EASYPC
