
 /**
 * @file item_io.cpp
 * @brief Routines for reading and saving of structures in ASCII file common to Pcbnew and CvPcb.
 *  This is migrationary and temporary while we move the IO_MGR.
 */

#include <fctsys.h>
#include <confirm.h>
#include <kicad_string.h>
#include <build_version.h>
#include <wxPcbStruct.h>
#include <richio.h>
#include <macros.h>
#include <pcbcommon.h>

#ifdef PCBNEW
/**
 * @todo Fix having to recompile the same file with a different defintion.  This is
 *       what C++ derivation was designed to solve.
 */
//#include "zones.h"
#endif

#include <zones.h>

#ifdef CVPCB
#include <cvpcb.h>
#endif

#include <config.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_pcb_text.h>
#include <class_zone.h>
#include <class_dimension.h>
#include <class_drawsegment.h>
#include <class_mire.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <autorout.h>

#include <3d_struct.h>
#include <trigo.h>
#include <class_edge_mod.h>
#include <pcbnew.h>
#include <drawtxt.h>

#define MAX_WIDTH 10000  /* Thickness (in 1 / 10000 ") of maximum reasonable features, text... */


#if 1 || !defined(USE_NEW_PCBNEW_SAVE)

bool BOARD::Save( FILE* aFile ) const
{
    bool        rc = false;
    BOARD_ITEM* item;

    // save the nets
    for( unsigned ii = 0; ii < GetNetCount(); ii++ )
        if( !FindNet( ii )->Save( aFile ) )
            goto out;

    // Saved nets do not include netclass names, so save netclasses after nets.
    m_NetClasses.Save( aFile );

    // save the modules
    for( item = m_Modules; item; item = item->Next() )
        if( !item->Save( aFile ) )
            goto out;

    for( item = m_Drawings; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_TEXT_T:
        case PCB_LINE_T:
        case PCB_TARGET_T:
        case PCB_DIMENSION_T:
            if( !item->Save( aFile ) )
                goto out;

            break;

        default:

            // future: throw exception here
#if defined(DEBUG)
            printf( "BOARD::Save() ignoring m_Drawings type %d\n", item->Type() );
#endif
            break;
        }
    }

    // do not save MARKER_PCBs, they can be regenerated easily

    // save the tracks & vias
    fprintf( aFile, "$TRACK\n" );

    for( item = m_Track; item; item = item->Next() )
    {
        if( !item->Save( aFile ) )
            goto out;
    }

    fprintf( aFile, "$EndTRACK\n" );

    // save the zones
    fprintf( aFile, "$ZONE\n" );

    for( item = m_Zone; item; item = item->Next() )
    {
        if( !item->Save( aFile ) )
            goto out;
    }

    fprintf( aFile, "$EndZONE\n" );

    // save the zone edges
    for( unsigned ii = 0; ii < m_ZoneDescriptorList.size(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_ZoneDescriptorList[ii];
        edge_zone->Save( aFile );
    }


    if( fprintf( aFile, "$EndBOARD\n" ) != sizeof("$EndBOARD\n") - 1 )
        goto out;

    rc = true;  // wrote all OK

out:
    return rc;
}


bool DRAWSEGMENT::Save( FILE* aFile ) const
{
    if( fprintf( aFile, "$DRAWSEGMENT\n" ) != sizeof("$DRAWSEGMENT\n") - 1 )
        return false;

    fprintf( aFile, "Po %d %d %d %d %d %d\n",
             m_Shape,
             m_Start.x, m_Start.y,
             m_End.x, m_End.y, m_Width );

    if( m_Type != S_CURVE )
    {
        fprintf( aFile, "De %d %d %g %lX %X\n",
                 m_Layer, m_Type, GetAngle(),
                 m_TimeStamp, GetStatus() );
    }
    else
    {
        fprintf( aFile, "De %d %d %g %lX %X %d %d %d %d\n",
                 m_Layer, m_Type, GetAngle(),
                 m_TimeStamp, GetStatus(),
                 m_BezierC1.x,m_BezierC1.y,
                 m_BezierC2.x,m_BezierC2.y);
    }

    if( fprintf( aFile, "$EndDRAWSEGMENT\n" ) != sizeof("$EndDRAWSEGMENT\n") - 1 )
        return false;

    return true;
}


/** Note: the old name of class NETINFO_ITEM was EQUIPOT
 * so in Save (and read) functions, for compatibility, we use EQUIPOT as
 * keyword
 */
bool NETINFO_ITEM::Save( FILE* aFile ) const
{
    bool success = false;

    fprintf( aFile, "$EQUIPOT\n" );
    fprintf( aFile, "Na %d %s\n", GetNet(), EscapedUTF8( m_Netname ).c_str() );
    fprintf( aFile, "St %s\n", "~" );

    if( fprintf( aFile, "$EndEQUIPOT\n" ) != sizeof("$EndEQUIPOT\n") - 1 )
        goto out;

    success = true;

out:
    return success;
}


bool PCB_TARGET::Save( FILE* aFile ) const
{
    bool rc = false;

    if( fprintf( aFile, "$PCB_TARGET\n" ) != sizeof("$PCB_TARGET\n")-1 )
        goto out;

    fprintf( aFile, "Po %X %d %d %d %d %d %8.8lX\n",
             m_Shape, m_Layer,
             m_Pos.x, m_Pos.y,
             m_Size, m_Width, m_TimeStamp );

    if( fprintf( aFile, "$EndPCB_TARGET\n" ) != sizeof("$EndPCB_TARGET\n")-1 )
        goto out;

    rc = true;

out:
    return rc;
}


bool ZONE_CONTAINER::Save( FILE* aFile ) const
{
    unsigned item_pos;
    int      ret;
    unsigned corners_count = m_Poly->corner.size();
    int      outline_hatch;
    char     padoption;

    fprintf( aFile, "$CZONE_OUTLINE\n" );

    // Save the outline main info
    ret = fprintf( aFile, "ZInfo %8.8lX %d %s\n",
                   m_TimeStamp, GetNet(),
                   EscapedUTF8( m_Netname ).c_str() );

    if( ret < 3 )
        return false;

    // Save the outline layer info
    ret = fprintf( aFile, "ZLayer %d\n", m_Layer );

    if( ret < 1 )
        return false;

    // Save the outline aux info
    switch( m_Poly->GetHatchStyle() )
    {
    default:
    case CPolyLine::NO_HATCH:
        outline_hatch = 'N';
        break;

    case CPolyLine::DIAGONAL_EDGE:
        outline_hatch = 'E';
        break;

    case CPolyLine::DIAGONAL_FULL:
        outline_hatch = 'F';
        break;
    }

    ret = fprintf( aFile, "ZAux %d %c\n", corners_count, outline_hatch );

    if( ret < 2 )
        return false;

    // Save pad option and clearance
    switch( m_PadOption )
    {
    default:
    case PAD_IN_ZONE:
        padoption = 'I';
        break;

    case THERMAL_PAD:
        padoption = 'T';
        break;

    case PAD_NOT_IN_ZONE:
        padoption = 'X';
        break;
    }

    ret = fprintf( aFile, "ZClearance %d %c\n", m_ZoneClearance, padoption );

    if( ret < 2 )
        return false;

    ret = fprintf( aFile, "ZMinThickness %d\n", m_ZoneMinThickness );

    if( ret < 1 )
        return false;

    ret = fprintf( aFile,
                   "ZOptions %d %d %c %d %d\n",
                   m_FillMode,
                   m_ArcToSegmentsCount,
                   m_IsFilled ? 'S' : 'F',
                   m_ThermalReliefGap,
                   m_ThermalReliefCopperBridge );

    if( ret < 3 )
        return false;

    ret = fprintf( aFile,
                   "ZSmoothing %d %d\n",
                   cornerSmoothingType, cornerRadius );

    if( ret < 2 )
        return false;

    // Save the corner list
    for( item_pos = 0; item_pos < corners_count; item_pos++ )
    {
        ret = fprintf( aFile, "ZCorner %d %d %d\n",
                       m_Poly->corner[item_pos].x, m_Poly->corner[item_pos].y,
                       m_Poly->corner[item_pos].end_contour );

        if( ret < 3 )
            return false;
    }

    // Save the PolysList
    if( m_FilledPolysList.size() )
    {
        fprintf( aFile, "$POLYSCORNERS\n" );

        for( unsigned ii = 0; ii < m_FilledPolysList.size(); ii++ )
        {
            const CPolyPt* corner = &m_FilledPolysList[ii];
            ret = fprintf( aFile,
                           "%d %d %d %d\n",
                           corner->x,
                           corner->y,
                           corner->end_contour,
                           corner->utility );

            if( ret < 4 )
                return false;
        }

        fprintf( aFile, "$endPOLYSCORNERS\n" );
    }

    // Save the filling segments list
    if( m_FillSegmList.size() )
    {
        fprintf( aFile, "$FILLSEGMENTS\n" );

        for( unsigned ii = 0; ii < m_FillSegmList.size(); ii++ )
        {
            ret = fprintf( aFile, "%d %d %d %d\n",
                           m_FillSegmList[ii].m_Start.x, m_FillSegmList[ii].m_Start.y,
                           m_FillSegmList[ii].m_End.x, m_FillSegmList[ii].m_End.y );

            if( ret < 4 )
                return false;
        }

        fprintf( aFile, "$endFILLSEGMENTS\n" );
    }

    fprintf( aFile, "$endCZONE_OUTLINE\n" );

    return true;
}


bool NETCLASSES::Save( FILE* aFile ) const
{
    bool result;

    // save the default first.
    result = m_Default.Save( aFile );

    if( result )
    {
        // the rest will be alphabetical in the *.brd file.
        for( const_iterator i = begin();  i!=end();  ++i )
        {
            NETCLASS*   netclass = i->second;

            result = netclass->Save( aFile );
            if( !result )
                break;
        }
    }

    return result;
}


bool NETCLASS::Save( FILE* aFile ) const
{
    bool result = true;

    fprintf( aFile, "$NCLASS\n" );
    fprintf( aFile, "Name %s\n",        EscapedUTF8( m_Name ).c_str() );
    fprintf( aFile, "Desc %s\n",        EscapedUTF8( GetDescription() ).c_str() );

    // Write parameters

    fprintf( aFile, "Clearance %d\n",       GetClearance() );
    fprintf( aFile, "TrackWidth %d\n",      GetTrackWidth() );

    fprintf( aFile, "ViaDia %d\n",          GetViaDiameter() );
    fprintf( aFile, "ViaDrill %d\n",        GetViaDrill() );

    fprintf( aFile, "uViaDia %d\n",         GetuViaDiameter() );
    fprintf( aFile, "uViaDrill %d\n",       GetuViaDrill() );

    // Write members:
    for( const_iterator i = begin();  i!=end();  ++i )
        fprintf( aFile, "AddNet %s\n", EscapedUTF8( *i ).c_str() );

    fprintf( aFile, "$EndNCLASS\n" );

    return result;
}


bool TEXTE_PCB::Save( FILE* aFile ) const
{
    if( m_Text.IsEmpty() )
        return true;

    if( fprintf( aFile, "$TEXTPCB\n" ) != sizeof("$TEXTPCB\n") - 1 )
        return false;

    const char* style = m_Italic ? "Italic" : "Normal";

    wxArrayString* list = wxStringSplit( m_Text, '\n' );

    for( unsigned ii = 0; ii < list->Count(); ii++ )
    {
        wxString txt  = list->Item( ii );

        if ( ii == 0 )
            fprintf( aFile, "Te %s\n", EscapedUTF8( txt ).c_str() );
        else
            fprintf( aFile, "nl %s\n", EscapedUTF8( txt ).c_str() );
    }

    delete list;

    fprintf( aFile, "Po %d %d %d %d %d %g\n",
             m_Pos.x, m_Pos.y, m_Size.x, m_Size.y, m_Thickness, GetOrientation() );

    char hJustify = 'L';
    switch( m_HJustify )
    {
    case GR_TEXT_HJUSTIFY_LEFT:
        hJustify = 'L';
        break;
    case GR_TEXT_HJUSTIFY_CENTER:
        hJustify = 'C';
        break;
    case GR_TEXT_HJUSTIFY_RIGHT:
        hJustify = 'R';
        break;
    default:
        hJustify = 'C';
        break;
    }

    fprintf( aFile, "De %d %d %lX %s %c\n", m_Layer,
             m_Mirror ? 0 : 1,
             m_TimeStamp, style, hJustify );

    if( fprintf( aFile, "$EndTEXTPCB\n" ) != sizeof("$EndTEXTPCB\n") - 1 )
        return false;

    return true;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool TEXTE_MODULE::Save( FILE* aFile ) const
{
    MODULE* parent = (MODULE*) GetParent();
    int     orient = m_Orient;

    // Due to the Pcbnew history, m_Orient is saved in screen value
    // but it is handled as relative to its parent footprint
    if( parent )
        orient += parent->m_Orient;

    int ret = fprintf( aFile, "T%d %d %d %d %d %d %d %c %c %d %c %s\n",
                      m_Type,
                      m_Pos0.x, m_Pos0.y,
                      m_Size.y, m_Size.x,
                      orient,
                      m_Thickness,
                      m_Mirror ? 'M' : 'N', m_NoShow ? 'I' : 'V',
                      GetLayer(),
                      m_Italic ? 'I' : 'N',
                      EscapedUTF8( m_Text ).c_str()
                      );

    return ret > 20;
}


bool EDGE_MODULE::Save( FILE* aFile ) const
{
    int ret = -1;

    switch( m_Shape )
    {
    case S_SEGMENT:
        ret = fprintf( aFile, "DS %d %d %d %d %d %d\n",
                       m_Start0.x, m_Start0.y,
                       m_End0.x, m_End0.y,
                       m_Width, m_Layer );
        break;

    case S_CIRCLE:
        ret = fprintf( aFile, "DC %d %d %d %d %d %d\n",
                       m_Start0.x, m_Start0.y,
                       m_End0.x, m_End0.y,
                       m_Width, m_Layer );
        break;

    case S_ARC:
        ret = fprintf( aFile, "DA %d %d %d %d %g %d %d\n",
                       m_Start0.x, m_Start0.y,
                       m_End0.x, m_End0.y,
                       GetAngle(),
                       m_Width, m_Layer );
        break;

    case S_POLYGON:
        ret = fprintf( aFile, "DP %d %d %d %d %d %d %d\n",
                       m_Start0.x, m_Start0.y,
                       m_End0.x, m_End0.y,
                       (int) m_PolyPoints.size(),
                       m_Width, m_Layer );

        for( unsigned i = 0;  i<m_PolyPoints.size();  ++i )
            fprintf( aFile, "Dl %d %d\n", m_PolyPoints[i].x, m_PolyPoints[i].y );

        break;

    default:

        // future: throw an exception here
#if defined(DEBUG)
        printf( "EDGE_MODULE::Save(): unexpected m_Shape: %d\n", m_Shape );
#endif
        break;
    }

    return ret > 5;
}


bool TRACK::Save( FILE* aFile ) const
{
    int type = 0;

    if( Type() == PCB_VIA_T )
        type = 1;

    fprintf( aFile, "Po %d %d %d %d %d %d %d\n", m_Shape,
             m_Start.x, m_Start.y, m_End.x, m_End.y, m_Width, m_Drill );

    fprintf( aFile, "De %d %d %d %lX %X\n",
             m_Layer, type, GetNet(),
             m_TimeStamp, GetStatus() );

    return true;
}


bool DIMENSION::Save( FILE* aFile ) const
{
    bool rc = false;

    // note: COTATION was the previous name of DIMENSION
    // this old keyword is used here for compatibility
    const char keyWordLine[] = "$COTATION\n";
    const char keyWordLineEnd[] = "$endCOTATION\n";

    if( fputs( keyWordLine, aFile ) == EOF )
        goto out;

    fprintf( aFile, "Ge %d %d %lX\n", m_Shape, m_Layer, m_TimeStamp );

    fprintf( aFile, "Va %d\n", m_Value );

    if( !m_Text.GetText().IsEmpty() )
        fprintf( aFile, "Te %s\n", EscapedUTF8( m_Text.GetText() ).c_str() );
    else
        fprintf( aFile, "Te \"?\"\n" );

    fprintf( aFile, "Po %d %d %d %d %d %g %d\n",
             m_Text.m_Pos.x, m_Text.m_Pos.y,
             m_Text.m_Size.x, m_Text.m_Size.y,
             m_Text.GetThickness(), m_Text.GetOrientation(),
             m_Text.m_Mirror ? 0 : 1 );

    fprintf( aFile, "Sb %d %d %d %d %d %d\n", S_SEGMENT,
             m_crossBarOx, m_crossBarOy,
             m_crossBarFx, m_crossBarFy, m_Width );

    fprintf( aFile, "Sd %d %d %d %d %d %d\n", S_SEGMENT,
             m_featureLineDOx, m_featureLineDOy,
             m_featureLineDFx, m_featureLineDFy, m_Width );

    fprintf( aFile, "Sg %d %d %d %d %d %d\n", S_SEGMENT,
             m_featureLineGOx, m_featureLineGOy,
             m_featureLineGFx, m_featureLineGFy, m_Width );

    fprintf( aFile, "S1 %d %d %d %d %d %d\n", S_SEGMENT,
             m_arrowD1Ox, m_arrowD1Oy,
             m_arrowD1Fx, m_arrowD1Fy, m_Width );

    fprintf( aFile, "S2 %d %d %d %d %d %d\n", S_SEGMENT,
             m_arrowD2Ox, m_arrowD2Oy,
             m_arrowD2Fx, m_arrowD2Fy, m_Width );


    fprintf( aFile, "S3 %d %d %d %d %d %d\n", S_SEGMENT,
             m_arrowG1Ox, m_arrowG1Oy,
             m_arrowG1Fx, m_arrowG1Fy, m_Width );

    fprintf( aFile, "S4 %d %d %d %d %d %d\n", S_SEGMENT,
             m_arrowG2Ox, m_arrowG2Oy,
             m_arrowG2Fx, m_arrowG2Fy, m_Width );

    if( fputs( keyWordLineEnd, aFile ) == EOF )
        goto out;

    rc = true;

out:
    return rc;
}


bool D_PAD::Save( FILE* aFile ) const
{
    int         cshape;
    const char* texttype;

    // check the return values for first and last fprints() in this function
    if( fprintf( aFile, "$PAD\n" ) != sizeof("$PAD\n") - 1 )
        return false;

    switch( m_PadShape )
    {
    case PAD_CIRCLE:
        cshape = 'C'; break;

    case PAD_RECT:
        cshape = 'R'; break;

    case PAD_OVAL:
        cshape = 'O'; break;

    case PAD_TRAPEZOID:
        cshape = 'T'; break;

    default:
        cshape = 'C';
        DisplayError( NULL, _( "Unknown pad shape" ) );
        break;
    }

    fprintf( aFile, "Sh \"%.4s\" %c %d %d %d %d %g\n",
             m_Padname, cshape, m_Size.x, m_Size.y,
             m_DeltaSize.x, m_DeltaSize.y, m_Orient );

    fprintf( aFile, "Dr %d %d %d", m_Drill.x, m_Offset.x, m_Offset.y );

    if( m_DrillShape == PAD_OVAL )
    {
        fprintf( aFile, " %c %d %d", 'O', m_Drill.x, m_Drill.y );
    }

    fprintf( aFile, "\n" );

    switch( m_Attribut )
    {
    case PAD_STANDARD:
        texttype = "STD"; break;

    case PAD_SMD:
        texttype = "SMD"; break;

    case PAD_CONN:
        texttype = "CONN"; break;

    case PAD_HOLE_NOT_PLATED:
        texttype = "HOLE"; break;

    default:
        texttype = "STD";
        DisplayError( NULL, wxT( "Invalid Pad attribute" ) );
        break;
    }

    fprintf( aFile, "At %s N %8.8X\n", texttype, m_layerMask );

    fprintf( aFile, "Ne %d %s\n", GetNet(), EscapedUTF8( m_Netname ).c_str() );

    fprintf( aFile, "Po %d %d\n", m_Pos0.x, m_Pos0.y );

    if( m_LengthDie != 0 )
        fprintf( aFile, "Le %d\n", m_LengthDie );

    if( m_LocalSolderMaskMargin != 0 )
        fprintf( aFile, ".SolderMask %d\n", m_LocalSolderMaskMargin );

    if( m_LocalSolderPasteMargin != 0 )
        fprintf( aFile, ".SolderPaste %d\n", m_LocalSolderPasteMargin );

    if( m_LocalSolderPasteMarginRatio != 0 )
        fprintf( aFile, ".SolderPasteRatio %g\n", m_LocalSolderPasteMarginRatio );

    if( m_LocalClearance != 0 )
        fprintf( aFile, ".LocalClearance %d\n", m_LocalClearance );

    if( fprintf( aFile, "$EndPAD\n" ) != sizeof("$EndPAD\n") - 1 )
        return false;

    return true;
}


bool MODULE::Save( FILE* aFile ) const
{
    char        statusTxt[8];
    BOARD_ITEM* item;

    bool rc = false;

    fprintf( aFile, "$MODULE %s\n", TO_UTF8( m_LibRef ) );

    memset( statusTxt, 0, sizeof(statusTxt) );
    if( IsLocked() )
        statusTxt[0] = 'F';
    else
        statusTxt[0] = '~';

    if( m_ModuleStatus & MODULE_is_PLACED )
        statusTxt[1] = 'P';
    else
        statusTxt[1] = '~';

    fprintf( aFile, "Po %d %d %g %d %8.8lX %8.8lX %s\n",
             m_Pos.x, m_Pos.y,
             GetOrientation(), m_Layer, m_LastEdit_Time,
             m_TimeStamp, statusTxt );

    fprintf( aFile, "Li %s\n", TO_UTF8( m_LibRef ) );

    if( !m_Doc.IsEmpty() )
    {
        fprintf( aFile, "Cd %s\n", TO_UTF8( m_Doc ) );
    }

    if( !m_KeyWord.IsEmpty() )
    {
        fprintf( aFile, "Kw %s\n", TO_UTF8( m_KeyWord ) );
    }

    fprintf( aFile, "Sc %8.8lX\n", m_TimeStamp );
    fprintf( aFile, "AR %s\n", TO_UTF8( m_Path ) );
    fprintf( aFile, "Op %X %X 0\n", m_CntRot90, m_CntRot180 );

    if( m_LocalSolderMaskMargin != 0 )
        fprintf( aFile, ".SolderMask %d\n", m_LocalSolderMaskMargin );

    if( m_LocalSolderPasteMargin != 0 )
        fprintf( aFile, ".SolderPaste %d\n", m_LocalSolderPasteMargin );

    if( m_LocalSolderPasteMarginRatio != 0 )
        fprintf( aFile, ".SolderPasteRatio %g\n", m_LocalSolderPasteMarginRatio );

    if( m_LocalClearance != 0 )
        fprintf( aFile, ".LocalClearance %d\n", m_LocalClearance );

    // attributes
    if( m_Attributs != MOD_DEFAULT )
    {
        fprintf( aFile, "At " );

        if( m_Attributs & MOD_CMS )
            fprintf( aFile, "SMD " );

        if( m_Attributs & MOD_VIRTUAL )
            fprintf( aFile, "VIRTUAL " );

        fprintf( aFile, "\n" );
    }

    // save reference
    if( !m_Reference->Save( aFile ) )
        goto out;

    // save value
    if( !m_Value->Save( aFile ) )
        goto out;

    // save drawing elements
    for( item = m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
        case PCB_MODULE_EDGE_T:
            if( !item->Save( aFile ) )
                goto out;

            break;

        default:
#if defined(DEBUG)
            printf( "MODULE::Save() ignoring type %d\n", item->Type() );
#endif
            break;
        }
    }

    // save the pads
    for( item = m_Pads;  item;  item = item->Next() )
        if( !item->Save( aFile ) )
            goto out;

    Write_3D_Descr( aFile );

    fprintf( aFile, "$EndMODULE  %s\n", TO_UTF8( m_LibRef ) );

    rc = true;
out:
    return rc;
}

/* Save the description of 3D MODULE
 */
int MODULE::Write_3D_Descr( FILE* File ) const
{
    char buf[512];

    for( S3D_MASTER* t3D = m_3D_Drawings;  t3D;  t3D = t3D->Next() )
    {
        if( !t3D->m_Shape3DName.IsEmpty() )
        {
            fprintf( File, "$SHAPE3D\n" );

            fprintf( File, "Na %s\n", EscapedUTF8( t3D->m_Shape3DName ).c_str() );

            sprintf( buf, "Sc %lf %lf %lf\n",
                     t3D->m_MatScale.x,
                     t3D->m_MatScale.y,
                     t3D->m_MatScale.z );
            fprintf( File, "%s", to_point( buf ) );

            sprintf( buf, "Of %lf %lf %lf\n",
                     t3D->m_MatPosition.x,
                     t3D->m_MatPosition.y,
                     t3D->m_MatPosition.z );
            fprintf( File, "%s", to_point( buf ) );

            sprintf( buf, "Ro %lf %lf %lf\n",
                     t3D->m_MatRotation.x,
                     t3D->m_MatRotation.y,
                     t3D->m_MatRotation.z );
            fprintf( File, "%s", to_point( buf ) );

            fprintf( File, "$EndSHAPE3D\n" );
        }
    }

    return 0;
}

#endif  // USE_NEW_PCBNEW_SAVE


#if 1 || !defined(USE_NEW_PCBNEW_LOAD)

/* Read pad from file.
 * The 1st line of descr ($PAD) is assumed to be already read
 * Syntax:
 * $PAD
 * Sh "N1" C 550 550 0 0 1800
 * Dr 310 0 0
 * At STD N 00C0FFFF
 * Do 3 "netname"
 * Po 6000 -6000
 * $EndPAD
 */
int D_PAD::ReadDescr( LINE_READER* aReader )
{
    char* Line;
    char  BufLine[1024], BufCar[256];
    char* PtLine;
    int   nn, ll, dx, dy;

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();

        if( Line[0] == '$' )
            return 0;

        PtLine = Line + 3;

        /* Decode the first code and read the corresponding data
         */
        switch( Line[0] )
        {
        case 'S': // = Sh
            /* Read pad name */
            nn = 0;

            while( (*PtLine != '"') && *PtLine )
                PtLine++;

            if( *PtLine )
                PtLine++;

            memset( m_Padname, 0, sizeof(m_Padname) );

            while( (*PtLine != '"') && *PtLine )
            {
                if( nn < (int) sizeof(m_Padname) )
                {
                    if( *PtLine > ' ' )
                    {
                        m_Padname[nn] = *PtLine; nn++;
                    }
                }
                PtLine++;
            }

            if( *PtLine == '"' )
                PtLine++;

            nn = sscanf( PtLine, " %s %d %d %d %d %lf",
                         BufCar, &m_Size.x, &m_Size.y,
                         &m_DeltaSize.x, &m_DeltaSize.y,
                         &m_Orient );

            ll = 0xFF & BufCar[0];

            /* Read pad shape */
            m_PadShape = PAD_CIRCLE;

            switch( ll )
            {
            case 'C':
                m_PadShape = PAD_CIRCLE; break;

            case 'R':
                m_PadShape = PAD_RECT; break;

            case 'O':
                m_PadShape = PAD_OVAL; break;

            case 'T':
                m_PadShape = PAD_TRAPEZOID; break;
            }

            ComputeShapeMaxRadius();
            break;

        case 'D':
            BufCar[0] = 0;
            nn = sscanf( PtLine, "%d %d %d %s %d %d", &m_Drill.x,
                         &m_Offset.x, &m_Offset.y, BufCar, &dx, &dy );
            m_Drill.y    = m_Drill.x;
            m_DrillShape = PAD_CIRCLE;

            if( nn >= 6 )       // Drill shape = OVAL ?
            {
                if( BufCar[0] == 'O' )
                {
                    m_Drill.x    = dx; m_Drill.y = dy;
                    m_DrillShape = PAD_OVAL;
                }
            }

            break;

        case 'A':
            nn = sscanf( PtLine, "%s %s %X", BufLine, BufCar,
                         &m_layerMask );

            /* BufCar is not used now update attributes */
            m_Attribut = PAD_STANDARD;
            if( strncmp( BufLine, "SMD", 3 ) == 0 )
                m_Attribut = PAD_SMD;

            if( strncmp( BufLine, "CONN", 4 ) == 0 )
                m_Attribut = PAD_CONN;

            if( strncmp( BufLine, "HOLE", 4 ) == 0 )
                m_Attribut = PAD_HOLE_NOT_PLATED;

            break;

        case 'N':       /* Read Netname */
            int netcode;
            nn = sscanf( PtLine, "%d", &netcode );
            SetNet( netcode );

            /* read Netname */
            ReadDelimitedText( BufLine, PtLine, sizeof(BufLine) );
            SetNetname( FROM_UTF8( StrPurge( BufLine ) ) );
        break;

        case 'P':
            nn    = sscanf( PtLine, "%d %d", &m_Pos0.x, &m_Pos0.y );
            m_Pos = m_Pos0;
            break;

        case 'L':
            int lengthdie;
            nn    = sscanf( PtLine, "%d", &lengthdie );
            m_LengthDie = lengthdie;
            break;

        case '.':    /* Read specific data */
            if( strnicmp( Line, ".SolderMask ", 12 ) == 0 )
                m_LocalSolderMaskMargin = atoi( Line + 12 );
            else if( strnicmp( Line, ".SolderPaste ", 13 )  == 0 )
                m_LocalSolderPasteMargin = atoi( Line + 13 );
            else if( strnicmp( Line, ".SolderPasteRatio ", 18 ) == 0 )
                m_LocalSolderPasteMarginRatio = atoi( Line + 18 );
            else if( strnicmp( Line, ".LocalClearance ", 16 ) == 0 )
                m_LocalClearance = atoi( Line + 16 );
            break;

        default:
            DisplayError( NULL, wxT( "Err Pad: Id inconnu" ) );
            return 1;
        }
    }

    return 2;   /* error : EOF */
}


/* Read 3D module from file. (Ascii)
 * The 1st line of descr ($MODULE) is assumed to be already read
 * Returns 0 if OK
 */
int MODULE::Read_3D_Descr( LINE_READER* aReader )
{
    char*       Line = aReader->Line();
    char*       text = Line + 3;

    S3D_MASTER* t3D = m_3D_Drawings;

    if( !t3D->m_Shape3DName.IsEmpty() )
    {
        S3D_MASTER* n3D = new S3D_MASTER( this );

        m_3D_Drawings.PushBack( n3D );

        t3D = n3D;
    }

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();

        switch( Line[0] )
        {
        case '$':
            if( Line[1] == 'E' )
                return 0;

            return 1;

        case 'N':       // Shape File Name
        {
            char buf[512];
            ReadDelimitedText( buf, text, 512 );
            t3D->m_Shape3DName = FROM_UTF8( buf );
            break;
        }

        case 'S':       // Scale
            sscanf( text, "%lf %lf %lf\n",
                    &t3D->m_MatScale.x,
                    &t3D->m_MatScale.y,
                    &t3D->m_MatScale.z );
            break;

        case 'O':       // Offset
            sscanf( text, "%lf %lf %lf\n",
                    &t3D->m_MatPosition.x,
                    &t3D->m_MatPosition.y,
                    &t3D->m_MatPosition.z );
            break;

        case 'R':       // Rotation
            sscanf( text, "%lf %lf %lf\n",
                    &t3D->m_MatRotation.x,
                    &t3D->m_MatRotation.y,
                    &t3D->m_MatRotation.z );
            break;

        default:
            break;
        }
    }

    return 1;
}


/* Read a MODULE description
 *  The first description line ($MODULE) is already read
 *  @return 0 if no error
 */
int MODULE::ReadDescr( LINE_READER* aReader )
{
    char* Line;
    char  BufLine[256], BufCar1[128], * PtLine;
    int   itmp1, itmp2;

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();
        if( Line[0] == '$' )
        {
            if( Line[1] == 'E' )
                break;

            if( Line[1] == 'P' )
            {
                D_PAD* pad = new D_PAD( this );
                pad->ReadDescr( aReader );
                RotatePoint( &pad->m_Pos, m_Orient );
                pad->m_Pos.x += m_Pos.x;
                pad->m_Pos.y += m_Pos.y;

                m_Pads.PushBack( pad );
                continue;
            }

            if( Line[1] == 'S' )
                Read_3D_Descr( aReader );
        }

        if( strlen( Line ) < 4 )
            continue;

        PtLine = Line + 3;

        /* Decode the first code of the current line and read the
         * corresponding data
         */
        switch( Line[0] )
        {
        case 'P':
            double orientation;
            memset( BufCar1, 0, sizeof(BufCar1) );
            sscanf( PtLine, "%d %d %lf %d %lX %lX %s",
                    &m_Pos.x, &m_Pos.y,
                    &orientation, &m_Layer,
                    &m_LastEdit_Time, &m_TimeStamp, BufCar1 );

            SetOrientation( orientation );

            m_ModuleStatus = 0;

            if( BufCar1[0] == 'F' )
                SetLocked( true );

            if( BufCar1[1] == 'P' )
                m_ModuleStatus |= MODULE_is_PLACED;

            break;

        case 'L':       /* Li = read the library name of the footprint */
            *BufLine = 0;
            sscanf( PtLine, " %s", BufLine );
            m_LibRef = FROM_UTF8( BufLine );
            break;

        case 'S':
            sscanf( PtLine, " %lX", &m_TimeStamp );
            break;


        case 'O':       /* (Op)tions for auto placement */
            itmp1 = itmp2 = 0;
            sscanf( PtLine, " %X %X", &itmp1, &itmp2 );

            m_CntRot180 = itmp2 & 0x0F;

            if( m_CntRot180 > 10 )
                m_CntRot180 = 10;

            m_CntRot90 = itmp1 & 0x0F;

            if( m_CntRot90 > 10 )
                m_CntRot90 = 0;

            itmp1 = (itmp1 >> 4) & 0x0F;

            if( itmp1 > 10 )
                itmp1 = 0;

            m_CntRot90 |= itmp1 << 4;
            break;

        case 'A':
            if( Line[1] == 't' )
            {
                /* At = (At)tributes of module */
                if( strstr( PtLine, "SMD" ) )
                    m_Attributs |= MOD_CMS;

                if( strstr( PtLine, "VIRTUAL" ) )
                    m_Attributs |= MOD_VIRTUAL;
            }

            if( Line[1] == 'R' )
            {
                // alternate reference, e.g. /478C2408/478AD1B6
                sscanf( PtLine, " %s", BufLine );
                m_Path = FROM_UTF8( BufLine );
            }

            break;

        case 'T':    /* Read a footprint text description (ref, value, or
                      * drawing */
            TEXTE_MODULE * textm;
            sscanf( Line + 1, "%d", &itmp1 );

            if( itmp1 == TEXT_is_REFERENCE )
                textm = m_Reference;
            else if( itmp1 == TEXT_is_VALUE )
                textm = m_Value;
            else        /* text is a drawing */
            {
                textm = new TEXTE_MODULE( this );
                m_Drawings.PushBack( textm );
            }
            textm->ReadDescr( aReader );
            break;

        case 'D':    /* read a drawing item */
            EDGE_MODULE * edge;
            edge = new EDGE_MODULE( this );
            m_Drawings.PushBack( edge );
            edge->ReadDescr( aReader );
            edge->SetDrawCoord();
            break;

        case 'C':    /* read documentation data */
            m_Doc = FROM_UTF8( StrPurge( PtLine ) );
            break;

        case 'K':    /* Read key words */
            m_KeyWord = FROM_UTF8( StrPurge( PtLine ) );
            break;

        case '.':    /* Read specific data */
            if( strnicmp( Line, ".SolderMask ", 12 ) == 0 )
                m_LocalSolderMaskMargin = atoi( Line + 12 );
            else if( strnicmp( Line, ".SolderPaste ", 13 )  == 0 )
                m_LocalSolderPasteMargin = atoi( Line + 13 );
            else if( strnicmp( Line, ".SolderPasteRatio ", 18 ) == 0 )
                m_LocalSolderPasteMarginRatio = atof( Line + 18 );
            else if( strnicmp( Line, ".LocalClearance ", 16 ) == 0 )
                m_LocalClearance = atoi( Line + 16 );

            break;

        default:
            break;
        }
    }

    /* Recalculate the bounding box */
    CalculateBoundingBox();
    return 0;
}


/* Read a description line like:
 *  DS 2600 0 2600 -600 120 21
 *  this description line is in Line
 *  EDGE_MODULE type can be:
 *  - Circle,
 *  - Segment (line)
 *  - Arc
 *  - Polygon
 *
 */
int EDGE_MODULE::ReadDescr( LINE_READER* aReader )
{
    int  ii;
    int  error = 0;
    char* Buf;
    char* Line;

    Line = aReader->Line();

    switch( Line[1] )
    {
    case 'S':
        m_Shape = S_SEGMENT;
        break;

    case 'C':
        m_Shape = S_CIRCLE;
        break;

    case 'A':
        m_Shape = S_ARC;
        break;

    case 'P':
        m_Shape = S_POLYGON;
        break;

    default:
        wxString msg;
        msg.Printf( wxT( "Unknown EDGE_MODULE type <%s>" ), Line );
        DisplayError( NULL, msg );
        error = 1;
        break;
    }

    switch( m_Shape )
    {
    case S_ARC:
        double angle;
        sscanf( Line + 3, "%d %d %d %d %lf %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &angle, &m_Width, &m_Layer );

        NORMALIZE_ANGLE_360( angle );
        SetAngle( angle );
        break;

    case S_SEGMENT:
    case S_CIRCLE:
        sscanf( Line + 3, "%d %d %d %d %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &m_Width, &m_Layer );
        break;

    case S_POLYGON:
        int pointCount;
        sscanf( Line + 3, "%d %d %d %d %d %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &pointCount, &m_Width, &m_Layer );

        m_PolyPoints.clear();
        m_PolyPoints.reserve( pointCount );

        for( ii = 0;  ii<pointCount;  ii++ )
        {
            if( aReader->ReadLine() )
            {
                Buf = aReader->Line();

                if( strncmp( Buf, "Dl", 2 ) != 0 )
                {
                    error = 1;
                    break;
                }

                int x;
                int y;
                sscanf( Buf + 3, "%d %d\n", &x, &y );

                m_PolyPoints.push_back( wxPoint( x, y ) );
            }
            else
            {
                error = 1;
                break;
            }
        }

        break;

    default:
        sscanf( Line + 3, "%d %d %d %d %d %d",
                &m_Start0.x, &m_Start0.y,
                &m_End0.x, &m_End0.y,
                &m_Width, &m_Layer );
        break;
    }

    // Check for a reasonable width:
    if( m_Width <= 1 )
        m_Width = 1;

    if( m_Width > MAX_WIDTH )
        m_Width = MAX_WIDTH;

    // Check for a reasonable layer:
    // m_Layer must be >= FIRST_NON_COPPER_LAYER, but because microwave footprints
    // can use the copper layers m_Layer < FIRST_NON_COPPER_LAYER is allowed.
    // @todo: changes use of EDGE_MODULE these footprints and allows only
    // m_Layer >= FIRST_NON_COPPER_LAYER
    if( (m_Layer < 0) || (m_Layer > LAST_NON_COPPER_LAYER) )
        m_Layer = SILKSCREEN_N_FRONT;

    return error;
}


bool DIMENSION::ReadDimensionDescr( LINE_READER* aReader )
{
    char* Line;
    char  Text[2048];

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();

        if( strnicmp( Line, "$EndDIMENSION", 4 ) == 0 )
            return true;

        if( Line[0] == 'V' )
        {
            sscanf( Line + 2, " %d", &m_Value );
            continue;
        }

        if( Line[0] == 'G' )
        {
            int layer;

            sscanf( Line + 2, " %d %d %lX", &m_Shape, &layer, &m_TimeStamp );

            if( layer < FIRST_NO_COPPER_LAYER )
                layer = FIRST_NO_COPPER_LAYER;

            if( layer > LAST_NO_COPPER_LAYER )
                layer = LAST_NO_COPPER_LAYER;

            SetLayer( layer );
            m_Text.SetLayer( layer );
            continue;
        }

        if( Line[0] == 'T' )
        {
            ReadDelimitedText( Text, Line + 2, sizeof(Text) );
            m_Text.m_Text = FROM_UTF8( Text );
            continue;
        }

        if( Line[0] == 'P' )
        {
            int normal_display = 1;
            int orientation;
            int thickness;
            sscanf( Line + 2, " %d %d %d %d %d %d %d",
                    &m_Text.m_Pos.x, &m_Text.m_Pos.y,
                    &m_Text.m_Size.x, &m_Text.m_Size.y,
                    &thickness, &orientation,
                    &normal_display );

            m_Text.m_Mirror = normal_display ? false : true;
            m_Pos = m_Text.m_Pos;
            m_Text.SetOrientation( orientation );
            m_Text.SetThickness( thickness );
            continue;
        }

        if( Line[0] == 'S' )
        {
            switch( Line[1] )
            {
                int Dummy;

            case 'b':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_crossBarOx, &m_crossBarOy,
                        &m_crossBarFx, &m_crossBarFy,
                        &m_Width );
                break;

            case 'd':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_featureLineDOx, &m_featureLineDOy,
                        &m_featureLineDFx, &m_featureLineDFy,
                        &Dummy );
                break;

            case 'g':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_featureLineGOx, &m_featureLineGOy,
                        &m_featureLineGFx, &m_featureLineGFy,
                        &Dummy );
                break;

            case '1':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_arrowD1Ox, &m_arrowD1Oy,
                        &m_arrowD1Fx, &m_arrowD1Fy,
                        &Dummy );
                break;

            case '2':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_arrowD2Ox, &m_arrowD2Oy,
                        &m_arrowD2Fx, &m_arrowD2Fy,
                        &Dummy );
                break;

            case '3':
                sscanf( Line + 2, " %d %d %d %d %d %d\n",
                        &Dummy,
                        &m_arrowG1Ox, &m_arrowG1Oy,
                        &m_arrowG1Fx, &m_arrowG1Fy,
                        &Dummy );
                break;

            case '4':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_arrowG2Ox, &m_arrowG2Oy,
                        &m_arrowG2Fx, &m_arrowG2Fy,
                        &Dummy );
                break;
            }

            continue;
        }
    }

    return false;
}


bool DRAWSEGMENT::ReadDrawSegmentDescr( LINE_READER* aReader )
{
    char* Line;

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();

        if( strnicmp( Line, "$End", 4 ) == 0 )
            return true; /* End of description */

        if( Line[0] == 'P' )
        {
            sscanf( Line + 2, " %d %d %d %d %d %d",
                    &m_Shape, &m_Start.x, &m_Start.y,
                    &m_End.x, &m_End.y, &m_Width );

            if( m_Width < 0 )
                m_Width = 0;
        }

        if( Line[0] == 'D' )
        {
            int status;
            char* token = 0;

            token = strtok( Line," " );

            for( int i = 0; (token = strtok( NULL," " )) != NULL; i++ )
            {
                switch( i )
                {
                case 0:
                    sscanf( token,"%d",&m_Layer );
                    break;
                case 1:
                    sscanf( token,"%d",&m_Type );
                    break;
                case 2:
                    double angle;
                    sscanf( token, "%lf", &angle );
                    SetAngle( angle );
                    break;
                case 3:
                    sscanf( token,"%lX",&m_TimeStamp );
                    break;
                case 4:
                    sscanf( token,"%X",&status );
                    break;
                    /* Bezier Control Points*/
                case 5:
                    sscanf( token,"%d",&m_BezierC1.x );
                    break;
                case 6:
                    sscanf( token,"%d",&m_BezierC1.y );
                    break;
                case 7:
                    sscanf( token,"%d",&m_BezierC2.x );
                    break;
                case 8:
                    sscanf( token,"%d",&m_BezierC2.y );
                    break;
                default:
                    break;
                }
            }

            if( m_Layer < FIRST_NO_COPPER_LAYER )
                m_Layer = FIRST_NO_COPPER_LAYER;

            if( m_Layer > LAST_NO_COPPER_LAYER )
                m_Layer = LAST_NO_COPPER_LAYER;

            SetState( status, ON );
        }
    }

    return false;
}


/* Read NETINFO_ITEM from file.
 * Returns 0 if OK
 * 1 if incomplete reading
 */
int NETINFO_ITEM::ReadDescr( LINE_READER* aReader )
{
    char* Line;
    char  Ltmp[1024];
    int   tmp;

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();
        if( strnicmp( Line, "$End", 4 ) == 0 )
            return 0;

        if( strncmp( Line, "Na", 2 ) == 0 )
        {
            sscanf( Line + 2, " %d", &tmp );
            SetNet( tmp );

            ReadDelimitedText( Ltmp, Line + 2, sizeof(Ltmp) );
            m_Netname = FROM_UTF8( Ltmp );
            continue;
        }
    }

    return 1;
}


/* Read the description from the PCB file.
 */
bool PCB_TARGET::ReadMirePcbDescr( LINE_READER* aReader )
{
    char* Line;

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();

        if( strnicmp( Line, "$End", 4 ) == 0 )
            return true;

        if( Line[0] == 'P' )
        {
            sscanf( Line + 2, " %X %d %d %d %d %d %lX",
                    &m_Shape, &m_Layer,
                    &m_Pos.x, &m_Pos.y,
                    &m_Size, &m_Width, &m_TimeStamp );

            if( m_Layer < FIRST_NO_COPPER_LAYER )
                m_Layer = FIRST_NO_COPPER_LAYER;

            if( m_Layer > LAST_NO_COPPER_LAYER )
                m_Layer = LAST_NO_COPPER_LAYER;
        }
    }

    return false;
}


int ZONE_CONTAINER::ReadDescr( LINE_READER* aReader )
{
    char* Line, * text;
    char  netname_buffer[1024];
    int   ret;
    int   outline_hatch = CPolyLine::NO_HATCH;
    bool  error = false, has_corner = false;

    netname_buffer[0] = 0;

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();

        if( strnicmp( Line, "ZCorner", 7 ) == 0 ) // new corner found
        {
            int x;
            int y;
            int flag;

            text = Line + 7;
            ret  = sscanf( text, "%d %d %d", &x, &y, &flag );

            if( ret < 3 )
            {
                error = true;
            }
            else
            {
                if( !has_corner )
                    m_Poly->Start( m_Layer, x, y, outline_hatch );
                else
                    AppendCorner( wxPoint( x, y ) );

                has_corner = true;

                if( flag )
                    m_Poly->Close();
            }
        }
        else if( strnicmp( Line, "ZInfo", 5 ) == 0 )   // general info found
        {
            int ts;
            int netcode;

            text = Line + 5;
            ret  = sscanf( text, "%X %d %s", &ts, &netcode, netname_buffer );

            if( ret < 3 )
            {
                error = true;
            }
            else
            {
                SetTimeStamp( ts );
                SetNet( netcode );
                ReadDelimitedText( netname_buffer, netname_buffer, 1024 );
                m_Netname = FROM_UTF8( netname_buffer );
            }
        }
        else if( strnicmp( Line, "ZLayer", 6 ) == 0 )  // layer found
        {
            int x;

            text = Line + 6;
            ret  = sscanf( text, "%d", &x );

            if( ret < 1 )
                error = true;
            else
                m_Layer = x;
        }
        else if( strnicmp( Line, "ZAux", 4 ) == 0 )    // aux info found
        {
            int  x;
            char hopt[10];

            text = Line + 4;
            ret  = sscanf( text, "%d %c", &x, hopt );

            if( ret < 2 )
            {
                error = true;
            }
            else
            {
                switch( hopt[0] )
                {
                case 'n':
                case 'N':
                    outline_hatch = CPolyLine::NO_HATCH;
                    break;

                case 'e':
                case 'E':
                    outline_hatch = CPolyLine::DIAGONAL_EDGE;
                    break;

                case 'f':
                case 'F':
                    outline_hatch = CPolyLine::DIAGONAL_FULL;
                    break;
                }
            }
            /* Set hatch mode later, after reading outlines corners data */
        }
        else if( strnicmp( Line, "ZSmoothing", 10 ) == 0 )
        {
            int tempSmoothingType;
            int tempCornerRadius;
            text = Line + 10;
            ret  = sscanf( text, "%d %d", &tempSmoothingType, &tempCornerRadius );

            if( ret < 2 )
                return false;

            if( tempSmoothingType >= ZONE_SETTING::SMOOTHING_LAST)
                return false;

            if( tempSmoothingType < 0 )
                return false;

            cornerSmoothingType = tempSmoothingType;
            SetCornerRadius( tempCornerRadius );
        }
        else if( strnicmp( Line, "ZOptions", 8 ) == 0 )    // Options info found
        {
            int  fillmode = 1;
            int  arcsegmentcount = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;
            char fillstate = 'F';
            text = Line + 8;
            ret  = sscanf( text, "%d %d %c %d %d", &fillmode, &arcsegmentcount, &fillstate,
                           &m_ThermalReliefGap, &m_ThermalReliefCopperBridge );

            if( ret < 1 )  // Must find 1 or more args.
                return false;
            else
                m_FillMode = fillmode ? 1 : 0;

            if( arcsegmentcount >= ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF )
                m_ArcToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF;

            m_IsFilled = (fillstate == 'S') ? true : false;
        }
        else if( strnicmp( Line, "ZClearance", 10 ) == 0 ) // Clearance and pad options info found
        {
            int  clearance = 200;
            char padoption;
            text = Line + 10;
            ret  = sscanf( text, "%d %1c", &clearance, &padoption );

            if( ret < 2 )
            {
                error = true;
            }
            else
            {
                m_ZoneClearance = clearance;

                switch( padoption )
                {
                case 'i':
                case 'I':
                    m_PadOption = PAD_IN_ZONE;
                    break;

                case 't':
                case 'T':
                    m_PadOption = THERMAL_PAD;
                    break;

                case 'x':
                case 'X':
                    m_PadOption = PAD_NOT_IN_ZONE;
                    break;
                }
            }
        }
        else if( strnicmp( Line, "ZMinThickness", 13 ) == 0 )    // Min Thickness info found
        {
            int thickness;
            text = Line + 13;
            ret  = sscanf( text, "%d", &thickness );

            if( ret < 1 )
                error = true;
            else
                m_ZoneMinThickness = thickness;
        }
        else if( strnicmp( Line, "$POLYSCORNERS", 13 ) == 0  )  // Read the PolysList (polygons used for fill areas in the zone)
        {
            while( aReader->ReadLine() )
            {
                Line = aReader->Line();

                if( strnicmp( Line, "$endPOLYSCORNERS", 4 ) == 0  )
                    break;

                CPolyPt corner;
                int     end_contour, utility;
                utility = 0;
                ret     = sscanf( Line,
                                  "%d %d %d %d",
                                  &corner.x,
                                  &corner.y,
                                  &end_contour,
                                  &utility );
                if( ret < 4 )
                    return false;

                corner.end_contour = end_contour ? true : false;
                corner.utility     = utility;
                m_FilledPolysList.push_back( corner );
            }
        }
        else if( strnicmp( Line, "$FILLSEGMENTS", 13 ) == 0  )
        {
            SEGMENT segm;
            while( aReader->ReadLine() )
            {
                Line = aReader->Line();

                if( strnicmp( Line, "$endFILLSEGMENTS", 4 ) == 0  )
                    break;

                ret = sscanf( Line,
                              "%d %d %d %d",
                              &segm.m_Start.x,
                              &segm.m_Start.y,
                              &segm.m_End.x,
                              &segm.m_End.y );
                if( ret < 4 )
                    return false;

                m_FillSegmList.push_back( segm );
            }
        }
        else if( strnicmp( Line, "$end", 4 ) == 0 )    // end of description
        {
            break;
        }
    }

    if( !IsOnCopperLayer() )
    {
        m_FillMode = 0;
        SetNet( 0 );
    }

    /* Set hatch here, when outlines corners are read */
    m_Poly->SetHatch( outline_hatch );

    return error ? 0 : 1;
}


bool NETCLASS::ReadDescr( LINE_READER* aReader )
{
    bool        result = false;
    char*       line;
    char        buf[1024];
    wxString    netname;

    while( aReader->ReadLine() )
    {
        line = aReader->Line();
        if( strnicmp( line, "AddNet", 6 ) == 0 )
        {
            ReadDelimitedText( buf, line + 6, sizeof(buf) );
            netname = FROM_UTF8( buf );
            Add( netname );
            continue;
        }

        if( strnicmp( line, "$endNCLASS", sizeof( "$endNCLASS" ) - 1 ) == 0 )
        {
            result = true;
            break;
        }

        if( strnicmp( line, "Clearance", 9 ) == 0 )
        {
            SetClearance( atoi( line + 9 ) );
            continue;
        }
        if( strnicmp( line, "TrackWidth", 10 ) == 0 )
        {
            SetTrackWidth( atoi( line + 10 ) );
            continue;
        }
        if( strnicmp( line, "ViaDia", 6 ) == 0 )
        {
            SetViaDiameter( atoi( line + 6 ) );
            continue;
        }
        if( strnicmp( line, "ViaDrill", 8 ) == 0 )
        {
            SetViaDrill( atoi( line + 8 ) );
            continue;
        }

        if( strnicmp( line, "uViaDia", 7 ) == 0 )
        {
            SetuViaDiameter( atoi( line + 7 ) );
            continue;
        }
        if( strnicmp( line, "uViaDrill", 9 ) == 0 )
        {
            SetuViaDrill( atoi( line + 9 ) );
            continue;
        }

        if( strnicmp( line, "Name", 4 ) == 0 )
        {
            ReadDelimitedText( buf, line + 4, sizeof(buf) );
            m_Name = FROM_UTF8( buf );
            continue;
        }
        if( strnicmp( line, "Desc", 4 ) == 0 )
        {
            ReadDelimitedText( buf, line + 4, sizeof(buf) );
            SetDescription( FROM_UTF8( buf ) );
            continue;
        }
    }

    return result;
}



/**
 * Function ReadTextePcbDescr
 * Read a text description from pcb file.
 *
 * For a single line text:
 *
 * $TEXTPCB
 * Te "Text example"
 * Po 66750 53450 600 800 150 0
 * From 24 1 0 Italic
 * $EndTEXTPCB
 *
 * For a multi line text
 *
 * $TEXTPCB
 * Te "Text example"
 * Nl "Line 2"
 * Po 66750 53450 600 800 150 0
 * From 24 1 0 Italic
 * $EndTEXTPCB
 * Nl "line nn" is a line added to the current text
 */
int TEXTE_PCB::ReadTextePcbDescr( LINE_READER* aReader )
{
    char* line;
    char  text[1024];
    char  style[256];

    while( aReader->ReadLine() )
    {
        line = aReader->Line();
        if( strnicmp( line, "$EndTEXTPCB", 11 ) == 0 )
            return 0;
        if( strncmp( line, "Te", 2 ) == 0 ) /* Text line (first line for multi line texts */
        {
            ReadDelimitedText( text, line + 2, sizeof(text) );
            m_Text = FROM_UTF8( text );
            continue;
        }
        if( strncmp( line, "nl", 2 ) == 0 ) /* next line of the current text */
        {
            ReadDelimitedText( text, line + 2, sizeof(text) );
            m_Text.Append( '\n' );
            m_Text += FROM_UTF8( text );
            continue;
        }
        if( strncmp( line, "Po", 2 ) == 0 )
        {
            double angle;
            sscanf( line + 2, " %d %d %d %d %d %lf",
                    &m_Pos.x, &m_Pos.y, &m_Size.x, &m_Size.y,
                    &m_Thickness, &angle );

            SetOrientation( angle );

            // Ensure the text has minimal size to see this text on screen:
            if( m_Size.x < 5 )
                m_Size.x = 5;
            if( m_Size.y < 5 )
                m_Size.y = 5;
            continue;
        }
        if( strncmp( line, "De", 2 ) == 0 )
        {
            style[0] = 0;
            int normal_display = 1;
            char hJustify = 'c';
            sscanf( line + 2, " %d %d %lX %s %c\n", &m_Layer, &normal_display,
                    &m_TimeStamp, style, &hJustify );

            m_Mirror = normal_display ? false : true;

            if( m_Layer < FIRST_COPPER_LAYER )
                m_Layer = FIRST_COPPER_LAYER;
            if( m_Layer > LAST_NO_COPPER_LAYER )
                m_Layer = LAST_NO_COPPER_LAYER;

            if( strnicmp( style, "Italic", 6 ) == 0 )
                m_Italic = 1;
            else
                m_Italic = 0;

            switch( hJustify )
            {
            case 'l':
            case 'L':
                m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
                break;
            case 'c':
            case 'C':
                m_HJustify = GR_TEXT_HJUSTIFY_CENTER;
                break;
            case 'r':
            case 'R':
                m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
                break;
            default:
                m_HJustify = GR_TEXT_HJUSTIFY_CENTER;
                break;
            }
            continue;
        }
    }

     // Set a reasonable width:
    if( m_Thickness < 1 )
        m_Thickness = 1;
    m_Thickness = Clamp_Text_PenSize( m_Thickness, m_Size );

    return 1;
}

/**
 * Function ReadDescr
 * Read description from a given line in "*.brd" format.
 * @param aReader The line reader object which contains the first line of description.
 * @return int - > 0 if success reading else 0.
 */
int TEXTE_MODULE::ReadDescr( LINE_READER* aReader )
{
    int     success = true;
    int     type;
    char    BufCar1[128], BufCar2[128], BufCar3[128];
    char*   line = aReader->Line();
    double  angle;

    int     layer = SILKSCREEN_N_FRONT;

    BufCar1[0] = 0;
    BufCar2[0] = 0;
    BufCar3[0] = 0;

    if( sscanf( line + 1, "%d %d %d %d %d %lf %d %s %s %d %s",
                &type,
                &m_Pos0.x, &m_Pos0.y,
                &m_Size.y, &m_Size.x,
                &angle,    &m_Thickness,
                BufCar1, BufCar2, &layer, BufCar3 ) >= 10 )
    {
        success = true;

        SetOrientation( angle );
    }


    if( (type != TEXT_is_REFERENCE) && (type != TEXT_is_VALUE) )
        type = TEXT_is_DIVERS;

    m_Type = type;

    // Due to the Pcbnew history, .m_Orient is saved in screen value
    // but it is handled as relative to its parent footprint
    m_Orient -= ( (MODULE*) m_Parent )->m_Orient;

    if( BufCar1[0] == 'M' )
        m_Mirror = true;
    else
        m_Mirror = false;

    if( BufCar2[0]  == 'I' )
        m_NoShow = true;
    else
        m_NoShow = false;

    if( BufCar3[0]  == 'I' )
        m_Italic = true;
    else
        m_Italic = false;

    // Test for a reasonable layer:
    if( layer < 0 )
        layer = 0;
    if( layer > LAST_NO_COPPER_LAYER )
        layer = LAST_NO_COPPER_LAYER;
    if( layer == LAYER_N_BACK )
        layer = SILKSCREEN_N_BACK;
    else if( layer == LAYER_N_FRONT )
        layer = SILKSCREEN_N_FRONT;

    SetLayer( layer );

    // Calculate the actual position.
    SetDrawCoord();


    // Search and read the "text" string (a quoted text).
    ReadDelimitedText( &m_Text, line );

    // Test for a reasonable size:
    if( m_Size.x < TEXTS_MIN_SIZE )
        m_Size.x = TEXTS_MIN_SIZE;
    if( m_Size.y < TEXTS_MIN_SIZE )
        m_Size.y = TEXTS_MIN_SIZE;

    // Set a reasonable width:
    if( m_Thickness < 1 )
        m_Thickness = 1;
    m_Thickness = Clamp_Text_PenSize( m_Thickness, m_Size );

    return success;
}

#endif  // USE_NEW_PCBNEW_LOAD
