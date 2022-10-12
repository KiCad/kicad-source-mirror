
## Joining the development team

To begin contributing to KiCad, you should first join the [KiCad Developers mailing list](https://groups.google.com/a/kicad.org/g/devlist). This mailing list is used for announcements about development progress (milestones, deadlines, new releases, etc.), discussing the implementation of new features, and for asking general questions about the KiCad codebase.

New developers are encouraged to start small with their contribution, and gradually work their way up to larger changes as they gain knowledge in the codebase. The development team labels issues in the [issue tracker](https://gitlab.com/kicad/code/kicad/-/issues) with "[starter](https://gitlab.com/kicad/code/kicad/-/issues?scope=all&utf8=%E2%9C%93&state=opened&label_name[]=starter)" when the issue or feature being requested is a good way for a new person to contribute to KiCad. Alternately, you can search through the issue tracker for any issues that look interesting and leave a comment saying you are interested in working on it. If you have any questions while working on these issues, please leave a comment in the issue with the question or comment so other developers can help you.

Developing any larger change, such as a new feature, should be discussed on the developers mailing list before substantial work is done. This allows for input from the lead development team to ensure the feature is aligned with the current development goals, and to prevent duplication of work by contributors.


## Submitting merge requests
KiCad welcomes contributions via merge requests on GitLab. Here are some tips to help make sure your contribution can be accepted quickly:

### General Guidelines:
1. Always create a new branch for merge requests instead of using your fork's master branch.
2. Make sure your code submission follows the [KiCad Code Style Guide](https://dev-docs.kicad.org/en/rules-guidelines/code-style/), see below for some details.
3. Make sure all User Interface changes follow the [User Interface Guidelines](https://dev-docs.kicad.org/en/rules-guidelines/ui/).
4. If you are planning a large change or new feature, be sure to ask on the [developer mailing list](https://groups.google.com/a/kicad.org/g/devlist) before you begin your work to see if anyone else is working on it and to ensure that it fits into the overall development plans.
5. Give merge requests a short and descriptive title that summarizes the major changes it contains. A longer description of the changes should be contained inside the description of the merge request.

### Code style and formatting

Make sure to read the [KiCad Code Style Guide](https://dev-docs.kicad.org/en/rules-guidelines/code-style/) if you haven't already. You can use the `clang-format` tool to check many, but not all, of these style requirements. When you create a merge request, one of the CI pipeline steps will be to run a formatting check on your contribution. This automatic check is not always 100% correct. Some tips to interpreting the results of automatic format checks:

1. Some of our formatting guidelines have exceptions, or only apply to certain situations. `clang-format` doesn't know about these nuances, so it will sometimes suggest that you make sweeping format changes to areas of a file near your code (even if you didn't change that code). Keep in mind Rule 7 of the style guide: when there is flexibility or doubt, follow the existing formatting of the file you are editing, rather than rigidly following `clang-format`.

2. `clang-format` doesn't know about our desire for nice column-formatting where applicable (Rule 4.1.2)

3. `clang-format` doesn't support our preferred lambda format (Rule 4.10)

4. `clang-format` suggests that you alphabetize any `#include` directives at the top of a file. Please do not do this for existing files unless you are making sweeping changes to the list of `#include`s anyway.

### GitLab settings

Please configure your personal fork of the KiCad project with the following settings:

1. Settings->General->Visibility->CI/CD should be enabled and set to "Everyone with access".
2. Settings->CI/CD->General pipelines Timeout should be set to 3 hours or longer.
3. The "Allow commits from members who can merge to the target branch." option check box at the bottom of your merge request must be checked.
