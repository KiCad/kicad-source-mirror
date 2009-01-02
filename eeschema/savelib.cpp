/****************************/
/*	EESchema - eesavlib.cpp	*/
/****************************/

/* Functions to save schematic libraries and library components (::Save() members)
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
	// Spaces are not allowed in text because it is not double quoted: changed to '~'
    text.Replace( wxT( " " ), wxT( "~" ) );

    fprintf( ExportFile, "T %d %d %d %d %d %d %d %s ",
            m_Orient,
            m_Pos.x, m_Pos.y,
            m_Size.x, m_Attributs,
            m_Unit, m_Convert,
            CONV_TO_UTF8( text ));

	fprintf( ExportFile, " %s %d",	m_Italic ? "Italic" : "Normal", m_Width );

	fprintf( ExportFile, "\n");
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
    int ccount = GetCornerCount();
    fprintf( ExportFile, "P %d %d %d %d",
             ccount,
             m_Unit, m_Convert,
             m_Width );
    for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
    {
        fprintf( ExportFile, "  %d %d", m_PolyPoints[ii].x, m_PolyPoints[ii].y );
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


/**********************************************************/
LibEDA_BaseStruct* CopyDrawEntryStruct( wxWindow*          frame,
                                        LibEDA_BaseStruct* DrawItem )
/**********************************************************/

/** Function CopyDrawEntryStruct
 * Duplicate a DrawLibItem
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
    for( OldField = OldEntry->m_Fields; OldField != NULL; OldField = OldField->Next() )
    {
        NewField = OldField->GenCopy();
        NewStruct->m_Fields.PushBack( NewField );
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
                LastItem->SetNext( NewDrawings );

            LastItem = NewDrawings;
            NewDrawings->SetNext( NULL );
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

/************************************************/
bool EDA_LibComponentStruct::Save( FILE* aFile )
/***********************************************/
/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
#define UNUSED 0
{
    LibEDA_BaseStruct* DrawEntry;
    LibDrawField*      Field;

    if( Type != ROOT )  // should not happen, but just in case
        return false;

    /* First line: it s a comment (component name for readers) */
    fprintf( aFile, "#\n# %s\n#\n", CONV_TO_UTF8( m_Name.m_Text ) );

    /* Save data */
    fprintf( aFile, "DEF" );
    if( (m_Name.m_Attributs & TEXT_NO_VISIBLE) == 0 )
        fprintf( aFile, " %s", CONV_TO_UTF8( m_Name.m_Text ) );
    else
        fprintf( aFile, " ~%s", CONV_TO_UTF8( m_Name.m_Text ) );

    if( !m_Prefix.m_Text.IsEmpty() )
        fprintf( aFile, " %s", CONV_TO_UTF8( m_Prefix.m_Text ) );
    else
        fprintf( aFile, " ~" );
    fprintf( aFile, " %d %d %c %c %d %c %c\n",
             UNUSED, m_TextInside,
             m_DrawPinNum ? 'Y' : 'N',
             m_DrawPinName ? 'Y' : 'N',
             m_UnitCount, m_UnitSelectionLocked ? 'L' : 'F',
             m_Options == ENTRY_POWER ? 'P' : 'N' );

    WriteLibEntryDateAndTime( aFile, this );

    /* Save fields */
    m_Prefix.Save( aFile );
    m_Name.Save( aFile );

    for( Field = m_Fields; Field != NULL; Field = Field->Next() )
    {
        if( Field->m_Text.IsEmpty() && Field->m_Name.IsEmpty() )
            continue;
        Field->Save( aFile );
    }

    /* Save the alias list: a line starting by "ALIAS" */
    if( m_AliasList.GetCount() != 0 )
    {
        fprintf( aFile, "ALIAS" );
        unsigned ii;
        for( ii = 0; ii < m_AliasList.GetCount(); ii++ )
            fprintf( aFile, " %s", CONV_TO_UTF8( m_AliasList[ii] ) );

        fprintf( aFile, "\n" );
    }

    /* Write the footprint filter list */
    if( m_FootprintList.GetCount() != 0 )
    {
        fprintf( aFile, "$FPLIST\n" );
        unsigned ii;
        for( ii = 0; ii < m_FootprintList.GetCount(); ii++ )
            fprintf( aFile, " %s\n", CONV_TO_UTF8( m_FootprintList[ii] ) );

        fprintf( aFile, "$ENDFPLIST\n" );
    }

    /* Save graphics items (including pins) */
    if( m_Drawings )
    {
        /* we sort the draw items, in order to have an edition more easy,
         *  when a file editing "by hand" is made */
        SortDrawItems();

        fprintf( aFile, "DRAW\n" );
        DrawEntry = m_Drawings;
        while( DrawEntry )
        {
            DrawEntry->Save( aFile );
            DrawEntry = DrawEntry->Next();
        }
        fprintf( aFile, "ENDDRAW\n" );
    }

    fprintf( aFile, "ENDDEF\n" );

    return true;
}


/***************************************/
bool LibCmpEntry::SaveDoc( FILE* aFile )
/***************************************/
/**
 * Function SaveDoc
 * writes the doc info out to a FILE in "*.dcm" format.
 * Only non empty fields are written.
 * If all fielsd are empty, does not write anything
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
{
    if( m_Doc.IsEmpty() && m_KeyWord.IsEmpty() && m_DocFile.IsEmpty() )
        return true;

    /* Generation des lignes utiles */
    fprintf( aFile, "#\n$CMP %s\n", CONV_TO_UTF8( m_Name.m_Text ) );

    if( ! m_Doc.IsEmpty() )
        fprintf( aFile, "D %s\n", CONV_TO_UTF8( m_Doc ) );

    if( ! m_KeyWord.IsEmpty() )
        fprintf( aFile, "K %s\n", CONV_TO_UTF8( m_KeyWord ) );

    if( ! m_DocFile.IsEmpty() )
        fprintf( aFile, "F %s\n", CONV_TO_UTF8( m_DocFile ) );

    fprintf( aFile, "$ENDCMP\n" );
    return true;
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
        if ( LibEntry->Type == ROOT )
        {
            if ( ! LibEntry->Save( libfile ) )
                success = false;
        }
        if ( docfile )
        {
            if ( ! LibEntry->SaveDoc( docfile ) )
                success = false;
        }

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
