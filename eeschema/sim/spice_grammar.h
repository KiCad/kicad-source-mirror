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


    struct eolComment : seq<one<';'>, until<eol>> {};
    struct commentLine : seq<one<'*', ';'>, until<eol>> {};


    struct linespaces : plus<not_at<eol>,
                             space> {};
    struct newline : seq<sor<eol,
                             eolComment>,
                         not_at<one<'+'>>> {};
                         
    struct continuation : seq<opt<linespaces>,
                              sor<eol,
                                  eolComment>,
                              star<commentLine>,
                              one<'+'>,
                              opt<linespaces>> {};

    struct sep : sor<continuation,
                     linespaces> {};


    struct param : plus<alnum> {};
    struct suffixUnit : plus<alpha> {};

    template <SIM_VALUE::TYPE Type, NOTATION Notation>
    struct paramValuePair : seq<param,
                                opt<sep>,
                                one<'='>,
                                opt<sep>,
                                number<Type, Notation>,
                                opt<suffixUnit>> {};
    template <NOTATION Notation>
    struct paramValuePairs : seq<opt<paramValuePair<SIM_VALUE::TYPE::FLOAT,
                                                    Notation>,
                                     star<sep,
                                          paramValuePair<SIM_VALUE::TYPE::FLOAT,
                                                         Notation>>>> {};
    struct modelName : plus<alnum,
                            star<sor<alnum,
                                     one<'!', '#', '$', '%', '[', ']', '_'>>>> {};
                     /*seq<alpha,
                           star<sor<alnum,
                                    one<'!', '#', '$', '%', '[', ']', '_'>>>> {};*/
    struct dotModelType : sor<TAO_PEGTL_ISTRING( "R" ),
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
                              TAO_PEGTL_ISTRING( "VDMOS" )> {};
    struct dotModel : seq<opt<sep>,
                          TAO_PEGTL_ISTRING( ".model" ),
                          sep,
                          modelName,
                          sep,
                          dotModelType,
                          sor<seq<opt<sep>,
                                  one<'('>,
                                  opt<sep>,
                                  paramValuePairs<NOTATION::SPICE>,
                                  opt<sep>,
                                  // Ngspice doesn't require the parentheses to match, though.
                                  one<')'>>,
                              seq<sep,
                                  paramValuePairs<NOTATION::SPICE>>>,
                          opt<sep>,
                          newline> {};


    struct dotSubcktPinName : seq<not_at<TAO_PEGTL_ISTRING( "params:" )>,
                                  plus<not_at<space>,
                                       any>> {};
    struct dotSubcktPinSequence : seq<opt<dotSubcktPinName,
                                          star<sep,
                                               dotSubcktPinName>>> {};
    struct dotSubcktEnd : seq<TAO_PEGTL_ISTRING( ".ends" ),
                              opt<sep>,
                              newline> {};
    struct dotSubckt : seq<opt<sep>,
                           TAO_PEGTL_ISTRING( ".subckt" ),
                           sep,
                           modelName,
                           sep,
                           dotSubcktPinSequence,
                           opt<sep,
                               TAO_PEGTL_ISTRING( "params:" ),
                               sep,
                               paramValuePairs<NOTATION::SPICE>>,
                           opt<sep>,
                           newline,
                           until<dotSubcktEnd>> {};


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
    struct spiceUnitGrammar : must<spiceUnit, eof> {};


    struct spiceSource : star<spiceUnit> {};
    struct spiceSourceGrammar : must<spiceSource, eof> {};
}

#endif // SPICE_GRAMMAR_H
