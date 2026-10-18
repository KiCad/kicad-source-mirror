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

#include "import_design_rules.h"

#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/log.h>


bool SaveImportedDesignRules( const wxString& aRules, const wxFileName& aBoardPath, REPORTER& aReporter )
{
    if( aRules.IsEmpty() )
        return false;

    // a nameless board would put the rules in a hidden file
    if( aBoardPath.GetName().IsEmpty() )
    {
        aReporter.Report( _( "Imported design rules were not saved, the board has no name.\n" ), RPT_SEVERITY_WARNING );
        return false;
    }

    wxFileName rulesPath( aBoardPath );
    rulesPath.SetExt( FILEEXT::DesignRulesFileExtension );

    // Creating exclusively both refuses an existing file and closes the race a FileExists test would leave open
    wxFile rulesFile;

    {
        // Refusal is expected and reported below, so keep wx from raising its own error
        wxLogNull noLog;
        rulesFile.Open( rulesPath.GetFullPath(), wxFile::write_excl );
    }

    if( !rulesFile.IsOpened() )
    {
        aReporter.Report( wxString::Format( rulesPath.FileExists()
                                                    ? _( "Imported design rules were not saved, '%s' already exists.\n" )
                                                    : _( "Could not write imported design rules to '%s'\n" ),
                                            rulesPath.GetFullPath() ),
                          RPT_SEVERITY_WARNING );
        return false;
    }

    bool written = rulesFile.Write( aRules );

    if( !rulesFile.Close() )
        written = false;

    // A partial file would not parse, and because it is created exclusively it would block the next import
    if( !written )
    {
        wxRemoveFile( rulesPath.GetFullPath() );

        aReporter.Report( wxString::Format( _( "Could not write imported design rules to '%s'\n" ),
                                            rulesPath.GetFullPath() ),
                          RPT_SEVERITY_WARNING );
        return false;
    }

    return true;
}
