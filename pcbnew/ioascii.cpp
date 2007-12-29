
/********************************************************************/
/* Routines de lecture et sauvegarde des structures en format ASCii */
/*  Fichier common a PCBNEW et CVPCB								*/
/********************************************************************/

/* ioascii.cpp */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#ifdef PCBNEW
#include "pcbnew.h"
#include "autorout.h"
#endif

#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "protos.h"

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
       TRACK* PtSegm, int StructType, int* LineNum, int NumSegm )
/**********************************************************************/

/** Read a list of segments (Tracks, zones)
 * @return items count or - count if no end block ($End...) found.
 */
{
    int             shape, width, layer, type, flags, net_code;
    int             ii = 0;
    char            line1[256];
    char            line2[256];
    
    TRACK* NewTrack;

    while( GetLine( File, line1, LineNum ) )
    {
        int             makeType;
        unsigned long   timeStamp;
        
        if( line1[0] == '$' )
        {
            return ii;      /* end of segmentlist: OK */
        }

        // Read the 2nd line to determine the exact type, one of:
        // TYPETRACK, TYPEVIA, or TYPEZONE.  The type field in 2nd line
        // differentiates between TYPETRACK and TYPEVIA.  With virtual
        // functions in use, it is critical to instantiate the TYPEVIA exactly.        
        if( GetLine( File, line2, LineNum ) == NULL )
            break;
        
        if( line2[0] == '$' )
            break;

        // parse the 2nd line first to determine the type of object
        sscanf( line2 + 2, " %d %d %d %lX %X", &layer, &type, &net_code,
                &timeStamp, &flags );

        if( StructType==TYPETRACK && type==1 )
            makeType = TYPEVIA;
        else
            makeType = StructType;
        
        switch( makeType )
        {
        default:
        case TYPETRACK:
            NewTrack = new TRACK( m_Pcb );
            break;

        case TYPEVIA:
            NewTrack = new SEGVIA( m_Pcb );
            break;

        case TYPEZONE:
            NewTrack = new SEGZONE( m_Pcb );
            break;
        }

        NewTrack->Insert( m_Pcb, PtSegm );
        
        PtSegm = NewTrack;
        
        PtSegm->m_TimeStamp = timeStamp;

        int arg_count = sscanf( line1 + 2, " %d %d %d %d %d %d %d", &shape,
                                &PtSegm->m_Start.x, &PtSegm->m_Start.y,
                                &PtSegm->m_End.x, &PtSegm->m_End.y, &width,
                                &PtSegm->m_Drill );

        PtSegm->m_Width = width;
        PtSegm->m_Shape = shape;

        if( arg_count < 7 )
            PtSegm->m_Drill = -1;

        PtSegm->SetLayer( layer );
        PtSegm->SetNet( net_code ); 
        PtSegm->SetState( flags, ON );
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

        if( stricmp( Line, "TrackWidth" ) == 0 )
        {
            g_DesignSettings.m_CurrentTrackWidth = atoi( data );
            AddHistory( g_DesignSettings.m_CurrentTrackWidth, TYPETRACK );
            continue;
        }

        if( stricmp( Line, "TrackWidthHistory" ) == 0 )
        {
            int tmp = atoi( data );
            AddHistory( tmp, TYPETRACK );
            continue;
        }

        if( stricmp( Line, "TrackClearence" ) == 0 )
        {
            g_DesignSettings.m_TrackClearence = atoi( data );
            continue;
        }

        if( stricmp( Line, "ZoneClearence" ) == 0 )
        {
            g_DesignSettings.m_ZoneClearence = atoi( data );
            continue;
        }

        if( stricmp( Line, "GridSize" ) == 0 )
        {
            wxSize Grid;
            Grid.x = atoi( data );
            data   = strtok( NULL, " =\n\r" );
            if( data )
                Grid.y = atoi( data );
            else
                Grid.y = Grid.x;
            GetScreen()->SetGrid( Grid );
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
            
            GetScreen()->m_UserGrid = g_UserGrid;
            data = strtok( NULL, " =\n\r" );
            if( data )
            {
                if( stricmp( data, "mm" ) == 0 )
                    g_UserGrid_Unit = MILLIMETRE;
                else
                    g_UserGrid_Unit = INCHES;
                GetScreen()->m_UserGridUnit = g_UserGrid_Unit;
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
            AddHistory( g_DesignSettings.m_CurrentViaSize, TYPEVIA );
            continue;
        }

        if( stricmp( Line, "ViaSizeHistory" ) == 0 )
        {
            int tmp = atoi( data );
            AddHistory( tmp, TYPEVIA );
            continue;
        }

        if( stricmp( Line, "ViaDrill" ) == 0 )
        {
            g_DesignSettings.m_ViaDrill = atoi( data );
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
/***************************************************************/
static int WriteSetup( FILE* File, WinEDA_BasePcbFrame* frame )
/***************************************************************/
{
    char text[1024];
    int  ii, jj;

    fprintf( File, "$SETUP\n" );
    sprintf( text, "InternalUnit %f INCH\n", 1.0 / PCB_INTERNAL_UNIT );
    fprintf( File, text );

    if( frame->GetScreen()->m_UserGridIsON )
        ii = jj = -1;
    else
    {
        ii = frame->GetScreen()->GetGrid().x;
        jj = frame->GetScreen()->GetGrid().y;
    }
    
    sprintf( text, "GridSize %d %d\n", ii, jj );
    fprintf( File, text );

    sprintf( text, "UserGridSize %lf %lf %s\n",
             frame->GetScreen()->m_UserGrid.x, frame->GetScreen()->m_UserGrid.y,
             ( g_UserGrid_Unit == 0 ) ? "INCH" : "mm" );
    fprintf( File, text );

    fprintf( File, "ZoneGridSize %d\n", g_GridRoutingSize );

    fprintf( File, "Layers %d\n", g_DesignSettings.m_CopperLayerCount );
    fprintf( File, "TrackWidth %d\n", g_DesignSettings.m_CurrentTrackWidth );
    for( ii = 0; ii < HIST0RY_NUMBER; ii++ )
    {
        if( g_DesignSettings.m_TrackWidhtHistory[ii] == 0 )
            break;
        fprintf( File, "TrackWidthHistory %d\n",
                 g_DesignSettings.m_TrackWidhtHistory[ii] );
    }

    fprintf( File, "TrackClearence %d\n", g_DesignSettings.m_TrackClearence );
    fprintf( File, "ZoneClearence %d\n", g_DesignSettings.m_ZoneClearence );

    fprintf( File, "DrawSegmWidth %d\n", g_DesignSettings.m_DrawSegmentWidth );
    fprintf( File, "EdgeSegmWidth %d\n", g_DesignSettings.m_EdgeSegmentWidth );
    fprintf( File, "ViaSize %d\n", g_DesignSettings.m_CurrentViaSize );
    fprintf( File, "ViaDrill %d\n", g_DesignSettings.m_ViaDrill );
    for( ii = 0; ii < HIST0RY_NUMBER; ii++ )
    {
        if( g_DesignSettings.m_ViaSizeHistory[ii] == 0 )
            break;
        fprintf( File, "ViaSizeHistory %d\n", g_DesignSettings.m_ViaSizeHistory[ii] );
    }

    fprintf( File, "TextPcbWidth %d\n", g_DesignSettings.m_PcbTextWidth );
    fprintf( File, "TextPcbSize %d %d\n",
             g_DesignSettings.m_PcbTextSize.x, g_DesignSettings.m_PcbTextSize.y );
    
    fprintf( File, "EdgeModWidth %d\n", ModuleSegmentWidth );
    fprintf( File, "TextModSize %d %d\n", ModuleTextSize.x, ModuleTextSize.y );
    fprintf( File, "TextModWidth %d\n", ModuleTextWidth );
    fprintf( File, "PadSize %d %d\n", g_Pad_Master.m_Size.x, g_Pad_Master.m_Size.y );
    fprintf( File, "PadDrill %d\n", g_Pad_Master.m_Drill.x );

    fprintf( File, "AuxiliaryAxisOrg %d %d\n",
             frame->m_Auxiliary_Axis_Position.x, frame->m_Auxiliary_Axis_Position.y );
    
    fprintf( File, "$EndSETUP\n\n" );
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
    for( NbModules = 0; PtStruct != NULL; PtStruct = PtStruct->Pnext )
        NbModules++;

    PtStruct = m_Pcb->m_Drawings; NbDrawItem = 0;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
        NbDrawItem++;

    fprintf( File, "Ndraw %d\n", NbDrawItem );
    fprintf( File, "Ntrack %d\n", m_Pcb->GetNumSegmTrack() );
    fprintf( File, "Nzone %d\n", m_Pcb->GetNumSegmZone() );

    fprintf( File, "Nmodule %d\n", NbModules );
    fprintf( File, "Nnets %d\n", m_Pcb->m_NbNets );

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
    Ki_PageDescr* sheet = screen->m_CurrentSheet;

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
                    screen->m_CurrentSheet = sheet;
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
int WinEDA_PcbFrame::ReadPcbFile( wxDC* DC, FILE* File, bool Append )
/********************************************************************/

/** ReadPcbFile
 * Read a board file  <file>.brd
 * @param Append if 0: a previoulsy loaded boar is delete before loadin the file.
 *  else all items of the board file are added to the existing board
 */
{
    char            Line[1024];
    int             LineNum = 0;
    int             nbsegm, nbmod;
    BOARD_ITEM*     LastStructPcb = NULL, * StructPcb;
    MODULE*         LastModule    = NULL, * Module;
    EQUIPOT*        LastEquipot   = NULL, * Equipot;

    wxBusyCursor    dummy;

    // Switch the locale to standard C (needed to print floating point numbers like 1.3)
    setlocale( LC_NUMERIC, "C" );

    NbDraw = NbTrack = NbZone = NbMod = NbNets = -1;
    m_Pcb->m_NbNets     = 0;
    m_Pcb->m_Status_Pcb = 0;
    nbmod = 0;

    if( Append )
    {
        LastModule = m_Pcb->m_Modules;
        for( ; LastModule != NULL; LastModule = (MODULE*) LastModule->Pnext )
        {
            if( LastModule->Pnext == NULL )
                break;
        }

        LastStructPcb = m_Pcb->m_Drawings;
        for( ; LastStructPcb != NULL; LastStructPcb = LastStructPcb->Next() )
        {
            if( LastStructPcb->Pnext == NULL )
                break;
        }

        LastEquipot = m_Pcb->m_Equipots;
        for( ; LastEquipot != NULL; LastEquipot = (EQUIPOT*) LastEquipot->Pnext )
        {
            if( LastEquipot->Pnext == NULL )
                break;
        }
    }

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
            ReadSheetDescr( m_CurrentScreen, File, &LineNum );
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
            Equipot = new EQUIPOT( m_Pcb );
            Equipot->ReadEquipotDescr( File, &LineNum );
            if( LastEquipot == NULL )
            {
                m_Pcb->m_Equipots = Equipot;
                Equipot->Pback    = m_Pcb;
            }
            else
            {
                Equipot->Pback     = LastEquipot;
                LastEquipot->Pnext = Equipot;
            }
            LastEquipot = Equipot;
            m_Pcb->m_NbNets++;
            continue;
        }

        if( strnicmp( Line, "$CZONE_OUTLINE", 7 ) == 0 )
        {
			ZONE_CONTAINER * zone_descr = new ZONE_CONTAINER(m_Pcb);
			zone_descr->ReadDescr( File, &LineNum );
			m_Pcb->m_ZoneDescriptorList.push_back(zone_descr);
		}

        if( strnicmp( Line, "$MODULE", 7 ) == 0 )
        {
            Module = new MODULE( m_Pcb );
            if( Module == NULL )
                continue;
            Module->ReadDescr( File, &LineNum );

            if( LastModule == NULL )
            {
                m_Pcb->m_Modules = Module;
                Module->Pback    = m_Pcb;
            }
            else
            {
                Module->Pback     = LastModule;
                LastModule->Pnext = Module;
            }
            LastModule = Module;
            nbmod++;
            continue;
        }

        if( strnicmp( Line, "$TEXTPCB", 8 ) == 0 )
        {
            TEXTE_PCB* pcbtxt = new TEXTE_PCB( m_Pcb );
            StructPcb = pcbtxt;
            pcbtxt->ReadTextePcbDescr( File, &LineNum );
            if( LastStructPcb == NULL )
            {
                m_Pcb->m_Drawings = StructPcb;
                StructPcb->Pback  = m_Pcb;
            }
            else
            {
                StructPcb->Pback     = LastStructPcb;
                LastStructPcb->Pnext = StructPcb;
            }
            LastStructPcb = StructPcb;
            continue;
        }

        if( strnicmp( Line, "$DRAWSEGMENT", 10 ) == 0 )
        {
            DRAWSEGMENT* DrawSegm = new DRAWSEGMENT( m_Pcb );
            DrawSegm->ReadDrawSegmentDescr( File, &LineNum );
            if( LastStructPcb == NULL )
            {
                m_Pcb->m_Drawings = DrawSegm;
                DrawSegm->Pback   = m_Pcb;
            }
            else
            {
                DrawSegm->Pback      = LastStructPcb;
                LastStructPcb->Pnext = DrawSegm;
            }
            LastStructPcb = DrawSegm;
            continue;
        }


        if( strnicmp( Line, "$COTATION", 9 ) == 0 )
        {
            COTATION* Cotation = new COTATION( m_Pcb );
            Cotation->ReadCotationDescr( File, &LineNum );
            if( LastStructPcb == NULL )
            {
                m_Pcb->m_Drawings = Cotation;
                Cotation->Pback   = m_Pcb;
            }
            else
            {
                Cotation->Pback      = LastStructPcb;
                LastStructPcb->Pnext = Cotation;
            }
            LastStructPcb = Cotation;
            continue;
        }

        if( strnicmp( Line, "$MIREPCB", 8 ) == 0 )
        {
            MIREPCB* Mire = new MIREPCB( m_Pcb );
            Mire->ReadMirePcbDescr( File, &LineNum );

            if( LastStructPcb == NULL )
            {
                m_Pcb->m_Drawings = Mire;
                Mire->Pback = m_Pcb;
            }
            else
            {
                Mire->Pback = LastStructPcb;
                LastStructPcb->Pnext = Mire;
            }
            LastStructPcb = Mire;
            continue;
        }

        if( strnicmp( Line, "$TRACK", 6 ) == 0 )
        {
            TRACK* StartTrack = m_Pcb->m_Track;
            nbsegm = 0;

            if( Append )
            {
                for( ; StartTrack != NULL; StartTrack = StartTrack->Next() )
                {
                    if( StartTrack->Pnext == NULL )
                        break;
                }
            }

#ifdef PCBNEW
            int ii = ReadListeSegmentDescr( File, StartTrack, TYPETRACK,
                                            &LineNum, NbTrack );
            m_Pcb->m_NbSegmTrack += ii;
#endif
            continue;
        }

        if( strnicmp( Line, "$ZONE", 5 ) == 0 )
        {
            SEGZONE* StartZone = m_Pcb->m_Zone;

            if( Append )
            {
                for( ; StartZone != NULL; StartZone = StartZone->Next() )
                {
                    if( StartZone->Pnext == NULL )
                        break;
                }
            }

#ifdef PCBNEW
            int ii = ReadListeSegmentDescr( File, StartZone, TYPEZONE,
                                            &LineNum, NbZone );
            m_Pcb->m_NbSegmZone += ii;
#endif
            continue;
        }
    }

    setlocale( LC_NUMERIC, "" );      // revert to the current  locale

    Affiche_Message( wxEmptyString );
	
	BestZoom();
	DrawPanel->ReDraw(DC, true);

#ifdef PCBNEW
    Compile_Ratsnest( DC, TRUE );
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
    setlocale( LC_NUMERIC, "C" );
    
    /* Ecriture de l'entete PCB : */
    fprintf( aFile, "PCBNEW-BOARD Version %d date %s\n\n", g_CurrentVersionPCB,
            DateAndTime( line ) );

    WriteGeneralDescrPcb( aFile );
    WriteSheetDescr( m_CurrentScreen, aFile );
    WriteSetup( aFile, this );

    rc = m_Pcb->Save( aFile );
    
    setlocale( LC_NUMERIC, "" );      // revert to the current locale
    wxEndBusyCursor();
    
    if( !rc )
        DisplayError( this, wxT( "Unable to save PCB file" ) );
    else
        Affiche_Message( wxEmptyString );
    
    return rc;
}

#endif
