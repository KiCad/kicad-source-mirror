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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIPTRACE_BINARY_READER_H
#define DIPTRACE_BINARY_READER_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include <wx/string.h>


namespace DIPTRACE
{

enum class STRING_ENCODING
{
    BY_VERSION,
    LEGACY_ASCII,
    UTF16_BE
};

/// Bias value added to stored 3-byte unsigned integers.
/// Stored 0x0F4240 decodes to logical 0.
constexpr int INT3_BIAS = 1000000;

/// Bias value added to stored 4-byte unsigned integers.
/// Stored 0x3B9ACA00 decodes to logical 0.
constexpr int INT4_BIAS = 1000000000;

/// DipTrace uses 762 units per mil (30 000 units per mm).
/// 1 DipTrace unit = 100/3 nm ≈ 33.33 nm.
/// The fractional scale requires int64 arithmetic in the conversion functions.
constexpr double DIPTRACE_COORD_TO_MM = 1.0 / 30000.0;

/// DipTrace stores angles with 100 000 units per degree.
constexpr double DIPTRACE_ANGLE_TO_DEG = 0.00001;

/// Format version at or below which strings use the legacy ASCII encoding
/// (int3 byte-count + raw ASCII bytes).  Versions above this threshold use
/// the modern encoding (uint16-BE char-count + UTF-16-BE data).
///
/// Individual format parsers (PCB, schematic) may define additional
/// version thresholds for structural layout differences that are independent
/// of string encoding.  For example, the schematic parser uses a separate
/// cutover version for field layout changes specific to the .dch format.
constexpr int LEGACY_STRING_VERSION = 37;

/// Maximum sane string length (in characters) accepted by the reader.
/// Anything larger is treated as a corrupt read.
constexpr int MAX_STRING_CHARS = 10000;


/**
 * Low-level binary reader for DipTrace file formats.
 *
 * DipTrace files (.dip for PCB, .dch for schematic) use a proprietary
 * big-endian binary encoding with biased integers.  This reader loads
 * the entire file into memory and provides methods to decode each
 * primitive type.
 *
 * Shared by both the PCB and schematic importers.
 */
class BINARY_READER
{
public:
    /**
     * Construct a reader by loading the given file into memory.
     *
     * @param aFileName path to the DipTrace binary file.
     * @throw IO_ERROR if the file cannot be opened or read.
     */
    BINARY_READER( const wxString& aFileName );

    ~BINARY_READER();

    // -- Position management -------------------------------------------------

    /** Return the current byte offset within the file. */
    size_t GetOffset() const { return m_offset; }

    /**
     * Set the read position to an absolute byte offset.
     *
     * @param aOffset byte offset from the start of the file.
     * @throw IO_ERROR if \a aOffset is beyond the end of the file.
     */
    void SetOffset( size_t aOffset );

    /**
     * Advance the read position by the given number of bytes.
     *
     * @param aBytes number of bytes to skip.
     * @throw IO_ERROR if skipping would read past the end of the file.
     */
    void Skip( size_t aBytes );

    /** Return the total size of the loaded file in bytes. */
    size_t GetFileSize() const { return m_data.size(); }

    /** Return true if the read position has reached or passed the end of file. */
    bool IsEOF() const { return m_offset >= m_data.size(); }

    /** Return a pointer to the raw file data buffer. */
    const uint8_t* GetData() const { return m_data.data(); }

    // -- Version management --------------------------------------------------

    /**
     * Set the DipTrace format version.
     *
     * The version affects default string decoding: versions <= 37 use the
     * legacy int3+ASCII encoding while versions >= 39 use uint16+UTF-16-BE.
     * File-family parsers can override this with SetStringEncoding().
     *
     * @param aVersion DipTrace format version number.
     */
    void SetVersion( int aVersion ) { m_version = aVersion; }

    /** Return the currently configured format version. */
    int GetVersion() const { return m_version; }

    /**
     * Override DipTrace string decoding when a file family has a deterministic
     * string-encoding cutover that differs from the shared version threshold.
     */
    void SetStringEncoding( STRING_ENCODING aEncoding ) { m_stringEncoding = aEncoding; }

    /**
     * Detect the string encoding from the bytes at @p aProbeOffset, which must sit at the start
     * of a non-empty DipTrace string, and pin the encoding override accordingly.
     *
     * DipTrace ships files of the same format version in both encodings (e.g. v37 schematics exist
     * as legacy ASCII and as UTF-16-BE), so the version threshold alone is not reliable. The two
     * framings are mutually exclusive on real string bytes -- the ASCII length is the int3 bias
     * 0x0F42xx while a small UTF-16 char count starts 0x00xx -- so exactly one parses as a printable
     * string. The encoding is left unchanged when the probe is inconclusive.
     *
     * @param aProbeOffset file offset of a known non-empty string field.
     */
    void DetectStringEncoding( size_t aProbeOffset );

    // -- Primitive readers ---------------------------------------------------

    /**
     * Read a single unsigned byte and advance the position by 1.
     *
     * @throw IO_ERROR on read past end of file.
     */
    uint8_t ReadByte();

    /**
     * Read a 3-byte big-endian biased integer (bias 1,000,000) and advance
     * the position by 3.
     *
     * Decoding: value = (byte[0]<<16 | byte[1]<<8 | byte[2]) - 1,000,000
     *
     * @return the decoded signed integer value.
     * @throw IO_ERROR on read past end of file.
     */
    int ReadInt3();

    /**
     * Read a 4-byte big-endian biased integer (bias 1,000,000,000) and
     * advance the position by 4.
     *
     * Decoding: value = (byte[0]<<24 | byte[1]<<16 | byte[2]<<8 | byte[3])
     *                    - 1,000,000,000
     *
     * @return the decoded signed integer value.
     * @throw IO_ERROR on read past end of file.
     */
    int ReadInt4();

    /**
     * Read a string using the configured encoding.
     *
     * - legacy ASCII: int3(byte_count) + ASCII bytes
     * - UTF-16-BE:    uint16-BE(char_count) + UTF-16-BE data
     *
     * @return the decoded wxString.
     * @throw IO_ERROR on read past end of file or corrupt string data.
     */
    wxString ReadString();

    /**
     * Read a 3-byte RGB color value.
     *
     * @param[out] r red component (0-255).
     * @param[out] g green component (0-255).
     * @param[out] b blue component (0-255).
     * @throw IO_ERROR on read past end of file.
     */
    void ReadColor( uint8_t& r, uint8_t& g, uint8_t& b );

    /**
     * Read a block of raw bytes into the caller's buffer.
     *
     * @param aDst destination buffer (must hold at least \a aCount bytes).
     * @param aCount number of bytes to read.
     * @throw IO_ERROR on read past end of file.
     */
    void ReadBytes( uint8_t* aDst, size_t aCount );

    // -- Peek methods (do not advance position) ------------------------------

    /**
     * Peek at the next 3-byte biased integer without advancing the position.
     *
     * @throw IO_ERROR if there are fewer than 3 bytes remaining.
     */
    int PeekInt3() const;

    /**
     * Peek at the next 4-byte biased integer without advancing the position.
     *
     * @throw IO_ERROR if there are fewer than 4 bytes remaining.
     */
    int PeekInt4() const;

    /**
     * Peek at the next byte without advancing the position.
     *
     * @throw IO_ERROR if at end of file.
     */
    uint8_t PeekByte() const;

    // -- Coordinate conversion helpers ---------------------------------------

    /**
     * Convert a DipTrace coordinate value (10 nm units) to KiCad nanometers.
     *
     * @param aDipTraceCoord coordinate in DipTrace units.
     * @return coordinate in nanometers.
     */
    static int DipTraceToKiCadNm( int aDipTraceCoord );

    /**
     * Convert a DipTrace coordinate value (10 nm units) to millimeters.
     *
     * @param aDipTraceCoord coordinate in DipTrace units.
     * @return coordinate in millimeters.
     */
    static double DipTraceToMM( int aDipTraceCoord );

    // -- Search helpers ------------------------------------------------------

    /**
     * Search for a byte pattern in the file data.
     *
     * @param aPattern pointer to the pattern bytes.
     * @param aPatternLen length of the pattern in bytes.
     * @param aStart byte offset to begin searching (inclusive).
     * @param aEnd byte offset to stop searching (exclusive).  Use 0 to
     *             search to end of file.
     * @return byte offset of the first match, or @c std::string::npos if not
     *         found.
     */
    size_t FindPattern( const uint8_t* aPattern, size_t aPatternLen,
                        size_t aStart, size_t aEnd ) const;

    /**
     * Search for a UTF-16-BE encoded string in the file data, including its
     * two-byte length prefix.
     *
     * @param aStr the string to search for.
     * @param aStart byte offset to begin searching.
     * @param aEnd byte offset to stop searching.  Use 0 to search to end of
     *             file.
     * @return byte offset of the length prefix of the first match, or
     *         @c std::string::npos if not found.
     */
    size_t FindString( const wxString& aStr, size_t aStart, size_t aEnd ) const;

    // -- Try-read methods (return false instead of throwing) ------------------

    /**
     * Attempt to read a string at the current position.
     *
     * On success, the position is advanced past the string and @a aResult is
     * populated.  On failure the position is restored and @c false is returned.
     *
     * Uses the version-appropriate encoding, with additional sanity checks:
     * character count must be <= 500, and all decoded characters must be
     * printable or whitespace.
     *
     * @param[out] aResult receives the decoded string on success.
     * @return true if a valid string was read.
     */
    bool TryReadString( wxString& aResult );

private:
    /**
     * Read a v39+ UTF-16-BE string: uint16-BE char count + UTF-16-BE data.
     */
    wxString ReadStringUTF16();

    /**
     * Read a v37 legacy ASCII string: int3(byte_count) + raw ASCII bytes.
     */
    wxString ReadStringASCII();

    /**
     * Attempt to read a UTF-16-BE string with validation.  Returns false and
     * restores the position on failure.
     */
    bool TryReadStringUTF16( wxString& aResult );

    /**
     * Attempt to read a legacy ASCII string with validation.  Returns false
     * and restores the position on failure.
     */
    bool TryReadStringASCII( wxString& aResult );

    /**
     * Verify that all characters in \a aStr are printable or common
     * whitespace (space, tab, CR, LF).
     */
    static bool IsPrintableString( const wxString& aStr );

    /**
     * Throw IO_ERROR with a message indicating a read past end of file.
     *
     * @param aBytesNeeded number of bytes the caller tried to read.
     */
    void ThrowEOFError( size_t aBytesNeeded ) const;

    std::vector<uint8_t> m_data;      ///< Entire file contents loaded into memory.
    size_t               m_offset;    ///< Current read position (byte offset).
    int                  m_version;   ///< DipTrace format version.
    STRING_ENCODING      m_stringEncoding; ///< Explicit string encoding override.
};

} // namespace DIPTRACE

#endif // DIPTRACE_BINARY_READER_H
