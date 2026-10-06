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

#include <io/easypc/easypc_classes_root.h>
#include <io/easypc/easypc_classes_connectivity.h>
#include <io/easypc/easypc_classes_library.h>
#include <io/easypc/easypc_classes_project.h>

#include <algorithm>
#include <climits>

#include <ki_exception.h>
#include <wx/translation.h>


namespace EASYPC
{

namespace
{

    /// Design colours before 18000: a fixed sequence of colours and flags
    constexpr FIELD_ROW LEGACY_COLOURS[] = { { 0, INT_MAX, "i2 b i6" },
                                             { 9000, INT_MAX, "i" },
                                             { 0, INT_MAX, "b i b i b i5 b i b8" },
                                             { 0, 0x837, "b" },
                                             { 0x838, INT_MAX, "b2" },
                                             { 2001, INT_MAX, "i b" },
                                             { 3000, INT_MAX, "i b" },
                                             { 6000, INT_MAX, "b2 i3" },
                                             { 7000, INT_MAX, "o a b i b17" },
                                             { 7001, INT_MAX, "b" },
                                             { 8000, INT_MAX, "b4" },
                                             { 8000, 0x1f42, "b2" },
                                             { 8000, INT_MAX, "b s" },
                                             { 0x1f43, INT_MAX, "b8 i4" },
                                             { 9000, INT_MAX, "b i" },
                                             { 9001, INT_MAX, "b i" },
                                             { 0x2712, INT_MAX, "b6 f" },
                                             { 0x2716, INT_MAX, "b" },
                                             { 0x2717, INT_MAX, "i2 b i2 b i2 b i b i b i b" },
                                             { 12001, INT_MAX, "i b" },
                                             { 0x2ee6, INT_MAX, "i b" },
                                             { 0x3e81, INT_MAX, "i b" },
                                             { 13000, INT_MAX, "b2 i b3 i i b2 i b2" },
                                             { 0x32c9, INT_MAX, "i b" },
                                             { 14000, INT_MAX, "i b i b b21" },
                                             { 14001, INT_MAX, "b" },
                                             { 0x36b3, INT_MAX, "b2" },
                                             { 15000, INT_MAX, "i b2 i b4" },
                                             { 16001, INT_MAX, "b2" },
                                             { 17000, INT_MAX, "b i b i" } };


    /// The design defaults after the net class
    constexpr FIELD_ROW DEFAULTS_TAIL[] = { { 7000, INT_MAX, "o4 s i3 b i2" }, { 8001, INT_MAX, "b s o b s o3" },
                                            { 0x1f43, INT_MAX, "s" },          { 9000, INT_MAX, "o" },
                                            { 0x2716, INT_MAX, "s" },          { 0x2717, INT_MAX, "o2 s i3 o2 s 1 i3" },
                                            { 12001, INT_MAX, "o2 s" },        { 0x2ee4, INT_MAX, "s 12" },
                                            { 0x2ee6, INT_MAX, "o4 s" },       { 14000, INT_MAX, "o6 s3 i3 1 i2 13" },
                                            { 0x36b2, INT_MAX, "s" },          { 0x36b3, INT_MAX, "1 s i2" },
                                            { 16001, INT_MAX, "i s" },         { 18000, INT_MAX, "1 i2 1 s i2 13" } };


    /// The design from the hatch styles to the grids
    constexpr FIELD_ROW DESIGN_TAIL[] = { { 12000, INT_MAX, "o" }, { 16000, INT_MAX, "o2" }, { 17000, INT_MAX, "o" },
                                          { 22000, INT_MAX, "o" }, { 0x6d62, INT_MAX, "s" }, { 29001, INT_MAX, "o" },
                                          { 30000, INT_MAX, "o" }, { 0x7531, INT_MAX, "o" }, { 0x36b1, INT_MAX, "o" } };


    // Classes whose fields no importer reads
    constexpr FIELD_ROW UNITS[] = { { 0, INT_MAX, "i2" } };
    constexpr FIELD_ROW DESIGN_VIEW[] = { { 0, INT_MAX, "i3" } };
    constexpr FIELD_ROW ONE_LIST[] = { { 0, INT_MAX, "o" } };
    constexpr FIELD_ROW GRID[] = { { 0, INT_MAX, "s b i2 b i2 b i b2 i5 b" },
                                   { 17000, INT_MAX, "b" },
                                   { 20000, INT_MAX, "b" } };
    constexpr FIELD_ROW SCREEN_GRID[] = { { 0, INT_MAX, "i" }, { 9000, INT_MAX, "i b3" }, { 13000, INT_MAX, "b" } };


    /// Set a parameter, growing the array only when the value differs from the implied 0
    void setGrow( std::vector<int32_t>& aArray, size_t aIndex, int32_t aValue )
    {
        if( ( aIndex < aArray.size() ? aArray[aIndex] : 0 ) == aValue )
            return;

        if( aArray.size() <= aIndex )
            aArray.resize( aIndex + 1, 0 );

        aArray[aIndex] = aValue;
    }


    void clearIfSet( std::vector<int32_t>& aArray, size_t aIndex )
    {
        if( aIndex < aArray.size() )
            aArray[aIndex] = 0;
    }


    LOAD_TASK loadMember( ARCHIVE& aAr, OBJECT& aMember, const char* aClass, bool aDesignOwned )
    {
        if( DESIGN_MEMBER* member = dynamic_cast<DESIGN_MEMBER*>( &aMember ) )
            member->DesignOwned = aDesignOwned;

        co_await aAr.Embedded( aMember, aClass );
    }


    /// The working grid before 14001
    struct SOURCE_OLD_WORKING_GRID : DESIGN_MEMBER
    {
        LOAD_TASK Load( ARCHIVE& aAr ) override
        {
            // Only a constructed owner is tested; without one the owner tag is always read
            if( !DesignOwned || ItemFormat( aAr ) >= 0x835 )
                co_await DESIGN_MEMBER::Load( aAr );

            aAr.Skip( ItemFormat( aAr ) >= 9000 ? 12 : 8 );
        }
    };


    struct SOURCE_ROUTING_PARAMETERS : DESIGN_MEMBER
    {
        LOAD_TASK Load( ARCHIVE& aAr ) override
        {
            // The gate is taken from the constructed owner, before the owner tag
            const int v = ItemFormat( aAr );

            co_await DESIGN_MEMBER::Load( aAr );

            // From 3000 a tool parameter map; every 2105 and 2106 file stores the six flags as 4-byte BOOLs
            if( v >= 3000 )
                aAr.Skip( 8 * size_t( aAr.Count() ) );
            else
                co_await SkipFields( aAr, v >= 0x839 ? "i6 b6 i4" : "i6" );
        }
    };


    struct SOURCE_PLACEMENT_PARAMETERS : DESIGN_MEMBER
    {
        LOAD_TASK Load( ARCHIVE& aAr ) override
        {
            co_await DESIGN_MEMBER::Load( aAr );

            if( ItemFormat( aAr ) > 0x1f41 )
                aAr.Skip( 4 * size_t( std::max( 0, aAr.I32() ) ) );
            else
                co_await SkipFields( aAr, "b4 i4" );
        }
    };


    struct SOURCE_MANUFACTURE_DATA : DESIGN_MEMBER
    {
        LOAD_TASK Load( ARCHIVE& aAr ) override
        {
            co_await DESIGN_MEMBER::Load( aAr );

            if( ItemFormat( aAr ) < 10000 )
                co_return;

            for( uint32_t i = aAr.Count(); i > 0; --i )
                aAr.SkipString();

            aAr.Skip( 4 * size_t( aAr.Count() ) );
        }
    };


    struct SOURCE_DESIGN_COLOURS : DESIGN_MEMBER
    {
        LOAD_TASK Load( ARCHIVE& aAr ) override
        {
            co_await DESIGN_MEMBER::Load( aAr );

            const int v = ItemFormat( aAr );

            if( v <= 17999 )
            {
                co_await SkipRows( aAr, v, LEGACY_COLOURS );
                co_return;
            }

            // Plain int32 counts: id to colour, then three id to flag tables
            aAr.Skip( 8 * size_t( std::max( 0, aAr.I32() ) ) );

            for( int i = 0; i < 3; ++i )
                aAr.Skip( 5 * size_t( std::max( 0, aAr.I32() ) ) );

            co_await SkipFields( aAr, "o a s f" );
        }
    };


    /// Nets without a class use the design's first net class
    void assignDefaultNetClass( OBJECT* aNets, OBJECT* aNetClasses )
    {
        const std::vector<OBJECT*>& classes = ListItems( aNetClasses );

        for( SOURCE_NET* net : TypedItems<SOURCE_NET>( aNets ) )
        {
            if( !net->NetClass && !classes.empty() )
                net->NetClass = classes.front();
        }
    }

} // namespace


POINT32 ReadPoint( ARCHIVE& aAr )
{
    POINT32 p;
    p.X = aAr.I32();
    p.Y = aAr.I32();
    return p;
}


LOAD_TASK SkipMember( ARCHIVE& aAr, const char* aClass, bool aDesignOwned )
{
    std::unique_ptr<OBJECT> member = ARCHIVE::Create( aClass );
    co_await                loadMember( aAr, *member, aClass, aDesignOwned );
}


LOAD_TASK DESIGN_MEMBER::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_DATA::Load( aAr );
    OwnerRead = true;
}


int DESIGN_MEMBER::ItemFormat( const ARCHIVE& aAr ) const
{
    // Before the owner tag is read the owner is whatever the constructor set
    if( !OwnerRead )
        return DesignOwned ? aAr.DocumentVersion() : aAr.CurrentReadFormat();

    return SOURCE_DESIGN_DATA::ItemFormat( aAr );
}


LOAD_TASK SOURCE_CHECK_PARAMETERS::Load( ARCHIVE& aAr )
{
    // The gate is taken from the constructed owner; older designs store no checks
    if( ItemFormat( aAr ) <= 12999 )
        co_return;

    co_await DESIGN_MEMBER::Load( aAr );

    for( uint32_t i = aAr.Count(); i > 0; --i )
    {
        int32_t key = aAr.I32();
        Values[key] = aAr.I32();
    }
}


LOAD_TASK SOURCE_DESIGN_PARAMETERS::Load( ARCHIVE& aAr )
{
    static constexpr FIELD_ROW ROWS[] = {
        { 13000, INT_MAX, "1" }, { 14000, INT_MAX, "s" }, { 0x36b2, INT_MAX, "s" }, { 17000, INT_MAX, "s" }
    };

    co_await DESIGN_MEMBER::Load( aAr );

    const int v = ItemFormat( aAr );

    aAr.Skip( 4 * size_t( std::max( 0, aAr.I32() ) ) ); // wire lengths

    if( v > 8999 )
    {
        aAr.SkipString();

        for( uint32_t i = aAr.Count(); i > 0; --i )
            Params.push_back( aAr.I32() );
    }

    co_await SkipRows( aAr, v, ROWS );

    if( v > 17999 )
        co_await SkipFields( aAr, THERMAL_RULE_SET );

    if( v >= 22000 )
        aAr.Skip( 4 );

    // Parameters newer than the file take their defaults
    if( v < 22000 )
    {
        setGrow( Params, 24, 0x6338 );
        setGrow( Params, 25, 0xf80c );
        clearIfSet( Params, 27 );
        clearIfSet( Params, 31 );
        setGrow( Params, 28, Param( 4 ) );
    }

    if( v < 26000 )
    {
        setGrow( Params, 37, 0xfe );
        clearIfSet( Params, 38 );
    }
}


LOAD_TASK SOURCE_DEFAULTS::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    // Line style, hatch style from 12001, track and pad styles
    co_await             SkipFields( aAr, v > 12000 ? "o4" : "o3" );
    TextStyle = co_await aAr.Object();
    co_await             SkipFields( aAr, "o s5 o b4" );

    if( v > 0x7d4 )
        aAr.SkipString();

    if( v > 0x7d6 )
        NetClass = aAr.ReadString();

    co_await SkipRows( aAr, v, DEFAULTS_TAIL );
}


LOAD_TASK SOURCE_DESIGN::Load( ARCHIVE& aAr )
{
    // OLE items
    for( uint32_t i = aAr.U32(); i > 0; --i )
        co_await aAr.Object();

    size_t versionPos = aAr.Pos();
    Version = aAr.I32();

    if( Version > NEWEST_ITEM_FORMAT && !( Version >= 30000 && Version < 31000 ) )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Easy-PC design format %d at offset %zu is too new." ), Version, versionPos ) );
    }

    const int v = Version;

    aAr.SetDocumentVersion( v );
    aAr.SetBooleanIsByte( v > 7999 );
    Product = v >= 0x1f44 ? aAr.I32() : PRODUCT_EASYPC;
    aAr.SetProduct( Product );
    co_await SkipFields( aAr, v < 0x834 ? "i4 b i" : "i4" ); // design area, old units

    Areas = co_await   aAr.Object();
    Boards = co_await  aAr.Object();
    Coppers = co_await aAr.Object();

    if( v > 2999 )
        co_await aAr.Object(); // layer types

    Layers = co_await aAr.Object();

    if( v > 0x2713 )
        co_await aAr.Object(); // layer spans

    NetClasses = co_await         aAr.Object();
    Symbols = co_await            aAr.Object();
    Nets = co_await               aAr.Object();
    Pads = co_await               aAr.Object();
    co_await                      aAr.Object(); // pad styles
    Texts = co_await              aAr.Object();
    co_await                      aAr.Object(); // text styles
    Spacings = co_await           aAr.Object();
    Attributes = co_await         aAr.Object();
    co_await                      SkipFields( aAr, "o2" ); // attribute names, components
    ComponentInstances = co_await aAr.Object();
    co_await                      aAr.Object(); // errors
    Defaults = co_await           aAr.ObjectAs<SOURCE_DEFAULTS>();
    co_await                      aAr.Object();            // track styles
    co_await                      SkipFields( aAr, "o2" ); // line styles, schematic components
    Buses = co_await              aAr.Object();
    SymbolInstances = co_await    aAr.Object();

    // Old routing values, bus terminal offset, origins and overlays
    co_await SkipFields( aAr, v < 0x835 ? "i3 o2" : "i o2" );
    IsolationGap = aAr.I32();

    if( v > 3999 )
        aAr.Skip( 8 ); // minimum copper area

    ThermalRelief = aAr.I32();

    if( v > 15000 )
        aAr.Skip( 4 );

    if( v > 0x3e81 )
        ThermalAngled = aAr.Bool();

    aAr.Skip( 4 );

    if( v > 0x7d5 )
    {
        aAr.InBool();
        Origin = ReadPoint( aAr );
    }

    if( v > 0x835 )
        Groups = co_await aAr.Object();

    if( v > 2999 )
    {
        co_await SkipFields( aAr, "s b o" );
        AdjustTextRotation = aAr.InBool();
    }

    if( v >= 0x2713 )
        ValuePositions = co_await aAr.Object();

    co_await SkipRows( aAr, v, DESIGN_TAIL );

    if( v < 0x36b1 )
    {
        co_await SkipMember( aAr, "CScreenGrid", true );
        co_await SkipMember( aAr, "COldWorkingGrid", true );
    }

    co_await SkipMember( aAr, "CDesignColours", true );
    co_await SkipMember( aAr, "CDesignView", true );

    if( v > 0x833 )
        co_await SkipMember( aAr, "CUnits", true );

    if( v > 0x834 )
        co_await SkipMember( aAr, "CRoutingParameters", true );

    if( v > 2999 )
        co_await SkipMember( aAr, "CPlacementParameters", true );

    if( v > 7999 )
        co_await SkipFields( aAr, "o2" ); // pictures

    if( v > 8000 )
        co_await loadMember( aAr, DesignParameters, "CDesignParameters", true );

    if( v >= 9000 )
    {
        aAr.Skip( 8 );
        co_await SkipMember( aAr, "CUnits", true );
    }

    if( v > 9999 )
        co_await SkipMember( aAr, "CManufactureData", true );

    if( v >= 13000 )
    {
        co_await loadMember( aAr, DrcParameters, "CCheckParameters", true );
        co_await SkipMember( aAr, "CCheckParameters", true );
        co_await SkipFields( aAr, "o2" ); // ruler stops, sheets
    }

    if( v > 22999 )
        co_await SkipMember( aAr, "CComponentRules" );

    if( v > 23999 )
        co_await SkipMember( aAr, "CComponentColours" );

    assignDefaultNetClass( Nets, NetClasses );
}


void RegisterRootClasses( REGISTRY& aReg )
{
    Register<SOURCE_DESIGN>( aReg, { "CPcbDesign", "CScmDesign" } );
    Register<SOURCE_DESIGN_LIST>( aReg, { "CGrids" } );
    RegisterSkipped<DESIGN_MEMBER>( aReg, "CUnits", UNITS );
    Register<SOURCE_DESIGN_COLOURS>( aReg, { "CDesignColours" } );
    Register<SOURCE_ROUTING_PARAMETERS>( aReg, { "CRoutingParameters" } );
    Register<SOURCE_CHECK_PARAMETERS>( aReg, { "CCheckParameters" } );
    Register<SOURCE_PLACEMENT_PARAMETERS>( aReg, { "CPlacementParameters" } );
    Register<SOURCE_MANUFACTURE_DATA>( aReg, { "CManufactureData" } );
    Register<SOURCE_DESIGN_PARAMETERS>( aReg, { "CDesignParameters" } );
    Register<SOURCE_OLD_WORKING_GRID>( aReg, { "COldWorkingGrid" } );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CDesignView", DESIGN_VIEW );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CGrid", GRID );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CScreenGrid", SCREEN_GRID );
    Register<SOURCE_DEFAULTS>( aReg, { "CDefaults" } );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CComponentRules", ONE_LIST );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CComponentColours", ONE_LIST );
    Register<SOURCE_PROJECT>( aReg, { "CProject" } );
    Register<SOURCE_PROJECT_ITEM>( aReg, { "CProjectItem" } );
}

} // namespace EASYPC
