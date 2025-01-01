/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SIM_VALUE_H
#define SIM_VALUE_H

#include <wx/string.h>
#include <optional>
#include <complex>
#include <memory>
#include <pegtl.hpp>

// Undef some annoying defines in windows headers added by pegtl.hpp
// that can create issues in .cpp files, mainly on msys2
#if defined (__MINGW32__)
#if defined ( LoadLibrary )
    #undef LoadLibrary
#endif
#if defined ( GetClassInfo )
    #undef GetClassInfo
#endif
#endif


namespace SIM_VALUE_GRAMMAR
{
    using namespace tao::pegtl;

    enum class NOTATION
    {
        SI,
        SPICE
    };
}


class SIM_VALUE
{
public:
    using NOTATION = SIM_VALUE_GRAMMAR::NOTATION;

    // Names like BOOL, INT, FLOAT need to be prefixed or MS Windows compilers will complain (enum
    // class doesn't help).
    enum TYPE
    {
        TYPE_BOOL,
        TYPE_INT,
        TYPE_FLOAT,
        TYPE_COMPLEX,
        TYPE_STRING,

        TYPE_BOOL_VECTOR,
        TYPE_INT_VECTOR,
        TYPE_FLOAT_VECTOR,
        TYPE_COMPLEX_VECTOR
    };

    static std::string ConvertNotation( const std::string& aString, NOTATION aFromNotation,
                                        NOTATION aToNotation );

    static std::string Normalize( double aValue );

    static std::string ToSpice( const std::string& aString );

    static double ToDouble( const std::string& aString, double aDefault = NAN );

    static int ToInt( const std::string& aString, int aDefault = -1 );

    static bool Equal( double aLH, const std::string& aRH );
};


namespace SIM_VALUE_GRAMMAR
{
    template <NOTATION Notation>
    std::string allowedIntChars;


    struct digits : plus<tao::pegtl::digit> {}; // For some reason it fails on just "digit".

    struct sign : one<'+', '-'> {};

    struct intPart : seq<opt<sign>, digits> {};

    //struct fracPartPrefix : one<'.'> {};
    struct fracPart : digits {};
    //struct fracPartWithPrefix : seq<fracPartPrefix, fracPart> {};


    template <SIM_VALUE::TYPE ValueType>
    struct significand;

    template <> struct significand<SIM_VALUE::TYPE_FLOAT> :
        sor<seq<intPart, one<'.'>, fracPart>,
            seq<intPart, one<'.'>>,
            intPart,
            seq<one<'.'>, fracPart>,
            one<'.'>,
            one<'-'>> {};

    template <> struct significand<SIM_VALUE::TYPE_INT> : intPart {};


    struct exponentPrefix : one<'e', 'E'> {};
    struct exponent : seq<opt<sign>, opt<digits>> {};
    struct exponentWithPrefix : seq<exponentPrefix, exponent> {};


    template <SIM_VALUE::TYPE ValueType, NOTATION Notation>
    struct unitPrefix;

    template <> struct unitPrefix<SIM_VALUE::TYPE_INT, NOTATION::SI>
        : one<'k', 'K', 'M', 'G', 'T', 'P', 'E'> {};
    template <> struct unitPrefix<SIM_VALUE::TYPE_INT, NOTATION::SPICE>
        : sor<TAO_PEGTL_ISTRING( "k" ),
              TAO_PEGTL_ISTRING( "Meg" ),
              TAO_PEGTL_ISTRING( "G" ),
              TAO_PEGTL_ISTRING( "T" )> {};

    template <> struct unitPrefix<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>
        : one<'a', 'f', 'p', 'n', 'u', 'm', 'k', 'K', 'M', 'G', 'T', 'P', 'E'> {};
    template <> struct unitPrefix<SIM_VALUE::TYPE_FLOAT, NOTATION::SPICE>
        : sor<TAO_PEGTL_ISTRING( "f" ),
              TAO_PEGTL_ISTRING( "p" ),
              TAO_PEGTL_ISTRING( "n" ),
              TAO_PEGTL_ISTRING( "u" ),
              TAO_PEGTL_ISTRING( "Meg" ), // "Meg" must be before "m".
              TAO_PEGTL_ISTRING( "m" ),
              //TAO_PEGTL_ISTRING( "mil" ),
              TAO_PEGTL_ISTRING( "k" ),
              TAO_PEGTL_ISTRING( "G" ),
              TAO_PEGTL_ISTRING( "T" )> {};


    template <NOTATION Notation>
    struct garbageSuffix;

    template <> struct garbageSuffix<NOTATION::SI> : seq<> {};
    template <> struct garbageSuffix<NOTATION::SPICE> : star<alpha> {};


    template <SIM_VALUE::TYPE ValueType, NOTATION Notation>
    struct number : seq<opt<significand<ValueType>>,
                        opt<exponentWithPrefix>,
                        opt<unitPrefix<ValueType, Notation>>,
                        garbageSuffix<Notation>> {};

    template <SIM_VALUE::TYPE ValueType, NOTATION Notation>
    struct numberGrammar : must<number<ValueType, Notation>, tao::pegtl::eof> {};


    bool IsValid( const std::string& aString,
                  SIM_VALUE::TYPE aValueType = SIM_VALUE::TYPE_FLOAT,
                  NOTATION aNotation = NOTATION::SI );
}

#endif // SIM_VALUE_H
