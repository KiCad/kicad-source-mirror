/**************************************************************/
/*	librairy editor: edition of component general properties  */
/**************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

enum id_libedit {
    ID_PANEL_ALIAS,
    ID_COPY_DOC_TO_ALIAS,
    ID_BROWSE_DOC_FILES,
    ID_ADD_ALIAS,
    ID_DELETE_ONE_ALIAS,
    ID_DELETE_ALL_ALIAS,
    ID_ON_SELECT_FIELD
};


extern int CurrentUnit;

/* Dialog box to edit a libentry (a component in library) properties */

/* Creates a NoteBook dialog
 *  Edition:
 *  Doc and keys words
 *  Parts per package
 *  General properties
 * Fileds are NOT edited here. There is a specific dialog box to do that
 */

#include "dialog_edit_component_in_lib.cpp"


/*****************************************************************/
void WinEDA_LibeditFrame::InstallLibeditFrame( void )
/*****************************************************************/
{
    WinEDA_PartPropertiesFrame* frame =
        new WinEDA_PartPropertiesFrame( this );

    int IsModified = frame->ShowModal(); frame->Destroy();

    if( IsModified )
        Refresh();
}


/*****************************************************/
void WinEDA_PartPropertiesFrame::InitBuffers()
/*****************************************************/

/* Init the buffers to a default value,
 *  or to values from CurrentLibEntry if CurrentLibEntry != NULL
 */
{
    m_AliasLocation = -1;
    if( CurrentLibEntry == NULL )
    {
        m_Title = _( "Lib Component Properties" );
        return;
    }

    wxString msg_text = _( "Properties for " );
    if( !CurrentAliasName.IsEmpty() )
    {
        m_AliasLocation = LocateAlias( CurrentLibEntry->m_AliasList, CurrentAliasName );
        m_Title = msg_text + CurrentAliasName +
                  _( "(alias of " ) +
                  wxString( CurrentLibEntry->m_Name.m_Text )
                  + wxT( ")" );
    }
    else
    {
        m_Title = msg_text + CurrentLibEntry->m_Name.m_Text;
        CurrentAliasName.Empty();
    }
}


/*****************************************************/
void WinEDA_PartPropertiesFrame::BuildPanelAlias()
/*****************************************************/

/* create the panel for component alias list editing
 */
{
    wxButton*   Button;

    m_PanelAlias->SetFont( *g_DialogFont );
    wxBoxSizer* PanelAliasBoxSizer = new    wxBoxSizer( wxHORIZONTAL );

    m_PanelAlias->SetSizer( PanelAliasBoxSizer );
    wxBoxSizer* LeftBoxSizer = new          wxBoxSizer( wxVERTICAL );

    PanelAliasBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );

    wxStaticText* Msg = new                 wxStaticText( m_PanelAlias, -1, _( "Alias" ) );

    Msg->SetForegroundColour( wxColour( 200, 0, 0 ) );
    LeftBoxSizer->Add( Msg, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    m_PartAliasList = new                   wxListBox( m_PanelAlias,
                                                       -1,
                                                       wxDefaultPosition, wxSize( 200, 250 ),
                                                       0, NULL,
                                                       wxLB_ALWAYS_SB | wxLB_SINGLE );

    LeftBoxSizer->Add( m_PartAliasList, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );

    PanelAliasBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    Button = new                    wxButton( m_PanelAlias, ID_ADD_ALIAS, _( "Add" ) );

    Button->SetForegroundColour( *wxBLUE );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    m_ButtonDeleteOneAlias = new    wxButton( m_PanelAlias, ID_DELETE_ONE_ALIAS,
                                             _( "Delete" ) );

    m_ButtonDeleteOneAlias->SetForegroundColour( *wxRED );
    RightBoxSizer->Add( m_ButtonDeleteOneAlias, 0, wxGROW | wxALL, 5 );

    m_ButtonDeleteAllAlias = new wxButton( m_PanelAlias, ID_DELETE_ALL_ALIAS,
                                          _( "Delete All" ) );

    m_ButtonDeleteAllAlias->SetForegroundColour( *wxRED );
    if( !CurrentAliasName.IsEmpty() )
        m_ButtonDeleteAllAlias->Enable( FALSE );
    RightBoxSizer->Add( m_ButtonDeleteAllAlias, 0, wxGROW | wxALL, 5 );


    /* lecture des noms des alias */
    if( CurrentLibEntry )
    {
        for( unsigned ii = 0; ii < CurrentLibEntry->m_AliasList.GetCount(); ii += ALIAS_NEXT )
            m_PartAliasList->Append( CurrentLibEntry->m_AliasList[ii + ALIAS_NAME] );
    }

    if( (CurrentLibEntry == NULL) || (CurrentLibEntry->m_AliasList.GetCount() == 0) )
    {
        m_ButtonDeleteAllAlias->Enable( FALSE );
        m_ButtonDeleteOneAlias->Enable( FALSE );
    }
}


/*****************************************************************/
void WinEDA_PartPropertiesFrame::BuildPanelFootprintFilter()
/*****************************************************************/

/* create the panel for footprint filtering in cvpcb list
 */
{
    m_PanelFootprintFilter = new wxPanel( m_NoteBook,
                                          -1,
                                          wxDefaultPosition,
                                          wxDefaultSize,
                                          wxSUNKEN_BORDER | wxTAB_TRAVERSAL );

    m_NoteBook->AddPage( m_PanelFootprintFilter, _( "Footprint Filter" ) );

    m_PanelFootprintFilter->SetFont( *g_DialogFont );

    wxBoxSizer* PanelFpFilterBoxSizer = new wxBoxSizer( wxHORIZONTAL );

    m_PanelFootprintFilter->SetSizer( PanelFpFilterBoxSizer );
    wxBoxSizer* LeftBoxSizer = new          wxBoxSizer( wxVERTICAL );

    PanelFpFilterBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );

    wxStaticText* Msg = new                 wxStaticText( m_PanelFootprintFilter, -1, _(
                                                             "Footprints" ) );

    Msg->SetForegroundColour( wxColour( 200, 0, 0 ) );
    LeftBoxSizer->Add( Msg, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    m_FootprintFilterListBox = new          wxListBox( m_PanelFootprintFilter,
                                                       -1,
                                                       wxDefaultPosition, wxSize( 200, 250 ),
                                                       0, NULL,
                                                       wxLB_ALWAYS_SB | wxLB_SINGLE );

    LeftBoxSizer->Add( m_FootprintFilterListBox, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    wxBoxSizer* RightBoxSizer = new         wxBoxSizer( wxVERTICAL );

    PanelFpFilterBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    wxButton* Button = new                  wxButton( m_PanelFootprintFilter,
                                                     ID_ADD_FOOTPRINT_FILTER, _(
                                                         "Add" ) );

    Button->SetForegroundColour( *wxBLUE );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    m_ButtonDeleteOneFootprintFilter = new  wxButton( m_PanelFootprintFilter,
                                                     ID_DELETE_ONE_FOOTPRINT_FILTER,
                                                     _(
                                                         "Delete" ) );

    m_ButtonDeleteOneFootprintFilter->SetForegroundColour( *wxRED );
    RightBoxSizer->Add( m_ButtonDeleteOneFootprintFilter, 0, wxGROW | wxALL, 5 );

    m_ButtonDeleteAllFootprintFilter = new wxButton( m_PanelFootprintFilter,
                                                    ID_DELETE_ALL_FOOTPRINT_FILTER,
                                                    _(
                                                        "Delete All" ) );

    m_ButtonDeleteAllFootprintFilter->SetForegroundColour( *wxRED );
    RightBoxSizer->Add( m_ButtonDeleteAllFootprintFilter, 0, wxGROW | wxALL, 5 );


    /* Read the Footprint Filter list */
    if( CurrentLibEntry )
    {
        for( unsigned ii = 0; ii < CurrentLibEntry->m_FootprintList.GetCount(); ii++ )
            m_FootprintFilterListBox->Append( CurrentLibEntry->m_FootprintList[ii] );
    }

    if( (CurrentLibEntry == NULL) || (CurrentLibEntry->m_FootprintList.GetCount() == 0) )
    {
        m_ButtonDeleteAllFootprintFilter->Enable( FALSE );
        m_ButtonDeleteOneFootprintFilter->Enable( FALSE );
    }
}


/*****************************************************/
void WinEDA_PartPropertiesFrame::BuildPanelDoc()
/*****************************************************/

/* create the panel for component doc editing
 */
{
    wxString msg_text;

    if( m_AliasLocation >= 0 )
        msg_text = CurrentLibEntry->m_AliasList[m_AliasLocation + ALIAS_DOC];
    else
    {
        if( CurrentLibEntry )
            msg_text = CurrentLibEntry->m_Doc;
    }
    m_Doc->SetValue( msg_text );

    msg_text.Empty();
    if( m_AliasLocation >= 0 )
        msg_text = CurrentLibEntry->m_AliasList[m_AliasLocation + ALIAS_KEYWORD];
    else
    {
        if( CurrentLibEntry )
            msg_text = CurrentLibEntry->m_KeyWord;
    }
    m_Keywords->SetValue( msg_text );

    msg_text.Empty();
    if( m_AliasLocation >= 0 )
        msg_text = CurrentLibEntry->m_AliasList[m_AliasLocation + ALIAS_DOC_FILENAME];
    else
    {
        if( CurrentLibEntry )
            msg_text = CurrentLibEntry->m_DocFile;
    }
    m_Docfile->SetValue( msg_text );

    if( m_AliasLocation < 0 )
        m_ButtonCopyDoc->Enable( FALSE );
}


/*****************************************************/
void WinEDA_PartPropertiesFrame::BuildPanelBasic()
/*****************************************************/

/* create the basic panel for component properties editing
 */
{
    m_PanelBasic->SetFont( *g_DialogFont );

    AsConvertButt = new wxCheckBox( m_PanelBasic, -1, _( "As Convert" ) );

    if( g_AsDeMorgan )
        AsConvertButt->SetValue( TRUE );
    m_OptionsBoxSizer->Add( AsConvertButt, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    ShowPinNumButt = new  wxCheckBox( m_PanelBasic, -1, _( "Show Pin Num" ) );

    if( CurrentLibEntry )
    {
        if( CurrentLibEntry->m_DrawPinNum )
            ShowPinNumButt->SetValue( TRUE );
    }
    else
        ShowPinNumButt->SetValue( TRUE );
    m_OptionsBoxSizer->Add( ShowPinNumButt, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    ShowPinNameButt = new wxCheckBox( m_PanelBasic, -1, _( "Show Pin Name" ) );

    if( CurrentLibEntry )
    {
        if( CurrentLibEntry->m_DrawPinName )
            ShowPinNameButt->SetValue( TRUE );
    }
    else
        ShowPinNameButt->SetValue( TRUE );
    m_OptionsBoxSizer->Add( ShowPinNameButt, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    m_PinsNameInsideButt = new wxCheckBox( m_PanelBasic, -1, _( "Pin Name Inside" ) );

    if( CurrentLibEntry )
    {
        if( CurrentLibEntry->m_TextInside )
            m_PinsNameInsideButt->SetValue( TRUE );
    }
    else
        m_PinsNameInsideButt->SetValue( TRUE );
    m_OptionsBoxSizer->Add( m_PinsNameInsideButt, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    int number, number_of_units;
    if( CurrentLibEntry )
        number_of_units = CurrentLibEntry->m_UnitCount;
    else
        number_of_units = 1;
    SelNumberOfUnits->SetValue( number_of_units );

    if( CurrentLibEntry && CurrentLibEntry->m_TextInside )
        number = CurrentLibEntry->m_TextInside;
    else
        number = 40;
    m_SetSkew->SetValue( number );

    if( CurrentLibEntry )
    {
        if( CurrentLibEntry->m_Options == ENTRY_POWER )
            m_OptionPower->SetValue( TRUE );
    }

    if( CurrentLibEntry )
    {
        if( CurrentLibEntry->m_UnitSelectionLocked )
            m_OptionPartsLocked->SetValue( TRUE );
    }
}


/**************************************************************************/
void WinEDA_PartPropertiesFrame::PartPropertiesAccept( wxCommandEvent& event )
/**************************************************************************/

/* Updaye the current component parameters
 */
{
    int ii, jj;

    if( CurrentLibEntry == NULL )
    {
        Close(); return;
    }

    m_Parent->GetScreen()->SetModify();
    m_Parent->SaveCopyInUndoList( CurrentLibEntry );

    /* Update the doc, keyword and doc filename strings */
    if( m_AliasLocation < 0 )
    {
        CurrentLibEntry->m_Doc     = m_Doc->GetValue();
        CurrentLibEntry->m_KeyWord = m_Keywords->GetValue();
        CurrentLibEntry->m_DocFile = m_Docfile->GetValue();
    }
    else
    {
        CurrentLibEntry->m_AliasList[m_AliasLocation + ALIAS_DOC]     = m_Doc->GetValue();
        CurrentLibEntry->m_AliasList[m_AliasLocation + ALIAS_KEYWORD] = m_Keywords->GetValue();
        CurrentLibEntry->m_AliasList[m_AliasLocation + ALIAS_DOC_FILENAME] = m_Docfile->GetValue();
    }

    /* Update the alias list */
    /* 1 - Add names: test for a not existing name in old alias list: */
    jj = m_PartAliasList->GetCount();
    for( ii = 0; ii < jj; ii++ )
    {
        if( LocateAlias( CurrentLibEntry->m_AliasList, m_PartAliasList->GetString( ii ) ) < 0 )
        {
            // new alias must be created
            CurrentLibEntry->m_AliasList.Add( m_PartAliasList->GetString( ii ) );
            CurrentLibEntry->m_AliasList.Add( wxEmptyString );      // Add a void doc string
            CurrentLibEntry->m_AliasList.Add( wxEmptyString );      // Add a void keyword list string
            CurrentLibEntry->m_AliasList.Add( wxEmptyString );      // Add a void doc filename string
        }
    }

    /* 2 - Remove delete names: test for an non existing name in new alias list: */
    int kk, kkmax = CurrentLibEntry->m_AliasList.GetCount();
    for( kk = 0; kk < kkmax; )
    {
        jj = m_PartAliasList->GetCount();
        wxString aliasname = CurrentLibEntry->m_AliasList[kk];
        for( ii = 0; ii < jj; ii++ )
        {
            if( aliasname.CmpNoCase( m_PartAliasList->GetString( ii ).GetData() ) == 0 )
            {
                kk += ALIAS_NEXT; // Alias exist in new list. keep it and test next old name
                break;
            }
        }

        if( ii == jj ) // Alias not found in new list, remove it (4 strings in kk position)
        {
            for( ii = 0; ii < ALIAS_NEXT; ii++ )
                CurrentLibEntry->m_AliasList.RemoveAt( kk );

            kkmax = CurrentLibEntry->m_AliasList.GetCount();
        }
    }

    ii = SelNumberOfUnits->GetValue();
    if( ChangeNbUnitsPerPackage( ii ) )
        m_RecreateToolbar = TRUE;

    if( AsConvertButt->GetValue() )
    {
        if( !g_AsDeMorgan )
        {
            g_AsDeMorgan = 1;
            if( SetUnsetConvert() )
                m_RecreateToolbar = TRUE;
        }
    }
    else
    {
        if( g_AsDeMorgan )
        {
            g_AsDeMorgan = 0;
            if( SetUnsetConvert() )
                m_RecreateToolbar = TRUE;
        }
    }

    CurrentLibEntry->m_DrawPinNum  = ShowPinNumButt->GetValue() ? 1 : 0;
    CurrentLibEntry->m_DrawPinName = ShowPinNameButt->GetValue() ? 1 : 0;

    if( m_PinsNameInsideButt->GetValue() == FALSE )
        CurrentLibEntry->m_TextInside = 0;
    else
        CurrentLibEntry->m_TextInside = m_SetSkew->GetValue();

    if( m_OptionPower->GetValue() == TRUE )
        CurrentLibEntry->m_Options = ENTRY_POWER;
    else
        CurrentLibEntry->m_Options = ENTRY_NORMAL;

    /* Set the option "Units locked".
     *  Obviously, cannot be TRUE if there is only one part */
    CurrentLibEntry->m_UnitSelectionLocked = m_OptionPartsLocked->GetValue();
    if( CurrentLibEntry->m_UnitCount <= 1 )
        CurrentLibEntry->m_UnitSelectionLocked = FALSE;

    if( m_RecreateToolbar )
        m_Parent->ReCreateHToolbar();

    m_Parent->DisplayLibInfos();

    /* Update the footprint filter list */
    CurrentLibEntry->m_FootprintList.Clear();
    jj = m_FootprintFilterListBox->GetCount();
    for( ii = 0; ii < jj; ii++ )
        CurrentLibEntry->m_FootprintList.Add( m_FootprintFilterListBox->GetString( ii ) );

    EndModal( 1 );
}


/*******************************************************************************/
void WinEDA_PartPropertiesFrame::CopyDocToAlias( wxCommandEvent& WXUNUSED (event) )
/******************************************************************************/
{
    if( CurrentLibEntry == NULL )
        return;
    if( CurrentAliasName.IsEmpty() )
        return;

    m_Doc->SetValue( CurrentLibEntry->m_Doc );
    m_Docfile->SetValue( CurrentLibEntry->m_DocFile );
    m_Keywords->SetValue( CurrentLibEntry->m_KeyWord );
}


/**********************************************************/
void WinEDA_PartPropertiesFrame::DeleteAllAliasOfPart(
    wxCommandEvent& WXUNUSED (event) )
/**********************************************************/
{
    CurrentAliasName.Empty();
    if( CurrentLibEntry )
    {
        if( IsOK( this, _( "Ok to Delete Alias LIST" ) ) )
        {
            m_PartAliasList->Clear();
            m_RecreateToolbar = TRUE;
            m_ButtonDeleteAllAlias->Enable( FALSE );
            m_ButtonDeleteOneAlias->Enable( FALSE );
        }
    }
}


/*******************************************************************************/
void WinEDA_PartPropertiesFrame::AddAliasOfPart( wxCommandEvent& WXUNUSED (event) )
/*******************************************************************************/

/* Add a new name to the alias list box
 *  New name cannot be the root name, and must not exists
 */
{
    wxString Line;
    wxString aliasname;

    if( CurrentLibEntry == NULL )
        return;

    if( Get_Message( _( "New alias:" ), _( "Component Alias" ), Line, this ) != 0 )
        return;

    Line.Replace( wxT( " " ), wxT( "_" ) );
    aliasname = Line;

    if( CurrentLibEntry->m_Name.m_Text.CmpNoCase( Line ) == 0 )
    {
        DisplayError( this, _( "This is the Root Part" ), 10 ); return;
    }

    /* test for an existing name: */
    int ii, jj = m_PartAliasList->GetCount();
    for( ii = 0; ii < jj; ii++ )
    {
        if( aliasname.CmpNoCase( m_PartAliasList->GetString( ii ) ) == 0 )
        {
            DisplayError( this, _( "Already in use" ), 10 );
            return;
        }
    }

    m_PartAliasList->Append( aliasname );
    if( CurrentAliasName.IsEmpty() )
        m_ButtonDeleteAllAlias->Enable( TRUE );
    m_ButtonDeleteOneAlias->Enable( TRUE );

    m_RecreateToolbar = TRUE;
}


/********************************************************/
void WinEDA_PartPropertiesFrame::DeleteAliasOfPart(
    wxCommandEvent& WXUNUSED (event) )
/********************************************************/
{
    wxString aliasname = m_PartAliasList->GetStringSelection();

    if( aliasname.IsEmpty() )
        return;
    if( aliasname == CurrentAliasName )
    {
        wxString msg = CurrentAliasName + _( " is Current Selected Alias!" );
        DisplayError( this, msg );
        return;
    }

    int ii = m_PartAliasList->GetSelection();
    m_PartAliasList->Delete( ii );

    if( !CurrentLibEntry || (CurrentLibEntry->m_AliasList.GetCount() == 0) )
    {
        m_ButtonDeleteAllAlias->Enable( FALSE );
        m_ButtonDeleteOneAlias->Enable( FALSE );
    }
    m_RecreateToolbar = TRUE;
}


/********************************************************************/
bool WinEDA_PartPropertiesFrame::ChangeNbUnitsPerPackage( int MaxUnit )
/********************************************************************/

/* Routine de modification du nombre d'unites par package pour le
 *  composant courant;
 */
{
    int OldNumUnits, ii, FlagDel = -1;
    LibEDA_BaseStruct* DrawItem, * NextDrawItem;

    if( CurrentLibEntry == NULL )
        return FALSE;

    /* Si pas de changement: termine */
    if( CurrentLibEntry->m_UnitCount == MaxUnit )
        return FALSE;

    OldNumUnits = CurrentLibEntry->m_UnitCount;
    if( OldNumUnits < 1 )
        OldNumUnits = 1;

    CurrentLibEntry->m_UnitCount = MaxUnit;


    /* Traitement des unites enlevees ou rajoutees */
    if( OldNumUnits > CurrentLibEntry->m_UnitCount )
    {
        DrawItem = CurrentLibEntry->m_Drawings;
        for( ; DrawItem != NULL; DrawItem = NextDrawItem )
        {
            NextDrawItem = DrawItem->Next();
            if( DrawItem->m_Unit > MaxUnit )  /* Item a effacer */
            {
                if( FlagDel < 0 )
                {
                    if( IsOK( this, _( "Delete units" ) ) )
                    {
                        /* Si part selectee n'existe plus: selection 1ere unit */
                        if( CurrentUnit > MaxUnit )
                            CurrentUnit = 1;
                        FlagDel = 1;
                    }
                    else
                    {
                        FlagDel = 0;
                        MaxUnit = OldNumUnits;
                        CurrentLibEntry->m_UnitCount = MaxUnit;
                        return FALSE;
                    }
                }
                DeleteOneLibraryDrawStruct( m_Parent->DrawPanel, NULL, CurrentLibEntry,
                                            DrawItem, 0 );
            }
        }

        return TRUE;
    }

    if( OldNumUnits < CurrentLibEntry->m_UnitCount )
    {
        DrawItem = CurrentLibEntry->m_Drawings;
        for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
        {
            /* Duplication des items pour autres elements */
            if( DrawItem->m_Unit == 1 )
            {
                for( ii = OldNumUnits + 1; ii <= MaxUnit; ii++ )
                {
                    NextDrawItem = CopyDrawEntryStruct( this, DrawItem );
                    NextDrawItem->SetNext( CurrentLibEntry->m_Drawings );
                    CurrentLibEntry->m_Drawings = NextDrawItem;
                    NextDrawItem->m_Unit = ii;
                }
            }
        }
    }
    return TRUE;
}


/*****************************************************/
bool WinEDA_PartPropertiesFrame::SetUnsetConvert()
/*****************************************************/

/* cr�e ou efface (selon option AsConvert) les �l�ments
 *  de la repr�sentation convertie d'un composant
 */
{
    int FlagDel = 0;
    LibEDA_BaseStruct* DrawItem = NULL, * NextDrawItem;

    if( g_AsDeMorgan )  /* Representation convertie a creer */
    {
        /* Traitement des elements a ajouter ( pins seulement ) */
        if( CurrentLibEntry )
            DrawItem = CurrentLibEntry->m_Drawings;
        for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
        {
            /* Duplication des items pour autres elements */
            if( DrawItem->Type() != COMPONENT_PIN_DRAW_TYPE )
                continue;
            if( DrawItem->m_Convert == 1 )
            {
                if( FlagDel == 0 )
                {
                    if( IsOK( this, _( "Create pins for Convert items" ) ) )
                        FlagDel = 1;
                    else
                    {
                        if( IsOK( this, _( "Part as \"De Morgan\" anymore" ) ) )
                            return TRUE;

                        g_AsDeMorgan = 0; return FALSE;
                    }
                }
                NextDrawItem = CopyDrawEntryStruct( this, DrawItem );
                NextDrawItem->SetNext( CurrentLibEntry->m_Drawings );
                CurrentLibEntry->m_Drawings = NextDrawItem;
                NextDrawItem->m_Convert     = 2;
            }
        }
    }
    else               /* Representation convertie a supprimer */
    {
        /* Traitement des elements � supprimer */
        if( CurrentLibEntry )
            DrawItem = CurrentLibEntry->m_Drawings;
        for( ; DrawItem != NULL; DrawItem = NextDrawItem )
        {
            NextDrawItem = DrawItem->Next();
            if( DrawItem->m_Convert > 1 )  /* Item a effacer */
            {
                if( FlagDel == 0 )
                {
                    if( IsOK( this, _( "Delete Convert items" ) ) )
                    {
                        CurrentConvert = 1;
                        FlagDel = 1;
                    }
                    else
                    {
                        g_AsDeMorgan = 1;
                        return FALSE;
                    }
                }
                m_Parent->GetScreen()->SetModify();
                DeleteOneLibraryDrawStruct( m_Parent->DrawPanel,
                                            NULL,
                                            CurrentLibEntry,
                                            DrawItem,
                                            0 );
            }
        }
    }
    return TRUE;
}


/****************************************************************************/
void WinEDA_PartPropertiesFrame::BrowseAndSelectDocFile( wxCommandEvent& event )
/****************************************************************************/
{
    wxString FullFileName, mask;

    wxString docpath( g_RealLibDirBuffer ), filename;

    docpath += wxT( "doc" );
    docpath += STRING_DIR_SEP;
    mask     = wxT( "*" );
    FullFileName = EDA_FileSelector( _( "Doc Files" ),
                                     docpath,       /* Chemin par defaut */
                                     wxEmptyString, /* nom fichier par defaut */
                                     wxEmptyString, /* extension par defaut */
                                     mask,          /* Masque d'affichage */
                                     this,
                                     wxFD_OPEN,
                                     TRUE
                                     );
    if( FullFileName.IsEmpty() )
        return;

    // Suppression du chemin par defaut pour le fichier de doc:

    /* If the library path is already in the library search paths
     * list, just add the library name to the list.  Otherwise, add
     * the library name with the full or relative path.
     * the relative path, when possible is preferable,
     * because it preserve use of default libraries paths, when the path is a sub path of these default paths
     *
     */
    wxFileName fn = FullFileName;
    int        pathlen = -1;                                                    // path len, used to find the better subpath within defualts paths
    if( wxGetApp().GetLibraryPathList().Index( fn.GetPath() ) != wxNOT_FOUND )  // Ok, trivial case
        filename = fn.GetName();
    else                                                                        // not in the default, : see if this file is in a subpath:
    {
        filename = fn.GetPathWithSep() + fn.GetName();
        for( unsigned kk = 0; kk < wxGetApp().GetLibraryPathList().GetCount(); kk++ )
        {
            if( fn.MakeRelativeTo( wxGetApp().GetLibraryPathList()[kk] ) )
            {
                if( pathlen < 0                             // a subpath is found
                   || pathlen > (int) fn.GetPath().Len() )  // a better subpath if found
                {
                    filename = fn.GetPathWithSep() + fn.GetName();
                    pathlen  = fn.GetPath().Len();
                }
                fn = FullFileName;  //Try to find a better subpath
            }
        }
    }
    m_Docfile->SetValue( filename );
}


/**********************************************************/
void WinEDA_PartPropertiesFrame::DeleteAllFootprintFilter(
    wxCommandEvent& WXUNUSED (event) )
/**********************************************************/
{
    if( IsOK( this, _( "Ok to Delete FootprintFilter LIST" ) ) )
    {
        m_FootprintFilterListBox->Clear();
        m_ButtonDeleteAllFootprintFilter->Enable( FALSE );
        m_ButtonDeleteOneFootprintFilter->Enable( FALSE );
    }
}


/*******************************************************************************/
void WinEDA_PartPropertiesFrame::AddFootprintFilter( wxCommandEvent& WXUNUSED (event) )
/*******************************************************************************/

/* Add a new name to the alias list box
 *  New name cannot be the root name, and must not exists
 */
{
    wxString Line;

    if( CurrentLibEntry == NULL )
        return;

    if( Get_Message( _( "New FootprintFilter:" ), _( "Footprint Filter" ), Line, this ) != 0 )
        return;

    Line.Replace( wxT( " " ), wxT( "_" ) );

    /* test for an existing name: */
    int ii, jj = m_FootprintFilterListBox->GetCount();
    for( ii = 0; ii < jj; ii++ )
    {
        if( Line.CmpNoCase( m_FootprintFilterListBox->GetString( ii ) ) == 0 )
        {
            DisplayError( this, _( "Already in use" ), 10 );
            return;
        }
    }

    m_FootprintFilterListBox->Append( Line );
    m_ButtonDeleteAllFootprintFilter->Enable( TRUE );
    m_ButtonDeleteOneFootprintFilter->Enable( TRUE );
}


/********************************************************/
void WinEDA_PartPropertiesFrame::DeleteOneFootprintFilter(
    wxCommandEvent& WXUNUSED (event) )
/********************************************************/
{
    int ii = m_FootprintFilterListBox->GetSelection();

    m_FootprintFilterListBox->Delete( ii );

    if( !CurrentLibEntry || (m_FootprintFilterListBox->GetCount() == 0) )
    {
        m_ButtonDeleteAllFootprintFilter->Enable( FALSE );
        m_ButtonDeleteOneFootprintFilter->Enable( FALSE );
    }
}
