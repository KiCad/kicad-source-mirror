/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file footprint_info.cpp
 */


/*
 * Functions to read footprint libraries and fill m_footprints by available footprints names
 * and their documentation (comments and keywords)
 */
#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <appl_wxstruct.h>
#include <wildcards_and_files_ext.h>
#include <footprint_info.h>
#include <io_mgr.h>
#include <fp_lib_table.h>

#include <class_module.h>


bool FOOTPRINT_LIST::ReadFootprintFiles( wxArrayString& aFootprintsLibNames )
{
    // Clear data before reading files
    m_filesNotFound.Empty();
    m_filesInvalid.Empty();
    m_List.clear();

    // try
    {
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        // Parse Libraries Listed
        for( unsigned ii = 0; ii < aFootprintsLibNames.GetCount(); ii++ )
        {
            // Footprint library file names can be fully qualified or file name only.
            wxFileName filename = aFootprintsLibNames[ii];

            if( !filename.FileExists() )
            {
                filename = wxGetApp().FindLibraryPath( filename.GetFullName() );

                if( !filename.FileExists() )
                {
                    filename = wxFileName( wxEmptyString, aFootprintsLibNames[ii],
                                           LegacyFootprintLibPathExtension );

                    filename = wxGetApp().FindLibraryPath( filename.GetFullName() );
                }
            }

            wxLogDebug( wxT( "Path <%s> -> <%s>." ), GetChars( aFootprintsLibNames[ii] ),
                        GetChars( filename.GetFullPath() ) );

            if( !filename.FileExists() )
            {
                m_filesNotFound << aFootprintsLibNames[ii] << wxT( "\n" );
                continue;
            }

            try
            {
                wxArrayString fpnames = pi->FootprintEnumerate( filename.GetFullPath() );

                for( unsigned i=0; i<fpnames.GetCount();  ++i )
                {
                    std::auto_ptr<MODULE> m( pi->FootprintLoad( filename.GetFullPath(),
                                                                fpnames[i] ) );

                    // we're loading what we enumerated, all must be there.
                    wxASSERT( m.get() );

                    FOOTPRINT_INFO* fpinfo = new FOOTPRINT_INFO();

                    fpinfo->SetLibraryName( filename.GetName() );
                    fpinfo->SetLibraryPath( filename.GetFullPath() );
                    fpinfo->m_Module   = fpnames[i];
                    fpinfo->m_padCount = m->GetPadCount( MODULE::DO_NOT_INCLUDE_NPTH );
                    fpinfo->m_KeyWord  = m->GetKeywords();
                    fpinfo->m_Doc      = m->GetDescription();

                    AddItem( fpinfo );
                }
            }
            catch( IO_ERROR ioe )
            {
                m_filesInvalid << ioe.errorText << wxT( "\n" );
            }
        }
    }

    /*  caller should catch this, UI seems not wanted here.
    catch( IO_ERROR ioe )
    {
        DisplayError( NULL, ioe.errorText );
        return false;
    }
    */

    m_List.sort();

    return true;
}


bool FOOTPRINT_LIST::ReadFootprintFiles( FP_LIB_TABLE& aTable )
{
    // Clear data before reading files
    m_filesNotFound.Empty();
    m_filesInvalid.Empty();
    m_List.clear();

    std::vector< wxString > libNickNames = aTable.GetLogicalLibs();

    // Parse Libraries Listed
    for( unsigned ii = 0; ii < libNickNames.size(); ii++ )
    {
        const FP_LIB_TABLE::ROW* row = aTable.FindRow( libNickNames[ii] );

        wxCHECK2_MSG( row != NULL, continue,
                      wxString::Format( wxT( "No library name <%s> found in footprint library "
                                             "table." ), GetChars( libNickNames[ii] ) ) );
        try
        {
            PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::EnumFromStr( row->GetType() ) ) );

            wxString      path = FP_LIB_TABLE::ExpandSubstitutions( row->GetFullURI() );
            wxArrayString fpnames = pi->FootprintEnumerate( path );

            for( unsigned i=0;  i<fpnames.GetCount();  ++i )
            {
                std::auto_ptr<MODULE> m( pi->FootprintLoad( path, fpnames[i] ) );

                // we're loading what we enumerated, all must be there.
                wxASSERT( m.get() );

                FOOTPRINT_INFO* fpinfo = new FOOTPRINT_INFO();

                fpinfo->SetLibraryName( libNickNames[ii] );
                fpinfo->SetLibraryPath( path );
                fpinfo->m_Module   = fpnames[i];
                fpinfo->m_padCount = m->GetPadCount( MODULE::DO_NOT_INCLUDE_NPTH );
                fpinfo->m_KeyWord  = m->GetKeywords();
                fpinfo->m_Doc      = m->GetDescription();

                AddItem( fpinfo );
            }
        }
        catch( IO_ERROR ioe )
        {
            m_filesInvalid << ioe.errorText << wxT( "\n" );
        }
    }

    m_List.sort();

    return true;
}


bool FOOTPRINT_INFO::InLibrary( const wxString& aLibrary ) const
{
    if( aLibrary.IsEmpty() )
        return false;

    if( aLibrary == m_libName || aLibrary == m_libPath )
        return true;

    wxFileName filename = aLibrary;

    if( filename.GetExt().IsEmpty() )
        filename.SetExt( LegacyFootprintLibPathExtension );

    if( filename.GetFullPath() == m_libPath )
        return true;

    if( filename.GetPath().IsEmpty() )
        filename = wxGetApp().FindLibraryPath( filename.GetFullName() );

    return filename.GetFullPath() == m_libPath;
}
