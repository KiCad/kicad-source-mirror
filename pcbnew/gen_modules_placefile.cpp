/*************************************/
/* fichier gen_modules_placefile.cpp */
/*************************************/

/*
 *  1 - create ascii files for automatic placement of smd components
 *  2 - create a module report (pos and module descr) (ascii file)
 */
#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
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
static int ListeModCmp( const void* o1, const void* o2 )
{
    LIST_MOD*   ref = (LIST_MOD*) o1;
    LIST_MOD*   cmp = (LIST_MOD*) o2;

    return StrLenNumCmp( ref->m_Reference, cmp->m_Reference, 16 );
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
    bool        doBoardBack = false;
    MODULE*     module;
    LIST_MOD*   Liste = 0;
    char        line[1024];
    char        Buff[80];
    wxString    fnFront;
    wxString    fnBack;
    wxString    extension;
    wxString    msg;
    wxString    frontLayerName;
    wxString    backLayerName;
    wxString    Title;
    FILE*       fpFront = 0;
    FILE*       fpBack = 0;
    bool        switchedLocale = false;

    /* Calcul des echelles de conversion */
    double conv_unit = 0.0001; /* unites = INCHES */

//	if(IF_DRILL_METRIC) conv_unit = 0.000254; /* unites = mm */

    File_Place_Offset = m_Auxiliary_Axis_Position;

    /* Calcul du nombre de modules utiles ( Attribut CMS, non VIRTUAL ) ) */
    int moduleCount = 0;

    for( module = GetBoard()->m_Modules;  module;  module = module->Next() )
    {
        if( module->m_Attributs & MOD_VIRTUAL )
        {
            D(printf("skipping module %s because it's virtual\n", CONV_TO_UTF8(module->GetReference()) );)
            continue;
        }

        if( (module->m_Attributs & MOD_CMS)  == 0 )
        {
#if 1 && defined(DEBUG)  // enable this code to fix a bunch of mis-labeled modules:
            if( !HasNonSMDPins( module ) )
            {
                // all module's pins are SMD, mark the part for pick and place
                module->m_Attributs |= MOD_CMS;
            }
            else
            {
                printf( "skipping %s because its attribute is not CMS and it has non SMD pins\n", CONV_TO_UTF8(module->GetReference()) );
                continue;
            }
#else
            continue;
#endif
        }

        if( module->GetLayer() == COPPER_LAYER_N )
            doBoardBack = true;

        moduleCount++;
    }

    if( moduleCount == 0 )
    {
        DisplayError( this, _( "No Modules for Automated Placement" ), 20 );
        return;
    }

    fnFront = GetScreen()->m_FileName;

    frontLayerName = GetBoard()->GetLayerName( CMP_N );
    extension.Printf( wxT("-%s.pos"), frontLayerName.GetData() );

    ChangeFileNameExt( fnFront, extension );

    fpFront = wxFopen( fnFront, wxT( "wt" ) );
    if( fpFront == 0 )
    {
        msg = _( "Unable to create " ) + fnFront;
        DisplayError( this, msg );
        goto exit;
    }

    if( doBoardBack )
    {
        fnBack = GetScreen()->m_FileName;

        backLayerName = GetBoard()->GetLayerName( COPPER_LAYER_N );
        extension.Printf( wxT("-%s.pos"), backLayerName.GetData() );

        ChangeFileNameExt( fnBack, extension );
        fpBack = wxFopen( fnBack, wxT( "wt" ) );
        if( fpBack == 0 )
        {
            msg = _( "Unable to create " ) + fnBack;
            DisplayError( this, msg );
            goto exit;
        }
    }

    // Switch the locale to standard C (needed to print floating point numbers like 1.3)
    SetLocaleTo_C_standard( );
    switchedLocale = true;

    /* Affichage du bilan : */
    MsgPanel->EraseMsgBox();
    Affiche_1_Parametre( this, 0, _( "Component side place file:" ), fnFront, BLUE );

    if( doBoardBack )
        Affiche_1_Parametre( this, 32, _( "Copper side place file:" ), fnBack, BLUE );

    msg.Empty(); msg << moduleCount;
    Affiche_1_Parametre( this, 65, _( "Module count" ), msg, RED );

    /* Etablissement de la liste des modules par ordre alphabetique */
    Liste = (LIST_MOD*) MyZMalloc( moduleCount * sizeof(LIST_MOD) );

    module = GetBoard()->m_Modules;
    for( int ii = 0;  module;  module = module->Next() )
    {
        if( module->m_Attributs & MOD_VIRTUAL )
            continue;

        if( (module->m_Attributs & MOD_CMS)  == 0 )
            continue;

        Liste[ii].m_Module    = module;
        Liste[ii].m_Reference = module->m_Reference->m_Text;
        Liste[ii].m_Value     = module->m_Value->m_Text;
        ii++;
    }

    qsort( Liste, moduleCount, sizeof(LIST_MOD), ListeModCmp );

    /* Generation entete du fichier 'commentaires) */
    sprintf( line, "### Module positions - created on %s ###\n",
            DateAndTime( Buff ) );
    fputs( line, fpFront );
    if( doBoardBack )
        fputs( line, fpBack );

    Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
    sprintf( line, "### Printed by PcbNew version %s\n", CONV_TO_UTF8( Title ) );
    fputs( line, fpFront );
    if( doBoardBack )
        fputs( line, fpBack );

    sprintf( line, "## Unit = inches, Angle = deg.\n" );
    fputs( line, fpFront );
    if( doBoardBack )
        fputs( line, fpBack );

    sprintf( line, "## Side : %s\n", CONV_TO_UTF8( frontLayerName ) );
    fputs( line, fpFront );

    if( doBoardBack )
    {
        sprintf( line, "## Side : %s\n", CONV_TO_UTF8( backLayerName ) );
        fputs( line, fpBack );
    }

    sprintf( line,
             "# Ref    Val                  PosX       PosY        Rot     Side\n" );
    fputs( line, fpFront );
    if( doBoardBack )
        fputs( line, fpBack );

    /* Generation lignes utiles du fichier */
    for( int ii = 0; ii < moduleCount; ii++ )
    {
        wxPoint  module_pos;
        wxString ref = Liste[ii].m_Reference;
        wxString val = Liste[ii].m_Value;
        sprintf( line, "%-8.8s %-16.16s ", CONV_TO_UTF8( ref ), CONV_TO_UTF8( val ) );

        module_pos    = Liste[ii].m_Module->m_Pos;
        module_pos.x -= File_Place_Offset.x;
        module_pos.y -= File_Place_Offset.y;

        char* text = line + strlen( line );
        sprintf( text, " %9.4f  %9.4f  %8.1f    ",
                 module_pos.x * conv_unit,
                 module_pos.y * conv_unit,
                 double(Liste[ii].m_Module->m_Orient) / 10 );

        int layer = Liste[ii].m_Module->GetLayer();

        wxASSERT( layer==CMP_N || layer==COPPER_LAYER_N );

        if( layer == CMP_N )
        {
            strcat( line, CONV_TO_UTF8( frontLayerName ) );
            strcat( line, "\n" );
            fputs( line, fpFront );
        }
        else if( layer == COPPER_LAYER_N )
        {
            strcat( line, CONV_TO_UTF8( backLayerName ) );
            strcat( line, "\n" );
            fputs( line, fpBack );
        }
    }

    /* Generation fin du fichier */
    fputs( "## End\n", fpFront );

    if( doBoardBack )
        fputs( "## End\n", fpBack );

    msg = frontLayerName + wxT( " File: " ) + fnFront;

    if( doBoardBack )
        msg += wxT("\n\n") + backLayerName + wxT( " File: " ) + fnBack;

    DisplayInfo( this, msg );


exit:   // the only safe way out of here, no returns please.

    if( Liste )
        MyFree( Liste );

    if( switchedLocale )
        SetLocaleTo_Default( );      // revert to the current  locale

    if( fpFront )
        fclose( fpFront );

    if( fpBack )
        fclose( fpBack );
}


/**************************************************************/
void WinEDA_PcbFrame::GenModuleReport( wxCommandEvent& event )
/**************************************************************/

/* Print a module report.
 */
{
    double   conv_unit;
    MODULE*  Module;
    D_PAD*   pad;
    char     line[1024], Buff[80];
    wxString FullFileName, fnFront, msg;
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
    SetLocaleTo_C_standard( );

    /* Generation entete du fichier 'commentaires) */
    sprintf( line, "## Module report - date %s\n", DateAndTime( Buff ) );
    fputs( line, rptfile );

    wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
    sprintf( line, "## Created by PcbNew version %s\n", CONV_TO_UTF8( Title ) );
    fputs( line, rptfile );
    fputs( "## Unit = inches, Angle = deg.\n", rptfile );

    /* Generation lignes utiles du fichier */
    fputs( "##\n", rptfile );
    fputs( "\n$BeginDESCRIPTION\n", rptfile );

    GetBoard()->ComputeBoundaryBox();
    fputs( "\n$BOARD\n", rptfile );
    fputs( "unit INCH\n", rptfile );
    sprintf( line, "upper_left_corner %9.6f %9.6f\n",
             GetBoard()->m_BoundaryBox.GetX() * conv_unit,
             GetBoard()->m_BoundaryBox.GetY() * conv_unit );
    fputs( line, rptfile );

    sprintf( line, "lower_right_corner %9.6f %9.6f\n",
             GetBoard()->m_BoundaryBox.GetRight() * conv_unit,
             GetBoard()->m_BoundaryBox.GetBottom() * conv_unit );
    fputs( line, rptfile );

    fputs( "$EndBOARD\n\n", rptfile );

    Module = (MODULE*) GetBoard()->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        sprintf( line, "$MODULE \"%s\"\n", CONV_TO_UTF8( Module->m_Reference->m_Text ) );
        fputs( line, rptfile );

        sprintf( line, "reference \"%s\"\n", CONV_TO_UTF8( Module->m_Reference->m_Text ) );
        fputs( line, rptfile );
        sprintf( line, "value \"%s\"\n", CONV_TO_UTF8( Module->m_Value->m_Text ) );
        fputs( line, rptfile );
        sprintf( line, "footprint \"%s\"\n", CONV_TO_UTF8( Module->m_LibRef ) );
        fputs( line, rptfile );

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

        sprintf( line, "position %9.6f %9.6f\n",
                 module_pos.x * conv_unit,
                 module_pos.y * conv_unit );
        fputs( line, rptfile );

        sprintf( line, "orientation  %.2f\n", (double) Module->m_Orient / 10 );
        if( Module->GetLayer() == CMP_N )
            strcat( line, "layer component\n" );
        else if( Module->GetLayer() == COPPER_LAYER_N )
            strcat( line, "layer copper\n" );
        else
            strcat( line, "layer other\n" );
        fputs( line, rptfile );

        Module->Write_3D_Descr( rptfile );

        for( pad = Module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            fprintf( rptfile, "$PAD \"%.4s\"\n", pad->m_Padname );
            sprintf( line, "position %9.6f %9.6f\n",
                     pad->m_Pos0.x * conv_unit,
                     pad->m_Pos0.y * conv_unit );
            fputs( line, rptfile );

            sprintf( line, "size %9.6f %9.6f\n",
                     pad->m_Size.x * conv_unit,
                     pad->m_Size.y * conv_unit );
            fputs( line, rptfile );
            sprintf( line, "drill %9.6f\n", pad->m_Drill.x * conv_unit );
            fputs( line, rptfile );
            sprintf( line, "shape_offset %9.6f %9.6f\n",
                     pad->m_Offset.x * conv_unit,
                     pad->m_Offset.y * conv_unit );
            fputs( line, rptfile );

            sprintf( line, "orientation  %.2f\n", double(pad->m_Orient - Module->m_Orient) / 10 );
            fputs( line, rptfile );
            const char* shape_name[6] =
                { "??? ", "Circ", "Rect", "Oval", "trap", "spec" };
            sprintf( line, "Shape  %s\n", shape_name[pad->m_PadShape] );
            fputs( line, rptfile );

            int layer = 0;
            if( pad->m_Masque_Layer & CUIVRE_LAYER )
                layer = 1;
            if( pad->m_Masque_Layer & CMP_LAYER )
                layer |= 2;
            const char* layer_name[4] = { "??? ", "copper", "component", "all" };
            sprintf( line, "Layer  %s\n", layer_name[layer] );
            fputs( line, rptfile );
            fprintf( rptfile, "$EndPAD\n" );
        }

        fprintf( rptfile, "$EndMODULE  %s\n\n",
                (const char*) Module->m_Reference->m_Text.GetData() );
    }

    /* Write board Edges */
    EDA_BaseStruct* PtStruct;
    for( PtStruct = GetBoard()->m_Drawings; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        if( PtStruct->Type() != TYPE_DRAWSEGMENT )
            continue;
        if( ( (DRAWSEGMENT*) PtStruct )->GetLayer() != EDGE_N )
            continue;
        WriteDrawSegmentPcb( (DRAWSEGMENT*) PtStruct, rptfile );
    }

    /* Generation fin du fichier */
    fputs( "$EndDESCRIPTION\n", rptfile );
    fclose( rptfile );
    SetLocaleTo_Default( );      // revert to the current  locale
}


/*******************************************************************/
void WriteDrawSegmentPcb( DRAWSEGMENT* PtDrawSegment, FILE* rptfile )
/*******************************************************************/

/* Sortie sur rptfile d'un segment type drawing PCB:
 *      Les contours sont de differents type:
 *      segment
 *      cercle
 *      arc
 */
{
    double conv_unit, ux0, uy0, dx, dy;
    double rayon, width;
    char   line[1024];

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
        fprintf( rptfile, "$CIRCLE \n" );
        fprintf( rptfile, "centre %.6lf %.6lf\n", ux0, uy0 );
        fprintf( rptfile, "radius %.6lf\n", rayon );
        fprintf( rptfile, "width %.6lf\n", width );
        fprintf( rptfile, "$EndCIRCLE \n" );
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

            fprintf( rptfile, "$ARC \n" );
            fprintf( rptfile, "centre %.6lf %.6lf\n", ux0, uy0 );
            fprintf( rptfile, "start %.6lf %.6lf\n", endx * conv_unit, endy * conv_unit );
            fprintf( rptfile, "end %.6lf %.6lf\n", dx, dy );
            fprintf( rptfile, "width %.6lf\n", width );
            fprintf( rptfile, "$EndARC \n" );
        }
        break;

    default:
        sprintf( line, "$LINE \n" );
        fputs( line, rptfile );

        fprintf( rptfile, "start %.6lf %.6lf\n", ux0, uy0 );
        fprintf( rptfile, "end %.6lf %.6lf\n", dx, dy );
        fprintf( rptfile, "width %.6lf\n", width );
        fprintf( rptfile, "$EndLINE \n" );
        break;
    }
}
