	/***************************************/
	/** menucfg : configuration de CVPCB  **/
	/***************************************/

/* cree et/ou affiche et modifie la configuration de CVPCB */

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "fctsys.h"
#include "common.h"

#include "cvpcb.h"
#include "protos.h"


	/*****************************************/
	/* classe pour la frame de Configuration */
	/*****************************************/
#include "dialog_cvpcb_config.cpp"



/***************************************************/
void WinEDA_CvpcbFrame::CreateConfigWindow()
/***************************************************/
/* Creation de la fenetre de configuration de CVPCB */
{
KiConfigCvpcbFrame * ConfigFrame = new KiConfigCvpcbFrame(this);
	ConfigFrame->ShowModal(); ConfigFrame->Destroy();
}


/*********************************************/
void KiConfigCvpcbFrame::SetDialogDatas()
/*********************************************/
{
	m_ListLibr->InsertItems(g_LibName_List,0);
	m_ListEquiv->InsertItems(g_ListName_Equ,0);

	m_LibDirCtrl = new WinEDA_EnterText(this,
				_("Lib Dir:"), g_UserLibDirBuffer,
				m_RightBoxSizer, wxDefaultSize);

	m_NetInputExtCtrl = new WinEDA_EnterText(this,
				_("Net Input Ext:"),NetInExtBuffer,
				m_NetExtBoxSizer, wxDefaultSize);

	wxString DocModuleFileName =
		g_EDA_Appl->m_EDA_CommonConfig->Read( DOC_FOOTPRINTS_LIST_KEY, DEFAULT_FOOTPRINTS_LIST_FILENAME);
	m_TextHelpModulesFileName = new WinEDA_EnterText(this,
				_("Module Doc File:"),  DocModuleFileName,
				m_RightBoxSizer, wxDefaultSize);

	/* Create info on Files ext */
	wxStaticText * StaticText;
	wxString text;
	text.Printf( wxT("%s     %s"), _("Cmp ext:"), g_ExtCmpBuffer.GetData() );
	StaticText = new wxStaticText(this, -1,text);
	m_FileExtList->Add(StaticText, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE);

	text.Printf( wxT("%s      %s"), _("Lib ext:"), LibExtBuffer.GetData());
	StaticText = new wxStaticText(this, -1,text);
	m_FileExtList->Add(StaticText, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE);

	text.Printf( wxT("%s %s"), _("NetOut ext:"), NetExtBuffer.GetData());
	StaticText = new wxStaticText(this, -1,text);
	m_FileExtList->Add(StaticText, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE);

	text.Printf( wxT("%s  %s"), _("Equiv ext:"), g_EquivExtBuffer.GetData());
	StaticText = new wxStaticText(this, -1,text);
	m_FileExtList->Add(StaticText, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE);

	text.Printf( wxT("%s  %s"), _("Retro ext:"), ExtRetroBuffer.GetData());
	StaticText = new wxStaticText(this, -1,text);
	m_FileExtList->Add(StaticText, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE);
}

/********************************************************/
void KiConfigCvpcbFrame::AcceptCfg(wxCommandEvent& event)
/********************************************************/
{
	Update();
	Close();
}

/**********************************/
void KiConfigCvpcbFrame::Update()
/**********************************/
{
wxString msg;

	if ( ! m_DoUpdate ) return;
	NetInExtBuffer = m_NetInputExtCtrl->GetValue();
	g_EDA_Appl->m_EDA_CommonConfig->Write( DOC_FOOTPRINTS_LIST_KEY,
			m_TextHelpModulesFileName->GetValue());

	msg = m_LibDirCtrl->GetValue();
	if ( msg != g_UserLibDirBuffer )
	{
		g_UserLibDirBuffer = m_LibDirCtrl->GetValue();
		SetRealLibraryPath( wxT("modules") );
		listlib();
		ListModIsModified = 1;
		m_Parent->BuildFootprintListBox();
	}
}


/****************************************************/
void KiConfigCvpcbFrame::SaveCfg(wxCommandEvent& event)
/****************************************************/
{
	Update();
	Save_Config(this);
}

/******************************************************/
void KiConfigCvpcbFrame::ReadOldCfg(wxCommandEvent& event)
/******************************************************/
{
wxString line;

	NetInNameBuffer.Replace(WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP);

wxString FullFileName = NetInNameBuffer.AfterLast('/');

	ChangeFileNameExt( FullFileName, g_Prj_Config_Filename_ext );

	FullFileName = EDA_FileSelector(_("Read config file"),
					wxGetCwd(),					/* Chemin par defaut */
					FullFileName,		/* nom fichier par defaut */
					g_Prj_Config_Filename_ext,				/* extension par defaut */
					FullFileName,				/* Masque d'affichage */
					this,
					wxFD_OPEN,
					TRUE				/* ne change pas de repertoire courant */
					);
	if ( FullFileName.IsEmpty() ) return;
	if ( ! wxFileExists(FullFileName) )
	{
		line.Printf( _("File %s not found"), FullFileName.GetData());
		DisplayError(this, line); return;
	}

	Read_Config( FullFileName );
	m_DoUpdate = FALSE;
	Close(TRUE);
}


/*******************************************************/
void KiConfigCvpcbFrame::LibDelFct(wxCommandEvent& event)
/*******************************************************/
{
int ii;

	ii = m_ListLibr->GetSelection();
	if ( ii < 0 ) return;

	ListModIsModified = 1;
	g_LibName_List.RemoveAt(ii);

	/* suppression de la reference dans la liste des librairies */
	m_ListLibr->Delete(ii);

	g_UserLibDirBuffer = m_LibDirCtrl->GetValue();
	SetRealLibraryPath( wxT("modules") );
	listlib();

	m_Parent->BuildFootprintListBox();

}

/********************************************************/
void KiConfigCvpcbFrame::LibAddFct(wxCommandEvent& event)
/********************************************************/
{
int ii;
wxString FullFileName, ShortLibName, mask;

	ii = m_ListLibr->GetSelection();
	if ( event.GetId() == ADD_LIB )	/* Ajout apres selection */
		{
		ii ++;
		}
	if ( ii < 0 ) ii = 0;

	Update();
	mask = wxT("*") + LibExtBuffer;

	wxFileDialog FilesDialog(this, _("Library Files:"), g_RealLibDirBuffer,
		wxEmptyString, mask,
		wxFD_DEFAULT_STYLE | wxFD_MULTIPLE);

	FilesDialog.ShowModal();
	wxArrayString Filenames;
	FilesDialog.GetPaths(Filenames);

	if ( Filenames.GetCount() == 0 )
		return;

	for ( unsigned jj = 0; jj < Filenames.GetCount(); jj ++ )
	{
		FullFileName = Filenames[jj];
		ShortLibName = MakeReducedFileName(FullFileName,g_RealLibDirBuffer,LibExtBuffer);

		//Add or insert new library name
		if ( g_LibName_List.Index(ShortLibName) == wxNOT_FOUND)
		{
			ListModIsModified = 1;
			g_LibName_List.Insert(ShortLibName, ii++);
		}
		else
		{
			wxString msg;
			msg << wxT("<") << ShortLibName << wxT("> : ") << _("Library already in use");
			DisplayError(this, msg);
		}
	}

	g_UserLibDirBuffer = m_LibDirCtrl->GetValue();
	SetRealLibraryPath( wxT("modules") );
	listlib();

	m_Parent->BuildFootprintListBox();

	m_ListLibr->Clear();
	m_ListLibr->InsertItems(g_LibName_List, 0);

}


/********************************************************/
void KiConfigCvpcbFrame::EquDelFct(wxCommandEvent& event)
/********************************************************/
{
int ii;

	ii = m_ListEquiv->GetSelection();
	if ( ii < 0 ) return;

	g_ListName_Equ.RemoveAt(ii);
	m_ListEquiv->Delete(ii);
}

/********************************************************/
void KiConfigCvpcbFrame::EquAddFct(wxCommandEvent& event)
/********************************************************/
{
int ii;
wxString FullFileName, ShortLibName, mask;

	ii = m_ListEquiv->GetSelection();
	if ( event.GetId() == ADD_EQU ) ii ++;	/* Ajout apres selection */
	if ( ii < 0 ) ii = 0;

	Update();
	mask = wxT("*") + g_EquivExtBuffer;

	wxFileDialog FilesDialog(this, _("Equiv Files:"), g_RealLibDirBuffer,
		wxEmptyString, mask,
		wxFD_DEFAULT_STYLE | wxFD_MULTIPLE);

	FilesDialog.ShowModal();
	wxArrayString Filenames;
	FilesDialog.GetFilenames(Filenames);

	if ( Filenames.GetCount() == 0 )
		return;

	for ( unsigned jj = 0; jj < Filenames.GetCount(); jj ++ )
	{
		FullFileName = Filenames[jj];
		ShortLibName = MakeReducedFileName(FullFileName,g_RealLibDirBuffer,g_EquivExtBuffer);

		//Add or insert new equiv library name
		if ( g_ListName_Equ.Index(ShortLibName) == wxNOT_FOUND)
		{
			g_ListName_Equ.Insert(ShortLibName, ii++);
		}
		else
		{
			wxString msg;
			msg << wxT("<") << ShortLibName << wxT("> : ") << _("Library already in use");
			DisplayError(this, msg);
		}
	}

	/* Update display list */
	g_UserLibDirBuffer = m_LibDirCtrl->GetValue();
	SetRealLibraryPath( wxT("modules") );
	listlib();

	m_ListEquiv->Clear();
	m_ListEquiv->InsertItems(g_ListName_Equ, 0);
}


