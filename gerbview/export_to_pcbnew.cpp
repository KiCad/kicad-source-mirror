/* export_to_pcbnew.cpp */

/*
 *  Export the layers to pcbnew
 */

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"

#include "protos.h"

/* Routines Locales : */
static int SavePcbFormatAscii( WinEDA_GerberFrame* frame,
                               FILE* File, int* LayerLookUpTable );

/* Variables Locales */


/************************************************************************/
void WinEDA_GerberFrame::ExportDataInPcbnewFormat( wxCommandEvent& event )
/************************************************************************/

/* Export data in pcbnew format
 */
{
    wxString FullFileName, msg;

    wxString PcbExt( wxT( ".brd" ) );

    FILE* dest;

    msg = wxT( "*" ) + PcbExt;
    FullFileName = EDA_FileSelector( _( "Board file name:" ),
                                     wxEmptyString, /* Chemin par defaut */
                                     wxEmptyString, /* nom fichier par defaut */
                                     PcbExt,        /* extension par defaut */
                                     msg,           /* Masque d'affichage */
                                     this,
                                     wxFD_SAVE,
                                     FALSE
                                     );
    if( FullFileName == wxEmptyString )
        return;

    int* LayerLookUpTable;
    if( ( LayerLookUpTable = InstallDialogLayerPairChoice( this ) ) != NULL )
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
        GetScreen()->m_FileName = FullFileName;
        SavePcbFormatAscii( this, dest, LayerLookUpTable );
        fclose( dest );
    }
}


/***************************************************************/
static int WriteSetup( FILE* File, BOARD* Pcb )
/***************************************************************/
{
    char text[1024];

    fprintf( File, "$SETUP\n" );
    sprintf( text, "InternalUnit %f INCH\n", 1.0 / PCB_INTERNAL_UNIT );
    fprintf( File, text );

    Pcb->m_BoardSettings->m_CopperLayerCount = g_DesignSettings.m_CopperLayerCount;
    fprintf( File, "Layers %d\n", g_DesignSettings.m_CopperLayerCount );

    fprintf( File, "$EndSETUP\n\n" );
    return 1;
}


/******************************************************/
static bool WriteGeneralDescrPcb( BOARD* Pcb, FILE* File )
/******************************************************/
{
    int NbLayers;

    /* generation du masque des couches autorisees */
    NbLayers = Pcb->m_BoardSettings->m_CopperLayerCount;
    fprintf( File, "$GENERAL\n" );
    fprintf( File, "LayerCount %d\n", NbLayers );

    /* Generation des coord du rectangle d'encadrement */
    Pcb->ComputeBoundaryBox();
    fprintf( File, "Di %d %d %d %d\n",
            Pcb->m_BoundaryBox.GetX(), Pcb->m_BoundaryBox.GetY(),
            Pcb->m_BoundaryBox.GetRight(),
            Pcb->m_BoundaryBox.GetBottom() );

    fprintf( File, "$EndGENERAL\n\n" );
    return TRUE;
}


/*******************************************************************/
static int SavePcbFormatAscii( WinEDA_GerberFrame* frame, FILE* File,
                               int* LayerLookUpTable )
/*******************************************************************/

/* Routine de sauvegarde du PCB courant sous format ASCII
 *  retourne
 *      1 si OK
 *      0 si sauvegarde non faite
 */
{
    char            Line[256];
    TRACK*          track;
    TRACK*          next_track;
    BOARD_ITEM*     PtStruct;
    BOARD_ITEM*     NextStruct;
    BOARD*          GerberPcb = frame->m_Pcb;
    BOARD*          Pcb;

    wxBeginBusyCursor();

    /* Create an image of gerber data */
    Pcb = new BOARD( NULL, frame );

    for( track = GerberPcb->m_Track; track != NULL; track = (TRACK*) track->Pnext )
    {
        int layer = track->GetLayer();
        int pcb_layer_number = LayerLookUpTable[layer];
        if( pcb_layer_number < 0 )
            continue;
        
        if( pcb_layer_number > CMP_N )
        {
            DRAWSEGMENT* drawitem = new DRAWSEGMENT( NULL, TYPEDRAWSEGMENT );

            drawitem->SetLayer( pcb_layer_number );
            drawitem->m_Start = track->m_Start;
            drawitem->m_End   = track->m_End;
            drawitem->m_Width = track->m_Width;
            drawitem->Pnext   = Pcb->m_Drawings;
            Pcb->m_Drawings   = drawitem;
        }
        else
        {
            TRACK*  newtrack;
            
            // replace spots with vias when possible
            if( (track->m_Shape == S_SPOT_CIRCLE) 
             || (track->m_Shape == S_SPOT_RECT)  
             || (track->m_Shape == S_SPOT_OVALE) )
            {
                newtrack = new SEGVIA( (const SEGVIA&) *track );

                // A spot is found, and can be a via: change it to via, and delete other
                // spots at same location
                newtrack->m_Shape = VIA_NORMALE;
                
                newtrack->SetLayer( 0x0F );  // Layers are 0 to 15 (Cu/Cmp)
                
                newtrack->m_Drill = -1;
                
                // Compute the via position from track position ( Via position is the 
                // position of the middle of the track segment ) 
                newtrack->m_Start.x = (newtrack->m_Start.x + newtrack->m_End.x) / 2;
                newtrack->m_Start.y = (newtrack->m_Start.y + newtrack->m_End.y) / 2;
                newtrack->m_End = newtrack->m_Start;
            }
            else    // a true TRACK
            {
                newtrack = new TRACK( *track );
                newtrack->SetLayer( pcb_layer_number );
            }
            
            newtrack->Insert( Pcb, NULL );
        }
    }

    // delete redundant vias
    for( track = Pcb->m_Track; track != NULL; track = track->Next() )
    {
        if( track->m_Shape != VIA_NORMALE )
            continue;
        
        // Search and delete others vias
        TRACK* alt_track = track->Next();
        for( ; alt_track != NULL; alt_track = next_track )
        {
            next_track = alt_track->Next();
            if( alt_track->m_Shape != VIA_NORMALE )
                continue;

            if( alt_track->m_Start != track->m_Start )
                continue;

            // delete track
            alt_track->UnLink();
            delete alt_track;
        }
    }

    // Switch the locale to standard C (needed to print floating point numbers like 1.3)
    setlocale( LC_NUMERIC, "C" );
    
    // write the PCB heading
    fprintf( File, "PCBNEW-BOARD Version %d date %s\n\n", g_CurrentVersionPCB,
            DateAndTime( Line ) );
    WriteGeneralDescrPcb( Pcb, File );
    WriteSetup( File, Pcb );

    // write the useful part of the pcb
    PtStruct = Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPETEXTE:
            ( (TEXTE_PCB*) PtStruct )->WriteTextePcbDescr( File );
            break;

        case TYPEDRAWSEGMENT:
            ( (DRAWSEGMENT*) PtStruct )->WriteDrawSegmentDescr( File );
            break;

        default:
            break;
        }
    }

    fprintf( File, "$TRACK\n" );
    for( track = Pcb->m_Track; track != NULL; track = (TRACK*) track->Pnext )
    {
        track->WriteTrackDescr( File );
    }

    fprintf( File, "$EndTRACK\n" );

    fprintf( File, "$EndBOARD\n" );

    // Delete the copy
    for( PtStruct = Pcb->m_Drawings; PtStruct != NULL; PtStruct = NextStruct )
    {
        NextStruct = PtStruct->Next();
        delete PtStruct;
    }

    for( track = Pcb->m_Track; track != NULL; track = next_track )
    {
        next_track = (TRACK*) track->Pnext;
        delete track;
    }

    delete Pcb;

    setlocale( LC_NUMERIC, "" );      // revert to the current  locale
    wxEndBusyCursor();
    return 1;
}
