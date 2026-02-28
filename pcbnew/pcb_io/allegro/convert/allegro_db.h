/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
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
#include <unordered_set>

#include <math/vector2d.h>
#include <math/box2.h>

#include <convert/allegro_pcb_structs.h>


namespace ALLEGRO
{

class BLOCK_BASE;
struct DB_OBJ;
class DB_OBJ_RESOLVER;


struct RESOLVABLE
{
    RESOLVABLE( const DB_OBJ* aParent, const char* aDebugName = nullptr ) :
            m_Parent( aParent ),
            m_DebugName( aDebugName )
    {
    }

    virtual bool Resolve( const DB_OBJ_RESOLVER& aResolver ) = 0;

    virtual std::string DebugString() const;

    const DB_OBJ* m_Parent;
    const char*   m_DebugName;
};


struct DB_REF : public RESOLVABLE
{
    DB_REF( const DB_OBJ* aParent, uint32_t aTargetKey, const char* aDebugName ) :
            RESOLVABLE( aParent, aDebugName ),
            m_TargetKey( aTargetKey ),
            m_Target( nullptr )
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

static DB_REF DB_NULLREF{ nullptr, 0, "<NULL>" };

/**
 * Chain of DB references that lead from the head to the tail
 */
struct DB_REF_CHAIN : public RESOLVABLE
{
    DB_REF_CHAIN( const DB_OBJ* aParent, uint32_t aHead, uint32_t aTail, const char* aDebugName ) :
            RESOLVABLE( aParent ),
            m_Head( aHead ),
            m_Tail( aTail )
    {
        m_DebugName = aDebugName;
    }

    DB_REF_CHAIN( const DB_OBJ* aParent ) :
            DB_REF_CHAIN( aParent, 0, 0, "<unknown>" )
    {
    }

    bool Resolve( const DB_OBJ_RESOLVER& aResolver ) override;

    /**
     * Visit all objects in the chain
     */
    void Visit( std::function<void( const DB_OBJ& aObj )> aVisitor ) const;
    void Visit( std::function<void( DB_OBJ& aObj )> aVisitor );

    std::function<const DB_REF&( const DB_OBJ& )> m_NextRefGetter;

    // The head/tail key values
    uint32_t                                 m_Head;
    uint32_t                                 m_Tail;

    // The objects in the chain
    std::vector<DB_OBJ*> m_Chain;
};


struct DB_STR_REF : public RESOLVABLE
{
    DB_STR_REF( const DB_OBJ* aParent, uint32_t aTargetKey, const char* aDebugName ) :
            RESOLVABLE( aParent, aDebugName ),
            m_StringKey( aTargetKey ),
            m_String( nullptr )
    {
    }

    bool Resolve( const DB_OBJ_RESOLVER& aResolver ) override;

    uint32_t        m_StringKey;
    const wxString* m_String;
};

static const DB_STR_REF DB_STRNULLREF{ nullptr, 0, "<NULL>" };

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

    /**
     * In v18+ files, linked list sentinel nodes are real blocks with small keys.
     * References pointing to these sentinels are logically null (end-of-chain).
     */
    virtual bool IsSentinel( uint32_t aKey ) const { return false; }
};


/**
 * A DB_OBJ represents one object in an Allegro database.
 */
struct DB_OBJ
{
    using TYPE_ID = uint8_t;

    // Where this block was in the file (for debugging)
    struct FILE_LOC
    {
        size_t  m_Offset;
        uint8_t m_BlockType;
    };

    DB_OBJ( uint32_t aKey ) :
            m_Valid( false ),
            m_Key( aKey ),
            m_Loc( 0, 0 )
    {
    }

    DB_OBJ() :
            DB_OBJ( 0 )
    {
    }

    virtual ~DB_OBJ() {}

    /**
     * All blocks are denoted by a type which allows dispatch to the appropriate subclass.
     */
    virtual TYPE_ID GetType() const = 0;

    /*
     * A type name for debugging/tracing purposes.
     */
    virtual const char* TypeName() const
    {
        return "<UNNAMED>";
    }

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

    uint32_t GetKey() const { return m_Key; }

    /**
     * Return the reference to the next object in the default chain for this object.
     *
     * Some methods of iterating use other "nexts" in the object, fo those you should use
     * the appropriate field instead of this method.
     */
    virtual const DB_REF& GetNext() const
    {
        return DB_NULLREF;
    }

    // Set to true when the object is fully resolved and valid
    bool m_Valid;
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

    bool IsSentinel( uint32_t aKey ) const override
    {
        return m_SentinelKeys.count( aKey ) > 0;
    }

    void AddSentinelKey( uint32_t aKey )
    {
        if( aKey != 0 )
            m_SentinelKeys.insert( aKey );
    }

    virtual void InsertBlock( std::unique_ptr<BLOCK_BASE> aBlock ) = 0;

protected:
    void visitLinkedList( const FILE_HEADER::LINKED_LIST                     aLList,
                          std::function<const DB_REF&( const DB_OBJ& aObj )> aVisitor ) const;

    void reserveObjects( size_t aCount ) { m_Objects.reserve( aCount ); }

    /**
     * Resolve all DB_OBJ references without throwing on failure.
     * Objects that can't fully resolve are marked invalid but processing continues.
     */
    void ResolveObjectLinksBestEffort();

private:
    // Main store of DB objects.
    std::unordered_map<uint32_t, std::unique_ptr<DB_OBJ>> m_Objects;

    // V18+ linked list sentinel keys. References to these are null (end-of-chain).
    std::unordered_set<uint32_t> m_SentinelKeys;

public:
    std::unordered_map<uint32_t, wxString> m_StringTable;
};


enum BRD_TYPE
{
    BRD_ARC,            // 0x01
    BRD_FIELD,          // 0x03 subtype 0x68...
    BRD_TRACK,          // 0x05
    BRD_NET_ASSIGN,     // 0x04
    BRD_COMPONENT,      // 0x06
    BRD_COMPONENT_INST, // 0x07
    BRD_PIN_NUMBER,     // 0x08
    BRD_x0e_RECT,       // 0x0E
    BRD_FUNCTION_SLOT,  // 0x0F
    BRD_FUNCTION_INST,  // 0x10
    BRD_PIN_NAME,       // 0x11
    BRD_XREF,           // 0x12
    BRD_GRAPHIC_SEG,    // 0x14
    BRD_LINE,           // 0x15, 0x16, 0x17
    BRD_NET,            // 0x1B
    BRD_x20,            // 0x20
    BRD_SHAPE,          // 0x28
    BRD_FP_DEF,         // 0x2B
    BRD_FP_INST,        // 0x2D
    BRD_CONNECTION,     // 0x2E
    BRD_PLACED_PAD,     // 0x32
    BRD_VIA,            // 0x33
    BRD_KEEPOUT,        // 0x34
    BRD_x35,
    BRD_x36,
    BRD_PTR_ARRAY,       // 0x37
    BRD_FILM_LAYER_LIST, // 0x39
    BRD_FILM,            // 0x3a
    BRD_x3b,
    BRD_x3c,
};

struct BRD_DB_OBJ : public DB_OBJ
{
    BRD_DB_OBJ( BRD_TYPE aType, uint32_t aKey ) :
            DB_OBJ( aKey ),
            m_Type( aType )
    {
    }

    BRD_DB_OBJ() :
            DB_OBJ()
    {
    }

    TYPE_ID GetType() const override
    {
        return static_cast<TYPE_ID>( m_Type );
    }

    BRD_TYPE GetBrdType() const { return m_Type; }

private:
    BRD_TYPE m_Type;
};


class BRD_DB;
struct COMPONENT;
struct COMPONENT_INST;
struct FOOTPRINT_INSTANCE;
struct FUNCTION_INSTANCE;
struct NET;
struct NET_ASSIGN;
struct PLACED_PAD;
struct PIN_NAME;
struct PIN_NUMBER;
struct TRACK;


/**
 * 0x01 ARC objects
 */
struct ARC : public BRD_DB_OBJ
{
    ARC( const BLK_0x01_ARC& aBlk );

    DB_REF m_Parent;

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "ARC"; }
};


/**
 * 0x03 FIELD objects
 *
 * These can be of several subtypes
 */
struct FIELD : public BRD_DB_OBJ
{
    FIELD( const BLK_0x03_FIELD& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override { return true; }

    const char* TypeName() const override { return "FIELD"; }

    uint8_t m_SubType;
    DB_REF  m_Next;

    // Unclear if just hdr1 or both are needed for a complete field type determination
    uint32_t m_Hdr1;
    uint32_t m_Hdr2;

    std::variant<wxString, uint32_t> m_FieldValue;

    // Expects that the field contains a string and returns it
    const wxString& ExpectString() const;
};


/**
 * A field list is a linked list of 0x03. This class adds accessors for
 * picking out specific fields by subtype/code
 */
class FIELD_LIST
{
public:
    FIELD_LIST( DB_REF_CHAIN& aChain ) :
            m_Chain( aChain )
    {
    }

    /**
     * Get the integer value of the field with the given code, if in the list.
     *
     * If found, it is expected to be an integer field.
     */
    std::optional<int> GetOptFieldExpectInt( uint16_t aFieldCode ) const;

    const wxString* GetOptFieldExpectString( uint16_t aFieldCode ) const;

    /**
     * Get the raw variant value of the field with the given code, if present.
     * Returns nullopt if the field is not in the list.
     */
    std::optional<std::variant<wxString, uint32_t>> GetOptField( uint16_t aFieldCode ) const;

private:
    DB_REF_CHAIN& m_Chain;
};


/**
 * 0x04 NET_ASSIGN objects
 */
struct NET_ASSIGN : public BRD_DB_OBJ
{
    NET_ASSIGN( const BRD_DB& aBrd, const BLK_0x04_NET_ASSIGNMENT& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "NET_ASSIGN"; }

    DB_REF m_Next;
    ///< Reference to an 0x1B NET object
    DB_REF m_Net;
    ///< Reference to an 0x05 TRACK or 0x32 PLACED_PAD object
    DB_REF m_ConnItem;

    const ALLEGRO::NET& GetNet() const;
};


/**
 * 0x05 TRACK
 */
struct TRACK : public BRD_DB_OBJ
{
    TRACK( const BRD_DB& aBrd, const BLK_0x05_TRACK& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    DB_REF m_Next;
};

struct COMPONENT_INST;

/**
 * COMPONENT 0x06 objects.
 *
 * One per component definition (symbol) in the library.
 */
struct COMPONENT : public BRD_DB_OBJ
{
    COMPONENT( const BRD_DB& aBrd, const BLK_0x06_COMPONENT& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "COMPONENT"; }

    DB_REF m_Next;
    DB_STR_REF m_CompDeviceType;
    DB_STR_REF m_SymbolName;
    DB_REF_CHAIN m_Instances;
    DB_REF m_PtrFunctionSlot;
    DB_REF m_PtrPinNumber;
    DB_REF m_Fields;

    const COMPONENT_INST& GetComponentInstance() const;
    const wxString* GetComponentDeviceType() const;
};


/**
 * COMPONENT_INST 0x07 objects
 *
 * These represent instances of COMPONENTs placed on the board.
 */
struct COMPONENT_INST : public BRD_DB_OBJ
{
    COMPONENT_INST( const BLK_0x07_COMPONENT_INST& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "COMPONENT_INST"; }

    DB_REF     m_Next;
    DB_STR_REF   m_TextStr;
    DB_REF       m_FunctionInst;
    DB_REF_CHAIN m_X03Chain;
    DB_REF_CHAIN m_Pads;

    COMPONENT* m_ParentComponent = nullptr;

    const COMPONENT* GetParentComponent() const { return m_ParentComponent; }

    const wxString*          GetRefDesStr() const;
    const COMPONENT_INST*    GetNextInstance() const;
    const FUNCTION_INSTANCE& GetFunctionInstance() const;
    const PLACED_PAD&        GetFirstPad() const;
};


/**
 * 0x08 objects.
 */
struct PIN_NUMBER : public BRD_DB_OBJ
{
    PIN_NUMBER( const BLK_0x08_PIN_NUMBER& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "PIN_NUMBER"; }

    DB_REF     m_Next;
    DB_STR_REF m_PinNumberStr;
    DB_REF     m_PinName;

    const wxString* GetNumber() const;
    const PIN_NAME* GetPinName() const;
};


/**
 * 0x0E objects: ??
 */
struct RECT_OBJ : public BRD_DB_OBJ
{
    RECT_OBJ( const BRD_DB& aBrd, const BLK_0x0E_RECT& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "RECT_OBJ"; }

    DB_REF m_Next;
    EDA_ANGLE m_Rotation;
};


/**
 * A FUNCTION_SLOT (0x0F) object represents a single function slot within a symbol.
 */
struct FUNCTION_SLOT : public BRD_DB_OBJ
{
    FUNCTION_SLOT( const BLK_0x0F_FUNCTION_SLOT& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "FUNCTION_SLOT"; }

    DB_STR_REF m_SlotName;
    wxString m_CompDeviceType;

    DB_REF m_Component;
    DB_REF m_PinName;

    const wxString* GetName() const;
};


/**
 * A FUNCTION (0x10) object represents a logical function, which is an
 * _instance_ of a single function slot within a symbol.
 */
struct FUNCTION_INSTANCE : public BRD_DB_OBJ
{
    FUNCTION_INSTANCE( const BLK_0x10_FUNCTION_INST& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "FUNCTION_INSTANCE"; }

    DB_REF     m_Slot;
    DB_REF     m_Fields;
    DB_STR_REF m_FunctionName;
    DB_REF     m_ComponentInstance;

    const wxString*       GetName() const;
    const COMPONENT_INST& GetComponentInstance() const;
    const FUNCTION_SLOT&  GetFunctionSlot() const;
};


/**
 * 0x11 objects.
 */
struct PIN_NAME : public BRD_DB_OBJ
{
    PIN_NAME( const BLK_0x11_PIN_NAME& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "PIN_NAME"; }

    DB_REF     m_Next;
    DB_STR_REF m_PinNameStr;
    DB_REF     m_PinNumber;

    const wxString*   GetName() const;
    const PIN_NUMBER* GetPinNumber() const;
};


/**
 * 0x12 objects.
 */
struct XREF_OBJ : public BRD_DB_OBJ
{
    XREF_OBJ( const BLK_0x12_XREF& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "XREF_OBJ"; }

    DB_REF m_Ptr1;
    DB_REF m_Ptr2;
    DB_REF m_Ptr3;
};


/**
 * 0x14 objects (a line or arc graphic segment)
 */
struct GRAPHIC_SEG : public BRD_DB_OBJ
{
    GRAPHIC_SEG( const BRD_DB& aBrd, const BLK_0x14_GRAPHIC& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "GRAPHIC_SEG"; }

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
struct LINE : public BRD_DB_OBJ
{
    LINE( const BLK_0x15_16_17_SEGMENT& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "LINE"; }

    DB_REF m_Parent;
    DB_REF m_Next;

    VECTOR2I m_Start;
    VECTOR2I m_End;
    int m_Width;
};


/**
 * 0x1B NET objects
 */
struct NET : public BRD_DB_OBJ
{
    enum class STATUS
    {
        REGULAR,
        SCHEDULED,
        NO_RAT,
    };

    NET( const BRD_DB& aBrd, const BLK_0x1B_NET& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "NET"; }

    DB_REF m_Next;
    DB_STR_REF m_NetNameStr;
    // Not clear if this is ever not 1 entry, but 0x04s have a next field
    DB_REF_CHAIN m_NetAssignments;

    DB_REF_CHAIN m_FieldsChain;
    FIELD_LIST   m_Fields; // wrapper

    STATUS m_Status;

    const wxString* GetName() const;
    STATUS          GetStatus() const;

    const wxString* GetLogicalPath() const;

    std::optional<int> GetNetMinLineWidth() const;
    std::optional<int> GetNetMaxLineWidth() const;
    std::optional<int> GetNetMinNeckWidth() const;
    std::optional<int> GetNetMaxNeckLength() const;
};


/**
 * 0x20 objects. Purpose unknown.
 */
class UNKNOWN_0x20 : public BRD_DB_OBJ
{
public:
    UNKNOWN_0x20( const BRD_DB& aBrd, const BLK_0x20_UNKNOWN& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "UNKNOWN_0x20"; }

    DB_REF m_Next;
};


/**
 * 0x28 SHAPE objects
 */
class SHAPE : public BRD_DB_OBJ
{
public:
    SHAPE( const BRD_DB& aBrd, const BLK_0x28_SHAPE& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "SHAPE"; }

    DB_REF m_Next;
    DB_REF_CHAIN m_Segments;
};


/**
 * 0x2B objects
 */
struct FOOTPRINT_DEF : public BRD_DB_OBJ
{
    FOOTPRINT_DEF( const BRD_DB& aBrd, const BLK_0x2B_FOOTPRINT_DEF& aBlk );

    DB_REF     m_Next;
    DB_STR_REF m_FpStr;
    DB_REF     m_SymLibPath;

    DB_REF_CHAIN m_Instances;

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "FOOTPRINT_DEF"; }

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
struct FOOTPRINT_INSTANCE : public BRD_DB_OBJ
{
    FOOTPRINT_INSTANCE( const BLK_0x2D_FOOTPRINT_INST& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "FOOTPRINT_INSTANCE"; }

    DB_REF m_Next;
    DB_REF m_ComponentInstance;
    double m_X;
    double m_Y;
    double m_Rotation;
    bool   m_Mirrored;

    // Chain of PLACED_PADs
    DB_REF_CHAIN m_Pads;

    // Chain of graphic segments (SEGMENT, ARC)
    DB_REF_CHAIN m_Graphics;

    // Backlink to the parent footprint definition
    FOOTPRINT_DEF* m_Parent;

    const COMPONENT_INST* GetComponentInstance() const;
    const wxString*       GetName() const;
};


/**
 * 0x2E objects.
 */
struct CONNECTION_OBJ : public BRD_DB_OBJ
{
    CONNECTION_OBJ( const BRD_DB& aBrd, const BLK_0x2E_CONNECTION& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "CONNECTION_OBJ"; }

    DB_REF m_Next;
    DB_REF m_NetAssign;
    DB_REF m_Connection;
    VECTOR2I m_Position;

};


/**
 * 0x32 Placed Pad objects.
 */
struct PLACED_PAD : public BRD_DB_OBJ
{
    PLACED_PAD( const BRD_DB& aBrd, const BLK_0x32_PLACED_PAD& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "PLACED_PAD"; }

    DB_REF m_Next;
    DB_REF m_NextInFp;
    DB_REF m_NextInCompInst;
    // DB_REF m_Ratline; // 0x23;
    DB_REF   m_NetAssign;
    DB_REF   m_PinNumber;
    DB_REF   m_PinNumText;
    uint32_t m_Flags;
    BOX2I    m_Bounds;

    const wxString* GetPinName() const;
    const wxString* GetPinNumber() const;

    const NET* GetNet() const;
};


/**
 * 0x33 VIA objects.
 */
struct VIA : public BRD_DB_OBJ
{
    VIA( const BRD_DB& aBrd, const BLK_0x33_VIA& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "VIA"; }

    DB_REF m_Next;
    DB_REF m_NetAssign;
    BOX2I m_Bounds;
};


/**
 * 0x37 PTR_ARRAY objects.
 */
struct PTR_ARRAY : public BRD_DB_OBJ
{
    PTR_ARRAY( const BRD_DB& aBrd, const BLK_0x37_PTR_ARRAY& aBlk );

    bool ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) override;

    const char* TypeName() const override { return "PTR_ARRAY"; }

    const DB_REF& GetNext() const override { return m_Next; }

    DB_REF m_Next;
    DB_REF m_Parent;

    std::vector<DB_REF> m_Ptrs;
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
    const COMPONENT* m_Component;

    const COMPONENT_INST* m_ComponentInstance;
    // FUNCTION
    const FUNCTION_INSTANCE* m_Function;
    //
    const FOOTPRINT_INSTANCE* m_FootprintInstance;

    // COMPONENT_PIN, LOGICAL_PIN have these
    const PLACED_PAD* m_Pad;

    const NET* m_Net;
};


using VIEW_OBJS_VISITOR = std::function<void( const VIEW_OBJS& aViewObjs )>;


/**
 * An Allegro database that represents a .brd file (amd presumably .dra)
 */
class BRD_DB : public DB
{
public:
    /**
     * Converts blocks of "raw" binary-ish data into a DB_OBJ of the appropriate type to be
     * stored in the DB.
     *
     * As constructed, the objects may have dangling references to other object that will
     * need to be resolved only after all objects are inserted into the DB.
     *
     * This class encapsulates context/state for that process.
     */
    class OBJ_FACTORY
    {
    public:
        OBJ_FACTORY( const BRD_DB& aBrdDb ) :
                m_brdDb( aBrdDb )
        {
        }

        std::unique_ptr<DB_OBJ> CreateObject( const BLOCK_BASE& aBlock ) const;

    private:
        const BRD_DB& m_brdDb;
    };

    BRD_DB();

    void InsertBlock( std::unique_ptr<BLOCK_BASE> aBlock ) override;

    /**
     * Pre-allocate storage for the expected number of objects and strings.
     * Avoids incremental rehashing as elements are inserted.
     */
    void ReserveCapacity( size_t aObjectCount, size_t aStringCount );

    /**
     * When true, InsertBlock skips DB_OBJ creation for high-volume block types
     * (segments, graphics, arcs) that the BOARD_BUILDER accesses only through
     * raw BLOCK_BASE. Types needed by VisitNets (NET, FIELD, etc.) still get
     * full DB_OBJ resolution.
     */
    void SetLeanMode( bool aLean ) { m_leanMode = aLean; }

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
    void VisitFootprintInstances( VIEW_OBJS_VISITOR aVisitor ) const;
    void visitFootprintInstances( const FOOTPRINT_DEF& aFpDef, VIEW_OBJS_VISITOR aVisitor ) const;

    /**
     * Access the function instances in the database.
     *
     * This iterates the 0x06 linked list and finds the functions.
     *
     * If the function is assigned to a component, the component is set.
     * If the component is placed, the symbol is also set.
     */
    void VisitFunctionInstances( VIEW_OBJS_VISITOR aVisitor ) const;

    void VisitComponents( VIEW_OBJS_VISITOR aVisitor ) const;

    /**
     * Visit all component pins in the database.
     */
    void VisitComponentPins( VIEW_OBJS_VISITOR aVisitor ) const;

    void VisitNets( VIEW_OBJS_VISITOR aVisitor ) const;

    void VisitConnectedGeometry( VIEW_OBJS_VISITOR aVisitor ) const;

    /**
     * Get a raw block by its key (for compatibility with BOARD_BUILDER).
     */
    const BLOCK_BASE* GetObjectByKey( uint32_t aKey ) const
    {
        auto it = m_ObjectKeyMap.find( aKey );
        return it != m_ObjectKeyMap.end() ? it->second : nullptr;
    }

    /**
     * Get a string from the string table by key.
     */
    const wxString& GetString( uint32_t aKey ) const
    {
        static const wxString empty;
        auto it = m_StringTable.find( aKey );
        return it != m_StringTable.end() ? it->second : empty;
    }

    // It's not fully clear how much of the header is brd specific or is a more general
    // DB format (or is there is a more general format). Clearly much of it (linked lists,
    // for example) is very board-related.
    // For now, keep it up here, but generalities can push down to DB.
    FMT_VER                      m_FmtVer;
    std::unique_ptr<FILE_HEADER> m_Header;

    // Raw block storage for backward compatibility with BOARD_BUILDER
    std::vector<std::unique_ptr<BLOCK_BASE>> m_Blocks;
    std::unordered_map<uint32_t, BLOCK_BASE*> m_ObjectKeyMap;

private:
    // The factory that will turn BLOCKs into DB_OBJs
    OBJ_FACTORY m_ObjFactory;

    bool m_leanMode;
};

} // namespace ALLEGRO
