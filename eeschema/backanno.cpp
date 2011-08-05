/****************************************************************
 * EESchema: backanno.cpp
 *  (functions for backannotating Footprint info
 ****************************************************************/

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "appl_wxstruct.h"
#include "wxEeschemaStruct.h"
#include "build_version.h"

#include "general.h"
#include "sch_sheet_path.h"
#include "sch_component.h"
#include "netlist.h"


const wxString BackAnnotateFileWildcard( wxT( "EESchema Back Annotation File (*.stf)|*.stf" ) );


bool SCH_EDIT_FRAME::ProcessStuffFile( FILE* aFile, bool aSetFieldAttributeToVisible  )
{
    int   LineNum = 0;
    char* cp, Ref[256], FootPrint[256], Line[1024];
    SCH_SHEET_LIST SheetList;
    wxString reference;
    wxString footprint;

    // Build a flat list of components in schematic:
    SCH_REFERENCE_LIST referencesList;
    SheetList.GetComponents( referencesList, false );

    while( GetLine( aFile, Line, &LineNum, sizeof(Line) ) )
    {
        if( sscanf( Line, "comp = \"%s module = \"%s", Ref, FootPrint ) == 2 )
        {
            for( cp = Ref; *cp; cp++ )
                if( *cp == '"' )
                    *cp = 0;

            for( cp = FootPrint; *cp; cp++ )
                if( *cp == '"' )
                    *cp = 0;

            reference = FROM_UTF8( Ref );
            footprint = FROM_UTF8( FootPrint );

            // Search the component in the flat list
            for( unsigned ii = 0; ii < referencesList.GetCount(); ii++ )
            {
                if( reference.CmpNoCase( referencesList[ii].GetRef() ) == 0 )
                {
                    // We have found a candidate.
                    // Note: it can be not unique (multiple parts per package)
                    // So we do not stop the search here
                    SCH_COMPONENT* component = referencesList[ii].GetComponent();
                    SCH_FIELD * fpfield = component->GetField( FOOTPRINT );
                    if( fpfield->m_Text.IsEmpty()
                        && ( fpfield->m_Pos == wxPoint( 0, 0 ) ) )
                    {
                        fpfield->m_Orient = component->GetField( VALUE )->m_Orient;
                        fpfield->m_Pos    = component->GetField( VALUE )->m_Pos;
                        fpfield->m_Pos.y -= 100;
                    }

                    fpfield->m_Text = footprint;

                    if( aSetFieldAttributeToVisible )
                        component->GetField( FOOTPRINT )->m_Attributs &= ~TEXT_NO_VISIBLE;
                    else
                        component->GetField( FOOTPRINT )->m_Attributs |= TEXT_NO_VISIBLE;
                }
            }
        }
    }

    fclose( aFile );
    return true;
}


bool SCH_EDIT_FRAME::ReadInputStuffFile()
{
    wxString title, filename;
    FILE*    file;
    wxString msg;
    bool     visible = false;

    wxFileDialog dlg( this, _( "Load Back Annotate File" ), wxEmptyString, wxEmptyString,
                      BackAnnotateFileWildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    filename = dlg.GetPath();
    title  = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();
    title += wxT( " " ) + filename;
    SetTitle( title );

    int response = wxMessageBox( _( "Do you want to make all the foot print fields visible?" ),
                                 _( "Field Display Option" ),
                                 wxYES_NO | wxICON_QUESTION | wxCANCEL, this );

    if( response == wxCANCEL )
        return false;

    if( response == wxYES )
        visible = true;

    file = wxFopen( filename, wxT( "rt" ) );

    if( file == NULL )
    {
        msg.Printf( _( "Failed to open back annotate file <%s>" ), filename.GetData() );
        DisplayError( this, msg );
        return false;
    }

    ProcessStuffFile( file, visible );

    return true;
}
