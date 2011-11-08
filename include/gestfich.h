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
bool OpenPDF( const wxString& file );

void OpenFile( const wxString& file );

bool EDA_DirectorySelector( const wxString& Title,
                            wxString&       Path,
                            int             flag,       /* reserve */
                            wxWindow*       Frame,
                            const wxPoint&  Pos );

/* Selection file dialog box:
 * Dialog title
 * Default path
 * default filename
 * default filename extension
 * filter for filename list
 * parent frame
 * wxFD_SAVE, wxFD_OPEN ..
 * true = keep the current path
 */
wxString EDA_FileSelector( const wxString& Title,
                           const wxString& Path,
                           const wxString& FileName,
                           const wxString& Ext,
                           const wxString& Mask,
                           wxWindow*       Frame,
                           int             flag,
                           const bool      keep_working_directory,
                           const wxPoint&  Pos = wxPoint( -1, -1 ) );


/**
 * Function MakeReducedFileName
 * calculate the "reduced" filename from \a fullfilename.
 *
 * @param fullfilename = full filename
 * @param default_path = default path
 * @param default_ext = default extension
 * @return  the "reduced" filename, i.e.:
 *  without path if it is default_path
 *  with ./ if the path is the current path
 *  without extension if extension is default_ext
 *
 *  the new filename is in unix like notation ('/' as path separator)
 */
wxString MakeReducedFileName( const wxString& fullfilename,
                              const wxString& default_path,
                              const wxString& default_ext );

EDA_LIST_DIALOG* GetFileNames( char* Directory, char* Mask );


/**
 * Function ExecuteFile
 * calls the executable file \a ExecFile with the command line parameters \a param.
 */
int ExecuteFile( wxWindow* frame, const wxString& ExecFile,
                 const wxString& param = wxEmptyString );

/**
 * Function AddDelimiterString
 * Add un " to the start and the end of string (if not already done).
 * @param string = string to modify
 */
void AddDelimiterString( wxString& string );

/**
 * Function FindKicadHelpPath
 * finds the absolute path for KiCad "help" (or "help/&ltlanguage&gt")
 *  Find path kicad/doc/help/xx/ or kicad/doc/help/:
 *  from BinDir
 *  else from environment variable KICAD
 *  else from one of s_HelpPathList
 *  typically c:/kicad/doc/help or /usr/share/kicad/help
 *            or /usr/local/share/kicad/help
 *  (must have kicad in path name)
 *
 *  xx = iso639-1 language id (2 letters (generic) or 4 letters):
 *  fr = french (or fr_FR)
 *  en = English (or en_GB or en_US ...)
 *  de = deutch
 *  es = spanish
 *  pt = portuguese (or pt_BR ...)
 *
 *  default = en (if not found = fr)
 */
wxString FindKicadHelpPath();

/**
 * Function ReturnKicadDatasPath
 * returns the data path common to KiCad.
 * If environment variable KICAD is defined (KICAD = path to kicad)
 * Returns \<KICAD\> /;
 * Otherwise returns \<path of binaries\> / (if "kicad" is in the path name)
 * Otherwise returns /usr /share/kicad/
 *
 * Note:
 * The \\ are replaced by / (a la Unix)
 */
wxString ReturnKicadDatasPath();

/**
 * Function FindKicadFile
 * searches the executable file shortname in KiCad binary path  and return full file
 * name if found or shortname if the kicad binary path is kicad/bin.
 *
 *  kicad binary path is found from:
 *  BinDir
 *  or environment variable KICAD
 *  or (default) c:\\kicad or /usr/local/kicad
 *  or default binary path
 */
wxString FindKicadFile( const wxString& shortname );

/**
 * Quote return value of wxFileName::GetFullPath().
 *
 * This allows file name paths with spaces to be used as parameters to
 * ProcessExecute function calls.
 * @param fn is the filename to wrap
 * @param format if provided, can be used to transform the nature of the
 *    wrapped filename to another platform.
 */
extern wxString QuoteFullPath( wxFileName& fn, wxPathFormat format = wxPATH_NATIVE );

#endif /* __INCLUDE__GESTFICH_H__ */
