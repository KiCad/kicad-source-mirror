/*************************************/
/* fichier gen_modules_placefile.cpp */
/*************************************/

/*
 *  1 - create ascii files for automatic placement of smd components
 *  2 - create a module report (pos and module descr) (ascii file)
 */
#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "trigo.h"

class LIST_MOD      /* Permet de lister les elements utiles des modules */
{
public:
    MODULE*       m_Module;
    const wxChar* m_Reference;
    const wxChar* m_Value;
};


/* variables locale : */
static wxPoint File_Place_Offset;   /* Offset des coord de placement pour le fichier généré */

/* Routines Locales */
static void WriteDrawSegmentPcb( DRAWSEGMENT* PtDrawSegment, FILE* rptfile );

/* Routine de tri utilisee par GenereModulesPosition() */
static int ListeModCmp( LIST_MOD* Ref, LIST_MOD* Cmp )
{
    return StrLenNumCmp( Ref->m_Reference, Cmp->m_Reference, 16 );
}


#if defined(DEBUG)

/**
 * Function HasNonSMDPins
 * returns true if the given module has any non smd pins, such as through hole
 * and therefore cannot be placed automatically.
 */
static bool HasNonSMDPins( MODULE* aModule )
{
    D_PAD* pad;

    for( pad = aModule->m_Pads;  pad;  pad = pad->Next() )
    {
        if( pad->m_Attribut != PAD_SMD )
            return true;
    }

    return false;
}

#endif


/**************************************************************/
void WinEDA_PcbFrame::GenModulesPosition( wxCommandEvent& event )
/**************************************************************/

/* Routine de generation du fichier de positionnement des modules,
 *  utilisé pour les machines de placement de composants
 */
{
    float     conv_unit;
    int       NbMod, ii;
    bool      GenCu = FALSE;
    MODULE*   Module;
    LIST_MOD* Liste;
    char      Line[1024], Buff[80];
    wxString  NameLayerCu, NameLayerCmp, msg;
    FILE*     LayerCu = NULL, * LayerCmp = NULL;

    /* Calcul des echelles de conversion */
    conv_unit = 0.0001; /* unites = INCHES */

//	if(IF_DRILL_METRIC) conv_unit = 0.000254; /* unites = mm */

    File_Place_Offset = m_Auxiliary_Axis_Position;

    /* Calcul du nombre de modules utiles ( Attribut CMS, non VIRTUAL ) ) */
    NbMod = 0;

    for( Module = m_Pcb->m_Modules;  Module;  Module = Module->Next() )
    {
        if( Module->m_Attributs & MOD_VIRTUAL )
        {
            D(printf("skipping module %s because its virtual\n", CONV_TO_UTF8(Module->GetReference()) );)
            continue;
        }

        if( (Module->m_Attributs & MOD_CMS)  == 0 )
        {
#if 0 && defined(DEBUG)  // enable this code to fix a bunch of mis-labeled modules:
            if( !HasNonSMDPins( Module ) )
                Module->m_Attributs |= MOD_CMS;     // all pins are SMD, fix the problem
            else
            {
                printf( "skipping %s because its attribute is not CMS and it has non SMD pins\n", CONV_TO_UTF8(Module->GetReference()) );
                continue;
            }
#else
            continue;
#endif
        }

        if( Module->GetLayer() == COPPER_LAYER_N )
            GenCu = TRUE;
        NbMod++;
    }

    if( NbMod == 0 )
    {
        DisplayError( this, _( "No Modules for Automated Placement" ), 20 ); return;
    }


    /* Init nom fichier */
    NameLayerCmp = GetScreen()->m_FileName;
    ChangeFileNameExt( NameLayerCmp, wxT( "-cmp.pos" ) );

    LayerCmp = wxFopen( NameLayerCmp, wxT( "wt" ) );
    if( LayerCmp == 0 )
    {
        msg = _( "Unable to create " ) + NameLayerCu;
        DisplayError( this, msg ); return;
    }

    if( GenCu )
    {
        NameLayerCu = GetScreen()->m_FileName;
        ChangeFileNameExt( NameLayerCu, wxT( "-copper.pos" ) );
        LayerCu = wxFopen( NameLayerCu, wxT( "wt" ) );
        if( LayerCu == 0 )
        {
            msg = _( "Unable to create " ) + NameLayerCu;
            DisplayError( this, msg );
            fclose( LayerCmp );
            return;
        }
    }

    // Switch the locale to standard C (needed to print floating point numbers like 1.3)
    setlocale( LC_NUMERIC, "C" );

    /* Affichage du bilan : */
    MsgPanel->EraseMsgBox();
    Affiche_1_Parametre( this, 0, _( "Component side place file:" ), NameLayerCmp, BLUE );

    if( GenCu )
        Affiche_1_Parametre( this, 32, _( "Copper side place file:" ), NameLayerCu, BLUE );

    msg.Empty(); msg << NbMod;
    Affiche_1_Parametre( this, 65, _( "Module count" ), msg, RED );


    /* Etablissement de la liste des modules par ordre alphabetique */
    Liste = (LIST_MOD*) MyZMalloc( NbMod * sizeof(LIST_MOD) );

    Module = m_Pcb->m_Modules;
    for( ii = 0;  Module;  Module = Module->Next() )
    {
        if( Module->m_Attributs & MOD_VIRTUAL )
            continue;
        if( (Module->m_Attributs & MOD_CMS)  == 0 )
            continue;

        Liste[ii].m_Module    = Module;
        Liste[ii].m_Reference = Module->m_Reference->m_Text;
        Liste[ii].m_Value = Module->m_Value->m_Text;
        ii++;
    }

    qsort( Liste, NbMod, sizeof(LIST_MOD),
           ( int( * ) ( const void*, const void* ) )ListeModCmp );


    /* Generation entete du fichier 'commentaires) */
    sprintf( Line, "### Module positions - created on %s ###\n",
            DateAndTime( Buff ) );
    fputs( Line, LayerCmp );
    if( GenCu )
        fputs( Line, LayerCu );

    wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
    sprintf( Line, "### Printed by PcbNew version %s\n", CONV_TO_UTF8( Title ) );
    fputs( Line, LayerCmp );
    if( GenCu )
        fputs( Line, LayerCu );

    sprintf( Line, "## Unit = inches, Angle = deg.\n" );
    fputs( Line, LayerCmp );
    if( GenCu )
        fputs( Line, LayerCu );

    sprintf( Line, "## Side : Components\n" );
    fputs( Line, LayerCmp );

    if( GenCu )
    {
        sprintf( Line, "## Side : Copper\n" );
        fputs( Line, LayerCu );
    }

    sprintf( Line,
             "# Ref    Val                  PosX       PosY        Rot     Side\n" );
    fputs( Line, LayerCmp );
    if( GenCu )
        fputs( Line, LayerCu );

    /* Generation lignes utiles du fichier */
    for( ii = 0; ii < NbMod; ii++ )
    {
        wxPoint  module_pos;
        wxString ref = Liste[ii].m_Reference;
        wxString val = Liste[ii].m_Value;
        sprintf( Line, "%-8.8s %-16.16s ", CONV_TO_UTF8( ref ), CONV_TO_UTF8( val ) );

        module_pos    = Liste[ii].m_Module->m_Pos;
        module_pos.x -= File_Place_Offset.x;
        module_pos.y -= File_Place_Offset.y;

        char* text = Line + strlen( Line );
        sprintf( text, " %9.4f  %9.4f  %8.1f    ",
                 (float) module_pos.x * conv_unit,
                 (float) module_pos.y * conv_unit,
                 (float) Liste[ii].m_Module->m_Orient / 10 );

        if( Liste[ii].m_Module->GetLayer() == CMP_N )
        {
            strcat( Line, "Cmp.\n" );
            fputs( Line, LayerCmp );
        }
        else if( Liste[ii].m_Module->GetLayer() == COPPER_LAYER_N )
        {
            strcat( Line, "Cu\n" );
            fputs( Line, LayerCu );
        }
    }

    /* Generation fin du fichier */
    fputs( "## End\n", LayerCmp );
    fclose( LayerCmp );
    if( GenCu )
    {
        fputs( "## End\n", LayerCu );
        fclose( LayerCu );
    }
    MyFree( Liste );
    setlocale( LC_NUMERIC, "" );      // revert to the current  locale

    msg = wxT( "Cmp File: " ) + NameLayerCmp;
    if( GenCu )
        msg += wxT( "\nCu File: " ) + NameLayerCu;

    DisplayInfo( this, msg );
}


/**************************************************************/
void WinEDA_PcbFrame::GenModuleReport( wxCommandEvent& event )
/**************************************************************/

/* Print a module report.
 */
{
    float    conv_unit;
    MODULE*  Module;
    D_PAD*   pad;
    char     Line[1024], Buff[80];
    wxString FullFileName, NameLayerCmp, msg;
    FILE*    rptfile;
    wxPoint  module_pos;

    /* Calcul des echelles de conversion */
    conv_unit = 0.0001; /* unites = INCHES */

//	if(IF_DRILL_METRIC) conv_unit = 0.000254; /* unites = mm */

    File_Place_Offset = wxPoint( 0, 0 );

    /* Init nom fichier */
    FullFileName = GetScreen()->m_FileName;
    ChangeFileNameExt( FullFileName, wxT( ".rpt" ) );

    rptfile = wxFopen( FullFileName, wxT( "wt" ) );
    if( rptfile == NULL )
    {
        msg = _( "Unable to create " ) + FullFileName;
        DisplayError( this, msg ); return;
    }

    // Switch the locale to standard C (needed to print floating point numbers like 1.3)
    setlocale( LC_NUMERIC, "C" );

    /* Generation entete du fichier 'commentaires) */
    sprintf( Line, "## Module report - date %s\n", DateAndTime( Buff ) );
    fputs( Line, rptfile );

    wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
    sprintf( Line, "## Created by PcbNew version %s\n", CONV_TO_UTF8( Title ) );
    fputs( Line, rptfile );
    fputs( "## Unit = inches, Angle = deg.\n", rptfile );

    /* Generation lignes utiles du fichier */
    fputs( "##\n", rptfile );
    fputs( "\n$BeginDESCRIPTION\n", rptfile );

    m_Pcb->ComputeBoundaryBox();
    fputs( "\n$BOARD\n", rptfile );
    fputs( "unit INCH\n", rptfile );
    sprintf( Line, "upper_left_corner %9.6f %9.6f\n",
             (float) m_Pcb->m_BoundaryBox.GetX() * conv_unit,
             (float) m_Pcb->m_BoundaryBox.GetY() * conv_unit );
    fputs( Line, rptfile );

    sprintf( Line, "lower_right_corner %9.6f %9.6f\n",
             (float) ( m_Pcb->m_BoundaryBox.GetRight() ) * conv_unit,
             (float) ( m_Pcb->m_BoundaryBox.GetBottom() ) * conv_unit );
    fputs( Line, rptfile );

    fputs( "$EndBOARD\n\n", rptfile );

    Module = (MODULE*) m_Pcb->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        sprintf( Line, "$MODULE \"%s\"\n", CONV_TO_UTF8( Module->m_Reference->m_Text ) );
        fputs( Line, rptfile );

        sprintf( Line, "reference \"%s\"\n", CONV_TO_UTF8( Module->m_Reference->m_Text ) );
        fputs( Line, rptfile );
        sprintf( Line, "value \"%s\"\n", CONV_TO_UTF8( Module->m_Value->m_Text ) );
        fputs( Line, rptfile );
        sprintf( Line, "footprint \"%s\"\n", CONV_TO_UTF8( Module->m_LibRef ) );
        fputs( Line, rptfile );

        msg = wxT( "attribut" );
        if( Module->m_Attributs & MOD_VIRTUAL )
            msg += wxT( " virtual" );
        if( Module->m_Attributs & MOD_CMS )
            msg += wxT( " smd" );
        if( ( Module->m_Attributs & (MOD_VIRTUAL | MOD_CMS) ) == 0 )
            msg += wxT( " none" );
        msg += wxT( "\n" );
        fputs( CONV_TO_UTF8( msg ), rptfile );

        module_pos    = Module->m_Pos;
        module_pos.x -= File_Place_Offset.x;
        module_pos.y -= File_Place_Offset.y;
        sprintf( Line, "position %9.6f %9.6f\n",
                 (float) module_pos.x * conv_unit,
                 (float) module_pos.y * conv_unit );
        fputs( Line, rptfile );

        sprintf( Line, "orientation  %.2f\n", (float) Module->m_Orient / 10 );
        if( Module->GetLayer() == CMP_N )
            strcat( Line, "layer component\n" );
        else if( Module->GetLayer() == COPPER_LAYER_N )
            strcat( Line, "layer copper\n" );
        else
            strcat( Line, "layer other\n" );
        fputs( Line, rptfile );

        Module->Write_3D_Descr( rptfile );

        for( pad = Module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            fprintf( rptfile, "$PAD \"%.4s\"\n", pad->m_Padname );
            sprintf( Line, "position %9.6f %9.6f\n",
                     (float) pad->m_Pos0.x * conv_unit,
                     (float) pad->m_Pos0.y * conv_unit );
            fputs( Line, rptfile );

            sprintf( Line, "size %9.6f %9.6f\n",
                     (float) pad->m_Size.x * conv_unit,
                     (float) pad->m_Size.y * conv_unit );
            fputs( Line, rptfile );
            sprintf( Line, "drill %9.6f\n", (float) pad->m_Drill.x * conv_unit );
            fputs( Line, rptfile );
            sprintf( Line, "shape_offset %9.6f %9.6f\n",
                     (float) pad->m_Offset.x * conv_unit,
                     (float) pad->m_Offset.y * conv_unit );
            fputs( Line, rptfile );

            sprintf( Line, "orientation  %.2f\n", (float) (pad->m_Orient - Module->m_Orient) / 10 );
            fputs( Line, rptfile );
            const char* shape_name[6] =
                { "??? ", "Circ", "Rect", "Oval", "trap", "spec" };
            sprintf( Line, "Shape  %s\n", shape_name[pad->m_PadShape] );
            fputs( Line, rptfile );

            int layer = 0;
            if( pad->m_Masque_Layer & CUIVRE_LAYER )
                layer = 1;
            if( pad->m_Masque_Layer & CMP_LAYER )
                layer |= 2;
            const char* layer_name[4] = { "??? ", "copper", "component", "all" };
            sprintf( Line, "Layer  %s\n", layer_name[layer] );
            fputs( Line, rptfile );
            fprintf( rptfile, "$EndPAD\n" );
        }

        fprintf( rptfile, "$EndMODULE  %s\n\n",
                (const char*) Module->m_Reference->m_Text.GetData() );
    }

    /* Write board Edges */
    EDA_BaseStruct* PtStruct;
    for( PtStruct = m_Pcb->m_Drawings; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        if( PtStruct->Type() != TYPEDRAWSEGMENT )
            continue;
        if( ( (DRAWSEGMENT*) PtStruct )->GetLayer() != EDGE_N )
            continue;
        WriteDrawSegmentPcb( (DRAWSEGMENT*) PtStruct, rptfile );
    }

    /* Generation fin du fichier */
    fputs( "$EndDESCRIPTION\n", rptfile );
    fclose( rptfile );
    setlocale( LC_NUMERIC, "" );      // revert to the current  locale
}


/*******************************************************************/
void WriteDrawSegmentPcb( DRAWSEGMENT* PtDrawSegment, FILE* rptfile )
/*******************************************************************/

/* Sortie dsur rptfile d'un segment type drawing PCB:
 *      Les contours sont de differents type:
 *      segment
 *      cercle
 *      arc
 */
{
    double conv_unit, ux0, uy0, dx, dy;
    double rayon, width;
    char   Line[1024];

    /* Calcul des echelles de conversion */
    conv_unit = 0.0001; /* unites = INCHES */
    /* coord de depart */
    ux0 = PtDrawSegment->m_Start.x * conv_unit;
    uy0 = PtDrawSegment->m_Start.y * conv_unit;
    /* coord d'arrivee */
    dx = PtDrawSegment->m_End.x * conv_unit;
    dy = PtDrawSegment->m_End.y * conv_unit;

    width = PtDrawSegment->m_Width * conv_unit;

    switch( PtDrawSegment->m_Shape )
    {
    case S_CIRCLE:
        rayon = hypot( dx - ux0, dy - uy0 );
        sprintf( Line, "$CIRCLE \n" ); fputs( Line, rptfile );
        sprintf( Line, "centre %.6lf %.6lf\n", ux0, uy0 );
        sprintf( Line, "radius %.6lf\n", rayon );
        sprintf( Line, "width %.6lf\n", width );
        sprintf( Line, "$EndCIRCLE \n" );
        fputs( Line, rptfile );
        break;

    case S_ARC:
    {
        int endx = PtDrawSegment->m_End.x, endy = PtDrawSegment->m_End.y;
        rayon = hypot( dx - ux0, dy - uy0 );
        RotatePoint( &endx,
                     &endy,
                     PtDrawSegment->m_Start.x,
                     PtDrawSegment->m_Start.y,
                     PtDrawSegment->m_Angle );
        sprintf( Line, "$ARC \n" ); fputs( Line, rptfile );
        sprintf( Line, "centre %.6lf %.6lf\n", ux0, uy0 );
        sprintf( Line, "start %.6lf %.6lf\n", endx * conv_unit, endy * conv_unit );
        sprintf( Line, "end %.6lf %.6lf\n", dx, dy );
        sprintf( Line, "width %.6lf\n", width );
        sprintf( Line, "$EndARC \n" );
        fputs( Line, rptfile );
    }
        break;

    default:
        sprintf( Line, "$LINE \n" );
        fputs( Line, rptfile );
        sprintf( Line, "start %.6lf %.6lf\n", ux0, uy0 );
        sprintf( Line, "end %.6lf %.6lf\n", dx, dy );
        sprintf( Line, "width %.6lf\n", width );
        sprintf( Line, "$EndLINE \n" );
        fputs( Line, rptfile );
        break;
    }
}
