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

#ifndef SPICE_GRAMMAR_H
#define SPICE_GRAMMAR_H

#include <sim/sim_value.h>


namespace SPICE_GRAMMAR
{
    using namespace SIM_VALUE_GRAMMAR;


    struct garbage : plus<one<' ', '\t', '=', '(', ')', ','>> {};
    struct leaders : plus<one<' ', '\t'>> {};
    struct trailers : plus<one<' ', '\t', '\v', '\f'>> {};

    struct garbageOrEolf : sor<garbage, eolf> {};

    // NOTE: In Ngspice, a '$' opening a comment must be preceded by ' ', ',', or '\t'. We don't
    //       implement that here - this may cause problems in the future.
    // Ngspice supports '//' for comments.
    struct eolfCommentStart : sor<one<';', '$'>,
                                 string<'/', '/'>> {};

    struct eolfComment : seq<eolfCommentStart,
                            until<eolf>> {};


    struct commentLine : seq<opt<garbage>,
                             one<'*'>,
                             until<eolf>> {};


    struct newline : seq<sor<eolf,
                             eolfComment>,
                         not_at<one<'+'>>> {};

    struct backslashContinuation : seq<string<'\\', '\\'>,
                                       opt<trailers>,
                                       eolf> {};

    struct commentBackslashContinuation : seq<eolfCommentStart,
                                              seq<star<not_at<eolf>,
                                                       not_at<string<'\\', '\\'>,
                                                              opt<trailers>,
                                                              eolf>,
                                                       any>,
                                                  string<'\\', '\\'>,
                                                  opt<trailers>,
                                                  eolf>> {};


    struct plusContinuation : seq<sor<eolf,
                                      eolfComment>,
                                  star<commentLine>,
                                  opt<leaders>,
                                  one<'+'>> {};

    struct continuation : seq<opt<garbage>,
                              sor<backslashContinuation,
                                  commentBackslashContinuation,
                                  plusContinuation>,
                              opt<garbage>> {};


    // Token separator.
    struct sep : sor<plus<continuation>,
                     garbage> {};

    struct modelName : plus<not_at<garbageOrEolf>, any> {};

    struct dotModelType : sor<// VDMOS models have a special syntax.
                              seq<TAO_PEGTL_ISTRING( "vdmos" ),
                                  sep,
                                  sor<TAO_PEGTL_ISTRING( "nchan" ),
                                      TAO_PEGTL_ISTRING( "pchan" )>>,
                              plus<not_at<garbageOrEolf>, any>> {};

    struct vectorExpr : seq<one<'['>,
                            star<not_one<']'>>,
                            one<']'>> {};

    struct bracedExpr : seq<one<'{'>,
                            star<not_one<'}'>>,
                            one<'}'>> {};

    // Ngspice has some heuristic logic to allow + and - in tokens. We replicate that here.
    struct tokenStart : seq<opt<one<'+', '-'>>,
                            opt<seq<star<sor<tao::pegtl::digit,
                                             one<'.'>>>,
                                    one<'e', 'E'>,
                                    opt<one<'+', '-'>>>>> {};

    struct token : seq<tokenStart,
                       star<not_at<eolf>,
                            not_at<backslashContinuation>,
                            not_one<' ', '\t', '=', '(', ')', ',', ';'>>> {};

    // Param names cannot be `token` because LTspice models contain spurious values without
    // parameter names, which we need to skip, and because tokens can include a very limited
    // subset of un-braced expressions
    struct param : identifier {};
    struct paramValue : sor<bracedExpr,
                            vectorExpr,
                            token> {};

    struct paramValuePair : seq<param,
                                sep,
                                paramValue> {};
    struct paramValuePairs : list<paramValuePair, sep> {};

    struct cplSep : opt<one<' '>> {};
    struct cplParamValue : sor<list<bracedExpr, cplSep>,
                               vectorExpr,
                               list<token, cplSep>> {};
    struct cplParamValuePair : seq<param,
                                   sep,
                                   cplParamValue> {};
    struct cplParamValuePairs : list<cplParamValuePair, sep> {};

    struct dotModelAko : seq<opt<sep>,
                             if_must<seq<TAO_PEGTL_ISTRING( ".model" ),
                                         sep,
                                         modelName,
                                         sep,
                                         TAO_PEGTL_ISTRING( "ako:" )>,
                                     opt<sep>,
                                     modelName,
                                     opt<sep,
                                         dotModelType>,
                                     opt<sep,
                                         paramValuePairs>,
                                     opt<sep>,
                                     newline>> {};

    struct dotModelCPL : seq<opt<sep>,
                             if_must<seq<TAO_PEGTL_ISTRING( ".model" ),
                                         sep,
                                         modelName,
                                         sep,
                                         TAO_PEGTL_ISTRING( "CPL" )>,
                                     opt<sep,
                                         cplParamValuePairs>,
                                     opt<sep>,
                                     newline>> {};

    struct dotModel : seq<opt<sep>,
                          if_must<TAO_PEGTL_ISTRING( ".model" ),
                                  sep,
                                  modelName,
                                  sep,
                                  dotModelType,
                                  opt<sep,
                                      paramValuePairs>,
                                  opt<sep>,
                                  newline>> {};

    struct dotSubcktParamValuePair : seq<not_at<eolfComment>,
                                         param,
                                         // TODO: Check if these `star<space>`s match Ngspice's
                                         // behavior.
                                         star<space>,
                                         opt<plusContinuation>,
                                         one<'='>,
                                         opt<plusContinuation>,
                                         star<space>,
                                         paramValue> {};

    struct dotSubcktParamValuePairs : list<dotSubcktParamValuePair, sep> {};

    struct dotSubcktParamsStart : sor<TAO_PEGTL_ISTRING( "params:" ),
                                      seq<newline,
                                          TAO_PEGTL_ISTRING( ".param" )>> {};

    struct dotSubcktParams : seq<opt<dotSubcktParamsStart>,
                                 opt<sep>,
                                 dotSubcktParamValuePairs> {};

    struct dotSubcktPinName : seq<not_at<dotSubcktParams>,
                                  not_at<eolfComment>,
                                  plus<not_at<space>, any>> {};

    struct dotSubcktPinSequence : list<dotSubcktPinName, sep> {};

    struct dotSubcktEnd : seq<opt<sep>,
                              TAO_PEGTL_ISTRING( ".ends" ),
                              until<newline>> {};

    struct spiceUnit;
    struct dotSubckt : seq<opt<sep>,
                           if_must<TAO_PEGTL_ISTRING( ".subckt" ),
                                   sep,
                                   modelName,
                                   opt<sep,
                                       dotSubcktPinSequence>,
                                   opt<sep,
                                       dotSubcktParams>,
                                   opt<sep>,
                                   newline,
                                   until<dotSubcktEnd,
                                         spiceUnit>>> {};


    struct modelUnit : seq<star<commentLine>,
                           sor<dotModelAko,
                               dotModelCPL,
                               dotModel,
                               dotSubckt>> {};


    // Intentionally no if_must<>.
    struct dotControl : seq<opt<sep>,
                            TAO_PEGTL_ISTRING( ".control" ),
                            until<TAO_PEGTL_ISTRING( ".endc" )>,
                            until<newline>> {};


    struct dotTitleTitle : star<not_at<newline>, any> {};
    // Intentionally no if_must<>.
    struct dotTitle : seq<opt<sep>,
                          TAO_PEGTL_ISTRING( ".title" ),
                          sep,
                          dotTitleTitle,
                          newline> {};


    struct dotIncludePathWithoutQuotes : star<not_one<'"'>> {};
    struct dotIncludePathWithoutApostrophes : star<not_one<'\''>> {};
    struct dotIncludePath : star<not_at<newline>, any> {};
    // Intentionally no if_must<>.
    struct dotInclude : seq<opt<sep>,
                            TAO_PEGTL_ISTRING( ".inc" ),
                            star<not_at<garbageOrEolf>, any>,
                            sep,
                            sor<seq<one<'\"'>,
                                    dotIncludePathWithoutQuotes,
                                    one<'\"'>>,
                                seq<one<'\''>,
                                    dotIncludePathWithoutApostrophes,
                                    one<'\''>>,
                                dotIncludePath>,
                            opt<sep>,
                            newline> {};


    // Intentionally no if_must<>.
    struct dotLine : seq<opt<sep>,
                         one<'.'>,
                         until<newline>> {};


    // Intentionally no if_must<>.
    struct kLine : seq<opt<sep>,
                       one<'K'>,
                       until<sep>,
                       one<'L'>,
                       until<sep>,
                       one<'L'>,
                       until<sep>,
                       until<newline>> {};

    struct unknownLine : seq<plus<not_at<newline>, any>,
                             until<newline>> {};


    struct spiceUnit : sor<modelUnit,
                           dotControl,
                           dotTitle,
                           dotInclude,
                           dotLine,
                           kLine,
                           eol, // Empty line. This is necessary to terminate on EOF.
                           unknownLine> {};
    struct spiceUnitGrammar : must<spiceUnit> {};


    struct spiceSource : star<spiceUnit> {};
    struct spiceSourceGrammar : must<spiceSource> {};


    template <typename> inline constexpr const char* errorMessage = nullptr;
    template <> inline constexpr auto errorMessage<newline> =
            "expected newline";
    template <> inline constexpr auto errorMessage<sep> =
            "expected token separator (one or more whitespace, parenthesis, '=', ',', or line continuation)";
    template <> inline constexpr auto errorMessage<opt<sep>> =
            "";
    template <> inline constexpr auto errorMessage<modelName> =
            "expected model name";
    template <> inline constexpr auto errorMessage<dotModelType> =
            "expected model type";
    template <> inline constexpr auto errorMessage<opt<sep, dotModelType>> =
            "";
    template <> inline constexpr auto errorMessage<opt<sep, paramValuePairs>> =
            "";
    template <> inline constexpr auto errorMessage<opt<sep, cplParamValuePairs>> =
            "";
    template <> inline constexpr auto errorMessage<opt<sep, dotSubcktPinSequence>> =
            "";
    template <> inline constexpr auto errorMessage<opt<sep, dotSubcktParams>> =
            "";
    template <> inline constexpr auto errorMessage<until<dotSubcktEnd, spiceUnit>> =
            "expected (possibly empty) sequence of Spice lines followed by an .ends line";
    template <> inline constexpr auto errorMessage<spiceUnit> =
            "expected Spice directive, item, subcircuit definitions, or empty or commented-out line";
    template <> inline constexpr auto errorMessage<spiceSource> =
            "expected zero or more Spice directives, items, subcircuit definitions, or empty or commented-out lines";

    // We create a custom PEGTL control to modify the parser error messages.
    struct error
    {
        template <typename Rule> static constexpr bool raise_on_failure = false;
        template <typename Rule> static constexpr auto message = errorMessage<Rule>;
    };

    template <typename Rule> using control = must_if<error>::control<Rule>;
}

#endif // SPICE_GRAMMAR_H
