/************************************/
/* PCBNEW: traitement des netlistes */
/************************************/

/* Fichier NETLIST.CPP */

/*
 *  Fonction de lecture de la netliste pour:
 *  - Chargement modules et nouvelles connexions
 *  - Test des modules (modules manquants ou en trop
 *  - Recalcul du chevelu
 *
 *  Remarque importante:
 *  Lors de la lecture de la netliste pour Chargement modules
 *  et nouvelles connexions, l'identification des modules peut se faire selon
 *  2 criteres:
 *      - la reference (U2, R5 ..): c'est le mode normal
 *      - le Time Stamp (Signature Temporelle), a utiliser apres reannotation
 *          d'un schema, donc apres modification des references sans pourtant
 *          avoir reellement modifie le schema
 */
#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "protos.h"

#define TESTONLY   1        /* ctes utilisees lors de l'appel a */
#define READMODULE 0        /*  ReadNetModuleORCADPCB */

/* Structures locales */
class MODULEtoLOAD : public EDA_BaseStruct
{
public:
    wxString m_LibName;
    wxString m_CmpName;
    wxString m_Path;

public:
    MODULEtoLOAD( const wxString& libname,
                  const wxString& cmpname,
                  int timestamp,
                  const wxString& path);
    ~MODULEtoLOAD() { };
    MODULEtoLOAD* Next() { return (MODULEtoLOAD*) Pnext; }
};

/* Fonctions locales : */
static void SortListModulesToLoadByLibname( int NbModules );


/* Variables locales */
static int           s_NbNewModules;
static MODULEtoLOAD* s_ModuleToLoad_List;
FILE* source;
static bool          ChangeExistantModule;
static bool          DisplayWarning = TRUE;
static int           DisplayWarningCount;

/*****************************/
/* class WinEDA_NetlistFrame */
/*****************************/
#include "dialog_netlist.cpp"


/*************************************************************************/
void WinEDA_PcbFrame::InstallNetlistFrame( wxDC* DC, const wxPoint& pos )
/*************************************************************************/
{
    WinEDA_NetlistFrame* frame = new WinEDA_NetlistFrame( this, DC );

    frame->ShowModal(); frame->Destroy();
}


/***************************************************************/
bool WinEDA_NetlistFrame::OpenNetlistFile( wxCommandEvent& event )
/***************************************************************/

/*
 *  routine de selection et d'ouverture du fichier Netlist
 */
{
    wxString FullFileName;
    wxString msg;

    if( NetNameBuffer.IsEmpty() ) /* Pas de nom specifie */
        Set_NetlisteName( event );

    if( NetNameBuffer.IsEmpty() )
        return FALSE;                             /* toujours Pas de nom specifie */

    FullFileName = NetNameBuffer;

    source = wxFopen( FullFileName, wxT( "rt" ) );
    if( source == 0 )
    {
        msg.Printf( _( "Netlist file %s not found" ), FullFileName.GetData() );
        DisplayError( this, msg );
        return FALSE;
    }

    msg = wxT( "Netlist " ); msg << FullFileName;
    SetTitle( msg );

    return TRUE;
}


/**************************************************************/
void WinEDA_NetlistFrame::ReadPcbNetlist( wxCommandEvent& event )
/**************************************************************/

/* mise a jour des empreintes :
 *  corrige les Net Names, les textes, les "TIME STAMP"
 *
 *  Analyse les lignes:
 # EESchema Netlist Version 1.0 generee le  18/5/2005-12:30:22
 *  (
 *  ( 40C08647 $noname R20 4,7K {Lib=R}
 *  (    1 VCC )
 *  (    2 MODB_1 )
 *  )
 *  ( 40C0863F $noname R18 4,7_k {Lib=R}
 *  (    1 VCC )
 *  (    2 MODA_1 )
 *  )
 *  }
 #End
 */
{
    int      LineNum, State, Comment;
    MODULE*  Module = NULL;
    D_PAD*   PtPad;
    char     Line[256];
    char*    Text;
    int      UseFichCmp = 1;
    wxString msg;

    ChangeExistantModule = m_ChangeExistantModuleCtrl->GetSelection() == 1 ? TRUE : FALSE;
    DisplayWarning = m_DisplayWarningCtrl->GetValue();
    if( DisplayWarning )
        DisplayWarningCount = 8;
    else
        DisplayWarningCount = 0;

    if( !OpenNetlistFile( event ) )
        return;

    msg = _( "Read Netlist " ) + NetNameBuffer;
    m_MessageWindow->AppendText( msg );

    m_Parent->m_CurrentScreen->SetModify();
    m_Parent->m_Pcb->m_Status_Pcb = 0; State = 0; LineNum = 0; Comment = 0;
    s_NbNewModules = 0;

    /* Premiere lecture de la netliste: etablissement de la liste
     *  des modules a charger
     */
    while( GetLine( source, Line, &LineNum ) )
    {
        Text = StrPurge( Line );

        if( Comment ) /* Commentaires en cours */
        {
            if( ( Text = strchr( Text, '}' ) ) == NULL )
                continue;
            Comment = 0;
        }
        if( *Text == '{' ) /* Commentaires */
        {
            Comment = 1;
            if( ( Text = strchr( Text, '}' ) ) == NULL )
                continue;
        }

        if( *Text == '(' )
            State++;

        if( *Text == ')' )
            State--;

        if( State == 2 )
        {
            Module = ReadNetModule( Text, &UseFichCmp, TESTONLY );
            continue;
        }

        if( State >= 3 ) /* la ligne de description d'un pad est ici non analysee */
        {
            State--;
        }
    }

    /* Chargement des nouveaux modules */
    if( s_NbNewModules )
    {
        LoadListeModules( m_DC );

        // Free module list:
        MODULEtoLOAD* item, * next_item;
        for( item = s_ModuleToLoad_List; item != NULL; item = next_item )
        {
            next_item = item->Next();
            delete item;
        }

        s_ModuleToLoad_List = NULL;
    }

    /* Relecture de la netliste, tous les modules sont ici charges */
    fseek( source, 0, SEEK_SET ); LineNum = 0;
    while( GetLine( source, Line, &LineNum ) )
    {
        Text = StrPurge( Line );

        if( Comment ) /* Commentaires en cours */
        {
            if( ( Text = strchr( Text, '}' ) ) == NULL )
                continue;
            Comment = 0;
        }
        if( *Text == '{' ) /* Commentaires */
        {
            Comment = 1;
            if( ( Text = strchr( Text, '}' ) ) == NULL )
                continue;
        }

        if( *Text == '(' )
            State++;
        if( *Text == ')' )
            State--;

        if( State == 2 )
        {
            Module = ReadNetModule( Text, &UseFichCmp, READMODULE );
            if( Module == NULL )
            {      /* empreinte non trouvee dans la netliste */
                continue;
            }
            else /* Raz netnames sur pads */
            {
                PtPad = Module->m_Pads;
                for( ; PtPad != NULL; PtPad = (D_PAD*) PtPad->Pnext )
                {
                    PtPad->m_Netname = wxEmptyString;
                }
            }
            continue;
        }

        if( State >= 3 )
        {
            if( Module )
            {
                SetPadNetName( Text, Module );
            }
            State--;
        }
    }

    fclose( source );

    /* Mise a jour et Cleanup du circuit imprime: */
    m_Parent->Compile_Ratsnest( m_DC, TRUE );

    if( m_Parent->m_Pcb->m_Track )
    {
        if( m_DeleteBadTracks->GetSelection() == 1 )
        {
            wxBeginBusyCursor();;
            Netliste_Controle_piste( m_Parent, m_DC, TRUE );
            m_Parent->Compile_Ratsnest( m_DC, TRUE );
            wxEndBusyCursor();;
        }
    }

    m_Parent->m_Pcb->Display_Infos( m_Parent );
}


/****************************************************************************/
MODULE* WinEDA_NetlistFrame::ReadNetModule( char* Text, int* UseFichCmp,
                                            int TstOnly )
/****************************************************************************/

/* charge la description d'une empreinte ,netliste type PCBNEW
 *  et met a jour le module correspondant
 *
 *  Si TstOnly == 0 si le module n'existe pas, il est charge
 *  Si TstOnly != 0 si le module n'existe pas, il est ajoute a la liste des
 *      modules a charger
 *  Text contient la premiere ligne de la description
 * UseFichCmp est un flag
 *          si != 0, le fichier des composants .CMP sera utilise
 *          est remis a 0 si le fichier n'existe pas
 *
 *  Analyse les lignes type:
 *  ( 40C08647 $noname R20 4,7K {Lib=R}
 *  (    1 VCC )
 *  (    2 MODB_1 )
 */
{
    MODULE*       Module;
    char*         text;
    wxString      TextTimeStamp;
    wxString      TextNameLibMod;
    wxString      TextValeur;
    wxString      TextCmpName;
    wxString      NameLibCmp;
    unsigned long TimeStamp = 0;
    int           Error = 0;
    char          Line[1024];
    bool          Found;

    strcpy( Line, Text );

    TextValeur = wxT( "~" );

    if( ( text = strtok( Line, " ()\t\n" ) ) == NULL )
        Error = 1;
    else
        TextTimeStamp = CONV_FROM_UTF8( text );

    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        Error = 1;
    else
        TextNameLibMod = CONV_FROM_UTF8( text );

    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        Error = 1;
    else
        TextCmpName = CONV_FROM_UTF8( text );

    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        Error = -1;
    else
        TextValeur = CONV_FROM_UTF8( text );

    if( Error > 0 )
        return NULL;

    wxString LocalTimeStamp = TextTimeStamp.AfterLast('/');
    LocalTimeStamp.ToULong( &TimeStamp, 16 );

    /* Tst si composant deja charge */
    Module = (MODULE*) m_Parent->m_Pcb->m_Modules;
    MODULE* NextModule;
    for( Found = FALSE; Module != NULL; Module = NextModule )
    {
        NextModule = (MODULE*) Module->Pnext;
        if( m_Select_By_Timestamp->GetSelection()  == 1 ) /* Reconnaissance par signature temporelle */
        {
            //if( TimeStamp == Module->m_TimeStamp )
            if(TextTimeStamp.CmpNoCase(Module->m_Path))
                Found = TRUE;
        }
        else    /* Reconnaissance par Reference */
        {
            if( TextCmpName.CmpNoCase( Module->m_Reference->m_Text ) == 0 )
                Found = TRUE;
        }
        if( Found ) // Test si module (m_LibRef) et module specifie en netlist concordent
        {
            if( TstOnly != TESTONLY )
            {
                NameLibCmp = TextNameLibMod;
                if( *UseFichCmp )
                {
                    if( m_Select_By_Timestamp->GetSelection()  == 1 )
                    {   /* Reconnaissance par signature temporelle */
                        *UseFichCmp = ReadListeModules( NULL, TimeStamp, NameLibCmp );
                    }
                    else    /* Reconnaissance par Reference */
                    {
                        *UseFichCmp = ReadListeModules( &TextCmpName, 0l, NameLibCmp );
                    }
                }
                if( Module->m_LibRef.CmpNoCase( NameLibCmp ) != 0 )
                { // Module Mismatch: Current module and module specified in netlist are diff.
                    if( ChangeExistantModule )
                    {
                        MODULE* NewModule =
                            m_Parent->Get_Librairie_Module( this, wxEmptyString, NameLibCmp, TRUE );
                        if( NewModule ) /* Nouveau module trouve : changement de module */
                            Module = m_Parent->Exchange_Module( this, Module, NewModule );
                    }
                    else
                    {
                        wxString msg;
                        msg.Printf(
                            _( "Cmp %s: Mismatch! module is [%s] and netlist said [%s]\n" ),
                            TextCmpName.GetData(), Module->m_LibRef.GetData(),
                            NameLibCmp.GetData() );
                        m_MessageWindow->AppendText( msg );
                        if( DisplayWarningCount > 0 )
                        {
                            DisplayError( this, msg, 2 );
                            DisplayWarningCount--;
                        }
                    }
                }
            }
            break;
        }
    }

    if( Module == NULL )    /* Module a charger */
    {
        NameLibCmp = TextNameLibMod;

        if( *UseFichCmp )
        {
            if( m_Select_By_Timestamp->GetSelection()  == 1 )
            {       /* Reconnaissance par signature temporelle */
                *UseFichCmp = ReadListeModules( NULL, TimeStamp, NameLibCmp );
            }
            else    /* Reconnaissance par Reference */
            {
                *UseFichCmp = ReadListeModules( &TextCmpName, 0l, NameLibCmp );
            }
        }


        if( TstOnly == TESTONLY )
            AddToList( NameLibCmp, TextCmpName, TimeStamp, TextTimeStamp );
        else
        {
            wxString msg;
            msg.Printf( _( "Component [%s] not found" ), TextCmpName.GetData() );
            m_MessageWindow->AppendText( msg + wxT( "\n" ) );
            if( DisplayWarningCount> 0 )
            {
                DisplayError( this, msg, 2 );
                DisplayWarningCount--;
            }
        }
        return NULL;    /* Le module n'avait pas pu etre charge */
    }

    /* mise a jour des reperes ( nom et ref "Time Stamp") si module charge */
    Module->m_Reference->m_Text = TextCmpName;
    Module->m_Value->m_Text = TextValeur;
    Module->m_TimeStamp = TimeStamp;
    Module->m_Path = TextTimeStamp;

#if defined(DEBUG)
    printf("in ReadNetModule() m_Path = %s\n",
           CONV_TO_UTF8(Module->m_Path) );
#endif

    return Module;  /* composant trouve */
}


/********************************************************************/
int WinEDA_NetlistFrame::SetPadNetName( char* Text, MODULE* Module )
/********************************************************************/

/*
 *  Met a jour le netname de 1 pastille, Netliste ORCADPCB
 *  entree :
 *      Text = ligne de netliste lue ( (pad = net) )
 *      Module = adresse de la structure MODULE a qui appartient les pads
 */
{
    D_PAD*   pad;
    char*    TextPinName, * TextNetName;
    int      Error = 0;
    bool     trouve;
    char     Line[256];
    wxString Msg;

    if( Module == NULL )
        return 0;

    strcpy( Line, Text );

    if( ( TextPinName = strtok( Line, " ()\t\n" ) ) == NULL )
        Error = 1;
    if( ( TextNetName = strtok( NULL, " ()\t\n" ) ) == NULL )
        Error = 1;
    if( Error )
        return 0;

    /* recherche du pad */
    pad = Module->m_Pads; trouve = FALSE;
    for( ; pad != NULL; pad = (D_PAD*) pad->Pnext )
    {
        if( strnicmp( TextPinName, pad->m_Padname, 4 ) == 0 )
        { /* trouve */
            trouve = TRUE;
            if( *TextNetName != '?' )
                pad->m_Netname = CONV_FROM_UTF8( TextNetName );
            else
                pad->m_Netname.Empty();
        }
    }

    if( !trouve && (DisplayWarningCount > 0) )
    {
        wxString pin_name = CONV_FROM_UTF8( TextPinName );
        Msg.Printf( _( "Module [%s]: Pad [%s] not found" ),
                   Module->m_Reference->m_Text.GetData(), pin_name.GetData() );
        DisplayError( this, Msg, 1 );
        DisplayWarningCount--;
    }

    return trouve;
}


/*****************************************************/
MODULE* WinEDA_PcbFrame::ListAndSelectModuleName()
/*****************************************************/

/*	liste les noms des modules du PCB
 *  Retourne:
 *      un pointeur sur le module selectionne
 *      NULL si pas de selection
 */
{
    int            ii, jj, nb_empr;
    MODULE*        Module;
    WinEDAListBox* ListBox;
    const wxChar** ListNames = NULL;

    if( m_Pcb->m_Modules == NULL )
    {
        DisplayError( this, _( "No Modules" ) ); return 0;
    }

    /* Calcul du nombre des modules */
    nb_empr = 0; Module = (MODULE*) m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
        nb_empr++;

    ListNames = (const wxChar**) MyZMalloc( (nb_empr + 1) * sizeof(wxChar *) );
    Module    = (MODULE*) m_Pcb->m_Modules;
    for( ii = 0; Module != NULL; Module = (MODULE*) Module->Pnext, ii++ )
    {
        ListNames[ii] = Module->m_Reference->m_Text.GetData();
    }

    ListBox = new WinEDAListBox( this, _( "Components" ),
                                 ListNames, wxEmptyString );
    ii = ListBox->ShowModal(); ListBox->Destroy();


    if( ii < 0 )    /* Pas de selection */
    {
        Module = NULL;
    }
    else /* Recherche du module selectionne */
    {
        Module = (MODULE*) m_Pcb->m_Modules;
        for( jj = 0; Module != NULL; Module = (MODULE*) Module->Pnext, jj++ )
        {
            if( Module->m_Reference->m_Text.Cmp( ListNames[ii] ) == 0 )
                break;
        }
    }

    free( ListNames );
    return Module;
}


/***************************************************************/
void WinEDA_NetlistFrame::ModulesControle( wxCommandEvent& event )
/***************************************************************/

/* donne la liste :
 *  1 - des empreintes doubl�es sur le PCB
 *  2 - des empreintes manquantes par rapport a la netliste
 *  3 - des empreintes suppl�mentaires par rapport a la netliste
 */
#define MAX_LEN_TXT 32
{
    int               ii, NbModulesPcb;
    MODULE*           Module, * pt_aux;
    int               NbModulesNetListe, nberr = 0;
    WinEDA_TextFrame* List;
    wxArrayString     ModuleListFromNetlist;

    /* determination du nombre des modules du PCB*/
    NbModulesPcb = 0; Module = (MODULE*) m_Parent->m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
        NbModulesPcb++;

    if( NbModulesPcb == 0 )
    {
        DisplayError( this, _( "No modules" ), 10 ); return;
    }

    /* Construction de la liste des references des modules de la netliste */
    NbModulesNetListe = BuildListeNetModules( event, ModuleListFromNetlist );
    if( NbModulesNetListe < 0 )
        return;                         /* fichier non trouve */

    if( NbModulesNetListe == 0 )
    {
        DisplayError( this, _( "No modules in NetList" ), 10 ); return;
    }

    List = new WinEDA_TextFrame( this, _( "Check Modules" ) );

    /* recherche des doubles */
    List->Append( _( "Duplicates" ) );

    Module = (MODULE*) m_Parent->m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
    {
        pt_aux = (MODULE*) Module->Pnext;
        for( ; pt_aux != NULL; pt_aux = (MODULE*) pt_aux->Pnext )
        {
            if( Module->m_Reference->m_Text.CmpNoCase( pt_aux->m_Reference->m_Text ) == 0 )
            {
                List->Append( Module->m_Reference->m_Text );
                nberr++;
                break;
            }
        }
    }

    /* recherche des manquants par rapport a la netliste*/
    List->Append( _( "Lack:" ) );

    for( ii = 0; ii < NbModulesNetListe; ii++ )
    {
        Module = (MODULE*) m_Parent->m_Pcb->m_Modules;
        for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
        {
            if( Module->m_Reference->m_Text.CmpNoCase(
                    ModuleListFromNetlist[ii] ) == 0 )
            {
                break;
            }
        }

        if( Module == NULL )    // Module missing, not found in board
        {
            List->Append( ModuleListFromNetlist[ii] );
            nberr++;
        }
    }

    /* recherche des modules supplementaires (i.e. Non en Netliste) */
    List->Append( _( "Not in Netlist:" ) );

    Module = (MODULE*) m_Parent->m_Pcb->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        for( ii = 0; ii < NbModulesNetListe; ii++ )
        {
            if( Module->m_Reference->m_Text.CmpNoCase(
                    ModuleListFromNetlist[ii] ) == 0 )
            {
                break;/* Module trouve en netliste */
            }
        }

        if( ii == NbModulesNetListe )   /* Module not found in netlist */
        {
            List->Append( Module->m_Reference->m_Text );
            nberr++;
        }
    }

    List->ShowModal(); List->Destroy();
}


/**************************************************************************************************/
int WinEDA_NetlistFrame::BuildListeNetModules( wxCommandEvent& event, wxArrayString& BufName )
/**************************************************************************************************/

/*
 *  charge en BufName la liste des noms des modules de la netliste,
 *  retourne le nombre des modules cit�s dans la netliste
 */
{
    int  textlen;
    int  nb_modules_lus;
    int  State, LineNum, Comment;
    char Line[1024], * Text, * LibModName;

    if( !OpenNetlistFile( event ) )
        return -1;

    State = 0; LineNum = 0; Comment = 0;
    nb_modules_lus = 0;
    textlen = MAX_LEN_TXT;

    while( GetLine( source, Line, &LineNum ) )
    {
        Text = StrPurge( Line );
        if( Comment ) /* Commentaires en cours */
        {
            if( ( Text = strchr( Text, '}' ) ) == NULL )
                continue;
            Comment = 0;
        }
        if( *Text == '{' ) /* Commentaires */
        {
            Comment = 1;
            if( ( Text = strchr( Text, '}' ) ) == NULL )
                continue;
        }

        if( *Text == '(' )
            State++;
        if( *Text == ')' )
            State--;

        if( State == 2 )
        {
            int Error = 0;
            if( strtok( Line, " ()\t\n" ) == NULL )
                Error = 1;                                                  /* TimeStamp */
            if( ( LibModName = strtok( NULL, " ()\t\n" ) ) == NULL )
                Error = 1;                                                  /* nom Lib */
            /* Lecture du nom (reference) du composant: */
            if( ( Text = strtok( NULL, " ()\t\n" ) ) == NULL )
                Error = 1;
            nb_modules_lus++;
            BufName.Add( CONV_FROM_UTF8( Text ) );
            continue;
        }

        if( State >= 3 )
        {
            State--; /* Lecture 1 ligne relative au Pad */
        }
    }

    fclose( source );
    return nb_modules_lus;
}


/*****************************************************************************************/
int WinEDA_NetlistFrame::ReadListeModules( const wxString* RefCmp, long TimeStamp,
                                           wxString& NameModule )
/*****************************************************************************************/

/*
 *  Lit le fichier .CMP donnant l'equivalence Modules / Composants
 *  Retourne:
 *  Si ce fichier existe:
 *      1 et le nom module dans NameModule
 *      -1 si module non trouve en fichier
 *  sinon 0;
 *
 *  parametres d'appel:
 *      RefCmp		(NULL si selection par TimeStamp)
 *      TimeStamp	(signature temporelle si elle existe, NULL sinon)
 *      pointeur sur le buffer recevant le nom du module
 *
 *  Exemple de fichier:
 *
 *  Cmp-Mod V01 Genere par PcbNew le 29/10/2003-13:11:6
 *
 *  BeginCmp
 *  TimeStamp = 322D3011;
 *  Reference = BUS1;
 *  ValeurCmp = BUSPC;
 *  IdModule  = BUS_PC;
 *  EndCmp
 *
 *  BeginCmp
 *  TimeStamp = 32307DE2;
 *  Reference = C1;
 *  ValeurCmp = 47uF;
 *  IdModule  = CP6;
 *  EndCmp
 *
 */
{
    wxString CmpFullFileName;
    wxString refcurrcmp, idmod;
    char     ia[1024];
    int      timestamp;
    char*    ptcar;
    FILE*    FichCmp;

    if( (RefCmp == NULL) && (TimeStamp == 0) )
        return 0;

    CmpFullFileName = NetNameBuffer;
    ChangeFileNameExt( CmpFullFileName, NetCmpExtBuffer );

    FichCmp = wxFopen( CmpFullFileName, wxT( "rt" ) );
    if( FichCmp == NULL )
    {
        wxString msg;
        msg.Printf( _( "File <%s> not found, use Netlist for lib module selection" ),
                   CmpFullFileName.GetData() );
        DisplayError( this, msg, 20 );
        return 0;
    }

    while( fgets( ia, sizeof(ia), FichCmp ) != NULL )
    {
        if( strnicmp( ia, "BeginCmp", 8 ) != 0 )
            continue;

        /* Ici une description de 1 composant commence */
        refcurrcmp.Empty();
        idmod.Empty();
        timestamp = -1;
        while( fgets( ia, sizeof(ia), FichCmp ) != NULL )
        {
            if( strnicmp( ia, "EndCmp", 6 ) == 0 )
                break;

            if( strnicmp( ia, "Reference =", 11 ) == 0 )
            {
                ptcar = ia + 11;
                ptcar = strtok( ptcar, " =;\t\n" );
                if( ptcar )
                    refcurrcmp = CONV_FROM_UTF8( ptcar );
                continue;
            }

            if( strnicmp( ia, "IdModule  =", 11 ) == 0 )
            {
                ptcar = ia + 11;
                ptcar = strtok( ptcar, " =;\t\n" );
                if( ptcar )
                    idmod = CONV_FROM_UTF8( ptcar );
                continue;
            }
            if( strnicmp( ia, "TimeStamp =", 11 ) == 0 )
            {
                ptcar = ia + 11;
                ptcar = strtok( ptcar, " =;\t\n" );
                if( ptcar )
                    sscanf( ptcar, "%X", &timestamp );
            }
        }

        /* Fin lecture 1 descr composant */

        /* Test du Composant lu en fichier: est-il le bon */
        if( RefCmp )
        {
            if( RefCmp->CmpNoCase( refcurrcmp ) == 0 )
            {
                fclose( FichCmp );
                NameModule = idmod;
                return 1;
            }
        }
        else if( TimeStamp != -1 )
        {
            if( TimeStamp == timestamp )
            {
                fclose( FichCmp );
                NameModule = idmod;
                return 1;
            }
        }
    }

    fclose( FichCmp );
    return -1;
}


/***************************************************************/
void WinEDA_NetlistFrame::Set_NetlisteName( wxCommandEvent& event )
/***************************************************************/

/* Selection un nouveau nom de netliste
 *  Affiche la liste des fichiers netlistes pour selection sur liste
 */
{
    wxString fullfilename, mask( wxT( "*" ) );

    mask += NetExtBuffer;

    fullfilename = EDA_FileSelector( _( "Netlist Selection:" ),
                                     wxEmptyString,     /* Chemin par defaut */
                                     NetNameBuffer,     /* nom fichier par defaut */
                                     NetExtBuffer,      /* extension par defaut */
                                     mask,              /* Masque d'affichage */
                                     this,
                                     0,
                                     TRUE
                                     );

    if( fullfilename.IsEmpty() )
        return;
    NetNameBuffer = fullfilename;
    SetTitle( fullfilename );
}


/***********************************************************************************/
void WinEDA_NetlistFrame::AddToList( const wxString& NameLibCmp, const wxString& CmpName,
                                     int TimeStamp, const wxString& path)
/************************************************************************************/

/* Fontion copiant en memoire de travail les caracteristiques
 *  des nouveaux modules
 */
{
    MODULEtoLOAD* NewMod;

    NewMod = new MODULEtoLOAD( NameLibCmp, CmpName, TimeStamp, path );
    NewMod->Pnext = s_ModuleToLoad_List;
    s_ModuleToLoad_List = NewMod;
    s_NbNewModules++;
}


/***************************************************************/
void WinEDA_NetlistFrame::LoadListeModules( wxDC* DC )
/***************************************************************/

/* Routine de chargement des nouveaux modules en une seule lecture des
 *  librairies
 *  Si un module vient d'etre charge il est duplique, ce qui evite une lecture
 *  inutile de la librairie
 */
{
    MODULEtoLOAD* ref, * cmp;
    int           ii;
    MODULE*       Module = NULL;
    wxPoint       OldPos = m_Parent->m_CurrentScreen->m_Curseur;

    if( s_NbNewModules == 0 )
        return;

    SortListModulesToLoadByLibname( s_NbNewModules );
    ref = cmp = s_ModuleToLoad_List;

    // Calcul de la coordonn�e de placement des modules:
    if( m_Parent->SetBoardBoundaryBoxFromEdgesOnly() )
    {
        m_Parent->m_CurrentScreen->m_Curseur.x = m_Parent->m_Pcb->m_BoundaryBox.GetRight() + 5000;
        m_Parent->m_CurrentScreen->m_Curseur.y = m_Parent->m_Pcb->m_BoundaryBox.GetBottom() +
                                                 10000;
    }
    else
    {
        m_Parent->m_CurrentScreen->m_Curseur = wxPoint( 0, 0 );
    }

    for( ii = 0; ii < s_NbNewModules; ii++, cmp = cmp->Next() )
    {
        if( (ii == 0) || ( ref->m_LibName != cmp->m_LibName) )
        {   /* Nouveau Module a charger */
            Module = m_Parent->Get_Librairie_Module( this, wxEmptyString, cmp->m_LibName, FALSE );
            ref    = cmp;
            if( Module == NULL )
            {
                wxString msg;
                msg.Printf( _( "Component [%s]: footprint <%s> not found" ),
                           cmp->m_CmpName.GetData(), cmp->m_LibName.GetData() );
                DisplayError( this, msg );
                continue;
            }
            m_Parent->Place_Module( Module, DC );

            /* mise a jour des reperes ( nom et ref "Time Stamp")
             *  si module charge */
            Module->m_Reference->m_Text = cmp->m_CmpName;
            Module->m_TimeStamp = cmp->m_TimeStamp;
            Module->m_Path = cmp->m_Path;
        }
        else
        {
            /* module deja charge, on peut le dupliquer */
            MODULE* newmodule;
            if( Module == NULL )
                continue;                   /* module non existant en libr */
            newmodule = new MODULE( m_Parent->m_Pcb );
            newmodule->Copy( Module );
            newmodule->AddToChain( Module );
            Module = newmodule;
            Module->m_Reference->m_Text = cmp->m_CmpName;
            Module->m_TimeStamp = cmp->m_TimeStamp;
            Module->m_Path = cmp->m_Path;
        }
    }

    m_Parent->m_CurrentScreen->m_Curseur = OldPos;
}


/* Routine utilisee par qsort pour le tri des modules a charger
 */
static int SortByLibName( MODULEtoLOAD** ref, MODULEtoLOAD** cmp )
{
    int ii = (*ref)->m_LibName.CmpNoCase( (*cmp)->m_LibName );

    return ii;
}


/*************************************************/
void SortListModulesToLoadByLibname( int NbModules )
/**************************************************/

/* Rearrage la liste des modules List par ordre alphabetique des noms lib des modules
 */
{
    MODULEtoLOAD** base_list, * item;
    int            ii;

    base_list = (MODULEtoLOAD**) MyMalloc( NbModules * sizeof(MODULEtoLOAD *) );

    for( ii = 0, item = s_ModuleToLoad_List; ii < NbModules; ii++ )
    {
        base_list[ii] = item;
        item = item->Next();
    }

    qsort( base_list, NbModules, sizeof(MODULEtoLOAD *),
           ( int( * ) ( const void*, const void* ) )SortByLibName );

    // Reconstruction du chainage:
    s_ModuleToLoad_List = *base_list;
    for( ii = 0; ii < NbModules - 1; ii++ )
    {
        item = base_list[ii];
        item->Pnext = base_list[ii + 1];
    }

    // Dernier item: Pnext = NULL:
    item = base_list[ii];
    item->Pnext = NULL;

    free( base_list );
}


/*****************************************************************************/
MODULEtoLOAD::MODULEtoLOAD( const wxString& libname, const wxString& cmpname,
                            int timestamp, const wxString& path ) : EDA_BaseStruct( TYPE_NOT_INIT )
/*****************************************************************************/
{
    m_LibName   = libname;
    m_CmpName   = cmpname;
    m_TimeStamp = timestamp;
    m_Path 		= path;
}
