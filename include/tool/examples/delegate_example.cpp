/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <cstdio>
#include <string>

#include <tool/delegate.h>

class MyClass
{
public:
    int MyMethod( const string& arg )
    {
        return arg.length();
    }
};

typedef DELEGATE<int, const string&> MyDelegate;

main()
{
    MyClass t1;
    MyClass t2;

    MyDelegate ptr1( &t1, &MyClass::MyMethod );
    MyDelegate ptr2( &t2, &MyClass::MyMethod );

    int retval1, retval2;

    retval1 = ptr1( "apples" );
    retval2 = ptr2( "cherries" );

    return 0;
}
