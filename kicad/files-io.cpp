	/****************/
	/* files-io.cpp */
	/****************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include <wx/fs_zip.h>
#include <wx/docview.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>

#include "common.h"

#include "bitmaps.h"
#include "protos.h"

#include "id.h"

#include "kicad.h"
#include "prjconfig.h"

#define ZIP_EXT wxT(".zip")
#define ZIP_MASK wxT("*.zip")

static void Create_NewPrj_Config( const wxString PrjFullFileName);

/***********************************************************/
void WinEDA_MainFrame::Process_Files(wxCommandEvent& event)
/***********************************************************/
/* Gestion generale  des commandes de sauvegarde
*/
{
int id = event.GetId();
wxString path = wxGetCwd();
wxString fullfilename;
bool IsNew = FALSE;
	
	switch (id)
		{
		case ID_SAVE_PROJECT: /* Update project File */
			Save_Prj_Config();
 			break;

		case ID_LOAD_FILE_1:
		case ID_LOAD_FILE_2:
		case ID_LOAD_FILE_3:
		case ID_LOAD_FILE_4:
		case ID_LOAD_FILE_5:
		case ID_LOAD_FILE_6:
		case ID_LOAD_FILE_7:
		case ID_LOAD_FILE_8:
		case ID_LOAD_FILE_9:
		case ID_LOAD_FILE_10:
			m_PrjFileName = GetLastProject( id - ID_LOAD_FILE_1);
			SetLastProject(m_PrjFileName);
			ReCreateMenuBar();
			Load_Prj_Config();
			break;

		case ID_NEW_PROJECT:
			IsNew = TRUE;
		case ID_LOAD_PROJECT:
			SetLastProject(m_PrjFileName);
			fullfilename = EDA_FileSelector( IsNew ? _("Create Project files:") : _("Load Project files:"),
					path,		  		/* Chemin par defaut */
					wxEmptyString,					/* nom fichier par defaut */
					g_Prj_Config_Filename_ext,	/* extension par defaut */
					wxT("*") + g_Prj_Config_Filename_ext, /* Masque d'affichage */
					this,
					IsNew ? wxFD_SAVE : wxFD_OPEN,
					FALSE
					);
			if ( fullfilename.IsEmpty() ) break;
				
			ChangeFileNameExt(fullfilename, g_Prj_Config_Filename_ext);
			m_PrjFileName = fullfilename;
			if ( IsNew ) Create_NewPrj_Config( m_PrjFileName);
			SetLastProject(m_PrjFileName);
			Load_Prj_Config();
			break;


		case ID_SAVE_AND_ZIP_FILES:
			CreateZipArchive(wxEmptyString);
			break;
			
		case ID_READ_ZIP_ARCHIVE:
			UnZipArchive(wxEmptyString);
			break;
		
        default: DisplayError(this, wxT("WinEDA_MainFrame::Process_Files error"));
			break;
		}
}


/**************************************************************/
static void Create_NewPrj_Config(const wxString PrjFullFileName)
/**************************************************************/
/* Cree un nouveau fichier projet a partir du modele
*/
{
wxString msg;
	// Init default config filename
	g_Prj_Config_LocalFilename.Empty();
	g_Prj_Default_Config_FullFilename =
				ReturnKicadDatasPath() +
				wxT("template/kicad") +
				g_Prj_Config_Filename_ext;

	if ( ! wxFileExists(g_Prj_Default_Config_FullFilename) )
	{
		msg = _("Template file non found ") + g_Prj_Default_Config_FullFilename;
		DisplayInfo(NULL, msg);
	}
	
	else
	{
		if ( wxFileExists(PrjFullFileName) )
		{
			msg = _("File ") + PrjFullFileName
				+ _(" exists! OK to continue?");
			if ( IsOK(NULL, msg) )
			{
			wxCopyFile(g_Prj_Default_Config_FullFilename,
							PrjFullFileName);
			}
		}
	}

	g_SchematicRootFileName = wxFileNameFromPath(PrjFullFileName);
	ChangeFileNameExt(g_SchematicRootFileName, g_SchExtBuffer);
	g_BoardFileName = wxFileNameFromPath(PrjFullFileName);
	ChangeFileNameExt(g_BoardFileName, g_BoardExtBuffer);
	
	EDA_Appl->WriteProjectConfig(PrjFullFileName, wxT("/general"), CfgParamList);
}


/**********************************************************************/
void WinEDA_MainFrame::UnZipArchive(const wxString FullFileName)
/**********************************************************************/
/* Lit un fichier archive .zip et le decompresse dans le repertoire courant
*/
{
wxString filename = FullFileName;
wxString msg;
wxString old_cwd = wxGetCwd();
	
	if ( filename.IsEmpty() )
		filename = EDA_FileSelector( _("Unzip Project:"),
					wxEmptyString,		  		/* Chemin par defaut */
					wxEmptyString,				/* nom fichier par defaut */
					ZIP_EXT,		/* extension par defaut */
					ZIP_MASK,		/* Masque d'affichage */
					this,
					wxFD_OPEN,
					TRUE
					);
	if ( filename.IsEmpty() ) return;

	msg = _("\nOpen ") + filename + wxT("\n");
	PrintMsg( msg );

wxString target_dirname = wxDirSelector ( _("Target Directory"),
				wxEmptyString, 0, wxDefaultPosition, this);
	if ( target_dirname.IsEmpty() ) return;
		
	wxSetWorkingDirectory(target_dirname);
	msg = _("Unzip in ") + target_dirname + wxT("\n");
	PrintMsg( msg );
	
wxFileSystem zipfilesys;
	zipfilesys.AddHandler(new wxZipFSHandler);
	filename += wxT("#zip:");
	zipfilesys.ChangePathTo(filename);
	
wxFSFile * zipfile = NULL;
wxString localfilename	= zipfilesys.FindFirst( wxT("*.*") );
	
	while ( !localfilename.IsEmpty() )
	{
		zipfile = zipfilesys.OpenFile(localfilename);
		if (zipfile == NULL)
		{
			DisplayError(this, wxT("Zip file read error"));
			break;
		}
		wxString unzipfilename = localfilename.AfterLast(':');
		msg = _("Extract file ") + unzipfilename;
		PrintMsg(msg);
		wxInputStream * stream = zipfile->GetStream();
		wxFFileOutputStream * ofile = new wxFFileOutputStream(unzipfilename);
		if ( ofile->Ok() )
		{
			ofile->Write(*stream);
			PrintMsg( _(" OK\n") );
		}
		else PrintMsg( _(" *ERROR*\n") );

		delete ofile;	
		delete zipfile;
		localfilename = zipfilesys.FindNext();
	}
	PrintMsg( wxT("** end **\n") );
	
	wxSetWorkingDirectory(old_cwd);
}

/********************************************************************/
void WinEDA_MainFrame::CreateZipArchive(const wxString FullFileName)
/********************************************************************/
{
wxString filename = FullFileName;
wxString zip_file_fullname;
wxString msg;
wxString curr_path = wxGetCwd();
	
	if ( filename.IsEmpty() )
	{
		filename = m_PrjFileName;
		ChangeFileNameExt(filename, wxT(".zip"));
		filename = EDA_FileSelector( _("Archive Project files:"),
					wxEmptyString, 		/* Chemin par defaut */
					filename,		/* nom fichier par defaut */
					ZIP_EXT,		/* extension par defaut */
					ZIP_MASK,		/* Masque d'affichage */
					this,
					wxFD_SAVE,
					FALSE
					);
	}
	if ( filename.IsEmpty() ) return;


wxFileName zip_name(filename);
	zip_file_fullname = zip_name.GetFullName();
	AddDelimiterString( zip_file_fullname );

wxChar * Ext_to_arch[] = {    /* Liste des extensions des fichiers à sauver */
	wxT("*.sch"), wxT("*.lib"), wxT("*.cmp"), wxT("*.brd"),
	wxT("*.net"), wxT("*.pro"), wxT("*.pho"), wxT("*.py"),
	wxT("*.pdf"), wxT("*.txt"),
	NULL};
int ii = 0;
wxString zip_cmd = wxT("-O ") + zip_file_fullname;
	filename = wxFindFirstFile(Ext_to_arch[ii]);
	while ( !filename.IsEmpty() )
	{
	wxFileName name(filename);
	wxString fullname = name.GetFullName();
		AddDelimiterString( fullname );
		zip_cmd += wxT(" ") + fullname;
		msg = _("Compress file ") + fullname + wxT("\n");
		PrintMsg(msg);

		filename = wxFindNextFile();
		while (filename.IsEmpty() )
		{
			ii++;
			if (Ext_to_arch[ii]) filename = wxFindFirstFile(Ext_to_arch[ii]);
			else break;
		}			
	}

#ifdef __WINDOWS__
#define ZIPPER wxT("minizip.exe")
#else
#define ZIPPER wxT("minizip")
#endif	
	if ( ExecuteFile(this, ZIPPER, zip_cmd) >= 0 )
	{
		msg = _("\nCreate Zip Archive ") + zip_file_fullname;
		PrintMsg( msg );
		PrintMsg( wxT("\n** end **\n") );
	}
	else PrintMsg( wxT("Minizip command error, abort\n") );
	
	wxSetWorkingDirectory(curr_path);
}
