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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "model_substitution_helpers.h"

#include <list>
#include <unordered_set>

#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/filename.h>

#include <filename_resolver.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <settings/common_settings.h>


namespace MODEL_SUBSTITUTION
{

namespace
{

/// Extensions, ordered by user preference, that identify a "current"
/// (STEP-era) replacement for a VRML model.  No leading dots.
const std::vector<wxString>& stepExtensions()
{
    static const std::vector<wxString> exts = {
        wxS( "step" ), wxS( "stp" ), wxS( "stpz" ),
        wxS( "step.gz" ), wxS( "stp.gz" ),
        wxS( "iges" ), wxS( "igs" )
    };

    return exts;
}


/// Dotted, lowercase STEP extensions; matches what we test against file
/// suffixes.  Built once alongside stepExtensions() to keep the two in sync.
const std::vector<wxString>& dottedStepExtensions()
{
    static const std::vector<wxString> dotted = []
    {
        std::vector<wxString> result;
        result.reserve( stepExtensions().size() );

        for( const wxString& ext : stepExtensions() )
            result.push_back( wxS( "." ) + ext );

        return result;
    }();

    return dotted;
}


bool hasStepExtension( const wxString& aPath )
{
    const wxString lower = aPath.Lower();

    for( const wxString& dotted : dottedStepExtensions() )
    {
        if( lower.length() > dotted.length()
                && lower.Right( dotted.length() ) == dotted )
        {
            return true;
        }
    }

    return false;
}


/// Strip any recognised 3D extension (WRL or STEP family) from @p aName and
/// lowercase; '-' and ' ' normalise to '_' so that "R_0603" and "R-0603"
/// collide.
wxString normalizeStem( const wxString& aName )
{
    wxString stem = aName;

    static const wxString stripList[] = {
        wxS( ".step.gz" ), wxS( ".stp.gz" ),
        wxS( ".wrl" ),  wxS( ".wrz" ),
        wxS( ".step" ), wxS( ".stp" ), wxS( ".stpz" ),
        wxS( ".iges" ), wxS( ".igs" )
    };

    for( const wxString& ext : stripList )
    {
        if( stem.length() > ext.length()
                && stem.Right( ext.length() ).Lower() == ext )
        {
            stem = stem.Left( stem.length() - ext.length() );
            break;
        }
    }

    stem.MakeLower();

    stem.Replace( wxS( "-" ), wxS( "_" ) );
    stem.Replace( wxS( " " ), wxS( "_" ) );

    return stem;
}


wxString parentDirName( const wxString& aPath )
{
    wxFileName fn( aPath );
    const wxArrayString& dirs = fn.GetDirs();

    return dirs.empty() ? wxString() : dirs.Last();
}


/// Collect unique, existing, absolute directory paths that should be scanned
/// for STEP siblings.  Mirrors the traversal done by the interactive migration
/// dialog's catalog builder so the two agree on where to look.
std::vector<wxString> gatherScanDirs( const wxString& aProjectPath,
                                      const FILENAME_RESOLVER* aResolver )
{
    std::vector<wxString> result;
    std::unordered_set<wxString> seen;

    auto addDir = [&]( const wxString& aDir )
    {
        if( aDir.IsEmpty() )
            return;

        wxFileName norm( aDir, wxEmptyString );
        norm.Normalize( wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS );
        const wxString key = norm.GetPath().Lower();

        if( !seen.insert( key ).second )
            return;

        if( wxDir::Exists( norm.GetPath() ) )
            result.push_back( norm.GetPath() );
    };

    if( aResolver )
    {
        if( const std::list<SEARCH_PATH>* paths = aResolver->GetPaths() )
        {
            for( const SEARCH_PATH& sp : *paths )
                addDir( sp.m_Pathexp );
        }
    }

    if( !aProjectPath.IsEmpty() )
    {
        wxFileName prj3D( aProjectPath, wxEmptyString );
        prj3D.AppendDir( wxS( "3dshapes" ) );
        addDir( prj3D.GetPath() );
    }

    if( COMMON_SETTINGS* common = Pgm().GetCommonSettings() )
    {
        for( const wxString& dir : common->m_Extra3DSearchDirs )
            addDir( dir );
    }

    return result;
}

}  // namespace


bool IsWrlExtension( const wxString& aFilename )
{
    const wxString ext = aFilename.AfterLast( '.' ).Lower();

    return ext == wxS( "wrl" ) || ext == wxS( "wrz" );
}


void STEP_CATALOG::Build( const wxString& aProjectPath, const FILENAME_RESOLVER* aResolver )
{
    m_byStem.clear();

    for( const wxString& dir : gatherScanDirs( aProjectPath, aResolver ) )
    {
        wxArrayString files;
        ::CollectFilesLoopSafe( dir, files, wxEmptyString, wxDIR_FILES | wxDIR_DIRS );

        for( const wxString& file : files )
        {
            if( !hasStepExtension( file ) )
                continue;

            const wxString stem = normalizeStem( wxFileName( file ).GetFullName() );

            if( stem.IsEmpty() )
                continue;

            m_byStem[stem].push_back( file );
        }
    }
}


wxString STEP_CATALOG::FindMatchFor( const wxString& aMissingWrl ) const
{
    if( !IsWrlExtension( aMissingWrl ) )
        return wxEmptyString;

    const wxString stem = normalizeStem( wxFileName( aMissingWrl ).GetFullName() );

    if( stem.IsEmpty() )
        return wxEmptyString;

    auto it = m_byStem.find( stem );

    if( it == m_byStem.end() || it->second.empty() )
        return wxEmptyString;

    const wxString wrlParent = parentDirName( aMissingWrl );

    if( !wrlParent.IsEmpty() )
    {
        for( const wxString& candidate : it->second )
        {
            if( parentDirName( candidate ).CmpNoCase( wrlParent ) == 0 )
                return candidate;
        }
    }

    return it->second.front();
}

}  // namespace MODEL_SUBSTITUTION
