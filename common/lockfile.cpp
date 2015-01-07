/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014-2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <wx/filename.h>
#include <wx/snglinst.h>
#include <common.h>


wxSingleInstanceChecker* LockFile( const wxString& aFileName )
{
    // first make absolute and normalize, to avoid that different lock files
    // for the same file can be created
    wxFileName fn( aFileName );

    fn.MakeAbsolute();

    wxString lockFileName = fn.GetFullPath() + wxT( ".lock" );

    lockFileName.Replace( wxT( "/" ), wxT( "_" ) );

    // We can have filenames coming from Windows, so also convert Windows separator
    lockFileName.Replace( wxT( "\\" ), wxT( "_" ) );

    wxSingleInstanceChecker* p = new wxSingleInstanceChecker( lockFileName,
                                                              GetKicadLockFilePath() );

    if( p->IsAnotherRunning() )
    {
        delete p;
        p = NULL;
    }

    return p;
}

