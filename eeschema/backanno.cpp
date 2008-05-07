/****************************************************************
* EESchema: backanno.cpp
*  (functions for backannotating Footprint info
****************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

/* Variables Locales */

#include "dialog_backanno.cpp"

#include "protos.h"

/**************************************************************/
bool WinEDA_SchematicFrame::ProcessStuffFile( FILE* StuffFile )
/**************************************************************/
{
    int             LineNum = 0;
    char*           cp, Ref[256], FootPrint[256], Line[1024];
    SCH_ITEM*       DrawList = NULL;
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

            // printf("'%s' '%s'\n", Ref, FootPrint);

            wxString    reference = CONV_FROM_UTF8( Ref );

            DrawList = WinEDA_SchematicFrame::FindComponentAndItem(
                reference, TRUE, 2, wxEmptyString, false );

            if( DrawList == NULL )
                continue;

            if( DrawList->Type() == TYPE_SCH_COMPONENT )
            {
                Cmp = (SCH_COMPONENT*) DrawList;
                TextField = &Cmp->m_Field[FOOTPRINT];
                TextField->m_Text = CONV_FROM_UTF8( FootPrint );
            }
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
