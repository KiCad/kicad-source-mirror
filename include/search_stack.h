#ifndef SEARCH_STACK_H_
#define SEARCH_STACK_H_

#include <wx/filefn.h>
#include <wx/filename.h>
#include <project.h>


/**
 * Class SEARCH_STACK
 * looks for files in a number of places.  Augments wxPathList.
 * I chose the name because it sounded like a stack of work, as a reminder
 * that anything you put in here means searching work at some point in time.
 * (An alternative is to simply know where something is.)
 */
class SEARCH_STACK : public wxPathList, public PROJECT::_ELEM
{
public:

#if defined(DEBUG)
    void Show( const char* aPrefix ) const;
#endif

    /**
     * Function  FilenameWithRelativePathInSearchList
     * returns the shortest possible path which can be use later to find
     * a full path from this SEARCH_STACK.
     * <p>
     * If the library path is already in the library search paths list,
     * just add the library name to the list. Otherwise, add the library
     * name with the full or relative path. The relative path is preferable
     * because it preserves use of default libraries paths, when the path
     * is a sub path of these default paths. Note we accept only sub paths
     * not relative paths starting by ../ that are not subpaths and are
     * outside kicad libs paths
     *
     * @param aFullFilename The filename with path and extension.
     * @param aBaseDir The absolute path on which relative paths in this
     *  SEARCH_STACK are based.
     * @return a short filename (with extension) with only a relative path if
     *         this filename can be found in library paths
     */
    wxString FilenameWithRelativePathInSearchList(
            const wxString& aFullFilename, const wxString& aBaseDir );

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
     * Function Split
     * separates aPathString into individual paths.
     * @param aResult is where to put the paths, it should be empty upon entry.
     * @param aPathString is concatonated string with interposing ';' or ':' separators.
     * @return int - the count of paths found in aPathString
     */
    static int Split( wxArrayString* aResult, const wxString aPathString );

#if 1   // this function is so poorly designed it deserves not to exist.
    /**
     * Function LastVisitedPath
     * is a quirky function inherited from old code that seems to serve particular
     * needs in the UI.  It returns what is called the last visited directory, or
     * if aSubPathToSearch is empty, the first path in this SEARCH_STACK
     * ( but not the CWD ).
     *
     * @todo add more here if you can figure it out.
     *
     * @param aSubPathToSearch is the preferred sub path to search in path list
     */
    const wxString LastVisitedPath( const wxString& aSubPathToSearch = wxEmptyString );
#endif

};

#endif  // SEARCH_STACK_H_
