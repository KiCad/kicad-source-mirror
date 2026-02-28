/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "convert/allegro_db.h"

#include <set>

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
    case 0x03: return static_cast<const BLOCK<BLK_0x03_FIELD>&>( block ).GetData().m_Key;
    case 0x04: return static_cast<const BLOCK<BLK_0x04_NET_ASSIGNMENT>&>( block ).GetData().m_Key;
    case 0x05: return static_cast<const BLOCK<BLK_0x05_TRACK>&>( block ).GetData().m_Key;
    case 0x06: return static_cast<const BLOCK<BLK_0x06_COMPONENT>&>( block ).GetData().m_Key;
    case 0x07: return static_cast<const BLOCK<BLK_0x07_COMPONENT_INST>&>( block ).GetData().m_Key;
    case 0x08: return static_cast<const BLOCK<BLK_0x08_PIN_NUMBER>&>( block ).GetData().m_Key;
    case 0x09: return static_cast<const BLOCK<BLK_0x09_FILL_LINK>&>( block ).GetData().m_Key;
    case 0x0A: return static_cast<const BLOCK<BLK_0x0A_DRC>&>( block ).GetData().m_Key;
    case 0x0C: return static_cast<const BLOCK<BLK_0x0C_PIN_DEF>&>( block ).GetData().m_Key;
    case 0x0D: return static_cast<const BLOCK<BLK_0x0D_PAD>&>( block ).GetData().m_Key;
    case 0x0E: return static_cast<const BLOCK<BLK_0x0E_RECT>&>( block ).GetData().m_Key;
    case 0x0F: return static_cast<const BLOCK<BLK_0x0F_FUNCTION_SLOT>&>( block ).GetData().m_Key;
    case 0x10: return static_cast<const BLOCK<BLK_0x10_FUNCTION_INST>&>( block ).GetData().m_Key;
    case 0x11: return static_cast<const BLOCK<BLK_0x11_PIN_NAME>&>( block ).GetData().m_Key;
    case 0x12: return static_cast<const BLOCK<BLK_0x12_XREF>&>( block ).GetData().m_Key;
    case 0x14: return static_cast<const BLOCK<BLK_0x14_GRAPHIC>&>( block ).GetData().m_Key;
    case 0x15:
    case 0x16:
    case 0x17: return static_cast<const BLOCK<BLK_0x15_16_17_SEGMENT>&>( block ).GetData().m_Key;
    case 0x1B: return static_cast<const BLOCK<BLK_0x1B_NET>&>( block ).GetData().m_Key;
    case 0x1C: return static_cast<const BLOCK<BLK_0x1C_PADSTACK>&>( block ).GetData().m_Key;
    case 0x1D: return static_cast<const BLOCK<BLK_0x1D_CONSTRAINT_SET>&>( block ).GetData().m_Key;
    case 0x1E: return static_cast<const BLOCK<BLK_0x1E_SI_MODEL>&>( block ).GetData().m_Key;
    case 0x1F: return static_cast<const BLOCK<BLK_0x1F_PADSTACK_DIM>&>( block ).GetData().m_Key;
    case 0x20: return static_cast<const BLOCK<BLK_0x20_UNKNOWN>&>( block ).GetData().m_Key;
    case 0x21: return static_cast<const BLOCK<BLK_0x21_BLOB>&>( block ).GetData().m_Key;
    case 0x22: return static_cast<const BLOCK<BLK_0x22_UNKNOWN>&>( block ).GetData().m_Key;
    case 0x23: return static_cast<const BLOCK<BLK_0x23_RATLINE>&>( block ).GetData().m_Key;
    case 0x24: return static_cast<const BLOCK<BLK_0x24_RECT>&>( block ).GetData().m_Key;
    case 0x26: return static_cast<const BLOCK<BLK_0x26_MATCH_GROUP>&>( block ).GetData().m_Key;
    case 0x28: return static_cast<const BLOCK<BLK_0x28_SHAPE>&>( block ).GetData().m_Key;
    case 0x29: return static_cast<const BLOCK<BLK_0x29_PIN>&>( block ).GetData().m_Key;
    case 0x2A: return static_cast<const BLOCK<BLK_0x2A_LAYER_LIST>&>( block ).GetData().m_Key;
    case 0x2B: return static_cast<const BLOCK<BLK_0x2B_FOOTPRINT_DEF>&>( block ).GetData().m_Key;
    case 0x2C: return static_cast<const BLOCK<BLK_0x2C_TABLE>&>( block ).GetData().m_Key;
    case 0x2D: return static_cast<const BLOCK<BLK_0x2D_FOOTPRINT_INST>&>( block ).GetData().m_Key;
    case 0x2E: return static_cast<const BLOCK<BLK_0x2E_CONNECTION>&>( block ).GetData().m_Key;
    case 0x2F: return static_cast<const BLOCK<BLK_0x2F_UNKNOWN>&>( block ).GetData().m_Key;
    case 0x30: return static_cast<const BLOCK<BLK_0x30_STR_WRAPPER>&>( block ).GetData().m_Key;
    case 0x31: return static_cast<const BLOCK<BLK_0x31_SGRAPHIC>&>( block ).GetData().m_Key;
    case 0x32: return static_cast<const BLOCK<BLK_0x32_PLACED_PAD>&>( block ).GetData().m_Key;
    case 0x33: return static_cast<const BLOCK<BLK_0x33_VIA>&>( block ).GetData().m_Key;
    case 0x34: return static_cast<const BLOCK<BLK_0x34_KEEPOUT>&>( block ).GetData().m_Key;
    case 0x36: return static_cast<const BLOCK<BLK_0x36_DEF_TABLE>&>( block ).GetData().m_Key;
    case 0x37: return static_cast<const BLOCK<BLK_0x37_PTR_ARRAY>&>( block ).GetData().m_Key;
    case 0x38: return static_cast<const BLOCK<BLK_0x38_FILM>&>( block ).GetData().m_Key;
    case 0x39: return static_cast<const BLOCK<BLK_0x39_FILM_LAYER_LIST>&>( block ).GetData().m_Key;
    case 0x3A: return static_cast<const BLOCK<BLK_0x3A_FILM_LIST_NODE>&>( block ).GetData().m_Key;
    case 0x3C: return static_cast<const BLOCK<BLK_0x3C_KEY_LIST>&>( block ).GetData().m_Key;
    default: break;
    }
    // clang-format off

    return std::nullopt;
}


/**
 * Next ref getter for any chain where all objects uses the default "next" field.
 *
 * If any object in the chain doesn't use the default "next" field, you should set a custom
 * getter.
 */
static const DB_REF& GetPrimaryNext( const DB_OBJ& obj )
{
    return obj.GetNext();
}


uint32_t BLOCK_BASE::GetKey() const
{
    std::optional<uint32_t> key = GetBlockKey( *this );
    return key.value_or( 0 );
}


std::string RESOLVABLE::DebugString() const
{
    std::string s = (m_Parent ? m_Parent->TypeName() : "<no parent>") + std::string(":") + (m_DebugName ? m_DebugName : "<unknown>");
    return s;
}


bool DB_REF::Resolve( const DB_OBJ_RESOLVER& aResolver )
{
    if( m_TargetKey == m_EndKey )
    {
        m_Target = nullptr;
        return true;
    }

    m_Target = aResolver.Resolve( m_TargetKey );

    if( !m_Target )
    {
        // V18+ linked list sentinel keys are not in the DB but are valid
        // null-reference targets (end-of-chain markers)
        if( aResolver.IsSentinel( m_TargetKey ) )
        {
            return true;
        }

        wxLogTrace( "ALLEGRO_EXTRACT", "Failed to resolve DB_REF target key %#010x for %s",
                    m_TargetKey, m_DebugName ? m_DebugName : "<unknown>" );
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
    wxCHECK_MSG( m_NextRefGetter != nullptr, false, wxString::Format(
        "%s: Must set m_NextRefGetter before resolving DB_REF_CHAIN", DebugString() ) );

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
    std::set<uint32_t> visitedKeys;

    while( n != nullptr )
    {
        m_Chain.push_back( n );

        const DB_REF& nextRef = m_NextRefGetter( *n );

        const uint32_t nextKey = nextRef.m_TargetKey;

        if( visitedKeys.count( nextKey ) > 0 )
        {
            THROW_IO_ERROR( wxString::Format( "Detected loop in DB_REF_CHAIN at key %#010x for %s",
                                              nextKey, m_DebugName ? m_DebugName : "<unknown>" ) );
        }
        visitedKeys.insert( nextKey );

        if( nextKey == m_Tail )
        {
            return true;
        }

        if( nextKey == 0 )
        {
            // Reached end of chain before tail
            break;
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


void BRD_DB::ReserveCapacity( size_t aObjectCount, size_t aStringCount )
{
    // The object and strings count come from user input directly
    // clamp them in case we get a gigantic number
    aObjectCount = std::min( aObjectCount, static_cast<size_t>( 1e6 ) );
    aStringCount = std::min( aStringCount, static_cast<size_t>( 1e6 ) );

    m_Blocks.reserve( aObjectCount );
    m_ObjectKeyMap.reserve( aObjectCount );
    reserveObjects( aObjectCount );
    m_StringTable.reserve( aStringCount );
}


void BRD_DB::InsertBlock( std::unique_ptr<BLOCK_BASE> aBlock )
{
    bool skipDbObj = false;

    if( m_leanMode )
    {
        // Skip DB_OBJ creation for high-volume types that the BOARD_BUILDER accesses
        // exclusively through raw BLOCK_BASE via m_ObjectKeyMap and LL_WALKER.
        // Types like NET (0x1B) and FIELD (0x03) must still create DB_OBJ for VisitNets.
        uint8_t t = aBlock->GetBlockType();
        skipDbObj = ( t == 0x01 || t == 0x14 || t == 0x15 || t == 0x16 || t == 0x17 );
    }

    if( !skipDbObj )
    {
        std::unique_ptr<DB_OBJ> dbObj = m_ObjFactory.CreateObject( *aBlock );

        if( dbObj )
            AddObject( std::move( dbObj ) );
    }

    if( aBlock->GetKey() != 0 )
        m_ObjectKeyMap[aBlock->GetKey()] = aBlock.get();

    m_Blocks.push_back( std::move( aBlock ) );
}

std::unique_ptr<DB_OBJ> BRD_DB::OBJ_FACTORY::CreateObject( const BLOCK_BASE& aBlock ) const
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
        const BLK_0x03_FIELD& blk03 = BLK_DATA( aBlock, BLK_0x03_FIELD );
        obj = std::make_unique<FIELD>( blk03 );
        break;
    }
    case 0x04:
    {
        const BLK_0x04_NET_ASSIGNMENT& netBlk = BLK_DATA( aBlock, BLK_0x04_NET_ASSIGNMENT );
        obj = std::make_unique<NET_ASSIGN>( m_brdDb, netBlk );
        break;
    }
    case 0x05:
    {
        const BLK_0x05_TRACK& trackBlk = BLK_DATA( aBlock, BLK_0x05_TRACK );
        obj = std::make_unique<TRACK>( m_brdDb, trackBlk );
        break;
    }
    case 0x06:
    {
        const BLK_0x06_COMPONENT& compBlk = BLK_DATA( aBlock, BLK_0x06_COMPONENT );
        obj = std::make_unique<COMPONENT>( m_brdDb, compBlk );
        break;
    }
    case 0x07:
    {
        const BLK_0x07_COMPONENT_INST& strBlk = BLK_DATA( aBlock, BLK_0x07_COMPONENT_INST );
        obj = std::make_unique<COMPONENT_INST>( strBlk );
        break;
    }
    case 0x08:
    {
        const BLK_0x08_PIN_NUMBER& symbolBlk = BLK_DATA( aBlock, BLK_0x08_PIN_NUMBER );
        obj = std::make_unique<PIN_NUMBER>( symbolBlk );
        break;
    }
    case 0x0E:
    {
        const BLK_0x0E_RECT& pinBlk = BLK_DATA( aBlock, BLK_0x0E_RECT );
        obj = std::make_unique<RECT_OBJ>( m_brdDb, pinBlk );
        break;
    }
    case 0x0F:
    {
        const BLK_0x0F_FUNCTION_SLOT& funcSlotBlk = BLK_DATA( aBlock, BLK_0x0F_FUNCTION_SLOT );
        obj = std::make_unique<FUNCTION_SLOT>( funcSlotBlk );
        break;
    }
    case 0x10:
    {
        const BLK_0x10_FUNCTION_INST& funcInstBlk = BLK_DATA( aBlock, BLK_0x10_FUNCTION_INST );
        obj = std::make_unique<FUNCTION_INSTANCE>( funcInstBlk );
        break;
    }
    case 0x11:
    {
        const BLK_0x11_PIN_NAME& pinNameBlk = BLK_DATA( aBlock, BLK_0x11_PIN_NAME );
        obj = std::make_unique<PIN_NAME>( pinNameBlk );
        break;
    }
    case 0x12:
    {
        const BLK_0x12_XREF& blk = BLK_DATA( aBlock, BLK_0x12_XREF );
        obj = std::make_unique<XREF_OBJ>( blk );
        break;
    }
    case 0x14:
    {
        const BLK_0x14_GRAPHIC& lineBlk = BLK_DATA( aBlock, BLK_0x14_GRAPHIC );
        obj = std::make_unique<GRAPHIC_SEG>( m_brdDb, lineBlk );
        break;
    }
    case 0x15:
    case 0x16:
    case 0x17:
    {
        const BLK_0x15_16_17_SEGMENT& seg = BLK_DATA( aBlock, BLK_0x15_16_17_SEGMENT );
        obj = std::make_unique<LINE>( seg );
        break;
    }
    case 0x1b:
    {
        const BLK_0x1B_NET& netBlk = BLK_DATA( aBlock, BLK_0x1B_NET );
        obj = std::make_unique<NET>( m_brdDb, netBlk );
        break;
    }
    case 0x20:
    {
        const BLK_0x20_UNKNOWN& blk = BLK_DATA( aBlock, BLK_0x20_UNKNOWN );
        obj = std::make_unique<UNKNOWN_0x20>( m_brdDb, blk );
        break;
    }
    case 0x28:
    {
        const BLK_0x28_SHAPE& shapeBlk = BLK_DATA( aBlock, BLK_0x28_SHAPE );
        obj = std::make_unique<SHAPE>( m_brdDb, shapeBlk );
        break;
    }
    case 0x2b: // Footprint
    {
        const BLK_0x2B_FOOTPRINT_DEF& fpBlk = BLK_DATA( aBlock, BLK_0x2B_FOOTPRINT_DEF );
        obj = std::make_unique<FOOTPRINT_DEF>( m_brdDb, fpBlk );
        break;
    }
    case 0x2d:
    {
        const BLK_0x2D_FOOTPRINT_INST& fpInstBlk = BLK_DATA( aBlock, BLK_0x2D_FOOTPRINT_INST );
        obj = std::make_unique<FOOTPRINT_INSTANCE>( fpInstBlk );
        break;
    }
    case 0x2e:
    {
        const BLK_0x2E_CONNECTION& padBlk = BLK_DATA( aBlock, BLK_0x2E_CONNECTION );
        obj = std::make_unique<CONNECTION_OBJ>( m_brdDb, padBlk );
        break;
    }
    case 0x32:
    {
        const BLK_0x32_PLACED_PAD& placedPadBlk = BLK_DATA( aBlock, BLK_0x32_PLACED_PAD );
        obj =  std::make_unique<PLACED_PAD>( m_brdDb, placedPadBlk );
        break;
    }
    case 0x33:
    {
        const BLK_0x33_VIA& viaBlk = BLK_DATA( aBlock, BLK_0x33_VIA );
        obj =  std::make_unique<VIA>( m_brdDb, viaBlk );
        break;
    }
    case 0x37:
    {
        const BLK_0x37_PTR_ARRAY& arrBlk = BLK_DATA( aBlock, BLK_0x37_PTR_ARRAY );
        obj = std::make_unique<PTR_ARRAY>( m_brdDb, arrBlk );
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


DB_OBJ* DB::Resolve( uint32_t aKey ) const
{
    auto it = m_Objects.find( aKey );
    if( it != m_Objects.end() )
    {
        return it->second.get();
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


void DB::ResolveObjectLinksBestEffort()
{
    for( auto& [key, obj] : m_Objects )
        obj->m_Valid = obj->ResolveRefs( *this );
}


void DB::visitLinkedList( const FILE_HEADER::LINKED_LIST            aLList,
                          std::function<const DB_REF&( const DB_OBJ& aObj )> aVisitor ) const
{
    const DB_OBJ* node = Resolve( aLList.m_Head );

    size_t iterations = 0;

    while( node )
    {
        const DB_REF& nextRef = aVisitor( *node );

        if( !nextRef.m_Target && nextRef.m_TargetKey != aLList.m_Tail
            && !IsSentinel( nextRef.m_TargetKey ) )
        {
            THROW_IO_ERROR( wxString::Format( "Unexpected end of linked list: could not find %#010x", nextRef.m_TargetKey ) );
        }

        if( iterations++ >= 1e6 )
        {
            THROW_IO_ERROR( wxString::Format( "Excessive list length: key was %#010x", nextRef.m_TargetKey ) );
        }

        node = nextRef.m_Target;
    }
}


static bool CheckTypeIs( const DB_REF& aRef, DB_OBJ::TYPE_ID aType, bool aCanBeNull )
{
    if( aRef.m_Target == nullptr )
        return aCanBeNull;

    const DB_OBJ::TYPE_ID objType = aRef.m_Target->GetType();
    return objType == aType;
}

static bool CheckTypeIsOneOf( const DB_REF& aRef, const std::vector<DB_OBJ::TYPE_ID>& aTypes, bool aCanBeNull )
{
    if( aRef.m_Target == nullptr )
        return aCanBeNull;

    const DB_OBJ::TYPE_ID objType = aRef.m_Target->GetType();
    return std::find( aTypes.begin(), aTypes.end(), objType ) != aTypes.end();
}


// static DB_REF& FollowFootprintInstanceNext( DB_OBJ& aObj )
// {
//     switch( aObj.GetType() )
//     {
//     case DB_OBJ::BRD_FP_INST:
//         return static_cast<const FOOTPRINT_INSTANCE&>(aObj).m_Next.m_TargetKey;
//     }
// }


ARC::ARC( const BLK_0x01_ARC& aBlk ) :
    BRD_DB_OBJ( BRD_ARC, aBlk.m_Key ),
    m_Parent( this, aBlk.m_Parent, "m_parent" )
{
}


bool ARC::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    // m_Parent may point to objects of types we don't parse, so don't fail if it can't be resolved.
    m_Parent.Resolve( aResolver );

    return true;
}


NET_ASSIGN::NET_ASSIGN( const BRD_DB& aBrd, const BLK_0x04_NET_ASSIGNMENT& aBlk ):
    BRD_DB_OBJ( BRD_NET_ASSIGN, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_Net( this, aBlk.m_Net, "m_Net" ),
    m_ConnItem( this, aBlk.m_ConnItem, "m_ConnItem" )
{
}


bool NET_ASSIGN::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_Net.Resolve( aResolver );
    ok &= m_ConnItem.Resolve( aResolver );

    ok &= CheckTypeIs( m_Net, BRD_NET, false );

    return ok;
}


const NET& NET_ASSIGN::GetNet() const
{
    if( m_Net.m_Target == nullptr )
    {
        THROW_IO_ERROR( wxString::Format( "NET_ASSIGN::GetNet: NET reference is null for key %#010x", m_Key ) );
    }

    return static_cast<const ALLEGRO::NET&>( *m_Net.m_Target );
}


TRACK::TRACK( const BRD_DB& aBrd, const BLK_0x05_TRACK& aBlk ):
    BRD_DB_OBJ( BRD_TRACK, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" )
{
}


bool TRACK::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );

    return ok;
}


FIELD::FIELD( const BLK_0x03_FIELD& aBlk ):
    BRD_DB_OBJ( BRD_FIELD, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" )
{
    m_SubType = aBlk.m_SubType;
    m_Hdr1 = aBlk.m_Hdr1;
    m_Hdr2 = aBlk.m_Hdr2;

    switch ( aBlk.m_SubType )
    {
    case 0x68:
    {
        m_FieldValue = wxString(std::get<std::string>( aBlk.m_Substruct ) );
        break;
    }
    case 0x66:
    {
        m_FieldValue = std::get<uint32_t>( aBlk.m_Substruct );
        break;
    }
    }
}


const wxString& FIELD::ExpectString() const
{
    try
    {
        return std::get<wxString>( m_FieldValue );
    }
    catch( const std::bad_variant_access& )
    {
        THROW_IO_ERROR( "FIELD::ExpectString: Field value is not a string" );
    }
}


std::optional<int> FIELD_LIST::GetOptFieldExpectInt( uint16_t aFieldCode ) const
{
    for( const DB_OBJ* obj : m_Chain.m_Chain )
    {
        // Some chains can contain non-FIELD objects like 0x30
        // not clear if that is always true
        if( !obj || obj->GetType() != BRD_FIELD )
            continue;

        const FIELD& field = static_cast<const FIELD&>( *obj );
        if( field.m_Hdr1 == aFieldCode )
        {
            try
            {
                return std::get<uint32_t>( field.m_FieldValue );
            }
            catch( const std::bad_variant_access& )
            {
                THROW_IO_ERROR( wxString::Format( "FIELD code %#04x is not an integer (subtype: %#04x )", aFieldCode,
                                                field.m_SubType ) );
            }
        }
    }

    return std::nullopt;
}


const wxString* FIELD_LIST::GetOptFieldExpectString( uint16_t aFieldCode ) const
{
    for( const DB_OBJ* obj : m_Chain.m_Chain )
    {
        // Some chains can contain non-FIELD objects like 0x30
        // not clear if that is always true
        if( !obj || obj->GetType() != BRD_FIELD)
            continue;

        const FIELD& field = static_cast<const FIELD&>( *obj );
        if( field.m_Hdr1 == aFieldCode )
        {
            try
            {
                return &std::get<wxString>( field.m_FieldValue );
            }
            catch( const std::bad_variant_access& )
            {
                THROW_IO_ERROR( wxString::Format( "FIELD code %#04x is not a string (subtype: %#04x )", aFieldCode,
                                                field.m_SubType ) );
            }
        }
    }

    return nullptr;
}


std::optional<std::variant<wxString, uint32_t>> FIELD_LIST::GetOptField( uint16_t aFieldCode ) const
{
    for( const DB_OBJ* obj : m_Chain.m_Chain )
    {
        if( !obj || obj->GetType() != BRD_FIELD )
            continue;

        const FIELD& field = static_cast<const FIELD&>( *obj );

        if( field.m_Hdr1 == aFieldCode )
            return field.m_FieldValue;
    }

    return std::nullopt;
}


COMPONENT::COMPONENT( const BRD_DB& aBrd, const BLK_0x06_COMPONENT& aBlk ):
    BRD_DB_OBJ( BRD_COMPONENT, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_CompDeviceType( this, aBlk.m_CompDeviceType, "m_CompDeviceType" ),
    m_SymbolName( this, aBlk.m_SymbolName, "m_SymbolName" ),
    m_Instances( this, aBlk.m_FirstInstPtr, aBlk.m_Key, "m_Instances" ),
    m_PtrFunctionSlot( this, aBlk.m_PtrFunctionSlot, "m_PtrFunctionSlot" ),
    m_PtrPinNumber( this, aBlk.m_PtrPinNumber, "m_PtrPinNumber" ),
    m_Fields( this, aBlk.m_Fields, "m_Fields" )
{
    m_Instances.m_NextRefGetter = GetPrimaryNext;
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

    ok &= CheckTypeIs( m_PtrFunctionSlot, BRD_FUNCTION_SLOT, true );

    return ok;
}


const wxString* COMPONENT::GetComponentDeviceType() const
{
    return m_CompDeviceType.m_String;
}


COMPONENT_INST::COMPONENT_INST( const BLK_0x07_COMPONENT_INST& aBlk ):
    BRD_DB_OBJ( BRD_COMPONENT_INST, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_TextStr( this, aBlk.m_RefDesStrPtr, "m_TextStr" ),
    m_FunctionInst( this, aBlk.m_FunctionInstPtr, "m_FunctionInst" ),
    m_X03Chain( this, aBlk.m_X03Ptr, aBlk.m_Key, "m_X03Chain" ),
    m_Pads( this, aBlk.m_FirstPadPtr, aBlk.m_Key, "m_Pads" )
{
    m_Pads.m_NextRefGetter = []( const DB_OBJ& aObj ) -> const DB_REF&
    {
        const PLACED_PAD& pad = static_cast<const PLACED_PAD&>( aObj );
        return pad.m_NextInCompInst;
    };
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
    if( m_Next.m_Target == nullptr )
    {
        wxLogTrace( "ALLEGRO_EXTRACT", "COMPONENT_INST::GetNextInstance: Null m_Next reference for key %#010x", m_Key );
        return nullptr;
    }

    // If the next is not a COMPONENT_INST, it's the end of the list
    if( m_Next.m_Target->GetType() != BRD_COMPONENT_INST )
    {
        return nullptr;
    }

    return static_cast<const COMPONENT_INST*>( m_Next.m_Target );
}


PIN_NUMBER::PIN_NUMBER( const BLK_0x08_PIN_NUMBER& aBlk ) :
    BRD_DB_OBJ( BRD_PIN_NUMBER, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_PinNumberStr( this, aBlk.GetStrPtr(), "m_PinNumberStr" ),
    m_PinName( this, aBlk.m_PinNamePtr, "m_PinName" )
{
}


bool PIN_NUMBER::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_PinNumberStr.Resolve( aResolver );
    ok &= m_PinName.Resolve( aResolver );

    ok &= CheckTypeIs( m_PinName, BRD_PIN_NAME, true );

    return ok;
}


const wxString* PIN_NUMBER::GetNumber() const
{
    return m_PinNumberStr.m_String;
}


const PIN_NAME* PIN_NUMBER::GetPinName() const
{
    if( m_PinName.m_Target == nullptr )
    {
        return nullptr;
    }

    return static_cast<const PIN_NAME*>( m_PinName.m_Target );
}


RECT_OBJ::RECT_OBJ( const BRD_DB& aBrd, const BLK_0x0E_RECT& aBlk ) :
    BRD_DB_OBJ( BRD_x0e_RECT, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" )
{
    m_Rotation = EDA_ANGLE( static_cast<double>( aBlk.m_Rotation ) / 1000.0, DEGREES_T );
}


bool RECT_OBJ::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );

    return ok;
}


PIN_NAME::PIN_NAME( const BLK_0x11_PIN_NAME& aBlk ) :
    BRD_DB_OBJ( BRD_PIN_NAME, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_PinNameStr( this, aBlk.m_PinNameStrPtr, "m_PinNameStr" ),
    m_PinNumber( this, aBlk.m_PinNumberPtr, "m_PinNumber" )
{
}

bool PIN_NAME::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_PinNameStr.Resolve( aResolver );
    ok &= m_Next.Resolve( aResolver );
    ok &= m_PinNumber.Resolve( aResolver );

    ok &= CheckTypeIs( m_PinNumber, BRD_PIN_NUMBER, true );

    return ok;
}


const wxString* PIN_NAME::GetName() const
{
    return m_PinNameStr.m_String;
}


const PIN_NUMBER* PIN_NAME::GetPinNumber() const
{
    if( m_PinNumber.m_Target == nullptr )
    {
        return nullptr;
    }

    return static_cast<const PIN_NUMBER*>( m_PinNumber.m_Target );
}


XREF_OBJ::XREF_OBJ( const BLK_0x12_XREF& aBlk ) :
    BRD_DB_OBJ( BRD_XREF, aBlk.m_Key ),
    m_Ptr1( this, aBlk.m_Ptr1, "m_Ptr1" ),
    m_Ptr2( this, aBlk.m_Ptr2, "m_Ptr2" ),
    m_Ptr3( this, aBlk.m_Ptr3, "m_Ptr3" )
{
}


bool XREF_OBJ::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    // These pointers may point to objects we don't parse, so don't fail if resolution fails
    m_Ptr1.Resolve( aResolver );
    m_Ptr2.Resolve( aResolver );
    m_Ptr3.Resolve( aResolver );

    return true;
}


GRAPHIC_SEG::GRAPHIC_SEG( const BRD_DB& aBrd, const BLK_0x14_GRAPHIC& aBlk ):
    BRD_DB_OBJ( BRD_GRAPHIC_SEG, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_Parent( this, aBlk.m_Parent, "m_Parent" ),
    m_Segment( this, aBlk.m_SegmentPtr, "m_Segment" )
{
}


bool GRAPHIC_SEG::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Parent.Resolve( aResolver );
    ok &= m_Next.Resolve( aResolver );
    ok &= m_Segment.Resolve( aResolver );

    ok &= CheckTypeIsOneOf( m_Segment,
        {
            BRD_LINE,
            BRD_ARC,
        },
        false );

    return ok;
}


LINE::LINE( const BLK_0x15_16_17_SEGMENT& aBlk ):
    BRD_DB_OBJ( BRD_LINE, aBlk.m_Key ),
    m_Parent( this, aBlk.m_Parent, "m_Parent" ),
    m_Next( this, aBlk.m_Next, "m_Next" )
{
    m_Start.x = aBlk.m_StartX;
    m_Start.y = aBlk.m_StartY;
    m_End.x = aBlk.m_EndX;
    m_End.y = aBlk.m_EndY;

    m_Width = aBlk.m_Width;
}


bool LINE::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    // m_Next may point to objects we don't parse (terminator values), so don't fail if it
    // can't be resolved. Segments are iterated by following the parent SHAPE/TRACK.
    m_Next.Resolve( aResolver );

    return true;
}


FOOTPRINT_DEF::FOOTPRINT_DEF( const BRD_DB& aBrd, const BLK_0x2B_FOOTPRINT_DEF& aBlk ):
    BRD_DB_OBJ( BRD_FP_DEF, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_FpStr( this, aBlk.m_FpStrRef, "m_FpStr" ),
    m_SymLibPath( this, aBlk.m_SymLibPathPtr, "m_SymLibPath" ),
    m_Instances( this, aBlk.m_FirstInstPtr, aBlk.m_Key, "m_Instances" )
{
    // 0x2Bs are linked together in a list from the board header
    m_Next.m_EndKey = aBrd.m_Header->m_LL_0x2B.m_Tail;

    m_Instances.m_NextRefGetter = GetPrimaryNext;
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
        if( aObj.GetType() != BRD_FP_INST )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "FOOTPRINT_DEF::ResolveRefs: Unexpected type in footprint instance chain: %d", static_cast<int>( aObj.GetType() ) );
            return;
        }

        FOOTPRINT_INSTANCE& fpInst = static_cast<FOOTPRINT_INSTANCE&>( aObj );
        fpInst.m_Parent = this;
    } );

    wxLogTrace( "ALLEGRO_EXTRACT", "Resolved %zu footprint instances for footprint def key %#010x", m_Instances.m_Chain.size(), GetKey() );

    ok &= m_SymLibPath.Resolve( aResolver );

    ok &= CheckTypeIs( m_Next, BRD_FP_DEF, true );

    return ok;
}


const wxString* FOOTPRINT_DEF::GetLibPath() const
{
    if( m_SymLibPath.m_Target == nullptr )
        return nullptr;

    const FIELD& symLibPath = static_cast<const FIELD&>( *m_SymLibPath.m_Target );
    return &symLibPath.ExpectString();
}


FOOTPRINT_INSTANCE::FOOTPRINT_INSTANCE( const BLK_0x2D_FOOTPRINT_INST& aBlk ):
    BRD_DB_OBJ( BRD_FP_INST, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_ComponentInstance( this, aBlk.GetInstRef(), "m_ComponentInstance" ),
    m_Pads( this, aBlk.m_FirstPadPtr, aBlk.m_Key, "m_Pads" ),
    m_Graphics( this, aBlk.m_GraphicPtr, aBlk.m_Key, "m_Graphics" )
{
    m_Pads.m_NextRefGetter = []( const DB_OBJ& aObj ) -> const DB_REF&
    {

        if (aObj.GetType() != BRD_PLACED_PAD )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "FOOTPRINT_INSTANCE::m_Pads: Unexpected type in pad chain: %d", static_cast<int>( aObj.GetType() ) );
            return DB_NULLREF;
        }

        return static_cast<const PLACED_PAD&>( aObj ).m_NextInFp;
    };

    m_Graphics.m_NextRefGetter = GetPrimaryNext;

    // This will be filled in by the 0x2B resolution
    m_Parent = nullptr;

    m_X = aBlk.m_CoordX;
    m_Y = aBlk.m_CoordY;
    m_Rotation = aBlk.m_Rotation;
    m_Mirrored = ( aBlk.m_Layer != 0 );
}


bool FOOTPRINT_INSTANCE::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_ComponentInstance.Resolve( aResolver );
    ok &= m_Pads.Resolve( aResolver );
    ok &= m_Graphics.Resolve( aResolver );

    ok &= CheckTypeIs( m_ComponentInstance, BRD_COMPONENT_INST, true );

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


FUNCTION_SLOT::FUNCTION_SLOT( const BLK_0x0F_FUNCTION_SLOT& aBlk ):
    BRD_DB_OBJ( BRD_FUNCTION_SLOT, aBlk.m_Key ),
    m_SlotName( this, aBlk.m_SlotName, "m_SlotName" ),
    m_Component( this, aBlk.m_Ptr0x06, "m_Component" ),
    m_PinName( this, aBlk.m_Ptr0x11, "m_PinName" )
{
}


bool FUNCTION_SLOT::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_SlotName.Resolve( aResolver );

    // m_Component may point to objects we don't parse, so don't fail if resolution fails
    m_Component.Resolve( aResolver );

    ok &= m_PinName.Resolve( aResolver );

    return ok;
}


const wxString* FUNCTION_SLOT::GetName() const
{
    return m_SlotName.m_String;
}


FUNCTION_INSTANCE::FUNCTION_INSTANCE( const BLK_0x10_FUNCTION_INST& aBlk ):
    BRD_DB_OBJ( BRD_FUNCTION_INST, aBlk.m_Key ),
    m_Slot( this, aBlk.m_Slots, "m_Slot" ),
    m_Fields( this, aBlk.m_Fields, "m_Fields" ),
    m_FunctionName( this, aBlk.m_FunctionName, "m_FunctionName" ),
    m_ComponentInstance( this, aBlk.m_ComponentInstPtr, "m_ComponentInstance" )
{
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
    if( m_ComponentInstance.m_Target == nullptr )
    {
        THROW_IO_ERROR(
                "FUNCTION_INSTANCE::GetComponentInstance: Null reference to COMPONENT_INST" );
    }

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
    BRD_DB_OBJ( BRD_NET, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_NetNameStr( this, aBlk.m_NetName, "m_NetNameStr" ),
    m_NetAssignments( this, aBlk.m_Assignment, aBlk.m_Key, "m_NetAssignments" ),
    m_FieldsChain( this, aBlk.m_FieldsPtr, aBlk.m_Key, "m_FieldsChain" ),
    m_Fields( m_FieldsChain ),
    m_Status( STATUS::REGULAR )
{
    m_NetAssignments.m_NextRefGetter = GetPrimaryNext;
    m_FieldsChain.m_NextRefGetter = GetPrimaryNext;

    // Unsure where status is stored; default to REGULAR
}


bool NET::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_NetNameStr.Resolve( aResolver );
    ok &= m_NetAssignments.Resolve( aResolver );
    ok &= m_FieldsChain.Resolve( aResolver );
    return ok;
}


const wxString* NET::GetName() const
{
    return m_NetNameStr.m_String;
}


NET::STATUS NET::GetStatus() const
{
    return m_Status;
}


const wxString* NET::GetLogicalPath() const
{
    return m_Fields.GetOptFieldExpectString( FIELD_KEYS::LOGICAL_PATH );
}


std::optional<int> NET::GetNetMinLineWidth() const
{
    return m_Fields.GetOptFieldExpectInt( FIELD_KEYS::MIN_LINE_WIDTH );
}


std::optional<int> NET::GetNetMaxLineWidth() const
{
    return m_Fields.GetOptFieldExpectInt( FIELD_KEYS::MAX_LINE_WIDTH );
}


std::optional<int> NET::GetNetMinNeckWidth() const
{
    return m_Fields.GetOptFieldExpectInt( FIELD_KEYS::MIN_NECK_WIDTH );
}


std::optional<int> NET::GetNetMaxNeckLength() const
{
    return m_Fields.GetOptFieldExpectInt( FIELD_KEYS::MAX_NECK_LENGTH );
}


UNKNOWN_0x20::UNKNOWN_0x20( const BRD_DB& aBrd, const BLK_0x20_UNKNOWN& aBlk ):
    BRD_DB_OBJ( BRD_x20, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" )
{
}


bool UNKNOWN_0x20::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    // m_Next may point to objects we don't parse, so don't fail if it can't be resolved.
    m_Next.Resolve( aResolver );

    return true;
}


SHAPE::SHAPE( const BRD_DB& aBrd, const BLK_0x28_SHAPE& aBlk ):
    BRD_DB_OBJ( BRD_SHAPE, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_Segments( this, aBlk.m_FirstSegmentPtr, aBlk.m_Key, "m_Segments" )
{
    m_Segments.m_Tail = aBrd.m_Header->m_LL_Shapes.m_Tail;
    m_Segments.m_NextRefGetter = GetPrimaryNext;
}


bool SHAPE::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    // m_Next may point to objects we don't parse, so don't fail if it can't be resolved.
    // The SHAPE linked list is used internally by Allegro but not needed for board building.
    m_Next.Resolve( aResolver );

    ok &= m_Segments.Resolve( aResolver );

    return ok;
}


CONNECTION_OBJ::CONNECTION_OBJ( const BRD_DB& aBrd, const BLK_0x2E_CONNECTION& aBlk ):
    BRD_DB_OBJ( BRD_CONNECTION, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_NetAssign( this, aBlk.m_NetAssignment, "m_NetAssign" ),
    m_Connection( this, aBlk.m_Connection, "m_Connection" ),
    m_Position( aBlk.m_CoordX, aBlk.m_CoordY )
{
}


bool CONNECTION_OBJ::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_NetAssign.Resolve( aResolver );

    // m_Connection may point to objects we don't parse, so don't fail if resolution fails
    m_Connection.Resolve( aResolver );

    return ok;
}


PLACED_PAD::PLACED_PAD( const BRD_DB& aBrd, const BLK_0x32_PLACED_PAD& aBlk ):
    BRD_DB_OBJ( BRD_PLACED_PAD, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_NextInFp( this, aBlk.m_NextInFp, "m_NextInFp" ),
    m_NextInCompInst( this, aBlk.m_NextInCompInst, "m_NextInCompInst" ),
    m_NetAssign( this, aBlk.m_NetPtr, "m_NetAssign" ),
    m_PinNumber( this, aBlk.m_PtrPinNumber, "m_PinNumber" ),
    m_PinNumText( this, aBlk.m_NameText, "m_PinNumText" )
{
    m_Bounds = BOX2I(
        {aBlk.m_Coords[0], aBlk.m_Coords[1] },
        { aBlk.m_Coords[2], aBlk.m_Coords[3] }
    );
}


bool PLACED_PAD::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_NextInFp.Resolve( aResolver );
    ok &= m_NextInCompInst.Resolve( aResolver );
    // ok &= m_Padstack.Resolve( aResolver );
    ok &= m_PinNumber.Resolve( aResolver );
    ok &= m_NetAssign.Resolve( aResolver );

    ok &= CheckTypeIs( m_PinNumber, BRD_PIN_NUMBER, true );


    return ok;
}


const wxString* PLACED_PAD::GetPinNumber() const
{
    if( m_PinNumber.m_Target == nullptr )
        return nullptr;

    return static_cast<const PIN_NUMBER*>( m_PinNumber.m_Target )->GetNumber();
}


const wxString* PLACED_PAD::GetPinName() const
{
    if( m_PinNumber.m_Target == nullptr )
        return nullptr;

    const PIN_NUMBER* pinNumber = static_cast<const PIN_NUMBER*>( m_PinNumber.m_Target );
    const PIN_NAME* pinName = pinNumber->GetPinName();

    if( pinName == nullptr )
        return nullptr;

    return pinName->GetName();
}


const NET* PLACED_PAD::GetNet() const
{
    if( m_NetAssign.m_Target == nullptr )
        return nullptr;

    const NET_ASSIGN* netAssign = static_cast<const NET_ASSIGN*>( m_NetAssign.m_Target );

    return &netAssign->GetNet();
}


VIA::VIA( const BRD_DB& aBrd, const BLK_0x33_VIA& aBlk ):
    BRD_DB_OBJ( BRD_VIA, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_NetAssign( this, aBlk.m_NetPtr, "m_NetAssign" )
{
}


bool VIA::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );

    return ok;
}


PTR_ARRAY::PTR_ARRAY( const BRD_DB& aBrd, const BLK_0x37_PTR_ARRAY& aBlk ) :
    BRD_DB_OBJ( BRD_PTR_ARRAY, aBlk.m_Key ),
    m_Next( this, aBlk.m_Next, "m_Next" ),
    m_Parent( this, aBlk.m_GroupPtr, "m_GroupPtr" )
{
    m_Ptrs.reserve( aBlk.m_Count );
    for( size_t i = 0; i < aBlk.m_Count; ++i )
    {
        m_Ptrs.emplace_back( this, aBlk.m_Ptrs[i], nullptr );
    }
}


bool PTR_ARRAY::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Next.Resolve( aResolver );
    ok &= m_Parent.Resolve( aResolver );

    for( auto& ptr : m_Ptrs )
    {
        // These may point to objects we don't parse, so don't fail if resolution fails
        ptr.Resolve( aResolver );
    }

    return ok;
}


static void collectSentinelKeys( const FILE_HEADER& aHeader, DB& aDb )
{
    auto addTail = [&]( const FILE_HEADER::LINKED_LIST& aLL )
    {
        aDb.AddSentinelKey( aLL.m_Tail );
    };

    addTail( aHeader.m_LL_0x04 );
    addTail( aHeader.m_LL_0x06 );
    addTail( aHeader.m_LL_0x0C );
    addTail( aHeader.m_LL_Shapes );
    addTail( aHeader.m_LL_0x14 );
    addTail( aHeader.m_LL_0x1B_Nets );
    addTail( aHeader.m_LL_0x1C );
    addTail( aHeader.m_LL_0x24_0x28 );
    addTail( aHeader.m_LL_Unknown1 );
    addTail( aHeader.m_LL_0x2B );
    addTail( aHeader.m_LL_0x03_0x30 );
    addTail( aHeader.m_LL_0x0A );
    addTail( aHeader.m_LL_0x1D_0x1E_0x1F );
    addTail( aHeader.m_LL_Unknown2 );
    addTail( aHeader.m_LL_0x38 );
    addTail( aHeader.m_LL_0x2C );
    addTail( aHeader.m_LL_0x0C_2 );
    addTail( aHeader.m_LL_Unknown3 );
    addTail( aHeader.m_LL_0x36 );
    addTail( aHeader.GetUnknown5() );
    addTail( aHeader.m_LL_Unknown6 );
    addTail( aHeader.m_LL_0x0A_2 );

    if( aHeader.m_LL_V18_1.has_value() )
    {
        addTail( aHeader.m_LL_V18_1.value() );
        addTail( aHeader.m_LL_V18_2.value() );
        addTail( aHeader.m_LL_V18_3.value() );
        addTail( aHeader.m_LL_V18_4.value() );
        addTail( aHeader.m_LL_V18_5.value() );
        addTail( aHeader.m_LL_V18_6.value() );
    }
}


BRD_DB::BRD_DB() :
        m_FmtVer( FMT_VER::V_UNKNOWN ),
        m_ObjFactory( *this ),
        m_leanMode( false )
{
}


bool BRD_DB::ResolveAndValidate()
{
    if( m_FmtVer >= FMT_VER::V_180 )
        collectSentinelKeys( *m_Header, *this );

    if( m_leanMode )
    {
        // In lean mode, DB_OBJ was not created for high-volume types (segments, graphics,
        // arcs). Other DB_OBJ types that reference those keys will fail to resolve, so we
        // attempt best-effort resolution. The NET and FIELD objects the builder needs only
        // reference each other and will resolve correctly.
        ResolveObjectLinksBestEffort();
        return true;
    }

    // Try strict resolution first. If it fails (some boards have object types the parser
    // doesn't fully support), fall back to best-effort which allows partial imports.
    try
    {
        ResolveObjectLinks();
    }
    catch( const IO_ERROR& e )
    {
        wxLogTrace( "ALLEGRO_EXTRACT",
                    "Strict reference resolution failed (%s), retrying with best-effort",
                    e.Problem() );

        ResolveObjectLinksBestEffort();
    }

    return true;
}


void BRD_DB::VisitFootprintDefs( BRD_DB::FP_DEF_VISITOR aVisitor ) const
{
    const auto fpDefNextFunc = [&]( const DB_OBJ& aObj ) -> const DB_REF&
    {
        if( aObj.GetType() != BRD_FP_DEF )
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
        if( aObj.GetType() != BRD_FP_INST )
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
        if( aObj.GetType() != BRD_COMPONENT )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "  Not a component object, skipping key %#010x", aObj.GetKey() );
            return DB_NULLREF;
        }

        const COMPONENT& component = static_cast<const COMPONENT&>( aObj );

        viewObjs.m_Component = &component;

        const auto compInstVisitor = [&]( const DB_OBJ& aCompInst )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "Visiting component instance key %#010x", aCompInst.GetKey() );

            if( aCompInst.GetType() != BRD_COMPONENT_INST )
            {
                wxLogTrace( "ALLEGRO_EXTRACT", "  Not a component instance, skipping key %#010x", aCompInst.GetKey() );
                return;
            }

            const COMPONENT_INST& compInst = static_cast<const COMPONENT_INST&>( aCompInst );

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

        if( compInst == nullptr )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "  No component instance in view objs, skipping" );
            return;
        }

        const auto padVisitor = [&]( const DB_OBJ& aObj )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "Visiting pad key %#010x", aObj.GetKey() );
            if( aObj.GetType() != BRD_PLACED_PAD )
            {
                wxLogTrace( "ALLEGRO_EXTRACT", "  Not a placed pad, skipping key %#010x, type", aObj.GetKey() );
                return;
            }

            const PLACED_PAD& placedPad = static_cast<const PLACED_PAD&>( aObj );

            VIEW_OBJS viewObj = aViewObjs;
            viewObj.m_Pad = &placedPad;
            viewObj.m_Net = placedPad.GetNet();

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
        if( aObj.GetType() != BRD_NET )
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
        // const NET& net = *aViewObjs.m_Net;

        // const NET_ASSIGN* netAssign = net.GetAssignment();

        // if( netAssign == nullptr )
        // {
        //     wxLogTrace( "ALLEGRO_EXTRACT", "  Net %#010x has no assignment, skipping", net.GetKey() );
        //     return;
        // }
    };

    VisitNets( netVisitor );
}
