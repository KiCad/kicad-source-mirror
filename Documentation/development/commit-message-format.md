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

There is an [alias](#commit_fixes_alias) to simplify this step.

# Changelog tags {#commit_changelog_tag}

To facilitate following the code changes, you should include a changelog tag to
indicate modifications noticable by the users. There are three types of
changelog tags:

- `NEW` to denote a new feature
- `CHANGED` to indicate a modification of an existing feature
- `REMOVED` to inform about removal of an existing feature

There is no need to add changelog tags for commits that do not modify the way
the users interact with the software, such as code refactoring or a bugfix for
unexpected behavior. The main purpose of the changelog tags is to generate the
release notes and notify the documentation maintainers about changes. Keep that
in mind when deciding whether to add a changelog tag.

When a commit with changelog tags is pushed, the committer should create a new
issue in the [documentation
repository](http://github.com/KiCad/kicad-doc/issues) to notify the
documentation maintainers. You should include a link to the commit containing
the reported changes.

## Extracting changelog {#commit_extract_changelog}

Thanks to the changelog tags, it is easy to extract the changelog using git
commands:

    git log -E --grep="ADD:|NEW:|REMOVE[D]?:|CHANGE[D]?:" --since="1 Jan 2017"
    git log -E --grep="ADD:|NEW:|REMOVE[D]?:|CHANGE[D]?:" <commit hash>

KiCad provides an [alias](#commit_changelog_alias) to shorten the
changelog extraction commands.

# Example # {#commit_example}

Following is an example of a properly formatted commit message:

    Eeschema: Adding line styling options
    
    NEW: Adds support in eeschema for changing the default line style,
    width and color on a case-by-case basis.
    
    CHANGED: "Wire" lines now optionally include data on the line style,
    width and color if they differ from the default.
    
    Fixes: lp:594059
    * https://bugs.launchpad.net/kicad/+bug/594059
    
    Fixes: lp:1405026
    * https://bugs.launchpad.net/kicad/+bug/1405026

# Git aliases file # {#commit_git_aliases}

There is a file containing helpful git aliases located at
`helpers/git/fixes_alias`. To install it, run in the source repository:

    git config --add include.path $(pwd)/helpers/git/fixes_alias

## 'fixes' alias # {#commit_fixes_alias}

Once the alias configuration file is installed, it may be used to amend the
most recent commit to include the bug report link:

    git fixes 1234567

The example command would add the following lines to the last commit message:

    Fixes: lp:1234567
    * https://bugs.launchpad.net/kicad/+bug/1234567

## 'changelog' alias # {#commit_changelog_alias}

With the alias configuration file installed, you get an alias to extract the changelog:

    git changelog --since="1 Jan 2017"
    git changelog <commit hash>
