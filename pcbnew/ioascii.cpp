 /**
 * @file ioascii.cpp
 * @brief Routines for reading and saving of structures in ASCII file common to Pcbnew and CvPcb.
 */

#include "fctsys.h"
#include "confirm.h"
#include "kicad_string.h"
#include "build_version.h"
#include "wxPcbStruct.h"
#include "richio.h"
#include "macros.h"
#include "pcbcommon.h"

/**
 * @todo Fix having to recompile the same file with a different defintion.  This is
 *       what C++ derivation was designed to solve.
 */
#ifdef PCBNEW
#include "zones.h"
#endif

#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "class_board.h"
#include "class_module.h"
#include "class_track.h"
#include "class_pcb_text.h"
#include "class_zone.h"
#include "class_dimension.h"
#include "class_drawsegment.h"
#include "class_mire.h"

#include "pcbnew.h"
#include "pcbnew_id.h"
#include "autorout.h"
#include "pcb_plot_params.h"


/* ASCII format of structures:
 *
 * Structure PAD:
 *
 * $PAD
 * Sh "name" form DIMVA dimH dV dH East: general form dV, dH = delta size
 * Dr. diam dV dH: drill: diameter drilling offsets
 * At Type S / N layers: standard, cms, conn, hole, meca.,
 *    Stack / Normal, 32-bit hexadecimal: occupation layers
 * Nm net_code netname
 * Po posrefX posrefy: reFX position, Y (0 = east position / anchor)
 * $EndPAD
 *
 * Module Structure
 *
 * $MODULE namelib
 * Po ax ay east layer masquelayer m_TimeCode
 *    ax ay ord = anchor (position module)
 *    east = east to 0.1 degree
 *    layer = layer number
 *    masquelayer = silkscreen layer for
 *    m_TimeCode internal use (groups)
 * Li <namelib>
 *
 * Cd <text> description of the component (Component Doc)
 * Kw <text> List of key words
 *
 * Sc schematic timestamp, reference schematic
 *
 * Op rot90 rot180 placement Options Auto (court rot 90, 180)
 *    rot90 is about 2x4-bit:
 *    lsb = cost rot 90, rot court msb = -90;
 *
 * Tn px py DIMVA dimh East thickness mirror visible "text"
 *    n = type (0 = ref, val = 1,> 1 = qcq
 *    Texts POS x, y / anchor and orient module 0
 *    DIMVA dimh East
 *    mirror thickness (Normal / Mirror)
 *    Visible V / I
 * DS ox oy fx fy w
 *    Edge: coord segment ox, oy has fx, fy, on
 *    was the anchor and orient 0
 *    thickness w
 * DC ox oy fx fy w descr circle (center, 1 point, thickness)
 * $PAD
 * $EndPAD section pads if available
 * $Endmodule
 */


int NbDraw, NbTrack, NbZone, NbMod, NbNets;


/** Read a list of segments (Tracks, zones)
 * @return items count or - count if no end block ($End...) found.
 */
int PCB_BASE_FRAME::ReadListeSegmentDescr( LINE_READER* aReader,
                                           TRACK* insertBeforeMe,
                                           int    StructType,
                                           int    NumSegm )
{
    int    shape, width, drill, layer, type, flags, net_code;
    int    tempStartX, tempStartY;
    int    tempEndX, tempEndY;
    int    ii = 0;
    char*  line;

    TRACK* newTrack;

    while( aReader->ReadLine() )
    {
        line = aReader->Line();
        int           makeType;
        unsigned long timeStamp;

        if( line[0] == '$' )
        {
            return ii;      /* end of segmentlist: OK */
        }

        int arg_count = sscanf( line + 2, " %d %d %d %d %d %d %d", &shape,
                                &tempStartX, &tempStartY,
                                &tempEndX, &tempEndY, &width,
                                &drill );

        // Read the 2nd line to determine the exact type, one of:
        // TYPE_TRACK, TYPE_VIA, or TYPE_ZONE.  The type field in 2nd line
        // differentiates between TYPE_TRACK and TYPE_VIA.  With virtual
        // functions in use, it is critical to instantiate the TYPE_VIA
        // exactly.
        if( !aReader->ReadLine() )
            break;

        line = aReader->Line();

        if( line[0] == '$' )
            break;

        // parse the 2nd line first to determine the type of object
        sscanf( line + 2, " %d %d %d %lX %X", &layer, &type, &net_code,
                &timeStamp, &flags );

        if( StructType==TYPE_TRACK && type==1 )
            makeType = TYPE_VIA;
        else
            makeType = StructType;

        switch( makeType )
        {
        default:
        case TYPE_TRACK:
            newTrack = new TRACK( GetBoard() );
            GetBoard()->m_Track.Insert( newTrack, insertBeforeMe );
            break;

        case TYPE_VIA:
            newTrack = new SEGVIA( GetBoard() );
            GetBoard()->m_Track.Insert( newTrack, insertBeforeMe );
            break;

        case TYPE_ZONE:     // this is now deprecated, but exits in old boards
            newTrack = new SEGZONE( GetBoard() );
            GetBoard()->m_Zone.Insert( (SEGZONE*) newTrack, (SEGZONE*) insertBeforeMe );
            break;
        }

        newTrack->m_TimeStamp = timeStamp;

        newTrack->m_Start.x = tempStartX;
        newTrack->m_Start.y = tempStartY;
        newTrack->m_End.x = tempEndX;
        newTrack->m_End.y = tempEndY;

        newTrack->m_Width = width;
        newTrack->m_Shape = shape;

        if( arg_count < 7 || drill <= 0 )
            newTrack->SetDrillDefault();
        else
            newTrack->SetDrillValue( drill );

        newTrack->SetLayer( layer );

        if( makeType == TYPE_VIA ) // Ensure layers are OK when possible:
        {
            if( newTrack->Shape() == VIA_THROUGH )
                ( (SEGVIA*) newTrack )->SetLayerPair( LAYER_N_FRONT, LAYER_N_BACK );
        }

        newTrack->SetNet( net_code );
        newTrack->SetState( flags, ON );
    }

    DisplayError( this, _( "Error: Unexpected end of file !" ) );
    return -ii;
}


int PCB_BASE_FRAME::ReadGeneralDescrPcb( LINE_READER* aReader )
{
    char* Line, * data;

    while(  aReader->ReadLine() )
    {
        Line = aReader->Line();
        data = strtok( Line, " =\n\r" );

        if( strnicmp( data, "$EndGENERAL", 10 ) == 0 )
            break;

        if( stricmp( data, "EnabledLayers" ) == 0 )
        {
            int EnabledLayers = 0;
            data = strtok( NULL, " =\n\r" );
            sscanf( data, "%X", &EnabledLayers );

            // Setup layer visibility
            GetBoard()->SetEnabledLayers( EnabledLayers );

            continue;
        }

        if( strncmp( data, "Ly", 2 ) == 0 )    // Old format for Layer count
        {
            int Masque_Layer = 1, ii;
            data = strtok( NULL, " =\n\r" );
            sscanf( data, "%X", &Masque_Layer );

            // Setup layer count
            int layer_count = 0;

            for( ii = 0; ii < NB_COPPER_LAYERS; ii++ )
            {
                if( Masque_Layer & 1 )
                    layer_count++;

                Masque_Layer >>= 1;
            }

            GetBoard()->SetCopperLayerCount( layer_count );

            continue;
        }

        if( stricmp( data, "BoardThickness" ) == 0 )
        {
            data = strtok( NULL, " =\n\r" );
            GetBoard()->GetBoardDesignSettings()->m_BoardThickness = atoi( data );
            continue;
        }

        if( strnicmp( data, "Links", 5 ) == 0 )
        {
            // Info only, do nothing
            continue;
        }

        if( strnicmp( data, "NoConn", 6 ) == 0 )
        {
            data = strtok( NULL, " =\n\r" );
            GetBoard()->m_NbNoconnect = atoi( data );
            continue;
        }

        if( strnicmp( data, "Di", 2 ) == 0 )
        {
            wxSize pcbsize, screensize;
            data = strtok( NULL, " =\n\r" );
            GetBoard()->m_BoundaryBox.SetX( atoi( data ) );
            data = strtok( NULL, " =\n\r" );
            GetBoard()->m_BoundaryBox.SetY( atoi( data ) );
            data = strtok( NULL, " =\n\r" );
            GetBoard()->m_BoundaryBox.SetWidth( atoi( data ) - GetBoard()->m_BoundaryBox.GetX() );
            data = strtok( NULL, " =\n\r" );
            GetBoard()->m_BoundaryBox.SetHeight( atoi( data ) - GetBoard()->m_BoundaryBox.GetY() );
            continue;
        }

        /* Reading the number of segments of type DRAW, TRACK, ZONE */
        if( stricmp( data, "Ndraw" ) == 0 )
        {
            data   = strtok( NULL, " =\n\r" );
            NbDraw = atoi( data );
            continue;
        }

        if( stricmp( data, "Ntrack" ) == 0 )
        {
            data    = strtok( NULL, " =\n\r" );
            NbTrack = atoi( data );
            continue;
        }

        if( stricmp( data, "Nzone" ) == 0 )
        {
            data   = strtok( NULL, " =\n\r" );
            NbZone = atoi( data );
            continue;
        }

        if( stricmp( data, "Nmodule" ) == 0 )
        {
            data  = strtok( NULL, " =\n\r" );
            NbMod = atoi( data );
            continue;
        }

        if( stricmp( data, "Nnets" ) == 0 )
        {
            data   = strtok( NULL, " =\n\r" );
            NbNets = atoi( data );
            continue;
        }
    }

    return 1;
}


int PCB_BASE_FRAME::ReadSetup( LINE_READER* aReader )
{
    char*     Line;
    char*     data;

    NETCLASS* netclass_default = GetBoard()->m_NetClasses.GetDefault();

    while(  aReader->ReadLine() )
    {
        Line = aReader->Line();

        if( strnicmp( Line, "PcbPlotParams", 13 ) == 0 )
        {
            PCB_PLOT_PARAMS_PARSER parser( &Line[13], aReader->GetSource() );

            try
            {
                g_PcbPlotOptions.Parse( &parser );
            }
            catch( IO_ERROR& e )
            {
                wxString msg;
                msg.Printf( _( "Error reading PcbPlotParams from %s:\n%s" ),
                            aReader->GetSource().GetData(),
                            e.errorText.GetData() );
                wxMessageBox( msg, _( "Open Board File" ), wxICON_ERROR );
            }

            continue;
        }

        strtok( Line, " =\n\r" );
        data = strtok( NULL, " =\n\r" );

        if( stricmp( Line, "$EndSETUP" ) == 0 )
        {
            // Until such time as the *.brd file does not have the
            // global parameters:
            // "TrackWidth", "TrackMinWidth", "ViaSize", "ViaDrill",
            // "ViaMinSize", and "TrackClearence", put those same global
            // values into the default NETCLASS until later board load
            // code should override them.  *.brd files which have been
            // saved with knowledge of NETCLASSes will override these
            // defaults, old boards will not.
            //
            // @todo: I expect that at some point we can remove said global
            //        parameters from the *.brd file since the ones in the
            //        default netclass serve the same purpose.  If needed
            //        at all, the global defaults should go into a preferences
            //        file instead so they are there to start new board
            //        projects.
            GetBoard()->m_NetClasses.GetDefault()->SetParams();

            return 0;
        }

        if( stricmp( Line, "AuxiliaryAxisOrg" ) == 0 )
        {
            int gx = 0, gy = 0;
            gx   = atoi( data );
            data = strtok( NULL, " =\n\r" );

            if( data )
                gy = atoi( data );

            m_Auxiliary_Axis_Position.x = gx;
            m_Auxiliary_Axis_Position.y = gy;
            continue;
        }

#ifdef PCBNEW

        if( stricmp( Line, "Layers" ) == 0 )
        {
            int tmp;
            sscanf( data, "%d", &tmp );
            GetBoard()->SetCopperLayerCount( tmp );
            continue;
        }

        const int LAYERKEYZ = sizeof("Layer[") - 1;

        if( strncmp( Line, "Layer[", LAYERKEYZ ) == 0 )
        {
            // parse:
            // Layer[n]  <a_Layer_name_with_no_spaces> <LAYER_T>

            char* cp    = Line + LAYERKEYZ;
            int   layer = atoi( cp );

            if( data )
            {
                wxString layerName = FROM_UTF8( data );
                GetBoard()->SetLayerName( layer, layerName );

                data = strtok( NULL, " \n\r" );

                if( data )
                {
                    LAYER_T type = LAYER::ParseType( data );
                    GetBoard()->SetLayerType( layer, type );
                }
            }

            continue;
        }

        if( stricmp( Line, "TrackWidth" ) == 0 )    // no more used
        {
            continue;
        }

        if( stricmp( Line, "TrackWidthList" ) == 0 )
        {
            int tmp = atoi( data );
            GetBoard()->m_TrackWidthList.push_back( tmp );
            continue;
        }

        if( stricmp( Line, "TrackClearence" ) == 0 )
        {
            netclass_default->SetClearance( atoi( data ) );
            continue;
        }

        if( stricmp( Line, "TrackMinWidth" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_TrackMinWidth = atoi( data );
            continue;
        }

        if( stricmp( Line, "ZoneClearence" ) == 0 )
        {
            g_Zone_Default_Setting.m_ZoneClearance = atoi( data );
            continue;
        }

        if( stricmp( Line, "DrawSegmWidth" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_DrawSegmentWidth = atoi( data );
            continue;
        }

        if( stricmp( Line, "EdgeSegmWidth" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_EdgeSegmentWidth = atoi( data );
            continue;
        }

        if( stricmp( Line, "ViaSize" ) == 0 )    // no more used
        {
            continue;
        }

        if( stricmp( Line, "ViaMinSize" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_ViasMinSize = atoi( data );
            continue;
        }

        if( stricmp( Line, "MicroViaSize" ) == 0 )  // Not used
        {
            continue;
        }

        if( stricmp( Line, "MicroViaMinSize" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_MicroViasMinSize = atoi( data );
            continue;
        }

        if( stricmp( Line, "ViaSizeList" ) == 0 )
        {
            int           tmp = atoi( data );
            VIA_DIMENSION via_dim;
            via_dim.m_Diameter = tmp;
            data = strtok( NULL, " \n\r" );

            if( data )
            {
                tmp = atoi( data );
                via_dim.m_Drill = tmp > 0 ? tmp : 0;
            }

            GetBoard()->m_ViasDimensionsList.push_back( via_dim );
            continue;
        }

        if( stricmp( Line, "ViaDrill" ) == 0 )
        {
            int diameter = atoi( data );
            netclass_default->SetViaDrill( diameter );
            continue;
        }

        if( stricmp( Line, "ViaMinDrill" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_ViasMinDrill = atoi( data );
            continue;
        }

        if( stricmp( Line, "MicroViaDrill" ) == 0 )
        {
            int diameter = atoi( data );
            netclass_default->SetuViaDrill( diameter );
            continue;
        }

        if( stricmp( Line, "MicroViaMinDrill" ) == 0 )
        {
            int diameter = atoi( data );
            GetBoard()->GetBoardDesignSettings()->m_MicroViasMinDrill = diameter;
            continue;
        }

        if( stricmp( Line, "MicroViasAllowed" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_MicroViasAllowed = atoi( data );
            continue;
        }

        if( stricmp( Line, "TextPcbWidth" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_PcbTextWidth = atoi( data );
            continue;
        }

        if( stricmp( Line, "TextPcbSize" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_PcbTextSize.x = atoi( data );
            data = strtok( NULL, " =\n\r" );
            GetBoard()->GetBoardDesignSettings()->m_PcbTextSize.y = atoi( data );
            continue;
        }

        if( stricmp( Line, "EdgeModWidth" ) == 0 )
        {
            g_ModuleSegmentWidth = atoi( data );
            continue;
        }

        if( stricmp( Line, "TextModWidth" ) == 0 )
        {
            g_ModuleTextWidth = atoi( data );
            continue;
        }

        if( stricmp( Line, "TextModSize" ) == 0 )
        {
            g_ModuleTextSize.x = atoi( data );
            data = strtok( NULL, " =\n\r" );
            g_ModuleTextSize.y = atoi( data );
            continue;
        }

        if( stricmp( Line, "PadSize" ) == 0 )
        {
            g_Pad_Master.m_Size.x = atoi( data );
            data = strtok( NULL, " =\n\r" );
            g_Pad_Master.m_Size.y = atoi( data );
            continue;
        }

        if( stricmp( Line, "PadDrill" ) == 0 )
        {
            g_Pad_Master.m_Drill.x = atoi( data );
            g_Pad_Master.m_Drill.y = g_Pad_Master.m_Drill.x;
            continue;
        }

        if( stricmp( Line, "Pad2MaskClearance" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_SolderMaskMargin = atoi( data );
            continue;
        }

        if( stricmp( Line, "Pad2PasteClearance" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_SolderPasteMargin = atoi( data );
            continue;
        }

        if( stricmp( Line, "Pad2PasteClearanceRatio" ) == 0 )
        {
            GetBoard()->GetBoardDesignSettings()->m_SolderPasteMarginRatio = atof( data );
            continue;
        }

        if( stricmp( Line, "GridOrigin" ) == 0 )
        {
            int Ox = 0;
            int Oy = 0;

            Ox = atoi( data );
            data = strtok( NULL, " =\n\r" );

            if ( data )
                Oy = atoi( data );

            GetScreen()->m_GridOrigin.x = Ox;
            GetScreen()->m_GridOrigin.y = Oy;

            continue;
        }
#endif

    }

    /* Ensure tracks and vias sizes lists are ok:
     * Sort lists by by increasing value and remove duplicates
     * (the first value is not tested, because it is the netclass value
     */
    sort( GetBoard()->m_ViasDimensionsList.begin() + 1, GetBoard()->m_ViasDimensionsList.end() );
    sort( GetBoard()->m_TrackWidthList.begin() + 1, GetBoard()->m_TrackWidthList.end() );

    for( unsigned ii = 1; ii < GetBoard()->m_ViasDimensionsList.size() - 1; ii++ )
    {
        if( GetBoard()->m_ViasDimensionsList[ii] == GetBoard()->m_ViasDimensionsList[ii + 1] )
        {
            GetBoard()->m_ViasDimensionsList.erase( GetBoard()->m_ViasDimensionsList.begin() + ii );
            ii--;
        }
    }

    for( unsigned ii = 1; ii < GetBoard()->m_TrackWidthList.size() - 1; ii++ )
    {
        if( GetBoard()->m_TrackWidthList[ii] == GetBoard()->m_TrackWidthList[ii + 1] )
        {
            GetBoard()->m_TrackWidthList.erase( GetBoard()->m_TrackWidthList.begin() + ii );
            ii--;
        }
    }

    return 1;
}


#ifdef PCBNEW

static int WriteSetup( FILE* aFile, PCB_EDIT_FRAME* aFrame, BOARD* aBoard )
{
    NETCLASS* netclass_default = aBoard->m_NetClasses.GetDefault();
    char      text[1024];

    fprintf( aFile, "$SETUP\n" );
    sprintf( text, "InternalUnit %f INCH\n", 1.0 / PCB_INTERNAL_UNIT );
    fprintf( aFile, "%s", text );

    fprintf( aFile, "Layers %d\n", aBoard->GetCopperLayerCount() );

    unsigned layerMask = g_TabAllCopperLayerMask[aBoard->GetCopperLayerCount() - 1];

    for( int layer = 0; layerMask; ++layer, layerMask >>= 1 )
    {
        if( layerMask & 1 )
        {
            fprintf( aFile, "Layer[%d] %s %s\n", layer,
                     TO_UTF8( aBoard->GetLayerName( layer ) ),
                     LAYER::ShowType( aBoard->GetLayerType( layer ) ) );
        }
    }

    // Save current default track width, for compatibility with older Pcbnew version;
    fprintf( aFile, "TrackWidth %d\n", aBoard->GetCurrentTrackWidth() );

    // Save custom tracks width list (the first is not saved here: this is the netclass value
    for( unsigned ii = 1; ii < aBoard->m_TrackWidthList.size(); ii++ )
        fprintf( aFile, "TrackWidthList %d\n", aBoard->m_TrackWidthList[ii] );


    fprintf( aFile, "TrackClearence %d\n", netclass_default->GetClearance() );
    fprintf( aFile, "ZoneClearence %d\n", g_Zone_Default_Setting.m_ZoneClearance );
    fprintf( aFile, "TrackMinWidth %d\n", aBoard->GetBoardDesignSettings()->m_TrackMinWidth );

    fprintf( aFile, "DrawSegmWidth %d\n", aBoard->GetBoardDesignSettings()->m_DrawSegmentWidth );
    fprintf( aFile, "EdgeSegmWidth %d\n", aBoard->GetBoardDesignSettings()->m_EdgeSegmentWidth );

    // Save current default via size, for compatibility with older Pcbnew version;
    fprintf( aFile, "ViaSize %d\n", netclass_default->GetViaDiameter() );
    fprintf( aFile, "ViaDrill %d\n", netclass_default->GetViaDrill() );
    fprintf( aFile, "ViaMinSize %d\n", aBoard->GetBoardDesignSettings()->m_ViasMinSize );
    fprintf( aFile, "ViaMinDrill %d\n", aBoard->GetBoardDesignSettings()->m_ViasMinDrill );

    // Save custom vias diameters list (the first is not saved here: this is
    // the netclass value
    for( unsigned ii = 1; ii < aBoard->m_ViasDimensionsList.size(); ii++ )
        fprintf( aFile, "ViaSizeList %d %d\n",
                 aBoard->m_ViasDimensionsList[ii].m_Diameter,
                 aBoard->m_ViasDimensionsList[ii].m_Drill );

    // for old versions compatibility:
    fprintf( aFile, "MicroViaSize %d\n", netclass_default->GetuViaDiameter() );
    fprintf( aFile, "MicroViaDrill %d\n", netclass_default->GetuViaDrill() );
    fprintf( aFile,
             "MicroViasAllowed %d\n",
             aBoard->GetBoardDesignSettings()->m_MicroViasAllowed );
    fprintf( aFile,
             "MicroViaMinSize %d\n",
             aBoard->GetBoardDesignSettings()->m_MicroViasMinSize );
    fprintf( aFile,
             "MicroViaMinDrill %d\n",
             aBoard->GetBoardDesignSettings()->m_MicroViasMinDrill );

    fprintf( aFile, "TextPcbWidth %d\n", aBoard->GetBoardDesignSettings()->m_PcbTextWidth );
    fprintf( aFile,
             "TextPcbSize %d %d\n",
             aBoard->GetBoardDesignSettings()->m_PcbTextSize.x,
             aBoard->GetBoardDesignSettings()->m_PcbTextSize.y );

    fprintf( aFile, "EdgeModWidth %d\n", g_ModuleSegmentWidth );
    fprintf( aFile, "TextModSize %d %d\n", g_ModuleTextSize.x, g_ModuleTextSize.y );
    fprintf( aFile, "TextModWidth %d\n", g_ModuleTextWidth );
    fprintf( aFile, "PadSize %d %d\n", g_Pad_Master.m_Size.x, g_Pad_Master.m_Size.y );
    fprintf( aFile, "PadDrill %d\n", g_Pad_Master.m_Drill.x );
    fprintf( aFile,
             "Pad2MaskClearance %d\n",
             aBoard->GetBoardDesignSettings()->m_SolderMaskMargin );

    if( aBoard->GetBoardDesignSettings()->m_SolderPasteMargin != 0 )
        fprintf( aFile,
                 "Pad2PasteClearance %d\n",
                 aBoard->GetBoardDesignSettings()->m_SolderPasteMargin );

    if( aBoard->GetBoardDesignSettings()->m_SolderPasteMarginRatio != 0 )
        fprintf( aFile,
                 "Pad2PasteClearanceRatio %g\n",
                 aBoard->GetBoardDesignSettings()->m_SolderPasteMarginRatio );

    if ( aFrame->GetScreen()->m_GridOrigin != wxPoint( 0, 0 ) )
    {
        fprintf( aFile,
                 "GridOrigin %d %d\n",
                 aFrame->GetScreen()->m_GridOrigin.x,
                 aFrame->GetScreen()->m_GridOrigin.y );
    }

    fprintf( aFile,
             "AuxiliaryAxisOrg %d %d\n",
             aFrame->m_Auxiliary_Axis_Position.x,
             aFrame->m_Auxiliary_Axis_Position.y );

    STRING_FORMATTER sf;

    g_PcbPlotOptions.Format( &sf, 0 );

    wxString record = FROM_UTF8( sf.GetString().c_str() );
    record.Replace( wxT("\n"), wxT(""), true );
    record.Replace( wxT("  "), wxT(" "), true);
    fprintf( aFile, "PcbPlotParams %s\n", TO_UTF8( record ) );
    fprintf( aFile, "$EndSETUP\n\n" );
    return 1;
}


#endif


bool PCB_EDIT_FRAME::WriteGeneralDescrPcb( FILE* File )
{
    EDA_ITEM* PtStruct = GetBoard()->m_Modules;
    int       NbModules, NbDrawItem, NbLayers;

    /* Write copper layer count */
    NbLayers = GetBoard()->GetCopperLayerCount();
    fprintf( File, "$GENERAL\n" );
    fprintf( File, "encoding utf-8\n");
    fprintf( File, "LayerCount %d\n", NbLayers );

    // Write old format for Layer count (for compatibility with old versions of
    // pcbnew
    fprintf( File,
             "Ly %8X\n",
             g_TabAllCopperLayerMask[NbLayers - 1] | ALL_NO_CU_LAYERS );
    fprintf( File, "EnabledLayers %08X\n", GetBoard()->GetEnabledLayers() );
    fprintf( File, "Links %d\n", GetBoard()->GetRatsnestsCount() );
    fprintf( File, "NoConn %d\n", GetBoard()->m_NbNoconnect );

    /* Write Bounding box info */
    GetBoard()->ComputeBoundingBox();
    fprintf( File, "Di %d %d %d %d\n",
             GetBoard()->m_BoundaryBox.GetX(),
             GetBoard()->m_BoundaryBox.GetY(),
             GetBoard()->m_BoundaryBox.GetRight(),
             GetBoard()->m_BoundaryBox.GetBottom() );

    /* Write segment count for footprints, drawings, track and zones */
    /* Calculate the footprint count */
    for( NbModules = 0; PtStruct != NULL; PtStruct = PtStruct->Next() )
        NbModules++;

    PtStruct = GetBoard()->m_Drawings; NbDrawItem = 0;

    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
        NbDrawItem++;

    fprintf( File, "Ndraw %d\n", NbDrawItem );
    fprintf( File, "Ntrack %d\n", GetBoard()->GetNumSegmTrack() );
    fprintf( File, "Nzone %d\n", GetBoard()->GetNumSegmZone() );
    fprintf( File, "BoardThickness %d\n", GetBoard()->GetBoardDesignSettings()->m_BoardThickness );
    fprintf( File, "Nmodule %d\n", NbModules );
    fprintf( File, "Nnets %d\n", GetBoard()->m_NetInfo->GetCount() );
    fprintf( File, "$EndGENERAL\n\n" );
    return true;
}


/**
 * Function WriteSheetDescr
 * Save the page information (size, texts, date ..)
 * @param screen BASE_SCREEN to save
 * @param File = an open FILE to write info
 */
bool WriteSheetDescr( BASE_SCREEN* screen, FILE* File )
{
    Ki_PageDescr* sheet = screen->m_CurrentSheetDesc;

    fprintf( File, "$SHEETDESCR\n" );
    fprintf( File, "Sheet %s %d %d\n",
             TO_UTF8( sheet->m_Name ), sheet->m_Size.x, sheet->m_Size.y );
    fprintf( File, "Title %s\n",        EscapedUTF8( screen->m_Title ).c_str() );
    fprintf( File, "Date %s\n",         EscapedUTF8( screen->m_Date ).c_str() );
    fprintf( File, "Rev %s\n",          EscapedUTF8( screen->m_Revision ).c_str() );
    fprintf( File, "Comp %s\n",         EscapedUTF8( screen->m_Company ).c_str() );
    fprintf( File, "Comment1 %s\n",     EscapedUTF8( screen->m_Commentaire1 ).c_str() );
    fprintf( File, "Comment2 %s\n",     EscapedUTF8( screen->m_Commentaire2 ).c_str() );
    fprintf( File, "Comment3 %s\n",     EscapedUTF8( screen->m_Commentaire3 ).c_str() );
    fprintf( File, "Comment4 %s\n",     EscapedUTF8( screen->m_Commentaire4 ).c_str() );

    fprintf( File, "$EndSHEETDESCR\n\n" );
    return true;
}


static bool ReadSheetDescr( BASE_SCREEN* screen, LINE_READER* aReader )
{
    char* Line, buf[1024], * text;

    while(  aReader->ReadLine() )
    {
        Line = aReader->Line();

        if( strnicmp( Line, "$End", 4 ) == 0 )
            return true;

        if( strnicmp( Line, "Sheet", 4 ) == 0 )
        {
            text = strtok( Line, " \t\n\r" );
            text = strtok( NULL, " \t\n\r" );
            Ki_PageDescr* sheet = g_SheetSizeList[0];
            int           ii;

            for( ii = 0; sheet != NULL; ii++, sheet = g_SheetSizeList[ii] )
            {
                if( stricmp( TO_UTF8( sheet->m_Name ), text ) == 0 )
                {
                    screen->m_CurrentSheetDesc = sheet;

                    if( sheet == &g_Sheet_user )
                    {
                        text = strtok( NULL, " \t\n\r" );

                        if( text )
                            sheet->m_Size.x = atoi( text );

                        text = strtok( NULL, " \t\n\r" );

                        if( text )
                            sheet->m_Size.y = atoi( text );
                    }

                    break;
                }
            }

            continue;
        }

        if( strnicmp( Line, "Title", 2 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Title = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Date", 2 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Date = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Rev", 2 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Revision = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comp", 4 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Company = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment1", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Commentaire1 = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment2", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Commentaire2 = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment3", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Commentaire3 = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment4", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Commentaire4 = FROM_UTF8( buf );
            continue;
        }
    }

    return false;
}


int PCB_EDIT_FRAME::ReadPcbFile( LINE_READER* aReader, bool Append )
{
    char*        Line;

    wxBusyCursor dummy;

    // Switch the locale to standard C (needed to read floating point numbers
    // like 1.3)
    SetLocaleTo_C_standard();

    BOARD* board = GetBoard();

    NbDraw = NbTrack = NbZone = NbMod = NbNets = -1;

    board->m_Status_Pcb = 0;
    board->m_NetClasses.Clear();

    // Put a dollar sign in front, and test for a specific length of characters
    // The -1 is to omit the trailing \0 which is included in sizeof() on a
    // string.
#define TESTLINE( x ) (strncmp( Line, "$" x, sizeof("$" x) - 1 ) == 0)

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();
        // put the more frequent ones at the top

        if( TESTLINE( "MODULE" ) )
        {
            MODULE* Module = new MODULE( board );

            if( Module == NULL )
                continue;

            board->Add( Module, ADD_APPEND );
            Module->ReadDescr( aReader );
            continue;
        }

        if( TESTLINE( "DRAWSEGMENT" ) )
        {
            DRAWSEGMENT* DrawSegm = new DRAWSEGMENT( board );
            board->Add( DrawSegm, ADD_APPEND );
            DrawSegm->ReadDrawSegmentDescr( aReader );
            continue;
        }

        if( TESTLINE( "EQUIPOT" ) )
        {
            NETINFO_ITEM* net = new NETINFO_ITEM( board );
            board->m_NetInfo->AppendNet( net );
            net->ReadDescr( aReader );
            continue;
        }

        if( TESTLINE( "TEXTPCB" ) )
        {
            TEXTE_PCB* pcbtxt = new TEXTE_PCB( board );
            board->Add( pcbtxt, ADD_APPEND );
            pcbtxt->ReadTextePcbDescr( aReader );
            continue;
        }

        if( TESTLINE( "TRACK" ) )
        {

#ifdef PCBNEW
            TRACK* insertBeforeMe = Append ? NULL : board->m_Track.GetFirst();
            ReadListeSegmentDescr( aReader, insertBeforeMe, TYPE_TRACK, NbTrack );
#endif

            continue;
        }

        if( TESTLINE( BRD_NETCLASS ) )
        {
            // create an empty NETCLASS without a name.
            NETCLASS* netclass = new NETCLASS( board, wxEmptyString );

            // fill it from the *.brd file, and establish its name.
            netclass->ReadDescr( aReader );

            if( !board->m_NetClasses.Add( netclass ) )
            {
                // Must have been a name conflict, this is a bad board file.
                // User may have done a hand edit to the file.
                // Delete netclass if board could not take ownership of it.
                delete netclass;

                // @todo: throw an exception here, this is a bad board file.
            }

            continue;
        }

        if( TESTLINE( "CZONE_OUTLINE" ) )
        {
            ZONE_CONTAINER* zone_descr = new ZONE_CONTAINER( board );
            zone_descr->ReadDescr( aReader );
            if( zone_descr->GetNumCorners() > 2 )       // should always occur
                board->Add( zone_descr );
            else
                delete zone_descr;
            continue;
        }

        if( TESTLINE( "COTATION" ) )
        {
            DIMENSION* Dimension = new DIMENSION( board );
            board->Add( Dimension, ADD_APPEND );
            Dimension->ReadDimensionDescr( aReader );
            continue;
        }

        if( TESTLINE( "PCB_TARGET" ) )
        {
            PCB_TARGET* Mire = new PCB_TARGET( board );
            board->Add( Mire, ADD_APPEND );
            Mire->ReadMirePcbDescr( aReader );
            continue;
        }

        if( TESTLINE( "ZONE" ) )
        {

#ifdef PCBNEW
            SEGZONE* insertBeforeMe = Append ? NULL : board->m_Zone.GetFirst();

            ReadListeSegmentDescr( aReader, insertBeforeMe, TYPE_ZONE, NbZone );
#endif

            continue;
        }

        if( TESTLINE( "GENERAL" ) )
        {
            ReadGeneralDescrPcb( aReader );
            continue;
        }

        if( TESTLINE( "SHEETDESCR" ) )
        {
            ReadSheetDescr( GetScreen(), aReader );
            continue;
        }

        if( TESTLINE( "SETUP" ) )
        {
            if( !Append )
            {
                ReadSetup( aReader );
            }
            else
            {
                while( aReader->ReadLine() )
                {
                    Line = aReader->Line();

                    if( TESTLINE( "EndSETUP" ) )
                        break;
                }
            }

            continue;
        }

        if( TESTLINE( "EndPCB" ) )
            break;
    }

    SetLocaleTo_Default();       // revert to the current  locale

    GetBoard()->m_Status_Pcb = 0;

    // Build the net info list
    GetBoard()->m_NetInfo->BuildListOfNets();

    board->SynchronizeNetsAndNetClasses();

    SetStatusText( wxEmptyString );
    BestZoom();
    return 1;
}


#ifdef PCBNEW

/* Save the current PCB in ASCII format
 * Returns
 * 1 if OK
 * 0 if error occurs saving file.
 */
int PCB_EDIT_FRAME::SavePcbFormatAscii( FILE* aFile )
{
    bool rc;
    char line[256];

    GetBoard()->m_Status_Pcb &= ~CONNEXION_OK;

    wxBeginBusyCursor();

    // Switch the locale to standard C (needed to print floating point numbers
    // like 1.3)
    SetLocaleTo_C_standard();

    /* Writing file header. */
    fprintf( aFile, "PCBNEW-BOARD Version %d date %s\n\n", g_CurrentVersionPCB,
             DateAndTime( line ) );
    fprintf( aFile, "# Created by Pcbnew%s\n\n", TO_UTF8( GetBuildVersion() ) );

    GetBoard()->SynchronizeNetsAndNetClasses();

    // Select default Netclass before writing file.
    // Useful to save default values in headers
    GetBoard()->SetCurrentNetClass( GetBoard()->m_NetClasses.GetDefault()->GetName() );

    WriteGeneralDescrPcb( aFile );
    WriteSheetDescr( GetScreen(), aFile );
    WriteSetup( aFile, this, GetBoard() );

    rc = GetBoard()->Save( aFile );

    SetLocaleTo_Default();       // revert to the current locale
    wxEndBusyCursor();

    if( !rc )
        DisplayError( this, wxT( "Unable to save PCB file" ) );
    else
        SetStatusText( wxEmptyString );

    return rc;
}


#endif
