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
#include "general.h"
#include "protos.h"
#include "libeditfrm.h"
#include "class_library.h"

#include <boost/foreach.hpp>


/*
 * Read a component shape file (a symbol file *.sym )and add data (graphic
 * items) to the current component.
 *
 * A symbol file *.sym has the same format as a library, and contains only
 * one symbol
 */
void WinEDA_LibeditFrame::LoadOneSymbol( void )
{
    LIB_COMPONENT* Component;
    FILE*          ImportFile;
    wxString       msg, err;
    CMP_LIBRARY*   Lib;

    /* Exit if no library entry is selected or a command is in progress. */
    if( m_component == NULL || ( m_drawItem && m_drawItem->m_Flags ) )
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
        DisplayError( this, _( "Warning: more than 1 part in symbol file." ) );

    Component = (LIB_COMPONENT*) Lib->GetFirstEntry();
    LIB_DRAW_ITEM_LIST& drawList = Component->GetDrawItemList();

    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawList )
    {
        if( item.m_Unit )
            item.m_Unit = m_unit;
        if( item.m_Convert )
            item.m_Convert = m_convert;
        item.m_Flags    = IS_NEW;
        item.m_Selected = IS_SELECTED;

        LIB_DRAW_ITEM* newItem = item.GenCopy();
        newItem->SetParent( m_component );
        m_component->AddDrawItem( newItem );
    }

    // Remove duplicated drawings:
    m_component->RemoveDuplicateDrawItems();

    // Clear flags
    m_component->ClearSelectedItems();

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
    wxString msg;
    FILE*    ExportFile;

    if( m_component->GetDrawItemList().empty() )
        return;

    /* Creation du fichier symbole */
    wxString default_path = wxGetApp().ReturnLastVisitedLibraryPath();

    wxFileDialog dlg( this, _( "Export Symbol Drawings" ), default_path,
                      m_component->m_Name.m_Text, SymbolFileWildcard,
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
             CONV_TO_UTF8( m_component->m_Name.m_Text ) );

    /* Generation des lignes utiles */
    fprintf( ExportFile, "DEF %s",
             CONV_TO_UTF8( m_component->m_Name.m_Text ) );
    if( !m_component->m_Prefix.m_Text.IsEmpty() )
        fprintf( ExportFile, " %s",
                 CONV_TO_UTF8( m_component->m_Prefix.m_Text ) );
    else
        fprintf( ExportFile, " ~" );

    fprintf( ExportFile, " %d %d %c %c %d %d %c\n",
             0, /* unused */
             m_component->m_TextInside,
             m_component->m_DrawPinNum ? 'Y' : 'N',
             m_component->m_DrawPinName ? 'Y' : 'N',
             1, 0 /* unused */, 'N' );

    /* Position / orientation / visibilite des champs */
    m_component->m_Prefix.Save( ExportFile );
    m_component->m_Name.Save( ExportFile );

    LIB_DRAW_ITEM_LIST& drawList = m_component->GetDrawItemList();

    fprintf( ExportFile, "DRAW\n" );

    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawList )
    {
        /* Elimination des elements non relatifs a l'unite */
        if( m_unit && item.m_Unit && ( item.m_Unit != m_unit ) )
            continue;
        if( m_convert && item.m_Convert && ( item.m_Convert != m_convert ) )
            continue;

        item.Save( ExportFile );
    }

    fprintf( ExportFile, "ENDDRAW\n" );
    fprintf( ExportFile, "ENDDEF\n" );
    fclose( ExportFile );
}


/***************************************************************************/
/* Routine de placement du point d'ancrage ( reference des coordonnes pour */
/* le trace) du composant courant                                          */
/*  Toutes les coord apparaissant dans les structures sont modifiees       */
/*  pour repositionner le point repere par le curseur souris au point      */
/*  d'ancrage ( coord 0,0 ).                                               */
/***************************************************************************/
void WinEDA_LibeditFrame::PlaceAncre()
{
    if( m_component == NULL )
        return;

    wxPoint offset( -GetScreen()->m_Curseur.x, GetScreen()->m_Curseur.y );

    GetScreen()->SetModify();

    m_component->SetOffset( offset );

    /* Redraw the symbol */
    GetScreen()->m_Curseur.x = GetScreen()->m_Curseur.y = 0;
    Recadre_Trace( TRUE );
    GetScreen()->SetRefreshReq();
}
