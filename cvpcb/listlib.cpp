/***************/
/* listlib.cpp */
/**(************/


/*
 * Functions to read footprint libraries and create the list of availlable footprints
 * and their documentation (comments and keywords)
 */
#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "macros.h"
#include "appl_wxstruct.h"

#include "cvpcb.h"
#include "protos.h"

#include "dialog_load_error.h"

/* MDC and MOD file strings */
static wxString s_files_not_found;
static wxString s_files_invalid;


static void ReadDocLib( const wxString& ModLibName, FOOTPRINT_LIST& list );


/**
 * Read the list of libraries and generate a list modules.
 *
 * Module description format:
 *   $MODULE c64acmd
 *     Li c64acmd DIN connector
 *     Cd Europe 96 Contact AC male right
 *     Kw PAD_CONN DIN
 *   $Endmodule
 *
 */
bool LoadFootprintFiles( const wxArrayString& libNames,
                         FOOTPRINT_LIST& list )
{
    FILE*       file;
    char        buffer[1024];
    wxFileName  filename;
    int         end;
    FOOTPRINT*  ItemLib;
    unsigned    i;
    wxString    tmp, msg;
    char*       result;

    /* Check if footprint list is not empty */
    if( !list.empty() )
        list.clear();

    /* Check if there are footprint libraries in project file */
    if( libNames.GetCount() == 0 )
    {
        wxMessageBox( _( "No PCB foot print libraries are listed in the current project file." ),
                      _( "Project File Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    /* Parse Libraries Listed */
    for( i = 0; i < libNames.GetCount(); i++ )
    {
        filename = libNames[i];
        filename.SetExt( ModuleFileExtension );

        tmp = wxGetApp().FindLibraryPath( filename );

        if( !tmp )
        {
            s_files_not_found << filename.GetFullName() << wxT("\n");
            continue;
        }

        /* Open library file */
        file = wxFopen( tmp, wxT( "rt" ) );

        if( file == NULL )
        {
            s_files_invalid << tmp <<  _(" (file cannot be opened)") << wxT("\n");
            continue;
        }

        /* Check if library type is valid */
        result = fgets( buffer, 32, file );
        if( strncmp( buffer, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
        {
            s_files_invalid << tmp << _(" (Not a Kicad file)") << wxT("\n");
            fclose( file );
            continue;
        }

        /* TODO: Read the number of components. */
        fseek( file, 0, 0 );

        end = 0;
        while( !end && fgets( buffer, 255, file ) != NULL )
        {
            if( strnicmp( buffer, "$INDEX", 6 ) == 0 )
            {
                while( fgets( buffer, 255, file ) != NULL )
                {
                    if( strnicmp( buffer, "$EndINDEX", 6 ) == 0 )
                    {
                        end = 1;
                        break;
                    }

                    ItemLib = new FOOTPRINT();
                    ItemLib->m_Module = CONV_FROM_UTF8( StrPurge( buffer ) );
                    ItemLib->m_LibName = tmp;
                    list.push_back( ItemLib );
                }

                if( !end )
                {
                    s_files_invalid << tmp << _(" (Unexpected end of file)") << wxT("\n");
                }
            }
        }

        fclose( file );
        ReadDocLib( tmp, list );
    }

    /* Display if there are mdc files not found */
    if( !s_files_not_found.IsEmpty() || !s_files_invalid.IsEmpty() )
    {
        DIALOG_LOAD_ERROR dialog(NULL);
        if( !s_files_not_found.IsEmpty() )
        {
            wxString message = _("Some files could not be found!");
            dialog.MessageSet(message);
            dialog.ListSet(s_files_not_found);
            s_files_not_found.Empty();
        }

        /* Display if there are mdc files invalid */
        if( !s_files_invalid.IsEmpty() )
        {
            dialog.MessageSet( _("Some files are invalid!"));
            dialog.ListSet(s_files_invalid);
            s_files_invalid.Empty();
        }
        dialog.ShowModal();
    }

    list.sort();

    return true;
}


/**
 * Read the file Doc combines a library ModLibName.
 * Create the list of doc strings pointed to by list
 * ModLibName = full file name of the library modules
 */
static void ReadDocLib( const wxString& ModLibName, FOOTPRINT_LIST& list )
{
    FOOTPRINT* NewMod;
    char       Line[1024];
    wxString   ModuleName;
    FILE*      mdc_file;
    wxFileName mdc_filename = ModLibName;

    /* Set mdc filename extension */
    mdc_filename.SetExt( wxT( "mdc" ) );

    /* Check if mdc file exists and can be opened */
    if( ( mdc_file = wxFopen( mdc_filename.GetFullPath(), wxT( "rt" ) ) ) == NULL )
    {
        s_files_not_found += mdc_filename.GetFullPath() + wxT("\n");
        return;
    }

    /* Check if mdc file is valid */
    GetLine( mdc_file, Line, NULL, sizeof(Line) - 1 );
    if( strnicmp( Line, ENTETE_LIBDOC, L_ENTETE_LIB ) != 0 )
    {
        s_files_invalid += mdc_filename.GetFullPath() + wxT("\n");
        return;
    }

    /* Read the mdc file */
    while( GetLine( mdc_file, Line, NULL, sizeof(Line) - 1 ) )
    {
        NewMod = NULL;
        if( Line[0] != '$' )
            continue;
        if( Line[1] == 'E' )
            break;
        if( Line[1] == 'M' )
        {
            /* Parse file line by line */
            while( GetLine( mdc_file, Line, NULL, sizeof(Line) - 1 ) )
            {
                /* $EndMODULE */
                if( Line[0] == '$' )
                    break;

                switch( Line[0] )
                {
                    /* LibName */
                    case 'L':       /* LibName */
                        ModuleName = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
                        BOOST_FOREACH( FOOTPRINT& footprint, list )
                        {
                            if( ModuleName == footprint.m_Module )
                            {
                                NewMod = &footprint;
                                break;
                            }
                        }
                    break;

                    /* KeyWords */
                    case 'K':
                        if( NewMod && (!NewMod->m_KeyWord) )
                        NewMod->m_KeyWord = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
                    break;

                    /* Doc */
                    case 'C':
                        if( NewMod && ( !NewMod->m_Doc ) )
                        NewMod->m_Doc = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
                    break;
                }
            }
        } /* Parsed one module documentation */
    } /* Parsed complete library documentation */

    fclose( mdc_file );
}
