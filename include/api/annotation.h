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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_API_ANNOTATION_H
#define KICAD_API_ANNOTATION_H

#include <reporter.h>
#include <wx/string.h>

struct ANNOTATION_RESULT
{
    int errorCount = 0;
    int warningCount = 0;
    int changeCount = 0;
    int newFootprintCount = 0;
};

class API_ANNOTATION_REPORTER : public WX_STRING_REPORTER
{
public:
    REPORTER& Report( const wxString& aText, SEVERITY aSeverity ) override
    {
        if( aSeverity == RPT_SEVERITY_ERROR )
            errorCount++;
        else if( aSeverity == RPT_SEVERITY_WARNING )
            warningCount++;

        return WX_STRING_REPORTER::Report( aText, aSeverity );
    }

    int errorCount = 0;
    int warningCount = 0;
};

#endif // KICAD_API_ANNOTATION_H
