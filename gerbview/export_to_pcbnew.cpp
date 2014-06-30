/**
 * @file export_to_pcbnew.cpp
 * @brief Export the layers to Pcbnew.
 */

#include <vector>

#include <fctsys.h>
#include <common.h>
#include <confirm.h>
#include <macros.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <trigo.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_gerber_draw_item.h>
#include <select_layers_to_pcb.h>
#include <build_version.h>
#include <wildcards_and_files_ext.h>

#define TO_PCB_UNIT( x ) KiROUND( x / IU_PER_DECIMILS )

#define TRACK_TYPE  0
#define VIA_TYPE    1

/* A helper class to export a Gerber set of files to Pcbnew
 */
class GBR_TO_PCB_EXPORTER
{
private:
    GERBVIEW_FRAME*         m_gerbview_frame;   // the main gerber frame
    wxString                m_pcb_file_name;    // BOARD file to write to
    FILE*                   m_fp;               // the board file
    int                     m_pcbCopperLayersCount;
    std::vector<wxPoint>    m_vias_coordinates; // list of already generated vias,
                                                // used to export only once a via
                                                // having a given coordinate
public:
    GBR_TO_PCB_EXPORTER( GERBVIEW_FRAME* aFrame, const wxString& aFileName );
    ~GBR_TO_PCB_EXPORTER();

    /**
     * Function ExportPcb
     * saves a board from a set of Gerber images.
     */
    bool    ExportPcb( LAYER_NUM* LayerLookUpTable, int aCopperLayers );

private:
    /**
     * Function export_non_copper_item
     * write a non copper line or arc to the board file.
     * @param aGbrItem = the Gerber item (line, arc) to export
     * @param aLayer = the technical layer to use
     */
    void    export_non_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer );

    /**
     * Function export_copper_item
     * write a track or via) to the board file.
     * @param aGbrItem = the Gerber item (line, arc, flashed) to export
     * @param aLayer = the copper layer to use
     */
    void    export_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer );

    /**
     * Function export_flashed_copper_item
     * write a via to the board file (always uses a via through).
     * @param aGbrItem = the flashed Gerber item to export
     */
    void    export_flashed_copper_item( GERBER_DRAW_ITEM* aGbrItem );

    /**
     * Function export_segline_copper_item
     * write a track (not via) to the board file.
     * @param aGbrItem = the Gerber item (line only) to export
     * @param aLayer = the copper layer to use
     */
    void    export_segline_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer );

    /**
     * Function export_segarc_copper_item
     * write a set of tracks (arcs are approximated by track segments)
     * to the board file.
     * @param aGbrItem = the Gerber item (arc only) to export
     * @param aLayer = the copper layer to use
     */
    void    export_segarc_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer );

    /**
     * function writePcbLineItem
     * basic write function to write a DRAWSEGMENT item or a TRACK/VIA item
     * to the board file
     */
    void    writePcbLineItem( int aShape, int aType, wxPoint& aStart, wxPoint& aEnd,
                              int aWidth, LAYER_NUM aLayer, int aDrill, int aAngle = 0 );

    /**
     * function writePcbHeader
     * Write a very basic header to the board file
     */
    void    writePcbHeader();
};


GBR_TO_PCB_EXPORTER::GBR_TO_PCB_EXPORTER( GERBVIEW_FRAME* aFrame, const wxString& aFileName )
{
    m_gerbview_frame    = aFrame;
    m_pcb_file_name     = aFileName;
}


GBR_TO_PCB_EXPORTER::~GBR_TO_PCB_EXPORTER()
{
}


/* Export data in Pcbnew format
 * remember Pcbnew uses a Y reversed axis, so we must negate all Y coordinates
 */
void GERBVIEW_FRAME::ExportDataInPcbnewFormat( wxCommandEvent& event )
{
    int layercount = 0;

    // Count the Gerber layers which are actually currently used
    for( LAYER_NUM ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
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

    wxString        fileName;
    wxString        path = wxGetCwd();;

    wxFileDialog    filedlg( this, _( "Board file name:" ),
                             path, fileName, LegacyPcbFileWildcard,
                             wxFD_SAVE );

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
        if( !IsOK( this, _( "OK to change the existing file ?" ) ) )
            return;
    }

    GBR_TO_PCB_EXPORTER gbr_exporter( this, fileName );

    gbr_exporter.ExportPcb( layerdlg->GetLayersLookUpTable(),
                            layerdlg->GetCopperLayersCount() );
}


bool GBR_TO_PCB_EXPORTER::ExportPcb( LAYER_NUM* LayerLookUpTable, int aCopperLayers )
{
    m_fp = wxFopen( m_pcb_file_name, wxT( "wt" ) );

    if( m_fp == NULL )
    {
        wxString msg;
        msg.Printf( _( "Cannot create file <%s>" ), GetChars( m_pcb_file_name ) );
        DisplayError( m_gerbview_frame, msg );
        return false;
    }

    m_pcbCopperLayersCount = aCopperLayers;

    writePcbHeader();

    // create an image of gerber data
    // First: non copper layers:
    GERBER_DRAW_ITEM* gerb_item = m_gerbview_frame->GetItemsList();
    int pcbCopperLayerMax = 31;

    for( ; gerb_item; gerb_item = gerb_item->Next() )
    {
        int layer = gerb_item->GetLayer();
        LAYER_NUM pcb_layer_number = LayerLookUpTable[layer];

        if( !IsPcbLayer( pcb_layer_number ) )
            continue;

        if( pcb_layer_number > pcbCopperLayerMax )
            export_non_copper_item( gerb_item, pcb_layer_number );
    }

    // Copper layers
    fprintf( m_fp, "$TRACK\n" );
    gerb_item = m_gerbview_frame->GetItemsList();

    for( ; gerb_item; gerb_item = gerb_item->Next() )
    {
        int layer = gerb_item->GetLayer();
        LAYER_NUM pcb_layer_number = LayerLookUpTable[layer];

        if( pcb_layer_number < 0 || pcb_layer_number > pcbCopperLayerMax )
            continue;

        else
            export_copper_item( gerb_item, pcb_layer_number );
    }

    fprintf( m_fp, "$EndTRACK\n" );
    fprintf( m_fp, "$EndBOARD\n" );

    fclose( m_fp );
    m_fp = NULL;
    return true;
}


void GBR_TO_PCB_EXPORTER::export_non_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer )
{
    #define SEG_SHAPE   0
    #define ARC_SHAPE   2
    int     shape   = SEG_SHAPE;

    // please note: the old PCB format only has integer support for angles
    int     angle   = 0;
    wxPoint seg_start, seg_end;

    seg_start   = aGbrItem->m_Start;
    seg_end     = aGbrItem->m_End;

    if( aGbrItem->m_Shape == GBR_ARC )
    {
        double  a = atan2( (double) ( aGbrItem->m_Start.y - aGbrItem->m_ArcCentre.y),
                           (double) ( aGbrItem->m_Start.x - aGbrItem->m_ArcCentre.x ) );
        double  b = atan2( (double) ( aGbrItem->m_End.y - aGbrItem->m_ArcCentre.y ),
                           (double) ( aGbrItem->m_End.x - aGbrItem->m_ArcCentre.x ) );

        shape       = ARC_SHAPE;
        angle       = KiROUND( RAD2DECIDEG(a - b) );
        seg_start   = aGbrItem->m_ArcCentre;

        if( angle < 0 )
        {
            NEGATE( angle );
            seg_end = aGbrItem->m_Start;
        }
    }

    // Reverse Y axis:
    NEGATE( seg_start.y );
    NEGATE( seg_end.y );
    writePcbLineItem( shape, 0, seg_start, seg_end, aGbrItem->m_Size.x, aLayer, -2, angle );
}


void GBR_TO_PCB_EXPORTER::export_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer )
{
    switch( aGbrItem->m_Shape )
    {
    case GBR_SPOT_CIRCLE:
    case GBR_SPOT_RECT:
    case GBR_SPOT_OVAL:
        // replace spots with vias when possible
        export_flashed_copper_item( aGbrItem );
        break;

    case GBR_ARC:
        export_segarc_copper_item( aGbrItem, aLayer );
        break;

    default:
        export_segline_copper_item( aGbrItem, aLayer );
        break;
    }
}


void GBR_TO_PCB_EXPORTER::export_segline_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer )
{
    wxPoint seg_start, seg_end;

    seg_start   = aGbrItem->m_Start;
    seg_end     = aGbrItem->m_End;

    // Reverse Y axis:
    NEGATE( seg_start.y );
    NEGATE( seg_end.y );

    writePcbLineItem( 0, TRACK_TYPE, seg_start, seg_end, aGbrItem->m_Size.x, aLayer, -1 );
}


void GBR_TO_PCB_EXPORTER::export_segarc_copper_item( GERBER_DRAW_ITEM* aGbrItem, LAYER_NUM aLayer )
{
    double  a = atan2( (double) ( aGbrItem->m_Start.y - aGbrItem->m_ArcCentre.y ),
                       (double) ( aGbrItem->m_Start.x - aGbrItem->m_ArcCentre.x ) );
    double  b = atan2( (double) ( aGbrItem->m_End.y - aGbrItem->m_ArcCentre.y ),
                       (double) ( aGbrItem->m_End.x - aGbrItem->m_ArcCentre.x ) );

    wxPoint start   = aGbrItem->m_Start;
    wxPoint end     = aGbrItem->m_End;

    /* Because Pcbnew does not know arcs in tracks,
     * approximate arc by segments (SEG_COUNT__CIRCLE segment per 360 deg)
     * The arc is drawn in an anticlockwise direction from the start point to the end point.
     */
    #define SEG_COUNT_CIRCLE    16
    #define DELTA_ANGLE         2 * M_PI / SEG_COUNT_CIRCLE

    // calculate the number of segments from a to b.
    // we want CNT_PER_360 segments fo a circle
    if( a > b )
        b += 2 * M_PI;

    wxPoint curr_start = start;
    wxPoint seg_start, seg_end;

    int     ii = 1;

    for( double rot = a; rot < (b - DELTA_ANGLE); rot += DELTA_ANGLE, ii++ )
    {
        seg_start = curr_start;
        wxPoint curr_end = start;
        RotatePoint( &curr_end, aGbrItem->m_ArcCentre,
                     -RAD2DECIDEG( DELTA_ANGLE * ii ) );
        seg_end = curr_end;
        // Reverse Y axis:
        NEGATE( seg_start.y );
        NEGATE( seg_end.y );
        writePcbLineItem( 0, TRACK_TYPE, seg_start, seg_end, aGbrItem->m_Size.x, aLayer, -1 );
        curr_start = curr_end;
    }

    if( end != curr_start )
    {
        seg_start   = curr_start;
        seg_end     = end;
        // Reverse Y axis:
        NEGATE( seg_start.y );
        NEGATE( seg_end.y );
        writePcbLineItem( 0, TRACK_TYPE, seg_start, seg_end, aGbrItem->m_Size.x, aLayer, -1 );
    }
}


/*
 * creates a via from a flashed gerber item.
 * Flashed items are usually pads or vias, so we try to export all of them
 * using vias
 */
void GBR_TO_PCB_EXPORTER::export_flashed_copper_item( GERBER_DRAW_ITEM* aGbrItem )
{
    // First, explore already created vias, before creating a new via
    for( unsigned ii = 0; ii < m_vias_coordinates.size(); ii++ )
    {
        if( m_vias_coordinates[ii] == aGbrItem->m_Start ) // Already created
            return;
    }

    m_vias_coordinates.push_back( aGbrItem->m_Start );

    wxPoint via_pos;
    int     width;

    via_pos = aGbrItem->m_Start;
    width   = (aGbrItem->m_Size.x + aGbrItem->m_Size.y) / 2;
    // Reverse Y axis:
    NEGATE( via_pos.y );
    // Layers are 0 to 15 (Cu/Cmp) = 0x0F
    #define IS_VIA 1
    #define SHAPE_VIA_THROUGH 3
    // XXX EVIL usage of LAYER
    writePcbLineItem( SHAPE_VIA_THROUGH, IS_VIA, via_pos, via_pos, width,
                      0x0F, -1 );
}


void GBR_TO_PCB_EXPORTER::writePcbHeader()
{
    fprintf( m_fp, "PCBNEW-BOARD Version 1 date %s\n\n# Created by GerbView %s\n\n",
             TO_UTF8( DateAndTime() ), TO_UTF8( GetBuildVersion() ) );
    fprintf( m_fp, "$GENERAL\n" );
    fprintf( m_fp, "encoding utf-8\n" );
    fprintf( m_fp, "Units deci-mils\n" );

    // Write copper layer count
    fprintf( m_fp, "LayerCount %d\n", m_pcbCopperLayersCount );

    fprintf( m_fp, "$EndGENERAL\n\n" );

    // Creates void setup
    fprintf( m_fp, "$SETUP\n" );
    fprintf( m_fp, "$EndSETUP\n\n" );
}


void GBR_TO_PCB_EXPORTER::writePcbLineItem( int aShape, int aType, wxPoint& aStart, wxPoint& aEnd,
                                            int aWidth, LAYER_NUM aLayer, int aDrill, int aAngle )
{
    if( aDrill <= -2 )
        fprintf( m_fp, "$DRAWSEGMENT\n" );

    fprintf( m_fp, "Po %d %d %d %d %d %d\n", aShape,
             TO_PCB_UNIT( aStart.x ), TO_PCB_UNIT( aStart.y ),
             TO_PCB_UNIT( aEnd.x ), TO_PCB_UNIT( aEnd.y ),
             TO_PCB_UNIT( aWidth ) );
    fprintf( m_fp, "De %d %d %d %lX %X",
             aLayer, aType, aAngle, 0l, 0 );

    if( aDrill > -2 )
        fprintf( m_fp, " %d", aDrill );

    fprintf( m_fp, "\n" );

    if( aDrill <= -2 )
        fprintf( m_fp, "$EndDRAWSEGMENT\n" );
}
