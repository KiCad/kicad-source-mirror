#include <dxf2brd_items.h>
#include <wxPcbStruct.h>
#include <convert_from_iu.h>

bool InvokeDXFDialogImport( PCB_EDIT_FRAME* aCaller )
{
    wxFileDialog dlg( aCaller,
                      wxT( "Open File" ),
                      wxEmptyString, wxEmptyString,
                      wxT( "dxf Files (*.dxf)|*.dxf|*.DXF" ),
                      wxFD_OPEN|wxFD_FILE_MUST_EXIST );
    dlg.ShowModal();

    wxString fileName = dlg.GetPath();

    if( !fileName.IsEmpty() )
    {
        BOARD * brd = aCaller->GetBoard();
        DXF2BRD_CONVERTER dxf_importer;

        // Set coordinates offset for import (offset is given in mm)
        double offsetY = - aCaller->GetPageSizeIU().y * MM_PER_IU;
        dxf_importer.SetOffset( 0.0, offsetY );
        dxf_importer.SetBrdLayer( DRAW_N );
        dxf_importer.ImportDxfFile( fileName, brd );
    }

    return true;
}
