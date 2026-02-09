/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
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
 *
 * This file contains file format knowledge derived from the gEDA/pcb project:
 *
 *   gEDA/gaf  - Copyright (C) 1998-2010 Ales Hvezda
 *               Copyright (C) 1998-2016 gEDA Contributors
 *   Lepton EDA - Copyright (C) 2017-2024 Lepton EDA Contributors
 *
 * Both projects are licensed under the GNU General Public License v2 or later.
 * See https://github.com/lepton-eda/lepton-eda and
 *     https://github.com/rlutz/geda-gaf
 */

/**
 * @file pcb_io_geda.cpp
 * @brief Geda PCB file plugin implementation file.
 */
#include "pcb_io/geda/pcb_io_geda.h"

#include <kiplatform/io.h>
#include <wildcards_and_files_ext.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <math/util.h>      // for KiROUND

#include <board.h>
#include <board_design_settings.h>
#include <font/fontconfig.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <macros.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <reporter.h>
#include <zone.h>
#include <wx_filename.h>

#include <wx/dir.h>
#include <wx/log.h>
#include <wx/filename.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <boost/ptr_container/ptr_map.hpp>
#include <filter_reader.h>


static inline long parseInt( const wxString& aValue, double aScalar )
{
    double value = std::numeric_limits<double>::max();

    /*
     * In 2011 gEDA/pcb introduced values with units, like "10mm" or "200mil".
     * Unit-less values are still centimils (100000 units per inch), like with
     * the previous format.
     *
     * Distinction between the even older format (mils, 1000 units per inch)
     * and the pre-2011 format is done in ::parseFOOTPRINT already; the
     * distinction is by whether an object definition opens with '(' or '['.
     * All values with explicit unit open with a '[' so there's no need to
     * consider this distinction when parsing them.
     *
     * The solution here is to watch for a unit and, if present, convert the
     * value to centimils. All unit-less values are read unaltered. This way
     * the code below can continue to consider all read values to be in mils or
     * centimils. It also matches the strategy gEDA/pcb uses for backwards
     * compatibility with its own layouts.
     *
     * Fortunately gEDA/pcb allows only units 'mil' and 'mm' in files, see
     * definition of ALLOW_READABLE in gEDA/pcb's pcb_printf.h. So we don't
     * have to test for all 11 units gEDA/pcb allows in user dialogs.
     */
    if( aValue.EndsWith( wxT( "mm" ) ) )
    {
        aScalar *= 100000.0 / 25.4;
    }
    else if( aValue.EndsWith( wxT( "mil" ) ) )
    {
        aScalar *= 100.;
    }

    // This conversion reports failure on strings as simple as "1000", still
    // it returns the right result in &value. Thus, ignore the return value.
    aValue.ToCDouble(&value);

    if( value == std::numeric_limits<double>::max() ) // conversion really failed
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot convert '%s' to an integer." ),
                                          aValue.GetData() ) );
    }

    return KiROUND( value * aScalar );
}


#define TEXT_DEFAULT_SIZE  ( 40 * pcbIUScale.IU_PER_MILS )
#define OLD_GPCB_UNIT_CONV pcbIUScale.IU_PER_MILS
#define NEW_GPCB_UNIT_CONV ( 0.01 * pcbIUScale.IU_PER_MILS )


/**
 * helper class for creating a footprint library cache.
 *
 * The new footprint library design is a file path of individual footprint files
 * that contain a single footprint per file.  This class is a helper only for the
 * footprint portion of the PLUGIN API, and only for the #PCB_IO_KICAD_SEXPR plugin.  It is
 * private to this implementation file so it is not placed into a header.
 */
class GPCB_FPL_CACHE_ENTRY
{
public:
    GPCB_FPL_CACHE_ENTRY( FOOTPRINT* aFootprint, const WX_FILENAME& aFileName ) :
            m_filename( aFileName ),
            m_footprint( aFootprint )
    { }

    WX_FILENAME GetFileName() const  { return m_filename; }
    std::unique_ptr<FOOTPRINT>& GetFootprint() { return m_footprint; }

private:
    WX_FILENAME m_filename;       ///< The full file name and path of the footprint to cache.
    std::unique_ptr<FOOTPRINT> m_footprint;
};


class GPCB_FPL_CACHE
{
public:
    GPCB_FPL_CACHE( PCB_IO_GEDA* aOwner, const wxString& aLibraryPath );

    wxString GetPath() const { return m_lib_path.GetPath(); }
    bool IsWritable() const { return m_lib_path.IsOk() && m_lib_path.IsDirWritable(); }
    boost::ptr_map<std::string, GPCB_FPL_CACHE_ENTRY>& GetFootprints() { return m_footprints; }

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any PLUGIN.
    // Catch these exceptions higher up please.

    /// Save not implemented for the Geda PCB footprint library format.

    void Load();

    void Remove( const wxString& aFootprintName );

    /**
     * Generate a timestamp representing all source files in the cache (including the
     * parent directory).
     *
     * Timestamps should not be considered ordered.  They either match or they don't.
     */
    static long long GetTimestamp( const wxString& aLibPath );

    /**
     * Return true if the cache is not up-to-date.
     */
    bool IsModified();

private:
    FOOTPRINT* parseFOOTPRINT( LINE_READER* aLineReader );

    /**
     * Test \a aFlag for \a aMask or \a aName.
     *
     * @param aFlag is a list of flags to test against: can be a bit field flag or a list name flag
     *              a bit field flag is an hexadecimal value: Ox00020000 a list name flag is a
     *              string list of flags, comma separated like square,option1.
     * @param aMask is the flag list to test.
     * @param aName is the flag name to find in list.
     * @return true if found.
     */
    bool testFlags( const wxString& aFlag, long aMask, const wxChar* aName );

    /**
     * Extract parameters and tokens from \a aLineReader and adds them to \a aParameterList.
     *
     * Delimiter characters are:
     * [ ] ( )  Begin and end of parameter list and units indicator
     * " is a string delimiter
     * space is the param separator
     * The first word is the keyword
     * the second item is one of ( or [
     * other are parameters (number or delimited string)
     * last parameter is ) or ]
     *
     * @param aParameterList This list of parameters parsed.
     * @param aLineReader The line reader object to parse.
     */
    void parseParameters( wxArrayString& aParameterList, LINE_READER* aLineReader );

    PCB_IO_GEDA*    m_owner;            ///< Plugin object that owns the cache.
    wxFileName      m_lib_path;         ///< The path of the library.

    boost::ptr_map<std::string, GPCB_FPL_CACHE_ENTRY> m_footprints;  ///< Map of footprint filename
                                                                     ///<   to cache entries.

    bool            m_cache_dirty;      ///< Stored separately because it's expensive to check
                                        ///< m_cache_timestamp against all the files.
    long long       m_cache_timestamp;  ///< A hash of the timestamps for all the footprint
                                        ///< files.
};


GPCB_FPL_CACHE::GPCB_FPL_CACHE( PCB_IO_GEDA* aOwner, const wxString& aLibraryPath )
{
    m_owner = aOwner;
    m_lib_path.SetPath( aLibraryPath );
    m_cache_timestamp = 0;
    m_cache_dirty = true;
}


void GPCB_FPL_CACHE::Load()
{
    m_cache_dirty = false;
    m_cache_timestamp = 0;

    // Note: like our .pretty footprint libraries, the gpcb footprint libraries are folders,
    // and the footprints are the .fp files inside this folder.

    wxDir dir( m_lib_path.GetPath() );

    if( !dir.IsOpened() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Footprint library '%s' not found." ),
                                          m_lib_path.GetPath().GetData() ) );
    }

    wxString fullName;
    wxString fileSpec = wxT( "*." ) + wxString( FILEEXT::GedaPcbFootprintLibFileExtension );

    // wxFileName construction is egregiously slow.  Construct it once and just swap out
    // the filename thereafter.
    WX_FILENAME fn( m_lib_path.GetPath(), wxT( "dummyName" ) );

    if( !dir.GetFirst( &fullName, fileSpec ) )
        return;

    wxString cacheErrorMsg;

    do
    {
        fn.SetFullName( fullName );

        // Queue I/O errors so only files that fail to parse don't get loaded.
        try
        {
            // reader now owns fp, will close on exception or return
            FILE_LINE_READER reader( fn.GetFullPath() );
            std::string      name = TO_UTF8( fn.GetName() );
            FOOTPRINT*       footprint = parseFOOTPRINT( &reader );

            // The footprint name is the file name without the extension.
            footprint->SetFPID( LIB_ID( wxEmptyString, fn.GetName() ) );
            m_footprints.insert( name, new GPCB_FPL_CACHE_ENTRY( footprint, fn ) );
        }
        catch( const IO_ERROR& ioe )
        {
            if( !cacheErrorMsg.IsEmpty() )
                cacheErrorMsg += wxT( "\n\n" );

            cacheErrorMsg += ioe.What();
        }
    } while( dir.GetNext( &fullName ) );

    if( !cacheErrorMsg.IsEmpty() )
        THROW_IO_ERROR( cacheErrorMsg );
}


void GPCB_FPL_CACHE::Remove( const wxString& aFootprintName )
{
    std::string footprintName = TO_UTF8( aFootprintName );

    auto it = m_footprints.find( footprintName );

    if( it == m_footprints.end() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library '%s' has no footprint '%s'." ),
                                          m_lib_path.GetPath().GetData(),
                                          aFootprintName.GetData() ) );
    }

    // Remove the footprint from the cache and delete the footprint file from the library.
    wxString fullPath = it->second->GetFileName().GetFullPath();
    m_footprints.erase( footprintName );
    wxRemoveFile( fullPath );
}


bool GPCB_FPL_CACHE::IsModified()
{
    m_cache_dirty = m_cache_dirty || GetTimestamp( m_lib_path.GetFullPath() ) != m_cache_timestamp;

    return m_cache_dirty;
}


long long GPCB_FPL_CACHE::GetTimestamp( const wxString& aLibPath )
{
    wxString fileSpec = wxT( "*." ) + wxString( FILEEXT::GedaPcbFootprintLibFileExtension );

    return KIPLATFORM::IO::TimestampDir( aLibPath, fileSpec );
}


FOOTPRINT* GPCB_FPL_CACHE::parseFOOTPRINT( LINE_READER* aLineReader )
{
    int                        paramCnt;

    // GPCB unit = 0.01 mils and Pcbnew 0.1.
    double                     conv_unit = NEW_GPCB_UNIT_CONV;
    wxString                   msg;
    wxArrayString              parameters;
    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( nullptr );

    if( aLineReader->ReadLine() == nullptr )
    {
        msg = aLineReader->GetSource() + wxT( ": empty file" );
        THROW_IO_ERROR( msg );
    }

    parameters.Clear();
    parseParameters( parameters, aLineReader );
    paramCnt = parameters.GetCount();

    /* From the Geda PCB documentation, valid Element definitions:
     *   Element [SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags]
     *   Element (NFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TNFlags)
     *   Element (NFlags "Desc" "Name" "Value" TX TY TDir TScale TNFlags)
     *   Element (NFlags "Desc" "Name" TX TY TDir TScale TNFlags)
     *   Element ("Desc" "Name" TX TY TDir TScale TNFlags)
     */

    if( parameters[0].CmpNoCase( wxT( "Element" ) ) != 0 )
    {
        msg.Printf( _( "Unknown token '%s'" ), parameters[0] );
        THROW_PARSE_ERROR( msg, aLineReader->GetSource(), (const char *)aLineReader,
                           aLineReader->LineNumber(), 0 );
    }

    if( paramCnt < 10 || paramCnt > 14 )
    {
        msg.Printf( _( "Element token contains %d parameters." ), paramCnt );
        THROW_PARSE_ERROR( msg, aLineReader->GetSource(), (const char *)aLineReader,
                           aLineReader->LineNumber(), 0 );
    }

    // Test symbol after "Element": if [ units = 0.01 mils, and if ( units = 1 mil
    if( parameters[1] == wxT( "(" ) )
        conv_unit = OLD_GPCB_UNIT_CONV;

    if( paramCnt > 10 )
    {
        footprint->SetLibDescription( parameters[3] );
        footprint->SetReference( parameters[4] );
    }
    else
    {
        footprint->SetLibDescription( parameters[2] );
        footprint->SetReference( parameters[3] );
    }

    // Read value
    if( paramCnt > 10 )
        footprint->SetValue( parameters[5] );

    // With gEDA/pcb, value is meaningful after instantiation, only, so it's
    // often empty in bare footprints.
    if( footprint->Value().GetText().IsEmpty() )
        footprint->Value().SetText( wxT( "VAL**" ) );

    if( footprint->Reference().GetText().IsEmpty() )
        footprint->Reference().SetText( wxT( "REF**" ) );

    while( aLineReader->ReadLine() )
    {
        parameters.Clear();
        parseParameters( parameters, aLineReader );

        if( parameters.IsEmpty() || parameters[0] == wxT( "(" ) )
            continue;

        if( parameters[0] == wxT( ")" ) )
            break;

        paramCnt = parameters.GetCount();

        // Test units value for a string line param (more than 3 parameters : ident [ xx ] )
        if( paramCnt > 3 )
        {
            if( parameters[1] == wxT( "(" ) )
                conv_unit = OLD_GPCB_UNIT_CONV;
            else
                conv_unit = NEW_GPCB_UNIT_CONV;
        }

        wxLogTrace( traceGedaPcbPlugin, wxT( "%s parameter count = %d." ),
                    parameters[0], paramCnt );

        // Parse a line with format: ElementLine [X1 Y1 X2 Y2 Thickness]
        if( parameters[0].CmpNoCase( wxT( "ElementLine" ) ) == 0 )
        {
            if( paramCnt != 8 )
            {
                msg.Printf( wxT( "ElementLine token contains %d parameters." ), paramCnt );
                THROW_PARSE_ERROR( msg, aLineReader->GetSource(), (const char *)aLineReader,
                                   aLineReader->LineNumber(), 0 );
            }

            PCB_SHAPE* shape = new PCB_SHAPE( footprint.get(), SHAPE_T::SEGMENT );
            shape->SetLayer( F_SilkS );
            shape->SetStart( VECTOR2I( static_cast<int>( parseInt( parameters[2], conv_unit ) ),
                                       static_cast<int>( parseInt( parameters[3], conv_unit ) ) ) );
            shape->SetEnd( VECTOR2I( static_cast<int>( parseInt( parameters[4], conv_unit ) ),
                                     static_cast<int>( parseInt( parameters[5], conv_unit ) ) ) );
            shape->SetStroke( STROKE_PARAMS( static_cast<int>( parseInt( parameters[6], conv_unit ) ),
                                             LINE_STYLE::SOLID ) );

            shape->Rotate( { 0, 0 }, footprint->GetOrientation() );
            shape->Move( footprint->GetPosition() );

            footprint->Add( shape );
            continue;
        }

        // Parse an arc with format: ElementArc [X Y Width Height StartAngle DeltaAngle Thickness]
        if( parameters[0].CmpNoCase( wxT( "ElementArc" ) ) == 0 )
        {
            if( paramCnt != 10 )
            {
                msg.Printf( wxT( "ElementArc token contains %d parameters." ), paramCnt );
                THROW_PARSE_ERROR( msg, aLineReader->GetSource(), (const char *)aLineReader,
                                   aLineReader->LineNumber(), 0 );
            }

            // Pcbnew does know ellipse so we must have Width = Height
            PCB_SHAPE* shape = new PCB_SHAPE( footprint.get(), SHAPE_T::ARC );
            shape->SetLayer( F_SilkS );
            footprint->Add( shape );

            // for and arc: ibuf[3] = ibuf[4]. Pcbnew does not know ellipses
            int      radius = static_cast<int>( ( parseInt( parameters[4], conv_unit ) +
                                                   parseInt( parameters[5], conv_unit ) ) / 2 );

            VECTOR2I centre( static_cast<int>( parseInt( parameters[2], conv_unit ) ),
                             static_cast<int>( parseInt( parameters[3], conv_unit ) ) );

            // Pcbnew start angles are inverted and 180 degrees from Geda PCB angles.
            EDA_ANGLE start_angle( static_cast<int>( parseInt( parameters[6], -10.0 ) ),
                                   TENTHS_OF_A_DEGREE_T );
            start_angle += ANGLE_180;

            // Pcbnew delta angle direction is the opposite of Geda PCB delta angles.
            EDA_ANGLE sweep_angle( static_cast<int>( parseInt( parameters[7], -10.0 ) ),
                                   TENTHS_OF_A_DEGREE_T );

            // Geda PCB does not support circles.
            if( sweep_angle == -ANGLE_360 )
            {
                shape->SetShape( SHAPE_T::CIRCLE );
                shape->SetCenter( centre );
                shape->SetEnd( centre + VECTOR2I( radius, 0 ) );
            }
            else
            {
                // Calculate start point coordinate of arc
                VECTOR2I arcStart( radius, 0 );
                RotatePoint( arcStart, -start_angle );
                shape->SetCenter( centre );
                shape->SetStart( arcStart + centre );

                // Angle value is clockwise in gpcb and Pcbnew.
                shape->SetArcAngleAndEnd( sweep_angle, true );
            }

            shape->SetStroke( STROKE_PARAMS( static_cast<int>( parseInt( parameters[8], conv_unit ) ),
                                             LINE_STYLE::SOLID ) );

            shape->Rotate( { 0, 0 }, footprint->GetOrientation() );
            shape->Move( footprint->GetPosition() );
            continue;
        }

        // Parse a Pad with no hole with format:
        //   Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]
        //   Pad (rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" NFlags)
        //   Pad (aX1 aY1 aX2 aY2 Thickness "Name" "Number" NFlags)
        //   Pad (aX1 aY1 aX2 aY2 Thickness "Name" NFlags)
        if( parameters[0].CmpNoCase( wxT( "Pad" ) ) == 0 )
        {
            if( paramCnt < 10 || paramCnt > 13 )
            {
                msg.Printf( wxT( "Pad token contains %d parameters." ), paramCnt );
                THROW_PARSE_ERROR( msg, aLineReader->GetSource(), (const char *)aLineReader,
                                   aLineReader->LineNumber(), 0 );
            }

            std::unique_ptr<PAD> pad = std::make_unique<PAD>( footprint.get() );

            static const LSET pad_front( { F_Cu, F_Mask, F_Paste } );
            static const LSET pad_back( { B_Cu, B_Mask, B_Paste } );

            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
            pad->SetAttribute( PAD_ATTRIB::SMD );
            pad->SetLayerSet( pad_front );

            if( testFlags( parameters[paramCnt-2], 0x0080, wxT( "onsolder" ) ) )
                pad->SetLayerSet( pad_back );

            // Set the pad name:
            // Pcbnew pad name is used for electrical connection calculations.
            // Accordingly it should be mapped to gEDA's pin/pad number,
            // which is used for the same purpose.
            // gEDA also features a pin/pad "name", which is an arbitrary string
            // and set to the pin name of the netlist on instantiation. Many gEDA
            // bare footprints use identical strings for name and number, so this
            // can be a bit confusing.
            pad->SetNumber( parameters[paramCnt-3] );

            int x1 = static_cast<int>( parseInt( parameters[2], conv_unit ) );
            int x2 = static_cast<int>( parseInt( parameters[4], conv_unit ) );
            int y1 = static_cast<int>( parseInt( parameters[3], conv_unit ) );
            int y2 = static_cast<int>( parseInt( parameters[5], conv_unit ) );
            int width = static_cast<int>( parseInt( parameters[6], conv_unit ) );
            VECTOR2I delta( x2 - x1, y2 - y1 );
            double angle = atan2( (double)delta.y, (double)delta.x );

            // Get the pad clearance and the solder mask clearance.
            if( paramCnt == 13 )
            {
                int clearance = static_cast<int>( parseInt( parameters[7], conv_unit ) );
                // One of gEDA's oddities is that clearance between pad and polygon
                // is given as the gap on both sides of the pad together, so for
                // KiCad it has to halfed.
                pad->SetLocalClearance( clearance / 2 );

                // In GEDA, the mask value is the size of the hole in this
                // solder mask. In Pcbnew, it is a margin, therefore the distance
                // between the copper and the mask
                int maskMargin = static_cast<int>( parseInt( parameters[8], conv_unit ) );
                maskMargin = ( maskMargin - width ) / 2;
                pad->SetLocalSolderMaskMargin( maskMargin );
            }

            // Negate angle (due to Y reversed axis)
            EDA_ANGLE orient( -angle, RADIANS_T );
            pad->SetOrientation( orient );

            VECTOR2I padPos( ( x1 + x2 ) / 2, ( y1 + y2 ) / 2 );

            pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( delta.EuclideanNorm() + width, width ) );

            padPos += footprint->GetPosition();
            pad->SetPosition( padPos );

            if( !testFlags( parameters[paramCnt-2], 0x0100, wxT( "square" ) ) )
            {
                if( pad->GetSize( PADSTACK::ALL_LAYERS ).x == pad->GetSize( PADSTACK::ALL_LAYERS ).y )
                    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
                else
                    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
            }

            if( pad->GetSizeX() > 0 && pad->GetSizeY() > 0 )
            {
                footprint->Add( pad.release() );
            }
            else
            {
                wxLogError( _( "Invalid zero-sized pad ignored in\nfile: %s" ),
                            aLineReader->GetSource() );
            }

            continue;
        }

        // Parse a Pin with through hole with format:
        //    Pin [rX rY Thickness Clearance Mask Drill "Name" "Number" SFlags]
        //    Pin (rX rY Thickness Clearance Mask Drill "Name" "Number" NFlags)
        //    Pin (aX aY Thickness Drill "Name" "Number" NFlags)
        //    Pin (aX aY Thickness Drill "Name" NFlags)
        //    Pin (aX aY Thickness "Name" NFlags)
        if( parameters[0].CmpNoCase( wxT( "Pin" ) ) == 0 )
        {
            if( paramCnt < 8 || paramCnt > 12 )
            {
                msg.Printf( wxT( "Pin token contains %d parameters." ), paramCnt );
                THROW_PARSE_ERROR( msg, aLineReader->GetSource(), (const char *)aLineReader,
                                   aLineReader->LineNumber(), 0 );
            }

            PAD* pad = new PAD( footprint.get() );

            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );

            static const LSET pad_set = LSET::AllCuMask() | LSET( { F_SilkS, F_Mask, B_Mask } );

            pad->SetLayerSet( pad_set );

            if( testFlags( parameters[paramCnt-2], 0x0100, wxT( "square" ) ) )
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );

            // Set the pad name:
            // Pcbnew pad name is used for electrical connection calculations.
            // Accordingly it should be mapped to gEDA's pin/pad number,
            // which is used for the same purpose.
            pad->SetNumber( parameters[paramCnt-3] );

            VECTOR2I padPos( static_cast<int>( parseInt( parameters[2], conv_unit ) ),
                             static_cast<int>( parseInt( parameters[3], conv_unit ) ) );

            int padSize = static_cast<int>( parseInt( parameters[4], conv_unit ) );

            pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( padSize, padSize ) );

            int drillSize = 0;

            // Get the pad clearance, solder mask clearance, and drill size.
            if( paramCnt == 12 )
            {
                int clearance = static_cast<int>( parseInt( parameters[5], conv_unit ) );
                // One of gEDA's oddities is that clearance between pad and polygon
                // is given as the gap on both sides of the pad together, so for
                // KiCad it has to halfed.
                pad->SetLocalClearance( clearance / 2 );

                // In GEDA, the mask value is the size of the hole in this
                // solder mask. In Pcbnew, it is a margin, therefore the distance
                // between the copper and the mask
                int maskMargin = static_cast<int>( parseInt( parameters[6], conv_unit ) );
                maskMargin = ( maskMargin - padSize ) / 2;
                pad->SetLocalSolderMaskMargin( maskMargin );

                drillSize = static_cast<int>( parseInt( parameters[7], conv_unit ) );
            }
            else
            {
                drillSize = static_cast<int>( parseInt( parameters[5], conv_unit ) );
            }

            pad->SetDrillSize( VECTOR2I( drillSize, drillSize ) );

            padPos += footprint->GetPosition();
            pad->SetPosition( padPos );

            if( pad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CIRCLE
                && pad->GetSize( PADSTACK::ALL_LAYERS ).x != pad->GetSize( PADSTACK::ALL_LAYERS ).y )
            {
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
            }

            footprint->Add( pad );
            continue;
        }
    }

    footprint->AutoPositionFields();

    return footprint.release();
}


void GPCB_FPL_CACHE::parseParameters( wxArrayString& aParameterList, LINE_READER* aLineReader )
{
    char     key;
    wxString tmp;
    char*    line = aLineReader->Line();

    // Last line already ready in main parser loop.
    while( *line != 0 )
    {
        key = *line;
        line++;

        switch( key )
        {
        case '[':
        case '(':
            if( !tmp.IsEmpty() )
            {
                aParameterList.Add( tmp );
                tmp.Clear();
            }

            tmp.Append( key );
            aParameterList.Add( tmp );
            tmp.Clear();

            // Opening delimiter "(" after Element statement.  Any other occurrence is part
            // of a keyword definition.
            if( aParameterList.GetCount() == 1 )
            {
                wxLogTrace( traceGedaPcbPlugin, dump( aParameterList ) );
                return;
            }

            break;

        case ']':
        case ')':
            if( !tmp.IsEmpty() )
            {
                aParameterList.Add( tmp );
                tmp.Clear();
            }

            tmp.Append( key );
            aParameterList.Add( tmp );
            wxLogTrace( traceGedaPcbPlugin, dump( aParameterList ) );
            return;

        case '\n':
        case '\r':
            // Element descriptions can span multiple lines.
            line = aLineReader->ReadLine();
            KI_FALLTHROUGH;

        case '\t':
        case ' ':
            if( !tmp.IsEmpty() )
            {
                aParameterList.Add( tmp );
                tmp.Clear();
            }

            break;

        case '"':
            // Handle empty quotes.
            if( *line == '"' )
            {
                line++;
                tmp.Clear();
                aParameterList.Add( wxEmptyString );
                break;
            }

            while( *line != 0 )
            {
                key = *line;
                line++;

                if( key == '"' )
                {
                    aParameterList.Add( tmp );
                    tmp.Clear();
                    break;
                }
                else
                {
                    tmp.Append( key );
                }
            }

            break;

        case '#':
            line = aLineReader->ReadLine();

            if( !line )
                return;

            break;

        default:
            tmp.Append( key );
            break;
        }
    }
}


bool GPCB_FPL_CACHE::testFlags( const wxString& aFlag, long aMask, const wxChar* aName )
{
    wxString number;

    if( aFlag.StartsWith( wxT( "0x" ), &number ) || aFlag.StartsWith( wxT( "0X" ), &number ) )
    {
        long lflags;

        if( number.ToLong( &lflags, 16 ) && ( lflags & aMask ) )
            return true;
    }
    else if( aFlag.Contains( aName ) )
    {
        return true;
    }

    return false;
}


PCB_IO_GEDA::PCB_IO_GEDA() : PCB_IO( wxS( "gEDA PCB" ) ),
    m_cache( nullptr ),
    m_ctl( 0 ),
    m_numCopperLayers( 2 )
{
    m_reader = nullptr;
    init( nullptr );
}


PCB_IO_GEDA::PCB_IO_GEDA( int aControlFlags ) : PCB_IO( wxS( "gEDA PCB" ) ),
    m_cache( nullptr ),
    m_ctl( aControlFlags ),
    m_numCopperLayers( 2 )
{
    m_reader = nullptr;
    init( nullptr );
}


PCB_IO_GEDA::~PCB_IO_GEDA()
{
    for( FOOTPRINT* fp : m_cachedFootprints )
        delete fp;

    delete m_cache;
}


void PCB_IO_GEDA::init( const std::map<std::string, UTF8>* aProperties )
{
    m_props = aProperties;
}


void PCB_IO_GEDA::validateCache( const wxString& aLibraryPath, bool checkModified  )
{
    if( !m_cache || ( checkModified && m_cache->IsModified() ) )
    {
        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new GPCB_FPL_CACHE( this, aLibraryPath );
        m_cache->Load();
    }
}


FOOTPRINT* PCB_IO_GEDA::ImportFootprint( const wxString&        aFootprintPath,
                                         wxString&              aFootprintNameOut,
                                         const std::map<std::string, UTF8>* aProperties )
{
    wxFileName fn( aFootprintPath );

    FILE_LINE_READER         freader( aFootprintPath );
    WHITESPACE_FILTER_READER reader( freader );

    reader.ReadLine();
    char* line = reader.Line();

    if( !line )
        return nullptr;

    if( strncasecmp( line, "Element", strlen( "Element" ) ) != 0 )
        return nullptr;

    aFootprintNameOut = fn.GetName();

    return FootprintLoad( fn.GetPath(), aFootprintNameOut );
}


void PCB_IO_GEDA::FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                                      bool aBestEfforts, const std::map<std::string, UTF8>* aProperties )
{
    wxDir     dir( aLibraryPath );
    wxString  errorMsg;

    if( !dir.IsOpened() )
    {
        if( aBestEfforts )
            return;
        else
            THROW_IO_ERROR( wxString::Format( _( "Footprint library '%s' not found." ), aLibraryPath ) );
    }

    init( aProperties );

    try
    {
        validateCache( aLibraryPath );
    }
    catch( const IO_ERROR& ioe )
    {
        errorMsg = ioe.What();
    }

    // Some of the files may have been parsed correctly so we want to add the valid files to
    // the library.

    for( const auto& footprint : m_cache->GetFootprints() )
        aFootprintNames.Add( From_UTF8( footprint.first.c_str() ) );

    if( !errorMsg.IsEmpty() && !aBestEfforts )
        THROW_IO_ERROR( errorMsg );
}


const FOOTPRINT* PCB_IO_GEDA::getFootprint( const wxString& aLibraryPath,
                                            const wxString& aFootprintName,
                                            const std::map<std::string, UTF8>* aProperties,
                                            bool checkModified )
{
    init( aProperties );

    validateCache( aLibraryPath, checkModified );

    auto it = m_cache->GetFootprints().find( TO_UTF8( aFootprintName ) );

    if( it == m_cache->GetFootprints().end() )
        return nullptr;

    return it->second->GetFootprint().get();
}


FOOTPRINT* PCB_IO_GEDA::FootprintLoad( const wxString& aLibraryPath,
                                       const wxString& aFootprintName,
                                       bool  aKeepUUID,
                                       const std::map<std::string, UTF8>* aProperties )
{
    // Suppress font substitution warnings (RAII - automatically restored on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( nullptr );

    const FOOTPRINT* footprint = getFootprint( aLibraryPath, aFootprintName, aProperties, true );

    if( footprint )
    {
        FOOTPRINT* copy = (FOOTPRINT*) footprint->Duplicate( IGNORE_PARENT_GROUP );
        copy->SetParent( nullptr );
        return copy;
    }

    return nullptr;
}


void PCB_IO_GEDA::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
                                   const std::map<std::string, UTF8>* aProperties )
{
    init( aProperties );

    validateCache( aLibraryPath );

    if( !m_cache->IsWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library '%s' is read only." ),
                                          aLibraryPath.GetData() ) );
    }

    m_cache->Remove( aFootprintName );
}


bool PCB_IO_GEDA::DeleteLibrary( const wxString& aLibraryPath, const std::map<std::string, UTF8>* aProperties )
{
    wxFileName fn;
    fn.SetPath( aLibraryPath );

    // Return if there is no library path to delete.
    if( !fn.DirExists() )
        return false;

    if( !fn.IsDirWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Insufficient permissions to delete folder '%s'." ),
                                          aLibraryPath.GetData() ) );
    }

    wxDir dir( aLibraryPath );

    if( dir.HasSubDirs() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library folder '%s' has unexpected sub-folders." ),
                                          aLibraryPath.GetData() ) );
    }

    // All the footprint files must be deleted before the directory can be deleted.
    if( dir.HasFiles() )
    {
        unsigned      i;
        wxFileName    tmp;
        wxArrayString files;

        wxDir::GetAllFiles( aLibraryPath, &files );

        for( i = 0;  i < files.GetCount();  i++ )
        {
            tmp = files[i];

            if( tmp.GetExt() != FILEEXT::KiCadFootprintFileExtension )
            {
                THROW_IO_ERROR( wxString::Format( _( "Unexpected file '%s' found in library '%s'." ),
                                                  files[i].GetData(),
                                                  aLibraryPath.GetData() ) );
            }
        }

        for( i = 0;  i < files.GetCount();  i++ )
        {
            wxRemoveFile( files[i] );
        }
    }

    wxLogTrace( traceGedaPcbPlugin, wxT( "Removing footprint library '%s'" ),
                aLibraryPath.GetData() );

    // Some of the more elaborate wxRemoveFile() crap puts up its own wxLog dialog
    // we don't want that.  we want bare metal portability with no UI here.
    if( !wxRmdir( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Footprint library '%s' cannot be deleted." ),
                                          aLibraryPath.GetData() ) );
    }

    // For some reason removing a directory in Windows is not immediately updated.  This delay
    // prevents an error when attempting to immediately recreate the same directory when over
    // writing an existing library.
#ifdef __WINDOWS__
    wxMilliSleep( 250L );
#endif

    if( m_cache && m_cache->GetPath() == aLibraryPath )
    {
        delete m_cache;
        m_cache = nullptr;
    }

    return true;
}


long long PCB_IO_GEDA::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    return GPCB_FPL_CACHE::GetTimestamp( aLibraryPath );
}


bool PCB_IO_GEDA::IsLibraryWritable( const wxString& aLibraryPath )
{
    init( nullptr );

    validateCache( aLibraryPath );

    return m_cache->IsWritable();
}


// =====================================================================
// Board-level import
// =====================================================================


void PCB_IO_GEDA::parseParameters( wxArrayString& aParameterList, LINE_READER* aLineReader )
{
    char     key;
    wxString tmp;
    char*    line = aLineReader->Line();

    while( *line != 0 )
    {
        key = *line;
        line++;

        switch( key )
        {
        case '[':
        case '(':
            if( !tmp.IsEmpty() )
            {
                aParameterList.Add( tmp );
                tmp.Clear();
            }

            tmp.Append( key );
            aParameterList.Add( tmp );
            tmp.Clear();

            if( aParameterList.GetCount() == 1 )
            {
                wxLogTrace( traceGedaPcbPlugin, dump( aParameterList ) );
                return;
            }

            break;

        case ']':
        case ')':
            if( !tmp.IsEmpty() )
            {
                aParameterList.Add( tmp );
                tmp.Clear();
            }

            tmp.Append( key );
            aParameterList.Add( tmp );
            wxLogTrace( traceGedaPcbPlugin, dump( aParameterList ) );
            return;

        case '\n':
        case '\r':
            line = aLineReader->ReadLine();

            if( !line )
                return;

            KI_FALLTHROUGH;

        case '\t':
        case ' ':
            if( !tmp.IsEmpty() )
            {
                aParameterList.Add( tmp );
                tmp.Clear();
            }

            break;

        case '"':
            if( *line == '"' )
            {
                line++;
                tmp.Clear();
                aParameterList.Add( wxEmptyString );
                break;
            }

            while( *line != 0 )
            {
                key = *line;
                line++;

                if( key == '"' )
                {
                    aParameterList.Add( tmp );
                    tmp.Clear();
                    break;
                }
                else
                {
                    tmp.Append( key );
                }
            }

            break;

        case '#':
            line = aLineReader->ReadLine();

            if( !line )
                return;

            break;

        default:
            tmp.Append( key );
            break;
        }
    }
}


bool PCB_IO_GEDA::testFlags( const wxString& aFlag, long aMask, const wxChar* aName )
{
    wxString number;

    if( aFlag.StartsWith( wxT( "0x" ), &number ) || aFlag.StartsWith( wxT( "0X" ), &number ) )
    {
        long lflags;

        if( number.ToLong( &lflags, 16 ) && ( lflags & aMask ) )
            return true;
    }
    else if( aFlag.Contains( aName ) )
    {
        return true;
    }

    return false;
}


bool PCB_IO_GEDA::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    wxFileInputStream input( aFileName );

    if( !input.IsOk() )
        return false;

    wxTextInputStream text( input );

    for( int i = 0; i < 20; i++ )
    {
        if( input.Eof() )
            return false;

        wxString line = text.ReadLine();

        if( line.Contains( wxS( "PCB[" ) ) || line.Contains( wxS( "PCB(" ) ) )
            return true;
    }

    return false;
}


PCB_LAYER_ID PCB_IO_GEDA::mapLayer( int aGedaLayer, const wxString& aLayerName ) const
{
    wxString name = aLayerName.Lower();

    if( name.Contains( wxT( "outline" ) ) || name.Contains( wxT( "route" ) ) )
        return Edge_Cuts;

    if( name.Contains( wxT( "silk" ) ) )
    {
        if( name.Contains( wxT( "solder" ) ) || name.Contains( wxT( "bottom" ) ) )
            return B_SilkS;

        return F_SilkS;
    }

    if( name.Contains( wxT( "mask" ) ) )
    {
        if( name.Contains( wxT( "solder" ) ) || name.Contains( wxT( "bottom" ) ) )
            return B_Mask;

        return F_Mask;
    }

    if( name.Contains( wxT( "paste" ) ) )
    {
        if( name.Contains( wxT( "solder" ) ) || name.Contains( wxT( "bottom" ) ) )
            return B_Paste;

        return F_Paste;
    }

    if( name.Contains( wxT( "fab" ) ) )
        return F_Fab;

    // Copper layers: gEDA uses 1-based numbering. 1 = component/top, 2 = solder/bottom.
    if( name.Contains( wxT( "component" ) ) || name.Contains( wxT( "top" ) )
        || ( aGedaLayer == 1 && !name.Contains( wxT( "solder" ) ) ) )
    {
        return F_Cu;
    }

    if( name.Contains( wxT( "solder" ) ) || name.Contains( wxT( "bottom" ) )
        || aGedaLayer == 2 )
    {
        return B_Cu;
    }

    // Inner copper layers (gEDA layer numbers 3+)
    if( aGedaLayer >= 3 && aGedaLayer <= 16 )
    {
        int innerIdx = aGedaLayer - 3;
        PCB_LAYER_ID innerLayers[] = { In1_Cu, In2_Cu, In3_Cu, In4_Cu, In5_Cu, In6_Cu,
                                       In7_Cu, In8_Cu, In9_Cu, In10_Cu, In11_Cu, In12_Cu,
                                       In13_Cu, In14_Cu };

        if( innerIdx < 14 )
            return innerLayers[innerIdx];
    }

    return F_Cu;
}


void PCB_IO_GEDA::parseVia( wxArrayString& aParameters, double aConvUnit )
{
    // Via[X Y Thickness Clearance Mask Drill "Name" SFlags]
    int paramCnt = aParameters.GetCount();

    if( paramCnt < 10 )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "Via token contains %d parameters, expected at least 10." ), paramCnt ) );
    }

    PCB_VIA* via = new PCB_VIA( m_board );

    int x         = static_cast<int>( parseInt( aParameters[2], aConvUnit ) );
    int y         = static_cast<int>( parseInt( aParameters[3], aConvUnit ) );
    int thickness = static_cast<int>( parseInt( aParameters[4], aConvUnit ) );
    int drill     = static_cast<int>( parseInt( aParameters[7], aConvUnit ) );

    via->SetPosition( VECTOR2I( x, y ) );
    via->SetWidth( PADSTACK::ALL_LAYERS, thickness );
    via->SetDrill( drill );
    via->SetViaType( VIATYPE::THROUGH );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetNet( NETINFO_LIST::OrphanedItem() );

    m_board->Add( via, ADD_MODE::APPEND );
}


FOOTPRINT* PCB_IO_GEDA::parseElement( wxArrayString& aParameters, LINE_READER* aLineReader,
                                      double aConvUnit )
{
    int       paramCnt = aParameters.GetCount();
    double    conv_unit = aConvUnit;
    wxString  msg;

    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( m_board );

    if( paramCnt < 10 || paramCnt > 14 )
    {
        msg.Printf( _( "Element token contains %d parameters." ), paramCnt );
        THROW_IO_ERROR( msg );
    }

    // The long form has SFlags, Desc, Name, Value, MX, MY, TX, TY, TDir, TScale, TSFlags
    // paramCnt == 14: Element [ SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags ]
    // paramCnt == 12: Element ( NFlags "Desc" "Name" "Value" TX TY TDir TScale TNFlags )
    int descIdx, nameIdx, valueIdx, mxIdx;

    if( paramCnt > 10 )
    {
        descIdx  = 3;
        nameIdx  = 4;
        valueIdx = 5;
        mxIdx    = 6;
    }
    else
    {
        descIdx  = 2;
        nameIdx  = 3;
        valueIdx = -1;
        mxIdx    = -1;
    }

    footprint->SetLibDescription( aParameters[descIdx] );
    footprint->SetReference( aParameters[nameIdx] );

    if( valueIdx > 0 )
        footprint->SetValue( aParameters[valueIdx] );

    if( footprint->Value().GetText().IsEmpty() )
        footprint->Value().SetText( wxT( "VAL**" ) );

    if( footprint->Reference().GetText().IsEmpty() )
        footprint->Reference().SetText( wxT( "REF**" ) );

    // Set footprint position from MX, MY (absolute board coordinates)
    if( mxIdx > 0 && paramCnt > 12 )
    {
        int mx = static_cast<int>( parseInt( aParameters[mxIdx], conv_unit ) );
        int my = static_cast<int>( parseInt( aParameters[mxIdx + 1], conv_unit ) );
        footprint->SetPosition( VECTOR2I( mx, my ) );
    }

    wxArrayString parameters;

    while( aLineReader->ReadLine() )
    {
        parameters.Clear();
        parseParameters( parameters, aLineReader );

        if( parameters.IsEmpty() || parameters[0] == wxT( "(" ) )
            continue;

        if( parameters[0] == wxT( ")" ) )
            break;

        paramCnt = parameters.GetCount();

        if( paramCnt > 3 )
        {
            if( parameters[1] == wxT( "(" ) )
                conv_unit = OLD_GPCB_UNIT_CONV;
            else
                conv_unit = NEW_GPCB_UNIT_CONV;
        }

        // ElementLine [X1 Y1 X2 Y2 Thickness]
        if( parameters[0].CmpNoCase( wxT( "ElementLine" ) ) == 0 )
        {
            if( paramCnt != 8 )
                continue;

            PCB_SHAPE* shape = new PCB_SHAPE( footprint.get(), SHAPE_T::SEGMENT );
            shape->SetLayer( F_SilkS );
            shape->SetStart( VECTOR2I( static_cast<int>( parseInt( parameters[2], conv_unit ) ),
                                       static_cast<int>( parseInt( parameters[3], conv_unit ) ) ) );
            shape->SetEnd( VECTOR2I( static_cast<int>( parseInt( parameters[4], conv_unit ) ),
                                     static_cast<int>( parseInt( parameters[5], conv_unit ) ) ) );
            shape->SetStroke( STROKE_PARAMS( static_cast<int>( parseInt( parameters[6], conv_unit ) ),
                                             LINE_STYLE::SOLID ) );

            shape->Rotate( { 0, 0 }, footprint->GetOrientation() );
            shape->Move( footprint->GetPosition() );

            footprint->Add( shape );
            continue;
        }

        // ElementArc [X Y Width Height StartAngle DeltaAngle Thickness]
        if( parameters[0].CmpNoCase( wxT( "ElementArc" ) ) == 0 )
        {
            if( paramCnt != 10 )
                continue;

            PCB_SHAPE* shape = new PCB_SHAPE( footprint.get(), SHAPE_T::ARC );
            shape->SetLayer( F_SilkS );
            footprint->Add( shape );

            int radius = static_cast<int>( ( parseInt( parameters[4], conv_unit )
                                             + parseInt( parameters[5], conv_unit ) ) / 2 );

            VECTOR2I centre( static_cast<int>( parseInt( parameters[2], conv_unit ) ),
                             static_cast<int>( parseInt( parameters[3], conv_unit ) ) );

            EDA_ANGLE start_angle( static_cast<int>( parseInt( parameters[6], -10.0 ) ),
                                   TENTHS_OF_A_DEGREE_T );
            start_angle += ANGLE_180;

            EDA_ANGLE sweep_angle( static_cast<int>( parseInt( parameters[7], -10.0 ) ),
                                   TENTHS_OF_A_DEGREE_T );

            if( sweep_angle == -ANGLE_360 )
            {
                shape->SetShape( SHAPE_T::CIRCLE );
                shape->SetCenter( centre );
                shape->SetEnd( centre + VECTOR2I( radius, 0 ) );
            }
            else
            {
                VECTOR2I arcStart( radius, 0 );
                RotatePoint( arcStart, -start_angle );
                shape->SetCenter( centre );
                shape->SetStart( arcStart + centre );
                shape->SetArcAngleAndEnd( sweep_angle, true );
            }

            shape->SetStroke( STROKE_PARAMS( static_cast<int>( parseInt( parameters[8], conv_unit ) ),
                                             LINE_STYLE::SOLID ) );

            shape->Rotate( { 0, 0 }, footprint->GetOrientation() );
            shape->Move( footprint->GetPosition() );
            continue;
        }

        // Pad [rX1 rY1 rX2 rY2 Thickness Clearance Mask "Name" "Number" SFlags]
        if( parameters[0].CmpNoCase( wxT( "Pad" ) ) == 0 )
        {
            if( paramCnt < 10 || paramCnt > 13 )
                continue;

            std::unique_ptr<PAD> pad = std::make_unique<PAD>( footprint.get() );

            static const LSET pad_front( { F_Cu, F_Mask, F_Paste } );
            static const LSET pad_back( { B_Cu, B_Mask, B_Paste } );

            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
            pad->SetAttribute( PAD_ATTRIB::SMD );
            pad->SetLayerSet( pad_front );

            if( testFlags( parameters[paramCnt - 2], 0x0080, wxT( "onsolder" ) ) )
                pad->SetLayerSet( pad_back );

            pad->SetNumber( parameters[paramCnt - 3] );

            int x1 = static_cast<int>( parseInt( parameters[2], conv_unit ) );
            int x2 = static_cast<int>( parseInt( parameters[4], conv_unit ) );
            int y1 = static_cast<int>( parseInt( parameters[3], conv_unit ) );
            int y2 = static_cast<int>( parseInt( parameters[5], conv_unit ) );
            int width = static_cast<int>( parseInt( parameters[6], conv_unit ) );
            VECTOR2I delta( x2 - x1, y2 - y1 );
            double angle = atan2( (double) delta.y, (double) delta.x );

            if( paramCnt == 13 )
            {
                int clearance = static_cast<int>( parseInt( parameters[7], conv_unit ) );
                pad->SetLocalClearance( clearance / 2 );

                int maskMargin = static_cast<int>( parseInt( parameters[8], conv_unit ) );
                maskMargin = ( maskMargin - width ) / 2;
                pad->SetLocalSolderMaskMargin( maskMargin );
            }

            EDA_ANGLE orient( -angle, RADIANS_T );
            pad->SetOrientation( orient );

            VECTOR2I padPos( ( x1 + x2 ) / 2, ( y1 + y2 ) / 2 );

            pad->SetSize( PADSTACK::ALL_LAYERS,
                          VECTOR2I( delta.EuclideanNorm() + width, width ) );

            padPos += footprint->GetPosition();
            pad->SetPosition( padPos );

            if( !testFlags( parameters[paramCnt - 2], 0x0100, wxT( "square" ) ) )
            {
                if( pad->GetSize( PADSTACK::ALL_LAYERS ).x
                    == pad->GetSize( PADSTACK::ALL_LAYERS ).y )
                {
                    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
                }
                else
                {
                    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
                }
            }

            if( pad->GetSizeX() > 0 && pad->GetSizeY() > 0 )
                footprint->Add( pad.release() );

            continue;
        }

        // Pin [rX rY Thickness Clearance Mask Drill "Name" "Number" SFlags]
        if( parameters[0].CmpNoCase( wxT( "Pin" ) ) == 0 )
        {
            if( paramCnt < 8 || paramCnt > 12 )
                continue;

            PAD* pad = new PAD( footprint.get() );

            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );

            static const LSET pad_set = LSET::AllCuMask()
                                        | LSET( { F_SilkS, F_Mask, B_Mask } );

            pad->SetLayerSet( pad_set );

            if( testFlags( parameters[paramCnt - 2], 0x0100, wxT( "square" ) ) )
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );

            pad->SetNumber( parameters[paramCnt - 3] );

            VECTOR2I padPos( static_cast<int>( parseInt( parameters[2], conv_unit ) ),
                             static_cast<int>( parseInt( parameters[3], conv_unit ) ) );

            int padSize = static_cast<int>( parseInt( parameters[4], conv_unit ) );
            pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( padSize, padSize ) );

            int drillSize = 0;

            if( paramCnt == 12 )
            {
                int clearance = static_cast<int>( parseInt( parameters[5], conv_unit ) );
                pad->SetLocalClearance( clearance / 2 );

                int maskMargin = static_cast<int>( parseInt( parameters[6], conv_unit ) );
                maskMargin = ( maskMargin - padSize ) / 2;
                pad->SetLocalSolderMaskMargin( maskMargin );

                drillSize = static_cast<int>( parseInt( parameters[7], conv_unit ) );
            }
            else
            {
                drillSize = static_cast<int>( parseInt( parameters[5], conv_unit ) );
            }

            pad->SetDrillSize( VECTOR2I( drillSize, drillSize ) );

            padPos += footprint->GetPosition();
            pad->SetPosition( padPos );

            if( pad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CIRCLE
                && pad->GetSize( PADSTACK::ALL_LAYERS ).x
                       != pad->GetSize( PADSTACK::ALL_LAYERS ).y )
            {
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
            }

            footprint->Add( pad );
            continue;
        }
    }

    // Handle the onsolder element flag to flip bottom-side components.
    // In the long form, SFlags is at index 2; in the short form, NFlags is at index 2.
    wxString elementFlags = aParameters[2];

    if( elementFlags.Contains( wxT( "onsolder" ) ) )
        footprint->Flip( footprint->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    footprint->AutoPositionFields();

    return footprint.release();
}


void PCB_IO_GEDA::parseLayer( wxArrayString& aParameters, LINE_READER* aLineReader,
                              double aConvUnit )
{
    // Layer(N "name") (  ... objects ... )
    // In new format: Layer[N "name"]
    int paramCnt = aParameters.GetCount();

    if( paramCnt < 4 )
        return;

    long layerNum = 0;
    aParameters[2].ToLong( &layerNum );

    wxString layerName;

    if( paramCnt > 4 )
        layerName = aParameters[3];

    PCB_LAYER_ID kicadLayer = mapLayer( (int) layerNum, layerName );

    bool isCopperLayer = IsCopperLayer( kicadLayer );

    if( isCopperLayer )
    {
        // gEDA layer numbers are 1-based (1=component, 2=solder, 3+=inner)
        int layerCount = static_cast<int>( layerNum );

        if( layerCount > m_numCopperLayers )
            m_numCopperLayers = layerCount;
    }

    wxArrayString parameters;
    double        conv_unit = aConvUnit;

    while( aLineReader->ReadLine() )
    {
        parameters.Clear();
        parseParameters( parameters, aLineReader );

        if( parameters.IsEmpty() || parameters[0] == wxT( "(" ) )
            continue;

        if( parameters[0] == wxT( ")" ) )
            break;

        paramCnt = parameters.GetCount();

        if( paramCnt > 3 )
        {
            if( parameters[1] == wxT( "(" ) )
                conv_unit = OLD_GPCB_UNIT_CONV;
            else
                conv_unit = NEW_GPCB_UNIT_CONV;
        }

        // Line[X1 Y1 X2 Y2 Thickness Clearance SFlags]
        if( parameters[0].CmpNoCase( wxT( "Line" ) ) == 0 )
        {
            if( paramCnt < 9 )
                continue;

            int x1        = static_cast<int>( parseInt( parameters[2], conv_unit ) );
            int y1        = static_cast<int>( parseInt( parameters[3], conv_unit ) );
            int x2        = static_cast<int>( parseInt( parameters[4], conv_unit ) );
            int y2        = static_cast<int>( parseInt( parameters[5], conv_unit ) );
            int thickness = static_cast<int>( parseInt( parameters[6], conv_unit ) );

            if( isCopperLayer )
            {
                PCB_TRACK* track = new PCB_TRACK( m_board );
                track->SetStart( VECTOR2I( x1, y1 ) );
                track->SetEnd( VECTOR2I( x2, y2 ) );
                track->SetWidth( thickness );
                track->SetLayer( kicadLayer );
                track->SetNet( NETINFO_LIST::OrphanedItem() );
                m_board->Add( track, ADD_MODE::APPEND );
            }
            else
            {
                PCB_SHAPE* shape = new PCB_SHAPE( m_board, SHAPE_T::SEGMENT );
                shape->SetStart( VECTOR2I( x1, y1 ) );
                shape->SetEnd( VECTOR2I( x2, y2 ) );
                shape->SetStroke( STROKE_PARAMS( thickness, LINE_STYLE::SOLID ) );
                shape->SetLayer( kicadLayer );
                m_board->Add( shape, ADD_MODE::APPEND );
            }

            continue;
        }

        // Arc[X Y Width Height Thickness Clearance StartAngle DeltaAngle SFlags]
        if( parameters[0].CmpNoCase( wxT( "Arc" ) ) == 0 )
        {
            if( paramCnt < 11 )
                continue;

            int cx        = static_cast<int>( parseInt( parameters[2], conv_unit ) );
            int cy        = static_cast<int>( parseInt( parameters[3], conv_unit ) );
            int arcWidth  = static_cast<int>( parseInt( parameters[4], conv_unit ) );
            int arcHeight = static_cast<int>( parseInt( parameters[5], conv_unit ) );
            int thickness = static_cast<int>( parseInt( parameters[6], conv_unit ) );
            int radius    = ( arcWidth + arcHeight ) / 2;

            VECTOR2I centre( cx, cy );

            EDA_ANGLE start_angle( static_cast<int>( parseInt( parameters[8], -10.0 ) ),
                                   TENTHS_OF_A_DEGREE_T );
            start_angle += ANGLE_180;

            EDA_ANGLE sweep_angle( static_cast<int>( parseInt( parameters[9], -10.0 ) ),
                                   TENTHS_OF_A_DEGREE_T );

            if( isCopperLayer )
            {
                PCB_ARC* arc = new PCB_ARC( m_board );
                arc->SetLayer( kicadLayer );
                arc->SetWidth( thickness );
                arc->SetNet( NETINFO_LIST::OrphanedItem() );

                VECTOR2I arcStart( radius, 0 );
                RotatePoint( arcStart, -start_angle );
                arc->SetStart( arcStart + centre );

                VECTOR2I arcMid( radius, 0 );
                RotatePoint( arcMid, -start_angle - sweep_angle / 2 );
                arc->SetMid( arcMid + centre );

                VECTOR2I arcEnd( radius, 0 );
                RotatePoint( arcEnd, -start_angle - sweep_angle );
                arc->SetEnd( arcEnd + centre );

                m_board->Add( arc, ADD_MODE::APPEND );
            }
            else
            {
                PCB_SHAPE* shape = new PCB_SHAPE( m_board, SHAPE_T::ARC );
                shape->SetLayer( kicadLayer );

                if( sweep_angle == -ANGLE_360 )
                {
                    shape->SetShape( SHAPE_T::CIRCLE );
                    shape->SetCenter( centre );
                    shape->SetEnd( centre + VECTOR2I( radius, 0 ) );
                }
                else
                {
                    VECTOR2I arcStart( radius, 0 );
                    RotatePoint( arcStart, -start_angle );
                    shape->SetCenter( centre );
                    shape->SetStart( arcStart + centre );
                    shape->SetArcAngleAndEnd( sweep_angle, true );
                }

                shape->SetStroke( STROKE_PARAMS( thickness, LINE_STYLE::SOLID ) );
                m_board->Add( shape, ADD_MODE::APPEND );
            }

            continue;
        }

        // Polygon(SFlags) ( [X Y] [X Y] ... )
        if( parameters[0].CmpNoCase( wxT( "Polygon" ) ) == 0 )
        {
            ZONE* zone = new ZONE( m_board );
            zone->SetLayer( kicadLayer );
            zone->SetNetCode( NETINFO_LIST::UNCONNECTED );
            zone->SetLocalClearance( 0 );
            zone->SetAssignedPriority( 0 );

            const int outlineIdx = -1;
            bool      parsingPoints = false;

            while( aLineReader->ReadLine() )
            {
                wxArrayString polyParams;
                parseParameters( polyParams, aLineReader );

                if( polyParams.IsEmpty() )
                    continue;

                if( polyParams[0] == wxT( ")" ) )
                    break;

                if( polyParams[0] == wxT( "(" ) )
                {
                    parsingPoints = true;
                    continue;
                }

                if( !parsingPoints )
                    continue;

                // Parse coordinate pairs [X Y]
                for( size_t i = 0; i < polyParams.GetCount(); i++ )
                {
                    if( polyParams[i] == wxT( "[" ) && i + 2 < polyParams.GetCount() )
                    {
                        int px = static_cast<int>( parseInt( polyParams[i + 1], conv_unit ) );
                        int py = static_cast<int>( parseInt( polyParams[i + 2], conv_unit ) );
                        zone->AppendCorner( VECTOR2I( px, py ), outlineIdx );
                        i += 3;  // skip past X, Y, ]
                    }
                }
            }

            if( zone->GetNumCorners() >= 3 )
            {
                zone->SetIsFilled( false );
                m_board->Add( zone, ADD_MODE::APPEND );
            }
            else
            {
                delete zone;
            }

            continue;
        }

        // Text[X Y Direction Scale "String" SFlags]
        if( parameters[0].CmpNoCase( wxT( "Text" ) ) == 0 )
        {
            if( paramCnt < 8 )
                continue;

            PCB_TEXT* text = new PCB_TEXT( m_board );
            text->SetLayer( kicadLayer );

            int tx = static_cast<int>( parseInt( parameters[2], conv_unit ) );
            int ty = static_cast<int>( parseInt( parameters[3], conv_unit ) );
            text->SetPosition( VECTOR2I( tx, ty ) );

            long direction = 0;
            parameters[4].ToLong( &direction );

            EDA_ANGLE textAngle( static_cast<double>( direction ) * 90.0, DEGREES_T );
            text->SetTextAngle( textAngle );

            long scale = 100;
            parameters[5].ToLong( &scale );

            int textSize = KiROUND( TEXT_DEFAULT_SIZE * static_cast<double>( scale ) / 100.0 );
            text->SetTextSize( VECTOR2I( textSize, textSize ) );

            text->SetText( parameters[6] );
            m_board->Add( text, ADD_MODE::APPEND );
            continue;
        }
    }
}


void PCB_IO_GEDA::parseNetList( LINE_READER* aLineReader )
{
    // NetList() (
    //   Net("netname" "style") (
    //     Connect("refdes-pinnumber")
    //   )
    // )

    // Build a lookup map for fast refdes -> footprint resolution
    std::map<wxString, FOOTPRINT*> fpByRef;

    for( FOOTPRINT* fp : m_board->Footprints() )
        fpByRef[fp->GetReference()] = fp;

    wxArrayString parameters;

    while( aLineReader->ReadLine() )
    {
        parameters.Clear();
        parseParameters( parameters, aLineReader );

        if( parameters.IsEmpty() )
            continue;

        if( parameters[0] == wxT( ")" ) )
            break;

        if( parameters[0] == wxT( "(" ) )
            continue;

        // Net("netname" "style") (
        if( parameters[0].CmpNoCase( wxT( "Net" ) ) == 0 )
        {
            wxString netName;

            if( parameters.GetCount() > 3 )
                netName = parameters[2];

            // Create or find the net
            NETINFO_ITEM* netInfo = nullptr;
            auto          it = m_netMap.find( netName );

            if( it != m_netMap.end() )
            {
                netInfo = it->second;
            }
            else
            {
                netInfo = new NETINFO_ITEM( m_board, netName );
                m_board->Add( netInfo );
                m_netMap[netName] = netInfo;
            }

            // Parse Connect entries within this Net
            while( aLineReader->ReadLine() )
            {
                wxArrayString netParams;
                parseParameters( netParams, aLineReader );

                if( netParams.IsEmpty() )
                    continue;

                if( netParams[0] == wxT( ")" ) )
                    break;

                if( netParams[0] == wxT( "(" ) )
                    continue;

                // Connect("refdes-pinnumber")
                if( netParams[0].CmpNoCase( wxT( "Connect" ) ) == 0 && netParams.GetCount() > 3 )
                {
                    wxString connectStr = netParams[2];

                    // Find the last hyphen to split refdes from pinnumber
                    int lastDash = connectStr.Find( '-', true );

                    if( lastDash == wxNOT_FOUND )
                        continue;

                    wxString refdes    = connectStr.Left( lastDash );
                    wxString pinNumber = connectStr.Mid( lastDash + 1 );

                    auto fpIt = fpByRef.find( refdes );

                    if( fpIt == fpByRef.end() )
                        continue;

                    for( PAD* pad : fpIt->second->Pads() )
                    {
                        if( pad->GetNumber() == pinNumber )
                        {
                            pad->SetNet( netInfo );
                            break;
                        }
                    }
                }
            }
        }
    }
}


BOARD* PCB_IO_GEDA::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                               const std::map<std::string, UTF8>* aProperties,
                               PROJECT* aProject )
{
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( &LOAD_INFO_REPORTER::GetInstance() );

    init( aProperties );

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    std::unique_ptr<BOARD> deleter( aAppendToMe ? nullptr : m_board );

    for( FOOTPRINT* fp : m_cachedFootprints )
        delete fp;

    m_cachedFootprints.clear();
    m_netMap.clear();
    m_numCopperLayers = 2;

    FILE_LINE_READER reader( aFileName );

    double conv_unit = NEW_GPCB_UNIT_CONV;

    while( reader.ReadLine() )
    {
        wxArrayString parameters;
        parseParameters( parameters, &reader );

        if( parameters.IsEmpty() )
            continue;

        int paramCnt = parameters.GetCount();

        if( paramCnt > 3 )
        {
            if( parameters[1] == wxT( "(" ) )
                conv_unit = OLD_GPCB_UNIT_CONV;
            else
                conv_unit = NEW_GPCB_UNIT_CONV;
        }

        // PCB["name" width height]
        if( parameters[0].CmpNoCase( wxT( "PCB" ) ) == 0 )
        {
            if( paramCnt > 4 )
            {
                int boardWidth  = static_cast<int>( parseInt( parameters[3], conv_unit ) );
                int boardHeight = static_cast<int>( parseInt( parameters[4], conv_unit ) );

                // Set page size from board dimensions
                VECTOR2I pageSize( boardWidth, boardHeight );
                PAGE_INFO page;
                page.SetWidthMils( boardWidth / pcbIUScale.IU_PER_MILS );
                page.SetHeightMils( boardHeight / pcbIUScale.IU_PER_MILS );
                m_board->SetPageSettings( page );
            }

            continue;
        }

        // FileVersion[YYYYMMDD]
        if( parameters[0].CmpNoCase( wxT( "FileVersion" ) ) == 0 )
            continue;

        // Grid, Cursor, Thermal, DRC, Flags, Groups, Styles -- skip
        if( parameters[0].CmpNoCase( wxT( "Grid" ) ) == 0
            || parameters[0].CmpNoCase( wxT( "Cursor" ) ) == 0
            || parameters[0].CmpNoCase( wxT( "Thermal" ) ) == 0
            || parameters[0].CmpNoCase( wxT( "DRC" ) ) == 0
            || parameters[0].CmpNoCase( wxT( "Flags" ) ) == 0
            || parameters[0].CmpNoCase( wxT( "Groups" ) ) == 0
            || parameters[0].CmpNoCase( wxT( "Styles" ) ) == 0
            || parameters[0].CmpNoCase( wxT( "Attribute" ) ) == 0 )
        {
            continue;
        }

        // Via[X Y Thickness Clearance Mask Drill "Name" SFlags]
        if( parameters[0].CmpNoCase( wxT( "Via" ) ) == 0 )
        {
            parseVia( parameters, conv_unit );
            continue;
        }

        // Element[SFlags "Desc" "Name" "Value" MX MY TX TY TDir TScale TSFlags] (...)
        if( parameters[0].CmpNoCase( wxT( "Element" ) ) == 0 )
        {
            FOOTPRINT* fp = parseElement( parameters, &reader, conv_unit );

            if( fp )
            {
                m_board->Add( fp, ADD_MODE::APPEND );

                // Cache a copy for GetImportedCachedLibraryFootprints
                FOOTPRINT* fpCopy = static_cast<FOOTPRINT*>( fp->Clone() );
                fpCopy->SetParent( nullptr );
                m_cachedFootprints.push_back( fpCopy );
            }

            continue;
        }

        // Layer(N "name") ( ... )
        if( parameters[0].CmpNoCase( wxT( "Layer" ) ) == 0 )
        {
            parseLayer( parameters, &reader, conv_unit );
            continue;
        }

        // Rat[X1 Y1 Group1 X2 Y2 Group2 SFlags] -- skip rats nest
        if( parameters[0].CmpNoCase( wxT( "Rat" ) ) == 0 )
            continue;

        // NetList() ( ... )
        if( parameters[0].CmpNoCase( wxT( "NetList" ) ) == 0 )
        {
            parseNetList( &reader );
            continue;
        }
    }

    // Set copper layer count
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    m_board->SetCopperLayerCount( std::max( 2, m_numCopperLayers ) );

    LSET enabledLayers = bds.GetEnabledLayers();
    enabledLayers.set( F_Cu );
    enabledLayers.set( B_Cu );
    enabledLayers.set( F_SilkS );
    enabledLayers.set( B_SilkS );
    enabledLayers.set( F_Mask );
    enabledLayers.set( B_Mask );
    enabledLayers.set( Edge_Cuts );
    bds.SetEnabledLayers( enabledLayers );

    m_board->m_LegacyDesignSettingsLoaded = true;
    m_board->m_LegacyNetclassesLoaded = true;

    deleter.release();
    return m_board;
}


std::vector<FOOTPRINT*> PCB_IO_GEDA::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> retval;

    for( FOOTPRINT* fp : m_cachedFootprints )
        retval.push_back( static_cast<FOOTPRINT*>( fp->Clone() ) );

    return retval;
}
