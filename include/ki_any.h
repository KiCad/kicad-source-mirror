/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// This code is a modified version of the GCC standard library implementation (see original
// licence below):

// Copyright (C) 2014-2024 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.
//
// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.


/**
 * @file ki_any.h
 * @brief An implementation of std::any_cast, which uses type_info::hash_code to check validity of
 *        cast types.
 *
 * This is required as Clang compares types as being equivalent based on their type_info pointer
 * locations. These are not guaranteed to be the same with identical types linked in multiple
 * targets from shared libraries. The current Clang implementation of type_info::hash_code is
 * based on the type names, which should be consistent across translation units.
 */
#ifndef INCLUDE_KI_ANY_H_
#define INCLUDE_KI_ANY_H_

#include <initializer_list>
#include <new>
#include <typeinfo>
#include <utility>

namespace ki
{

/*
 * Disambiguation helpers
 */

template <typename>
inline constexpr bool is_in_place_type_v = false;

template <typename T>
inline constexpr bool is_in_place_type_v<std::in_place_type_t<T>> = true;

/**
 * Exception class thrown by a failed @c any_cast
 */
class bad_any_cast final : public std::bad_cast
{
public:
    const char* what() const noexcept override { return "bad ki::any_cast"; }
};

/**
 * A type-safe container of any type.
 *
 * An `any` object's state is either empty or it stores a contained object
 * of CopyConstructible type.
 */
class any
{
    // Holds either a pointer to a heap object or the contained object itself
    union Storage
    {
        constexpr Storage() : m_ptr{ nullptr } {}

        // Prevent trivial copies of this type as the buffer might hold a non-POD
        Storage( const Storage& ) = delete;
        Storage& operator=( const Storage& ) = delete;

        void*         m_ptr;
        unsigned char m_buffer[sizeof( m_ptr )];
    };

    template <typename T, bool Safe = std::is_nothrow_move_constructible_v<T>,
              bool Fits = ( sizeof( T ) <= sizeof( Storage ) )
                          && ( alignof( T ) <= alignof( Storage ) )>
    using Use_Internal_Storage = std::integral_constant<bool, Safe && Fits>;

    template <typename T>
    struct Manager_Internal; // uses small-object optimization

    template <typename T>
    struct Manager_External; // creates contained object on the heap

    template <typename T>
    using Manager = std::conditional_t<Use_Internal_Storage<T>::value, Manager_Internal<T>,
                                       Manager_External<T>>;

    template <typename T, typename V = std::decay_t<T>>
    using decay_if_not_any = std::enable_if_t<!std::is_same_v<V, any>, V>;

    /// Emplace with an object created from @p args as the contained object.
    template <typename T, typename... Args, typename Mgr = Manager<T>>
    void do_emplace( Args&&... args )
    {
        reset();
        Mgr::do_create( m_storage, std::forward<Args>( args )... );
        m_manager = &Mgr::m_manage_fn;
    }

    /// Emplace with an object created from @p il and @p args as the contained object.
    template <typename T, typename U, typename... Args, typename Mgr = Manager<T>>
    void do_emplace( std::initializer_list<U> il, Args&&... args )
    {
        reset();
        Mgr::do_create( m_storage, il, std::forward<Args>( args )... );
        m_manager = &Mgr::m_manage_fn;
    }

    template <typename Res, typename T, typename... Args>
    using any_constructible =
            std::enable_if<std::is_copy_constructible_v<T> && std::is_constructible_v<T, Args...>,
                           Res>;

    template <typename T, typename... Args>
    using any_constructible_t = typename any_constructible<bool, T, Args...>::type;

    template <typename V, typename... Args>
    using any_emplace_t = typename any_constructible<V&, V, Args...>::type;

public:
    /// Default constructor, creates an empty object.
    constexpr any() noexcept : m_manager( nullptr ) {}

    /// Copy constructor, copies the state of @p other.
    any( const any& other )
    {
        if( !other.has_value() )
        {
            m_manager = nullptr;
        }
        else
        {
            Arg arg;
            arg.m_any = this;
            other.m_manager( Op_Clone, &other, &arg );
        }
    }

    /// Move constructor, transfer the state from @p other.
    any( any&& other ) noexcept
    {
        if( !other.has_value() )
        {
            m_manager = nullptr;
        }
        else
        {
            Arg arg;
            arg.m_any = this;
            other.m_manager( Op_Xfer, &other, &arg );
        }
    }

    /// Construct with a copy of @p value as the contained object.
    template <typename T, typename V = decay_if_not_any<T>, typename Mgr = Manager<V>,
              std::enable_if_t<std::is_copy_constructible_v<V> && !is_in_place_type_v<V>, bool> =
                      true>

    any( T&& value ) : m_manager( &Mgr::m_manage_fn )
    {
        Mgr::do_create( m_storage, std::forward<T>( value ) );
    }

    /// Construct with an object created from @p args as the contained object.
    template <typename T, typename... Args, typename V = std::decay_t<T>, typename Mgr = Manager<V>,
              any_constructible_t<V, Args&&...> = false>
    explicit any( std::in_place_type_t<T>, Args&&... args ) : m_manager( &Mgr::m_manage_fn )
    {
        Mgr::do_create( m_storage, std::forward<Args>( args )... );
    }

    /// Construct with an object created from @p il and @p args as the contained object.
    template <typename T, typename U, typename... Args, typename V = std::decay_t<T>,
              typename Mgr = Manager<V>,
              any_constructible_t<V, std::initializer_list<U>&, Args&&...> = false>
    explicit any( std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args ) :
            m_manager( &Mgr::m_manage_fn )
    {
        Mgr::do_create( m_storage, il, std::forward<Args>( args )... );
    }

    /// Destructor, calls @c reset().
    ~any() { reset(); }

    /// Copy the state of another object.
    any& operator=( const any& rhs )
    {
        *this = any( rhs );
        return *this;
    }

    /// Move assignment operator.
    any& operator=( any&& rhs ) noexcept
    {
        if( !rhs.has_value() )
        {
            reset();
        }
        else if( this != &rhs )
        {
            reset();
            Arg arg;
            arg.m_any = this;
            rhs.m_manager( Op_Xfer, &rhs, &arg );
        }

        return *this;
    }

    /// Store a copy of @p rhs as the contained object.
    template <typename T>
    std::enable_if_t<std::is_copy_constructible_v<decay_if_not_any<T>>, any&> operator=( T&& rhs )
    {
        *this = any( std::forward<T>( rhs ) );
        return *this;
    }

    /// Emplace with an object created from @p args as the contained object.
    template <typename T, typename... Args>
    any_emplace_t<std::decay_t<T>, Args...> emplace( Args&&... args )
    {
        using V = std::decay_t<T>;
        do_emplace<V>( std::forward<Args>( args )... );
        return *any::Manager<V>::do_access( m_storage );
    }

    /// Emplace with an object created from @p il and @p args as the contained object.
    template <typename T, typename U, typename... Args>
    any_emplace_t<std::decay_t<T>, std::initializer_list<U>&, Args&&...>
    emplace( std::initializer_list<U> il, Args&&... args )
    {
        using V = std::decay_t<T>;
        do_emplace<V, U>( il, std::forward<Args>( args )... );
        return *any::Manager<V>::do_access( m_storage );
    }

    /// If not empty, destroys the contained object.
    void reset() noexcept
    {
        if( has_value() )
        {
            m_manager( Op_Destroy, this, nullptr );
            m_manager = nullptr;
        }
    }

    /// Exchange state with another object.
    void swap( any& rhs ) noexcept
    {
        if( !has_value() && !rhs.has_value() )
            return;

        if( has_value() && rhs.has_value() )
        {
            if( this == &rhs )
                return;

            any tmp;
            Arg arg;
            arg.m_any = &tmp;
            rhs.m_manager( Op_Xfer, &rhs, &arg );
            arg.m_any = &rhs;
            m_manager( Op_Xfer, this, &arg );
            arg.m_any = this;
            tmp.m_manager( Op_Xfer, &tmp, &arg );
        }
        else
        {
            any* empty = !has_value() ? this : &rhs;
            any* full = !has_value() ? &rhs : this;
            Arg  arg;
            arg.m_any = empty;
            full->m_manager( Op_Xfer, full, &arg );
        }
    }

    /// Report whether there is a contained object or not.
    bool has_value() const noexcept { return m_manager != nullptr; }


    /// The @c typeid of the contained object, or @c typeid(void) if empty.
    const std::type_info& type() const noexcept
    {
        if( !has_value() )
            return typeid( void );

        Arg arg;
        m_manager( Op_Get_Type_Info, this, &arg );
        return *arg.m_typeinfo;
    }

    /// @cond undocumented
    template <typename T>
    static constexpr bool is_valid_any_cast()
    {
        return std::is_reference_v<T> || std::is_copy_constructible_v<T>;
    }
    /// @endcond

private:
    enum Op
    {
        Op_Access,
        Op_Get_Type_Info,
        Op_Clone,
        Op_Destroy,
        Op_Xfer
    };

    union Arg
    {
        void*                 m_obj;
        const std::type_info* m_typeinfo;
        any*                  m_any;
    };

    void ( *m_manager )( Op, const any*, Arg* );
    Storage m_storage;

    /// @cond undocumented
    template <typename T>
    friend void* any_caster( const any* any );
    /// @endcond

    // Manages in-place contained object
    template <typename T>
    struct Manager_Internal
    {
        static void m_manage_fn( Op which, const any* any, Arg* arg );

        template <typename U>
        static void do_create( Storage& storage, U&& value )
        {
            void* addr = &storage.m_buffer;
            ::new( addr ) T( std::forward<U>( value ) );
        }

        template <typename... Args>
        static void do_create( Storage& storage, Args&&... args )
        {
            void* addr = &storage.m_buffer;
            ::new( addr ) T( std::forward<Args>( args )... );
        }

        static T* do_access( const Storage& storage )
        {
            // The contained object is in storage.m_buffer
            const void* addr = &storage.m_buffer;
            return static_cast<T*>( const_cast<void*>( addr ) );
        }
    };

    // Manages externally (heap) contained object
    template <typename T>
    struct Manager_External
    {
        static void m_manage_fn( Op which, const any* any, Arg* arg );

        template <typename U>
        static void do_create( Storage& storage, U&& value )
        {
            storage.m_ptr = new T( std::forward<U>( value ) );
        }
        template <typename... Args>
        static void do_create( Storage& storage, Args&&... args )
        {
            storage.m_ptr = new T( std::forward<Args>( args )... );
        }
        static T* do_access( const Storage& storage )
        {
            // The contained object is in *storage.m_ptr
            return static_cast<T*>( storage.m_ptr );
        }
    };
};

/// Exchange the states of two @c any objects.
inline void swap( any& x, any& y ) noexcept
{
    x.swap( y );
}

/// Create a `any` holding a `T` constructed from `args...`.
template <typename T, typename... Args>
std::enable_if_t<std::is_constructible_v<any, std::in_place_type_t<T>, Args...>, any>
make_any( Args&&... args )
{
    return any( std::in_place_type<T>, std::forward<Args>( args )... );
}

/// Create an `any` holding a `T` constructed from `il` and `args...`.
template <typename T, typename U, typename... Args>
std::enable_if_t<
        std::is_constructible_v<any, std::in_place_type_t<T>, std::initializer_list<U>&, Args...>,
        any>
make_any( std::initializer_list<U> il, Args&&... args )
{
    return any( std::in_place_type<T>, il, std::forward<Args>( args )... );
}

/**
 * Access the contained object.
 *
 * @tparam  ValueType  A const-reference or CopyConstructible type.
 * @param   any        The object to access.
 * @return  The contained object.
 * @throw   bad_any_cast If <code>
 *          any.type() != typeid(remove_reference_t<ValueType>)
 *          </code>
 */
template <typename ValueType>
ValueType any_cast( const any& any )
{
    using U = std::remove_cvref_t<ValueType>;

    static_assert( any::is_valid_any_cast<ValueType>(),
                   "Template argument must be a reference or CopyConstructible type" );
    static_assert( std::is_constructible_v<ValueType, const U&>,
                   "Template argument must be constructible from a const value" );

    auto p = any_cast<U>( &any );

    if( p )
        return static_cast<ValueType>( *p );

    throw bad_any_cast{};
}

/**
 * Access the contained object.
 *
 * @tparam  ValueType  A reference or CopyConstructible type.
 * @param   any        The object to access.
 * @return  The contained object.
 * @throw   bad_any_cast If <code>
 *          any.type() != typeid(remove_reference_t<ValueType>)
 *          </code>
 * @{
 */
template <typename ValueType>
ValueType any_cast( any& any )
{
    using U = std::remove_cvref_t<ValueType>;

    static_assert( any::is_valid_any_cast<ValueType>(),
                   "Template argument must be a reference or CopyConstructible type" );
    static_assert( std::is_constructible_v<ValueType, U&>,
                   "Template argument must be constructible from an lvalue" );

    auto p = any_cast<U>( &any );

    if( p )
        return static_cast<ValueType>( *p );

    throw bad_any_cast{};
}

template <typename ValueType>
ValueType any_cast( any&& any )
{
    using U = std::remove_cvref_t<ValueType>;

    static_assert( any::is_valid_any_cast<ValueType>(),
                   "Template argument must be a reference or CopyConstructible type" );
    static_assert( std::is_constructible_v<ValueType, U>,
                   "Template argument must be constructible from an rvalue" );

    auto p = any_cast<U>( &any );

    if( p )
        return static_cast<ValueType>( std::move( *p ) );

    throw bad_any_cast{};
}

/// @}

/// @cond undocumented
template <typename T>
void* any_caster( const any* any )
{
    // any_cast<T> returns non-null if any->type() == typeid(T) and
    // typeid(T) ignores cv-qualifiers so remove them:
    using U = std::remove_cv_t<T>;

    if constexpr( !std::is_same_v<std::decay_t<U>, U> )
    {
        // The contained value has a decayed type, so if decay_t<U> is not U,
        // then it's not possible to have a contained value of type U
        return nullptr;
    }
    else if constexpr( !std::is_copy_constructible_v<U> )
    {
        // Only copy constructible types can be used for contained values
        return nullptr;
    }
    else if( any->m_manager == &any::Manager<U>::m_manage_fn
             || any->type().hash_code() == typeid( T ).hash_code() )
    {
        return any::Manager<U>::do_access( any->m_storage );
    }

    return nullptr;
}
/// @endcond

/**
 * Access the contained object.
 *
 * @tparam  ValueType  The type of the contained object.
 * @param   any       A pointer to the object to access.
 * @return  The address of the contained object if <code>
 *          any != nullptr && any.type() == typeid(ValueType)
 *          </code>, otherwise a null pointer.
 *
 * @{
 */
template <typename ValueType>
const ValueType* any_cast( const any* any ) noexcept
{
    static_assert( !std::is_void_v<ValueType> );

    // As an optimization, don't bother instantiating any_caster for
    // function types, since std::any can only hold objects
    if constexpr( std::is_object_v<ValueType> )
    {
        if( any )
            return static_cast<ValueType*>( any_caster<ValueType>( any ) );
    }

    return nullptr;
}

template <typename ValueType>
ValueType* any_cast( any* any ) noexcept
{
    static_assert( !std::is_void_v<ValueType> );

    if constexpr( std::is_object_v<ValueType> )
        if( any )
            return static_cast<ValueType*>( any_caster<ValueType>( any ) );
    return nullptr;
}
/// @}

template <typename T>
void any::Manager_Internal<T>::m_manage_fn( Op which, const any* any, Arg* arg )
{
    // The contained object is in m_storage.m_buffer
    auto ptr = reinterpret_cast<const T*>( &any->m_storage.m_buffer );
    switch( which )
    {
    case Op_Access: arg->m_obj = const_cast<T*>( ptr ); break;
    case Op_Get_Type_Info: arg->m_typeinfo = &typeid( T ); break;
    case Op_Clone:
        ::new( &arg->m_any->m_storage.m_buffer ) T( *ptr );
        arg->m_any->m_manager = any->m_manager;
        break;
    case Op_Destroy: ptr->~T(); break;
    case Op_Xfer:
        ::new( &arg->m_any->m_storage.m_buffer ) T( std::move( *const_cast<T*>( ptr ) ) );
        ptr->~T();
        arg->m_any->m_manager = any->m_manager;
        const_cast<ki::any*>( any )->m_manager = nullptr;
        break;
    }
}

template <typename T>
void any::Manager_External<T>::m_manage_fn( Op which, const any* any, Arg* arg )
{
    // The contained object is *m_storage.m_ptr
    auto ptr = static_cast<const T*>( any->m_storage.m_ptr );
    switch( which )
    {
    case Op_Access: arg->m_obj = const_cast<T*>( ptr ); break;
    case Op_Get_Type_Info: arg->m_typeinfo = &typeid( T ); break;
    case Op_Clone:
        arg->m_any->m_storage.m_ptr = new T( *ptr );
        arg->m_any->m_manager = any->m_manager;
        break;
    case Op_Destroy: delete ptr; break;
    case Op_Xfer:
        arg->m_any->m_storage.m_ptr = any->m_storage.m_ptr;
        arg->m_any->m_manager = any->m_manager;
        const_cast<ki::any*>( any )->m_manager = nullptr;
        break;
    }
}

} // namespace ki

#endif // INCLUDE_KI_ANY_H_
