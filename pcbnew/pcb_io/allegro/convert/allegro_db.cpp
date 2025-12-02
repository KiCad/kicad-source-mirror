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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "convert/allegro_db.h"

#include <wx/log.h>

#include <ki_exception.h>

#include <convert/allegro_pcb_structs.h>

using namespace ALLEGRO;


#define BLK_DATA( BLK, T ) static_cast<const BLOCK<T>&>( BLK ).GetData()


static std::optional<uint32_t> GetBlockKey( const BLOCK_BASE& block )
{
    // clang-format off
    switch( block.GetBlockType() )
    {
    case 0x01: return static_cast<const BLOCK<BLK_0x01_ARC>&>( block ).GetData().m_Key;
    case 0x03: return static_cast<const BLOCK<BLK_0x03>&>( block ).GetData().m_Key;
    case 0x04: return static_cast<const BLOCK<BLK_0x04_NET_ASSIGNMENT>&>( block ).GetData().m_Key;
    case 0x05: return static_cast<const BLOCK<BLK_0x05_TRACK>&>( block ).GetData().m_Key;
    case 0x06: return static_cast<const BLOCK<BLK_0x06>&>( block ).GetData().m_Key;
    case 0x07: return static_cast<const BLOCK<BLK_0x07>&>( block ).GetData().m_Key;
    case 0x08: return static_cast<const BLOCK<BLK_0x08>&>( block ).GetData().m_Key;
    case 0x09: return static_cast<const BLOCK<BLK_0x09>&>( block ).GetData().m_Key;
    case 0x0A: return static_cast<const BLOCK<BLK_0x0A_DRC>&>( block ).GetData().m_Key;
    case 0x0C: return static_cast<const BLOCK<BLK_0x0C>&>( block ).GetData().m_Key;
    case 0x0D: return static_cast<const BLOCK<BLK_0x0D_PAD>&>( block ).GetData().m_Key;
    case 0x0E: return static_cast<const BLOCK<BLK_0x0E>&>( block ).GetData().m_Key;
    case 0x0F: return static_cast<const BLOCK<BLK_0x0F>&>( block ).GetData().m_Key;
    case 0x10: return static_cast<const BLOCK<BLK_0x10>&>( block ).GetData().m_Key;
    case 0x11: return static_cast<const BLOCK<BLK_0x11>&>( block ).GetData().m_Key;
    case 0x12: return static_cast<const BLOCK<BLK_0x12>&>( block ).GetData().m_Key;
    case 0x14: return static_cast<const BLOCK<BLK_0x14>&>( block ).GetData().m_Key;
    case 0x15:
    case 0x16:
    case 0x17: return static_cast<const BLOCK<BLK_0x15_16_17_SEGMENT>&>( block ).GetData().m_Key;
    case 0x1B: return static_cast<const BLOCK<BLK_0x1B_NET>&>( block ).GetData().m_Key;
    case 0x1C: return static_cast<const BLOCK<BLK_0x1C_PADSTACK>&>( block ).GetData().m_Key;
    case 0x1D: return static_cast<const BLOCK<BLK_0x1D>&>( block ).GetData().m_Key;
    case 0x1E: return static_cast<const BLOCK<BLK_0x1E>&>( block ).GetData().m_Key;
    case 0x1F: return static_cast<const BLOCK<BLK_0x1F>&>( block ).GetData().m_Key;
    case 0x21: return static_cast<const BLOCK<BLK_0x21>&>( block ).GetData().m_Key;
    case 0x22: return static_cast<const BLOCK<BLK_0x22>&>( block ).GetData().m_Key;
    case 0x23: return static_cast<const BLOCK<BLK_0x23_RATLINE>&>( block ).GetData().m_Key;
    case 0x24: return static_cast<const BLOCK<BLK_0x24_RECT>&>( block ).GetData().m_Key;
    case 0x26: return static_cast<const BLOCK<BLK_0x26>&>( block ).GetData().m_Key;
    case 0x28: return static_cast<const BLOCK<BLK_0x28_SHAPE>&>( block ).GetData().m_Key;
    case 0x29: return static_cast<const BLOCK<BLK_0x29_PIN>&>( block ).GetData().m_Key;
    case 0x2A: return static_cast<const BLOCK<BLK_0x2A_LAYER_LIST>&>( block ).GetData().m_Key;
    case 0x2B: return static_cast<const BLOCK<BLK_0x2B>&>( block ).GetData().m_Key;
    case 0x2C: return static_cast<const BLOCK<BLK_0x2C_TABLE>&>( block ).GetData().m_Key;
    case 0x2D: return static_cast<const BLOCK<BLK_0x2D>&>( block ).GetData().m_Key;
    case 0x2E: return static_cast<const BLOCK<BLK_0x2E>&>( block ).GetData().m_Key;
    case 0x2F: return static_cast<const BLOCK<BLK_0x2F>&>( block ).GetData().m_Key;
    case 0x30: return static_cast<const BLOCK<BLK_0x30_STR_WRAPPER>&>( block ).GetData().m_Key;
    case 0x31: return static_cast<const BLOCK<BLK_0x31_SGRAPHIC>&>( block ).GetData().m_Key;
    case 0x32: return static_cast<const BLOCK<BLK_0x32_PLACED_PAD>&>( block ).GetData().m_Key;
    case 0x33: return static_cast<const BLOCK<BLK_0x33_VIA>&>( block ).GetData().m_Key;
    case 0x34: return static_cast<const BLOCK<BLK_0x34_KEEPOUT>&>( block ).GetData().m_Key;
    case 0x36: return static_cast<const BLOCK<BLK_0x36>&>( block ).GetData().m_Key;
    case 0x37: return static_cast<const BLOCK<BLK_0x37>&>( block ).GetData().m_Key;
    case 0x38: return static_cast<const BLOCK<BLK_0x38_FILM>&>( block ).GetData().m_Key;
    case 0x39: return static_cast<const BLOCK<BLK_0x39_FILM_LAYER_LIST>&>( block ).GetData().m_Key;
    case 0x3A: return static_cast<const BLOCK<TYPE_3A_FILM_LIST_NODE>&>( block ).GetData().m_Key;
    case 0x3C: return static_cast<const BLOCK<BLK_0x3C>&>( block ).GetData().m_Key;
    default: break;
    }
    // clang-format off

    return std::nullopt;
}


bool DB_REF::Resolve( const DB_OBJ_RESOLVER& aResolver )
{
    if( m_TargetKey == m_EndKey )
    {
        // Null reference
        m_Target = nullptr;
        return true;
    }

    // No conditions
    m_Target = aResolver.Resolve( m_TargetKey );


    if(!m_Target)
    {
        wxLogTrace( "ALLEGRO_EXTRACT", "Failed to resolve DB_REF target key %#010x for %s", m_TargetKey, m_DebugName ? m_DebugName : "<unknown>" );
    }

    return m_Target != nullptr;
}


bool DB_STR_REF::Resolve( const DB_OBJ_RESOLVER& aResolver )
{
    if( m_StringKey == 0 )
    {
        // Null string reference
        m_String = nullptr;
        return true;
    }

    m_String = aResolver.ResolveString( m_StringKey );

    if( !m_String )
    {
        wxLogTrace( "ALLEGRO_EXTRACT", "Failed to resolve DB_STR_REF string key %#010x for %s", m_StringKey, m_DebugName ? m_DebugName : "<unknown>" );
    }

    return m_String != nullptr;
}


bool DB_REF_CHAIN::Resolve( const DB_OBJ_RESOLVER& aResolver )
{
    if( m_Head == m_Tail )
    {
        return true;
    }

    if( m_Head == 0 )
    {
        // Empty chain
        return true;
    }

    DB_OBJ* n = aResolver.Resolve( m_Head );

    while( n != nullptr )
    {
        m_Chain.push_back( n );

        uint32_t nextKey = m_NextKeyGetter( *n );

        if( nextKey == m_Tail )
        {
            return true;
        }

        n = aResolver.Resolve( nextKey );
    }

    // Ended before the tail
    wxLogTrace( "ALLEGRO_EXTRACT", "Failed to resolve DB_REF_CHAIN up to tail %#010x for %s", m_Tail, m_DebugName ? m_DebugName : "<unknown>" );
    return false;
}


void DB_REF_CHAIN::Visit( std::function<void ( const DB_OBJ& aObj )> aVisitor ) const
{
    for( const DB_OBJ* node : m_Chain )
    {
        aVisitor( *node );
    }
}


void DB_REF_CHAIN::Visit( std::function<void ( DB_OBJ& aObj )> aVisitor )
{
    for( DB_OBJ* node : m_Chain )
    {
        aVisitor( *node );
    }
}


void BRD_DB::InsertBlock( const BLOCK_BASE& aBlock )
{
    std::unique_ptr<DB_OBJ> dbObj = createObject( aBlock );

    if( dbObj )
    {
        AddObject( std::move( dbObj ) );
    }
    else
    {
        // THROW?
    }
}

std::unique_ptr<DB_OBJ> BRD_DB::createObject( const BLOCK_BASE& aBlock )
{
    std::unique_ptr<DB_OBJ> obj;

    switch( aBlock.GetBlockType() )
    {
    case 0x01:
    {
        const BLK_0x01_ARC& arcBlk = BLK_DATA( aBlock, BLK_0x01_ARC );
        obj = std::make_unique<ARC>( arcBlk );
        break;
    }
    case 0x03:
    {
        const BLK_0x03& blk03 = BLK_DATA( aBlock, BLK_0x03 );

        switch( blk03.m_SubType )
        {
        case 0x68: // SYM_LIBRARY_PATH, ?...
            obj = std::make_unique<x03_TEXT>( blk03 );
            break;
        default:
            // Unknown subtype
            wxLogTrace( "ALLEGRO_EXTRACT", "Unknown BLK_0x03 subtype: 0x%02x", blk03.m_SubType );
            break;
        }

        break;
    }
    case 0x04:
    {
        const BLK_0x04_NET_ASSIGNMENT& netBlk = BLK_DATA( aBlock, BLK_0x04_NET_ASSIGNMENT );
        obj = std::make_unique<NET_ASSIGN>( *this, netBlk );
        break;
    }
    case 0x05:
    {
        const BLK_0x05_TRACK& trackBlk = BLK_DATA( aBlock, BLK_0x05_TRACK );
        obj = std::make_unique<TRACK>( *this, trackBlk );
        break;
    }
    case 0x06:
    {
        const BLK_0x06& compBlk = BLK_DATA( aBlock, BLK_0x06 );
        obj = std::make_unique<COMPONENT>( *this, compBlk );
        break;
    }
    case 0x07:
    {
        const BLK_0x07& strBlk = BLK_DATA( aBlock, BLK_0x07 );
        obj = std::make_unique<COMPONENT_INST>( strBlk );
        break;
    }
    case 0x08:
    {
        const BLK_0x08& symbolBlk = BLK_DATA( aBlock, BLK_0x08 );
        obj = std::make_unique<PIN_NUMBER>( symbolBlk );
        break;
    }
    case 0x0E:
    {
        const BLK_0x0E& pinBlk = BLK_DATA( aBlock, BLK_0x0E );
        obj = std::make_unique<X0E>( *this, pinBlk );
        break;
    }
    case 0x0F:
    {
        const BLK_0x0F& funcSlotBlk = BLK_DATA( aBlock, BLK_0x0F );
        obj = std::make_unique<FUNCTION_SLOT>( funcSlotBlk );
        break;
    }
    case 0x10:
    {
        const BLK_0x10& funcInstBlk = BLK_DATA( aBlock, BLK_0x10 );
        obj = std::make_unique<FUNCTION_INSTANCE>( funcInstBlk );
        break;
    }
    case 0x11:
    {
        const BLK_0x11& pinNameBlk = BLK_DATA( aBlock, BLK_0x11 );
        obj = std::make_unique<PIN_NAME>( pinNameBlk );
        break;
    }
    case 0x14:
    {
        const BLK_0x14& lineBlk = BLK_DATA( aBlock, BLK_0x14 );
        obj = std::make_unique<GRAPHIC_SEG>( *this, lineBlk );
        break;
    }
    case 0x15:
    case 0x16:
    case 0x17:
    {
        const BLK_0x15_16_17_SEGMENT& seg = BLK_DATA( aBlock, BLK_0x15_16_17_SEGMENT );
        obj = std::make_unique<LINE>( seg );
        // std::cout << "Seg" << std::endl;
        break;
    }
    case 0x1b:
    {
        const BLK_0x1B_NET& netBlk = BLK_DATA( aBlock, BLK_0x1B_NET );
        obj = std::make_unique<NET>( *this, netBlk );
        break;
    }
    case 0x28:
    {
        const BLK_0x28_SHAPE& shapeBlk = BLK_DATA( aBlock, BLK_0x28_SHAPE );
        obj = std::make_unique<SHAPE>( *this, shapeBlk );
        break;
    }
    case 0x2b: // Footprint
    {
        const BLK_0x2B& fpBlk = BLK_DATA( aBlock, BLK_0x2B );
        obj = std::make_unique<FOOTPRINT_DEF>( *this, fpBlk );
        break;
    }
    case 0x2d:
    {
        const BLK_0x2D& fpInstBlk = BLK_DATA( aBlock, BLK_0x2D );
        obj = std::make_unique<FOOTPRINT_INSTANCE>( fpInstBlk );
        break;
    }
    case 0x2e:
    {
        const BLK_0x2E& padBlk = BLK_DATA( aBlock, BLK_0x2E );
        obj = std::make_unique<X2E>( *this, padBlk );
        break;
    }
    case 0x32:
    {
        const BLK_0x32_PLACED_PAD& placedPadBlk = BLK_DATA( aBlock, BLK_0x32_PLACED_PAD );
        obj =  std::make_unique<PLACED_PAD>( *this, placedPadBlk );
        break;
    }
    case 0x33:
    {
        const BLK_0x33_VIA& viaBlk = BLK_DATA( aBlock, BLK_0x33_VIA );
        obj =  std::make_unique<VIA>( *this, viaBlk );
        break;
    }
    default:
        break;
    }

    if( obj )
    {
        obj->m_Loc = DB_OBJ::FILE_LOC{ aBlock.GetOffset(), aBlock.GetBlockType() };
    }

    return obj;
}


DB::DB()
{
}


DB::~DB()
{
}


void DB::AddObject( std::unique_ptr<DB_OBJ> aObject )
{
    m_Objects[aObject->GetKey()] = std::move( aObject );
}

#include <fmt/format.h>

DB_OBJ* DB::Resolve( uint32_t aKey ) const
{
    auto it = m_Objects.find( aKey );
    if( it != m_Objects.end() )
    {
        return it->second.get();
    }

    {
        // std::cout << fmt::format( "Failed to resolve key: {:010x}", aRef.m_TargetKey ) << std::endl;
    }
    return nullptr;
}


const wxString* DB::ResolveString( uint32_t aKey ) const
{
    auto it = m_StringTable.find( aKey );
    if( it != m_StringTable.end() )
    {
        return &it->second;
    }

    return nullptr;
}


void DB::ResolveObjectLinks()
{
    wxLogTrace( "ALLEGRO_EXTRACT", "Resolving %zu object references", m_Objects.size() );

    for( auto& [key, obj] : m_Objects )
    {
        obj->m_Valid = obj->ResolveRefs( *this );

        if( !obj->m_Valid )
        {
            // If we can't resolve the references, the DB is invalid and we cannot easily continue
            // as any references could explode later.
            THROW_IO_ERROR( wxString::Format( "Failed to resolve references for object key %#010x", key ) );
        }
    }
}


void DB::visitLinkedList( const FILE_HEADER::LINKED_LIST            aLList,
                          std::function<const DB_REF&( const DB_OBJ& aObj )> aVisitor ) const
{
    const DB_OBJ* node = Resolve( aLList.m_Head );
    uint32_t lastKey = aLList.m_Head;

    size_t iterations = 0;

    while( node )
    {
        const DB_REF& nextRef = aVisitor( *node );

        if( !nextRef.m_Target && nextRef.m_TargetKey != aLList.m_Tail )
        {
            THROW_IO_ERROR( wxString::Format( "Unexpected end of linked list: could not find %#010x", nextRef.m_TargetKey ) );
        }

        if( iterations++ >= 1e6 )
        {
            THROW_IO_ERROR( wxString::Format( "Excessive list length: key was %#010x", nextRef.m_TargetKey ) );
        }

        lastKey = nextRef.m_TargetKey;
        node = nextRef.m_Target;
    }
}


static bool CheckTypeIs( const DB_REF& aRef, DB_OBJ::TYPE aType, bool aCanBeNull )
{
    if( aRef.m_Target == nullptr )
        return aCanBeNull;

    const DB_OBJ::TYPE objType = aRef.m_Target->GetType();
    return objType == aType;
}

static bool CheckTypeIsOneOf( const DB_REF& aRef, const std::vector<DB_OBJ::TYPE>& aTypes, bool aCanBeNull )
{
    if( aRef.m_Target == nullptr )
        return aCanBeNull;

    const DB_OBJ::TYPE objType = aRef.m_Target->GetType();
    return std::find( aTypes.begin(), aTypes.end(), objType ) != aTypes.end();
}


// static DB_REF& FollowFootprintInstanceNext( DB_OBJ& aObj )
// {
//     switch( aObj.GetType() )
//     {
//     case DB_OBJ::TYPE::FP_INST:
//         return static_cast<const FOOTPRINT_INSTANCE&>(aObj).m_Next.m_TargetKey;
//     }
// }


ARC::ARC( const BLK_0x01_ARC& aBlk ) :
    DB_OBJ( DB_OBJ::TYPE::ARC, aBlk.m_Key )
{
    m_Parent.m_TargetKey = aBlk.m_Parent;
    m_Parent.m_DebugName = "ARC::m_Parent";
}


bool ARC::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Parent.Resolve( aResolver );

    return ok;
}


NET_ASSIGN::NET_ASSIGN( const BRD_DB& aBrd, const BLK_0x04_NET_ASSIGNMENT& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::NET_ASSIGN, aBlk.m_Key )
{
    m_Next.m_TargetKey = aBlk.m_Next;
    // m_Next.m_EndKey = aBrd.m_Header->m_LL_0x04.m_Tail;
    m_Next.m_DebugName = "NET_ASSIGN::m_Next";

    m_Net.m_TargetKey = aBlk.m_Net;
    m_Net.m_DebugName = "NET_ASSIGN::m_Net";

    m_ConnItem.m_TargetKey = aBlk.m_ConnItem;
    m_ConnItem.m_DebugName = "NET_ASSIGN::m_ConnItem";
}


bool NET_ASSIGN::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    // ok &= m_Net.Resolve( aResolver );
    ok &= m_ConnItem.Resolve( aResolver );

    return ok;
}


TRACK::TRACK( const BRD_DB& aBrd, const BLK_0x05_TRACK& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::TRACK, aBlk.m_Key )
{
    m_Next.m_TargetKey = aBlk.m_Next;
    // m_Next.m_EndKey = aBrd.m_Header->m_LL_0x05.m_Tail;
    m_Next.m_DebugName = "TRACK::m_Next";
}


bool TRACK::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );

    return ok;
}


x03_TEXT::x03_TEXT( const BLK_0x03& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::x03_TEXT, aBlk.m_Key )
{
    // wxASSERT( aBlk.m_Substruct.is<std::string>() );
    m_TextStr = std::get<std::string>( aBlk.m_Substruct );
}


COMPONENT::COMPONENT( const BRD_DB& aBrd, const BLK_0x06& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::COMPONENT, aBlk.m_Key )
{
    m_Next.m_TargetKey = aBlk.m_Next;
    m_Next.m_EndKey = aBrd.m_Header->m_LL_0x06.m_Tail;
    m_Next.m_DebugName = "COMPONENT::m_Next";

    m_CompDeviceType.m_StringKey = aBlk.m_CompDeviceType;
    m_CompDeviceType.m_DebugName = "COMPONENT::m_CompDeviceType";

    m_SymbolName.m_StringKey = aBlk.m_SymbolName;
    m_SymbolName.m_DebugName = "COMPONENT::m_SymbolName";

    m_Instances.m_Head = aBlk.m_FirstInstPtr;
    m_Instances.m_Tail = aBlk.m_Key;
    m_Instances.m_NextKeyGetter = []( const DB_OBJ& aObj )
    {
        const COMPONENT_INST& compInst = static_cast<const COMPONENT_INST&>( aObj );
        return compInst.m_Next.m_TargetKey;
    };
    m_Instances.m_DebugName = "COMPONENT::m_Instances";

    m_PtrFunctionSlot.m_TargetKey = aBlk.m_PtrFunctionSlot;
    m_PtrFunctionSlot.m_DebugName = "COMPONENT::m_PtrFunctionSlot";

    m_PtrPinNumber.m_TargetKey = aBlk.m_PtrPinNumber;
    m_PtrPinNumber.m_DebugName = "COMPONENT::m_PtrPinNumber";

    m_Fields.m_TargetKey = aBlk.m_Fields;
    m_Fields.m_DebugName = "COMPONENT::m_Fields";
}


bool COMPONENT::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_CompDeviceType.Resolve( aResolver );
    ok &= m_SymbolName.Resolve( aResolver );
    ok &= m_Instances.Resolve( aResolver );
    ok &= m_PtrFunctionSlot.Resolve( aResolver );
    ok &= m_PtrPinNumber.Resolve( aResolver );
    ok &= m_Fields.Resolve( aResolver );

    ok &= CheckTypeIs( m_PtrFunctionSlot, DB_OBJ::TYPE::FUNCTION_SLOT, true );

    return ok;
}


const wxString* COMPONENT::GetComponentDeviceType() const
{
    return m_CompDeviceType.m_String;
}


COMPONENT_INST::COMPONENT_INST( const BLK_0x07& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::COMPONENT_INST, aBlk.m_Key )
{
    m_TextStr.m_StringKey = aBlk.m_RefDesStrPtr;
    m_TextStr.m_DebugName = "COMPONENT_INST::m_TextStr";
    m_Next.m_TargetKey = aBlk.m_Next;
    m_Next.m_DebugName = "COMPONENT_INST::m_Next";

    m_FunctionInst.m_TargetKey = aBlk.m_FunctionInstPtr;
    m_FunctionInst.m_DebugName = "COMPONENT_INST::m_FunctionInst";

    m_Pads.m_Head = aBlk.m_FirstPadPtr;
    m_Pads.m_Tail = aBlk.m_Key;
    m_Pads.m_NextKeyGetter = []( const DB_OBJ& aObj ) -> uint32_t
    {
        const PLACED_PAD& pad = static_cast<const PLACED_PAD&>( aObj );
        return pad.m_NextInCompInst.m_TargetKey;
    };
    m_Pads.m_DebugName = "COMPONENT_INST::m_Pads";
}


bool COMPONENT_INST::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_TextStr.Resolve( aResolver );
    ok &= m_Next.Resolve( aResolver );
    ok &= m_FunctionInst.Resolve( aResolver );
    ok &= m_Pads.Resolve( aResolver );

    return ok;
}


const wxString* COMPONENT_INST::GetRefDesStr() const
{
    return m_TextStr.m_String;
}


const FUNCTION_INSTANCE& COMPONENT_INST::GetFunctionInstance() const
{
    if( m_FunctionInst.m_Target == nullptr )
    {
        THROW_IO_ERROR( "COMPONENT_INST::GetFunctionInstance: Null reference to FUNCTION_INSTANCE" );
    }

    return static_cast<const FUNCTION_INSTANCE&>( *m_FunctionInst.m_Target );
}


const COMPONENT_INST* COMPONENT_INST::GetNextInstance() const
{
    wxCHECK2_MSG( m_Next.m_Target, nullptr, "COMPONENT_INST::GetNextInstance: Null m_Next reference" );

    // If the next is not a COMPONENT_INST, it's the end of the list
    if( m_Next.m_Target->GetType() != DB_OBJ::TYPE::COMPONENT_INST )
    {
        return nullptr;
    }

    return static_cast<const COMPONENT_INST*>( m_Next.m_Target );
}


PIN_NUMBER::PIN_NUMBER( const BLK_0x08& aBlk ) :
    DB_OBJ( DB_OBJ::TYPE::PIN_NUMBER, aBlk.m_Key )
{
    m_PinName.m_TargetKey = aBlk.m_PinNamePtr;
    m_PinName.m_DebugName = "PIN_NUMBER::m_PinName";

    m_Next.m_TargetKey = aBlk.m_Next;
    m_Next.m_DebugName = "PIN_NUMBER::m_Next";

    // This can be in one of two fields depending on version
    if( aBlk.m_StrPtr.has_value() )
        m_PinNumberStr.m_StringKey = aBlk.m_StrPtr.value();
    else
        m_PinNumberStr.m_StringKey = aBlk.m_StrPtr16x.value();

    m_PinNumberStr.m_DebugName = "PIN_NUMBER::m_PinNumberStr";
}


bool PIN_NUMBER::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_PinNumberStr.Resolve( aResolver );
    ok &= m_PinName.Resolve( aResolver );

    ok &= CheckTypeIs( m_PinName, DB_OBJ::TYPE::PIN_NAME, true );

    return ok;
}


X0E::X0E( const BRD_DB& aBrd, const BLK_0x0E& aBlk ) :
    DB_OBJ( DB_OBJ::TYPE::x0e, aBlk.m_Key )
{
    m_Next.m_TargetKey = aBlk.m_Next;
    m_Next.m_EndKey = aBrd.m_Header->m_LL_Shapes.m_Tail;
    m_Next.m_DebugName = "X0E::m_Next";
}


bool X0E::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );

    return ok;
}


PIN_NAME::PIN_NAME( const BLK_0x11& aBlk ) :
    DB_OBJ( DB_OBJ::TYPE::PIN_NAME, aBlk.m_Key )
{
    m_PinNameStr.m_StringKey = aBlk.m_PinNameStrPtr;
    m_PinNameStr.m_DebugName = "PIN_NAME::m_PinNameStr";

    m_Next.m_TargetKey = aBlk.m_Next;
    m_Next.m_DebugName = "PIN_NAME::m_Next";

    m_PinNumber.m_TargetKey = aBlk.m_PinNumberPtr;
    m_PinNumber.m_DebugName = "PIN_NAME::m_PinNumber";
}

bool PIN_NAME::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_PinNameStr.Resolve( aResolver );
    ok &= m_Next.Resolve( aResolver );
    ok &= m_PinNumber.Resolve( aResolver );

    return ok;
}


GRAPHIC_SEG::GRAPHIC_SEG( const BRD_DB& aBrd, const BLK_0x14& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::GRAPHIC_SEG, aBlk.m_Key )
{
    m_Parent.m_TargetKey = aBlk.m_Parent;
    m_Parent.m_EndKey = aBrd.m_Header->m_LL_0x14.m_Tail;
    m_Parent.m_DebugName = "GRAPHIC_SEG::m_Parent";

    m_Next.m_TargetKey = aBlk.m_Next;
    m_Next.m_EndKey = aBrd.m_Header->m_LL_0x14.m_Tail;
    m_Next.m_DebugName = "GRAPHIC_SEG::m_Next";

    m_Segment.m_TargetKey = aBlk.m_SegmentPtr;
    m_Segment.m_DebugName = "GRAPHIC_SEG::m_Segment";
}


bool GRAPHIC_SEG::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Parent.Resolve( aResolver );
    ok &= m_Next.Resolve( aResolver );
    ok &= m_Segment.Resolve( aResolver );

    ok &= CheckTypeIsOneOf( m_Segment,
        {
            DB_OBJ::TYPE::LINE,
            DB_OBJ::TYPE::ARC,
        },
        false );

    return ok;
}


LINE::LINE( const BLK_0x15_16_17_SEGMENT& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::LINE, aBlk.m_Key )
{
    m_Parent.m_TargetKey = aBlk.m_Parent;
    m_Parent.m_DebugName = "SEGMENT::m_Parent";

    m_Next.m_TargetKey = aBlk.m_Next;
    m_Next.m_DebugName = "SEGMENT::m_Next";

    m_Start.x = aBlk.m_StartX;
    m_Start.y = aBlk.m_StartY;
    m_End.x = aBlk.m_EndX;
    m_End.y = aBlk.m_EndY;

    m_Width = aBlk.m_Width;
}


bool LINE::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    // ok &= m_Parent.Resolve( aResolver );
    ok &= m_Next.Resolve( aResolver );

    return ok;
}


FOOTPRINT_DEF::FOOTPRINT_DEF( const BRD_DB& aBrd, const BLK_0x2B& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::FP_DEF, aBlk.m_Key )
{
    // 0x2Bs are linked together in a list from the board header
    m_Next.m_EndKey = aBrd.m_Header->m_LL_0x2B.m_Tail;
    m_Next.m_TargetKey = aBlk.m_Next;

    m_FpStr.m_StringKey = aBlk.m_FpStrRef;

    m_Instances.m_NextKeyGetter = []( const DB_OBJ& aObj )
    {
        if (aObj.GetType() != DB_OBJ::TYPE::FP_INST )
            return 0U;

        return static_cast<const FOOTPRINT_INSTANCE&>( aObj ).m_Next.m_TargetKey;
    };
    m_Instances.m_Head = aBlk.m_FirstInstPtr;
    m_Instances.m_Tail = aBlk.m_Key;

    m_SymLibPath.m_TargetKey = aBlk.m_SymLibPathPtr;
    m_SymLibPath.m_DebugName = "FOOTPRINT_DEF::m_SymLibPath";
}


bool FOOTPRINT_DEF::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_FpStr.Resolve( aResolver );

    // Follow the chain of 0x2Ds until we get back here
    ok &= m_Instances.Resolve( aResolver );

    // Set backlink from instances to parent
    m_Instances.Visit( [&]( DB_OBJ& aObj )
    {
        FOOTPRINT_INSTANCE& fpInst = static_cast<FOOTPRINT_INSTANCE&>( aObj );
        fpInst.m_Parent = this;
    } );

    wxLogTrace( "ALLEGRO_EXTRACT", "Resolved %zu footprint instances for footprint def key %#010x", m_Instances.m_Chain.size(), GetKey() );

    ok &= m_SymLibPath.Resolve( aResolver );

    ok &= CheckTypeIs( m_Next, DB_OBJ::TYPE::FP_DEF, true );

    return ok;
}


const wxString* FOOTPRINT_DEF::GetLibPath() const
{
    if( m_SymLibPath.m_Target == nullptr )
        return nullptr;

    const x03_TEXT& symLibPath = static_cast<const x03_TEXT&>( *m_SymLibPath.m_Target );
    return &symLibPath.m_TextStr;
}


FOOTPRINT_INSTANCE::FOOTPRINT_INSTANCE( const BLK_0x2D& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::FP_INST, aBlk.m_Key )
{
    m_Next.m_TargetKey = aBlk.m_Next;

    // This can be in one of two fields
    m_ComponentInstance.m_TargetKey = aBlk.m_InstRef16x.value_or( aBlk.m_InstRef.value_or( 0 ) );

    m_Pads.m_Head = aBlk.m_FirstPadPtr;
    m_Pads.m_Tail = aBlk.m_Key;
    m_Pads.m_NextKeyGetter = []( const DB_OBJ& aObj )
    {

        if (aObj.GetType() != DB_OBJ::TYPE::PLACED_PAD )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "FOOTPRINT_INSTANCE::m_Pads: Unexpected type in pad chain: %d", static_cast<int>( aObj.GetType() ) );
            return 0U;
        }

        return static_cast<const PLACED_PAD&>( aObj ).m_NextInFp.m_TargetKey;
    };

    // This will be filled in by the 0x2B resolution
    m_Parent = nullptr;

    m_X = aBlk.m_CoordX;
    m_Y = aBlk.m_CoordY;
    m_Rotation = aBlk.m_Rotation;
    m_Mirrored = false; //aBlk.m_Flags & ??? //;
}


bool FOOTPRINT_INSTANCE::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_ComponentInstance.Resolve( aResolver );
    ok &= m_Pads.Resolve( aResolver );

    ok &= CheckTypeIs( m_ComponentInstance, DB_OBJ::TYPE::COMPONENT_INST, true );

    return ok;
}


const COMPONENT_INST* FOOTPRINT_INSTANCE::GetComponentInstance() const
{
    // The component instance is found in the 0x07 string ref
    // But it can be null (e.g. for dimensions)
    if( m_ComponentInstance.m_Target == nullptr )
        return nullptr;

    const COMPONENT_INST* componentInstance = static_cast<const COMPONENT_INST*>( m_ComponentInstance.m_Target );
    return componentInstance;
}



const wxString* FOOTPRINT_INSTANCE::GetName() const
{
    if( m_Parent == nullptr )
        return nullptr;

    return m_Parent->m_FpStr.m_String;
}


FUNCTION_SLOT::FUNCTION_SLOT( const BLK_0x0F& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::FUNCTION_SLOT, aBlk.m_Key )
{
    m_SlotName.m_StringKey = aBlk.m_SlotName;
    m_SlotName.m_DebugName = "FUNCTION_SLOT::m_SlotName";

    m_CompDeviceType = wxString::FromUTF8( aBlk.m_CompDeviceType.data() );

    m_Ptr0x06.m_TargetKey = aBlk.m_Ptr0x06;
    m_Ptr0x06.m_DebugName = "FUNCTION_SLOT::m_Ptr0x06";

    m_Ptr0x11.m_TargetKey = aBlk.m_Ptr0x11;
    m_Ptr0x11.m_DebugName = "FUNCTION_SLOT::m_Ptr0x11";
}


bool FUNCTION_SLOT::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_SlotName.Resolve( aResolver );
    ok &= m_Ptr0x06.Resolve( aResolver );
    ok &= m_Ptr0x11.Resolve( aResolver );

    return ok;
}


const wxString* FUNCTION_SLOT::GetName() const
{
    return m_SlotName.m_String;
}


FUNCTION_INSTANCE::FUNCTION_INSTANCE( const BLK_0x10& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::FUNCTION_INST, aBlk.m_Key )
{
    m_Slot.m_TargetKey = aBlk.m_Slots;
    m_Slot.m_DebugName = "FUNCTION_INSTANCE::m_Slots";

    m_Fields.m_TargetKey = aBlk.m_Fields;
    m_Fields.m_DebugName = "FUNCTION_INSTANCE::m_Fields";

    m_FunctionName.m_StringKey = aBlk.m_FunctionName;
    m_FunctionName.m_DebugName = "FUNCTION_INSTANCE::m_FunctionName";

    m_ComponentInstance.m_TargetKey = aBlk.m_ComponentInstPtr;
    m_ComponentInstance.m_DebugName = "FUNCTION_INSTANCE::m_ComponentInstance";
}


bool FUNCTION_INSTANCE::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Slot.Resolve( aResolver );
    ok &= m_Fields.Resolve( aResolver );
    ok &= m_FunctionName.Resolve( aResolver );
    ok &= m_ComponentInstance.Resolve( aResolver );

    return ok;
}


const wxString* FUNCTION_INSTANCE::GetName() const
{
    return m_FunctionName.m_String;
}


const COMPONENT_INST& FUNCTION_INSTANCE::GetComponentInstance() const
{
    return static_cast<const COMPONENT_INST&>( *m_ComponentInstance.m_Target );
}


const FUNCTION_SLOT& FUNCTION_INSTANCE::GetFunctionSlot() const
{
    if( m_Slot.m_Target == nullptr )
    {
        THROW_IO_ERROR( "FUNCTION_INSTANCE::GetFunctionSlot: Null reference to FUNCTION_SLOT" );
    }

    return static_cast<const FUNCTION_SLOT&>( *m_Slot.m_Target );
}


NET::NET( const BRD_DB& aBrd, const BLK_0x1B_NET& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::NET, aBlk.m_Key )
{
    m_Next.m_TargetKey = aBlk.m_Next;
    m_Next.m_EndKey = aBrd.m_Header->m_LL_0x1B_Nets.m_Tail;
    m_Next.m_DebugName = "NET::m_Next";

    m_NetNameStr.m_StringKey = aBlk.m_NetName;
    m_NetNameStr.m_DebugName = "NET::m_NetNameStr";

    m_NetAssignments.m_Head = aBlk.m_Assignment;
    m_NetAssignments.m_Tail = aBlk.m_Key;
    m_NetAssignments.m_NextKeyGetter = []( const DB_OBJ& aObj )
    {
        const NET_ASSIGN& netAssign = static_cast<const NET_ASSIGN&>( aObj );
        return netAssign.m_Next.m_TargetKey;
    };
    m_NetAssignments.m_DebugName = "NET::m_NetAssignments";
}


bool NET::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_NetNameStr.Resolve( aResolver );
    ok &= m_NetAssignments.Resolve( aResolver );

    return ok;
}


const wxString* NET::GetName() const
{
    return m_NetNameStr.m_String;
}


SHAPE::SHAPE( const BRD_DB& aBrd, const BLK_0x28_SHAPE& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::SHAPE, aBlk.m_Key )
{
    m_Next.m_TargetKey = aBlk.m_Next;
    // m_Next.m_EndKey = aBrd.m_Header->m_LL_0x28.m_Tail;
    m_Next.m_DebugName = "SHAPE::m_Next";

    m_Segments.m_TargetKey = aBlk.m_FirstSegmentPtr;
    m_Segments.m_DebugName = "SHAPE::m_Segments";
}


bool SHAPE::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_Segments.Resolve( aResolver );

    return ok;
}


X2E::X2E( const BRD_DB& aBrd, const BLK_0x2E& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::x2e, aBlk.m_Key )
{
    m_Next.m_TargetKey = aBlk.m_Next;
    // m_Next.m_EndKey = aBrd.m_Header->m_LL_0x2E.m_Tail;
    m_Next.m_DebugName = "X2E::m_Next";

    m_NetAssign.m_TargetKey = aBlk.m_NetAssignment;
    m_NetAssign.m_DebugName = "X2E::m_NetAssign";

    m_Connection.m_TargetKey = aBlk.m_Connection;
    m_Connection.m_DebugName = "X2E::m_Connection";

    m_Position.x = aBlk.m_CoordX;
    m_Position.y = aBlk.m_CoordY;
}


bool X2E::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_NetAssign.Resolve( aResolver );
    ok &= m_Connection.Resolve( aResolver );

    return ok;
}


PLACED_PAD::PLACED_PAD( const BRD_DB& aBrd, const BLK_0x32_PLACED_PAD& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::PLACED_PAD, aBlk.m_Key )
{
    m_Next.m_TargetKey = aBlk.m_Next;
    // m_Next.m_EndKey = aBrd.m_Header->m_LL_0x32.m_Tail;
    m_Next.m_DebugName = "PLACED_PAD::m_Next";

    m_NextInFp.m_TargetKey = aBlk.m_NextInFp;
    m_NextInFp.m_DebugName = "PLACED_PAD::m_NextInFp";

    m_NextInCompInst.m_TargetKey = aBlk.m_NextInCompInst;
    m_NextInCompInst.m_DebugName = "PLACED_PAD::m_NextInCompInst";

    // m_Padstack.m_TargetKey = aBlk.m_PadstackPtr;
    // m_Padstack.m_DebugName = "PLACED_PAD::m_Padstack";
}


bool PLACED_PAD::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_NextInFp.Resolve( aResolver );
    ok &= m_NextInCompInst.Resolve( aResolver );
    // ok &= m_Padstack.Resolve( aResolver );

    return ok;
}


VIA::VIA( const BRD_DB& aBrd, const BLK_0x33_VIA& aBlk ):
    DB_OBJ( DB_OBJ::TYPE::VIA, aBlk.m_Key )
{
    m_Next.m_TargetKey = aBlk.m_Next;
    // m_Next.m_EndKey = aBrd.m_Header->m_LL_0x33.m_Tail;
    m_Next.m_DebugName = "VIA::m_Next";
}


bool VIA::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );

    return ok;
}


bool BRD_DB::ResolveAndValidate()
{
    // First, iterate the whole table and resolve all the links
    ResolveObjectLinks();

    // Now, we can apply "board-y" knowledge and validate that various structures look
    // sensible

    // Or we can do the checks as we go... not sure which is best.

    bool ok = true;

    return true;
}


void BRD_DB::VisitFootprintDefs( BRD_DB::FP_DEF_VISITOR aVisitor ) const
{
    const auto fpDefNextFunc = [&]( const DB_OBJ& aObj ) -> const DB_REF&
    {
        if( aObj.GetType() != DB_OBJ::TYPE::FP_DEF )
            return DB_NULLREF;

        const FOOTPRINT_DEF& fpDef = static_cast<const FOOTPRINT_DEF&>( aObj );

        aVisitor( fpDef );

        return fpDef.m_Next;
    };

    visitLinkedList( m_Header->m_LL_0x2B, fpDefNextFunc );
}


void BRD_DB::visitFootprintInstances( const FOOTPRINT_DEF& aFpDef, VIEW_OBJS_VISITOR aVisitor ) const
{
    wxLogTrace( "ALLEGRO_EXTRACT", "Visiting footprint instances for footprint def key %#010x", aFpDef.GetKey() );

    VIEW_OBJS viewObjs;
    viewObjs.m_Board = this;

    const auto fpInstNextFunc = [&]( const DB_OBJ& aObj )
    {
        wxLogTrace( "ALLEGRO_EXTRACT", "Visiting footprint instance key %#010x", aObj.GetKey() );
        if( aObj.GetType() != DB_OBJ::TYPE::FP_INST )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "  Not a footprint instance, skipping key %#010x", aObj.GetKey() );
            return;
        }

        const FOOTPRINT_INSTANCE& fpInst = static_cast<const FOOTPRINT_INSTANCE&>( aObj );

        // viewObjs.m_FootprintDef = &aFpDef;
        viewObjs.m_FootprintInstance = &fpInst;

        const COMPONENT_INST* componentInstance = fpInst.GetComponentInstance();
        if( componentInstance )
        {
            viewObjs.m_Function = &componentInstance->GetFunctionInstance();
            viewObjs.m_Component = componentInstance->GetParentComponent();
        }

        aVisitor( viewObjs );
    };

    aFpDef.m_Instances.Visit( fpInstNextFunc );
}


void BRD_DB::VisitFootprintInstances( VIEW_OBJS_VISITOR aVisitor ) const
{
    wxLogTrace( "ALLEGRO_EXTRACT", "Visiting footprint instances" );

    VIEW_OBJS viewObjs;
    viewObjs.m_Board = this;

    // And now visit all the footprint instances, and then visit each field on each one
    VisitFootprintDefs(
            [&]( const FOOTPRINT_DEF& aFpDef )
            {
                visitFootprintInstances( aFpDef, aVisitor );
            } );
}


void BRD_DB::VisitFunctionInstances( VIEW_OBJS_VISITOR aVisitor ) const
{
    wxLogTrace( "ALLEGRO_EXTRACT", "Visiting function instances" );

    const auto componentVisitor = [&]( const VIEW_OBJS& aViewObjs )
    {
        aVisitor( aViewObjs );
    };

    // When is FUNCTION != COMPONENT? how should we iterate this?
    VisitComponents( componentVisitor );
}


void BRD_DB::VisitComponents( VIEW_OBJS_VISITOR aVisitor ) const
{
    wxLogTrace( "ALLEGRO_EXTRACT", "Visiting components" );

    VIEW_OBJS viewObjs;

    viewObjs.m_Board = this;

    const auto x06NextFunc = [&]( const DB_OBJ& aObj ) -> const DB_REF&
    {
        if( aObj.GetType() != DB_OBJ::TYPE::COMPONENT )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "  Not a component object, skipping key %#010x", aObj.GetKey() );
            return DB_NULLREF;
        }

        const COMPONENT& component = static_cast<const COMPONENT&>( aObj );

        viewObjs.m_Component = &component;

        const auto compInstVisitor = [&]( const DB_OBJ& aObj )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "Visiting component instance key %#010x", aObj.GetKey() );

            if( aObj.GetType() != DB_OBJ::TYPE::COMPONENT_INST )
            {
                wxLogTrace( "ALLEGRO_EXTRACT", "  Not a component instance, skipping key %#010x", aObj.GetKey() );
                return;
            }

            const COMPONENT_INST& compInst = static_cast<const COMPONENT_INST&>( aObj );

            const FUNCTION_INSTANCE& funcInst = compInst.GetFunctionInstance();
            viewObjs.m_ComponentInstance = &compInst;
            viewObjs.m_Function = &funcInst;

            aVisitor( viewObjs );
        };

        component.m_Instances.Visit( compInstVisitor );

        return component.m_Next;
    };

    visitLinkedList( m_Header->m_LL_0x06, x06NextFunc );
}


void BRD_DB::VisitComponentPins( VIEW_OBJS_VISITOR aVisitor ) const
{
    wxLogTrace( "ALLEGRO_EXTRACT", "Visiting component pins" );

    const auto componentVisitor = [&]( const VIEW_OBJS& aViewObjs )
    {
        // For each footprint instance, visit all the pins of the component
        const COMPONENT_INST* compInst = aViewObjs.m_ComponentInstance;

        wxCHECK2_MSG( compInst != nullptr, , "BRD_DB::VisitComponentPins: Null component instance in view objs" );

        const auto padVisitor = [&]( const DB_OBJ& aObj )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "Visiting pad key %#010x", aObj.GetKey() );
            if( aObj.GetType() != DB_OBJ::TYPE::PLACED_PAD )
            {
                wxLogTrace( "ALLEGRO_EXTRACT", "  Not a placed pad, skipping key %#010x, type", aObj.GetKey() );
                return;
            }

            const PLACED_PAD& placedPad = static_cast<const PLACED_PAD&>( aObj );

            VIEW_OBJS viewObj = aViewObjs;
            viewObj.m_Pad = &placedPad;

            aVisitor( viewObj );
        };

        compInst->m_Pads.Visit( padVisitor );
    };

    VisitComponents( componentVisitor );
}


void BRD_DB::VisitNets( VIEW_OBJS_VISITOR aVisitor ) const
{
    wxLogTrace( "ALLEGRO_EXTRACT", "Visiting nets" );

    VIEW_OBJS viewObjs;
    viewObjs.m_Board = this;

    const auto netNextFunc = [&]( const DB_OBJ& aObj ) -> const DB_REF&
    {
        if( aObj.GetType() != DB_OBJ::TYPE::NET )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "  Not a net object, skipping key %#010x", aObj.GetKey() );
            return DB_NULLREF;
        }

        const NET& net = static_cast<const NET&>( aObj );

        viewObjs.m_Net = &net;

        aVisitor( viewObjs );

        return net.m_Next;
    };

    visitLinkedList( m_Header->m_LL_0x1B_Nets, netNextFunc );
}


void BRD_DB::VisitConnectedGeometry( VIEW_OBJS_VISITOR aVisitor ) const
{
    wxLogTrace( "ALLEGRO_EXTRACT", "Visiting connected geometry" );

    VIEW_OBJS viewObjs;
    viewObjs.m_Board = this;

    const auto netVisitor = [&]( const VIEW_OBJS& aViewObjs )
    {
        const NET& net = *aViewObjs.m_Net;

        // const NET_ASSIGN* netAssign = net.GetAssignment();

        // if( netAssign == nullptr )
        // {
        //     wxLogTrace( "ALLEGRO_EXTRACT", "  Net %#010x has no assignment, skipping", net.GetKey() );
        //     return;
        // }



    };

    VisitNets( netVisitor );
}
