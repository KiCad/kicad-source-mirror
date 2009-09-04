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


static bool CompareSymbols( LibEDA_BaseStruct* DEntryRef,
                            LibEDA_BaseStruct* DEntryCompare );


/*
 * Read a component shape file (a symbol file *.sym )and add data (graphic
 * items) to the current component.
 *
 * A symbol file *.sym has the same format as a library, and contains only
 * one symbol
 */
void WinEDA_LibeditFrame::LoadOneSymbol( void )
{
    EDA_LibComponentStruct* Component;
    LibEDA_BaseStruct*      DrawEntry;
    FILE*                   ImportFile;
    wxString                msg, err;
    LibraryStruct*          Lib;

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

    Lib = new LibraryStruct( LIBRARY_TYPE_SYMBOL, fn.GetName(),
                             fn.GetFullPath() );

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

    Component = (EDA_LibComponentStruct*) Lib->GetFirstEntry();
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
    SuppressDuplicateDrawItem( CurrentLibEntry );

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


/*
 * Delete redundant graphic items.
 *
 * Useful after loading asymbole from a file symbol, because some graphic items
 * can be duplicated.
 */
void SuppressDuplicateDrawItem( EDA_LibComponentStruct* LibEntry )
{
    LibEDA_BaseStruct* DEntryRef, * DEntryCompare;
    bool  deleted;
    wxDC* DC = NULL;

    DEntryRef = LibEntry->m_Drawings;
    while( DEntryRef )
    {
        if( DEntryRef->Next() == NULL )
            return;
        DEntryCompare = DEntryRef->Next();
        if( DEntryCompare == NULL )
            return;
        deleted = 0;
        while( DEntryCompare )
        {
            if( CompareSymbols( DEntryRef, DEntryCompare ) == TRUE )
            {
                LibEntry->RemoveDrawItem( DEntryRef, NULL, DC );
                deleted = TRUE;
                break;
            }
            DEntryCompare = DEntryCompare->Next();
        }

        if( !deleted )
            DEntryRef = DEntryRef->Next();
        else
            DEntryRef = LibEntry->m_Drawings;
    }
}


/*
 * Compare 2 graphic items (arc, lines ...).
 *
 * return FALSE if different
 *        TRUE if they are identical, and therefore redundant
 */
static bool CompareSymbols( LibEDA_BaseStruct* DEntryRef,
                            LibEDA_BaseStruct* DEntryCompare )
{
    /* Comparaison des proprietes generales */
    if( DEntryRef->Type() != DEntryCompare->Type() )
        return FALSE;
    if( DEntryRef->m_Unit != DEntryCompare->m_Unit )
        return FALSE;
    if( DEntryRef->m_Convert != DEntryCompare->m_Convert )
        return FALSE;

    switch( DEntryRef->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
        #undef REFSTRUCT
        #undef CMPSTRUCT
        #define REFSTRUCT ( (LibDrawArc*) DEntryRef )
        #define CMPSTRUCT ( (LibDrawArc*) DEntryCompare )
        if( REFSTRUCT->m_Pos.x != CMPSTRUCT->m_Pos.x )
            return FALSE;
        if( REFSTRUCT->m_Pos.y != CMPSTRUCT->m_Pos.y )
            return FALSE;
        if( REFSTRUCT->t1 != CMPSTRUCT->t1 )
            return FALSE;
        if( REFSTRUCT->t2 != CMPSTRUCT->t2 )
            return FALSE;
        break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
        #undef REFSTRUCT
        #undef CMPSTRUCT
        #define REFSTRUCT ( (LibDrawCircle*) DEntryRef )
        #define CMPSTRUCT ( (LibDrawCircle*) DEntryCompare )
        if( REFSTRUCT->m_Pos.x != CMPSTRUCT->m_Pos.x )
            return FALSE;
        if( REFSTRUCT->m_Pos.y != CMPSTRUCT->m_Pos.y )
            return FALSE;
        if( REFSTRUCT->m_Rayon != CMPSTRUCT->m_Rayon )
            return FALSE;
        break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        #undef REFSTRUCT
        #undef CMPSTRUCT
        #define REFSTRUCT ( (LibDrawText*) DEntryRef )
        #define CMPSTRUCT ( (LibDrawText*) DEntryCompare )
        if( REFSTRUCT->m_Pos != CMPSTRUCT->m_Pos )
            return FALSE;
        if( REFSTRUCT->m_Size != CMPSTRUCT->m_Size )
            return FALSE;
        if( REFSTRUCT->m_Text != CMPSTRUCT->m_Text )
            return FALSE;
        break;

    case COMPONENT_RECT_DRAW_TYPE:
        #undef REFSTRUCT
        #undef CMPSTRUCT
        #define REFSTRUCT ( (LibDrawSquare*) DEntryRef )
        #define CMPSTRUCT ( (LibDrawSquare*) DEntryCompare )
        if( REFSTRUCT->m_Pos != CMPSTRUCT->m_Pos )
            return FALSE;
        if( REFSTRUCT->m_End != CMPSTRUCT->m_End )
            return FALSE;
        break;

    case COMPONENT_PIN_DRAW_TYPE:
        #undef REFSTRUCT
        #undef CMPSTRUCT
        #define REFSTRUCT ( (LibDrawPin*) DEntryRef )
        #define CMPSTRUCT ( (LibDrawPin*) DEntryCompare )
        if( REFSTRUCT->m_Pos != CMPSTRUCT->m_Pos )
            return FALSE;
        break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
        #undef REFSTRUCT
        #undef CMPSTRUCT
        #define REFSTRUCT ( (LibDrawPolyline*) DEntryRef )
        #define CMPSTRUCT ( (LibDrawPolyline*) DEntryCompare )
        if( REFSTRUCT->GetCornerCount() != CMPSTRUCT->GetCornerCount() )
            return FALSE;
        for( unsigned ii = 0; ii < REFSTRUCT->GetCornerCount(); ii++ )
        {
            if( REFSTRUCT->m_PolyPoints[ii] != CMPSTRUCT->m_PolyPoints[ii] )
                return false;
        }

        break;

    default:
        ;
    }

    return TRUE;
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
    EDA_LibComponentStruct* LibEntry;
    LibEDA_BaseStruct*      DrawEntry;

    LibEntry = CurrentLibEntry;
    if( LibEntry == NULL )
        return;

    wxSize offset( -GetScreen()->m_Curseur.x, GetScreen()->m_Curseur.y );

    GetScreen()->SetModify();

    LibEntry->m_Name.m_Pos   += offset;
    LibEntry->m_Prefix.m_Pos += offset;

    for( LibDrawField* field = LibEntry->m_Fields; field; field = field->Next() )
    {
        field->m_Pos += offset;
    }

    DrawEntry = LibEntry->m_Drawings;
    while( DrawEntry )
    {
        switch( DrawEntry->Type() )
        {
        case COMPONENT_ARC_DRAW_TYPE:
            #undef STRUCT
            #define STRUCT ( (LibDrawArc*) DrawEntry )
            STRUCT->m_Pos += offset;
            STRUCT->m_ArcStart += offset;
            STRUCT->m_ArcEnd   += offset;
            break;

        case COMPONENT_CIRCLE_DRAW_TYPE:
            #undef STRUCT
            #define STRUCT ( (LibDrawCircle*) DrawEntry )
            STRUCT->m_Pos += offset;
            break;

        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
            #undef STRUCT
            #define STRUCT ( (LibDrawText*) DrawEntry )
            STRUCT->m_Pos += offset;
            break;

        case COMPONENT_RECT_DRAW_TYPE:
            #undef STRUCT
            #define STRUCT ( (LibDrawSquare*) DrawEntry )
            STRUCT->m_Pos += offset;
            STRUCT->m_End += offset;
            break;

        case COMPONENT_PIN_DRAW_TYPE:
            #undef STRUCT
            #define STRUCT ( (LibDrawPin*) DrawEntry )
            STRUCT->m_Pos += offset;
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
            #undef STRUCT
            #define STRUCT ( (LibDrawPolyline*) DrawEntry )
            for( unsigned ii = 0; ii < STRUCT->GetCornerCount(); ii++ )
                STRUCT->m_PolyPoints[ii] += offset;

            break;

        default:
            break;
        }
        DrawEntry = DrawEntry->Next();
    }

    /* Redraw the symbol */
    GetScreen()->m_Curseur.x = GetScreen()->m_Curseur.y = 0;
    Recadre_Trace( TRUE );
    GetScreen()->SetRefreshReq();
}
