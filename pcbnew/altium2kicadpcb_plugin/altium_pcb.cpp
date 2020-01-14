/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include "altium_pcb.h"
#include "altium_parser_binary.h"

#include <class_board.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>

#include <compoundfilereader.h>
#include <utf.h>


const CFB::COMPOUND_FILE_ENTRY* FindStream(const CFB::CompoundFileReader& reader, const char* streamName)
{
    const CFB::COMPOUND_FILE_ENTRY* ret = nullptr;
    reader.EnumFiles(reader.GetRootEntry(), -1,
                     [&](const CFB::COMPOUND_FILE_ENTRY* entry, const CFB::utf16string& u16dir, int level)->void
                     {
                         if (reader.IsStream(entry))
                         {
                             std::string name = UTF16ToUTF8(entry->name);
                             if (u16dir.length() > 0)
                             {
                                 std::string dir = UTF16ToUTF8(u16dir.c_str());
                                 if (strncmp(streamName, dir.c_str(), dir.length()) == 0 &&
                                     streamName[dir.length()] == '\\' &&
                                     strcmp(streamName + dir.length() + 1, name.c_str()) == 0)
                                 {
                                     ret = entry;
                                 }
                             }
                             else
                             {
                                 if (strcmp(streamName, name.c_str()) == 0)
                                 {
                                     ret = entry;
                                 }
                             }
                         }
                     });
    return ret;
}


struct ALTIUM_RECORD
{
    enum
    {
        ARC                 = 1,
        PAD                 = 2,
        VIA                 = 3,
        TRACK               = 4,
        TEXT                = 5,
        FILL                = 6,
        REGION              = 11,
        MODEL               = 12
    };
};


struct ALTIUM_PAD_SHAPE
{
    enum
    {
        UNKNOWN             = 0,
        CIRCLE              = 1,
        RECT                = 2,
        OVAL                = 3
    };
};


struct ALTIUM_PAD_MODE
{
    enum
    {
        SIMPLE              = 0,
        TOP_MIDDLE_BOTTOM   = 1,
        FULL_STACK          = 2
    };
};


struct ALTIUM_LAYER
{
    enum
    {
        F_CU                = 1,
        IN4_CU              = 2,
        IN2_CU              = 3,
        IN3_CU              = 4,
        IN5_CU              = 6,
        IN6_CU              = 11,
        IN7_CU              = 12,
        B_CU                = 32,

        F_SILKS             = 33,
        B_SILKS             = 34,
        F_PASTE             = 35,
        B_PASTE             = 36,

        EDGE_CUTS           = 56,

        MULTILAYER          = 74
    };
};

PCB_LAYER_ID ALTIUM_PCB::kicad_layer( int aAltiumLayer ) const {
    switch( aAltiumLayer )
    {
        case ALTIUM_LAYER::F_CU:            return F_Cu;
        case ALTIUM_LAYER::IN2_CU:          return In2_Cu;
        case ALTIUM_LAYER::IN3_CU:          return In3_Cu;
        case ALTIUM_LAYER::IN4_CU:          return In4_Cu;
        case ALTIUM_LAYER::IN5_CU:          return In5_Cu;
        case ALTIUM_LAYER::IN6_CU:          return In6_Cu;
        case ALTIUM_LAYER::IN7_CU:          return In7_Cu;
        case ALTIUM_LAYER::B_CU:            return B_Cu;

        case ALTIUM_LAYER::F_SILKS:         return F_SilkS;
        case ALTIUM_LAYER::B_SILKS:         return B_SilkS;
        case ALTIUM_LAYER::F_PASTE:         return F_Paste;
        case ALTIUM_LAYER::B_PASTE:         return B_Paste;
        case ALTIUM_LAYER::EDGE_CUTS:       return Edge_Cuts;

        default:                            return UNDEFINED_LAYER;
    }
}

ALTIUM_PCB::ALTIUM_PCB(BOARD *aBoard) {
    m_board = aBoard;
}

ALTIUM_PCB::~ALTIUM_PCB() {

}

void ALTIUM_PCB::Parse( const CFB::CompoundFileReader& aReader ) {
    // Parse file header
    const CFB::COMPOUND_FILE_ENTRY* fileHeader = FindStream(aReader, "FileHeader");
    wxASSERT( fileHeader != nullptr );
    if (fileHeader != nullptr)
    {
        ParseFileHeader(aReader, fileHeader);
    }

    // Parse arcs
    const CFB::COMPOUND_FILE_ENTRY* arcs6 = FindStream(aReader, "Arcs6\\Data");
    wxASSERT( arcs6 != nullptr );
    if (arcs6 != nullptr)
    {
        ParseArcs6Data(aReader, arcs6);
    }

    // Parse pads
    const CFB::COMPOUND_FILE_ENTRY* pads6 = FindStream(aReader, "Pads6\\Data");
    wxASSERT( pads6 != nullptr );
    if (pads6 != nullptr)
    {
        ParsePads6Data(aReader, pads6);
    }

    // Parse vias
    const CFB::COMPOUND_FILE_ENTRY* vias6 = FindStream(aReader, "Vias6\\Data");
    wxASSERT( vias6 != nullptr );
    if (vias6 != nullptr)
    {
        ParseVias6Data(aReader, vias6);
    }

    // Parse tracks
    const CFB::COMPOUND_FILE_ENTRY* tracks6 = FindStream(aReader, "Tracks6\\Data");
    wxASSERT( tracks6 != nullptr );
    if (tracks6 != nullptr)
    {
        ParseTracks6Data(aReader, tracks6);
    }

    // Parse texts
    const CFB::COMPOUND_FILE_ENTRY* texts6 = FindStream(aReader, "Texts6\\Data");
    wxASSERT( texts6 != nullptr );
    if (texts6 != nullptr)
    {
        ParseTexts6Data(aReader, texts6);
    }
}

MODULE* ALTIUM_PCB::GetComponent( const u_int16_t id) {
    // I asume this is a special case where a elements belongs to the board.
    if( id == std::numeric_limits<u_int16_t>::max() ) {
        MODULE* module = new MODULE( m_board );
        m_board->Add(module);
        return module;
    }

    MODULE *module = m_components.size() > id ? m_components.at( id ) : nullptr;
    if ( module == nullptr )
    {
        module = new MODULE(m_board );
        m_board->Add(module);
        if (id >= m_components.size()) {
            m_components.resize(id + 1, nullptr);
        }
        m_components.insert(m_components.begin() + id, module );
    }
    return module;
}

void ALTIUM_PCB::ParseFileHeader( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry ) {
    ALTIUM_PARSER_BINARY reader(aReader, aEntry);

    reader.read_subrecord_length();
    std::cout << "HEADER: " << reader.read_string() << std::endl;  // tells me: PCB 5.0 Binary File

    //reader.subrecord_skip();

    // TODO: does not seem to work all the time at the moment
    //wxASSERT(!reader.parser_error());
    //wxASSERT(reader.bytes_remaining() == 0);
}

void ALTIUM_PCB::ParseArcs6Data( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry ) {
    ALTIUM_PARSER_BINARY reader( aReader, aEntry );

    while( !reader.parser_error() && reader.bytes_remaining() >= 4 /* TODO: use Header section of file */ ) {
        u_int8_t recordtype = reader.read<u_int8_t>();
        wxASSERT( recordtype == ALTIUM_RECORD::ARC );

        // Subrecord 1
        reader.read_subrecord_length();

        u_int8_t layer = reader.read<u_int8_t>();
        reader.skip(6);
        u_int16_t component = reader.read<u_int16_t>();
        reader.skip(4);
        wxPoint center = reader.read_point();
        u_int32_t radius = ALTIUM_PARSER_BINARY::kicad_unit( reader.read<u_int32_t>() );
        double startangle = reader.read<double>();
        double endangle = reader.read<double>();
        u_int32_t width = ALTIUM_PARSER_BINARY::kicad_unit( reader.read<u_int32_t>() );

        // TODO: better approach to select if item belongs to a MODULE
        DRAWSEGMENT* ds = nullptr;
        if (component == std::numeric_limits<u_int16_t>::max()) {
            ds = new DRAWSEGMENT( m_board );
            m_board->Add( ds );
        } else {
            MODULE* module = GetComponent( component );
            ds = new DRAWSEGMENT( module );
            m_board->Add( ds );  // TODO: why cannot I add it to MODULE?
        }

        ds->SetCenter( center );
        ds->SetWidth( width );
        PCB_LAYER_ID klayer = kicad_layer( layer );
        ds->SetLayer( klayer != UNDEFINED_LAYER ? klayer : Eco1_User );

        if (startangle == 0. && endangle == 360. ) {  // TODO: other variants to define circle?
            ds->SetShape( STROKE_T::S_CIRCLE );
            ds->SetArcStart( center -  wxPoint( 0, radius ));
        } else {
            ds->SetShape( STROKE_T::S_ARC );

            // TODO: something of this calculation seems wrong. Sometimes start is 90, sometimes 180deg wrong
            double angle = endangle < startangle ? 360. + endangle - startangle : endangle - startangle;
            ds->SetAngle( angle * 10. );

            double startradiant = startangle * M_PI / 180;
            wxPoint arcStartOffset = wxPoint(
                    static_cast<int32_t>(std::cos(startradiant) * radius),
                    static_cast<int32_t>(std::sin(startradiant) * radius) );
            ds->SetArcStart( center + arcStartOffset);  // TODO
        }

        reader.subrecord_skip();

        wxASSERT(!reader.parser_error());
    }

    wxASSERT( !reader.parser_error() );
    wxASSERT( reader.bytes_remaining() == 0 );
}

void ALTIUM_PCB::ParsePads6Data( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry ) {
    ALTIUM_PARSER_BINARY reader( aReader, aEntry );

    while( !reader.parser_error() && reader.bytes_remaining() >= 4*6 /* TODO: use Header section of file */ ) {
        u_int8_t recordtype = reader.read<u_int8_t>();
        wxASSERT( recordtype == ALTIUM_RECORD::PAD );

        // Subrecord 1
        size_t subrecord1 = reader.read_subrecord_length();
        wxASSERT( subrecord1 > 0 );
        std::string name = reader.read_string();
        wxASSERT( reader.subrecord_remaining() == 0 );
        reader.subrecord_skip();

        // Subrecord 2
        reader.read_subrecord_length();
        reader.subrecord_skip();

        // Subrecord 3
        reader.read_subrecord_length();
        reader.subrecord_skip();

        // Subrecord 4
        reader.read_subrecord_length();
        reader.subrecord_skip();

        // Subrecord 5
        size_t subrecord5 = reader.read_subrecord_length();
        wxASSERT( subrecord5 >= 120 );  // TODO: exact minimum length we know?

        u_int8_t layer = reader.read<u_int8_t>();
        reader.skip( 6 );
        u_int16_t component = reader.read<u_int16_t>();
        reader.skip( 4 );

        wxPoint position = reader.read_point();
        wxSize topsize = reader.read_size();
        wxSize midsize = reader.read_size();
        wxSize botsize = reader.read_size();

        u_int32_t holesize = ALTIUM_PARSER_BINARY::kicad_unit( reader.read<u_int32_t>() );
        u_int8_t topshape = reader.read<u_int8_t>();
        u_int8_t midshape = reader.read<u_int8_t>();
        u_int8_t botshape = reader.read<u_int8_t>();

        double direction = reader.read<double>();
        u_int8_t plated = reader.read<u_int8_t>();
        reader.skip( 1 );
        u_int8_t padmode = reader.read<u_int8_t>();
        reader.skip( 38 );
        u_int8_t pastemaskexpansionmode = reader.read<u_int8_t>();
        u_int8_t soldermaskexpansion = reader.read<u_int8_t>();
        reader.skip( 3 );
        double holerotation = reader.read<double>();

        // Create Pad
        MODULE *module = GetComponent( component );
        D_PAD *pad = new D_PAD( module );
        module->Add( pad );

        pad->SetName( name );
        pad->SetPosition( position );
        pad->SetSize( topsize );
        pad->SetOrientation( direction * 10. );
        if ( holesize == 0 ) {
            wxASSERT( layer != ALTIUM_LAYER::MULTILAYER );
            pad->SetAttribute( PAD_ATTR_T::PAD_ATTRIB_SMD );
        } else {
            wxASSERT( layer == ALTIUM_LAYER::MULTILAYER );  // TODO: I assume other values are possible as well?
            pad->SetAttribute( plated ? PAD_ATTR_T::PAD_ATTRIB_STANDARD : PAD_ATTR_T::PAD_ATTRIB_HOLE_NOT_PLATED );
            pad->SetDrillSize( wxSize(holesize, holesize) );
        }

        wxASSERT( padmode == ALTIUM_PAD_MODE::SIMPLE );
        // wxASSERT( topshape == midshape == botshape );
        switch ( topshape ) {
            case ALTIUM_PAD_SHAPE::RECT:
                pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_RECT );
                break;
            case ALTIUM_PAD_SHAPE::CIRCLE:
                pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_CIRCLE );
                break;
            case ALTIUM_PAD_SHAPE::OVAL:
                pad->SetShape( PAD_SHAPE_T::PAD_SHAPE_OVAL );
                break;
            case ALTIUM_PAD_SHAPE::UNKNOWN:
            default:
                wxFAIL_MSG("unknown shape");
                break;
        }

        switch ( layer ) {
            case ALTIUM_LAYER::F_CU:
                pad->SetLayer( F_Cu );
                pad->SetLayerSet( LSET( 3, F_Cu, F_Paste, F_Mask ) );
                break;
            case ALTIUM_LAYER::B_CU:
                pad->SetLayer( B_Cu );
                pad->SetLayerSet( LSET( 3, B_Cu, B_Paste, B_Mask ) );
                break;
            case ALTIUM_LAYER::MULTILAYER:
            default:
                pad->SetLayerSet( LSET::AllCuMask() );
                pad->SetLayerSet( pad->GetLayerSet().set( B_Mask).set( F_Mask ) ); // Solder Mask
                break;
        }

        if ( subrecord5 == 120 ) {
            u_int8_t tolayer = reader.read<u_int8_t>();
            reader.skip( 2 );
            u_int8_t fromlayer = reader.read<u_int8_t>();
            //reader.skip( 2 );
        } else if ( subrecord5 == 171 ) {

        }
        reader.subrecord_skip();

        // Subrecord 6
        reader.read_subrecord_length();
        reader.subrecord_skip();

        wxASSERT(!reader.parser_error());
    }

    wxASSERT( !reader.parser_error() );
    wxASSERT( reader.bytes_remaining() == 0 );
}

void ALTIUM_PCB::ParseVias6Data( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry ) {
    ALTIUM_PARSER_BINARY reader( aReader, aEntry );

    while( !reader.parser_error() && reader.bytes_remaining() >= 213 /* TODO: use Header section of file */ ) {
        u_int8_t recordtype = reader.read<u_int8_t>();
        wxASSERT( recordtype == ALTIUM_RECORD::VIA );

        reader.read_subrecord_length();
        reader.skip( 13 );
        wxPoint position = reader.read_point();
        u_int32_t diameter = ALTIUM_PARSER_BINARY::kicad_unit( reader.read<u_int32_t>() );
        u_int32_t holesize = ALTIUM_PARSER_BINARY::kicad_unit( reader.read<u_int32_t>() );

        VIA *via = new VIA( m_board );
        m_board->Add( via );

        via->SetPosition( position );
        via->SetWidth( diameter );
        via->SetDrill( holesize );
        via->SetViaType( VIATYPE::THROUGH ); // TODO

        reader.subrecord_skip();
        wxASSERT( !reader.parser_error() );
    }

    wxASSERT( !reader.parser_error() );
    wxASSERT( reader.bytes_remaining() == 0 );
}

void ALTIUM_PCB::ParseTracks6Data( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry ) {
    ALTIUM_PARSER_BINARY reader( aReader, aEntry );

    while( !reader.parser_error() && reader.bytes_remaining() >= 4 /* TODO: use Header section of file */ ) {
        u_int8_t recordtype = reader.read<u_int8_t>();
        wxASSERT( recordtype == ALTIUM_RECORD::TRACK );

        reader.read_subrecord_length();
        u_int8_t layer = reader.read<u_int8_t>();
        reader.skip( 12 );
        wxPoint start = reader.read_point();
        wxPoint end = reader.read_point();
        u_int32_t width = ALTIUM_PARSER_BINARY::kicad_unit( reader.read<u_int32_t>() );

        PCB_LAYER_ID klayer = kicad_layer( layer );
        if( klayer >= F_Cu && klayer <= B_Cu )
        {
            TRACK* track = new TRACK( m_board );
            m_board->Add( track );

            track->SetStart( start );
            track->SetEnd( end );
            track->SetWidth( width );
            track->SetLayer( klayer );
        }
        else
        {
            DRAWSEGMENT* ds = new DRAWSEGMENT( m_board );
            m_board->Add( ds );

            ds->SetStart( start );
            ds->SetEnd( end );
            ds->SetWidth( width );

            ds->SetLayer( klayer != UNDEFINED_LAYER ? klayer : Eco1_User );
        }

        reader.subrecord_skip();
    }

    wxASSERT( !reader.parser_error() );
    wxASSERT( reader.bytes_remaining() == 0 );
}

void ALTIUM_PCB::ParseTexts6Data( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry ) {
    ALTIUM_PARSER_BINARY reader( aReader, aEntry );

    while( !reader.parser_error() && reader.bytes_remaining() >= 4 /* TODO: use Header section of file */ ) {
        u_int8_t recordtype = reader.read<u_int8_t>();
        wxASSERT( recordtype == ALTIUM_RECORD::TEXT );

        // Subrecord 1 - Properties
        reader.read_subrecord_length();

        u_int8_t layer = reader.read<u_int8_t>();
        reader.skip(6);
        u_int16_t component = reader.read<u_int16_t>();
        reader.skip(4);
        wxPoint position = reader.read_point();
        u_int32_t height = ALTIUM_PARSER_BINARY::kicad_unit( reader.read<u_int32_t>() );
        reader.skip(2);
        double rotation = reader.read<double>();

        reader.subrecord_skip();

        // Subrecord 2 - String
        reader.read_subrecord_length();

        std::string text = reader.read_string(); // TODO: what about strings with length > 255?

        reader.subrecord_skip();
        wxASSERT( reader.subrecord_remaining() == 0 );

        // TODO: better approach to select if item belongs to a MODULE
        TEXTE_PCB* tx = nullptr;
        if (component == std::numeric_limits<u_int16_t>::max()) {
            tx = new TEXTE_PCB(m_board );
            m_board->Add(tx );
        } else {
            MODULE* module = GetComponent( component );
            tx = new TEXTE_PCB(module );
            m_board->Add(tx );  // TODO: why cannot I add it to MODULE?
        }

        tx->SetPosition( position );
        tx->SetTextHeight( height );
        tx->SetTextAngle( rotation * 10. );
        PCB_LAYER_ID klayer = kicad_layer( layer );
        tx->SetLayer( klayer != UNDEFINED_LAYER ? klayer : Eco1_User );
        tx->SetText( text );
        tx->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT ); // TODO: what byte

        wxASSERT(!reader.parser_error());
    }

    wxASSERT( !reader.parser_error() );
    wxASSERT( reader.bytes_remaining() == 0 );
}