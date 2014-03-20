#ifndef SEARCH_STACK_H_
#define SEARCH_STACK_H_

#include <wx/filefn.h>
#include <wx/filename.h>


/**
 * Class SEARCH_STACK
 * looks for files in a number of places.  Augments wxPathList.
 * I chose the name because it sounded like a stack of work, as a reminder
 * that anything you put in here means searching work at some point in time.
 * (An alternative is to simply know where something is.)
 */
class SEARCH_STACK : public wxPathList
{
public:

#if defined(DEBUG)
    void Show( const char* aPrefix ) const;
#endif

    /**
     * Function  FilenameWithRelativePathInSearchList
     * @return a short filename (with extension) with only a relative path if
     *         this filename can be found in library paths
     * @param aFullFilename The filename with path and extension.
     */
    wxString FilenameWithRelativePathInSearchList( const wxString& aFullFilename );

    wxString FindValidPath( const wxString& aFileName ) const
    {
#if 1   // might not be needed

        if( wxFileName::FileExists( aFileName ) )
            return aFileName;
        else
#endif
            return wxPathList::FindValidPath( aFileName );
    }

    /**
     * Function FindValidPath
     * KiCad saves user defined library files that are not in the standard
     * library search path list with the full file path.  Calling the library
     * search path list with a user library file will fail.  This helper method
     * solves that problem.
     * @param fileName
     * @return a wxEmptyString if library file is not found.
     */
    wxString FindValidPath( const wxFileName& aFileName ) const
    {
        // call wxPathList::FindValidPath( const wxString& );
        return wxPathList::FindValidPath( aFileName.GetFullPath() );
    }

    /**
     * Function AddPaths
     * insert or append path(s)
     *
     * @param aPaths = path or path list to add. paths must be
     *  separated by ";" on windows, or ":" | ";" on unix.
     *
     * @param aIndex = insertion point, -1 for append.
     */
    void AddPaths( const wxString& aPaths, int aIndex = -1 );

    /**
     * Function RemovePaths
     * removes the given path(s) from the library path list
     * @param aPaths = path or list of paths to remove. If list, paths must be separated by
     * ";" on windows, or ":" | ";" on unix.
     */
    void RemovePaths( const wxString& aPaths );

    /**
     * Function FindFileInSearchPaths
     * looks in "this" for \a aFilename, but first modifies every search
     * path by appending a list of path fragments from aSubdirs.  That modification
     * is not rentative.
     */
    wxString FindFileInSearchPaths( const wxString&      aFilename,
                                    const wxArrayString* aSubdirs = NULL );
};


/**
 * Class RETAINED_PATH
 * is a glamorous way to save a path you might need in the future.
 * It is simply a container for the two functions, if you can figure them out.
 * This whole concept is awkward, and the two function might have better been
 * non-member functions, simply globals.
 */
class RETAINED_PATH
{
public:

    /**
     * Function LastVisitedPath
     * returns the last visited directory, or aSubPathToSearch is empty, the first
     * path in lib path list ( but not the CWD ).
     * @todo add more here if you can figure it out.
     *
     * @param aSearchStack gives the set of directories to consider.
     * @param aSubPathToSearch  is the preferred sub path to search in path list
     */
    wxString LastVisitedPath( const SEARCH_STACK& aSStack,
                const wxString& aSubPathToSearch = wxEmptyString );

    void SaveLastVisitedPath( const wxString& aPath );

    void Clear();

private:
    wxString            m_retained_path;
};

#endif  // SEARCH_STACK_H_
