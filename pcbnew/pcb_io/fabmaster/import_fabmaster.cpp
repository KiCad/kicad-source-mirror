/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 BeagleBoard Foundation
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Seth Hillbrand <hillbrand@kipro-pcb.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include "import_fabmaster.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <utility>

#include <wx/log.h>

#include <board.h>
#include <board_design_settings.h>
#include <board_item.h>
#include <footprint.h>
#include <pad.h>
#include <padstack.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <zone.h>
#include <common.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_utils.h>
#include <string_utils.h>
#include <progress_reporter.h>
#include <math/util.h>

#include <wx/filename.h>


/**
 * Flag to enable #FABMASTER plugin debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceFabmaster[] = wxT( "KICAD_FABMASTER" );


void FABMASTER::checkpoint()
{
    const unsigned PROGRESS_DELTA = 250;

    if( m_progressReporter )
    {
        if( ++m_doneCount > m_lastProgressCount + PROGRESS_DELTA )
        {
            m_progressReporter->SetCurrentProgress( ( (double) m_doneCount )
                                                            / std::max( 1U, m_totalCount ) );

            if( !m_progressReporter->KeepRefreshing() )
                THROW_IO_ERROR( _( "File import canceled by user." ) );

            m_lastProgressCount = m_doneCount;
        }
    }
}


double FABMASTER::readDouble( const std::string& aStr ) const
{
    // This is bad, but at least don't return uninitialized data
    wxCHECK_MSG( !aStr.empty(), 0.0, "Empty string passed to readDouble" );

    std::istringstream istr( aStr );
    istr.imbue( std::locale::classic() );

    double doubleValue;
    istr >> doubleValue;
    return doubleValue;
}


int FABMASTER::readInt( const std::string& aStr ) const
{
    // This is bad, but at least don't return uninitialized data
    wxCHECK_MSG( !aStr.empty(), 0, "Empty string passed to readInt" );

    std::istringstream istr( aStr );
    istr.imbue( std::locale::classic() );

    int intValue;
    istr >> intValue;
    return intValue;
}


bool FABMASTER::Read( const std::string& aFile )
{
    std::ifstream ifs( aFile, std::ios::in | std::ios::binary );

    if( !ifs.is_open() )
        return false;

    m_filename = aFile;

    // Read/ignore all bytes in the file to find the size and then go back to the beginning
    ifs.ignore( std::numeric_limits<std::streamsize>::max() );
    std::streamsize length = ifs.gcount();
    ifs.clear();
    ifs.seekg( 0, std::ios_base::beg );

    std::string buffer( std::istreambuf_iterator<char>{ ifs }, {} );

    std::vector < std::string > row;

    // Reserve an estimate of the number of rows to prevent continual re-allocation
    // crashing (Looking at you MSVC)
    row.reserve( length / 100 );
    std::string cell;
    cell.reserve( 100 );

    bool quoted = false;

    for( auto& ch : buffer )
    {
        switch( ch )
        {
        case  '"':

            if( cell.empty() || cell[0] == '"' )
                quoted = !quoted;

            cell += ch;
            break;

        case '!':
            if( !quoted )
            {
                row.push_back( cell );
                cell.clear();
            }
            else
                cell += ch;

            break;

        case '\n':

            /// Rows end with "!" and we don't want to keep the empty cell
            if( !cell.empty() )
                row.push_back( cell );

            cell.clear();
            rows.push_back( row );
            row.clear();
            quoted = false;
            break;

        case '\r':
            break;

        default:
            cell += std::toupper( ch );
        }
    }

    // Handle last line without linebreak
    if( !cell.empty() || !row.empty() )
    {
        row.push_back( cell );
        cell.clear();
        rows.push_back( row );
        row.clear();
    }

    return true;
}


FABMASTER::section_type FABMASTER::detectType( size_t aOffset )
{
    single_row row;

    try
    {
        row = rows.at( aOffset );
    }
    catch( std::out_of_range& )
    {
        return UNKNOWN_EXTRACT;
    }

    if( row.size() < 3 )
        return UNKNOWN_EXTRACT;

    if( row[0].back() != 'A' )
        return UNKNOWN_EXTRACT;

    std::string row1 = row[1];
    std::string row2 = row[2];
    std::string row3{};

    /// We strip the underscores from all column names as some export variants use them and
    // some do not
    std::erase_if( row1, []( char c ){ return c == '_'; } );
    std::erase_if( row2, []( char c ){ return c == '_'; } );

    if( row.size() > 3 )
    {
        row3 = row[3];
        std::erase_if( row3, []( char c ){ return c == '_'; } );
    }

    if( row1 == "REFDES" && row2 == "COMPCLASS" )
        return EXTRACT_REFDES;

    if( row1 == "NETNAME" && row2 == "REFDES" )
        return EXTRACT_NETS;

    if( row1 == "CLASS" && row2 == "SUBCLASS" && row3.empty() )
        return EXTRACT_BASIC_LAYERS;

    if( row1 == "GRAPHICDATANAME" && row2 == "GRAPHICDATANUMBER" )
        return EXTRACT_GRAPHICS;

    if( row1 == "CLASS" && row2 == "SUBCLASS" && row3 == "GRAPHICDATANAME" )
        return EXTRACT_TRACES;

    if( row1 == "SYMNAME" && row2 == "PINNAME" )
        return FABMASTER_EXTRACT_PINS;

    if( row1 == "SYMNAME" && row2 == "SYMMIRROR" && row3 == "PINNAME" )
        return EXTRACT_PINS;

    if( row1 == "VIAX" && row2 == "VIAY" )
        return EXTRACT_VIAS;

    if( row1 == "SUBCLASS" && row2 == "PADSHAPENAME" )
        return EXTRACT_PAD_SHAPES;

    if( row1 == "PADNAME" )
        return EXTRACT_PADSTACKS;

    if( row1 == "LAYERSORT" )
        return EXTRACT_FULL_LAYERS;

    wxLogError( _( "Unknown FABMASTER section %s:%s at row %zu." ),
                row1.c_str(),
                row2.c_str(),
                aOffset );
    return UNKNOWN_EXTRACT;

}


double FABMASTER::processScaleFactor( size_t aRow )
{
    double retval = 0.0;

    if( aRow >= rows.size() )
        return -1.0;

    if( rows[aRow].size() < 11 )
    {
        wxLogError( _( "Invalid row size in J row %zu. Expecting 11 elements but found %zu." ),
                    aRow,
                    rows[aRow].size() );
        return -1.0;
    }

    for( int i = 7; i < 10 && retval < 1.0; ++i )
    {
        std::string units = rows[aRow][i];
        std::transform(units.begin(), units.end(),units.begin(), ::toupper);

        if( units == "MILS" )
            retval = pcbIUScale.IU_PER_MILS;
        else if( units == "MILLIMETERS" )
            retval = pcbIUScale.IU_PER_MM;
        else if( units == "MICRONS" )
            retval = pcbIUScale.IU_PER_MM * 10.0;
        else if( units == "INCHES" )
            retval = pcbIUScale.IU_PER_MILS * 1000.0;
    }

    if( retval < 1.0 )
    {
        wxLogError( _( "Could not find units value, defaulting to mils." ) );
        retval = pcbIUScale.IU_PER_MILS;
    }

    return retval;
}


int FABMASTER::getColFromName( size_t aRow, const std::string& aStr )
{
    if( aRow >= rows.size() )
        return -1;

    std::vector<std::string> header = rows[aRow];

    for( size_t i = 0; i < header.size(); i++ )
    {
        /// Some Fabmaster headers include the underscores while others do not
        /// so we strip them uniformly before comparing
        std::erase_if( header[i], []( const char c ) { return c == '_'; } );

        if( header[i] == aStr )
            return i;
    }

    THROW_IO_ERROR( wxString::Format( _( "Could not find column label %s." ), aStr.c_str() ) );
    return -1;
}


PCB_LAYER_ID FABMASTER::getLayer( const std::string& aLayerName )
{
    const auto& kicad_layer = layers.find( aLayerName);

    if( kicad_layer == layers.end() )
        return UNDEFINED_LAYER;
    else
        return static_cast<PCB_LAYER_ID>( kicad_layer->second.layerid );
}


size_t FABMASTER::processPadStackLayers( size_t aRow )
{
    size_t rownum = aRow + 2;

    if( rownum >= rows.size() )
        return -1;

    const single_row& header = rows[aRow];

    int pad_name_col        = getColFromName( aRow, "PADNAME" );
    int pad_num_col         = getColFromName( aRow, "RECNUMBER" );
    int pad_lay_col         = getColFromName( aRow, "LAYER" );
    int pad_fix_col         = getColFromName( aRow, "FIXFLAG" );
    int pad_via_col         = getColFromName( aRow, "VIAFLAG" );
    int pad_shape_col       = getColFromName( aRow, "PADSHAPE1" );
    int pad_width_col       = getColFromName( aRow, "PADWIDTH" );
    int pad_height_col      = getColFromName( aRow, "PADHGHT" );
    int pad_xoff_col        = getColFromName( aRow, "PADXOFF" );
    int pad_yoff_col        = getColFromName( aRow, "PADYOFF" );
    int pad_flash_col       = getColFromName( aRow, "PADFLASH" );
    int pad_shape_name_col  = getColFromName( aRow, "PADSHAPENAME" );

    for( ; rownum < rows.size() && rows[rownum].size() > 0 && rows[rownum][0] == "S"; ++rownum )
    {
        const single_row& row = rows[rownum];

        if( row.size() != header.size() )
        {
            wxLogError( _( "Invalid row size in row %zu. Expecting %zu elements but found %zu." ),
                        rownum,
                        header.size(),
                        row.size() );
            continue;
        }

        auto& pad_name = row[pad_name_col];
        auto& pad_num = row[pad_num_col];
        auto& pad_layer = row[pad_lay_col];
        auto& pad_is_fixed = row[pad_fix_col];
        auto& pad_is_via = row[pad_via_col];
        auto& pad_shape = row[pad_shape_col];
        auto& pad_width = row[pad_width_col];
        auto& pad_height = row[pad_height_col];
        auto& pad_xoff = row[pad_xoff_col];
        auto& pad_yoff = row[pad_yoff_col];
        auto& pad_flash = row[pad_flash_col];
        auto& pad_shapename = row[pad_shape_name_col];

        // This layer setting seems to be unused
        if( pad_layer == "INTERNAL_PAD_DEF" || pad_layer == "internal_pad_def" )
            continue;

        // Skip the technical layers
        if( pad_layer[0] == '~' )
            break;

        auto result = layers.emplace( pad_layer, FABMASTER_LAYER{} );
        FABMASTER_LAYER& layer = result.first->second;

        /// If the layer ids have not yet been assigned
        if( layer.id == 0 )
        {
            layer.name = pad_layer;
            layer.id = readInt( pad_num );
            layer.conductive = true;
        }
    }

    return 0;
}


/**
 * A!PADNAME!RECNUMBER!LAYER!FIXFLAG!VIAFLAG!PADSHAPE1!PADWIDTH!PADHGHT!
 *      PADXOFF!PADYOFF!PADFLASH!PADSHAPENAME!TRELSHAPE1!TRELWIDTH!TRELHGHT!
 *      TRELXOFF!TRELYOFF!TRELFLASH!TRELSHAPENAME!APADSHAPE1!APADWIDTH!APADHGHT!
 *      APADXOFF!APADYOFF!APADFLASH!APADSHAPENAME!
 */
size_t FABMASTER::processPadStacks( size_t aRow )
{
    size_t rownum = aRow + 2;

    if( rownum >= rows.size() )
        return -1;

    const single_row& header = rows[aRow];
    double scale_factor = processScaleFactor( aRow + 1 );

    if( scale_factor <= 0.0 )
        return -1;

    int pad_name_col        = getColFromName( aRow, "PADNAME" );
    int pad_num_col         = getColFromName( aRow, "RECNUMBER" );
    int pad_lay_col         = getColFromName( aRow, "LAYER" );
    int pad_fix_col         = getColFromName( aRow, "FIXFLAG" );
    int pad_via_col         = getColFromName( aRow, "VIAFLAG" );
    int pad_shape_col       = getColFromName( aRow, "PADSHAPE1" );
    int pad_width_col       = getColFromName( aRow, "PADWIDTH" );
    int pad_height_col      = getColFromName( aRow, "PADHGHT" );
    int pad_xoff_col        = getColFromName( aRow, "PADXOFF" );
    int pad_yoff_col        = getColFromName( aRow, "PADYOFF" );
    int pad_flash_col       = getColFromName( aRow, "PADFLASH" );
    int pad_shape_name_col  = getColFromName( aRow, "PADSHAPENAME" );

    for( ; rownum < rows.size() && rows[rownum].size() > 0 && rows[rownum][0] == "S"; ++rownum )
    {
        const single_row& row = rows[rownum];
        FM_PAD* pad;

        if( row.size() != header.size() )
        {
            wxLogError( _( "Invalid row size in row %zu. Expecting %zu elements but found %zu." ),
                        rownum,
                        header.size(),
                        row.size() );
            continue;
        }

        auto& pad_name = row[pad_name_col];
        auto& pad_num = row[pad_num_col];
        auto& pad_layer = row[pad_lay_col];
        auto& pad_is_fixed = row[pad_fix_col];
        auto& pad_is_via = row[pad_via_col];
        auto& pad_shape = row[pad_shape_col];
        auto& pad_width = row[pad_width_col];
        auto& pad_height = row[pad_height_col];
        auto& pad_xoff = row[pad_xoff_col];
        auto& pad_yoff = row[pad_yoff_col];
        auto& pad_flash = row[pad_flash_col];
        auto& pad_shapename = row[pad_shape_name_col];

        // This layer setting seems to be unused
        if( pad_layer == "INTERNAL_PAD_DEF" || pad_layer == "internal_pad_def" )
            continue;

        int recnum = KiROUND( readDouble( pad_num ) );

        auto new_pad = pads.find( pad_name );

        if( new_pad != pads.end() )
            pad = &new_pad->second;
        else
        {
            pads[pad_name] = FM_PAD();
            pad = &pads[pad_name];
            pad->name = pad_name;
        }

        /// Handle the drill layer
        if( pad_layer == "~DRILL" )
        {
            int drill_hit;
            int drill_x;
            int drill_y;

            try
            {
                drill_hit = KiROUND( std::fabs( readDouble( pad_shape ) * scale_factor ) );
                drill_x = KiROUND( std::fabs( readDouble( pad_width ) * scale_factor ) );
                drill_y = KiROUND( std::fabs( readDouble( pad_height ) * scale_factor ) );
            }
            catch( ... )
            {
                wxLogError( _( "Expecting drill size value but found %s!%s!%s in row %zu." ),
                            pad_shape.c_str(),
                            pad_width.c_str(),
                            pad_height.c_str(),
                            rownum );
                continue;
            }

            if( drill_hit == 0 )
            {
                pad->drill = false;
                continue;
            }

            pad->drill = true;

            // This is to account for broken fabmaster outputs where circle drill hits don't
            // actually get the drill hit value.
            if( drill_x == drill_y )
            {
                pad->drill_size_x = drill_hit;
                pad->drill_size_y = drill_hit;
            }
            else
            {
                pad->drill_size_x = drill_x;
                pad->drill_size_y = drill_y;
            }

            if( !pad_shapename.empty() && pad_shapename[0] == 'P' )
                pad->plated = true;

            continue;
        }

        if( pad_shape.empty() )
            continue;

        double w;
        double h;

        try
        {
            w = readDouble( pad_width ) * scale_factor;
            h = readDouble( pad_height ) * scale_factor;
        }
        catch( ... )
        {
            wxLogError( _( "Expecting pad size values but found %s : %s in row %zu." ),
                        pad_width.c_str(),
                        pad_height.c_str(),
                        rownum );
            continue;
        }

        if( w <= 0.0 )
            continue;

        auto layer = layers.find( pad_layer );

        if( layer != layers.end() )
        {
            if( layer->second.layerid == F_Cu )
                pad->top = true;
            else if( layer->second.layerid == B_Cu )
                pad->bottom = true;
        }

        if( w > std::numeric_limits<int>::max() || h > std::numeric_limits<int>::max() )
        {
            wxLogError( _( "Invalid pad size in row %zu." ), rownum );
            continue;
        }

        if( pad_layer == "~TSM" || pad_layer == "~BSM" )
        {
            if( w > 0.0 && h > 0.0 )
            {
                pad->mask_width = KiROUND( w );
                pad->mask_height = KiROUND( h );
            }
            continue;
        }

        if( pad_layer == "~TSP" || pad_layer == "~BSP" )
        {
            if( w > 0.0 && h > 0.0 )
            {
                pad->paste_width = KiROUND( w );
                pad->paste_height = KiROUND( h );
            }
            continue;
        }

        /// All remaining technical layers are not handled
        if( pad_layer[0] == '~' )
            continue;

        try
        {
            pad->x_offset = KiROUND( readDouble( pad_xoff ) * scale_factor );
            pad->y_offset = -KiROUND( readDouble( pad_yoff ) * scale_factor );
        }
        catch( ... )
        {
            wxLogError( _( "Expecting pad offset values but found %s:%s in row %zu." ),
                        pad_xoff.c_str(),
                        pad_yoff.c_str(),
                        rownum );
            continue;
        }

        if( w > 0.0 && h > 0.0 && recnum == 1 )
        {
            pad->width = KiROUND( w );
            pad->height = KiROUND( h );
            pad->via = ( std::toupper( pad_is_via[0] ) != 'V' );

            if( pad_shape == "CIRCLE" )
            {
                pad->height = pad->width;
                pad->shape = PAD_SHAPE::CIRCLE;
            }
            else if( pad_shape == "RECTANGLE" )
            {
                pad->shape = PAD_SHAPE::RECTANGLE;
            }
            else if( pad_shape == "ROUNDED_RECT" )
            {
                pad->shape = PAD_SHAPE::ROUNDRECT;
            }
            else if( pad_shape == "SQUARE" )
            {
                pad->shape = PAD_SHAPE::RECTANGLE;
                pad->height = pad->width;
            }
            else if( pad_shape == "OBLONG" || pad_shape == "OBLONG_X" || pad_shape == "OBLONG_Y" )
                pad->shape = PAD_SHAPE::OVAL;
            else if( pad_shape == "OCTAGON" )
            {
                pad->shape = PAD_SHAPE::RECTANGLE;
                pad->is_octogon = true;
            }
            else if( pad_shape == "SHAPE" )
            {
                pad->shape = PAD_SHAPE::CUSTOM;
                pad->custom_name = pad_shapename;
            }
            else
            {
                wxLogError( _( "Unknown pad shape name '%s' on layer '%s' in row %zu." ),
                            pad_shape.c_str(),
                            pad_layer.c_str(),
                            rownum );
                continue;
            }
        }
    }

    return rownum - aRow;
}


size_t FABMASTER::processSimpleLayers( size_t aRow )
{
    size_t rownum = aRow + 2;

     if( rownum >= rows.size() )
         return -1;

     auto& header = rows[aRow];
     double scale_factor = processScaleFactor( aRow + 1 );

     if( scale_factor <= 0.0 )
         return -1;

     int layer_class_col     = getColFromName( aRow, "CLASS" );
     int layer_subclass_col  = getColFromName( aRow, "SUBCLASS" );

     if( layer_class_col < 0 || layer_subclass_col < 0 )
         return -1;

     for( ; rownum < rows.size() && rows[rownum].size() > 0 && rows[rownum][0] == "S"; ++rownum )
     {
         const single_row& row = rows[rownum];

         if( row.size() != header.size() )
         {
             wxLogError( _( "Invalid row size in row %zu. Expecting %zu elements but found %zu." ),
                         rownum,
                         header.size(),
                         row.size() );
             continue;
         }

         auto result = layers.emplace( row[layer_subclass_col], FABMASTER_LAYER{} );
         FABMASTER_LAYER& layer = result.first->second;

         layer.name = row[layer_subclass_col];
         layer.positive = true;
         layer.conductive = false;

         if( row[layer_class_col] == "ANTI ETCH" )
         {
             layer.positive = false;
             layer.conductive = true;
         }
         else if( row[layer_class_col] == "ETCH" )
         {
             layer.conductive = true;
         }
     }

     return rownum - aRow;
}


bool FABMASTER::assignLayers()
{
    bool has_l1 = false;
    int max_layer = 0;
    std::string max_layer_name;

    std::vector<std::pair<std::string, int>> extra_layers
    {
        { "ASSEMBLY_TOP", F_Fab },
        { "ASSEMBLY_BOTTOM", B_Fab },
        { "PLACE_BOUND_TOP", F_CrtYd },
        { "PLACE_BOUND_BOTTOM", B_CrtYd },
    };

    std::vector<FABMASTER_LAYER*> layer_order;

    int next_user_layer = User_1;

    for( auto& el : layers )
    {
        FABMASTER_LAYER& layer = el.second;
        layer.layerid = UNSELECTED_LAYER;

        if( layer.conductive )
        {
            layer_order.push_back( &layer );
        }
        else if( ( layer.name.find( "SILK" ) != std::string::npos
                   && layer.name.find( "AUTOSILK" )
                              == std::string::npos ) // Skip the autosilk layer
                 || layer.name.find( "DISPLAY" ) != std::string::npos )
        {
            if( layer.name.find( "B" ) != std::string::npos )
                layer.layerid = B_SilkS;
            else
                layer.layerid = F_SilkS;
        }
        else if( layer.name.find( "MASK" ) != std::string::npos ||
                 layer.name.find( "MSK" ) != std::string::npos )
        {
            if( layer.name.find( "B" ) != std::string::npos )
                layer.layerid = B_Mask;
            else
                layer.layerid = F_Mask;
        }
        else if( layer.name.find( "PAST" ) != std::string::npos )
        {
            if( layer.name.find( "B" ) != std::string::npos )
                layer.layerid = B_Paste;
            else
                layer.layerid = F_Paste;
        }
        else if( layer.name.find( "NCLEGEND" ) != std::string::npos )
        {
            layer.layerid = Dwgs_User;
        }
        else
        {
            // Try to gather as many other layers into user layers as possible

            // Skip ones that seem like a waste of good layers
            if( layer.name.find( "AUTOSILK" ) == std::string::npos )
            {
                if( next_user_layer <= User_9 )
                {
                    // Assign the mapping
                    layer.layerid = next_user_layer;
                    next_user_layer += 2;
                }
                else
                {
                    // Out of additional layers
                    // For now, drop it, but maybr we could gather onto some other layer.
                    // Or implement a proper layer remapper.
                    layer.disable = true;
                    wxLogWarning( _( "No user layer to put layer %s" ), layer.name );
                }
            }
        }
    }

    std::sort( layer_order.begin(), layer_order.end(), FABMASTER_LAYER::BY_ID() );

    for( size_t layeri = 0; layeri < layer_order.size(); ++layeri )
    {
        FABMASTER_LAYER* layer = layer_order[layeri];
        if( layeri == 0 )
            layer->layerid = F_Cu;
        else if( layeri == layer_order.size() - 1 )
            layer->layerid = B_Cu;
        else
            layer->layerid = layeri * 2 + 2;
    }

    for( auto& new_pair : extra_layers )
    {
        FABMASTER_LAYER new_layer;

        new_layer.name = new_pair.first;
        new_layer.layerid = new_pair.second;
        new_layer.conductive = false;

        auto result = layers.emplace( new_pair.first, new_layer );

        if( !result.second )
        {
            result.first->second.layerid = new_pair.second;
            result.first->second.disable = false;
        }
    }

    for( const auto& [layer_name, fabmaster_layer] : layers )
    {
        wxLogTrace( traceFabmaster, wxT( "Layer %s -> KiCad layer %d" ), layer_name,
                    fabmaster_layer.layerid );
    }

    return true;
}


/**
 * A!LAYER_SORT!LAYER_SUBCLASS!LAYER_ARTWORK!LAYER_USE!LAYER_CONDUCTOR!LAYER_DIELECTRIC_CONSTANT!
 * LAYER_ELECTRICAL_CONDUCTIVITY!LAYER_MATERIAL!LAYER_SHIELD_LAYER!LAYER_THERMAL_CONDUCTIVITY!
 * LAYER_THICKNESS!
 */
size_t FABMASTER::processLayers( size_t aRow )
{
    size_t rownum = aRow + 2;

    if( rownum >= rows.size() )
        return -1;

    auto& header = rows[aRow];
    double scale_factor = processScaleFactor( aRow + 1 );

    if( scale_factor <= 0.0 )
        return -1;

    int layer_sort_col      = getColFromName( aRow, "LAYERSORT" );
    int layer_subclass_col  = getColFromName( aRow, "LAYERSUBCLASS" );
    int layer_art_col       = getColFromName( aRow, "LAYERARTWORK" );
    int layer_use_col       = getColFromName( aRow, "LAYERUSE" );
    int layer_cond_col      = getColFromName( aRow, "LAYERCONDUCTOR" );
    int layer_er_col        = getColFromName( aRow, "LAYERDIELECTRICCONSTANT" );
    int layer_rho_col       = getColFromName( aRow, "LAYERELECTRICALCONDUCTIVITY" );
    int layer_mat_col       = getColFromName( aRow, "LAYERMATERIAL" );

    if( layer_sort_col < 0 || layer_subclass_col < 0 || layer_art_col < 0 || layer_use_col < 0
            || layer_cond_col < 0 || layer_er_col < 0 || layer_rho_col < 0 || layer_mat_col < 0 )
        return -1;

    for( ; rownum < rows.size() && rows[rownum].size() > 0 && rows[rownum][0] == "S"; ++rownum )
    {
        const single_row& row = rows[rownum];

        if( row.size() != header.size() )
        {
            wxLogError( _( "Invalid row size in row %zu. Expecting %zu elements but found %zu." ),
                        rownum,
                        header.size(),
                        row.size() );
            continue;
        }

        auto& layer_sort = row[layer_sort_col];
        auto& layer_subclass = row[layer_subclass_col];
        auto& layer_art = row[layer_art_col];
        auto& layer_use = row[layer_use_col];
        auto& layer_cond = row[layer_cond_col];
        auto& layer_er = row[layer_er_col];
        auto& layer_rho = row[layer_rho_col];
        auto& layer_mat = row[layer_mat_col];

        if( layer_mat == "AIR" )
            continue;

        FABMASTER_LAYER layer;

        if( layer_subclass.empty() )
        {
            if( layer_cond != "NO" )
                layer.name = "In.Cu" + layer_sort;
            else
                layer.name = "Dielectric" + layer_sort;
        }

        layer.positive = ( layer_art != "NEGATIVE" );

        layers.emplace( layer.name, layer );
    }

    return rownum - aRow;
}


/**
 * A!SUBCLASS!PAD_SHAPE_NAME!GRAPHIC_DATA_NAME!GRAPHIC_DATA_NUMBER!RECORD_TAG!GRAPHIC_DATA_1!
 * GRAPHIC_DATA_2!GRAPHIC_DATA_3!GRAPHIC_DATA_4!GRAPHIC_DATA_5!GRAPHIC_DATA_6!GRAPHIC_DATA_7!
 * GRAPHIC_DATA_8!GRAPHIC_DATA_9!PAD_STACK_NAME!REFDES!PIN_NUMBER!
 */
size_t FABMASTER::processCustomPads( size_t aRow )
{
    size_t rownum = aRow + 2;

    if( rownum >= rows.size() )
        return -1;

    auto& header = rows[aRow];
    double scale_factor = processScaleFactor( aRow + 1 );

    if( scale_factor <= 0.0 )
        return -1;

    int pad_subclass_col     = getColFromName( aRow, "SUBCLASS" );
    int pad_shape_name_col   = getColFromName( aRow, "PADSHAPENAME" );
    int pad_grdata_name_col  = getColFromName( aRow, "GRAPHICDATANAME" );
    int pad_grdata_num_col   = getColFromName( aRow, "GRAPHICDATANUMBER" );
    int pad_record_tag_col   = getColFromName( aRow, "RECORDTAG" );
    int pad_grdata1_col      = getColFromName( aRow, "GRAPHICDATA1" );
    int pad_grdata2_col      = getColFromName( aRow, "GRAPHICDATA2" );
    int pad_grdata3_col      = getColFromName( aRow, "GRAPHICDATA3" );
    int pad_grdata4_col      = getColFromName( aRow, "GRAPHICDATA4" );
    int pad_grdata5_col      = getColFromName( aRow, "GRAPHICDATA5" );
    int pad_grdata6_col      = getColFromName( aRow, "GRAPHICDATA6" );
    int pad_grdata7_col      = getColFromName( aRow, "GRAPHICDATA7" );
    int pad_grdata8_col      = getColFromName( aRow, "GRAPHICDATA8" );
    int pad_grdata9_col      = getColFromName( aRow, "GRAPHICDATA9" );
    int pad_stack_name_col   = getColFromName( aRow, "PADSTACKNAME" );
    int pad_refdes_col       = getColFromName( aRow, "REFDES" );
    int pad_pin_num_col      = getColFromName( aRow, "PINNUMBER" );

    if( pad_subclass_col < 0 || pad_shape_name_col < 0 || pad_grdata1_col < 0 || pad_grdata2_col < 0
            || pad_grdata3_col < 0 || pad_grdata4_col < 0 || pad_grdata5_col < 0
            || pad_grdata6_col < 0 || pad_grdata7_col < 0 || pad_grdata8_col < 0
            || pad_grdata9_col < 0 || pad_stack_name_col < 0 || pad_refdes_col < 0
            || pad_pin_num_col < 0 )
        return -1;

    for( ; rownum < rows.size() && rows[rownum].size() > 0 && rows[rownum][0] == "S"; ++rownum )
    {
        const single_row& row = rows[rownum];

        if( row.size() != header.size() )
        {
            wxLogError( _( "Invalid row size in row %zu. Expecting %zu elements but found %zu." ),
                        rownum,
                        header.size(),
                        row.size() );

            continue;
        }

        auto& pad_layer         = row[pad_subclass_col];
        auto pad_shape_name     = row[pad_shape_name_col];
        auto& pad_record_tag    = row[pad_record_tag_col];

        GRAPHIC_DATA gr_data;
        gr_data.graphic_dataname = row[pad_grdata_name_col];
        gr_data.graphic_datanum = row[pad_grdata_num_col];
        gr_data.graphic_data1 = row[pad_grdata1_col];
        gr_data.graphic_data2 = row[pad_grdata2_col];
        gr_data.graphic_data3 = row[pad_grdata3_col];
        gr_data.graphic_data4 = row[pad_grdata4_col];
        gr_data.graphic_data5 = row[pad_grdata5_col];
        gr_data.graphic_data6 = row[pad_grdata6_col];
        gr_data.graphic_data7 = row[pad_grdata7_col];
        gr_data.graphic_data8 = row[pad_grdata8_col];
        gr_data.graphic_data9 = row[pad_grdata9_col];

        auto& pad_stack_name     = row[pad_stack_name_col];
        auto& pad_refdes         = row[pad_refdes_col];
        auto& pad_pin_num        = row[pad_pin_num_col];

        // N.B. We get the FIGSHAPE records as "FIG_SHAPE name".  We only want "name"
        // and we don't process other pad shape records
        std::string prefix( "FIG_SHAPE " );

        if( pad_shape_name.length() <= prefix.length()
                || !std::equal( prefix.begin(), prefix.end(), pad_shape_name.begin() ) )
        {
            continue;
        }

        // Custom pads are a series of records with the same record ID but incrementing
        // Sequence numbers.
        int id          = -1;
        int seq         = -1;

        if( std::sscanf( pad_record_tag.c_str(), "%d %d", &id, &seq ) != 2 )
        {
            wxLogError( _( "Invalid format for id string '%s' in custom pad row %zu." ),
                        pad_record_tag.c_str(),
                        rownum );
            continue;
        }

        auto name = pad_shape_name.substr( prefix.length() );
        name += "_" + pad_refdes + "_" + pad_pin_num;
        auto ret = pad_shapes.emplace( name, FABMASTER_PAD_SHAPE{} );

        auto& custom_pad = ret.first->second;

        // If we were able to insert the pad name, then we need to initialize the
        // record
        if( ret.second )
        {
            custom_pad.name = name;
            custom_pad.padstack = pad_stack_name;
            custom_pad.pinnum = pad_pin_num;
            custom_pad.refdes = pad_refdes;
        }

        // At this point we extract the individual graphical elements for processing the complex
        // pad.  The coordinates are in board origin format, so we'll need to fix the offset later
        // when we assign them to the modules.

        auto gr_item = std::unique_ptr<GRAPHIC_ITEM>( processGraphic( gr_data, scale_factor ) );

        if( gr_item )
        {
            gr_item->layer = pad_layer;
            gr_item->refdes = pad_refdes;
            gr_item->seq = seq;
            gr_item->subseq = 0;

            // emplace may fail here, in which case, it returns the correct position to use for
            // the existing map
            auto pad_it = custom_pad.elements.emplace( id, graphic_element{} );
            auto retval = pad_it.first->second.insert( std::move(gr_item ) );

            if( !retval.second )
            {
                wxLogError( _( "Could not insert graphical item %d into padstack '%s'." ),
                            seq,
                            pad_stack_name.c_str() );
            }
        }
        else
        {
            wxLogError( _( "Unrecognized pad shape primitive '%s' in row %zu." ),
                        gr_data.graphic_dataname,
                        rownum );
        }
    }

    return rownum - aRow;
}


FABMASTER::GRAPHIC_LINE* FABMASTER::processLine( const FABMASTER::GRAPHIC_DATA& aData,
                                                 double aScale )
{
    GRAPHIC_LINE* new_line = new GRAPHIC_LINE ;

    new_line->shape   = GR_SHAPE_LINE;
    new_line->start_x = KiROUND( readDouble( aData.graphic_data1 ) * aScale );
    new_line->start_y = -KiROUND( readDouble( aData.graphic_data2 ) * aScale );
    new_line->end_x   = KiROUND( readDouble( aData.graphic_data3 ) * aScale );
    new_line->end_y   = -KiROUND( readDouble( aData.graphic_data4 ) * aScale );
    new_line->width   = KiROUND( readDouble( aData.graphic_data5 ) * aScale );

    return new_line;
}


FABMASTER::GRAPHIC_ARC* FABMASTER::processArc( const FABMASTER::GRAPHIC_DATA& aData, double aScale )
{
    GRAPHIC_ARC* new_arc = new GRAPHIC_ARC ;

    new_arc->shape    = GR_SHAPE_ARC;
    new_arc->start_x  = KiROUND( readDouble( aData.graphic_data1 ) * aScale );
    new_arc->start_y  = -KiROUND( readDouble( aData.graphic_data2 ) * aScale );
    new_arc->end_x    = KiROUND( readDouble( aData.graphic_data3 ) * aScale );
    new_arc->end_y    = -KiROUND( readDouble( aData.graphic_data4 ) * aScale );
    new_arc->center_x = KiROUND( readDouble( aData.graphic_data5 ) * aScale );
    new_arc->center_y = -KiROUND( readDouble( aData.graphic_data6 ) * aScale );
    new_arc->radius   = KiROUND( readDouble( aData.graphic_data7 ) * aScale );
    new_arc->width    = KiROUND( readDouble( aData.graphic_data8 ) * aScale );

    new_arc->clockwise = ( aData.graphic_data9 != "COUNTERCLOCKWISE" );

    EDA_ANGLE startangle( VECTOR2I( new_arc->start_x, new_arc->start_y )
                            - VECTOR2I( new_arc->center_x, new_arc->center_y ) );
    EDA_ANGLE endangle( VECTOR2I( new_arc->end_x, new_arc->end_y )
                            - VECTOR2I( new_arc->center_x, new_arc->center_y ) );
    EDA_ANGLE angle;

    startangle.Normalize();
    endangle.Normalize();

    VECTOR2I center( new_arc->center_x, new_arc->center_y );
    VECTOR2I start( new_arc->start_x, new_arc->start_y );
    VECTOR2I mid( new_arc->start_x, new_arc->start_y );
    VECTOR2I end( new_arc->end_x, new_arc->end_y );

    angle = endangle - startangle;

    if( new_arc->clockwise && angle < ANGLE_0 )
        angle += ANGLE_360;
    if( !new_arc->clockwise && angle > ANGLE_0 )
        angle -= ANGLE_360;

    if( start == end )
        angle = -ANGLE_360;

    RotatePoint( mid, center, -angle / 2.0 );

    if( start == end )
        new_arc->shape = GR_SHAPE_CIRCLE;

    new_arc->result = SHAPE_ARC( start, mid, end, 0 );

    return new_arc;
}


FABMASTER::GRAPHIC_ARC* FABMASTER::processCircle( const GRAPHIC_DATA& aData, double aScale )
{
    /*
     * Example:
     *  S!DRAWING FORMAT!ASSY!CIRCLE!2!251744 1!-2488.00!1100.00!240.00!240.00!0!!!!!!
     *
     * Although this is a circle, we treat it as an 360 degree arc.
     * This is because files can contain circles in both forms and the arc form
     * is more convenient for directly adding to SHAPE_POLY_SET when needed.
     *
     * It will be identified as a circle based on the 'shape' field, and turned
     * back into a circle when needed (or used as an arc if it is part of a polygon).
     */

    std::unique_ptr<GRAPHIC_ARC> new_circle = std::make_unique<GRAPHIC_ARC>();

    new_circle->shape = GR_SHAPE_CIRCLE;

    const VECTOR2I center{
        KiROUND( readDouble( aData.graphic_data1 ) * aScale ),
        -KiROUND( readDouble( aData.graphic_data2 ) * aScale ),
    };
    const VECTOR2I size = KiROUND( readDouble( aData.graphic_data3 ) * aScale,
                                   readDouble( aData.graphic_data4 ) * aScale );

    if( size.x != size.y )
    {
        wxLogError( _( "Circle with unequal x and y radii (x=%d, y=%d)" ), size.x, size.y );
        return nullptr;
    }

    new_circle->width = KiROUND( readDouble( aData.graphic_data5 ) * aScale );

    new_circle->radius = size.x / 2;

    // Fake up a 360 degree arc
    const VECTOR2I start = center - VECTOR2I{ new_circle->radius, 0 };
    const VECTOR2I mid = center + VECTOR2I{ new_circle->radius, 0 };

    new_circle->start_x = start.x;
    new_circle->start_y = start.y;

    new_circle->end_x = start.x;
    new_circle->end_y = start.y;

    new_circle->center_x = center.x;
    new_circle->center_y = center.y;

    new_circle->clockwise = true;

    new_circle->result = SHAPE_ARC{ start, mid, start, 0 };

    return new_circle.release();
}


FABMASTER::GRAPHIC_RECTANGLE* FABMASTER::processRectangle( const FABMASTER::GRAPHIC_DATA& aData,
                                                           double aScale )
{
    /*
     * Examples:
     *  S!ROUTE KEEPOUT!BOTTOM!RECTANGLE!259!10076 1!-90.00!-1000.00!-60.00!-990.00!1!!!!!!
     */

    GRAPHIC_RECTANGLE* new_rect = new GRAPHIC_RECTANGLE;

    new_rect->shape   = GR_SHAPE_RECTANGLE;
    new_rect->start_x = KiROUND( readDouble( aData.graphic_data1 ) * aScale );
    new_rect->start_y = -KiROUND( readDouble( aData.graphic_data2 ) * aScale );
    new_rect->end_x   = KiROUND( readDouble( aData.graphic_data3 ) * aScale );
    new_rect->end_y   = -KiROUND( readDouble( aData.graphic_data4 ) * aScale );
    new_rect->fill    = aData.graphic_data5 == "1";
    new_rect->width   = 0;

    return new_rect;
}


FABMASTER::GRAPHIC_RECTANGLE* FABMASTER::processFigRectangle( const FABMASTER::GRAPHIC_DATA& aData,
                                                              double aScale )
{
    /*
     * Examples:
     *  S!MANUFACTURING!NCLEGEND-1-10!FIG_RECTANGLE!6!8318 1!4891.50!1201.00!35.43!26.57!0!!!!!!
     */

    auto new_rect = std::make_unique<GRAPHIC_RECTANGLE>();

    const int center_x = KiROUND( readDouble( aData.graphic_data1 ) * aScale );
    const int center_y = -KiROUND( readDouble( aData.graphic_data2 ) * aScale );

    const int size_x = KiROUND( readDouble( aData.graphic_data3 ) * aScale );
    const int size_y = KiROUND( readDouble( aData.graphic_data4 ) * aScale );

    new_rect->shape = GR_SHAPE_RECTANGLE;
    new_rect->start_x = center_x - size_x / 2;
    new_rect->start_y = center_y + size_y / 2;
    new_rect->end_x = center_x + size_x / 2;
    new_rect->end_y = center_y - size_y / 2;
    new_rect->fill = aData.graphic_data5 == "1";
    new_rect->width = 0;

    return new_rect.release();
}


FABMASTER::GRAPHIC_RECTANGLE* FABMASTER::processSquare( const FABMASTER::GRAPHIC_DATA& aData,
                                                        double                         aScale )
{
    /*
     * Example:
     *   S!DRAWING FORMAT!ASSY!SQUARE!5!250496 1!4813.08!2700.00!320.00!320.00!0!!!!!!
     */

    // This appears to be identical to a FIG_RECTANGLE
    return processFigRectangle( aData, aScale );
}


FABMASTER::GRAPHIC_OBLONG* FABMASTER::processOblong( const FABMASTER::GRAPHIC_DATA& aData,
                                                     double                         aScale )
{
    /*
     * Examples:
     *  S!DRAWING FORMAT!ASSY!OBLONG_X!11!250497 1!4449.08!2546.40!240.00!64.00!0!!!!!!
     *  S!DRAWING FORMAT!ASSY!OBLONG_Y!12!251256 1!15548.68!1900.00!280.00!720.00!0!!!!!!
     */
    auto new_oblong = std::make_unique<GRAPHIC_OBLONG>();

    new_oblong->shape = GR_SHAPE_OBLONG;
    new_oblong->oblong_x = aData.graphic_dataname == "OBLONG_X";
    new_oblong->start_x = KiROUND( readDouble( aData.graphic_data1 ) * aScale );
    new_oblong->start_y = -KiROUND( readDouble( aData.graphic_data2 ) * aScale );
    new_oblong->size_x = KiROUND( readDouble( aData.graphic_data3 ) * aScale );
    new_oblong->size_y = KiROUND( readDouble( aData.graphic_data4 ) * aScale );

    // Unclear if this is fill or width
    new_oblong->width = KiROUND( readDouble( aData.graphic_data5 ) * aScale );

    return new_oblong.release();
}


FABMASTER::GRAPHIC_POLYGON* FABMASTER::processPolygon( const FABMASTER::GRAPHIC_DATA& aData,
                                                       double                         aScale )
{
    /*
     * Examples:
     *  S!MANUFACTURING!NCLEGEND-1-6!TRIANGLE_1!18!252565 1!-965.00!5406.00!125.00!125.00!0!!!!!!
     *  S!MANUFACTURING!NCLEGEND-1-6!DIAMOND!7!252566 1!-965.00!5656.00!63.00!63.00!0!!!!!!
     *  S!MANUFACTURING!NCLEGEND-1-6!OCTAGON!3!252567 1!-965.00!5906.00!40.00!40.00!0!!!!!!
     *  S!MANUFACTURING!NCLEGEND-1-6!HEXAGON_Y!16!252568 1!-965.00!6156.00!35.00!35.00!0!!!!!!
     *  S!MANUFACTURING!NCLEGEND-1-6!HEXAGON_X!15!252569 1!-965.00!6406.00!12.00!12.00!0!!!!!!
     */

    const VECTOR2D c{
        readDouble( aData.graphic_data1 ) * aScale,
        -readDouble( aData.graphic_data2 ) * aScale,
    };

    const VECTOR2D s{
        readDouble( aData.graphic_data3 ) * aScale,
        readDouble( aData.graphic_data4 ) * aScale,
    };

    if( s.x != s.y )
    {
    }

    auto new_poly = std::make_unique<GRAPHIC_POLYGON>();
    new_poly->shape = GR_SHAPE_POLYGON;
    new_poly->width = KiROUND( readDouble( aData.graphic_data5 ) * aScale );

    int       radius = s.x / 2;
    bool      across_corners = true;
    EDA_ANGLE pt0_angle = ANGLE_90; // /Pointing up
    int       n_pts = 0;

    if( aData.graphic_dataname == "TRIANGLE_1" )
    {
        // Upright equilateral triangle (pointing upwards, horizontal base)
        // The size appears to be (?) the size of the circumscribing circle,
        // rather than the width of the base.
        n_pts = 3;
    }
    else if( aData.graphic_dataname == "DIAMOND" )
    {
        // Square diamond (can it be non-square?)
        // Size is point-to-point width/height
        n_pts = 4;
    }
    else if( aData.graphic_dataname == "HEXAGON_X" )
    {
        // Hexagon with horizontal top/bottom
        // Size is the overall width (across corners)
        n_pts = 6;
        pt0_angle = ANGLE_0;
    }
    else if( aData.graphic_dataname == "HEXAGON_Y" )
    {
        // Hexagon with vertical left/right sides
        // Size is the height (i.e. across corners)
        n_pts = 6;
    }
    else if( aData.graphic_dataname == "OCTAGON" )
    {
        // Octagon with horizontal/vertical sides
        // Size is the overall width (across flats)
        across_corners = false;
        pt0_angle = FULL_CIRCLE / 16;
        n_pts = 8;
    }
    else
    {
        wxCHECK_MSG( false, nullptr,
                     wxString::Format( "Unhandled polygon type: %s", aData.graphic_dataname ) );
    }

    new_poly->m_pts =
            KIGEOM::MakeRegularPolygonPoints( c, n_pts, radius, across_corners, pt0_angle );
    return new_poly.release();
}


FABMASTER::GRAPHIC_CROSS* FABMASTER::processCross( const FABMASTER::GRAPHIC_DATA& aData,
                                                   double                         aScale )
{
    /*
     * Examples:
     *  S!MANUFACTURING!NCLEGEND-1-6!CROSS!4!252571 1!-965.00!6906.00!6.00!6.00!0!!!!!!
     */
    auto new_cross = std::make_unique<GRAPHIC_CROSS>();

    new_cross->shape = GR_SHAPE_CROSS;
    new_cross->start_x = KiROUND( readDouble( aData.graphic_data1 ) * aScale );
    new_cross->start_y = -KiROUND( readDouble( aData.graphic_data2 ) * aScale );
    new_cross->size_x = KiROUND( readDouble( aData.graphic_data3 ) * aScale );
    new_cross->size_y = KiROUND( readDouble( aData.graphic_data4 ) * aScale );
    new_cross->width = KiROUND( readDouble( aData.graphic_data5 ) * aScale );

    return new_cross.release();
}


FABMASTER::GRAPHIC_TEXT* FABMASTER::processText( const FABMASTER::GRAPHIC_DATA& aData,
                                                 double                         aScale )
{
    GRAPHIC_TEXT* new_text = new GRAPHIC_TEXT;

    new_text->shape    = GR_SHAPE_TEXT;
    new_text->start_x  = KiROUND( readDouble( aData.graphic_data1 ) * aScale );
    new_text->start_y  = -KiROUND( readDouble( aData.graphic_data2 ) * aScale );
    new_text->rotation = KiROUND( readDouble( aData.graphic_data3 ) );
    new_text->mirror   = ( aData.graphic_data4 == "YES" );

    if( aData.graphic_data5 == "RIGHT" )
        new_text->orient = GR_TEXT_H_ALIGN_RIGHT;
    else if( aData.graphic_data5 == "CENTER" )
        new_text->orient = GR_TEXT_H_ALIGN_CENTER;
    else
        new_text->orient = GR_TEXT_H_ALIGN_LEFT;

    std::vector<std::string> toks = split( aData.graphic_data6, " \t" );

    if( toks.size() < 8 )
    {
            // We log the error here but continue in the case of too few tokens
            wxLogError( _( "Invalid token count. Expected 8 but found %zu." ), toks.size() );
            new_text->height = 0;
            new_text->width  = 0;
            new_text->ital   = false;
            new_text->thickness = 0;
    }
    else
    {
        // 0 = size
        // 1 = font
        new_text->height = KiROUND( readDouble( toks[2] ) * aScale );
        new_text->width  = KiROUND( readDouble( toks[3] ) * aScale );
        new_text->ital   = readDouble( toks[4] ) != 0.0;
        // 5 = character spacing
        // 6 = line spacing
        new_text->thickness = KiROUND( readDouble( toks[7] ) * aScale );
    }

    new_text->text = aData.graphic_data7;
    return new_text;
}


FABMASTER::GRAPHIC_ITEM* FABMASTER::processGraphic( const GRAPHIC_DATA& aData, double aScale )
{
    GRAPHIC_ITEM* retval = nullptr;

    if( aData.graphic_dataname == "LINE" )
        retval = processLine( aData, aScale );
    else if( aData.graphic_dataname == "ARC" )
        retval = processArc( aData, aScale );
    else if( aData.graphic_dataname == "CIRCLE" )
        retval = processCircle( aData, aScale );
    else if( aData.graphic_dataname == "RECTANGLE" )
        retval = processRectangle( aData, aScale );
    else if( aData.graphic_dataname == "FIG_RECTANGLE" )
        retval = processFigRectangle( aData, aScale );
    else if( aData.graphic_dataname == "SQUARE" )
        retval = processSquare( aData, aScale );
    else if( aData.graphic_dataname == "OBLONG_X" || aData.graphic_dataname == "OBLONG_Y" )
        retval = processOblong( aData, aScale );
    else if( aData.graphic_dataname == "TRIANGLE_1" || aData.graphic_dataname == "DIAMOND"
             || aData.graphic_dataname == "HEXAGON_X" || aData.graphic_dataname == "HEXAGON_Y"
             || aData.graphic_dataname == "OCTAGON" )
        retval = processPolygon( aData, aScale );
    else if( aData.graphic_dataname == "CROSS" )
        retval = processCross( aData, aScale );
    else if( aData.graphic_dataname == "TEXT" )
        retval = processText( aData, aScale );

    if( retval && !aData.graphic_data10.empty() )
    {
        if( aData.graphic_data10 == "CONNECT" )
            retval->type = GR_TYPE_CONNECT;
        else if( aData.graphic_data10 == "NOTCONNECT" )
            retval->type = GR_TYPE_NOTCONNECT;
        else if( aData.graphic_data10 == "SHAPE" )
            retval->type = GR_TYPE_NOTCONNECT;
        else if( aData.graphic_data10 == "VOID" )
            retval->type = GR_TYPE_NOTCONNECT;
        else if( aData.graphic_data10 == "POLYGON" )
            retval->type = GR_TYPE_NOTCONNECT;
        else
            retval->type = GR_TYPE_NONE;
    }

    return retval;
}


/**
 * A!GRAPHIC_DATA_NAME!GRAPHIC_DATA_NUMBER!RECORD_TAG!GRAPHIC_DATA_1!GRAPHIC_DATA_2!GRAPHIC_DATA_3!
 * GRAPHIC_DATA_4!GRAPHIC_DATA_5!GRAPHIC_DATA_6!GRAPHIC_DATA_7!GRAPHIC_DATA_8!GRAPHIC_DATA_9!
 * SUBCLASS!SYM_NAME!REFDES!
 */
size_t FABMASTER::processGeometry( size_t aRow )
{
    size_t rownum = aRow + 2;

    if( rownum >= rows.size() )
        return -1;

    const single_row& header = rows[aRow];
    double scale_factor = processScaleFactor( aRow + 1 );

    if( scale_factor <= 0.0 )
        return -1;

    int geo_name_col = getColFromName( aRow, "GRAPHICDATANAME" );
    int geo_num_col = getColFromName( aRow, "GRAPHICDATANUMBER" );
    int geo_tag_col = getColFromName( aRow, "RECORDTAG" );
    int geo_grdata1_col = getColFromName( aRow, "GRAPHICDATA1" );
    int geo_grdata2_col = getColFromName( aRow, "GRAPHICDATA2" );
    int geo_grdata3_col = getColFromName( aRow, "GRAPHICDATA3" );
    int geo_grdata4_col = getColFromName( aRow, "GRAPHICDATA4" );
    int geo_grdata5_col = getColFromName( aRow, "GRAPHICDATA5" );
    int geo_grdata6_col = getColFromName( aRow, "GRAPHICDATA6" );
    int geo_grdata7_col = getColFromName( aRow, "GRAPHICDATA7" );
    int geo_grdata8_col = getColFromName( aRow, "GRAPHICDATA8" );
    int geo_grdata9_col = getColFromName( aRow, "GRAPHICDATA9" );
    int geo_subclass_col = getColFromName( aRow, "SUBCLASS" );
    int geo_sym_name_col = getColFromName( aRow, "SYMNAME" );
    int geo_refdes_col = getColFromName( aRow, "REFDES" );

    if( geo_name_col < 0 || geo_num_col < 0 || geo_grdata1_col < 0 || geo_grdata2_col < 0
            || geo_grdata3_col < 0 || geo_grdata4_col < 0 || geo_grdata5_col < 0
            || geo_grdata6_col < 0 || geo_grdata7_col < 0 || geo_grdata8_col < 0
            || geo_grdata9_col < 0 || geo_subclass_col < 0 || geo_sym_name_col < 0
            || geo_refdes_col < 0 )
        return -1;

    for( ; rownum < rows.size() && rows[rownum].size() > 0 && rows[rownum][0] == "S"; ++rownum )
    {
        const single_row& row = rows[rownum];

        if( row.size() != header.size() )
        {
            wxLogError( _( "Invalid row size in row %zu. Expecting %zu elements but found %zu." ),
                        rownum,
                        header.size(),
                        row.size() );
            continue;
        }

        auto& geo_tag = row[geo_tag_col];

        GRAPHIC_DATA gr_data;
        gr_data.graphic_dataname = row[geo_name_col];
        gr_data.graphic_datanum = row[geo_num_col];
        gr_data.graphic_data1 = row[geo_grdata1_col];
        gr_data.graphic_data2 = row[geo_grdata2_col];
        gr_data.graphic_data3 = row[geo_grdata3_col];
        gr_data.graphic_data4 = row[geo_grdata4_col];
        gr_data.graphic_data5 = row[geo_grdata5_col];
        gr_data.graphic_data6 = row[geo_grdata6_col];
        gr_data.graphic_data7 = row[geo_grdata7_col];
        gr_data.graphic_data8 = row[geo_grdata8_col];
        gr_data.graphic_data9 = row[geo_grdata9_col];

        auto& geo_refdes = row[geo_refdes_col];

        // Grouped graphics are a series of records with the same record ID but incrementing
        // Sequence numbers.
        int id          = -1;
        int seq         = -1;
        int subseq      = 0;

        if( std::sscanf( geo_tag.c_str(), "%d %d %d", &id, &seq, &subseq ) < 2 )
        {
            wxLogError( _( "Invalid format for record_tag string '%s' in row %zu." ),
                        geo_tag.c_str(),
                        rownum );
            continue;
        }

        auto gr_item = std::unique_ptr<GRAPHIC_ITEM>( processGraphic( gr_data, scale_factor ) );

        if( !gr_item )
            continue;

        gr_item->layer = row[geo_subclass_col];
        gr_item->seq = seq;
        gr_item->subseq = subseq;

        if( geo_refdes.empty() )
        {
            if( board_graphics.empty() || board_graphics.back().id != id )
            {
                GEOM_GRAPHIC new_gr;
                new_gr.subclass = row[geo_subclass_col];
                new_gr.refdes   = row[geo_refdes_col];
                new_gr.name     = row[geo_sym_name_col];
                new_gr.id       = id;
                new_gr.elements = std::make_unique<graphic_element>();
                board_graphics.push_back( std::move( new_gr ) );
            }

            GEOM_GRAPHIC& graphic = board_graphics.back();
            graphic.elements->emplace( std::move( gr_item ) );
        }
        else
        {
            auto sym_gr_it = comp_graphics.emplace( geo_refdes,
                    std::map<int, GEOM_GRAPHIC>{} );
            auto map_it = sym_gr_it.first->second.emplace( id, GEOM_GRAPHIC{} );
            auto& gr = map_it.first;

            if( map_it.second )
            {
                gr->second.subclass = row[geo_subclass_col];
                gr->second.refdes   = row[geo_refdes_col];
                gr->second.name     = row[geo_sym_name_col];
                gr->second.id       = id;
                gr->second.elements = std::make_unique<graphic_element>();
            }

            auto result = gr->second.elements->emplace( std::move( gr_item ) );
        }
    }

    return rownum - aRow;
}


/**
 *  A!VIA_X!VIA_Y!PAD_STACK_NAME!NET_NAME!TEST_POINT!
 */
size_t FABMASTER::processVias( size_t aRow )
{
    size_t rownum = aRow + 2;

    if( rownum >= rows.size() )
        return -1;

    const single_row& header = rows[aRow];
    double scale_factor = processScaleFactor( aRow + 1 );

    if( scale_factor <= 0.0 )
        return -1;

    int viax_col = getColFromName( aRow, "VIAX" );
    int viay_col = getColFromName( aRow, "VIAY" );
    int padstack_name_col = getColFromName( aRow, "PADSTACKNAME" );
    int net_name_col = getColFromName( aRow, "NETNAME" );
    int test_point_col = getColFromName( aRow, "TESTPOINT" );

    if( viax_col < 0 || viay_col < 0 || padstack_name_col < 0 || net_name_col < 0
            || test_point_col < 0 )
        return -1;

    for( ; rownum < rows.size() && rows[rownum].size() > 0 && rows[rownum][0] == "S"; ++rownum )
    {
        const single_row& row = rows[rownum];

        if( row.size() != header.size() )
        {
            wxLogError( _( "Invalid row size in row %zu. Expecting %zu elements but found %zu." ),
                        rownum,
                        header.size(),
                        row.size() );
            continue;
        }

        vias.emplace_back( std::make_unique<FM_VIA>() );
        auto& via = vias.back();

        via->x = KiROUND( readDouble( row[viax_col] ) * scale_factor );
        via->y = -KiROUND( readDouble( row[viay_col] ) * scale_factor );
        via->padstack = row[padstack_name_col];
        via->net = row[net_name_col];
        via->test_point = ( row[test_point_col] == "YES" );
    }

    return rownum - aRow;
}


/**
 * A!CLASS!SUBCLASS!GRAPHIC_DATA_NAME!GRAPHIC_DATA_NUMBER!RECORD_TAG!GRAPHIC_DATA_1!GRAPHIC_DATA_2!
 * GRAPHIC_DATA_3!GRAPHIC_DATA_4!GRAPHIC_DATA_5!GRAPHIC_DATA_6!GRAPHIC_DATA_7!GRAPHIC_DATA_8!
 * GRAPHIC_DATA_9!NET_NAME!
 */
size_t FABMASTER::processTraces( size_t aRow )
{
    size_t rownum = aRow + 2;

    if( rownum >= rows.size() )
        return -1;

    const single_row& header = rows[aRow];
    double scale_factor = processScaleFactor( aRow + 1 );

    if( scale_factor <= 0.0 )
        return -1;

    int class_col = getColFromName( aRow, "CLASS" );
    int layer_col = getColFromName( aRow, "SUBCLASS" );
    int grdata_name_col = getColFromName( aRow, "GRAPHICDATANAME" );
    int grdata_num_col = getColFromName( aRow, "GRAPHICDATANUMBER" );
    int tag_col = getColFromName( aRow, "RECORDTAG" );
    int grdata1_col = getColFromName( aRow, "GRAPHICDATA1" );
    int grdata2_col = getColFromName( aRow, "GRAPHICDATA2" );
    int grdata3_col = getColFromName( aRow, "GRAPHICDATA3" );
    int grdata4_col = getColFromName( aRow, "GRAPHICDATA4" );
    int grdata5_col = getColFromName( aRow, "GRAPHICDATA5" );
    int grdata6_col = getColFromName( aRow, "GRAPHICDATA6" );
    int grdata7_col = getColFromName( aRow, "GRAPHICDATA7" );
    int grdata8_col = getColFromName( aRow, "GRAPHICDATA8" );
    int grdata9_col = getColFromName( aRow, "GRAPHICDATA9" );
    int netname_col = getColFromName( aRow, "NETNAME" );

    if( class_col < 0 || layer_col < 0 || grdata_name_col < 0 || grdata_num_col < 0
            || tag_col < 0 || grdata1_col < 0 || grdata2_col < 0 || grdata3_col < 0
            || grdata4_col < 0 || grdata5_col < 0 || grdata6_col < 0 || grdata7_col < 0
            || grdata8_col < 0 || grdata9_col < 0 || netname_col < 0 )
        return -1;

    for( ; rownum < rows.size() && rows[rownum].size() > 0 && rows[rownum][0] == "S"; ++rownum )
    {
        const single_row& row = rows[rownum];

        if( row.size() != header.size() )
        {
            wxLogError( _( "Invalid row size in row %zu.  Expecting %zu elements but found %zu." ),
                        rownum,
                        header.size(),
                        row.size() );
            continue;
        }

        GRAPHIC_DATA gr_data;
        gr_data.graphic_dataname = row[grdata_name_col];
        gr_data.graphic_datanum = row[grdata_num_col];
        gr_data.graphic_data1 = row[grdata1_col];
        gr_data.graphic_data2 = row[grdata2_col];
        gr_data.graphic_data3 = row[grdata3_col];
        gr_data.graphic_data4 = row[grdata4_col];
        gr_data.graphic_data5 = row[grdata5_col];
        gr_data.graphic_data6 = row[grdata6_col];
        gr_data.graphic_data7 = row[grdata7_col];
        gr_data.graphic_data8 = row[grdata8_col];
        gr_data.graphic_data9 = row[grdata9_col];

        const std::string& geo_tag = row[tag_col];
        // Grouped graphics are a series of records with the same record ID but incrementing
        // Sequence numbers.
        int id          = -1;
        int seq         = -1;
        int subseq      = 0;

        if( std::sscanf( geo_tag.c_str(), "%d %d %d", &id, &seq, &subseq ) < 2 )
        {
            wxLogError( _( "Invalid format for record_tag string '%s' in row %zu." ),
                        geo_tag.c_str(),
                        rownum );
            continue;
        }

        auto gr_item = std::unique_ptr<GRAPHIC_ITEM>( processGraphic( gr_data, scale_factor ) );

        if( !gr_item )
        {
            wxLogTrace( traceFabmaster,  _( "Unhandled graphic item '%s' in row %zu." ),
                        gr_data.graphic_dataname.c_str(),
                        rownum );
            continue;
        }

        auto new_trace = std::make_unique<TRACE>();
        new_trace->id      = id;
        new_trace->layer   = row[layer_col];
        new_trace->netname = row[netname_col];
        new_trace->lclass  = row[class_col];

        gr_item->layer = row[layer_col];
        gr_item->seq = seq;
        gr_item->subseq = subseq;

        // Collect the reference designator positions for the footprints later
        if( new_trace->lclass == "REF DES" )
        {
            auto result = refdes.emplace( std::move( new_trace ) );
            auto& ref   = *result.first;
            ref->segment.emplace( std::move( gr_item ) );
        }
        else if( new_trace->lclass == "DEVICE TYPE" || new_trace->lclass == "COMPONENT VALUE"
                 || new_trace->lclass == "TOLERANCE" )
        {
            // TODO: This seems like a value field, but it's not immediately clear how to map it
            // to the right footprint.
            // So these spam the board with huge amount of overlapping text.

            // Examples:
            //   S!DEVICE TYPE!SILKSCREEN_BOTTOM!TEXT!260!255815 1!2725.00!1675.00!270.000!YES!LEFT!45 0 60.00 48.00 0.000 0.00 0.00 0.00!CAP_0.1UF_X5R_6.3V_20% 0201 _40!!!!
            //   S!DEVICE TYPE!ASSEMBLY_BOTTOM!TEXT!260!255816 1!2725.00!1675.00!270.000!YES!LEFT!45 0 60.00 48.00 0.000 0.00 0.00 0.00!CAP_0.1UF_X5R_6.3V_20% 0201 _40!!!!
            //   S!COMPONENT VALUE!SILKSCREEN_BOTTOM!TEXT!260!18949 1!361.665!1478.087!270.000!YES!LEFT!31 0 30.000 20.000 0.000 6.000 31.000 6.000!0.01uF!!!!

            // For now, just don't do anything with them.
        }
        else if( gr_item->width == 0 )
        {
            auto result = zones.emplace( std::move( new_trace ) );
            auto& zone  = *result.first;
            auto gr_result = zone->segment.emplace( std::move( gr_item ) );

            if( !gr_result.second )
            {
                wxLogError( _( "Duplicate item for ID %d and sequence %d in row %zu." ),
                            id,
                            seq,
                            rownum );
            }
        }
        else
        {
            auto result = traces.emplace( std::move( new_trace ) );
            auto& trace  = *result.first;
            auto gr_result = trace->segment.emplace( std::move( gr_item ) );

            if( !gr_result.second )
            {
                wxLogError( _( "Duplicate item for ID %d and sequence %d in row %zu." ),
                            id,
                            seq,
                            rownum );
            }
        }
    }

    return rownum - aRow;
}


FABMASTER::SYMTYPE FABMASTER::parseSymType( const std::string& aSymType )
{
    if( aSymType == "PACKAGE" )
        return SYMTYPE_PACKAGE;
    else if( aSymType == "DRAFTING")
        return SYMTYPE_DRAFTING;
    else if( aSymType == "MECHANICAL" )
        return SYMTYPE_MECH;
    else if( aSymType == "FORMAT" )
        return SYMTYPE_FORMAT;

    return SYMTYPE_NONE;
}


FABMASTER::COMPCLASS FABMASTER::parseCompClass( const std::string& aCmpClass )
{
    if( aCmpClass == "IO" )
        return COMPCLASS_IO;
    else if( aCmpClass == "IC" )
        return COMPCLASS_IC;
    else if( aCmpClass == "DISCRETE" )
        return COMPCLASS_DISCRETE;

    return COMPCLASS_NONE;
}


/**
 * A!REFDES!COMP_CLASS!COMP_PART_NUMBER!COMP_HEIGHT!COMP_DEVICE_LABEL!COMP_INSERTION_CODE!SYM_TYPE!
 * SYM_NAME!SYM_MIRROR!SYM_ROTATE!SYM_X!SYM_Y!COMP_VALUE!COMP_TOL!COMP_VOLTAGE!
 */
size_t FABMASTER::processFootprints( size_t aRow )
{
    size_t rownum = aRow + 2;

    if( rownum >= rows.size() )
        return -1;

    const single_row& header = rows[aRow];
    double scale_factor = processScaleFactor( aRow + 1 );

    if( scale_factor <= 0.0 )
        return -1;

    int refdes_col = getColFromName( aRow, "REFDES" );
    int compclass_col = getColFromName( aRow, "COMPCLASS" );
    int comppartnum_col = getColFromName( aRow, "COMPPARTNUMBER" );
    int compheight_col = getColFromName( aRow, "COMPHEIGHT" );
    int compdevlabelcol = getColFromName( aRow, "COMPDEVICELABEL" );
    int compinscode_col = getColFromName( aRow, "COMPINSERTIONCODE" );
    int symtype_col = getColFromName( aRow, "SYMTYPE" );
    int symname_col = getColFromName( aRow, "SYMNAME" );
    int symmirror_col = getColFromName( aRow, "SYMMIRROR" );
    int symrotate_col = getColFromName( aRow, "SYMROTATE" );
    int symx_col = getColFromName( aRow, "SYMX" );
    int symy_col = getColFromName( aRow, "SYMY" );
    int compvalue_col = getColFromName( aRow, "COMPVALUE" );
    int comptol_col = getColFromName( aRow, "COMPTOL" );
    int compvolt_col = getColFromName( aRow, "COMPVOLTAGE" );

    if( refdes_col < 0 || compclass_col < 0 || comppartnum_col < 0 || compheight_col < 0
            || compdevlabelcol < 0 || compinscode_col < 0 || symtype_col < 0 || symname_col < 0
            || symmirror_col < 0 || symrotate_col < 0 || symx_col < 0 || symy_col < 0
            || compvalue_col < 0 || comptol_col < 0 || compvolt_col < 0 )
        return -1;

    for( ; rownum < rows.size() && rows[rownum].size() > 0 && rows[rownum][0] == "S"; ++rownum )
    {
        const single_row& row = rows[rownum];

        if( row.size() != header.size() )
        {
            wxLogError( _( "Invalid row size in row %zu. Expecting %zu elements but found %zu." ),
                        rownum,
                        header.size(),
                        row.size() );
            continue;
        }

        const wxString& refdes = row[refdes_col];

        if( row[symx_col].empty() || row[symy_col].empty() || row[symrotate_col].empty() )
        {
            wxLogError( _( "Missing X, Y, or rotation data in row %zu for refdes %s. "
                           "This may be an unplaced component." ),
                        rownum, refdes );
            continue;
        }

        auto cmp = std::make_unique<COMPONENT>();

        cmp->refdes = refdes;
        cmp->cclass = parseCompClass( row[compclass_col] );
        cmp->pn = row[comppartnum_col];
        cmp->height = row[compheight_col];
        cmp->dev_label = row[compdevlabelcol];
        cmp->insert_code = row[compinscode_col];
        cmp->type = parseSymType( row[symtype_col] );
        cmp->name = row[symname_col];
        cmp->mirror = ( row[symmirror_col] == "YES" );
        cmp->rotate = readDouble( row[symrotate_col] );
        cmp->x = KiROUND( readDouble( row[symx_col] ) * scale_factor );
        cmp->y = -KiROUND( readDouble( row[symy_col] ) * scale_factor );
        cmp->value = row[compvalue_col];
        cmp->tol = row[comptol_col];
        cmp->voltage = row[compvolt_col];

        auto vec = components.find( cmp->refdes );

        if( vec == components.end() )
        {
            auto retval = components.insert( std::make_pair( cmp->refdes, std::vector<std::unique_ptr<COMPONENT>>{} ) );

            vec = retval.first;
        }

        vec->second.push_back( std::move( cmp ) );
    }

    return rownum - aRow;
}


/**
 * A!SYM_NAME!SYM_MIRROR!PIN_NAME!PIN_NUMBER!PIN_X!PIN_Y!PAD_STACK_NAME!REFDES!PIN_ROTATION!
 * TEST_POINT!
 */
size_t FABMASTER::processPins( size_t aRow )
{
    size_t rownum = aRow + 2;

    if( rownum >= rows.size() )
        return -1;

    const single_row& header = rows[aRow];
    double scale_factor = processScaleFactor( aRow + 1 );

    if( scale_factor <= 0.0 )
        return -1;

    int symname_col = getColFromName( aRow, "SYMNAME" );
    int symmirror_col = getColFromName( aRow, "SYMMIRROR" );
    int pinname_col = getColFromName( aRow, "PINNAME" );
    int pinnum_col = getColFromName( aRow, "PINNUMBER" );
    int pinx_col = getColFromName( aRow, "PINX" );
    int piny_col = getColFromName( aRow, "PINY" );
    int padstack_col = getColFromName( aRow, "PADSTACKNAME" );
    int refdes_col = getColFromName( aRow, "REFDES" );
    int pinrot_col = getColFromName( aRow, "PINROTATION" );
    int testpoint_col = getColFromName( aRow, "TESTPOINT" );

    if( symname_col < 0 ||symmirror_col < 0 || pinname_col < 0 || pinnum_col < 0 || pinx_col < 0
            || piny_col < 0 || padstack_col < 0 || refdes_col < 0 || pinrot_col < 0
            || testpoint_col < 0 )
        return -1;

    for( ; rownum < rows.size() && rows[rownum].size() > 0 && rows[rownum][0] == "S"; ++rownum )
    {
        const single_row& row = rows[rownum];

        if( row.size() != header.size() )
        {
            wxLogError( _( "Invalid row size in row %zu. Expecting %zu elements but found %zu." ),
                        rownum,
                        header.size(),
                        row.size() );
            continue;
        }

        auto pin = std::make_unique<PIN>();

        pin->name = row[symname_col];
        pin->mirror = ( row[symmirror_col] == "YES" );
        pin->pin_name = row[pinname_col];
        pin->pin_number = row[pinnum_col];
        pin->pin_x = KiROUND( readDouble( row[pinx_col] ) * scale_factor );
        pin->pin_y = -KiROUND( readDouble( row[piny_col] ) * scale_factor );
        pin->padstack = row[padstack_col];
        pin->refdes = row[refdes_col];
        pin->rotation = readDouble( row[pinrot_col] );

        auto map_it = pins.find( pin->refdes );

        if( map_it == pins.end() )
        {
            auto retval = pins.insert( std::make_pair( pin->refdes, std::set<std::unique_ptr<PIN>,
                                                       PIN::BY_NUM>{} ) );
            map_it = retval.first;
        }

        map_it->second.insert( std::move( pin ) );
    }

    return rownum - aRow;
}


/**
 * A!NET_NAME!REFDES!PIN_NUMBER!PIN_NAME!PIN_GROUND!PIN_POWER!
 */
size_t FABMASTER::processNets( size_t aRow )
{
    size_t rownum = aRow + 2;

    if( rownum >= rows.size() )
        return -1;

    const single_row& header = rows[aRow];
    double scale_factor = processScaleFactor( aRow + 1 );

    if( scale_factor <= 0.0 )
        return -1;

    int netname_col = getColFromName( aRow, "NETNAME" );
    int refdes_col = getColFromName( aRow, "REFDES" );
    int pinnum_col = getColFromName( aRow, "PINNUMBER" );
    int pinname_col = getColFromName( aRow, "PINNAME" );
    int pingnd_col = getColFromName( aRow, "PINGROUND" );
    int pinpwr_col = getColFromName( aRow, "PINPOWER" );

    if( netname_col < 0 || refdes_col < 0 || pinnum_col < 0 || pinname_col < 0 || pingnd_col < 0
            || pinpwr_col < 0 )
        return -1;

    for( ; rownum < rows.size() && rows[rownum].size() > 0 && rows[rownum][0] == "S"; ++rownum )
    {
        const single_row& row = rows[rownum];

        if( row.size() != header.size() )
        {
            wxLogError( _( "Invalid row size in row %zu. Expecting %zu elements but found %zu." ),
                        rownum,
                        header.size(),
                        row.size() );
            continue;
        }

        NETNAME new_net;
        new_net.name = row[netname_col];
        new_net.refdes = row[refdes_col];
        new_net.pin_num = row[pinnum_col];
        new_net.pin_name = row[pinname_col];
        new_net.pin_gnd = ( row[pingnd_col] == "YES" );
        new_net.pin_pwr = ( row[pinpwr_col] == "YES" );

        pin_nets.emplace( std::make_pair( new_net.refdes, new_net.pin_num ), new_net );
        netnames.insert( row[netname_col] );
    }

    return rownum - aRow;
}


bool FABMASTER::Process()
{

    for( size_t i = 0; i < rows.size(); )
    {
        auto type = detectType( i );

        switch( type )
        {
        case EXTRACT_PADSTACKS:
        {
            /// We extract the basic layers from the padstacks first as this is the only place
            /// the stackup is kept in the basic fabmaster export
            processPadStackLayers( i );
            assignLayers();
            int retval = processPadStacks( i );

            i += std::max( retval, 1 );
            break;
        }

        case EXTRACT_FULL_LAYERS:
        {
            int retval = processLayers( i );

            i += std::max( retval, 1 );
            break;
        }

        case EXTRACT_BASIC_LAYERS:
        {
            int retval = processSimpleLayers( i );

            i += std::max( retval, 1 );
            break;
        }

        case EXTRACT_VIAS:
        {
            int retval = processVias( i );

            i += std::max( retval, 1 );
            break;
        }

        case EXTRACT_TRACES:
        {
            int retval = processTraces( i );

            i += std::max( retval, 1 );
            break;
        }

        case EXTRACT_REFDES:
        {
            int retval = processFootprints( i );

            i += std::max( retval, 1 );
            break;
        }

        case EXTRACT_NETS:
        {
            int retval = processNets( i );

            i += std::max( retval, 1 );
            break;
        }

        case EXTRACT_GRAPHICS:
        {
            int retval = processGeometry( i );

            i += std::max( retval, 1 );
            break;
        }

        case EXTRACT_PINS:
        {
            int retval = processPins( i );

            i += std::max( retval, 1 );
            break;
        }

        case EXTRACT_PAD_SHAPES:
        {
            int retval = processCustomPads( i );

            i += std::max( retval, 1 );
            break;
        }

        default:
            ++i;
            break;
        }

    }

    return true;
}


bool FABMASTER::loadZones( BOARD* aBoard )
{
    for( auto& zone : zones )
    {
        checkpoint();

        if( IsCopperLayer( getLayer( zone->layer ) ) || zone->layer == "ALL" )
        {
            loadZone( aBoard, zone );
        }
        else
        {
            if( zone->layer == "OUTLINE" || zone->layer == "DESIGN_OUTLINE" )
            {
                loadOutline( aBoard, zone );
            }
            else
            {
                loadPolygon( aBoard, zone );
            }
        }
    }

    /**
     * Zones in FABMASTER come in two varieties:
     * - Outlines with no net code attached
     * - Filled areas with net code attached
     *
     * In pcbnew, we want the outline with net code attached.  To determine which
     * outline should have which netcode, we look for overlapping areas.  Each unnetted zone
     * outline will be assigned the netcode that with the most hits on the edge of their
     * outline.
     */
    std::set<ZONE*> zones_to_delete;
    std::set<ZONE*> matched_fills;

    for( auto zone : aBoard->Zones() )
    {
        if( zone->GetNetCode() > 0 )
            zones_to_delete.insert( zone );
    }

    for( auto zone1 : aBoard->Zones() )
    {
        if( zone1->GetNetCode() > 0 )
            continue;

        SHAPE_LINE_CHAIN& outline1 = zone1->Outline()->Outline( 0 );
        std::vector<size_t> overlaps( aBoard->GetNetInfo().GetNetCount() + 1, 0 );
        std::map<int, std::vector<ZONE*>> net_to_fills;

        for( auto zone2 : aBoard->Zones() )
        {
            if( zone2->GetNetCode() <= 0 )
                continue;

            SHAPE_LINE_CHAIN& outline2 = zone2->Outline()->Outline( 0 );

            if( zone1->GetLayer() != zone2->GetLayer() )
                continue;

            if( !outline1.BBox().Intersects( outline2.BBox() ) )
                continue;

            size_t match_count = 0;

            for( auto& pt1 : outline1.CPoints() )
            {
                if( outline2.PointOnEdge( pt1, 1 ) )
                    match_count++;
            }

            for( auto& pt2 : outline2.CPoints() )
            {
                if( outline1.PointOnEdge( pt2, 1 ) )
                    match_count++;
            }

            if( match_count > 0 )
            {
                overlaps[zone2->GetNetCode()] += match_count;
                net_to_fills[zone2->GetNetCode()].push_back( zone2 );
            }
        }

        size_t max_net = 0;
        size_t max_net_id = 0;

        for( size_t el = 1; el < overlaps.size(); ++el )
        {
            if( overlaps[el] > max_net )
            {
                max_net = overlaps[el];
                max_net_id = el;
            }
        }

        if( max_net > 0 )
        {
            zone1->SetNetCode( max_net_id );

            for( ZONE* fill : net_to_fills[max_net_id] )
                matched_fills.insert( fill );
        }
    }

    for( auto zone : zones_to_delete )
    {
        if( matched_fills.find( zone ) != matched_fills.end() )
        {
            aBoard->Remove( zone );
            delete zone;
        }
    }

    return true;
}


void FABMASTER::setupText( const FABMASTER::GRAPHIC_TEXT& aGText, PCB_LAYER_ID aLayer,
                           PCB_TEXT& aText, const BOARD& aBoard, const OPT_VECTOR2I& aMirrorPoint )
{
    aText.SetHorizJustify( aGText.orient );

    aText.SetKeepUpright( false );

    EDA_ANGLE angle = EDA_ANGLE( aGText.rotation );
    angle.Normalize180();

    if( aMirrorPoint.has_value() )
    {
        aText.SetLayer( aBoard.FlipLayer( aLayer ) );
        aText.SetTextPos( VECTOR2I(
                aGText.start_x, 2 * aMirrorPoint->y - ( aGText.start_y - aGText.height / 2 ) ) );
        aText.SetMirrored( !aGText.mirror );

        aText.SetTextAngle( -angle + ANGLE_180 );
    }
    else
    {
        aText.SetLayer( aLayer );
        aText.SetTextPos( VECTOR2I( aGText.start_x, aGText.start_y - aGText.height / 2 ) );
        aText.SetMirrored( aGText.mirror );

        aText.SetTextAngle( angle );
    }

    if( std::abs( angle ) >= ANGLE_90 )
    {
        aText.SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    }

    aText.SetText( aGText.text );
    aText.SetItalic( aGText.ital );
    aText.SetTextThickness( aGText.thickness );
    aText.SetTextHeight( aGText.height );
    aText.SetTextWidth( aGText.width );
}


bool FABMASTER::loadFootprints( BOARD* aBoard )
{
    const NETNAMES_MAP& netinfo = aBoard->GetNetInfo().NetsByName();
    const auto& ds = aBoard->GetDesignSettings();

    for( auto& mod : components )
    {
        checkpoint();

        bool has_multiple = mod.second.size() > 1;

        for( int i = 0; i < mod.second.size(); ++i )
        {
            auto& src = mod.second[i];

            FOOTPRINT* fp = new FOOTPRINT( aBoard );

            wxString mod_ref = src->name;
            wxString lib_ref = m_filename.GetName();

            if( has_multiple )
                mod_ref.Append( wxString::Format( wxT( "_%d" ), i ) );

            ReplaceIllegalFileNameChars( lib_ref, '_' );
            ReplaceIllegalFileNameChars( mod_ref, '_' );

            wxString key = !lib_ref.empty() ? lib_ref + wxT( ":" ) + mod_ref : mod_ref;

            LIB_ID fpID;
            fpID.Parse( key, true );
            fp->SetFPID( fpID );

            fp->SetPosition( VECTOR2I( src->x, src->y ) );
            fp->SetOrientationDegrees( -src->rotate );

            // KiCad netlisting requires parts to have non-digit + digit annotation.
            // If the reference begins with a number, we prepend 'UNK' (unknown) for the source
            // designator
            wxString reference = src->refdes;

            if( !std::isalpha( src->refdes[0] ) )
                reference.Prepend( "UNK" );

            fp->SetReference( reference );

            fp->SetValue( src->value );
            fp->Value().SetLayer( F_Fab );
            fp->Value().SetVisible( false );

            // Set refdes invisible until we find the text for it
            // (otherwise we'll plonk a default-sized ref-des on the silkscreen layer
            // which wasn't there in the imported file)
            fp->Reference().SetVisible( false );

            for( auto& ref : refdes )
            {
                const GRAPHIC_TEXT& lsrc =
                        static_cast<const GRAPHIC_TEXT&>( **ref->segment.begin() );

                if( lsrc.text == src->refdes )
                {
                    PCB_TEXT*    txt = nullptr;
                    PCB_LAYER_ID layer = getLayer( ref->layer );

                    if( !IsPcbLayer( layer ) )
                    {
                        wxLogTrace( traceFabmaster, wxS( "The layer %s is not mapped?" ),
                                                         ref->layer.c_str() );
                        continue;
                    }

                    if( layer == F_SilkS || layer == B_SilkS )
                        txt = &( fp->Reference() );
                    else
                        txt = new PCB_TEXT( fp );

                    OPT_VECTOR2I flip_point = std::nullopt;
                    if( src->mirror )
                        flip_point = VECTOR2I( src->x, src->y );

                    const EDA_ANGLE fp_angle = EDA_ANGLE( lsrc.rotation ).Normalized();
                    txt->SetTextAngle( fp_angle );

                    setupText( lsrc, layer, *txt, *aBoard, flip_point );

                    if( txt != &fp->Reference() )
                        fp->Add( txt, ADD_MODE::APPEND );
                }
            }

            /// Always set the module to the top and flip later if needed
            /// When flipping later, we get the full coordinate transform for free
            fp->SetLayer( F_Cu );

            auto gr_it = comp_graphics.find( src->refdes );

            if( gr_it != comp_graphics.end() )
            {
                for( auto& gr_ref : gr_it->second )
            {
                auto& graphic = gr_ref.second;

                for( auto& seg : *graphic.elements )
                {
                    PCB_LAYER_ID layer = Dwgs_User;

                    if( IsPcbLayer( getLayer( seg->layer ) ) )
                        layer = getLayer( seg->layer );

                    STROKE_PARAMS defaultStroke( ds.GetLineThickness( layer ) );

                    switch( seg->shape )
                    {
                    case GR_SHAPE_LINE:
                    {
                        const GRAPHIC_LINE* lsrc = static_cast<const GRAPHIC_LINE*>( seg.get() );

                        PCB_SHAPE* line = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );

                        if( src->mirror )
                        {
                            line->SetLayer( aBoard->FlipLayer( layer ) );
                            line->SetStart( VECTOR2I( lsrc->start_x, 2 * src->y - lsrc->start_y ) );
                            line->SetEnd( VECTOR2I( lsrc->end_x, 2 * src->y - lsrc->end_y ) );
                        }
                        else
                        {
                            line->SetLayer( layer );
                            line->SetStart( VECTOR2I( lsrc->start_x, lsrc->start_y ) );
                            line->SetEnd( VECTOR2I( lsrc->end_x, lsrc->end_y ) );
                        }

                        line->SetStroke( STROKE_PARAMS( lsrc->width, LINE_STYLE::SOLID ) );

                        if( lsrc->width == 0 )
                            line->SetStroke( defaultStroke );

                        fp->Add( line, ADD_MODE::APPEND );
                        break;
                    }

                    case GR_SHAPE_CIRCLE:
                    {
                        const GRAPHIC_ARC& lsrc = static_cast<const GRAPHIC_ARC&>( *seg );

                        PCB_SHAPE* circle = new PCB_SHAPE( fp, SHAPE_T::CIRCLE );

                        circle->SetLayer( layer );
                        circle->SetCenter( VECTOR2I( lsrc.center_x, lsrc.center_y ) );
                        circle->SetEnd( VECTOR2I( lsrc.end_x, lsrc.end_y ) );
                        circle->SetWidth( lsrc.width );

                        if( IsBackLayer( layer ) )
                        {
                            // Circles seem to have a flip around the FP origin that lines don't have
                            const VECTOR2I fp_orig = fp->GetPosition();
                            circle->Mirror( fp_orig, FLIP_DIRECTION::TOP_BOTTOM );
                        }

                        if( lsrc.width == 0 )
                        {
                            // It seems that 0-width circles on DISPLAY_T/B layers are filled
                            // (but not, say, SILKSCREEN_T/B).
                            // There is an oblique reference to something like this here:
                            // https://github.com/plusea/EAGLE/blob/master/ulp/fabmaster.ulp
                            if( lsrc.layer == "DISPLAY_TOP" || lsrc.layer == "DISPLAY_BOTTOM" )
                                circle->SetFilled( true );
                            else
                                circle->SetWidth( ds.GetLineThickness( circle->GetLayer() ) );
                        }

                        if( src->mirror )
                            circle->Flip( circle->GetCenter(), FLIP_DIRECTION::TOP_BOTTOM );

                        fp->Add( circle, ADD_MODE::APPEND );
                        break;
                    }

                    case GR_SHAPE_ARC:
                    {
                        const GRAPHIC_ARC* lsrc = static_cast<const GRAPHIC_ARC*>( seg.get() );

                        std::unique_ptr<PCB_SHAPE> arc =
                                std::make_unique<PCB_SHAPE>( fp, SHAPE_T::ARC );

                        SHAPE_ARC sarc = lsrc->result;

                        if( IsBackLayer( layer ) )
                        {
                            // Arcs seem to have a vertical flip around the FP origin that lines don't have
                            // and are also flipped around their center (this is a best guess at the transformation)
                            const VECTOR2I fp_orig = fp->GetPosition();
                            sarc.Mirror( fp_orig, FLIP_DIRECTION::TOP_BOTTOM );
                            sarc.Mirror( sarc.GetCenter(), FLIP_DIRECTION::TOP_BOTTOM );
                        }

                        arc->SetLayer( layer );
                        arc->SetArcGeometry( sarc.GetP0(), sarc.GetArcMid(), sarc.GetP1() );
                        arc->SetStroke( STROKE_PARAMS( lsrc->width, LINE_STYLE::SOLID ) );

                        if( lsrc->width == 0 )
                            arc->SetStroke( defaultStroke );

                        if( src->mirror )
                            arc->Flip( arc->GetCenter(), FLIP_DIRECTION::TOP_BOTTOM );

                        fp->Add( arc.release(), ADD_MODE::APPEND );
                        break;
                    }

                    case GR_SHAPE_RECTANGLE:
                    {
                        const GRAPHIC_RECTANGLE *lsrc =
                                static_cast<const GRAPHIC_RECTANGLE*>( seg.get() );

                        PCB_SHAPE* rect = new PCB_SHAPE( fp, SHAPE_T::RECTANGLE );

                        if( src->mirror )
                        {
                            rect->SetLayer( aBoard->FlipLayer( layer ) );
                            rect->SetStart( VECTOR2I( lsrc->start_x, 2 * src->y - lsrc->start_y ) );
                            rect->SetEnd( VECTOR2I( lsrc->end_x, 2 * src->y - lsrc->end_y ) );
                        }
                        else
                        {
                            rect->SetLayer( layer );
                            rect->SetStart( VECTOR2I( lsrc->start_x, lsrc->start_y ) );
                            rect->SetEnd( VECTOR2I( lsrc->end_x, lsrc->end_y ) );
                        }

                        rect->SetStroke( defaultStroke );

                        fp->Add( rect, ADD_MODE::APPEND );
                        break;
                    }

                    case GR_SHAPE_TEXT:
                    {
                        const GRAPHIC_TEXT& lsrc = static_cast<const GRAPHIC_TEXT&>( *seg );

                        std::unique_ptr<PCB_TEXT> txt = std::make_unique<PCB_TEXT>( fp );

                        OPT_VECTOR2I flip_point;

                        if( src->mirror )
                            flip_point = VECTOR2I( src->x, src->y );

                        setupText( lsrc, layer, *txt, *aBoard, flip_point );

                        // FABMASTER doesn't have visibility flags but layers that are not silk
                        // should be hidden by default to prevent clutter.
                        if( txt->GetLayer() != F_SilkS && txt->GetLayer() != B_SilkS )
                        {
                            PCB_FIELD* field = new PCB_FIELD( *txt, FIELD_T::USER );
                            field->SetVisible( false );
                            fp->Add( field, ADD_MODE::APPEND );
                        }
                        else
                        {
                            fp->Add( txt.release(), ADD_MODE::APPEND );
                        }

                        break;
                    }

                    default:
                        continue;
                    }
                }
            }
            }

            auto pin_it = pins.find( src->refdes );

            if( pin_it != pins.end() )
            {
                for( auto& pin : pin_it->second )
                {
                    auto pin_net_it = pin_nets.find( std::make_pair( pin->refdes,
                                                                     pin->pin_number ) );
                    auto padstack = pads.find( pin->padstack );
                    std::string netname = "";

                    if( pin_net_it != pin_nets.end() )
                        netname = pin_net_it->second.name;

                    auto net_it = netinfo.find( netname );

                    std::unique_ptr<PAD> newpad = std::make_unique<PAD>( fp );

                    if( net_it != netinfo.end() )
                        newpad->SetNet( net_it->second );
                    else
                        newpad->SetNetCode( 0 );

                    newpad->SetX( pin->pin_x );

                    if( src->mirror )
                        newpad->SetY( 2 * src->y - pin->pin_y );
                    else
                        newpad->SetY( pin->pin_y );

                    newpad->SetNumber( pin->pin_number );

                    if( padstack == pads.end() )
                    {
                        wxLogError( _( "Unable to locate padstack %s in file %s\n" ),
                                      pin->padstack.c_str(), aBoard->GetFileName().wc_str() );
                        continue;
                    }
                    else
                    {
                        auto& pad = padstack->second;

                        newpad->SetShape( PADSTACK::ALL_LAYERS, pad.shape );

                        if( pad.shape == PAD_SHAPE::CUSTOM )
                        {
                            // Choose the smaller dimension to ensure the base pad
                            // is fully hidden by the custom pad
                            int pad_size = std::min( pad.width, pad.height );

                            newpad->SetSize( PADSTACK::ALL_LAYERS,
                                             VECTOR2I( pad_size / 2, pad_size / 2 ) );

                            std::string custom_name = pad.custom_name + "_" + pin->refdes + "_" +
                                                      pin->pin_number;
                            auto custom_it = pad_shapes.find( custom_name );

                            if( custom_it != pad_shapes.end() )
                            {

                                SHAPE_POLY_SET poly_outline;
                                int last_subseq = 0;
                                int hole_idx = -1;

                                poly_outline.NewOutline();

                                // Custom pad shapes have a group of elements
                                // that are a list of graphical polygons
                                for( const auto& el : (*custom_it).second.elements )
                                {
                                    // For now, we are only processing the custom pad for the
                                    // top layer
                                    // TODO: Use full padstacks when implementing in KiCad
                                    PCB_LAYER_ID primary_layer = src->mirror ? B_Cu : F_Cu;

                                    if( getLayer( ( *( el.second.begin() ) )->layer ) != primary_layer )
                                        continue;

                                    for( const auto& seg : el.second )
                                    {
                                        if( seg->subseq > 0 || seg->subseq != last_subseq )
                                        {
                                            poly_outline.Polygon(0).back().SetClosed( true );
                                            hole_idx = poly_outline.AddHole( SHAPE_LINE_CHAIN{} );
                                        }

                                        if( seg->shape == GR_SHAPE_LINE )
                                        {
                                            const GRAPHIC_LINE* src = static_cast<const GRAPHIC_LINE*>( seg.get() );

                                            if( poly_outline.VertexCount( 0, hole_idx ) == 0 )
                                                poly_outline.Append( src->start_x, src->start_y,
                                                                     0, hole_idx );

                                            poly_outline.Append( src->end_x, src->end_y, 0,
                                                                 hole_idx );
                                        }
                                        else if( seg->shape == GR_SHAPE_ARC )
                                        {
                                            const GRAPHIC_ARC* src = static_cast<const GRAPHIC_ARC*>( seg.get() );
                                            SHAPE_LINE_CHAIN&  chain = poly_outline.Hole( 0, hole_idx );

                                            chain.Append( src->result );
                                        }
                                    }
                                }

                                if( poly_outline.OutlineCount() < 1
                                        || poly_outline.Outline( 0 ).PointCount() < 3 )
                                {
                                    wxLogError( _( "Invalid custom pad '%s'. Replacing with "
                                                   "circular pad." ),
                                                custom_name.c_str() );
                                    newpad->SetShape( F_Cu, PAD_SHAPE::CIRCLE );
                                }
                                else
                                {
                                    poly_outline.Fracture();

                                    poly_outline.Move( -newpad->GetPosition() );

                                    if( src->mirror )
                                    {
                                        poly_outline.Mirror( VECTOR2I( 0, ( pin->pin_y - src->y ) ),
                                                             FLIP_DIRECTION::TOP_BOTTOM );
                                        poly_outline.Rotate( EDA_ANGLE( src->rotate - pin->rotation,
                                                                        DEGREES_T ) );
                                    }
                                    else
                                    {
                                        poly_outline.Rotate( EDA_ANGLE( -src->rotate + pin->rotation,
                                                                        DEGREES_T ) );
                                    }

                                    newpad->AddPrimitivePoly( PADSTACK::ALL_LAYERS, poly_outline, 0, true );
                                }

                                SHAPE_POLY_SET mergedPolygon;
                                newpad->MergePrimitivesAsPolygon( PADSTACK::ALL_LAYERS, &mergedPolygon );

                                if( mergedPolygon.OutlineCount() > 1 )
                                {
                                    wxLogError( _( "Invalid custom pad '%s'. Replacing with "
                                                   "circular pad." ),
                                                custom_name.c_str() );
                                    newpad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
                                }
                            }
                            else
                            {
                                wxLogError( _( "Could not find custom pad '%s'." ),
                                            custom_name.c_str() );
                            }
                        }
                        else
                        {
                            newpad->SetSize( PADSTACK::ALL_LAYERS,
                                             VECTOR2I( pad.width, pad.height ) );
                        }

                        if( pad.drill )
                        {
                            if( pad.plated )
                            {
                                newpad->SetAttribute( PAD_ATTRIB::PTH );
                                newpad->SetLayerSet( PAD::PTHMask() );
                            }
                            else
                            {
                                newpad->SetAttribute( PAD_ATTRIB::NPTH );
                                newpad->SetLayerSet( PAD::UnplatedHoleMask() );
                            }

                            if( pad.drill_size_x == pad.drill_size_y )
                                newpad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
                            else
                                newpad->SetDrillShape( PAD_DRILL_SHAPE::OBLONG );

                            newpad->SetDrillSize( VECTOR2I( pad.drill_size_x, pad.drill_size_y ) );
                        }
                        else
                        {
                            newpad->SetAttribute( PAD_ATTRIB::SMD );

                            if( pad.top )
                                newpad->SetLayerSet( PAD::SMDMask() );
                            else if( pad.bottom )
                                newpad->SetLayerSet( PAD::SMDMask().FlipStandardLayers() );
                        }
                    }

                    if( src->mirror )
                        newpad->SetOrientation( EDA_ANGLE( -src->rotate + pin->rotation,
                                                           DEGREES_T ) );
                    else
                        newpad->SetOrientation( EDA_ANGLE( src->rotate - pin->rotation,
                                                           DEGREES_T ) );

                    if( newpad->GetSizeX() > 0 || newpad->GetSizeY() > 0 )
                    {
                        fp->Add( newpad.release(), ADD_MODE::APPEND );
                    }
                    else
                    {
                        wxLogError( _( "Invalid zero-sized pad ignored in\nfile: %s" ),
                                    aBoard->GetFileName().wc_str() );
                    }
                }
            }

            if( src->mirror )
            {
                fp->SetOrientationDegrees( 180.0 - src->rotate );
                fp->Flip( fp->GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );
            }

            aBoard->Add( fp, ADD_MODE::APPEND );
        }
    }

    return true;
}


bool FABMASTER::loadLayers( BOARD* aBoard )
{
    LSET layer_set;

    /// The basic layers that get enabled for normal boards
    layer_set |= LSET::AllTechMask() | LSET::UserMask();

    for( auto& layer : layers )
    {
        checkpoint();

        if( layer.second.layerid >= PCBNEW_LAYER_ID_START )
            layer_set.set( layer.second.layerid );
    }

    aBoard->SetEnabledLayers( layer_set );

    for( auto& layer : layers )
    {
        if( layer.second.conductive )
        {
            aBoard->SetLayerName( static_cast<PCB_LAYER_ID>( layer.second.layerid ),
                    layer.second.name );
        }
    }

    return true;
}


bool FABMASTER::loadVias( BOARD* aBoard )
{
    const NETNAMES_MAP& netinfo = aBoard->GetNetInfo().NetsByName();
    const auto& ds = aBoard->GetDesignSettings();

    for( auto& via : vias )
    {
        checkpoint();

        auto net_it = netinfo.find( via->net );
        auto padstack = pads.find( via->padstack );

        PCB_VIA* new_via = new PCB_VIA( aBoard );

        new_via->SetPosition( VECTOR2I( via->x, via->y ) );

        if( net_it != netinfo.end() )
            new_via->SetNet( net_it->second );

        if( padstack == pads.end() )
        {
            new_via->SetDrillDefault();

            if( !ds.m_ViasDimensionsList.empty() )
            {
                new_via->SetWidth( PADSTACK::ALL_LAYERS, ds.m_ViasDimensionsList[0].m_Diameter );
                new_via->SetDrill( ds.m_ViasDimensionsList[0].m_Drill );
            }
            else
            {
                new_via->SetDrillDefault();
                new_via->SetWidth( PADSTACK::ALL_LAYERS, ds.m_ViasMinSize );
            }
        }
        else
        {
            new_via->SetDrill( padstack->second.drill_size_x );
            new_via->SetWidth( PADSTACK::ALL_LAYERS, padstack->second.width );
        }

        aBoard->Add( new_via, ADD_MODE::APPEND );
    }

    return true;
}


bool FABMASTER::loadNets( BOARD* aBoard )
{
    for( auto& net : netnames )
    {
        checkpoint();

        NETINFO_ITEM *newnet = new NETINFO_ITEM( aBoard, net );
        aBoard->Add( newnet, ADD_MODE::APPEND );
    }

    return true;
}


bool FABMASTER::loadEtch( BOARD* aBoard, const std::unique_ptr<FABMASTER::TRACE>& aLine )
{
    const NETNAMES_MAP& netinfo = aBoard->GetNetInfo().NetsByName();
    auto net_it = netinfo.find( aLine->netname );

    int  last_subseq = 0;
    ZONE* new_zone = nullptr;

    for( const auto& seg : aLine->segment )
    {
        PCB_LAYER_ID layer = getLayer( seg->layer );

        if( IsCopperLayer( layer ) )
        {
            switch( seg->shape )
            {
            case GR_SHAPE_LINE:
            {
                const GRAPHIC_LINE* src = static_cast<const GRAPHIC_LINE*>( seg.get() );

                PCB_TRACK* trk = new PCB_TRACK( aBoard );

                trk->SetLayer( layer );
                trk->SetStart( VECTOR2I( src->start_x, src->start_y ) );
                trk->SetEnd( VECTOR2I( src->end_x, src->end_y ) );
                trk->SetWidth( src->width );

                if( net_it != netinfo.end() )
                    trk->SetNet( net_it->second );

                aBoard->Add( trk, ADD_MODE::APPEND );
                break;
            }

            case GR_SHAPE_ARC:
            {
                const GRAPHIC_ARC* src = static_cast<const GRAPHIC_ARC*>( seg.get() );

                PCB_ARC* trk = new PCB_ARC( aBoard, &src->result );
                trk->SetLayer( layer );
                trk->SetWidth( src->width );

                if( net_it != netinfo.end() )
                    trk->SetNet( net_it->second );

                aBoard->Add( trk, ADD_MODE::APPEND );
                break;
            }

            default:
                // Defer to the generic graphics factory
                for( std::unique_ptr<BOARD_ITEM>& new_item : createBoardItems( *aBoard, layer, *seg ) )
                    aBoard->Add( new_item.release(), ADD_MODE::APPEND );

                break;
            }
        }
        else
        {
            wxLogError( _( "Expecting etch data to be on copper layer. Row found on layer '%s'" ),
                        seg->layer.c_str() );
        }
    }

    return true;
}


SHAPE_POLY_SET FABMASTER::loadShapePolySet( const graphic_element& aElement )
{
    SHAPE_POLY_SET poly_outline;
    int last_subseq = 0;
    int hole_idx = -1;

    poly_outline.NewOutline();

    for( const auto& seg : aElement )
    {
        if( seg->subseq > 0 || seg->subseq != last_subseq )
            hole_idx = poly_outline.AddHole( SHAPE_LINE_CHAIN{} );

        if( seg->shape == GR_SHAPE_LINE )
        {
            const GRAPHIC_LINE* src = static_cast<const GRAPHIC_LINE*>( seg.get() );

            if( poly_outline.VertexCount( 0, hole_idx ) == 0 )
                poly_outline.Append( src->start_x, src->start_y, 0, hole_idx );

            poly_outline.Append( src->end_x, src->end_y, 0, hole_idx );
        }
        else if( seg->shape == GR_SHAPE_ARC || seg->shape == GR_SHAPE_CIRCLE )
        {
            const GRAPHIC_ARC* src = static_cast<const GRAPHIC_ARC*>( seg.get() );
            SHAPE_LINE_CHAIN&  chain = poly_outline.Hole( 0, hole_idx );

            chain.Append( src->result );
        }
    }

    return poly_outline;
}


/*
 * The format doesn't seem to distinguish between open and closed polygons.
 * So the best we can really do is to try to detect an open polyline by looking
 * for a closed subsequence 0.
 *
 * For example three lines like this will be open:
 *
 *  +----
 *  |
 *  +----
 *
 * But four lines will be closed:
 *
 *  +----+
 *  |    |
 *  +----+
 *
 * This means that "closed" zones (which can have fill patterns in Allegro)
 * and "a bunch of lines, which happen to be closed) are not distinguishable,
 * but that just seems to be information thrown away on export to FABMASTER.
 */
bool FABMASTER::traceIsOpen( const FABMASTER::TRACE& aLine )
{
    if( aLine.segment.size() == 0 )
        return true;

    // First and last item in the first subsequence
    const GRAPHIC_ITEM* first = nullptr;
    const GRAPHIC_ITEM* last = nullptr;
    int                 first_subseq = -1;
    bool                have_multiple_subseqs = false;

    for( const std::unique_ptr<GRAPHIC_ITEM>& gr_item : aLine.segment )
    {
        if( first == nullptr )
        {
            first = gr_item.get();
            first_subseq = gr_item->subseq;
        }
        else if( gr_item->subseq == first_subseq )
        {
            last = gr_item.get();
        }
        else
        {
            have_multiple_subseqs = true;
            break;
        }
    }

    // Should have at least one item
    wxCHECK( first, true );

    // First subsequence was only one item
    if( !last )
    {
        // It can still be a closed polygon if the outer border is a circle
        // and there are inner shapes.
        if( first->shape == GR_SHAPE_CIRCLE && have_multiple_subseqs )
            return false;

        return true;
    }

    const VECTOR2I start{ first->start_x, first->start_y };

    // It's not always possible to find an end
    OPT_VECTOR2I end;

    switch( last->shape )
    {
    case GR_SHAPE_LINE:
    {
        const GRAPHIC_LINE& line = static_cast<const GRAPHIC_LINE&>( *last );
        end = VECTOR2I{ line.end_x, line.end_y };
        break;
    }

    case GR_SHAPE_ARC:
    {
        const GRAPHIC_ARC& arc = static_cast<const GRAPHIC_ARC&>( *last );
        end = VECTOR2I{ arc.end_x, arc.end_y };
        break;
    }

    default:
        // These shapes don't have "ends" that make sense for a polyline
        break;
    }

    // This looks like a closed polygon
    if( end.has_value() && start == end )
        return false;

    // Open polyline
    return true;
}


std::vector<std::unique_ptr<BOARD_ITEM>>
FABMASTER::createBoardItems( BOARD& aBoard, PCB_LAYER_ID aLayer, FABMASTER::GRAPHIC_ITEM& aGraphic )
{
    std::vector<std::unique_ptr<BOARD_ITEM>> new_items;

    const BOARD_DESIGN_SETTINGS& boardSettings = aBoard.GetDesignSettings();
    const STROKE_PARAMS          defaultStroke( boardSettings.GetLineThickness( aLayer ) );

    const auto setShapeParameters = [&]( PCB_SHAPE& aShape )
    {
        aShape.SetStroke( STROKE_PARAMS( aGraphic.width, LINE_STYLE::SOLID ) );

        if( aShape.GetWidth() == 0 )
            aShape.SetStroke( defaultStroke );
    };

    switch( aGraphic.shape )
    {
    case GR_SHAPE_TEXT:
    {
        const GRAPHIC_TEXT& src = static_cast<const GRAPHIC_TEXT&>( aGraphic );

        auto new_text = std::make_unique<PCB_TEXT>( &aBoard );

        if( IsBackLayer( aLayer ) )
        {
            new_text->SetMirrored( true );
        }

        setupText( src, aLayer, *new_text, aBoard, std::nullopt );

        new_items.emplace_back( std::move( new_text ) );
        break;
    }

    case GR_SHAPE_CROSS:
    {
        const GRAPHIC_CROSS& src = static_cast<const GRAPHIC_CROSS&>( aGraphic );

        const VECTOR2I c{ src.start_x, src.start_y };
        const VECTOR2I s{ src.size_x, src.size_y };

        const std::vector<SEG> segs = KIGEOM::MakeCrossSegments( c, s, ANGLE_0 );

        for( const SEG& seg : segs )
        {
            auto line = std::make_unique<PCB_SHAPE>( &aBoard );
            line->SetShape( SHAPE_T::SEGMENT );
            line->SetStart( seg.A );
            line->SetEnd( seg.B );

            setShapeParameters( *line );
            new_items.emplace_back( std::move( line ) );
        }
        break;
    }

    default:
    {
        // Simple single shape
        auto new_shape = std::make_unique<PCB_SHAPE>( &aBoard );

        setShapeParameters( *new_shape );

        switch( aGraphic.shape )
        {
        case GR_SHAPE_LINE:
        {
            const GRAPHIC_LINE& src = static_cast<const GRAPHIC_LINE&>( aGraphic );

            new_shape->SetShape( SHAPE_T::SEGMENT );
            new_shape->SetStart( VECTOR2I( src.start_x, src.start_y ) );
            new_shape->SetEnd( VECTOR2I( src.end_x, src.end_y ) );

            break;
        }

        case GR_SHAPE_ARC:
        {
            const GRAPHIC_ARC& src = static_cast<const GRAPHIC_ARC&>( aGraphic );

            new_shape->SetShape( SHAPE_T::ARC );
            new_shape->SetArcGeometry( src.result.GetP0(), src.result.GetArcMid(),
                                       src.result.GetP1() );
            break;
        }

        case GR_SHAPE_CIRCLE:
        {
            const GRAPHIC_ARC& src = static_cast<const GRAPHIC_ARC&>( aGraphic );

            new_shape->SetShape( SHAPE_T::CIRCLE );
            new_shape->SetCenter( VECTOR2I( src.center_x, src.center_y ) );
            new_shape->SetRadius( src.radius );
            break;
        }

        case GR_SHAPE_RECTANGLE:
        {
            const GRAPHIC_RECTANGLE& src = static_cast<const GRAPHIC_RECTANGLE&>( aGraphic );

            new_shape->SetShape( SHAPE_T::RECTANGLE );
            new_shape->SetStart( VECTOR2I( src.start_x, src.start_y ) );
            new_shape->SetEnd( VECTOR2I( src.end_x, src.end_y ) );

            new_shape->SetFilled( src.fill );
            break;
        }

        case GR_SHAPE_POLYGON:
        {
            const GRAPHIC_POLYGON& src = static_cast<const GRAPHIC_POLYGON&>( aGraphic );
            new_shape->SetShape( SHAPE_T::POLY );
            new_shape->SetPolyPoints( src.m_pts );
            break;
        }

        case GR_SHAPE_OBLONG:
        {
            // Create as a polygon, but we could also make a group of two lines and two arcs
            const GRAPHIC_OBLONG& src = static_cast<const GRAPHIC_OBLONG&>( aGraphic );

            const VECTOR2I c{ src.start_x, src.start_y };
            VECTOR2I       s = c;
            int            w = 0;

            if( src.oblong_x )
            {
                w = src.size_y;
                s -= VECTOR2I{ ( src.size_x - w ) / 2, 0 };
            }
            else
            {
                w = src.size_x;
                s -= VECTOR2I{ 0, ( src.size_y - w ) / 2 };
            }

            SHAPE_SEGMENT seg( s, c - ( s - c ), w );

            SHAPE_POLY_SET poly;
            seg.TransformToPolygon( poly, boardSettings.m_MaxError, ERROR_LOC::ERROR_INSIDE );

            new_shape->SetShape( SHAPE_T::POLY );
            new_shape->SetPolyShape( poly );
            break;
        }

        default:
            wxLogError( _( "Unhandled shape type %d in polygon on layer %s, seq %d %d" ),
                        aGraphic.shape, aGraphic.layer, aGraphic.seq, aGraphic.subseq );
        }

        new_items.emplace_back( std::move( new_shape ) );
    }
    }

    for( std::unique_ptr<BOARD_ITEM>& new_item : new_items )
    {
        new_item->SetLayer( aLayer );
    }

    // If there's more than one, group them
    if( new_items.size() > 1 )
    {
        auto new_group = std::make_unique<PCB_GROUP>( &aBoard );
        for( std::unique_ptr<BOARD_ITEM>& new_item : new_items )
        {
            new_group->AddItem( new_item.get() );
        }
        new_items.emplace_back( std::move( new_group ) );
    }

    return new_items;
}


bool FABMASTER::loadPolygon( BOARD* aBoard, const std::unique_ptr<FABMASTER::TRACE>& aLine )
{
    if( aLine->segment.empty() )
        return false;

    PCB_LAYER_ID layer = Cmts_User;

    const PCB_LAYER_ID new_layer = getLayer( aLine->layer );

    if( IsPcbLayer( new_layer ) )
        layer = new_layer;

    const bool is_open = traceIsOpen( *aLine );

    if( is_open )
    {
        for( const auto& seg : aLine->segment )
        {
            for( std::unique_ptr<BOARD_ITEM>& new_item : createBoardItems( *aBoard, layer, *seg ) )
            {
                aBoard->Add( new_item.release(), ADD_MODE::APPEND );
            }
        }
    }
    else
    {
        STROKE_PARAMS defaultStroke( aBoard->GetDesignSettings().GetLineThickness( layer ) );

        SHAPE_POLY_SET poly_outline = loadShapePolySet( aLine->segment );

        poly_outline.Fracture();

        if( poly_outline.OutlineCount() < 1 || poly_outline.COutline( 0 ).PointCount() < 3 )
            return false;

        PCB_SHAPE* new_poly = new PCB_SHAPE( aBoard );

        new_poly->SetShape( SHAPE_T::POLY );
        new_poly->SetLayer( layer );

        // Polygons on the silk layer are filled but other layers are not/fill doesn't make sense
        if( layer == F_SilkS || layer == B_SilkS )
        {
            new_poly->SetFilled( true );
            new_poly->SetStroke( STROKE_PARAMS( 0 ) );
        }
        else
        {
            new_poly->SetStroke(
                    STROKE_PARAMS( ( *( aLine->segment.begin() ) )->width, LINE_STYLE::SOLID ) );

            if( new_poly->GetWidth() == 0 )
                new_poly->SetStroke( defaultStroke );
        }

        new_poly->SetPolyShape( poly_outline );
        aBoard->Add( new_poly, ADD_MODE::APPEND );
    }

    return true;
}


bool FABMASTER::loadZone( BOARD* aBoard, const std::unique_ptr<FABMASTER::TRACE>& aLine )
{
    if( aLine->segment.size() < 3 )
        return false;

    SHAPE_POLY_SET* zone_outline = nullptr;
    ZONE* zone = nullptr;

    const NETNAMES_MAP& netinfo = aBoard->GetNetInfo().NetsByName();
    auto net_it = netinfo.find( aLine->netname );
    PCB_LAYER_ID layer = Cmts_User;
    auto new_layer = getLayer( aLine->layer );

    if( IsPcbLayer( new_layer ) )
        layer = new_layer;

    zone = new ZONE( aBoard );
    zone_outline = new SHAPE_POLY_SET;

    if( net_it != netinfo.end() )
        zone->SetNet( net_it->second );

    if( aLine->layer == "ALL" )
        zone->SetLayerSet( aBoard->GetLayerSet() & LSET::AllCuMask() );
    else
        zone->SetLayer( layer );

    zone->SetIsRuleArea( false );
    zone->SetDoNotAllowTracks( false );
    zone->SetDoNotAllowVias( false );
    zone->SetDoNotAllowPads( false );
    zone->SetDoNotAllowFootprints( false );
    zone->SetDoNotAllowZoneFills( false );

    if( aLine->lclass == "ROUTE KEEPOUT")
    {
        zone->SetIsRuleArea( true );
        zone->SetDoNotAllowTracks( true );
    }
    else if( aLine->lclass == "VIA KEEPOUT")
    {
        zone->SetIsRuleArea( true );
        zone->SetDoNotAllowVias( true );
    }
    else
    {
        zone->SetAssignedPriority( 50 );
    }

    zone->SetLocalClearance( 0 );
    zone->SetPadConnection( ZONE_CONNECTION::FULL );

    zone_outline->NewOutline();

    std::unique_ptr<SHAPE_LINE_CHAIN> pending_hole = nullptr;
    SHAPE_LINE_CHAIN*                 active_chain = &zone_outline->Outline( 0 );

    const auto add_hole_if_valid = [&]()
    {
        if( pending_hole )
        {
            pending_hole->SetClosed( true );

            // If we get junk holes, assert, but don't add them to the zone, as that
            // will cause crashes later.
            if( !KIGEOM::AddHoleIfValid( *zone_outline, std::move( *pending_hole ) ) )
            {
                wxLogMessage( _( "Invalid hole with %d points in zone on layer %s with net %s" ),
                              pending_hole->PointCount(), zone->GetLayerName(),
                              zone->GetNetname() );
            }

            pending_hole.reset();
        }
    };

    int last_subseq = 0;
    for( const auto& seg : aLine->segment )
    {
        if( seg->subseq > 0 && seg->subseq != last_subseq )
        {
            // Don't knock holes in the BOUNDARY systems.  These are the outer layers for
            // zone fills.
            if( aLine->lclass == "BOUNDARY" )
                break;

            add_hole_if_valid();
            pending_hole = std::make_unique<SHAPE_LINE_CHAIN>();
            active_chain = pending_hole.get();
            last_subseq = seg->subseq;
        }

        if( seg->shape == GR_SHAPE_LINE )
        {
            const GRAPHIC_LINE* src = static_cast<const GRAPHIC_LINE*>( seg.get() );
            const VECTOR2I      start( src->start_x, src->start_y );
            const VECTOR2I      end( src->end_x, src->end_y );

            if( active_chain->PointCount() == 0 )
            {
                active_chain->Append( start );
            }
            else
            {
                const VECTOR2I& last = active_chain->CLastPoint();

                // Not if this can ever happen, or what do if it does (add both points?).
                if( last != start )
                {
                    wxLogError( _( "Outline seems discontinuous: last point was %s, "
                                   "start point of next segment is %s" ),
                                last.Format(), start.Format() );
                }
            }

            active_chain->Append( end );
        }
        else if( seg->shape == GR_SHAPE_ARC || seg->shape == GR_SHAPE_CIRCLE )
        {
            /* Even if it says "circle", it's actually an arc, it's just closed */
            const GRAPHIC_ARC* src = static_cast<const GRAPHIC_ARC*>( seg.get() );
            active_chain->Append( src->result );
        }
        else
        {
            wxLogError( _( "Invalid shape type %d in zone outline" ), seg->shape );
        }
    }

    // Finalise the last hole, if any
    add_hole_if_valid();

    if( zone_outline->Outline( 0 ).PointCount() >= 3 )
    {
        zone->SetOutline( zone_outline );
        aBoard->Add( zone, ADD_MODE::APPEND );
    }
    else
    {
        delete( zone_outline );
        delete( zone );
    }

    return true;
}


bool FABMASTER::loadOutline( BOARD* aBoard, const std::unique_ptr<FABMASTER::TRACE>& aLine )
{
    PCB_LAYER_ID layer;

    if( aLine->lclass == "BOARD GEOMETRY" && aLine->layer != "DIMENSION" )
        layer = Edge_Cuts;
    else if( aLine->lclass == "DRAWING FORMAT" )
        layer = Dwgs_User;
    else
        layer = Cmts_User;

    for( auto& seg : aLine->segment )
    {
        for( std::unique_ptr<BOARD_ITEM>& new_item : createBoardItems( *aBoard, layer, *seg ) )
        {
            aBoard->Add( new_item.release(), ADD_MODE::APPEND );
        }
    }

    return true;
}


bool FABMASTER::loadGraphics( BOARD* aBoard )
{

    for( auto& geom : board_graphics )
    {
        checkpoint();

        PCB_LAYER_ID layer;

        // The pin numbers are not useful for us outside of the footprints
        if( geom.subclass == "PIN_NUMBER" )
            continue;

        layer = getLayer( geom.subclass );

        if( !IsPcbLayer( layer ) )
            layer = Cmts_User;

        if( !geom.elements->empty() )
        {
            /// Zero-width segments/arcs are polygon outlines
            if( ( *( geom.elements->begin() ) )->width == 0 )
            {
                SHAPE_POLY_SET poly_outline = loadShapePolySet( *( geom.elements ) );

                poly_outline.Fracture();

                if( poly_outline.OutlineCount() < 1 || poly_outline.COutline( 0 ).PointCount() < 3 )
                    continue;

                PCB_SHAPE* new_poly = new PCB_SHAPE( aBoard, SHAPE_T::POLY );
                new_poly->SetLayer( layer );
                new_poly->SetPolyShape( poly_outline );
                new_poly->SetStroke( STROKE_PARAMS( 0 ) );

                if( layer == F_SilkS || layer == B_SilkS )
                    new_poly->SetFilled( true );

                aBoard->Add( new_poly, ADD_MODE::APPEND );
            }
        }

        for( auto& seg : *geom.elements )
        {
            for( std::unique_ptr<BOARD_ITEM>& new_item : createBoardItems( *aBoard, layer, *seg ) )
            {
                aBoard->Add( new_item.release(), ADD_MODE::APPEND );
            }
        }
    }

    return true;

}


bool FABMASTER::orderZones( BOARD* aBoard )
{
    std::vector<ZONE*> sortedZones;
    std::copy( aBoard->Zones().begin(), aBoard->Zones().end(), std::back_inserter( sortedZones ) );
    std::sort( sortedZones.begin(), sortedZones.end(),
            [&]( const ZONE* a, const ZONE* b )
            {
                if( a->GetLayer() == b->GetLayer() )
                    return a->GetBoundingBox().GetArea() > b->GetBoundingBox().GetArea();

                return a->GetLayer() < b->GetLayer();
            } );

    PCB_LAYER_ID layer = UNDEFINED_LAYER;
    unsigned int priority = 0;

    for( ZONE* zone : sortedZones )
    {
        /// Rule areas do not have priorities
        if( zone->GetIsRuleArea() )
            continue;

        if( zone->GetLayer() != layer )
        {
            layer = zone->GetLayer();
            priority = 0;
        }

        zone->SetAssignedPriority( priority );
        priority += 10;
    }

    return true;
}


bool FABMASTER::LoadBoard( BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter )
{
    aBoard->SetFileName( m_filename.GetFullPath() );
    m_progressReporter = aProgressReporter;

    m_totalCount = netnames.size()
                    + layers.size()
                    + vias.size()
                    + components.size()
                    + zones.size()
                    + board_graphics.size()
                    + traces.size();
    m_doneCount = 0;

    loadNets( aBoard );
    loadLayers( aBoard );
    loadVias( aBoard );
    loadFootprints( aBoard );
    loadZones( aBoard );
    loadGraphics( aBoard );

    for( auto& track : traces )
    {
        checkpoint();

        if( track->lclass == "ETCH" )
            loadEtch( aBoard, track);
        else if( track->layer == "OUTLINE" || track->layer == "DIMENSION" )
            loadOutline( aBoard, track );
        else
            loadPolygon( aBoard, track );
    }

    orderZones( aBoard );

    return true;
}
