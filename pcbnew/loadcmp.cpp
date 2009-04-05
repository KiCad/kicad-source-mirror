/**********************************************/
/* Footprints selection and loading functions */
/**********************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eda_doc.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "get_component_dialog.h"
#include "appl_wxstruct.h"

#include "pcbnew.h"
#include "protos.h"

class ModList
{
public:
    ModList* Next;
    wxString m_Name, m_Doc, m_KeyWord;

public:
    ModList()
    {
        Next = NULL;
    }


    ~ModList()
    {
    }
};

/* Fonctions locales */
static void DisplayCmpDoc( wxString& Name );
static void ReadDocLib( const wxString& ModLibName );

/*****/

/* variables locales */
static ModList* MList;


/***************************************************************************/
void WinEDA_ModuleEditFrame::Load_Module_Module_From_BOARD( MODULE* Module )
/***************************************************************************/
{
    MODULE* NewModule;
    WinEDA_BasePcbFrame* parent = (WinEDA_BasePcbFrame*) GetParent();

    if( Module == NULL )
    {
        if( parent->GetBoard() == NULL || parent->GetBoard()->m_Modules == NULL )
            return;

        Module = Select_1_Module_From_BOARD( parent->GetBoard() );
    }

    if( Module == NULL )
        return;

    SetCurItem( NULL );

    Clear_Pcb( TRUE );

    GetBoard()->m_Status_Pcb = 0;
    NewModule = new MODULE( GetBoard() );
    NewModule->Copy( Module );
    NewModule->m_Link = Module->m_TimeStamp;

    Module = NewModule;

    GetBoard()->Add( Module );

    Module->m_Flags = 0;

    build_liste_pads();

    GetScreen()->m_Curseur.x = GetScreen()->m_Curseur.y = 0;
    Place_Module( Module, NULL );
    if( Module->GetLayer() != CMP_N )
        GetBoard()->Change_Side_Module( Module, NULL );
    Rotate_Module( NULL, Module, 0, FALSE );
    GetScreen()->ClrModify();
    Zoom_Automatique( TRUE );
}


/****************************************************************************/
MODULE* WinEDA_BasePcbFrame::Load_Module_From_Library( const wxString& library,
                                                       wxDC*           DC )
/****************************************************************************/
/* Permet de charger un module directement a partir de la librairie */
{
    MODULE*              module;
    wxPoint              curspos = GetScreen()->m_Curseur;
    wxString             ModuleName, keys;
    static wxArrayString HistoryList;
    bool AllowWildSeach = TRUE;

    /* Ask for a component name or key words */
    ModuleName = GetComponentName( this, HistoryList, _( "Place module" ), NULL );
    ModuleName.MakeUpper();
    if( ModuleName.IsEmpty() )  /* Cancel command */
    {
        DrawPanel->MouseToCursorSchema();
        return NULL;
    }


    if( ModuleName[0] == '=' )   // Selection by keywords
    {
        AllowWildSeach = FALSE;
        keys = ModuleName.AfterFirst( '=' );
        ModuleName = Select_1_Module_From_List( this, library, wxEmptyString, keys );
        if( ModuleName.IsEmpty() )  /* Cancel command */
        {
            DrawPanel->MouseToCursorSchema();
            return NULL;
        }
    }
    else if( ( ModuleName.Contains( wxT( "?" ) ) ) || ( ModuleName.Contains( wxT( "*" ) ) ) ) // Selection wild card
    {
        AllowWildSeach = FALSE;
        ModuleName = Select_1_Module_From_List( this, library, ModuleName, wxEmptyString );
        if( ModuleName.IsEmpty() )
        {
            DrawPanel->MouseToCursorSchema();
            return NULL;    /* annulation de commande */
        }
    }

    module = Get_Librairie_Module( this, library, ModuleName, FALSE );

    if( (module == NULL) && AllowWildSeach )    /* Attemp to search with wildcard */
    {
        AllowWildSeach = FALSE;
        wxString wildname = wxChar( '*' ) + ModuleName + wxChar( '*' );
        ModuleName = wildname;
        ModuleName = Select_1_Module_From_List( this, library, ModuleName, wxEmptyString );
        if( ModuleName.IsEmpty() )
        {
            DrawPanel->MouseToCursorSchema();
            return NULL;    /* annulation de commande */
        }
        else
            module = Get_Librairie_Module( this, library, ModuleName, TRUE );
    }

    GetScreen()->m_Curseur = curspos;
    DrawPanel->MouseToCursorSchema();

    if( module )
    {
        AddHistoryComponentName( HistoryList, ModuleName );

        module->m_Flags     = IS_NEW;
        module->m_Link      = 0;
        module->m_TimeStamp = GetTimeStamp();
        GetBoard()->m_Status_Pcb = 0;
        module->SetPosition( curspos );
        build_liste_pads();
        module->Draw( DrawPanel, DC, GR_OR );
    }

    return module;
}


/*****************************************************************************
 *
 *  Analyse les LIBRAIRIES pour trouver le module demande
 *  Si ce module est trouve, le copie en memoire, et le
 *  chaine en fin de liste des modules
 *      - Entree:
 *          name_cmp = nom du module
 *      - Retour:
 *          Pointeur sur le nouveau module.
 *
 *****************************************************************************/
MODULE* WinEDA_BasePcbFrame::Get_Librairie_Module( wxWindow* winaff,
                                                   const wxString& library,
                                                   const wxString& ModuleName,
                                                   bool show_msg_err )
{
    int        LineNum, Found = 0;
    wxFileName fn;
    char       Line[512];
    wxString   Name;
    wxString   msg, tmp;
    MODULE*    NewModule;
    FILE*      file = NULL;
    unsigned   ii;

    for( ii = 0; ii < g_LibName_List.GetCount(); ii++ )
    {
        fn = wxFileName( wxEmptyString, g_LibName_List[ii],
                         ModuleFileExtension );

        tmp = wxGetApp().GetLibraryPathList().FindValidPath( fn.GetFullName() );

        if( !tmp )
        {
            msg.Printf( _( "PCB footprint library file <%s> not found in " \
                           "search paths." ), fn.GetFullName().c_str() );
            wxMessageBox( msg, _( "Library Load Error" ),
                          wxOK | wxICON_ERROR, this );
            continue;
        }

        file = wxFopen( tmp, wxT( "rt" ) );

        if( file == NULL )
        {
            msg.Printf( _( "Could not open PCB footprint library file <%s>." ),
                        tmp.c_str() );
            wxMessageBox( msg, _( "Library Load Error" ),
                          wxOK | wxICON_ERROR, this );
            continue;
        }

        msg.Printf( _( "Scan Lib: %s" ), tmp.c_str() );
        Affiche_Message( msg );

        /* lecture entete chaine definie par ENTETE_LIBRAIRIE */
        LineNum = 0;
        GetLine( file, Line, &LineNum );
        StrPurge( Line );
        if( strnicmp( Line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
        {
            msg.Printf( _( "<%s> is not a valid Kicad PCB footprint library " \
                           "file." ), tmp.c_str() );
            wxMessageBox( msg, _( "Library Load Error" ),
                          wxOK | wxICON_ERROR, this );
            fclose( file );
            return NULL;
        }

        /* Lecture de la liste des composants de la librairie */
        Found = 0;
        while( !Found && GetLine( file, Line, &LineNum ) )
        {
            if( strnicmp( Line, "$MODULE", 6 ) == 0 )
                break;
            if( strnicmp( Line, "$INDEX", 6 ) == 0 )
            {
                while( GetLine( file, Line, &LineNum ) )
                {
                    if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
                        break;
                    StrPurge( Line );
                    msg = CONV_FROM_UTF8( Line );
                    if( msg.CmpNoCase( ModuleName ) == 0 )
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
            Name = CONV_FROM_UTF8( Line + 8 );

            if( Name.CmpNoCase( ModuleName ) == 0 )  /* composant localise */
            {
                NewModule = new MODULE( GetBoard() );

                // Switch the locale to standard C (needed to print
                // floating point numbers like 1.3)
                SetLocaleTo_C_standard( );
                NewModule->ReadDescr( file, &LineNum );
                SetLocaleTo_Default( );        // revert to the current locale
                GetBoard()->Add( NewModule, ADD_APPEND );
                fclose( file );
                Affiche_Message( wxEmptyString );
                return NewModule;
            }
        }

        fclose( file );
    }

    if( show_msg_err )
    {
        msg.Printf( _( "Module <%s> not found" ), ModuleName.c_str() );
        DisplayError( winaff, msg );
    }

    return NULL;
}


/***************************************************************/
wxString WinEDA_BasePcbFrame::Select_1_Module_From_List(
    WinEDA_DrawFrame* active_window,
    const wxString& Library,
    const wxString& Mask, const wxString& KeyWord )
/***************************************************************/

/*
 *  Affiche la liste des modules des librairies
 *  Recherche dans la librairie Library ou generale si Library == NULL
 *  Mask = Filtre d'affichage( Mask = wxEmptyString pour listage non filtré )
 *  KeyWord = Liste de mots cles, Recherche limitee aux composants
 *      ayant ces mots cles ( KeyWord = wxEmptyString pour listage de tous les modules )
 *
 *  retourne wxEmptyString si abort ou probleme
 *  ou le nom du module
 */
{
    int             LineNum;
    unsigned        ii, NbModules;
    char            Line[1024];
    wxFileName      fn;
    static wxString OldName;/* Memorise le nom du dernier composant charge */
    wxString        CmpName, tmp;
    FILE*           file;
    wxString        msg;

    WinEDAListBox*  ListBox = new WinEDAListBox( active_window, wxEmptyString,
                                                 NULL, OldName, DisplayCmpDoc,
                                                 wxColour( 200, 200, 255 ) );

    wxBeginBusyCursor();

    /* Recherche des composants en librairies */
    NbModules = 0;
    for( ii = 0; ii < g_LibName_List.GetCount(); ii++ )
    {
        /* Calcul du nom complet de la librairie */
        if( Library.IsEmpty() )
        {
            fn = wxFileName( wxEmptyString, g_LibName_List[ii],
                             ModuleFileExtension );
        }
        else
            fn = wxFileName( wxEmptyString, Library, ModuleFileExtension );


        tmp = wxGetApp().GetLibraryPathList().FindValidPath( fn.GetFullName() );

        if( !tmp )
        {
            msg.Printf( _( "PCB footprint library file <%s> not found in " \
                           "search paths." ), fn.GetFullName().c_str() );
            wxMessageBox( msg, _( "Library Load Error" ),
                          wxOK | wxICON_ERROR, this );
            continue;
        }

        ReadDocLib( tmp );

        if( !KeyWord.IsEmpty() )    /* Inutile de lire la librairie si selection
                                     *  par mots cles, deja lus */
        {
            if( !Library.IsEmpty() )
                break;
            continue;
        }

        file = wxFopen( tmp, wxT( "rt" ) );

        if( file  == NULL )
        {
            if( !Library.IsEmpty() )
                break;
            continue;
        }

        // Statusbar library loaded message
        msg = _( "Library " ) + fn.GetFullPath() + _(" loaded");
        Affiche_Message( msg );

        /* lecture entete */
        LineNum = 0;
        GetLine( file, Line, &LineNum, sizeof(Line) - 1 );

        if( strnicmp( Line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
        {
            msg.Printf( _( "<%s> is not a valid Kicad PCB footprint library " \
                           "file." ), tmp.c_str() );
            wxMessageBox( msg, _( "Library Load Error" ),
                          wxOK | wxICON_ERROR, this );
            fclose( file );
            continue;
        }

        /* Lecture de la librairie */
        while( GetLine( file, Line, &LineNum, sizeof(Line) - 1 ) )
        {
            if( Line[0] != '$' )
                continue;
            if( strnicmp( Line, "$MODULE", 6 ) == 0 )
                break;
            if( strnicmp( Line, "$INDEX", 6 ) == 0 )
            {
                while( GetLine( file, Line, &LineNum ) )
                {
                    if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
                        break;
                    strupper( Line );
                    msg = CONV_FROM_UTF8( StrPurge( Line ) );
                    if( Mask.IsEmpty() )
                    {
                        ListBox->Append( msg );
                        NbModules++;
                    }
                    else if( WildCompareString( Mask, msg, FALSE ) )
                    {
                        ListBox->Append( msg );
                        NbModules++;
                    }
                }
            } /* Fin Lecture INDEX */
        }

        /* Fin lecture 1 Librairie */
        fclose( file );
        file = NULL;

        if( !Library.IsEmpty() )
            break;
    }

    /*  creation de la liste des modules si recherche par mots-cles */
    if( !KeyWord.IsEmpty() )
    {
        ModList* ItemMod = MList;
        while( ItemMod != NULL )
        {
            if( KeyWordOk( KeyWord, ItemMod->m_KeyWord ) )
            {
                NbModules++;
                ListBox->Append( ItemMod->m_Name );
            }
            ItemMod = ItemMod->Next;
        }
    }

    wxEndBusyCursor();

    msg.Printf( _( "Modules [%d items]" ), NbModules );
    ListBox->SetTitle( msg );
    ListBox->SortList();

    ii = ListBox->ShowModal();
    if( ii >= 0 )
        CmpName = ListBox->GetTextSelection();
    else
        CmpName.Empty();

    ListBox->Destroy();

    /* liberation mem de la liste des textes doc module */
    while( MList != NULL )
    {
        ModList* NewMod = MList->Next;
        delete MList;
        MList = NewMod;
    }

    if( CmpName != wxEmptyString )
        OldName = CmpName;

    return CmpName;
}


/******************************************/
static void DisplayCmpDoc( wxString& Name )
/*******************************************/

/* Routine de recherche et d'affichage de la doc du composant Name
 *  La liste des doc est pointee par MList
 */
{
    ModList* Mod = MList;

    if( !Mod )
    {
        Name.Empty();
        return;
    }

    /* Recherche de la description */
    while( Mod )
    {
        if( !Mod->m_Name.IsEmpty() && (Mod->m_Name.CmpNoCase( Name ) == 0) )
            break;
        Mod = Mod->Next;
    }

    if( Mod )
    {
        Name  = !Mod->m_Doc.IsEmpty() ? Mod->m_Doc  : wxT( "No Doc" );
        Name += wxT( "\nKeyW: " );
        Name += !Mod->m_KeyWord.IsEmpty() ? Mod->m_KeyWord : wxT( "No Keyword" );
    }
    else
        Name = wxEmptyString;
}


/***************************************************/
static void ReadDocLib( const wxString& ModLibName )
/***************************************************/

/* Routine de lecture du fichier Doc associe a la librairie ModLibName.
 *  Cree en memoire la chaine liste des docs pointee par MList
 *  ModLibName = full file Name de la librairie Modules
 */
{
    ModList*   NewMod;
    char       Line[1024];
    FILE*      LibDoc;
    wxFileName fn = ModLibName;

    fn.SetExt( EXT_DOC );

    if( ( LibDoc = wxFopen( fn.GetFullPath(), wxT( "rt" ) ) ) == NULL )
        return;

    GetLine( LibDoc, Line, NULL, sizeof(Line) - 1 );
    if( strnicmp( Line, ENTETE_LIBDOC, L_ENTETE_LIB ) != 0 )
        return;

    /* Lecture de la librairie */
    while( GetLine( LibDoc, Line, NULL, sizeof(Line) - 1 ) )
    {
        if( Line[0] != '$' )
            continue;
        if( Line[1] == 'E' )
            break;;
        if( Line[1] == 'M' ) /* Debut decription 1 module */
        {
            NewMod = new ModList();
            NewMod->Next = MList;
            MList = NewMod;
            while( GetLine( LibDoc, Line, NULL, sizeof(Line) - 1 ) )
            {
                if( Line[0] ==  '$' ) /* $EndMODULE */
                    break;

                switch( Line[0] )
                {
                case 'L':       /* LibName */
                    NewMod->m_Name = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
                    break;

                case 'K':       /* KeyWords */
                    NewMod->m_KeyWord = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
                    break;

                case 'C':       /* Doc */
                    NewMod->m_Doc = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
                    break;
                }
            }
        } /* lecture 1 descr module */
    }

    /* Fin lecture librairie */
    fclose( LibDoc );
}


/********************************************************************/
MODULE* WinEDA_BasePcbFrame::Select_1_Module_From_BOARD( BOARD* Pcb )
/********************************************************************/

/* Affiche la liste des modules du PCB en cours
 *  Retourne un pointeur si module selectionne
 *  retourne NULL sinon
 */
{
    int             ii;
    MODULE*         Module;
    static wxString OldName;/* Memorise le nom du dernier composant charge */
    wxString        CmpName, msg;

    WinEDAListBox*  ListBox = new WinEDAListBox( this, wxEmptyString,
                                                 NULL, wxEmptyString, NULL,
                                                 wxColour( 200, 200, 255 ) );

    /* Recherche des composants en BOARD */
    ii     = 0;
    Module = Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Next() )
    {
        ii++;
        ListBox->Append( Module->m_Reference->m_Text );
    }

    msg.Printf( _( "Modules [%d items]" ), ii );
    ListBox->SetTitle( msg );

    ListBox->SortList();

    ii = ListBox->ShowModal();
    if( ii >= 0 )
        CmpName = ListBox->GetTextSelection();
    else
        CmpName.Empty();

    ListBox->Destroy();

    if( CmpName == wxEmptyString )
        return NULL;

    OldName = CmpName;

    // Recherche du pointeur sur le module
    Module = Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Next() )
    {
        if( CmpName.CmpNoCase( Module->m_Reference->m_Text ) == 0 )
            break;
    }

    return Module;
}
