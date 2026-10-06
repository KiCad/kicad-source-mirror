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

#include <io/easypc/easypc_classes_library.h>


namespace EASYPC
{

namespace
{

    /// The item's own format, which also sets the boolean width for what follows
    void serializeFormat( ARCHIVE& aAr, SELF_VERSIONED& aItem )
    {
        SOURCE_DESIGN_ITEM::LoadFormatted( aAr, aItem.Format );
        aAr.SetBooleanIsByte( aItem.Format > 7999 );
    }

} // namespace


LOAD_TASK SOURCE_SYMBOL::Load( ARCHIVE& aAr )
{
    serializeFormat( aAr, *this );
    co_await SOURCE_DESIGN_ITEM::Load( aAr );
    LoadOldFormat( aAr, Format );

    const int v = Format;

    const bool nameFirst = v >= 22000 || aAr.Product() != PRODUCT_DESIGNSPARK;

    if( nameFirst )
        Name = aAr.ReadString();

    Shapes = co_await aAr.Object();
    Pads = co_await   aAr.Object();
    Texts = co_await  aAr.Object();

    if( !nameFirst )
        Name = aAr.ReadString();

    if( v > 10002 )
        ValuePositions = co_await aAr.Object();

    OriginX = aAr.I32();
    OriginY = aAr.I32();
    Timestamp = aAr.I32();

    if( v < 10003 )
    {
        NameOrigin = ARCHIVE::Create( "CSymbolNameOrigin" );

        if( v > 2999 )
            aAr.MapObject( NameOrigin.get() );

        co_await aAr.Embedded( *NameOrigin, "CSymbolNameOrigin" );

        // An old symbol's name origin becomes its name position; types 0x1f without a layer, 0xf with one
        const SOURCE_ORIGIN& origin = static_cast<const SOURCE_ORIGIN&>( *NameOrigin );
        DerivedNamePosition = std::make_unique<SOURCE_VALUE_POSITION>();
        DerivedNamePosition->Position = origin.Position;
        DerivedNamePosition->Layer = origin.Layer;
        DerivedNamePosition->TextStyle = origin.TextStyle;
        DerivedNamePosition->Types = origin.Layer ? 0xf : 0x1f;

        DerivedValuePositions = std::make_unique<SOURCE_DESIGN_LIST>();
        DerivedValuePositions->Items.push_back( DerivedNamePosition.get() );
        ValuePositions = DerivedValuePositions.get();
    }
    else if( v >= 12003 )
    {
        bool tail = true;

        co_await aAr.Object(); // origins

        if( v < 14001 )
        {
            co_await SkipMember( aAr, "CScreenGrid" );
            co_await SkipMember( aAr, "CUnits" );
            co_await SkipMember( aAr, "COldWorkingGrid" );
            tail = v > 12006;
        }
        else
        {
            co_await SkipMember( aAr, "CGrid" );
            co_await SkipMember( aAr, "CUnits" );
        }

        if( tail )
        {
            aAr.InBool();
            aAr.Skip( 16 ); // system and relative origins

            if( v > 12999 )
                co_await aAr.Object();

            if( v > 15001 )
            {
                Library = aAr.ReadString();
                aAr.Skip( 1 );
            }
        }
    }

    if( v > 28000 )
        co_await aAr.Object();
}


LOAD_TASK SOURCE_PIN_NAME::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );
    co_await LoadRemainder( aAr );

    if( ItemFormat( aAr ) > 2103 )
        Visible = aAr.InBool();
}


LOAD_TASK SOURCE_PIN_NAME_INSTANCE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Definition = co_await aAr.Object();

    if( ItemFormat( aAr ) > 2103 )
        Visible = aAr.InBool();
}


LOAD_TASK SOURCE_GATE::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Symbol = aAr.ReadString();

    for( int32_t i = aAr.I32(); i > 0; --i )
        PinNames.push_back( aAr.ReadString() );

    if( ItemFormat( aAr ) > 2003 )
        Library = aAr.ReadString();
}


const std::vector<OBJECT*>& ListItems( const OBJECT* aList )
{
    static const std::vector<OBJECT*> empty;

    if( const SOURCE_DESIGN_LIST* list = dynamic_cast<const SOURCE_DESIGN_LIST*>( aList ) )
        return list->Items;

    if( const SOURCE_DESIGN_ARRAY* array = dynamic_cast<const SOURCE_DESIGN_ARRAY*>( aList ) )
        return array->Items;

    return empty;
}


LOAD_TASK SOURCE_DESIGN_ARRAY::Load( ARCHIVE& aAr )
{
    Parent = co_await aAr.Object();

    for( uint32_t i = aAr.Count(); i > 0; --i )
        Items.push_back( co_await aAr.Object() );
}


LOAD_TASK SOURCE_GATE_MAP::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    if( ItemFormat( aAr ) > 8999 )
    {
        Pins = co_await aAr.Object();
        co_return;
    }

    for( int32_t i = aAr.I32(); i > 0; --i )
        PcbSymbolPins.push_back( aAr.I32() );
}


LOAD_TASK SOURCE_GATE_PIN::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    if( ItemFormat( aAr ) < 12005 )
        PcbSymbolPin = wxString::Format( wxS( "%d" ), aAr.I32() );
    else
        PcbSymbolPin = aAr.ReadString();

    PinNumber = aAr.ReadString();
    NetName = aAr.ReadString();

    if( ItemFormat( aAr ) > 22999 )
        PinType = aAr.U32();
}


LOAD_TASK SOURCE_SCM_COMPONENT::Load( ARCHIVE& aAr )
{
    serializeFormat( aAr, *this );
    co_await SOURCE_DESIGN_ITEM::Load( aAr );
    LoadOldFormat( aAr, Format );

    const int     f = Format;
    const int32_t product = aAr.Product();

    // The DesignSpark products store the package first
    if( product == PRODUCT_DESIGNSPARK || product == PRODUCT_DESIGNSPARK_CREATOR || product == PRODUCT_DESIGNSPARK_PRO )
    {
        Package = aAr.ReadString();
        Name = aAr.ReadString();
    }
    else
    {
        Name = aAr.ReadString();
        Package = aAr.ReadString();
    }

    Gates = co_await aAr.Object();
    DefaultReference = aAr.ReadString();
    Description = aAr.ReadString();

    if( f > 4999 )
        aAr.Skip( 4 ); // show flags

    if( f > 15001 )
    {
        Library = aAr.ReadString();
        aAr.Skip( 1 );
    }

    if( f > 16999 )
        SuppressionFlags = aAr.U32();

    if( f > 21999 )
        aAr.Skip( 1 );

    // Reset once loaded, so later objects chaining to this one gate on 30001
    Format = NEWEST_ITEM_FORMAT;
}


LOAD_TASK SOURCE_PCB_COMPONENT::Load( ARCHIVE& aAr )
{
    serializeFormat( aAr, *this );
    co_await SOURCE_DESIGN_ITEM::Load( aAr );
    LoadOldFormat( aAr, Format );

    Package = aAr.ReadString();
    Symbol = aAr.ReadString();
    GateMaps = co_await aAr.Object();
    aAr.SkipString(); // package description
    aAr.Skip( 4 );    // version

    if( Format > 2003 )
        Library = aAr.ReadString();

    // A null tag in every known file
    if( Format > 21999 )
        co_await aAr.Object();

    Format = NEWEST_ITEM_FORMAT;
}


LOAD_TASK SOURCE_COMPONENT::Load( ARCHIVE& aAr )
{
    co_await SOURCE_DESIGN_ITEM::Load( aAr );

    Board = co_await     aAr.ObjectAs<SOURCE_PCB_COMPONENT>();
    Schematic = co_await aAr.ObjectAs<SOURCE_SCM_COMPONENT>();
}


bool IsPadList( const wxString& aPcbSymbolPin )
{
    if( aPcbSymbolPin.Contains( wxS( "," ) ) || aPcbSymbolPin.Contains( wxS( "+" ) )
        || aPcbSymbolPin.Contains( wxS( "=" ) ) )
    {
        return true;
    }

    // "-1" is the unmapped marker, any other '-' is a range
    return aPcbSymbolPin.Find( '-' ) > 0;
}


std::vector<wxString> ExpandPinNumberList( const wxString& aList, wxString* aDisplay )
{
    // A range wider than any part is file damage and stays one literal item
    constexpr uint64_t MAX_PIN_RANGE = 100000;

    std::vector<wxString> out;
    wxString              list = aList;
    int                   eq = list.Find( '=' );

    if( eq != wxNOT_FOUND )
    {
        if( aDisplay )
            *aDisplay = list.Left( eq );

        list = list.Mid( eq + 1 );
    }

    wxString item;

    auto flush = [&]()
    {
        long first = 0;
        long last = 0;
        int  dash = item.Find( '-' );

        if( dash > 0 && item.Left( dash ).ToLong( &first ) && item.Mid( dash + 1 ).ToLong( &last ) && first <= last
            && uint64_t( last ) - uint64_t( first ) < MAX_PIN_RANGE )
        {
            // Stopping before the increment keeps a range ending at LONG_MAX from overflowing
            for( long n = first;; ++n )
            {
                out.push_back( wxString::Format( wxS( "%ld" ), n ) );

                if( n == last )
                    break;
            }
        }
        else if( !item.IsEmpty() )
        {
            out.push_back( item );
        }

        item.Clear();
    };

    for( wxUniChar ch : list )
    {
        if( ch == ',' || ch == '+' )
            flush();
        else
            item += ch;
    }

    flush();
    return out;
}


void RegisterLibraryClasses( REGISTRY& aReg )
{
    Register<SOURCE_SYMBOL>( aReg, { "CSymbol" } );
    Register<SOURCE_SCM_COMPONENT>( aReg, { "CScmComponent" } );
    Register<SOURCE_PCB_COMPONENT>( aReg, { "CPcbComponent" } );
    Register<SOURCE_COMPONENT>( aReg, { "CComponent" } );
    Register<SOURCE_GATE>( aReg, { "CGate" } );
    Register<SOURCE_GATE_MAP>( aReg, { "CGateMap" } );
    Register<SOURCE_GATE_PIN>( aReg, { "CGatePin" } );
    Register<SOURCE_DESIGN_ARRAY>( aReg, { "CDesignArray", "CGateArray", "CGateMapArray", "CGatePinArray" } );
    Register<SOURCE_DESIGN_LIST>( aReg, { "CSymbolArray", "CComponentArray", "CScmComponentArray" } );
    Register<SOURCE_PIN_NAME>( aReg, { "CPinName", "CPinNumber" } );
    Register<SOURCE_PIN_NAME_INSTANCE>( aReg, { "CPinNameInstance", "CPinNumberInstance" } );

    // These carry the plain text position and origin fields
    Register<SOURCE_TEXT_POSITION>( aReg, { "CSymbolName" } );
    Register<SOURCE_ORIGIN>( aReg, { "CSymbolNameOrigin" } );
}

} // namespace EASYPC
