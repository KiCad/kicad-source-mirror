/****************************************************************
* EESchema: backanno.cpp
*  (functions for backannotating Footprint info
****************************************************************/

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

/* Variables Locales */

#include "dialog_backanno.cpp"

/**************************************************************/
SCH_COMPONENT * WinEDA_SchematicFrame::FindComponentByRef(
     const wxString& reference )
/**************************************************************/
{
    DrawSheetPath*      sheet;
    SCH_ITEM*           DrawList  = NULL;
    EDA_SheetList       SheetList( NULL );

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
	DrawList = (SCH_ITEM*) sheet->LastDrawList();
	for( ; (DrawList != NULL); DrawList = DrawList->Next() )
	{
	    if( DrawList->Type() == TYPE_SCH_COMPONENT )
	    {
		SCH_COMPONENT* pSch;

		pSch = (SCH_COMPONENT*) DrawList;
		if( reference.CmpNoCase( pSch->GetRef(sheet) ) == 0 )
		    return pSch;
	    }
	}
    }
    return NULL;
}

/**************************************************************/
bool WinEDA_SchematicFrame::ProcessStuffFile( FILE* StuffFile )
/**************************************************************/
{
    int             LineNum = 0;
    char*           cp, Ref[256], FootPrint[256], Line[1024];
    SCH_COMPONENT*  Cmp;
    PartTextStruct* TextField;

    while( GetLine( StuffFile, Line, &LineNum, sizeof(Line) ) )
    {
        if( sscanf( Line, "comp = \"%s module = \"%s", Ref, FootPrint ) == 2 )
        {
            for( cp = Ref; *cp; cp++ )
                if( *cp == '"' )
                    *cp = 0;

            for( cp = FootPrint; *cp; cp++ )
                if( *cp == '"' )
                    *cp = 0;

            wxString    reference = CONV_FROM_UTF8( Ref );

            Cmp = WinEDA_SchematicFrame::FindComponentByRef( reference );
            if( Cmp == NULL )
                continue;

#if defined(DEBUG)
            printf( "  %s %s\n", CONV_TO_UTF8(Cmp->m_Field[REFERENCE].m_Text),
		 		     CONV_TO_UTF8(Cmp->m_Field[VALUE].m_Text) );
#endif
            TextField = &Cmp->m_Field[FOOTPRINT];
            TextField->m_Text = CONV_FROM_UTF8( FootPrint );
        }
    }

    return true;
}


/**************************************************************/
bool WinEDA_SchematicFrame::ReadInputStuffFile()
/**************************************************************/

/* Backann footprint info to schematic.
 */
{
    wxString Line, filename;
    FILE*    StuffFile;
    wxString msg;

    filename = EDA_FileSelector( _( "Load Stuff File" ),
        wxEmptyString,                                      /* Chemin par defaut */
        wxEmptyString,                                      /* nom fichier par defaut */
        wxT( ".stf" ),                                      /* extension par defaut */
        wxT( "*.stf" ),                                     /* Masque d'affichage */
        this,
        wxFD_OPEN,
        FALSE
        );

    if( filename.IsEmpty() )
        return FALSE;

    Line  = g_Main_Title + wxT( " " ) + GetBuildVersion();
    Line += wxT( " " ) + filename;
    SetTitle( Line );

    if( filename.IsEmpty() )
        return FALSE;

    StuffFile = wxFopen( filename, wxT( "rt" ) );
    if( StuffFile == NULL )
    {
        msg.Printf( _( "Failed to open Stuff File <%s>" ), filename.GetData() );
        DisplayError( this, msg, 20 );
        return FALSE;
    }

    ProcessStuffFile( StuffFile );

    return TRUE;
}
