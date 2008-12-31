/*************************************************/
/* Functions to Load  from file and save to file */
/* the graphic shapes  used to draw a component  */
/* When using the import/export symbol options	 */
/* files are the *.sym files 					 */
/*************************************************/

/* fichier symbedit.cpp */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/* Routines locales */
static bool CompareSymbols( LibEDA_BaseStruct* DEntryRef,
                            LibEDA_BaseStruct* DEntryCompare );

/* Variables locales */


/***************************************************/
void WinEDA_LibeditFrame::LoadOneSymbol( wxDC* DC )
/***************************************************/

/* Read a component shape file and add data (graphic items) to the current
 *  component.
 */
{
    int                     NumOfParts;
    PriorQue*               Entries;
    EDA_LibComponentStruct* LibEntry = NULL;
    LibEDA_BaseStruct*      DrawEntry;
    wxString                FullFileName, mask;
    FILE*                   ImportFile;
    wxString                msg;

    if( CurrentDrawItem )
        return;
    if( CurrentLibEntry == NULL )
        return;

    DrawPanel->m_IgnoreMouseEvents = TRUE;

    mask = wxT( "*" ) + g_SymbolExtBuffer;
    FullFileName = EDA_FileSelector( _( "Import symbol drawings:" ),
                                     g_RealLibDirBuffer,    /* Chemin par defaut */
                                     wxEmptyString,         /* nom fichier par defaut */
                                     g_SymbolExtBuffer,     /* extension par defaut */
                                     mask,                  /* Masque d'affichage */
                                     this,
                                     0,
                                     TRUE
                                     );

    GetScreen()->m_Curseur = wxPoint( 0, 0 );
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    if( FullFileName.IsEmpty() )
        return;


    /* Load data */
    ImportFile = wxFopen( FullFileName, wxT( "rt" ) );
    if( ImportFile == NULL )
    {
        msg.Printf( _( "Failed to open Symbol File <%s>" ), FullFileName.GetData() );
        DisplayError( this, msg, 20 );
        return;
    }


    Entries = LoadLibraryAux( this, NULL, ImportFile, &NumOfParts );
    fclose( ImportFile );

    if( Entries == NULL )
        return;

    if( NumOfParts > 1 )
        DisplayError( this, _( "Warning: more than 1 part in Symbol File" ), 20 );

    LibEntry = (EDA_LibComponentStruct*) PQFirst( &Entries, FALSE );

    if( LibEntry == NULL )
        DisplayError( this, _( "Symbol File is void" ), 20 );

    else /* add data to the current symbol */
    {
        DrawEntry = LibEntry->m_Drawings;
        while( DrawEntry )
        {
            if( DrawEntry->m_Unit )
                DrawEntry->m_Unit = CurrentUnit;
            if( DrawEntry->m_Convert )
                DrawEntry->m_Convert = CurrentConvert;
            DrawEntry->m_Flags    = IS_NEW;
            DrawEntry->m_Selected = IS_SELECTED;

            if( DrawEntry->Next() == NULL )
            {   /* Fin de liste trouvee */
                DrawEntry->SetNext( CurrentLibEntry->m_Drawings );
                CurrentLibEntry->m_Drawings = LibEntry->m_Drawings;
                LibEntry->m_Drawings = NULL;
                break;
            }
            DrawEntry = DrawEntry->Next();
        }

        SuppressDuplicateDrawItem( CurrentLibEntry );
        GetScreen()->SetModify();

        // Move (and place ) the new draw items:
        HandleBlockBegin( DC, -1, GetScreen()->m_Curseur );
        HandleBlockEnd( DC );
        RedrawActiveWindow( DC, TRUE );
    }

    PQFreeFunc( Entries, ( void( * ) ( void* ) )FreeLibraryEntry );
}


/********************************************/
void WinEDA_LibeditFrame::SaveOneSymbol()
/********************************************/

/* Save in file the current symbol
 *  file format is like the standard libraries, but there is only one symbol
 *  Invisible pins are not saved
 */
{
    EDA_LibComponentStruct* LibEntry = CurrentLibEntry;
    int Unit = CurrentUnit, convert = CurrentConvert;
    LibEDA_BaseStruct*      DrawEntry;
    wxString FullFileName, mask;
    wxString msg;
    FILE*    ExportFile;

    if( LibEntry->m_Drawings == NULL )
        return;

    /* Creation du fichier symbole */
    mask = wxT( "*" ) + g_SymbolExtBuffer;
    FullFileName = EDA_FileSelector( _( "Export symbol drawings:" ),
                                     g_RealLibDirBuffer,    /* Chemin par defaut */
                                     wxEmptyString,         /* nom fichier par defaut */
                                     g_SymbolExtBuffer,     /* extension par defaut */
                                     mask,                  /* Masque d'affichage */
                                     this,
                                     wxFD_SAVE,
                                     TRUE
                                     );
    if( FullFileName.IsEmpty() )
        return;

    ExportFile = wxFopen( FullFileName, wxT( "wt" ) );
    if( ExportFile == NULL )
    {
        msg.Printf( _( "Unable to create <%s>" ), FullFileName.GetData() );
        DisplayError( this, msg );
        return;
    }

    msg.Printf( _( "Save Symbol in [%s]" ), FullFileName.GetData() );
    Affiche_Message( msg );

    /* Creation de l'entete de la librairie */
    char Line[256];
    fprintf( ExportFile, "%s %d.%d  %s  Date: %s\n", LIBFILE_IDENT,
            LIB_VERSION_MAJOR, LIB_VERSION_MINOR,
            "SYMBOL", DateAndTime( Line ) );

    /* Creation du commentaire donnant le nom du composant */
    fprintf( ExportFile, "# SYMBOL %s\n#\n",
            (const char*) LibEntry->m_Name.m_Text.GetData() );

    /* Generation des lignes utiles */
    fprintf( ExportFile, "DEF %s", (const char*) LibEntry->m_Name.m_Text.GetData() );
    if( !LibEntry->m_Prefix.m_Text.IsEmpty() )
        fprintf( ExportFile, " %s", (const char*) LibEntry->m_Prefix.m_Text.GetData() );
    else
        fprintf( ExportFile, " ~" );

    fprintf( ExportFile, " %d %d %c %c %d %d %c\n",
             0, /* unused */
             LibEntry->m_TextInside,
             LibEntry->m_DrawPinNum ? 'Y' : 'N',
             LibEntry->m_DrawPinName ? 'Y' : 'N',
             1, 0 /* unused */, 'N' );

    /* Position / orientation / visibilite des champs */
    LibEntry->m_Prefix.Save( ExportFile );
    LibEntry->m_Name.Save( ExportFile );

    DrawEntry = LibEntry->m_Drawings;
    if( DrawEntry )
    {
        fprintf( ExportFile, "DRAW\n" );
        for( ; DrawEntry != NULL; DrawEntry = DrawEntry->Next() )
        {
            /* Elimination des elements non relatifs a l'unite */
            if( Unit && DrawEntry->m_Unit && (DrawEntry->m_Unit != Unit) )
                continue;
            if( convert && DrawEntry->m_Convert && (DrawEntry->m_Convert != convert) )
                continue;

            DrawEntry->Save( ExportFile );
        }
        fprintf( ExportFile, "ENDDRAW\n" );
    }
    fprintf( ExportFile, "ENDDEF\n" );
    fclose( ExportFile );
}


/*****************************************************************/
void SuppressDuplicateDrawItem( EDA_LibComponentStruct* LibEntry )
/*****************************************************************/

/* Delete redundant graphic items.
 *  Useful after loading asymbole from a file symbol, because some graphic items
 *  can be duplicated.
 */
{
    LibEDA_BaseStruct* DEntryRef, * DEntryCompare;
    bool deleted;
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
                DeleteOneLibraryDrawStruct( NULL, DC, LibEntry, DEntryRef, 1 );
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


/********************************************************************/
static bool CompareSymbols( LibEDA_BaseStruct* DEntryRef,
                            LibEDA_BaseStruct* DEntryCompare )
/********************************************************************/

/* Compare 2 graphic items (arc, lines ...).
 *  return FALSE si different
 *          TRUE si they are identical, and therefore redundant
 */
{
    int ii;
    int* ptref, * ptcomp;

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
        if( REFSTRUCT->m_CornersCount != CMPSTRUCT->m_CornersCount )
            return FALSE;
        ptref  = REFSTRUCT->m_PolyList;
        ptcomp = CMPSTRUCT->m_PolyList;
        for( ii = 2 * REFSTRUCT->m_CornersCount; ii > 0; ii-- )
        {
            if( *ptref != *ptcomp )
                return FALSE;
            ptref++; ptcomp++;
        }

        break;

    default:
        ;
    }

    return TRUE;
}


/***************************************************************************/
/* Routine de placement du point d'ancrage ( reference des coordonnes pour */
/* le trace) du composant courant											  */
/*	Toutes les coord apparaissant dans les structures sont modifiees		  */
/*	pour repositionner le point repere par le curseur souris au point	  */
/*	d'ancrage ( coord 0,0 ).                                               */
/***************************************************************************/

void WinEDA_LibeditFrame::PlaceAncre()
{
    EDA_LibComponentStruct* LibEntry;
    LibEDA_BaseStruct*      DrawEntry;

    LibEntry = CurrentLibEntry;
    if( LibEntry == NULL )
        return;

    wxSize  offset( -GetScreen()->m_Curseur.x, GetScreen()->m_Curseur.y );

    GetScreen()->SetModify();

    LibEntry->m_Name.m_Pos   += offset;
    LibEntry->m_Prefix.m_Pos += offset;

    for( LibDrawField* field = LibEntry->m_Fields;  field;  field = field->Next() )
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
            STRUCT->m_Pos       += offset;
            STRUCT->m_ArcStart  += offset;
            STRUCT->m_ArcEnd    += offset;
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
            int     ii;
            int*    ptsegm;
            for( ptsegm = STRUCT->m_PolyList,
                 ii = STRUCT->m_CornersCount;  ii > 0;  ii-- )
            {
                *ptsegm++ += offset.x;
                *ptsegm++ += offset.y;
            }
            break;

        default:
            ;
        }
        DrawEntry = DrawEntry->Next();
    }

    /* Redraw the symbol */
    GetScreen()->m_Curseur.x = GetScreen()->m_Curseur.y = 0;
    Recadre_Trace( TRUE );
    GetScreen()->SetRefreshReq();
}
