// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_FUNCTOR_HPP_INCLUDED
#define MOCK_FUNCTOR_HPP_INCLUDED

#include "../config.hpp"
#include "function.hpp"
#include "mutex.hpp"
#include "singleton.hpp"

namespace mock { namespace detail {
    class functor_mutex_t : public singleton<functor_mutex_t>, public mutex
    {
        MOCK_SINGLETON_CONS(functor_mutex_t);
    };
    MOCK_SINGLETON_INST(functor_mutex)

    template<typename Signature>
    struct functor : function<Signature>
    {
        functor()
        {
            scoped_lock _(functor_mutex);
            // MOCK_FUNCTOR creates 2 functor objects:
            // The user-usable one with the passed name and a 2nd used by MOCK_EXPECT with a suffixed name
            // We need the 2nd to be a copy of the first and use a static variable for storing a pointer to the first
            static functor* f = nullptr;
            if(f)
            {
                // Release the lock from the first call (see below) so other threads can create functors again
                // after the function exits (the scoped_lock still holds the mutex)
                functor_mutex.unlock();
                // Copy the first functor to the current (2nd) one
                *this = *f;
                f = nullptr;
            } else
            {
                // This is the first object, store its pointer
                f = this;
                // Lock the mutex again so only this thread can create new instances of a functor
                // making sure that we copy the right instance above and not one from a concurrent thread
                functor_mutex.lock();
            }
        }
    };
}} // namespace mock::detail

#endif // MOCK_FUNCTOR_HPP_INCLUDED
