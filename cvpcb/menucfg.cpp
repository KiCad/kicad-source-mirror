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


/* Routines Locales */


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

	switch( g_NetType )
	{
		case TYPE_NON_SPECIFIE:
		case TYPE_ORCADPCB2:
			m_NetFormatBox->SetSelection(0);
			break;

		case TYPE_PCAD:
			break;

		case TYPE_VIEWLOGIC_WIR:
			m_NetFormatBox->SetSelection(1);
			break;

		case TYPE_VIEWLOGIC_NET:
			m_NetFormatBox->SetSelection(2);
			break;

		default:
			break;
	}

	m_LibDirCtrl = new WinEDA_EnterText(this,
				_("Lib Dir:"), g_UserLibDirBuffer,
				m_RightBoxSizer, wxDefaultSize);

	m_NetInputExtCtrl = new WinEDA_EnterText(this,
				_("Net Input Ext:"),NetInExtBuffer,
				m_NetExtBoxSizer, wxDefaultSize);

	m_PkgExtCtrl = new WinEDA_EnterText(this,
				_("Pkg Ext:"), PkgInExtBuffer,
				m_PkgExtBoxSizer, wxDefaultSize);
				
	wxString DocModuleFileName =
		EDA_Appl->m_EDA_CommonConfig->Read( wxT("module_doc_file"), wxT("pcbnew/footprints.pdf"));
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
	PkgInExtBuffer = m_PkgExtCtrl->GetValue();
	EDA_Appl->m_EDA_CommonConfig->Write( wxT("module_doc_file"),
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
	FullFileName = EDA_FileSelector( _("Libraries"),
					g_RealLibDirBuffer,		/* Chemin par defaut */
					wxEmptyString,					/* nom fichier par defaut */
					LibExtBuffer,		/* extension par defaut */
					mask,				/* Masque d'affichage */
					this,
					0,
					TRUE				/* ne chage pas de repertoire courant */
					);
	if (FullFileName == wxEmptyString ) return;

	ShortLibName = MakeReducedFileName(FullFileName,g_RealLibDirBuffer,LibExtBuffer);

	g_LibName_List.Insert(ShortLibName, ii);
	
	g_UserLibDirBuffer = m_LibDirCtrl->GetValue();
	SetRealLibraryPath( wxT("modules") );
	listlib();
	ListModIsModified = 1;

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
	FullFileName = EDA_FileSelector( _("Equiv"),
					g_RealLibDirBuffer,		/* Chemin par defaut */
					wxEmptyString,					/* nom fichier par defaut */
					g_EquivExtBuffer,		/* extension par defaut */
					mask,				/* Masque d'affichage */
					this,
					0,
					TRUE				/* ne chage pas de repertoire courant */
					);

	if (FullFileName == wxEmptyString ) return;

	ShortLibName = MakeReducedFileName(FullFileName,g_RealLibDirBuffer,g_EquivExtBuffer);

	g_ListName_Equ.Insert(ShortLibName, ii);

	/* Mise a jour de l'affichage */
	g_UserLibDirBuffer = m_LibDirCtrl->GetValue();
	SetRealLibraryPath( wxT("modules") );
	listlib();
	
	m_ListEquiv->Clear();
	m_ListEquiv->InsertItems(g_ListName_Equ, 0);
}




/*****************************************************************/
void KiConfigCvpcbFrame::ReturnNetFormat(wxCommandEvent& event)
/*****************************************************************/
{
int ii;

	ii = m_NetFormatBox->GetSelection();
	g_NetType = TYPE_ORCADPCB2;
	if ( ii == 1 ) g_NetType = TYPE_VIEWLOGIC_WIR;
	if ( ii == 2 ) g_NetType = TYPE_VIEWLOGIC_NET;

}

