/*************************************************/
/* Functions to Load  from file and save to file */
/* the graphic shapes  used to draw a component  */
/* When using the import/export symbol options   */
/* files are the *.sym files                     */
/*************************************************/

/* fichier symbedit.cpp */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"


/*
 * Read a component shape file (a symbol file *.sym )and add data (graphic
 * items) to the current component.
 *
 * A symbol file *.sym has the same format as a library, and contains only
 * one symbol
 */
void WinEDA_LibeditFrame::LoadOneSymbol( void )
{
    LIB_COMPONENT*     Component;
    LibEDA_BaseStruct* DrawEntry;
    FILE*              ImportFile;
    wxString           msg, err;
    CMP_LIBRARY*       Lib;

    /* Exit if no library entry is selected or a command is in progress. */
    if( CurrentLibEntry == NULL
       || ( CurrentDrawItem && CurrentDrawItem->m_Flags ) )
        return;

    DrawPanel->m_IgnoreMouseEvents = TRUE;

    wxString default_path = wxGetApp().ReturnLastVisitedLibraryPath();

    wxFileDialog dlg( this, _( "Import Symbol Drawings" ), default_path,
                      wxEmptyString, SymbolFileWildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    GetScreen()->m_Curseur = wxPoint( 0, 0 );
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    wxFileName fn = dlg.GetPath();
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    /* Load data */
    ImportFile = wxFopen( fn.GetFullPath(), wxT( "rt" ) );

    if( ImportFile == NULL )
    {
        msg.Printf( _( "Failed to open Symbol File <%s>" ),
                    (const wxChar*) fn.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    Lib = new CMP_LIBRARY( LIBRARY_TYPE_SYMBOL, fn );

    if( !Lib->Load( err ) )
    {
        msg.Printf( _( "Error <%s> occurred loading symbol library <%s>." ),
                   (const wxChar*) err, (const wxChar*) fn.GetName() );
        DisplayError( this, msg );
        fclose( ImportFile );
        delete Lib;
        return;
    }

    fclose( ImportFile );

    if( Lib->IsEmpty() )
    {
        msg.Printf( _( "No components found in symbol library <%s>." ),
                   (const wxChar*) fn.GetName() );
        delete Lib;
        return;
    }

    if( Lib->GetCount() > 1 )
        DisplayError( this, _( "Warning: more than 1 part in Symbol File" ) );

    Component = (LIB_COMPONENT*) Lib->GetFirstEntry();
    DrawEntry = Component->m_Drawings;

    while( DrawEntry )
    {
        if( DrawEntry->m_Unit )
            DrawEntry->m_Unit = CurrentUnit;
        if( DrawEntry->m_Convert )
            DrawEntry->m_Convert = CurrentConvert;
        DrawEntry->m_Flags    = IS_NEW;
        DrawEntry->m_Selected = IS_SELECTED;

        if( DrawEntry->Next() == NULL ) /* Fin de liste trouvee */
        {
            DrawEntry->SetNext( CurrentLibEntry->m_Drawings );
            CurrentLibEntry->m_Drawings = Component->m_Drawings;
            Component->m_Drawings = NULL;
            break;
        }

        DrawEntry = DrawEntry->Next();
    }

    // Remove duplicated drawings:
    CurrentLibEntry->RemoveDuplicateDrawItems();

    // Clear flags
    DrawEntry = CurrentLibEntry->m_Drawings;
    while( DrawEntry )
    {
        DrawEntry->m_Flags    = 0;
        DrawEntry->m_Selected = 0;
        DrawEntry = DrawEntry->Next();
    }

    GetScreen()->SetModify();
    DrawPanel->Refresh();

    delete Lib;
}


/*
 * Save in file the current symbol.
 *
 * The symbol file format is like the standard libraries, but there is only
 * one symbol.
 *
 * Invisible pins are not saved
 */
void WinEDA_LibeditFrame::SaveOneSymbol()
{
    LibEDA_BaseStruct* DrawEntry;
    wxString           msg;
    FILE*              ExportFile;

    if( CurrentLibEntry->m_Drawings == NULL )
        return;

    /* Creation du fichier symbole */
    wxString default_path = wxGetApp().ReturnLastVisitedLibraryPath();

    wxFileDialog dlg( this, _( "Export Symbol Drawings" ), default_path,
                      CurrentLibEntry->m_Name.m_Text, SymbolFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName fn = dlg.GetPath();

    /* The GTK file chooser doesn't return the file extension added to
     * file name so add it here. */
    if( fn.GetExt().IsEmpty() )
        fn.SetExt( SymbolFileExtension );

    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    ExportFile = wxFopen( fn.GetFullPath(), wxT( "wt" ) );

    if( ExportFile == NULL )
    {
        msg.Printf( _( "Unable to create <%s>" ),
                    (const wxChar*) fn.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    msg.Printf( _( "Save Symbol in [%s]" ), (const wxChar*) fn.GetPath() );
    Affiche_Message( msg );

    /* Creation de l'entete de la librairie */
    char Line[256];
    fprintf( ExportFile, "%s %d.%d  %s  Date: %s\n", LIBFILE_IDENT,
             LIB_VERSION_MAJOR, LIB_VERSION_MINOR,
             "SYMBOL", DateAndTime( Line ) );

    /* Creation du commentaire donnant le nom du composant */
    fprintf( ExportFile, "# SYMBOL %s\n#\n",
             CONV_TO_UTF8( CurrentLibEntry->m_Name.m_Text ) );

    /* Generation des lignes utiles */
    fprintf( ExportFile, "DEF %s",
             CONV_TO_UTF8( CurrentLibEntry->m_Name.m_Text ) );
    if( !CurrentLibEntry->m_Prefix.m_Text.IsEmpty() )
        fprintf( ExportFile, " %s",
                 CONV_TO_UTF8( CurrentLibEntry->m_Prefix.m_Text ) );
    else
        fprintf( ExportFile, " ~" );

    fprintf( ExportFile, " %d %d %c %c %d %d %c\n",
             0, /* unused */
             CurrentLibEntry->m_TextInside,
             CurrentLibEntry->m_DrawPinNum ? 'Y' : 'N',
             CurrentLibEntry->m_DrawPinName ? 'Y' : 'N',
             1, 0 /* unused */, 'N' );

    /* Position / orientation / visibilite des champs */
    CurrentLibEntry->m_Prefix.Save( ExportFile );
    CurrentLibEntry->m_Name.Save( ExportFile );
    DrawEntry = CurrentLibEntry->m_Drawings;

    if( DrawEntry )
    {
        fprintf( ExportFile, "DRAW\n" );
        for( ; DrawEntry != NULL; DrawEntry = DrawEntry->Next() )
        {
            /* Elimination des elements non relatifs a l'unite */
            if( CurrentUnit && DrawEntry->m_Unit
                && ( DrawEntry->m_Unit != CurrentUnit ) )
                continue;
            if( CurrentConvert && DrawEntry->m_Convert
               && ( DrawEntry->m_Convert != CurrentConvert ) )
                continue;

            DrawEntry->Save( ExportFile );
        }

        fprintf( ExportFile, "ENDDRAW\n" );
    }

    fprintf( ExportFile, "ENDDEF\n" );
    fclose( ExportFile );
}


/***************************************************************************/
/* Routine de placement du point d'ancrage ( reference des coordonnes pour */
/* le trace) du composant courant                                             */
/*  Toutes les coord apparaissant dans les structures sont modifiees          */
/*  pour repositionner le point repere par le curseur souris au point     */
/*  d'ancrage ( coord 0,0 ).                                               */
/***************************************************************************/
void WinEDA_LibeditFrame::PlaceAncre()
{
    if( CurrentLibEntry == NULL )
        return;

    wxPoint offset( -GetScreen()->m_Curseur.x, GetScreen()->m_Curseur.y );

    GetScreen()->SetModify();

    CurrentLibEntry->SetOffset( offset );

    /* Redraw the symbol */
    GetScreen()->m_Curseur.x = GetScreen()->m_Curseur.y = 0;
    Recadre_Trace( TRUE );
    GetScreen()->SetRefreshReq();
}
