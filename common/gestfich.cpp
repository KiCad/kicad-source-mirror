/************************************************/
/*	 MODULE: gestfich.cpp						*/
/*	 ROLE: fonctions de gestion de fichiers	*/
/************************************************/

// For compilers that support precompilation, includes "wx.h".
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "confirm.h"

#ifdef  __WINDOWS__
#ifndef _MSC_VER
//#include <dir.h>
#endif
#endif

#include "common.h"
#include "macros.h"
#include "gestfich.h"

#include "wx/mimetype.h"
#include "wx/filename.h"


/* List of default paths used to locate help files and kicad library files.
 *
 * Under windows, kicad search its files from the binary path file (first argument when running "main")
 * So for a standard install, default paths are not mandatory, but they exist, just in case.
 * kicad is often installed in c:/Program Files/kicad or c:/kicad (or d: or e: ... )
 * and the directory "share" has no meaning under windows.
 *
 * Under linux, the problem is more complex.
 * In fact there are 3 cases:
 * 1 - When released in a distribution:
 * binaries are in /usr/bin, kicad libs in /usr/share/kicad/ and doc in /usr/share/doc/kicad/
 * 2 - When compiled by an user:
 * binaries also can be  in /usr/local/bin, kicad libs in /usr/local/share/kicad/ and doc in /usr/local/share/doc/kicad/
 * 3 - When in an "universal tarball" or build for a server:
 * all files are in /usr/local/kicad
 * This is mandatory when kicad is installed on a server (in a school for instance) because one can export /usr/local/kicad
 * and obviously the others paths cannot be used
 * (cannot be mounted by the client, because they are already used).
 *
 * in cases 1 and 2 kicad files cannot be found from the binary path.
 * in case 3 kicad files can be found from the binary path only if this is a kicad binary file which is launched.
 *     But if an user creates a symbolic link to the actual binary file to run kicad, the binary path is not good
 *     and the defaults paths must be used
 *
 * Note:
 * kicad uses first the bin path lo locace kicad tree.
 * if not found kicad uses the environment variable KICAD to find its files
 * and at last kicad uses the default paths.
 * So we can export (linux and windows) the variable KICAD:
 *  like export KICAD=/my_path/kicad if /my_path/kicad is not a default path
 */

// Path list for online help
static wxString    s_HelpPathList[] = {
#ifdef __WINDOWS__
    wxT( "c:/kicad/doc/help/" ),
    wxT( "d:/kicad/doc/help/" ),
    wxT( "c:/Program Files/kicad/doc/help/" ),
    wxT( "d:/Program Files/kicad/doc/help/" ),
#else
    wxT( "/usr/share/doc/kicad/help/" ),
    wxT( "/usr/local/share/doc/kicad/help/" ),
    wxT( "/usr/local/kicad/doc/help/" ),        // default install for "universal tarballs" and build for a server (new)
    wxT( "/usr/local/kicad/help/" ),            // default install for "universal tarballs" and build for a server (old)
#endif
    wxT( "end_list" )                           // End of list symbol, do not change
};

// Path list for kicad data files
static wxString    s_KicadDataPathList[] = {
#ifdef __WINDOWS__
    wxT( "c:/kicad/share/" ),
    wxT( "d:/kicad/share/" ),
    wxT( "c:/kicad/" ),
    wxT( "d:/kicad/" ),
    wxT( "c:/Program Files/kicad/share/" ),
    wxT( "d:/Program Files/kicad/share/" ),
    wxT( "c:/Program Files/kicad/" ),
    wxT( "d:/Program Files/kicad/" ),
#else
    wxT( "/usr/share/kicad/" ),
    wxT( "/usr/local/share/kicad/" ),
    wxT( "/usr/local/kicad/share/" ),       // default data path for "universal tarballs" and build for a server (new)
    wxT( "/usr/local/kicad/" ),             // default data path for "universal tarballs" and build for a server (old)
#endif
    wxT( "end_list" )                       // End of list symbol, do not change
};

// Path list for kicad binary files
static wxString    s_KicadBinaryPathList[] = {
#ifdef __WINDOWS__
    wxT( "c:/kicad/bin/" ),
    wxT( "d:/kicad/bin/" ),
    wxT( "c:/Program Files/kicad/bin/" ),
    wxT( "d:/Program Files/kicad/bin/" ),
#else
    wxT( "/usr/bin/" ),
    wxT( "/usr/local/bin/" ),
    wxT( "/usr/local/kicad/bin/" ),
#endif
    wxT( "end_list" )                               // End of list symbol, do not change
};


/***************************************************************************/
wxString MakeReducedFileName( const wxString& fullfilename,
                              const wxString& default_path,
                              const wxString& default_ext )
/***************************************************************************/

/** Function MakeReducedFileName
 * Calculate the "reduced" filename from
 * @param  fullfilename = full filename
 * @param  default_path = default path
 * @param  default_ext = default extension
 *
 * @return  the "reduced" filename, i.e.:
 *  without path if it is default_path
 *  wiht ./ if the path is the current path
 *  without extension if extension is default_ext
 *
 *  the new flename is in unix like notation ('/' as path separator)
 */
{
    wxString reduced_filename = fullfilename;
    wxString Cwd, ext, path;

    Cwd  = default_path;
    ext  = default_ext;
    path = wxPathOnly( reduced_filename ) + UNIX_STRING_DIR_SEP;
    reduced_filename.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
    Cwd.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
    if( Cwd.Last() != '/' )
        Cwd += UNIX_STRING_DIR_SEP;
    path.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

#ifdef __WINDOWS__

    // names are case insensitive under windows
    path.MakeLower();
    Cwd.MakeLower();
    ext.MakeLower();
#endif

    // if the path is "default_path" -> remove it
    wxString root_path = path.Left( Cwd.Length() );
    if( root_path == Cwd )
    {
        reduced_filename.Remove( 0, Cwd.Length() );
    }
    else    // if the path is the current path -> change path to ./
    {
        Cwd = wxGetCwd() + UNIX_STRING_DIR_SEP;
#ifdef __WINDOWS__
        Cwd.MakeLower();
#endif
        Cwd.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
        if( path == Cwd )
        {   // the path is the current path -> path = "./"
            reduced_filename.Remove( 0, Cwd.Length() );
            wxString tmp = wxT( "./" ) + reduced_filename;
            reduced_filename = tmp;
        }
    }

    // remove extension if == default_ext:
    if( !ext.IsEmpty() && reduced_filename.Contains( ext ) )
        reduced_filename.Truncate( reduced_filename.Length() - ext.Length() );

    return reduced_filename;
}


/*******************************************/
void AddDelimiterString( wxString& string )
/*******************************************/

/** Function AddDelimiterString
 * Add un " to the start and the end of string (if not already done).
 * @param string = string to modify
 */
{
    wxString text;

    if( !string.StartsWith( wxT( "\"" ) ) )
        text = wxT( "\"" );
    text += string;
    if( (text.Last() != '"' ) || (text.length() <= 1) )
        text += wxT( "\"" );
    string = text;
}


/***********************************/
/* Selection Directory dialog box: */
/***********************************/

bool EDA_DirectorySelector( const wxString& Title,      /* Titre de la fenetre */
                            wxString&       Path,       /* Chemin par defaut */
                            int             flag,       /* reserve */
                            wxWindow*       Frame,      /* parent frame */
                            const wxPoint&  Pos )
{
    int          ii;
    bool         selected = FALSE;

    wxDirDialog* DirFrame = new wxDirDialog(
        Frame,
        wxString( Title ),
        Path,                   /* Chemin par defaut */
        flag,
        Pos );

    ii = DirFrame->ShowModal();
    if( ii == wxID_OK )
    {
        Path     = DirFrame->GetPath();
        selected = TRUE;
    }

    DirFrame->Destroy();
    return selected;
}


/******************************/
/* Selection file dialog box: */
/******************************/

wxString EDA_FileSelector( const wxString& Title,                   /* Dialog title */
                           const wxString& Path,                    /* Default path */
                           const wxString& FileName,                /* default filename */
                           const wxString& Ext,                     /* default filename extension */
                           const wxString& Mask,                    /* filter for filename list */
                           wxWindow*       Frame,                   /* parent frame */
                           int             flag,                    /* wxFD_SAVE, wxFD_OPEN ..*/
                           const bool      keep_working_directory,  /* true = keep the current path */
                           const wxPoint&  Pos )
{
    wxString fullfilename;
    wxString curr_cwd    = wxGetCwd();
    wxString defaultname = FileName;
    wxString defaultpath = Path;
    wxString dotted_Ext = wxT(".") + Ext;

    defaultname.Replace( wxT( "/" ), STRING_DIR_SEP );
    defaultpath.Replace( wxT( "/" ), STRING_DIR_SEP );
    if( defaultpath.IsEmpty() )
        defaultpath = wxGetCwd();

    wxSetWorkingDirectory( defaultpath );

#if 0 && defined (DEBUG)
    printf(
        "defaultpath=\"%s\" defaultname=\"%s\" Ext=\"%s\" Mask=\"%s\" flag=%d keep_working_directory=%d\n",
        CONV_TO_UTF8( defaultpath ),
        CONV_TO_UTF8( defaultname ),
        CONV_TO_UTF8( Ext ),
        CONV_TO_UTF8( Mask ),
        flag,
        keep_working_directory
        );
#endif

    fullfilename = wxFileSelector( wxString( Title ),
                                   defaultpath,
                                   defaultname,
                                   dotted_Ext,
                                   Mask,
                                   flag, /* open mode wxFD_OPEN, wxFD_SAVE .. */
                                   Frame,
                                   Pos.x, Pos.y );

    if( keep_working_directory )
        wxSetWorkingDirectory( curr_cwd );

    return fullfilename;
}


/********************************************************/
wxString FindKicadHelpPath()
/********************************************************/

/** Function FindKicadHelpPath
 * Find an absolute path for KiCad "help" (or "help/<language>")
 *  Find path kicad/doc/help/xx/ or kicad/doc/help/:
 *  from BinDir
 *  else from environment variable KICAD
 *  else from one of s_HelpPathList
 *  typically c:\kicad\doc\help or /usr/share/kicad/help
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
 *
 */
{
    wxString FullPath, LangFullPath, tmp;
    wxString LocaleString;
    bool     PathFound = FALSE;

    /* find kicad/help/ */
    tmp = wxGetApp().m_BinDir;
    if( tmp.Last() == '/' )
        tmp.RemoveLast();
    FullPath     = tmp.BeforeLast( '/' ); // cd ..
    FullPath    += wxT( "/doc/help/" );
    LocaleString = wxGetApp().m_Locale->GetCanonicalName();

    wxString path_tmp = FullPath;
#ifdef __WINDOWS__
    path_tmp.MakeLower();
#endif
    if( path_tmp.Contains( wxT( "kicad" ) ) )
    {
        if( wxDirExists( FullPath ) )
            PathFound = TRUE;
    }

    /* find kicad/help/ from environment variable  KICAD */
    if( !PathFound && wxGetApp().m_Env_Defined )
    {
        FullPath = wxGetApp().m_KicadEnv + wxT( "/doc/help/" );
        if( wxDirExists( FullPath ) )
            PathFound = TRUE;
    }

    /* find kicad/help/ from "s_HelpPathList" */
    int ii = 0;
    while( !PathFound )
    {
        FullPath = s_HelpPathList[ii++];
        if( FullPath == wxT( "end_list" ) )
            break;
        if( wxDirExists( FullPath ) )
            PathFound = TRUE;
    }

    if( PathFound )
    {
        LangFullPath = FullPath + LocaleString + UNIX_STRING_DIR_SEP;
        if( wxDirExists( LangFullPath ) )
            return LangFullPath;

        LangFullPath = FullPath + LocaleString.Left( 2 ) + UNIX_STRING_DIR_SEP;
        if( wxDirExists( LangFullPath ) )
            return LangFullPath;

        LangFullPath = FullPath + wxT( "en/" );
        if( wxDirExists( LangFullPath ) )
            return LangFullPath;
        else
        {
            LangFullPath = FullPath + wxT( "fr/" );
            if( wxDirExists( LangFullPath ) )
                return LangFullPath;
        }
        return FullPath;
    }
    return wxEmptyString;
}


/********************************************************/
wxString FindKicadFile( const wxString& shortname )
/********************************************************/

/* Search the executable file shortname in kicad binary path
 *  and return full file name if found or shortname
 *  kicad binary path is
 *  kicad/bin
 *
 *  kicad binary path is found from:
 *  BinDir
 *  or environment variable KICAD
 *  or (default) c:\kicad ou /usr/local/kicad
 *  or default binary path
 */
{
    wxString FullFileName;

    /* test de la presence du fichier shortname dans le repertoire de
     *  des binaires de kicad */
    FullFileName = wxGetApp().m_BinDir + shortname;
    if( wxFileExists( FullFileName ) )
        return FullFileName;

    /* test de la presence du fichier shortname dans le repertoire
     *  defini par la variable d'environnement KICAD */
    if( wxGetApp().m_Env_Defined )
    {
        FullFileName = wxGetApp().m_KicadEnv + shortname;
        if( wxFileExists( FullFileName ) )
            return FullFileName;
    }

    /* find binary file from default path list:
     *  /usr/local/kicad/linux or c:/kicad/winexe
     *  (see s_KicadDataPathList) */
    int ii = 0;
    while( 1 )
    {
        if( s_KicadBinaryPathList[ii] == wxT( "end_list" ) )
            break;
        FullFileName = s_KicadBinaryPathList[ii++] + shortname;
        if( wxFileExists( FullFileName ) )
            return FullFileName;
    }

    return shortname;
}


/* Call the executable file "ExecFile", with params "param"
 */
int ExecuteFile( wxWindow* frame, const wxString& ExecFile,
                 const wxString& param )
{
    wxString FullFileName;


    FullFileName = FindKicadFile( ExecFile );

    if( wxFileExists( FullFileName ) )
    {
        if( !param.IsEmpty() )
            FullFileName += wxT( " " ) + param;
        ProcessExecute( FullFileName );
        return 0;
    }

    wxString msg;
    msg.Printf( _( "Command <%s> could not found" ), GetChars( ExecFile ) );
    DisplayError( frame, msg, 20 );
    return -1;
}


/***********************************/
wxString ReturnKicadDatasPath()
/***********************************/

/* Retourne le chemin des donnees communes de kicad.
 *  Si variable d'environnement KICAD definie (KICAD = chemin pour kicad),
 *      retourne <KICAD>/;
 *  Sinon retourne <Chemin des binaires>/ (si "kicad" est dans le nom du chemin)
 *  Sinon retourne /usr/share/kicad/
 *
 *  Remarque:
 *  Les \ sont remplac�s par / (a la mode Unix)
 */
{
    bool     PathFound = FALSE;
    wxString data_path;

    if( wxGetApp().m_Env_Defined )  // Chemin impose par la variable d'environnement
    {
        data_path = wxGetApp().m_KicadEnv;
        PathFound = TRUE;
    }
    else    // Chemin cherche par le chemin des executables
    {
        // le chemin est bindir../
        wxString tmp = wxGetApp().m_BinDir;
#ifdef __WINDOWS__
        tmp.MakeLower();
#endif
        if( tmp.Contains( wxT( "kicad" ) ) )
        {
#ifdef __WINDOWS__
            tmp = wxGetApp().m_BinDir;
#endif
            if( tmp.Last() == '/' )
                tmp.RemoveLast();
            data_path  = tmp.BeforeLast( '/' ); // id cd ../
            data_path += UNIX_STRING_DIR_SEP;

            // Old versions of kicad use kicad/ as default for data
            // and last versions kicad/share/
            // So we search for kicad/share/ first
            wxString old_path = data_path;
            data_path += wxT( "share/" );
            if( wxDirExists( data_path ) )
                PathFound = TRUE;
            else if( wxDirExists( old_path ) )
            {
                data_path = old_path;
                PathFound = TRUE;
            }
        }
    }

    /* find kicad from default path list:
     *  /usr/local/kicad/ or c:/kicad/
     *  (see s_KicadDataPathList) */
    int ii = 0;
    while( !PathFound )
    {
        if( s_KicadDataPathList[ii] == wxT( "end_list" ) )
            break;
        data_path = s_KicadDataPathList[ii++];
        if( wxDirExists( data_path ) )
            PathFound = TRUE;
    }

    if( PathFound )
    {
        data_path.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
        if( data_path.Last() != '/' )
            data_path += UNIX_STRING_DIR_SEP;
    }
    else
        data_path.Empty();

    return data_path;
}


/*
 * Return the prefered editor name
 */
wxString& WinEDA_App::GetEditorName()
{
    wxString editorname = m_EditorName;

    // We get the prefered editor name from environment variable first.
    if( editorname.IsEmpty() )
    {
        wxGetEnv( wxT( "EDITOR" ), &editorname );
    }
    if( editorname.IsEmpty() ) // We must get a prefered editor name
    {
        DisplayInfoMessage( NULL, _( "No default editor found, you must choose it" ) );
        wxString mask( wxT( "*" ) );

#ifdef __WINDOWS__
        mask += wxT( ".exe" );
#endif
        editorname = EDA_FileSelector( _( "Prefered Editor:" ), wxEmptyString,
                                       wxEmptyString, wxEmptyString, mask,
                                       NULL, wxFD_OPEN, true );
    }

    if( !editorname.IsEmpty() )
    {
        m_EditorName = editorname;
        m_EDA_CommonConfig->Write( wxT( "Editor" ), m_EditorName );
    }

    return m_EditorName;
}


/***********************************/
bool OpenPDF( const wxString& file )
/***********************************/

/** Function OpenPDF
 * run the PDF viewer and display a PDF file
 * @param file = PDF file to open
 * @return true is success, false if no PDF viewer found
 */
{
    wxString command;
    wxString filename = file;
    wxString type;
    bool     success = false;

    wxGetApp().ReadPdfBrowserInfos();
    if( !wxGetApp().m_PdfBrowserIsDefault )    //  Run the prefered PDF Browser
    {
        AddDelimiterString( filename );
        command = wxGetApp().m_PdfBrowser + wxT( " " ) + filename;
    }
    else
    {
        wxFileType* filetype = NULL;
        wxFileType::MessageParameters params( filename, type );
        filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( wxT( "pdf" ) );
        if( filetype )
            success = filetype->GetOpenCommand( &command, params );
        delete filetype;
#ifndef __WINDOWS__

        // Bug ? under linux wxWidgets returns acroread as PDF viewer,even it not exists
        if( command.StartsWith( wxT( "acroread" ) ) ) // Workaround
            success = false;
#endif
        if( success && !command.IsEmpty() )
        {
            success = ProcessExecute( command );
            if( success )
                return success;
        }

        success = false;
        command.Empty();

        if( !success )
        {
#ifndef __WINDOWS__
            AddDelimiterString( filename );
            /* here is a list of PDF viewers candidates */
            const static wxString tries[] =
            {
                wxT( "/usr/bin/evince" ),
                wxT( "/usr/bin/gpdf" ),
                wxT( "/usr/bin/konqueror" ),
                wxT( "/usr/bin/kpdf" ),
                wxT( "/usr/bin/xpdf" ),
                wxT( "" ),
            };

            for( int ii = 0; ; ii++ )
            {
                if( tries[ii].IsEmpty() )
                    break;

                if( wxFileExists( tries[ii] ) )
                {
                    command = tries[ii] + wxT( " " ) + filename;
                    break;
                }
            }

#endif
        }
    }

    if( !command.IsEmpty() )
    {
        success = ProcessExecute( command );
        if( !success )
        {
            wxString msg = _( "Problem while running the PDF viewer" );
            msg << _( "\n command is " ) << command;
            DisplayError( NULL, msg );
        }
    }
    else
    {
        wxString msg = _( "Unable to find a PDF viewer for" );
        msg << wxT( " " ) << filename;
        DisplayError( NULL, msg );
        success = false;
    }

    return success;
}


/*************************************/
void OpenFile( const wxString& file )
/*************************************/
{
    wxString    command;
    wxString    filename = file;

    wxFileName  CurrentFileName( filename );
    wxString    ext, type;

    ext = CurrentFileName.GetExt();
    wxFileType* filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( ext );

    bool        success = false;

    wxFileType::MessageParameters params( filename, type );
    if( filetype )
        success = filetype->GetOpenCommand( &command, params );
    delete filetype;

    if( success && !command.IsEmpty() )
        ProcessExecute( command );
}


wxString QuoteFullPath( wxFileName& fn, wxPathFormat format )
{
    return wxT( "\"" ) + fn.GetFullPath() + wxT( "\"" );
}
