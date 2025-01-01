/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file streamwrapper.h
 */

#ifndef STREAMWRAPPER_H
#define STREAMWRAPPER_H

#include <iostream>


#if defined( _WIN32 ) && defined( __GNUC__ )
#include <ext/stdio_filebuf.h>

#define OSTREAM std::ostream

#define OPEN_OSTREAM( var, name )                                                           \
    kicad::stream var##_BUF_;                                                               \
    std::ostream& var = *var##_BUF_.Open( name, std::ios_base::out | std::ios_base::trunc   \
                                                        | std::ios_base::binary )

#define OPEN_ISTREAM( var, name )                                                           \
    kicad::stream var##_BUF_;                                                               \
    std::istream& var = *var##_BUF_.Open( name, std::ios_base::in | std::ios_base::binary )

#define OPEN_IOSTREAM( var, name )                                                          \
    kicad::stream  var##_BUF_;                                                              \
    std::iostream& var = *var##_BUF_.Open( name, std::ios_base::out | std::ios_base::in     \
                                                         | std::ios_base::binary )

#define CLOSE_STREAM( var ) var##_BUF_.Close()

/**
 * \namespace kicad
 */
namespace kicad
{
/**
 * This is equivalent to std::stream but accepts UTF8 chars in filenames.
 */
class stream
{
private:
    __gnu_cxx::stdio_filebuf<char>* m_buf;
    std::iostream*                  m_stream;

public:
    stream();
    virtual ~stream();

    std::iostream* Open( const char* aFileName, std::ios_base::openmode aMode );
    void           Close( void );

    std::iostream* GetStream( void );
};
} // namespace kicad


#elif defined( _MSC_VER ) // defined( _WIN32 ) && defined( __GNUC__ )

#define OSTREAM std::ofstream

#define OPEN_OSTREAM( var, name )                                                           \
    std::ofstream var;                                                                      \
    var.open( wxString::FromUTF8Unchecked( name ).wc_str(),                                 \
              std::ios_base::out | std::ios_base::trunc | std::ios_base::binary )

#define OPEN_ISTREAM( var, name )                                                           \
    std::ifstream var;                                                                      \
    var.open( wxString::FromUTF8Unchecked( name ).wc_str(),                                 \
              std::ios_base::in | std::ios_base::binary )

#define OPEN_IOSTREAM( var, name )                                                          \
    std::fstream var;                                                                       \
    var.open( wxString::FromUTF8Unchecked( name ).wc_str(),                                 \
              std::ios_base::out | std::ios_base::in | std::ios_base::binary )

#define CLOSE_STREAM( var ) var.close()

#else // defined( _WIN32 ) && defined( __GNUC__ )

#define OSTREAM std::ofstream

#define OPEN_OSTREAM( var, name )                                                           \
    std::ofstream var;                                                                      \
    var.open( name, std::ios_base::out | std::ios_base::trunc )

#define OPEN_ISTREAM( var, name )                                                           \
    std::ifstream var;                                                                      \
    var.open( name, std::ios_base::in )

#define OPEN_IOSTREAM( var, name )                                                          \
    std::fstream var;                                                                       \
    var.open( name, std::ios_base::out | std::ios_base::in )

#define CLOSE_STREAM( var ) var.close()

#endif // defined( _WIN32 ) && defined( __GNUC__ )

#endif  // STREAMWRAPPER_H
