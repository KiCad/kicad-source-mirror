/* Helper class to read/write footprints libraries
*/

#ifndef _FOOTPRINT_LIBRARY_H_
#define _FOOTPRINT_LIBRARY_H_

#include <filter_reader.h>

class FOOTPRINT_LIBRARY
{
public:
    wxArrayString m_List;       // list of footprints, used to read/write INDEX section
    wxString m_LibraryName;     // the full library name
    int m_LineNum;              // the line count

private:
    FILTER_READER * m_reader;   // FILTER_READER to read file. If NULL, use m_file
    FILE * m_file;              // footprint file to read/write.

public:
    /**
     * ctor
     * @param aFile = a FILE * pointer used for write operations,
     * and read operations when aReader = NULL
     * @param aReader = a FILTER_READER pointer used for read operations
     * If NULL, a direct aFile read is used
     */
    FOOTPRINT_LIBRARY( FILE * aFile, FILTER_READER * aReader = NULL );

    ~FOOTPRINT_LIBRARY() { }

    /**
     * function IsLibrary
     * Read the library file Header
     * return > 0 if this file is a footprint lib
     * (currentlu return 1 but could be a value > 1 for future file formats
     */
    int IsLibrary( );

    /**
     * function RebuildIndex
     * Read the full library file and build the list of footprints found
     * Do not use the $INDEX ... $EndINDEX section
     */
    bool RebuildIndex();

    /**
     * function ReadSectionIndex
     * Read the $INDEX ... $EndINDEX section
     * list of footprints is stored in m_List
     */
    bool ReadSectionIndex();

    /**
     * Function WriteHeader
     * Write the library header
     */
    bool WriteHeader();

    /**
     * Function WriteSectionIndex
     * Write the $INDEX ... $EndINDEX section.
     * This section is filled by names in m_List
     */
    bool WriteSectionIndex();

    /**
     * Function WriteEndOfFile
     * Write the last line section.
     */
    bool WriteEndOfFile();

    /**
     * Function FindInList
     * Search for aName int m_List
     * @return true if found
     */
    bool FindInList( const wxString & aName );

    /**
     * Function RemoveFromList
     * Search for aName int m_List and remove it
     * @return true if found and removed
     */
    bool RemoveFromList( const wxString & aName );

    /**
     * Function SortList
     * Sort m_List in alphabetic order
     */
    void SortList();
};

#endif
