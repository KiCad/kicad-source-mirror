/*
* This program source code file is part of KICAD, a free EDA CAD application.
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

#ifndef COMMON_OBSERVABLE_H__
#define COMMON_OBSERVABLE_H__

#include <cassert>
#include <memory>
#include <vector>
#include <utility>

/**
 * A model subscriber implementation using links to represent connections.  Subscribers
 * can be removed during notification.  If no observers are registered, size is the size
 * of a shared_ptr.
 */
namespace UTIL
{
class LINK;

namespace DETAIL
{
    struct OBSERVABLE_BASE
    {
    public:
        OBSERVABLE_BASE();
        OBSERVABLE_BASE( OBSERVABLE_BASE& other );

        ~OBSERVABLE_BASE();

        size_t size() const;

    private:
        friend class UTIL::LINK;

        struct IMPL
        {
            IMPL( OBSERVABLE_BASE* owned_by = nullptr );
            bool is_shared() const;
            void set_shared();
            ~IMPL();

            void add_observer( void* observer );
            void remove_observer( void* observer );
            void collect();

            bool is_iterating() const;

            void enter_iteration();
            void leave_iteration();

            std::vector<void*> observers_;
            unsigned int       iteration_count_;
            OBSERVABLE_BASE*   owned_by_;
        };

        void allocate_impl();
        void allocate_shared_impl();

        void deallocate_impl();

        std::shared_ptr<IMPL> get_shared_impl();

    protected:
        void on_observers_empty();

        void enter_iteration();
        void leave_iteration();

        void add_observer( void* observer );
        void remove_observer( void* observer );

        std::shared_ptr<IMPL> impl_;
    };

} // namespace DETAIL


/**
 * Simple RAII-handle to a subscription.
 */
class LINK
{
public:
    LINK();
    LINK( std::shared_ptr<DETAIL::OBSERVABLE_BASE::IMPL> token, void* observer );
    LINK( LINK&& other );
    LINK( const LINK& ) = delete;

    void  operator=( const LINK& ) = delete;
    LINK& operator=( LINK&& other );

    void reset();

    explicit operator bool() const;

    ~LINK();

private:
    std::shared_ptr<DETAIL::OBSERVABLE_BASE::IMPL> token_;
    void*                                          observer_;
};


template <typename ObserverInterface>
class OBSERVABLE : public DETAIL::OBSERVABLE_BASE
{
public:
    /**
     * Construct an observable with empty non-shared subscription list.
     */
    OBSERVABLE() {}

    /**
     * Construct an observable with a shared subscription list.
     *
     * @param aInherit Observable to share the subscription list with.
     */
    OBSERVABLE( OBSERVABLE& aInherit ) : OBSERVABLE_BASE( aInherit ) {}

    /**
     * Add a subscription without RAII link.
     *
     * @param aObserver Observer to subscribe.
     */
    void SubscribeUnmanaged( ObserverInterface* aObserver )
    {
        OBSERVABLE_BASE::add_observer( static_cast<void*>( aObserver ) );
    }

    /**
     * Add a subscription returning an RAII link.
     *
     * @param aObserver observer to subscribe
     * @return RAII link controlling the lifetime of the subscription
     */
    LINK Subscribe( ObserverInterface* aObserver )
    {
        OBSERVABLE_BASE::add_observer( static_cast<void*>( aObserver ) );
        return LINK( impl_, static_cast<void*>( aObserver ) );
    }

    /**
     * Cancel the subscription of a subscriber.
     *
     * This can be called during notification calls.
     *
     * @param aObserver observer to remove from the subscription list.
     */
    void Unsubscribe( ObserverInterface* aObserver )
    {
        OBSERVABLE_BASE::remove_observer( static_cast<void*>( aObserver ) );
    }

    /**
     * Notify event to all subscribed observers.
     *
     * @param Ptr is a pointer to method of the observer interface.
     * @param aArgs is a list of arguments to each notification call, will be perfectly forwarded.
     */
    template <typename... Args1, typename... Args2>
    void Notify( void ( ObserverInterface::*Ptr )( Args1... ), Args2&&... aArgs )
    {
        static_assert( sizeof...( Args1 ) == sizeof...( Args2 ), "argument counts don't match" );

        if( impl_ )
        {
            enter_iteration();
            try
            {
                for( auto* void_ptr : impl_->observers_ )
                {
                    if( void_ptr )
                    {
                        auto* typed_ptr = static_cast<ObserverInterface*>( void_ptr );
                        ( typed_ptr->*Ptr )( std::forward<Args2>( aArgs )... );
                    }
                }
            }
            catch( ... )
            {
                leave_iteration();
                throw;
            }

            leave_iteration();
        }
    }

    /**
     * Notify event to all subscribed observers but one to be ignore.
     *
     * @param Ptr is a pointer to method of the observer interface.
     * @param aIgnore is an observer to ignore during this notification.
     * @param aArgs is a list of arguments to each notification call, will be perfectly forwarded.
     */
    template <typename... Args1, typename... Args2>
    void NotifyIgnore( void ( ObserverInterface::*Ptr )( Args1... ), ObserverInterface* aIgnore,
                       Args2&&... aArgs )
    {
        static_assert( sizeof...( Args1 ) == sizeof...( Args2 ), "argument counts don't match" );

        if( impl_ )
        {
            enter_iteration();

            try
            {
                for( auto* void_ptr : impl_->observers_ )
                {
                    if( void_ptr && void_ptr != aIgnore )
                    {
                        auto* typed_ptr = static_cast<ObserverInterface*>( void_ptr );
                        ( typed_ptr->*Ptr )( std::forward<Args2>( aArgs )... );
                    }
                }
            }
            catch( ... )
            {
                leave_iteration();
                throw;
            }

        }
    }
};

} // namespace UTIL

#endif
