// Boost.Range library
//
//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_JOIN_HPP_INCLUDED
#define BOOST_RANGE_JOIN_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/range/detail/join_iterator.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/iterator_range.hpp>

namespace boost
{

template<class SinglePassRange1, class SinglePassRange2>
iterator_range<range_detail::join_iterator<
    BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange1>::type,
    BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange2>::type,
    BOOST_DEDUCED_TYPENAME add_const<
        BOOST_DEDUCED_TYPENAME range_value<const SinglePassRange1>::type>::type>
>
join(const SinglePassRange1& r1, const SinglePassRange2& r2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange2> ));

    typedef range_detail::join_iterator<
                BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange1>::type,
                BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange2>::type,
                BOOST_DEDUCED_TYPENAME add_const<
                    BOOST_DEDUCED_TYPENAME range_value<const SinglePassRange1>::type>::type> iterator_t;

    return iterator_range<iterator_t>(
        iterator_t(r1, r2, range_detail::join_iterator_begin_tag()),
        iterator_t(r1, r2, range_detail::join_iterator_end_tag()));
}

template<class SinglePassRange1, class SinglePassRange2>
iterator_range<range_detail::join_iterator<
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange1>::type,
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange2>::type,
    BOOST_DEDUCED_TYPENAME range_value<SinglePassRange1>::type>
>
join(SinglePassRange1& r1, SinglePassRange2& r2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange2> ));

    typedef range_detail::join_iterator<
        BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange1>::type,
        BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange2>::type,
        BOOST_DEDUCED_TYPENAME range_value<SinglePassRange1>::type> iterator_t;

    return iterator_range<iterator_t>(
        iterator_t(r1, r2, range_detail::join_iterator_begin_tag()),
        iterator_t(r1, r2, range_detail::join_iterator_end_tag()));
}

} // namespace boost

#endif // include guard
