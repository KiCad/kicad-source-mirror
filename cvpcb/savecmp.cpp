/**************/
/* savecmp()  */
/**************/

/* sauvegarde la liste des associations composants/empreintes */

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


/* Chaines de caractere d'identification */
char EnteteCmpMod[] = { "Cmp-Mod V01" };

const wxString titleComponentLibErr( _( "Component Library Error" ) );


/*
 * Routine de sauvegarde du fichier des modules
 *   Retourne 1 si OK
 *           0 si ecriture non faite
 */
int WinEDA_CvpcbFrame::SaveComponentList( const wxString& NetlistFullFileName )
{
    FILE*       dest;
    wxFileName  fn( NetlistFullFileName );
    char        Line[1024];
    wxString    Title = wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion();

    /* calcul du nom du fichier */
    fn.SetExt( ComponentFileExtension );

    dest = wxFopen( fn.GetFullPath(), wxT( "wt" ) );
    if( dest == NULL )
        return 0;                   /* Erreur ecriture */

    fprintf( dest, "%s", EnteteCmpMod );
    fprintf( dest, " Created by %s", CONV_TO_UTF8( Title ) );
    fprintf( dest, " date = %s\n", DateAndTime( Line ) );

    BOOST_FOREACH( COMPONENT& component, m_components )
    {
        fprintf( dest, "\nBeginCmp\n" );
        fprintf( dest, "TimeStamp = %s;\n",
                 CONV_TO_UTF8( component.m_TimeStamp ) );
        fprintf( dest, "Reference = %s;\n",
                 CONV_TO_UTF8( component.m_Reference ) );
        fprintf( dest, "ValeurCmp = %s;\n",
                 CONV_TO_UTF8( component.m_Value ) );
        fprintf( dest, "IdModule  = %s;\n",
                 CONV_TO_UTF8( component.m_Module ) );
        fprintf( dest, "EndCmp\n" );
    }

    fprintf( dest, "\nEndListe\n" );
    fclose( dest );
    return 1;
}


/*
 * recupere la liste des associations composants/empreintes
 */
bool LoadComponentFile( const wxString& fileName, COMPONENT_LIST& list )
{
    wxString    timestamp, valeur, ilib, namecmp, msg;
    bool        read_cmp_data = FALSE, eof = FALSE;
    char        Line[1024], * ident, * data;
    FILE*       source;
    wxFileName  fn = fileName;

    /* calcul du nom du fichier */
    fn.SetExt( ComponentFileExtension );

    source = wxFopen( fn.GetFullPath(), wxT( "rt" ) );
    if( source == NULL )
    {
        msg.Printf( _( "Cannot open component library <%s>." ),
                    fn.GetFullPath().c_str() );
        wxMessageBox( msg, titleComponentLibErr, wxOK | wxICON_ERROR );
        return false;
    }

    /* Identification du Type de fichier CmpMod */
    if( fgets( Line, 79, source ) == 0 )
    {
        msg.Printf( _( " <%s> does not appear to be a valid Kicad component " \
                       "library." ), fn.GetFullPath().c_str() );
        wxMessageBox( msg, titleComponentLibErr, wxOK | wxICON_ERROR );
        fclose( source );
        return false;
    }

    if( strnicmp( Line, EnteteCmpMod, 11 ) != 0 ) /* old file version*/
    {
        msg.Printf( _( "<%s> is an old version component file." ) );
        wxMessageBox( msg, titleComponentLibErr, wxOK | wxICON_ERROR );
        fclose( source );
        return false;
    }

    /* lecture de la liste */
    while( !eof && fgets( Line, 79, source ) != 0 )
    {
        if( strnicmp( Line, "EndListe", 8 ) == 0 )
            break;

        /* Recherche du debut de description du composant */
        if( strnicmp( Line, "BeginCmp", 8 ) != 0 )
            continue;
        timestamp.Empty();
        valeur.Empty();
        ilib.Empty();
        namecmp.Empty();
        read_cmp_data = TRUE;

        while( !eof && read_cmp_data )
        {
            if( fgets( Line, 1024, source ) == 0 )
            {
                eof = TRUE;
                break;
            }

            if( strnicmp( Line, "EndCmp", 6 ) == 0 )
            {
                read_cmp_data = TRUE;
                break;
            }

            ident = strtok( Line, "=;\n\r" );
            data  = strtok( NULL, ";\n\r" );

            if( strnicmp( ident, "TimeStamp", 9 ) == 0 )
            {
                timestamp = CONV_FROM_UTF8( data );
                timestamp.Trim( TRUE );
                timestamp.Trim( FALSE );
                continue;
            }

            if( strnicmp( ident, "Reference", 9 ) == 0 )
            {
                namecmp = CONV_FROM_UTF8( data );
                namecmp.Trim( TRUE );
                namecmp.Trim( FALSE );
                continue;
            }

            if( strnicmp( ident, "ValeurCmp", 9 ) == 0 )
            {
                valeur = CONV_FROM_UTF8( data );
                valeur.Trim( TRUE );
                valeur.Trim( FALSE );
                continue;
            }

            if( strnicmp( ident, "IdModule", 8 ) == 0 )
            {
                ilib = CONV_FROM_UTF8( data );
                ilib.Trim( TRUE );
                ilib.Trim( FALSE );
                continue;
            }
        } /* Fin lecture description de 1 composant */

        /* Recherche du composant correspondant en netliste et
         *    mise a jour de ses parametres */
        BOOST_FOREACH( COMPONENT& component, list )
        {
            if( namecmp != component.m_Reference )
                continue;

            /* composant identifie , copie du nom du module correspondant */
            component.m_Module = ilib;
        }
    }

    fclose( source );
    return true;
}
