/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// This file is a standalone one-header-file library using only C++03.
//
// It assumes that the program is compiled for the "Itanium" C++ ABI
// (http://itanium-cxx-abi.github.io/cxx-abi/), a common C++ ABI used on many
// CPU architectures and operating systems.
//

#ifndef RTTI_DUMP
#define RTTI_DUMP

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <set>
#include <string>
#include <typeinfo>

#if defined( __ANDROID__ ) && !defined( RTTI_DUMP_USE_PRINTF )
#include <android/log.h>
#define RTTI_DUMP_LOG( fmt, ... )                                                                  \
    __android_log_print( ANDROID_LOG_INFO, "rtti_dump", fmt, ##__VA_ARGS__ )
#else
#define RTTI_DUMP_LOG( fmt, ... ) printf( fmt "\n", ##__VA_ARGS__ )
#endif

// Avoid compiler warnings.
#define RTTI_DUMP_UNUSED __attribute__( ( unused ) )

// Use an anonymous namespace so this header file can be included at the top of
// multiple C++ source files without breaking the One Definition Rule.
namespace rtti_dump
{
namespace
{

    // Returns the type of the current exception object or NULL if the thread is
    // not currently in a catch block.
    extern "C" const std::type_info* __cxa_current_exception_type();

    // Using run-time type information, returns an std::type_info* corresponding to
    // the most-derived class of a pointer to an object. The pointed-to type must
    // be a polymorphic class. (i.e. The class must have a virtual function or a
    // base class with a virtual function.)
    //
    // The function can return NULL if the object's vtable comes from an object
    // file compiled without -frtti.
    template <typename T>
    RTTI_DUMP_UNUSED const std::type_info* runtime_typeid( const volatile T* dynptr )
    {
        T* dynptr_unqual = const_cast<T*>( dynptr );

        // Use dynamic_cast<void*> to ensure that T is polymorphic. The result is
        // discarded just in case the most-derived object vtable and the subobject
        // vtable point to different typeinfo objects. (XXX: I *think* that's
        // impossible, though.)
        (void) sizeof( dynamic_cast<void*>( dynptr_unqual ) );

        void* vptr = *reinterpret_cast<void**>( dynptr_unqual );
        void* typeid_void = reinterpret_cast<void**>( vptr )[-1];
        return reinterpret_cast<const std::type_info*>( typeid_void );
    }

    // Returns the name of the DSO or binary containing the given address.
    RTTI_DUMP_UNUSED std::string dladdr_fname( const void* ptr )
    {
        Dl_info info = { 0 };

        if( !dladdr( const_cast<void*>( ptr ), &info ) )
        {
            char buf[64];
            snprintf( buf, sizeof( buf ), "[error: dladdr failed - %d]", errno );
            return buf;
        }
        else
        {
            return std::string( info.dli_fname );
        }
    }

    // Dump the address of the std::type_info object, its name, and the shared
    // object where the type_info object is defined.
    RTTI_DUMP_UNUSED
    void dump_type( const std::type_info* type, const char* label = "dump_type", int indent = 0 )
    {
        const std::string prefix = label + std::string( ": " ) + std::string( indent, ' ' );

        if( type == NULL )
        {
            RTTI_DUMP_LOG( "%sERROR: dump_type called with type==NULL!", prefix.c_str() );
        }
        else
        {
            struct type_info
            {
                virtual ~type_info() {}
                const char* type_name;
            };

            assert( sizeof( type_info ) == sizeof( std::type_info ) );
            const char* const name = type->name();
            const char* const raw_name = reinterpret_cast<const type_info*>( type )->type_name;

            if( name == raw_name )
            {
                RTTI_DUMP_LOG( "%stype %s:", prefix.c_str(), name );
            }
            else if( raw_name + 1 == name )
            {
                RTTI_DUMP_LOG( "%stype %s (raw name == '%s' @ %p):", prefix.c_str(), name, raw_name,
                               raw_name );
            }
            else
            {
                RTTI_DUMP_LOG( "%stype %s (raw name == %p):", prefix.c_str(), name, raw_name );
            }

            RTTI_DUMP_LOG( "%s    type_info obj:  %p (in %s)", prefix.c_str(), type,
                           dladdr_fname( type ).c_str() );
            RTTI_DUMP_LOG( "%s    type_info name: %p (in %s)", prefix.c_str(), name,
                           dladdr_fname( name ).c_str() );
        }
    }

    // Call from a catch block to dump the type of the current exception.
    RTTI_DUMP_UNUSED
    void dump_current_exception( const char* label = "dump_current_exception" )
    {
        const std::type_info* type = __cxa_current_exception_type();

        if( type != NULL )
        {
            dump_type( type, label );
        }
        else
        {
            RTTI_DUMP_LOG( "%s: ERROR: dump_current_exception called outside a catch block!",
                           label );
        }
    }

    namespace hierarchy_dumper_internals
    {

        // std::type_info has virtual member functions, so the most-derived type of
        // a pointer to a std::type_info object can be determined at run-time by
        // looking for the std::type_info's own std::type_info. We rely upon this
        // property to walk a class's RTTI graph at run-time.

        struct __class_type_info : std::type_info
        {
        };

        struct __si_class_type_info : __class_type_info
        {
            const __class_type_info* __base_type;
        };

        struct __base_class_type_info
        {
            const __class_type_info* __base_type;
            long                     __offset_flags;
        };

        struct __vmi_class_type_info : __class_type_info
        {
            unsigned int           __flags;
            unsigned int           __base_count;
            __base_class_type_info __base_info[1];
        };

        class Dumper
        {
            const char*                     label_;
            std::set<const std::type_info*> seen_;

        public:
            Dumper( const char* label ) : label_( label ) {}
            void dump_type( const std::type_info* info, int indent );
        };

        const int kIndent = 4;

        void Dumper::dump_type( const std::type_info* info, int indent )
        {
            ::rtti_dump::dump_type( info, label_, indent * kIndent );

            if( info == NULL )
            {
                return;
            }

            const std::type_info*         info_type = runtime_typeid( info );
            __base_class_type_info        lone_base = { 0 };
            const __base_class_type_info* base_table = NULL;
            unsigned int                  base_count = 0;

            // Determine type equality using a string comparison, because this dumping
            // system doesn't trust std::type_info::operator== to work with multiple
            // shared objects.

            const int sub_indent_sp = ( indent + 1 ) * kIndent;

            if( info_type == NULL )
            {
                // I don't think info_type can ever be NULL here.
                RTTI_DUMP_LOG( "%s: %*sERROR: runtime_typeid(info) was NULL!", label_,
                               sub_indent_sp, "" );
            }
            else if( !strcmp( info_type->name(), "N10__cxxabiv120__si_class_type_infoE" ) )
            {
                const __si_class_type_info& infox =
                        *reinterpret_cast<const __si_class_type_info*>( info );
                lone_base.__base_type = infox.__base_type;
                base_count = 1;
                base_table = &lone_base;
            }
            else if( !strcmp( info_type->name(), "N10__cxxabiv121__vmi_class_type_infoE" ) )
            {
                const __vmi_class_type_info& infox =
                        *reinterpret_cast<const __vmi_class_type_info*>( info );
                base_count = infox.__base_count;
                base_table = infox.__base_info;
            }

            if( base_count > 0 )
            {
                if( seen_.find( info ) != seen_.end() )
                {
                    RTTI_DUMP_LOG( "%s: %*sbase classes: ...elided...", label_, sub_indent_sp, "" );
                }
                else
                {
                    RTTI_DUMP_LOG( "%s: %*sbase classes:", label_, sub_indent_sp, "" );
                    seen_.insert( info );

                    for( unsigned int i = 0; i < base_count; ++i )
                    {
                        dump_type( base_table[i].__base_type, indent + 2 );
                    }
                }
            }
        }
    } // namespace hierarchy_dumper_internals

    // Dump the std::type_info object, and if it represents a class with base
    // classes, then dumps the class hierarchy.
    RTTI_DUMP_UNUSED
    void dump_class_hierarchy( const std::type_info* info,
                               const char*           label = "dump_class_hierarchy" )
    {
        hierarchy_dumper_internals::Dumper dumper( label );
        dumper.dump_type( info, 0 );
    }

} // anonymous namespace
} // namespace rtti_dump

#endif // RTTI_DUMP
