/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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


// This is a propertytree test utility.
// It can convert XML to sexpressions or beautify s-expressions in non-specctra mode.


#include <assert.h>
#include <ptree.h>
#include <richio.h>
#include <dsnlexer.h>
#include <macros.h>
#include <boost/property_tree/xml_parser.hpp>


void usage()
{
    fprintf( stderr, "Usage: parser_gen <grammar_s-expression_file>\n" );
    exit( 1 );
}


int main( int argc, char** argv )
{
    if( argc != 2 )
    {
        usage();
    }

    FILE*   fp = fopen( argv[1], "r" );
    if( !fp )
    {
        fprintf( stderr, "Unable to open '%s'\n", argv[1] );
        usage();
    }

    static const KEYWORD empty_keywords[1] = {};

    DSNLEXER   lexer( empty_keywords, 0, fp, FROM_UTF8( argv[1] ) );

    try
    {
        PTREE   doc;

#if 0
        using namespace boost::property_tree;

        read_xml( argv[1], doc, xml_parser::trim_whitespace | xml_parser::no_comments );
#else
        Scan( &doc, &lexer );
#endif

#if 1
        STRING_FORMATTER sf;
        Format( &sf, 0, 0, doc );
        printf( "%s", sf.GetString().c_str() );
#else
        // writing the unchanged ptree in file2.xml
        boost::property_tree::xml_writer_settings<char> settings( ' ', 2 );
        write_xml( "/tmp/output.xml", doc, std::locale(), settings );
#endif

    }
    catch( const IO_ERROR& ioe )
    {
        fprintf( stderr, "%s\n", TO_UTF8( ioe.errorText ) );
    }
}

