/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SPICE_LIBRARY_PARSER_H
#define SPICE_LIBRARY_PARSER_H

#include <wx/string.h>

class SIM_LIBRARY_SPICE;
class REPORTER;

class SPICE_LIBRARY_PARSER
{
public:
    SPICE_LIBRARY_PARSER( SIM_LIBRARY_SPICE &aLibrary, bool aForceFullParse ) :
            m_library( aLibrary )
    {};

    virtual ~SPICE_LIBRARY_PARSER()
    {};

    virtual void ReadFile( const wxString& aFilePath, REPORTER& firstPass );

protected:
    void parseFile( const wxString& aFilePath, REPORTER& aReporter,
                    std::vector<std::pair<std::string, std::string>>* aModelQueue );

private:
    SIM_LIBRARY_SPICE& m_library;
};

#endif // SPICE_LIBRARY_PARSER_H
