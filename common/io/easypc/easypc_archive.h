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

/**
 * @file easypc_archive.h
 * @brief Reader for the serialized streams of Easy-PC and DesignSpark PCB.
 *
 * The format has no lengths: classes and objects share one load array that back references index, so every byte
 * must be consumed in stream order.  Loaders are coroutines run from an explicit frame stack, because
 * the cyclic net, node and pad graph of a real design nests objects hundreds deep at their first reference.
 */

#ifndef EASYPC_ARCHIVE_H
#define EASYPC_ARCHIVE_H

#include <coroutine>
#include <cstdint>
#include <exception>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <wx/string.h>


namespace EASYPC
{

class ARCHIVE;


/// A loader coroutine.  Awaiting one queues it on its archive's frame stack instead of nesting the call.
class [[nodiscard]] LOAD_TASK
{
public:
    struct promise_type
    {
        // The archive is found among the coroutine's parameters so the task can reach its frame stack
        template <typename... ARGS>
        promise_type( ARGS&... aArgs )
        {
            ( find( aArgs ), ... );
        }

        LOAD_TASK get_return_object()
        {
            return LOAD_TASK( std::coroutine_handle<promise_type>::from_promise( *this ) );
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void                return_void() noexcept {}
        void                unhandled_exception() noexcept { m_error = std::current_exception(); }

        void find( ARCHIVE& aArchive ) { m_archive = &aArchive; }

        template <typename T>
        void find( T& )
        {
        }

        ARCHIVE*           m_archive = nullptr;
        std::exception_ptr m_error;
    };

    using HANDLE = std::coroutine_handle<promise_type>;

    LOAD_TASK( LOAD_TASK&& aOther ) noexcept :
            m_handle( std::exchange( aOther.m_handle, {} ) )
    {
    }

    LOAD_TASK& operator=( LOAD_TASK&& ) = delete;

    ~LOAD_TASK()
    {
        if( m_handle )
            m_handle.destroy();
    }

    HANDLE Release() { return std::exchange( m_handle, {} ); }

    bool await_ready() const noexcept { return false; }
    void await_suspend( std::coroutine_handle<> aAwaiting );
    void await_resume() const noexcept {}

private:
    explicit LOAD_TASK( HANDLE aHandle ) :
            m_handle( aHandle )
    {
    }

    HANDLE m_handle;
};


/// Root of every deserialized object
struct OBJECT
{
    virtual ~OBJECT() = default;

    /// Read this object's fields
    virtual LOAD_TASK Load( ARCHIVE& aAr );

    /// The version this object gates on; the base is the archive's current read format
    virtual int ItemFormat( const ARCHIVE& aAr ) const;

    const char* ClassName = "object"; ///< registered class name
    uint16_t    Schema = 0;           ///< object schema from the class tag, 0 for an embedded member
    size_t      Offset = 0;           ///< stream offset of the tag or of an embedded member's first field
};


using REGISTRY = std::map<std::string, std::function<std::unique_ptr<OBJECT>()>, std::less<>>;


/// Register the class names whose objects load as a T
template <typename T>
void Register( REGISTRY& aReg, std::initializer_list<const char*> aNames )
{
    for( const char* name : aNames )
        aReg[name] = []() -> std::unique_ptr<OBJECT>
        {
            return std::make_unique<T>();
        };
}


// One per class cluster, in that cluster's easypc_classes_<cluster>.cpp
void RegisterRootClasses( REGISTRY& aReg );
void RegisterStyleClasses( REGISTRY& aReg );
void RegisterGeometryClasses( REGISTRY& aReg );
void RegisterLibraryClasses( REGISTRY& aReg );
void RegisterConnectivityClasses( REGISTRY& aReg );


/// Document kind, from the FileType stream or the file extension for projects
enum class DOC_KIND
{
    PCB,
    SCHEMATIC,
    PCB_SYMBOL, ///< one item stream of a PCB Symbol Library
    SCH_SYMBOL, ///< one item stream of a Schematic Symbol Library
    COMPONENT,  ///< one stream of a Component Library storage
    PROJECT
};


/// Product keys, the design header's or the library LibFormat's
enum PRODUCT_KEY : int32_t
{
    PRODUCT_EASYPC = 1,
    PRODUCT_DESIGNSPARK = 0x943e3,
    PRODUCT_DESIGNSPARK_CREATOR = 0xcf153,
    PRODUCT_DESIGNSPARK_PRO = 0xe002e,
    PRODUCT_PROTOPCB = 0x311f3
};


/// One archive load stream; it owns every object it creates
class ARCHIVE
{
public:
    /// aData must outlive the archive; aProduct is the product key in force
    ARCHIVE( const uint8_t* aData, size_t aSize, DOC_KIND aKind, int32_t aProduct );

    // Every read is bounds checked and throws IO_ERROR past the end
    uint8_t  U8();
    bool     Bool() { return U8() != 0; }
    bool     InBool(); ///< one or four bytes, depending on the active format
    uint16_t U16();
    uint32_t U32();
    int32_t  I32() { return static_cast<int32_t>( U32() ); }
    uint64_t U64();
    double   F64();
    uint32_t Count();      ///< a 16-bit count, 0xFFFF escaping to 32 bits
    wxString ReadString(); ///< ANSI (windows-1252) or the 0xFF 0xFFFE UTF-16 form
    void     SkipString();
    void     Skip( size_t aLen );

    /// Throw unless aLen more bytes remain
    void Require( size_t aLen ) const;

    /// The next object tag for a loader to co_await: null, a back reference, or a new object fully loaded
    class OBJECT_AWAITER
    {
    public:
        explicit OBJECT_AWAITER( ARCHIVE& aArchive ) :
                m_archive( aArchive )
        {
        }

        bool    await_ready();
        void    await_suspend( std::coroutine_handle<> aAwaiting );
        OBJECT* await_resume() const { return m_object; }

    protected:
        ARCHIVE& m_archive;
        OBJECT*  m_object = nullptr;
    };

    OBJECT_AWAITER Object() { return OBJECT_AWAITER( *this ); }

    /// Object that must be a T or null
    template <typename T>
    class TYPED_AWAITER : public OBJECT_AWAITER
    {
    public:
        using OBJECT_AWAITER::OBJECT_AWAITER;

        T* await_resume() const
        {
            T* typed = dynamic_cast<T*>( m_object );

            if( m_object && !typed )
                m_archive.throwBadClass( *m_object );

            return typed;
        }
    };

    template <typename T>
    TYPED_AWAITER<T> ObjectAs()
    {
        return TYPED_AWAITER<T>( *this );
    }

    /// An embedded member for a loader to co_await: no tag and no load-array slot
    class EMBEDDED_AWAITER
    {
    public:
        EMBEDDED_AWAITER( ARCHIVE& aArchive, OBJECT& aObject ) :
                m_archive( aArchive ),
                m_object( aObject )
        {
        }

        bool await_ready() const { return false; }
        void await_suspend( std::coroutine_handle<> aAwaiting );
        void await_resume() const noexcept {}

    private:
        ARCHIVE& m_archive;
        OBJECT&  m_object;
    };

    EMBEDDED_AWAITER Embedded( OBJECT& aObject, const char* aClass )
    {
        aObject.ClassName = aClass;
        return EMBEDDED_AWAITER( *this, aObject );
    }

    /// ReadObject and LoadEmbedded outside any loader, running the nested loads to completion
    OBJECT* ReadObject();
    void    LoadEmbedded( OBJECT& aObject, const char* aClass );

    /// A load-array slot for an object not read through a tag
    void MapObject( OBJECT* aObject );

    /// An object of a registered class, not yet loaded
    static std::unique_ptr<OBJECT> Create( const char* aClass );

    // Stream state that persists across objects: boolean width, shape format and current item format
    void SetBooleanIsByte( bool aByte ) { m_boolIsByte = aByte; }
    int  ShapeFormat() const { return m_shapeFormat; }
    void SetShapeFormat( int aFormat ) { m_shapeFormat = aFormat; }
    int  CurrentReadFormat() const { return m_currentReadFormat; }
    void SetReadFormat( int aFormat ) { m_currentReadFormat = aFormat; }

    /// A null parent tag is the document: the header version in a design, else the current read format
    int OrphanFormat() const;

    int  DocumentVersion() const { return m_documentVersion; }
    void SetDocumentVersion( int aVersion ) { m_documentVersion = aVersion; }

    DOC_KIND Kind() const { return m_kind; }
    bool     IsDesign() const { return m_kind == DOC_KIND::PCB || m_kind == DOC_KIND::SCHEMATIC; }
    int32_t  Product() const { return m_product; }
    void     SetProduct( int32_t aProduct ) { m_product = aProduct; }

    /// Bounds ItemFormat parent walks, which recurse through file-supplied parent tags
    size_t& ParentWalkDepth() const { return m_parentWalkDepth; }
    size_t  LoadArraySize() const { return m_load.size(); }

    size_t   Pos() const { return m_pos; }
    size_t   Size() const { return m_size; }
    bool     AtEnd() const { return m_pos == m_size; }
    uint16_t ObjectSchema() const { return m_objectSchema; }

    std::vector<std::unique_ptr<OBJECT>> TakeObjects() { return std::move( m_objects ); }

    static constexpr int MAX_DEPTH = 100000;

private:
    friend class LOAD_TASK;

    [[noreturn]] void throwBadClass( const OBJECT& aObject ) const;

    /// Read a tag; a new object is created and mapped but not loaded (aIsNew)
    OBJECT* readTag( bool& aIsNew );

    void pushObjectLoad( OBJECT& aObject, uint16_t aSchema );
    void pushTask( LOAD_TASK::HANDLE aHandle );

    /// Run aRoot and everything it awaits until it finishes, rethrowing its failure
    void drive( LOAD_TASK aRoot );

    /// The string length prefix; aUnicode for the UTF-16 form
    uint32_t stringLength( bool& aUnicode );

    struct FRAME
    {
        LOAD_TASK::HANDLE Handle;
        OBJECT*           Object = nullptr; ///< whose Load this is, named in a failure
        uint16_t          SavedSchema = 0;
        bool              ObjectLoad = false; ///< counts towards MAX_DEPTH
    };

    struct SLOT
    {
        const char* ClassName = nullptr; ///< set for a class slot
        uint16_t    Schema = 0;
        OBJECT*     Object = nullptr;
    };

    const uint8_t* m_data;
    size_t         m_size;
    size_t         m_pos = 0;
    DOC_KIND       m_kind;
    int32_t        m_product;

    std::vector<SLOT>                    m_load; ///< slot 0 is the null slot
    std::vector<std::unique_ptr<OBJECT>> m_objects;
    std::vector<FRAME>                   m_frames;
    uint16_t                             m_objectSchema = 0;
    int                                  m_depth = 0;
    bool                                 m_errorHasContext = false;

    bool m_boolIsByte = true;
    int  m_shapeFormat = 3;
    int  m_currentReadFormat = -1;
    int  m_documentVersion = 0;

    mutable size_t m_parentWalkDepth = 0;
};


/**
 * Read and drop a run of fields described by aOps, each letter optionally followed by a repeat count: b
 * a format-dependent boolean, 1 a byte, i an int32, f a double, s a string, o an object, a a counted int32 array.
 * Spaces are ignored.
 */
LOAD_TASK SkipFields( ARCHIVE& aAr, std::string_view aOps );


/// A run of fields present from version since to until inclusive
struct FIELD_ROW
{
    /// Rejects an unknown op at compile time, since a misread op would misalign every later field
    consteval FIELD_ROW( int aSince, int aUntil, const char* aOps ) :
            Since( aSince ),
            Until( aUntil ),
            Ops( aOps )
    {
        for( const char* p = aOps; *p; ++p )
        {
            if( std::string_view( "b1ifsoa 0123456789" ).find( *p ) == std::string_view::npos )
                throw "unknown SkipFields op";
        }
    }

    int         Since;
    int         Until;
    const char* Ops;
};


/// SkipFields for every row aVersion falls in, in order
LOAD_TASK SkipRows( ARCHIVE& aAr, int aVersion, std::span<const FIELD_ROW> aRows );


/// A class no importer reads: its base's fields, then a field program on its format
template <typename BASE>
struct SKIPPED : BASE
{
    LOAD_TASK Load( ARCHIVE& aAr ) override
    {
        co_await BASE::Load( aAr );
        co_await SkipRows( aAr, this->ItemFormat( aAr ), Rows );
    }

    std::span<const FIELD_ROW> Rows;
};


template <typename BASE>
void RegisterSkipped( REGISTRY& aReg, const char* aName, std::span<const FIELD_ROW> aRows )
{
    aReg[aName] = [aRows]() -> std::unique_ptr<OBJECT>
    {
        std::unique_ptr<SKIPPED<BASE>> obj = std::make_unique<SKIPPED<BASE>>();
        obj->Rows = aRows;
        return obj;
    };
}

} // namespace EASYPC

#endif // EASYPC_ARCHIVE_H
