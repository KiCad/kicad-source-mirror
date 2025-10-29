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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <magic_enum.hpp>
#include <wx/log.h>

#include <common.h>
#include <lib_symbol.h>
#include <sch_shape.h>
#include <sch_pin.h>
#include <sch_text.h>
#include <macros.h>
#include <richio.h>
#include <string_utils.h>
#include <template_fieldnames.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>

#include "sch_io_kicad_legacy_lib_cache.h"
#include "sch_io_kicad_legacy_helpers.h"


#define LIB_VERSION_MAJOR 2  ///< Legacy symbol library major version.
#define LIB_VERSION_MINOR 4  ///< Legacy symbol library minor version.

#define LIB_VERSION( major, minor ) ( major * 100 + minor )

/** Legacy symbol library (.lib) file header. */
#define LIBFILE_IDENT     "EESchema-LIBRARY Version"

/** Legacy symbol library document (.dcm) file header. */
#define DOCFILE_IDENT     "EESchema-DOCLIB  Version 2.0"

/**
 * Library versions 2.4 and lower use the old separate library (.lib) and
 * document (.dcm) files.  Symbol libraries after 2.4 merged the library
 * and document files into a single library file.  This macro checks if the
 * library version supports the old format.
 */
#define USE_OLD_DOC_FILE_FORMAT( major, minor )                      \
    ( LIB_VERSION( major, minor ) <= LIB_VERSION( 2, 4 ) )


const int fill_tab[3] = { 'N', 'F', 'f' };


SCH_IO_KICAD_LEGACY_LIB_CACHE::SCH_IO_KICAD_LEGACY_LIB_CACHE( const wxString& aFullPathAndFileName ) :
    SCH_IO_LIB_CACHE( aFullPathAndFileName )
{
    m_versionMajor = -1;
    m_versionMinor = -1;
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::Load()
{
    if( !m_libFileName.FileExists() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library file '%s' not found." ),
                                          m_libFileName.GetFullPath() ) );
    }

    wxCHECK_RET( m_libFileName.IsAbsolute(),
                 wxString::Format( "Cannot use relative file paths in legacy plugin to "
                                   "open library '%s'.", m_libFileName.GetFullPath() ) );

    wxLogTrace( traceSchLegacyPlugin, "Loading legacy symbol file '%s'",
                m_libFileName.GetFullPath() );

    FILE_LINE_READER reader( m_libFileName.GetFullPath() );

    if( !reader.ReadLine() )
        THROW_IO_ERROR( _( "Unexpected end of file." ) );

    const char* line = reader.Line();

    if( !strCompare( "EESchema-LIBRARY Version", line, &line ) )
    {
        // Old .sym files (which are libraries with only one symbol, used to store and reuse shapes)
        // EESchema-LIB Version x.x SYMBOL. They are valid files.
        if( !strCompare( "EESchema-LIB Version", line, &line ) )
            SCH_PARSE_ERROR( "file is not a valid symbol or symbol library file", reader, line );
    }

    m_versionMajor = parseInt( reader, line, &line );

    if( *line == '/' )
    {
        // Some old libraries use a version syntax like
        // EESchema-LIBRARY Version  2/10/2006-18:49:15
        // use 2.3 version numer to read the file
        m_versionMajor = 2;
        m_versionMinor = 3;
    }
    else
    {
        if( *line != '.' )
            SCH_PARSE_ERROR( "invalid file version formatting in header", reader, line );

        line++;

        m_versionMinor = parseInt( reader, line, &line );
    }

    if( m_versionMajor < 1 || m_versionMinor < 0 || m_versionMinor > 99 )
        SCH_PARSE_ERROR( "invalid file version in header", reader, line );

    // Check if this is a symbol library which is the same as a symbol library but without
    // any alias, documentation, footprint filters, etc.
    if( strCompare( "SYMBOL", line, &line ) )
    {
        // Symbol files add date and time stamp info to the header.
        m_libType = SCH_LIB_TYPE::LT_SYMBOL;

        /// @todo Probably should check for a valid date and time stamp even though it's not used.
    }
    else
    {
        m_libType = SCH_LIB_TYPE::LT_EESCHEMA;
    }

    while( reader.ReadLine() )
    {
        line = reader.Line();

        if( *line == '#' || isspace( *line ) )  // Skip comments and blank lines.
            continue;

        // Headers where only supported in older library file formats.
        if( m_libType == SCH_LIB_TYPE::LT_EESCHEMA && strCompare( "$HEADER", line ) )
            loadHeader( reader );

        if( strCompare( "DEF", line ) )
        {
            // Read one DEF/ENDDEF symbol entry from library:
            LIB_SYMBOL* symbol = LoadPart( reader, m_versionMajor, m_versionMinor, &m_symbols );

            m_symbols[ symbol->GetName() ] = symbol;
        }
    }

    SCH_IO_KICAD_LEGACY_LIB_CACHE::IncrementModifyHash();

    // Remember the file modification time of library file when the
    // cache snapshot was made, so that in a networked environment we will
    // reload the cache as needed.
    m_fileModTime = GetLibModificationTime();

    if( USE_OLD_DOC_FILE_FORMAT( m_versionMajor, m_versionMinor ) )
        loadDocs();
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::loadDocs()
{
    const char* line;
    wxString    text;
    wxString    aliasName;
    wxFileName  fn = m_libFileName;
    LIB_SYMBOL* symbol = nullptr;;

    fn.SetExt( FILEEXT::LegacySymbolDocumentFileExtension );

    // Not all libraries will have a document file.
    if( !fn.FileExists() )
        return;

    if( !fn.IsFileReadable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Insufficient permissions to read library '%s'." ),
                                          fn.GetFullPath() ) );
    }

    FILE_LINE_READER reader( fn.GetFullPath() );

    line = reader.ReadLine();

    if( !line )
        THROW_IO_ERROR( _( "symbol document library file is empty" ) );

    if( !strCompare( DOCFILE_IDENT, line, &line ) )
        SCH_PARSE_ERROR( "invalid document library file version formatting in header", reader, line );

    while( reader.ReadLine() )
    {
        line = reader.Line();

        if( *line == '#' )    // Comment line.
            continue;

        if( !strCompare( "$CMP", line, &line ) != 0 )
            SCH_PARSE_ERROR( "$CMP command expected", reader, line );

        aliasName = wxString::FromUTF8( line );
        aliasName.Trim();

        LIB_SYMBOL_MAP::iterator it = m_symbols.find( aliasName );

        if( it == m_symbols.end() )
        {
            wxLogWarning( "Symbol '%s' not found in library:\n\n"
                          "'%s'\n\nat line %d offset %d",
                          aliasName,
                          fn.GetFullPath(),
                          reader.LineNumber(),
                          (int) (line - reader.Line() ) );
        }
        else
        {
            symbol = it->second;
        }

        // Read the current alias associated doc.
        // if the alias does not exist, just skip the description
        // (Can happen if a .dcm is not synchronized with the corresponding .lib file)
        while( reader.ReadLine() )
        {
            line = reader.Line();

            if( !line )
                SCH_PARSE_ERROR( "unexpected end of file", reader, line );

            if( strCompare( "$ENDCMP", line, &line ) )
                break;

            text = From_UTF8( line + 2 );
            // Remove spaces at eol, and eol chars:
            text = text.Trim();

            switch( line[0] )
            {
            case 'D':
                if( symbol )
                    symbol->SetDescription( text );
                break;

            case 'K':
                if( symbol )
                    symbol->SetKeyWords( text );
                break;

            case 'F':
                if( symbol )
                    symbol->GetField( FIELD_T::DATASHEET )->SetText( text );
                break;

            case 0:
            case '\n':
            case '\r':
            case '#':
                // Empty line or commment
                break;

            default:
                SCH_PARSE_ERROR( "expected token in symbol definition", reader, line );
            }
        }
    }
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::loadHeader( FILE_LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxASSERT( strCompare( "$HEADER", line, &line ) );

    while( aReader.ReadLine() )
    {
        line = (char*) aReader;

        // The time stamp saved in old library files is not used or saved in the latest
        // library file version.
        if( strCompare( "TimeStamp", line, &line ) )
            continue;
        else if( strCompare( "$ENDHEADER", line, &line ) )
            return;
    }

    SCH_PARSE_ERROR( "$ENDHEADER not found", aReader, line );
}


LIB_SYMBOL* SCH_IO_KICAD_LEGACY_LIB_CACHE::LoadPart( LINE_READER& aReader, int aMajorVersion,
                                                     int aMinorVersion, LIB_SYMBOL_MAP* aMap )
{
    const char* line = aReader.Line();

    while( *line == '#' )
        aReader.ReadLine();

    if( !strCompare( "DEF", line, &line ) )
        SCH_PARSE_ERROR( "invalid symbol definition", aReader, line );

    long num;
    size_t pos = 4;                               // "DEF" plus the first space.
    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \t\r\n" );

    if( tokens.CountTokens() < 8 )
        SCH_PARSE_ERROR( "invalid symbol definition", aReader, line );

    // Read DEF line:
    std::unique_ptr<LIB_SYMBOL> symbol = std::make_unique<LIB_SYMBOL>( wxEmptyString );

    wxString name, prefix, tmp;

    name = tokens.GetNextToken();

    // This fixes a dubious decision to escape LIB_ID characters. Escaped LIB_IDs broke rescue
    // library look up.  Legacy LIB_IDs should not be escaped.
    if( name != UnescapeString( name ) )
        name = UnescapeString( name );

    pos += name.size() + 1;

    prefix = tokens.GetNextToken();
    pos += prefix.size() + 1;

    tmp = tokens.GetNextToken();
    pos += tmp.size() + 1;                        // NumOfPins, unused.

    tmp = tokens.GetNextToken();                  // Pin name offset.

    if( !tmp.ToLong( &num ) )
    {
        THROW_PARSE_ERROR( "invalid pin offset", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    symbol->SetPinNameOffset( schIUScale.MilsToIU( (int)num ) );

    tmp = tokens.GetNextToken();                  // Show pin numbers.

    if( !( tmp == "Y" || tmp == "N") )
    {
        THROW_PARSE_ERROR( "expected Y or N", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    symbol->SetShowPinNumbers( ( tmp == "N" ) ? false : true );

    tmp = tokens.GetNextToken();                  // Show pin names.

    if( !( tmp == "Y" || tmp == "N") )
    {
        THROW_PARSE_ERROR( "expected Y or N", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    symbol->SetShowPinNames( ( tmp == "N" ) ? false : true );

    tmp = tokens.GetNextToken();                  // Number of units.

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid unit count", aReader.GetSource(), aReader.Line(), aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    symbol->SetUnitCount( (int)num, true );

    // Ensure m_unitCount is >= 1.  Could be read as 0 in old libraries.
    if( symbol->GetUnitCount() < 1 )
        symbol->SetUnitCount( 1, true );

    // Copy symbol name and prefix.

    // The root alias is added to the alias list by SetName() which is called by SetText().
    if( name.IsEmpty() )
    {
        symbol->SetName( "~" );
    }
    else if( name[0] != '~' )
    {
        symbol->SetName( name );
    }
    else
    {
        symbol->SetName( name.Right( name.Length() - 1 ) );
        symbol->GetValueField().SetVisible( false );
    }

    // Don't set the library alias, this is determined by the symbol library table.
    symbol->SetLibId( LIB_ID( wxEmptyString, symbol->GetName() ) );

    SCH_FIELD& reference = symbol->GetReferenceField();

    if( prefix == "~" )
    {
        reference.Empty();
        reference.SetVisible( false );
    }
    else
    {
        reference.SetText( prefix );
    }

    // In version 2.2 and earlier, this parameter was a '0' which was just a place holder.
    // The was no concept of interchangeable multiple unit symbols.
    if( LIB_VERSION( aMajorVersion, aMinorVersion ) > 0
     && LIB_VERSION( aMajorVersion, aMinorVersion ) <= LIB_VERSION( 2, 2 ) )
    {
        // Nothing needs to be set since the default setting for symbols with multiple
        // units were never interchangeable.  Just parse the 0 an move on.
        tmp = tokens.GetNextToken();
        pos += tmp.size() + 1;
    }
    else
    {
        tmp = tokens.GetNextToken();

        if( tmp == "L" )
            symbol->LockUnits( true );
        else if( tmp == "F" || tmp == "0" )
            symbol->LockUnits( false );
        else
            THROW_PARSE_ERROR( "expected L, F, or 0", aReader.GetSource(), aReader.Line(),
                               aReader.LineNumber(), pos );

        pos += tmp.size() + 1;
    }

    // There is the optional power symbol flag.
    if( tokens.HasMoreTokens() )
    {
        tmp = tokens.GetNextToken();

        if( tmp == "P" )
            symbol->SetGlobalPower();
        else if( tmp == "N" )
            symbol->SetNormal();
        else
            THROW_PARSE_ERROR( "expected P or N", aReader.GetSource(), aReader.Line(),
                               aReader.LineNumber(), pos );
    }

    line = aReader.ReadLine();

    // Read lines until "ENDDEF" is found.
    while( line )
    {
        if( *line == '#' )                                  // Comment
            ;
        else if( strCompare( "Ti", line, &line ) )          // Modification date is ignored.
            continue;
        else if( strCompare( "ALIAS", line, &line ) )       // Aliases
            loadAliases( symbol, aReader, aMap );
        else if( *line == 'F' )                             // Fields
            loadField( symbol, aReader );
        else if( strCompare( "DRAW", line, &line ) )        // Drawing objects.
            loadDrawEntries( symbol, aReader, aMajorVersion, aMinorVersion );
        else if( strCompare( "$FPLIST", line, &line ) )     // Footprint filter list
            loadFootprintFilters( symbol, aReader );
        else if( strCompare( "ENDDEF", line, &line ) )      // End of symbol description
        {
            symbol->SetHasDeMorganBodyStyles( symbol->HasLegacyAlternateBodyStyle() );
            return symbol.release();
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "missing ENDDEF", aReader, line );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::loadAliases( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                                 LINE_READER&                 aReader,
                                                 LIB_SYMBOL_MAP*              aMap )
{
    wxString newAliasName;
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "ALIAS", line, &line ), "Invalid ALIAS section" );

    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \t\r\n" );

    // Parse the ALIAS list.
    while( tokens.HasMoreTokens() )
    {
        newAliasName = tokens.GetNextToken();

        if( aMap )
        {
            LIB_SYMBOL* newSymbol = new LIB_SYMBOL( newAliasName );

            // Inherit the parent mandatory field attributes.
            for( FIELD_T fieldId : MANDATORY_FIELDS )
            {
                SCH_FIELD* field = newSymbol->GetField( fieldId );
                SCH_FIELD* parentField = aSymbol->GetField( fieldId );

                *field = *parentField;

                if( fieldId == FIELD_T::VALUE )
                    field->SetText( newAliasName );

                field->SetParent( newSymbol );
            }

            newSymbol->SetParent( aSymbol.get() );
            newSymbol->SetHasDeMorganBodyStyles( newSymbol->HasLegacyAlternateBodyStyle() );

            // This will prevent duplicate aliases.
            (*aMap)[ newSymbol->GetName() ] = newSymbol;
        }
    }
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::loadField( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                               LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_RET( *line == 'F', "Invalid field line" );

    wxString    text;
    int         legacy_field_id;

    if( sscanf( line + 1, "%d", &legacy_field_id ) != 1 || legacy_field_id < 0 )
        SCH_PARSE_ERROR( "invalid field ID", aReader, line + 1 );

    FIELD_T    id = FIELD_T::USER;
    SCH_FIELD* field;

    // Map fixed legacy IDs
    switch( legacy_field_id )
    {
        case 0: id = FIELD_T::REFERENCE; break;
        case 1: id = FIELD_T::VALUE;     break;
        case 2: id = FIELD_T::FOOTPRINT; break;
        case 3: id = FIELD_T::DATASHEET; break;
    }

    if( id != FIELD_T::USER )
    {
        field = aSymbol->GetField( id );
    }
    else
    {
        field = new SCH_FIELD( aSymbol.get(), id );
        aSymbol->AddDrawItem( field, false );
    }

    // Skip to the first double quote.
    while( *line != '"' && *line != 0 )
        line++;

    if( *line == 0 )
        SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, line );

    parseQuotedString( text, aReader, line, &line, true );

    // The value field needs to be "special" escaped.  The other fields are
    // escaped normally and don't need special handling
    if( id == FIELD_T::VALUE )
        text = EscapeString( text, CTX_QUOTED_STR );

    // Doctor the *.lib file field which has a "~" in blank fields.  New saves will
    // not save like this.
    if( text.size() == 1 && text[0] == '~' )
        field->SetText( wxEmptyString );
    else
        field->SetText( ConvertToNewOverbarNotation( text ) );

    VECTOR2I pos;

    pos.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pos.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    field->SetPosition( pos );

    VECTOR2I textSize;

    textSize.x = textSize.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    field->SetTextSize( textSize );

    char textOrient = parseChar( aReader, line, &line );

    if( textOrient == 'H' )
        field->SetTextAngle( ANGLE_HORIZONTAL );
    else if( textOrient == 'V' )
        field->SetTextAngle( ANGLE_VERTICAL );
    else
        SCH_PARSE_ERROR( "invalid field text orientation parameter", aReader, line );

    char textVisible = parseChar( aReader, line, &line );

    if( textVisible == 'V' )
        field->SetVisible( true );
    else if ( textVisible == 'I' )
        field->SetVisible( false );
    else
        SCH_PARSE_ERROR( "invalid field text visibility parameter", aReader, line );

    // It may be technically correct to use the library version to determine if the field text
    // attributes are present.  If anyone knows if that is valid and what version that would be,
    // please change this to test the library version rather than an EOL or the quoted string
    // of the field name.
    if( *line != 0 && *line != '"' )
    {
        char textHJustify = parseChar( aReader, line, &line );

        if( textHJustify == 'C' )
            field->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        else if( textHJustify == 'L' )
            field->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        else if( textHJustify == 'R' )
            field->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else
            SCH_PARSE_ERROR( "invalid field text horizontal justification", aReader, line );

        wxString attributes;

        parseUnquotedString( attributes, aReader, line, &line );

        size_t attrSize = attributes.size();

        if( !(attrSize == 3 || attrSize == 1 ) )
            SCH_PARSE_ERROR( "invalid field text attributes size", aReader, line );

        switch( (wxChar) attributes[0] )
        {
        case 'C': field->SetVertJustify( GR_TEXT_V_ALIGN_CENTER ); break;
        case 'B': field->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM ); break;
        case 'T': field->SetVertJustify( GR_TEXT_V_ALIGN_TOP );    break;
        default:  SCH_PARSE_ERROR( "invalid field text vertical justification", aReader, line );
        }

        if( attrSize == 3 )
        {
            wxChar attr_1 = attributes[1];
            wxChar attr_2 = attributes[2];

            if( attr_1 == 'I' )        // Italic
                field->SetItalicFlag( true );
            else if( attr_1 != 'N' )   // No italics is default, check for error.
                SCH_PARSE_ERROR( "invalid field text italic parameter", aReader, line );

            if ( attr_2 == 'B' )       // Bold
                field->SetBoldFlag( true );
            else if( attr_2 != 'N' )   // No bold is default, check for error.
                SCH_PARSE_ERROR( "invalid field text bold parameter", aReader, line );
        }
    }

    // Fields in RAM must always have names.
    if( field->IsMandatory() )
    {
        // Fields in RAM must always have names, because we are trying to get
        // less dependent on field ids and more dependent on names.
        // Plus assumptions are made in the field editors.
        field->SetName( GetCanonicalFieldName( field->GetId() ) );

        // Ensure the VALUE field = the symbol name (can be not the case
        // with malformed libraries: edited by hand, or converted from other tools)
        if( id == FIELD_T::VALUE )
            field->SetText( aSymbol->GetName() );
    }
    else
    {
        wxString fieldName = wxEmptyString;
        parseQuotedString( fieldName, aReader, line, &line, true ); // Optional.

        if( fieldName.IsEmpty() )
            return;

        wxString candidateFieldName = fieldName;
        int      suffix = 0;

        //Deduplicate field name
        while( aSymbol->GetField( candidateFieldName ) != nullptr )
            candidateFieldName = wxString::Format( "%s_%d", fieldName, ++suffix );

        field->SetName( candidateFieldName );
    }
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::loadDrawEntries( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                                     LINE_READER&                 aReader,
                                                     int                          aMajorVersion,
                                                     int                          aMinorVersion )
{
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "DRAW", line, &line ), "Invalid DRAW section" );

    line = aReader.ReadLine();

    while( line )
    {
        if( strCompare( "ENDDRAW", line, &line ) )
        {
            aSymbol->GetDrawItems().sort();
            return;
        }

        switch( line[0] )
        {
        case 'A':    // Arc
            aSymbol->AddDrawItem( loadArc( aReader ), false );
            break;

        case 'C':    // Circle
            aSymbol->AddDrawItem( loadCircle( aReader ), false );
            break;

        case 'T':    // Text
            aSymbol->AddDrawItem( loadText( aReader, aMajorVersion, aMinorVersion ), false );
            break;

        case 'S':    // Square
            aSymbol->AddDrawItem( loadRect( aReader ), false );
            break;

        case 'X':    // Pin Description
            aSymbol->AddDrawItem( loadPin( aSymbol, aReader ), false );
            break;

        case 'P':    // Polyline
            aSymbol->AddDrawItem( loadPolyLine( aReader ), false );
            break;

        case 'B':    // Bezier Curves
            aSymbol->AddDrawItem( loadBezier( aReader ), false );
            break;

        case '#':    // Comment
        case '\n':   // Empty line
        case '\r':
        case 0:
            break;

        default:
            SCH_PARSE_ERROR( "undefined DRAW entry", aReader, line );
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "File ended prematurely loading symbol draw element.", aReader, line );
}


FILL_T SCH_IO_KICAD_LEGACY_LIB_CACHE::parseFillMode( LINE_READER& aReader, const char* aLine,
                                                     const char** aOutput )
{
    switch ( parseChar( aReader, aLine, aOutput ) )
    {
    case 'F': return FILL_T::FILLED_SHAPE;
    case 'f': return FILL_T::FILLED_WITH_BG_BODYCOLOR;
    case 'N': return FILL_T::NO_FILL;
    default:  break;
    }

    SCH_PARSE_ERROR( "invalid fill type, expected f, F, or N", aReader, aLine );
}


/**
 * This function based on version 6.0 is required for reading legacy arcs.
 * Changing it in any way will likely break arcs.
 */
static bool MapAnglesV6( int* aAngle1, int* aAngle2 )
{
    auto DECIDEG2RAD = []( double deg ) -> double
    {
        return deg * M_PI / 1800.0;
    };

    int    angle, delta;
    double x, y;
    bool   swap = false;

    delta = *aAngle2 - *aAngle1;

    if( delta >= 1800 )
    {
        *aAngle1 -= 1;
        *aAngle2 += 1;
    }

    x = cos( DECIDEG2RAD( *aAngle1 ) );
    y = -sin( DECIDEG2RAD( *aAngle1 ) );
    *aAngle1 = KiROUND( RAD2DECIDEG( atan2( y, x ) ) );

    x = cos( DECIDEG2RAD( *aAngle2 ) );
    y = -sin( DECIDEG2RAD( *aAngle2 ) );
    *aAngle2 = KiROUND( RAD2DECIDEG( atan2( y, x ) ) );

    NORMALIZE_ANGLE_POS( *aAngle1 );
    NORMALIZE_ANGLE_POS( *aAngle2 );

    if( *aAngle2 < *aAngle1 )
        *aAngle2 += 3600;

    if( *aAngle2 - *aAngle1 > 1800 ) // Need to swap the two angles
    {
        angle = ( *aAngle1 );
        *aAngle1 = ( *aAngle2 );
        *aAngle2 = angle;

        NORMALIZE_ANGLE_POS( *aAngle1 );
        NORMALIZE_ANGLE_POS( *aAngle2 );

        if( *aAngle2 < *aAngle1 )
            *aAngle2 += 3600;

        swap = true;
    }

    if( delta >= 1800 )
    {
        *aAngle1 += 1;
        *aAngle2 -= 1;
    }

    return swap;
}


SCH_SHAPE* SCH_IO_KICAD_LEGACY_LIB_CACHE::loadArc( LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "A", line, &line ), nullptr, "Invalid arc definition" );

    SCH_SHAPE* arc = new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE );

    VECTOR2I center;

    center.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    center.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

    arc->SetPosition( center );

    int radius = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    int angle1 = parseInt( aReader, line, &line );
    int angle2 = parseInt( aReader, line, &line );

    NORMALIZE_ANGLE_POS( angle1 );
    NORMALIZE_ANGLE_POS( angle2 );

    arc->SetUnit( parseInt( aReader, line, &line ) );
    arc->SetBodyStyle( parseInt( aReader, line, &line ) );

    STROKE_PARAMS stroke( schIUScale.MilsToIU( parseInt( aReader, line, &line ) ),
                          LINE_STYLE::SOLID );

    arc->SetStroke( stroke );

    // Old libraries (version <= 2.2) do not have always this FILL MODE param when fill mode
    // is no fill (default mode).
    if( *line != 0 )
        arc->SetFillMode( parseFillMode( aReader, line, &line ) );

    // Actual Coordinates of arc ends are read from file
    if( *line != 0 )
    {
        VECTOR2I arcStart, arcEnd;

        arcStart.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
        arcStart.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
        arcEnd.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
        arcEnd.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

        arc->SetStart( arcStart );
        arc->SetEnd( arcEnd );
    }
    else
    {
        // Actual Coordinates of arc ends are not read from file
        // (old library), calculate them
        VECTOR2I arcStart( radius, 0 );
        VECTOR2I arcEnd( radius, 0 );

        RotatePoint( arcStart, EDA_ANGLE( angle1, EDA_ANGLE_T::TENTHS_OF_A_DEGREE_T ) );
        arcStart += arc->GetCenter();
        arc->SetStart( arcStart );
        RotatePoint( arcEnd, EDA_ANGLE( angle2, EDA_ANGLE_T::TENTHS_OF_A_DEGREE_T ) );
        arcEnd += arc->GetCenter();
        arc->SetEnd( arcEnd );
    }

    /**
     * This accounts for an oddity in the old library format, where the symbol is overdefined.
     * The previous draw (based on wxwidgets) used start point and end point and always drew
     * counter-clockwise.  The new GAL draw takes center, radius and start/end angles.  All of
     * these points were stored in the file, so we need to mimic the swapping of start/end
     * points rather than using the stored angles in order to properly map edge cases.
     */
    if( MapAnglesV6( &angle1, &angle2 ) )
    {
        VECTOR2I temp = arc->GetEnd();
        arc->SetEnd( arc->GetStart() );
        arc->SetStart( temp );
    }

    return arc;
}


SCH_SHAPE* SCH_IO_KICAD_LEGACY_LIB_CACHE::loadCircle( LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "C", line, &line ), nullptr, "Invalid circle definition" );

    SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );

    VECTOR2I center;

    center.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    center.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

    int radius = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

    circle->SetStart( center );
    circle->SetEnd( VECTOR2I( center.x + radius, center.y ) );
    circle->SetUnit( parseInt( aReader, line, &line ) );
    circle->SetBodyStyle( parseInt( aReader, line, &line ) );

    STROKE_PARAMS stroke( schIUScale.MilsToIU( parseInt( aReader, line, &line ) ),
                          LINE_STYLE::SOLID );

    circle->SetStroke( stroke );

    if( *line != 0 )
        circle->SetFillMode( parseFillMode( aReader, line, &line ) );

    return circle;
}


SCH_ITEM* SCH_IO_KICAD_LEGACY_LIB_CACHE::loadText( LINE_READER& aReader,
                                                   int aMajorVersion, int aMinorVersion )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "T", line, &line ), nullptr, "Invalid SCH_TEXT definition" );

    double   angleInTenths;
    VECTOR2I center;
    VECTOR2I size;
    wxString str;
    bool     visible;
    int      unit;
    int      bodyStyle;

    angleInTenths = parseInt( aReader, line, &line );

    center.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    center.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    size.x = size.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    visible = !parseInt( aReader, line, &line );
    unit = parseInt( aReader, line, &line );
    bodyStyle = parseInt( aReader, line, &line );

    // If quoted string loading fails, load as not quoted string.
    if( *line == '"' )
    {
        parseQuotedString( str, aReader, line, &line );

        str = ConvertToNewOverbarNotation( str );
    }
    else
    {
        parseUnquotedString( str, aReader, line, &line );

        // In old libs, "spaces" are replaced by '~' in unquoted strings:
        str.Replace( "~", " " );
    }

    if( !str.IsEmpty() )
    {
        // convert two apostrophes back to double quote
        str.Replace( "''", "\"" );
    }

    SCH_ITEM* sch_item = nullptr;
    EDA_TEXT* eda_text = nullptr;

    if( !visible )
    {
        SCH_FIELD* field = new SCH_FIELD( nullptr, FIELD_T::USER );
        sch_item = field;
        eda_text = field;
    }
    else
    {
        SCH_TEXT* sch_text = new SCH_TEXT( center, str, LAYER_DEVICE );
        sch_item = sch_text;
        eda_text = sch_text;
    }

    eda_text->SetTextAngle( EDA_ANGLE( angleInTenths, TENTHS_OF_A_DEGREE_T ) );
    eda_text->SetTextSize( size );
    eda_text->SetTextPos( center );
    eda_text->SetVisible( visible );
    sch_item->SetUnit( unit );
    sch_item->SetBodyStyle( bodyStyle );

    // Here things are murky and not well defined.  At some point it appears the format
    // was changed to add text properties.  However rather than add the token to the end of
    // the text definition, it was added after the string and no mention if the file
    // verion was bumped or not so this code make break on very old symbol libraries.
    //
    // Update: apparently even in the latest version this can be different so added a test
    //         for end of line before checking for the text properties.
    if( LIB_VERSION( aMajorVersion, aMinorVersion ) > 0
             && LIB_VERSION( aMajorVersion, aMinorVersion ) > LIB_VERSION( 2, 0 )
             && !is_eol( *line ) )
    {
        if( strCompare( "Italic", line, &line ) )
            eda_text->SetItalicFlag( true );
        else if( !strCompare( "Normal", line, &line ) )
            SCH_PARSE_ERROR( "invalid eda_text stype, expected 'Normal' or 'Italic'", aReader, line );

        if( parseInt( aReader, line, &line ) > 0 )
            eda_text->SetBoldFlag( true );

        // Some old libaries version > 2.0 do not have these options for eda_text justification:
        if( !is_eol( *line ) )
        {
            switch( parseChar( aReader, line, &line ) )
            {
            case 'L': eda_text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );   break;
            case 'C': eda_text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER ); break;
            case 'R': eda_text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );  break;
            default: SCH_PARSE_ERROR( "invalid horizontal eda_text justication; expected L, C, or R",
                                      aReader, line );
            }

            switch( parseChar( aReader, line, &line ) )
            {
            case 'T': eda_text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );    break;
            case 'C': eda_text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER ); break;
            case 'B': eda_text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM ); break;
            default: SCH_PARSE_ERROR( "invalid vertical eda_text justication; expected T, C, or B",
                                      aReader, line );
            }
        }
    }

    return sch_item;
}


SCH_SHAPE* SCH_IO_KICAD_LEGACY_LIB_CACHE::loadRect( LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "S", line, &line ), nullptr, "Invalid rectangle definition" );

    SCH_SHAPE* rectangle = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );

    VECTOR2I pos;

    pos.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pos.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    rectangle->SetPosition( pos );

    VECTOR2I end;

    end.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    end.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    rectangle->SetEnd( end );

    rectangle->SetUnit( parseInt( aReader, line, &line ) );
    rectangle->SetBodyStyle( parseInt( aReader, line, &line ) );

    STROKE_PARAMS stroke( schIUScale.MilsToIU( parseInt( aReader, line, &line ) ),
                          LINE_STYLE::SOLID );

    rectangle->SetStroke( stroke );


    if( *line != 0 )
        rectangle->SetFillMode( parseFillMode( aReader, line, &line ) );

    return rectangle;
}


SCH_PIN* SCH_IO_KICAD_LEGACY_LIB_CACHE::loadPin( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                                 LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "X", line, &line ), nullptr, "Invalid SCH_PIN definition" );

    wxString name;
    wxString number;

    size_t pos = 2;                               // "X" plus ' ' space character.
    wxString tmp;
    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \t\r\n" );

    if( tokens.CountTokens() < 11 )
        SCH_PARSE_ERROR( "invalid pin definition", aReader, line );

    tmp = tokens.GetNextToken();
    name = tmp;
    pos += tmp.size() + 1;

    tmp = tokens.GetNextToken();
    number = tmp ;
    pos += tmp.size() + 1;

    long num;
    VECTOR2I position;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
    {
        THROW_PARSE_ERROR( "invalid pin X coordinate", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    position.x = schIUScale.MilsToIU( (int) num );

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
    {
        THROW_PARSE_ERROR( "invalid pin Y coordinate", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    position.y = -schIUScale.MilsToIU( (int) num );

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
    {
        THROW_PARSE_ERROR( "invalid pin length", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    int length = schIUScale.MilsToIU( (int) num );


    tmp = tokens.GetNextToken();

    if( tmp.size() > 1 )
    {
        THROW_PARSE_ERROR( "invalid pin orientation", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;

    PIN_ORIENTATION orientation;

    switch( static_cast<char>( tmp[0] ) )
    {
    case 'U': orientation = PIN_ORIENTATION::PIN_UP; break;
    case 'D': orientation = PIN_ORIENTATION::PIN_DOWN; break;
    case 'L': orientation = PIN_ORIENTATION::PIN_LEFT; break;
    case 'R': /* fall-through */
    default:  orientation = PIN_ORIENTATION::PIN_RIGHT; break;
    }

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
    {
        THROW_PARSE_ERROR( "invalid pin number text size", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    int numberTextSize = schIUScale.MilsToIU( (int) num );

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
    {
        THROW_PARSE_ERROR( "invalid pin name text size", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    int nameTextSize = schIUScale.MilsToIU( (int) num );

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
    {
        THROW_PARSE_ERROR( "invalid pin unit", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    int unit = (int) num;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
    {
        THROW_PARSE_ERROR( "invalid pin body style", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    int bodyStyle = (int) num;

    tmp = tokens.GetNextToken();

    if( tmp.size() != 1 )
    {
        THROW_PARSE_ERROR( "invalid pin type", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    char type = tmp[0];
    ELECTRICAL_PINTYPE pinType;

    switch( type )
    {
    case 'I': pinType = ELECTRICAL_PINTYPE::PT_INPUT;         break;
    case 'O': pinType = ELECTRICAL_PINTYPE::PT_OUTPUT;        break;
    case 'B': pinType = ELECTRICAL_PINTYPE::PT_BIDI;          break;
    case 'T': pinType = ELECTRICAL_PINTYPE::PT_TRISTATE;      break;
    case 'P': pinType = ELECTRICAL_PINTYPE::PT_PASSIVE;       break;
    case 'U': pinType = ELECTRICAL_PINTYPE::PT_UNSPECIFIED;   break;
    case 'W': pinType = ELECTRICAL_PINTYPE::PT_POWER_IN;      break;
    case 'w': pinType = ELECTRICAL_PINTYPE::PT_POWER_OUT;     break;
    case 'C': pinType = ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR; break;
    case 'E': pinType = ELECTRICAL_PINTYPE::PT_OPENEMITTER;   break;
    case 'N': pinType = ELECTRICAL_PINTYPE::PT_NC;            break;
    default:
        THROW_PARSE_ERROR( "unknown pin type", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    SCH_PIN* pin = new SCH_PIN( aSymbol.get(),
                                ConvertToNewOverbarNotation( name ),
                                ConvertToNewOverbarNotation( number ),
                                orientation,
                                pinType,
                                length,
                                nameTextSize,
                                numberTextSize,
                                bodyStyle,
                                position,
                                unit );

    // Optional
    if( tokens.HasMoreTokens() )       /* Special Symbol defined */
    {
        tmp = tokens.GetNextToken();

        enum
        {
            INVERTED        = 1 << 0,
            CLOCK           = 1 << 1,
            LOWLEVEL_IN     = 1 << 2,
            LOWLEVEL_OUT    = 1 << 3,
            FALLING_EDGE    = 1 << 4,
            NONLOGIC        = 1 << 5
        };

        int flags = 0;

        for( int j = (int) tmp.size(); j > 0; )
        {
            switch( tmp[--j].GetValue() )
            {
            case '~': break;
            case 'N': pin->SetVisible( false ); break;
            case 'I': flags |= INVERTED;        break;
            case 'C': flags |= CLOCK;           break;
            case 'L': flags |= LOWLEVEL_IN;     break;
            case 'V': flags |= LOWLEVEL_OUT;    break;
            case 'F': flags |= FALLING_EDGE;    break;
            case 'X': flags |= NONLOGIC;        break;
            default: THROW_PARSE_ERROR( "invalid pin attribut", aReader.GetSource(),
                                        aReader.Line(), aReader.LineNumber(), pos );
            }

            pos += 1;
        }

        switch( flags )
        {
        case 0:                   pin->SetShape( GRAPHIC_PINSHAPE::LINE );               break;
        case INVERTED:            pin->SetShape( GRAPHIC_PINSHAPE::INVERTED );           break;
        case CLOCK:               pin->SetShape( GRAPHIC_PINSHAPE::CLOCK );              break;
        case INVERTED | CLOCK:    pin->SetShape( GRAPHIC_PINSHAPE::INVERTED_CLOCK );     break;
        case LOWLEVEL_IN:         pin->SetShape( GRAPHIC_PINSHAPE::INPUT_LOW );          break;
        case LOWLEVEL_IN | CLOCK: pin->SetShape( GRAPHIC_PINSHAPE::CLOCK_LOW );          break;
        case LOWLEVEL_OUT:        pin->SetShape( GRAPHIC_PINSHAPE::OUTPUT_LOW );         break;
        case FALLING_EDGE:        pin->SetShape( GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK ); break;
        case NONLOGIC:            pin->SetShape( GRAPHIC_PINSHAPE::NONLOGIC );           break;
        default: SCH_PARSE_ERROR( "pin attributes do not define a valid pin shape", aReader, line );
        }
    }

    return pin;
}


SCH_SHAPE* SCH_IO_KICAD_LEGACY_LIB_CACHE::loadPolyLine( LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "P", line, &line ), nullptr, "Invalid poly definition" );

    SCH_SHAPE* polyLine = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

    int points = parseInt( aReader, line, &line );
    polyLine->SetUnit( parseInt( aReader, line, &line ) );
    polyLine->SetBodyStyle( parseInt( aReader, line, &line ) );

    STROKE_PARAMS stroke( schIUScale.MilsToIU( parseInt( aReader, line, &line ) ), LINE_STYLE::SOLID );

    polyLine->SetStroke( stroke );

    VECTOR2I pt;

    for( int i = 0; i < points; i++ )
    {
        pt.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
        pt.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
        polyLine->AddPoint( pt );
    }

    if( *line != 0 )
        polyLine->SetFillMode( parseFillMode( aReader, line, &line ) );

    return polyLine;
}


SCH_SHAPE* SCH_IO_KICAD_LEGACY_LIB_CACHE::loadBezier( LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "B", line, &line ), nullptr, "Invalid Bezier definition" );

    int points = parseInt( aReader, line, &line );

    wxCHECK_MSG( points == 4, NULL, "Invalid Bezier curve definition" );

    SCH_SHAPE* bezier = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_DEVICE );

    bezier->SetUnit( parseInt( aReader, line, &line ) );
    bezier->SetBodyStyle( parseInt( aReader, line, &line ) );

    STROKE_PARAMS stroke ( schIUScale.MilsToIU( parseInt( aReader, line, &line ) ), LINE_STYLE::SOLID );

    bezier->SetStroke( stroke );

    VECTOR2I pt;

    pt.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pt.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    bezier->SetStart( pt );

    pt.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pt.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    bezier->SetBezierC1( pt );

    pt.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pt.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    bezier->SetBezierC2( pt );

    pt.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pt.y = -schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    bezier->SetEnd( pt );

    bezier->RebuildBezierToSegmentsPointsList( schIUScale.mmToIU( ARC_LOW_DEF_MM ) );

    if( *line != 0 )
        bezier->SetFillMode( parseFillMode( aReader, line, &line ) );

    return bezier;
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::loadFootprintFilters( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                                          LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "$FPLIST", line, &line ), "Invalid footprint filter list" );

    line = aReader.ReadLine();

    wxArrayString footprintFilters;

    while( line )
    {
        if( strCompare( "$ENDFPLIST", line, &line ) )
        {
            aSymbol->SetFPFilters( footprintFilters );
            return;
        }

        wxString footprint;

        parseUnquotedString( footprint, aReader, line, &line );
        footprintFilters.Add( footprint );
        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "File ended prematurely while loading footprint filters.", aReader, line );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::Save( const std::optional<bool>& aOpt )
{
    wxCHECK( aOpt, /* void */ );

    bool doSaveDocFile = *aOpt;

    if( !m_isModified )
        return;

    // Write through symlinks, don't replace them
    wxFileName fn = GetRealFile();

    auto formatter = std::make_unique<FILE_OUTPUTFORMATTER>( fn.GetFullPath() );
    formatter->Print( 0, "%s %d.%d\n", LIBFILE_IDENT, LIB_VERSION_MAJOR, LIB_VERSION_MINOR );
    formatter->Print( 0, "#encoding utf-8\n");

    for( LIB_SYMBOL_MAP::iterator it = m_symbols.begin();  it != m_symbols.end();  it++ )
    {
        if( !it->second->IsRoot() )
            continue;

        SaveSymbol( it->second, *formatter.get(), &m_symbols );
    }

    formatter->Print( 0, "#\n#End Library\n" );
    formatter.reset();

    m_fileModTime = TimestampDir( fn.GetPath(), fn.GetFullName() );
    m_isModified = false;

    if( doSaveDocFile )
        saveDocFile();
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::SaveSymbol( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter,
                                                LIB_SYMBOL_MAP* aMap )
{
    /*
     * NB:
     * Some of the rescue code still uses the legacy format as an intermediary, so we have
     * to keep this code.
     */

    wxCHECK_RET( aSymbol && aSymbol->IsRoot(), "Invalid LIB_SYMBOL pointer." );

    // LIB_ALIAS objects are deprecated but we still need to gather up the derived symbols
    // and save their names for the old file format.
    wxArrayString aliasNames;

    if( aMap )
    {
        for( auto& entry : *aMap )
        {
            LIB_SYMBOL* symbol = entry.second;

            if( symbol->IsDerived() && symbol->GetParent().lock() == aSymbol->SharedPtr() )
                aliasNames.Add( symbol->GetName() );
        }
    }

    SCH_FIELD&  value = aSymbol->GetValueField();

    // First line: it s a comment (symbol name for readers)
    aFormatter.Print( 0, "#\n# %s\n#\n", TO_UTF8( value.GetText() ) );

    // Save data
    aFormatter.Print( 0, "DEF" );
    aFormatter.Print( 0, " %s", TO_UTF8( value.GetText() ) );

    SCH_FIELD& reference = aSymbol->GetReferenceField();

    if( !reference.GetText().IsEmpty() )
        aFormatter.Print( 0, " %s", TO_UTF8( reference.GetText() ) );
    else
        aFormatter.Print( 0, " ~" );

    aFormatter.Print( 0, " %d %d %c %c %d %c %c\n",
                      0, schIUScale.IUToMils( aSymbol->GetPinNameOffset() ),
                      aSymbol->GetShowPinNumbers() ? 'Y' : 'N',
                      aSymbol->GetShowPinNames() ? 'Y' : 'N',
                      aSymbol->GetUnitCount(), aSymbol->UnitsLocked() ? 'L' : 'F',
                      aSymbol->IsGlobalPower() ? 'P' : 'N' );

    timestamp_t dateModified = aSymbol->GetLastModDate();

    if( dateModified != 0 )
    {
        int sec  = dateModified & 63;
        int min  = ( dateModified >> 6 ) & 63;
        int hour = ( dateModified >> 12 ) & 31;
        int day  = ( dateModified >> 17 ) & 31;
        int mon  = ( dateModified >> 22 ) & 15;
        int year = ( dateModified >> 26 ) + 1990;

        aFormatter.Print( 0, "Ti %d/%d/%d %d:%d:%d\n", year, mon, day, hour, min, sec );
    }

    std::vector<SCH_FIELD*> orderedFields;
    aSymbol->GetFields( orderedFields );

    // NB: FieldIDs in legacy libraries must be consecutive, and include user fields
    int legacy_field_id = 0;

    for( SCH_FIELD* field : orderedFields )
        saveField( field, legacy_field_id++, aFormatter );

    // Save the alias list: a line starting by "ALIAS".
    if( !aliasNames.IsEmpty() )
    {
        aFormatter.Print( 0, "ALIAS" );

        for( unsigned i = 0; i < aliasNames.GetCount(); i++ )
            aFormatter.Print( 0, " %s", TO_UTF8( aliasNames[i] ) );

        aFormatter.Print( 0, "\n" );
    }

    wxArrayString footprints = aSymbol->GetFPFilters();

    // Write the footprint filter list
    if( footprints.GetCount() != 0 )
    {
        aFormatter.Print( 0, "$FPLIST\n" );

        for( unsigned i = 0; i < footprints.GetCount(); i++ )
            aFormatter.Print( 0, " %s\n", TO_UTF8( footprints[i] ) );

        aFormatter.Print( 0, "$ENDFPLIST\n" );
    }

    // Save graphics items (including pins)
    if( !aSymbol->GetDrawItems().empty() )
    {
        // Sort the draw items in order to editing a file editing by hand.
        aSymbol->GetDrawItems().sort();

        aFormatter.Print( 0, "DRAW\n" );

        for( SCH_ITEM& item : aSymbol->GetDrawItems() )
        {
            switch( item.Type() )
            {
            case SCH_PIN_T:
                savePin( static_cast<SCH_PIN*>( &item ), aFormatter );

                break;
            case SCH_TEXT_T:
                saveText( static_cast<SCH_TEXT*>( &item ), aFormatter );
                break;

            case SCH_SHAPE_T:
            {
                SCH_SHAPE& shape = static_cast<SCH_SHAPE&>( item );

                switch( shape.GetShape() )
                {
                case SHAPE_T::ARC:       saveArc( &shape, aFormatter );        break;
                case SHAPE_T::BEZIER:    saveBezier( &shape, aFormatter );     break;
                case SHAPE_T::CIRCLE:    saveCircle( &shape, aFormatter );     break;
                case SHAPE_T::POLY:      savePolyLine( &shape, aFormatter );   break;
                case SHAPE_T::RECTANGLE: saveRectangle( &shape, aFormatter );  break;
                default:                                                       break;
                }

                break;
            }

            default:
                break;
            }
        }

        aFormatter.Print( 0, "ENDDRAW\n" );
    }

    aFormatter.Print( 0, "ENDDEF\n" );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::saveArc( SCH_SHAPE* aArc, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aArc && aArc->GetShape() == SHAPE_T::ARC, "Invalid ARC object." );

    EDA_ANGLE startAngle, endAngle;

    aArc->CalcArcAngles( endAngle, startAngle );
    startAngle.Normalize180();
    endAngle.Normalize180();

    aFormatter.Print( 0, "A %d %d %d %d %d %d %d %d %c %d %d %d %d\n",
                      schIUScale.IUToMils( aArc->GetPosition().x ),
                      schIUScale.IUToMils( -aArc->GetPosition().y ),
                      schIUScale.IUToMils( aArc->GetRadius() ),
                      startAngle.AsTenthsOfADegree(),
                      endAngle.AsTenthsOfADegree(),
                      aArc->GetUnit(),
                      aArc->GetBodyStyle(),
                      schIUScale.IUToMils( aArc->GetWidth() ),
                      fill_tab[ static_cast<int>( aArc->GetFillMode() ) - 1 ],
                      schIUScale.IUToMils( aArc->GetStart().x ),
                      schIUScale.IUToMils( -aArc->GetStart().y ),
                      schIUScale.IUToMils( aArc->GetEnd().x ),
                      schIUScale.IUToMils( -aArc->GetEnd().y ) );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::saveBezier( SCH_SHAPE* aBezier, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aBezier && aBezier->GetShape() == SHAPE_T::BEZIER, "Invalid BEZIER object." );

    aFormatter.Print( 0, "B 4 %d %d %d",
                      aBezier->GetUnit(),
                      aBezier->GetBodyStyle(),
                      schIUScale.IUToMils( aBezier->GetWidth() ) );

    aFormatter.Print( 0, " %d %d %d %d %d %d %d %d",
                      schIUScale.IUToMils( aBezier->GetStart().x ),
                      schIUScale.IUToMils( -aBezier->GetStart().y ),
                      schIUScale.IUToMils( aBezier->GetBezierC1().x ),
                      schIUScale.IUToMils( -aBezier->GetBezierC1().y ),
                      schIUScale.IUToMils( aBezier->GetBezierC2().x ),
                      schIUScale.IUToMils( -aBezier->GetBezierC2().y ),
                      schIUScale.IUToMils( aBezier->GetEnd().x ),
                      schIUScale.IUToMils( -aBezier->GetEnd().y ) );

    aFormatter.Print( 0, " %c\n", fill_tab[ static_cast<int>( aBezier->GetFillMode() ) - 1 ] );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::saveCircle( SCH_SHAPE* aCircle, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aCircle && aCircle->GetShape() == SHAPE_T::CIRCLE, "Invalid CIRCLE object." );

    aFormatter.Print( 0, "C %d %d %d %d %d %d %c\n",
                      schIUScale.IUToMils( aCircle->GetPosition().x ),
                      schIUScale.IUToMils( -aCircle->GetPosition().y ),
                      schIUScale.IUToMils( aCircle->GetRadius() ),
                      aCircle->GetUnit(),
                      aCircle->GetBodyStyle(),
                      schIUScale.IUToMils( aCircle->GetWidth() ),
                      fill_tab[ static_cast<int>( aCircle->GetFillMode() ) - 1 ] );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::saveField( const SCH_FIELD* aField, int aLegacyFieldIdx,
                                               OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aField && aField->Type() == SCH_FIELD_T, "Invalid SCH_FIELD object." );

    int      hjustify, vjustify;
    wxString text = aField->GetText();

    hjustify = 'C';

    if( aField->GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
        hjustify = 'L';
    else if( aField->GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
        hjustify = 'R';

    vjustify = 'C';

    if( aField->GetVertJustify() == GR_TEXT_V_ALIGN_BOTTOM )
        vjustify = 'B';
    else if( aField->GetVertJustify() == GR_TEXT_V_ALIGN_TOP )
        vjustify = 'T';

    aFormatter.Print( 0, "F%d %s %d %d %d %c %c %c %c%c%c",
                      aLegacyFieldIdx,
                      EscapedUTF8( text ).c_str(),       // wraps in quotes
                      schIUScale.IUToMils( aField->GetTextPos().x ),
                      schIUScale.IUToMils( -aField->GetTextPos().y ),
                      schIUScale.IUToMils( aField->GetTextWidth() ),
                      aField->GetTextAngle().IsHorizontal() ? 'H' : 'V',
                      aField->IsVisible() ? 'V' : 'I',
                      hjustify, vjustify,
                      aField->IsItalic() ? 'I' : 'N',
                      aField->IsBold() ? 'B' : 'N' );

    // Translated names were stored in legacy files, so it's important not to save the
    // default names as they weren't yet canonical.
    if( !aField->IsMandatory()
            && !aField->GetName().IsEmpty()
            && aField->GetName() != GetUserFieldName( aLegacyFieldIdx, !DO_TRANSLATE ) )
    {
        aFormatter.Print( 0, " %s", EscapedUTF8( aField->GetName() ).c_str() );
    }

    aFormatter.Print( 0, "\n" );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::savePin( const SCH_PIN* aPin, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aPin && aPin->Type() == SCH_PIN_T, "Invalid SCH_PIN object." );

    int      Etype;

    switch( aPin->GetType() )
    {
    default:
    case ELECTRICAL_PINTYPE::PT_INPUT:         Etype = 'I'; break;
    case ELECTRICAL_PINTYPE::PT_OUTPUT:        Etype = 'O'; break;
    case ELECTRICAL_PINTYPE::PT_BIDI:          Etype = 'B'; break;
    case ELECTRICAL_PINTYPE::PT_TRISTATE:      Etype = 'T'; break;
    case ELECTRICAL_PINTYPE::PT_PASSIVE:       Etype = 'P'; break;
    case ELECTRICAL_PINTYPE::PT_UNSPECIFIED:   Etype = 'U'; break;
    case ELECTRICAL_PINTYPE::PT_POWER_IN:      Etype = 'W'; break;
    case ELECTRICAL_PINTYPE::PT_POWER_OUT:     Etype = 'w'; break;
    case ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR: Etype = 'C'; break;
    case ELECTRICAL_PINTYPE::PT_OPENEMITTER:   Etype = 'E'; break;
    case ELECTRICAL_PINTYPE::PT_NC:            Etype = 'N'; break;
    }

    if( !aPin->GetName().IsEmpty() )
        aFormatter.Print( 0, "X %s", TO_UTF8( aPin->GetName() ) );
    else
        aFormatter.Print( 0, "X ~" );

    int pin_orient = 'L';   // Printed as a char in lib file

    switch( aPin->GetOrientation() )
    {
        case PIN_ORIENTATION::PIN_RIGHT: pin_orient = 'R'; break;
        case PIN_ORIENTATION::PIN_LEFT: pin_orient = 'L'; break;
        case PIN_ORIENTATION::PIN_UP: pin_orient = 'U'; break;
        case PIN_ORIENTATION::PIN_DOWN: pin_orient = 'D'; break;
        case PIN_ORIENTATION::INHERIT: pin_orient = 'L'; break;     // Should not happens
    }

    aFormatter.Print( 0, " %s %d %d %d %c %d %d %d %d %c",
                      aPin->GetNumber().IsEmpty() ? "~" : TO_UTF8( aPin->GetNumber() ),
                      schIUScale.IUToMils( aPin->GetPosition().x ),
                      schIUScale.IUToMils( -aPin->GetPosition().y ),
                      schIUScale.IUToMils( (int) aPin->GetLength() ),
                      pin_orient,
                      schIUScale.IUToMils( aPin->GetNumberTextSize() ),
                      schIUScale.IUToMils( aPin->GetNameTextSize() ),
                      aPin->GetUnit(),
                      aPin->GetBodyStyle(),
                      Etype );

    if( aPin->GetShape() != GRAPHIC_PINSHAPE::LINE || !aPin->IsVisible() )
        aFormatter.Print( 0, " " );

    if( !aPin->IsVisible() )
        aFormatter.Print( 0, "N" );

    switch( aPin->GetShape() )
    {
    case GRAPHIC_PINSHAPE::LINE:                                            break;
    case GRAPHIC_PINSHAPE::INVERTED:           aFormatter.Print( 0, "I" );  break;
    case GRAPHIC_PINSHAPE::CLOCK:              aFormatter.Print( 0, "C" );  break;
    case GRAPHIC_PINSHAPE::INVERTED_CLOCK:     aFormatter.Print( 0, "IC" ); break;
    case GRAPHIC_PINSHAPE::INPUT_LOW:          aFormatter.Print( 0, "L" );  break;
    case GRAPHIC_PINSHAPE::CLOCK_LOW:          aFormatter.Print( 0, "CL" ); break;
    case GRAPHIC_PINSHAPE::OUTPUT_LOW:         aFormatter.Print( 0, "V" );  break;
    case GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK: aFormatter.Print( 0, "F" );  break;
    case GRAPHIC_PINSHAPE::NONLOGIC:           aFormatter.Print( 0, "X" );  break;
    default:                                   wxFAIL_MSG( "Invalid pin shape" );
    }

    aFormatter.Print( 0, "\n" );

    const_cast<SCH_PIN*>( aPin )->ClearFlags( IS_CHANGED );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::savePolyLine( SCH_SHAPE* aPolyLine,
                                                  OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aPolyLine && aPolyLine->GetShape() == SHAPE_T::POLY, "Invalid POLY object." );

    aFormatter.Print( 0, "P %d %d %d %d",
                      (int) aPolyLine->GetPolyShape().Outline( 0 ).GetPointCount(),
                      aPolyLine->GetUnit(),
                      aPolyLine->GetBodyStyle(),
                      schIUScale.IUToMils( aPolyLine->GetWidth() ) );

    for( const VECTOR2I& pt : aPolyLine->GetPolyShape().Outline( 0 ).CPoints() )
        aFormatter.Print( 0, " %d %d", schIUScale.IUToMils( pt.x ), -schIUScale.IUToMils( pt.y ) );

    aFormatter.Print( 0, " %c\n", fill_tab[ static_cast<int>( aPolyLine->GetFillMode() ) - 1 ] );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::saveRectangle( SCH_SHAPE* aRectangle,
                                                   OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aRectangle && aRectangle->GetShape() == SHAPE_T::RECTANGLE,
                 "Invalid RECT object." );

    aFormatter.Print( 0, "S %d %d %d %d %d %d %d %c\n",
                      schIUScale.IUToMils( aRectangle->GetPosition().x ),
                      schIUScale.IUToMils( -aRectangle->GetPosition().y ),
                      schIUScale.IUToMils( aRectangle->GetEnd().x ),
                      schIUScale.IUToMils( -aRectangle->GetEnd().y ),
                      aRectangle->GetUnit(),
                      aRectangle->GetBodyStyle(),
                      schIUScale.IUToMils( aRectangle->GetWidth() ),
                      fill_tab[ static_cast<int>( aRectangle->GetFillMode() ) - 1 ] );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::saveText( const SCH_TEXT* aText, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aText && aText->Type() == SCH_TEXT_T, "Invalid SCH_TEXT object." );

    wxString text = aText->GetText();

    if( text.Contains( wxT( " " ) ) || text.Contains( wxT( "~" ) ) || text.Contains( wxT( "\"" ) ) )
    {
        // convert double quote to similar-looking two apostrophes
        text.Replace( wxT( "\"" ), wxT( "''" ) );
        text = wxT( "\"" ) + text + wxT( "\"" );
    }

    aFormatter.Print( 0, "T %d %d %d %d %d %d %d %s",
                      aText->GetTextAngle().AsTenthsOfADegree(),
                      schIUScale.IUToMils( aText->GetTextPos().x ),
                      schIUScale.IUToMils( -aText->GetTextPos().y ),
                      schIUScale.IUToMils( aText->GetTextWidth() ),
                      !aText->IsVisible(),
                      aText->GetUnit(),
                      aText->GetBodyStyle(),
                      TO_UTF8( text ) );

    aFormatter.Print( 0, " %s %d", aText->IsItalic() ? "Italic" : "Normal", aText->IsBold() );

    char hjustify = 'C';

    if( aText->GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
        hjustify = 'L';
    else if( aText->GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
        hjustify = 'R';

    char vjustify = 'C';

    if( aText->GetVertJustify() == GR_TEXT_V_ALIGN_BOTTOM )
        vjustify = 'B';
    else if( aText->GetVertJustify() == GR_TEXT_V_ALIGN_TOP )
        vjustify = 'T';

    aFormatter.Print( 0, " %c %c\n", hjustify, vjustify );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::saveDocFile()
{
    /*
     * NB:
     * Some of the rescue code still uses the legacy format as an intermediary, so we have
     * to keep this code.
     */

    wxFileName fileName = m_libFileName;

    fileName.SetExt( FILEEXT::LegacySymbolDocumentFileExtension );
    FILE_OUTPUTFORMATTER formatter( fileName.GetFullPath() );

    formatter.Print( 0, "%s\n", DOCFILE_IDENT );

    for( LIB_SYMBOL_MAP::iterator it = m_symbols.begin();  it != m_symbols.end();  ++it )
    {
        wxString description =  it->second->GetDescription();
        wxString keyWords = it->second->GetKeyWords();
        wxString docFileName = it->second->GetDatasheetField().GetText();

        if( description.IsEmpty() && keyWords.IsEmpty() && docFileName.IsEmpty() )
            continue;

        formatter.Print( 0, "#\n$CMP %s\n", TO_UTF8( it->second->GetName() ) );

        if( !description.IsEmpty() )
            formatter.Print( 0, "D %s\n", TO_UTF8( description ) );

        if( !keyWords.IsEmpty() )
            formatter.Print( 0, "K %s\n", TO_UTF8( keyWords ) );

        if( !docFileName.IsEmpty() )
            formatter.Print( 0, "F %s\n", TO_UTF8( docFileName ) );

        formatter.Print( 0, "$ENDCMP\n" );
    }

    formatter.Print( 0, "#\n#End Doc Library\n" );
}


void SCH_IO_KICAD_LEGACY_LIB_CACHE::DeleteSymbol( const wxString& aSymbolName )
{
    LIB_SYMBOL_MAP::iterator it = m_symbols.find( aSymbolName );

    if( it == m_symbols.end() )
        THROW_IO_ERROR( wxString::Format( _( "library %s does not contain a symbol named %s" ),
                                          m_libFileName.GetFullName(), aSymbolName ) );

    LIB_SYMBOL* symbol = it->second;

    if( symbol->IsRoot() )
    {
        LIB_SYMBOL* rootSymbol = symbol;

        // Remove the root symbol and all its children.
        m_symbols.erase( it );

        LIB_SYMBOL_MAP::iterator it1 = m_symbols.begin();

        while( it1 != m_symbols.end() )
        {
            if( it1->second->IsDerived()
              && it1->second->GetParent().lock() == rootSymbol->SharedPtr() )
            {
                delete it1->second;
                it1 = m_symbols.erase( it1 );
            }
            else
            {
                it1++;
            }
        }

        delete rootSymbol;
    }
    else
    {
        // Just remove the alias.
        m_symbols.erase( it );
        delete symbol;
    }

    SCH_IO_KICAD_LEGACY_LIB_CACHE::IncrementModifyHash();
    m_isModified = true;
}
