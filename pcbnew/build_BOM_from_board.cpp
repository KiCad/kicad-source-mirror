/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/* build_BOM_from_board.cpp */


#include <confirm.h>
#include <macros.h>
#include <string_utils.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <project.h>
#include <wildcards_and_files_ext.h>
#include <footprint.h>
#include <tools/board_editor_control.h>
#include <wx/filedlg.h>
#include <vector>


/* creates a BOM list from board
 *  The format is:
 *  "Id";"Designator";"Footprint";"Number";"Designation";"Supplier and ref";
 *  1;"P1";"DB25FC";1;"DB25FEMELLE";;;
 *  2;"U9";"PGA120";1;"4003APG120";;;
 *  3;"JP1";"pin_array_8x2";1;"CONN_8X2";;;
 *  4;"RR1";"r_pack9";1;"9x1K";;;
 *  5;"X1";"HC-18UH";1;"8MHz";;;
 *  6;"U8";"24dip300";1;"EP600";;;
 *  7;"U5";"32dip600";1;"628128";;;
 *  8;"C2,C3";"C1";2;"47pF";;;
 *  9;"U1";"20dip300";1;"74LS245";;;
 *  10;"U3";"20dip300";1;"74LS541";;;
 *  11;"U2";"20dip300";1;"74LS688";;;
 *  12;"C1,C4,C5,C6";"CP6";4;"47uF";;;
 */

class BOM_ENTRY
{
public:
    std::vector<wxString> m_Refs;
    wxString              m_Val;
    LIB_ID                m_FPID;
    int                   m_Count;
};


int BOARD_EDITOR_CONTROL::GenBOMFileFromBoard( const TOOL_EVENT& aEvent )
{
    BOARD*     board = m_frame->GetBoard();
    wxFileName fn;
    FILE*      fp_bom;

    if( board->Footprints().empty() )
    {
        m_frame->ShowInfoBarError( _( "Cannot export BOM: there are no footprints on the PCB." ) );
        return 0;
    }

    /* Set the file extension: */
    fn = board->GetFileName();
    fn.SetExt( FILEEXT::CsvFileExtension );

    wxString pro_dir = wxPathOnly( m_frame->Prj().GetProjectFullName() );

    wxFileDialog dlg( m_frame, _( "Save Bill of Materials" ), pro_dir, fn.GetFullName(),
                      FILEEXT::CsvFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return 0;

    fn = dlg.GetPath();

    fp_bom = wxFopen( fn.GetFullPath(), wxT( "wt" ) );

    if( fp_bom == nullptr )
    {
        DisplayError( m_frame, wxString::Format( _( "Failed to create file '%s'." ), fn.GetFullPath() ) );
        return 0;
    }

    // Write header:
    wxString msg = wxT( "\"" );
    msg << _( "Id" ) << wxT( "\";\"" );
    msg << _( "Designator" ) << wxT( "\";\"" );
    msg << _( "Footprint" ) << wxT( "\";\"" );
    msg << _( "Quantity" ) << wxT( "\";\"" );
    msg << _( "Designation" ) << wxT( "\";\"" );
    msg << _( "Supplier and ref" ) << wxT( "\";\n" );
    fprintf( fp_bom, "%s", TO_UTF8( msg ) );

    // Build list
    std::vector<BOM_ENTRY> list;

    for( FOOTPRINT* footprint : board->Footprints() )
    {
        if( footprint->GetAttributes() & FP_EXCLUDE_FROM_BOM )
            continue;

        bool valExist = false;

        // try to find component in existing list
        for( BOM_ENTRY& curEntry : list )
        {
            if( curEntry.m_Val == footprint->GetValue() && curEntry.m_FPID == footprint->GetFPID() )
            {
                curEntry.m_Refs.emplace_back( footprint->Reference().GetShownText( false ) );
                curEntry.m_Count++;

                valExist = true;
                break;
            }
        }

        // If component does not exist yet, create new one and append it to the list.
        if( !valExist )
        {
            list.emplace_back();
            BOM_ENTRY& newEntry = list.back();
            newEntry.m_Val = footprint->Value().GetShownText( false );
            newEntry.m_Refs.emplace_back( footprint->Reference().GetShownText( false ) );
            newEntry.m_FPID = footprint->GetFPID();
            newEntry.m_Count = 1;
        }
    }

    for( BOM_ENTRY& curEntry : list )
    {
        std::sort( curEntry.m_Refs.begin(), curEntry.m_Refs.end(),
                []( const wxString& lhs, const wxString& rhs )
                {
                    return StrNumCmp( lhs, rhs, true /* ignore case */ ) < 0;
                } );
    }

    std::sort( list.begin(), list.end(),
            []( const BOM_ENTRY& lhs, const BOM_ENTRY& rhs )
            {
                return StrNumCmp( lhs.m_Refs[0], rhs.m_Refs[0], true /* ignore case */ ) < 0;
            } );

    // Print list.
    int id = 1;

    for( const BOM_ENTRY& curEntry : list )
    {
        msg.Empty();

        msg << id++ << wxT( ";\"" );

        msg << curEntry.m_Refs[0];

        for( int ii = 1; ii < (int) curEntry.m_Refs.size(); ++ii )
            msg << wxT( ", " ) << curEntry.m_Refs[ii];

        msg << wxT( "\";\"" );

        msg << From_UTF8( curEntry.m_FPID.GetLibItemName().c_str() ) << wxT( "\";" );
        msg << curEntry.m_Count << wxT( ";\"" );
        msg << curEntry.m_Val << wxT( "\";;;\n" );
        fprintf( fp_bom, "%s", TO_UTF8( msg ) );
    }

    fclose( fp_bom );

    return 0;
}
