#ifndef IMPORT_PROJ_H
#define IMPORT_PROJ_H

#include "kicad_manager_frame.h"
#include <wx/filename.h>
#include <core/typeinfo.h>

/**
 * A helper class to import non Kicad project.
 * */
class IMPORT_PROJ_HELPER
{
public:
    IMPORT_PROJ_HELPER( KICAD_MANAGER_FRAME* aframe, const wxString& aFile,
                        const wxString& aSchFileExtension, const wxString& aPcbFileExtension );
    const wxFileName& GetProj();
    wxString          GetProjPath();
    void              SetProjPath( const wxString aPath );
    wxString          GetProjFullPath();
    wxString          GetProjName();

    /**
     * @brief Appends a new directory with the name of the project file
     *        Keep iterating until an empty directory is found
     */
    void              CreateEmptyDirForProject();

    void              SetProjAbsolutePath();

    /**
     * @brief Copies project files to the destination directory
     * @param displayError calls OutputCopyError() if true
     */
    bool              CopyImportedFiles( bool displayError = true );

    /**
     * @brief Converts imported files to kicad type files.
     *        Types of imported files are needed for conversion
     * @param aImportedSchFileType type of the imported schematic
     * @param aImportedPcbFileType type of the imported PCB
     */
    void              AssociateFilesWithProj( int aImportedSchFileType, int aImportedPcbFileType );

private:
    KICAD_MANAGER_FRAME* m_frame;
    wxFileName           m_sch;
    wxFileName           m_shCopy;
    wxFileName           m_pcb;
    wxFileName           m_pcbCopy;
    wxFileName           m_pro;
    bool                 CopyImportedFile( KICAD_T aKicad_T, bool displayError = true );
    void                 OutputCopyError( const wxFileName& aSrc, const wxFileName& aFileCopy );
    void                 AssociateFileWithProj( KICAD_T aKicad_T, int aImportedFileType );
};

#endif