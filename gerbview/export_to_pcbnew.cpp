/* export_to_pcbnew.cpp */

/*
 *  Export the layers to pcbnew
 */

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"

#include "gerbview.h"
#include "class_board_design_settings.h"
#include "class_gerber_draw_item.h"

static int SavePcbFormatAscii( WinEDA_GerberFrame* frame,
                               FILE* File, int* LayerLookUpTable );


/* Export data in pcbnew format
 */
void WinEDA_GerberFrame::ExportDataInPcbnewFormat( wxCommandEvent& event )
{
    int  ii = 0;
    bool no_used_layers = true; // Changed to false if any used layer found

    // Check whether any of the Gerber layers are actually currently used
    while( no_used_layers && ii < 32 )
    {
        if( g_GERBER_List[ii] != NULL )
            no_used_layers = false;
        ii++;
    }

    if( no_used_layers )
    {
        DisplayInfoMessage( this,
                           _( "None of the Gerber layers contain any data" ) );
        return;
    }

    wxString FullFileName, msg;

    wxString PcbExt( wxT( ".brd" ) );

    FILE*    dest;

    msg = wxT( "*" ) + PcbExt;
    FullFileName = EDA_FileSelector( _( "Board file name:" ),
                                     wxEmptyString,
                                     wxEmptyString,
                                     PcbExt,
                                     msg,
                                     this,
                                     wxFD_SAVE,
                                     FALSE
                                     );
    if( FullFileName == wxEmptyString )
        return;

    int* LayerLookUpTable;
    if( ( LayerLookUpTable = InstallDialogLayerPairChoice( ) ) != NULL )
    {
        if( wxFileExists( FullFileName ) )
        {
            if( !IsOK( this, _( "Ok to change the existing file ?" ) ) )
                return;
        }
        dest = wxFopen( FullFileName, wxT( "wt" ) );
        if( dest == 0 )
        {
            msg = _( "Unable to create " ) + FullFileName;
            DisplayError( this, msg );
            return;
        }
        GetScreen()->SetFileName( FullFileName );
        SavePcbFormatAscii( this, dest, LayerLookUpTable );
        fclose( dest );
    }
}


static int WriteSetup( FILE* File, BOARD* Pcb )
{
    char text[1024];

    fprintf( File, "$SETUP\n" );
    sprintf( text, "InternalUnit %f INCH\n", 1.0 / PCB_INTERNAL_UNIT );
    fprintf( File, "%s", text );

    fprintf( File, "Layers %d\n", Pcb->GetCopperLayerCount() );

    fprintf( File, "$EndSETUP\n\n" );
    return 1;
}


static bool WriteGeneralDescrPcb( BOARD* Pcb, FILE* File )
{
    int NbLayers;

    /* Print the copper layer count */
    NbLayers = Pcb->GetCopperLayerCount();
    fprintf( File, "$GENERAL\n" );
    fprintf( File, "LayerCount %d\n", NbLayers );

    /* Compute and print the board bounding box */
    Pcb->ComputeBoundingBox();
    fprintf( File, "Di %d %d %d %d\n",
            Pcb->m_BoundaryBox.GetX(), Pcb->m_BoundaryBox.GetY(),
            Pcb->m_BoundaryBox.GetRight(),
            Pcb->m_BoundaryBox.GetBottom() );

    fprintf( File, "$EndGENERAL\n\n" );
    return TRUE;
}


/* Routine to save the board
 * @param frame = pointer to the main frame
 * @param File = FILE * pointer to an already opened file
 * @param LayerLookUpTable = look up table: pcbnew layer for each gerber layer
 * @return 1 if OK, 0 if fail
 */
static int SavePcbFormatAscii( WinEDA_GerberFrame* frame, FILE* aFile,
                               int* LayerLookUpTable )
{
    char   line[256];
    BOARD* gerberPcb = frame->GetBoard();
    BOARD* pcb;

    wxBeginBusyCursor();

    // create an image of gerber data
    pcb = new BOARD( NULL, frame );
    BOARD_ITEM* item = gerberPcb->m_Drawings;
    for( ; item; item = item->Next() )
    {
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;
        int layer = gerb_item->GetLayer();
        int pcb_layer_number = LayerLookUpTable[layer];
        if( pcb_layer_number < 0 || pcb_layer_number > LAST_NO_COPPER_LAYER )
            continue;

        if( pcb_layer_number > LAST_COPPER_LAYER )
        {
            DRAWSEGMENT* drawitem = new DRAWSEGMENT( pcb, TYPE_DRAWSEGMENT );

            drawitem->SetLayer( pcb_layer_number );
            drawitem->m_Start = gerb_item->m_Start;
            drawitem->m_End   = gerb_item->m_End;
            drawitem->m_Width = gerb_item->m_Size.x;

            if( gerb_item->m_Shape == GBR_ARC )
            {
                double cx = gerb_item->m_ArcCentre.x;
                double cy = gerb_item->m_ArcCentre.y;
                double a  = atan2( gerb_item->m_Start.y - cy,
                                   gerb_item->m_Start.x - cx );
                double b  = atan2( gerb_item->m_End.y - cy, gerb_item->m_End.x - cx );

                drawitem->m_Shape   = S_ARC;
                drawitem->m_Angle   = (int) fmod(
                     (a - b) / M_PI * 1800.0 + 3600.0, 3600.0 );
                drawitem->m_Start.x = (int) cx;
                drawitem->m_Start.y = (int) cy;
            }

            pcb->Add( drawitem );
        }
        else
        {
            TRACK* newtrack;

            // replace spots with vias when possible
            if( gerb_item->m_Shape == GBR_SPOT_CIRCLE
                || gerb_item->m_Shape == GBR_SPOT_RECT
                || gerb_item->m_Shape == GBR_SPOT_OVAL )
            {
                newtrack = new SEGVIA( pcb );

                // A spot is found, and can be a via: change it to via, and
                // delete other
                // spots at same location
                newtrack->m_Shape = VIA_THROUGH;
                newtrack->SetLayer( 0x0F );  // Layers are 0 to 15 (Cu/Cmp)
                newtrack->SetDrillDefault();
                newtrack->m_Start = newtrack->m_End = gerb_item->m_Start;
                newtrack->m_Width = (gerb_item->m_Size.x + gerb_item->m_Size.y) / 2;
            }
            else    // a true TRACK
            {
                newtrack = new TRACK( pcb );
                newtrack->SetLayer( pcb_layer_number );
                newtrack->m_Start = gerb_item->m_Start;
                newtrack->m_End = gerb_item->m_End;
                newtrack->m_Width = gerb_item->m_Size.x;
            }

            pcb->Add( newtrack );
        }
    }

    // delete redundant vias
    for( TRACK * track = pcb->m_Track; track; track = track->Next() )
    {
        if( track->m_Shape != VIA_THROUGH )
            continue;

        // Search and delete others vias
        TRACK* next_track;
        TRACK* alt_track = track->Next();
        for( ; alt_track; alt_track = next_track )
        {
            next_track = alt_track->Next();
            if( alt_track->m_Shape != VIA_THROUGH )
                continue;

            if( alt_track->m_Start != track->m_Start )
                continue;

            // delete track
            alt_track->UnLink();
            delete alt_track;
        }
    }

    // Switch the locale to standard C (needed to print floating point numbers
    // like 1.3)
    SetLocaleTo_C_standard();

    // write the PCB heading
    fprintf( aFile, "PCBNEW-BOARD Version %d date %s\n\n", g_CurrentVersionPCB,
            DateAndTime( line ) );
    WriteGeneralDescrPcb( pcb, aFile );
    WriteSetup( aFile, pcb );

    // write the useful part of the pcb
    pcb->Save( aFile );

    // the destructor should destroy all owned sub-objects
    delete pcb;

    SetLocaleTo_Default();       // revert to the current locale
    wxEndBusyCursor();
    return 1;
}
