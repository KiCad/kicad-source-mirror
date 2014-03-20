/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <macros.h>             // FROM_UTF8()
#include <wx/config.h>
#include <wx/stdpaths.h>

#include <kiface_i.h>
#include <pgm_base.h>

#include <common.h>
#include <gr_basic.h>


static const wxChar showPageLimitsKey[]  = wxT( "ShowPageLimits" );
static const wxChar backgroundColorKey[] = wxT( "BackgroundColor" );


/// Initialize aDst SEARCH_STACK with KIFACE (DSO) specific settings.
/// A non-member function so it an be moved easily, plus it's nobody's business.
static void setSearchPaths( SEARCH_STACK* aDst, KIWAY::FACE_T aId )
{
    SEARCH_STACK    bases;

    SystemDirsAppend( &bases );
    aDst->Clear();

    for( unsigned i = 0;  i < bases.GetCount();  ++i )
    {
        wxFileName fn( bases[i], wxEmptyString );

        // Add schematic library file path to search path list.
        // we must add <kicad path>/library and <kicad path>/library/doc
        if( aId == KIWAY::FACE_SCH )
        {
            fn.AppendDir( wxT( "library" ) );
            aDst->AddPaths( fn.GetPath() );

            // Add schematic doc file path (library/doc)to search path list.
            fn.AppendDir( wxT( "doc" ) );
            aDst->AddPaths( fn.GetPath() );

            fn.RemoveLastDir();
            fn.RemoveLastDir();     // "../../"  up twice, removing library/doc/
        }

        // Add PCB library file path to search path list.
        if( aId == KIWAY::FACE_PCB || aId == KIWAY::FACE_CVPCB )
        {
            fn.AppendDir( wxT( "modules" ) );
            aDst->AddPaths( fn.GetPath() );

            // Add 3D module library file path to search path list.
            fn.AppendDir( wxT( "packages3d" ) );
            aDst->AddPaths( fn.GetPath() );

            fn.RemoveLastDir();
            fn.RemoveLastDir();     // "../../" up twice, remove modules/packages3d
        }

        // Add KiCad template file path to search path list.
        fn.AppendDir( wxT( "template" ) );
        aDst->AddPaths( fn.GetPath() );
    }

#if 1 && defined(DEBUG)
    aDst->Show( "kiway" );
#endif
}


bool KIFACE_I::start_common()
{
    m_bm.Init();

    m_bm.m_config->Read( showPageLimitsKey, &g_ShowPageLimits );

    // FIXME OSX Mountain Lion (10.8)
    // Seems that Read doesn't found anything and ColorFromInt
    // Asserts - I'm unable to reproduce on 10.7

    int draw_bg_color = BLACK;      // Default for all apps but Eeschema

    if( m_id == KIWAY::FACE_SCH )
        draw_bg_color = WHITE;      // Default for Eeschema

    m_bm.m_config->Read( backgroundColorKey, &draw_bg_color );

    g_DrawBgColor = ColorFromInt( draw_bg_color );

    setSearchPaths( &m_bm.m_search, m_id );

    return true;
}


void KIFACE_I::end_common()
{
    m_bm.End();
}


wxString KIFACE_I::GetHelpFile()
{
    wxString        fn;
    wxArrayString   subdirs;
    wxArrayString   altsubdirs;

    // FIXME: This is not the ideal way to handle this.  Unfortunately, the
    //        CMake install paths seem to be a moving target so this crude
    //        hack solves the problem of install path differences between
    //        Windows and non-Windows platforms.

    // Partially fixed, but must be enhanced

    // Create subdir tree for "standard" linux distributions, when KiCad comes
    // from a distribution files are in /usr/share/doc/kicad/help and binaries
    // in /usr/bin or /usr/local/bin
    subdirs.Add( wxT( "share" ) );
    subdirs.Add( wxT( "doc" ) );
    subdirs.Add( wxT( "kicad" ) );
    subdirs.Add( wxT( "help" ) );

    // Create subdir tree for linux and Windows KiCad pack.
    // Note  the pack form under linux is also useful if a user wants to
    // install KiCad to a server because there is only one path to mount
    // or export (something like /usr/local/kicad).
    // files are in <install dir>/kicad/doc/help
    // (often /usr/local/kicad/kicad/doc/help)
    // <install dir>/kicad/ is retrieved from m_BinDir
    altsubdirs.Add( wxT( "doc" ) );
    altsubdirs.Add( wxT( "help" ) );

    /* Search for a help file.
     *  we *must* find a help file.
     *  so help is searched in directories in this order:
     *  help/<canonical name> like help/en_GB
     *  help/<short name> like help/en
     *  help/en
     */

    wxLocale* i18n = Pgm().GetLocale();

    // Step 1 : Try to find help file in help/<canonical name>
    subdirs.Add( i18n->GetCanonicalName() );
    altsubdirs.Add( i18n->GetCanonicalName() );

    fn = m_bm.m_search.FindFileInSearchPaths( m_bm.m_help_file, &altsubdirs );

    if( !fn  )
        fn = m_bm.m_search.FindFileInSearchPaths( m_bm.m_help_file, &subdirs );

    // Step 2 : if not found Try to find help file in help/<short name>
    if( !fn  )
    {
        subdirs.RemoveAt( subdirs.GetCount() - 1 );
        altsubdirs.RemoveAt( altsubdirs.GetCount() - 1 );

        // wxLocale::GetName() does not return always the short name
        subdirs.Add( i18n->GetName().BeforeLast( '_' ) );
        altsubdirs.Add( i18n->GetName().BeforeLast( '_' ) );

        fn = m_bm.m_search.FindFileInSearchPaths( m_bm.m_help_file, &altsubdirs );

        if( !fn )
            fn = m_bm.m_search.FindFileInSearchPaths( m_bm.m_help_file, &subdirs );
    }

    // Step 3 : if not found Try to find help file in help/en
    if( !fn )
    {
        subdirs.RemoveAt( subdirs.GetCount() - 1 );
        altsubdirs.RemoveAt( altsubdirs.GetCount() - 1 );
        subdirs.Add( wxT( "en" ) );
        altsubdirs.Add( wxT( "en" ) );

        fn = m_bm.m_search.FindFileInSearchPaths( m_bm.m_help_file, &altsubdirs );

        if( !fn )
            fn = m_bm.m_search.FindFileInSearchPaths( m_bm.m_help_file, &subdirs );
    }

    return fn;
}
