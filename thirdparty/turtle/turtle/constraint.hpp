// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_CONSTRAINT_HPP_INCLUDED
#define MOCK_CONSTRAINT_HPP_INCLUDED

#include "config.hpp"
#include "log.hpp"
#include "unwrap_reference.hpp"
#include <boost/preprocessor/array.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <functional>
#include <type_traits>

namespace mock {
template<typename Constraint>
struct constraint
{
    constraint() {}
    constraint(const Constraint& c) : c_(c) {}
    Constraint c_;
};

namespace detail {
    template<typename Lhs, typename Rhs>
    class and_
    {
    public:
        and_(const Lhs& lhs, const Rhs& rhs) : lhs_(lhs), rhs_(rhs) {}
        template<typename Actual>
        bool operator()(const Actual& actual) const
        {
            return lhs_(actual) && rhs_(actual);
        }
        friend std::ostream& operator<<(std::ostream& s, const and_& a)
        {
            return s << "( " << mock::format(a.lhs_) << " && " << mock::format(a.rhs_) << " )";
        }

    private:
        Lhs lhs_;
        Rhs rhs_;
    };

    template<typename Lhs, typename Rhs>
    class or_
    {
    public:
        or_(const Lhs& lhs, const Rhs& rhs) : lhs_(lhs), rhs_(rhs) {}
        template<typename Actual>
        bool operator()(const Actual& actual) const
        {
            return lhs_(actual) || rhs_(actual);
        }
        friend std::ostream& operator<<(std::ostream& s, const or_& o)
        {
            return s << "( " << mock::format(o.lhs_) << " || " << mock::format(o.rhs_) << " )";
        }

    private:
        Lhs lhs_;
        Rhs rhs_;
    };

    template<typename Constraint>
    class not_
    {
    public:
        explicit not_(const Constraint& c) : c_(c) {}
        template<typename Actual>
        bool operator()(const Actual& actual) const
        {
            return !c_(actual);
        }
        friend std::ostream& operator<<(std::ostream& s, const not_& n) { return s << "! " << mock::format(n.c_); }

    private:
        Constraint c_;
    };
} // namespace detail

template<typename Lhs, typename Rhs>
const constraint<detail::or_<Lhs, Rhs>> operator||(const constraint<Lhs>& lhs, const constraint<Rhs>& rhs)
{
    return detail::or_<Lhs, Rhs>(lhs.c_, rhs.c_);
}

template<typename Lhs, typename Rhs>
const constraint<detail::and_<Lhs, Rhs>> operator&&(const constraint<Lhs>& lhs, const constraint<Rhs>& rhs)
{
    return detail::and_<Lhs, Rhs>(lhs.c_, rhs.c_);
}

template<typename Constraint>
const constraint<detail::not_<Constraint>> operator!(const constraint<Constraint>& c)
{
    return detail::not_<Constraint>(c.c_);
}
} // namespace mock

#define MOCK_UNARY_CONSTRAINT(Name, n, Args, Expr)                                                               \
    namespace detail {                                                                                           \
        struct Name                                                                                              \
        {                                                                                                        \
            template<typename Actual>                                                                            \
            bool operator()(const Actual& actual) const                                                          \
            {                                                                                                    \
                (void)actual;                                                                                    \
                return Expr;                                                                                     \
            }                                                                                                    \
            friend std::ostream& operator<<(std::ostream& s, const Name&) { return s << BOOST_STRINGIZE(Name); } \
        };                                                                                                       \
    }                                                                                                            \
    const mock::constraint<detail::Name> Name;

#define MOCK_CONSTRAINT_ASSIGN(z, n, d) expected##n(std::forward<T##n>(e##n))

#define MOCK_CONSTRAINT_UNWRAP_REF(z, n, d) mock::unwrap_ref(expected##n)

#define MOCK_CONSTRAINT_FORMAT(z, n, d) BOOST_PP_IF(n, << ", " <<, ) mock::format(c.expected##n)

#define MOCK_CONSTRAINT_MEMBER(z, n, d) Expected_##n expected##n;

#define MOCK_CONSTRAINT_TPL_TYPE(z, n, d) std::decay_t<const T##n>

#define MOCK_CONSTRAINT_CREF_PARAM(z, n, Args) \
    const mock::unwrap_reference_t<Expected_##n>& BOOST_PP_ARRAY_ELEM(n, Args)

#define MOCK_CONSTRAINT_ARG(z, n, Args) T##n&& BOOST_PP_ARRAY_ELEM(n, Args)

#define MOCK_CONSTRAINT_ARGS(z, n, Args) T##n&& e##n

#define MOCK_CONSTRAINT_PARAM(z, n, Args) std::forward<T##n>(BOOST_PP_ARRAY_ELEM(n, Args))

#define MOCK_NARY_CONSTRAINT(Name, n, Args, Expr)                                                                   \
    namespace detail {                                                                                              \
        template<BOOST_PP_ENUM_PARAMS(n, typename Expected_)>                                                       \
        struct Name                                                                                                 \
        {                                                                                                           \
            template<BOOST_PP_ENUM_PARAMS(n, typename T)>                                                           \
            explicit Name(BOOST_PP_ENUM(n, MOCK_CONSTRAINT_ARGS, _)) : BOOST_PP_ENUM(n, MOCK_CONSTRAINT_ASSIGN, _)  \
            {}                                                                                                      \
            template<typename Actual>                                                                               \
            bool operator()(const Actual& actual) const                                                             \
            {                                                                                                       \
                return test(actual, BOOST_PP_ENUM(n, MOCK_CONSTRAINT_UNWRAP_REF, _));                               \
            }                                                                                                       \
            template<typename Actual>                                                                               \
            bool test(const Actual& actual, BOOST_PP_ENUM(n, MOCK_CONSTRAINT_CREF_PARAM, (n, Args))) const          \
            {                                                                                                       \
                return Expr;                                                                                        \
            }                                                                                                       \
            friend std::ostream& operator<<(std::ostream& s, const Name& c)                                         \
            {                                                                                                       \
                return s << BOOST_STRINGIZE(Name) << "( " << BOOST_PP_REPEAT(n, MOCK_CONSTRAINT_FORMAT, _) << " )"; \
            }                                                                                                       \
            BOOST_PP_REPEAT(n, MOCK_CONSTRAINT_MEMBER, _)                                                           \
        };                                                                                                          \
    }                                                                                                               \
    template<BOOST_PP_ENUM_PARAMS(n, typename T)>                                                                   \
    mock::constraint<detail::Name<BOOST_PP_ENUM(n, MOCK_CONSTRAINT_TPL_TYPE, _)>> Name(                             \
      BOOST_PP_ENUM(n, MOCK_CONSTRAINT_ARG, (n, Args)))                                                             \
    {                                                                                                               \
        return detail::Name<BOOST_PP_ENUM(n, MOCK_CONSTRAINT_TPL_TYPE, _)>(                                         \
          BOOST_PP_ENUM(n, MOCK_CONSTRAINT_PARAM, (n, Args)));                                                      \
    }

#define MOCK_CONSTRAINT_EXT(Name, n, Args, Expr) \
    BOOST_PP_IF(n, MOCK_NARY_CONSTRAINT, MOCK_UNARY_CONSTRAINT)(Name, n, Args, Expr)

#define MOCK_CONSTRAINT_AUX_AUX(Name, n, Array) \
    MOCK_CONSTRAINT_EXT(Name, n, BOOST_PP_ARRAY_TO_TUPLE(BOOST_PP_ARRAY_POP_BACK(Array)), BOOST_PP_ARRAY_ELEM(n, Array))

#define MOCK_CONSTRAINT_AUX(Name, Size, Tuple) MOCK_CONSTRAINT_AUX_AUX(Name, BOOST_PP_DEC(Size), (Size, Tuple))

#define MOCK_CONSTRAINT(Name, ...) MOCK_CONSTRAINT_AUX(Name, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), (__VA_ARGS__))

#endif // MOCK_CONSTRAINT_HPP_INCLUDED
