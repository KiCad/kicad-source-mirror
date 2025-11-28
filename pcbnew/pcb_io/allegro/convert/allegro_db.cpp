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
    const std::optional<uint32_t> blkKey = GetBlockKey( aBlock );

    // Cannot add un-keyed objects
    if( !blkKey.has_value() )
        return nullptr;

    std::unique_ptr<DB_OBJ> obj;

    switch( aBlock.GetBlockType() )
    {
    case 0x01:
    {
        const BLK_0x01_ARC& arcBlk = BLK_DATA( aBlock, BLK_0x01_ARC );

        // std::cout << "Arc" << std::endl;

        auto arc = std::make_unique<ARC>();

        arc->m_Parent = DB_REF( arcBlk.m_Parent );

        // obj = std::move( arc );
        break;
    }
    case 0x03:
    {
        const BLK_0x03& blk03 = BLK_DATA( aBlock, BLK_0x03 );

        switch( blk03.m_SubType )
        {
        case 0x68: // SYM_LIBRARY_PATH, ?...
            obj = std::make_unique<x03_TEXT>( blk03 );
            wxLogTrace( "ALLEGRO_EXTRACT", "Created x03_TEXT object for key %#010x", *blkKey );
            break;
        default:
            // Unknown subtype
            wxLogTrace( "ALLEGRO_EXTRACT", "Unknown BLK_0x03 subtype: 0x%02x", blk03.m_SubType );
            break;
        }

        break;
    }
    case 0x07:
    {
        const BLK_0x07& strBlk = BLK_DATA( aBlock, BLK_0x07 );
        obj = std::make_unique<TEXT>( strBlk );
        break;
    }
    case 0x15:
    case 0x16:
    case 0x17:
    {
        const BLK_0x15_16_17_SEGMENT& seg = BLK_DATA( aBlock, BLK_0x15_16_17_SEGMENT );

        // std::cout << "Seg" << std::endl;
        break;
    }
    case 0x2b: // Footprint
    {
        const BLK_0x2B& fpBlk = BLK_DATA( aBlock, BLK_0x2B );
        obj = std::make_unique<FOOTPRINT_DEF>( *this, fpBlk );
        break;
    }
    case 0x2d: // Footprint instance
    {
        const BLK_0x2D& fpInstBlk = BLK_DATA( aBlock, BLK_0x2D );
        obj = std::make_unique<FOOTPRINT_INSTANCE>( fpInstBlk );
        break;
    }
    default:
        break;
    }

    if( obj )
    {
        obj->m_Key = *blkKey;
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


bool ARC::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_Parent.Resolve( aResolver );

    if(!ok)
        wxLogTrace( "ALLEGRO_EXTRACT", "Failed to resolve ARC key %#010x", GetKey() );

    return ok;
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

TEXT::TEXT( const BLK_0x07& aBlk )
{
    m_TextStr.m_StringKey = aBlk.m_RefDesRef;
}


bool TEXT::ResolveRefs( const DB_OBJ_RESOLVER& aResolver )
{
    bool ok = true;

    ok &= m_TextStr.Resolve( aResolver );

    return ok;
}


x03_TEXT::x03_TEXT( const BLK_0x03& aBlk )
{
    // wxASSERT( aBlk.m_Substruct.is<std::string>() );
    m_TextStr = std::get<std::string>( aBlk.m_Substruct );
}







LINE::LINE( const BLK_0x15_16_17_SEGMENT& aBlk )
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

    ok &= m_Parent.Resolve( aResolver );
    ok &= m_Next.Resolve( aResolver );

    if( !ok )
        wxLogTrace( "ALLEGRO_EXTRACT", "Failed to resolve LINE key %#010x", GetKey() );

    return ok;
}


FOOTPRINT_DEF::FOOTPRINT_DEF( const BRD_DB& aBrd, const BLK_0x2B& aBlk )
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


FOOTPRINT_INSTANCE::FOOTPRINT_INSTANCE( const BLK_0x2D& aBlk )
{
    m_Next.m_TargetKey = aBlk.m_Next;

    // This can be in one of two fields
    m_RefDes.m_TargetKey = aBlk.m_InstRef16x.value_or( aBlk.m_InstRef.value() );

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
    ok &= m_RefDes.Resolve( aResolver );

    ok &= CheckTypeIs( m_RefDes, DB_OBJ::TYPE::TEXT, true );

    return ok;
}


const wxString* FOOTPRINT_INSTANCE::GetRefDes() const
{
    // The ref des is found in the 0x07 string ref
    // But it can be null (e.g. for dimensions)
    if( m_RefDes.m_Target == nullptr )
        return nullptr;

    TEXT& textObj = static_cast<TEXT&>( *m_RefDes.m_Target );
    return textObj.m_TextStr.m_String;
}



const wxString* FOOTPRINT_INSTANCE::GetName() const
{
    if( m_Parent == nullptr )
        return nullptr;

    return m_Parent->m_FpStr.m_String;
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


void BRD_DB::VisitFootprintDefs( std::function<void(const FOOTPRINT_DEF& aFpDef)> aVisitor ) const
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


void BRD_DB::VisitFootprintInstances( const FOOTPRINT_DEF& aFpDef, BRD_DB::FP_INST_VISITOR aVisitor ) const
{
    wxLogTrace( "ALLEGRO_EXTRACT", "Visiting footprint instances for footprint def key %#010x", aFpDef.GetKey() );

    const auto fpInstNextFunc = [&]( const DB_OBJ& aObj )
    {
        wxLogTrace( "ALLEGRO_EXTRACT", "Visiting footprint instance key %#010x", aObj.GetKey() );
        if( aObj.GetType() != DB_OBJ::TYPE::FP_INST )
        {
            wxLogTrace( "ALLEGRO_EXTRACT", "  Not a footprint instance, skipping key %#010x", aObj.GetKey() );
            return;
        }

        const FOOTPRINT_INSTANCE& fpInst = static_cast<const FOOTPRINT_INSTANCE&>( aObj );

        aVisitor( fpInst );
    };

    aFpDef.m_Instances.Visit( fpInstNextFunc );
}
