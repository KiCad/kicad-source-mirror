// Copyright (c) 2017-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_CONFIG_HPP
#define TAO_PEGTL_CONFIG_HPP

#if defined( TAO_PEGTL_NAMESPACE )
#pragma message( "TAO_PEGTL_NAMESPACE is deprecated" )
#else
#define TAO_PEGTL_NAMESPACE tao::pegtl
#endif

#endif
