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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef LOCALE_IO_H
#define LOCALE_IO_H

#include <kicommon.h>
#include <string>

class wxLocale;

/**
 * Instantiate the current locale within a scope in which you are expecting
 * exceptions to be thrown.
 *
 * The constructor sets a "C" language locale option, to read/print files with floating
 * point  numbers.  The destructor insures that the default locale is restored whether an
 * exception is thrown or not.
 */
class KICOMMON_API LOCALE_IO
{
public:
    LOCALE_IO();
    ~LOCALE_IO();

private:
    // The locale in use before switching to the "C" locale
    // (the locale can be set by user, and is not always the system locale)
    [[maybe_unused]]wxLocale* m_wxLocale;   // used when using wxLocale
    std::string m_user_locale;              // used when using setlocale or wxSetlocale
};

#endif
