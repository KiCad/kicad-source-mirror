// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define MOCK_REF_ARG(z, n, d) \
    typename ref_arg< T##n >::type

namespace mock
{
namespace detail
{
    template< typename Signature > class matcher_base;

    template<
        BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, typename T) >
    class matcher_base< void( BOOST_PP_ENUM_PARAMS(MOCK_NUM_ARGS, T) ) >
        : boost::noncopyable
    {
    public:
        virtual ~matcher_base() {}

        virtual bool operator()(
            BOOST_PP_ENUM(MOCK_NUM_ARGS, MOCK_REF_ARG, _) ) = 0;

        friend std::ostream& operator<<(
            std::ostream& s, const matcher_base& m )
        {
            m.serialize( s );
            return s;
        }

    private:
        virtual void serialize( std::ostream& ) const = 0;
    };
}
} // mock

#undef MOCK_REF_ARG
