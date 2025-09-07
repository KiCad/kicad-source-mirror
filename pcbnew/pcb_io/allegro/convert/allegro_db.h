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

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include <convert/allegro_pcb_structs.h>

namespace ALLEGRO
{

class BLOCK_BASE;
class DB_OBJ;
class DB_OBJ_RESOLVER;


struct RESOLVABLE
{
    virtual bool Resolve( const DB_OBJ_RESOLVER& aResolver ) = 0;
};


struct DB_REF : public RESOLVABLE
{
    explicit constexpr DB_REF( uint32_t aTargetKey ) :
            m_TargetKey( aTargetKey ),
            m_Target( nullptr )
    {
    }

    constexpr DB_REF() :
            DB_REF( 0 )
    {
    }

    bool Resolve( const DB_OBJ_RESOLVER& aResolver ) override;

    uint32_t m_TargetKey;
    DB_OBJ*  m_Target;
};

static constexpr DB_REF DB_NULLREF = {};

struct DB_REF_CHAIN : public RESOLVABLE
{
    DB_REF_CHAIN( uint32_t aHead, uint32_t aTail ) :
            m_Head( aHead ),
            m_Tail( aTail )
    {
    }

    DB_REF_CHAIN() :
            DB_REF_CHAIN( 0, 0 )
    {
    }

    bool Resolve( const DB_OBJ_RESOLVER& aResolver ) override;

    std::function<uint32_t( const DB_OBJ& )> m_NextKeyGetter;
    uint32_t                                 m_Head;
    uint32_t                                 m_Tail;
};


struct DB_STR_REF : public RESOLVABLE
{
    explicit constexpr DB_STR_REF( uint32_t aTargetKey ) :
            m_StringKey( aTargetKey ),
            m_String( nullptr )
    {
    }

    constexpr DB_STR_REF() :
            DB_STR_REF( 0 )
    {
    }

    bool Resolve( const DB_OBJ_RESOLVER& aResolver ) override;

    uint32_t        m_StringKey;
    const wxString* m_String;
};

static constexpr DB_STR_REF DB_STRNULLREF = {};

/**
 * Some interface that can yield DB_OBJs for keys
 */
class DB_OBJ_RESOLVER
{
public:
    virtual ~DB_OBJ_RESOLVER() {}

    /**
     * Resolve the given reference
     */
    virtual DB_OBJ* Resolve( uint32_t aKey ) const = 0;

    virtual const wxString* ResolveString( uint32_t aKey ) const = 0;
};


/**
 * A DB_OBJ represents one object in an Allegro database.
 */
struct DB_OBJ
{
    enum class TYPE
    {
        ARC,
        SEGMENT,
        FP_DEF,
        FP_INST,
    };

    // Where this block was in the file (for debugging)
    struct FILE_LOC
    {
        size_t  m_Offset;
        uint8_t m_BlockType;
    };

    DB_OBJ() :
            m_Valid( false ),
            m_Key( 0 ),
            m_Loc( 0, 0 )
    {
    }

    virtual ~DB_OBJ() {}

    /**
     * Called when all objects in the DB are read and can be resolved by their IDs by other objects.
     *
     * Exactly what fields a given object needs to resolve and what happens if the resolution fails is up to that object.
     *
     * This can also validate that the objects found are of the expected types.
     *
     * Before calling this, you cannot expect an DB_REF to have a valid target.
     *
     * @return true if all fields in the object are resolved and valid
     */
    virtual bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) = 0;

    virtual TYPE GetType() const = 0;

    uint32_t GetKey() const { return m_Key; }

    bool     m_Valid;
    uint32_t m_Key;
    FILE_LOC m_Loc;
};


/**
 * An ALLEGRO::DB is the represention of the actual data stored within
 * an Allegro file, without specific reference to the format on disk. Mostly,
 * this DB handles objects in lists.
 */
class DB : public DB_OBJ_RESOLVER
{
public:
    DB();

    ~DB();

    void AddObject( std::unique_ptr<DB_OBJ> aObject );

    void AddString( uint32_t aKey, wxString&& aStr ) { m_StringTable.emplace( aKey, std::move( aStr ) ); }

    size_t GetObjectCount() const { return m_Objects.size(); }

    /**
     * Iterate the database and resolve links.
     *
     * This has to be done after all the objects are read, as they are not
     * necessarily read in order.
     */
    void ResolveObjectLinks();

    /**
     * Implement the object resolver interface
     */
    DB_OBJ* Resolve( uint32_t aRef ) const override;

    const wxString* ResolveString( uint32_t aRef ) const override;

    virtual void InsertBlock( const BLOCK_BASE& aBlock ) = 0;

protected:
    void visitLinkedList( const FILE_HEADER::LINKED_LIST                     aLList,
                          std::function<const DB_REF&( const DB_OBJ& aObj )> aVisitor ) const;

private:
    // Main store of DB objects.
    std::unordered_map<uint32_t, std::unique_ptr<DB_OBJ>> m_Objects;

public:
    std::unordered_map<uint32_t, wxString> m_StringTable;
};


struct ARC : public DB_OBJ
{
    DB_REF m_Parent;

    TYPE GetType() const { return TYPE::ARC; };
    ;
    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;
};


struct FOOTPRINT_INSTANCE;

/**
 *
 */
struct FOOTPRINT_DEF : public DB_OBJ
{
    FOOTPRINT_DEF( const BLK_0x2B& aBlk );

    DB_REF     m_Next;
    DB_STR_REF m_FpStr;

    DB_REF_CHAIN m_Instances;

    TYPE GetType() const { return TYPE::FP_DEF; };

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;
};


struct FOOTPRINT_INSTANCE : public DB_OBJ
{
    FOOTPRINT_INSTANCE( const BLK_0x2D& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
    DB_REF m_RefDes;
};


/**
 * An Allegro database that represents a .brd file (amd presumably .dra)
 */
class BRD_DB : public DB
{
public:
    BRD_DB() :
            m_FmtVer( FMT_VER::V_UNKNOWN )
    {
    }

    void InsertBlock( const BLOCK_BASE& aBlock ) override;

    /**
     * Iterate all the links we know about and fill in the object links
     *
     * This means that when we come to use the objects, we don't have to keep
     * looking them up in the DB and handling failures.
     */
    bool ResolveAndValidate();

    /**
     * Access the footprint defs in the database.
     *
     * This iterates the 0x2B linked list.
     */
    void VisitFootprintDefs( std::function<void( const FOOTPRINT_DEF& aFpDef )> aVisitor ) const;

    // It's not fully clear how much of the header is brd specific or is a more general
    // DB format (or is there is a more general format). Clearly much of it (linked lists,
    // for example) is very board-related.
    // For now, keep it up here, but generalities can push down to DB.
    FMT_VER                      m_FmtVer;
    std::unique_ptr<FILE_HEADER> m_Header;

private:
    std::unique_ptr<DB_OBJ> createObject( const BLOCK_BASE& aBlock );
};


} // namespace ALLEGRO
