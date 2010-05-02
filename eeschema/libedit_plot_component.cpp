/*******************************************/
/*  EESchema - libedit_plot_component.cpp  */
/*******************************************/

#include "fctsys.h"

#include "gr_basic.h"
#include "common.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "eeschema_id.h"

#include "program.h"
#include "general.h"

//#include "protos.h"
#include "libeditframe.h"
#include "class_library.h"

/** function OnPlotCurrentComponent
 * plot in SVG or PNG format the curren component
 */
void WinEDA_LibeditFrame::OnPlotCurrentComponent( wxCommandEvent& event )
{
    LIB_COMPONENT* cmp = GetComponent();
    wxString   FullFileName;
    wxString   file_ext;
    wxString   mask;

    if( cmp == NULL )
    {
        wxMessageBox( _( "No component" ) );
        return;
    }

    switch( event.GetId() )
    {
    case ID_LIBEDIT_GEN_PNG_FILE:
    {
        bool       fmt_is_jpeg = false; // could be selectable later. so keep this option.

        file_ext = fmt_is_jpeg ? wxT( "jpg" ) : wxT( "png" );
        mask     = wxT( "*." ) + file_ext;
        wxFileName fn( cmp->GetName() );
        fn.SetExt( file_ext );

        FullFileName = EDA_FileSelector( _( "Filename:" ), wxGetCwd(),
                              fn.GetFullName(), file_ext, mask, this,
                              wxFD_SAVE, TRUE );

        if( FullFileName.IsEmpty() )
            return;

        // calling wxYield is mandatory under Linux, after closing the file selector dialog
        // to refresh the screen before creating the PNG or JPEG image from screen
        wxYield();
        CreatePNGorJPEGFile( FullFileName, fmt_is_jpeg );
    }
        break;

    case ID_LIBEDIT_GEN_SVG_FILE:
    {
        file_ext = wxT( "svg" );
        mask     = wxT( "*." ) + file_ext;
        wxFileName fn( cmp->GetName() );
        fn.SetExt( file_ext );
        FullFileName = EDA_FileSelector( _( "Filename:" ), wxGetCwd(),
                              fn.GetFullName(), file_ext, mask, this,
                              wxFD_SAVE, TRUE );

        if( FullFileName.IsEmpty() )
            return;

        /* Give a size to the SVG draw area = component size + margin
         * the margin is 10% the size of the component size
         */
        wxSize pagesize = GetScreen()->ReturnPageSize( );
        wxSize componentSize =
            m_component->GetBoundaryBox(m_unit, m_convert).m_Size;
        // Add a small margin to the plot bounding box
        componentSize.x = (int)(componentSize.x * 1.2);
        componentSize.y = (int)(componentSize.y * 1.2);
        GetScreen()->SetPageSize( componentSize );
        SVG_Print_Component( FullFileName );
        GetScreen()->SetPageSize( pagesize );
    }
        break;
    }
}


/** function CreatePNGorJPEGFile
 * Create an image (screenshot) of the current component.
 *  Output file format is png or jpeg
 * @param aFileName = the full filename
 * @param aFmt_jpeg = true to use JPEG file format, false to use PNG file format
 */
void WinEDA_LibeditFrame::CreatePNGorJPEGFile( const wxString& aFileName, bool aFmt_jpeg )
{
    wxSize     image_size = DrawPanel->GetClientSize();

    wxClientDC dc( DrawPanel );
    wxBitmap   bitmap( image_size.x, image_size.y );
    wxMemoryDC memdc;

    memdc.SelectObject( bitmap );
    memdc.Blit( 0, 0, image_size.x, image_size.y, &dc, 0, 0 );
    memdc.SelectObject( wxNullBitmap );

    wxImage image = bitmap.ConvertToImage();

    if( !image.SaveFile( aFileName,
                         aFmt_jpeg ? wxBITMAP_TYPE_JPEG :
                         wxBITMAP_TYPE_PNG ) )
    {
        wxString msg;
        msg.Printf( _( "Can't save file <%s>" ), GetChars( aFileName ) );
        wxMessageBox( msg );
    }

    image.Destroy();
}

/** Virtual function PrintPage
 * used to print a page.
 * @param aDC = wxDC given by the calling print function
 * @param aPrint_Sheet_Ref = true to print page references
 * @param aPrintMask = not used here
 * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
 * @param aData = a pointer on an auxiliary data (not used here)
 */
void WinEDA_LibeditFrame::PrintPage( wxDC* aDC, bool aPrint_Sheet_Ref,
                                  int aPrintMask, bool aPrintMirrorMode,
                                    void * aData)
{
    if( ! m_component )
        return;

    wxSize pagesize = GetScreen()->ReturnPageSize( );
    /* Plot item centered to the page
     * In libedit, the component is centered at 0,0 coordinates.
     * So we must plot it with an offset = pagesize/2.
     */
    wxPoint plot_offset;
    plot_offset.x = pagesize.x/2;
    plot_offset.y = pagesize.y/2;

    m_component->Draw( DrawPanel, aDC, plot_offset, m_unit,
                       m_convert, GR_DEFAULT_DRAWMODE );
}


