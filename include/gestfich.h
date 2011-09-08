/**
 * This file is part of the common library
 * TODO brief description
 * @file  gestfich.h
 * @see   common.h
 */


#ifndef __INCLUDE__GESTFICH_H__
#define __INCLUDE__GESTFICH_H__ 1

#include <wx/filename.h>


/* Forward class declarations. */
class EDA_LIST_DIALOG;


/**
 * Function OpenPDF
 * run the PDF viewer and display a PDF file
 * @param file = PDF file to open
 * @return true is success, false if no PDF viewer found
 */
bool     OpenPDF( const wxString& file );

void     OpenFile( const wxString& file );

bool     EDA_DirectorySelector( const wxString& Title,
                                wxString&       Path,
                                int             flag,       /* reserve */
                                wxWindow*       Frame,
                                const wxPoint&  Pos );

wxString EDA_FileSelector( const wxString& Title,
                           const wxString& Path,
                           const wxString& FileName,
                           const wxString& Ext,
                           const wxString& Mask,
                           wxWindow*       Frame,
                           int             flag,
                           const bool      keep_working_directory,
                           const wxPoint&  Pos = wxPoint( -1, -1 ) );


/* Return file name without path or extension.
 *
 * If the path is in the default kicad path, ./ is prepended to the
 * file name.  If the file name has the default extension, the file
 * name is returned without an extension.
 */

wxString MakeReducedFileName( const wxString& fullfilename,
                              const wxString& default_path,
                              const wxString& default_ext );

EDA_LIST_DIALOG* GetFileNames( char* Directory, char* Mask );


int            ExecuteFile( wxWindow* frame, const wxString& ExecFile,
                             const wxString& param = wxEmptyString );
void           AddDelimiterString( wxString& string );

/* Find absolute path for kicad/help (or kicad/help/<language>) */
wxString       FindKicadHelpPath();


/* Return the kicad common data path. */
wxString       ReturnKicadDatasPath();


/* Search the executable file shortname in kicad binary path and return
 * full file name if found or shortname */
wxString       FindKicadFile( const wxString& shortname );


/**
 * Quote return value of wxFileName::GetFullPath().
 *
 * This allows file name paths with spaces to be used as parameters to
 * ProcessExecute function calls.
 * @param fn is the filename to wrap
 * @param format if provided, can be used to transform the nature of the
 *    wrapped filename to another platform.
 */
extern wxString QuoteFullPath( wxFileName& fn,
                               wxPathFormat format = wxPATH_NATIVE );

#endif /* __INCLUDE__GESTFICH_H__ */
