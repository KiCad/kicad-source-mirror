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

#include <io/easypc/easypc_classes_styles.h>

#include <algorithm>
#include <climits>

#include <ki_exception.h>
#include <wx/translation.h>

#include <io/easypc/easypc_classes_geometry.h>


namespace EASYPC
{

namespace
{

    // Legacy spacing keys in stream order (below 15000)
    constexpr int32_t LEGACY_SPACING_KEYS[] = { 0x40004,  0x40002, 0x100004, 0x200004, 0x40001,  0x20002,  0x100002,
                                                0x200002, 0x20001, 0x100010, 0x200010, 0x100001, 0x200020, 0x200001,
                                                0x80004,  0x80002, 0x80008,  0x100008, 0x200008, 0x80001 };

    // Legitimate parent chains are a few objects long; this keeps a crafted cycle off the stack
    constexpr size_t MAX_PARENT_WALK = 1024;

    // Classes whose fields no importer reads
    constexpr FIELD_ROW ATTRIBUTE_ARRAY[] = { { 0, INT_MAX, "o" } };
    constexpr FIELD_ROW BARCODE_FONT[] = { { 0, INT_MAX, "i s i" } };
    constexpr FIELD_ROW HATCH_STYLE[] = { { 0, INT_MAX, "s b i f" } };
    constexpr FIELD_ROW NOTE_CATEGORY[] = { { 0, INT_MAX, "s i2 13 i2" } };

    // A layer after the net: the colour sets, flags and draw order of each version
    constexpr FIELD_ROW OLD_LAYER_TAIL[] = { { 0x7d3, INT_MAX, "i" },     { 5000, INT_MAX, "b" },
                                             { 7000, 0x2711, "b" },       { 8000, 0x2711, "i" },
                                             { 0x2712, INT_MAX, "b2 i" }, { 9000, INT_MAX, "i" },
                                             { 0x2716, INT_MAX, "i" },    { 9000, INT_MAX, "i" },
                                             { 13000, INT_MAX, "i2" },    { 14000, INT_MAX, "b" } };


    void checkItemFormat( const ARCHIVE& aAr, int aFormat )
    {
        if( aFormat > NEWEST_ITEM_FORMAT )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "Easy-PC item format %d at offset %zu is too new." ), aFormat, aAr.Pos() ) );
        }
    }

} // namespace


// ---- bases -------------------------------------------------------------------------------------------------------

LOAD_TASK SOURCE_DESIGN_LIST::Load( ARCHIVE& aAr )
{
    Parent = co_await aAr.Object();

    if( ItemFormat( aAr ) > 0x839 )
    {
        while( OBJECT* item = co_await aAr.Object() )
            Items.push_back( item );

        co_return;
    }

    // Older lists are counted
    for( uint32_t i = aAr.Count(); i > 0; --i )
        Items.push_back( co_await aAr.Object() );
}


int SOURCE_DESIGN_LIST::ItemFormat( const ARCHIVE& aAr ) const
{
    if( !Parent )
        return aAr.OrphanFormat();

    return dynamic_cast<const SOURCE_DESIGN_ITEM*>( Parent ) ? Parent->ItemFormat( aAr ) : aAr.CurrentReadFormat();
}


LOAD_TASK SOURCE_DESIGN_DATA::Load( ARCHIVE& aAr )
{
    Parent = co_await aAr.Object();
}


int SOURCE_DESIGN_DATA::ItemFormat( const ARCHIVE& aAr ) const
{
    return Parent ? aAr.CurrentReadFormat() : aAr.OrphanFormat();
}


LOAD_TASK SOURCE_DESIGN_ITEM::Load( ARCHIVE& aAr )
{
    Parent = co_await aAr.Object();

    // Taken after the parent is known because ItemFormat walks it
    int v = ItemFormat( aAr );

    Attributes = co_await aAr.Object();

    // The group, and from 29000 the group list
    co_await SkipFields( aAr, v > 28999 ? "o2" : v > 0x835 ? "o" : "" );
}


int SOURCE_DESIGN_ITEM::ItemFormat( const ARCHIVE& aAr ) const
{
    if( !Parent )
        return aAr.OrphanFormat();

    if( !dynamic_cast<const SOURCE_DESIGN_ITEM*>( Parent ) && !dynamic_cast<const SOURCE_COMMON_SHAPE*>( Parent )
        && !dynamic_cast<const SOURCE_COMMON_SEGMENT*>( Parent ) )
    {
        return aAr.CurrentReadFormat();
    }

    // Parent tags are file data, so a crafted file can make the chain cyclic
    size_t& depth = aAr.ParentWalkDepth();

    if( depth > MAX_PARENT_WALK )
        THROW_IO_ERROR(
                wxString::Format( _( "Easy-PC %s at offset %zu has a cyclic parent chain." ), ClassName, Offset ) );

    struct UNWIND
    {
        size_t& Depth;
        ~UNWIND() { --Depth; }
    } unwind{ ++depth };

    return Parent->ItemFormat( aAr );
}


void SOURCE_DESIGN_ITEM::LoadFormatted( ARCHIVE& aAr, int& aFormat )
{
    aFormat = aAr.ObjectSchema() < 2 ? 0 : aAr.I32();
    checkItemFormat( aAr, aFormat );
    aAr.SetReadFormat( aFormat );
}


void SOURCE_DESIGN_ITEM::LoadOldFormat( ARCHIVE& aAr, int& aFormat ) const
{
    if( ItemFormat( aAr ) != 0 )
        return;

    aFormat = aAr.I32();
    checkItemFormat( aAr, aFormat );
    aAr.SetReadFormat( aFormat );
}


// ---- attributes and styles ---------------------------------------------------------------------------------------

LOAD_TASK SOURCE_ATTRIBUTE_NAME::Load( ARCHIVE& aAr )
{
    static constexpr FIELD_ROW ROWS[] = { { 8001, INT_MAX, "s" }, { 0x2ee4, INT_MAX, "i" } };

    co_await SOURCE_DESIGN_ITEM::Load( aAr );
    Name = aAr.ReadString();
    co_await SkipRows( aAr, ItemFormat( aAr ), ROWS );
}


LOAD_TASK SOURCE_ATTRIBUTE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    AttributeName = co_await aAr.ObjectAs<SOURCE_ATTRIBUTE_NAME>();
    Value = aAr.ReadString();
    Displayed = aAr.InBool();
}


LOAD_TASK SOURCE_LINE_STYLE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Name = aAr.ReadString();
    Width = aAr.I32();

    if( ItemFormat( aAr ) > 10999 )
    {
        aAr.Skip( 12 ); // dash and gap lengths
    }
}


LOAD_TASK SOURCE_TYPE_FONT::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );
    aAr.Skip( 0x9c ); // a Win32 ENUMLOGFONTA
}


LOAD_TASK SOURCE_TEXT_STYLE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    Name = aAr.ReadString();
    Height = aAr.I32();
    LineWidth = aAr.I32();

    if( v > 4999 )
    {
        aAr.InBool(); // underlined
        Font = co_await aAr.Object();

        if( v < 0x32c9 )
            co_return;

        aAr.Skip( 4 * size_t( aAr.Count() ) ); // barcode parameters
    }

    if( v > 21999 )
    {
        InterlineHeightPercent = aAr.I32();
        CharWidthPercent = aAr.I32();
        Proportional = aAr.Bool();
    }

    if( v > 29999 )
        aAr.SkipString();
}


LOAD_TASK SOURCE_TRACK_STYLE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Name = aAr.ReadString();
    Width = aAr.I32();
}


LOAD_TASK SOURCE_PAD_STYLE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    Name = aAr.ReadString();
    Size = aAr.I32();
    Shape = static_cast<PAD_SHAPE>( aAr.I32() );
    Drill = aAr.I32();
    Length = aAr.I32();
    Exceptions = co_await aAr.Object();

    if( v > 8999 )
    {
        Plated = aAr.InBool();
        Dimension3 = aAr.I32();
    }

    // Schematic pad styles below 11004 lose their drill in memory
    if( v < 0x2afc && aAr.Kind() == DOC_KIND::SCHEMATIC )
        Drill = 0;

    if( v > 12000 )
    {
        DrillShape = static_cast<PAD_SHAPE>( aAr.I32() );
        DrillLength = aAr.I32();
        DrillCornerRadius = aAr.I32();

        // No known file turns a slot, and an unturned slot would be drilled across its intended axis
        if( aAr.Bool() && DrillShape != PAD_SHAPE::ROUND )
            THROW_IO_ERROR( wxString::Format( _( "Unsupported Easy-PC turned slot in pad style '%s'." ), Name ) );
    }

    if( v > 17999 )
        aAr.Skip( 4 );

    if( v > 23999 )
        aAr.Skip( 4 ); // thermal spoke width
}


LOAD_TASK SOURCE_PAD_STYLE_EXCEPTION::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Layer = co_await aAr.Object();
    Shape = static_cast<PAD_SHAPE>( aAr.I32() );
    Size = aAr.I32();
    Length = aAr.I32();

    const int v = ItemFormat( aAr );

    if( v > 10000 )
        Dimension3 = aAr.I32();

    if( v > 17999 )
        aAr.Skip( 4 );

    if( v > 23999 )
        aAr.Skip( 4 );
}


// ---- layers ------------------------------------------------------------------------------------------------------

LOAD_TASK SOURCE_LAYER_TYPE::Load( ARCHIVE& aAr )
{
    static constexpr FIELD_ROW ROWS[] = { { 13000, INT_MAX, "b" },
                                          { 21000, INT_MAX, "b i" },
                                          { 0x6d63, INT_MAX, "13" } };

    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Name = aAr.ReadString();
    Usage = aAr.I32();
    AllLayerPadsEnabled = aAr.InBool();
    ViasEnabled = aAr.InBool();
    SurfacePadInstancesEnabled = aAr.InBool();
    AllLayerPadInstancesEnabled = aAr.InBool();

    const int v = ItemFormat( aAr );

    if( v > 8999 )
    {
        Oversize = aAr.I32();
        OversizeType = static_cast<LAYER_OVERSIZE_TYPE>( aAr.I32() );
    }

    SurfacePadsEnabled = v < 0x2ee6 ? AllLayerPadsEnabled : aAr.Bool();
    co_await SkipRows( aAr, v, ROWS );
}


LOAD_TASK SOURCE_LAYER::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    Name = aAr.ReadString();
    Side = static_cast<LAYER_SIDE>( aAr.I32() );

    if( v < 3000 )
        TypeName = aAr.ReadString();
    else
        Type = co_await aAr.ObjectAs<SOURCE_LAYER_TYPE>();

    Usage = aAr.I32();
    Displayed = aAr.InBool();

    if( v > 17999 )
    {
        // A colour map with a plain int32 count, then the bias
        aAr.Skip( 8 * size_t( aAr.U32() ) + 4 );
        Net = co_await       aAr.Object();
        co_await             SkipFields( aAr, "b3 i b" );
        Terminals = co_await aAr.Object();
    }
    else
    {
        aAr.Skip( 40 ); // nine colours and the bias
        Net = co_await aAr.Object();
        co_await       SkipRows( aAr, v, OLD_LAYER_TAIL );

        if( v > 0x3a99 )
            Terminals = co_await aAr.Object();
    }

    if( TypeName.IsEmpty() )
        co_return;

    // A type name: a design types the layer by its usage, a library item makes a usage 4 type
    ConvertedType = std::make_unique<SOURCE_LAYER_TYPE>();
    ConvertedType->Name = TypeName;
    ConvertedType->Usage = aAr.IsDesign() ? Usage : LAYER_USAGE_NON_ELECTRICAL;
    Type = ConvertedType.get();

    if( TypeName.CmpNoCase( wxS( "Silk Screen" ) ) == 0 )
    {
        Type->SurfacePadInstancesEnabled = false;
        Type->AllLayerPadInstancesEnabled = false;
        Type->ViasEnabled = false;
        Type->AllLayerPadsEnabled = false;
        Type->SurfacePadsEnabled = false;
    }
}


LOAD_TASK SOURCE_LAYER_SPAN::Load( ARCHIVE& aAr )
{
    co_await SOURCE_LAYER::Load( aAr );

    TopLayer = co_await    aAr.ObjectAs<SOURCE_LAYER>();
    BottomLayer = co_await aAr.ObjectAs<SOURCE_LAYER>();
}


LOAD_TASK SOURCE_LAYER_ARRAY::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_LIST::Load( aAr );

    aAr.Skip( 4 ); // next colour

    // The loader moves the first bottom layer set to the end
    auto it = std::find_if( Items.begin(), Items.end(),
                            []( const OBJECT* aItem )
                            {
                                const SOURCE_LAYER* layer = dynamic_cast<const SOURCE_LAYER*>( aItem );
                                return layer && layer->Side == LAYER_SIDE::BOTTOM
                                       && ( layer->EffectiveUsage() & LAYER_USAGE_SET );
                            } );

    if( it != Items.end() )
        std::rotate( it, it + 1, Items.end() );
}


// ---- net classes and spacings ------------------------------------------------------------------------------------

LOAD_TASK SOURCE_NET_CLASS::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Name = aAr.ReadString();
    Type = aAr.I32();
    TrackStyle1 = co_await aAr.ObjectAs<SOURCE_TRACK_STYLE>();
    TrackStyle2 = co_await aAr.ObjectAs<SOURCE_TRACK_STYLE>();
    ViaStyle = co_await    aAr.ObjectAs<SOURCE_PAD_STYLE>();

    const int v = ItemFormat( aAr );

    if( v > 0x32c9 )
    {
        aAr.Skip( 14 ); // track length difference and min/max limits with their switches
    }

    // The thermal rules, own colour and connection display
    if( v > 17999 )
    {
        co_await SkipFields( aAr, THERMAL_RULE_SET );
        aAr.Skip( 9 );
    }

    if( v > 24999 )
    {
        TentedVias = aAr.I32();
        aAr.Skip( 1 ); // diff pair switch
        DiffPairGap = aAr.I32();
        aAr.Skip( 5 ); // skew
    }

    if( v > 26999 )
    {
        aAr.Skip( 10 ); // via count and stub length limits with their switches
    }
}


LOAD_TASK SOURCE_SPACINGS::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    const int v = ItemFormat( aAr );

    if( v < 0x3a98 )
    {
        Bidirectional = true;

        for( int32_t key : LEGACY_SPACING_KEYS )
            Values[key] = aAr.I32();

        if( v > 0x1f42 )
            Values[0x400040] = aAr.I32();

        co_return;
    }

    Bidirectional = aAr.Bool();

    // A counted map of key to clearance
    for( uint32_t i = aAr.Count(); i > 0; --i )
    {
        int32_t key = aAr.I32();
        Values[key] = aAr.I32();
    }
}


SOURCE_DESIGN_SPACINGS::SOURCE_DESIGN_SPACINGS()
{
    // A design's spacings start at 0.254 mm
    for( int32_t key : LEGACY_SPACING_KEYS )
        Values[key] = 2540;

    Values[0x400040] = 0;
}


LOAD_TASK SOURCE_DESIGN_SPACINGS::Load( ARCHIVE& aAr )
{
    co_await SOURCE_SPACINGS::Load( aAr );

    // No known file holds a class spacing, so the list must be a null tag
    if( ItemFormat( aAr ) > 15000 )
        co_await aAr.Object();
}


void RegisterStyleClasses( REGISTRY& aReg )
{
    Register<SOURCE_DESIGN_ITEM>( aReg, { "CDesignItem" } );
    Register<SOURCE_ATTRIBUTE_NAME>( aReg, { "CAttributeName" } );
    Register<SOURCE_ATTRIBUTE>( aReg, { "CAttribute" } );
    Register<SOURCE_LINE_STYLE>( aReg, { "CLineStyle" } );
    Register<SOURCE_TYPE_FONT>( aReg, { "CTypeFont" } );
    Register<SOURCE_TEXT_STYLE>( aReg, { "CTextStyle" } );
    Register<SOURCE_TRACK_STYLE>( aReg, { "CTrackStyle" } );
    Register<SOURCE_PAD_STYLE>( aReg, { "CPadStyle" } );
    Register<SOURCE_PAD_STYLE_EXCEPTION>( aReg, { "CPadStyleException" } );
    Register<SOURCE_LAYER_TYPE>( aReg, { "CLayerType" } );
    Register<SOURCE_LAYER>( aReg, { "CLayer" } );
    Register<SOURCE_LAYER_SPAN>( aReg, { "CLayerSpan" } );
    Register<SOURCE_LAYER_ARRAY>( aReg, { "CLayerArray" } );
    Register<SOURCE_NET_CLASS>( aReg, { "CNetClass" } );
    Register<SOURCE_SPACINGS>( aReg, { "CSpacings" } );
    Register<SOURCE_DESIGN_SPACINGS>( aReg, { "CDesignSpacings" } );
    RegisterSkipped<SOURCE_DESIGN_LIST>( aReg, "CAttributeArray", ATTRIBUTE_ARRAY );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CBarcodeFont", BARCODE_FONT );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CHatchStyle", HATCH_STYLE );
    RegisterSkipped<SOURCE_DESIGN_ITEM>( aReg, "CNoteCategory", NOTE_CATEGORY );
    Register<SOURCE_DESIGN_LIST>( aReg,
                                  { "CDesignList", "CAttributeNameArray", "CLineStyleArray", "CTextStyleArray",
                                    "CTrackStyleArray", "CHatchStyleList", "CPadStyleArray", "CPadStyleExceptionList",
                                    "CLayerTypeList", "CLayerSpanArray", "CNetClassArray", "CNoteCategoryList" } );
}

} // namespace EASYPC
