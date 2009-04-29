/********************************************************/
/**** Routine de lecture et visu d'un fichier GERBER ****/
/********************************************************/

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"

/* Format Gerber : NOTES :
 *  Fonctions preparatoires:
 *  Gn =
 *  G01			interpolation lineaire ( trace de droites )
 *  G02,G20,G21	Interpolation circulaire , sens trigo < 0
 *  G03,G30,G31	Interpolation circulaire , sens trigo > 0
 *  G04			commentaire
 *  G06			Interpolation parabolique
 *  G07			Interpolation cubique
 *  G10			interpolation lineaire ( echelle 10x )
 *  G11			interpolation lineaire ( echelle 0.1x )
 *  G12			interpolation lineaire ( echelle 0.01x )
 *  G52			plot symbole reference par Dnn code
 *  G53			plot symbole reference par Dnn ; symbole tourne de -90 degres
 *  G54			Selection d'outil
 *  G55			Mode exposition photo
 *  G56			plot symbole reference par Dnn A code
 *  G57			affiche le symbole reference sur la console
 *  G58			plot et affiche le symbole reference sur la console
 *  G60			interpolation lineaire ( echelle 100x )
 *  G70			Unites = Inches
 *  G71			Unites = Millimetres
 *  G74			supprime interpolation circulaire sur 360 degre, revient a G01
 *  G75			Active interpolation circulaire sur 360 degre
 *  G90			Mode Coordonnees absolues
 *  G91			Mode Coordonnees Relatives
 *
 *  Coordonnees X,Y
 *  X,Y sont suivies de + ou - et de m+n chiffres (non separes)
 *          m = partie entiere
 *          n = partie apres la virgule
 *           formats classiques : 	m = 2, n = 3 (format 2.3)
 *                                  m = 3, n = 4 (format 3.4)
 *  ex:
 *  G__ X00345Y-06123 D__*
 *
 *  Outils et D_CODES
 *  numero d'outil ( identification des formes )
 *  1 a 99 	(classique)
 *  1 a 999
 *  D_CODES:
 *
 *  D01 ... D9 = codes d'action:
 *  D01			= activation de lumiere (baisser de plume) lors du d�placement
 *  D02			= extinction de lumiere (lever de plume) lors du d�placement
 *  D03			= Flash
 *  D09			= VAPE Flash
 *  D51			= precede par G54 -> Select VAPE
 *
 *  D10 ... D255 = Indentification d'outils ( d'ouvertures )
 *              Ne sont pas tj dans l'ordre ( voir tableau dans PCBPLOT.H)
 */


/* Variables locales : */

/* Routines Locales */


/* Routine de Lecture d'un fichier de D Codes.
 *  Accepte format standard ou ALSPCB
 *      un ';' demarre un commentaire.
 *
 *  Format Standard:
 *  tool,	 Horiz,		  Vert,	  drill, vitesse, acc. ,Type ; [DCODE (commentaire)]
 *  ex:	   1,		  12,		12,		0,		  0,	 0,	  3 ; D10
 *
 *  Format ALSPCB:
 *  Ver  ,  Hor , Type , Tool [,Drill]
 *  ex:	0.012, 0.012,  L   , D10
 *
 *  Classe les caract en buf_tmp sous forme de tableau de structures D_CODE.
 *  Retourne:
 *      < 0 si erreur:
 *          -1 = Fichier non trouve
 *          -2 = Erreur lecture fichier
 *      Rang de D_code maxi lu ( nbr de dcodes )
 *
 *
 *  Representation interne:
 *
 *  Les lignes sont repr�sent�es par des TRACKS standards
 *  Les Flash  sont repr�sent�es par des DRAWSEGMENTS
 *      - ronds ou ovales: DRAWSEGMENTS
 *      - rectangles: DRAWSEGMENTS
 *  la reference aux D-CODES est plac�e dans le membre GetNet()
 */


/********************************************************/
bool WinEDA_GerberFrame::Read_GERBER_File( wxDC*           DC,
                                           const wxString& GERBER_FullFileName,
                                           const wxString& D_Code_FullFileName )
/********************************************************/

/* Read a gerber file (RS274D or RS274X format).
 *  Normal format:
 *      Imperial
 *      Absolute
 *      end of block = *
 *      CrLf after each command
 *      G codes repetes
 */
{
    int           G_commande = 0,
                  D_commande = 0;       /* command number for G et D commands (like G04 or D02) */

    char          line[GERBER_BUFZ];

    wxString      msg;
    char*         text;
    int           layer;    /* current layer used in gerbview */
    GERBER*       gerber;
    wxPoint       pos;
    int           error = 0;

    layer = GetScreen()->m_Active_Layer;

    if( g_GERBER_List[layer] == NULL )
    {
        g_GERBER_List[layer] = new GERBER( layer );
    }

    gerber = g_GERBER_List[layer];

    /* Set the gerber scale: */
    gerber->ResetDefaultValues();

    /* Read the gerber file */
    gerber->m_Current_File = wxFopen( GERBER_FullFileName, wxT( "rt" ) );
    if( gerber->m_Current_File == 0 )
    {
        msg = _( "File " ) + GERBER_FullFileName + _( " not found" );
        DisplayError( this, msg, 10 );
        return FALSE;
    }

    gerber->m_FileName = GERBER_FullFileName;

    wxString path = wxPathOnly( GERBER_FullFileName );
    if( path != wxEmptyString )
        wxSetWorkingDirectory( path );

    wxBusyCursor show_wait;
    SetLocaleTo_C_standard( );

    while( TRUE )
    {
        if( fgets( line, sizeof(line), gerber->m_Current_File ) == NULL ) // E.O.F
        {
            if( gerber->m_FilesPtr == 0 )
                break;

            fclose( gerber->m_Current_File );

            gerber->m_FilesPtr--;
            gerber->m_Current_File =
                gerber->m_FilesList[gerber->m_FilesPtr];

            continue;
        }

        text = StrPurge( line );

        while( text && *text )
        {
            switch( *text )
            {
            case ' ':
            case '\r':
            case '\n':
                text++;
                break;

            case '*':       // End commande
                gerber->m_CommandState = END_BLOCK;
                text++;
                break;

            case 'M':       // End file
                gerber->m_CommandState = CMD_IDLE;
                while( *text )
                    text++;

                break;

            case 'G':    /* Line type Gxx : command */
                G_commande = gerber->ReturnGCodeNumber( text );
                gerber->Execute_G_Command( text, G_commande );
                break;

            case 'D':       /* Line type Dxx : Tool selection (xx > 0) or command if xx = 0..9*/
                D_commande = gerber->ReturnDCodeNumber( text );
                gerber->Execute_DCODE_Command( this, DC, text, D_commande );
                break;

            case 'X':
            case 'Y':                   /* Move or draw command */
                pos = gerber->ReadXYCoord( text );
                if( *text == '*' )      // command like X12550Y19250*
                {
                    gerber->Execute_DCODE_Command( this, DC, text,
                                                   gerber->m_Last_Pen_Command );
                }
                break;

            case 'I':
            case 'J':       /* Auxiliary Move command */
                pos = gerber->ReadIJCoord( text );
                break;

            case '%':
                if( gerber->m_CommandState != ENTER_RS274X_CMD )
                {
                    gerber->m_CommandState = ENTER_RS274X_CMD;

                    if( !gerber->ReadRS274XCommand( this, DC, line, text ) )
                    {
                        error++;
                    }
                }
                else        //Error
                {
                    error++;
                    gerber->m_CommandState = CMD_IDLE;
                    text++;
                }
                break;

            default:
                text++;
                error++;
                break;
            }
        }
    }

    if( error )
    {
        msg.Printf( _( "%d errors while reading Gerber file [%s]" ),
                    error, GERBER_FullFileName.GetData() );
        DisplayError( this, msg );
    }
    fclose( gerber->m_Current_File );

    SetLocaleTo_Default( );

    /* Init  DCodes list and perhaps read a DCODES file,
     * if the gerber file is only a RS274D file (without any aperture information)
     */
    if( !gerber->m_Has_DCode )
    {
        wxFileName fn;

        if( D_Code_FullFileName.IsEmpty() )
        {
            wxString wildcard;

            fn = GERBER_FullFileName;
            fn.SetExt( g_PenFilenameExt );
            wildcard.Printf( _( "Gerber DCODE files (%s)|*.%s" ),
                             g_PenFilenameExt.c_str(), g_PenFilenameExt.c_str());
            wildcard += AllFilesWildcard;

            wxFileDialog dlg( this, _( "Load GERBER DCODE File" ),
                              wxEmptyString, fn.GetFullName(), wildcard,
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST );
        }
        else
            fn = D_Code_FullFileName;

        if( fn.IsOk() )
        {
            Read_D_Code_File( fn.GetFullPath() );
            CopyDCodesSizeToItems();
        }
        else
            return false;
    }

    return true;
}
