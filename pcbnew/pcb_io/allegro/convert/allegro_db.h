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

#include <math/vector2d.h>
#include <math/box2.h>

#include <convert/allegro_pcb_structs.h>


namespace ALLEGRO
{

class BLOCK_BASE;
class DB_OBJ;
class DB_OBJ_RESOLVER;


struct RESOLVABLE
{
    virtual bool Resolve( const DB_OBJ_RESOLVER& aResolver ) = 0;

    const char* m_DebugName = nullptr;
};


struct DB_REF : public RESOLVABLE
{
    explicit constexpr DB_REF( uint32_t aTargetKey ) :
            m_TargetKey( aTargetKey ),
            m_EndKey( 0 ),
            m_Target( nullptr )
    {
    }

    constexpr DB_REF() :
            DB_REF( 0 )
    {
    }

    bool Resolve( const DB_OBJ_RESOLVER& aResolver ) override;

    uint32_t m_TargetKey;
    // If the ref points to the next item, eventually this will be equal to EndKey
    // which may not be a resolvable object (e.g. a header LinkedList tail pointer, which is
    // an artificial value).
    // This is not a resolution failure, but it also means the reference is null.
    uint32_t m_EndKey;
    DB_OBJ*  m_Target;
};

static constexpr DB_REF DB_NULLREF = {};

/**
 * Chain of DB references that lead from the head to the tail
 */
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

    /**
     * Visit all objects in the chain
     */
    void Visit( std::function<void( const DB_OBJ& aObj )> aVisitor ) const;
    void Visit( std::function<void( DB_OBJ& aObj )> aVisitor );

    std::function<uint32_t( const DB_OBJ& )> m_NextKeyGetter;
    uint32_t                                 m_Head;
    uint32_t                                 m_Tail;

    // The objects in the chain
    std::vector<DB_OBJ*> m_Chain;
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
        x03_TEXT,    // 0x03 subtype 0x68...
        TRACK,       // 0x05
        NET_ASSIGN,  // 0x04
        x06,         // 0x06 ?? component?
        REFDES,      // 0x07
        PIN_NUMBER,     // 0x08
        x0e,            // 0x0E
        FUNCTION_SLOT,  // 0x0F
        FUNCTION_INST,  // 0x10
        PIN_NAME,       // 0x11
        GRAPHIC_SEG,    // 0x14
        LINE,           // 0x15, 0x16, 0x17
        NET,            // 0x1B
        SHAPE,          // 0x28
        FP_DEF,         // 0x2B
        FP_INST,        // 0x2D
        x2e,            // 0x2E
        PLACED_PAD,     // 0x32
        VIA,            // 0x33
        KEEPOUT,        // 0x34
        x35,
        x36,
        x37,
        FILM_LAYER_LIST, // 0x39
        FILM,            // 0x3a
        x3b,
        x3c,
};

    // Where this block was in the file (for debugging)
    struct FILE_LOC
    {
        size_t  m_Offset;
        uint8_t m_BlockType;
    };

    DB_OBJ( TYPE aType, uint32_t aKey ) :
            m_Valid( false ),
            m_Type( aType ),
            m_Key( aKey ),
            m_Loc( 0, 0 )
    {
    }

    DB_OBJ() :
            DB_OBJ( TYPE::ARC, 0 )
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

    TYPE GetType() const { return m_Type; }

    uint32_t GetKey() const { return m_Key; }

    // Set to true when the object is fully resolved and valid
    bool     m_Valid;
    // The type of this object
    TYPE     m_Type;
    // The unique key of this object in the DB
    uint32_t m_Key;
    // Location in the file (for debugging)
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
     *
     * This is done once per DB after loading. We can just resolve everything
     * on-demand (with or without caching), but resolving upfront means we can detect
     * bad references earlier which is useful when the DB data is not fully known.
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


struct BRD_DB;
struct FOOTPRINT_INSTANCE;
struct FUNCTION_INSTANCE;
struct PLACED_PAD;
struct REFDES;

/**
 * 0x01 ARC objects
 */
struct ARC : public DB_OBJ
{
    ARC( const BLK_0x01_ARC& aBlk );

    DB_REF m_Parent;

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;
};


/**
 * 0x03 subtype 0x68
 */
struct x03_TEXT : public DB_OBJ
{
    x03_TEXT( const BLK_0x03& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override { return true; }

    wxString m_TextStr;
};


/**
 * 0x04 NET_ASSIGN objects
 */
struct NET_ASSIGN : public DB_OBJ
{
    NET_ASSIGN( const BRD_DB& aBrd, const BLK_0x04_NET_ASSIGNMENT& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
    ///< Reference to an 0x1B NET object
    DB_REF m_Net;
    ///< Reference to an 0x05 TRACK or 0x32 PLACED_PAD object
    DB_REF m_ConnItem;
};


/**
 * 0x05 TRACK
 */
struct TRACK : public DB_OBJ
{
    TRACK( const BRD_DB& aBrd, const BLK_0x05_TRACK& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
};


/**
 * 0x06 objects.
 */
struct x06_OBJECT : public DB_OBJ
{
    x06_OBJECT( const BRD_DB& aBrd, const BLK_0x06& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
    DB_STR_REF m_CompDeviceType;
    DB_STR_REF m_SymbolName;
    DB_REF m_PtrRefDes;
    DB_REF m_PtrFunctionSlot;
    DB_REF m_PtrPinNumber;
    DB_REF m_Fields;

    const REFDES& GetRefDes() const;
    const wxString* GetComponentDeviceType() const;
};


/**
 * 0x07 objects
 */
struct REFDES : public DB_OBJ
{
    REFDES( const BLK_0x07& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF     m_Next;
    DB_STR_REF m_TextStr;
    DB_REF     m_FunctionInst;
    DB_REF     m_X03Chain;
    DB_REF     m_FirstPad;

    x06_OBJECT* m_ParentComponent = nullptr;

    const x06_OBJECT* GetParentComponent() const { return m_ParentComponent; }

    const wxString* GetRefDesStr() const;
    const REFDES* GetNextRefDes() const;
    const FUNCTION_INSTANCE& GetFunctionInstance() const;
    const PLACED_PAD& GetFirstPad() const;
};


/**
 * 0x08 objects.
 */
struct PIN_NUMBER : public DB_OBJ
{
    PIN_NUMBER( const BLK_0x08& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_STR_REF m_PinNumberStr;
    DB_REF     m_PinName;
    DB_REF     m_Next;
};


/**
 * 0x0E objects: ??
 */
struct X0E : public DB_OBJ
{
    X0E( const BRD_DB& aBrd, const BLK_0x0E& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
};


/**
 * A FUNCTION_SLOT (0x0F) object represents a single function slot within a symbol.
 */
struct FUNCTION_SLOT : public DB_OBJ
{
    FUNCTION_SLOT( const BLK_0x0F& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_STR_REF m_SlotName;
    wxString m_CompDeviceType;

    DB_REF m_NextSlot;
    DB_REF m_Ptr0x06;
    DB_REF m_Ptr0x11;

    const wxString* GetName() const;
};


/**
 * A FUNCTION (0x10) object represents a logical function, which is an
 * _instance_ of a single function slot within a symbol.
 */
struct FUNCTION_INSTANCE : public DB_OBJ
{
    FUNCTION_INSTANCE( const BLK_0x10& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Slot;
    DB_REF m_Fields;
    DB_STR_REF m_FunctionName;
    DB_REF m_RefDes;

    const wxString* GetName() const;
    const REFDES& GetRefDes() const;
    const FUNCTION_SLOT& GetFunctionSlot() const;
};


/**
 * 0x11 objects.
 */
struct PIN_NAME: public DB_OBJ
{
    PIN_NAME( const BLK_0x11& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF     m_Next;
    DB_STR_REF m_PinNameStr;
    DB_REF     m_PinNumber;
};


/**
 * 0x14 objects (a line or arc graphic segment)
 */
struct GRAPHIC_SEG: public DB_OBJ
{
    GRAPHIC_SEG( const BRD_DB& aBrd, const BLK_0x14& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
    DB_REF m_Parent;
    DB_REF m_Segment; // ARC or LINE

    // 0x03?
    // DB_REF m_Ptr0x03;
    // 0x26?
};


/**
 * LINE objects (0x15, 0x16, 0x17)
 */
struct LINE : public DB_OBJ
{
    LINE( const BLK_0x15_16_17_SEGMENT& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Parent;
    DB_REF m_Next;

    VECTOR2I m_Start;
    VECTOR2I m_End;
    int m_Width;
};


/**
 * 0x1B NET objects
 */
struct NET : public DB_OBJ
{
    NET( const BRD_DB& aBrd, const BLK_0x1B_NET& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
    DB_STR_REF m_NetNameStr;
};


/**
 * 0x28 SHAPE objects
 */
class SHAPE : public DB_OBJ
{
public:
    SHAPE( const BRD_DB& aBrd, const BLK_0x28_SHAPE& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
    DB_REF m_Parent;
    DB_REF m_Segments;
};


/**
 * 0x2B objects
 */
struct FOOTPRINT_DEF : public DB_OBJ
{
    FOOTPRINT_DEF( const BRD_DB& aBrd, const BLK_0x2B& aBlk );

    DB_REF     m_Next;
    DB_STR_REF m_FpStr;
    DB_REF     m_SymLibPath;

    DB_REF_CHAIN m_Instances;

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    /**
     * Get the library path for this footprint definition
     *
     * For example: C:/OrCAD/OrCAD_16.6_Lite/share/pcb/pcb_lib/symbols/res2012x50n_0805.psm
     *
     * This can be empty, for example for DRAFTING type footprints like dimensions.
     */
    const wxString* GetLibPath() const;
};


/**
 * 0x2D objects
 */
struct FOOTPRINT_INSTANCE : public DB_OBJ
{
    FOOTPRINT_INSTANCE( const BLK_0x2D& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
    DB_REF m_RefDes;
    double m_X;
    double m_Y;
    double m_Rotation;
    bool   m_Mirrored;

    // Backlink to the parent footprint definition
    FOOTPRINT_DEF* m_Parent;

    const REFDES*   GetRefDes() const;
    const wxString* GetName() const;
};


/**
 * 0x2E objects.
 */
struct X2E : public DB_OBJ
{
    X2E( const BRD_DB& aBrd, const BLK_0x2E& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
    DB_REF m_NetAssign;
    DB_REF m_Connection;
    VECTOR2I m_Position;

};


/**
 * 0x32 Placed Pad objects.
 */
struct PLACED_PAD : public DB_OBJ
{
    PLACED_PAD( const BRD_DB& aBrd, const BLK_0x32_PLACED_PAD& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
    // DB_REF m_Ratline; // 0x23;
    uint32_t m_Flags;
    BOX2I m_Bounds;
};


/**
 * 0x33 VIA objects.
 */
struct VIA: public DB_OBJ
{
    VIA( const BRD_DB& aBrd, const BLK_0x33_VIA& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
    DB_REF m_NetAssign;
    BOX2I m_Bounds;
};


/**
 * When processing a view, some objects are available and some are not.
 *
 * Every item in a view will produce one of these, which will contain the
 * relevant objects for that row.
 */
struct VIEW_OBJS
{
    VIEW_OBJS() :
            m_Board( nullptr ),
            m_Component( nullptr ),
            m_Function( nullptr ),
            m_FootprintInstance( nullptr )
    {
    }

    // All views
    const BRD_DB* m_Board;
    // COMPONENT, COMPONENT_PIN, SYMBOL, FUNCTION
    const x06_OBJECT* m_Component;
    // FUNCTION
    const FUNCTION_INSTANCE* m_Function;
    //
    const FOOTPRINT_INSTANCE* m_FootprintInstance;
};


using VIEW_OBJS_VISITOR = std::function<void( const VIEW_OBJS& aViewObjs )>;


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

    using FP_DEF_VISITOR = std::function<void( const FOOTPRINT_DEF& aFpDef )>;

    /**
     * Access the footprint defs in the database.
     *
     * This iterates the 0x2B linked list.
     */
    void VisitFootprintDefs( FP_DEF_VISITOR aFpDef ) const;

    /**
     * Access the footprint instances in the database.
     *
     * This iterates the 0x2D linked list for a given footprint def.
     */
    void VisitFootprintInstances( const FOOTPRINT_DEF& aFpDef, VIEW_OBJS_VISITOR aVisitor ) const;

    /**
     * Access the function instances in the database.
     *
     * This iterates the 0x06 linked list and finds the functions.
     */
    void VisitFunctionInstances( VIEW_OBJS_VISITOR aVisitor ) const;

    // It's not fully clear how much of the header is brd specific or is a more general
    // DB format (or is there is a more general format). Clearly much of it (linked lists,
    // for example) is very board-related.
    // For now, keep it up here, but generalities can push down to DB.
    FMT_VER                      m_FmtVer;
    std::unique_ptr<FILE_HEADER> m_Header;

private:
    /**
     * Convert a block of "raw" binary-ish data into a DB_OBJ of the appropriate type to be
     * stored in the DB.
     *
     * As constructed, the objects may have dangling references to other object that will
     * need to be resolved only after all objects are inserted into the DB.
     */
    std::unique_ptr<DB_OBJ> createObject( const BLOCK_BASE& aBlock );
};

} // namespace ALLEGRO
