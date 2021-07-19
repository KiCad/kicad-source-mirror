/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kicad_string.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <wildcards_and_files_ext.h>
#include <footprint.h>
#include <wx/listimpl.cpp>
#include <wx/filedlg.h>


/* creates a BOM list from board
 *  The format is:
 *  "Id";"Designator";"Package";"Number";"Designation";"Supplier and ref";
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

const wxString CsvFileExtension( wxT( "csv" ) );    // BOM file extension

class BOM_ENTRY
{
public:
    wxString m_Ref;
    wxString m_Val;
    LIB_ID   m_FPID;
    int      m_Id;
    int      m_Count;
};

WX_DECLARE_LIST( BOM_ENTRY, BOM_ENTRY_LIST );

WX_DEFINE_LIST( BOM_ENTRY_LIST )


void PCB_EDIT_FRAME::RecreateBOMFileFromBoard( wxCommandEvent& aEvent )
{
    wxFileName fn;
    FILE*      fp_bom;
    wxString   msg;

    if( GetBoard()->Footprints().empty() )
    {
        ShowInfoBarError( _( "Cannot export BOM: there are no footprints on the PCB." ) );
        return;
    }

    /* Set the file extension: */
    fn = GetBoard()->GetFileName();
    fn.SetExt( CsvFileExtension );

    wxString pro_dir = wxPathOnly( Prj().GetProjectFullName() );

    wxFileDialog dlg( this, _( "Save Bill of Materials" ), pro_dir, fn.GetFullName(),
                      CsvFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    fp_bom = wxFopen( fn.GetFullPath(), wxT( "wt" ) );

    if( fp_bom == nullptr )
    {
        msg.Printf( _( "Failed to create file '%s'." ), fn.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    // Write header:
    msg = wxT( "\"" );
    msg << _( "Id" ) << wxT( "\";\"" );
    msg << _( "Designator" ) << wxT( "\";\"" );
    msg << _( "Package" ) << wxT( "\";\"" );
    msg << _( "Quantity" ) << wxT( "\";\"" );
    msg << _( "Designation" ) << wxT( "\";\"" );
    msg << _( "Supplier and ref" ) << wxT( "\";\n" );
    fprintf( fp_bom, "%s", TO_UTF8( msg ) );

    // Build list
    BOM_ENTRY_LIST list;
    int            i = 1;

    for( FOOTPRINT* footprint : GetBoard()->Footprints() )
    {
        bool valExist = false;

        // try to find component in existing list
        for( auto iter = list.begin(); iter != list.end(); ++iter )
        {
            BOM_ENTRY* curEntry = *iter;

            if( curEntry->m_Val == footprint->GetValue()
                    && curEntry->m_FPID == footprint->GetFPID() )
            {
                curEntry->m_Ref.Append( wxT( ", " ), 1 );
                curEntry->m_Ref.Append( footprint->Reference().GetShownText() );
                curEntry->m_Count++;

                valExist = true;
                break;
            }
        }

        // If component does not exist yet, create new one and append it to the list.
        if( valExist == false )
        {
            BOM_ENTRY* newEntry = new BOM_ENTRY();
            newEntry->m_Id  = i++;
            newEntry->m_Val = footprint->Value().GetShownText();
            newEntry->m_Ref = footprint->Reference().GetShownText();
            newEntry->m_FPID = footprint->GetFPID();
            newEntry->m_Count = 1;
            list.Append( newEntry );
        }
    }

    // Print list. Also delete temporary created objects.
    for( size_t ii = list.GetCount(); ii > 0; ii-- )
    {
        BOM_ENTRY* curEntry = *list.begin();   // Because the first object will be removed
                                               // from list, all objects will be get here

        msg.Empty();

        msg << curEntry->m_Id << wxT( ";\"" );
        msg << curEntry->m_Ref << wxT( "\";\"" );
        msg << FROM_UTF8( curEntry->m_FPID.GetLibItemName().c_str() ) << wxT( "\";" );
        msg << curEntry->m_Count << wxT( ";\"" );
        msg << curEntry->m_Val << wxT( "\";;;\n" );
        fprintf( fp_bom, "%s", TO_UTF8( msg ) );

        // We do not need this object, now: remove it from list and delete it
        list.DeleteObject( curEntry );
        delete curEntry;
    }

    fclose( fp_bom );
}
