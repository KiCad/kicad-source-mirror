/****************************/
/*	EESchema - eesavlib.cpp	*/
/****************************/

/* Write Routines to save schematic libraries and library components (::Save() members)
 */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

/* Routines locales */

/* Variables locales */


static bool WriteLibEntryDateAndTime( FILE*                   ExportFile,
                                      EDA_LibComponentStruct* LibEntry );


static int fill_tab[3] = { 'N', 'F', 'f' };

/***********************************************/
bool LibDrawArc::Save( FILE* ExportFile ) const
/***********************************************/

/* format
 *  A centre_posx centre_posy rayon start_angle end_angle unit convert fill('N', 'F' ou 'f') startx starty endx endy
 */
{
    int x1 = t1; if( x1 > 1800 )
        x1 -= 3600;

    int x2 = t2; if( x2 > 1800 )
        x2 -= 3600;

    fprintf( ExportFile, "A %d %d %d %d %d %d %d %d %c %d %d %d %d\n",
             m_Pos.x, m_Pos.y,
             m_Rayon, x1, x2,
             m_Unit, m_Convert,
             m_Width, fill_tab[m_Fill],
             m_ArcStart.x, m_ArcStart.y, m_ArcEnd.x, m_ArcEnd.y );
    return true;
}


/***************************************************/
bool LibDrawCircle::Save( FILE* ExportFile ) const
/***************************************************/
{
    fprintf( ExportFile, "C %d %d %d %d %d %d %c\n",
             m_Pos.x, m_Pos.y,
             m_Rayon,
             m_Unit, m_Convert,
             m_Width, fill_tab[m_Fill] );
    return true;
}


/************************************************/
bool LibDrawText::Save( FILE* ExportFile ) const
/************************************************/
{
    wxString text = m_Text;

    text.Replace( wxT( " " ), wxT( "~" ) );   // Spaces are not allowed: changed to '~'

    fprintf( ExportFile, "T %d %d %d %d %d %d %d %s\n",
            m_Horiz,
            m_Pos.x, m_Pos.y,
            m_Size.x, m_Type,
            m_Unit, m_Convert,
            CONV_TO_UTF8( text ) );
    return true;
}


/***************************************************/
bool LibDrawSquare::Save( FILE* ExportFile ) const
/***************************************************/
{
    fprintf( ExportFile, "S %d %d %d %d %d %d %d %c\n",
             m_Pos.x, m_Pos.y, m_End.x, m_End.y,
             m_Unit, m_Convert,
             m_Width, fill_tab[m_Fill] );
    return true;
}


/************************************************/
bool LibDrawPin::Save( FILE* ExportFile ) const
/************************************************/
{
    wxString StringPinNum;
    int      Etype;

    switch( m_PinType )
    {
    default:
    case PIN_INPUT:         Etype = 'I';    break;
    case PIN_OUTPUT:        Etype = 'O';    break;
    case PIN_BIDI:          Etype = 'B';    break;
    case PIN_TRISTATE:      Etype = 'T';    break;
    case PIN_PASSIVE:       Etype = 'P';    break;
    case PIN_UNSPECIFIED:   Etype = 'U';    break;
    case PIN_POWER_IN:      Etype = 'W';    break;
    case PIN_POWER_OUT:     Etype = 'w';    break;
    case PIN_OPENCOLLECTOR: Etype = 'C';    break;
    case PIN_OPENEMITTER:   Etype = 'E';    break;
    }

    ReturnPinStringNum( StringPinNum );
    if( StringPinNum.IsEmpty() )
        StringPinNum = wxT( "~" );

    if( !m_PinName.IsEmpty() )
        fprintf( ExportFile, "X %s", CONV_TO_UTF8( m_PinName ) );
    else
        fprintf( ExportFile, "X ~" );

    fprintf( ExportFile, " %s %d %d %d %c %d %d %d %d %c",
             CONV_TO_UTF8( StringPinNum ),
             m_Pos.x, m_Pos.y,
             (int) m_PinLen, (int) m_Orient,
             m_PinNumSize, m_PinNameSize,
             m_Unit, m_Convert, Etype );

    if( (m_PinShape) || (m_Attributs & PINNOTDRAW) )
        fprintf( ExportFile, " " );
    if( m_Attributs & PINNOTDRAW )
        fprintf( ExportFile, "N" );
    if( m_PinShape & INVERT )
        fprintf( ExportFile, "I" );
    if( m_PinShape & CLOCK )
        fprintf( ExportFile, "C" );
    if( m_PinShape & LOWLEVEL_IN )
        fprintf( ExportFile, "L" );
    if( m_PinShape & LOWLEVEL_OUT )
        fprintf( ExportFile, "V" );

    fprintf( ExportFile, "\n" );
    return true;
}


/****************************************************/
bool LibDrawPolyline::Save( FILE* ExportFile ) const
/****************************************************/
{
    int ii, * ptpoly;

    fprintf( ExportFile, "P %d %d %d %d",
             m_CornersCount,
             m_Unit, m_Convert,
             m_Width );
    ptpoly = m_PolyList;
    for( ii = m_CornersCount; ii > 0; ii-- )
    {
        fprintf( ExportFile, "  %d %d", *ptpoly, *(ptpoly + 1) );
        ptpoly += 2;
    }

    fprintf( ExportFile, " %c\n", fill_tab[m_Fill] );
    return true;
}

/****************************************************/
bool LibDrawSegment::Save( FILE* ExportFile ) const
/****************************************************/
{
    fprintf( ExportFile, "L %d %d %d",
             m_Unit, m_Convert,
             m_Width );
    return true;
}


/**************************************************/
bool LibDrawField::Save( FILE* ExportFile ) const
/**************************************************/
{
    int      hjustify, vjustify;
    wxString text = m_Text;

    hjustify = 'C';
    if( m_HJustify == GR_TEXT_HJUSTIFY_LEFT )
        hjustify = 'L';
    else if( m_HJustify == GR_TEXT_HJUSTIFY_RIGHT )
        hjustify = 'R';
    vjustify = 'C';
    if( m_VJustify == GR_TEXT_VJUSTIFY_BOTTOM )
        vjustify = 'B';
    else if( m_VJustify == GR_TEXT_VJUSTIFY_TOP )
        vjustify = 'T';
    if( text.IsEmpty() )
        text = wxT( "~" );
    fprintf( ExportFile, "F%d \"%s\" %d %d %d %c %c %c %c",
             m_FieldId, CONV_TO_UTF8( text ),
             m_Pos.x, m_Pos.y,
             m_Size.x,
             m_Orient == 0 ? 'H' : 'V',
             (m_Attributs & TEXT_NO_VISIBLE ) ? 'I' : 'V',
             hjustify, vjustify );

    // Save field name, if necessary
    if( m_FieldId >= FIELD1 && !m_Name.IsEmpty() )
        fprintf( ExportFile, " \"%s\"", CONV_TO_UTF8( m_Name ) );

    fprintf( ExportFile, "\n" );
    return true;
}


/**********************************************************/
LibEDA_BaseStruct* CopyDrawEntryStruct( wxWindow*          frame,
                                        LibEDA_BaseStruct* DrawItem )
/**********************************************************/

/* Duplicate a DrawLibItem
 * the new item is only created, it is not put in the current component linked list
 * @param DrawEntry = DrawLibItem * item to duplicate
 * @return a pointer to the new item
 * A better way to duplicate a DrawLibItem is to have a virtual GenCopy() in LibEDA_BaseStruct class (ToDo).
 */
{
    LibEDA_BaseStruct* NewDrawItem = NULL;
    wxString           msg;

    switch( DrawItem->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
        NewDrawItem = ( (LibDrawArc*) DrawItem )->GenCopy();
        break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
        NewDrawItem = ( (LibDrawCircle*) DrawItem )->GenCopy();
        break;

    case COMPONENT_RECT_DRAW_TYPE:
        NewDrawItem = ( (LibDrawSquare*) DrawItem )->GenCopy();
        break;

    case COMPONENT_PIN_DRAW_TYPE:
        NewDrawItem = ( (LibDrawPin*) DrawItem )->GenCopy();
        break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        NewDrawItem = ( (LibDrawText*) DrawItem )->GenCopy();
        break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
        NewDrawItem = ( (LibDrawPolyline*) DrawItem )->GenCopy();
        break;

    default:
        msg.Printf( wxT( "CopyDrawLibEntryStruct: unknown Draw Type %d" ),
                   DrawItem->Type() );
        DisplayError( frame, msg );
        break;
    }

    return NewDrawItem;
}


/*************************************************************************************************/
EDA_LibComponentStruct* CopyLibEntryStruct( wxWindow* frame, EDA_LibComponentStruct* OldEntry )
/*************************************************************************************************/

/* Routine de copie d'une partlib
 *  Parametres d'entree: pointeur sur la structure de depart
 *  Parametres de sortie: pointeur sur la structure creee
 *  Do not copy new items ( i.e. with m_Flag  & IS_NEW)
 */
{
    EDA_LibComponentStruct* NewStruct;
    LibEDA_BaseStruct*      NewDrawings, * OldDrawings;
    LibEDA_BaseStruct*      LastItem;
    LibDrawField*           OldField, * NewField;

    if( OldEntry->Type != ROOT )
    {
        DisplayError( frame, wxT( "CopyLibEntryStruct(): Type != ROOT" ) );
        return NULL;
    }

    NewStruct = new EDA_LibComponentStruct( NULL );

    OldEntry->m_Prefix.Copy( &NewStruct->m_Prefix );
    OldEntry->m_Name.Copy( &NewStruct->m_Name );

    NewStruct->m_UnitCount   = OldEntry->m_UnitCount;
    NewStruct->m_TextInside  = OldEntry->m_TextInside;
    NewStruct->m_DrawPinNum  = OldEntry->m_DrawPinNum;
    NewStruct->m_DrawPinName = OldEntry->m_DrawPinName;
    NewStruct->m_Options = OldEntry->m_Options;
    NewStruct->m_UnitSelectionLocked = OldEntry->m_UnitSelectionLocked;

    /* Copie des sous structures: */
    NewStruct->m_AliasList = OldEntry->m_AliasList;
    NewStruct->m_Doc     = OldEntry->m_Doc;
    NewStruct->m_KeyWord = OldEntry->m_KeyWord;
    NewStruct->m_DocFile = OldEntry->m_DocFile;

    /* Copie des champs */
    for( OldField = OldEntry->Fields; OldField != NULL;
         OldField = (LibDrawField*) OldField->Pnext )
    {
        NewField = OldField->GenCopy();
        NewField->Pnext   = NewStruct->Fields;
        NewStruct->Fields = NewField;
    }

    /* Copie des elements type Drawing */
    LastItem = NULL;
    for( OldDrawings = OldEntry->m_Drawings; OldDrawings != NULL; OldDrawings = OldDrawings->Next() )
    {
        if( ( OldDrawings->m_Flags & IS_NEW) != 0 )
            continue;

        NewDrawings = CopyDrawEntryStruct( frame, OldDrawings );
        if( NewDrawings )
        {
            if( LastItem == NULL )
                NewStruct->m_Drawings = NewDrawings;
            else
                LastItem->Pnext = NewDrawings;

            LastItem = NewDrawings;
            NewDrawings->Pnext = NULL;
        }
        else	// Should nevers occurs, just in case...
        {       // CopyDrawEntryStruct() was not able to duplicate the type of OldDrawings
                // occurs when an unexpected type is encountered
            DisplayError( frame, wxT( "CopyLibEntryStruct(): error: aborted" ) );
            break;
        }
    }

    /* Copy the footprint filter list */
    for( unsigned ii = 0; ii < OldEntry->m_FootprintList.GetCount(); ii++ )
        NewStruct->m_FootprintList.Add( OldEntry->m_FootprintList[ii] );

    return NewStruct;
}


/*************************************************************************/
int WriteOneLibEntry( FILE* ExportFile, EDA_LibComponentStruct* LibEntry )
/*************************************************************************/

/* Routine d'ecriture du composant pointe par LibEntry
 *  dans le fichier ExportFile( qui doit etre deja ouvert)
 *  return: 0 si Ok
 *          -1 si err write
 *          1 si composant non ecrit ( type ALIAS )
 */
#define UNUSED 0
{
    LibEDA_BaseStruct* DrawEntry;
    LibDrawField*      Field;

    if( LibEntry->Type != ROOT )
        return 1;

    /* Creation du commentaire donnant le nom du composant */
    fprintf( ExportFile, "#\n# %s\n#\n", CONV_TO_UTF8( LibEntry->m_Name.m_Text ) );

    /* Generation des lignes utiles */
    fprintf( ExportFile, "DEF" );
    if( (LibEntry->m_Name.m_Attributs & TEXT_NO_VISIBLE) == 0 )
        fprintf( ExportFile, " %s", CONV_TO_UTF8( LibEntry->m_Name.m_Text ) );
    else
        fprintf( ExportFile, " ~%s", CONV_TO_UTF8( LibEntry->m_Name.m_Text ) );

    if( !LibEntry->m_Prefix.m_Text.IsEmpty() )
        fprintf( ExportFile, " %s", CONV_TO_UTF8( LibEntry->m_Prefix.m_Text ) );
    else
        fprintf( ExportFile, " ~" );
    fprintf( ExportFile, " %d %d %c %c %d %c %c\n",
             UNUSED, LibEntry->m_TextInside,
             LibEntry->m_DrawPinNum ? 'Y' : 'N',
             LibEntry->m_DrawPinName ? 'Y' : 'N',
             LibEntry->m_UnitCount, LibEntry->m_UnitSelectionLocked ? 'L' : 'F',
             LibEntry->m_Options == ENTRY_POWER ? 'P' : 'N' );

    WriteLibEntryDateAndTime( ExportFile, LibEntry );

    /* Position / orientation / visibilite des champs */
    LibEntry->m_Prefix.Save( ExportFile );
    LibEntry->m_Name.Save( ExportFile );

    for( Field = LibEntry->Fields; Field!= NULL;
         Field = (LibDrawField*) Field->Pnext )
    {
        if( Field->m_Text.IsEmpty() && Field->m_Name.IsEmpty() )
            continue;
        Field->Save( ExportFile );
    }

    /* Sauvegarde de la ligne "ALIAS" */
    if( LibEntry->m_AliasList.GetCount() != 0 )
    {
        fprintf( ExportFile, "ALIAS" );
        unsigned ii;
        for( ii = 0; ii < LibEntry->m_AliasList.GetCount(); ii++ )
            fprintf( ExportFile, " %s", CONV_TO_UTF8( LibEntry->m_AliasList[ii] ) );

        fprintf( ExportFile, "\n" );
    }

    /* Write the footprint filter list */
    if( LibEntry->m_FootprintList.GetCount() != 0 )
    {
        fprintf( ExportFile, "$FPLIST\n" );
        unsigned ii;
        for( ii = 0; ii < LibEntry->m_FootprintList.GetCount(); ii++ )
            fprintf( ExportFile, " %s\n", CONV_TO_UTF8( LibEntry->m_FootprintList[ii] ) );

        fprintf( ExportFile, "$ENDFPLIST\n" );
    }

    /* Sauvegarde des elements de trace */
    DrawEntry = LibEntry->m_Drawings;
    if( LibEntry->m_Drawings )
    {
        /* we sort the draw items, in order to have an edition more easy,
         *  when a file editing "by hand" is made */
        LibEntry->SortDrawItems();

        fprintf( ExportFile, "DRAW\n" );
        DrawEntry = LibEntry->m_Drawings;
        while( DrawEntry )
        {
            DrawEntry->Save( ExportFile );
            DrawEntry = DrawEntry->Next();
        }
        fprintf( ExportFile, "ENDDRAW\n" );
    }

    fprintf( ExportFile, "ENDDEF\n" );

    return 0;
}


/*************************************************************************/
int WriteOneDocLibEntry( FILE* ExportFile, EDA_LibComponentStruct* LibEntry )
/*************************************************************************/

/* Routine d'ecriture de la doc du composant pointe par LibEntry
 *  dans le fichier ExportFile( qui doit etre deja ouvert)
 *  return: 0 si Ok
 *          1 si err write
 *  Cependant, si i tous les Pointeurs sur textes sont nulls ( pas de Doc )
 *      rien ne sera ecrit.
 */
{
    if( ( LibEntry->m_Doc.IsEmpty() )
       && ( LibEntry->m_KeyWord.IsEmpty() )
       && ( LibEntry->m_DocFile.IsEmpty() ) )
        return 0;

    /* Generation des lignes utiles */
    fprintf( ExportFile, "#\n$CMP %s\n", CONV_TO_UTF8( LibEntry->m_Name.m_Text ) );

    if( !LibEntry->m_Doc.IsEmpty() )
        fprintf( ExportFile, "D %s\n", CONV_TO_UTF8( LibEntry->m_Doc ) );

    if( !LibEntry->m_KeyWord.IsEmpty() )
        fprintf( ExportFile, "K %s\n", CONV_TO_UTF8( LibEntry->m_KeyWord ) );

    if( !LibEntry->m_DocFile.IsEmpty() )
        fprintf( ExportFile, "F %s\n", CONV_TO_UTF8( LibEntry->m_DocFile ) );

    fprintf( ExportFile, "$ENDCMP\n" );
    return 0;
}


/*********************************************************************************/
bool LibraryStruct::SaveLibrary( const wxString& FullFileName )
/*********************************************************************************/
/**
 * Function SaveLibrary
 * writes the data structures for this object out to 2 file
 * the library in "*.lib" format.
 * the doc file in "*.dcm" format.
 * creates a backup file for each  file (.bak and .bck)
 * @param aFullFileName The full lib filename.
 * @return bool - true if success writing else false.
 */
{
    FILE* libfile, *docfile;
    EDA_LibComponentStruct* LibEntry;
    wxString libname, docname, backupname, msg;

    libname = FullFileName;

    /* the old .lib file is renamed .bak */
    if( wxFileExists( libname ) )
    {
        backupname = libname; ChangeFileNameExt( backupname, wxT( ".bak" ) );
        wxRemoveFile( backupname );
        if( !wxRenameFile( libname, backupname ) )
        {
            msg = wxT( "Failed to rename old lib file " ) + backupname;
            DisplayError( NULL, msg, 20 );
        }
    }

    docname = FullFileName; ChangeFileNameExt( docname, DOC_EXT );
    /* L'ancien fichier doc lib est renomme en .bck */
    if( wxFileExists( docname ) )
    {
        backupname = docname; ChangeFileNameExt( backupname, wxT( ".bck" ) );
        wxRemoveFile( backupname );
        if( !wxRenameFile( docname, backupname ) )
        {
            msg = wxT( "Failed to save old doc lib file " ) + backupname;
            DisplayError( NULL, msg, 20 );
        }
    }


    libfile = wxFopen( libname, wxT( "wt" ) );
    if( libfile == NULL )
     {
        msg = wxT( "Failed to create Lib File " ) + libname;
        DisplayError( NULL, msg, 20 );
        return false;
    }

    docfile = wxFopen( docname, wxT( "wt" ) );
    if( docfile == NULL )
    {
        msg = wxT( "Failed to create DocLib File " ) + docname;
        DisplayError( NULL, msg, 20 );
    }

    m_Modified = 0;

    /* Creation de l'entete de la librairie */
    m_TimeStamp = GetTimeStamp();
    WriteHeader( libfile );

    /* Sauvegarde des composant: */
    PQCompFunc( (PQCompFuncType) LibraryEntryCompare );
    LibEntry = (EDA_LibComponentStruct*) PQFirst( &m_Entries, FALSE );
    char Line[256];
    fprintf( docfile, "%s  Date: %s\n", DOCFILE_IDENT,
            DateAndTime( Line ) );

    bool success = true;
    while( LibEntry )
    {
        if (  WriteOneLibEntry( libfile, LibEntry ) != 0 )
            success = false;
        if ( docfile )
            if ( WriteOneDocLibEntry( docfile, LibEntry ) != 0 )
                success = false;

        LibEntry = (EDA_LibComponentStruct*)
                   PQNext( m_Entries, LibEntry, NULL );
    }

    fprintf( libfile, "#\n#End Library\n" );
    if ( docfile )
        fprintf( docfile, "#\n#End Doc Library\n" );
    fclose( libfile );
    fclose( docfile );
    return success;
}


/*************************************************************************************/
static bool WriteLibEntryDateAndTime( FILE* ExportFile, EDA_LibComponentStruct* LibEntry )
/*************************************************************************************/

/* lit date et time de modif composant sous le format:
 *  "Ti yy/mm/jj hh:mm:ss"
 */
{
    int year, mon, day, hour, min, sec;

    if( LibEntry->m_LastDate == 0 )
        return TRUE;

    sec  = LibEntry->m_LastDate & 63;
    min  = (LibEntry->m_LastDate >> 6) & 63;
    hour = (LibEntry->m_LastDate >> 12) & 31;
    day  = (LibEntry->m_LastDate >> 17) & 31;
    mon  = (LibEntry->m_LastDate >> 22) & 15;
    year = (LibEntry->m_LastDate >> 26) + 1990;

    fprintf( ExportFile, "Ti %d/%d/%d %d:%d:%d\n", year, mon, day, hour, min, sec );

    return TRUE;
}
