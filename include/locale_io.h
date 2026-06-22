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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LOCALE_IO_H
#define LOCALE_IO_H

#include <kicommon.h>

/**
 * Instantiate the current locale within a scope in which you are expecting
 * exceptions to be thrown.
 *
 * The constructor sets a "C" language locale option, to read/print files with floating
 * point  numbers.  The destructor insures that the default locale is restored whether an
 * exception is thrown or not.
 *
 * The nesting count and the process-global locale transition it drives are serialized with a
 * mutex, so instances may safely be created and destroyed from multiple threads at once.
 */
class KICOMMON_API LOCALE_IO
{
public:
    LOCALE_IO();
    ~LOCALE_IO();

    LOCALE_IO( const LOCALE_IO& ) = delete;
    LOCALE_IO& operator=( const LOCALE_IO& ) = delete;
};

#endif
