///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_layers_setup_base__
#define __dialog_layers_setup_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/choice.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/scrolwin.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_ADHESFRONTNAME 1000
#define ID_ADHESFRONTCHECKBOX 1001
#define ID_ADHESFRONTCHOICE 1002
#define ID_SOLDPFRONTNAME 1003
#define ID_SOLDPFRONTCHECKBOX 1004
#define ID_SOLDPFRONTCHOICE 1005
#define ID_SILKSFRONTNAME 1006
#define ID_SILKSFRONTCHECKBOX 1007
#define ID_SILKSFRONTCHOICE 1008
#define ID_MASKFRONTNAME 1009
#define ID_MASKFRONTCHECKBOX 1010
#define ID_MASKFRONTCHOICE 1011
#define ID_FRONTNAME 1012
#define ID_FRONTCHECKBOX 1013
#define ID_FRONTCHOICE 1014
#define ID_INNER2NAME 1015
#define ID_INNER2CHECKBOX 1016
#define ID_INNER2CHOICE 1017
#define ID_INNER3NAME 1018
#define ID_INNER3CHECKBOX 1019
#define ID_INNER3CHOICE 1020
#define ID_INNER4NAME 1021
#define ID_INNER4CHECKBOX 1022
#define ID_INNER4CHOICE 1023
#define ID_INNER5NAME 1024
#define ID_INNER5CHECKBOX 1025
#define ID_INNER5CHOICE 1026
#define ID_INNER6NAME 1027
#define ID_INNER6CHECKBOX 1028
#define ID_INNER6CHOICE 1029
#define ID_INNER7NAME 1030
#define ID_INNER7CHECKBOX 1031
#define ID_INNER7CHOICE 1032
#define ID_INNER8NAME 1033
#define ID_INNER8CHECKBOX 1034
#define ID_INNER8CHOICE 1035
#define ID_INNER9NAME 1036
#define ID_INNER9CHECKBOX 1037
#define ID_INNER9CHOICE 1038
#define ID_INNER10NAME 1039
#define ID_INNER10CHECKBOX 1040
#define ID_INNER10CHOICE 1041
#define ID_INNER11NAME 1042
#define ID_INNER11CHECKBOX 1043
#define ID_INNER11CHOICE 1044
#define ID_INNER12NAME 1045
#define ID_INNER12CHECKBOX 1046
#define ID_INNER12CHOICE 1047
#define ID_INNER13NAME 1048
#define ID_INNER13CHECKBOX 1049
#define ID_INNER13CHOICE 1050
#define ID_INNER14NAME 1051
#define ID_INNER14CHECKBOX 1052
#define ID_INNER14CHOICE 1053
#define ID_INNER15NAME 1054
#define ID_INNER15CHECKBOX 1055
#define ID_INNER15CHOICE 1056
#define ID_BACKNAME 1057
#define ID_BACKCHECKBOX 1058
#define ID_BACKCHOICE 1059
#define ID_MASKBACKNAME 1060
#define ID_MASKBACKCHECKBOX 1061
#define ID_MASKBACKCHOICE 1062
#define ID_SILKSBACKNAME 1063
#define ID_SILKSBACKCHECKBOX 1064
#define ID_SILKSBACKCHOICE 1065
#define ID_SOLDPBACKNAME 1066
#define ID_SOLDPBACKCHECKBOX 1067
#define ID_SOLDPBACKCHOICE 1068
#define ID_ADHESBACKNAME 1069
#define ID_ADHESBACKCHECKBOX 1070
#define ID_ADHESBACKCHOICE 1071
#define ID_PCBEDGESNAME 1072
#define ID_PCBEDGESCHECKBOX 1073
#define ID_PCBEDGESCHOICE 1074
#define ID_ECO2NAME 1075
#define ID_ECO2CHECKBOX 1076
#define ID_ECO2CHOICE 1077
#define ID_ECO1NAME 1078
#define ID_ECO1CHECKBOX 1079
#define ID_ECO1CHOICE 1080
#define ID_COMMENTSNAME 1081
#define ID_COMMENTSCHECKBOX 1082
#define ID_COMMENTSCHOICE 1083
#define ID_DRAWINGSNAME 1084
#define ID_DRAWINGSCHECKBOX 1085
#define ID_DRAWINGSCHOICE 1086

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LAYERS_SETUP_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LAYERS_SETUP_BASE : public wxDialog 
{
	private:
	
	protected:
		wxChoice* m_PresetsChoice;
		wxChoice* m_CopperLayersChoice;
		wxPanel* m_TitlePanel;
		wxScrolledWindow* m_LayersListPanel;
		wxFlexGridSizer* m_LayerListFlexGridSizer;
		wxStaticText* m_AdhesFrontName;
		wxPanel* m_AdhesFrontPanel;
		wxCheckBox* m_AdhesFrontCheckBox;
		wxStaticText* m_AdhesFrontStaticText;
		wxStaticText* m_SoldPFrontName;
		wxPanel* m_SoldPFrontPanel;
		wxCheckBox* m_SoldPFrontCheckBox;
		wxStaticText* m_SoldPFrontStaticText;
		wxStaticText* m_SilkSFrontName;
		wxPanel* m_SilkSFrontPanel;
		wxCheckBox* m_SilkSFrontCheckBox;
		wxStaticText* m_SilkSFrontStaticText;
		wxStaticText* m_MaskFrontName;
		wxPanel* m_MaskFrontPanel;
		wxCheckBox* m_MaskFrontCheckBox;
		wxStaticText* m_MaskFrontStaticText;
		wxTextCtrl* m_FrontName;
		wxPanel* m_FrontPanel;
		wxCheckBox* m_FrontCheckBox;
		wxChoice* m_FrontChoice;
		wxTextCtrl* m_Inner2Name;
		wxPanel* m_Inner2Panel;
		wxCheckBox* m_Inner2CheckBox;
		wxChoice* m_Inner2Choice;
		wxTextCtrl* m_Inner3Name;
		wxPanel* m_Inner3Panel;
		wxCheckBox* m_Inner3CheckBox;
		wxChoice* m_Inner3Choice;
		wxTextCtrl* m_Inner4Name;
		wxPanel* m_Inner4Panel;
		wxCheckBox* m_Inner4CheckBox;
		wxChoice* m_Inner4Choice;
		wxTextCtrl* m_Inner5Name;
		wxPanel* m_Inner5Panel;
		wxCheckBox* m_Inner5CheckBox;
		wxChoice* m_Inner5Choice;
		wxTextCtrl* m_Inner6Name;
		wxPanel* m_Inner6Panel;
		wxCheckBox* m_Inner6CheckBox;
		wxChoice* m_Inner6Choice;
		wxTextCtrl* m_Inner7Name;
		wxPanel* m_Inner7Panel;
		wxCheckBox* m_Inner7CheckBox;
		wxChoice* m_Inner7Choice;
		wxTextCtrl* m_Inner8Name;
		wxPanel* m_Inner8Panel;
		wxCheckBox* m_Inner8CheckBox;
		wxChoice* m_Inner8Choice;
		wxTextCtrl* m_Inner9Name;
		wxPanel* m_Inner9Panel;
		wxCheckBox* m_Inner9CheckBox;
		wxChoice* m_Inner9Choice;
		wxTextCtrl* m_Inner10Name;
		wxPanel* m_Inner10Panel;
		wxCheckBox* m_Inner10CheckBox;
		wxChoice* m_Inner10Choice;
		wxTextCtrl* m_Inner11Name;
		wxPanel* m_Inner11Panel;
		wxCheckBox* m_Inner11CheckBox;
		wxChoice* m_Inner11Choice;
		wxTextCtrl* m_Inner12Name;
		wxPanel* m_Inner12Panel;
		wxCheckBox* m_Inner12CheckBox;
		wxChoice* m_Inner12Choice;
		wxTextCtrl* m_Inner13Name;
		wxPanel* m_Inner13Panel;
		wxCheckBox* m_Inner13CheckBox;
		wxChoice* m_Inner13Choice;
		wxTextCtrl* m_Inner14Name;
		wxPanel* m_Inner14Panel;
		wxCheckBox* m_Inner14CheckBox;
		wxChoice* m_Inner14Choice;
		wxTextCtrl* m_Inner15Name;
		wxPanel* m_Inner15Panel;
		wxCheckBox* m_Inner15CheckBox;
		wxChoice* m_Inner15Choice;
		wxTextCtrl* m_BackName;
		wxPanel* m_BackPanel;
		wxCheckBox* m_BackCheckBox;
		wxChoice* m_BackChoice;
		wxStaticText* m_MaskBackName;
		wxPanel* m_panel21;
		wxCheckBox* m_MaskBackCheckBox;
		wxStaticText* m_MaskBackStaticText;
		wxStaticText* m_SilkSBackName;
		wxPanel* m_panel22;
		wxCheckBox* m_SilkSBackCheckBox;
		wxStaticText* m_SilkSBackStaticText;
		wxStaticText* m_SoldPBackName;
		wxPanel* m_panel23;
		wxCheckBox* m_SoldPBackCheckBox;
		wxStaticText* m_SoldPBackStaticText;
		wxStaticText* m_AdhesBackName;
		wxPanel* m_panel24;
		wxCheckBox* m_AdhesBackCheckBox;
		wxStaticText* m_AdhesBackStaticText;
		wxStaticText* m_PCBEdgesName;
		wxPanel* m_panel25;
		wxCheckBox* m_PCBEdgesCheckBox;
		wxStaticText* m_PCBEdgesStaticText;
		wxStaticText* m_Eco2Name;
		wxPanel* m_panel26;
		wxCheckBox* m_Eco2CheckBox;
		wxStaticText* m_Eco2StaticText;
		wxStaticText* m_Eco1Name;
		wxPanel* m_panel27;
		wxCheckBox* m_Eco1CheckBox;
		wxStaticText* m_Eco1StaticText;
		wxStaticText* m_CommentsName;
		wxPanel* m_panel28;
		wxCheckBox* m_CommentsCheckBox;
		wxStaticText* m_CommentsStaticText;
		wxStaticText* m_DrawingsName;
		wxPanel* m_panel29;
		wxCheckBox* m_DrawingsCheckBox;
		wxStaticText* m_DrawingsStaticText;
		wxStdDialogButtonSizer* m_sdbSizer2;
		wxButton* m_sdbSizer2OK;
		wxButton* m_sdbSizer2Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnPresetsChoice( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCopperLayersChoice( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCheckBox( wxCommandEvent& event ){ event.Skip(); }
		virtual void DenyChangeCheckBox( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkButtonClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_LAYERS_SETUP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Layer Setup"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_LAYERS_SETUP_BASE();
	
};

#endif //__dialog_layers_setup_base__
