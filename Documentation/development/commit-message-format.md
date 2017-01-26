# Commit Message Format # {#commit_messages}

[TOC]

Commit messages should begin with a subject line; try to limit this to no more
than 50-72 characters. The body of the message should be separated from the
subject by a blank line and wrapped at 72 characters. The body of a commit
message should explain what the commit does and why, but do not explain *how*
as the code itself should do that.

# Linking a commit to a bug report # {#commit_bug_link}

If your commit fixes a bug that has been reported in the [Launchpad bug
tracker](https://bugs.launchpad.net/kicad/+bugs), end your commit with the
following lines to mark it as fixed, where `1234567` represents the actual
bug ID. A bot will automatically set the bug status to "Fix Committed" and link
to the commit once it is merged.

    Fixes: lp:1234567
    https://bugs.launchpad.net/kicad/+bug/1234567

## Helper alias for linking to bugs # {#commit_link_helper}

There is a helper alias located at `helpers/git/fixes_alias` to simplify adding
these links. To install it, run in the source repository:

    git config --add include.path $(pwd)/helpers/git/fixes_alias

Once the alias is installed, it may be used to amend the most recent commit to
include the bug report link:

    git fixes 1234567

# Example # {#commit_example}

Following is an example of a properly formatted commit message:

    Allow editing locked modules in modedit
    
    Since you have to explicitly enter the module editor with the menu or
    hotkey, allowing editing of module sub-parts once in should not cause
    any unexpected changes.
    
    Fixes: lp:1591625
    * https://bugs.launchpad.net/kicad/+bug/1591625
