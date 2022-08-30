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

#ifndef SPICE_GRAMMAR_H
#define SPICE_GRAMMAR_H

#include <sim/sim_value.h>


namespace SPICE_GRAMMAR
{
    using namespace SIM_VALUE_GRAMMAR;


    struct garbage : plus<one<' ', '\t', '=', '(', ')', ','>> {};
    struct leaders : plus<one<' ', '\t'>> {};
    struct trailers : plus<one<' ', '\t', '\v', '\f'>> {};

    // NOTE: In Ngspice, a '$' opening a comment must be preceded by ' ', ',', or '\t'. We don't
    //       implement that here - this may cause problems in the future.
    // Ngspice supports '//' for comments.
    struct eolCommentStart : sor<one<';', '$'>,
                                 string<'/', '/'>> {};

    struct eolComment : seq<eolCommentStart,
                            until<eol>> {};
                                            

    struct commentLine : seq<opt<garbage>,
                             one<'*'>,
                             until<eol>> {};


    struct newline : seq<sor<eol,
                             eolComment>,
                         not_at<one<'+'>>> {};

    struct backslashContinuation : seq<string<'\\', '\\'>,
                                       opt<trailers>,
                                       eol> {};

    struct commentBackslashContinuation : seq<eolCommentStart,
                                              seq<star<not_at<eol>,
                                                       not_at<string<'\\', '\\'>,
                                                              opt<trailers>,
                                                              eol>,
                                                       any>,
                                                  string<'\\', '\\'>,
                                                  opt<trailers>,
                                                  eol>> {};
                                                  

    struct plusContinuation : seq<sor<eol,
                                      eolComment>,
                                  star<commentLine>,
                                  opt<leaders>,
                                  one<'+'>> {};

    struct continuation : seq<opt<garbage>,
                              sor<backslashContinuation,
                                  commentBackslashContinuation,
                                  plusContinuation>,
                              opt<garbage>> {};
                             
                

    struct sep : sor<plus<continuation>,
                     garbage> {};

    // Ngspice has some heuristic logic to allow + and - in tokens. We mimic that here.
    struct tokenStart : seq<opt<one<'+', '-'>>,
                            opt<seq<star<sor<tao::pegtl::digit,
                                             one<'.'>>>,
                                    one<'e', 'E'>,
                                    opt<one<'+', '-'>>>>> {};

    struct token : seq<tokenStart,
                       star<not_at<eol>,
                            not_at<backslashContinuation>,
                            not_one<' ', '\t', '=', '(', ')', ',', '+', '-', '*', '/', '^', ';'>>>
        {};

    struct param : token {};
    struct paramValue : token {};

    struct paramValuePair : seq<param,
                                sep,
                                paramValue> {};

    struct paramValuePairs : seq<opt<paramValuePair>,
                                 star<sep,
                                      paramValuePair>> {};
    struct modelName : star<sor<alnum,
                                one<'!', '#', '$', '%', '[', ']', '_'>>> {};
                     /*seq<alpha,
                           star<sor<alnum,
                                    one<'!', '#', '$', '%', '[', ']', '_'>>>> {};*/
    /*struct dotModelType : sor<TAO_PEGTL_ISTRING( "R" ),
                              TAO_PEGTL_ISTRING( "C" ),
                              TAO_PEGTL_ISTRING( "L" ),
                              TAO_PEGTL_ISTRING( "SW" ),
                              TAO_PEGTL_ISTRING( "CSW" ),
                              TAO_PEGTL_ISTRING( "URC" ),
                              TAO_PEGTL_ISTRING( "LTRA" ),
                              TAO_PEGTL_ISTRING( "D" ),
                              TAO_PEGTL_ISTRING( "NPN" ),
                              TAO_PEGTL_ISTRING( "PNP" ),
                              TAO_PEGTL_ISTRING( "NJF" ),
                              TAO_PEGTL_ISTRING( "PJF" ),
                              TAO_PEGTL_ISTRING( "NMOS" ),
                              TAO_PEGTL_ISTRING( "PMOS" ),
                              TAO_PEGTL_ISTRING( "NMF" ),
                              TAO_PEGTL_ISTRING( "PMF" ),
                              TAO_PEGTL_ISTRING( "VDMOS" )> {};*/
    struct dotModelType : plus<alpha> {};
    struct dotModel : seq<opt<sep>,
                          TAO_PEGTL_ISTRING( ".model" ),
                          sep,
                          modelName,
                          sep,
                          dotModelType,
                          opt<sor<seq<opt<sep>,
                                      one<'('>,
                                      opt<sep>,
                                      paramValuePairs,
                                      opt<sep>,
                                      // Ngspice doesn't require the parentheses to match, though.
                                      one<')'>>,
                                  seq<sep,
                                      paramValuePairs>>>,
                          opt<sep>,
                          newline> {};


    struct dotSubcktPinName : seq<not_at<TAO_PEGTL_ISTRING( "params:" )>,
                                  plus<not_at<space>,
                                       any>> {};
    struct dotSubcktPinSequence : seq<opt<dotSubcktPinName,
                                          star<sep,
                                               dotSubcktPinName>>> {};
    struct dotSubcktParams : seq<TAO_PEGTL_ISTRING( "params:" ),
                                 sep,
                                 paramValuePairs> {};
    struct dotSubcktEnd : seq<TAO_PEGTL_ISTRING( ".ends" ),
                              until<newline>> {};
    struct spiceUnit;
    struct dotSubckt : seq<opt<sep>,
                           TAO_PEGTL_ISTRING( ".subckt" ),
                           sep,
                           modelName,
                           sep,
                           dotSubcktPinSequence,
                           opt<sep,
                               dotSubcktParams>,
                           opt<sep>,
                           newline,
                           until<dotSubcktEnd, spiceUnit>> {};


    struct modelUnit : sor<dotModel,
                           dotSubckt> {};


    struct dotTitleTitle : star<not_at<newline>, any> {};
    struct dotTitle : seq<opt<sep>,
                          TAO_PEGTL_ISTRING( ".title" ),
                          sep,
                          dotTitleTitle,
                          newline> {};


    struct dotIncludePathWithoutQuotes : star<not_one<'"'>> {};
    struct dotIncludePathWithoutApostrophes : star<not_one<'\''>> {};
    struct dotIncludePath : star<not_at<newline>, any> {};
    struct dotInclude : seq<opt<sep>,
                            TAO_PEGTL_ISTRING( ".include" ),
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


    struct dotLine : seq<opt<sep>,
                         one<'.'>,
                         until<newline>> {};

    struct unknownLine : until<newline> {};


    struct spiceUnit : sor<modelUnit,
                           dotTitle,
                           dotInclude,
                           dotLine,
                           unknownLine> {};
    struct spiceUnitGrammar : must<spiceUnit, tao::pegtl::eof> {};


    struct spiceSource : star<spiceUnit> {};
    struct spiceSourceGrammar : must<spiceSource, tao::pegtl::eof> {};
}

#endif // SPICE_GRAMMAR_H
