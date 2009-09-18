/***********************************/
/*	Headers for library definition */
/***********************************/

#ifndef CLASS_LIBRARY_H
#define CLASS_LIBRARY_H


#include "class_libentry.h"


/* Helpers for creating a list of component libraries. */
class CMP_LIBRARY;

typedef boost::ptr_vector< CMP_LIBRARY > CMP_LIBRARY_LIST;

extern bool operator<( const CMP_LIBRARY& item1, const CMP_LIBRARY& item2 );


/**
 * Component library object.
 *
 * Component libraries are used to load, save, search, and otherwise manipulate
 * component library files.
 */

class CMP_LIBRARY
{
public:
    int            m_Type;         /* type indicator */
    wxString       m_Header;       /* first line of loaded library. */
    unsigned long  m_TimeStamp;    // Signature temporelle
    int            m_Flags;        // variable used in some functions

public:
    CMP_LIBRARY( int type, const wxFileName& fullname );
    CMP_LIBRARY( int type, const wxString& fullname )
    {
        CMP_LIBRARY( type, wxFileName( fullname ) );
    }
    ~CMP_LIBRARY();

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
    void LoadAliases( LIB_COMPONENT* component );

    void RemoveEntry( const wxString& name );

public:
    /**
     * Get library entry status.
     *
     * @return true if there are no entries in the library.
     */
    bool IsEmpty() const
    {
        return m_Entries.empty();
    }

    /**
     * Get the number of entries in the library.
     *
     * @return The number of component and alias entries.
     */
    int GetCount() const
    {
        return m_Entries.size();
    }

    bool IsModified() const
    {
        return m_IsModified;
    }


    bool IsCache() const { return m_IsCache; }


    void SetModified( void ) { m_IsModified = true; }


    void SetCache( void ) { m_IsCache = true; }


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
    CMP_LIB_ENTRY* FindEntry( const wxChar* name );

    /**
     * Find entry by name and type.
     *
     * @param name - Name of entry, case insensitive.
     * @param type - Type of entry, root or alias.
     *
     * @return Pointer to entry if found.  NULL if not found.
     */
    CMP_LIB_ENTRY* FindEntry( const wxChar* name, LibrEntryType type );

    /**
     * Find component by name.
     *
     * This is a helper for FindEntry so casting a CMP_LIB_ENTRY pointer to
     * a LIB_COMPONENT pointer is not required.
     *
     * @param name - Name of component, case insensitive.
     * @param searchAliases - Searches for component by alias name as well as
     *                        component name if true.
     *
     * @return Pointer to component if found.  NULL if not found.
     */
    LIB_COMPONENT* FindComponent( const wxChar* name );

    /**
     * Find alias by name.
     *
     * This is a helper for FindEntry so casting a CMP_LIB_ENTRY pointer to
     * a LIB_ALIAS pointer is not required.
     *
     * @param name - Name of alias, case insensitive.
     *
     * @return Pointer to alias if found.  NULL if not found.
     */
    LIB_ALIAS* FindAlias( const wxChar* name )
    {
        return (LIB_ALIAS*) FindEntry( name, ALIAS );
    }

    /**
     * Add a new alias entry to the library.
     *
     * First check if a component or alias with the same name already exists
     * in the library and add alias if no conflict occurs.  Once the alias
     * is added to the library it is owned by the library.  Deleting the
     * alias pointer will render the library unstable.  Use RemoveEntry to
     * remove the alias from the library.
     *
     * @param alias - Alias to add to library.
     *
     * @return bool - True if alias added to library.  False if conflict
     *                exists.
     */
    bool AddAlias( LIB_ALIAS* alias );

    /**
     * Add component entry to library.
     *
     * @param cmp - Component to add.
     *
     * @return Pointer to added component if successful.
     */
    LIB_COMPONENT* AddComponent( LIB_COMPONENT* cmp );

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
    void RemoveEntry( CMP_LIB_ENTRY* entry );

    /**
     * Replace an existing component entry in the library.
     *
     * @param oldComponent - The component to replace.
     * @param newComponent - The new component.
     */
    LIB_COMPONENT* ReplaceComponent( LIB_COMPONENT* oldComponent,
                                     LIB_COMPONENT* newComponent );

    /**
     * Return the first entry in the library.
     *
     * @return The first entry or NULL if the library has no entries.
     */
    CMP_LIB_ENTRY* GetFirstEntry()
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
     * @return CMP_LIB_ENTRY - Pointer to next entry if entry name is found.
     *                         Otherwise NULL.
     */
    CMP_LIB_ENTRY* GetNextEntry( const wxChar* name );


    /**
     * Find previous library entry by name.
     *
     * If the name of the entry is the first entry in the library, the last
     * entry in the list is returned.
     *
     * @param name - Name of current entry.
     *
     * @return CMP_LIB_ENTRY - Pointer to previous entry if entry name is found.
     *                         Otherwise NULL.
     */
    CMP_LIB_ENTRY* GetPreviousEntry( const wxChar* name );

    /**
     * Return the file name without path or extension.
     *
     * @return wxString - Name of library file.
     */
    wxString GetName() const { return m_fileName.GetName(); }

    /**
     * Return the full file library name with path and extension.
     *
     * @return wxString - Full library file name with path and extension.
     */
    wxString GetFullFileName() { return m_fileName.GetFullPath(); }

    /**
     * Set the component library file name.
     *
     * @param fileName - New library file name.
     */
    void SetFileName( const wxFileName fileName )
    {
        if( fileName != m_fileName )
            m_fileName = fileName;
    }

    /*
     * The following static methods are for manipulating the list of
     * component libraries.  This eliminates the need for yet another
     * global variable ( formerly g_LibraryList ) and gives some measure
     * of safety from abusing the library list.
     */

    /**
     * Load a component library file.
     *
     * @param fileName - File name of the component library to load.
     * @param errMsg - Error message if the component library failed to load.
     *
     * @return Pointer to library object if library file loaded successfully.
     *         Otherwise NULL.
     */
    static CMP_LIBRARY* LoadLibrary( const wxFileName& fileName,
                                     wxString& errMsg );

    /**
     * Add a compnent library to the library list.
     *
     * @param fileName - File name object of component library.
     * @param errMsg - Error message if the component library failed to load.
     *
     * @return bool - True if library loaded properly otherwise false.
     */
    static bool AddLibrary( const wxFileName& fileName, wxString& errMsg );

    /**
     * Insert a compnent library to the library list.
     *
     * @param fileName - File name object of component library.
     * @param errMsg - Error message if the component library failed to load.
     * @param i - Iterator to insert library in front of.
     *
     * @return bool - True if library loaded properly otherwise false.
     */
    static bool AddLibrary( const wxFileName& fileName, wxString& errMsg,
                            CMP_LIBRARY_LIST::iterator& i );

    /**
     * Remove component library from the library list.
     *
     * @param name - Name of component library to remove.
     */
    static void RemoveLibrary( const wxString& name );

    /**
     * Find component library by name.
     *
     * @param name - Library file name without path or extension to find.
     *
     * @return CMP_LIBRARY* - Pointer to component library if found,
     *                          otherwise NULL.
     */
    static CMP_LIBRARY* FindLibrary( const wxString& name );

    /**
     * Get the list of component library file names without path and extension.
     *
     * @param sorted - Sort the list of name if true.  Otherwise use the
     *                 library load order.
     *
     * @return wxArrayString - The list of library names.
     */
    static wxArrayString GetLibraryNames( bool sorted = true );

    /**
     * Search all libraries in the list for a component.
     *
     * A component object will always be returned.  If the entry found
     * is an alias.  The root component will be found and returned.
     *
     * @param name - Name of component to search for.
     * @param libNaem - Name of the library to search for component.
     *
     * @return Pointer to a valid component object if found.  Otherwise NULL.
     */
    static LIB_COMPONENT* FindLibraryComponent(
        const wxString& name, const wxString& libName = wxEmptyString );

    /**
     * Search all libraries in the list for an entry.
     *
     * The object can be either a component or an alias.
     *
     * @param name - Name of component to search for.
     * @param libNaem - Name of the library to search for entry.
     *
     * @return Pointer to a valid entry object if found.  Otherwise NULL.
     */
    static CMP_LIB_ENTRY* FindLibraryEntry(
        const wxString& name,
        const wxString& libName = wxEmptyString );

    /**
     * Remove all cache libraries from library list.
     */
    static void RemoveCacheLibrary( void );

    static int GetLibraryCount( void ) { return m_LibraryList.size(); }

    static CMP_LIBRARY_LIST& GetLibraryList( void )
    {
        return m_LibraryList;
    }

    static void SetSortOrder( const wxArrayString& sortOrder )
    {
        m_LibraryListSortOrder = sortOrder;
    }

    static wxArrayString& GetSortOrder( void )
    {
        return m_LibraryListSortOrder;
    }

protected:
    wxFileName     m_fileName;      /* Library file name. */
    wxDateTime     m_DateTime;      /* Library save time and date. */
    int            m_verMajor;      /* Library major version number. */
    int            m_verMinor;      /* Library minor version number. */
    LIB_ENTRY_LIST m_Entries;       /* Parts themselves are saved here. */
    bool           m_IsModified;    /* Library modification status. */
    bool           m_IsCache;       /* False for the "standard" libraries,
                                     * True for the library cache */

    static CMP_LIBRARY_LIST m_LibraryList;
    static wxArrayString    m_LibraryListSortOrder;

    friend class CMP_LIB_ENTRY;
};


/**
 * Case insensitive library name comparison.
 */
extern bool operator==( const CMP_LIBRARY& lib, const wxChar* name );
extern bool operator!=( const CMP_LIBRARY& lib, const wxChar* name );

#endif  //  CLASS_LIBRARY_H
