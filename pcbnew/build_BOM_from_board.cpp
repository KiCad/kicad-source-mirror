/* build_BOM_from_board.cpp */


#include "fctsys.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

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
    wxString m_Pkg;
    int      m_Id;
    int      m_CmpCount;
};
WX_DECLARE_LIST( cmp, CmpList );

WX_DEFINE_LIST( CmpList )

void PCB_EDIT_FRAME::RecreateBOMFileFromBoard( wxCommandEvent& aEvent )
{
    wxFileName fn;
    FILE*      FichBom;
    MODULE*    Module = GetBoard()->m_Modules;
    wxString   msg;


    if( Module == NULL )
    {
        DisplayError( this, _( "No Modules!" ) );
        return;
    }

    /* Set the file extension: */
    fn = GetScreen()->GetFileName();
    fn.SetExt( CsvFileExtension );

    wxFileDialog dlg( this, _( "Save Bill of Materials" ), wxGetCwd(),
                      fn.GetFullName(), CsvFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    FichBom = wxFopen( fn.GetFullPath(), wxT( "wt" ) );

    if( FichBom == NULL )
    {
        msg = _( "Unable to create file " ) + fn.GetFullPath();
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
    fprintf( FichBom, "%s", TO_UTF8( msg ) );

    // Build list
    CmpList           list;
    cmp*              comp = NULL;
    CmpList::iterator iter;
    int i = 1;
    while( Module != NULL )
    {
        bool valExist = false;

        // try to find component in existing list
        for( iter = list.begin(); iter != list.end(); iter++ )
        {
            cmp* current = *iter;
            if( (current->m_Val == Module->m_Value->m_Text) && (current->m_Pkg == Module->m_LibRef) )
            {
                current->m_Ref.Append( wxT( ", " ), 1 );
                current->m_Ref.Append( Module->m_Reference->m_Text );
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
            comp->m_Val = Module->m_Value->m_Text;
            comp->m_Ref = Module->m_Reference->m_Text;
            comp->m_Pkg = Module->m_LibRef;
            comp->m_CmpCount = 1;
            list.Append( comp );
        }

        // increment module
        Module = Module->Next();
    }

    // Print list
    for( iter = list.begin(); iter != list.end(); iter++ )
    {
        cmp* current = *iter;

        msg.Empty();

        msg << current->m_Id << wxT( ";\"" );
        msg << current->m_Ref << wxT( "\";\"" );
        msg << current->m_Pkg << wxT( "\";" );
        msg << current->m_CmpCount << wxT( ";\"" );
        msg << current->m_Val << wxT( "\";;;\n" );
        fprintf( FichBom, "%s", TO_UTF8( msg ) );

        list.DeleteObject( current );
        delete (current);
    }


    fclose( FichBom );
}
