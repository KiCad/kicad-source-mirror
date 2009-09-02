/***********************************/
/*	Headers for library definition */
/***********************************/

#ifndef CLASS_LIBRARY_H
#define CLASS_LIBRARY_H


#include "class_libentry.h"


/**
 * Component library object.
 *
 * Component libraries are used to load, save, search, and otherwise manipulate
 * component library files.
 */

class LibraryStruct
{
public:
    int            m_Type;          /* type indicator */
    wxString       m_Name;          /* Library file name (without path). */
    wxString       m_FullFileName;  /* Full File Name (with path) of library. */
    wxString       m_Header;        /* first line of loaded library. */
    LibraryStruct* m_Pnext;         /* Point on next lib in chain. */
    unsigned long  m_TimeStamp;     // Signature temporelle
    int            m_Flags;         // variable used in some functions
    bool           m_IsLibCache;    /* False for the "standard" libraries,
                                     * True for the library cache */

public:
    LibraryStruct( int type, const wxString& name, const wxString& fullname );
    ~LibraryStruct();

    /**
     * Save library to file.
     *
     * Two files are created.  The component objects are save as component
     * library (*.lib) files.  The alias objects are save as document
     * definition (*.dcm) files.  If the component library already exists,
     * it is backup up in file *.bak.  If the document definition file
     * already exists, it is backed up in file *.bck.
     *
     * @param aFullFileName - The library filename with path.
     *
     * @return bool - true if success writing else false.
     */
    bool Save( const wxString& aFullFileName );

    /**
     * Load library from file.
     *
     * @param errMsg - Error message if load fails.
     *
     * @return bool - True if load was successful otherwise false.
     */
    bool Load( wxString& errMsg );

    bool LoadDocs( wxString& errMsg );

private:
    bool SaveHeader( FILE* file );

    bool LoadHeader( FILE* file, int* LineNum );
    void LoadAliases( EDA_LibComponentStruct* component );

    void RemoveEntry( const wxString& name );

public:
    LibraryStruct( const wxChar* fileName = NULL );

    /**
     * Get library entry status.
     *
     * @return true if there are no entries in the library.
     */
    bool IsEmpty()
    {
        return m_Entries.empty();
    }

    /**
     * Get the number of entries in the library.
     *
     * @return The number of component and alias entries.
     */
    int GetCount()
    {
        return m_Entries.size();
    }

    bool IsModified()
    {
        return m_IsModified;
    }

    /**
     * Load a string array with the names of all the entries in this library.
     *
     * @param names - String array to place entry names into.
     * @param sort - Sort names if true.
     * @param makeUpperCase - Force entry names to upper case.
     */
    void GetEntryNames( wxArrayString& names, bool sort = true,
                        bool makeUpperCase = true );

    /**
     * Load string array with entry names matching name and/or key word.
     *
     * This currently mimics the old behavior of calling KeyWordOk() and
     * WildCompareString().  The names array will be populated with the
     * library entry names that meat the search criteria on exit.
     *
     * @todo Convert the search functions to use regular expressions which
     *       should give better search capability.
     *
     * @param names - String array to place entry names into.
     * @param nameSearch - Name wild card search criteria.
     * @param keySearch - Key word search criteria.
     * @param sort - Sort names if true.
     */
    void SearchEntryNames( wxArrayString& names,
                           const wxString& nameSearch = wxEmptyString,
                           const wxString& keySearch = wxEmptyString,
                           bool sort = true );

    /**
     * Find entry by name.
     *
     * @param name - Name of entry, case insensitive.
     *
     * @return Pointer to entry if found.  NULL if not found.
     */
    LibCmpEntry* FindEntry( const wxChar* name );

    /**
     * Find entry by name and type.
     *
     * If the search type is an alias, the return entry can be either an
     * alias or a component object.  If the search type is a component
     * (root) type, the object returned will be a component.  This was
     * done to emulate the old search pattern.
     *
     * @param name - Name of entry, case insensitive.
     * @param type - Type of entry, root or alias.
     *
     * @return Pointer to entry if found.  NULL if not found.
     */
    LibCmpEntry* FindEntry( const wxChar* name, LibrEntryType type );

    /**
     * Add component entry to library.
     *
     * @param cmp - Component to add.
     *
     * @return Pointer to added component if successful.
     */
    EDA_LibComponentStruct* AddComponent( EDA_LibComponentStruct* cmp );

    /**
     * Remove an entry from the library.
     *
     * If the entry is an alias, the alias is removed from the library and from
     * the alias list of the root component.  If the entry is a root component
     * with no aliases, it is removed from the library.  If the entry is a root
     * component with aliases, the root component is renamed to the name of
     * the first alias and the root name for all remaining aliases are updated
     * to reflect the new root name.
     *
     * @param entry - Entry to remove from library.
     */
    void RemoveEntry( LibCmpEntry* entry );

    /**
     * Return the first entry in the library.
     *
     * @return The first entry or NULL if the library has no entries.
     */
    LibCmpEntry* GetFirstEntry()
    {
        return &m_Entries.front();
    }

    /**
     * Find next library entry by name.
     *
     * If the name of the entry is the last entry in the library, the first
     * entry in the list is returned.
     *
     * @param name - Name of current entry.
     *
     * @return LibCmpEntry - Pointer to next entry if entry name is found.
     *                       Otherwise NULL.
     */
    LibCmpEntry* GetNextEntry( const wxChar* name );


    /**
     * Find previous library entry by name.
     *
     * If the name of the entry is the first entry in the library, the last
     * entry in the list is returned.
     *
     * @param name - Name of current entry.
     *
     * @return LibCmpEntry - Pointer to previous entry if entry name is found.
     *                       Otherwise NULL.
     */
    LibCmpEntry* GetPreviousEntry( const wxChar* name );

    wxString GetName();

protected:
    wxFileName     m_fileName;      /* Library file name. */
    wxDateTime     m_DateTime;      /* Library save time and date. */
    wxString       m_Version;       /* Library save version. */
    LIB_ENTRY_LIST m_Entries;       /* Parts themselves are saved here. */
    bool           m_IsModified;    /* Library modification status. */

    friend class EDA_LibComponentStruct;
};


/**
 * Case insensitive library name comparison.
 */
extern bool operator==( const LibraryStruct& lib, const wxChar* name );
extern bool operator!=( const LibraryStruct& lib, const wxChar* name );

#endif  //  CLASS_LIBRARY_H
