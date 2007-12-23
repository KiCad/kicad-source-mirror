
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


#include <cstdarg>
#include <cstdio>

#include <boost/ptr_container/ptr_vector.hpp>


#include "fctsys.h"
#include "pcbstruct.h"

#include "dsn.h"

namespace DSN {


/**
 * Class ELEM
 * is a base class for any DSN element.  It is not a parent node so it 
 * cannot contain other elements but it can be extended to hold fields 
 * for any DSN element which contains no other elements, only fields.
 */ 
class ELEM
{
protected:    
    DSN_T   type;

public:    
    
    ELEM( DSN_T aType ) :
       type( aType )
    {
    }
    
    virtual ~ELEM()
    {
        printf("~ELEM(%p %d)\n", this, Type() );
    }
    
    DSN_T   Type() { return type; }
    
    virtual void Test()
    {
        printf("virtual Test()\n" ); 
    }
};



/**
 * Class PARENT
 * is a base class holder for any DSN element.  It can contain other
 * elements, including elements derived from this class.
 */ 
 class PARENT : public ELEM
{
    //  see http://www.boost.org/libs/ptr_container/doc/ptr_sequence_adapter.html
    typedef boost::ptr_vector<ELEM> ELEM_ARRAY;
    
    ELEM_ARRAY                      kids;      ///< of pointers                   
   
    
public:

    PARENT( DSN_T aType ) :
       ELEM( aType )
    {
    }
    
    virtual ~PARENT()
    {
        printf("~PARENT(%p %d)\n", this, Type() );
    }

    //-----< list operations >--------------------------------------------
    
    void    Append( ELEM* aElem )
    {
        kids.push_back( aElem );
    }

    ELEM*   Replace( int aIndex, ELEM* aElem )
    {
        ELEM_ARRAY::auto_type ret = kids.replace( aIndex, aElem );
        return ret.release();
    }

    ELEM*   Remove( int aIndex )
    {
        ELEM_ARRAY::auto_type ret = kids.release( kids.begin()+aIndex );
        return ret.release();
    }
    
    ELEM& operator[]( int aIndex )
    {
        return kids[aIndex];
    }

    const ELEM& operator[]( int aIndex ) const
    {
        return kids[aIndex];
    }
    
    void    Insert( int aIndex, ELEM* aElem )
    {
        kids.insert( kids.begin()+aIndex, aElem );
    }
    
    ELEM* At( int aIndex )
    {
        return &kids[aIndex];
    }
    
    void    Delete( int aIndex )
    {
        kids.erase( kids.begin()+aIndex );
    }
        
};
   


 
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


} // namespace DSN


using namespace DSN;

// a test to verify some of the list management functions.

int main( int argc, char** argv )
{
    PARENT parent( T_pcb );

    PARENT* child = new PARENT( T_absolute );
    
    parent.Append( child );

    parent.At(0)->Test();
    
    child->Append( new ELEM( T_absolute ) );
    
}


//EOF
