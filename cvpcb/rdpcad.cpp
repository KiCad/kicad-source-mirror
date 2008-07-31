/****************/
/* rdpcad()  */
/***************/

/* convertit la netliste PCAD en netliste standard (fichier temporaire)
  * assure la reaffectation des alimentations selon le format :
  * {I VALEUR<SEPARATEUR>(pin1,pin2,...=newalim).PRT ID
 */

#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "colors.h"
#include "cvpcb.h"
#include "protos.h"

#define SEPARATEUR '|'

/* routines locales : */
int pin();

/***************************************/
int WinEDA_CvpcbFrame::rdpcad()
/***************************************/
{
    int       i, j, k, l;
    char      Line[1024];
    char      label[80];    /* buffer des references composants */
    char      val[80];      /* buffer des valeurs */
    char*     ptchar;       /* pointeur de service */
    STORECMP* Cmp;

    modified = 0;
    Rjustify = 0;

    /* Raz buffer et variable de gestion */
    if( g_BaseListeCmp )
        FreeMemoryComponants();

    /* Ouverture du fichier source  */
    source = wxFopen( FFileName, wxT( "rt" ) );
    if( source == 0 )
    {
        wxString msg;
        msg.Printf( _( "File <%s> not found" ), FFileName.GetData() );
        DisplayError( this, msg ); return -1;
    }

    /* Lecture entete qui doit etre "{COMPONENT ORCAD.PCB" ou "{ OrCAD PCB"*/
    fgets( Line, 255, source );
    i = strncmp( Line, "{COMPONENT ORCAD.PCB", 9 );  /* net type PCAD */

    if( i != 0 )
    {
        wxString msg, Lineconv = CONV_FROM_UTF8( Line );
        msg.Printf( _( "Unknown file format <%s>" ), Lineconv.GetData() );
        DisplayError( this, msg );
        fclose( source ); return -3;
    }

    SetStatusText( _( "Netlist Format: Pcad" ), 0 );

    /* Lecture de la liste */

    for( ; ; )
    {
        /* recherche du debut de la description d'un composant */

        if( fgets( Line, 80, source ) == 0 )
            break;

        /* suppression des blancs en d‚but de ligne */
        i = 0; while( Line[i] == ' ' )
            i++;

        /* elimination des lignes vides : */
        if( Line[i] < ' ' )
            continue;

        if( strncmp( &Line[i], "{I", 2 ) != 0 )
            continue;

        /****************************/
        /* debut description trouv‚ */
        /****************************/

        i += 3; /* i pointe le 1er caractere de la valeur du composant */

        for( j = 0; j < 20; j++ )
        {
            label[j] = 0; val[j] = ' ';
        }

        val[16] = 0;

        for( j = 0; j < 80; j++ )
            alim[j] = 0;

        /* lecture valeur du composant ( toujours reecrire sur 8 caracteres) */

        /* recherche fin de valeur (.PRT) */
        ptchar = strstr( Line, ".PRT" );
        if( ptchar == 0 )
        {
            //Netlist error: todo: display a message error
        }
        k = ptchar - Line;

        for( j = 0; i < k; i++ )
        {
            if( Line[i] == SEPARATEUR )
                break;
            if( j < 8 )
                val[j++] = Line[i];
        }

        if( (Line[++i] == '(') && (Line[k - 1] == ')' ) )
        {
            i++; l = 0; while( k - 1 > i )
                alim[l++] = Line[i++];
        }
        else
            i = k;

        /* recherche reference du composant */
        while( Line[i] != ' ' )
            i++;               /* elimination fin valeur */

        while( Line[i] == ' ' )
            i++;               /* recherche debut reference */

        /* debut reference trouv‚ */
        for( k = 0; k < 8; i++, k++ )
        {
            if( Line[i] <= ' ' )
                break;
            label[k] = Line[i];
        }

        /* classement du composant ,suivi de sa valeur */
        Cmp = new STORECMP();
        Cmp->Pnext       = g_BaseListeCmp;
        g_BaseListeCmp   = Cmp;
        Cmp->m_Reference = CONV_FROM_UTF8( label );
        Cmp->m_Valeur    = CONV_FROM_UTF8( val );
        pin();
        nbcomp++;
    }

    fclose( source );

    /* reclassement alpab‚tique : */
    g_BaseListeCmp = TriListeComposantss( g_BaseListeCmp, nbcomp );

    return 0;
}


/***********************************/
/* pin() : analyse liste des pines */
/***********************************/

int pin()
{
    int  i, j, k;
    char numpin[9], net[9];
    char Line[1024];

    for( ; ; )
    {
        /* recherche du debut de la description des pins d'un composant */

        if( fgets( Line, 80, source ) == 0 )
            return -1;
        /* suppression des blancs en d‚but de ligne */
        i = 0; while( Line[i] == ' ' )
            i++;

        /* elimination des lignes vides : */
        if( Line[i] < ' ' )
            continue;

        if( strncmp( &Line[i], "{CN", 3 ) != 0 )
            continue;

        /* debut description trouv‚ */
        for( ; ; )
        {
            if( fgets( Line, 80, source ) == 0 )
                return -1;
            /* suppression des blancs en d‚but de ligne */
            i = 0; while( Line[i] == ' ' )
                i++;


            /* elimination des lignes vides : */
            if( Line[i] < ' ' )
                continue;

            /* fin de description ? */
            if( Line[i] == '}' )
                return 0;

            memset( net, 0, sizeof(net) );
            memset( numpin, 0, sizeof(numpin) );


            /* lecture name pin , 4 lettres */
            for( j = 0; j < 4; j++, i++ )
            {
                if( Line[i] == ' ' )
                    break;
                numpin[j] = Line[i];
            }

            j = 0;

            /* recherche affectation forc‚e de net  */
            if( reaffect( &numpin[j], net ) != 0 )
            {
//			fprintf(dest,"%s:%s\n",&numpin[j],net) ;
                continue;
            }

            /* recherche netname */
            while( Line[i] == ' ' )
                i++;               /* recherche debut reference */

            /* debut netname trouv‚ */
            for( k = 0; k < 8; i++, k++ )
            {
                if( Line[i] <= ' ' )
                    break;
                net[k] = Line[i];
            }

            /* les pins non connect‚es sont ‚limin‚es :*/
            if( net[0] == '?' )
                continue;

//		fprintf(dest,"%s:%s\n",&numpin[j],net) ;
        }
    }
}


/**************************************/
int reaffect( char* ib, char* net )
/**************************************/

/* force un nom de net pour une pin
  * ib = reference de pin , net = nouveau net ; alim = ligne de consigne
 */
{
    char* pt, * pt0, npin[12];
    int   i;

    pt = alim;

    while( *pt != 0 )
    {
        memset( npin, 0, sizeof(npin) );

        /* recherche separateur (':' ou ',') */
        while( (*pt != ':') && (*pt != ',') )
        {
            pt++; if( *pt == 0 )
                return 0;
        }

        /* suppression des blancs eventuels */
        pt0 = pt; /* save position du nom du net */
        pt0--; while( *pt0 == ' ' )
            pt0--;

        for( i = 3; i >= 0; i-- )
        {
            if( (*pt0 == ',') || (*pt0 ==' ') )
                break;
            npin[i] = *pt0;
            if( pt0 == alim )
                break;
            pt0--;
        }

        while( npin[0] == ' ' ) /* suppression des espaces a gauche */
        {
            npin[0] = npin[1];
            npin[1] = npin[2];
            npin[2] = npin[3];
            npin[3] = ' ';
        }

        if( strncmp( npin, ib, 4 ) == 0 ) /* pin trouv‚e */
        {
            pt++; while( (*pt == ' ' ) && (*pt != 0 ) )
                pt++;

            i = 0;
            while( (*pt != 0) && (*pt != ',') && ( i < 8 ) )
            {
                net[i++] = *pt++;
            }

            net[i] = 0;
            return 1;
        }
        pt++;
    }

    return 0;
}
