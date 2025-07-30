#ifndef PROJECT_GIT_UTILS_H
#define PROJECT_GIT_UTILS_H

#include <git2.h>
#include <wx/string.h>

namespace KIGIT
{

/** Utility class with helper functions for project level git operations. */
class PROJECT_GIT_UTILS
{
public:
    /**
     * Discover and open the repository that contains the given file.
     *
     * @param aFilename Absolute path to a file inside the repository.
     * @return Newly opened git_repository or nullptr on failure.
     */
    static git_repository* GetRepositoryForFile( const char* aFilename );

    /**
     * Create a new branch based on HEAD.
     *
     * @param aRepo       Repository in which to create the branch.
     * @param aBranchName Name of the new branch.
     * @return 0 on success, libgit2 error code on failure.
     */
    static int CreateBranch( git_repository* aRepo, const wxString& aBranchName );
};

} // namespace KIGIT

#endif // PROJECT_GIT_UTILS_H
