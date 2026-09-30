/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: SYSUEric <jzzhuang666@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string>
#include <algorithm>
#include <filesystem>
#include <locale>
#include <map>
#include <set>
#include "odb_util.h"
#include <string_utils.h>
#include <wx/chartype.h>
#include <wx/dir.h>
#include <wx/regex.h>
#include <board.h>
#include <drill/drill_operation.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include "odb_defines.h"
#include "pcb_io_odbpp.h"

namespace ODB
{

int IpcViaType( const DRILL_OPERATION& aOperation, bool aTop )
{
    bool covered = aTop ? aOperation.m_TopCovered : aOperation.m_BottomCovered;
    bool plugged = aTop ? aOperation.m_TopPlugged : aOperation.m_BottomPlugged;
    bool tented = aTop ? aOperation.m_TopTented : aOperation.m_BottomTented;

    // IPC-4761 type VII requires filling, so capping alone cannot select it
    if( aOperation.m_Filled )
    {
        if( aOperation.m_Capped )
            return 7;

        return tented || covered ? 6 : 5;
    }

    if( plugged )
        return tented || covered ? 4 : 3;

    if( covered )
        return 2;

    return tented ? 1 : 0;
}


bool ViaInPad( const PCB_VIA& aVia )
{
    const BOARD* board = aVia.GetBoard();

    if( !board )
        return false;

    std::shared_ptr<SHAPE_SEGMENT> hole = aVia.GetEffectiveHoleShape();

    if( !hole )
        return false;

    BOX2I holeBox = hole->BBox();

    // Copper pours do not make ordinary stitching vias via-in-pad
    for( FOOTPRINT* footprint : board->Footprints() )
    {
        if( !footprint->GetBoundingBox( false ).Intersects( holeBox ) )
            continue;

        for( PAD* pad : footprint->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::NPTH || !pad->GetBoundingBox().Intersects( holeBox ) )
                continue;

            for( PCB_LAYER_ID layer : { F_Cu, B_Cu } )
            {
                if( !aVia.IsOnLayer( layer ) || !pad->IsOnLayer( layer ) )
                    continue;

                std::shared_ptr<SHAPE> copper = pad->GetEffectiveShape( layer );

                if( copper && copper->Collide( hole.get(), 0 ) )
                    return true;
            }
        }
    }

    return false;
}


std::vector<ODB_TYPE> OverridableLayerTypes( PCB_LAYER_ID aLayer )
{
    if( IsCopperLayer( aLayer ) )
        return { ODB_TYPE::SIGNAL, ODB_TYPE::POWER_GROUND, ODB_TYPE::MIXED };

    // Component, drill, rout and dielectric rows carry structure a drawing layer cannot supply
    return { ODB_TYPE::SILK_SCREEN, ODB_TYPE::SOLDER_MASK, ODB_TYPE::SOLDER_PASTE,
             ODB_TYPE::DOCUMENT,    ODB_TYPE::MASK,        ODB_TYPE::CONDUCTIVE_PASTE };
}


wxString GenODBString( const wxString& aStr )
{
    wxString str;

    for( size_t ii = 0; ii < aStr.Len(); ++ii )
    {
        // Rule: we can only use the standard ASCII, control excluded
        wxUniChar ch = aStr[ii];

        if( ch > 126 || !std::isgraph( static_cast<unsigned char>( ch ) ) )
            ch = '?';

        str += ch;
    }

    // Rule: only uppercase
    str.MakeUpper();

    return str;
}


static wxUniChar NextCharacter( const wxString& aStr, size_t& aIndex )
{
    wxUniChar    character = aStr[aIndex];
    unsigned int code = character.GetValue();

    // Windows wxString stores supplementary characters as surrogate pairs
    if( code >= 0xD800 && code <= 0xDBFF && aIndex + 1 < aStr.size() )
    {
        unsigned int next = aStr[aIndex + 1].GetValue();

        if( next >= 0xDC00 && next <= 0xDFFF )
            ++aIndex;
    }

    return character;
}


wxString GenLegalNetName( const wxString& aStr )
{
    wxString out;
    out.reserve( aStr.size() );

    for( size_t ii = 0; ii < aStr.size(); ++ii )
    {
        wxUniChar    c = NextCharacter( aStr, ii );
        unsigned int code = c.GetValue();

        if( ( code >= 33 && code <= 126 ) && code != ';' )
        {
            out += c;
        }
        else
        {
            out += '_';
        }
    }

    return out;
}


wxString GenLegalComponentName( const wxString& aStr )
{
    wxString out;
    out.reserve( aStr.length() );

    for( size_t ii = 0; ii < aStr.Len(); ++ii )
    {
        unsigned int code = NextCharacter( aStr, ii ).GetValue();

        // ODB++ component names must be printable ASCII (33-126), no spaces or semicolons
        if( code >= 33 && code <= 126 && code != ';' )
        {
            out.append( 1, static_cast<char>( code ) );
        }
        else
        {
            out.append( 1, '_' ); // Replace invalid characters with underscore
        }
    }

    return out;
}


// The names of these ODB++ entities must comply with
// the rules for legal entity names:
// product, model, step, layer, symbol, and attribute.
wxString GenLegalEntityName( const wxString& aStr )
{
    wxString out;
    out.reserve( aStr.size() );

    for( size_t ii = 0; ii < aStr.size(); ++ii )
    {
        unsigned int code = NextCharacter( aStr, ii ).GetValue();
        char         character;

        if( code >= 'A' && code <= 'Z' )
            character = static_cast<char>( code - 'A' + 'a' );
        else if( ( code >= 'a' && code <= 'z' ) || ( code >= '0' && code <= '9' ) || code == '-' || code == '_'
                 || code == '+' || code == '.' )
            character = static_cast<char>( code );
        else
            character = '_';

        out.append( 1, character );
    }

    if( out.length() > 64 )
    {
        out.Truncate( 64 );
    }

    while( !out.IsEmpty() && ( out[0] == '.' || out[0] == '-' || out[0] == '+' ) )
    {
        out.erase( 0, 1 );
    }

    while( !out.IsEmpty() && out.Last() == '.' )
    {
        out.RemoveLast();
    }

    return out;
}


std::vector<wxString> UniqueNames( const std::vector<wxString>& aBases, const std::set<wxString>& aReserved,
                                   size_t aMaxLength )
{
    std::vector<wxString> bases;
    bases.reserve( aBases.size() );

    for( const wxString& base : aBases )
    {
        wxCHECK_MSG( !base.IsEmpty(), aBases, wxS( "ODB++ name base must not be empty" ) );
        bases.push_back( base.Left( aMaxLength ) );
    }

    const std::set<wxString> originalBases( bases.begin(), bases.end() );
    std::set<wxString>       used = aReserved;
    std::vector<wxString>    names;
    names.reserve( bases.size() );

    // Names are never freed, so a base resumes after the last suffix it took
    std::map<wxString, size_t> nextSuffix;

    for( const wxString& base : bases )
    {
        wxString legal = base;

        if( used.contains( legal ) )
        {
            size_t& suffixNumber = nextSuffix.try_emplace( base, 2 ).first->second;

            for( ; ; ++suffixNumber )
            {
                wxString suffix = wxString::Format( wxS( "_%zu" ), suffixNumber );
                legal = base.Left( aMaxLength - suffix.length() ) + suffix;

                if( !originalBases.contains( legal ) && !used.contains( legal ) )
                    break;
            }
        }

        names.push_back( legal );
        used.insert( legal );
    }

    return names;
}


VARIANT_NAMES VARIANT_NAMES::Build( const std::vector<wxString>& aNames )
{
    VARIANT_NAMES result;
    std::vector<wxString> bases;
    std::vector<bool>     unnamed;
    bases.reserve( aNames.size() );

    for( const wxString& name : aNames )
    {
        wxString base = ODB::GenLegalEntityName( name );
        unnamed.push_back( base.IsEmpty() );
        bases.push_back( base.IsEmpty() ? wxString( wxS( "variant" ) ) : base );
    }

    std::vector<wxString> legalNames = UniqueNames( bases, {}, 64 );

    for( size_t index = 0; index < aNames.size(); ++index )
    {
        const wxString& name = aNames[index];
        const wxString& base = bases[index];
        const wxString& legal = legalNames[index];

        if( unnamed[index] || legal != base )
            result.m_renamed.push_back( name );

        result.m_names.emplace_back( name, legal );
    }

    result.m_listsFit = result.Join( aNames ).length() <= 1000;
    return result;
}


wxString VARIANT_NAMES::LegalName( const wxString& aName ) const
{
    for( const auto& [source, legal] : m_names )
    {
        if( source.CmpNoCase( aName ) == 0 )
            return legal;
    }

    return ODB::GenLegalEntityName( aName );
}


wxString VARIANT_NAMES::SelectedName( const wxString& aName ) const
{
    if( !m_listsFit )
        return wxString();

    for( const auto& [source, legal] : m_names )
    {
        if( source.CmpNoCase( aName ) == 0 )
            return legal;
    }

    return wxString();
}


wxString VARIANT_NAMES::Join( const std::vector<wxString>& aNames ) const
{
    wxString result;

    for( const wxString& name : aNames )
    {
        if( !result.IsEmpty() )
            result += ':';

        result += LegalName( name );
    }

    return result;
}


void RemoveWhitespace( wxString& aStr )
{
    aStr.Trim().Trim( false );
    wxRegEx spaces( "\\s" );
    spaces.Replace( &aStr, "_" );
}


wxString Double2String( const ODB_FORMAT& aFormat, double aVal )
{
    return FormatTrimmedDecimal( aVal, aFormat.m_sigfig );
}


std::string Double2String( double aVal, int32_t aDigits )
{
    // We don't want to output -0.0 as this value is just 0 for fabs
    if( aVal == -0.0 )
        aVal = 0.0;

    wxString str = wxString::FromCDouble( aVal, aDigits );

    return str.ToStdString();
}


wxString SymDouble2String( const ODB_FORMAT& aFormat, double aVal )
{
    return Double2String( aFormat, aFormat.m_symbolScale * aVal );
}


wxString Data2String( const ODB_FORMAT& aFormat, double aVal )
{
    return Double2String( aFormat, aFormat.m_scale * aVal );
}


std::pair<wxString, wxString> AddXY( const ODB_FORMAT& aFormat, const VECTOR2I& aVec )
{
    return { Double2String( aFormat, aFormat.m_scale * ( aVec.x - aFormat.m_originOffset.x ) ),
             Double2String( aFormat, -aFormat.m_scale * ( aVec.y - aFormat.m_originOffset.y ) ) };
}


std::pair<wxString, wxString> AddRelativeXY( const ODB_FORMAT& aFormat, const VECTOR2I& aVec )
{
    return { Double2String( aFormat, aFormat.m_scale * aVec.x ),
             Double2String( aFormat, -aFormat.m_scale * aVec.y ) };
}


VECTOR2I GetShapePosition( const PCB_SHAPE& aShape )
{
    VECTOR2D pos{};

    switch( aShape.GetShape() )
    {
    // Rectangles in KiCad are mapped by their corner while ODBPP uses the center
    case SHAPE_T::RECTANGLE:
        pos = aShape.GetPosition()
              + VECTOR2I( aShape.GetRectangleWidth() / 2.0, aShape.GetRectangleHeight() / 2.0 );
        break;
    // Both KiCad and ODBPP use the center of the circle
    case SHAPE_T::CIRCLE:
    // KiCad uses the exact points on the board
    case SHAPE_T::POLY:
    case SHAPE_T::BEZIER:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::ARC:
    case SHAPE_T::ELLIPSE:
    case SHAPE_T::ELLIPSE_ARC:
    case SHAPE_T::UNDEFINED:
        pos = aShape.GetPosition();
        break;
    }

    return pos;
}
} // namespace ODB


void ODB_TREE_WRITER::CreateEntityDirectory( const wxString& aPareDir,
                                             const wxString& aSubDir /*= wxEmptyString*/ )
{
    wxFileName path = wxFileName::DirName( aPareDir );

    wxArrayString subDirs = wxFileName::DirName( aSubDir.Lower() ).GetDirs();

    for( size_t i = 0; i < subDirs.GetCount(); i++ )
        path.AppendDir( subDirs[i] );

    if( !path.DirExists() )
    {
        if( !path.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            throw( std::runtime_error( "Could not create directory" + path.GetPath() ) );
        }
    }

    m_currentPath = path.GetPath();
}


ODB_FILE_WRITER::ODB_FILE_WRITER( ODB_TREE_WRITER& aTreeWriter, const wxString& aFileName ) :
        m_treeWriter( aTreeWriter )
{
    CreateFile( aFileName );
}


void ODB_FILE_WRITER::CreateFile( const wxString& aFileName )
{
    if( aFileName.IsEmpty() || m_treeWriter.GetCurrentPath().IsEmpty() )
        return;

    wxFileName fn;
    fn.SetPath( m_treeWriter.GetCurrentPath() );
    fn.SetFullName( aFileName );

    wxString dirPath = fn.GetPath();

    if( !wxDir::Exists( dirPath ) )
    {
        if( !wxDir::Make( dirPath ) )
            throw( std::runtime_error( "Could not create directory" + dirPath ) );
    }

    if( !fn.IsDirWritable() || ( fn.Exists() && !fn.IsFileWritable() ) )
        return;

    if( m_ostream.is_open() )
        m_ostream.close();

    // Windows narrow stream paths use the process code page, which may not be UTF-8
#ifdef __WXMSW__
    m_ostream.open( std::filesystem::path( std::wstring( fn.GetFullPath().wc_str() ) ),
                    std::ios_base::out | std::ios_base::trunc | std::ios_base::binary );
#else
    m_ostream.open( TO_UTF8( fn.GetFullPath() ),
                    std::ios_base::out | std::ios_base::trunc | std::ios_base::binary );
#endif

    m_ostream.imbue( std::locale::classic() );

    if( !m_ostream.is_open() || !m_ostream.good() )
        throw std::runtime_error( "Failed to open file: " + fn.GetFullPath() );
}


bool ODB_FILE_WRITER::CloseFile()
{
    if( m_ostream.is_open() )
    {
        m_ostream.close();

        if( !m_ostream.good() )
            throw std::runtime_error( "close file failed" );
    }

    return true;
}


void ODB_TEXT_WRITER::WriteEquationLine( const std::string& var, int value )
{
    WriteIndent();
    m_ostream << var << "=" << value << std::endl;
}


void ODB_TEXT_WRITER::WriteEquationLine( const wxString& var, const wxString& value )
{
    WriteIndent();
    m_ostream << var << "=" << value << std::endl;
}


void ODB_TEXT_WRITER::WriteIndent()
{
    if( in_array )
        m_ostream << "    ";
}


void ODB_TEXT_WRITER::BeginArray( const std::string& a )
{
    if( in_array )
        throw std::runtime_error( "already in array" );
    in_array = true;
    m_ostream << a << " {" << std::endl;
}


void ODB_TEXT_WRITER::EndArray()
{
    if( !in_array )
        throw std::runtime_error( "not in array" );
    in_array = false;
    m_ostream << "}" << std::endl << std::endl;
}


ODB_TEXT_WRITER::ARRAY_PROXY::ARRAY_PROXY( ODB_TEXT_WRITER& aWriter, const std::string& aStr ) :
        m_writer( aWriter )
{
    m_writer.BeginArray( aStr );
}


ODB_TEXT_WRITER::ARRAY_PROXY::~ARRAY_PROXY()
{
    m_writer.EndArray();
}


ODB_DRILL_TOOLS::ODB_DRILL_TOOLS( const wxString& aUnits, const wxString& aThickness,
                                  const wxString& aUserParams ) :
        m_units( aUnits ), m_thickness( aThickness ), m_userParams( aUserParams )
{
}


void ODB_DRILL_TOOLS::AddDrillTool( const ODB_FORMAT& aFormat, const wxString& aType, int aDiameter,
                                    const wxString& aType2 )
{
    // ODB++ tools use microns or mils, the same scale as symbol sizes
    wxString size = ODB::SymDouble2String( aFormat, aDiameter );

    // NUM names a physical drill bit, so a size that recurs across holes reuses its tool
    for( const TOOLS& existing : m_tools )
    {
        if( existing.m_type == aType && existing.m_type2 == aType2
            && existing.m_drillSize == size )
        {
            return;
        }
    }

    TOOLS tool;
    tool.m_num = m_tools.size() + 1;
    tool.m_type = aType;
    tool.m_type2 = aType2;
    tool.m_finishSize = size;
    tool.m_drillSize = size;

    m_tools.push_back( tool );
}


void ODB_DRILL_TOOLS::GenerateFile( std::ostream& aStream )
{
    ODB_TEXT_WRITER twriter( aStream );

    twriter.WriteEquationLine( "UNITS", m_units );
    twriter.WriteEquationLine( "THICKNESS", m_thickness );
    twriter.WriteEquationLine( "USER_PARAMS", m_userParams );

    for( const auto& tool : m_tools )
    {
        const auto array_proxy = twriter.MakeArrayProxy( "TOOLS" );
        twriter.WriteEquationLine( "NUM", tool.m_num );
        twriter.WriteEquationLine( "TYPE", tool.m_type );
        twriter.WriteEquationLine( "TYPE2", tool.m_type2 );
        twriter.WriteEquationLine( "MIN_TOL", tool.m_minTol );
        twriter.WriteEquationLine( "MAX_TOL", tool.m_maxTol );
        twriter.WriteEquationLine( "BIT", tool.m_bit );
        twriter.WriteEquationLine( "FINISH_SIZE", tool.m_finishSize );
        twriter.WriteEquationLine( "DRILL_SIZE", tool.m_drillSize );
    }
}
