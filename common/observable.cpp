#include "observable.h"

namespace UTIL {

    namespace DETAIL {

        template< typename T >
        struct equals {
            equals( const T& val ) : val_( val ) {}

            bool operator()( const T& val ) {
                return val == val_;
            }

        private:
            const T& val_;
        };

        OBSERVABLE_BASE::IMPL::IMPL( OBSERVABLE_BASE* owned_by )
            : owned_by_( owned_by ), iteration_count_( 0 )
        {}

        bool OBSERVABLE_BASE::IMPL::is_shared() const
        {
            return owned_by_ == nullptr;
        }

        void OBSERVABLE_BASE::IMPL::set_shared()
        {
            owned_by_ = nullptr;
        }

        OBSERVABLE_BASE::IMPL::~IMPL()
        {
        }

        void OBSERVABLE_BASE::IMPL::remove_observer( void* observer )
        {
            if(iteration_count_) {
                for(auto*& ptr : observers_) {
                    if(ptr == observer) {
                        ptr = nullptr;
                    }
                }
            }
            else {
                collect();
                if(observers_.empty() && owned_by_) {
                    owned_by_->on_observers_empty();
                }
            }
        }

        void OBSERVABLE_BASE::IMPL::collect()
        {
            auto it = std::remove_if( observers_.begin(), observers_.end(), DETAIL::equals<void*>( nullptr ) );
            observers_.erase( it, observers_.end() );
        }
    }

    LINK::LINK()
    {
    }

    LINK::LINK( std::shared_ptr<DETAIL::OBSERVABLE_BASE::IMPL> token, void* observer )
        : token_( std::move( token ) ), observer_( observer )
    {
    }

    LINK::LINK( LINK&& other )
        : token_( std::move( other.token_ ) ), observer_( other.observer_ )
    {
        other.token_.reset();
    }

    LINK& LINK::operator=( LINK&& other )
    {
        token_ = std::move( other.token_ );
        other.token_.reset();
        observer_ = other.observer_;
        return *this;
    }

    LINK::operator bool() const {
        return token_ ? true : false;
    }

    LINK::~LINK()
    {
        reset();
    }

    void LINK::reset()
    {
        if(token_) {
            token_->remove_observer( observer_ );
        }
    }

    namespace DETAIL {

        OBSERVABLE_BASE::OBSERVABLE_BASE()
        {
        }

        OBSERVABLE_BASE::OBSERVABLE_BASE( OBSERVABLE_BASE& other )
            : impl_( other.get_shared_impl() )
        {
        }

        OBSERVABLE_BASE::~OBSERVABLE_BASE()
        {
        }

        size_t OBSERVABLE_BASE::size() const {
            if(impl_) {
                return impl_->observers_.size();
            }
            else {
                return 0;
            }
        }

        void OBSERVABLE_BASE::remove_observer( void* observer ) {
            assert( impl_ );
            impl_->remove_observer( observer );
        }

        void OBSERVABLE_BASE::allocate_impl() {
            if(!impl_) {
                impl_ = std::make_shared<IMPL>( this );
            }
        }

        void OBSERVABLE_BASE::allocate_shared_impl()
        {
            if(!impl_) {
                impl_ = std::make_shared<IMPL>();
            }
            else {
                impl_->set_shared();
            }
        }

        void OBSERVABLE_BASE::deallocate_impl() {
            impl_.reset();
        }

        void OBSERVABLE_BASE::enter_iteration() {
            if(impl_) {
                ++impl_->iteration_count_;
            }
        }

        void OBSERVABLE_BASE::leave_iteration() {
            if(impl_) {
                --impl_->iteration_count_;
                if(impl_->iteration_count_ == 0) {
                    if(!impl_->is_shared() && impl_.use_count() == 1) {
                        impl_.reset();
                    }
                    else {
                        impl_->collect();
                    }
                }
            }
        }

        void OBSERVABLE_BASE::on_observers_empty()
        {
            // called by an impl that is owned by this, ie. it is a non-shared impl
            // also it is not iterating
            deallocate_impl();
        }

        std::shared_ptr<OBSERVABLE_BASE::IMPL> OBSERVABLE_BASE::get_shared_impl()
        {
            allocate_shared_impl();
            return impl_;
        }

        void OBSERVABLE_BASE::add_observer( void* observer ) {
            allocate_impl();
            impl_->observers_.push_back( observer );
        }

    }

}
