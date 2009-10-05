/******************************/
/* PCBNEW: echange de modules */
/******************************/

/* Fichier xchmod.cpp */

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "dialog_exchange_modules_base.h"

// Local variables:
int s_SelectionMode = 0;    // Remember the last exchange option, when exit dialog.

/************************************/
/* class DIALOG_EXCHANGE_MODULE */
/************************************/

class DIALOG_EXCHANGE_MODULE : public DIALOG_EXCHANGE_MODULE_BASE
{
private:

    WinEDA_PcbFrame* m_Parent;
    MODULE* m_CurrentModule;

public:

    // Constructor and destructor
    DIALOG_EXCHANGE_MODULE( WinEDA_PcbFrame* aParent, MODULE* aModule );
    ~DIALOG_EXCHANGE_MODULE() { };

private:
    void    OnSelectionClicked( wxCommandEvent& event );
    void    OnOkClick( wxCommandEvent& event );
    void    OnQuit( wxCommandEvent& event );
    void    BrowseAndSelectFootprint( wxCommandEvent& event );
    void    Init();

    void    Change_Module();
    void    Change_ModuleId( bool aUseValue );
    void    Change_ModuleAll();
    int     Maj_ListeCmp( const wxString& reference, const wxString& old_name,
                          const wxString& new_name, bool ShowError );
    bool Change_1_Module( MODULE*         Module,
                             const wxString& new_module,
                             PICKED_ITEMS_LIST* aUndoPickList,
                             bool            ShowError );
};


DIALOG_EXCHANGE_MODULE::DIALOG_EXCHANGE_MODULE( WinEDA_PcbFrame* parent, MODULE* Module ) :
    DIALOG_EXCHANGE_MODULE_BASE( parent )
{
    m_Parent = parent;
    m_CurrentModule = Module;
    Init();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void WinEDA_PcbFrame::InstallExchangeModuleFrame( MODULE* Module )
{
    DIALOG_EXCHANGE_MODULE dialog( this, Module );

    dialog.ShowModal();
}


void DIALOG_EXCHANGE_MODULE::OnQuit( wxCommandEvent& WXUNUSED(event) )
{
    s_SelectionMode = m_Selection->GetSelection();
    Close( true );    // true is to force the frame to close
}


void DIALOG_EXCHANGE_MODULE::Init()
{
    SetFocus();

    m_OldModule->AppendText( m_CurrentModule->m_LibRef );
    m_NewModule->AppendText( m_CurrentModule->m_LibRef );
    m_OldValue->AppendText( m_CurrentModule->m_Value->m_Text );
    m_Selection->SetSelection( s_SelectionMode );

    // Enable/disable widgets:
    wxCommandEvent event;
    OnSelectionClicked( event );
}


void DIALOG_EXCHANGE_MODULE::OnOkClick( wxCommandEvent& event )
{
    s_SelectionMode = m_Selection->GetSelection();
    switch( m_Selection->GetSelection() )
    {
    case 0:
        Change_Module();
        break;

    case 1:
        Change_ModuleId( false );
        break;

    case 2:
        Change_ModuleId( true );
        break;

    case 3:
        Change_ModuleAll();
        break;
    }
}


void DIALOG_EXCHANGE_MODULE::OnSelectionClicked( wxCommandEvent& event )
{
    switch( m_Selection->GetSelection() )
    {
    case 0:
    case 1:
    case 2:
        m_NewModule->Enable( true );
        m_Browsebutton->Enable( true );
        break;

    case 3:
        m_NewModule->Enable( false );
        m_Browsebutton->Enable( false );
        break;
    }
}


/************************************************************************/

/*
 *  Met a jour le fichier name.CMP (s'il existe) apres un echange de module
 *  (par la commande changeMod), si les modules sont geres par ce fichier
 *
 *  Si ShowError != 0 affiche message d'erreur si le fichier .cmp n'est pas
 *  trouve.
 *  Retoure 1 si erreur
 */
int DIALOG_EXCHANGE_MODULE::Maj_ListeCmp( const wxString& reference,
                                          const wxString& old_name,
                                          const wxString& new_name,
                                          bool            ShowError )
{
    wxFileName fn;
    wxFileName tmpFileName;
    FILE*      FichCmp, * NewFile;
    char       Line[1024];
    wxString   msg;

    if( old_name == new_name )
        return 0;                           /* pas de changement de nom */

    /* Calcul nom fichier CMP par changement de l'extension du nom netliste */
    fn = m_Parent->GetScreen()->m_FileName;
    fn.SetExt( NetCmpExtBuffer );

    // Modification du fichier .cmp correcpondant
    FichCmp = wxFopen( fn.GetFullPath(), wxT( "rt" ) );
    if( FichCmp == NULL )
    {
        if( ShowError )
        {
            msg.Printf( _( "file %s not found" ), fn.GetFullPath().c_str() );
            m_WinMessages->AppendText( msg );
        }
        return 1;
    }

    /* Analyse du fichier et modif */
    tmpFileName = fn;
    tmpFileName.SetExt( wxT( "$$$" ) );
    NewFile = wxFopen( tmpFileName.GetFullPath(), wxT( "wt" ) );
    if( NewFile == NULL )
    {
        if( ShowError )
        {
            msg.Printf( _( "Unable to create file %s" ),
                       tmpFileName.GetFullPath().c_str() );
            m_WinMessages->AppendText( msg );
        }
        return 1;
    }

    (void) fgets( Line, sizeof(Line), FichCmp );
    fprintf( NewFile, "Cmp-Mod V01 Genere par PcbNew le %s\n",
            DateAndTime( Line ) );

    bool start_descr = false;
    while( fgets( Line, sizeof(Line), FichCmp ) != NULL )
    {
        if( strnicmp( Line, "Reference = ", 9 ) == 0 )
        {
            char buf[1024];
            strcpy( buf, Line + 12 );
            strtok( buf, ";\n\r" );
            if( stricmp( buf, CONV_TO_UTF8( reference ) ) == 0 )
            {
                start_descr = true;
            }
        }

        if( (strnicmp( Line, "Begin", 5 ) == 0)
           || (strnicmp( Line, "End", 3 ) == 0) )
        {
            start_descr = false;
        }

        if( start_descr && strnicmp( Line, "IdModule", 8 ) == 0 )
        {
            sprintf( Line + 8, "  = %s;\n", CONV_TO_UTF8( new_name ) );

            msg = wxT( " * in <" ) + fn.GetFullPath() + wxT( ">.\n" );
            m_WinMessages->AppendText( msg );

            start_descr = false;
        }
        fputs( Line, NewFile );
    }

    fclose( FichCmp );
    fclose( NewFile );
    wxRemoveFile( fn.GetFullPath() );
    wxRenameFile( tmpFileName.GetFullPath(), fn.GetFullPath() );
    return 0;
}


/*
 * Routine de changement d'un module:
 *  Change le module pointe par la souris, par un autre en conservant
 *      - meme orientation
 *      - meme position
 *      - memes textes valeur et ref
 *      - memes netnames pour pads de meme nom
 */
void DIALOG_EXCHANGE_MODULE::Change_Module()
{
    wxString newmodulename = m_NewModule->GetValue();

    if( newmodulename == wxEmptyString )
        return;

    PICKED_ITEMS_LIST pickList;

    if( Change_1_Module( m_CurrentModule, newmodulename, &pickList, true ) )
    {
        m_Parent->Compile_Ratsnest( NULL, true );
        m_Parent->DrawPanel->Refresh();
    }

    if( pickList.GetCount() )
        m_Parent->SaveCopyInUndoList( pickList, UR_UNSPECIFIED );
}


/*
 * Routine de changement de tous les modules de meme nom lib que celui
 *  selectionne, en conservant
 *      - meme orientation
 *      - meme position
 *      - memes textes valeur et ref
 *      - memes netnames pour pads de meme nom
 *  et en remplacant l'ancien module par le noveau module
 *  Attention: m_CurrentModule ne pointe plus sur le module de reference
 *  puisque celui ci a ete change!!
 */
void DIALOG_EXCHANGE_MODULE::Change_ModuleId( bool aUseValue )
{
    wxString msg;
    MODULE*  Module, * PtBack;
    bool     change = false;
    wxString newmodulename = m_NewModule->GetValue();
    wxString value, lib_reference;
    bool     check_module_value = false;
    int      ShowErr = 3;           // Affiche 3 messages d'err maxi

    if( m_Parent->GetBoard()->m_Modules == NULL )
        return;
    if( newmodulename == wxEmptyString )
        return;

    lib_reference = m_CurrentModule->m_LibRef;
    if( aUseValue )
    {
        check_module_value = true;
        value = m_CurrentModule->m_Value->m_Text;
        msg.Printf( _( "Change modules <%s> -> <%s> (val = %s)?" ),
                   m_CurrentModule->m_LibRef.GetData(),
                   newmodulename.GetData(),
                   m_CurrentModule->m_Value->m_Text.GetData() );
    }
    else
    {
        msg.Printf( _( "Change modules <%s> -> <%s> ?" ),
                   lib_reference.GetData(), newmodulename.GetData() );
    }

    if( !IsOK( this, msg ) )
        return;

    /* Le changement s'effectue a partir du dernier module car la routine
     *  Change_1_Module() modifie le dernier module de la liste
     */

    PICKED_ITEMS_LIST pickList;
   /* note: for the first module in chain (the last here), Module->Back() points the board or is NULL
     */
    Module = m_Parent->GetBoard()->m_Modules.GetLast();
    for( ; Module && ( Module->Type() == TYPE_MODULE ); Module = PtBack )
    {
        PtBack = Module->Back();
        if( lib_reference.CmpNoCase( Module->m_LibRef ) != 0 )
            continue;
        if( check_module_value )
        {
            if( value.CmpNoCase( Module->m_Value->m_Text ) != 0 )
                continue;
        }
        if( Change_1_Module( Module, newmodulename.GetData(), &pickList, ShowErr ) )
            change = true;
        else if( ShowErr )
            ShowErr--;
    }

    if( change )
    {
        m_Parent->Compile_Ratsnest( NULL, true );
        m_Parent->DrawPanel->Refresh();
    }

    if( pickList.GetCount() )
        m_Parent->SaveCopyInUndoList( pickList, UR_UNSPECIFIED );
}


/*
 * Routine de changement de tous les modules par les modules de meme nom lib:
 *  en conservant
 *      - meme orientation
 *      - meme position
 *      - memes textes valeur et ref
 *      - memes netnames pour pads de meme nom
 */
void DIALOG_EXCHANGE_MODULE::Change_ModuleAll()
{
    MODULE* Module, * PtBack;
    bool    change  = false;
    int     ShowErr = 3; // Affiche 3 messages d'err maxi

    if( m_Parent->GetBoard()->m_Modules == NULL )
        return;

    if( !IsOK( this, _( "Change ALL modules ?" ) ) )
        return;

    /* Le changement s'effectue a partir du dernier module car la routine
     *  Change_1_Module() modifie le dernier module de la liste
     */

     PICKED_ITEMS_LIST pickList;

   /* note: for the first module in chain (the last here), Module->Back() points the board or is NULL
     */
    Module = m_Parent->GetBoard()->m_Modules.GetLast();
    for( ; Module && ( Module->Type() == TYPE_MODULE ); Module = PtBack )
    {
        PtBack = Module->Back();
        if( Change_1_Module( Module, Module->m_LibRef.GetData(), &pickList, ShowErr ) )
            change = true;
        else if( ShowErr )
            ShowErr--;
    }

    if( change )
    {
        m_Parent->Compile_Ratsnest( NULL, true );
        m_Parent->DrawPanel->Refresh();
    }
    if( pickList.GetCount() )
        m_Parent->SaveCopyInUndoList( pickList, UR_UNSPECIFIED );
}


/*
 * Routine de changement d'un module:
 *  Change le module de numero empr, avec le module de nom new_module
 *      - meme orientation
 *      - meme position
 *      - memes textes valeur et ref
 *      - memes netnames pour pads de meme nom
 *  Retourne :
 *      false si pas de changement ( si le nouveau module n'est pas en libr)
 *      true si OK
 * Ratsnest *must be recalculated* after modules changes
 */
bool DIALOG_EXCHANGE_MODULE::Change_1_Module( MODULE*         Module,
                                                 const wxString& new_module,
                                                 PICKED_ITEMS_LIST* aUndoPickList,
                                                 bool            ShowError )
{
    wxString namecmp, oldnamecmp;
    MODULE*  NewModule;
    wxString Line;

    if( Module == NULL )
        return false;

    wxBusyCursor dummy;

    /* Memorisation des parametres utiles de l'ancien module */
    oldnamecmp = Module->m_LibRef;
    namecmp    = new_module;

    /* Chargement du module */
    Line.Printf( _( "Change module %s (%s)  " ),
                Module->m_Reference->m_Text.GetData(), oldnamecmp.GetData() );
    m_WinMessages->AppendText( Line );

    namecmp.Trim( true );
    namecmp.Trim( false );
    NewModule = m_Parent->Get_Librairie_Module( wxEmptyString,
                                                namecmp,
                                                ShowError );
    if( NewModule == NULL )  /* Nouveau module NON trouve, reaffichage de l'ancien */
    {
        m_WinMessages->AppendText( wxT( "No\n" ) );
        return false;
    }

    if( Module == m_CurrentModule )
        m_CurrentModule = NewModule;
    m_WinMessages->AppendText( wxT( "Ok\n" ) );

    m_Parent->Exchange_Module( Module, NewModule, aUndoPickList );

    Maj_ListeCmp( NewModule->m_Reference->m_Text,
                  oldnamecmp,
                  namecmp,
                  ShowError );

    return true;
}


/** function Exchange_Module
 * Replaces OldModule by NewModule, using OldModule settings:
 * position, orientation, pad netnames ...)
 * OldModule is deleted or put in undo list.
 * @param aOldModule = footprint to replace
 * @param aNewModule = footprint to put
 * @param aUndoPickList = the undo list used to save  OldModule. If null, OldModule is deleted
 */
void WinEDA_PcbFrame::Exchange_Module( MODULE*   aOldModule,
                                              MODULE*   aNewModule,
                                            PICKED_ITEMS_LIST* aUndoPickList)
{
    wxPoint oldpos; /* memorisation temporaire pos curseur */
    D_PAD*  pad, * old_pad;

    if( (aOldModule->Type() != TYPE_MODULE) || (aNewModule->Type() != TYPE_MODULE) )
    {
        wxMessageBox( wxT( "WinEDA_PcbFrame::Exchange_Module() StuctType error" ) );
        return;
    }

    aNewModule->SetParent( GetBoard() );

    GetBoard()->m_Status_Pcb = 0;
    oldpos = GetScreen()->m_Curseur;
    GetScreen()->m_Curseur = aOldModule->m_Pos;

    /* place module without ratsnets refresh: this will be made later
     * when all modules are on board
     */
    Place_Module( aNewModule, NULL, true );
    GetScreen()->m_Curseur = oldpos;

    /* Flip footprint if needed */
    if( aOldModule->GetLayer() != aNewModule->GetLayer() )
    {
        aNewModule->Flip( aNewModule->m_Pos );
    }

    /* Rotate footprint if needed */
    if( aOldModule->m_Orient != aNewModule->m_Orient )
    {
        Rotate_Module( NULL, aNewModule, aOldModule->m_Orient, false );
    }

    /* Update reference and value */
    aNewModule->m_Reference->m_Text = aOldModule->m_Reference->m_Text;
    aNewModule->m_Value->m_Text     = aOldModule->m_Value->m_Text;

    /* Mise a jour des autres parametres */
    aNewModule->m_TimeStamp = aOldModule->m_TimeStamp;
    aNewModule->m_Path = aOldModule->m_Path;

    /* Update pad netnames ( when possible) */
    pad = aNewModule->m_Pads;
    for( ; pad != NULL; pad = pad->Next() )
    {
        pad->SetNetname( wxEmptyString );
        pad->SetNet( 0 );
        old_pad = aOldModule->m_Pads;
        for( ; old_pad != NULL; old_pad = old_pad->Next() )
        {
            if( strnicmp( pad->m_Padname, old_pad->m_Padname,
                         sizeof(pad->m_Padname) ) == 0 )
            {
                pad->SetNetname( old_pad->GetNetname() );
                pad->SetNet( old_pad->GetNet() );
            }
        }
    }

    if( aUndoPickList )
    {
        GetBoard()->Remove(aOldModule);
        ITEM_PICKER picker_old(aOldModule, UR_DELETED);
        ITEM_PICKER picker_new(aNewModule, UR_NEW);
        aUndoPickList->PushItem(picker_old);
        aUndoPickList->PushItem(picker_new);
    }
    else /* Effacement de l'ancien module */
        aOldModule->DeleteStructure();

    GetBoard()->m_Status_Pcb = 0;
    aNewModule->m_Flags = 0;
    GetScreen()->SetModify();
}


/*
 * affiche la liste des modules en librairie et selectione 1 nom
 */
void DIALOG_EXCHANGE_MODULE::BrowseAndSelectFootprint( wxCommandEvent& event )
{
    wxString newname;

    newname = m_Parent->Select_1_Module_From_List( m_Parent,
                                                   wxEmptyString,
                                                   wxEmptyString,
                                                   wxEmptyString );
    if( newname != wxEmptyString )
        m_NewModule->SetValue( newname );
}


/**
 * Function RecreateBOMFileFromBoard
 * Recreates a .cmp file from the current loaded board
 * this is the same as created by cvpcb.
 * can be used if this file is lost
 */
void WinEDA_PcbFrame::RecreateCmpFileFromBoard( wxCommandEvent& aEvent )
{
    wxFileName fn;
    FILE*      FichCmp;
    char       Line[1024];
    MODULE*    Module = GetBoard()->m_Modules;
    wxString   msg;
    wxString   wildcard;

    if( Module == NULL )
    {
        DisplayError( this, _( "No Modules!" ) );
        return;
    }

    /* Calcul nom fichier CMP par changement de l'extension du nom netliste */
    fn = GetScreen()->m_FileName;
    fn.SetExt( NetCmpExtBuffer );
    wildcard = _( "Component files (." ) + NetCmpExtBuffer + wxT( ")|*." ) +
               NetCmpExtBuffer;

    wxFileDialog dlg( this, _( "Save Component Files" ), wxEmptyString,
                      fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    FichCmp = wxFopen( fn.GetFullPath(), wxT( "wt" ) );
    if( FichCmp == NULL )
    {
        msg = _( "Unable to create file " ) + fn.GetFullPath();
        DisplayError( this, msg );
        return;
    }

    (void) fgets( Line, sizeof(Line), FichCmp );
    fprintf( FichCmp, "Cmp-Mod V01 Genere par PcbNew le %s\n",
            DateAndTime( Line ) );

    for( ; Module != NULL; Module = Module->Next() )
    {
        fprintf( FichCmp, "\nBeginCmp\n" );
        fprintf( FichCmp, "TimeStamp = %8.8lX\n", Module->m_TimeStamp );
        fprintf( FichCmp, "Path = %s\n", CONV_TO_UTF8( Module->m_Path ) );
        fprintf( FichCmp, "Reference = %s;\n",
                 !Module->m_Reference->m_Text.IsEmpty() ?
                 CONV_TO_UTF8( Module->m_Reference->m_Text ) : "[NoRef]" );
        fprintf( FichCmp, "ValeurCmp = %s;\n",
                 !Module->m_Value->m_Text.IsEmpty() ?
                 CONV_TO_UTF8( Module->m_Value->m_Text ) : "[NoVal]" );
        fprintf( FichCmp, "IdModule  = %s;\n",
                CONV_TO_UTF8( Module->m_LibRef ) );
        fprintf( FichCmp, "EndCmp\n" );
    }

    fprintf( FichCmp, "\nEndListe\n" );
    fclose( FichCmp );
}
