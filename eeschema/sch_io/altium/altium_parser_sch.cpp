/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include <iostream>
#include <unordered_map>
#include <charconv>

#include <base_units.h>
#include <ki_exception.h>

#include <wx/log.h>

#include "sch_io/altium/altium_parser_sch.h"
#include "io/altium/altium_binary_parser.h"
#include "io/altium/altium_props_utils.h"


ALTIUM_SCH_RECORD ReadRecord( const std::map<wxString, wxString>& aProps )
{
    int recordId = ALTIUM_PROPS_UTILS::ReadInt( aProps, "RECORD", -1 );
    return static_cast<ALTIUM_SCH_RECORD>( recordId );
}


constexpr int Altium2KiCadUnit( const int val, const int frac )
{
    constexpr double int_limit = ( std::numeric_limits<int>::max() - 10 ) / 2.54;

    double dbase = 10 * schIUScale.MilsToIU( val );
    double dfrac = schIUScale.MilsToIU( frac ) / 10000.0;

    return KiROUND( std::clamp( ( dbase + dfrac ) / 10.0, -int_limit, int_limit ) ) * 10;
}


int ReadKiCadUnitFrac( const std::map<wxString, wxString>& aProps, const wxString& aKey )
{
    // a unit is stored using two fields, denoting the size in mils and a fraction size
    int key     = ALTIUM_PROPS_UTILS::ReadInt( aProps, aKey, 0 );
    int keyFrac = ALTIUM_PROPS_UTILS::ReadInt( aProps, aKey + "_FRAC", 0 );
    return Altium2KiCadUnit( key, keyFrac );
}


int ReadKiCadUnitFrac1( const std::map<wxString, wxString>& aProps, const wxString& aKey )
{
    // a unit is stored using two fields, denoting the size in mils and a fraction size
    // Dunno why Altium invents different units for the same purpose
    int key     = ALTIUM_PROPS_UTILS::ReadInt( aProps, aKey, 0 );
    int keyFrac = ALTIUM_PROPS_UTILS::ReadInt( aProps, aKey + "_FRAC1", 0 );
    return Altium2KiCadUnit( key * 10, keyFrac );
}


int ReadOwnerIndex( const std::map<wxString, wxString>& aProperties )
{
    return ALTIUM_PROPS_UTILS::ReadInt( aProperties, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
}


int ReadOwnerPartId( const std::map<wxString, wxString>& aProperties )
{
    return ALTIUM_PROPS_UTILS::ReadInt( aProperties, "OWNERPARTID", ALTIUM_COMPONENT_NONE );
}


template <typename T>
T ReadEnum( const std::map<wxString, wxString>& aProps, const wxString& aKey, int aLower,
            int aUpper, T aDefault )
{
    int value = ALTIUM_PROPS_UTILS::ReadInt( aProps, aKey, static_cast<int>( aDefault ) );

    if( value < aLower || value > aUpper )
        return aDefault;
    else
        return static_cast<T>( value );
}


ASCH_STORAGE_FILE::ASCH_STORAGE_FILE( const std::map<wxString, wxString>& aProps )
{
    filename = ALTIUM_PROPS_UTILS::ReadString( aProps, "NAME", "" );
    size_t dataSize = ALTIUM_PROPS_UTILS::ReadInt( aProps, "DATA_LEN", 0 );

    wxString     hexData = ALTIUM_PROPS_UTILS::ReadString( aProps, "DATA", "" );
    const size_t charCount = hexData.size();

    if( charCount != dataSize * 2 )
    {
        THROW_IO_ERROR( wxString::Format( "Invalid binary file hex data size. Chars expected: %d, "
                                          "hex string length: %d",
                                          int( dataSize * 2 ), int( hexData.size() ) ) );
    }

    data.resize( dataSize );

    char    str[3] = { 0 };
    uint8_t b = 0;
    size_t  outputId = 0;

    for( size_t inputId = 1; inputId < charCount; inputId += 2 )
    {
        str[0] = (char) hexData[inputId - 1];
        str[1] = (char) hexData[inputId];

        std::from_chars( str, str + 2, b, 16 );
        data[outputId] = b;

        outputId++;
    }
}


ASCH_STORAGE_FILE::ASCH_STORAGE_FILE( ALTIUM_BINARY_PARSER& aReader )
{
    aReader.Skip( 5 );
    filename = aReader.ReadWxString();
    uint32_t dataSize = aReader.Read<uint32_t>();
    data = aReader.ReadVector( dataSize );

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( "Storage stream was not parsed correctly" );
}


ASCH_ADDITIONAL_FILE::ASCH_ADDITIONAL_FILE( ALTIUM_BINARY_PARSER& aReader )
{
    aReader.Skip( 5 );
    FileName = aReader.ReadWxString();
    uint32_t dataSize = aReader.Read<uint32_t>();
    Data = aReader.ReadVector( dataSize );

    if( aReader.HasParsingError() )
        THROW_IO_ERROR( "Additional stream was not parsed correctly" );
}


ASCH_SYMBOL::ASCH_SYMBOL( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::COMPONENT );

    uniqueid = ALTIUM_PROPS_UTILS::ReadString( aProps, "UNIQUEID", "" );
    currentpartid = ALTIUM_PROPS_UTILS::ReadInt( aProps, "CURRENTPARTID", ALTIUM_COMPONENT_NONE );
    libreference = ALTIUM_PROPS_UTILS::ReadString( aProps, "LIBREFERENCE", "" );
    sourcelibraryname = ALTIUM_PROPS_UTILS::ReadString( aProps, "SOURCELIBRARYNAME", "" );
    componentdescription = ALTIUM_PROPS_UTILS::ReadString( aProps, "COMPONENTDESCRIPTION", "" );

    orientation = ALTIUM_PROPS_UTILS::ReadInt( aProps, "ORIENTATION", 0 );
    isMirrored = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISMIRRORED", false );
    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    partcount = ALTIUM_PROPS_UTILS::ReadInt( aProps, "PARTCOUNT", 0 );
    displaymodecount = ALTIUM_PROPS_UTILS::ReadInt( aProps, "DISPLAYMODECOUNT", 0 );
    m_indexInSheet = ALTIUM_PROPS_UTILS::ReadInt( aProps, "INDEXINSHEET", -1 );

    // DISPLAYMODE may be a string. Leave displaymode at 0 in this case.
    displaymode = 0;
    wxString displayModeStr = ALTIUM_PROPS_UTILS::ReadString( aProps, "DISPLAYMODE", "" );

    long v = 0;

    if( displayModeStr.ToCLong( &v ) )
        displaymode = int( v );
}


ASCH_TEMPLATE::ASCH_TEMPLATE( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::TEMPLATE );

    filename = ALTIUM_PROPS_UTILS::ReadString( aProps, "FILENAME", "" );
}


ASCH_PIN::ASCH_PIN( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::PIN );

    ownerpartdisplaymode = ALTIUM_PROPS_UTILS::ReadInt( aProps, "OWNERPARTDISPLAYMODE", 0 );

    name       = ALTIUM_PROPS_UTILS::ReadString( aProps, "NAME", "" );
    text       = ALTIUM_PROPS_UTILS::ReadString( aProps, "TEXT", "" );
    designator = ALTIUM_PROPS_UTILS::ReadString( aProps, "DESIGNATOR", "" );

    int symbolOuterInt = ALTIUM_PROPS_UTILS::ReadInt( aProps, "SYMBOL_OUTER", 0 );
    symbolOuter        = ASCH_PIN_SYMBOL::FromInt( symbolOuterInt );

    int symbolInnerInt = ALTIUM_PROPS_UTILS::ReadInt( aProps, "SYMBOL_INNER", 0 );
    symbolInner        = ASCH_PIN_SYMBOL::FromInt( symbolInnerInt );

    int symbolOuterEdgeInt = ALTIUM_PROPS_UTILS::ReadInt( aProps, "SYMBOL_OUTEREDGE", 0 );
    symbolOuterEdge        = ASCH_PIN_SYMBOL::FromInt( symbolOuterEdgeInt );

    int symbolInnerEdgeInt = ALTIUM_PROPS_UTILS::ReadInt( aProps, "SYMBOL_INNEREDGE", 0 );
    symbolInnerEdge        = ASCH_PIN_SYMBOL::FromInt( symbolInnerEdgeInt );

    electrical = ReadEnum<ASCH_PIN_ELECTRICAL>( aProps, "ELECTRICAL", 0, 7,
                                                ASCH_PIN_ELECTRICAL::PIN_INPUT );

    int pinconglomerate = ALTIUM_PROPS_UTILS::ReadInt( aProps, "PINCONGLOMERATE", 0 );

    orientation    = static_cast<ASCH_RECORD_ORIENTATION>( pinconglomerate & 0x03 );
    hidden         = ( pinconglomerate & 0x04 ) != 0;
    showPinName    = ( pinconglomerate & 0x08 ) != 0;
    showDesignator = ( pinconglomerate & 0x10 ) != 0;
    // 0x20 is unknown
    locked         = ( pinconglomerate & 0x40 ) != 0;


    int x     = ALTIUM_PROPS_UTILS::ReadInt( aProps, "LOCATION.X", 0 );
    int xfrac = ALTIUM_PROPS_UTILS::ReadInt( aProps, "LOCATION.X_FRAC", 0 );
    int y     = ALTIUM_PROPS_UTILS::ReadInt( aProps, "LOCATION.Y", 0 );
    int yfrac = ALTIUM_PROPS_UTILS::ReadInt( aProps, "LOCATION.Y_FRAC", 0 );
    location  = VECTOR2I( Altium2KiCadUnit( x, xfrac ), -Altium2KiCadUnit( y, yfrac ) );

    int p     = ALTIUM_PROPS_UTILS::ReadInt( aProps, "PINLENGTH", 0 );
    int pfrac = ALTIUM_PROPS_UTILS::ReadInt( aProps, "PINLENGTH_FRAC", 0 );
    pinlength = Altium2KiCadUnit( p, pfrac );

    // this code calculates the location as required by KiCad without rounding error attached
    int kicadX     = x;
    int kicadXfrac = xfrac;
    int kicadY     = y;
    int kicadYfrac = yfrac;

    int offsetY = p;
    int offsetYfrac = pfrac;

    switch( orientation )
    {
    case ASCH_RECORD_ORIENTATION::RIGHTWARDS:
        kicadX += offsetY;
        kicadXfrac += offsetYfrac;
        break;

    case ASCH_RECORD_ORIENTATION::UPWARDS:
        kicadY += offsetY;
        kicadYfrac += offsetYfrac;
        break;

    case ASCH_RECORD_ORIENTATION::LEFTWARDS:
        kicadX -= offsetY;
        kicadXfrac -= offsetYfrac;
        break;

    case ASCH_RECORD_ORIENTATION::DOWNWARDS:
        kicadY -= offsetY;
        kicadYfrac -= offsetYfrac;
        break;

    default:
        wxLogWarning( "Pin has unexpected orientation" );
        break;
    }

    kicadLocation = VECTOR2I( Altium2KiCadUnit( kicadX, kicadXfrac ),
                              -Altium2KiCadUnit( kicadY, kicadYfrac ) );
}


ASCH_OWNER_INTERFACE::ASCH_OWNER_INTERFACE( const std::map<wxString, wxString>& aProps )
{
    ownerindex = ReadOwnerIndex( aProps );
    ownerpartid = ReadOwnerPartId( aProps );
    ownerpartdisplaymode = ALTIUM_PROPS_UTILS::ReadInt( aProps, "OWNERPARTDISPLAYMODE", 0 );
    indexinsheet = ALTIUM_PROPS_UTILS::ReadInt( aProps, "INDEXINSHEET", 0 );
    IsNotAccesible = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISNOTACCESIBLE", false );
}


ASCH_FILL_INTERFACE::ASCH_FILL_INTERFACE( const std::map<wxString, wxString>& aProps )
{
    AreaColor = ALTIUM_PROPS_UTILS::ReadInt( aProps, "AREACOLOR", 0 );
    IsSolid   = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISSOLID", false );
    IsTransparent = ALTIUM_PROPS_UTILS::ReadBool( aProps, "TRANSPARENT", false );
}


ASCH_BORDER_INTERFACE::ASCH_BORDER_INTERFACE( const std::map<wxString, wxString>& aProps )
{
    LineWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );

    // Altium line width 0 means hairline.  Since KiCad doesn't have a hairline, we
    // represent it as a 1 mil line.
    if( LineWidth == 0 )
        LineWidth = schIUScale.MilsToIU( 1 );

    Color = ALTIUM_PROPS_UTILS::ReadInt( aProps, "COLOR", 0 );
}


ASCH_LABEL::ASCH_LABEL( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::LABEL );

    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    text = ALTIUM_PROPS_UTILS::ReadString( aProps, "TEXT", "" );

    textColor  = 0;
    fontId     = ALTIUM_PROPS_UTILS::ReadInt( aProps, "FONTID", 0 );
    isMirrored = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISMIRRORED", false );

    justification = ReadEnum<ASCH_LABEL_JUSTIFICATION>( aProps, "JUSTIFICATION", 0, 8,
                                                        ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );
}


ASCH_TEXT_FRAME::ASCH_TEXT_FRAME( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::NOTE
              || ReadRecord( aProps ) == ALTIUM_SCH_RECORD::TEXT_FRAME );

    BottomLeft = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                           -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    TopRight = VECTOR2I( ReadKiCadUnitFrac( aProps, "CORNER.X" ),
                         -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) );

    Location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    Size = VECTOR2I( ReadKiCadUnitFrac( aProps, "CORNER.X" ) - Location.x,
                    -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) - Location.y );

    Text = ALTIUM_PROPS_UTILS::ReadString( aProps, "TEXT", "" );
    Text.Replace( "~1", "\n", true );

    FontID = ALTIUM_PROPS_UTILS::ReadInt( aProps, "FONTID", 0 );
    IsWordWrapped = ALTIUM_PROPS_UTILS::ReadBool( aProps, "WORDWRAP", false );
    ShowBorder = ALTIUM_PROPS_UTILS::ReadBool( aProps, "SHOWBORDER", false );
    TextMargin = ReadKiCadUnitFrac( aProps, "TEXTMARGIN" );

    AreaColor = ALTIUM_PROPS_UTILS::ReadInt( aProps, "AREACOLOR", 0 );
    BorderColor = ALTIUM_PROPS_UTILS::ReadInt( aProps, "COLOR", 0 );
    TextColor = ALTIUM_PROPS_UTILS::ReadInt( aProps, "TEXTCOLOR", 0 );

    BorderWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
    isSolid = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISSOLID", false );

    Alignment = ReadEnum<ASCH_TEXT_FRAME_ALIGNMENT>( aProps, "ALIGNMENT", 1, 3,
                                                     ASCH_TEXT_FRAME_ALIGNMENT::LEFT );
}


ASCH_NOTE::ASCH_NOTE( const std::map<wxString, wxString>& aProperties ) :
        ASCH_TEXT_FRAME( aProperties )
{
    wxASSERT( ReadRecord( aProperties ) == ALTIUM_SCH_RECORD::NOTE );

    author = ALTIUM_PROPS_UTILS::ReadString( aProperties, "AUTHOR", "" );
}


ASCH_BEZIER::ASCH_BEZIER( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps ),
        ASCH_BORDER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::BEZIER );

    int locationCount = ALTIUM_PROPS_UTILS::ReadInt( aProps, "LOCATIONCOUNT", 0 );

    for( int i = 1; i <= locationCount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( ReadKiCadUnitFrac( aProps, "X" + si ),
                             -ReadKiCadUnitFrac( aProps, "Y" + si ) );
    }
}


ASCH_POLYLINE::ASCH_POLYLINE( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps ),
        ASCH_BORDER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::POLYLINE );

    int locationCount = ALTIUM_PROPS_UTILS::ReadInt( aProps, "LOCATIONCOUNT", 0 );

    for( int i = 1; i <= locationCount; i++ )
    {
        const wxString si = std::to_string( i );
        Points.emplace_back( ReadKiCadUnitFrac( aProps, "X" + si ),
                             -ReadKiCadUnitFrac( aProps, "Y" + si ) );
    }

    auto lineStyleExt = ReadEnum( aProps, "LINESTYLEEXT", 0, 3, ASCH_POLYLINE_LINESTYLE::SOLID );
    LineStyle = ReadEnum( aProps, "LINESTYLE", 0, 3, lineStyleExt ); // overwrite if present.
}


ASCH_POLYGON::ASCH_POLYGON( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps ),
        ASCH_FILL_INTERFACE( aProps ),
        ASCH_BORDER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::POLYGON );

    int locationCount = ALTIUM_PROPS_UTILS::ReadInt( aProps, "LOCATIONCOUNT", 0 );

    for( int i = 1; i <= locationCount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( ReadKiCadUnitFrac( aProps, "X" + si ),
                             -ReadKiCadUnitFrac( aProps, "Y" + si ) );
    }
}


ASCH_ROUND_RECTANGLE::ASCH_ROUND_RECTANGLE( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps ),
        ASCH_FILL_INTERFACE( aProps ),
        ASCH_BORDER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::ROUND_RECTANGLE );

    BottomLeft = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                           -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    TopRight = VECTOR2I( ReadKiCadUnitFrac( aProps, "CORNER.X" ),
                         -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) );

    CornerRadius = VECTOR2I( ReadKiCadUnitFrac( aProps, "CORNERXRADIUS" ),
                             -ReadKiCadUnitFrac( aProps, "CORNERYRADIUS" ) );
}


ASCH_ARC::ASCH_ARC( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps ),
        ASCH_BORDER_INTERFACE( aProps ),
        ASCH_FILL_INTERFACE( aProps )
{
    m_IsElliptical = ReadRecord( aProps ) == ALTIUM_SCH_RECORD::ELLIPTICAL_ARC;
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::ARC || m_IsElliptical );

    m_Center = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    m_Radius = ReadKiCadUnitFrac( aProps, "RADIUS" );
    m_SecondaryRadius = m_Radius;

    if( m_IsElliptical )
        m_SecondaryRadius = ReadKiCadUnitFrac( aProps, "SECONDARYRADIUS" );

    m_StartAngle = ALTIUM_PROPS_UTILS::ReadDouble( aProps, "STARTANGLE", 0 );
    m_EndAngle   = ALTIUM_PROPS_UTILS::ReadDouble( aProps, "ENDANGLE", 0 );
}


ASCH_PIECHART::ASCH_PIECHART( const std::map<wxString, wxString>& aProps ) :
        ASCH_ARC( aProps )
{}


ASCH_ELLIPSE::ASCH_ELLIPSE( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps ),
        ASCH_FILL_INTERFACE( aProps ),
        ASCH_BORDER_INTERFACE( aProps )
{
    ALTIUM_SCH_RECORD record = ReadRecord( aProps );

    wxASSERT( record == ALTIUM_SCH_RECORD::ELLIPSE || record == ALTIUM_SCH_RECORD::ELLIPTICAL_ARC );

    Center = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                       -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    Radius = ReadKiCadUnitFrac( aProps, "RADIUS" );
    SecondaryRadius = ReadKiCadUnitFrac( aProps, "SECONDARYRADIUS" );
}


ASCH_LINE::ASCH_LINE( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps ),
        ASCH_BORDER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::LINE );

    point1 = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                       -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    point2 = VECTOR2I( ReadKiCadUnitFrac( aProps, "CORNER.X" ),
                       -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) );

    auto lineStyleExt = ReadEnum( aProps, "LINESTYLEEXT", 0, 3, ASCH_POLYLINE_LINESTYLE::SOLID );
    LineStyle = ReadEnum( aProps, "LINESTYLE", 0, 3, lineStyleExt ); // overwrite if present.
}


ASCH_SIGNAL_HARNESS::ASCH_SIGNAL_HARNESS( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::SIGNAL_HARNESS );


    int locationCount = ALTIUM_PROPS_UTILS::ReadInt( aProps, "LOCATIONCOUNT", 0 );

    for( int i = 1; i <= locationCount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( ReadKiCadUnitFrac( aProps, "X" + si ),
                             -ReadKiCadUnitFrac( aProps, "Y" + si ) );
    }

    color = ALTIUM_PROPS_UTILS::ReadInt( aProps, "COLOR", 0 );
    lineWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
}


ASCH_HARNESS_CONNECTOR::ASCH_HARNESS_CONNECTOR( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::HARNESS_CONNECTOR );


    m_location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    m_size = VECTOR2I( ReadKiCadUnitFrac( aProps, "XSIZE" ), ReadKiCadUnitFrac( aProps, "YSIZE" ) );

    m_color = ALTIUM_PROPS_UTILS::ReadInt( aProps, "COLOR", 0 );
    m_areaColor = ALTIUM_PROPS_UTILS::ReadInt( aProps, "AREACOLOR", 0 );

    indexinsheet = 0;
    m_lineWidth = 0;;
    m_primaryConnectionPosition = ALTIUM_PROPS_UTILS::ReadInt( aProps, "PRIMARYCONNECTIONPOSITION", 0 );
    m_harnessConnectorSide = ReadEnum<ASCH_SHEET_ENTRY_SIDE>( aProps, "SIDE", 0, 3, ASCH_SHEET_ENTRY_SIDE::RIGHT );

}


ASCH_HARNESS_ENTRY::ASCH_HARNESS_ENTRY( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::HARNESS_ENTRY );

    // use SCH_IO_ALTIUM::m_harnessEntryParent instead, because this property sometimes
    // does not exist in altium file!
    // ownerindex = ReadOwnerIndex( aProps );


    DistanceFromTop = ReadKiCadUnitFrac1( aProps, "DISTANCEFROMTOP" );

    Side = ReadEnum<ASCH_SHEET_ENTRY_SIDE>( aProps, "SIDE", 0, 3, ASCH_SHEET_ENTRY_SIDE::LEFT );

    Name = ALTIUM_PROPS_UTILS::ReadString( aProps, "NAME", "" );

    OwnerIndexAdditionalList = ALTIUM_PROPS_UTILS::ReadBool( aProps, "OWNERINDEXADDITIONALLIST", true );

    Color = ALTIUM_PROPS_UTILS::ReadInt( aProps, "COLOR", 0 );
    AreaColor = ALTIUM_PROPS_UTILS::ReadInt( aProps, "AREACOLOR", 0 );
    TextColor = ALTIUM_PROPS_UTILS::ReadInt( aProps, "TEXTCOLOR", 0 );
    TextFontID = ALTIUM_PROPS_UTILS::ReadInt( aProps, "TEXTFONTID", 0 );
    TextStyle = 0;
}


ASCH_HARNESS_TYPE::ASCH_HARNESS_TYPE( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::HARNESS_TYPE );

    //ownerindex = ReadOwnerIndex( aProps ); // use SCH_IO_ALTIUM::m_harnessEntryParent instead!

    Text = ALTIUM_PROPS_UTILS::ReadString( aProps, "TEXT", "" );

    Location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    IsHidden = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISHIDDEN", false );
    OwnerIndexAdditionalList = ALTIUM_PROPS_UTILS::ReadBool( aProps, "OWNERINDEXADDITIONALLIST", true );

    Color = ALTIUM_PROPS_UTILS::ReadInt( aProps, "COLOR", 0 );
    FontID = ALTIUM_PROPS_UTILS::ReadInt( aProps, "TEXTFONTID", 0 );
}


ASCH_RECTANGLE::ASCH_RECTANGLE( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps ),
        ASCH_FILL_INTERFACE( aProps ),
        ASCH_BORDER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::RECTANGLE );

    BottomLeft = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                           -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    TopRight = VECTOR2I( ReadKiCadUnitFrac( aProps, "CORNER.X" ),
                         -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) );

}


ASCH_SHEET_SYMBOL::ASCH_SHEET_SYMBOL( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::SHEET_SYMBOL );

    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    size     = VECTOR2I( ReadKiCadUnitFrac( aProps, "XSIZE" ),
                         ReadKiCadUnitFrac( aProps, "YSIZE" ) );

    isSolid = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISSOLID", false );

    color     = ALTIUM_PROPS_UTILS::ReadInt( aProps, "COLOR", 0 );
    areacolor = ALTIUM_PROPS_UTILS::ReadInt( aProps, "AREACOLOR", 0 );
}


ASCH_SHEET_ENTRY::ASCH_SHEET_ENTRY( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::SHEET_ENTRY );

    // some magic, because it stores those infos in a different unit??
    distanceFromTop = ReadKiCadUnitFrac1( aProps, "DISTANCEFROMTOP" );

    side = ReadEnum<ASCH_SHEET_ENTRY_SIDE>( aProps, "SIDE", 0, 3, ASCH_SHEET_ENTRY_SIDE::LEFT );

    name = ALTIUM_PROPS_UTILS::ReadString( aProps, "NAME", "" );

    iotype = ReadEnum<ASCH_PORT_IOTYPE>( aProps, "IOTYPE", 0, 3, ASCH_PORT_IOTYPE::UNSPECIFIED );
    style = ReadEnum<ASCH_PORT_STYLE>( aProps, "STYLE", 0, 7, ASCH_PORT_STYLE::NONE_HORIZONTAL );
}


ASCH_POWER_PORT::ASCH_POWER_PORT( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::POWER_PORT );


    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    text        = ALTIUM_PROPS_UTILS::ReadString( aProps, "TEXT", "" );
    showNetName = ALTIUM_PROPS_UTILS::ReadBool( aProps, "SHOWNETNAME", true );

    style = ReadEnum<ASCH_POWER_PORT_STYLE>( aProps, "STYLE", 0, 10,
                                             ASCH_POWER_PORT_STYLE::CIRCLE );
}


ASCH_PORT::ASCH_PORT( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::PORT );


    Location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    Name = ALTIUM_PROPS_UTILS::ReadString( aProps, "NAME", "" );
    HarnessType = ALTIUM_PROPS_UTILS::ReadString( aProps, "HARNESSTYPE", "" );

    Width  = ReadKiCadUnitFrac( aProps, "WIDTH" );
    Height = ReadKiCadUnitFrac( aProps, "HEIGHT" );

    IOtype = ReadEnum<ASCH_PORT_IOTYPE>( aProps, "IOTYPE", 0, 3, ASCH_PORT_IOTYPE::UNSPECIFIED );
    Style = ReadEnum<ASCH_PORT_STYLE>( aProps, "STYLE", 0, 7, ASCH_PORT_STYLE::NONE_HORIZONTAL );

    AreaColor = ALTIUM_PROPS_UTILS::ReadInt( aProps, "AREACOLOR", 0 );
    Color = ALTIUM_PROPS_UTILS::ReadInt( aProps, "COLOR", 0 );
    FontID = ALTIUM_PROPS_UTILS::ReadInt( aProps, "TEXTFONTID", 0 );
    TextColor = ALTIUM_PROPS_UTILS::ReadInt( aProps, "TEXTCOLOR", 0 );

    m_align = ReadEnum<ASCH_PORT_ALIGNMENT>( aProps, "ALIGNMENT", 0, 2, ASCH_PORT_ALIGNMENT::CENTER );
}


ASCH_NO_ERC::ASCH_NO_ERC( const std::map<wxString, wxString>& aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::NO_ERC );

    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    isActive   = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISACTIVE", true );
    suppressAll = ALTIUM_PROPS_UTILS::ReadInt( aProps, "SUPPRESSALL", true );
}


ASCH_NET_LABEL::ASCH_NET_LABEL( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::NET_LABEL );

    text = ALTIUM_PROPS_UTILS::ReadString( aProps, "TEXT", "" );

    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    justification = ReadEnum<ASCH_LABEL_JUSTIFICATION>( aProps, "JUSTIFICATION", 0, 8,
                                                        ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );
}


ASCH_BUS::ASCH_BUS( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::BUS );

    int locationcount = ALTIUM_PROPS_UTILS::ReadInt( aProps, "LOCATIONCOUNT", 0 );

    for( int i = 1; i <= locationcount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( ReadKiCadUnitFrac( aProps, "X" + si ),
                             -ReadKiCadUnitFrac( aProps, "Y" + si ) );
    }

    lineWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
}


ASCH_WIRE::ASCH_WIRE( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::WIRE );

    int locationcount = ALTIUM_PROPS_UTILS::ReadInt( aProps, "LOCATIONCOUNT", 0 );

    for( int i = 1; i <= locationcount; i++ )
    {
        const wxString si = std::to_string( i );
        points.emplace_back( ReadKiCadUnitFrac( aProps, "X" + si ),
                             -ReadKiCadUnitFrac( aProps, "Y" + si ) );
    }

    lineWidth = ReadKiCadUnitFrac( aProps, "LINEWIDTH" );
}


ASCH_JUNCTION::ASCH_JUNCTION( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::JUNCTION );


    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
}


ASCH_IMAGE::ASCH_IMAGE( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps ),
        ASCH_BORDER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::IMAGE );

    filename = ALTIUM_PROPS_UTILS::ReadString( aProps, "FILENAME", "" );

    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    corner = VECTOR2I( ReadKiCadUnitFrac( aProps, "CORNER.X" ),
                       -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) );

    embedimage = ALTIUM_PROPS_UTILS::ReadBool( aProps, "EMBEDIMAGE", false );
    keepaspect = ALTIUM_PROPS_UTILS::ReadBool( aProps, "KEEPASPECT", false );
}


ASCH_SHEET_FONT::ASCH_SHEET_FONT( const std::map<wxString, wxString>& aProps, int aId ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::SHEET );

    const wxString sid = std::to_string( aId );

    FontName = ALTIUM_PROPS_UTILS::ReadString( aProps, "FONTNAME" + sid, "" );

    Size     = ReadKiCadUnitFrac( aProps, "SIZE" + sid );
    Rotation = ALTIUM_PROPS_UTILS::ReadInt( aProps, "ROTATION" + sid, 0 );

    Italic    = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ITALIC" + sid, false );
    Bold      = ALTIUM_PROPS_UTILS::ReadBool( aProps, "BOLD" + sid, false );
    Underline = ALTIUM_PROPS_UTILS::ReadBool( aProps, "UNDERLINE" + sid, false );

    AreaColor = ALTIUM_PROPS_UTILS::ReadInt( aProps, "AREACOLOR" + sid, 0 );
}


VECTOR2I ASchSheetGetSize( ASCH_SHEET_SIZE aSheetSize )
{
    // From: https://github.com/vadmium/python-altium/blob/master/format.md#sheet
    switch( aSheetSize )
    {
    default:
    case ASCH_SHEET_SIZE::A4:      return { 1150,  760 };
    case ASCH_SHEET_SIZE::A3:      return { 1550, 1110 };
    case ASCH_SHEET_SIZE::A2:      return { 2230, 1570 };
    case ASCH_SHEET_SIZE::A1:      return { 3150, 2230 };
    case ASCH_SHEET_SIZE::A0:      return { 4460, 3150 };
    case ASCH_SHEET_SIZE::A:       return {  950,  750 };
    case ASCH_SHEET_SIZE::B:       return { 1500,  950 };
    case ASCH_SHEET_SIZE::C:       return { 2000, 1500 };
    case ASCH_SHEET_SIZE::D:       return { 3200, 2000 };
    case ASCH_SHEET_SIZE::E:       return { 4200, 3200 };
    case ASCH_SHEET_SIZE::LETTER:  return { 1100,  850 };
    case ASCH_SHEET_SIZE::LEGAL:   return { 1400,  850 };
    case ASCH_SHEET_SIZE::TABLOID: return { 1700, 1100 };
    case ASCH_SHEET_SIZE::ORCAD_A: return {  990,  790 };
    case ASCH_SHEET_SIZE::ORCAD_B: return { 1540,  990 };
    case ASCH_SHEET_SIZE::ORCAD_C: return { 2060, 1560 };
    case ASCH_SHEET_SIZE::ORCAD_D: return { 3260, 2060 };
    case ASCH_SHEET_SIZE::ORCAD_E: return { 4280, 3280 };
    }
}


ASCH_SHEET::ASCH_SHEET( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::SHEET );

    int fontidcount = ALTIUM_PROPS_UTILS::ReadInt( aProps, "FONTIDCOUNT", 0 );

    for( int i = 1; i <= fontidcount; i++ )
        fonts.emplace_back( aProps, i );

    useCustomSheet = ALTIUM_PROPS_UTILS::ReadBool( aProps, "USECUSTOMSHEET", false );

    customSize = VECTOR2I( ReadKiCadUnitFrac( aProps, "CUSTOMX" ),
                           ReadKiCadUnitFrac( aProps, "CUSTOMY" ) );

    sheetSize = ReadEnum<ASCH_SHEET_SIZE>( aProps, "SHEETSTYLE", 0, 17, ASCH_SHEET_SIZE::A4 );
    sheetOrientation = ReadEnum<ASCH_SHEET_WORKSPACEORIENTATION>(
            aProps, "WORKSPACEORIENTATION", 0, 1, ASCH_SHEET_WORKSPACEORIENTATION::LANDSCAPE );
}


ASCH_SHEET_NAME::ASCH_SHEET_NAME( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::SHEET_NAME );

    text = ALTIUM_PROPS_UTILS::ReadString( aProps, "TEXT", "" );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    isHidden = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISHIDDEN", false );
}


ASCH_FILE_NAME::ASCH_FILE_NAME( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::FILE_NAME );

    text = ALTIUM_PROPS_UTILS::ReadString( aProps, "TEXT", "" );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    isHidden = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISHIDDEN", false );
}


ASCH_DESIGNATOR::ASCH_DESIGNATOR( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::DESIGNATOR );

    name = ALTIUM_PROPS_UTILS::ReadString( aProps, "NAME", "" );
    text = ALTIUM_PROPS_UTILS::ReadString( aProps, "TEXT", "" );
    fontId = ALTIUM_PROPS_UTILS::ReadInt( aProps, "FONTID", 0 );

    justification = ReadEnum<ASCH_LABEL_JUSTIFICATION>( aProps, "JUSTIFICATION", 0, 8,
                                                        ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
}


ASCH_IMPLEMENTATION::ASCH_IMPLEMENTATION( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::IMPLEMENTATION );

    ownerindex = ALTIUM_PROPS_UTILS::ReadInt( aProps, "OWNERINDEX", ALTIUM_COMPONENT_NONE );
    name = ALTIUM_PROPS_UTILS::ReadString( aProps, "MODELNAME", "" );
    type = ALTIUM_PROPS_UTILS::ReadString( aProps, "MODELTYPE", "" );
    libname = ALTIUM_PROPS_UTILS::ReadString( aProps, "MODELDATAFILE0", "" );
    description = ALTIUM_PROPS_UTILS::ReadString( aProps, "DESCRIPTION", "" );
    isCurrent = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISCURRENT", false );
}


ASCH_IMPLEMENTATION_LIST::ASCH_IMPLEMENTATION_LIST( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::IMPLEMENTATION_LIST );
}


ASCH_BUS_ENTRY::ASCH_BUS_ENTRY( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::BUS_ENTRY );

    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );
    corner = VECTOR2I( ReadKiCadUnitFrac( aProps, "CORNER.X" ),
                       -ReadKiCadUnitFrac( aProps, "CORNER.Y" ) );
}


ASCH_PARAMETER::ASCH_PARAMETER( const std::map<wxString, wxString>& aProps ) :
        ASCH_OWNER_INTERFACE( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::PARAMETER );

    location = VECTOR2I( ReadKiCadUnitFrac( aProps, "LOCATION.X" ),
                         -ReadKiCadUnitFrac( aProps, "LOCATION.Y" ) );

    justification = ReadEnum<ASCH_LABEL_JUSTIFICATION>( aProps, "JUSTIFICATION", 0, 8,
                                                        ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT );

    orientation = ReadEnum<ASCH_RECORD_ORIENTATION>( aProps, "ORIENTATION", 0, 3,
                                                     ASCH_RECORD_ORIENTATION::RIGHTWARDS );

    name = ALTIUM_PROPS_UTILS::ReadString( aProps, "NAME", "" );
    text = ALTIUM_PROPS_UTILS::ReadString( aProps, "TEXT", "" );

    isHidden   = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISHIDDEN", false );
    isMirrored = ALTIUM_PROPS_UTILS::ReadBool( aProps, "ISMIRRORED", false );
    isShowName = ALTIUM_PROPS_UTILS::ReadBool( aProps, "SHOWNAME", false );

    fontId = ALTIUM_PROPS_UTILS::ReadInt( aProps, "FONTID", 0 );
}


ASCH_HYPERLINK::ASCH_HYPERLINK( const std::map<wxString, wxString>& aProps ) :
        ASCH_LABEL( aProps )
{
    wxASSERT( ReadRecord( aProps ) == ALTIUM_SCH_RECORD::HYPERLINK );

    url = ALTIUM_PROPS_UTILS::ReadString( aProps, "URL", "" );
}