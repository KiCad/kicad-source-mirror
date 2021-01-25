// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_MOCK_HPP_INCLUDED
#define MOCK_MOCK_HPP_INCLUDED

#include "cleanup.hpp"
#include "config.hpp"
#include "detail/mock_impl.hpp"
#include "object.hpp"
#include "reset.hpp"
#include "verify.hpp"

/// MOCK_CLASS( name )
/// Define a class
#define MOCK_CLASS(name) struct name : mock::object

/// MOCK_BASE_CLASS( name, base )
/// Define a class deriving from a base class
#define MOCK_BASE_CLASS(name, ...) struct name : __VA_ARGS__, mock::object, mock::detail::base<__VA_ARGS__>

/// MOCK_PROTECT_SIGNATURE( signature )
/// Use this with MOCK_FUNCTION/MOCK_*_METHOD if the return type contains commas
#define MOCK_PROTECT_SIGNATURE(...) mock::detail::unwrap_signature_t<void(__VA_ARGS__)>

/// MOCK_FUNCTOR( name, signature )
/// Define a callable variable/member
#define MOCK_FUNCTOR(name, ...) mock::detail::functor<__VA_ARGS__> name, name##_mock

/// MOCK_CONVERSION_OPERATOR( [calling convention] name, type, identifier )
/// generates both const and non-const conversion operators
#define MOCK_CONVERSION_OPERATOR(M, T, identifier)              \
    M T() const { return MOCK_ANONYMOUS_HELPER(identifier)(); } \
    M T() { return MOCK_ANONYMOUS_HELPER(identifier)(); }       \
    MOCK_METHOD_HELPER(T(), identifier)
/// MOCK_CONST_CONVERSION_OPERATOR( [calling convention] name, type, identifier )
/// generates only a const conversion operator
#define MOCK_CONST_CONVERSION_OPERATOR(M, T, identifier)        \
    M T() const { return MOCK_ANONYMOUS_HELPER(identifier)(); } \
    MOCK_METHOD_HELPER(T(), identifier)
/// MOCK_NON_CONST_CONVERSION_OPERATOR( [calling convention] name, type, identifier )
/// generates only a non-const conversion operator
#define MOCK_NON_CONST_CONVERSION_OPERATOR(M, T, identifier) \
    M T() { return MOCK_ANONYMOUS_HELPER(identifier)(); }    \
    MOCK_METHOD_HELPER(T(), identifier)

/// MOCK_CONSTRUCTOR( [calling convention] name, arity, parameters, identifier )
/// As constructors do not have a return type, the usual signature gets restricted here to just the parameters.
#define MOCK_CONSTRUCTOR(T, arity, parameters, identifier) MOCK_CONSTRUCTOR_AUX(T, arity, parameters, identifier)

/// MOCK_DESTRUCTOR( [calling convention] ~name, identifier )
#define MOCK_DESTRUCTOR(T, identifier)           \
    T()                                          \
    {                                            \
        try                                      \
        {                                        \
            MOCK_ANONYMOUS_HELPER(identifier)(); \
        } catch(...)                             \
        {}                                       \
    }                                            \
    MOCK_METHOD_HELPER(void(), identifier)

/// MOCK_METHOD( [calling convention] name, arity[, signature[, identifier]] )
/// generates both const and non-const methods
/// The 'signature' can be omitted if it can be uniquely identified from the base class
/// if 'identifier' is omitted it will default to 'name'
#define MOCK_METHOD(M, ...)                                                 \
    MOCK_METHOD_EXT(M,                                                      \
                    MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ),                    \
                    MOCK_VARIADIC_ELEM_1(__VA_ARGS__, MOCK_SIGNATURE(M), ), \
                    MOCK_VARIADIC_ELEM_2(__VA_ARGS__, M, M, ))
/// MOCK_CONST_METHOD( [calling convention] name, arity[, signature[, identifier]] )
/// generates only the const version of the method
/// The 'signature' can be omitted if it can be uniquely identified from the base class
/// if 'identifier' is omitted it will default to 'name'
#define MOCK_CONST_METHOD(M, ...)                                                 \
    MOCK_CONST_METHOD_EXT(M,                                                      \
                          MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ),                    \
                          MOCK_VARIADIC_ELEM_1(__VA_ARGS__, MOCK_SIGNATURE(M), ), \
                          MOCK_VARIADIC_ELEM_2(__VA_ARGS__, M, M, ))
/// MOCK_NON_CONST_METHOD( [calling convention] name, arity[, signature[, identifier]] )
/// generates only the non-const version of the method
/// The 'signature' can be omitted if it can be uniquely identified from the base class
/// if 'identifier' is omitted it will default to 'name'
#define MOCK_NON_CONST_METHOD(M, ...)                                                 \
    MOCK_NON_CONST_METHOD_EXT(M,                                                      \
                              MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ),                    \
                              MOCK_VARIADIC_ELEM_1(__VA_ARGS__, MOCK_SIGNATURE(M), ), \
                              MOCK_VARIADIC_ELEM_2(__VA_ARGS__, M, M, ))

/// MOCK_FUNCTION( [calling convention] name, arity, signature[, identifier] )
/// if 'identifier' is omitted it will default to 'name'
#define MOCK_FUNCTION(F, arity, ...) \
    MOCK_FUNCTION_AUX(F, arity, MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ), MOCK_VARIADIC_ELEM_1(__VA_ARGS__, F, ), inline)

/// MOCK_STATIC_METHOD( [calling convention] name, arity, signature[, identifier] )
/// if 'identifier' is omitted it will default to 'name'
#define MOCK_STATIC_METHOD(F, arity, ...) \
    MOCK_FUNCTION_AUX(F, arity, MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ), MOCK_VARIADIC_ELEM_1(__VA_ARGS__, F, ), static)

/// MOCK_EXPECT( identifier )
/// Begin setting up expectation for the identifier
#define MOCK_EXPECT(identifier) MOCK_HELPER(identifier).expect(__FILE__, __LINE__)
/// MOCK_RESET( identifier )
/// Reset all pending expectations for the identifier
#define MOCK_RESET(identifier) MOCK_HELPER(identifier).reset(__FILE__, __LINE__)
/// MOCK_VERIFY( identifier )
/// Verify all expectations for the identifier have been met
#define MOCK_VERIFY(identifier) MOCK_HELPER(identifier).verify(__FILE__, __LINE__)

#endif // MOCK_MOCK_HPP_INCLUDED
