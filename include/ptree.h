/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef PTREE_H_
#define PTREE_H_

/*
Implement "KiCad s-expression" support for boost::property_tree's ptree, the 8
bit string version of property_tree. The ram resident structure of the ptree is
mostly compatible with one created using the xml_parser from
boost::property_tree, with slight differences in the way atoms are stored. The
result is you can use Format() to convert from xml to s-expression, but not the
other way around. You can write a simple s-expression beautifier in just a few
lines of code.

The main value however is the s-expression parser, i.e. Scan(), which is an
alternative to crafting a custom recursive descent parser for a particular
grammar. The tipping point depends on whether you want to read only a small
portion of a much larger document. If so, then using the ptree will likely be a
"faster to code" route. Documentation on how to navigate a ptree can be found on
the boost website and there are a number of examples in the
pcbnew/pcb_io_eagle.cpp file in this project. Powerful path navigation support
makes it easy to extract a subset of a ptree.
*/


#include <boost/property_tree/ptree_fwd.hpp>
#include <richio.h>
#include <dsnlexer.h>

typedef boost::property_tree::ptree         PTREE;
typedef const PTREE                         CPTREE;
typedef boost::property_tree::ptree_error   PTREE_ERROR;

/**
 * Fill an empty #PTREE with information from a KiCad s-expression stream.
 *
 * Use a #DSNLEXER with an empty keyword table as @a aLexer.  Useful for parsing s-expression
 * files or strings of arbitrary grammars, say from a file or clipboard.  The s-expression
 * must be "KiCad compatible".  See Documentation/s-expressions.txt for this KiCad compatible
 * definition (it is the non-specctra mode).  And also see in tools/property_tree.cpp for
 * example usage.
 *
 * <code>
 *
 * FILE* fp = fopen( argv[1], "r" );
 *
 * static const KEYWORD empty_keywords[1] = {};
 *
 * DSNLEXER   lexer( empty_keywords, 0, fp, wxString( From_UTF8( argv[1] ) ) );
 *
 * try
 * {
 *     PTREE   doc;
 *     Scan( &doc, &lexer );
 * }
 * catch( const IO_ERROR& ioe )
 * {
 *     fprintf( stderr, "%s\n", TO_UTF8( ioe.errorText ) );
 * }
 *
 * </code>
 */
void Scan( PTREE* aTree, DSNLEXER* aLexer );

/**
 * Output a #PTREE into s-expression format via an #OUTPUTFORMATTER derivative.
 */
void Format( OUTPUTFORMATTER* out, int aNestLevel, int aCtl, const CPTREE& aTree );

#endif  // PTREE_H_
