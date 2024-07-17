/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EMBEDDED_FILES_H
#define EMBEDDED_FILES_H

#include <map>

#include <wx/string.h>
#include <wx/filename.h>

#include <embedded_files_lexer.h>
#include <wildcards_and_files_ext.h>
#include <richio.h>
#include <picosha2.h>

class EMBEDDED_FILES
{
public:
    struct EMBEDDED_FILE
    {
        enum class FILE_TYPE
        {
            FONT,
            MODEL,
            WORKSHEET,
            DATASHEET,
            OTHER
        };

        EMBEDDED_FILE() :
                type( FILE_TYPE::OTHER ),
                is_valid( false )
        {}

        bool Validate()
        {
            std::string new_sha;
            picosha2::hash256_hex_string( decompressedData, new_sha );

            is_valid = ( new_sha == data_sha );
            return is_valid;
        }

        wxString GetLink() const
        {
            return wxString::Format( "%s://%s", FILEEXT::KiCadUriPrefix, name );
        }

        wxString          name;
        FILE_TYPE         type;
        bool              is_valid;
        std::string       compressedEncodedData;
        std::vector<char> decompressedData;
        std::string       data_sha;
    };

    enum class RETURN_CODE : int
    {
        OK,                  // Success
        FILE_NOT_FOUND,      // File not found on disk
        PERMISSIONS_ERROR,   // Could not read/write file
        FILE_ALREADY_EXISTS, // File already exists in the collection
        OUT_OF_MEMORY,       // Could not allocate memory
        CHECKSUM_ERROR,      // Checksum in file does not match data
    };

    EMBEDDED_FILES() = default;

    ~EMBEDDED_FILES()
    {
        for( auto& file : m_files )
            delete file.second;
    }

    /**
     * Loads a file from disk and adds it to the collection.
     * @param aName is the name of the file to load.
     * @param aOverwrite is true if the file should be overwritten if it already exists.
    */
    EMBEDDED_FILE* AddFile( const wxFileName& aName, bool aOverwrite );

    /**
     * Appends a file to the collection.
    */
    void AddFile( EMBEDDED_FILE* aFile );

    /**
     * Removes a file from the collection and frees the memory.
     * @param aName is the name of the file to remove.
    */
    void RemoveFile( const wxString& name, bool aErase = true );

    /**
     * Output formatter for the embedded files.
     * @param aOut is the output formatter.
     * @param aNestLevel is the current indentation level.
     * @param aWriteData is true if the actual data should be written.  This is false when writing an element
     *                   that is already embedded in a file that itself has embedded files (boards, schematics, etc.)
    */
    void WriteEmbeddedFiles( OUTPUTFORMATTER& aOut, int aNestLevel, bool aWriteData ) const;

    /**
     * Returns the link for an embedded file.
     * @param aFile is the file to get the link for.
     * @return the link for the file to be used in a hyperlink.
    */
    wxString GetEmbeddedFileLink( const EMBEDDED_FILE& aFile ) const
    {
        return aFile.GetLink();
    }

    bool HasFile( const wxString& name ) const
    {
        wxFileName fileName( name );

        return m_files.find( fileName.GetFullName() ) != m_files.end();
    }

    bool IsEmpty() const
    {
        return m_files.empty();
    }

    /**
     * Helper function to get a list of fonts for fontconfig to add to the library.
     *
     * This is neccesary because EMBEDDED_FILES lives in common at the moment and
     * fontconfig is in libkicommon.  This will create the cache files in the KiCad
     * cache directory (if they do not already exist) and return the temp files names
    */
    const std::vector<wxString>* UpdateFontFiles();

    /**
     * If we just need the cached version of the font files, we can use this function which
     * is const and will not update the font files.
    */
    const std::vector<wxString>* GetFontFiles() const;

    /**
     * Removes all embedded fonts from the collection
    */
    void ClearEmbeddedFonts();

    /**
     * Takes data from the #decompressedData buffer and compresses it using ZSTD
     * into the #compressedEncodedData buffer. The data is then Base64 encoded.
     *
     * This call is used when adding a new file to the collection from disk
    */
    static RETURN_CODE  CompressAndEncode( EMBEDDED_FILE& aFile );

    /**
     * Takes data from the #compressedEncodedData buffer and Base64 decodes it.
     * The data is then decompressed using ZSTD and stored in the #decompressedData buffer.
     *
     * This call is used when loading the embedded files using the parsers.
    */
    static RETURN_CODE  DecompressAndDecode( EMBEDDED_FILE& aFile );

    /**
     * Returns the embedded file with the given name or nullptr if it does not exist.
    */
    EMBEDDED_FILE* GetEmbeddedFile( const wxString& aName ) const
    {
        auto it = m_files.find( aName );

        return it == m_files.end() ? nullptr : it->second;
    }

    const std::map<wxString, EMBEDDED_FILE*>& EmbeddedFileMap() const
    {
        return m_files;
    }

    wxFileName GetTemporaryFileName( const wxString& aName ) const;

    wxFileName GetTemporaryFileName( EMBEDDED_FILE* aFile ) const;

    void ClearEmbeddedFiles( bool aDeleteFiles = true )
    {
        for( auto& file : m_files )
        {
            if( aDeleteFiles )
                delete file.second;
        }

        m_files.clear();
    }

    virtual void EmbedFonts() {};

    void SetAreFontsEmbedded( bool aEmbedFonts )
    {
        m_embedFonts = aEmbedFonts;
    }

    bool GetAreFontsEmbedded() const
    {
        return m_embedFonts;
    }

private:
    std::map<wxString, EMBEDDED_FILE*> m_files;
    std::vector<wxString>              m_fontFiles;

protected:
    bool m_embedFonts = false; // If set, fonts will be embedded in the element on save
                               // Otherwise, font files embedded in the element will be
                               // removed on save
};



class EMBEDDED_FILES_PARSER : public EMBEDDED_FILES_LEXER
{
public:
    EMBEDDED_FILES_PARSER( LINE_READER* aReader ) :
            EMBEDDED_FILES_LEXER( aReader )
    {
    }

    void ParseEmbedded( EMBEDDED_FILES* aFiles );

};
#endif // EMBEDDED_FILES_H