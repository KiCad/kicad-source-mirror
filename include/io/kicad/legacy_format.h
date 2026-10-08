/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include <wx/base64.h>
#include <wx/stream.h>

#include <base_units.h>
#include <common.h>
#include <embedded_files.h>
#include <io/kicad/legacy_text_format.h>
#include <ki_exception.h>
#include <kiid.h>
#include <lib_id.h>
#include <mmh3_hash.h>
#include <page_info.h>
#include <richio.h>
#include <string_utils.h>
#include <stroke_params.h>
#include <title_block.h>

namespace KICAD_FORMAT::LEGACY
{

// Serialization grammar from 9.0.0 ccafeabf1503 and 10.0.7 93a8a4827b0b.
// Only primitive quoting and number conversion are shared with the current implementation.

inline constexpr int OMIT_HIDE = 1 << 6;


inline bool IsDefaultTextV10( const EDA_TEXT& aText )
{
    return !aText.IsMirrored() && aText.GetHorizJustify() == GR_TEXT_H_ALIGN_CENTER
           && aText.GetVertJustify() == GR_TEXT_V_ALIGN_CENTER && aText.GetAutoThickness() && !aText.IsItalic()
           && !aText.IsBold() && !aText.IsMultilineAllowed() && aText.GetFontName().IsEmpty();
}

inline void FormatBool( OUTPUTFORMATTER* aOut, const wxString& aKey, bool aValue )
{
    aOut->Print( "(%ls %s)", aKey.wc_str(), aValue ? "yes" : "no" );
}


inline void FormatOptBool( OUTPUTFORMATTER* aOut, const wxString& aKey, std::optional<bool> aValue )
{
    if( aValue.has_value() )
        FormatBool( aOut, aKey, *aValue );
    else
        aOut->Print( "(%ls none)", aKey.wc_str() );
}


inline void FormatUuid( OUTPUTFORMATTER* aOut, const KIID& aUuid )
{
    aOut->Print( "(uuid %s)", aOut->Quotew( aUuid.AsString() ).c_str() );
}


inline UTF8 FormatLibId( const LIB_ID& aId )
{
    UTF8 result;

    if( !aId.GetLibNickname().empty() )
    {
        result += aId.GetLibNickname();
        result += ':';
    }

    result += aId.GetLibItemName();
    return result;
}


inline void FormatStreamData( OUTPUTFORMATTER& aOut, const wxStreamBuffer& aStream )
{
    aOut.Print( "(data" );
    const wxString encoded = wxBase64Encode( aStream.GetBufferStart(), aStream.GetBufferSize() );

    for( size_t first = 0; first < encoded.Length(); first += 76 )
        aOut.Print( "\n\"%s\"", TO_UTF8( encoded( first, 76 ) ) );

    aOut.Print( ")" );
}


inline const char* LineStyleToken( LINE_STYLE aStyle )
{
    switch( aStyle )
    {
    case LINE_STYLE::DASH: return "dash";
    case LINE_STYLE::DOT: return "dot";
    case LINE_STYLE::DASHDOT: return "dash_dot";
    case LINE_STYLE::DASHDOTDOT: return "dash_dot_dot";
    case LINE_STYLE::SOLID: return "solid";
    case LINE_STYLE::DEFAULT: return "default";
    }

    THROW_IO_ERROR( wxT( "Unreviewed line style in a frozen writer." ) );
}


// Stroke and title-block grammar is identical in both pinned releases.
inline void FormatStroke( OUTPUTFORMATTER* aOut, const STROKE_PARAMS& aStroke, const EDA_IU_SCALE& aScale )
{
    const auto color = aStroke.GetColor();

    if( color == KIGFX::COLOR4D::UNSPECIFIED )
    {
        aOut->Print( "(stroke (width %s) (type %s))",
                     EDA_UNIT_UTILS::FormatInternalUnits( aScale, aStroke.GetWidth() ).c_str(),
                     LineStyleToken( aStroke.GetLineStyle() ) );
    }
    else
    {
        aOut->Print( "(stroke (width %s) (type %s) (color %d %d %d %s))",
                     EDA_UNIT_UTILS::FormatInternalUnits( aScale, aStroke.GetWidth() ).c_str(),
                     LineStyleToken( aStroke.GetLineStyle() ), KiROUND( color.r * 255.0 ), KiROUND( color.g * 255.0 ),
                     KiROUND( color.b * 255.0 ), FormatDouble2Str( color.a ).c_str() );
    }
}


inline const char* PageTypeToken( PAGE_SIZE_TYPE aType )
{
    switch( aType )
    {
    case PAGE_SIZE_TYPE::A5: return "A5";
    case PAGE_SIZE_TYPE::A4: return "A4";
    case PAGE_SIZE_TYPE::A3: return "A3";
    case PAGE_SIZE_TYPE::A2: return "A2";
    case PAGE_SIZE_TYPE::A1: return "A1";
    case PAGE_SIZE_TYPE::A0: return "A0";
    case PAGE_SIZE_TYPE::A: return "A";
    case PAGE_SIZE_TYPE::B: return "B";
    case PAGE_SIZE_TYPE::C: return "C";
    case PAGE_SIZE_TYPE::D: return "D";
    case PAGE_SIZE_TYPE::E: return "E";
    case PAGE_SIZE_TYPE::GERBER: return "GERBER";
    case PAGE_SIZE_TYPE::USLetter: return "USLetter";
    case PAGE_SIZE_TYPE::USLegal: return "USLegal";
    case PAGE_SIZE_TYPE::USLedger: return "USLedger";
    case PAGE_SIZE_TYPE::User: return "User";
    }

    THROW_IO_ERROR( wxT( "Unreviewed paper type in a frozen writer." ) );
}


inline void FormatPageV9( OUTPUTFORMATTER* aOut, const PAGE_INFO& aPage )
{
    aOut->Print( "(paper %s", aOut->Quotew( PageTypeToken( aPage.GetType() ) ).c_str() );

    if( aPage.GetType() == PAGE_SIZE_TYPE::User )
        aOut->Print( " %g %g", aPage.GetWidthMils() * 25.4 / 1000.0, aPage.GetHeightMils() * 25.4 / 1000.0 );

    if( aPage.GetType() != PAGE_SIZE_TYPE::User && aPage.IsPortrait() )
        aOut->Print( " portrait" );

    aOut->Print( ")" );
}


inline void FormatPageV10( OUTPUTFORMATTER* aOut, const PAGE_INFO& aPage )
{
    aOut->Print( "(paper %s", aOut->Quotew( PageTypeToken( aPage.GetType() ) ).c_str() );

    if( aPage.GetType() == PAGE_SIZE_TYPE::User )
    {
        aOut->Print( " %s %s", FormatDouble2Str( aPage.GetWidthMils() * 25.4 / 1000.0 ).c_str(),
                     FormatDouble2Str( aPage.GetHeightMils() * 25.4 / 1000.0 ).c_str() );
    }

    if( aPage.GetType() != PAGE_SIZE_TYPE::User && aPage.IsPortrait() )
        aOut->Print( " portrait" );

    aOut->Print( ")" );
}


inline void FormatTitle( OUTPUTFORMATTER* aOut, const TITLE_BLOCK& aTitle )
{
    bool empty = aTitle.GetTitle().IsEmpty() && aTitle.GetDate().IsEmpty() && aTitle.GetRevision().IsEmpty()
                 && aTitle.GetCompany().IsEmpty();

    for( int ii = 0; ii < 9; ++ii )
        empty = empty && aTitle.GetComment( ii ).IsEmpty();

    if( empty )
        return;

    aOut->Print( "(title_block" );

    if( !aTitle.GetTitle().IsEmpty() )
        aOut->Print( "(title %s)", aOut->Quotew( aTitle.GetTitle() ).c_str() );

    if( !aTitle.GetDate().IsEmpty() )
        aOut->Print( "(date %s)", aOut->Quotew( aTitle.GetDate() ).c_str() );

    if( !aTitle.GetRevision().IsEmpty() )
        aOut->Print( "(rev %s)", aOut->Quotew( aTitle.GetRevision() ).c_str() );

    if( !aTitle.GetCompany().IsEmpty() )
        aOut->Print( "(company %s)", aOut->Quotew( aTitle.GetCompany() ).c_str() );

    for( int ii = 0; ii < 9; ++ii )
    {
        if( !aTitle.GetComment( ii ).IsEmpty() )
            aOut->Print( "(comment %d %s)", ii + 1, aOut->Quotew( aTitle.GetComment( ii ) ).c_str() );
    }

    aOut->Print( ")" );
}


inline void FormatText( OUTPUTFORMATTER* aOut, const EDA_TEXT& aText, const EDA_IU_SCALE& aScale, int aControlBits,
                        bool aV9 )
{
    aOut->Print( "(effects(font" );

    if( aText.GetFont() && !aText.GetFont()->GetName().IsEmpty() )
        aOut->Print( "(face %s)", aOut->Quotew( aText.GetFont()->GetName() ).c_str() );

    aOut->Print( "(size %s %s)", EDA_UNIT_UTILS::FormatInternalUnits( aScale, aText.GetTextHeight() ).c_str(),
                 EDA_UNIT_UTILS::FormatInternalUnits( aScale, aText.GetTextWidth() ).c_str() );

    if( aText.GetLineSpacing() != 1.0 )
        aOut->Print( "(line_spacing %s)", FormatDouble2Str( aText.GetLineSpacing() ).c_str() );

    const int thickness = GetLegacyTextThickness( aText );

    if( aV9 ? aText.GetTextThickness() != 0 : !aText.GetAutoThickness() )
        aOut->Print( "(thickness %s)", EDA_UNIT_UTILS::FormatInternalUnits( aScale, thickness ).c_str() );

    if( aText.IsBold() )
        FormatBool( aOut, "bold", true );

    if( aText.IsItalic() )
        FormatBool( aOut, "italic", true );

    if( !( aControlBits & ( 1 << 11 ) ) && aText.GetTextColor() != KIGFX::COLOR4D::UNSPECIFIED )
    {
        const auto color = aText.GetTextColor();
        aOut->Print( "(color %d %d %d %s)", KiROUND( color.r * 255.0 ), KiROUND( color.g * 255.0 ),
                     KiROUND( color.b * 255.0 ), FormatDouble2Str( color.a ).c_str() );
    }

    aOut->Print( ")" );

    if( aText.IsMirrored() || aText.GetHorizJustify() != GR_TEXT_H_ALIGN_CENTER
        || aText.GetVertJustify() != GR_TEXT_V_ALIGN_CENTER )
    {
        aOut->Print( "(justify" );

        if( aText.GetHorizJustify() != GR_TEXT_H_ALIGN_CENTER )
            aOut->Print( aText.GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT ? " left" : " right" );

        if( aText.GetVertJustify() != GR_TEXT_V_ALIGN_CENTER )
            aOut->Print( aText.GetVertJustify() == GR_TEXT_V_ALIGN_TOP ? " top" : " bottom" );

        if( aText.IsMirrored() )
            aOut->Print( " mirror" );

        aOut->Print( ")" );
    }

    if( aV9 && !( aControlBits & OMIT_HIDE ) && !aText.IsVisible() )
        FormatBool( aOut, "hide", true );

    if( !( aControlBits & ( 1 << 12 ) ) && aText.HasHyperlink() )
        aOut->Print( "(href %s)", aOut->Quotew( aText.GetHyperlink() ).c_str() );

    aOut->Print( ")" );
}


inline void FormatTextV9( OUTPUTFORMATTER* aOut, const EDA_TEXT& aText, const EDA_IU_SCALE& aScale, int aControlBits )
{
    FormatText( aOut, aText, aScale, aControlBits, true );
}


inline void FormatTextV10( OUTPUTFORMATTER* aOut, const EDA_TEXT& aText, const EDA_IU_SCALE& aScale, int aControlBits )
{
    FormatText( aOut, aText, aScale, aControlBits, false );
}


inline std::string EmbeddedFileChecksum( const EMBEDDED_FILES::EMBEDDED_FILE& aFile )
{
    if( aFile.compressedEncodedData.empty() )
        return aFile.data_hash;

    const std::vector<char>*      payload = &aFile.decompressedData;
    EMBEDDED_FILES::EMBEDDED_FILE decoded;

    if( payload->empty() )
    {
        // Decode a scratch copy so zero-byte and encoded-only attachments leave the source intact.
        decoded = aFile;

        if( EMBEDDED_FILES::DecompressAndDecode( decoded ) != EMBEDDED_FILES::RETURN_CODE::OK )
            THROW_IO_ERROR( wxT( "Cannot decode embedded file for an older KiCad version: " ) + aFile.name );

        payload = &decoded.decompressedData;
    }

    // The target readers use the checksum from before the MMH3 tail-byte correction.
    MMH3_HASH hash( EMBEDDED_FILES::Seed() );
    hash.addDataV1( reinterpret_cast<const uint8_t*>( payload->data() ), payload->size() );
    return hash.digest().ToString();
}


inline void FormatEmbeddedFiles( OUTPUTFORMATTER& aOut, const EMBEDDED_FILES& aFiles, bool aWriteData, bool aV9 )
{
    aOut.Print( "(embedded_files " );

    for( const auto& [name, entry] : aFiles.EmbeddedFileMap() )
    {
        const auto& file = *entry;

        if( !aV9 && file.compressedEncodedData.empty() )
            continue;

        // The 9.0 reader requires bar delimiters and cannot read the old writer's empty data node.
        if( aV9 && aWriteData && file.compressedEncodedData.empty() )
            THROW_IO_ERROR( wxT( "Empty embedded file data cannot be written for KiCad 9." ) );

        const char* type = nullptr;

        switch( file.type )
        {
        case EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::DATASHEET: type = "datasheet"; break;
        case EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::FONT: type = "font"; break;
        case EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::MODEL: type = "model"; break;
        case EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::WORKSHEET: type = "worksheet"; break;
        case EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::OTHER: type = "other"; break;
        default: THROW_IO_ERROR( wxT( "Unreviewed embedded file type in a frozen writer." ) );
        }

        const std::string checksum = EmbeddedFileChecksum( file );
        aOut.Print( "(file (name %s)(type %s)", aOut.Quotew( file.name ).c_str(), type );

        if( aWriteData )
        {
            aOut.Print( "(data" );

            for( size_t first = 0; first < file.compressedEncodedData.length(); first += 76 )
            {
                const size_t remaining = file.compressedEncodedData.length() - first;
                const int    length = static_cast<int>( std::min<size_t>( remaining, 76 ) );
                aOut.Print( "\n%1s%.*s%s\n", first ? "" : "|", length, file.compressedEncodedData.data() + first,
                            remaining == size_t( length ) ? "|" : "" );
            }

            aOut.Print( ")" );
        }

        aOut.Print( "(checksum %s))", aOut.Quotew( checksum ).c_str() );
    }

    aOut.Print( ")" );
}


inline void FormatEmbeddedFilesV9( OUTPUTFORMATTER& aOut, const EMBEDDED_FILES& aFiles, bool aWriteData )
{
    FormatEmbeddedFiles( aOut, aFiles, aWriteData, true );
}


inline void FormatEmbeddedFilesV10( OUTPUTFORMATTER& aOut, const EMBEDDED_FILES& aFiles, bool aWriteData )
{
    FormatEmbeddedFiles( aOut, aFiles, aWriteData, false );
}

} // namespace KICAD_FORMAT::LEGACY
