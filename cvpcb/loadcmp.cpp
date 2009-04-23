/***************************************************/
/* Localisation des elements pointes par la souris */
/***************************************************/

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
#include "cvstruct.h"


/*****************************************************************
 *
 *   Analyse les LIBRAIRIES pour trouver le module demande
 *   Si ce module est trouve, le copie en memoire, et le
 *   chaine en fin de liste des modules
 *       - Entree:
 *           name_cmp = nom du module
 *       - Retour:
 *           Pointeur sur le nouveau module.
 *
 *****************************************************************/
MODULE* WinEDA_DisplayFrame::Get_Module( const wxString& CmpName )
{
    int        LineNum, Found = 0;
    unsigned   ii;
    char       Line[1024], Name[255];
    wxString   tmp, msg;
    wxFileName fn;
    MODULE*    Module = NULL;
    WinEDA_CvpcbFrame* parent = ( WinEDA_CvpcbFrame* ) GetParent();

    for( ii = 0; ii < parent->m_ModuleLibNames.GetCount(); ii++ )
    {
        /* Calcul du nom complet de la librairie */
        fn = parent->m_ModuleLibNames[ii];
        fn.SetExt( ModuleFileExtension );

        tmp = wxGetApp().FindLibraryPath( fn );

        if( !tmp )
        {
            msg.Printf( _( "PCB foot print library file <%s> could not be " \
                           "found in the default search paths." ),
                        fn.GetFullName().c_str() );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR, this );
            continue;
        }

        FILE* file = wxFopen( tmp, wxT( "rt" ) );

        if( file == NULL )
        {
            msg.Printf( _( "Could not open PCB foot print library file <%s>." ),
                        tmp.c_str() );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR, this );
            continue;
        }

        /* lecture entete chaine definie par ENTETE_LIBRAIRIE */
        LineNum = 0;
        GetLine( file, Line, &LineNum );
        StrPurge( Line );

        if( strnicmp( Line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
        {
            msg.Printf( _( "<%s> is not a valid Kicad PCB foot print library." ),
                        tmp.c_str() );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR, this );
            fclose( file );
            return NULL;
        }

        /* Lecture de la liste des composants de la librairie */
        Found = 0;
        while( !Found && GetLine( file, Line, &LineNum ) )
        {
            if( strncmp( Line, "$MODULE", 6 ) == 0 )
                break;

            if( strnicmp( Line, "$INDEX", 6 ) == 0 )
            {
                while( GetLine( file, Line, &LineNum ) )
                {
                    if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
                        break;

                    StrPurge( Line );
                    if( stricmp( Line, CONV_TO_UTF8( CmpName ) ) == 0 )
                    {
                        Found = 1;
                        break; /* Trouve! */
                    }
                }
            }
        }

        /* Lecture de la librairie */
        while( Found && GetLine( file, Line, &LineNum ) )
        {
            if( Line[0] != '$' )
                continue;

            if( Line[1] != 'M' )
                continue;

            if( strnicmp( Line, "$MODULE", 7 ) != 0 )
                continue;

            /* Lecture du nom du composant */
            sscanf( Line + 7, " %s", Name );
            if( stricmp( Name, CONV_TO_UTF8( CmpName ) ) == 0 )  /* composant localise */
            {
                Module = new MODULE( GetBoard() );

                // Switch the locale to standard C (needed to print floating point numbers like 1.3)
                SetLocaleTo_C_standard();
                Module->ReadDescr( file, &LineNum );
                SetLocaleTo_Default();       // revert to the current  locale
                Module->SetPosition( wxPoint( 0, 0 ) );
                fclose( file );
                return Module;
            }
        }

        fclose( file );
        file = NULL;
    }

    msg.Printf( _( "Module %s not found" ), CmpName.GetData() );
    DisplayError( this, msg );
    return NULL;
}
