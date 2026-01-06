/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fmt/format.h>

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
static const double conv_unit_inch = 0.001 / pcbIUScale.IU_PER_MILS ;      // units = in
static const char unit_text_inch[] = "## Unit = inches, Angle = deg.\n";

static const double conv_unit_mm = 1.0 / pcbIUScale.IU_PER_MM;             // units = mm
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
                                          bool aExcludeAllTH, bool aExcludeDNP, bool aExcludeBOM,
                                          bool aTopSide, bool aBottomSide, bool aFormatCSV,
                                          bool aUseAuxOrigin, bool aNegateBottomX )
{
    m_board         = aBoard;
    m_unitsMM       = aUnitsMM;
    m_onlySMD       = aOnlySMD;
    m_excludeAllTH  = aExcludeAllTH;
    m_excludeDNP    = aExcludeDNP;
    m_excludeBOM    = aExcludeBOM;
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
    wxString wxLine;        // wxString used for UTF-8 line

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

        if( footprint->GetExcludedFromPosFilesForVariant( m_variant ) )
            continue;

        if( m_onlySMD && !( footprint->GetAttributes() & FP_SMD ) )
            continue;

        if( m_excludeAllTH && footprint->HasThroughHolePads() )
            continue;

        if( m_excludeDNP && footprint->GetDNPForVariant( m_variant ) )
            continue;

        if( m_excludeBOM && footprint->GetExcludedFromBOMForVariant( m_variant ) )
            continue;

        m_fpCount++;

        LIST_MOD item;
        item.m_Footprint = footprint;
        item.m_Reference = footprint->Reference().GetShownText( false );
        item.m_Value     = footprint->Value().GetShownText( false );
        item.m_Layer     = footprint->GetLayer();

        lenRefText = std::max( lenRefText, (int) item.m_Reference.length() );
        lenValText = std::max( lenValText, (int) item.m_Value.length() );
        lenPkgText = std::max( lenPkgText, (int) item.m_Footprint->GetFPID().GetLibItemName().length() );

        list.push_back( std::move( item ) );
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
            wxASSERT( IsExternalCopperLayer( layer ) );

            if( layer == B_Cu && m_negateBottomX )
                footprint_pos.x = - footprint_pos.x;

            wxLine = wxT( "\"" ) + list[ii].m_Reference;
            wxLine << wxT( "\"" ) << csv_sep;
            wxLine << wxT( "\"" ) << list[ii].m_Value;
            wxLine << wxT( "\"" ) << csv_sep;
            wxLine << wxT( "\"" ) << list[ii].m_Footprint->GetFPID().GetLibItemName().wx_str();
            wxLine << wxT( "\"" ) << csv_sep;

            wxLine << wxString::Format( wxT( "%f%c%f%c%f" ),
                                        footprint_pos.x * conv_unit,
                                        csv_sep,
                                        // Keep the Y axis oriented from bottom to top,
                                        // ( change y coordinate sign )
                                        -footprint_pos.y * conv_unit,
                                        csv_sep,
                                        list[ii].m_Footprint->GetOrientation().AsDegrees() );
            wxLine << csv_sep;

            wxLine << ( (layer == F_Cu ) ? PLACE_FILE_EXPORTER::GetFrontSideName()
                                         : PLACE_FILE_EXPORTER::GetBackSideName() );
            wxLine << '\n';

            buffer += TO_UTF8( wxLine );
        }
    }
    else
    {
        // Write file header
        snprintf( line, sizeof(line), "### Footprint positions - created on %s ###\n",
                  TO_UTF8( GetISO8601CurrentDateTime() ) );

        buffer += line;

        wxString Title = GetBuildVersion();
        snprintf( line, sizeof(line), "### Printed by KiCad version %s\n", TO_UTF8( Title ) );
        buffer += line;

        buffer +=  unit_text;
        buffer += "## Side : ";

        if( m_side == PCB_BACK_SIDE )
            buffer += GetBackSideName();
        else if( m_side == PCB_FRONT_SIDE )
            buffer += GetFrontSideName();
        else if( m_side == PCB_BOTH_SIDES )
            buffer += "All";
        else
            buffer += "---";

        buffer += "\n";

        snprintf( line, sizeof(line), "%-*s  %-*s  %-*s  %9.9s  %9.9s  %8.8s  %s\n",
                  lenRefText, "# Ref",
                  lenValText, "Val",
                  lenPkgText, "Package",
                  "PosX", "PosY", "Rot", "Side" );
        buffer += line;

        for( int ii = 0; ii < m_fpCount; ii++ )
        {
            VECTOR2I footprint_pos;
            footprint_pos  = list[ii].m_Footprint->GetPosition();
            footprint_pos -= m_place_Offset;

            int layer = list[ii].m_Footprint->GetLayer();
            wxASSERT( IsExternalCopperLayer( layer ) );

            if( layer == B_Cu && m_negateBottomX )
                footprint_pos.x = - footprint_pos.x;

            wxString ref = list[ii].m_Reference;
            wxString val = list[ii].m_Value;
            wxString pkg = list[ii].m_Footprint->GetFPID().GetLibItemName();
            ref.Replace( wxT( " " ), wxT( "_" ) );
            val.Replace( wxT( " " ), wxT( "_" ) );
            pkg.Replace( wxT( " " ), wxT( "_" ) );
            wxLine.Printf( wxT( "%-*s  %-*s  %-*s  %9.4f  %9.4f  %8.4f  %s\n" ),
                           lenRefText, std::move( ref ),
                           lenValText, std::move( val ),
                           lenPkgText, std::move( pkg ),
                           footprint_pos.x * conv_unit,
                           // Keep the coordinates in the first quadrant, (i.e. change y sign)
                           -footprint_pos.y * conv_unit,
                           list[ii].m_Footprint->GetOrientation().AsDegrees(),
                           ( layer == F_Cu ) ? GetFrontSideName() : GetBackSideName() );
            buffer += TO_UTF8( wxLine );
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

    buffer += fmt::format( "## Footprint report - date {}\n", TO_UTF8( GetISO8601CurrentDateTime() ) );

    wxString Title = GetBuildVersion();
    buffer += fmt::format( "## Printed by KiCad version {}\n", TO_UTF8( Title ) );

    buffer += unit_text;

    buffer += "\n$BeginDESCRIPTION\n";

    BOX2I bbbox = m_board->ComputeBoundingBox( false );

    buffer += "\n$BOARD\n";

    buffer += fmt::format( "upper_left_corner {:9.6f} {:9.6f}\n",
              bbbox.GetX() * conv_unit,
              bbbox.GetY() * conv_unit );

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

        buffer += fmt::format( "$MODULE {}\n", TO_UTF8( ref ) );

        buffer += fmt::format( "reference {}\n", TO_UTF8( ref ) );
        buffer += fmt::format( "value {}\n", TO_UTF8( value ) );
        buffer += fmt::format( "footprint {}\n", footprint->GetFPID().Format().c_str() );

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

        buffer += fmt::format( "position {:9.6f} {:9.6f}  orientation {:.2f}\n",
                  footprint_pos.x * conv_unit,
                  footprint_pos.y * conv_unit,
                  footprint->GetOrientation().AsDegrees() );

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
            buffer += fmt::format( "$PAD \"{}\"\n", TO_UTF8( pad->GetNumber() ) );

            int layer = 0;

            if( pad->GetLayerSet()[B_Cu] )
                layer = 1;

            if( pad->GetLayerSet()[F_Cu] )
                layer |= 2;

            // TODO(JE) padstacks
            static const char* layer_name[4] = { "nocopper", "back", "front", "both" };
            buffer += fmt::format( "Shape {} Layer {}\n",
                      TO_UTF8( pad->ShowLegacyPadShape( PADSTACK::ALL_LAYERS ) ),
                      layer_name[layer] );

            VECTOR2I padPos = pad->GetFPRelativePosition();

            buffer += fmt::format( "position {:9.6f} {:9.6f} size {:9.6f} {:9.6f} orientation {:.2f}\n",
                      padPos.x * conv_unit,
                      padPos.y * conv_unit,
                      pad->GetSize( PADSTACK::ALL_LAYERS ).x * conv_unit,
                      pad->GetSize( PADSTACK::ALL_LAYERS ).y * conv_unit,
                      pad->GetOrientation().AsDegrees() );

            buffer += fmt::format( "drill {:9.6f}\n", pad->GetDrillSize().x * conv_unit );

            buffer += fmt::format( "shape_offset {:9.6f} {:9.6f}\n",
                      pad->GetOffset( PADSTACK::ALL_LAYERS ).x * conv_unit,
                      pad->GetOffset( PADSTACK::ALL_LAYERS ).y * conv_unit );

            buffer += "$EndPAD\n";
        }

        buffer += fmt::format( "$EndMODULE {}\n\n", TO_UTF8( ref ) );
    }

    // Generate EOF.
    buffer += "$EndDESCRIPTION\n";

    return buffer;
}


wxString PLACE_FILE_EXPORTER::DecorateFilename( const wxString& aBaseName, bool aFront, bool aBack )
{
    if( aFront && aBack )
        return aBaseName + wxT( "-" ) + wxT( "all" );
    else if( aFront )
        return aBaseName + wxT( "-" ) + GetFrontSideName();
    else if( aBack )
        return aBaseName + wxT( "-" ) + GetBackSideName();
    else
        return aBaseName;
}
