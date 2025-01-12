/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <env_paths.h>
#include <project.h>
#include <wx/filename.h>

static bool normalizeAbsolutePaths( const wxFileName& aPathA, const wxFileName& aPathB,
                                    wxString* aResultPath )
{
    wxCHECK_MSG( aPathA.IsAbsolute(), false,
                 aPathA.GetPath() + wxS( " is not an absolute path." ) );
    wxCHECK_MSG( aPathB.IsAbsolute(), false,
                 aPathB.GetPath() + wxS( " is not an absolute path." ) );

    if( aPathA.GetPath() == aPathB.GetPath() )
        return true;

    // Not sure all of volume checks are necessary since wxFileName::GetVolume() returns
    // an empty string if the path has no volume.
    if( ( aPathA.GetDirCount() > aPathB.GetDirCount() )
      || ( aPathA.HasVolume() && !aPathB.HasVolume() )
      || ( !aPathA.HasVolume() && aPathB.HasVolume() )
      || ( ( aPathA.HasVolume() && aPathB.HasVolume() )
         && ( aPathA.GetVolume().CmpNoCase( aPathB.GetVolume() ) != 0 ) ) )
        return false;

    wxArrayString aDirs = aPathA.GetDirs();
    wxArrayString bDirs = aPathB.GetDirs();

    size_t i = 0;

    while( i < aDirs.GetCount() )
    {
        if( aDirs[i] != bDirs[i] )
            return false;

        i++;
    }

    if( aResultPath )
    {
        while( i < bDirs.GetCount() )
        {
            *aResultPath += bDirs[i] + wxT( "/" );
            i++;
        }
    }

    return true;
}


wxString NormalizePath( const wxFileName& aFilePath, const ENV_VAR_MAP* aEnvVars,
                        const wxString& aProjectPath )
{
    wxFileName envPath;
    wxString   varName;
    wxString   remainingPath;
    wxString   normalizedFullPath;
    int        pathDepth = 0;

    if( aEnvVars )
    {
        for( const std::pair<const wxString, ENV_VAR_ITEM>& entry : *aEnvVars )
        {
            // Don't bother normalizing paths that don't exist or the user cannot read.
            if( !wxFileName::DirExists( entry.second.GetValue() )
                    || !wxFileName::IsDirReadable( entry.second.GetValue() ) )
            {
                continue;
            }

            envPath.SetPath( entry.second.GetValue() );

            wxString tmp;

            if( normalizeAbsolutePaths( envPath, aFilePath, &tmp ) )
            {
                int newDepth = envPath.GetDirs().GetCount();

                // Only use the variable if it removes more directories than the previous ones
                if( newDepth > pathDepth )
                {
                    pathDepth     = newDepth;
                    varName       = entry.first;
                    remainingPath = tmp;
                }

                // @fixme Shouldn't we break here if an environment variable path is found or try
                //        at least try to pick the best match?
            }
        }
    }

    if( varName.IsEmpty() && !aProjectPath.IsEmpty()
        && wxFileName( aProjectPath ).IsAbsolute() && wxFileName( aFilePath ).IsAbsolute() )
    {
        envPath.SetPath( aProjectPath );

        if( normalizeAbsolutePaths( envPath, aFilePath, &remainingPath ) )
            varName = PROJECT_VAR_NAME;
    }

    if( varName.IsEmpty() )
    {
        normalizedFullPath = aFilePath.GetFullPath();
    }
    else
    {
        normalizedFullPath = wxString::Format( "${%s}/", varName );

        if( !remainingPath.IsEmpty() )
            normalizedFullPath += remainingPath;

        normalizedFullPath += aFilePath.GetFullName();
    }

    return normalizedFullPath;
}


wxString NormalizePath( const wxFileName& aFilePath, const ENV_VAR_MAP* aEnvVars,
                        const PROJECT* aProject )
{
    if( aProject )
        return NormalizePath( aFilePath, aEnvVars, aProject->GetProjectPath() );
    else
        return NormalizePath( aFilePath, aEnvVars, "" );
}


// Create file path by appending path and file name. This approach allows the filename
// to contain a relative path, whereas wxFileName::SetPath() would replace the
// relative path
static wxString createFilePath( const wxString& aPath, const wxString& aFileName )
{
    wxString path( aPath );

    if( !path.EndsWith( wxFileName::GetPathSeparator() ) )
        path.Append( wxFileName::GetPathSeparator() );

    return path + aFileName;
}


wxString ResolveFile( const wxString& aFileName, const ENV_VAR_MAP* aEnvVars,
                      const PROJECT* aProject )
{
    wxFileName full( aFileName );

    if( full.IsAbsolute() )
        return full.GetFullPath();

    if( aProject )
    {
        wxFileName fn( createFilePath( aProject->GetProjectPath(), aFileName ) );

        if( fn.Exists() )
            return fn.GetFullPath();
    }

    if( aEnvVars )
    {
        for( const std::pair<const wxString, ENV_VAR_ITEM>& entry : *aEnvVars )
        {
            wxFileName fn( createFilePath( entry.second.GetValue(), aFileName ) );

            if( fn.Exists() )
                return fn.GetFullPath();
        }
    }

    return wxEmptyString;
}


bool PathIsInsideProject( const wxString& aFileName, const PROJECT* aProject, wxFileName* aSubPath )
{
    wxFileName fn( aFileName );
    wxFileName prj( aProject->GetProjectPath() );

    wxArrayString pdirs = prj.GetDirs();
    wxArrayString fdirs = fn.GetDirs();

    if( fdirs.size() < pdirs.size() )
        return false;

    for( size_t i = 0; i < pdirs.size(); i++ )
    {
        if( fdirs[i] != pdirs[i] )
            return false;
    }

    // Now we know that fn is inside prj
    if( aSubPath )
    {
        aSubPath->Clear();

        for( size_t i = pdirs.size(); i < fdirs.size(); i++ )
            aSubPath->AppendDir( fdirs[i] );
    }

    return true;
}
