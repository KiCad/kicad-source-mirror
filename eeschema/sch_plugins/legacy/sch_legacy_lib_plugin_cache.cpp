/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <lib_shape.h>
#include <lib_field.h>
#include <lib_pin.h>
#include <lib_text.h>
#include <macros.h>
#include <richio.h>
#include <string_utils.h>
#include <template_fieldnames.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>

#include "sch_legacy_lib_plugin_cache.h"
#include "sch_legacy_plugin_helpers.h"


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


SCH_LEGACY_PLUGIN_CACHE::SCH_LEGACY_PLUGIN_CACHE( const wxString& aFullPathAndFileName ) :
    SCH_LIB_PLUGIN_CACHE( aFullPathAndFileName )
{
    m_versionMajor = -1;
    m_versionMinor = -1;
}


void SCH_LEGACY_PLUGIN_CACHE::Load()
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

    if( *line != '.' )
        SCH_PARSE_ERROR( "invalid file version formatting in header", reader, line );

    line++;

    m_versionMinor = parseInt( reader, line, &line );

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

    SCH_LEGACY_PLUGIN_CACHE::IncrementModifyHash();

    // Remember the file modification time of library file when the
    // cache snapshot was made, so that in a networked environment we will
    // reload the cache as needed.
    m_fileModTime = GetLibModificationTime();

    if( USE_OLD_DOC_FILE_FORMAT( m_versionMajor, m_versionMinor ) )
        loadDocs();
}


void SCH_LEGACY_PLUGIN_CACHE::loadDocs()
{
    const char* line;
    wxString    text;
    wxString    aliasName;
    wxFileName  fn = m_libFileName;
    LIB_SYMBOL* symbol = nullptr;;

    fn.SetExt( LegacySymbolDocumentFileExtension );

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
    {
        SCH_PARSE_ERROR( "invalid document library file version formatting in header",
                         reader, line );
    }

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
            wxLogWarning( "Symbol '%s' not found in library:\n\n"
                          "'%s'\n\nat line %d offset %d", aliasName, fn.GetFullPath(),
                          reader.LineNumber(), (int) (line - reader.Line() ) );
        else
            symbol = it->second;

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

            text = FROM_UTF8( line + 2 );
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
                    symbol->GetFieldById( DATASHEET_FIELD )->SetText( text );
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


void SCH_LEGACY_PLUGIN_CACHE::loadHeader( FILE_LINE_READER& aReader )
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


LIB_SYMBOL* SCH_LEGACY_PLUGIN_CACHE::LoadPart( LINE_READER& aReader, int aMajorVersion,
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
    wxStringTokenizer tokens( utf8Line, " \r\n\t" );

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
        THROW_PARSE_ERROR( "expected Y or N", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

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
    {
        THROW_PARSE_ERROR( "invalid unit count", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    symbol->SetUnitCount( (int)num );

    // Ensure m_unitCount is >= 1.  Could be read as 0 in old libraries.
    if( symbol->GetUnitCount() < 1 )
        symbol->SetUnitCount( 1 );

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

    LIB_FIELD& reference = symbol->GetReferenceField();

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
            symbol->SetPower();
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
            return symbol.release();
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "missing ENDDEF", aReader, line );
}


void SCH_LEGACY_PLUGIN_CACHE::loadAliases( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                           LINE_READER&                 aReader,
                                           LIB_SYMBOL_MAP*              aMap )
{
    wxString newAliasName;
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "ALIAS", line, &line ), "Invalid ALIAS section" );

    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \r\n\t" );

    // Parse the ALIAS list.
    while( tokens.HasMoreTokens() )
    {
        newAliasName = tokens.GetNextToken();

        if( aMap )
        {
            LIB_SYMBOL* newSymbol = new LIB_SYMBOL( newAliasName );

            // Inherit the parent mandatory field attributes.
            for( int id = 0; id < MANDATORY_FIELDS; ++id )
            {
                LIB_FIELD* field = newSymbol->GetFieldById( id );

                // the MANDATORY_FIELDS are exactly that in RAM.
                wxASSERT( field );

                LIB_FIELD* parentField = aSymbol->GetFieldById( id );

                wxASSERT( parentField );

                *field = *parentField;

                if( id == VALUE_FIELD )
                    field->SetText( newAliasName );

                field->SetParent( newSymbol );
            }

            newSymbol->SetParent( aSymbol.get() );

            // This will prevent duplicate aliases.
            (*aMap)[ newSymbol->GetName() ] = newSymbol;
        }
    }
}


void SCH_LEGACY_PLUGIN_CACHE::loadField( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                         LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_RET( *line == 'F', "Invalid field line" );

    wxString    text;
    int         id;

    if( sscanf( line + 1, "%d", &id ) != 1 || id < 0 )
        SCH_PARSE_ERROR( "invalid field ID", aReader, line + 1 );

    LIB_FIELD* field;

    if( id >= 0 && id < MANDATORY_FIELDS )
    {
        field = aSymbol->GetFieldById( id );

        // this will fire only if somebody broke a constructor or editor.
        // MANDATORY_FIELDS are always present in ram resident symbols, no
        // exceptions, and they always have their names set, even fixed fields.
        wxASSERT( field );
    }
    else
    {
        field = new LIB_FIELD( aSymbol.get(), id );
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
    if( id == VALUE_FIELD )
        text = EscapeString( text, CTX_QUOTED_STR );

    // Doctor the *.lib file field which has a "~" in blank fields.  New saves will
    // not save like this.
    if( text.size() == 1 && text[0] == '~' )
        field->SetText( wxEmptyString );
    else
        field->SetText( ConvertToNewOverbarNotation( text ) );

    VECTOR2I pos;

    pos.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pos.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
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
                field->SetItalic( true );
            else if( attr_1 != 'N' )   // No italics is default, check for error.
                SCH_PARSE_ERROR( "invalid field text italic parameter", aReader, line );

            if ( attr_2 == 'B' )       // Bold
                field->SetBold( true );
            else if( attr_2 != 'N' )   // No bold is default, check for error.
                SCH_PARSE_ERROR( "invalid field text bold parameter", aReader, line );
        }
    }

    // Fields in RAM must always have names.
    if( id >= 0 && id < MANDATORY_FIELDS )
    {
        // Fields in RAM must always have names, because we are trying to get
        // less dependent on field ids and more dependent on names.
        // Plus assumptions are made in the field editors.
        field->m_name = TEMPLATE_FIELDNAME::GetDefaultFieldName( id );

        // Ensure the VALUE field = the symbol name (can be not the case
        // with malformed libraries: edited by hand, or converted from other tools)
        if( id == VALUE_FIELD )
            field->SetText( aSymbol->GetName() );
    }
    else
    {
        parseQuotedString( field->m_name, aReader, line, &line, true );  // Optional.
    }
}


void SCH_LEGACY_PLUGIN_CACHE::loadDrawEntries( std::unique_ptr<LIB_SYMBOL>& aSymbol,
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
            aSymbol->AddDrawItem( loadArc( aSymbol, aReader ), false );
            break;

        case 'C':    // Circle
            aSymbol->AddDrawItem( loadCircle( aSymbol, aReader ), false );
            break;

        case 'T':    // Text
            aSymbol->AddDrawItem( loadText( aSymbol, aReader, aMajorVersion, aMinorVersion ),
                                  false );
            break;

        case 'S':    // Square
            aSymbol->AddDrawItem( loadRect( aSymbol, aReader ), false );
            break;

        case 'X':    // Pin Description
            aSymbol->AddDrawItem( loadPin( aSymbol, aReader ), false );
            break;

        case 'P':    // Polyline
            aSymbol->AddDrawItem( loadPolyLine( aSymbol, aReader ), false );
            break;

        case 'B':    // Bezier Curves
            aSymbol->AddDrawItem( loadBezier( aSymbol, aReader ), false );
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


FILL_T SCH_LEGACY_PLUGIN_CACHE::parseFillMode( LINE_READER& aReader, const char* aLine,
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


LIB_SHAPE* SCH_LEGACY_PLUGIN_CACHE::loadArc( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                             LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "A", line, &line ), nullptr, "Invalid arc definition" );

    LIB_SHAPE* arc = new LIB_SHAPE( aSymbol.get(), SHAPE_T::ARC );

    VECTOR2I center;

    center.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    center.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

    arc->SetPosition( center );

    (void) schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

    EDA_ANGLE angle1( parseInt( aReader, line, &line ), TENTHS_OF_A_DEGREE_T );
    EDA_ANGLE angle2( parseInt( aReader, line, &line ), TENTHS_OF_A_DEGREE_T );

    arc->SetUnit( parseInt( aReader, line, &line ) );
    arc->SetConvert( parseInt( aReader, line, &line ) );

    STROKE_PARAMS stroke( schIUScale.MilsToIU( parseInt( aReader, line, &line ) ), PLOT_DASH_TYPE::SOLID );

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
        arcStart.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
        arcEnd.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
        arcEnd.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

        arc->SetStart( arcStart );
        arc->SetEnd( arcEnd );
    }
    // Actual Coordinates of arc ends are not read from file (old library), calculate them
    else
    {
        arc->SetArcAngleAndEnd( angle2 - angle1, true );
    }

    /*
     * Current file format stores start-mid-end and so doesn't care about winding. We
     * store start-end with an implied winding internally though.
     * This issue is only for 180 deg arcs, because 180 deg are a limit to handle arcs in
     * legacy libs.
     *
     * So a workaround is to slightly change the arc angle to
     * avoid 180 deg arc after correction
     */
    EDA_ANGLE arc_angle = arc->GetArcAngle();

    if( arc_angle == ANGLE_180 )
    {
        VECTOR2I new_center = CalcArcCenter( arc->GetStart(), arc->GetEnd(),
                              EDA_ANGLE( 179.5, DEGREES_T ) );
        arc->SetCenter( new_center );
    }

    // In legacy libraries, an arc angle is always <= 180.0 degrees
    // So if the created arc is > 180 degrees, swap arc ends to have a < 180 deg arc.
    if( arc->GetArcAngle() > ANGLE_180 )
    {
        VECTOR2I new_end = arc->GetStart();
        VECTOR2I new_start = arc->GetEnd();
        arc->SetStart( new_start );
        arc->SetEnd( new_end );
    }

    return arc;
}


LIB_SHAPE* SCH_LEGACY_PLUGIN_CACHE::loadCircle( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                                LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "C", line, &line ), nullptr, "Invalid circle definition" );

    LIB_SHAPE* circle = new LIB_SHAPE( aSymbol.get(), SHAPE_T::CIRCLE );

    VECTOR2I center;

    center.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    center.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

    int radius = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

    circle->SetStart( center );
    circle->SetEnd( VECTOR2I( center.x + radius, center.y ) );
    circle->SetUnit( parseInt( aReader, line, &line ) );
    circle->SetConvert( parseInt( aReader, line, &line ) );

    STROKE_PARAMS stroke( schIUScale.MilsToIU( parseInt( aReader, line, &line ) ), PLOT_DASH_TYPE::SOLID );

    circle->SetStroke( stroke );

    if( *line != 0 )
        circle->SetFillMode( parseFillMode( aReader, line, &line ) );

    return circle;
}


LIB_TEXT* SCH_LEGACY_PLUGIN_CACHE::loadText( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                             LINE_READER&                 aReader,
                                             int                          aMajorVersion,
                                             int                          aMinorVersion )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "T", line, &line ), nullptr, "Invalid LIB_TEXT definition" );

    LIB_TEXT* text = new LIB_TEXT( aSymbol.get() );
    double    angleInTenths = parseInt( aReader, line, &line );

    text->SetTextAngle( EDA_ANGLE( angleInTenths, TENTHS_OF_A_DEGREE_T ) );

    VECTOR2I center;

    center.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    center.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    text->SetPosition( center );

    VECTOR2I size;

    size.x = size.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    text->SetTextSize( size );
    text->SetVisible( !parseInt( aReader, line, &line ) );
    text->SetUnit( parseInt( aReader, line, &line ) );
    text->SetConvert( parseInt( aReader, line, &line ) );

    wxString str;

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

    text->SetText( str );

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
            text->SetItalic( true );
        else if( !strCompare( "Normal", line, &line ) )
            SCH_PARSE_ERROR( "invalid text stype, expected 'Normal' or 'Italic'", aReader, line );

        if( parseInt( aReader, line, &line ) > 0 )
            text->SetBold( true );

        // Some old libaries version > 2.0 do not have these options for text justification:
        if( !is_eol( *line ) )
        {
            switch( parseChar( aReader, line, &line ) )
            {
            case 'L': text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );   break;
            case 'C': text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER ); break;
            case 'R': text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );  break;
            default: SCH_PARSE_ERROR( "invalid horizontal text justication; expected L, C, or R",
                                      aReader, line );
            }

            switch( parseChar( aReader, line, &line ) )
            {
            case 'T': text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );    break;
            case 'C': text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER ); break;
            case 'B': text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM ); break;
            default: SCH_PARSE_ERROR( "invalid vertical text justication; expected T, C, or B",
                                      aReader, line );
            }
        }
    }

    return text;
}


LIB_SHAPE* SCH_LEGACY_PLUGIN_CACHE::loadRect( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                              LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "S", line, &line ), nullptr, "Invalid rectangle definition" );

    LIB_SHAPE* rectangle = new LIB_SHAPE( aSymbol.get(), SHAPE_T::RECTANGLE );

    VECTOR2I pos;

    pos.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pos.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    rectangle->SetPosition( pos );

    VECTOR2I end;

    end.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    end.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    rectangle->SetEnd( end );

    rectangle->SetUnit( parseInt( aReader, line, &line ) );
    rectangle->SetConvert( parseInt( aReader, line, &line ) );

    STROKE_PARAMS stroke( schIUScale.MilsToIU( parseInt( aReader, line, &line ) ), PLOT_DASH_TYPE::SOLID );

    rectangle->SetStroke( stroke );


    if( *line != 0 )
        rectangle->SetFillMode( parseFillMode( aReader, line, &line ) );

    return rectangle;
}


LIB_PIN* SCH_LEGACY_PLUGIN_CACHE::loadPin( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                           LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "X", line, &line ), nullptr, "Invalid LIB_PIN definition" );

    wxString name;
    wxString number;

    size_t pos = 2;                               // "X" plus ' ' space character.
    wxString tmp;
    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \r\n\t" );

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
    position.y = schIUScale.MilsToIU( (int) num );

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

    PIN_ORIENTATION orientation = PIN_ORIENTATION::PIN_RIGHT;
    std::optional<PIN_ORIENTATION> optVal = magic_enum::enum_cast<PIN_ORIENTATION>( tmp[0] );

    if( optVal.has_value() )
        orientation = optVal.value();

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
        THROW_PARSE_ERROR( "invalid pin alternate body type", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );
    }

    pos += tmp.size() + 1;
    int convert = (int) num;

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


    LIB_PIN* pin = new LIB_PIN( aSymbol.get(),
                                ConvertToNewOverbarNotation( name ),
                                ConvertToNewOverbarNotation( number ),
                                orientation,
                                pinType,
                                length,
                                nameTextSize,
                                numberTextSize,
                                convert,
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

        for( int j = tmp.size(); j > 0; )
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
        default:
            SCH_PARSE_ERROR( "pin attributes do not define a valid pin shape", aReader, line );
        }
    }

    return pin;
}


LIB_SHAPE* SCH_LEGACY_PLUGIN_CACHE::loadPolyLine( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                                   LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "P", line, &line ), nullptr, "Invalid poly definition" );

    LIB_SHAPE* polyLine = new LIB_SHAPE( aSymbol.get(), SHAPE_T::POLY );

    int points = parseInt( aReader, line, &line );
    polyLine->SetUnit( parseInt( aReader, line, &line ) );
    polyLine->SetConvert( parseInt( aReader, line, &line ) );

    STROKE_PARAMS stroke( schIUScale.MilsToIU( parseInt( aReader, line, &line ) ), PLOT_DASH_TYPE::SOLID );

    polyLine->SetStroke( stroke );

    VECTOR2I pt;

    for( int i = 0; i < points; i++ )
    {
        pt.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
        pt.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
        polyLine->AddPoint( pt );
    }

    if( *line != 0 )
        polyLine->SetFillMode( parseFillMode( aReader, line, &line ) );

    return polyLine;
}


LIB_SHAPE* SCH_LEGACY_PLUGIN_CACHE::loadBezier( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                                LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "B", line, &line ), nullptr, "Invalid Bezier definition" );

    int points = parseInt( aReader, line, &line );

    wxCHECK_MSG( points == 4, NULL, "Invalid Bezier curve definition" );

    LIB_SHAPE* bezier = new LIB_SHAPE( aSymbol.get(), SHAPE_T::BEZIER );

    bezier->SetUnit( parseInt( aReader, line, &line ) );
    bezier->SetConvert( parseInt( aReader, line, &line ) );

    STROKE_PARAMS stroke ( schIUScale.MilsToIU( parseInt( aReader, line, &line ) ), PLOT_DASH_TYPE::SOLID );

    bezier->SetStroke( stroke );

    VECTOR2I pt;

    pt.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pt.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    bezier->SetStart( pt );

    pt.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pt.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    bezier->SetBezierC1( pt );

    pt.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pt.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    bezier->SetBezierC2( pt );

    pt.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pt.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    bezier->SetEnd( pt );

    bezier->RebuildBezierToSegmentsPointsList( bezier->GetWidth() );

    if( *line != 0 )
        bezier->SetFillMode( parseFillMode( aReader, line, &line ) );

    return bezier;
}


void SCH_LEGACY_PLUGIN_CACHE::loadFootprintFilters( std::unique_ptr<LIB_SYMBOL>& aSymbol,
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


void SCH_LEGACY_PLUGIN_CACHE::Save( const std::optional<bool>& aOpt )
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

    m_fileModTime = fn.GetModificationTime();
    m_isModified = false;

    if( doSaveDocFile )
        saveDocFile();
}


void SCH_LEGACY_PLUGIN_CACHE::SaveSymbol( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter,
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

            if( symbol->IsAlias() && symbol->GetParent().lock() == aSymbol->SharedPtr() )
                aliasNames.Add( symbol->GetName() );
        }
    }

    LIB_FIELD&  value = aSymbol->GetValueField();

    // First line: it s a comment (symbol name for readers)
    aFormatter.Print( 0, "#\n# %s\n#\n", TO_UTF8( value.GetText() ) );

    // Save data
    aFormatter.Print( 0, "DEF" );
    aFormatter.Print( 0, " %s", TO_UTF8( value.GetText() ) );

    LIB_FIELD& reference = aSymbol->GetReferenceField();

    if( !reference.GetText().IsEmpty() )
        aFormatter.Print( 0, " %s", TO_UTF8( reference.GetText() ) );
    else
        aFormatter.Print( 0, " ~" );

    aFormatter.Print( 0, " %d %d %c %c %d %c %c\n",
                      0, schIUScale.IUToMils( aSymbol->GetPinNameOffset() ),
                      aSymbol->ShowPinNumbers() ? 'Y' : 'N',
                      aSymbol->ShowPinNames() ? 'Y' : 'N',
                      aSymbol->GetUnitCount(), aSymbol->UnitsLocked() ? 'L' : 'F',
                      aSymbol->IsPower() ? 'P' : 'N' );

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

    std::vector<LIB_FIELD*> fields;
    aSymbol->GetFields( fields );

    // Mandatory fields:
    // may have their own save policy so there is a separate loop for them.
    // Empty fields are saved, because the user may have set visibility,
    // size and orientation
    for( int i = 0; i < MANDATORY_FIELDS; ++i )
        saveField( fields[i], aFormatter );

    // User defined fields:
    // may have their own save policy so there is a separate loop for them.
    int fieldId = MANDATORY_FIELDS;     // really wish this would go away.

    for( unsigned i = MANDATORY_FIELDS; i < fields.size(); ++i )
    {
        // There is no need to save empty fields, i.e. no reason to preserve field
        // names now that fields names come in dynamically through the template
        // fieldnames.
        if( !fields[i]->GetText().IsEmpty() )
        {
            fields[i]->SetId( fieldId++ );
            saveField( fields[i], aFormatter );
        }
    }

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

        for( LIB_ITEM& item : aSymbol->GetDrawItems() )
        {
            switch( item.Type() )
            {
            default:
            case LIB_FIELD_T:     /* Fields have already been saved above. */  break;
            case LIB_PIN_T:       savePin( (LIB_PIN* ) &item, aFormatter );    break;
            case LIB_TEXT_T:      saveText( ( LIB_TEXT* ) &item, aFormatter ); break;
            case LIB_SHAPE_T:
            {
                LIB_SHAPE& shape = static_cast<LIB_SHAPE&>( item );

                switch( shape.GetShape() )
                {
                case SHAPE_T::ARC:    saveArc( &shape, aFormatter );           break;
                case SHAPE_T::BEZIER: saveBezier( &shape, aFormatter );        break;
                case SHAPE_T::CIRCLE: saveCircle( &shape, aFormatter );        break;
                case SHAPE_T::POLY:   savePolyLine( &shape, aFormatter );      break;
                case SHAPE_T::RECTANGLE:   saveRectangle( &shape, aFormatter );     break;
                default:                                                       break;
                }
            }
            }
        }

        aFormatter.Print( 0, "ENDDRAW\n" );
    }

    aFormatter.Print( 0, "ENDDEF\n" );
}


void SCH_LEGACY_PLUGIN_CACHE::saveArc( LIB_SHAPE* aArc, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aArc && aArc->GetShape() == SHAPE_T::ARC, "Invalid ARC object." );

    EDA_ANGLE startAngle, endAngle;

    aArc->CalcArcAngles( startAngle, endAngle );
    startAngle.Normalize180();
    endAngle.Normalize180();

    aFormatter.Print( 0, "A %d %d %d %d %d %d %d %d %c %d %d %d %d\n",
                      schIUScale.IUToMils( aArc->GetPosition().x ),
                      schIUScale.IUToMils( aArc->GetPosition().y ),
                      schIUScale.IUToMils( aArc->GetRadius() ),
                      startAngle.AsTenthsOfADegree(),
                      endAngle.AsTenthsOfADegree(),
                      aArc->GetUnit(),
                      aArc->GetConvert(),
                      schIUScale.IUToMils( aArc->GetWidth() ),
                      fill_tab[ static_cast<int>( aArc->GetFillMode() ) - 1 ],
                      schIUScale.IUToMils( aArc->GetStart().x ),
                      schIUScale.IUToMils( aArc->GetStart().y ),
                      schIUScale.IUToMils( aArc->GetEnd().x ),
                      schIUScale.IUToMils( aArc->GetEnd().y ) );
}


void SCH_LEGACY_PLUGIN_CACHE::saveBezier( LIB_SHAPE* aBezier, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aBezier && aBezier->GetShape() == SHAPE_T::BEZIER, "Invalid BEZIER object." );

    aFormatter.Print( 0, "B 4 %d %d %d",
                      aBezier->GetUnit(),
                      aBezier->GetConvert(),
                      schIUScale.IUToMils( aBezier->GetWidth() ) );

    aFormatter.Print( 0, " %d %d %d %d %d %d %d %d",
                      schIUScale.IUToMils( aBezier->GetStart().x ),
                      schIUScale.IUToMils( aBezier->GetStart().y ),
                      schIUScale.IUToMils( aBezier->GetBezierC1().x ),
                      schIUScale.IUToMils( aBezier->GetBezierC1().y ),
                      schIUScale.IUToMils( aBezier->GetBezierC2().x ),
                      schIUScale.IUToMils( aBezier->GetBezierC2().y ),
                      schIUScale.IUToMils( aBezier->GetEnd().x ),
                      schIUScale.IUToMils( aBezier->GetEnd().y ) );

    aFormatter.Print( 0, " %c\n", fill_tab[ static_cast<int>( aBezier->GetFillMode() ) - 1 ] );
}


void SCH_LEGACY_PLUGIN_CACHE::saveCircle( LIB_SHAPE* aCircle, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aCircle && aCircle->GetShape() == SHAPE_T::CIRCLE, "Invalid CIRCLE object." );

    aFormatter.Print( 0, "C %d %d %d %d %d %d %c\n",
                      schIUScale.IUToMils( aCircle->GetPosition().x ),
                      schIUScale.IUToMils( aCircle->GetPosition().y ),
                      schIUScale.IUToMils( aCircle->GetRadius() ),
                      aCircle->GetUnit(),
                      aCircle->GetConvert(),
                      schIUScale.IUToMils( aCircle->GetWidth() ),
                      fill_tab[ static_cast<int>( aCircle->GetFillMode() ) - 1 ] );
}


void SCH_LEGACY_PLUGIN_CACHE::saveField( const LIB_FIELD* aField, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aField && aField->Type() == LIB_FIELD_T, "Invalid LIB_FIELD object." );

    int      hjustify, vjustify;
    int      id = aField->GetId();
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
                      id,
                      EscapedUTF8( text ).c_str(),       // wraps in quotes
                      schIUScale.IUToMils( aField->GetTextPos().x ),
                      schIUScale.IUToMils( aField->GetTextPos().y ),
                      schIUScale.IUToMils( aField->GetTextWidth() ),
                      aField->GetTextAngle().IsHorizontal() ? 'H' : 'V',
                      aField->IsVisible() ? 'V' : 'I',
                      hjustify, vjustify,
                      aField->IsItalic() ? 'I' : 'N',
                      aField->IsBold() ? 'B' : 'N' );

    /* Save field name, if necessary
     * Field name is saved only if it is not the default name.
     * Just because default name depends on the language and can change from
     * a country to another
     */
    wxString defName = TEMPLATE_FIELDNAME::GetDefaultFieldName( id );

    if( id >= MANDATORY_FIELDS && !aField->m_name.IsEmpty() && aField->m_name != defName )
        aFormatter.Print( 0, " %s", EscapedUTF8( aField->m_name ).c_str() );

    aFormatter.Print( 0, "\n" );
}


void SCH_LEGACY_PLUGIN_CACHE::savePin( const LIB_PIN* aPin, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aPin && aPin->Type() == LIB_PIN_T, "Invalid LIB_PIN object." );

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

    aFormatter.Print( 0, " %s %d %d %d %c %d %d %d %d %c",
                      aPin->GetNumber().IsEmpty() ? "~" : TO_UTF8( aPin->GetNumber() ),
                      schIUScale.IUToMils( aPin->GetPosition().x ),
                      schIUScale.IUToMils( aPin->GetPosition().y ),
                      schIUScale.IUToMils( (int) aPin->GetLength() ),
                      (int) aPin->GetOrientation(),
                      schIUScale.IUToMils( aPin->GetNumberTextSize() ),
                      schIUScale.IUToMils( aPin->GetNameTextSize() ),
                      aPin->GetUnit(),
                      aPin->GetConvert(),
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

    const_cast<LIB_PIN*>( aPin )->ClearFlags( IS_CHANGED );
}


void SCH_LEGACY_PLUGIN_CACHE::savePolyLine( LIB_SHAPE* aPolyLine, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aPolyLine && aPolyLine->GetShape() == SHAPE_T::POLY, "Invalid POLY object." );

    aFormatter.Print( 0, "P %d %d %d %d",
                      (int) aPolyLine->GetPolyShape().Outline( 0 ).GetPointCount(),
                      aPolyLine->GetUnit(),
                      aPolyLine->GetConvert(),
                      schIUScale.IUToMils( aPolyLine->GetWidth() ) );

    for( const VECTOR2I& pt : aPolyLine->GetPolyShape().Outline( 0 ).CPoints() )
        aFormatter.Print( 0, " %d %d", schIUScale.IUToMils( pt.x ), schIUScale.IUToMils( pt.y ) );

    aFormatter.Print( 0, " %c\n", fill_tab[ static_cast<int>( aPolyLine->GetFillMode() ) - 1 ] );
}


void SCH_LEGACY_PLUGIN_CACHE::saveRectangle( LIB_SHAPE* aRectangle, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aRectangle && aRectangle->GetShape() == SHAPE_T::RECTANGLE, "Invalid RECT object." );

    aFormatter.Print( 0, "S %d %d %d %d %d %d %d %c\n",
                      schIUScale.IUToMils( aRectangle->GetPosition().x ),
                      schIUScale.IUToMils( aRectangle->GetPosition().y ),
                      schIUScale.IUToMils( aRectangle->GetEnd().x ),
                      schIUScale.IUToMils( aRectangle->GetEnd().y ),
                      aRectangle->GetUnit(),
                      aRectangle->GetConvert(),
                      schIUScale.IUToMils( aRectangle->GetWidth() ),
                      fill_tab[ static_cast<int>( aRectangle->GetFillMode() ) - 1 ] );
}


void SCH_LEGACY_PLUGIN_CACHE::saveText( const LIB_TEXT* aText, OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aText && aText->Type() == LIB_TEXT_T, "Invalid LIB_TEXT object." );

    wxString text = aText->GetText();

    if( text.Contains( wxT( " " ) ) || text.Contains( wxT( "~" ) ) || text.Contains( wxT( "\"" ) ) )
    {
        // convert double quote to similar-looking two apostrophes
        text.Replace( wxT( "\"" ), wxT( "''" ) );
        text = wxT( "\"" ) + text + wxT( "\"" );
    }

    aFormatter.Print( 0, "T %g %d %d %d %d %d %d %s",
                      (double) aText->GetTextAngle().AsTenthsOfADegree(),
                      schIUScale.IUToMils( aText->GetTextPos().x ),
                      schIUScale.IUToMils( aText->GetTextPos().y ),
                      schIUScale.IUToMils( aText->GetTextWidth() ),
                      !aText->IsVisible(),
                      aText->GetUnit(),
                      aText->GetConvert(),
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


void SCH_LEGACY_PLUGIN_CACHE::saveDocFile()
{
    /*
     * NB:
     * Some of the rescue code still uses the legacy format as an intermediary, so we have
     * to keep this code.
     */

    wxFileName fileName = m_libFileName;

    fileName.SetExt( LegacySymbolDocumentFileExtension );
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


void SCH_LEGACY_PLUGIN_CACHE::DeleteSymbol( const wxString& aSymbolName )
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
            if( it1->second->IsAlias()
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

    SCH_LEGACY_PLUGIN_CACHE::IncrementModifyHash();
    m_isModified = true;
}
