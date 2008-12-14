
/********************************************************************/
/* Routines de lecture et sauvegarde des structures en format ASCii */
/*  Fichier common a PCBNEW et CVPCB								*/
/********************************************************************/

/* ioascii.cpp */

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"

#ifdef PCBNEW
#include "pcbnew.h"
#include "autorout.h"
#include "zones.h"
#endif

#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "id.h"


/* Format des structures de sauvegarde type ASCII :

 Structure PAD:

 $PAD
 Sh "name" forme dimv dimH dV dH orient  :forme generale dV, dH = delta dimensions
 Dr diam, dV dH					:drill : diametre offsets de percage
 At type S/N layers				: type standard,cms,conn,hole,meca.,
                             Stack/Normal,
                             Hexadecimal 32 bits: occupation des couches
 Nm net_code netname
 Po posrefX posrefy :			position refX,Y (= position orient 0 / ancre)
 $EndPAD

****** Structure module ***********

 $MODULE namelib
 Po ax ay orient layer masquelayer m_TimeCode
                                     ax ay = coord ancre (position module)
                                     orient = orient en 0.1 degre
                                     layer = numero de couche
                                     masquelayer = couche pour serigraphie
                                     m_TimeCode a usage interne (groupements)
 Li <namelib>

 Cd <text>						Description du composant (Composant Doc)
 Kw <text>						Liste des mots cle

 Sc schematimestamp						de reference schematique

 Op rot90 rot180					Options de placement auto (cout rot 90, 180 )
                             rot90 est sur 2x4 bits:
                             lsb = cout rot 90, msb = cout rot -90;

 Tn px py dimv dimh orient epaisseur miroir visible "texte"
                             n = type (0 = ref, 1 = val, > 1 =qcq
                             Textes POS x,y / ancre et orient module 0
                             dimv dimh orient
                             epaisseur miroir (Normal/miroir)
                             visible V/I
 DS ox oy fx fy w
                             edge: segment coord ox,oy a fx,fy, relatives
                             a l'ancre et orient 0
                             epaisseur w
 DC ox oy fx fy w				descr cercle (centre, 1 point, epaisseur)
 $PAD
 $EndPAD							section pads s'il y en a
 $EndMODULE
*/

extern Ki_PageDescr* SheetList[];

/* Local Variables */
int NbDraw, NbTrack, NbZone, NbMod, NbNets;


/**********************************************************************/
int WinEDA_BasePcbFrame::ReadListeSegmentDescr( FILE* File,
       TRACK* insertBeforeMe, int StructType, int* LineNum, int NumSegm )
/**********************************************************************/

/** Read a list of segments (Tracks, zones)
 * @return items count or - count if no end block ($End...) found.
 */
{
    int             shape, width, drill, layer, type, flags, net_code;
    int             ii = 0;
    char            line1[256];
    char            line2[256];

    TRACK*          newTrack;

    while( GetLine( File, line1, LineNum ) )
    {
        int             makeType;
        unsigned long   timeStamp;

        if( line1[0] == '$' )
        {
            return ii;      /* end of segmentlist: OK */
        }

        // Read the 2nd line to determine the exact type, one of:
        // TYPE_TRACK, TYPE_VIA, or TYPE_ZONE.  The type field in 2nd line
        // differentiates between TYPE_TRACK and TYPE_VIA.  With virtual
        // functions in use, it is critical to instantiate the TYPE_VIA exactly.
        if( GetLine( File, line2, LineNum ) == NULL )
            break;

        if( line2[0] == '$' )
            break;

        // parse the 2nd line first to determine the type of object
        sscanf( line2 + 2, " %d %d %d %lX %X", &layer, &type, &net_code,
                &timeStamp, &flags );

        if( StructType==TYPE_TRACK && type==1 )
            makeType = TYPE_VIA;
        else
            makeType = StructType;

        switch( makeType )
        {
        default:
        case TYPE_TRACK:
            newTrack = new TRACK( m_Pcb );
            m_Pcb->m_Track.Insert( newTrack, insertBeforeMe );
            break;

        case TYPE_VIA:
            newTrack = new SEGVIA( m_Pcb );
            m_Pcb->m_Track.Insert( newTrack, insertBeforeMe );
            break;

        case TYPE_ZONE:
            newTrack = new SEGZONE( m_Pcb );
            m_Pcb->m_Zone.Insert( (SEGZONE*)newTrack, (SEGZONE*)insertBeforeMe );
            break;
        }

        newTrack->m_TimeStamp = timeStamp;

        int arg_count = sscanf( line1 + 2, " %d %d %d %d %d %d %d", &shape,
                                &newTrack->m_Start.x, &newTrack->m_Start.y,
                                &newTrack->m_End.x, &newTrack->m_End.y, &width,
                                &drill );

        newTrack->m_Width = width;
        newTrack->m_Shape = shape;

        if( arg_count < 7 || drill <= 0 )
            newTrack->SetDrillDefault();
        else
            newTrack->SetDrillValue(drill);

        newTrack->SetLayer( layer );
        newTrack->SetNet( net_code );
        newTrack->SetState( flags, ON );
    }

    DisplayError( this, _( "Error: Unexpected end of file !" ) );
    return -ii;
}


/**********************************************************************************/
int WinEDA_BasePcbFrame::ReadGeneralDescrPcb( FILE* File, int* LineNum )
/**********************************************************************************/
{
    char         Line[1024], * data;

    while(  GetLine( File, Line, LineNum ) != NULL )
    {
        data = strtok( Line, " =\n\r" );
        if( strnicmp( data, "$EndGENERAL", 10 ) == 0 )
            break;

        if( strncmp( data, "Ly", 2 ) == 0 )    // Old format for Layer count
        {
            int Masque_Layer = 1, ii;
            data = strtok( NULL, " =\n\r" );
            sscanf( data, "%X", &Masque_Layer );

            // Setup layer count
            m_Pcb->m_BoardSettings->m_CopperLayerCount = 0;
            for( ii = 0; ii < NB_COPPER_LAYERS; ii++ )
            {
                if( Masque_Layer & 1 )
                    m_Pcb->m_BoardSettings->m_CopperLayerCount++;
                Masque_Layer >>= 1;
            }

            continue;
        }

        if( strnicmp( data, "Links", 5 ) == 0 )
        {
            data = strtok( NULL, " =\n\r" );
            m_Pcb->m_NbLinks = atoi( data );
            continue;
        }

        if( strnicmp( data, "NoConn", 6 ) == 0 )
        {
            data = strtok( NULL, " =\n\r" );
            m_Pcb->m_NbNoconnect = atoi( data );
            continue;
        }

        if( strnicmp( data, "Di", 2 ) == 0 )
        {
            wxSize pcbsize, screensize;
            data = strtok( NULL, " =\n\r" );
            m_Pcb->m_BoundaryBox.SetX( atoi( data ) );
            data = strtok( NULL, " =\n\r" );
            m_Pcb->m_BoundaryBox.SetY( atoi( data ) );
            data = strtok( NULL, " =\n\r" );
            m_Pcb->m_BoundaryBox.SetWidth( atoi( data ) - m_Pcb->m_BoundaryBox.GetX() );
            data = strtok( NULL, " =\n\r" );
            m_Pcb->m_BoundaryBox.SetHeight( atoi( data ) - m_Pcb->m_BoundaryBox.GetY() );
            continue;
        }

        /* Lecture du nombre de segments type DRAW , TRACT, ZONE */
        if( stricmp( data, "Ndraw" ) == 0 )
        {
            data   = strtok( NULL, " =\n\r" );
            NbDraw = atoi( data );;
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


/*************************************************************/
int WinEDA_BasePcbFrame::ReadSetup( FILE* File, int* LineNum )
/*************************************************************/
{
    char Line[1024], * data;

    while(  GetLine( File, Line, LineNum ) != NULL )
    {
        strtok( Line, " =\n\r" );
        data = strtok( NULL, " =\n\r" );

        if( stricmp( Line, "$EndSETUP" ) == 0 )
        {
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
            m_Pcb->m_BoardSettings->m_CopperLayerCount = tmp;
            continue;
        }

        const int LAYERKEYZ = sizeof("Layer[")-1;

        if( strncmp( Line, "Layer[", LAYERKEYZ ) == 0 )
        {
            // parse:
            // Layer[n]  <a_Layer_name_with_no_spaces> <LAYER_T>

            char* cp = Line + LAYERKEYZ;
            int layer = atoi(cp);

            if( data )
            {
                wxString layerName = CONV_FROM_UTF8( data );
                m_Pcb->SetLayerName( layer, layerName );

                data = strtok( NULL, " " );
                if( data )
                {
                    LAYER_T type = LAYER::ParseType( data );
                    m_Pcb->SetLayerType( layer, type );
                }
            }
            continue;
        }

        if( stricmp( Line, "TrackWidth" ) == 0 )
        {
            g_DesignSettings.m_CurrentTrackWidth = atoi( data );
            AddHistory( g_DesignSettings.m_CurrentTrackWidth, TYPE_TRACK );
            continue;
        }

        if( stricmp( Line, "TrackWidthHistory" ) == 0 )
        {
            int tmp = atoi( data );
            AddHistory( tmp, TYPE_TRACK );
            continue;
        }

        if( stricmp( Line, "TrackClearence" ) == 0 )
        {
            g_DesignSettings.m_TrackClearence = atoi( data );
            continue;
        }

        if( stricmp( Line, "ZoneClearence" ) == 0 )
        {
            g_Zone_Default_Setting.m_ZoneClearance = atoi( data );
            continue;
        }

        if( stricmp( Line, "ZoneGridSize" ) == 0 )
        {
            g_GridRoutingSize = atoi( data );
            continue;
        }

        if( stricmp( Line, "UserGridSize" ) == 0 )
        {
            wxString msg;
            if( data )
            {
                msg = CONV_FROM_UTF8( data );
                msg.ToDouble( &g_UserGrid.x );
            }
            else
                continue;

            data = strtok( NULL, " =\n\r" );
            if( data )
            {
                msg = CONV_FROM_UTF8( data );
                msg.ToDouble( &g_UserGrid.y );
            }
            else
                g_UserGrid.y = g_UserGrid.x;

            data = strtok( NULL, " =\n\r" );
            if( data )
            {
                if( stricmp( data, "mm" ) == 0 )
                    g_UserGrid_Unit = MILLIMETRE;
                else
                    g_UserGrid_Unit = INCHES;
                GetScreen()->AddGrid( g_UserGrid, g_UserGrid_Unit,
                                      ID_POPUP_GRID_USER );
            }
            continue;
        }

        if( stricmp( Line, "DrawSegmWidth" ) == 0 )
        {
            g_DesignSettings.m_DrawSegmentWidth = atoi( data );
            continue;
        }

        if( stricmp( Line, "EdgeSegmWidth" ) == 0 )
        {
            g_DesignSettings.m_EdgeSegmentWidth = atoi( data );
            continue;
        }

        if( stricmp( Line, "ViaSize" ) == 0 )
        {
            g_DesignSettings.m_CurrentViaSize = atoi( data );
            AddHistory( g_DesignSettings.m_CurrentViaSize, TYPE_VIA );
            continue;
        }

        if( stricmp( Line, "MicroViaSize" ) == 0 )
        {
            g_DesignSettings.m_CurrentMicroViaSize = atoi( data );
            continue;
        }

        if( stricmp( Line, "ViaSizeHistory" ) == 0 )
        {
            int tmp = atoi( data );
            AddHistory( tmp, TYPE_VIA );
            continue;
        }

        if( stricmp( Line, "ViaDrill" ) == 0 )
        {
            g_DesignSettings.m_ViaDrill = atoi( data );
            continue;
        }

        if( stricmp( Line, "MicroViaDrill" ) == 0 )
        {
            g_DesignSettings.m_MicroViaDrill = atoi( data );
            continue;
        }

        if( stricmp( Line, "MicroViasAllowed" ) == 0 )
        {
            g_DesignSettings.m_MicroViasAllowed = atoi( data );
            continue;
        }

        if( stricmp( Line, "TextPcbWidth" ) == 0 )
        {
            g_DesignSettings.m_PcbTextWidth = atoi( data );
            continue;
        }

        if( stricmp( Line, "TextPcbSize" ) == 0 )
        {
            g_DesignSettings.m_PcbTextSize.x = atoi( data );
            data = strtok( NULL, " =\n\r" );
            g_DesignSettings.m_PcbTextSize.y = atoi( data );
            continue;
        }

        if( stricmp( Line, "EdgeModWidth" ) == 0 )
        {
            ModuleSegmentWidth = atoi( data );
            continue;
        }

        if( stricmp( Line, "TextModWidth" ) == 0 )
        {
            ModuleTextWidth = atoi( data );
            continue;
        }

        if( stricmp( Line, "TextModSize" ) == 0 )
        {
            ModuleTextSize.x = atoi( data );
            data = strtok( NULL, " =\n\r" );
            ModuleTextSize.y = atoi( data );
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
#endif
    }

    return 1;
}


#ifdef PCBNEW
/******************************************************************************/
static int WriteSetup( FILE* aFile, WinEDA_BasePcbFrame* aFrame, BOARD* aBoard )
/******************************************************************************/
{
    char text[1024];
    int  ii;

    fprintf( aFile, "$SETUP\n" );
    sprintf( text, "InternalUnit %f INCH\n", 1.0 / PCB_INTERNAL_UNIT );
    fprintf( aFile, text );

    sprintf( text, "UserGridSize %lf %lf %s\n", g_UserGrid.x, g_UserGrid.y,
             ( g_UserGrid_Unit == 0 ) ? "INCH" : "mm" );
    fprintf( aFile, text );

    fprintf( aFile, "ZoneGridSize %d\n", g_GridRoutingSize );

    fprintf( aFile, "Layers %d\n", aBoard->GetCopperLayerCount() );

    unsigned layerMask = g_TabAllCopperLayerMask[aBoard->GetCopperLayerCount()-1];

    for( int layer=0;  layerMask;  ++layer, layerMask>>=1 )
    {
        if( layerMask & 1 )
        {
            fprintf( aFile, "Layer[%d] %s %s\n", layer,
                    CONV_TO_UTF8( aBoard->GetLayerName(layer) ),
                    LAYER::ShowType( aBoard->GetLayerType( layer ) ) );
        }
    }

    fprintf( aFile, "TrackWidth %d\n", g_DesignSettings.m_CurrentTrackWidth );
    for( int ii = 0; ii < HISTORY_NUMBER; ii++ )
    {
        if( g_DesignSettings.m_TrackWidthHistory[ii] == 0 )
            break;
        fprintf( aFile, "TrackWidthHistory %d\n",
                 g_DesignSettings.m_TrackWidthHistory[ii] );
    }

    fprintf( aFile, "TrackClearence %d\n", g_DesignSettings.m_TrackClearence );
    fprintf( aFile, "ZoneClearence %d\n", g_Zone_Default_Setting.m_ZoneClearance );

    fprintf( aFile, "DrawSegmWidth %d\n", g_DesignSettings.m_DrawSegmentWidth );
    fprintf( aFile, "EdgeSegmWidth %d\n", g_DesignSettings.m_EdgeSegmentWidth );
    fprintf( aFile, "ViaSize %d\n", g_DesignSettings.m_CurrentViaSize );
    fprintf( aFile, "ViaDrill %d\n", g_DesignSettings.m_ViaDrill );

    for( ii = 0; ii < HISTORY_NUMBER; ii++ )
    {
        if( g_DesignSettings.m_ViaSizeHistory[ii] == 0 )
            break;
        fprintf( aFile, "ViaSizeHistory %d\n", g_DesignSettings.m_ViaSizeHistory[ii] );
    }

    fprintf( aFile, "MicroViaSize %d\n", g_DesignSettings.m_CurrentMicroViaSize);
    fprintf( aFile, "MicroViaDrill %d\n", g_DesignSettings.m_MicroViaDrill);
    fprintf( aFile, "MicroViasAllowed %d\n", g_DesignSettings.m_MicroViasAllowed);

    fprintf( aFile, "TextPcbWidth %d\n", g_DesignSettings.m_PcbTextWidth );
    fprintf( aFile, "TextPcbSize %d %d\n",
             g_DesignSettings.m_PcbTextSize.x, g_DesignSettings.m_PcbTextSize.y );

    fprintf( aFile, "EdgeModWidth %d\n", ModuleSegmentWidth );
    fprintf( aFile, "TextModSize %d %d\n", ModuleTextSize.x, ModuleTextSize.y );
    fprintf( aFile, "TextModWidth %d\n", ModuleTextWidth );
    fprintf( aFile, "PadSize %d %d\n", g_Pad_Master.m_Size.x, g_Pad_Master.m_Size.y );
    fprintf( aFile, "PadDrill %d\n", g_Pad_Master.m_Drill.x );

    fprintf( aFile, "AuxiliaryAxisOrg %d %d\n",
             aFrame->m_Auxiliary_Axis_Position.x, aFrame->m_Auxiliary_Axis_Position.y );

    fprintf( aFile, "$EndSETUP\n\n" );
    return 1;
}

#endif


/******************************************************/
bool WinEDA_PcbFrame::WriteGeneralDescrPcb( FILE* File )
/******************************************************/
{
    EDA_BaseStruct* PtStruct = m_Pcb->m_Modules;
    int             NbModules, NbDrawItem, NbLayers;

    /* Write copper layer count */
    NbLayers = m_Pcb->m_BoardSettings->m_CopperLayerCount;
    fprintf( File, "$GENERAL\n" );
    fprintf( File, "LayerCount %d\n", NbLayers );

    // Write old format for Layer count (for compatibility with old versions of pcbnew
    fprintf( File, "Ly %8X\n", g_TabAllCopperLayerMask[NbLayers - 1] | ALL_NO_CU_LAYERS ); // For compatibility with old version of pcbnew
    fprintf( File, "Links %d\n", m_Pcb->m_NbLinks );
    fprintf( File, "NoConn %d\n", m_Pcb->m_NbNoconnect );

    /* Write Bounding box info */
    m_Pcb->ComputeBoundaryBox();
    fprintf( File, "Di %d %d %d %d\n",
            m_Pcb->m_BoundaryBox.GetX(), m_Pcb->m_BoundaryBox.GetY(),
            m_Pcb->m_BoundaryBox.GetRight(),
            m_Pcb->m_BoundaryBox.GetBottom() );

    /* Write segment count for footprints, drawings, track and zones */
    /* Calculate the footprint count */
    for( NbModules = 0; PtStruct != NULL; PtStruct = PtStruct->Next() )
        NbModules++;

    PtStruct = m_Pcb->m_Drawings; NbDrawItem = 0;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
        NbDrawItem++;

    fprintf( File, "Ndraw %d\n", NbDrawItem );
    fprintf( File, "Ntrack %d\n", m_Pcb->GetNumSegmTrack() );
    fprintf( File, "Nzone %d\n", m_Pcb->GetNumSegmZone() );

    fprintf( File, "Nmodule %d\n", NbModules );
    fprintf( File, "Nnets %d\n", m_Pcb->m_Equipots.GetCount() );

    fprintf( File, "$EndGENERAL\n\n" );
    return TRUE;
}


/******************************************************/
bool WriteSheetDescr( BASE_SCREEN* screen, FILE* File )
/******************************************************/
/** Function WriteSheetDescr
*  Save the page information (size, texts, date ..)
* @param screen BASE_SCREEN to save
 * @param File = an openen FILE to write info
*/
{
    Ki_PageDescr* sheet = screen->m_CurrentSheetDesc;

    fprintf( File, "$SHEETDESCR\n" );
    fprintf( File, "Sheet %s %d %d\n",
             CONV_TO_UTF8( sheet->m_Name ), sheet->m_Size.x, sheet->m_Size.y );
    fprintf( File, "Title \"%s\"\n", CONV_TO_UTF8( screen->m_Title ) );
    fprintf( File, "Date \"%s\"\n", CONV_TO_UTF8( screen->m_Date ) );
    fprintf( File, "Rev \"%s\"\n", CONV_TO_UTF8( screen->m_Revision ) );
    fprintf( File, "Comp \"%s\"\n", CONV_TO_UTF8( screen->m_Company ) );
    fprintf( File, "Comment1 \"%s\"\n", CONV_TO_UTF8( screen->m_Commentaire1 ) );
    fprintf( File, "Comment2 \"%s\"\n", CONV_TO_UTF8( screen->m_Commentaire2 ) );
    fprintf( File, "Comment3 \"%s\"\n", CONV_TO_UTF8( screen->m_Commentaire3 ) );
    fprintf( File, "Comment4 \"%s\"\n", CONV_TO_UTF8( screen->m_Commentaire4 ) );

    fprintf( File, "$EndSHEETDESCR\n\n" );
    return TRUE;
}


/***************************************************************************/
static bool ReadSheetDescr( BASE_SCREEN* screen, FILE* File, int* LineNum )
/***************************************************************************/
{
    char Line[1024], buf[1024], * text;

    while(  GetLine( File, Line, LineNum ) != NULL )
    {
        if( strnicmp( Line, "$End", 4 ) == 0 )
            return TRUE;

        if( strnicmp( Line, "Sheet", 4 ) == 0 )
        {
            text = strtok( Line, " \t\n\r" );
            text = strtok( NULL, " \t\n\r" );
            Ki_PageDescr* sheet = SheetList[0];
            int           ii;
            for( ii = 0; sheet != NULL; ii++, sheet = SheetList[ii] )
            {
                if( stricmp( CONV_TO_UTF8( sheet->m_Name ), text ) == 0 )
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
            screen->m_Title = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Date", 2 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Date = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Rev", 2 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Revision = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comp", 4 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Company = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment1", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Commentaire1 = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment2", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Commentaire2 = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment3", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Commentaire3 = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment4", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            screen->m_Commentaire4 = CONV_FROM_UTF8( buf );
            continue;
        }
    }

    return FALSE;
}


/********************************************************************/
int WinEDA_PcbFrame::ReadPcbFile( FILE* File, bool Append )
/********************************************************************/

/** ReadPcbFile
 * Read a board file  <file>.brd
 * @param Append if 0: a previoulsy loaded boar is delete before loadin the file.
 *  else all items of the board file are added to the existing board
 */
{
    char            Line[1024];
    int             LineNum = 0;

    wxBusyCursor    dummy;

    // Switch the locale to standard C (needed to read floating point numbers like 1.3)
    SetLocaleTo_C_standard( );

    NbDraw = NbTrack = NbZone = NbMod = NbNets = -1;
    m_Pcb->m_Status_Pcb = 0;

    while( GetLine( File, Line, &LineNum ) != NULL )
    {
        if( strnicmp( Line, "$EndPCB", 6 ) == 0 )
            break;

        if( strnicmp( Line, "$GENERAL", 8 ) == 0 )
        {
            ReadGeneralDescrPcb( File, &LineNum );
            continue;
        }

        if( strnicmp( Line, "$SHEETDESCR", 11 ) == 0 )
        {
            ReadSheetDescr( GetScreen(), File, &LineNum );
            continue;
        }

        if( strnicmp( Line, "$SETUP", 6 ) == 0 )
        {
            if( !Append )
            {
                ReadSetup( File, &LineNum );
            }
            else
            {
                while( GetLine( File, Line, &LineNum ) != NULL )
                    if( strnicmp( Line, "$EndSETUP", 6 ) == 0 )
                        break;
            }
            continue;
        }

        if( strnicmp( Line, "$EQUIPOT", 7 ) == 0 )
        {
            EQUIPOT* Equipot = new EQUIPOT( m_Pcb );
            m_Pcb->m_Equipots.PushBack( Equipot );
            Equipot->ReadDescr( File, &LineNum );
            continue;
        }

        if( strnicmp( Line, "$CZONE_OUTLINE", 7 ) == 0 )
        {
            ZONE_CONTAINER * zone_descr = new ZONE_CONTAINER(m_Pcb);
            zone_descr->ReadDescr( File, &LineNum );
            if ( zone_descr->GetNumCorners( ) > 2 )     // should not occur
                m_Pcb->Add(zone_descr);
            else delete zone_descr;
            continue;
        }

        if( strnicmp( Line, "$MODULE", 7 ) == 0 )
        {
            MODULE* Module = new MODULE( m_Pcb );

            if( Module == NULL )
                continue;

            m_Pcb->Add( Module, ADD_APPEND );
            Module->ReadDescr( File, &LineNum );
            continue;
        }

        if( strnicmp( Line, "$TEXTPCB", 8 ) == 0 )
        {
            TEXTE_PCB* pcbtxt = new TEXTE_PCB( m_Pcb );
            m_Pcb->Add( pcbtxt, ADD_APPEND );
            pcbtxt->ReadTextePcbDescr( File, &LineNum );
            continue;
        }

        if( strnicmp( Line, "$DRAWSEGMENT", 10 ) == 0 )
        {
            DRAWSEGMENT* DrawSegm = new DRAWSEGMENT( m_Pcb );
            m_Pcb->Add( DrawSegm, ADD_APPEND );
            DrawSegm->ReadDrawSegmentDescr( File, &LineNum );
            continue;
        }

        if( strnicmp( Line, "$COTATION", 9 ) == 0 )
        {
            COTATION* Cotation = new COTATION( m_Pcb );
            m_Pcb->Add( Cotation, ADD_APPEND );
            Cotation->ReadCotationDescr( File, &LineNum );
            continue;
        }

        if( strnicmp( Line, "$MIREPCB", 8 ) == 0 )
        {
            MIREPCB* Mire = new MIREPCB( m_Pcb );
            m_Pcb->Add( Mire, ADD_APPEND );
            Mire->ReadMirePcbDescr( File, &LineNum );
            continue;
        }

        if( strnicmp( Line, "$TRACK", 6 ) == 0 )
        {
#ifdef PCBNEW
            TRACK* insertBeforeMe = Append ? NULL : m_Pcb->m_Track.GetFirst();
            ReadListeSegmentDescr( File, insertBeforeMe, TYPE_TRACK,
                                            &LineNum, NbTrack );
            D( m_Pcb->m_Track.VerifyListIntegrity(); )
#endif
            continue;
        }

        if( strnicmp( Line, "$ZONE", 5 ) == 0 )
        {
#ifdef PCBNEW
            SEGZONE* insertBeforeMe = Append ? NULL : m_Pcb->m_Zone.GetFirst();

            ReadListeSegmentDescr( File, insertBeforeMe, TYPE_ZONE,
                                            &LineNum, NbZone );
            D( m_Pcb->m_Zone.VerifyListIntegrity(); )
#endif
            continue;
        }
    }

    SetLocaleTo_Default( );      // revert to the current  locale

    Affiche_Message( wxEmptyString );

    BestZoom();

#ifdef PCBNEW
    // Build connectivity info
    Compile_Ratsnest( NULL, TRUE );
#endif
    return 1;
}


#ifdef PCBNEW
/***************************************************/
int WinEDA_PcbFrame::SavePcbFormatAscii( FILE* aFile )
/****************************************************/

/* Routine de sauvegarde du PCB courant sous format ASCII
 *  retourne
 *      1 si OK
 *      0 si sauvegarde non faite
 */
{
    bool    rc;
    char    line[256];

    m_Pcb->m_Status_Pcb &= ~CONNEXION_OK;

    wxBeginBusyCursor();

    // Switch the locale to standard C (needed to print floating point numbers like 1.3)
    SetLocaleTo_C_standard( );

    /* Ecriture de l'entete PCB : */
    fprintf( aFile, "PCBNEW-BOARD Version %d date %s\n\n", g_CurrentVersionPCB,
            DateAndTime( line ) );

    WriteGeneralDescrPcb( aFile );
    WriteSheetDescr( GetScreen(), aFile );
    WriteSetup( aFile, this, m_Pcb );

    rc = m_Pcb->Save( aFile );

    SetLocaleTo_Default( );      // revert to the current locale
    wxEndBusyCursor();

    if( !rc )
        DisplayError( this, wxT( "Unable to save PCB file" ) );
    else
        Affiche_Message( wxEmptyString );

    return rc;
}

#endif
