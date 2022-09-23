/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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

#include <eda_tools.h>
#include <wx/textfile.h>

bool IsFileFromEDATool( const wxFileName& aFileName, const EDA_TOOLS aTool )
{
    wxTextFile textFile;

    if( textFile.Open( aFileName.GetFullPath() ) )
    {
        switch( aTool )
        {
        case EDA_TOOLS::EAGLE:
            if( textFile.GetLineCount() > 2
                && textFile[1].StartsWith( wxT( "<!DOCTYPE eagle SYSTEM" ) )
                && textFile[2].StartsWith( wxT( "<eagle version" ) ) )
            {
                textFile.Close();
                return true;
            }
            break;

        default: break;
        }
        textFile.Close();
    }

    return false;
}
