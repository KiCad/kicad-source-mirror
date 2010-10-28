/*************************************************/
/* Functions to Load from file and save to file  */
/* the graphic shapes used to draw a component   */
/* When using the import/export symbol options   */
/* files are the *.sym files                     */
/*************************************************/

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
#include "libeditframe.h"
#include "class_library.h"

#include <boost/foreach.hpp>
#include <wx/ffile.h>


/*
 * Read a component shape file (symbol file *.sym ) and add data (graphic
 * items) to the current component.
 *
 * A symbol file *.sym has the same format as a library, and contains only
 * one symbol
 */
void WinEDA_LibeditFrame::LoadOneSymbol( void )
{
    LIB_COMPONENT* Component;
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

    Lib = new CMP_LIBRARY( LIBRARY_TYPE_SYMBOL, fn );

    if( !Lib->Load( err ) )
    {
        msg.Printf( _( "Error <%s> occurred loading symbol library <%s>." ),
                    GetChars( err ), GetChars( fn.GetName() ) );
        DisplayError( this, msg );
        delete Lib;
        return;
    }

    if( Lib->IsEmpty() )
    {
        msg.Printf( _( "No components found in symbol library <%s>." ),
                    GetChars( fn.GetName() ) );
        delete Lib;
        return;
    }

    if( Lib->GetCount() > 1 )
    {
        msg.Printf( _( "More than one part in symbol file <%s>." ),
                    GetChars( fn.GetName() ) );
        wxMessageBox( msg, _( "Warning" ), wxOK | wxICON_EXCLAMATION, this );
    }

    Component = Lib->GetFirstEntry()->GetComponent();
    LIB_DRAW_ITEM_LIST& drawList = Component->GetDrawItemList();

    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawList )
    {
        if( item.Type() == COMPONENT_FIELD_DRAW_TYPE )
            continue;
        if( item.GetUnit() )
            item.SetUnit( m_unit );
        if( item.GetConvert() )
            item.SetConvert( m_convert );
        item.m_Flags    = IS_NEW;
        item.m_Selected = IS_SELECTED;

        LIB_DRAW_ITEM* newItem = item.GenCopy();
        newItem->SetParent( m_component );
        m_component->AddDrawItem( newItem );
    }

    m_component->RemoveDuplicateDrawItems();
    m_component->ClearSelectedItems();

    OnModify( );
    DrawPanel->Refresh();

    delete Lib;
}


/*
 * Save the current symbol to a file.
 *
 * The symbol file format is like the standard libraries, but there is only
 * one symbol.
 *
 * Invisible pins are not saved
 */
void WinEDA_LibeditFrame::SaveOneSymbol()
{
    wxString msg;

    if( m_component->GetDrawItemList().empty() )
        return;

    wxString default_path = wxGetApp().ReturnLastVisitedLibraryPath();

    wxFileDialog dlg( this, _( "Export Symbol Drawings" ), default_path,
                      m_component->GetName(), SymbolFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName fn = dlg.GetPath();

    /* The GTK file chooser doesn't return the file extension added to
     * file name so add it here. */
    if( fn.GetExt().IsEmpty() )
        fn.SetExt( SymbolFileExtension );

    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    wxFFile file( fn.GetFullPath(), wxT( "wt" ) );

    if( !file.IsOpened() )
    {
        msg.Printf( _( "Unable to create file <%s>" ),
                    GetChars( fn.GetFullPath() ) );
        DisplayError( this, msg );
        return;
    }

    msg.Printf( _( "Saving symbol in [%s]" ), GetChars( fn.GetPath() ) );
    SetStatusText( msg );

    wxString line;

    /* File header */
    line << wxT( LIBFILE_IDENT ) << wxT( " " ) << LIB_VERSION_MAJOR
         << wxT( "." ) << LIB_VERSION_MINOR << wxT( "  SYMBOL  " )
         << wxT( "Date: " ) << DateAndTime() << wxT( "\n" );

    /* Component name comment and definition. */
    line << wxT( "# SYMBOL " ) << m_component->GetName() << wxT( "\n#\nDEF " )
         << m_component->GetName() << wxT( " " );

    if( !m_component->GetReferenceField().m_Text.IsEmpty() )
        line << m_component->GetReferenceField().m_Text << wxT( " " );
    else
        line << wxT( "~ " );

    line << 0 << wxT( " " ) << m_component->GetPinNameOffset() << wxT( " " );

    if( m_component->ShowPinNumbers() )
        line << wxT( "Y " );
    else
        line << wxT( "N " );

    if( m_component->ShowPinNames() )
        line << wxT( "Y " );
    else
        line << wxT( "N " );

    line << wxT( "1 0 N\n" );

    if( !file.Write( line )
        || !m_component->GetReferenceField().Save( file.fp() )
        || !m_component->GetValueField().Save( file.fp() )
        || !file.Write( wxT( "DRAW\n" ) ) )
        return;

    LIB_DRAW_ITEM_LIST& drawList = m_component->GetDrawItemList();

    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawList )
    {
        if( item.Type() == COMPONENT_FIELD_DRAW_TYPE )
            continue;
        /* Don't save unused parts or alternate body styles. */
        if( m_unit && item.GetUnit() && ( item.GetUnit() != m_unit ) )
            continue;
        if( m_convert && item.GetConvert() && ( item.GetConvert() != m_convert ) )
            continue;

        if( !item.Save( file.fp() ) )
            return;
    }

    if( !file.Write( wxT( "ENDDRAW\n" ) )
        || !file.Write( wxT( "ENDDEF\n" ) ) )
        return;
}


/*
 * Place anchor reference coordinators for current component
 *
 * All coordinates of the object are offset to the cursor position.
 */
void WinEDA_LibeditFrame::PlaceAncre()
{
    if( m_component == NULL )
        return;

    wxPoint offset( -GetScreen()->m_Curseur.x, GetScreen()->m_Curseur.y );

    OnModify( );

    m_component->SetOffset( offset );

    /* Redraw the symbol */
    GetScreen()->m_Curseur.x = GetScreen()->m_Curseur.y = 0;
    Recadre_Trace( TRUE );
    DrawPanel->Refresh();
}
