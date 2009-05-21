/**************/
/* listlib.cpp */
/**************/

/*
 *  cherche toutes les ref <chemin lib>*.??? si nom fichier pr‚sent,
 *  ou examine <chemin lib>[MODULE.LIB]
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


/* routines locales : */
static void ReadDocLib( const wxString& ModLibName, FOOTPRINT_LIST& list );


/**
 * Routine lisant la liste des librairies, et generant la liste chainee
 *  des modules disponibles
 *
 *  Module descr format:
 *  $MODULE c64acmd
 *  Li c64acmd
 *  Cd Connecteur DIN Europe 96 Contacts AC male droit
 *  Kw PAD_CONN DIN
 *  $EndMODULE
 *
 */
bool LoadFootprintFiles( const wxArrayString& libNames,
                         FOOTPRINT_LIST& list )
{
    FILE*      file;   /* pour lecture librairie */
    char       buffer[1024];
    wxFileName fn;
    int        end;
    FOOTPRINT*  ItemLib;
    unsigned   ii;
    wxString   tmp, msg;

    if( !list.empty() )
    {
        list.clear();
    }

    if( libNames.GetCount() == 0 )
    {
        wxMessageBox( _( "No PCB foot print libraries are listed in the current project file." ),
                      _( "Project File Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    /* Lecture des Librairies */
    for( ii = 0; ii < libNames.GetCount(); ii++ )
    {
        /* Calcul du nom complet de la librairie */
        fn = libNames[ii];
        fn.SetExt( ModuleFileExtension );

        tmp = wxGetApp().FindLibraryPath( fn );

        if( !tmp )
        {
            msg.Printf( _( "PCB foot print library file <%s> could not be found in the default search paths." ),
                        fn.GetFullName().c_str() );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
            continue;
        }

        /* acces a une librairie */
        file = wxFopen( tmp, wxT( "rt" ) );

        if( file == NULL )
        {
            msg.Printf( _( "Could not open PCB foot print library file <%s>." ),
                        tmp.c_str() );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
            continue;
        }

        /* Controle du type de la librairie : */
        fgets( buffer, 32, file );
        if( strncmp( buffer, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
        {
            msg.Printf( _( "<%s> is not a valid Kicad PCB foot print library" ),
                        tmp.c_str() );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
            fclose( file );
            continue;
        }

        /* Lecture du nombre de composants */
        fseek( file, 0, 0 );

        /* lecture nom des composants : */
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
                    msg.Printf( _( "Unexpected end of file occurred while parsing PCB foot print library <%s>." ),
                                tmp.c_str() );
                    wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
                }
            }
        }

        fclose( file );
        ReadDocLib( tmp, list );
    }

    list.sort();

    return true;
}


/* Routine de lecture du fichier Doc associe a la librairie ModLibName.
 *   Cree en memoire la chaine liste des docs pointee par MList
 *   ModLibName = full file Name de la librairie Modules
 */
static void ReadDocLib( const wxString& ModLibName, FOOTPRINT_LIST& list )
{
    FOOTPRINT* NewMod;
    char       Line[1024];
    wxString   ModuleName;
    wxString   msg;
    FILE*      LibDoc;
    wxFileName fn = ModLibName;

    fn.SetExt( wxT( "mdc" ) );

    if( ( LibDoc = wxFopen( fn.GetFullPath(), wxT( "rt" ) ) ) == NULL )
    {
        msg.Printf( _( "Could not open PCB foot print library document file <%s>." ),
                    fn.GetFullPath().c_str() );
        wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
        return;
    }

    GetLine( LibDoc, Line, NULL, sizeof(Line) - 1 );
    if( strnicmp( Line, ENTETE_LIBDOC, L_ENTETE_LIB ) != 0 )
    {
        msg.Printf( _( "<%s> is not a valid PCB foot print library document file." ),
                    fn.GetFullPath().c_str() );
        wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
        return;
    }

    /* Lecture de la librairie */
    while( GetLine( LibDoc, Line, NULL, sizeof(Line) - 1 ) )
    {
        NewMod = NULL;
        if( Line[0] != '$' )
            continue;
        if( Line[1] == 'E' )
            break;;
        if( Line[1] == 'M' )    /* Debut decription 1 module */
        {
            while( GetLine( LibDoc, Line, NULL, sizeof(Line) - 1 ) )
            {
                if( Line[0] ==  '$' )   /* $EndMODULE */
                    break;
                switch( Line[0] )
                {
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

                case 'K':       /* KeyWords */
                    if( NewMod && (!NewMod->m_KeyWord) )
                        NewMod->m_KeyWord = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
                    break;

                case 'C':       /* Doc */
                    if( NewMod && ( !NewMod->m_Doc ) )
                        NewMod->m_Doc = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
                    break;
                }
            }
        }       /* lecture 1 descr module */
    }           /* Fin lecture librairie */

    fclose( LibDoc );
}
