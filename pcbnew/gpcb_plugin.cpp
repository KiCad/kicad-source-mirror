/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
 *
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

/**
 * @file gpcb_plugin.cpp
 * @brief Geda PCB file plugin implementation file.
 */

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <trigo.h>
#include <wildcards_and_files_ext.h>
#include <filter_reader.h>

#include <class_board.h>
#include <class_module.h>
#include <class_pcb_text.h>
#include <class_drawsegment.h>
#include <class_edge_mod.h>
#include <gpcb_plugin.h>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <boost/ptr_container/ptr_map.hpp>
#include <memory.h>

/**
 * Definition for enabling and disabling footprint library trace output.  See the
 * wxWidgets documentation on using the WXTRACE environment variable.
 */
static const wxString traceFootprintLibrary( wxT( "GedaPcbFootprintLib" ) );


static const char delims[] = " \t\r\n";

static bool inline isSpace( int c ) { return strchr( delims, c ) != 0; }

static void inline traceParams( wxArrayString& aParams )
{
    wxString tmp;

    for( unsigned i = 0;  i < aParams.GetCount();  i++ )
    {
        if( aParams[i].IsEmpty() )
            tmp << wxT( "\"\" " );
        else
            tmp << aParams[i] << wxT( " " );
    }

    wxLogTrace( traceFootprintLibrary, tmp );
}


static inline long parseInt( const wxString& aValue, double aScalar )
{
    double value = LONG_MAX;

    /*
     * In 2011 gEDA/pcb introduced values with units, like "10mm" or "200mil".
     * Unit-less values are still centimils (100000 units per inch), like with
     * the previous format.
     *
     * Distinction between the even older format (mils, 1000 units per inch)
     * and the pre-2011 format is done in ::parseMODULE already; the
     * distinction is by wether an object definition opens with '(' or '['.
     * All values with explicite unit open with a '[' so there's no need to
     * consider this distinction when parsing them.
     *
     * The solution here is to watch for a unit and, if present, convert the
     * value to centimils. All unit-less values are read unaltered. This way
     * the code below can contine to consider all read values to be in mils or
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
    if( value == LONG_MAX ) // conversion really failed
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot convert \"%s\" to an integer" ),
                                          aValue.GetData() ) );
        return 0;
    }

    return KiROUND( value * aScalar );
}


// Tracing for token parameter arrays.
#ifdef DEBUG
#define TRACE_PARAMS( arr )  traceParams( arr );
#else
#define TRACE_PARAMS( arr )                            // Expands to nothing on non-debug builds.
#endif


/**
 * Class GPCB_FPL_CACHE_ITEM
 * is helper class for creating a footprint library cache.
 *
 * The new footprint library design is a file path of individual module files
 * that contain a single module per file.  This class is a helper only for the
 * footprint portion of the PLUGIN API, and only for the #PCB_IO plugin.  It is
 * private to this implementation file so it is not placed into a header.
 */
class GPCB_FPL_CACHE_ITEM
{
    wxFileName         m_file_name; ///< The the full file name and path of the footprint to cache.
    bool               m_writable;  ///< Writability status of the footprint file.
    wxDateTime         m_mod_time;  ///< The last file modified time stamp.
    std::auto_ptr<MODULE> m_module;

public:
    GPCB_FPL_CACHE_ITEM( MODULE* aModule, const wxFileName& aFileName );

    wxString    GetName() const { return m_file_name.GetDirs().Last(); }
    wxFileName  GetFileName() const { return m_file_name; }
    bool        IsModified() const;
    MODULE*     GetModule() const { return m_module.get(); }
    void        UpdateModificationTime() { m_mod_time = m_file_name.GetModificationTime(); }
};


GPCB_FPL_CACHE_ITEM::GPCB_FPL_CACHE_ITEM( MODULE* aModule, const wxFileName& aFileName ) :
    m_module( aModule )
{
    m_file_name = aFileName;
    m_writable = true;          // temporary init

    if( m_file_name.FileExists() )
        m_mod_time = m_file_name.GetModificationTime();
    else
        m_mod_time.Now();
}


bool GPCB_FPL_CACHE_ITEM::IsModified() const
{
    if( !m_file_name.FileExists() )
        return false;

    return m_file_name.GetModificationTime() != m_mod_time;
}


typedef boost::ptr_map< std::string, GPCB_FPL_CACHE_ITEM >  MODULE_MAP;
typedef MODULE_MAP::iterator                                MODULE_ITER;
typedef MODULE_MAP::const_iterator                          MODULE_CITER;


class GPCB_FPL_CACHE
{
    GPCB_PLUGIN*    m_owner;        /// Plugin object that owns the cache.
    wxFileName      m_lib_path;     /// The path of the library.
    wxDateTime      m_mod_time;     /// Footprint library path modified time stamp.
    MODULE_MAP      m_modules;      /// Map of footprint file name per MODULE*.

    MODULE* parseMODULE( LINE_READER* aLineReader ) throw( IO_ERROR, PARSE_ERROR );

    /**
     * Function testFlags
     * tests \a aFlag for \a aMask or \a aName.
     * @param aFlag = List of flags to test against: can be a bit field flag or a list name flag
     * a bit field flag is an hexadecimal value: Ox00020000
     * a list name flag is a string list of flags, comma separated like square,option1
     * @param aMask = flag list to test
     * @param aName = flag name to find in list
     * @return true if found
     */
    bool testFlags( const wxString& aFlag, long aMask, const wxChar* aName );

    /**
     * Function parseParameters
     * extracts parameters and tokens from \a aLineReader and adds them to \a aParameterList.
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
     * @param aLineReader    The line reader object to parse.
     */
    void parseParameters( wxArrayString& aParameterList, LINE_READER* aLineReader );

public:
    GPCB_FPL_CACHE( GPCB_PLUGIN* aOwner, const wxString& aLibraryPath );

    wxString GetPath() const { return m_lib_path.GetPath(); }
    wxDateTime GetLastModificationTime() const { return m_mod_time; }
    bool IsWritable() const { return m_lib_path.IsOk() && m_lib_path.IsDirWritable(); }
    MODULE_MAP& GetModules() { return m_modules; }

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any PLUGIN.
    // Catch these exceptions higher up please.

    /// Save not implemented for the Geda PCB footprint library format.

    void Load();

    void Remove( const wxString& aFootprintName );

    wxDateTime GetLibModificationTime() const;

    /**
     * Function IsModified
     * check if the footprint cache has been modified relative to \a aLibPath
     * and \a aFootprintName.
     *
     * @param aLibPath is a path to test the current cache library path against.
     * @param aFootprintName is the footprint name in the cache to test.  If the footprint
     *                       name is empty, the all the footprint files in the library are
     *                       checked to see if they have been modified.
     * @return true if the cache has been modified.
     */
    bool IsModified( const wxString& aLibPath,
                     const wxString& aFootprintName = wxEmptyString ) const;

    /**
     * Function IsPath
     * checks if \a aPath is the same as the current cache path.
     *
     * This tests paths by converting \a aPath using the native separators.  Internally
     * #FP_CACHE stores the current path using native separators.  This prevents path
     * miscompares on Windows due to the fact that paths can be stored with / instead of \\
     * in the footprint library table.
     *
     * @param aPath is the library path to test against.
     * @return true if \a aPath is the same as the cache path.
     */
    bool IsPath( const wxString& aPath ) const;
};


GPCB_FPL_CACHE::GPCB_FPL_CACHE( GPCB_PLUGIN* aOwner, const wxString& aLibraryPath )
{
    m_owner = aOwner;
    m_lib_path.SetPath( aLibraryPath );
}


wxDateTime GPCB_FPL_CACHE::GetLibModificationTime() const
{
    if( !m_lib_path.DirExists() )
        return wxDateTime::Now();

    return m_lib_path.GetModificationTime();
}


void GPCB_FPL_CACHE::Load()
{
    wxDir dir( m_lib_path.GetPath() );

    if( !dir.IsOpened() )
    {
        THROW_IO_ERROR( wxString::Format( _( "footprint library path '%s' does not exist" ),
                                          m_lib_path.GetPath().GetData() ) );
    }

    wxString fpFileName;
    wxString wildcard = wxT( "*." ) + GedaPcbFootprintLibFileExtension;

    if( !dir.GetFirst( &fpFileName, wildcard, wxDIR_FILES ) )
        return;

    do
    {
        wxFileName fn( m_lib_path.GetPath(), fpFileName );

        // reader now owns fp, will close on exception or return
        FILE_LINE_READER reader( fn.GetFullPath() );
        std::string      name = TO_UTF8( fn.GetName() );
        MODULE*          footprint = parseMODULE( &reader );

        // The footprint name is the file name without the extension.
        footprint->SetFPID( FPID( fn.GetName() ) );
        m_modules.insert( name, new GPCB_FPL_CACHE_ITEM( footprint, fn.GetName() ) );

    } while( dir.GetNext( &fpFileName ) );

    // Remember the file modification time of library file when the
    // cache snapshot was made, so that in a networked environment we will
    // reload the cache as needed.
    m_mod_time = GetLibModificationTime();
}


void GPCB_FPL_CACHE::Remove( const wxString& aFootprintName )
{
    std::string footprintName = TO_UTF8( aFootprintName );

    MODULE_CITER it = m_modules.find( footprintName );

    if( it == m_modules.end() )
    {
        THROW_IO_ERROR( wxString::Format( _( "library <%s> has no footprint '%s' to delete" ),
                                          m_lib_path.GetPath().GetData(),
                                          aFootprintName.GetData() ) );
    }

    // Remove the module from the cache and delete the module file from the library.
    wxString fullPath = it->second->GetFileName().GetFullPath();
    m_modules.erase( footprintName );
    wxRemoveFile( fullPath );
}


bool GPCB_FPL_CACHE::IsPath( const wxString& aPath ) const
{
    // Converts path separators to native path separators
    wxFileName newPath;
    newPath.AssignDir( aPath );

    return m_lib_path == newPath;
}


bool GPCB_FPL_CACHE::IsModified( const wxString& aLibPath, const wxString& aFootprintName ) const
{
    // The library is modified if the library path got deleted or changed.
    if( !m_lib_path.DirExists() || !IsPath( aLibPath ) )
        return true;

    // If no footprint was specified, check every file modification time against the time
    // it was loaded.
    if( aFootprintName.IsEmpty() )
    {
        for( MODULE_CITER it = m_modules.begin();  it != m_modules.end();  ++it )
        {
            wxFileName fn = m_lib_path;

            fn.SetName( it->second->GetFileName().GetName() );
            fn.SetExt( KiCadFootprintFileExtension );

            if( !fn.FileExists() )
            {
                wxLogTrace( traceFootprintLibrary,
                            wxT( "Footprint cache file '%s' does not exist." ),
                            fn.GetFullPath().GetData() );
                return true;
            }

            if( it->second->IsModified() )
            {
                wxLogTrace( traceFootprintLibrary,
                            wxT( "Footprint cache file '%s' has been modified." ),
                            fn.GetFullPath().GetData() );
                return true;
            }
        }
    }
    else
    {
        MODULE_CITER it = m_modules.find( TO_UTF8( aFootprintName ) );

        if( it == m_modules.end() || it->second->IsModified() )
            return true;
    }

    return false;
}


MODULE* GPCB_FPL_CACHE::parseMODULE( LINE_READER* aLineReader ) throw( IO_ERROR, PARSE_ERROR )
{
    #define TEXT_DEFAULT_SIZE  ( 40*IU_PER_MILS )
    #define OLD_GPCB_UNIT_CONV IU_PER_MILS

    // Old version unit = 1 mil, so conv_unit is 10 or 0.1
    #define NEW_GPCB_UNIT_CONV ( 0.01*IU_PER_MILS )

    int                   paramCnt;
    double                conv_unit = NEW_GPCB_UNIT_CONV; // GPCB unit = 0.01 mils and Pcbnew 0.1
    wxPoint               textPos;
    wxString              msg;
    wxArrayString         parameters;
    std::auto_ptr<MODULE> module( new MODULE( NULL ) );


    if( aLineReader->ReadLine() == NULL )
        THROW_IO_ERROR( "unexpected end of file" );

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
        msg.Printf( _( "unknown token \"%s\"" ), GetChars( parameters[0] ) );
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
        module->SetDescription( parameters[3] );
        module->SetReference( parameters[4] );
    }
    else
    {
        module->SetDescription( parameters[2] );
        module->SetReference( parameters[3] );
    }

    // Read value
    if( paramCnt > 10 )
        module->SetValue( parameters[5] );
    // With gEDA/pcb, value is meaningful after instantiation, only, so it's
    // often empty in bare footprints.
    if( module->Value().GetText().IsEmpty() )
        module->Value().SetText( wxT( "Val**" ) );


    if( paramCnt == 14 )
    {
        textPos = wxPoint( parseInt( parameters[8], conv_unit ),
                           parseInt( parameters[9], conv_unit ) );
    }
    else
    {
        textPos = wxPoint( parseInt( parameters[6], conv_unit ),
                           parseInt( parameters[7], conv_unit ) );
    }

    int orientation = parseInt( parameters[paramCnt-4], 1.0 );
    module->Reference().SetOrientation( (orientation % 2) ? 900 : 0 );

    // Calculate size: default height is 40 mils, width 30 mil.
    // real size is:  default * ibuf[idx+3] / 100 (size in gpcb is given in percent of default size
    int thsize = parseInt( parameters[paramCnt-3], TEXT_DEFAULT_SIZE ) / 100;
    thsize = std::max( (int)( 5 * IU_PER_MILS ), thsize ); // Ensure a minimal size = 5 mils
    int twsize = thsize * 30 / 40;
    int thickness = thsize / 8;

    // gEDA/pcb aligns top/left, not pcbnew's default, center/center.
    // Compensate for this by shifting the insertion point instead of the
    // aligment, because alignment isn't changeable in the GUI.
    textPos.x = textPos.x + twsize * module->GetReference().Len() / 2;
    textPos.y += thsize / 2;

    // gEDA/pcb draws a bit too low/left, while pcbnew draws a bit too
    // high/right. Compensate for similar appearance.
    textPos.x -= thsize / 10;
    textPos.y += thsize / 2;

    module->Reference().SetTextPosition( textPos );
    module->Reference().SetPos0( textPos );
    module->Reference().SetSize( wxSize( twsize, thsize ) );
    module->Reference().SetThickness( thickness );

    // gEDA/pcb shows only one of value/reference/description at a time. Which
    // one is selectable by a global menu setting. pcbnew needs reference as
    // well as value visible, so place the value right below the reference.
    module->Value().SetOrientation( module->Reference().GetOrientation() );
    module->Value().SetSize( module->Reference().GetSize() );
    module->Value().SetThickness( module->Reference().GetThickness() );
    textPos.y += thsize * 13 / 10;  // 130% line height
    module->Value().SetTextPosition( textPos );
    module->Value().SetPos0( textPos );

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

        wxLogTrace( traceFootprintLibrary, wxT( "%s parameter count = %d." ),
                    GetChars( parameters[0] ), paramCnt );

        // Parse a line with format: ElementLine [X1 Y1 X2 Y2 Thickness]
        if( parameters[0].CmpNoCase( wxT( "ElementLine" ) ) == 0 )
        {
            if( paramCnt != 8 )
            {
                msg.Printf( wxT( "ElementLine token contains %d parameters." ), paramCnt );
                THROW_PARSE_ERROR( msg, aLineReader->GetSource(), (const char *)aLineReader,
                                   aLineReader->LineNumber(), 0 );
            }

            EDGE_MODULE* drawSeg = new EDGE_MODULE( module.get() );
            drawSeg->SetLayer( F_SilkS );
            drawSeg->SetShape( S_SEGMENT );
            drawSeg->SetStart0( wxPoint( parseInt( parameters[2], conv_unit ),
                                         parseInt( parameters[3], conv_unit ) ) );
            drawSeg->SetEnd0( wxPoint( parseInt( parameters[4], conv_unit ),
                                       parseInt( parameters[5], conv_unit ) ) );
            drawSeg->SetWidth( parseInt( parameters[6], conv_unit ) );
            drawSeg->SetDrawCoord();
            module->GraphicalItems().PushBack( drawSeg );
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
            EDGE_MODULE* drawSeg = new EDGE_MODULE( module.get() );
            drawSeg->SetLayer( F_SilkS );
            drawSeg->SetShape( S_ARC );
            module->GraphicalItems().PushBack( drawSeg );

            // for and arc: ibuf[3] = ibuf[4]. Pcbnew does not know ellipses
            int     radius = ( parseInt( parameters[4], conv_unit ) +
                               parseInt( parameters[5], conv_unit ) ) / 2;

            wxPoint centre( parseInt( parameters[2], conv_unit ),
                            parseInt( parameters[3], conv_unit ) );

            drawSeg->SetStart0( centre );

            // Pcbnew start angles are inverted and 180 degrees from Geda PCB angles.
            double start_angle = parseInt( parameters[6], -10.0 ) + 1800.0;

            // Pcbnew delta angle direction is the opposite of Geda PCB delta angles.
            double sweep_angle = parseInt( parameters[7], -10.0 );

            // Geda PCB does not support circles.
            if( sweep_angle == -3600.0 )
                drawSeg->SetShape( S_CIRCLE );

            // Angle value is clockwise in gpcb and Pcbnew.
            drawSeg->SetAngle( sweep_angle );
            drawSeg->SetEnd0( wxPoint( radius, 0 ) );

            // Calculate start point coordinate of arc
            wxPoint arcStart( drawSeg->GetEnd0() );
            RotatePoint( &arcStart, -start_angle );
            drawSeg->SetEnd0( centre + arcStart );
            drawSeg->SetWidth( parseInt( parameters[8], conv_unit ) );
            drawSeg->SetDrawCoord();
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

            D_PAD* pad = new D_PAD( module.get() );

            static const LSET pad_front( 3, F_Cu, F_Mask, F_Paste );
            static const LSET pad_back(  3, B_Cu, B_Mask, B_Paste );

            pad->SetShape( PAD_SHAPE_RECT );
            pad->SetAttribute( PAD_ATTRIB_SMD );
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
            pad->SetPadName( parameters[paramCnt-3] );

            int x1 = parseInt( parameters[2], conv_unit );
            int x2 = parseInt( parameters[4], conv_unit );
            int y1 = parseInt( parameters[3], conv_unit );
            int y2 = parseInt( parameters[5], conv_unit );
            int width = parseInt( parameters[6], conv_unit );
            wxPoint delta( x2 - x1, y2 - y1 );
            double angle = atan2( (double)delta.y, (double)delta.x );

            // Get the pad clearance and the solder mask clearance.
            if( paramCnt == 13 )
            {
                int clearance = parseInt( parameters[7], conv_unit );
                // One of gEDA's oddities is that clearance between pad and polygon
                // is given as the gap on both sides of the pad together, so for
                // KiCad it has to halfed.
                pad->SetLocalClearance( clearance / 2 );

                // In GEDA, the mask value is the size of the hole in this
                // solder mask. In Pcbnew, it is a margin, therefore the distance
                // between the copper and the mask
                int maskMargin = parseInt( parameters[8], conv_unit );
                maskMargin = ( maskMargin - width ) / 2;
                pad->SetLocalSolderMaskMargin( maskMargin );
            }

            // Negate angle (due to Y reversed axis) and convert it to internal units
            angle = - RAD2DECIDEG( angle );
            pad->SetOrientation( KiROUND( angle ) );

            wxPoint padPos( (x1 + x2) / 2, (y1 + y2) / 2 );

            pad->SetSize( wxSize( KiROUND( EuclideanNorm( delta ) ) + width,
                                  width ) );

            padPos += module->GetPosition();
            pad->SetPos0( padPos );
            pad->SetPosition( padPos );

            if( !testFlags( parameters[paramCnt-2], 0x0100, wxT( "square" ) ) )
            {
                if( pad->GetSize().x == pad->GetSize().y )
                    pad->SetShape( PAD_SHAPE_CIRCLE );
                else
                    pad->SetShape( PAD_SHAPE_OVAL );
            }

            module->Add( pad );
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

            D_PAD* pad = new D_PAD( module.get() );

            pad->SetShape( PAD_SHAPE_CIRCLE );

            static const LSET pad_set = LSET::AllCuMask() | LSET( 3, F_SilkS, F_Mask, B_Mask );

            pad->SetLayerSet( pad_set );

            if( testFlags( parameters[paramCnt-2], 0x0100, wxT( "square" ) ) )
                pad->SetShape( PAD_SHAPE_RECT );

            // Set the pad name:
            // Pcbnew pad name is used for electrical connection calculations.
            // Accordingly it should be mapped to gEDA's pin/pad number,
            // which is used for the same purpose.
            pad->SetPadName( parameters[paramCnt-3] );

            wxPoint padPos( parseInt( parameters[2], conv_unit ),
                            parseInt( parameters[3], conv_unit ) );

            int padSize = parseInt( parameters[4], conv_unit );

            pad->SetSize( wxSize( padSize, padSize ) );

            int drillSize = 0;

            // Get the pad clearance, solder mask clearance, and drill size.
            if( paramCnt == 12 )
            {
                int clearance = parseInt( parameters[5], conv_unit );
                // One of gEDA's oddities is that clearance between pad and polygon
                // is given as the gap on both sides of the pad together, so for
                // KiCad it has to halfed.
                pad->SetLocalClearance( clearance / 2 );

                // In GEDA, the mask value is the size of the hole in this
                // solder mask. In Pcbnew, it is a margin, therefore the distance
                // between the copper and the mask
                int maskMargin = parseInt( parameters[6], conv_unit );
                maskMargin = ( maskMargin - padSize ) / 2;
                pad->SetLocalSolderMaskMargin( maskMargin );

                drillSize = parseInt( parameters[7], conv_unit );
            }
            else
            {
                drillSize = parseInt( parameters[5], conv_unit );
            }

            pad->SetDrillSize( wxSize( drillSize, drillSize ) );
            padPos += module->GetPosition();
            pad->SetPos0( padPos );
            pad->SetPosition( padPos );

            if( pad->GetShape() == PAD_SHAPE_CIRCLE  &&  pad->GetSize().x != pad->GetSize().y )
                pad->SetShape( PAD_SHAPE_OVAL );

            module->Add( pad );
            continue;
        }
    }

    // Recalculate the bounding box
    module->CalculateBoundingBox();
    return module.release();
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
                TRACE_PARAMS( aParameterList );
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
            TRACE_PARAMS( aParameterList );
            return;

        case '\n':
        case '\r':
            // Element descriptions can span multiple lines.
            line = aLineReader->ReadLine();

            // Fall through is intentional.

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


GPCB_PLUGIN::GPCB_PLUGIN() :
    m_cache( 0 ),
    m_ctl( 0 )
{
    m_reader = NULL;
    init( 0 );
}


GPCB_PLUGIN::GPCB_PLUGIN( int aControlFlags ) :
    m_cache( 0 ),
    m_ctl( aControlFlags )
{
    m_reader = NULL;
    init( 0 );
}


GPCB_PLUGIN::~GPCB_PLUGIN()
{
    delete m_cache;
}


void GPCB_PLUGIN::init( const PROPERTIES* aProperties )
{
    m_props = aProperties;
}


void GPCB_PLUGIN::cacheLib( const wxString& aLibraryPath, const wxString& aFootprintName )
{
    if( !m_cache || m_cache->IsModified( aLibraryPath, aFootprintName ) )
    {
        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new GPCB_FPL_CACHE( this, aLibraryPath );
        m_cache->Load();
    }
}


wxArrayString GPCB_PLUGIN::FootprintEnumerate( const wxString&   aLibraryPath,
                                               const PROPERTIES* aProperties )
{
    LOCALE_IO     toggle;     // toggles on, then off, the C locale.
    wxArrayString ret;
    wxDir         dir( aLibraryPath );

    if( !dir.IsOpened() )
    {
        THROW_IO_ERROR( wxString::Format( _( "footprint library path '%s' does not exist" ),
                                          GetChars( aLibraryPath ) ) );
    }

    init( aProperties );

#if 1                         // Set to 0 to only read directory contents, not load cache.
    cacheLib( aLibraryPath );

    const MODULE_MAP& mods = m_cache->GetModules();


    for( MODULE_CITER it = mods.begin();  it != mods.end();  ++it )
    {
        ret.Add( FROM_UTF8( it->first.c_str() ) );
    }
#else
    wxString fpFileName;
    wxString wildcard = wxT( "*." ) + GedaPcbFootprintLibFileExtension;

    if( dir.GetFirst( &fpFileName, wildcard, wxDIR_FILES ) )
    {
        do
        {
            wxFileName fn( aLibraryPath, fpFileName );
            ret.Add( fn.GetName() );
        } while( dir.GetNext( &fpFileName ) );
    }
#endif

    return ret;
}


MODULE* GPCB_PLUGIN::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                                    const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    cacheLib( aLibraryPath, aFootprintName );

    const MODULE_MAP& mods = m_cache->GetModules();

    MODULE_CITER it = mods.find( TO_UTF8( aFootprintName ) );

    if( it == mods.end() )
    {
        return NULL;
    }

    // copy constructor to clone the already loaded MODULE
    return new MODULE( *it->second->GetModule() );
}


void GPCB_PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
                                   const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    cacheLib( aLibraryPath );

    if( !m_cache->IsWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library '%s' is read only" ),
                                          aLibraryPath.GetData() ) );
    }

    m_cache->Remove( aFootprintName );
}


bool GPCB_PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    wxFileName fn;
    fn.SetPath( aLibraryPath );

    // Return if there is no library path to delete.
    if( !fn.DirExists() )
        return false;

    if( !fn.IsDirWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "user does not have permission to delete directory '%s'" ),
                                          aLibraryPath.GetData() ) );
    }

    wxDir dir( aLibraryPath );

    if( dir.HasSubDirs() )
    {
        THROW_IO_ERROR( wxString::Format( _( "library directory '%s' has unexpected sub-directories" ),
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

            if( tmp.GetExt() != KiCadFootprintFileExtension )
            {
                THROW_IO_ERROR( wxString::Format( _( "unexpected file '%s' was found in library path '%s'" ),
                                                  files[i].GetData(), aLibraryPath.GetData() ) );
            }
        }

        for( i = 0;  i < files.GetCount();  i++ )
        {
            wxRemoveFile( files[i] );
        }
    }

    wxLogTrace( traceFootprintLibrary, wxT( "Removing footprint library '%s'" ),
                aLibraryPath.GetData() );

    // Some of the more elaborate wxRemoveFile() crap puts up its own wxLog dialog
    // we don't want that.  we want bare metal portability with no UI here.
    if( !wxRmdir( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "footprint library '%s' cannot be deleted" ),
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
        m_cache = NULL;
    }

    return true;
}


bool GPCB_PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    LOCALE_IO   toggle;

    init( NULL );

    cacheLib( aLibraryPath );

    return m_cache->IsWritable();
}
