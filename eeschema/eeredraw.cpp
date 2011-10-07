/**
 * @file eeredraw.cpp
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "appl_wxstruct.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "sch_bus_entry.h"
#include "sch_component.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_polyline.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"

#include "build_version.h"


void DrawDanglingSymbol( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& pos, int Color )
{
    BASE_SCREEN* screen = panel->GetScreen();

    if( !screen->m_IsPrinting ) /* Draw but do not print the Dangling Symbol */
    {
        GRRect( &panel->m_ClipBox, DC,
                pos.x - DANGLING_SYMBOL_SIZE, pos.y - DANGLING_SYMBOL_SIZE,
                pos.x + DANGLING_SYMBOL_SIZE, pos.y + DANGLING_SYMBOL_SIZE,
                0, Color );
    }
}


/*
 * Redraws only the active window which is assumed to be whole visible.
 */
void SCH_EDIT_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    wxString title;

    if( GetScreen() == NULL )
        return;

    DrawPanel->DrawBackGround( DC );

    GetScreen()->Draw( DrawPanel, DC, GR_DEFAULT_DRAWMODE );

    TraceWorkSheet( DC, GetScreen(), g_DrawDefaultLineThickness );

    if( DrawPanel->IsMouseCaptured() )
        DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, FALSE );

    DrawPanel->DrawCrossHair( DC );

    // Display the sheet filename, and the sheet path, for non root sheets
    if( GetScreen()->GetFileName() == m_DefaultSchematicFileName )
    {
        wxString msg = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();
        title.Printf( wxT( "%s [%s]" ), GetChars( msg), GetChars( GetScreen()->GetFileName() ) );
        SetTitle( title );
    }
    else
    {
#if 0
        title = wxT( "[" );
        title << GetScreen()->GetFileName() << wxT( "]  " ) << _( "Sheet" );
        title << wxT( " " ) << m_CurrentSheet->PathHumanReadable();

#else
        // Window title format:
        // [filename sheetpath] (/path/to/filedir)

        wxFileName t( GetScreen()->GetFileName() );

        // Often the /path/to/filedir is blank because of the FullFileName argument
        // passed to LoadOneEEFile() which omits the path on non-root schematics.
        // Making the path absolute solves this problem.
        t.MakeAbsolute();
        title = wxChar( '[' );
        title << t.GetName() << wxChar( ' ' );
        title << m_CurrentSheet->PathHumanReadable() << wxChar( ']' );

        title << wxChar( ' ' );
        title << wxChar( '(' ) << t.GetPath() << wxChar( ')' );

        if( !t.IsFileWritable() )
            title << _( " [Read Only]" );
#endif

        SetTitle( title );
    }
}
