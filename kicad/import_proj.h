#ifndef IMPORT_PROJ_H
#define IMPORT_PROJ_H

#include <wx/filename.h>
#include <core/typeinfo.h>
#include <core/utf8.h>

#include <map>

class KICAD_MANAGER_FRAME;

/**
 * A helper class to import non Kicad project.
 * */
class IMPORT_PROJ_HELPER
{
public:
    IMPORT_PROJ_HELPER( KICAD_MANAGER_FRAME*         aframe,
                        const std::vector<wxString>& aSchFileExtensions,
                        const std::vector<wxString>& aPcbFileExtensions );

    /**
     * @brief Appends a new directory with the name of the project file
     *        Keep iterating until an empty directory is found
     */
    void FindEmptyTargetDir();

    /**
     * @brief Converts imported files to kicad type files.
     *        Types of imported files are needed for conversion
     * @param aImportedSchFileType type of the imported schematic
     * @param aImportedPcbFileType type of the imported PCB
     */
    void ImportFiles( int aImportedSchFileType, int aImportedPcbFileType );

    wxFileName m_InputFile;
    wxFileName m_TargetProj;

private:
    KICAD_MANAGER_FRAME* m_frame;

    std::map<std::string, UTF8> m_properties;

    std::vector<wxString> m_copiedSchPaths;
    std::vector<wxString> m_copiedPcbPaths;

    std::vector<wxString> m_schExtenstions;
    std::vector<wxString> m_pcbExtenstions;

    void OutputCopyError( const wxFileName& aSrc, const wxFileName& aFileCopy );
    void ImportIndividualFile( KICAD_T aKicad_T, int aImportedFileType );

    void EasyEDAProProjectHandler();
};

#endif