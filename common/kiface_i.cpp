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
            // Add schematic doc file path (library/doc) to search path list.

            fn.AppendDir( wxT( "library" ) );
            aDst->AddPaths( fn.GetPath() );

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

    aDst->AddPaths( wxT( "/usr/local/share" ) );

#if 1 && defined(DEBUG)
    aDst->Show( "kiface" );
#endif
}


bool KIFACE_I::start_common( int aCtlBits )
{
    m_start_flags = aCtlBits;

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
    /* Save common preferences; the background still uses the old legacy
     * color numbers, not the new names */
    m_bm.m_config->Write( showPageLimitsKey, g_ShowPageLimits );
    m_bm.m_config->Write( backgroundColorKey, int( g_DrawBgColor ) );

    m_bm.End();
}

