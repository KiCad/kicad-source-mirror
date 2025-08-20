#ifndef SAVE_PROJECT_UTILS_H
#define SAVE_PROJECT_UTILS_H

#include <unordered_map>
#include <wx/filename.h>

class SCHEMATIC;
class SCH_SCREEN;
class SCH_SCREENS;

bool PrepareSaveAsFiles( SCHEMATIC& aSchematic, SCH_SCREENS& aScreens,
                         const wxFileName& aOldRoot, const wxFileName& aNewRoot,
                         bool aSaveCopy, bool aCopySubsheets, bool aIncludeExternSheets,
                         std::unordered_map<SCH_SCREEN*, wxString>& aFilenameMap,
                         wxString& aErrorMsg );

#endif
