/**
 * @file export_to_pcbnew.cpp
 * @brief Export the layers to Pcbnew.
 */

#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <macros.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <trigo.h>

#include <../pcbnew/class_track.h>
#include <../pcbnew/class_drawsegment.h>

#include <io_mgr.h>
#include <gerbview.h>
#include <class_board_design_settings.h>
#include <class_gerber_draw_item.h>
#include <select_layers_to_pcb.h>
#include <build_version.h>
#include <wildcards_and_files_ext.h>


/* A helper class to export a Gerber set of files to Pcbnew
*/
class GBR_TO_PCB_EXPORTER
{
public:
    GBR_TO_PCB_EXPORTER( GERBVIEW_FRAME* aFrame, const wxString& aFileName );
    ~GBR_TO_PCB_EXPORTER();

    /**
     * Function ExportPcb
     * saves a board from a gerber load.
     */
    bool ExportPcb( int* LayerLookUpTable );
    BOARD* GetBoard() { return m_pcb; }

private:
    void export_non_copper_item( GERBER_DRAW_ITEM* aGbrItem, int aLayer );
    void export_copper_item( GERBER_DRAW_ITEM* aGbrItem, int aLayer );
    void export_flashed_copper_item( GERBER_DRAW_ITEM* aGbrItem, int aLayer );
    void export_segline_copper_item( GERBER_DRAW_ITEM* aGbrItem, int aLayer );
    void export_segarc_copper_item( GERBER_DRAW_ITEM* aGbrItem, int aLayer );
    void cleanBoard();

    GERBVIEW_FRAME* m_gerbview_frame;   // the maint gerber frame
    wxString        m_file_name;        // BOARD file to write to
    BOARD*          m_pcb;              // the board to populate and export
};


GBR_TO_PCB_EXPORTER::GBR_TO_PCB_EXPORTER( GERBVIEW_FRAME* aFrame, const wxString& aFileName )
{
    m_gerbview_frame = aFrame;
    m_file_name = aFileName;
    m_pcb = new BOARD();
}


GBR_TO_PCB_EXPORTER::~GBR_TO_PCB_EXPORTER()
{
    delete m_pcb;
}


/* Export data in Pcbnew format
 * remember Pcbnew uses a Y reversed axis, so we must negate all Y coordinates
 */
void GERBVIEW_FRAME::ExportDataInPcbnewFormat( wxCommandEvent& event )
{
    int layercount = 0;

    // Count the Gerber layers which are actually currently used
    for( int ii = 0; ii < 32; ii++ )
    {
        if( g_GERBER_List[ii] != NULL )
            layercount++;

    }

    if( layercount == 0 )
    {
        DisplayInfoMessage( this,
                            _( "None of the Gerber layers contain any data" ) );
        return;
    }

    wxString fileName;
    wxString path = wxGetCwd();;

    wxFileDialog filedlg( this, _( "Board file name:" ),
                      path, fileName, LegacyPcbFileWildcard,
                      wxFD_OPEN );

    if( filedlg.ShowModal() == wxID_CANCEL )
        return;

    fileName = filedlg.GetPath();

    /* Install a dialog frame to choose the mapping
     * between gerber layers and Pcbnew layers
     */
    LAYERS_MAP_DIALOG* layerdlg = new LAYERS_MAP_DIALOG( this );
    int ok = layerdlg->ShowModal();
    layerdlg->Destroy();

    if( ok != wxID_OK )
        return;

    if( wxFileExists( fileName ) )
    {
        if( !IsOK( this, _( "Ok to change the existing file ?" ) ) )
            return;
    }

    GBR_TO_PCB_EXPORTER     gbr_exporter( this, fileName );

    gbr_exporter.ExportPcb( layerdlg->GetLayersLookUpTable() );
}


void GBR_TO_PCB_EXPORTER::cleanBoard()
{
    // delete redundant vias
    for( TRACK * track = m_pcb->m_Track; track; track = track->Next() )
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
}


bool GBR_TO_PCB_EXPORTER::ExportPcb( int* LayerLookUpTable )
{
    BOARD* gerberPcb = m_gerbview_frame->GetBoard();

    // create an image of gerber data
    BOARD_ITEM* item = gerberPcb->m_Drawings;

    for( ; item; item = item->Next() )
    {
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;
        int layer = gerb_item->GetLayer();
        int pcb_layer_number = LayerLookUpTable[layer];

        if( pcb_layer_number < 0 || pcb_layer_number > LAST_NO_COPPER_LAYER )
            continue;

        if( pcb_layer_number > LAST_COPPER_LAYER )
            export_non_copper_item( gerb_item, pcb_layer_number );
        else
            export_copper_item( gerb_item, pcb_layer_number );
    }

    cleanBoard();
    m_pcb->SetCopperLayerCount( LayerLookUpTable[32] );

    try
    {
        wxFileName  pcbFileName( m_file_name );
        PROPERTIES props;

        wxString header = wxString::Format(
            wxT( "PCBNEW-BOARD Version %d date %s\n\n# Created by GerbView%s\n\n" ),
            LEGACY_BOARD_FILE_VERSION, DateAndTime().GetData(),
            GetBuildVersion().GetData() );

        props["header"] = header;

        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );
        pi->Save( m_file_name, m_pcb, &props );
    }
    catch( IO_ERROR ioe )
    {
        DisplayError( m_gerbview_frame, ioe.errorText );
        return false;
    }

    return true;
}


void GBR_TO_PCB_EXPORTER::export_non_copper_item( GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    DRAWSEGMENT* drawitem = new DRAWSEGMENT( m_pcb, PCB_LINE_T );

    drawitem->SetLayer( aLayer );
    drawitem->SetStart( aGbrItem->m_Start );
    drawitem->SetEnd( aGbrItem->m_End );
    drawitem->SetWidth( aGbrItem->m_Size.x );

    if( aGbrItem->m_Shape == GBR_ARC )
    {
        double a  = atan2( (double)( aGbrItem->m_Start.y - aGbrItem->m_ArcCentre.y),
                           (double)( aGbrItem->m_Start.x - aGbrItem->m_ArcCentre.x ) );
        double b  = atan2( (double)( aGbrItem->m_End.y - aGbrItem->m_ArcCentre.y ),
                           (double)( aGbrItem->m_End.x - aGbrItem->m_ArcCentre.x ) );

        drawitem->SetShape( S_ARC );
        drawitem->SetAngle( wxRound( (a - b) / M_PI * 1800.0 ) );
        drawitem->SetStart( aGbrItem->m_ArcCentre );

        if( drawitem->GetAngle() < 0 )
        {
            drawitem->SetAngle( -drawitem->GetAngle() );
            drawitem->SetEnd( aGbrItem->m_Start );
        }
    }

    // Reverse Y axis:
    drawitem->SetStartY( -drawitem->GetStart().y );
    drawitem->SetEndY( -drawitem->GetEnd().y );

    m_pcb->Add( drawitem );
}


void GBR_TO_PCB_EXPORTER::export_copper_item( GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    switch( aGbrItem->m_Shape )
    {
        case GBR_SPOT_CIRCLE:
        case GBR_SPOT_RECT:
        case GBR_SPOT_OVAL:
            // replace spots with vias when possible
            export_flashed_copper_item( aGbrItem, aLayer );
            break;

        case GBR_ARC:
            export_segarc_copper_item( aGbrItem, aLayer );
            break;

        default:
            export_segline_copper_item( aGbrItem, aLayer );
            break;
    }
}


void GBR_TO_PCB_EXPORTER::export_segline_copper_item( GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    TRACK * newtrack = new TRACK( m_pcb );
    newtrack->SetLayer( aLayer );
    newtrack->m_Start = aGbrItem->m_Start;
    newtrack->m_End = aGbrItem->m_End;
    newtrack->m_Width = aGbrItem->m_Size.x;

    // Reverse Y axis:
    NEGATE( newtrack->m_Start.y );
    NEGATE( newtrack->m_End.y );

    m_pcb->Add( newtrack );
}


void GBR_TO_PCB_EXPORTER::export_segarc_copper_item( GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    double a  = atan2( (double)( aGbrItem->m_Start.y - aGbrItem->m_ArcCentre.y ),
                        (double)( aGbrItem->m_Start.x - aGbrItem->m_ArcCentre.x ) );
    double b  = atan2( (double)( aGbrItem->m_End.y - aGbrItem->m_ArcCentre.y ),
                        (double)( aGbrItem->m_End.x - aGbrItem->m_ArcCentre.x ) );

    wxPoint start = aGbrItem->m_Start;
    wxPoint end = aGbrItem->m_End;
    /* Because Pcbnew does not know arcs in tracks,
     * approximate arc by segments (SEG_COUNT__CIRCLE segment per 360 deg)
     * The arc is drawn in an anticlockwise direction from the start point to the end point.
     */
    #define SEG_COUNT_CIRCLE 16
    #define DELTA_ANGLE 2*M_PI/SEG_COUNT_CIRCLE

    // calculate the number of segments from a to b.
    // we want CNT_PER_360 segments fo a circle
    if( a > b )
        b += 2*M_PI;

    wxPoint curr_start = start;

    int ii = 1;
    for( double rot = a; rot < (b - DELTA_ANGLE); rot += DELTA_ANGLE, ii++ )
    {
        TRACK * newtrack = new TRACK( m_pcb );
        newtrack->SetLayer( aLayer );
        newtrack->m_Start = curr_start;
        wxPoint curr_end = start;
        RotatePoint( &curr_end, aGbrItem->m_ArcCentre, -(int)(DELTA_ANGLE * ii * 1800 / M_PI) );
        newtrack->m_End = curr_end;
        newtrack->m_Width = aGbrItem->m_Size.x;
        // Reverse Y axis:
        NEGATE( newtrack->m_Start.y );
        NEGATE( newtrack->m_End.y );
        m_pcb->Add( newtrack );
        curr_start = curr_end;
    }

    if( end != curr_start )
    {
        TRACK * newtrack = new TRACK( m_pcb );
        newtrack->SetLayer( aLayer );
        newtrack->m_Start = curr_start;
        newtrack->m_End = end;
        newtrack->m_Width = aGbrItem->m_Size.x;
        // Reverse Y axis:
        NEGATE( newtrack->m_Start.y );
        NEGATE( newtrack->m_End.y );
        m_pcb->Add( newtrack );
    }
}


/*
 * creates a via from a flashed gerber item.
 * Flashed items are usually pads or vias, so we try to export all of them
 * using vias
 */
void GBR_TO_PCB_EXPORTER::export_flashed_copper_item( GERBER_DRAW_ITEM* aGbrItem, int aLayer )
{
    SEGVIA * newtrack = new SEGVIA( m_pcb );

    newtrack->m_Shape = VIA_THROUGH;
    newtrack->SetLayer( 0x0F );  // Layers are 0 to 15 (Cu/Cmp)
    newtrack->SetDrillDefault();
    newtrack->m_Start = newtrack->m_End = aGbrItem->m_Start;
    newtrack->m_Width = (aGbrItem->m_Size.x + aGbrItem->m_Size.y) / 2;
    // Reverse Y axis:
    NEGATE( newtrack->m_Start.y );
    NEGATE( newtrack->m_End.y );
    m_pcb->Add( newtrack );
}
