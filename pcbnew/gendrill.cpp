/***************************************************************/
/* Functionbs to create EXCELLON drill files and report  files */
/***************************************************************/

#include "fctsys.h"

using namespace std;

#include <vector>

#include "common.h"
#include "plot_common.h"
#include "trigo.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "macros.h"

#include "gendrill.h"

/*
 *  Creates the drill files in EXCELLON format
 *  Number format:
 *      - Floating point format
 *      - integer format
 *      - integer format: "Trailling Zero" ( TZ ) or "Leading Zero"
 *  Units
 *      - Decimal
 *      - Metric
 *
 *  The drill maps can be created in HPGL or PS format
 *
 * dialog_gendrill.cpp  is the file (included in this file) which handles the Dialog box for drill file generation
 */


/* Local Function */
static void Gen_Line_EXCELLON( char* line, float x, float y );
static void Write_End_Of_File_Drill( FILE* aFile );

/* Local Variables : */
static float                   s_ConversionUnits;                   /* coeff de conversion des unites drill / pcb */
static int                     s_Unit_Drill_is_Inch = TRUE;         /* INCH,LZ (2:4) */
static int                     s_Zeros_Format = DECIMAL_FORMAT;
static DrillPrecision          s_Precision( 2, 4 );

static bool                    DrillOriginIsAuxAxis;    // Axis selection (main / auxiliary) for Drill Origin coordinates
static wxPoint                 File_Drill_Offset;       /* Offset des coord de percage pour le fichier g�n�r� */
static bool                    Minimal = false;
static bool                    Mirror  = true;

static std::vector<DRILL_TOOL> s_ToolListBuffer;
static std::vector<HOLE_INFO>  s_HoleListBuffer;

// Keywords for read and write config
#define ZerosFormatKey          wxT( "DrillZerosFormat" )
#define LeftPrecisionKey        wxT( "DrillLeftPrecisionOpt" )
#define RightPrecisionKey       wxT( "DrillRightPrecisionOpt" )
#define MirrorKey               wxT( "DrillMirrorYOpt" )
#define MinimalKey              wxT( "DrillMinHeader" )
#define UnitDrillInchKey        wxT( "DrillUnit" )
#define DrillOriginIsAuxAxisKey wxT( "DrillAuxAxis" )

#include "dialog_gendrill.cpp"   //  Dialog box for drill file generation


/************************************************/
void WinEDA_DrillFrame::InitDisplayParams( void )
/************************************************/

/* some param values initialisation before display dialog window
 */
{
    wxString msg;

    if( s_Zeros_Format==DECIMAL_FORMAT )
        m_Choice_Precision->Enable( false );
    if( DrillOriginIsAuxAxis )
        m_Choice_Drill_Offset->SetSelection( 1 );

    msg << s_Precision.m_lhs << wxT( ":" ) << s_Precision.m_rhs;
    m_Choice_Precision->SetStringSelection( msg );
    if( s_Zeros_Format==DECIMAL_FORMAT )
        m_Choice_Precision->Enable( false );

    msg = ReturnStringFromValue( g_UnitMetric,
                                 g_DesignSettings.m_ViaDrill,
                                 m_Parent->m_InternalUnits );
    msg += ReturnUnitSymbol( g_UnitMetric );
    m_ViaDrillValue->SetLabel( msg );

    msg = ReturnStringFromValue( g_UnitMetric,
                                 g_DesignSettings.m_MicroViaDrill,
                                 m_Parent->m_InternalUnits );
    msg += ReturnUnitSymbol( g_UnitMetric );
    m_MicroViaDrillValue->SetLabel( msg );

    msg.Empty();
    msg << g_HPGL_Pen_Num;
    m_PenNum->SetValue( msg );

    msg.Empty();
    msg << g_HPGL_Pen_Speed;
    m_PenSpeed->SetValue( msg );

    // See if we have some buried vias or/and microvias, and display microvias drill value if so
    m_ThroughViasCount = 0;
    m_MicroViasCount   = 0;
    m_BlindOrBuriedViasCount = 0;
    for( TRACK* track = m_Parent->m_Pcb->m_Track; track != NULL; track = track->Next() )
    {
        if( track->Type() != TYPEVIA )
            continue;
        if( track->Shape() == VIA_THROUGH )
            m_ThroughViasCount++;
        else if( track->Shape() == VIA_MICROVIA )
            m_MicroViasCount++;
        else if( track->Shape() == VIA_BLIND_BURIED )
            m_BlindOrBuriedViasCount++;
    }

    m_MicroViasDrillSizer->Enable( m_MicroViasCount );
    m_MicroViaDrillValue->Enable( m_MicroViasCount );

    // Pads holes cound:
    m_PadsHoleCount = 0;
    for( MODULE* module = m_Parent->m_Pcb->m_Modules; module != NULL; module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            if( pad->m_DrillShape == PAD_CIRCLE )
            {
                if( pad->m_Drill.x != 0 )
                    m_PadsHoleCount++;
            }
            else
                if( MIN( pad->m_Drill.x, pad->m_Drill.y ) != 0 )
                    m_PadsHoleCount++;
        }
    }

    msg = m_PadsCountInfoMsg->GetLabel();
    msg << wxT( " " ) << m_PadsHoleCount;
    m_PadsCountInfoMsg->SetLabel( msg );

    msg = m_ThroughViasInfoMsg->GetLabel();
    msg << wxT( " " ) << m_ThroughViasCount;
    m_ThroughViasInfoMsg->SetLabel( msg );

    msg = m_MicroViasInfoMsg->GetLabel();
    msg << wxT( " " ) << m_MicroViasCount;
    m_MicroViasInfoMsg->SetLabel( msg );

    msg = m_BuriedViasInfoMsg->GetLabel();
    msg << wxT( " " ) << m_BlindOrBuriedViasCount;
    m_BuriedViasInfoMsg->SetLabel( msg );
}


/**************************************/
void WinEDA_DrillFrame::SetParams( void )
/**************************************/
{
    wxString msg;
    long     ltmp;

    s_Unit_Drill_is_Inch = (m_Choice_Unit->GetSelection() == 0) ? FALSE : TRUE;
    Minimal = m_Check_Minimal->IsChecked();
    Mirror  = m_Check_Mirror->IsChecked();
    s_Zeros_Format = m_Choice_Zeros_Format->GetSelection();
    DrillOriginIsAuxAxis = m_Choice_Drill_Offset->GetSelection();

    msg = m_PenSpeed->GetValue();
    if( msg.ToLong( &ltmp ) )
        g_HPGL_Pen_Speed = ltmp;
    msg = m_PenNum->GetValue();

    if( msg.ToLong( &ltmp ) )
        g_HPGL_Pen_Num = ltmp;
    if( m_Choice_Drill_Offset->GetSelection() == 0 )
        File_Drill_Offset = wxPoint( 0, 0 );
    else
        File_Drill_Offset = m_Parent->m_Auxiliary_Axis_Position;

    /* get precision from radio box strings (this just makes it easier to
     *  change options later)*/
    wxString ps = m_Choice_Precision->GetStringSelection();
    wxString l  = ps.substr( 0, 1 );
    wxString r  = ps.substr( 2, 1 );


    // a long is not an int on all machines
    long lhs;
    long rhs;

    l.ToLong( &lhs );
    r.ToLong( &rhs );

    s_Precision.m_lhs = lhs;
    s_Precision.m_rhs = rhs;
}


/*****************************************************************/
void WinEDA_PcbFrame::InstallDrillFrame( wxCommandEvent& event )
/*****************************************************************/

/* This function displays and deletes the dialog frame for drill tools
 */
{
    wxConfig*          Config = m_Parent->m_EDA_Config;

    if( Config )
    {
        Config->Read( ZerosFormatKey, &s_Zeros_Format );
        Config->Read( LeftPrecisionKey, &s_Precision.m_lhs );
        Config->Read( RightPrecisionKey, &s_Precision.m_rhs );
        Config->Read( MirrorKey, &Mirror );
        Config->Read( MinimalKey, &Minimal );
        Config->Read( UnitDrillInchKey, &s_Unit_Drill_is_Inch );
        Config->Read( DrillOriginIsAuxAxisKey, &DrillOriginIsAuxAxis );
    }

    WinEDA_DrillFrame* frame = new WinEDA_DrillFrame( this );

    frame->ShowModal(); frame->Destroy();
}


/******************************************/
void WinEDA_DrillFrame::UpdateConfig()
/******************************************/
/* Save drill options: */
{
    SetParams();

    wxConfig* Config = m_Parent->m_Parent->m_EDA_Config;
    if( Config )
    {
        Config->Write( ZerosFormatKey, s_Zeros_Format );
        Config->Write( LeftPrecisionKey, s_Precision.m_lhs );
        Config->Write( RightPrecisionKey, s_Precision.m_rhs );
        Config->Write( MirrorKey, Mirror );
        Config->Write( MinimalKey, Minimal );
        Config->Write( UnitDrillInchKey, s_Unit_Drill_is_Inch );
        Config->Write( DrillOriginIsAuxAxisKey, DrillOriginIsAuxAxis );
    }
}


/*************************************************************/
void WinEDA_DrillFrame::GenDrillFiles( wxCommandEvent& event )
/*************************************************************/

/**
 * Function GenDrillFiles
 * Calls the functions to create EXCELLON drill files and/od drill map files
 * When all holes are through, one excellon file is created
 * when there are some partial holes (some blind or buried vias:
 * One excellon file is created, for all through holes.
 * And one file per layer pair, which have one or more holes, excluding through holes, already in the first file.
 */
{
    wxString FullFileName, Mask( wxT( "*" ) ), Ext( wxT( ".drl" ) );
    wxString BaseFileName;
    wxString layer_extend;              // added to the  Board FileName to create FullFileName (= Board FileName + layer pair names)
    wxString msg;
    bool     ExistsBuriedVias = false;  // If true, drill files are created layer pair by layer pair for buried vias
    int      layer1 = COPPER_LAYER_N;
    int      layer2 = LAYER_CMP_N;
    bool     gen_through_holes = true;

    UpdateConfig(); /* set params and Save drill options */

    m_Parent->MsgPanel->EraseMsgBox();

    /* Set conversion scale depending on drill file units */
    s_ConversionUnits = 0.0001;                             /* unites = INCHES */
    if( !s_Unit_Drill_is_Inch )
        s_ConversionUnits = 0.000254;                       /* unites = mm */

    if( m_MicroViasCount || m_BlindOrBuriedViasCount )
        ExistsBuriedVias = true;

    Mask += Ext;
    for( ; ; )
    {
        Build_Holes_List( m_Parent->m_Pcb, s_HoleListBuffer, s_ToolListBuffer,
                          layer1, layer2, gen_through_holes ? false : true );
        if( s_ToolListBuffer.size() > 0 ) //holes?
		{
			FullFileName = m_Parent->m_CurrentScreen->m_FileName;
			layer_extend.Empty();
			if( !gen_through_holes )
			{
				if( layer1 == COPPER_LAYER_N )
					layer_extend << wxT( "-copper" );
				else
					layer_extend << wxT( "-inner" ) << layer1;
				if( layer2 == LAYER_CMP_N )
					layer_extend << wxT( "-cmp" );
				else
					layer_extend << wxT( "-inner" ) << layer2;
			}
			layer_extend << Ext;
			ChangeFileNameExt( FullFileName, layer_extend );
	
			FullFileName = EDA_FileSelector( _( "Drill file" ),
											wxEmptyString, /* Chemin par defaut */
											FullFileName,  /* nom fichier par defaut */
											Ext,           /* extension par defaut */
											Mask,          /* Masque d'affichage */
											this,
											wxFD_SAVE,
											TRUE
											);
	
			if( FullFileName != wxEmptyString )
			{
				dest = wxFopen( FullFileName, wxT( "w" ) );
				if( dest == 0 )
				{
					msg = _( "Unable to create file " ) + FullFileName;
					DisplayError( this, msg );
					EndModal( 0 );
					return;
				}
	
				Create_Drill_File_EXCELLON( s_HoleListBuffer, s_ToolListBuffer );
			}
	
			switch( m_Choice_Drill_Map->GetSelection() )
			{
			case 0:
				break;
	
			case 1:
				GenDrillMap( FullFileName, s_HoleListBuffer, s_ToolListBuffer, PLOT_FORMAT_HPGL );
				break;
	
			case 2:
				GenDrillMap( FullFileName, s_HoleListBuffer, s_ToolListBuffer, PLOT_FORMAT_POST );
				break;
			}
	
			if(  !ExistsBuriedVias )
				break;
		}
        if(  gen_through_holes )
            layer2 = layer1 + 1;
        else
        {
            if( layer2 >= LAYER_CMP_N )                                 // no more layer pair to consider
                break;
            layer1++; layer2++;                                         // use next layer pair
            if( layer2 == g_DesignSettings.m_CopperLayerCount - 1 )     // The last layer is reached
                layer2 = LAYER_CMP_N;                                   // the last layer is always the component layer
        }
        gen_through_holes = false;
    }

    if( m_Choice_Drill_Report->GetSelection() > 0 )
    {
        FullFileName = m_Parent->m_CurrentScreen->m_FileName;
        GenDrillReport( FullFileName );
    }


    EndModal( 0 );
}


/********************************************************************/
void WinEDA_DrillFrame::UpdatePrecisionOptions( wxCommandEvent& event )
/********************************************************************/
{
    if( m_Choice_Unit->GetSelection()==1 )
    {   /* inch options   */
        m_Choice_Precision->SetString( 0, _( "2:3" ) );
        m_Choice_Precision->SetString( 1, _( "2:4" ) );
    }
    else
    {
        /* metric options */
        m_Choice_Precision->SetString( 0, _( "3:2" ) );
        m_Choice_Precision->SetString( 1, _( "3:3" ) );
    }
    if( m_Choice_Zeros_Format->GetSelection()==DECIMAL_FORMAT )
        m_Choice_Precision->Enable( false );
    else
        m_Choice_Precision->Enable( true );
}


/**********************************************************************************/
int WinEDA_DrillFrame::Create_Drill_File_EXCELLON( std::vector<HOLE_INFO>&  aHoleListBuffer,
                                                   std::vector<DRILL_TOOL>& aToolListBuffer )
/**********************************************************************************/

/**
 * Create the drill file in EXCELLON format
 * @return hole count
 * @param aHoleListBuffer = hole descriptor list
 * @param aToolListBuffer = Drill tools list
 */
{
    int   diam, holes_count;
    int   x0, y0, xf, yf, xc, yc;
    float xt, yt;
    char  line[1024];

    setlocale( LC_NUMERIC, "C" ); // Use the standard notation for float numbers

    Write_Excellon_Header( dest );

    holes_count = 0;
    int tool_reference = -2;

    /* Write the tool list */
    for( unsigned ii = 0; ii < aToolListBuffer.size(); ii++ )
    {
        if( s_Unit_Drill_is_Inch )  /* does it need T01, T02 or is T1,T2 ok?*/
            sprintf( line, "T%dC%.3f\n", ii + 1,
                     float (aToolListBuffer[ii].m_Diameter) * s_ConversionUnits );
        else
            sprintf( line, "T%dC%.3f\n", ii + 1,
                     float (aToolListBuffer[ii].m_Diameter) * s_ConversionUnits * 10.0 );

        fputs( line, dest );
    }

    fputs( "%\n", dest );

    if( !Minimal )
        fputs( "M47\n", dest );             /* Operator message */
    fputs( "G05\n", dest );                 /* Drill mode */
    /* Units : */
    if( s_Unit_Drill_is_Inch && !Minimal )
        fputs( "M72\n", dest ); /* M72 = inch mode */
    else if( !Minimal )
        fputs( "M71\n", dest ); /* M71 = metric mode */

    /* Read the hole file and generate lines for normal holes (oblong holes will be created later) */
    for( unsigned ii = 0; ii < aHoleListBuffer.size(); ii++ )
    {
        if( aHoleListBuffer[ii].m_Hole_Shape )
            continue; // oblong holes will be created later
        if( tool_reference != aHoleListBuffer[ii].m_Tool_Reference )
        {
            tool_reference = aHoleListBuffer[ii].m_Tool_Reference;
            sprintf( line, "T%d\n", tool_reference );
            fputs( line, dest );
        }

        x0 = aHoleListBuffer[ii].m_Hole_Pos_X - File_Drill_Offset.x;
        y0 = aHoleListBuffer[ii].m_Hole_Pos_Y - File_Drill_Offset.y;

        if( !Mirror )
            y0 *= -1;

        xt = float (x0) * s_ConversionUnits; yt = float (y0) * s_ConversionUnits;
        if( s_Unit_Drill_is_Inch )
        {
            Gen_Line_EXCELLON( line, xt, yt );
        }
        else
        {
            /* metric 3:3 */
            Gen_Line_EXCELLON( line, xt * 10, yt * 10 );
        }

        fputs( line, dest );
        holes_count++;
    }

    /* Read the hole file and generate lines for normal holes (oblong holes will be created later) */
    tool_reference = -2;    // set to a value not used for aHoleListBuffer[ii].m_Tool_Reference
    for( unsigned ii = 0; ii < aHoleListBuffer.size(); ii++ )
    {
        if( aHoleListBuffer[ii].m_Hole_Shape == 0 )
            continue; // wait for oblong holes
        if( tool_reference != aHoleListBuffer[ii].m_Tool_Reference )
        {
            tool_reference = aHoleListBuffer[ii].m_Tool_Reference;
            sprintf( line, "T%d\n", tool_reference );
            fputs( line, dest );
        }

        diam = MIN( aHoleListBuffer[ii].m_Hole_SizeX, aHoleListBuffer[ii].m_Hole_SizeY );
        if( diam == 0 )
            continue;

        /* Compute the hole coordinates: */
        xc = x0 = xf = aHoleListBuffer[ii].m_Hole_Pos_X - File_Drill_Offset.x;
        yc = y0 = yf = aHoleListBuffer[ii].m_Hole_Pos_Y - File_Drill_Offset.y;

        /* Compute the start and end coordinates for the shape */
        if( aHoleListBuffer[ii].m_Hole_SizeX < aHoleListBuffer[ii].m_Hole_SizeY )
        {
            int delta = (aHoleListBuffer[ii].m_Hole_SizeY - aHoleListBuffer[ii].m_Hole_SizeX) / 2;
            y0 -= delta; yf += delta;
        }
        else
        {
            int delta = (aHoleListBuffer[ii].m_Hole_SizeX - aHoleListBuffer[ii].m_Hole_SizeY ) / 2;
            x0 -= delta; xf += delta;
        }
        RotatePoint( &x0, &y0, xc, yc, aHoleListBuffer[ii].m_Hole_Orient );
        RotatePoint( &xf, &yf, xc, yc, aHoleListBuffer[ii].m_Hole_Orient );


        if( !Mirror )
        {
            y0 *= -1;  yf *= -1;
        }

        xt = float (x0) * s_ConversionUnits; yt = float (y0) * s_ConversionUnits;
        if( s_Unit_Drill_is_Inch )
            Gen_Line_EXCELLON( line, xt, yt );
        else
            Gen_Line_EXCELLON( line, xt * 10, yt * 10 );
        /* remove the '\n' from end of line, because we must add the "G85" command to the line: */
        for( int kk = 0; line[kk] != 0; kk++ )
            if( line[kk] == '\n' || line[kk] =='\r' )
                line[kk] = 0;

        fputs( line, dest );

        fputs( "G85", dest );    // add the "G85" command

        xt = float (xf) * s_ConversionUnits; yt = float (yf) * s_ConversionUnits;
        if( s_Unit_Drill_is_Inch )
            Gen_Line_EXCELLON( line, xt, yt );
        else
            Gen_Line_EXCELLON( line, xt * 10, yt * 10 );
        fputs( line, dest );
        fputs( "G05\n", dest );
        holes_count++;
    }

    Write_End_Of_File_Drill( dest );

    setlocale( LC_NUMERIC, "" );  // Revert to locale float notation

    return holes_count;
}


/***********************************************************************/
void Gen_Line_EXCELLON( char* line, float x, float y )
/***********************************************************************/

/* Created a line like:
 * X48000Y19500
 * According to the selected format
 */
{
    wxString xs, ys;
    int      xpad = s_Precision.m_lhs + s_Precision.m_rhs;
    int      ypad = xpad;

    /* I need to come up with an algorithm that handles any lhs:rhs format.*/
    /* one idea is to take more inputs for xpad/ypad when metric is used.  */

    switch( s_Zeros_Format )
    {
    default:
    case DECIMAL_FORMAT:
        sprintf( line, "X%.3fY%.3f\n", x, y );
        break;

    case SUPPRESS_LEADING:             /* that should work now */
        for( int i = 0; i< s_Precision.m_rhs; i++ )
        {
            x *= 10; y *= 10;
        }

        sprintf( line, "X%dY%d\n", (int) round( x ), (int) round( y ) );
        break;

    case SUPPRESS_TRAILING:
    {
        for( int i = 0; i< s_Precision.m_rhs; i++ )
        {
            x *= 10; y *= 10;
        }

        if( x<0 )
            xpad++;
        if( y<0 )
            ypad++;

        xs.Printf( wxT( "%0*d" ), xpad, (int) round( x ) );
        ys.Printf( wxT( "%0*d" ), ypad, (int) round( y ) );

        size_t j = xs.Len() - 1;
        while( xs[j] == '0' && j )
            xs.Truncate( j-- );

        j = ys.Len() - 1;
        while( ys[j] == '0' && j )
            ys.Truncate( j-- );

        sprintf( line, "X%sY%s\n", CONV_TO_UTF8( xs ), CONV_TO_UTF8( ys ) );
        break;
    }

    case KEEP_ZEROS:
        for( int i = 0; i< s_Precision.m_rhs; i++ )
        {
            x *= 10; y *= 10;
        }

        if( x<0 )
            xpad++;
        if( y<0 )
            ypad++;
        xs.Printf( wxT( "%0*d" ), xpad, (int) round( x ) );
        ys.Printf( wxT( "%0*d" ), ypad, (int) round( y ) );
        sprintf( line, "X%sY%s\n", CONV_TO_UTF8( xs ), CONV_TO_UTF8( ys ) );
        break;
    }
}


/************************************************************/
void WinEDA_DrillFrame::Write_Excellon_Header( FILE* aFile )
/************************************************************/

/* Print the DRILL file header. The full header is:
 * M48
 * ;DRILL file {PCBNEW (2007-11-29-b)} date 17/1/2008-21:02:35
 * ;FORMAT={2:4 / absolute / Pouces / Suppression zeros de t�te}
 * R,T
 * VER,1
 * FMAT,2
 * INCH,TZ
 * TCST,OFF
 * ICI,OFF
 * ATC,ON
 */
{
    char Line[256];

    fputs( "M48\n", aFile );

    if( !Minimal )
    {
        DateAndTime( Line );
        wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
        fprintf( dest, ";DRILL file {%s} date %s\n", CONV_TO_UTF8( Title ), Line );
        fputs( ";FORMAT={", aFile );
        fprintf( dest, "%s / absolute / ",
                CONV_TO_UTF8( m_Choice_Precision->GetStringSelection() ) );
        fprintf( dest, "%s / ", CONV_TO_UTF8( m_Choice_Unit->GetStringSelection() ) );
        fprintf( dest, "%s}\n", CONV_TO_UTF8( m_Choice_Zeros_Format->GetStringSelection() ) );

        fputs( "R,T\nVER,1\nFMAT,2\n", aFile );
    }

    if( s_Unit_Drill_is_Inch )
        fputs( "INCH", aFile );     // Si unites en INCHES
    else
        fputs( "METRIC", aFile );   // Si unites en mm

    switch( s_Zeros_Format )
    {
    case SUPPRESS_LEADING:
    case DECIMAL_FORMAT:
        fputs( ",TZ\n", aFile );
        break;

    case SUPPRESS_TRAILING:
        fputs( ",LZ\n", aFile );
        break;

    case KEEP_ZEROS:
        fputs( ",TZ\n", aFile ); // TZ is acceptable when all zeros are kept
        break;
    }

    if( !Minimal )
        fputs( "TCST,OFF\nICI,OFF\nATC,ON\n", aFile );
}


/****************************************/
void Write_End_Of_File_Drill( FILE* aFile )
/****************************************/
{
    //add if minimal here
    fputs( "T0\nM30\n", aFile ); fclose( aFile );
}


/**********************************************************************************/
void WinEDA_DrillFrame::GenDrillMap( const wxString aFileName,
                                     std::vector<HOLE_INFO>& aHoleListBuffer,
                                     std::vector<DRILL_TOOL>& buffer, int format )
/**********************************************************************************/

/* Genere le plan de percage (Drill map) format HPGL ou POSTSCRIPT
 */
{
    wxString FullFileName, Mask( wxT( "*" ) ), Ext;
    wxString msg;

    /* Init extension */
    switch( format )
    {
    case PLOT_FORMAT_HPGL:
        Ext = wxT( "-drl.plt" );
        break;

    case PLOT_FORMAT_POST:
        Ext = wxT( "-drl.ps" );
        break;

    default:
        DisplayError( this, wxT( "WinEDA_DrillFrame::GenDrillMap() error" ) );
        return;
    }

    /* Init file name */
    FullFileName = aFileName;
    ChangeFileNameExt( FullFileName, Ext );
    Mask += Ext;

    FullFileName = EDA_FileSelector( _( "Drill Map file" ),
                                     wxEmptyString, /* Chemin par defaut */
                                     FullFileName,  /* nom fichier par defaut */
                                     Ext,           /* extension par defaut */
                                     Mask,          /* Masque d'affichage */
                                     this,
                                     wxFD_SAVE,
                                     TRUE
                                     );
    if( FullFileName.IsEmpty() )
        return;

    dest = wxFopen( FullFileName, wxT( "wt" ) );
    if( dest == 0 )
    {
        msg = _( "Unable to create file" );
        msg << wxT( " <" ) << FullFileName << wxT( ">" );
        DisplayError( this, msg ); return;
    }

    GenDrillMapFile( m_Parent->m_Pcb,
                     dest,
                     FullFileName,
                     m_Parent->m_CurrentScreen->m_CurrentSheetDesc->m_Size,
                     s_HoleListBuffer,
                     s_ToolListBuffer,
                     s_Unit_Drill_is_Inch,
                     format );
}


/**************************************************************************************************/
void WinEDA_DrillFrame::GenDrillReport( const wxString aFileName )
/**************************************************************************************************/

/*
 *  Create a list of drill values and drill count
 */
{
    wxString FileName, Mask( wxT( "*" ) ), Ext( wxT( ".rpt" ) );
    wxString msg;

    FileName = aFileName;
    ChangeFileNameExt( FileName, wxT( "-drl" ) + Ext );
    Mask += Ext;

    FileName = EDA_FileSelector( _( "Drill Report file" ),
                                 wxEmptyString, /* Chemin par defaut */
                                 FileName,      /* nom fichier par defaut */
                                 Ext,           /* extension par defaut */
                                 Mask,          /* Masque d'affichage */
                                 this,
                                 wxFD_SAVE,
                                 TRUE
                                 );
    if( FileName.IsEmpty() )
        return;

    dest = wxFopen( FileName, wxT( "w" ) );
    if( dest == 0 )
    {
        msg = _( "Unable to create file " ) + FileName;
        DisplayError( this, msg );
        return;
    }
    GenDrillReportFile( dest, m_Parent->m_Pcb,
                        m_Parent->m_CurrentScreen->m_FileName,
                        s_Unit_Drill_is_Inch,
                        s_HoleListBuffer,
                        s_ToolListBuffer );
}
