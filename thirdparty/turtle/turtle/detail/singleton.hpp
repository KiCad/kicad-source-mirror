// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2014
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_SINGLETON_HPP
#define MOCK_SINGLETON_HPP

#include <boost/config.hpp>

namespace mock { namespace detail {

    template<typename Derived>
    class singleton
    {
    public:
        static Derived& instance()
        {
            static Derived the_inst;
            return the_inst;
        }

        singleton(singleton const&) = delete;
        singleton& operator=(singleton const&) = delete;

    protected:
        singleton() = default;
        ~singleton() = default;
    };

}} // namespace mock::detail

// Add a private ctor to the type to prevent misuse
#define MOCK_SINGLETON_CONS(type)               \
private:                                        \
    friend class mock::detail::singleton<type>; \
    type() = default

#define MOCK_SINGLETON_INST(inst) static BOOST_JOIN(inst, _t)& inst = BOOST_JOIN(inst, _t)::instance();

#endif // MOCK_SINGLETON_HPP
