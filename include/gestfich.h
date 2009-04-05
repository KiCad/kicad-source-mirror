/**
 * This file is part of the common libary
 * TODO brief description
 * @file  gestfich.h
 * @see   common.h
 */


#ifndef __INCLUDE__GESTFICH_H__
#define __INCLUDE__GESTFICH_H__ 1

#include <wx/filename.h>


/* Forward class declarations. */
class WinEDAListBox;


/** Function OpenPDF
 * run the PDF viewer and display a PDF file
 * @param file = PDF file to open
 * @return true is success, false if no PDF viewer found
 */
bool        OpenPDF( const wxString& file );

void        OpenFile( const wxString& file );

bool        EDA_DirectorySelector( const wxString& Title,           /* Titre de la fenetre */
                                   wxString&       Path,            /* Chemin par defaut */
                                   int             flag,            /* reserve */
                                   wxWindow*       Frame,           /* parent frame */
                                   const wxPoint&  Pos );

wxString EDA_FileSelector( const wxString &Title,                   /* Window title */
                           const wxString &Path,                    /* default path */
                           const wxString &FileName,                /*  default filename */
                           const wxString &Ext,                     /* default extension */
                           const wxString &Mask,                    /* Display filename mask */
                           wxWindow * Frame,                        /* parent frame */
                           int flag,                                /* wxSAVE, wxOPEN ..*/
                           const bool keep_working_directory,       /* true = do not change the C.W.D. */
                           const wxPoint& Pos = wxPoint( -1, -1 )
                           );


/* Calcule le nom complet d'un file d'apres les chaines
 *  dir = prefixe (chemin)
 *  shortname = nom avec ou sans chemin ou extension
 *  ext = extension
 *
 *  si la chaine name possede deja un chemin ou une extension, elles
 *  ne seront pas modifiees
 *
 *  retourne la chaine calculee */

wxString    MakeReducedFileName( const wxString& fullfilename,
                                 const wxString& default_path,
                                 const wxString& default_ext );

/* Calcule le nom "reduit" d'un file d'apres les chaines
 *  fullfilename = nom complet
 *  default_path = prefixe (chemin) par defaut
 *  default_ext = extension par defaut
 *
 *  retourne le nom reduit, c'est a dire:
 *  sans le chemin si le chemin est default_path
 *  avec ./ si si le chemin est le chemin courant
 *  sans l'extension si l'extension est default_ext
 *
 *  Renvoie un chemin en notation unix ('/' en separateur de repertoire)
 */

WinEDAListBox*  GetFileNames( char* Directory, char* Mask );


/* Change l'extension du "filename FullFileName" en NewExt.
 *     Retourne FullFileName */

int             ExecuteFile( wxWindow* frame, const wxString& ExecFile,
                             const wxString& param = wxEmptyString );
void            AddDelimiterString( wxString& string );

void            SetRealLibraryPath( const wxString& shortlibname ); /* met a jour
                                                                    *  le chemin des librairies RealLibDirBuffer (global)
                                                                    *  a partir de UserLibDirBuffer (global):
                                                                    *  Si UserLibDirBuffer non vide RealLibDirBuffer = UserLibDirBuffer.
                                                                    *  Sinon si variable d'environnement KICAD definie (KICAD = chemin pour kicad),
                                                                    *  UserLibDirBuffer = <KICAD>/shortlibname;
                                                                    *  Sinon UserLibDirBuffer = <Chemin des binaires>../shortlibname/
                                                                    */
wxString        FindKicadHelpPath();

/* Find absolute path for kicad/help (or kicad/help/<language>) */

wxString        ReturnKicadDatasPath();

/* Retourne le chemin des donnees communes de kicad. */

wxString        FindKicadFile( const wxString& shortname );

/* Search the executable file shortname in kicad binary path and return
 *  full file name if found or shortname */


/**
 * Quote return value of wxFileName::GetFullPath().
 *
 * This allows file name paths with spaces to be used as parameters to
 * ProcessExecute function calls.  This is a cheap and dirty hack and
 * should probably should be done in a class derived from wxFileName.
 */
extern wxString QuoteFullPath( wxFileName& fn,
                               wxPathFormat format = wxPATH_NATIVE );

#endif /* __INCLUDE__GESTFICH_H__ */

