// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "matcher_base_template.hpp"

#define MOCK_EXPECTATION_INITIALIZE(z, n, d) \
    BOOST_PP_COMMA_IF(n) c##n##_( c##n )

#define MOCK_EXPECTATION_MEMBER(z, n, d) \
    matcher< T##n, Constraint_##n > c##n##_;

#define MOCK_EXPECTATION_IS_VALID(z, n, d) \
    BOOST_PP_IF(n, &&,) c##n##_( mock::detail::move_if_not_lvalue_reference< T##n >( a##n ) )

#define MOCK_EXPECTATION_SERIALIZE(z, n, d) \
    BOOST_PP_IF(n, << ", " <<,) c##n##_

#define MOCK_EXPECTATION_SERIALIZE_ANY(z, n, d) \
    BOOST_PP_IF(n, << ", " <<,) "any"

#define MOCK_EXPECTATION_PARAM(z, n, Args) \
    mock::detail::move_if_not_lvalue_reference< T##n >( a##n )

#define MOCK_REF_ARG(z, n, d) \
    typename ref_arg< T##n >::type a##n

#define MOCK_REF_ARG_T(z, n, d) \
    typename ref_arg< T##n >::type

namespace mock
{
namespace detail
{
    template< typename Signature > class default_matcher;

    template<
        BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, typename T) >
    class default_matcher< void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) ) >
        : public matcher_base< void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) ) >
    {
    private:
        virtual bool operator()(
            BOOST_PP_ENUM(MOCK_NUM_ARGS, MOCK_REF_ARG_T, _) )
        {
            return true;
        }
        virtual void serialize( std::ostream& s ) const
        {
            s << "" BOOST_PP_REPEAT(MOCK_NUM_ARGS,
                MOCK_EXPECTATION_SERIALIZE_ANY, _);
        }
    };

#ifndef MOCK_NUM_ARGS_0

    template< typename Constraint, typename Signature > class single_matcher;

    template<
        BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, typename Constraint_),
        BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, typename T)
    >
    class single_matcher<
        void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, Constraint_) ),
        void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) )
    >
        : public matcher_base< void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) ) >
    {
    public:
        single_matcher(
            BOOST_PP_ENUM_BINARY_PARAMS(MOCK_NUM_ARGS, Constraint_, c) )
        : BOOST_PP_REPEAT(MOCK_NUM_ARGS,
            MOCK_EXPECTATION_INITIALIZE, _)
        {}

    private:
        virtual bool operator()(
            BOOST_PP_ENUM(MOCK_NUM_ARGS, MOCK_REF_ARG, _) )
        {
            return BOOST_PP_REPEAT(MOCK_NUM_ARGS,
                MOCK_EXPECTATION_IS_VALID, _);
        }
        virtual void serialize( std::ostream& s ) const
        {
            s << BOOST_PP_REPEAT(MOCK_NUM_ARGS,
                MOCK_EXPECTATION_SERIALIZE, _);
        }

    private:
        BOOST_PP_REPEAT(
            MOCK_NUM_ARGS, MOCK_EXPECTATION_MEMBER, _)
};

    template< typename F, typename Signature > class multi_matcher;

    template< typename F,
        BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, typename T) >
    class multi_matcher< F, void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) ) >
        : public matcher_base< void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) ) >
    {
    public:
        multi_matcher( const F& f )
            : f_( f )
        {}

    private:
        virtual bool operator()(
            BOOST_PP_ENUM(MOCK_NUM_ARGS, MOCK_REF_ARG, _) )
        {
            return f_( BOOST_PP_ENUM(MOCK_NUM_ARGS, MOCK_EXPECTATION_PARAM, _) );
        }
        virtual void serialize( std::ostream& s ) const
        {
            s << mock::format( f_ );
        }

    private:
        F f_;
    };

#endif

    template< typename Signature > class expectation;

    template< typename R
        BOOST_PP_ENUM_TRAILING_PARAMS(MOCK_NUM_ARGS, typename T) >
    class expectation< R (BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS,T)) >
        : public action< R, R (BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS,T)) >
    {
    public:
        expectation()
            : invocation_( boost::make_shared< unlimited >() )
            , matcher_(
                boost::make_shared<
                    default_matcher<
                        void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) )
                    >
                > () )
            , file_( "unknown location" )
            , line_( 0 )
        {}
        expectation( const char* file, int line )
            : invocation_( boost::make_shared< unlimited >() )
            , matcher_(
                boost::make_shared<
                    default_matcher<
                        void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) )
                    >
                > () )
            , file_( file )
            , line_( line )
        {}

        ~expectation()
        {
            for( sequences_cit it = sequences_.begin();
                it != sequences_.end(); ++it )
                (*it)->remove( this );
        }

        void invoke( const boost::shared_ptr< invocation >& i )
        {
            invocation_ = i;
        }

#ifndef MOCK_NUM_ARGS_0
        template<
            BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, typename Constraint_)
        >
        expectation& with(
            BOOST_PP_ENUM_BINARY_PARAMS(MOCK_NUM_ARGS, Constraint_, c) )
        {
            matcher_.reset(
                new single_matcher<
                    void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, Constraint_) ),
                    void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) )
                >( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, c) ) );
            return *this;
        }
#if MOCK_NUM_ARGS > 1
        template< typename Constraint >
        expectation& with( const Constraint& c )
        {
            matcher_.reset(
                new multi_matcher<
                    Constraint,
                    void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) )
                >( c ) );
            return *this;
        }
#endif
#endif

        void add( sequence& s )
        {
            s.impl_->add( this );
            sequences_.push_back( s.impl_ );
        }

        bool verify() const
        {
            return invocation_->verify();
        }

        bool is_valid(
            BOOST_PP_ENUM(MOCK_NUM_ARGS, MOCK_REF_ARG, _) ) const
        {
            return !invocation_->exhausted()
                && (*matcher_)( BOOST_PP_ENUM(MOCK_NUM_ARGS, MOCK_EXPECTATION_PARAM, _) );
        }

        bool invoke() const
        {
            for( sequences_cit it = sequences_.begin();
                it != sequences_.end(); ++it )
                if( ! (*it)->is_valid( this ) )
                    return false;
            bool result = invocation_->invoke();
            for( sequences_cit it = sequences_.begin();
                it != sequences_.end(); ++it )
                (*it)->invalidate( this );
            return result;
        }

        const char* file() const
        {
            return file_;
        }
        int line() const
        {
            return line_;
        }

        friend std::ostream& operator<<(
            std::ostream& s, const expectation& e )
        {
            return s << ( e.invocation_->exhausted() ? 'v' : '.' )
                << ' ' << *e.invocation_
#ifndef MOCK_NUM_ARGS_0
                << ".with( " << *e.matcher_ << " )"
#endif
                ;
        }

    private:
        typedef std::vector<
            boost::shared_ptr< sequence_impl >
        > sequences_type;
        typedef sequences_type::const_iterator sequences_cit;

        boost::shared_ptr< invocation > invocation_;
        boost::shared_ptr<
            matcher_base<
                void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) )
            >
        > matcher_;
        sequences_type sequences_;
        const char* file_;
        int line_;
    };
}
} // mock

#undef MOCK_EXPECTATION_INITIALIZE
#undef MOCK_EXPECTATION_MEMBER
#undef MOCK_EXPECTATION_IS_VALID
#undef MOCK_EXPECTATION_SERIALIZE
#undef MOCK_EXPECTATION_SERIALIZE_ANY
#undef MOCK_EXPECTATION_PARAM
#undef MOCK_REF_ARG
#undef MOCK_REF_ARG_T
#undef MOCK_RV_REF
