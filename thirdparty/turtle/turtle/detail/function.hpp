// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_FUNCTION_HPP_INCLUDED
#define MOCK_FUNCTION_HPP_INCLUDED

#include "../config.hpp"
#include "../log.hpp"
#include "../sequence.hpp"
#include "context.hpp"
#include "function_impl.hpp"
#include "type_name.hpp"
#include <boost/optional.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <ostream>

namespace mock { namespace detail {
    template<typename Signature>
    class function;

    template<typename R, typename... Ts>
    class function<R(Ts...)>
    {
    private:
        typedef function_impl<R(Ts...)> impl_type;
        typedef typename impl_type::wrapper_type expectation_type;
        typedef typename impl_type::error_type error_type;

    public:
        function() : impl_(std::make_shared<impl_type>()) {}

        bool verify() const { return impl_->verify(); }
        bool verify(const char* file, int line) const
        {
            error_type::pass(file, line);
            return impl_->verify();
        }
        void reset() { impl_->reset(); }
        void reset(const char* file, int line)
        {
            error_type::pass(file, line);
            impl_->reset();
        }

        expectation_type expect(const char* file, int line)
        {
            error_type::pass(file, line);
            return impl_->expect(file, line);
        }
        expectation_type expect() { return impl_->expect(); }

        R operator()(Ts... args) const { return (*impl_)(static_cast<ref_arg_t<Ts>>(args)...); }

        friend std::ostream& operator<<(std::ostream& s, const function& f) { return s << *f.impl_; }

        function& operator()(context& c, boost::unit_test::const_string instance)
        {
            impl_->add(c, impl_.get(), instance, boost::none, "");
            return *this;
        }

        void configure(context& c,
                       const void* p,
                       boost::unit_test::const_string instance,
                       boost::optional<type_name> type,
                       boost::unit_test::const_string name) const
        {
            impl_->add(c, p, instance, type, name);
        }

    private:
        std::shared_ptr<impl_type> impl_;
    };
}} // namespace mock::detail

#endif // MOCK_FUNCTION_HPP_INCLUDED
