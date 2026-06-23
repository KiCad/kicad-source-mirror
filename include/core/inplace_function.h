/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#ifndef INPLACE_FUNCTION_H
#define INPLACE_FUNCTION_H

#include <cstddef>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

/**
 * A std::function-style callable wrapper that stores the target in a fixed inline
 * buffer and never allocates on the heap.
 *
 * std::function heap-allocates whenever the callable exceeds its small-buffer size
 * (~16 bytes on libstdc++), which dominates hot paths that build a short-lived closure
 * per call.  INPLACE_FUNCTION keeps the closure inline, trading a compile-time size cap
 * for the elimination of those allocations.
 *
 * The stored callable must be nothrow-move-constructible and no larger than Capacity
 * bytes; either violation is a compile error.
 */
template <typename Sig, std::size_t Capacity = 4 * sizeof( void* )>
class INPLACE_FUNCTION;

template <typename R, typename... Args, std::size_t Capacity>
class INPLACE_FUNCTION<R( Args... ), Capacity>
{
public:
    INPLACE_FUNCTION() = default;

    template <typename F,
              typename = std::enable_if_t<
                      !std::is_same_v<std::decay_t<F>, INPLACE_FUNCTION>
                      && std::is_invocable_r_v<R, const std::decay_t<F>&, Args...>>>
    INPLACE_FUNCTION( F&& aCallable )
    {
        assign( std::forward<F>( aCallable ) );
    }

    INPLACE_FUNCTION( const INPLACE_FUNCTION& aOther ) { copyFrom( aOther ); }
    INPLACE_FUNCTION( INPLACE_FUNCTION&& aOther ) noexcept { moveFrom( aOther ); }

    INPLACE_FUNCTION& operator=( const INPLACE_FUNCTION& aOther )
    {
        if( this != &aOther )
        {
            // Build the copy before tearing down the old target so a throwing copy
            // leaves *this unchanged; the move back in is noexcept (see assign()).
            INPLACE_FUNCTION tmp( aOther );
            reset();
            moveFrom( tmp );
        }

        return *this;
    }

    INPLACE_FUNCTION& operator=( INPLACE_FUNCTION&& aOther ) noexcept
    {
        if( this != &aOther )
        {
            reset();
            moveFrom( aOther );
        }

        return *this;
    }

    template <typename F,
              typename = std::enable_if_t<
                      !std::is_same_v<std::decay_t<F>, INPLACE_FUNCTION>
                      && std::is_invocable_r_v<R, const std::decay_t<F>&, Args...>>>
    INPLACE_FUNCTION& operator=( F&& aCallable )
    {
        INPLACE_FUNCTION tmp( std::forward<F>( aCallable ) );
        reset();
        moveFrom( tmp );
        return *this;
    }

    ~INPLACE_FUNCTION() { reset(); }

    explicit operator bool() const { return m_invoke != nullptr; }

    R operator()( Args... aArgs ) const
    {
        if( !m_invoke )
            throw std::bad_function_call();

        return m_invoke( &m_storage, std::forward<Args>( aArgs )... );
    }

private:
    enum class OP { DESTROY, MOVE, COPY };

    using INVOKE_FN = R ( * )( const void*, Args&&... );
    using MANAGE_FN = void ( * )( OP, void*, const void* );

    template <typename F>
    void assign( F&& aCallable )
    {
        using DECAYED = std::decay_t<F>;
        static_assert( sizeof( DECAYED ) <= Capacity,
                       "callable too large for INPLACE_FUNCTION; raise its Capacity argument" );
        static_assert( alignof( DECAYED ) <= alignof( std::max_align_t ), "callable over-aligned" );
        static_assert( std::is_nothrow_move_constructible_v<DECAYED>,
                       "callable must be nothrow move constructible for INPLACE_FUNCTION" );

        ::new( &m_storage ) DECAYED( std::forward<F>( aCallable ) );

        m_invoke = []( const void* aStorage, Args&&... aArgs ) -> R
        {
            return ( *static_cast<const DECAYED*>( aStorage ) )( std::forward<Args>( aArgs )... );
        };

        m_manage = []( OP aOp, void* aDst, const void* aSrc )
        {
            switch( aOp )
            {
            case OP::DESTROY:
                static_cast<DECAYED*>( aDst )->~DECAYED();
                break;
            case OP::MOVE:
                ::new( aDst ) DECAYED( std::move( *const_cast<DECAYED*>( static_cast<const DECAYED*>( aSrc ) ) ) );
                break;
            case OP::COPY:
                ::new( aDst ) DECAYED( *static_cast<const DECAYED*>( aSrc ) );
                break;
            }
        };
    }

    void reset()
    {
        if( m_manage )
        {
            m_manage( OP::DESTROY, &m_storage, nullptr );
            m_manage = nullptr;
            m_invoke = nullptr;
        }
    }

    void moveFrom( INPLACE_FUNCTION& aOther )
    {
        if( aOther.m_manage )
        {
            aOther.m_manage( OP::MOVE, &m_storage, &aOther.m_storage );
            m_invoke = aOther.m_invoke;
            m_manage = aOther.m_manage;
            aOther.reset();
        }
    }

    // The COPY path keeps INPLACE_FUNCTION (and anything embedding it, e.g. VALUE) copyable;
    // the stored callable must therefore be copy-constructible, as std::function's target is.
    void copyFrom( const INPLACE_FUNCTION& aOther )
    {
        if( aOther.m_manage )
        {
            aOther.m_manage( OP::COPY, &m_storage, &aOther.m_storage );
            m_invoke = aOther.m_invoke;
            m_manage = aOther.m_manage;
        }
    }

    alignas( std::max_align_t ) unsigned char m_storage[Capacity];
    INVOKE_FN                                 m_invoke = nullptr;
    MANAGE_FN                                 m_manage = nullptr;
};

#endif // INPLACE_FUNCTION_H
