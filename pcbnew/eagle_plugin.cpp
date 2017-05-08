/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

Pcbnew PLUGIN for Eagle 6.x XML *.brd and footprint format.

XML parsing and converting:
Getting line numbers and byte offsets from the source XML file is not
possible using currently available XML libraries within KiCad project:
wxXmlDocument and boost::property_tree.

property_tree will give line numbers but no byte offsets, and only during
document loading. This means that if we have a problem after the document is
successfully loaded, there is no way to correlate back to line number and byte
offset of the problem. So a different approach is taken, one which relies on the
XML elements themselves using an XPATH type of reporting mechanism. The path to
the problem is reported in the error messages. This means keeping track of that
path as we traverse the XML document for the sole purpose of accurate error
reporting.

User can load the source XML file into firefox or other xml browser and follow
our error message.

Load() TODO's

*) verify zone fill clearances are correct

*/

#include <errno.h>

#include <wx/string.h>
#include <wx/xml/xml.h>

#include <common.h>
#include <macros.h>
#include <fctsys.h>
#include <trigo.h>
#include <macros.h>
#include <kicad_string.h>
#include <properties.h>
#include <wx/filename.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_pcb_text.h>
#include <class_dimension.h>

#include <eagle_plugin.h>

using namespace std;


typedef MODULE_MAP::iterator          MODULE_ITER;
typedef MODULE_MAP::const_iterator    MODULE_CITER;


/// Parse an eagle distance which is either mm, or mils if there is "mil" suffix.
/// Return is in BIU.
static double parseEagle( const wxString& aDistance )
{
    double ret = strtod( aDistance.c_str(), NULL );
    if( aDistance.npos != aDistance.find( "mil" ) )
        ret = IU_PER_MILS * ret;
    else
        ret = IU_PER_MM * ret;

    return ret;
}


void ERULES::parse( wxXmlNode* aRules )
{
    wxXmlNode* child = aRules->GetChildren();

    while( child )
    {
        if( child->GetName() == "param" )
        {
            const wxString& name = child->GetAttribute( "name" );
            const wxString& value = child->GetAttribute( "value" );

            if( name == "psElongationLong" )
                psElongationLong = wxAtoi( value );
            else if( name == "psElongationOffset" )
                psElongationOffset = wxAtoi( value );
            else if( name == "rvPadTop" )
                value.ToDouble( &rvPadTop );
            else if( name == "rlMinPadTop" )
                rlMinPadTop = parseEagle( value );
            else if( name == "rlMaxPadTop" )
                rlMaxPadTop = parseEagle( value );
            else if( name == "rvViaOuter" )
                value.ToDouble( &rvViaOuter );
            else if( name == "rlMinViaOuter" )
                rlMinViaOuter = parseEagle( value );
            else if( name == "rlMaxViaOuter" )
                rlMaxViaOuter = parseEagle( value );
            else if( name == "mdWireWire" )
                mdWireWire = parseEagle( value );
        }

        child = child->GetNext();
    }
}


EAGLE_PLUGIN::EAGLE_PLUGIN() :
    m_rules( new ERULES() ),
    m_xpath( new XPATH() ),
    m_mod_time( wxDateTime::Now() )
{
    init( NULL );

    clear_cu_map();
}


EAGLE_PLUGIN::~EAGLE_PLUGIN()
{
    delete m_rules;
    delete m_xpath;
}


const wxString EAGLE_PLUGIN::PluginName() const
{
    return wxT( "Eagle" );
}


const wxString EAGLE_PLUGIN::GetFileExtension() const
{
    return wxT( "brd" );
}


int inline EAGLE_PLUGIN::kicad( double d ) const
{
    return KiROUND( biu_per_mm * d );
}


wxSize inline EAGLE_PLUGIN::kicad_fontz( double d ) const
{
    // texts seem to better match eagle when scaled down by 0.95
    int kz = kicad( d ) * 95 / 100;
    return wxSize( kz, kz );
}


BOARD* EAGLE_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe,  const PROPERTIES* aProperties )
{
    LOCALE_IO       toggle;     // toggles on, then off, the C locale.
    wxXmlNode*      doc;

    init( aProperties );

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    // delete on exception, if I own m_board, according to aAppendToMe
    unique_ptr<BOARD> deleter( aAppendToMe ? NULL : m_board );

    try
    {
        // Load the document
        wxXmlDocument xmlDocument;
        wxFileName fn = aFileName;

        if( !xmlDocument.Load( fn.GetFullPath() ) )
            THROW_IO_ERROR( wxString::Format( _( "Unable to read file '%s'" ),
                                              fn.GetFullPath() ) );

        doc = xmlDocument.GetRoot();

        m_min_trace    = INT_MAX;
        m_min_via      = INT_MAX;
        m_min_via_hole = INT_MAX;

        loadAllSections( doc );

        BOARD_DESIGN_SETTINGS& designSettings = m_board->GetDesignSettings();

        if( m_min_trace < designSettings.m_TrackMinWidth )
            designSettings.m_TrackMinWidth = m_min_trace;

        if( m_min_via < designSettings.m_ViasMinSize )
            designSettings.m_ViasMinSize = m_min_via;

        if( m_min_via_hole < designSettings.m_ViasMinDrill )
            designSettings.m_ViasMinDrill = m_min_via_hole;

        if( m_rules->mdWireWire )
        {
            NETCLASSPTR defaultNetclass = designSettings.GetDefault();
            int         clearance = KiROUND( m_rules->mdWireWire );

            if( clearance < defaultNetclass->GetClearance() )
                defaultNetclass->SetClearance( clearance );
        }

        // should be empty, else missing m_xpath->pop()
        wxASSERT( m_xpath->Contents().size() == 0 );
    }
    // Catch all exceptions thrown from the parser.
    catch( const XML_PARSER_ERROR &exc )
    {
        string errmsg = exc.what();

        errmsg += "\n@ ";
        errmsg += m_xpath->Contents();

        THROW_IO_ERROR( errmsg );
    }

    // IO_ERROR exceptions are left uncaught, they pass upwards from here.

    // Ensure the copper layers count is a multiple of 2
    // Pcbnew does not like boards with odd layers count
    // (these boards cannot exist. they actually have a even layers count)
    int lyrcnt = m_board->GetCopperLayerCount();

    if( (lyrcnt % 2) != 0 )
    {
        lyrcnt++;
        m_board->SetCopperLayerCount( lyrcnt );
    }

    centerBoard();

    deleter.release();
    return m_board;
}


void EAGLE_PLUGIN::init( const PROPERTIES* aProperties )
{
    m_hole_count   = 0;
    m_min_trace    = 0;
    m_min_via      = 0;
    m_min_via_hole = 0;
    m_xpath->clear();
    m_pads_to_nets.clear();

    // m_templates.clear();     this is the FOOTPRINT cache too

    m_board = NULL;
    m_props = aProperties;

    mm_per_biu = 1/IU_PER_MM;
    biu_per_mm = IU_PER_MM;

    delete m_rules;
    m_rules = new ERULES();
}


void EAGLE_PLUGIN::clear_cu_map()
{
    // All cu layers are invalid until we see them in the <layers> section while
    // loading either a board or library.  See loadLayerDefs().
    for( unsigned i = 0;  i < DIM(m_cu_map);  ++i )
        m_cu_map[i] = -1;
}


void EAGLE_PLUGIN::loadAllSections( wxXmlNode* aDoc )
{
    wxXmlNode* drawing       = MapChildren( aDoc )["drawing"];
    NODE_MAP drawingChildren = MapChildren( drawing );

    wxXmlNode* board         = drawingChildren["board"];
    NODE_MAP boardChildren   = MapChildren( board );

    m_xpath->push( "eagle.drawing" );

    {
        m_xpath->push( "board" );

        wxXmlNode* designrules = boardChildren["designrules"];
        loadDesignRules( designrules );

        m_xpath->pop();
    }

    {
        m_xpath->push( "layers" );

        wxXmlNode* layers = drawingChildren["layers"];
        loadLayerDefs( layers );

        m_xpath->pop();
    }

    {
        m_xpath->push( "board" );

        wxXmlNode* plain = boardChildren["plain"];
        loadPlain( plain );

        wxXmlNode*  signals = boardChildren["signals"];
        loadSignals( signals );

        wxXmlNode*  libs = boardChildren["libraries"];
        loadLibraries( libs );

        wxXmlNode* elems = boardChildren["elements"];
        loadElements( elems );

        m_xpath->pop();     // "board"
    }

    m_xpath->pop();     // "eagle.drawing"
}


void EAGLE_PLUGIN::loadDesignRules( wxXmlNode* aDesignRules )
{
    m_xpath->push( "designrules" );
    m_rules->parse( aDesignRules );
    m_xpath->pop();     // "designrules"
}


void EAGLE_PLUGIN::loadLayerDefs( wxXmlNode* aLayers )
{
    typedef std::vector<ELAYER>     ELAYERS;
    typedef ELAYERS::const_iterator EITER;

    ELAYERS     cu;  // copper layers

    // Get the first layer and iterate
    wxXmlNode* layerNode = aLayers->GetChildren();

    // find the subset of layers that are copper, and active
    while( layerNode )
    {
        ELAYER  elayer( layerNode );

        if( elayer.number >= 1 && elayer.number <= 16 && ( !elayer.active || *elayer.active ) )
        {
            cu.push_back( elayer );
        }

        layerNode = layerNode->GetNext();
    }

    // establish cu layer map:
    int ki_layer_count = 0;

    for( EITER it = cu.begin();  it != cu.end();  ++it,  ++ki_layer_count )
    {
        if( ki_layer_count == 0 )
            m_cu_map[it->number] = F_Cu;
        else if( ki_layer_count == int( cu.size()-1 ) )
            m_cu_map[it->number] = B_Cu;
        else
        {
            // some eagle boards do not have contiguous layer number sequences.

#if 0   // pre PCB_LAYER_ID & LSET:
            m_cu_map[it->number] = cu.size() - 1 - ki_layer_count;
#else
            m_cu_map[it->number] = ki_layer_count;
#endif
        }
    }

#if 0 && defined(DEBUG)
    printf( "m_cu_map:\n" );
    for( unsigned i=0; i<DIM(m_cu_map);  ++i )
    {
        printf( "\t[%d]:%d\n", i, m_cu_map[i] );
    }
#endif

    // Set the layer names and cu count iff we're loading a board.
    if( m_board )
    {
        m_board->SetCopperLayerCount( cu.size() );

        for( EITER it = cu.begin();  it != cu.end();  ++it )
        {
            PCB_LAYER_ID layer =  kicad_layer( it->number );

            // these function provide their own protection against UNDEFINED_LAYER:
            m_board->SetLayerName( layer, FROM_UTF8( it->name.c_str() ) );
            m_board->SetLayerType( layer, LT_SIGNAL );

            // could map the colors here
        }
    }
}


void EAGLE_PLUGIN::loadPlain( wxXmlNode* aGraphics )
{
    m_xpath->push( "plain" );

    // Get the first graphic and iterate
    wxXmlNode* gr = aGraphics->GetChildren();

    // (polygon | wire | text | circle | rectangle | frame | hole)*
    while( gr )
    {
        wxString grName = gr->GetName();

        if( grName == "wire" )
        {
            m_xpath->push( "wire" );

            EWIRE        w( gr );
            PCB_LAYER_ID layer = kicad_layer( w.layer );

            wxPoint start( kicad_x( w.x1 ), kicad_y( w.y1 ) );
            wxPoint end(   kicad_x( w.x2 ), kicad_y( w.y2 ) );

            if( layer != UNDEFINED_LAYER )
            {
                DRAWSEGMENT* dseg = new DRAWSEGMENT( m_board );
                m_board->Add( dseg, ADD_APPEND );

                if( !w.curve )
                {
                    dseg->SetStart( start );
                    dseg->SetEnd( end );
                }
                else
                {
                    wxPoint center = kicad_arc_center( start, end, *w.curve);

                    dseg->SetShape( S_ARC );
                    dseg->SetStart( center );
                    dseg->SetEnd( start );
                    dseg->SetAngle( *w.curve * -10.0 ); // KiCad rotates the other way
                }

                dseg->SetTimeStamp( timeStamp( gr ) );
                dseg->SetLayer( layer );
                dseg->SetWidth( Millimeter2iu( DEFAULT_PCB_EDGE_THICKNESS ) );
            }
            m_xpath->pop();
        }
        else if( grName == "text" )
        {
            m_xpath->push( "text" );

            ETEXT        t( gr );
            PCB_LAYER_ID layer = kicad_layer( t.layer );

            if( layer != UNDEFINED_LAYER )
            {
                TEXTE_PCB* pcbtxt = new TEXTE_PCB( m_board );
                m_board->Add( pcbtxt, ADD_APPEND );

                pcbtxt->SetLayer( layer );
                pcbtxt->SetTimeStamp( timeStamp( gr ) );
                pcbtxt->SetText( FROM_UTF8( t.text.c_str() ) );
                pcbtxt->SetTextPos( wxPoint( kicad_x( t.x ), kicad_y( t.y ) ) );

                pcbtxt->SetTextSize( kicad_fontz( t.size ) );

                double  ratio = t.ratio ? *t.ratio : 8;     // DTD says 8 is default

                pcbtxt->SetThickness( kicad( t.size * ratio / 100 ) );

                int align = t.align ? *t.align : ETEXT::BOTTOM_LEFT;

                if( t.rot )
                {
                    int sign = t.rot->mirror ? -1 : 1;
                    pcbtxt->SetMirrored( t.rot->mirror );

                    double degrees = t.rot->degrees;

                    if( degrees == 90 || t.rot->spin )
                        pcbtxt->SetTextAngle( sign * t.rot->degrees * 10 );
                    else if( degrees == 180 )
                        align = ETEXT::TOP_RIGHT;
                    else if( degrees == 270 )
                    {
                        pcbtxt->SetTextAngle( sign * 90 * 10 );
                        align = ETEXT::TOP_RIGHT;
                    }
                    else // Ok so text is not at 90,180 or 270 so do some funny stuf to get placement right
                    {
                        if( ( degrees > 0 ) &&  ( degrees < 90 ) )
                            pcbtxt->SetTextAngle( sign * t.rot->degrees * 10 );
                        else if( ( degrees > 90 ) && ( degrees < 180 ) )
                        {
                            pcbtxt->SetTextAngle( sign * ( t.rot->degrees + 180 ) * 10 );
                            align = ETEXT::TOP_RIGHT;
                        }
                        else if( ( degrees > 180 ) && ( degrees < 270 ) )
                        {
                            pcbtxt->SetTextAngle( sign * ( t.rot->degrees - 180 ) * 10 );
                            align = ETEXT::TOP_RIGHT;
                        }
                        else if( ( degrees > 270 ) && ( degrees < 360 ) )
                        {
                            pcbtxt->SetTextAngle( sign * t.rot->degrees * 10 );
                            align = ETEXT::BOTTOM_LEFT;
                        }
                    }
                }

                switch( align )
                {
                case ETEXT::CENTER:
                    // this was the default in pcbtxt's constructor
                    break;

                case ETEXT::CENTER_LEFT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                    break;

                case ETEXT::CENTER_RIGHT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                    break;

                case ETEXT::TOP_CENTER:
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                    break;

                case ETEXT::TOP_LEFT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                    break;

                case ETEXT::TOP_RIGHT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                    break;

                case ETEXT::BOTTOM_CENTER:
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                    break;

                case ETEXT::BOTTOM_LEFT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                    break;

                case ETEXT::BOTTOM_RIGHT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                    break;
                }
            }
            m_xpath->pop();
        }
        else if( grName == "circle" )
        {
            m_xpath->push( "circle" );

            ECIRCLE      c( gr );
            PCB_LAYER_ID layer = kicad_layer( c.layer );

            if( layer != UNDEFINED_LAYER )       // unsupported layer
            {
                DRAWSEGMENT* dseg = new DRAWSEGMENT( m_board );
                m_board->Add( dseg, ADD_APPEND );

                dseg->SetShape( S_CIRCLE );
                dseg->SetTimeStamp( timeStamp( gr ) );
                dseg->SetLayer( layer );
                dseg->SetStart( wxPoint( kicad_x( c.x ), kicad_y( c.y ) ) );
                dseg->SetEnd( wxPoint( kicad_x( c.x + c.radius ), kicad_y( c.y ) ) );
                dseg->SetWidth( kicad( c.width ) );
            }
            m_xpath->pop();
        }
        else if( grName == "rectangle" )
        {
            // This seems to be a simplified rectangular [copper] zone, cannot find any
            // net related info on it from the DTD.
            m_xpath->push( "rectangle" );

            ERECT        r( gr );
            PCB_LAYER_ID layer = kicad_layer( r.layer );

            if( IsCopperLayer( layer ) )
            {
                // use a "netcode = 0" type ZONE:
                ZONE_CONTAINER* zone = new ZONE_CONTAINER( m_board );
                m_board->Add( zone, ADD_APPEND );

                zone->SetTimeStamp( timeStamp( gr ) );
                zone->SetLayer( layer );
                zone->SetNetCode( NETINFO_LIST::UNCONNECTED );

                ZONE_CONTAINER::HATCH_STYLE outline_hatch = ZONE_CONTAINER::DIAGONAL_EDGE;

                zone->AppendCorner( wxPoint( kicad_x( r.x1 ), kicad_y( r.y1 ) ) );
                zone->AppendCorner( wxPoint( kicad_x( r.x2 ), kicad_y( r.y1 ) ) );
                zone->AppendCorner( wxPoint( kicad_x( r.x2 ), kicad_y( r.y2 ) ) );
                zone->AppendCorner( wxPoint( kicad_x( r.x1 ), kicad_y( r.y2 ) ) );

                // this is not my fault:
                zone->SetHatch( outline_hatch, Mils2iu( zone->GetDefaultHatchPitchMils() ), true );
            }

            m_xpath->pop();
        }
        else if( grName == "hole" )
        {
            m_xpath->push( "hole" );
            EHOLE   e( gr );

            // Fabricate a MODULE with a single PAD_ATTRIB_HOLE_NOT_PLATED pad.
            // Use m_hole_count to gen up a unique name.

            MODULE* module = new MODULE( m_board );
            m_board->Add( module, ADD_APPEND );

            char    temp[40];
            sprintf( temp, "@HOLE%d", m_hole_count++ );
            module->SetReference( FROM_UTF8( temp ) );
            module->Reference().SetVisible( false );

            wxPoint pos( kicad_x( e.x ), kicad_y( e.y ) );

            module->SetPosition( pos );

            // Add a PAD_ATTRIB_HOLE_NOT_PLATED pad to this module.
            D_PAD* pad = new D_PAD( module );
            module->Pads().PushBack( pad );

            pad->SetShape( PAD_SHAPE_CIRCLE );
            pad->SetAttribute( PAD_ATTRIB_HOLE_NOT_PLATED );

            /* pad's position is already centered on module at relative (0, 0)
            wxPoint padpos( kicad_x( e.x ), kicad_y( e.y ) );

            pad->SetPos0( padpos );
            pad->SetPosition( padpos + module->GetPosition() );
            */

            wxSize  sz( kicad( e.drill ), kicad( e.drill ) );

            pad->SetDrillSize( sz );
            pad->SetSize( sz );

            pad->SetLayerSet( LSET::AllCuMask() );
            m_xpath->pop();
        }
        else if( grName == "frame" )
        {
            // picture this
        }
        else if( grName == "polygon" )
        {
            // could be on a copper layer, could be on another layer.
            // copper layer would be done using netCode=0 type of ZONE_CONTAINER.
        }
        else if( grName == "dimension" )
        {
            EDIMENSION d( gr );
            PCB_LAYER_ID layer = kicad_layer( d.layer );

            if( layer != UNDEFINED_LAYER )
            {
                DIMENSION* dimension = new DIMENSION( m_board );
                m_board->Add( dimension, ADD_APPEND );

                dimension->SetLayer( layer );
                // The origin and end are assumed to always be in this order from eagle
                dimension->SetOrigin( wxPoint( kicad_x( d.x1 ), kicad_y( d.y1 ) ) );
                dimension->SetEnd( wxPoint( kicad_x( d.x2 ), kicad_y( d.y2 ) ) );
                dimension->Text().SetTextSize( m_board->GetDesignSettings().m_PcbTextSize );

                int width = m_board->GetDesignSettings().m_PcbTextWidth;
                int maxThickness = Clamp_Text_PenSize( width, dimension->Text().GetTextSize() );

                if( width > maxThickness )
                    width = maxThickness;

                dimension->Text().SetThickness( width );
                dimension->SetWidth( width );

                // check which axis the dimension runs in
                // because the "height" of the dimension is perpendicular to that axis
                // Note the check is just if two axes are close enough to each other
                // Eagle appears to have some rounding errors
                if( fabs( d.x1 - d.x2 ) < 0.05 )
                    dimension->SetHeight( kicad_x( d.x1 - d.x3 ) );
                else
                    dimension->SetHeight( kicad_y( d.y3 - d.y1 ) );

                dimension->AdjustDimensionDetails();
            }
        }

        // Get next graphic
        gr = gr->GetNext();
    }
    m_xpath->pop();
}


void EAGLE_PLUGIN::loadLibrary( wxXmlNode* aLib, const string* aLibName )
{
    m_xpath->push( "packages" );

    // library will have <xmlattr> node, skip that and get the single packages node
    wxXmlNode* packages = MapChildren( aLib )["packages"];


    // Create a MODULE for all the eagle packages, for use later via a copy constructor
    // to instantiate needed MODULES in our BOARD.  Save the MODULE templates in
    // a MODULE_MAP using a single lookup key consisting of libname+pkgname.

    // Get the first package and iterate
    wxXmlNode* package = packages->GetChildren();

    while( package )
    {
        m_xpath->push( "package", "name" );

        const wxString& pack_ref = package->GetAttribute( "name" );

        string pack_name( pack_ref.ToStdString() );

        ReplaceIllegalFileNameChars( &pack_name );

        m_xpath->Value( pack_name.c_str() );

        string key = aLibName ? makeKey( *aLibName, pack_name ) : pack_name;

        MODULE* m = makeModule( package, pack_name );

        // add the templating MODULE to the MODULE template factory "m_templates"
        std::pair<MODULE_ITER, bool> r = m_templates.insert( {key, m} );

        if( !r.second
            // && !( m_props && m_props->Value( "ignore_duplicates" ) )
            )
        {
            wxString lib = aLibName ? FROM_UTF8( aLibName->c_str() ) : m_lib_path;
            wxString pkg = FROM_UTF8( pack_name.c_str() );

            wxString emsg = wxString::Format(
                _( "<package> name: '%s' duplicated in eagle <library>: '%s'" ),
                GetChars( pkg ),
                GetChars( lib )
                );
            THROW_IO_ERROR( emsg );
        }

        m_xpath->pop();

        package = package->GetNext();
    }

    m_xpath->pop();     // "packages"
}


void EAGLE_PLUGIN::loadLibraries( wxXmlNode* aLibs )
{
    m_xpath->push( "libraries.library", "name" );

    // Get the first library and iterate
    wxXmlNode* library = aLibs->GetChildren();

    while( library )
    {
        const string& lib_name = library->GetAttribute( "name" ).ToStdString();

        m_xpath->Value( lib_name.c_str() );

        loadLibrary( library, &lib_name );

        library = library->GetNext();
    }

    m_xpath->pop();
}


void EAGLE_PLUGIN::loadElements( wxXmlNode* aElements )
{
    m_xpath->push( "elements.element", "name" );

    EATTR   name;
    EATTR   value;
    bool refanceNamePresetInPackageLayout;
    bool valueNamePresetInPackageLayout;

    // Get the first element and iterate
    wxXmlNode* element = aElements->GetChildren();

    while( element )
    {
        if( element->GetName() != "element" )
            continue;

        EELEMENT    e( element );

        // use "NULL-ness" as an indication of presence of the attribute:
        EATTR*      nameAttr  = 0;
        EATTR*      valueAttr = 0;

        m_xpath->Value( e.name.c_str() );

        string pkg_key = makeKey( e.library, e.package );

        MODULE_CITER mi = m_templates.find( pkg_key );

        if( mi == m_templates.end() )
        {
            wxString emsg = wxString::Format( _( "No '%s' package in library '%s'" ),
                                              GetChars( FROM_UTF8( e.package.c_str() ) ),
                                              GetChars( FROM_UTF8( e.library.c_str() ) ) );
            THROW_IO_ERROR( emsg );
        }

        // copy constructor to clone the template
        MODULE* m = new MODULE( *mi->second );
        m_board->Add( m, ADD_APPEND );

        // update the nets within the pads of the clone
        for( D_PAD* pad = m->Pads();  pad;  pad = pad->Next() )
        {
            string pn_key  = makeKey( e.name, TO_UTF8( pad->GetPadName() ) );

            NET_MAP_CITER ni = m_pads_to_nets.find( pn_key );
            if( ni != m_pads_to_nets.end() )
            {
                const ENET* enet = &ni->second;
                pad->SetNetCode( enet->netcode );
            }
        }

        refanceNamePresetInPackageLayout = true;
        valueNamePresetInPackageLayout = true;
        m->SetPosition( wxPoint( kicad_x( e.x ), kicad_y( e.y ) ) );
        // Is >NAME field set in package layout ?
        if( m->GetReference().size() == 0 )
        {
            m->Reference().SetVisible( false ); // No so no show
            refanceNamePresetInPackageLayout = false;
        }
        // Is >VALUE field set in package layout
        if( m->GetValue().size() == 0 )
        {
            m->Value().SetVisible( false );     // No so no show
            valueNamePresetInPackageLayout = false;
        }
        m->SetReference( FROM_UTF8( e.name.c_str() ) );
        m->SetValue( FROM_UTF8( e.value.c_str() ) );

        if( !e.smashed )
        { // Not smashed so show NAME & VALUE
            if( valueNamePresetInPackageLayout )
                m->Value().SetVisible( true );  // Only if place holder in package layout
            if( refanceNamePresetInPackageLayout )
                m->Reference().SetVisible( true );   // Only if place holder in package layout
        }
        else if( *e.smashed == true )
        { // Smasted so set default to no show for NAME and VALUE
            m->Value().SetVisible( false );
            m->Reference().SetVisible( false );

            // initalize these to default values incase the <attribute> elements are not present.
            m_xpath->push( "attribute", "name" );

            // VALUE and NAME can have something like our text "effects" overrides
            // in SWEET and new schematic.  Eagle calls these XML elements "attribute".
            // There can be one for NAME and/or VALUE both.  Features present in the
            // EATTR override the ones established in the package only if they are
            // present here (except for rot, which if not present means angle zero).
            // So the logic is a bit different than in packageText() and in plain text.

            // Get the first attribute and iterate
            wxXmlNode* attribute = element->GetChildren();

            while( attribute )
            {
                if( attribute->GetName() != "attribute" )
                    continue;

                EATTR   a( attribute );

                if( a.name == "NAME" )
                {
                    name = a;
                    nameAttr = &name;

                    // do we have a display attribute ?
                    if( a.display  )
                    {
                        // Yes!
                        switch( *a.display )
                        {
                        case EATTR::VALUE :
                            nameAttr->name = e.name;
                            m->SetReference( e.name );
                            if( refanceNamePresetInPackageLayout )
                                m->Reference().SetVisible( true );
                            break;

                        case EATTR::NAME :
                            if( refanceNamePresetInPackageLayout )
                            {
                                m->SetReference( "NAME" );
                                m->Reference().SetVisible( true );
                            }
                            break;

                        case EATTR::BOTH :
                            if( refanceNamePresetInPackageLayout )
                                m->Reference().SetVisible( true );
                            nameAttr->name =  nameAttr->name + " = " + e.name;
                            m->SetReference( "NAME = " + e.name );
                            break;

                        case EATTR::Off :
                            m->Reference().SetVisible( false );
                            break;

                        default:
                            nameAttr->name =  e.name;
                            if( refanceNamePresetInPackageLayout )
                                m->Reference().SetVisible( true );
                        }
                    }
                    else
                        // No display, so default is visable, and show value of NAME
                        m->Reference().SetVisible( true );
                }
                else if( a.name == "VALUE" )
                {
                    value = a;
                    valueAttr = &value;

                    if( a.display  )
                    {
                        // Yes!
                        switch( *a.display )
                        {
                        case EATTR::VALUE :
                            valueAttr->value = e.value;
                            m->SetValue( e.value );
                            if( valueNamePresetInPackageLayout )
                                m->Value().SetVisible( true );
                            break;

                        case EATTR::NAME :
                            if( valueNamePresetInPackageLayout )
                                m->Value().SetVisible( true );
                            m->SetValue( "VALUE" );
                            break;

                        case EATTR::BOTH :
                            if( valueNamePresetInPackageLayout )
                                m->Value().SetVisible( true );
                            valueAttr->value = "VALUE = " + e.value;
                            m->SetValue( "VALUE = " + e.value );
                            break;

                        case EATTR::Off :
                            m->Value().SetVisible( false );
                            break;

                        default:
                            valueAttr->value =  e.value;
                            if( valueNamePresetInPackageLayout )
                                m->Value().SetVisible( true );
                        }
                    }
                    else
                        // No display, so default is visible, and show value of NAME
                        m->Value().SetVisible( true );

                }

                attribute = attribute->GetNext();
            }

            m_xpath->pop();     // "attribute"
        }

        orientModuleAndText( m, e, nameAttr, valueAttr );

        // Get next element
        element = element->GetNext();
    }

    m_xpath->pop();     // "elements.element"
}


void EAGLE_PLUGIN::orientModuleAndText( MODULE* m, const EELEMENT& e,
                    const EATTR* nameAttr, const EATTR* valueAttr )
{
    if( e.rot )
    {
        if( e.rot->mirror )
        {
            double orientation = e.rot->degrees + 180.0;
            m->SetOrientation( orientation * 10 );
            m->Flip( m->GetPosition() );
        }
        else
            m->SetOrientation( e.rot->degrees * 10 );
    }

    orientModuleText( m, e, &m->Reference(), nameAttr );
    orientModuleText( m, e, &m->Value(), valueAttr );
}


void EAGLE_PLUGIN::orientModuleText( MODULE* m, const EELEMENT& e,
                            TEXTE_MODULE* txt, const EATTR* aAttr )
{
    // Smashed part ?
    if( aAttr )
    { // Yes
        const EATTR& a = *aAttr;

        if( a.value )
        {
            txt->SetText( FROM_UTF8( a.value->c_str() ) );
        }

        if( a.x && a.y )    // boost::optional
        {
            wxPoint pos( kicad_x( *a.x ), kicad_y( *a.y ) );
            txt->SetTextPos( pos );
        }

        // Even though size and ratio are both optional, I am not seeing
        // a case where ratio is present but size is not.
        double  ratio = 8;
        wxSize  fontz = txt->GetTextSize();

        if( a.size )
        {
            fontz = kicad_fontz( *a.size );
            txt->SetTextSize( fontz );

            if( a.ratio )
                ratio = *a.ratio;
        }

        int  lw = int( fontz.y * ratio / 100.0 );
        txt->SetThickness( lw );

        int align = ETEXT::BOTTOM_LEFT;     // bottom-left is eagle default

        // The "rot" in a EATTR seems to be assumed to be zero if it is not
        // present, and this zero rotation becomes an override to the
        // package's text field.  If they did not want zero, they specify
        // what they want explicitly.
        double  degrees  = a.rot ? a.rot->degrees : 0;
        double  orient;      // relative to parent

        int     sign = 1;
        bool    spin = false;

        if( a.rot )
        {
            spin = a.rot->spin;
            sign = a.rot->mirror ? -1 : 1;
            txt->SetMirrored( a.rot->mirror );
        }

        if( degrees == 90 || degrees == 0 || spin )
        {
            orient = degrees - m->GetOrientation() / 10;
            txt->SetTextAngle( sign * orient * 10 );
        }
        else if( degrees == 180 )
        {
            orient = 0 - m->GetOrientation() / 10;
            txt->SetTextAngle( sign * orient * 10 );
            align = ETEXT::TOP_RIGHT;
        }
        else if( degrees == 270 )
        {
            orient = 90 - m->GetOrientation() / 10;
            align = ETEXT::TOP_RIGHT;
            txt->SetTextAngle( sign * orient * 10 );
        }
        else
        {
            orient = 90 - degrees - m->GetOrientation() / 10;
            txt->SetTextAngle( sign * orient * 10 );
        }

        switch( align )
        {
        case ETEXT::TOP_RIGHT:
            txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
            txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
            break;

        case ETEXT::BOTTOM_LEFT:
            txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            break;

        default:
            ;
        }
    }
    else    // Part is not smash so use Lib default for NAME/VALUE // the text is per the original package, sans <attribute>
    {
        double degrees = ( txt->GetTextAngle() + m->GetOrientation() ) / 10;

        // @todo there are a few more cases than these to contend with:
        if( (!txt->IsMirrored() && ( abs( degrees ) == 180 || abs( degrees ) == 270 ))
         || ( txt->IsMirrored() && ( degrees == 360 ) ) )
        {
            // ETEXT::TOP_RIGHT:
            txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
            txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        }
    }
}


MODULE* EAGLE_PLUGIN::makeModule( wxXmlNode* aPackage, const string& aPkgName ) const
{
    std::unique_ptr<MODULE>   m( new MODULE( m_board ) );

    m->SetFPID( LIB_ID( aPkgName ) );

    // Get the first package item and iterate
    wxXmlNode* packageItem = aPackage->GetChildren();

    while( packageItem )
    {
        const wxString& itemName = packageItem->GetName();

        if( itemName == "description" )
            m->SetDescription( FROM_UTF8( packageItem->GetNodeContent().c_str() ) );

        else if( itemName == "wire" )
            packageWire( m.get(), packageItem );

        else if( itemName == "pad" )
            packagePad( m.get(), packageItem );

        else if( itemName == "text" )
            packageText( m.get(), packageItem );

        else if( itemName == "rectangle" )
            packageRectangle( m.get(), packageItem );

        else if( itemName == "polygon" )
            packagePolygon( m.get(), packageItem );

        else if( itemName == "circle" )
            packageCircle( m.get(), packageItem );

        else if( itemName == "hole" )
            packageHole( m.get(), packageItem );

        else if( itemName == "smd" )
            packageSMD( m.get(), packageItem );

        packageItem = packageItem->GetNext();
    }

    return m.release();
}


void EAGLE_PLUGIN::packageWire( MODULE* aModule, wxXmlNode* aTree ) const
{
    EWIRE        w( aTree );
    PCB_LAYER_ID layer = kicad_layer( w.layer );

    if( IsNonCopperLayer( layer ) )     // only valid non-copper wires, skip copper package wires
    {
        wxPoint start( kicad_x( w.x1 ), kicad_y( w.y1 ) );
        wxPoint end(   kicad_x( w.x2 ), kicad_y( w.y2 ) );
        int     width = kicad( w.width );

        // FIXME: the cap attribute is ignored because kicad can't create lines
        //        with flat ends.
        EDGE_MODULE* dwg;
        if( !w.curve )
        {
            dwg = new EDGE_MODULE( aModule, S_SEGMENT );

            dwg->SetStart0( start );
            dwg->SetEnd0( end );
        }
        else
        {
            dwg = new EDGE_MODULE( aModule, S_ARC );
            wxPoint center = kicad_arc_center( start, end, *w.curve);

            dwg->SetStart0( center );
            dwg->SetEnd0( start );
            dwg->SetAngle( *w.curve * -10.0 ); // KiCad rotates the other way
        }

        dwg->SetLayer( layer );
        dwg->SetWidth( width );

        aModule->GraphicalItems().PushBack( dwg );
    }
}


void EAGLE_PLUGIN::packagePad( MODULE* aModule, wxXmlNode* aTree ) const
{
    // this is thru hole technology here, no SMDs
    EPAD e( aTree );

    D_PAD*  pad = new D_PAD( aModule );
    aModule->Pads().PushBack( pad );

    pad->SetPadName( FROM_UTF8( e.name.c_str() ) );

    // pad's "Position" is not relative to the module's,
    // whereas Pos0 is relative to the module's but is the unrotated coordinate.

    wxPoint padpos( kicad_x( e.x ), kicad_y( e.y ) );

    pad->SetPos0( padpos );

    RotatePoint( &padpos, aModule->GetOrientation() );

    pad->SetPosition( padpos + aModule->GetPosition() );

    pad->SetDrillSize( wxSize( kicad( e.drill ), kicad( e.drill ) ) );

    pad->SetLayerSet( LSET::AllCuMask().set( B_Mask ).set( F_Mask ) );

    if( e.shape )
    {
        switch( *e.shape )
        {
        case EPAD::ROUND:
            wxASSERT( pad->GetShape()==PAD_SHAPE_CIRCLE );    // verify set in D_PAD constructor
            break;

        case EPAD::OCTAGON:
            // no KiCad octagonal pad shape, use PAD_CIRCLE for now.
            // pad->SetShape( PAD_OCTAGON );
            wxASSERT( pad->GetShape()==PAD_SHAPE_CIRCLE );    // verify set in D_PAD constructor
            break;

        case EPAD::LONG:
            pad->SetShape( PAD_SHAPE_OVAL );
            break;

        case EPAD::SQUARE:
            pad->SetShape( PAD_SHAPE_RECT );
            break;

        case EPAD::OFFSET:
            ;   // don't know what to do here.
        }
    }
    else
    {
        // if shape is not present, our default is circle and that matches their default "round"
    }

    if( e.diameter )
    {
        int diameter = kicad( *e.diameter );
        pad->SetSize( wxSize( diameter, diameter ) );
    }
    else
    {
        double drillz  = pad->GetDrillSize().x;
        double annulus = drillz * m_rules->rvPadTop;   // copper annulus, eagle "restring"
        annulus = Clamp( m_rules->rlMinPadTop, annulus, m_rules->rlMaxPadTop );
        int diameter = KiROUND( drillz + 2 * annulus );
        pad->SetSize( wxSize( KiROUND( diameter ), KiROUND( diameter ) ) );
    }

    if( pad->GetShape() == PAD_SHAPE_OVAL )
    {
        // The Eagle "long" pad is wider than it is tall,
        // m_elongation is percent elongation
        wxSize sz = pad->GetSize();
        sz.x = ( sz.x * ( 100 + m_rules->psElongationLong ) ) / 100;
        pad->SetSize( sz );
    }

    if( e.rot )
    {
        pad->SetOrientation( e.rot->degrees * 10 );
    }

    // @todo: handle stop and thermal
}


void EAGLE_PLUGIN::packageText( MODULE* aModule, wxXmlNode* aTree ) const
{
    ETEXT        t( aTree );
    PCB_LAYER_ID layer = kicad_layer( t.layer );

    if( layer == UNDEFINED_LAYER )
    {
        layer = Cmts_User;
    }

    TEXTE_MODULE* txt;

    if( t.text == ">NAME" || t.text == ">name" )
        txt = &aModule->Reference();
    else if( t.text == ">VALUE" || t.text == ">value" )
        txt = &aModule->Value();
    else
    {
        // FIXME: graphical text items are rotated for some reason.
        txt = new TEXTE_MODULE( aModule );
        aModule->GraphicalItems().PushBack( txt );
    }

    txt->SetTimeStamp( timeStamp( aTree ) );
    txt->SetText( FROM_UTF8( t.text.c_str() ) );

    wxPoint pos( kicad_x( t.x ), kicad_y( t.y ) );

    txt->SetTextPos( pos );
    txt->SetPos0( pos - aModule->GetPosition() );

    txt->SetLayer( layer );
    txt->SetTextSize( kicad_fontz( t.size ) );

    double ratio = t.ratio ? *t.ratio : 8;  // DTD says 8 is default

    txt->SetThickness( kicad( t.size * ratio / 100 ) );

    int align = t.align ? *t.align : ETEXT::BOTTOM_LEFT;  // bottom-left is eagle default

    // An eagle package is never rotated, the DTD does not allow it.
    // angle -= aModule->GetOrienation();

    if( t.rot )
    {
        int sign = t.rot->mirror ? -1 : 1;
        txt->SetMirrored( t.rot->mirror );

        double degrees = t.rot->degrees;

        if( degrees == 90 || t.rot->spin )
            txt->SetTextAngle( sign * degrees * 10 );
        else if( degrees == 180 )
            align = ETEXT::TOP_RIGHT;
        else if( degrees == 270 )
        {
            align = ETEXT::TOP_RIGHT;
            txt->SetTextAngle( sign * 90 * 10 );
        }
    }

    switch( align )
    {
    case ETEXT::CENTER:
        // this was the default in pcbtxt's constructor
        break;

    case ETEXT::CENTER_LEFT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ETEXT::CENTER_RIGHT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case ETEXT::TOP_CENTER:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::TOP_LEFT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::TOP_RIGHT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::BOTTOM_CENTER:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case ETEXT::BOTTOM_LEFT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case ETEXT::BOTTOM_RIGHT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;
    }
}


void EAGLE_PLUGIN::packageRectangle( MODULE* aModule, wxXmlNode* aTree ) const
{
    ERECT        r( aTree );
    PCB_LAYER_ID layer = kicad_layer( r.layer );

    if( IsNonCopperLayer( layer ) )  // skip copper "package.rectangle"s
    {
        EDGE_MODULE* dwg = new EDGE_MODULE( aModule, S_POLYGON );
        aModule->GraphicalItems().PushBack( dwg );

        dwg->SetLayer( layer );
        dwg->SetWidth( 0 );

        dwg->SetTimeStamp( timeStamp( aTree ) );

        std::vector<wxPoint> pts;

        wxPoint start( wxPoint( kicad_x( r.x1 ), kicad_y( r.y1 ) ) );
        wxPoint end(   wxPoint( kicad_x( r.x1 ), kicad_y( r.y2 ) ) );

        pts.push_back( start );
        pts.push_back( wxPoint( kicad_x( r.x2 ), kicad_y( r.y1 ) ) );
        pts.push_back( wxPoint( kicad_x( r.x2 ), kicad_y( r.y2 ) ) );
        pts.push_back( end );

        dwg->SetPolyPoints( pts );

        dwg->SetStart0( start );
        dwg->SetEnd0( end );
    }
}


void EAGLE_PLUGIN::packagePolygon( MODULE* aModule, wxXmlNode* aTree ) const
{
    EPOLYGON    p( aTree );
    PCB_LAYER_ID    layer = kicad_layer( p.layer );

    if( IsNonCopperLayer( layer ) )  // skip copper "package.rectangle"s
    {
        EDGE_MODULE* dwg = new EDGE_MODULE( aModule, S_POLYGON );
        aModule->GraphicalItems().PushBack( dwg );

        dwg->SetWidth( 0 );     // it's filled, no need for boundary width

        /*
        switch( layer )
        {
        case Eco1_User:    layer = F_SilkS; break;
        case Eco2_User:    layer = B_SilkS;  break;

        // all MODULE templates (created from eagle packages) are on front layer
        // until cloned.
        case Cmts_User: layer = F_SilkS; break;
        }
        */

        dwg->SetLayer( layer );

        dwg->SetTimeStamp( timeStamp( aTree ) );

        std::vector<wxPoint> pts;
        // TODO: I think there's no way to know a priori the number of children in wxXmlNode :()
        // pts.reserve( aTree.size() );

        // Get the first vertex and iterate
        wxXmlNode* vertex = aTree->GetChildren();

        while( vertex )
        {
            if( vertex->GetName() != "vertex" )     // skip <xmlattr> node
                continue;

            EVERTEX v( vertex );

            pts.push_back( wxPoint( kicad_x( v.x ), kicad_y( v.y ) ) );

            vertex = vertex->GetNext();
        }

        dwg->SetPolyPoints( pts );

        dwg->SetStart0( *pts.begin() );
        dwg->SetEnd0( pts.back() );
    }
}


void EAGLE_PLUGIN::packageCircle( MODULE* aModule, wxXmlNode* aTree ) const
{
    ECIRCLE         e( aTree );
    PCB_LAYER_ID    layer = kicad_layer( e.layer );
    EDGE_MODULE*    gr = new EDGE_MODULE( aModule, S_CIRCLE );

    aModule->GraphicalItems().PushBack( gr );

    gr->SetWidth( kicad( e.width ) );

    switch( (int) layer )
    {
    case UNDEFINED_LAYER:   layer = Cmts_User;          break;
    /*
    case Eco1_User:            layer = F_SilkS; break;
    case Eco2_User:            layer = B_SilkS;  break;
    */
    default:
                            break;
    }

    gr->SetLayer( layer );
    gr->SetTimeStamp( timeStamp( aTree ) );

    gr->SetStart0( wxPoint( kicad_x( e.x ), kicad_y( e.y ) ) );
    gr->SetEnd0( wxPoint( kicad_x( e.x + e.radius ), kicad_y( e.y ) ) );
}


void EAGLE_PLUGIN::packageHole( MODULE* aModule, wxXmlNode* aTree ) const
{
    EHOLE   e( aTree );

    // we add a PAD_ATTRIB_HOLE_NOT_PLATED pad to this module.
    D_PAD* pad = new D_PAD( aModule );
    aModule->Pads().PushBack( pad );

    pad->SetShape( PAD_SHAPE_CIRCLE );
    pad->SetAttribute( PAD_ATTRIB_HOLE_NOT_PLATED );

    // Mechanical purpose only:
    // no offset, no net name, no pad name allowed
    // pad->SetOffset( wxPoint( 0, 0 ) );
    // pad->SetPadName( wxEmptyString );

    wxPoint padpos( kicad_x( e.x ), kicad_y( e.y ) );

    pad->SetPos0( padpos );
    pad->SetPosition( padpos + aModule->GetPosition() );

    wxSize  sz( kicad( e.drill ), kicad( e.drill ) );

    pad->SetDrillSize( sz );
    pad->SetSize( sz );

    pad->SetLayerSet( LSET::AllCuMask() /* | SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT */ );
}


void EAGLE_PLUGIN::packageSMD( MODULE* aModule, wxXmlNode* aTree ) const
{
    ESMD         e( aTree );
    PCB_LAYER_ID layer = kicad_layer( e.layer );

    if( !IsCopperLayer( layer ) )
    {
        return;
    }

    D_PAD*  pad = new D_PAD( aModule );
    aModule->Pads().PushBack( pad );

    pad->SetPadName( FROM_UTF8( e.name.c_str() ) );
    pad->SetShape( PAD_SHAPE_RECT );
    pad->SetAttribute( PAD_ATTRIB_SMD );

    // pad's "Position" is not relative to the module's,
    // whereas Pos0 is relative to the module's but is the unrotated coordinate.

    wxPoint padpos( kicad_x( e.x ), kicad_y( e.y ) );

    pad->SetPos0( padpos );

    RotatePoint( &padpos, aModule->GetOrientation() );

    pad->SetPosition( padpos + aModule->GetPosition() );

    pad->SetSize( wxSize( kicad( e.dx ), kicad( e.dy ) ) );

    pad->SetLayer( layer );

    static const LSET front( 3, F_Cu, F_Paste, F_Mask );
    static const LSET back(  3, B_Cu, B_Paste, B_Mask );

    if( layer == F_Cu )
        pad->SetLayerSet( front );
    else if( layer == B_Cu )
        pad->SetLayerSet( back );

    // Optional according to DTD
    if( e.roundness )    // set set shape to PAD_SHAPE_RECT above, in case roundness is not present
    {
        if( *e.roundness >= 75 )       // roundness goes from 0-100% as integer
        {
            if( e.dy == e.dx )
                pad->SetShape( PAD_SHAPE_CIRCLE );
            else
                pad->SetShape( PAD_SHAPE_OVAL );
        }
    }

    if( e.rot )
    {
        pad->SetOrientation( e.rot->degrees * 10 );
    }

    // don't know what stop, thermals, and cream should look like now.
}

/// non-owning container
typedef std::vector<ZONE_CONTAINER*>    ZONES;


void EAGLE_PLUGIN::loadSignals( wxXmlNode* aSignals )
{
    ZONES   zones;      // per net

    m_xpath->push( "signals.signal", "name" );

    int netCode = 1;

    // Get the first signal and iterate
    wxXmlNode* net = aSignals->GetChildren();

    while( net )
    {
        bool    sawPad = false;

        zones.clear();

        const string& nname = net->GetAttribute( "name" ).ToStdString();
        wxString netName = FROM_UTF8( nname.c_str() );
        m_board->Add( new NETINFO_ITEM( m_board, netName, netCode ) );

        m_xpath->Value( nname.c_str() );

        // Get the first net item and iterate
        wxXmlNode* netItem = net->GetChildren();

        // (contactref | polygon | wire | via)*
        while( netItem )
        {
            const wxString& itemName = netItem->GetName();
            if( itemName == "wire" )
            {
                m_xpath->push( "wire" );

                EWIRE        w( netItem );
                PCB_LAYER_ID layer = kicad_layer( w.layer );

                if( IsCopperLayer( layer ) )
                {
                    TRACK*  t = new TRACK( m_board );

                    t->SetTimeStamp( timeStamp( netItem ) );

                    t->SetPosition( wxPoint( kicad_x( w.x1 ), kicad_y( w.y1 ) ) );
                    t->SetEnd( wxPoint( kicad_x( w.x2 ), kicad_y( w.y2 ) ) );

                    int width = kicad( w.width );
                    if( width < m_min_trace )
                        m_min_trace = width;

                    t->SetWidth( width );
                    t->SetLayer( layer );
                    t->SetNetCode( netCode );

                    m_board->m_Track.Insert( t, NULL );
                }
                else
                {
                    // put non copper wires where the sun don't shine.
                }

                m_xpath->pop();
            }

            else if( itemName == "via" )
            {
                m_xpath->push( "via" );
                EVIA    v( netItem );

                PCB_LAYER_ID  layer_front_most = kicad_layer( v.layer_front_most );
                PCB_LAYER_ID  layer_back_most  = kicad_layer( v.layer_back_most );

                if( IsCopperLayer( layer_front_most ) &&
                    IsCopperLayer( layer_back_most ) )
                {
                    int  kidiam;
                    int  drillz = kicad( v.drill );
                    VIA* via = new VIA( m_board );
                    m_board->m_Track.Insert( via, NULL );

                    via->SetLayerPair( layer_front_most, layer_back_most );

                    if( v.diam )
                    {
                        kidiam = kicad( *v.diam );
                        via->SetWidth( kidiam );
                    }
                    else
                    {
                        double annulus = drillz * m_rules->rvViaOuter;  // eagle "restring"
                        annulus = Clamp( m_rules->rlMinViaOuter, annulus, m_rules->rlMaxViaOuter );
                        kidiam = KiROUND( drillz + 2 * annulus );
                        via->SetWidth( kidiam );
                    }

                    via->SetDrill( drillz );

                    if( kidiam < m_min_via )
                        m_min_via = kidiam;

                    if( drillz < m_min_via_hole )
                        m_min_via_hole = drillz;

                    if( layer_front_most == F_Cu && layer_back_most == B_Cu )
                        via->SetViaType( VIA_THROUGH );
                    else if( layer_front_most == F_Cu || layer_back_most == B_Cu )
                        via->SetViaType( VIA_MICROVIA );
                    else
                        via->SetViaType( VIA_BLIND_BURIED );

                    via->SetTimeStamp( timeStamp( netItem ) );

                    wxPoint pos( kicad_x( v.x ), kicad_y( v.y ) );

                    via->SetPosition( pos  );
                    via->SetEnd( pos );

                    via->SetNetCode( netCode );
                }
                m_xpath->pop();
            }

            else if( itemName == "contactref" )
            {
                m_xpath->push( "contactref" );
                // <contactref element="RN1" pad="7"/>

                const string& reference = netItem->GetAttribute( "element" ).ToStdString();
                const string& pad       = netItem->GetAttribute( "pad" ).ToStdString();

                string key = makeKey( reference, pad ) ;

                // D(printf( "adding refname:'%s' pad:'%s' netcode:%d netname:'%s'\n", reference.c_str(), pad.c_str(), netCode, nname.c_str() );)

                m_pads_to_nets[ key ] = ENET( netCode, nname );

                m_xpath->pop();

                sawPad = true;
            }

            else if( itemName == "polygon" )
            {
                m_xpath->push( "polygon" );

                EPOLYGON     p( netItem );
                PCB_LAYER_ID layer = kicad_layer( p.layer );

                if( IsCopperLayer( layer ) )
                {
                    // use a "netcode = 0" type ZONE:
                    ZONE_CONTAINER* zone = new ZONE_CONTAINER( m_board );
                    m_board->Add( zone, ADD_APPEND );
                    zones.push_back( zone );

                    zone->SetTimeStamp( timeStamp( netItem ) );
                    zone->SetLayer( layer );
                    zone->SetNetCode( netCode );

                    // Get the first vertex and iterate
                    wxXmlNode* vertex = netItem->GetChildren();

                    while( vertex )
                    {
                        if( vertex->GetName() != "vertex" )     // skip <xmlattr> node
                            continue;

                        EVERTEX v( vertex );

                        // Append the corner
                        zone->AppendCorner( wxPoint( kicad_x( v.x ), kicad_y( v.y ) ) );

                        vertex = vertex->GetNext();
                    }

                    // If the pour is a cutout it needs to be set to a keepout
                    if( p.pour == EPOLYGON::CUTOUT )
                    {
                        zone->SetIsKeepout( true );
                        zone->SetDoNotAllowCopperPour( true );
                        zone->SetHatchStyle( ZONE_CONTAINER::NO_HATCH );
                    }

                    // if spacing is set the zone should be hatched
                    if( p.spacing )
                        zone->SetHatch( ZONE_CONTAINER::DIAGONAL_EDGE, *p.spacing, true );

                    // clearances, etc.
                    zone->SetArcSegmentCount( 32 );     // @todo: should be a constructor default?
                    zone->SetMinThickness( kicad( p.width ) );

                    // FIXME: KiCad zones have very rounded corners compared to eagle.
                    //        This means that isolation amounts that work well in eagle
                    //        tend to make copper intrude in soldermask free areas around pads.
                    if( p.isolate )
                    {
                        zone->SetZoneClearance( kicad( *p.isolate ) );
                    }

                    // missing == yes per DTD.
                    bool thermals = !p.thermals || *p.thermals;
                    zone->SetPadConnection( thermals ? PAD_ZONE_CONN_THERMAL : PAD_ZONE_CONN_FULL );
                    if( thermals )
                    {
                        // FIXME: eagle calculates dimensions for thermal spokes
                        //        based on what the zone is connecting to.
                        //        (i.e. width of spoke is half of the smaller side of an smd pad)
                        //        This is a basic workaround
                        zone->SetThermalReliefGap( kicad( p.width + 0.05 ) );
                        zone->SetThermalReliefCopperBridge( kicad( p.width + 0.05 ) );
                    }

                    int rank = p.rank ? *p.rank : p.max_priority;
                    zone->SetPriority( rank );
                }

                m_xpath->pop();     // "polygon"
            }

            netItem = netItem->GetNext();
        }

        if( zones.size() && !sawPad )
        {
            // KiCad does not support an unconnected zone with its own non-zero netcode,
            // but only when assigned netcode = 0 w/o a name...
            for( ZONES::iterator it = zones.begin();  it != zones.end();  ++it )
                (*it)->SetNetCode( NETINFO_LIST::UNCONNECTED );

            // therefore omit this signal/net.
        }
        else
            netCode++;

        // Get next signal
        net = net->GetNext();
    }

    m_xpath->pop();     // "signals.signal"
}


PCB_LAYER_ID EAGLE_PLUGIN::kicad_layer( int aEagleLayer ) const
{
    int kiLayer;

    // eagle copper layer:
    if( aEagleLayer >= 1 && aEagleLayer < int( DIM( m_cu_map ) ) )
    {
        kiLayer = m_cu_map[aEagleLayer];
    }

    else
    {
        // translate non-copper eagle layer to pcbnew layer
        switch( aEagleLayer )
        {
            // Eagle says "Dimension" layer, but it's for board perimeter
            case EAGLE_LAYER::DIMENSION:     kiLayer = Edge_Cuts;    break;
            case EAGLE_LAYER::TPLACE:        kiLayer = F_SilkS;      break;
            case EAGLE_LAYER::BPLACE:        kiLayer = B_SilkS;      break;
            case EAGLE_LAYER::TNAMES:        kiLayer = F_SilkS;      break;
            case EAGLE_LAYER::BNAMES:        kiLayer = B_SilkS;      break;
            case EAGLE_LAYER::TVALUES:       kiLayer = F_SilkS;      break;
            case EAGLE_LAYER::BVALUES:       kiLayer = B_SilkS;      break;
            case EAGLE_LAYER::TSTOP:         kiLayer = F_Mask;       break;
            case EAGLE_LAYER::BSTOP:         kiLayer = B_Mask;       break;
            case EAGLE_LAYER::TCREAM:        kiLayer = F_Paste;      break;
            case EAGLE_LAYER::BCREAM:        kiLayer = B_Paste;      break;
            case EAGLE_LAYER::TFINISH:       kiLayer = F_Mask;       break;
            case EAGLE_LAYER::BFINISH:       kiLayer = B_Mask;       break;
            case EAGLE_LAYER::TGLUE:         kiLayer = F_Adhes;      break;
            case EAGLE_LAYER::BGLUE:         kiLayer = B_Adhes;      break;
            case EAGLE_LAYER::DOCUMENT:      kiLayer = Cmts_User;    break;
            case EAGLE_LAYER::REFERENCELC:   kiLayer = Cmts_User;    break;
            case EAGLE_LAYER::REFERENCELS:   kiLayer = Cmts_User;    break;

        // Packages show the future chip pins on SMD parts using layer 51.
        // This is an area slightly smaller than the PAD/SMD copper area.
        // Carry those visual aids into the MODULE on the fabrication layer,
        // not silkscreen. This is perhaps not perfect, but there is not a lot
        // of other suitable paired layers
            case EAGLE_LAYER::TDOCU:         kiLayer = F_Fab;        break;
            case EAGLE_LAYER::BDOCU:         kiLayer = B_Fab;        break;

        // thes layers are defined as user layers. put them on ECO layers
            case EAGLE_LAYER::USERLAYER1:    kiLayer = Eco1_User;    break;
            case EAGLE_LAYER::USERLAYER2:    kiLayer = Eco2_User;    break;
        default:
            // some layers do not map to KiCad
            wxASSERT_MSG( false, wxString::Format( "Unsupported Eagle layer %d", aEagleLayer ) );
            kiLayer = UNDEFINED_LAYER;      break;
        }
    }

    return PCB_LAYER_ID( kiLayer );
}


void EAGLE_PLUGIN::centerBoard()
{
    if( m_props )
    {
        UTF8 page_width;
        UTF8 page_height;

        if( m_props->Value( "page_width",  &page_width ) &&
            m_props->Value( "page_height", &page_height ) )
        {
            EDA_RECT bbbox = m_board->GetBoardEdgesBoundingBox();

            int w = atoi( page_width.c_str() );
            int h = atoi( page_height.c_str() );

            int desired_x = ( w - bbbox.GetWidth() )  / 2;
            int desired_y = ( h - bbbox.GetHeight() ) / 2;

            DBG(printf( "bbox.width:%d bbox.height:%d w:%d h:%d desired_x:%d desired_y:%d\n",
                bbbox.GetWidth(), bbbox.GetHeight(), w, h, desired_x, desired_y );)

            m_board->Move( wxPoint( desired_x - bbbox.GetX(), desired_y - bbbox.GetY() ) );
        }
    }
}


wxDateTime EAGLE_PLUGIN::getModificationTime( const wxString& aPath )
{
    wxFileName  fn( aPath );

    // Do not call wxFileName::GetModificationTime() on a non-existent file, because
    // if it fails, wx's implementation calls the crap wxLogSysError() which
    // eventually infects our UI with an unwanted popup window, so don't let it fail.
    if( !fn.IsFileReadable() )
    {
        wxString msg = wxString::Format(
            _( "File '%s' is not readable." ),
            GetChars( aPath ) );

        THROW_IO_ERROR( msg );
    }

    /*
    // update the writable flag while we have a wxFileName, in a network this
    // is possibly quite dynamic anyway.
    m_writable = fn.IsFileWritable();
    */

    wxDateTime modTime = fn.GetModificationTime();

    if( !modTime.IsValid() )
        modTime.Now();

    return modTime;
}


void EAGLE_PLUGIN::cacheLib( const wxString& aLibPath )
{
    try
    {
        wxDateTime  modtime = getModificationTime( aLibPath );

        // Fixes assertions in wxWidgets debug builds for the wxDateTime object.  Refresh the
        // cache if either of the wxDateTime objects are invalid or the last file modification
        // time differs from the current file modification time.
        bool load = !m_mod_time.IsValid() || !modtime.IsValid() ||
                    m_mod_time != modtime;

        if( aLibPath != m_lib_path || load )
        {
            wxXmlNode*  doc;
            LOCALE_IO   toggle;     // toggles on, then off, the C locale.

            m_templates.clear();

            // Set this before completion of loading, since we rely on it for
            // text of an exception.  Delay setting m_mod_time until after successful load
            // however.
            m_lib_path = aLibPath;

            // 8 bit "filename" should be encoded according to disk filename encoding,
            // (maybe this is current locale, maybe not, its a filesystem issue),
            // and is not necessarily utf8.
            string filename = (const char*) aLibPath.char_str( wxConvFile );

            // Load the document
            wxXmlDocument xmlDocument;
            wxFileName fn( filename );

            if( !xmlDocument.Load( fn.GetFullPath() ) )
                THROW_IO_ERROR( wxString::Format( _( "Unable to read file '%s'" ),
                                                  fn.GetFullPath() ) );

            doc = xmlDocument.GetRoot();

            wxXmlNode* drawing       = MapChildren( doc )["drawing"];
            NODE_MAP drawingChildren = MapChildren( drawing );

            // clear the cu map and then rebuild it.
            clear_cu_map();

            m_xpath->push( "eagle.drawing.layers" );
            wxXmlNode* layers  = drawingChildren["layers"];
            loadLayerDefs( layers );
            m_xpath->pop();

            m_xpath->push( "eagle.drawing.library" );
            wxXmlNode* library = drawingChildren["library"];
            loadLibrary( library, NULL );
            m_xpath->pop();

            m_mod_time = modtime;
        }
    }
    catch(...){}
    // TODO: Handle exceptions
    // catch( file_parser_error fpe )
    // {
    //     // for xml_parser_error, what() has the line number in it,
    //     // but no byte offset.  That should be an adequate error message.
    //     THROW_IO_ERROR( fpe.what() );
    // }
    //
    // // Class ptree_error is a base class for xml_parser_error & file_parser_error,
    // // so one catch should be OK for all errors.
    // catch( ptree_error pte )
    // {
    //     string errmsg = pte.what();
    //
    //     errmsg += " @\n";
    //     errmsg += m_xpath->Contents();
    //
    //     THROW_IO_ERROR( errmsg );
    // }
}


wxArrayString EAGLE_PLUGIN::FootprintEnumerate( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    init( aProperties );

    cacheLib( aLibraryPath );

    wxArrayString   ret;

    for( MODULE_CITER it = m_templates.begin();  it != m_templates.end();  ++it )
        ret.Add( FROM_UTF8( it->first.c_str() ) );

    return ret;
}


MODULE* EAGLE_PLUGIN::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
        const PROPERTIES* aProperties )
{
    init( aProperties );

    cacheLib( aLibraryPath );

    string key = TO_UTF8( aFootprintName );

    MODULE_CITER mi = m_templates.find( key );

    if( mi == m_templates.end() )
        return NULL;

    // copy constructor to clone the template
    MODULE* ret = new MODULE( *mi->second );

    return ret;
}


void EAGLE_PLUGIN::FootprintLibOptions( PROPERTIES* aListToAppendTo ) const
{
    PLUGIN::FootprintLibOptions( aListToAppendTo );

    /*
    (*aListToAppendTo)["ignore_duplicates"] = UTF8( _(
        "Ignore duplicately named footprints within the same Eagle library. "
        "Only the first similarly named footprint will be loaded."
        ));
    */
}


/*
void EAGLE_PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, const PROPERTIES* aProperties )
{
    // Eagle lovers apply here.
}


void EAGLE_PLUGIN::FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint, const PROPERTIES* aProperties )
{
}


void EAGLE_PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName )
{
}


void EAGLE_PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
}


bool EAGLE_PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
}


bool EAGLE_PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    return true;
}

*/
