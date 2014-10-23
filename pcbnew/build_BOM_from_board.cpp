/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <fctsys.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <macros.h>
#include <project.h>

#include <class_board.h>
#include <class_module.h>

#include <wx/listimpl.cpp>

/* creates a BOM list rom board
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

const wxString CsvFileWildcard( _( "Comma separated value files (*.csv)|*.csv" ) );


class cmp
{
public:
    wxString m_Ref;
    wxString m_Val;
    FPID     m_fpid;
    int      m_Id;
    int      m_CmpCount;
};
WX_DECLARE_LIST( cmp, CmpList );

WX_DEFINE_LIST( CmpList )

void PCB_EDIT_FRAME::RecreateBOMFileFromBoard( wxCommandEvent& aEvent )
{
    wxFileName fn;
    FILE*      fp_bom;
    MODULE*    module = GetBoard()->m_Modules;
    wxString   msg;

    if( module == NULL )
    {
        DisplayError( this, _( "No Modules!" ) );
        return;
    }

    /* Set the file extension: */
    fn = GetBoard()->GetFileName();
    fn.SetExt( CsvFileExtension );

    wxString pro_dir = wxPathOnly( Prj().GetProjectFullName() );

    wxFileDialog dlg( this, _( "Save Bill of Materials" ), pro_dir,
                      fn.GetFullName(), CsvFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    fp_bom = wxFopen( fn.GetFullPath(), wxT( "wt" ) );

    if( fp_bom == NULL )
    {
        msg.Printf( _( "Unable to create file <%s>" ), GetChars( fn.GetFullPath() ) );
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
    CmpList           list;
    cmp*              comp = NULL;
    CmpList::iterator iter;
    int               i = 1;

    while( module != NULL )
    {
        bool valExist = false;

        // try to find component in existing list
        for( iter = list.begin(); iter != list.end(); iter++ )
        {
            cmp* current = *iter;

            if( (current->m_Val == module->GetValue()) && (current->m_fpid == module->GetFPID()) )
            {
                current->m_Ref.Append( wxT( ", " ), 1 );
                current->m_Ref.Append( module->GetReference() );
                current->m_CmpCount++;

                valExist = true;
                break;
            }
        }

        // If component does not exist yet, create new one and append it to the list.
        if( valExist == false )
        {
            comp = new cmp();
            comp->m_Id  = i++;
            comp->m_Val = module->GetValue();
            comp->m_Ref = module->GetReference();
            comp->m_fpid = module->GetFPID();
            comp->m_CmpCount = 1;
            list.Append( comp );
        }

        // increment module
        module = module->Next();
    }

    // Print list
    for( iter = list.begin(); iter != list.end(); iter++ )
    {
        cmp* current = *iter;

        msg.Empty();

        msg << current->m_Id << wxT( ";\"" );
        msg << current->m_Ref << wxT( "\";\"" );
        msg << FROM_UTF8( current->m_fpid.Format().c_str() ) << wxT( "\";" );
        msg << current->m_CmpCount << wxT( ";\"" );
        msg << current->m_Val << wxT( "\";;;\n" );
        fprintf( fp_bom, "%s", TO_UTF8( msg ) );

        list.DeleteObject( current );
        delete (current);
    }

    fclose( fp_bom );
}
