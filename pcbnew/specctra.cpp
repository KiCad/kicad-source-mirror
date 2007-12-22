
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2007 Kicad Developers, see change_log.txt for contributors.
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

 
#include <stdarg.h>
#include <stdio.h>

#include "fctsys.h"
#include "pcbstruct.h"



 
 
/**
 * Class SPECCTRA_DB
 * holds a DSN data tree, usually coming from a DSN file.
 */
class SPECCTRA_DB
{
    FILE*   fp;    

    
    /**
     * Function print
     * formats and writes text to the output stream.
     * @param fmt A printf style format string.
     * @param ... a variable list of parameters that will get blended into 
     *  the output under control of the format string.
     */
    void print( const char* fmt, ... );


    
public:

    SPECCTRA_DB( FILE* aFile ) :
        fp( aFile )
    {
    }

    
    /**
     * Function Export
     * writes the given BOARD out as a SPECTRA DSN format file.
     * @param aBoard The BOARD to save.
     */
    void Export( BOARD* aBoard );
    
};



void SPECCTRA_DB::print( const char* fmt, ... )
{
    va_list     args;

    va_start( args, fmt );
    vfprintf( fp, fmt, args );
    va_end( args );
}


void SPECCTRA_DB::Export( BOARD* aBoard )
{
}


//EOF
