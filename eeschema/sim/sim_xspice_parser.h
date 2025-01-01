/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#ifndef SIM_XSPCIE_PARSER_H_
#define SIM_XSPCIE_PARSER_H_

#include "sim/sim_value.h"
#include <pegtl.hpp>
#include <pegtl/eol.hpp>
#include <pegtl/rules.hpp>

namespace SIM_XSPICE_PARSER_GRAMMAR
{
using namespace SIM_VALUE_GRAMMAR;

/**
* Notes:
*  spaces are allowed everywhere in any number
*  ~ can only be before ?
*  ~~ is not allowed
*  [] can enclose as many '?' with modifiers as we want
*  nested vectors are not allowed [ [?] ? ]
*  () and spaces are allowed and treated as separators
*  we want at least one node '?'
**/
struct nodeName : one<'?'>
{
};
struct squareBracketO : one<'['>
{
};
struct squareBracketC : one<']'>
{
};
struct invertionDigital : one<'~'>
{
};
struct sep : opt<plus<sor<space,
                          one<'('>,
                          one<')'>>>>
{
};

struct invertionSeparated : seq<sep, invertionDigital, sep>
{
};
struct portInversionDouble : if_must<invertionSeparated, not_at<invertionSeparated>>
{
};
struct portInversionVector : if_must<portInversionDouble, not_at<squareBracketO>>
{
};
struct portInversion : if_must<portInversionVector, not_at<one<'%'>>>
{
};

struct portModifiersSingleNames : sor<istring<'v', 'n', 'a', 'm'>,
                                      string<'v'>,
                                      istring<'i'>,
                                      istring<'g'>,
                                      istring<'h'>,
                                      istring<'d'>>
{
};
struct portModifierDifferentialNames
        : sor<istring<'v', 'd'>,
              istring<'i', 'd'>,
              istring<'g', 'd'>,
              istring<'h', 'd'>>
{
};

struct portModifierDigital : seq<one<'%'>, sep, istring<'d'>>
{
};
struct portModifiersSingle : seq<one<'%'>, sep, portModifiersSingleNames>
{
};
struct portModifiersDifferential : seq<one<'%'>, sep, portModifierDifferentialNames>
{
};
struct validPortTypes
        : until<if_must<one<'%'>, sep,
                        sor<portModifierDifferentialNames,
                            portModifiersSingleNames,
                            istring<'d'>>>>
{
};

struct nodeNameSeparated : seq<sep, nodeName, sep>
{
};
struct nodeDigital
        : seq<sep, opt<portModifierDigital>, opt<portInversion>, rep_min<1, nodeNameSeparated>>
{
};
struct nodeSingle : seq<sep, if_must<portModifiersSingle, sep, rep_min<1, nodeNameSeparated>>>
{
};
struct nodeDifferential
        : seq<sep, if_must<portModifiersDifferential, sep, rep_min<2, nodeNameSeparated>>>
{
};
struct nodeSequence : sor<nodeDifferential,
                          nodeDigital,
                          nodeSingle>
{
};

struct vectorPattern : if_must<squareBracketO, until<squareBracketC, nodeSequence>>
{
};
struct vectorExpr : seq<opt<portModifierDigital>, opt<portModifiersDifferential>,
                        opt<portModifiersSingle>, sep, vectorPattern>
{
};
struct nodeSequenceGrammar : must<at<rep_min<0, validPortTypes>>, sep,
                                  plus<sor<vectorExpr,
                                           nodeSequence>>,
                                       not_at<squareBracketC>>
{
};

template <typename>
inline constexpr const char* errorMessage = nullptr;
template <>
inline constexpr auto errorMessage<plus<sor<vectorExpr, nodeSequence>>> =
        "Expected at least one '?', are all modifiers and vectors correct?";
template <>
inline constexpr auto errorMessage<until<squareBracketC, nodeSequence>> =
        "Vectors [ must be closed ] and not nested.";
template <>
inline constexpr auto
        errorMessage<sor<portModifierDifferentialNames, portModifiersSingleNames, istring<'d'>>> =
                "Port type is invalid. '%%' needs to be followed by a valid name.";
template <>
inline constexpr auto errorMessage<at<rep_min<0, validPortTypes>>> = "";
template <>
inline constexpr auto errorMessage<rep_min<1, nodeNameSeparated>> =
        "Port type is invalid. '%%' needs to be followed by a valid name and a '?'.";
template <>
inline constexpr auto errorMessage<not_at<invertionSeparated>> = "'~~' is not supported.";
template <>
inline constexpr auto errorMessage<not_at<one<'%'>>> =
        "'~ %%d' not supported, consider changing to '%%d ~'.";
template <>
inline constexpr auto errorMessage<rep_min<2, nodeNameSeparated>> =
        "Differential ports need two nodes, and '~' is not supported for those nodes. Also check "
        "if port modifier name is valid.";
template <>
inline constexpr auto errorMessage<not_at<squareBracketO>> = "'~[' not supported.";
template <>
inline constexpr auto errorMessage<not_at<squareBracketC>> =
        "Vector is either empty, open or nested.";
template <>
inline constexpr auto errorMessage<sep> = "";


struct error
{
    template <typename Rule>
    static constexpr bool raise_on_failure = false;
    template <typename Rule>
    static constexpr auto message = errorMessage<Rule>;
};
template <typename Rule>
using control = must_if<error>::control<Rule>;


template <typename Rule>
struct spiceUnitSelector : std::false_type
{
};
template <>
struct spiceUnitSelector<squareBracketO> : std::true_type
{
};
template <>
struct spiceUnitSelector<portModifierDigital> : std::true_type
{
};
template <>
struct spiceUnitSelector<portModifiersSingle> : std::true_type
{
};
template <>
struct spiceUnitSelector<portModifiersDifferential> : std::true_type
{
};
template <>
struct spiceUnitSelector<invertionDigital> : std::true_type
{
};
template <>
struct spiceUnitSelector<squareBracketC> : std::true_type
{
};
template <>
struct spiceUnitSelector<nodeName> : std::true_type
{
};

} // namespace SIM_XSPICE_PARSER_GRAMMAR
#endif // SIM_XSPCIE_PARSER_H_