/***************************************************************/
/* Functionbs to create EXCELLON drill files and report  files */
/***************************************************************/

#include "fctsys.h"

#include "common.h"
#include "plot_common.h"
#include "trigo.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "macros.h"

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

class DRILL_TOOL
{
public:
    int m_TotalCount;
    int m_OvalCount;
    int m_Diameter;
};

/* drill precision format */
class DrillPrecision
{
public:
    int m_lhs;
    int m_rhs;

public:
    DrillPrecision( int l, int r ) { m_lhs = l; m_rhs = r; }
};

/* zeros format */
enum zeros_fmt {
    DECIMAL_FORMAT,
    SUPPRESS_LEADING,
    SUPPRESS_TRAILING,
    KEEP_ZEROS
};

/* Local Function */
static void Gen_Line_EXCELLON( char* line, float x, float y );
static void Write_End_Of_File_Drill();
static void PlotDrillSymbol( const wxPoint& position, int diametre, int num_forme, int format );
static void PlotOvalDrillSymbol( const wxPoint& position,
                                 const wxSize&  size,
                                 int            orient,
                                 int            format );

/* Local Variables : */
static int     s_DrillToolsCount;                 /* Nombre de forets a utiliser */
static float   conv_unit;                       /* coeff de conversion des unites drill / pcb */
static int     s_Unit_Drill_is_Inch = TRUE;       /* INCH,LZ (2:4) */
static int     s_Zeros_Format = DECIMAL_FORMAT;
static DrillPrecision s_Precision( 2, 4 );

static bool    DrillOriginIsAuxAxis;    // Axis selection (main / auxiliary) for Drill Origin coordinates
static wxPoint File_Drill_Offset;       /* Offset des coord de percage pour le fichier généré */
static bool    Minimal = false;
static bool    Mirror  = true;

// Keywords for read and write config
#define ZerosFormatKey          wxT( "DrillZerosFormat" )
#define LeftPrecisionKey        wxT( "DrillLeftPrecisionOpt" )
#define RightPrecisionKey       wxT( "DrillRightPrecisionOpt" )
#define MirrorKey               wxT( "DrillMirrorYOpt" )
#define MinimalKey              wxT( "DrillMinHeader" )
#define UnitDrillInchKey        wxT( "DrillUnit" )
#define DrillOriginIsAuxAxisKey wxT( "DrillAuxAxis" )

#include "dialog_gendrill.cpp"	//  Dialog box for drill file generation


/**********************************************/
void WinEDA_DrillFrame::InitDisplayParams(void)
/**********************************************/
/* some param values befor display dialog
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

	msg = ReturnStringFromValue( g_UnitMetric, g_DesignSettings.m_ViaDrill, m_Parent->m_InternalUnits );
	msg += ReturnUnitSymbol( g_UnitMetric );
	m_ViaDrillValue->SetLabel(msg);
	
	msg = ReturnStringFromValue( g_UnitMetric, g_DesignSettings.m_MicroViaDrill, m_Parent->m_InternalUnits );
	msg += ReturnUnitSymbol( g_UnitMetric );
	m_MicroViaDrillValue->SetLabel(msg);

	msg.Empty();
	msg << g_HPGL_Pen_Num;
	m_PenNum->SetValue(msg);
	
	msg.Empty();
	msg <<  g_HPGL_Pen_Speed;
	m_PenSpeed->SetValue(msg);
	
	// See if we have some microvias, and display drill value if so
	m_ThroughViasCount = 0;
	m_MicroViasCount = 0;
	m_BlindOrBuriedViasCount = 0;
	for ( TRACK * track = m_Parent->m_Pcb->m_Track; track != NULL; track = track->Next() )
	{
		if ( track->Type() != TYPEVIA ) continue;
		if ( track->Shape() == VIA_THROUGH )
			m_ThroughViasCount++;
		else if ( track->Shape() == VIA_MICROVIA )
			m_MicroViasCount++;
		else if ( track->Shape() == VIA_BLIND_BURIED )
			m_BlindOrBuriedViasCount++;
	}
	m_MicroViasDrillSizer->Enable(m_MicroViasCount);
	m_MicroViaDrillValue->Enable(m_MicroViasCount);
	
	/* Display statistics */
	// Pads holes cound:
	m_PadsHoleCount = 0;
	for ( MODULE * module = m_Parent->m_Pcb->m_Modules; module != NULL; module=module->Next() )
	{
		for ( D_PAD * pad = module->m_Pads; pad != NULL; pad = pad->Next() )
		{
            if( pad->m_DrillShape == PAD_CIRCLE )
                if( pad->m_Drill.x != 0) m_PadsHoleCount++;
            else
                if ( MIN( pad->m_Drill.x, pad->m_Drill.y ) != 0 ) m_PadsHoleCount++;
		}
	}
	msg = m_PadsCountInfoMsg->GetLabel();
	msg << wxT(" ") << m_PadsHoleCount;
	m_PadsCountInfoMsg->SetLabel(msg);

	msg = m_ThroughViasInfoMsg->GetLabel();
	msg << wxT(" ") << m_ThroughViasCount;
	m_ThroughViasInfoMsg->SetLabel(msg);
	
	msg = m_MicroViasInfoMsg->GetLabel();
	msg << wxT(" ") << m_MicroViasCount;
	m_MicroViasInfoMsg->SetLabel(msg);
	
	msg = m_BuriedViasInfoMsg->GetLabel();
	msg << wxT(" ") << m_BlindOrBuriedViasCount;
	m_BuriedViasInfoMsg->SetLabel(msg);
}


/**************************************/
void WinEDA_DrillFrame::SetParams(void)
/**************************************/
{
	wxString msg;
	long ltmp;

    s_Unit_Drill_is_Inch = (m_Choice_Unit->GetSelection() == 0) ? FALSE : TRUE;
    Minimal = m_Check_Minimal->IsChecked();
    Mirror  = m_Check_Mirror->IsChecked();
    s_Zeros_Format = m_Choice_Zeros_Format->GetSelection();
    DrillOriginIsAuxAxis = m_Choice_Drill_Offset->GetSelection();

    msg = m_PenSpeed->GetValue();
    if ( msg.ToLong(&ltmp) ) g_HPGL_Pen_Speed = ltmp;
    msg = m_PenNum->GetValue();

    if ( msg.ToLong(&ltmp) ) g_HPGL_Pen_Num = ltmp;
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
/* Calls the functions to create EXCELLOn drill files and/od drill map files
*/
{
    wxString FullFileName, Mask( wxT( "*" ) ), Ext( wxT( ".drl" ) );
    int      ii;
    wxString msg;

    UpdateConfig(); /* set params and Save drill options */

    m_Parent->MsgPanel->EraseMsgBox();

    /* Set conversion scale depending on drill file units */
    conv_unit = 0.0001;                             /* unites = INCHES */
    if( !s_Unit_Drill_is_Inch )
        conv_unit = 0.000254;                       /* unites = mm */

    /* Get the file name */
    FullFileName = m_Parent->m_CurrentScreen->m_FileName;
    ChangeFileNameExt( FullFileName, Ext );
    Mask += Ext;

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

        /* Init : */
        Affiche_1_Parametre( m_Parent, 0, _( "File" ), FullFileName, BLUE );

		setlocale (LC_NUMERIC, "C");	// Use the standard notation for float numbers
		Init_Drill();

        ii = Gen_Liste_Forets( (DRILL_TOOL*) adr_lowmem, TRUE );
        msg.Printf( wxT( "%d" ), ii );
        Affiche_1_Parametre( m_Parent, 30, _( "Tools" ), msg, BROWN );

        ii = Create_Drill_File_EXCELLON( (DRILL_TOOL*) adr_lowmem );
        msg.Printf( wxT( "%d" ), ii );
        Affiche_1_Parametre( m_Parent, 45, _( "Drill" ), msg, GREEN );

        Write_End_Of_File_Drill();
		setlocale (LC_NUMERIC, "");	// Revert to locale float notation
    }

    switch( m_Choice_Drill_Map->GetSelection() )
    {
    case 0:
        break;

    case 1:
        GenDrillMap( PLOT_FORMAT_HPGL );
        break;

    case 2:
        GenDrillMap( PLOT_FORMAT_POST );
        break;
    }

    if( m_Choice_Drill_Report->GetSelection() > 0 )
        GenDrillReport();

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


/***************************************************************/
int WinEDA_DrillFrame::Create_Drill_File_EXCELLON( DRILL_TOOL* buffer )
/***************************************************************/

/* Create the drill file in EXCELLON format
 *  Return hole count
 *  buffer: Drill tools list
 */
{
    DRILL_TOOL*  foret;
    TRACK*  pt_piste;
    D_PAD*  pt_pad;
    MODULE* Module;
    int     ii, diam, nb_trous;
    int     x0, y0, xf, yf, xc, yc;
    float   xt, yt;
    char    line[1024];

    /* Create the pad drill list : */
    nb_trous = 0;

    /* Read the hole file  */
    for( ii = 0, foret = buffer; ii < s_DrillToolsCount; ii++, foret++ )
    {
        sprintf( line, "T%d\n", ii + 1 ); fputs( line, dest );
        /* Read the via list */
        {
            pt_piste = m_Parent->m_Pcb->m_Track;
            for( ; pt_piste != NULL; pt_piste = (TRACK*) pt_piste->Pnext )
            {
                if( pt_piste->Type() != TYPEVIA )
                    continue;
                int via_drill = pt_piste->GetDrillValue();
                if( via_drill == 0 )
                    continue;
					
                if( foret->m_Diameter != via_drill )
                    continue;

                x0 = pt_piste->m_Start.x - File_Drill_Offset.x;
                y0 = pt_piste->m_Start.y - File_Drill_Offset.y;

                if( !Mirror )
                    y0 *= -1;

                xt = float (x0) * conv_unit; yt = float (y0) * conv_unit;
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
                nb_trous++;
            }
        }

        /* Read pad list and create Drill infos for round holes only: */
        Module = m_Parent->m_Pcb->m_Modules;
        for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
        {
            /* Read and analyse pads */
            pt_pad = (D_PAD*) Module->m_Pads;
            for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
            {
                if( pt_pad->m_DrillShape != PAD_CIRCLE )
                    continue;
                diam = pt_pad->m_Drill.x;
                if( diam == 0 )
                    continue;
                if( diam != foret->m_Diameter )
                    continue;

                /* Compute the hole coordinates: */
                x0 = pt_pad->m_Pos.x - File_Drill_Offset.x;
                y0 = pt_pad->m_Pos.y - File_Drill_Offset.y;

                if( !Mirror )
                    y0 *= -1;

                xt = float (x0) * conv_unit; yt = float (y0) * conv_unit;
                if( s_Unit_Drill_is_Inch )
                    Gen_Line_EXCELLON( line, xt, yt );
                else
                    Gen_Line_EXCELLON( line, xt * 10, yt * 10 );

                fputs( line, dest );
                nb_trous++;
            }     /* ens analyse one module */
        }	// End analyse module list


        /* Read pad list and create Drill infos for oblong holes only: */
        Module = m_Parent->m_Pcb->m_Modules;
        for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
        {
            /* Analyse pad list for the module */
            pt_pad = (D_PAD*) Module->m_Pads;
            for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
            {
                if( pt_pad->m_DrillShape != PAD_OVAL )
                    continue;

                diam = MIN( pt_pad->m_Drill.x, pt_pad->m_Drill.y );
                if( diam == 0 )
                    continue;
                if( diam != foret->m_Diameter )
                    continue;

                /* Compute the hole coordinates: */
                xc = x0 = xf = pt_pad->m_Pos.x - File_Drill_Offset.x;
                yc = y0 = yf = pt_pad->m_Pos.y - File_Drill_Offset.y;

                /* Compute the start and end coordinates for the shape */
                if( pt_pad->m_Drill.x < pt_pad->m_Drill.y )
                {
                    int delta = (pt_pad->m_Drill.y - pt_pad->m_Drill.x) / 2;
                    y0 -= delta; yf += delta;
                }
                else
                {
                    int delta = (pt_pad->m_Drill.x - pt_pad->m_Drill.y) / 2;
                    x0 -= delta; xf += delta;
                }
                RotatePoint( &x0, &y0, xc, yc, pt_pad->m_Orient );
                RotatePoint( &xf, &yf, xc, yc, pt_pad->m_Orient );


                if( !Mirror )
                {
                    y0 *= -1;  yf *= -1;
                }

                xt = float (x0) * conv_unit; yt = float (y0) * conv_unit;
                if( s_Unit_Drill_is_Inch )
                    Gen_Line_EXCELLON( line, xt, yt );
                else
                    Gen_Line_EXCELLON( line, xt * 10, yt * 10 );
                /* remove the '\n' from end of line, because we must add the "G85" command to the line: */
                for( int kk = 0; line[kk] != 0; kk++ )
                    if( line[kk] == '\n' || line[kk] =='\r' )
                        line[kk] = 0;

                fputs( line, dest );

                fputs( "G85", dest );	// add the "G85" command

                xt = float (xf) * conv_unit; yt = float (yf) * conv_unit;
                if( s_Unit_Drill_is_Inch )
                    Gen_Line_EXCELLON( line, xt, yt );
                else
                    Gen_Line_EXCELLON( line, xt * 10, yt * 10 );
                fputs( "G05\n", dest );
                nb_trous++;
            }  /* End Analyse pad list for the module */
        }      /* End analyse modules for the current tool */
    }

    return nb_trous;
}


/*****************************************************/
void Gen_Line_EXCELLON( char* line, float x, float y )
/*****************************************************/
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

    ;
}


/***************************************************/
DRILL_TOOL* GetOrAddForet( DRILL_TOOL* buffer, int diameter )
/***************************************************/

/* Search the drill tool for the diameter "diameter"
 *  Create a new one if not found
 */
{
    int    ii;
    DRILL_TOOL* foret;

    if( diameter == 0 )
        return NULL;

    /* Search for an existing tool: */
    for( ii = 0, foret = buffer; ii < s_DrillToolsCount; ii++, foret++ )
    {
        if( foret->m_Diameter == diameter ) /* found */
            return foret;
    }

    /* No tool found, we must create a new one */
    s_DrillToolsCount++;
    foret->m_TotalCount = 0;
    foret->m_OvalCount  = 0;
    foret->m_Diameter   = diameter;
    return foret;
}


/* Sort function by drill value */
int Sort_by_Drill_Value( void* foret1, void* foret2 )
{
    return ( (DRILL_TOOL*) foret1 )->m_Diameter - ( (DRILL_TOOL*) foret2 )->m_Diameter;
}


/*********************************************************************/
int WinEDA_DrillFrame::Gen_Liste_Forets( DRILL_TOOL* buffer, bool print_header )
/**********************************************************************/

/* Etablit la liste des forets de percage, dans l'ordre croissant des
 *  diametres
 *      Retourne:
 *      Nombre de Forets
 *
 *      Mise a jour: s_DrillToolsCount = nombre de forets differents
 *      Genere une liste de s_DrillToolsCount structures DRILL_TOOL a partir de buffer
 */
{
    DRILL_TOOL*  foret;
    D_PAD*  pt_pad;
    MODULE* Module;
    int     ii, diam;
    char    line[1024];

    s_DrillToolsCount = 0; foret = buffer;

    /* Create the via tools */
    TRACK* pt_piste = m_Parent->m_Pcb->m_Track;
    for( ; pt_piste != NULL; pt_piste = (TRACK*) pt_piste->Pnext )
    {
        if( pt_piste->Type() != TYPEVIA )
            continue;
        int via_drill = pt_piste->GetDrillValue();
        if( via_drill == 0 )
            continue;
        foret = GetOrAddForet( buffer, via_drill );
        if( foret )
            foret->m_TotalCount++;
    }

    /* Create the pad tools : */
    Module = m_Parent->m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
    {
        /* Examen  des pastilles */
        pt_pad = (D_PAD*) Module->m_Pads;
        for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
        {
            if( pt_pad->m_DrillShape == PAD_CIRCLE )
                diam = pt_pad->m_Drill.x;
            else
                diam = MIN( pt_pad->m_Drill.x, pt_pad->m_Drill.y );

            if( diam == 0 )
                continue;

            foret = GetOrAddForet( buffer, diam );
            if( foret )
            {
                foret->m_TotalCount++;
                if( pt_pad->m_DrillShape == PAD_OVAL )
                    foret->m_OvalCount++;
            }
        }

        /* Fin examen 1 module */
    }

    /* tri des forets par ordre de diametre croissant */
    qsort( buffer, s_DrillToolsCount, sizeof(DRILL_TOOL),
           ( int( * ) ( const void*, const void* ) )Sort_by_Drill_Value );

    /* Generation de la section liste des outils */
    if( print_header )
    {
        for( ii = 0, foret = (DRILL_TOOL*) buffer; ii < s_DrillToolsCount; ii++, foret++ )
        {
            if( s_Unit_Drill_is_Inch ) /* does it need T01, T02 or is T1,T2 ok?*/
                sprintf( line, "T%dC%.3f\n", ii + 1,
                         float (foret->m_Diameter) * conv_unit );
            else
                sprintf( line, "T%dC%.3f\n", ii + 1,
                         float (foret->m_Diameter) * conv_unit * 10.0 );

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
    }

    return s_DrillToolsCount;
}


/***************************************/
void WinEDA_DrillFrame::Init_Drill()
/***************************************/
/* Print the DRILL file header */
{
    char Line[256];

    fputs( "M48\n", dest );

    if( !Minimal )
    {
        DateAndTime( Line );
        wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
        fprintf( dest, ";DRILL file {%s} date %s\n", CONV_TO_UTF8( Title ), Line );
        fputs( ";FORMAT={", dest );
        fprintf( dest, "%s / absolute / ",
                CONV_TO_UTF8( m_Choice_Precision->GetStringSelection() ) );
        fprintf( dest, "%s / ", CONV_TO_UTF8( m_Choice_Unit->GetStringSelection() ) );
        fprintf( dest, "%s}\n", CONV_TO_UTF8( m_Choice_Zeros_Format->GetStringSelection() ) );

        fputs( "R,T\nVER,1\nFMAT,2\n", dest );
    }

    if( s_Unit_Drill_is_Inch )
        fputs( "INCH", dest );      // Si unites en INCHES
    else
        fputs( "METRIC", dest );    // Si unites en mm

    switch( s_Zeros_Format )
    {
    case SUPPRESS_LEADING:
    case DECIMAL_FORMAT:
        fputs( ",TZ\n", dest );
        break;

    case SUPPRESS_TRAILING:
        fputs( ",LZ\n", dest );
        break;

    case KEEP_ZEROS:
        fputs( ",TZ\n", dest ); // TZ is acceptable when all zeros are kept
        break;
    }

    if( !Minimal )
        fputs( "TCST,OFF\nICI,OFF\nATC,ON\n", dest );
}


/*****************************/
void Write_End_Of_File_Drill()
/*****************************/
{
    //add if minimal here
    fputs( "T0\nM30\n", dest ); fclose( dest );
}


/***********************************************/
void WinEDA_DrillFrame::GenDrillMap( int format )
/***********************************************/

/* Genere le plan de percage (Drill map) format HPGL ou POSTSCRIPT
 */
{
    int             ii, x, y;
    int             plotX, plotY, TextWidth;
    DRILL_TOOL*          foret;
    int             intervalle = 0, CharSize = 0;
    EDA_BaseStruct* PtStruct;
    int             old_g_PlotOrient = g_PlotOrient;
    wxString        FullFileName, Mask( wxT( "*" ) ), Ext;
    char            line[1024];
    int             dX, dY;
    wxPoint         BoardCentre;
    int             PlotMarge_in_mils = 400; // Margin in 1/1000 inch
    int             marge = PlotMarge_in_mils * U_PCB;
    wxSize          SheetSize;
    float           fTextScale = 1.0;
    double          scale_x    = 1.0, scale_y = 1.0;
    Ki_PageDescr*   SheetPS    = NULL;
    wxString        msg;

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
    FullFileName = m_Parent->m_CurrentScreen->m_FileName;
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

	setlocale (LC_NUMERIC, "C");	// Use the standard notation for float numbers
    g_PlotOrient = 0;
    /* calcul des dimensions et centre du PCB */
    m_Parent->m_Pcb->ComputeBoundaryBox();

    dX = m_Parent->m_Pcb->m_BoundaryBox.GetWidth();
    dY = m_Parent->m_Pcb->m_BoundaryBox.GetHeight();
    BoardCentre = m_Parent->m_Pcb->m_BoundaryBox.Centre();

    // Calcul de l'echelle du dessin du PCB,
    // Echelle 1 en HPGL, dessin sur feuille A4 en PS, + texte description des symboles
    switch( format )
    {
    case PLOT_FORMAT_HPGL:     /* Calcul des echelles de conversion format HPGL */
        Scale_X        = Scale_Y = 1.0;
        scale_x        = Scale_X * SCALE_HPGL;
        scale_y        = Scale_Y * SCALE_HPGL;
        fTextScale     = SCALE_HPGL;
        SheetSize      = m_Parent->m_CurrentScreen->m_CurrentSheet->m_Size;
        SheetSize.x   *= U_PCB;
        SheetSize.y   *= U_PCB;
        g_PlotOffset.x = 0;
        g_PlotOffset.y = (int) (SheetSize.y * scale_y);
        break;

    case PLOT_FORMAT_POST:
    {
        // calcul en unites internes des dimensions de la feuille ( connues en 1/1000 pouce )
        SheetPS     = &g_Sheet_A4;
        SheetSize.x = SheetPS->m_Size.x * U_PCB;
        SheetSize.y = SheetPS->m_Size.y * U_PCB;
        float Xscale = (float) ( SheetSize.x - (marge * 2) ) / dX;
        float Yscale = (float) ( SheetSize.y * 0.6 - (marge * 2) ) / dY;

        scale_x = scale_y = MIN( Xscale, Yscale );

        g_PlotOffset.x = -(SheetSize.x / 2) +
                         (int) (BoardCentre.x * scale_x) + marge;
        g_PlotOffset.y = SheetSize.y / 2 +
                         (int) (BoardCentre.y * scale_y) - marge;
        g_PlotOffset.y += SheetSize.y / 8;      /* decalage pour legende */
        break;
    }
    }

    dest = wxFopen( FullFileName, wxT( "wt" ) );
    if( dest == 0 )
    {
        msg.Printf( _( "Unable to create file <%s>" ), FullFileName.GetData() );
        DisplayError( this, msg ); return;
    }

    /* Init : */
    m_Parent->MsgPanel->EraseMsgBox();
    Affiche_1_Parametre( m_Parent, 0, _( "File" ), FullFileName, BLUE );

    /* Calcul de la liste des diametres de percage (liste des forets) */
    ii = Gen_Liste_Forets( (DRILL_TOOL*) adr_lowmem, FALSE );
    msg.Printf( wxT( "%d" ), ii );
    Affiche_1_Parametre( m_Parent, 48, _( "Tools" ), msg, BROWN );


    switch( format )
    {
    case PLOT_FORMAT_HPGL:
        InitPlotParametresHPGL( g_PlotOffset, scale_x, scale_y );
        PrintHeaderHPGL( dest, g_HPGL_Pen_Speed, g_HPGL_Pen_Num );
        break;

    case PLOT_FORMAT_POST:
    {
        int BBox[4];
        BBox[0] = BBox[1] = PlotMarge_in_mils;
        BBox[2] = SheetPS->m_Size.x - PlotMarge_in_mils;
        BBox[3] = SheetPS->m_Size.y - PlotMarge_in_mils;
        InitPlotParametresPS( g_PlotOffset, SheetPS,
                              (double) 1.0 / PCB_INTERNAL_UNIT, (double) 1.0 / PCB_INTERNAL_UNIT );
        SetDefaultLineWidthPS( 10 );    // Set line with to 10/1000 inch
        PrintHeaderPS( dest, wxT( "PCBNEW-PS" ), FullFileName, 1, BBox, wxLANDSCAPE );
        InitPlotParametresPS( g_PlotOffset, SheetPS, scale_x, scale_y );
    }
        break;

    default:
        break;
    }

    /* Draw items on edge layer */
    PtStruct = m_Parent->m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        switch( PtStruct->Type() )
        {
        case TYPEDRAWSEGMENT:
            PlotDrawSegment( (DRAWSEGMENT*) PtStruct, format, EDGE_LAYER );
            break;

        case TYPETEXTE:
            PlotTextePcb( (TEXTE_PCB*) PtStruct, format, EDGE_LAYER );
            break;

        case TYPECOTATION:
            PlotCotation( (COTATION*) PtStruct, format, EDGE_LAYER );
            break;

        case TYPEMIRE:
            PlotMirePcb( (MIREPCB*) PtStruct, format, EDGE_LAYER );
            break;

        case TYPEMARKER:	// do not draw
            break;

        default:
            DisplayError( this, wxT( "WinEDA_DrillFrame::GenDrillMap() : Unexpected Draw Type" ) );
            break;
        }
    }

    TextWidth = 50;    // Set Drill Symbols width in 1/10000 mils

    if( format == PLOT_FORMAT_POST )
    {
        sprintf( line, "%d setlinewidth\n", TextWidth );
        fputs( line, dest );
    }

    ii = Plot_Drill_PcbMap( (DRILL_TOOL*) adr_lowmem, format );
    msg.Printf( wxT( "%d" ), ii );
    Affiche_1_Parametre( m_Parent, 64, _( "Drill" ), msg, GREEN );

    /* Impression de la liste des symboles utilises */
    CharSize = 800;                         /* text size in 1/10000 mils */
    float CharScale = 1.0 / scale_x;        /* real scale will be CharScale * scale_x,
                                             *  because the global plot scale is scale_x */
    TextWidth  = (int) (50 * CharScale);    // Set text width
    intervalle = (int) (CharSize * CharScale) + TextWidth;

    switch( format )
    {
    case PLOT_FORMAT_HPGL:
    {
        /* generation des dim: commande SI x,y; x et y = dim en cm */
        char csize[256];
        sprintf( csize, "%2.3f", (float) CharSize * CharScale * 0.000254 );
        sprintf( line, "SI %s, %s;\n", csize, csize );
        break;
    }

    case PLOT_FORMAT_POST:
        /* Reglage de l'epaisseur de traits des textes */
        sprintf( line, "%d setlinewidth\n", TextWidth );
        break;

    default:
        *line = 0;
        break;
    }

    fputs( line, dest );

    switch( format )
    {
    default:
    case PLOT_FORMAT_POST:
        g_PlotOffset.x = 0;
        g_PlotOffset.y = 0;
        InitPlotParametresPS( g_PlotOffset, SheetPS, scale_x, scale_x );
        break;

    case PLOT_FORMAT_HPGL:
        InitPlotParametresHPGL( g_PlotOffset, scale_x, scale_x );
        break;
    }

    /* Trace des informations */

    /* Trace de "Infos" */
    plotX = marge + 1000;
    plotY = CharSize + 1000;
    x = plotX; y = plotY;
    x = +g_PlotOffset.x + (int) (x * fTextScale);
    y = g_PlotOffset.y - (int) (y * fTextScale);

    plotY += (int) ( intervalle * 1.2) + 200;

    switch( format )
    {
    case PLOT_FORMAT_HPGL:
        sprintf( line, "PU %d, %d; LBInfos\03;\n",
                x + (int) (intervalle * CharScale * fTextScale),
                y - (int) (CharSize / 2 * CharScale * fTextScale) );
        fputs( line, dest );
        break;

    case PLOT_FORMAT_POST:
        wxString Text = wxT( "Infos" );
        Plot_1_texte( format, Text, 0, TextWidth,
                      x, y,
                      (int) (CharSize * CharScale), (int) (CharSize * CharScale),
                      FALSE );
        break;
    }


    for( ii = 0, foret = (DRILL_TOOL*) adr_lowmem; ii < s_DrillToolsCount; ii++, foret++ )
    {
        int plot_diam;
        if( foret->m_TotalCount == 0 )
            continue;

        plot_diam = (int) (foret->m_Diameter);
        x = plotX; y = plotY;
        x = -g_PlotOffset.x + (int) (x * fTextScale);
        y = g_PlotOffset.y - (int) (y * fTextScale);
        PlotDrillSymbol( wxPoint( x, y ), plot_diam, ii, format );

        intervalle = (int) (CharSize * CharScale) + TextWidth;
        intervalle = (int) ( intervalle * 1.2);

        if( intervalle < (plot_diam + 200 + TextWidth) )
            intervalle = plot_diam + 200 + TextWidth;

        int rayon = plot_diam / 2;
        x = plotX + rayon + (int) (CharSize * CharScale); y = plotY;
        x = -g_PlotOffset.x + (int) (x * fTextScale);
        y = g_PlotOffset.y - (int) (y * fTextScale);

        /* Trace de la legende associee */
        switch( format )
        {
        case PLOT_FORMAT_HPGL:

            // List the diameter of each drill in the selected Drill Unit,
            // and then its diameter in the other Drill Unit.
            if( s_Unit_Drill_is_Inch )
                sprintf( line, "PU %d, %d; LB%2.3f\" / %2.2fmm ",
                         x + (int) (intervalle * CharScale * fTextScale),
                         y - (int) (CharSize / 2 * CharScale * fTextScale),
                         float (foret->m_Diameter) * 0.0001,
                         float (foret->m_Diameter) * 0.00254 );
            else
                sprintf( line, "PU %d, %d; LB%2.2fmm / %2.3f\" ",
                         x + (int) (intervalle * CharScale * fTextScale),
                         y - (int) (CharSize / 2 * CharScale * fTextScale),
                         float (foret->m_Diameter) * 0.00254,
                         float (foret->m_Diameter) * 0.0001 );
            fputs( line, dest );

            // Now list how many holes and ovals are associated with each drill.
            if( ( foret->m_TotalCount == 1 ) && ( foret->m_OvalCount == 0 ) )
                sprintf( line, "(1 hole)\n" );
            else if( foret->m_TotalCount == 1 )  // && ( foret->m_OvalCount == 1 )
                sprintf( line, "(1 hole) (with 1 oblong)\n" );
            else if( foret->m_OvalCount == 0 )
                sprintf( line, "(%d holes)\n",
                         foret->m_TotalCount );
            else if( foret->m_OvalCount == 1 )
                sprintf( line, "(%d holes) (with 1 oblong)\n",
                         foret->m_TotalCount );
            else  //  if ( foret->m_OvalCount > 1 )
                sprintf( line, "(%d holes) (with %d oblongs)\n",
                         foret->m_TotalCount,
                         foret->m_OvalCount );
            fputs( line, dest );
            fputs( "\03;\n", dest );
            break;

        case PLOT_FORMAT_POST:

            // List the diameter of each drill in the selected Drill Unit,
            // and then its diameter in the other Drill Unit.
            if( s_Unit_Drill_is_Inch )
                sprintf( line, "%2.3f\" / %2.2fmm ",
                         float (foret->m_Diameter) * 0.0001,
                         float (foret->m_Diameter) * 0.00254 );
            else
                sprintf( line, "%2.2fmm / %2.3f\" ",
                         float (foret->m_Diameter) * 0.00254,
                         float (foret->m_Diameter) * 0.0001 );
            msg = CONV_FROM_UTF8( line );

            // Now list how many holes and ovals are associated with each drill.
            if( ( foret->m_TotalCount == 1 ) && ( foret->m_OvalCount == 0 ) )
                sprintf( line, "(1 hole)" );
            else if( foret->m_TotalCount == 1 )  // && ( foret->m_OvalCount == 1 )
                sprintf( line, "(1 hole) (with 1 oblong)" );
            else if( foret->m_OvalCount == 0 )
                sprintf( line, "(%d holes)",
                         foret->m_TotalCount );
            else if( foret->m_OvalCount == 1 )
                sprintf( line, "(%d holes) (with 1 oblong)",
                         foret->m_TotalCount );
            else  // if ( foret->m_OvalCount > 1 )
                sprintf( line, "(%d holes) (with %d oblongs)",
                         foret->m_TotalCount,
                         foret->m_OvalCount );
            msg += CONV_FROM_UTF8( line );
            Plot_1_texte( format, msg, 0, TextWidth,
                          x, y,
                          (int) (CharSize * CharScale),
                          (int) (CharSize * CharScale),
                          FALSE );
            break;
        }

        plotY += intervalle;
    }

 
    switch( format )
    {
    case PLOT_FORMAT_HPGL:
        CloseFileHPGL( dest );
        break;

    case PLOT_FORMAT_POST:
        CloseFilePS( dest );
        break;
    }

	setlocale (LC_NUMERIC, "");	// Revert to local notation for float numbers

    g_PlotOrient = old_g_PlotOrient;
}


/********************************************************************/
int WinEDA_DrillFrame::Plot_Drill_PcbMap( DRILL_TOOL* buffer, int format )
/*********************************************************************/

/** Trace la liste des trous a percer en format HPGL ou POSTSCRIPT
 * @return drill count
 * @param buffer = drill list buffer
 * @param format = ouput format (hpgl / ps)
 */
{
    DRILL_TOOL*  foret;
    TRACK*  pt_piste;
    D_PAD*  pt_pad;
    MODULE* Module;
    int     shape_id, diam, nb_trous;
    wxPoint pos;
    wxSize  size;

    nb_trous = 0;

    /* create the drill list */
    if( s_DrillToolsCount > 13 )
    {
        DisplayInfo( this,
                     _(
                         " Drill map: Too many diameter values to draw to draw one symbol per drill value (max 13)\nPlot uses circle shape for some drill values" ),
                     10 );
    }

	// Plot the drill map:
    for( shape_id = 0, foret = (DRILL_TOOL*) buffer; shape_id < s_DrillToolsCount; shape_id++, foret++ )
    {
        /* create the via drill map */
        {
            pt_piste = m_Parent->m_Pcb->m_Track;
            for( ; pt_piste != NULL; pt_piste = (TRACK*) pt_piste->Pnext )
            {
                if( pt_piste->Type() != TYPEVIA )
                    continue;
                int via_drill = pt_piste->GetDrillValue();
				if( via_drill != foret->m_Diameter )
					continue;
                pos = pt_piste->m_Start;
                PlotDrillSymbol( pos, via_drill, shape_id, format );

                nb_trous++;
            }
        }
        /* create the pad drill map: */
        for( Module = m_Parent->m_Pcb->m_Modules; Module != NULL; Module = (MODULE*) Module->Pnext )
        {
            pt_pad = (D_PAD*) Module->m_Pads;
            for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
            {
                switch( pt_pad->m_DrillShape )
                {
                case PAD_CIRCLE:
                    diam = pt_pad->m_Drill.x;
                    if( diam != foret->m_Diameter )
                        continue;
                    PlotDrillSymbol( pt_pad->m_Pos, diam, shape_id, format );
                    break;

                case PAD_OVAL:
                    if( pt_pad->m_DrillShape != PAD_OVAL )
                        continue;
                    diam = MIN( pt_pad->m_Drill.x, pt_pad->m_Drill.y );
                    if( diam != foret->m_Diameter )
                        continue;
                    PlotOvalDrillSymbol( pt_pad->m_Pos, pt_pad->m_Drill, pt_pad->m_Orient, format );
                    break;
                }

                nb_trous++;
            }

            /* Fin examen 1 module */
        }

        /* Fin 1 passe de foret */
    }

    /* fin analyse des trous de modules */

    return nb_trous;
}


/************************************************************************************/
void PlotDrillSymbol( const wxPoint& position, int diametre, int num_forme, int format )
/************************************************************************************/

/* Trace un motif de numero de forme num_forme, aux coord x0, y0.
 *  x0, y0 = coordonnees tables
 *  diametre = diametre (coord table) du trou
 *  num_forme = index ( permet de generer des formes caract )
 */
{
    int  rayon = diametre / 2;

    void (*FctPlume)( wxPoint pos, int state );
    int  x0, y0;

    x0 = position.x; y0 = position.y;
    FctPlume = Move_Plume_HPGL;
    if( format == PLOT_FORMAT_POST )
        FctPlume = LineTo_PS;

    switch( num_forme )
    {
    case 0:     /* vias : forme en X */
        FctPlume( wxPoint( x0 - rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 + rayon ), 'D' );
        FctPlume( wxPoint( x0 + rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 - rayon, y0 + rayon ), 'D' );
        break;

    case 1:     /* Cercle */
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pastille_RONDE_HPGL( wxPoint( x0, y0 ), diametre, FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pastille_RONDE_POST( wxPoint( x0, y0 ), diametre, FILAIRE );
        break;

    case 2:     /* forme en + */
        FctPlume( wxPoint( x0, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0, y0 + rayon ), 'D' );
        FctPlume( wxPoint( x0 + rayon, y0 ), 'U' );
        FctPlume( wxPoint( x0 - rayon, y0 ), 'D' );
        break;

    case 3:     /* forme en X cercle */
        FctPlume( wxPoint( x0 - rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 + rayon ), 'D' );
        FctPlume( wxPoint( x0 + rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 - rayon, y0 + rayon ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pastille_RONDE_HPGL( wxPoint( x0, y0 ), diametre, FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pastille_RONDE_POST( wxPoint( x0, y0 ), diametre, FILAIRE );
        break;

    case 4:     /* forme en cercle barre de - */
        FctPlume( wxPoint( x0 - rayon, y0 ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pastille_RONDE_HPGL( wxPoint( x0, y0 ), diametre, FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pastille_RONDE_POST( wxPoint( x0, y0 ), diametre, FILAIRE );
        break;

    case 5:     /* forme en cercle barre de | */
        FctPlume( wxPoint( x0, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0, y0 + rayon ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pastille_RONDE_HPGL( wxPoint( x0, y0 ), diametre, FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pastille_RONDE_POST( wxPoint( x0, y0 ), diametre, FILAIRE );
        break;

    case 6:     /* forme en carre */
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        break;

    case 7:     /* forme en losange */
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        break;

    case 8:     /* forme en carre barre par un X*/
        FctPlume( wxPoint( x0 - rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 + rayon ), 'D' );
        FctPlume( wxPoint( x0 + rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 - rayon, y0 + rayon ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        break;

    case 9:     /* forme en losange barre par un +*/
        FctPlume( wxPoint( x0, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0, y0 + rayon ), 'D' );
        FctPlume( wxPoint( x0 + rayon, y0 ), 'U' );
        FctPlume( wxPoint( x0 - rayon, y0 ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        break;

    case 10:     /* forme en carre barre par un '/' */
        FctPlume( wxPoint( x0 - rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 + rayon ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        break;

    case 11:     /* forme en losange barre par un |*/
        FctPlume( wxPoint( x0, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0, y0 + rayon ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        break;

    case 12:     /* forme en losange barre par un -*/
        FctPlume( wxPoint( x0 - rayon, y0 ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        break;

    default:
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pastille_RONDE_HPGL( wxPoint( x0, y0 ), diametre, FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pastille_RONDE_POST( wxPoint( x0, y0 ), diametre, FILAIRE );
        break;
    }

    if( format == PLOT_FORMAT_HPGL )
        Plume_HPGL( 'U' );
}


/*******************************************************************************************/
void PlotOvalDrillSymbol( const wxPoint& position, const wxSize& size, int orient, int format )
/*******************************************************************************************/
{
    switch( format )
    {
    case PLOT_FORMAT_HPGL:
        trace_1_pastille_OVALE_HPGL( position, size, orient, FILAIRE );
        break;

    case PLOT_FORMAT_POST:
        trace_1_pastille_OVALE_POST( position, size, orient, FILAIRE );
        break;
    }
}


/*******************************************/
void WinEDA_DrillFrame::GenDrillReport()
/*******************************************/

/*
 *  Create a list of drill values and drill count
 */
{
    wxString FileName, Mask( wxT( "*" ) ), Ext( wxT( ".rpt" ) );
    int      ii, TotalHoleCount;
    char     line[1024];
    DRILL_TOOL*   foret;
    wxString msg;

    FileName = m_Parent->m_CurrentScreen->m_FileName;
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

    m_Parent->MsgPanel->EraseMsgBox();
    Affiche_1_Parametre( m_Parent, 0, _( "File" ), FileName, BLUE );

    /* Determine the list of the different drill diameters. */
    ii = Gen_Liste_Forets( (DRILL_TOOL*) adr_lowmem, FALSE );
    msg.Printf( wxT( "%d" ), ii );
    Affiche_1_Parametre( m_Parent, 30, _( "Tools" ), msg, BROWN );

    fprintf( dest, "Drill report for %s\n", CONV_TO_UTF8( m_Parent->m_CurrentScreen->m_FileName ) );

    fprintf( dest, "Created on %s\n", DateAndTime( line ) );

    // List which Drill Unit option had been selected for the associated drill file.
    if( s_Unit_Drill_is_Inch )
        fputs( "Selected Drill Unit: Imperial (\")\n\n", dest );
    else
        fputs( "Selected Drill Unit: Metric (mm)\n\n", dest );

    TotalHoleCount = 0;

    for( ii = 0, foret = (DRILL_TOOL*) adr_lowmem; ii < s_DrillToolsCount; ii++, foret++ )
    {
        // List the tool number assigned to each drill,
        // then its diameter in the selected Drill Unit,
        // and then its diameter in the other Drill Unit.
        if( s_Unit_Drill_is_Inch )
            sprintf( line, "T%d  %2.3f\"  %2.2fmm  ",
                     ii + 1,
                     float (foret->m_Diameter) * 0.0001,
                     float (foret->m_Diameter) * 0.00254 );
        else
            sprintf( line, "T%d  %2.2fmm  %2.3f\"  ",
                     ii + 1,
                     float (foret->m_Diameter) * 0.00254,
                     float (foret->m_Diameter) * 0.0001 );
        fputs( line, dest );

        // Now list how many holes and ovals are associated with each drill.
        if( ( foret->m_TotalCount == 1 ) && ( foret->m_OvalCount == 0 ) )
            sprintf( line, "(1 hole)\n" );
        else if( foret->m_TotalCount == 1 )
            sprintf( line, "(1 hole)  (with 1 oblong)\n" );
        else if( foret->m_OvalCount == 0 )
            sprintf( line, "(%d holes)\n",
                     foret->m_TotalCount );
        else if( foret->m_OvalCount == 1 )
            sprintf( line, "(%d holes)  (with 1 oblong)\n",
                     foret->m_TotalCount );
        else  //  if ( foret->m_OvalCount > 1 )
            sprintf( line, "(%d holes)  (with %d oblongs)\n",
                     foret->m_TotalCount,
                     foret->m_OvalCount );
        fputs( line, dest );

        TotalHoleCount += foret->m_TotalCount;
    }

    msg.Printf( wxT( "%d" ), TotalHoleCount );
    Affiche_1_Parametre( m_Parent, 45, _( "Drill" ), msg, GREEN );

    fclose( dest );
}
