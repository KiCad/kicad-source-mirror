/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/*
 *  1 - create ascii/csv files for automatic placement of smd components
 *  2 - create a footprint report (pos and footprint descr) (ascii file)
 */

#include <string_utils.h>
#include <macros.h>
#include <locale_io.h>
#include <board_design_settings.h>
#include <build_version.h>
#include <exporters/place_file_exporter.h>
#include <pad.h>

#include <wx/dirdlg.h>

class LIST_MOD      // An helper class used to build a list of useful footprints.
{
public:
    FOOTPRINT*  m_Footprint;      // Link to the actual footprint
    wxString    m_Reference;      // Its schematic reference
    wxString    m_Value;          // Its schematic value
    int         m_Layer;          // its side (B_Cu, or F_Cu)
};


// Defined values to write coordinates using inches or mm:
static const double conv_unit_inch = 0.001 / pcbIUScale.IU_PER_MILS ;      // units = INCHES
static const char unit_text_inch[] = "## Unit = inches, Angle = deg.\n";

static const double conv_unit_mm = 1.0 / pcbIUScale.IU_PER_MM; // units = mm
static const char unit_text_mm[] = "## Unit = mm, Angle = deg.\n";

// Sort function use by GenerefootprintsPosition()
// sort is made by side (layer) top layer first
// then by reference increasing order
static bool sortFPlist( const LIST_MOD& ref, const LIST_MOD& tst )
{
    if( ref.m_Layer == tst.m_Layer )
        return StrNumCmp( ref.m_Reference, tst.m_Reference ) < 0;

    return ref.m_Layer > tst.m_Layer;
}


enum SELECT_SIDE
{
    PCB_NO_SIDE,
    PCB_BACK_SIDE,
    PCB_FRONT_SIDE,
    PCB_BOTH_SIDES
};

PLACE_FILE_EXPORTER::PLACE_FILE_EXPORTER( BOARD* aBoard, bool aUnitsMM, bool aOnlySMD,
                                          bool aExcludeAllTH, bool aTopSide, bool aBottomSide,
                                          bool aFormatCSV, bool aUseAuxOrigin, bool aNegateBottomX )
{
    m_board         = aBoard;
    m_unitsMM       = aUnitsMM;
    m_onlySMD       = aOnlySMD;
    m_excludeAllTH  = aExcludeAllTH;
    m_fpCount       = 0;
    m_negateBottomX = aNegateBottomX;

    if( aTopSide && aBottomSide )
        m_side = PCB_BOTH_SIDES;
    else if( aTopSide )
        m_side = PCB_FRONT_SIDE;
    else if( aBottomSide )
        m_side = PCB_BACK_SIDE;
    else
        m_side = PCB_NO_SIDE;

    m_formatCSV = aFormatCSV;

    if( aUseAuxOrigin )
        m_place_Offset = m_board->GetDesignSettings().GetAuxOrigin();
    else
        m_place_Offset = VECTOR2I( 0, 0 );
}


std::string PLACE_FILE_EXPORTER::GenPositionData()
{
    std::string buffer;
    char line[1024];        // A line to print intermediate data

    // Minimal text lengths:
    m_fpCount = 0;
    int lenRefText = 8;
    int lenValText = 8;
    int lenPkgText = 16;

    // Calculating the number of useful footprints (CMS attribute, not VIRTUAL)
    m_fpCount = 0;

    // Select units:
    double conv_unit = m_unitsMM ? conv_unit_mm : conv_unit_inch;
    const char *unit_text = m_unitsMM ? unit_text_mm : unit_text_inch;

    // Build and sort the list of footprints alphabetically
    std::vector<LIST_MOD> list;

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        if( m_side != PCB_BOTH_SIDES )
        {
            if( footprint->GetLayer() == B_Cu && m_side != PCB_BACK_SIDE )
                continue;
            if( footprint->GetLayer() == F_Cu && m_side != PCB_FRONT_SIDE )
                continue;
        }

        if( footprint->GetAttributes() & FP_EXCLUDE_FROM_POS_FILES )
            continue;

        if( m_onlySMD && !( footprint->GetAttributes() & FP_SMD ) )
            continue;

        if( m_excludeAllTH && footprint->HasThroughHolePads() )
            continue;

        m_fpCount++;

        LIST_MOD item;
        item.m_Footprint = footprint;
        item.m_Reference = footprint->Reference().GetShownText( false );
        item.m_Value     = footprint->Value().GetShownText( false );
        item.m_Layer     = footprint->GetLayer();
        list.push_back( item );

        lenRefText = std::max( lenRefText, (int) item.m_Reference.length() );
        lenValText = std::max( lenValText, (int) item.m_Value.length() );
        lenPkgText = std::max( lenPkgText, (int) item.m_Footprint->GetFPID().GetLibItemName().length() );
    }

    if( list.size() > 1 )
        sort( list.begin(), list.end(), sortFPlist );

    // Switch the locale to standard C (needed to print floating point numbers)
    LOCALE_IO   toggle;

    if( m_formatCSV )
    {
        wxChar csv_sep = ',';

        // Set first line:;
        snprintf( line, sizeof(line), "Ref%cVal%cPackage%cPosX%cPosY%cRot%cSide\n",
                 csv_sep, csv_sep, csv_sep, csv_sep, csv_sep, csv_sep );

        buffer += line;

        for( int ii = 0; ii < m_fpCount; ii++ )
        {
            VECTOR2I footprint_pos;
            footprint_pos  = list[ii].m_Footprint->GetPosition();
            footprint_pos -= m_place_Offset;

            int layer = list[ii].m_Footprint->GetLayer();
            wxASSERT( layer == F_Cu || layer == B_Cu );

            if( layer == B_Cu && m_negateBottomX )
                footprint_pos.x = - footprint_pos.x;

            wxString tmp = wxT( "\"" ) + list[ii].m_Reference;
            tmp << wxT( "\"" ) << csv_sep;
            tmp << wxT( "\"" ) << list[ii].m_Value;
            tmp << wxT( "\"" ) << csv_sep;
            tmp << wxT( "\"" ) << list[ii].m_Footprint->GetFPID().GetLibItemName().wx_str();
            tmp << wxT( "\"" ) << csv_sep;

            tmp << wxString::Format( wxT( "%f%c%f%c%f" ),
                                     footprint_pos.x * conv_unit,
                                     csv_sep,
                                     // Keep the Y axis oriented from bottom to top,
                                     // ( change y coordinate sign )
                                     -footprint_pos.y * conv_unit,
                                     csv_sep,
                                     list[ii].m_Footprint->GetOrientation().AsDegrees() );
            tmp << csv_sep;

            tmp << ( (layer == F_Cu ) ? PLACE_FILE_EXPORTER::GetFrontSideName()
                                      : PLACE_FILE_EXPORTER::GetBackSideName() );
            tmp << '\n';

            buffer += TO_UTF8( tmp );
        }
    }
    else
    {
        // Write file header
        snprintf( line, sizeof(line), "### Footprint positions - created on %s ###\n", TO_UTF8( DateAndTime() ) );

        buffer += line;

        wxString Title = GetBuildVersion();
        snprintf( line, sizeof(line), "### Printed by KiCad version %s\n", TO_UTF8( Title ) );
        buffer += line;

        buffer +=  unit_text;
        buffer += "## Side : ";

        if( m_side == PCB_BACK_SIDE )
            buffer += GetBackSideName().c_str();
        else if( m_side == PCB_FRONT_SIDE )
            buffer += GetFrontSideName().c_str();
        else if( m_side == PCB_BOTH_SIDES )
            buffer += "All";
        else
            buffer += "---";

        buffer += "\n";

        snprintf( line, sizeof(line), "%-*s  %-*s  %-*s  %9.9s  %9.9s  %8.8s  %s\n",
                  int(lenRefText), "# Ref",
                  int(lenValText), "Val",
                  int(lenPkgText), "Package",
                  "PosX", "PosY", "Rot", "Side" );
        buffer += line;

        for( int ii = 0; ii < m_fpCount; ii++ )
        {
            VECTOR2I footprint_pos;
            footprint_pos  = list[ii].m_Footprint->GetPosition();
            footprint_pos -= m_place_Offset;

            int layer = list[ii].m_Footprint->GetLayer();
            wxASSERT( layer == F_Cu || layer == B_Cu );

            if( layer == B_Cu && m_negateBottomX )
                footprint_pos.x = - footprint_pos.x;

            wxString ref = list[ii].m_Reference;
            wxString val = list[ii].m_Value;
            wxString pkg = list[ii].m_Footprint->GetFPID().GetLibItemName();
            ref.Replace( wxT( " " ), wxT( "_" ) );
            val.Replace( wxT( " " ), wxT( "_" ) );
            pkg.Replace( wxT( " " ), wxT( "_" ) );
            snprintf( line, sizeof(line), "%-*s  %-*s  %-*s  %9.4f  %9.4f  %8.4f  %s\n",
                      lenRefText, TO_UTF8( ref ),
                      lenValText, TO_UTF8( val ),
                      lenPkgText, TO_UTF8( pkg ),
                      footprint_pos.x * conv_unit,
                      // Keep the coordinates in the first quadrant,
                      // (i.e. change y sign
                      -footprint_pos.y * conv_unit,
                      list[ii].m_Footprint->GetOrientation().AsDegrees(),
                      (layer == F_Cu ) ? GetFrontSideName().c_str() : GetBackSideName().c_str() );
            buffer += line;
        }

        // Write EOF
        buffer += "## End\n";
    }

    return buffer;
}


std::string PLACE_FILE_EXPORTER::GenReportData()
{
    std::string buffer;

    m_place_Offset = VECTOR2I( 0, 0 );

    // Select units:
    double conv_unit = m_unitsMM ? conv_unit_mm : conv_unit_inch;
    const char *unit_text = m_unitsMM ? unit_text_mm : unit_text_inch;

    LOCALE_IO   toggle;

    // Generate header file comments.)
    char line[1024];
    snprintf( line, sizeof(line), "## Footprint report - date %s\n", TO_UTF8( DateAndTime() ) );
    buffer += line;

    wxString Title = GetBuildVersion();
    snprintf( line, sizeof(line), "## Created by KiCad version %s\n", TO_UTF8( Title ) );
    buffer += line;

    buffer += unit_text;

    buffer += "\n$BeginDESCRIPTION\n";

    BOX2I bbbox = m_board->ComputeBoundingBox();

    buffer += "\n$BOARD\n";

    snprintf( line, sizeof(line), "upper_left_corner %9.6f %9.6f\n",
              bbbox.GetX() * conv_unit, bbbox.GetY() * conv_unit );
    buffer += line;

    snprintf( line, sizeof(line), "lower_right_corner %9.6f %9.6f\n",
              bbbox.GetRight()  * conv_unit, bbbox.GetBottom() * conv_unit );
    buffer += line;

    buffer += "$EndBOARD\n\n";

    std::vector<FOOTPRINT*> sortedFootprints;

    for( FOOTPRINT* footprint : m_board->Footprints() )
        sortedFootprints.push_back( footprint );

    std::sort( sortedFootprints.begin(), sortedFootprints.end(),
               []( FOOTPRINT* a, FOOTPRINT* b ) -> bool
               {
                   return StrNumCmp( a->GetReference(), b->GetReference(), true ) < 0;
               });

    for( FOOTPRINT* footprint : sortedFootprints )
    {
        wxString ref = footprint->Reference().GetShownText( false );
        wxString value = footprint->Value().GetShownText( false );

        snprintf( line, sizeof(line), "$MODULE %s\n", TO_UTF8( ref ) );
        buffer += line;

        snprintf( line, sizeof(line), "reference %s\n", TO_UTF8( ref ) );
        snprintf( line, sizeof(line), "value %s\n",  TO_UTF8( value ) );
        snprintf( line, sizeof(line), "footprint %s\n", footprint->GetFPID().Format().c_str() );
        buffer += line;

        buffer += "attribut";

        if(( footprint->GetAttributes() & ( FP_THROUGH_HOLE | FP_SMD ) ) == 0 )
            buffer += " virtual";

        if( footprint->GetAttributes() & FP_SMD )
            buffer += " smd";

        if( footprint->GetAttributes() & FP_THROUGH_HOLE )
            buffer += " none";

        buffer += "\n";

        VECTOR2I footprint_pos = footprint->GetPosition();
        footprint_pos -= m_place_Offset;

        snprintf( line, sizeof(line), "position %9.6f %9.6f  orientation %.2f\n",
                  footprint_pos.x * conv_unit,
                  footprint_pos.y * conv_unit,
                  footprint->GetOrientation().AsDegrees() );
        buffer += line;

        if( footprint->GetLayer() == F_Cu )
            buffer += "layer front\n";
        else if( footprint->GetLayer() == B_Cu )
            buffer += "layer back\n";
        else
            buffer += "layer other\n";

        std::vector<PAD*> sortedPads;

        for( PAD* pad : footprint->Pads() )
            sortedPads.push_back( pad );

        std::sort( sortedPads.begin(), sortedPads.end(),
                   []( PAD* a, PAD* b ) -> bool
                   {
                       return StrNumCmp( a->GetNumber(), b->GetNumber(), true ) < 0;
                   });

        for( PAD* pad : sortedPads )
        {
            snprintf( line, sizeof(line), "$PAD \"%s\"\n", TO_UTF8( pad->GetNumber() ) );
            buffer += line;

            int layer = 0;

            if( pad->GetLayerSet()[B_Cu] )
                layer = 1;

            if( pad->GetLayerSet()[F_Cu] )
                layer |= 2;

            static const char* layer_name[4] = { "nocopper", "back", "front", "both" };
            snprintf( line, sizeof(line), "Shape %s Layer %s\n",
                      TO_UTF8( pad->ShowPadShape() ),
                      layer_name[layer] );
            buffer += line;

            VECTOR2I padPos = pad->GetFPRelativePosition();

            snprintf( line, sizeof(line), "position %9.6f %9.6f  size %9.6f %9.6f  orientation %.2f\n",
                      padPos.x * conv_unit,
                      padPos.y * conv_unit,
                      pad->GetSize().x * conv_unit,
                      pad->GetSize().y * conv_unit,
                      ( pad->GetOrientation() - footprint->GetOrientation() ).AsDegrees() );
            buffer += line;

            snprintf( line, sizeof(line), "drill %9.6f\n", pad->GetDrillSize().x * conv_unit );
            buffer += line;

            snprintf( line, sizeof(line), "shape_offset %9.6f %9.6f\n",
                      pad->GetOffset().x * conv_unit,
                      pad->GetOffset().y * conv_unit );
            buffer += line;

            buffer += "$EndPAD\n";
        }

        snprintf( line, sizeof(line), "$EndMODULE  %s\n\n", TO_UTF8( ref ) );
        buffer += line;
    }

    // Generate EOF.
    buffer += "$EndDESCRIPTION\n";

    return buffer;
}
