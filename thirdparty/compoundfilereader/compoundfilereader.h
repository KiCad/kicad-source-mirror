/**
    Microsoft Compound File (and Property Set) Reader
    http://en.wikipedia.org/wiki/Compound_File_Binary_Format

    Format specification:
        MS-CFB: https://msdn.microsoft.com/en-us/library/dd942138.aspx
        MS-OLEPS: https://msdn.microsoft.com/en-us/library/dd942421.aspx

    Note:
    1. For simplification, the code assumes that the target system is little-endian.

    2. The reader operates the passed buffer in-place.
       You must keep the input buffer valid when you are using the reader.

    3. Single thread usage.

    Example 1: print all streams in a compound file
    \code
        CFB::CompoundFileReader reader(buffer, len);
        reader.EnumFiles(reader.GetRootEntry(), -1,
            [&](const CFB::COMPOUND_FILE_ENTRY* entry, const CFB::utf16string& dir, int level)->void
        {
            bool isDirectory = !reader.IsStream(entry);
            std::string name = UTF16ToUTF8(entry->name);
            std::string indentstr(level * 4, ' ');
            printf("%s%s%s%s\n", indentstr.c_str(), isDirectory ? "[" : "", name.c_str(), isDirectory ? "]" : "");
        });
    \endcode
*/

#pragma once

#include <algorithm>
#include <stdint.h>
#include <string.h>
#include <exception>
#include <stdexcept>
#include <functional>

/***********************************************************************************************************************
 * If we are compiling on Apple with Clang >= 17, the version of LLVM no longer includes a generic template for
 * char_traits for char types which are not specified in the C++ standard. We define our own here for types required by
 * the JSON library.
 *
 * From: https://github.com/llvm/llvm-project/commit/c3668779c13596e223c26fbd49670d18cd638c40
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 **********************************************************************************************************************/

#ifdef __APPLE__
#if __clang_major__ >= 17

template <>
struct std::char_traits<uint16_t>
{
    using char_type = uint16_t;
    using int_type = int;
    using off_type = std::streamoff;
    using pos_type = std::streampos;
    using state_type = mbstate_t;

    static inline void assign( char_type& __c1, const char_type& __c2 ) noexcept { __c1 = __c2; }

    static inline bool eq( char_type __c1, char_type __c2 ) noexcept { return __c1 == __c2; }

    static inline bool lt( char_type __c1, char_type __c2 ) noexcept { return __c1 < __c2; }

    static constexpr int compare( const char_type* __s1, const char_type* __s2, size_t __n )
    {
        for( ; __n; --__n, ++__s1, ++__s2 )
        {
            if( lt( *__s1, *__s2 ) )
                return -1;

            if( lt( *__s2, *__s1 ) )
                return 1;
        }

        return 0;
    }

    static size_t length( const char_type* __s )
    {
        size_t __len = 0;

        for( ; !eq( *__s, char_type( 0 ) ); ++__s )
            ++__len;

        return __len;
    }

    static constexpr const char_type* find( const char_type* __s, size_t __n, const char_type& __a )
    {
        for( ; __n; --__n )
        {
            if( eq( *__s, __a ) )
                return __s;
            ++__s;
        }

        return nullptr;
    }

    static constexpr char_type* move( char_type* __s1, const char_type* __s2, size_t __n )
    {
        if( __n == 0 )
            return __s1;

        char_type* __r = __s1;

        if( __s1 < __s2 )
        {
            for( ; __n; --__n, ++__s1, ++__s2 )
                assign( *__s1, *__s2 );
        }
        else if( __s2 < __s1 )
        {
            __s1 += __n;
            __s2 += __n;

            for( ; __n; --__n )
                assign( *--__s1, *--__s2 );
        }

        return __r;
    }

    static constexpr char_type* copy( char_type* __s1, const char_type* __s2, size_t __n )
    {
        char_type* __r = __s1;

        for( ; __n; --__n, ++__s1, ++__s2 )
            assign( *__s1, *__s2 );

        return __r;
    }

    static char_type* assign( char_type* __s, size_t __n, char_type __a )
    {
        char_type* __r = __s;

        for( ; __n; --__n, ++__s )
            assign( *__s, __a );

        return __r;
    }

    static inline constexpr int_type not_eof( int_type __c ) noexcept
    {
        return eq_int_type( __c, eof() ) ? ~eof() : __c;
    }

    static inline char_type to_char_type( int_type __c ) noexcept { return char_type( __c ); }

    static inline int_type to_int_type( char_type __c ) noexcept { return static_cast<int_type>( __c ); }

    static inline constexpr bool eq_int_type( int_type __c1, int_type __c2 ) noexcept { return __c1 == __c2; }

    static inline constexpr int_type eof() noexcept { return static_cast<int_type>( EOF ); }
};

#endif
#endif

/**********************************************************************************************************************/

namespace CFB
{
    struct CFBException : public std::runtime_error
    {
        CFBException(const char* desc) : std::runtime_error(desc) {}
    };
    struct WrongFormat : public CFBException
    {
        WrongFormat() : CFBException("Wrong file format") {}
    };
    struct FileCorrupted : public CFBException
    {
        FileCorrupted() : CFBException("File corrupted") {}
    };

#pragma pack(push)
#pragma pack(1)

struct COMPOUND_FILE_HDR
{
    unsigned char signature[8];
    unsigned char unused_clsid[16];
    uint16_t minorVersion;
    uint16_t majorVersion;
    uint16_t byteOrder;
    uint16_t sectorShift;
    uint16_t miniSectorShift;
    unsigned char reserved[6];
    uint32_t numDirectorySector;
    uint32_t numFATSector;
    uint32_t firstDirectorySectorLocation;
    uint32_t transactionSignatureNumber;
    uint32_t miniStreamCutoffSize;
    uint32_t firstMiniFATSectorLocation;
    uint32_t numMiniFATSector;
    uint32_t firstDIFATSectorLocation;
    uint32_t numDIFATSector;
    uint32_t headerDIFAT[109];
};

struct COMPOUND_FILE_ENTRY
{
    uint16_t name[32];
    uint16_t nameLen;
    uint8_t type;
    uint8_t colorFlag;
    uint32_t leftSiblingID;         // Note that it's actually the left/right child in the RB-tree.
    uint32_t rightSiblingID;        // So entry.leftSibling.rightSibling does NOT go back to entry.
    uint32_t childID;
    unsigned char clsid[16];
    uint32_t stateBits;
    uint64_t creationTime;
    uint64_t modifiedTime;
    uint32_t startSectorLocation;
    uint64_t size;
};

struct PROPERTY_SET_STREAM_HDR
{
    unsigned char byteOrder[2];
    uint16_t version;
    uint32_t systemIdentifier;
    unsigned char clsid[16];
    uint32_t numPropertySets;
    struct
    {
        char fmtid[16];
        uint32_t offset;
    } propertySetInfo[1];
};

struct PROPERTY_SET_HDR
{
    uint32_t size;
    uint32_t NumProperties;
    struct
    {
        uint32_t id;
        uint32_t offset;
    } propertyIdentifierAndOffset[1];
};

#pragma pack(pop)

const size_t MAXREGSECT = 0xFFFFFFFA;

struct helper
{
    static uint32_t ParseUint32(const void* buffer)
    {
        return *static_cast<const uint32_t*>(buffer);
    }
};

typedef std::basic_string<uint16_t> utf16string;
typedef std::function<int(const COMPOUND_FILE_ENTRY*, const utf16string& dir, int level)>
    EnumFilesCallback;

class CompoundFileReader
{
public:
    CompoundFileReader(const void* buffer, size_t len)
        : m_buffer(static_cast<const unsigned char*>(buffer))
        , m_bufferLen(len)
        , m_hdr(static_cast<const COMPOUND_FILE_HDR*>(buffer))
        , m_sectorSize(512)
        , m_minisectorSize(64)
        , m_miniStreamStartSector(0)
    {
        if (buffer == NULL || len == 0) throw std::invalid_argument("");

        if (m_bufferLen < sizeof(*m_hdr) ||
            memcmp(m_hdr->signature, "\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1", 8) != 0)
        {
            throw WrongFormat();
        }

        m_sectorSize = m_hdr->majorVersion == 3 ? 512 : 4096;

        // The file must contains at least 3 sectors
        if (m_bufferLen < m_sectorSize * 3) throw FileCorrupted();

        const COMPOUND_FILE_ENTRY* root = GetEntry(0);
        if (root == NULL) throw FileCorrupted();

        m_miniStreamStartSector = root->startSectorLocation;
    }

    /// Get entry (directory or file) by its ID.
    /// Pass "0" to get the root directory entry. -- This is the start point to navigate the compound file.
    /// Use the returned object to access child entries.
    const COMPOUND_FILE_ENTRY* GetEntry(size_t entryID) const
    {
        if (entryID == 0xFFFFFFFF)
        {
            return NULL;
        }

        if (m_bufferLen / sizeof(COMPOUND_FILE_ENTRY) <= entryID)
        {
            throw std::invalid_argument("");
        }

        size_t sector = 0;
        size_t offset = 0;
        LocateFinalSector(m_hdr->firstDirectorySectorLocation, entryID * sizeof(COMPOUND_FILE_ENTRY), &sector, &offset);
        return reinterpret_cast<const COMPOUND_FILE_ENTRY*>(SectorOffsetToAddress(sector, offset));
    }

    const COMPOUND_FILE_ENTRY* GetRootEntry() const
    {
        return GetEntry(0);
    }

    const COMPOUND_FILE_HDR* GetFileInfo() const
    {
        return m_hdr;
    }

    /// Get file(stream) data start with "offset".
    /// The buffer must have enough space to store "len" bytes. Typically "len" is derived by the steam length.
    void ReadFile(const COMPOUND_FILE_ENTRY* entry, size_t offset, char* buffer, size_t len) const
    {
        if (entry->size < offset || entry->size - offset < len) throw std::invalid_argument("");

        if (entry->size < m_hdr->miniStreamCutoffSize)
        {
            ReadMiniStream(entry->startSectorLocation, offset, buffer, len);
        }
        else
        {
            ReadStream(entry->startSectorLocation, offset, buffer, len);
        }
    }

    bool IsPropertyStream(const COMPOUND_FILE_ENTRY* entry) const
    {
        // defined in [MS-OLEPS] 2.23 "Property Set Stream and Storage Names"
        return entry->name[0] == 5;
    }

    bool IsStream(const COMPOUND_FILE_ENTRY* entry) const
    {
        return entry->type == 2;
    }

    void EnumFiles(const COMPOUND_FILE_ENTRY* entry, int maxLevel, EnumFilesCallback callback) const
    {
        utf16string dir;
        EnumNodes(GetEntry(entry->childID), 0, maxLevel, dir, callback);
    }

private:

    // Enum entries with same level, including 'entry' itself
    void EnumNodes(const COMPOUND_FILE_ENTRY* entry, int currentLevel, int maxLevel,
        const utf16string& dir, EnumFilesCallback callback) const
    {
        if (maxLevel > 0 && currentLevel >= maxLevel)
            return;
        if (entry == nullptr)
            return;

        if( callback(entry, dir, currentLevel + 1) != 0 )
            return;

        const COMPOUND_FILE_ENTRY* child = GetEntry(entry->childID);
        if (child != nullptr)
        {
            utf16string newDir = dir;
            if (dir.length() != 0)
                newDir.append(1, '\n');
            newDir.append(entry->name, entry->nameLen / 2);
            EnumNodes(GetEntry(entry->childID), currentLevel + 1, maxLevel, newDir, callback);
        }

        EnumNodes(GetEntry(entry->leftSiblingID), currentLevel, maxLevel, dir, callback);
        EnumNodes(GetEntry(entry->rightSiblingID), currentLevel, maxLevel, dir, callback);
    }

    void ReadStream(size_t sector, size_t offset, char* buffer, size_t len) const
    {
        LocateFinalSector(sector, offset, &sector, &offset);

        // copy as many as possible in each step
        // copylen typically iterate as: m_sectorSize - offset   -->   m_sectorSize   -->   m_sectorSize  --> ... -->    remaining
        while (len > 0)
        {
            const unsigned char* src = SectorOffsetToAddress(sector, offset);
            size_t copylen = std::min(len, m_sectorSize - offset);
            if (m_buffer + m_bufferLen < src + copylen) throw FileCorrupted();

            memcpy(buffer, src, copylen);
            buffer += copylen;
            len -= copylen;
            sector = GetNextSector(sector);
            offset = 0;
        }
    }

    // Same logic as "ReadStream" except that use MiniStream functions instead
    void ReadMiniStream(size_t sector, size_t offset, char* buffer, size_t len) const
    {
        LocateFinalMiniSector(sector, offset, &sector, &offset);

        // copy as many as possible in each step
        // copylen typically iterate as: m_sectorSize - offset   -->   m_sectorSize   -->   m_sectorSize  --> ... -->    remaining
        while (len > 0)
        {
            const unsigned char* src = MiniSectorOffsetToAddress(sector, offset);
            size_t copylen = std::min(len, m_minisectorSize - offset);
            if (m_buffer + m_bufferLen < src + copylen) throw FileCorrupted();

            memcpy(buffer, src, copylen);
            buffer += copylen;
            len -= copylen;
            sector = GetNextMiniSector(sector);
            offset = 0;
        }
    }

    size_t GetNextSector(size_t sector) const
    {
        // lookup FAT
        size_t entriesPerSector = m_sectorSize / 4;
        size_t fatSectorNumber = sector / entriesPerSector;
        size_t fatSectorLocation = GetFATSectorLocation(fatSectorNumber);
        return helper::ParseUint32(SectorOffsetToAddress(fatSectorLocation, sector % entriesPerSector * 4));
    }

    size_t GetNextMiniSector(size_t miniSector) const
    {
        size_t sector, offset;
        LocateFinalSector(m_hdr->firstMiniFATSectorLocation, miniSector * 4, &sector, &offset);
        return helper::ParseUint32(SectorOffsetToAddress(sector, offset));
    }

    // Get absolute address from sector and offset.
    const unsigned char* SectorOffsetToAddress(size_t sector, size_t offset) const
    {
        if (sector >= MAXREGSECT ||
            offset >= m_sectorSize ||
            m_bufferLen <= static_cast<uint64_t>(m_sectorSize) * sector + m_sectorSize + offset)
        {
            throw FileCorrupted();
        }

        return m_buffer + m_sectorSize + m_sectorSize * sector + offset;
    }

    const unsigned char* MiniSectorOffsetToAddress(size_t sector, size_t offset) const
    {
        if (sector >= MAXREGSECT ||
            offset >= m_minisectorSize ||
            m_bufferLen <= static_cast<uint64_t>(m_minisectorSize) * sector + offset)
        {
            throw FileCorrupted();
        }


        LocateFinalSector(m_miniStreamStartSector, sector * m_minisectorSize + offset, &sector, &offset);
        return SectorOffsetToAddress(sector, offset);
    }

    // Locate the final sector/offset when original offset expands multiple sectors
    void LocateFinalSector(size_t sector, size_t offset, size_t* finalSector, size_t* finalOffset) const
    {
        while (offset >= m_sectorSize)
        {
            offset -= m_sectorSize;
            sector = GetNextSector(sector);
        }
        *finalSector = sector;
        *finalOffset = offset;
    }

    void LocateFinalMiniSector(size_t sector, size_t offset, size_t* finalSector, size_t* finalOffset) const
    {
        while (offset >= m_minisectorSize)
        {
            offset -= m_minisectorSize;
            sector = GetNextMiniSector(sector);
        }
        *finalSector = sector;
        *finalOffset = offset;
    }

    size_t GetFATSectorLocation(size_t fatSectorNumber) const
    {
        if (fatSectorNumber < 109)
        {
            return m_hdr->headerDIFAT[fatSectorNumber];
        }
        else
        {
            fatSectorNumber -= 109;
            size_t entriesPerSector = m_sectorSize / 4 - 1;
            size_t difatSectorLocation = m_hdr->firstDIFATSectorLocation;
            while (fatSectorNumber >= entriesPerSector)
            {
                fatSectorNumber -= entriesPerSector;
                const unsigned char* addr = SectorOffsetToAddress(difatSectorLocation, m_sectorSize - 4);
                difatSectorLocation = helper::ParseUint32(addr);
            }
            return helper::ParseUint32(SectorOffsetToAddress(difatSectorLocation, fatSectorNumber * 4));
        }
    }

private:
    const unsigned char * m_buffer;
    size_t m_bufferLen;

    const COMPOUND_FILE_HDR* m_hdr;
    size_t m_sectorSize;
    size_t m_minisectorSize;
    size_t m_miniStreamStartSector;
};

class PropertySet
{
public:
    PropertySet(const void* buffer, size_t len, const char* fmtid)
        : m_buffer(static_cast<const unsigned char*>(buffer))
        , m_bufferLen(len)
        , m_hdr(reinterpret_cast<const PROPERTY_SET_HDR*>(buffer))
        , m_fmtid(fmtid)
    {
        if (m_bufferLen < sizeof(*m_hdr) ||
            m_bufferLen < sizeof(*m_hdr) + (m_hdr->NumProperties - 1) * sizeof(m_hdr->propertyIdentifierAndOffset[0]))
        {
            throw FileCorrupted();
        }
   }

    /// return the string property in UTF-16 format
    const uint16_t* GetStringProperty(uint32_t propertyID)
    {
        for (uint32_t i = 0; i < m_hdr->NumProperties; i++)
        {
            if (m_hdr->propertyIdentifierAndOffset[i].id == propertyID)
            {
                uint32_t offset = m_hdr->propertyIdentifierAndOffset[i].offset;
                if (m_bufferLen < offset + 8) throw FileCorrupted();
                uint32_t stringLengthInChar = helper::ParseUint32(m_buffer + offset + 4);
                if (m_bufferLen < offset + 8 + stringLengthInChar*2) throw FileCorrupted();
                return reinterpret_cast<const uint16_t*>(m_buffer + offset + 8);
            }
        }

        return NULL;
    }

    /// Note: Getting property of types other than "string" is not implemented yet.
    ///       However most other types are simpler than string so can be easily added. see [MS-OLEPS]

    const char* GetFmtID()
    {
        return m_fmtid;
    }

private:
    const unsigned char* m_buffer;
    size_t m_bufferLen;
    const PROPERTY_SET_HDR* m_hdr;
    const char* m_fmtid; // 16 bytes
};

class PropertySetStream
{
public:
    PropertySetStream(const void* buffer, size_t len)
        : m_buffer(static_cast<const unsigned char*>(buffer))
        , m_bufferLen(len)
        , m_hdr(reinterpret_cast<const PROPERTY_SET_STREAM_HDR*>(buffer))
    {
        if (m_bufferLen < sizeof(*m_hdr) ||
            m_bufferLen < sizeof(*m_hdr) + (m_hdr->numPropertySets - 1) * sizeof(m_hdr->propertySetInfo[0]))
        {
            throw FileCorrupted();
        }
    }

    size_t GetPropertySetCount()
    {
       return m_hdr->numPropertySets;
    }

    PropertySet GetPropertySet(size_t index)
    {
        if (index >= GetPropertySetCount()) throw FileCorrupted();
        uint32_t offset = m_hdr->propertySetInfo[index].offset;
        if (m_bufferLen < offset + 4) throw FileCorrupted();
        uint32_t size = helper::ParseUint32(m_buffer + offset);
        if (m_bufferLen < offset + size) throw FileCorrupted();
        return PropertySet(m_buffer + offset, size, m_hdr->propertySetInfo[index].fmtid);
    }

private:
    const unsigned char * m_buffer;
    size_t m_bufferLen;
    const PROPERTY_SET_STREAM_HDR* m_hdr;
};

}