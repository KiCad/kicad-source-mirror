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
};

#endif  // SEARCH_STACK_H_
