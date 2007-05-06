	/***********/
	/* kicad.h */
	/***********/

#ifndef KICAD_H
#define KICAD_H

#include <wx/treectrl.h>

/* Message de presentation */
eda_global wxString g_Main_Title
#ifdef MAIN
 = wxT("KiCad")
#endif
;

#define WinEDA_CommandFrame wxSashLayoutWindow

class WinEDA_TreePrj;
class WinEDA_PrjFrame;

	/*******************************************/
	/* classe pour la Fenetre generale de kicad*/
	/*******************************************/

class WinEDA_MainFrame: public WinEDA_BasicFrame
{
public:

	WinEDA_CommandFrame * m_CommandWin;
	WinEDA_PrjFrame * m_LeftWin;
	wxSashLayoutWindow * m_BottomWin;
	wxTextCtrl * m_DialogWin;
	WinEDA_Toolbar * m_VToolBar;	// Toolbar Vertical bord d'ecran
	wxString m_PrjFileName;
	int m_LeftWin_Width;
	int m_CommandWin_Height;

private:
	wxMenu * m_FilesMenu;

public:
	// Constructor and destructor
	WinEDA_MainFrame(WinEDA_App * eda_app, wxWindow *parent, const wxString & title,
					const wxPoint& pos, const wxSize& size);

	~WinEDA_MainFrame(void);

	void OnCloseWindow(wxCloseEvent & Event);
	void OnSize(wxSizeEvent& event);
	void OnPaint(wxPaintEvent & event);
	void ReDraw(wxDC * DC);
	void OnSashDrag(wxSashEvent& event);
	void Load_Prj_Config(void);
	void Save_Prj_Config(void);
	void Process_Fct(wxCommandEvent& event);
	void Process_Files(wxCommandEvent& event);
	void Process_Config(wxCommandEvent& event);
	void Process_Special_Functions(wxCommandEvent& event);
	void Process_Preferences(wxCommandEvent& event);
	void ReCreateMenuBar(void);
	void RecreateBaseHToolbar(void);
	void CreateCommandToolbar(void);
	void PrintMsg(const wxString & text);
	void SetLanguage(wxCommandEvent& event);

	void CreateZipArchive(const wxString FullFileName);
	void UnZipArchive(const wxString FullFileName);

	DECLARE_EVENT_TABLE()
};

/* Fenetre d'affichage des fichiers du projet */
class WinEDA_PrjFrame : public wxSashLayoutWindow
{
public:
	WinEDA_MainFrame * m_Parent;
	WinEDA_TreePrj * m_TreeProject;

public:
	WinEDA_PrjFrame(WinEDA_MainFrame * parent,
				const wxPoint & pos, const wxSize & size );
	~WinEDA_PrjFrame(void) {}
	void OnSelect(wxTreeEvent & Event);
	void ReCreateTreePrj(void);
	DECLARE_EVENT_TABLE()
};


/** Classe TreeCtrl des fichiers projets **/
class WinEDA_TreePrj : public wxTreeCtrl
{
private:
	WinEDA_PrjFrame * m_Parent;
	wxImageList * m_ImageList;

public:
	WinEDA_TreePrj(WinEDA_PrjFrame *parent);
	~WinEDA_TreePrj();
};

eda_global wxString g_SchematicRootFileName;
eda_global wxString g_BoardFileName;

#ifdef MAIN
wxString g_SchExtBuffer(wxT(".sch"));
wxString g_BoardExtBuffer(wxT(".brd"));
wxString g_NetlistExtBuffer(wxT(".net"));
wxString g_GerberExtBuffer(wxT(".pho"));
#else
eda_global wxString g_SchExtBuffer;
eda_global wxString g_BoardExtBuffer;
eda_global wxString g_NetlistExtBuffer;
eda_global wxString g_GerberExtBuffer;
#endif

#endif
