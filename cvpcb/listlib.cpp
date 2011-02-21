/**
 * @file listlib.cpp
 */


/*
 * Functions to read footprint libraries and fill m_footprints by availlable footprints names
 * and their documentation (comments and keywords)
 */
#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"
#include "kicad_string.h"
#include "macros.h"
#include "appl_wxstruct.h"

#include "cvpcb.h"
#include "cvpcb_mainframe.h"
#include "richio.h"
#include "filter_reader.h"
#include "footprint_info.h"

#include "dialog_load_error.h"

/*
 * Read the list of libraries (*.mod files) and populates m_footprints ( list of availaible modules in libs ).
 * for each module are stored
 *      the module name
 *      documentation string
 *      associated keywords
 * Module description format:
 *   $MODULE c64acmd                    First line of module description
 *   Li c64acmd DIN connector           Library reference
 *   Cd Europe 96 AC male vertical      documentation string
 *   Kw PAD_CONN DIN                    associated keywords
 *   ...... other data (pads, outlines ..)
 *   $Endmodule
 *
 */
bool CVPCB_MAINFRAME::LoadFootprintFiles( )
{
    FILE*       file;
    wxFileName  filename;
    wxString    libname;
    wxString files_not_found;
    wxString files_invalid;

    /* Check if footprint m_footprints is not empty */
    if( !m_footprints.empty() )
        m_footprints.clear();

    /* Check if there are footprint libraries in project file */
    if( m_ModuleLibNames.GetCount() == 0 )
    {
        wxMessageBox( _( "No PCB footprint libraries are listed in the current project file." ),
                      _( "Project File Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    /* Parse Libraries Listed */
    for( unsigned ii = 0; ii < m_ModuleLibNames.GetCount(); ii++ )
    {
        filename = m_ModuleLibNames[ii];
        filename.SetExt( ModuleFileExtension );

        libname = wxGetApp().FindLibraryPath( filename );

        if( libname.IsEmpty() )
        {
            files_not_found << filename.GetFullName() << wxT("\n");
            continue;
        }

        /* Open library file */
        file = wxFopen( libname, wxT( "rt" ) );

        if( file == NULL )
        {
            files_invalid << libname <<  _(" (file cannot be opened)") << wxT("\n");
            continue;
        }

        FILE_LINE_READER    fileReader( file, libname );
        FILTER_READER       reader( fileReader );

        /* Read header. */
        reader.ReadLine();
        char * line = reader.Line();
        StrPurge( line );

        if( strnicmp( line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
        {
            wxString msg;
            msg.Printf( _( "<%s> is not a valid Kicad PCB footprint library." ),
                        GetChars( libname ) );
            files_invalid << msg << wxT("\n");
            continue;
        }

        // Read library
        bool end = false;
        while( !end && reader.ReadLine() )
        {
            line = reader.Line();
            StrPurge( line );
            if( strnicmp( line, "$EndLIBRARY", 11 ) == 0 )
            {
                end = true;
                break;
            }
            if( strnicmp( line, "$MODULE", 7 ) == 0 )
            {

                line += 7;
                FOOTPRINT_INFO*  ItemLib = new FOOTPRINT_INFO();
                ItemLib->m_Module = CONV_FROM_UTF8( StrPurge( line ) );
                ItemLib->m_LibName = libname;
                m_footprints.push_back( ItemLib );

                while( reader.ReadLine() )
                {
                    line = reader.Line();
                    StrPurge( line );
                    if( strnicmp( line, "$EndMODULE", 10 ) == 0 )
                        break;

                    int id = ((line[0] & 0xFF) << 8) + (line[1] & 0xFF);
                    switch( id )
                    {
                    /* KeyWords */
                    case (('K'<<8) + 'w'):
                        ItemLib->m_KeyWord = CONV_FROM_UTF8( StrPurge( line + 3 ) );
                    break;

                    /* Doc */
                    case (('C'<<8) + 'd'):
                        ItemLib->m_Doc = CONV_FROM_UTF8( StrPurge( line + 3 ) );
                    break;
                    }
                }
            }
        }

        if( !end )
        {
            files_invalid << libname << _(" (Unexpected end of file)") << wxT("\n");
        }
    }
    m_footprints.sort();

    /* Display error messages, if any */
    if( !files_not_found.IsEmpty() || !files_invalid.IsEmpty() )
    {
        DIALOG_LOAD_ERROR dialog(NULL);
        if( !files_not_found.IsEmpty() )
        {
            wxString message = _("Some files could not be found!");
            dialog.MessageSet(message);
            dialog.ListSet(files_not_found);
        }

        /* Display if there are invalid files */
        if( !files_invalid.IsEmpty() )
        {
            dialog.MessageSet( _("Some files are invalid!"));
            dialog.ListSet(files_invalid);
        }
        dialog.ShowModal();
    }

    return true;
}
