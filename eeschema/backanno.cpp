/****************************************************************
* EESchema: backanno.cpp
*  (functions for backannotating Footprint info
****************************************************************/

#include "fctsys.h"
#include "common.h"
#include "program.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "general.h"


bool WinEDA_SchematicFrame::FillFootprintFieldForAllInstancesofComponent(
    const wxString& aReference,
    const wxString& aFootPrint,
    bool            aSetVisible )
{
/** function FillFootprintFieldForAllInstancesofComponent
 * Search for component "aReference", and place a Footprint in Footprint field
 * @param aReference = reference of the component to initialize
 * @param aFootPrint = new value for the filed Fottprint component
 * @param aSetVisible = true to have the field visible, false to set the
 * invisible flag
 * @return true if the given component is found
 * Note:
 * the component is searched in the whole schematic, and because some
 * components
 * have more than one instance (multiple parts per package components)
 * the search is not stopped when a reference is found (all instances must be
 * found).
 */
    DrawSheetPath* sheet;
    SCH_ITEM*      DrawList = NULL;
    EDA_SheetList  SheetList;
    SCH_COMPONENT* Cmp;
    bool           found = false;

    for( sheet = SheetList.GetFirst();
        sheet != NULL;
        sheet = SheetList.GetNext() )
    {
        DrawList = (SCH_ITEM*) sheet->LastDrawList();
        for( ; (DrawList != NULL); DrawList = DrawList->Next() )
        {
            if( DrawList->Type() != TYPE_SCH_COMPONENT )
                continue;

            Cmp = (SCH_COMPONENT*) DrawList;
            if( aReference.CmpNoCase( Cmp->GetRef( sheet ) ) == 0 )
            {
                // Found: Init Footprint Field

                /* Give a reasonable value to the field position and
                 * orientation, if the text is empty at position 0, because
                 * it is probably not yet initialized
                 */
                if( Cmp->GetField( FOOTPRINT )->m_Text.IsEmpty()
                   && ( Cmp->GetField( FOOTPRINT )->m_Pos == wxPoint( 0, 0 ) ) )
                {
                    Cmp->GetField( FOOTPRINT )->m_Orient = Cmp->GetField(
                        VALUE )->m_Orient;
                    Cmp->GetField( FOOTPRINT )->m_Pos    = Cmp->GetField(
                        VALUE )->m_Pos;
                    Cmp->GetField( FOOTPRINT )->m_Pos.y -= 100;
                }
                Cmp->GetField( FOOTPRINT )->m_Text = aFootPrint;
                if( aSetVisible )
                    Cmp->GetField( FOOTPRINT )->m_Attributs &=
                        ~TEXT_NO_VISIBLE;
                else
                    Cmp->GetField( FOOTPRINT )->m_Attributs |= TEXT_NO_VISIBLE;
                found = true;
            }
        }
    }

    return found;
}


bool WinEDA_SchematicFrame::ProcessStuffFile( FILE* aStuffFile, bool
                                              aSetFielsAttributeToVisible  )
{
/** Function ProcessStuffFile
 * Read a "stuff" file created by cvpcb.
 * That file has lines like:
 * comp = "C1" module = "CP6"
 * comp = "C2" module = "C1"
 * comp = "C3" module = "C1"
 * "comp =" gives the component reference
 * "module =" gives the footprint name
 *
 * @param aStuffFile = file (*.stf) to Read.
 * @param aSetFielsAttributeToVisible = true to set the footprint field flag to
 * visible
 * @return true if OK.
 */
    int   LineNum = 0;
    char* cp, Ref[256], FootPrint[256], Line[1024];

    while( GetLine( aStuffFile, Line, &LineNum, sizeof(Line) ) )
    {
        if( sscanf( Line, "comp = \"%s module = \"%s", Ref, FootPrint ) == 2 )
        {
            for( cp = Ref; *cp; cp++ )
                if( *cp == '"' )
                    *cp = 0;

            for( cp = FootPrint; *cp; cp++ )
                if( *cp == '"' )
                    *cp = 0;

            wxString reference = CONV_FROM_UTF8( Ref );
            wxString Footprint = CONV_FROM_UTF8( FootPrint );
            FillFootprintFieldForAllInstancesofComponent(
                reference,
                Footprint,
                aSetFielsAttributeToVisible );
        }
    }

    return true;
}


bool WinEDA_SchematicFrame::ReadInputStuffFile()
{
/* Backann footprint info to schematic.
 */
    wxString Line, filename;
    FILE*    StuffFile;
    wxString msg;
    bool     SetFieldToVisible = true;

    filename = EDA_FileSelector( _( "Load Stuff File" ),
                                 wxEmptyString,
                                 wxEmptyString,
                                 wxT( ".stf" ),
                                 wxT( "*.stf" ),
                                 this,
                                 wxFD_OPEN,
                                 FALSE
                                 );

    if( filename.IsEmpty() )
        return FALSE;

    Line  = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();
    Line += wxT( " " ) + filename;
    SetTitle( Line );

    if( filename.IsEmpty() )
        return FALSE;

    int diag = wxMessageBox(
        _( "Set the Footprint Field to Visible ?" ),
        _( "Field Display Option" ),
        wxYES_NO | wxICON_QUESTION | wxCANCEL, this );

    if( diag == wxCANCEL )
        return false;
    if( diag == wxYES )
        SetFieldToVisible = true;
    else
        SetFieldToVisible = false;

    StuffFile = wxFopen( filename, wxT( "rt" ) );
    if( StuffFile == NULL )
    {
        msg.Printf( _( "Failed to open Stuff File <%s>" ), filename.GetData() );
        DisplayError( this, msg, 20 );
        return FALSE;
    }

    ProcessStuffFile( StuffFile, SetFieldToVisible );

    return TRUE;
}
