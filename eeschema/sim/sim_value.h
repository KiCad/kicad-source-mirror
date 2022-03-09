/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <core/optional.h>
#include <memory>
#include <pegtl.hpp>


class SIM_VALUE_BASE
{
public:
    enum class TYPE
    {
        BOOL,
        INT,
        FLOAT,
        COMPLEX,
        STRING,

        BOOL_VECTOR,
        INT_VECTOR,
        FLOAT_VECTOR,
        COMPLEX_VECTOR
    };

    static std::unique_ptr<SIM_VALUE_BASE> Create( TYPE aType, wxString aString );
    static std::unique_ptr<SIM_VALUE_BASE> Create( TYPE aType );

    virtual ~SIM_VALUE_BASE() = default;
    SIM_VALUE_BASE() = default;

    void operator=( const wxString& aString );
    virtual bool operator==( const SIM_VALUE_BASE& aOther ) const = 0;

    virtual void FromString( const wxString& aString ) = 0;
    virtual wxString ToString() const = 0;

    // For parsers that don't accept strings with our suffixes.
    virtual wxString ToSimpleString() const = 0;
};


template <typename T>
class SIM_VALUE : public SIM_VALUE_BASE
{
public:
    SIM_VALUE() = default;
    SIM_VALUE( const T& aValue );

    void FromString( const wxString& aString ) override;
    wxString ToString() const override;
    wxString ToSimpleString() const override;

    void operator=( const T& aValue );
    bool operator==( const SIM_VALUE_BASE& aOther ) const override;

private:
    OPT<T> m_value = NULLOPT;

    wxString getMetricSuffix();
};


namespace SIM_VALUE_PARSER
{
    using namespace tao::pegtl;

    enum class NOTATION
    {
        SI,
        SPICE
    };

    template <NOTATION Notation>
    wxString allowedIntChars;


    struct spaces : plus<space> {};
    struct digits : plus<tao::pegtl::digit> {}; // For some reason it fails on just "digit".

    struct sign : one<'+', '-'> {};

    struct intPart : digits {};

    //struct fracPartPrefix : one<'.'> {};
    struct fracPart : digits {};
    //struct fracPartWithPrefix : seq<fracPartPrefix, fracPart> {};


    template <SIM_VALUE_BASE::TYPE ValueType>
    struct significand;
    
    template <> struct significand<SIM_VALUE_BASE::TYPE::FLOAT> :
        sor<seq<intPart, one<'.'>, fracPart>,
            seq<intPart, one<'.'>>,
            intPart,
            seq<one<'.'>, fracPart>,
            one<'.'>> {};

    template <> struct significand<SIM_VALUE_BASE::TYPE::INT> : intPart {};


    struct exponentPrefix : one<'e', 'E'> {};
    struct exponent : seq<opt<sign>, opt<digits>> {};
    struct exponentWithPrefix : seq<exponentPrefix, exponent> {};


    template <SIM_VALUE_BASE::TYPE ValueType, NOTATION Notation>
    struct metricSuffix;

    template <> struct metricSuffix<SIM_VALUE_BASE::TYPE::INT, NOTATION::SI>
        : one<'k', 'K', 'M', 'G', 'T', 'P', 'E'> {};
    template <> struct metricSuffix<SIM_VALUE_BASE::TYPE::INT, NOTATION::SPICE>
        : sor<TAO_PEGTL_ISTRING( "k" ),
              TAO_PEGTL_ISTRING( "Meg" ),
              TAO_PEGTL_ISTRING( "G" ),
              TAO_PEGTL_ISTRING( "T" )> {};

    template <> struct metricSuffix<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SI>
        : one<'a', 'f', 'p', 'n', 'u', 'm', 'k', 'K', 'M', 'G', 'T', 'P', 'E'> {};
    template <> struct metricSuffix<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SPICE>
        : sor<TAO_PEGTL_ISTRING( "f" ),
              TAO_PEGTL_ISTRING( "p" ),
              TAO_PEGTL_ISTRING( "n" ),
              TAO_PEGTL_ISTRING( "u" ),
              TAO_PEGTL_ISTRING( "m" ),
              //TAO_PEGTL_ISTRING( "mil" ),
              TAO_PEGTL_ISTRING( "k" ),
              TAO_PEGTL_ISTRING( "Meg" ),
              TAO_PEGTL_ISTRING( "G" ),
              TAO_PEGTL_ISTRING( "T" )> {};


    template <SIM_VALUE_BASE::TYPE ValueType, NOTATION Notation>
    struct number : seq<significand<ValueType>,
                        opt<exponentWithPrefix>,
                        opt<metricSuffix<ValueType, Notation>>> {};

    template <SIM_VALUE_BASE::TYPE ValueType, NOTATION Notation>
    struct numberGrammar : must<opt<number<ValueType, Notation>>, eof> {};


    template <typename Rule>
    struct numberSelector : std::false_type {};

    template <> struct numberSelector<significand<SIM_VALUE_BASE::TYPE::INT>> : std::true_type {};
    template <> struct numberSelector<significand<SIM_VALUE_BASE::TYPE::FLOAT>> : std::true_type {};
    template <> struct numberSelector<intPart> : std::true_type {};
    template <> struct numberSelector<fracPart> : std::true_type {};
    template <> struct numberSelector<exponent> : std::true_type {};
    template <> struct numberSelector<metricSuffix<SIM_VALUE_BASE::TYPE::INT, NOTATION::SI>>
        : std::true_type {};
    template <> struct numberSelector<metricSuffix<SIM_VALUE_BASE::TYPE::INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct numberSelector<metricSuffix<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SI>>
        : std::true_type {};
    template <> struct numberSelector<metricSuffix<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SPICE>>
        : std::true_type {};

    struct PARSE_RESULT
    {
        bool isEmpty = true;
        std::string significand;
        OPT<long> intPart;
        OPT<long> fracPart;
        OPT<long> exponent;
        OPT<long> metricSuffixExponent;
    };

    bool IsValid( const wxString& aString,
                  SIM_VALUE_BASE::TYPE aValueType = SIM_VALUE_BASE::TYPE::FLOAT,
                  NOTATION aNotation = NOTATION::SI );
    PARSE_RESULT Parse( const wxString& aString,
                        SIM_VALUE_BASE::TYPE aValueType = SIM_VALUE_BASE::TYPE::FLOAT,
                        NOTATION aNotation = NOTATION::SI );

    long MetricSuffixToExponent( std::string aMetricSuffix, NOTATION aNotation = NOTATION::SI );
    wxString ExponentToMetricSuffix( double aExponent, long& aReductionExponent,
                                     NOTATION aNotation = NOTATION::SI );
}

#endif // SIM_VALUE_H
