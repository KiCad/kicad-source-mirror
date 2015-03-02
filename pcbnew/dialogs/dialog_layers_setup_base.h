///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_LAYERS_SETUP_BASE_H__
#define __DIALOG_LAYERS_SETUP_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/scrolwin.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CRTYDFRONTNAME 1000
#define ID_CRTYDFRONTCHECKBOX 1001
#define ID_CRTYDFRONTCHOICE 1002
#define ID_FABFRONTNAME 1003
#define ID_FABFRONTCHECKBOX 1004
#define ID_FABFRONTCHOICE 1005
#define ID_ADHESFRONTNAME 1006
#define ID_ADHESFRONTCHECKBOX 1007
#define ID_ADHESFRONTCHOICE 1008
#define ID_SOLDPFRONTNAME 1009
#define ID_SOLDPFRONTCHECKBOX 1010
#define ID_SOLDPFRONTCHOICE 1011
#define ID_SILKSFRONTNAME 1012
#define ID_SILKSFRONTCHECKBOX 1013
#define ID_SILKSFRONTCHOICE 1014
#define ID_MASKFRONTNAME 1015
#define ID_MASKFRONTCHECKBOX 1016
#define ID_MASKFRONTCHOICE 1017
#define ID_FRONTNAME 1018
#define ID_FRONTCHECKBOX 1019
#define ID_FRONTCHOICE 1020
#define ID_IN1NAME 1021
#define ID_IN1CHECKBOX 1022
#define ID_IN1CHOICE 1023
#define ID_IN2NAME 1024
#define ID_IN2CHECKBOX 1025
#define ID_IN2CHOICE 1026
#define ID_IN3NAME 1027
#define ID_IN3CHECKBOX 1028
#define ID_IN3CHOICE 1029
#define ID_IN4NAME 1030
#define ID_IN4CHECKBOX 1031
#define ID_IN4CHOICE 1032
#define ID_IN5NAME 1033
#define ID_IN5CHECKBOX 1034
#define ID_IN5CHOICE 1035
#define ID_IN6NAME 1036
#define ID_IN6CHECKBOX 1037
#define ID_IN6CHOICE 1038
#define ID_IN7NAME 1039
#define ID_IN7CHECKBOX 1040
#define ID_IN7CHOICE 1041
#define ID_IN8NAME 1042
#define ID_IN8CHECKBOX 1043
#define ID_IN8CHOICE 1044
#define ID_IN9NAME 1045
#define ID_IN9CHECKBOX 1046
#define ID_IN9CHOICE 1047
#define ID_IN10NAME 1048
#define ID_IN10CHECKBOX 1049
#define ID_IN10CHOICE 1050
#define ID_IN11NAME 1051
#define ID_IN11CHECKBOX 1052
#define ID_IN11CHOICE 1053
#define ID_IN12NAME 1054
#define ID_IN12CHECKBOX 1055
#define ID_IN12CHOICE 1056
#define ID_IN13NAME 1057
#define ID_IN13CHECKBOX 1058
#define ID_IN13CHOICE 1059
#define ID_IN14NAME 1060
#define ID_IN14CHECKBOX 1061
#define ID_IN14CHOICE 1062
#define ID_IN15NAME 1063
#define ID_IN15CHECKBOX 1064
#define ID_IN15CHOICE 1065
#define ID_IN16NAME 1066
#define ID_IN16CHECKBOX 1067
#define ID_IN16CHOICE 1068
#define ID_IN17NAME 1069
#define ID_IN17CHECKBOX 1070
#define ID_IN17CHOICE 1071
#define ID_IN18NAME 1072
#define ID_IN18CHECKBOX 1073
#define ID_IN18CHOICE 1074
#define ID_IN19NAME 1075
#define ID_IN19CHECKBOX 1076
#define ID_IN19CHOICE 1077
#define ID_IN20NAME 1078
#define ID_IN20CHECKBOX 1079
#define ID_IN20CHOICE 1080
#define ID_IN21NAME 1081
#define ID_IN21CHECKBOX 1082
#define ID_IN21CHOICE 1083
#define ID_IN22NAME 1084
#define ID_IN22CHECKBOX 1085
#define ID_IN22CHOICE 1086
#define ID_IN23NAME 1087
#define ID_IN23CHECKBOX 1088
#define ID_IN24NAME 1089
#define ID_IN24CHECKBOX 1090
#define ID_IN24CHOICE 1091
#define ID_IN25NAME 1092
#define ID_IN25CHECKBOX 1093
#define ID_IN25CHOICE 1094
#define ID_IN26NAME 1095
#define ID_IN26CHECKBOX 1096
#define ID_IN26CHOICE 1097
#define ID_IN27NAME 1098
#define ID_IN27CHECKBOX 1099
#define ID_IN27CHOICE 1100
#define ID_IN28NAME 1101
#define ID_IN28CHECKBOX 1102
#define ID_IN28CHOICE 1103
#define ID_IN29NAME 1104
#define ID_IN29CHECKBOX 1105
#define ID_IN29CHOICE 1106
#define ID_IN30NAME 1107
#define ID_IN30CHECKBOX 1108
#define ID_IN30CHOICE 1109
#define ID_BACKNAME 1110
#define ID_BACKCHECKBOX 1111
#define ID_BACKCHOICE 1112
#define ID_MASKBACKNAME 1113
#define ID_MASKBACKCHECKBOX 1114
#define ID_MASKBACKCHOICE 1115
#define ID_SILKSBACKNAME 1116
#define ID_SILKSBACKCHECKBOX 1117
#define ID_SILKSBACKCHOICE 1118
#define ID_SOLDPBACKNAME 1119
#define ID_SOLDPBACKCHECKBOX 1120
#define ID_SOLDPBACKCHOICE 1121
#define ID_ADHESBACKNAME 1122
#define ID_ADHESBACKCHECKBOX 1123
#define ID_ADHESBACKCHOICE 1124
#define ID_FABBACKNAME 1125
#define ID_FABBACKCHECKBOX 1126
#define ID_FABBACKCHOICE 1127
#define ID_CRTYDBACKNAME 1128
#define ID_CRTYDBACKCHECKBOX 1129
#define ID_CRTYDBACKCHOICE 1130
#define ID_PCBEDGESNAME 1131
#define ID_PCBEDGESCHECKBOX 1132
#define ID_PCBEDGESCHOICE 1133
#define ID_MARGINNAME 1134
#define ID_MARGINCHECKBOX 1135
#define ID_ECO2CHOICE 1136
#define ID_ECO2NAME 1137
#define ID_ECO2CHECKBOX 1138
#define ID_ECO1NAME 1139
#define ID_ECO1CHECKBOX 1140
#define ID_ECO1CHOICE 1141
#define ID_COMMENTSNAME 1142
#define ID_COMMENTSCHECKBOX 1143
#define ID_COMMENTSCHOICE 1144
#define ID_DRAWINGSNAME 1145
#define ID_DRAWINGSCHECKBOX 1146
#define ID_DRAWINGSCHOICE 1147

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LAYERS_SETUP_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LAYERS_SETUP_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticTextGrouping;
		wxChoice* m_PresetsChoice;
		wxStaticText* m_staticTextCopperLayers;
		wxChoice* m_CopperLayersChoice;
		wxStaticText* m_staticTextLayers;
		wxPanel* m_TitlePanel;
		wxScrolledWindow* m_LayersListPanel;
		wxFlexGridSizer* m_LayerListFlexGridSizer;
		wxStaticText* m_CrtYdFrontName;
		wxPanel* m_CrtYdFrontPanel;
		wxCheckBox* m_CrtYdFrontCheckBox;
		wxStaticText* m_CrtYdFrontStaticText;
		wxStaticText* m_FabFrontName;
		wxPanel* m_FabFrontPanel;
		wxCheckBox* m_FabFrontCheckBox;
		wxStaticText* m_FabFrontStaticText;
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
		wxTextCtrl* m_In1Name;
		wxPanel* m_In1Panel;
		wxCheckBox* m_In1CheckBox;
		wxChoice* m_In1Choice;
		wxTextCtrl* m_In2Name;
		wxPanel* m_In2Panel;
		wxCheckBox* m_In2CheckBox;
		wxChoice* m_In2Choice;
		wxTextCtrl* m_In3Name;
		wxPanel* m_In3Panel;
		wxCheckBox* m_In3CheckBox;
		wxChoice* m_In3Choice;
		wxTextCtrl* m_In4Name;
		wxPanel* m_In4Panel;
		wxCheckBox* m_In4CheckBox;
		wxChoice* m_In4Choice;
		wxTextCtrl* m_In5Name;
		wxPanel* m_In5Panel;
		wxCheckBox* m_In5CheckBox;
		wxChoice* m_In5Choice;
		wxTextCtrl* m_In6Name;
		wxPanel* m_In6Panel;
		wxCheckBox* m_In6CheckBox;
		wxChoice* m_In6Choice;
		wxTextCtrl* m_In7Name;
		wxPanel* m_In7Panel;
		wxCheckBox* m_In7CheckBox;
		wxChoice* m_In7Choice;
		wxTextCtrl* m_In8Name;
		wxPanel* m_In8Panel;
		wxCheckBox* m_In8CheckBox;
		wxChoice* m_In8Choice;
		wxTextCtrl* m_In9Name;
		wxPanel* m_In9Panel;
		wxCheckBox* m_In9CheckBox;
		wxChoice* m_In9Choice;
		wxTextCtrl* m_In10Name;
		wxPanel* m_In10Panel;
		wxCheckBox* m_In10CheckBox;
		wxChoice* m_In10Choice;
		wxTextCtrl* m_In11Name;
		wxPanel* m_In11Panel;
		wxCheckBox* m_In11CheckBox;
		wxChoice* m_In11Choice;
		wxTextCtrl* m_In12Name;
		wxPanel* m_In12Panel;
		wxCheckBox* m_In12CheckBox;
		wxChoice* m_In12Choice;
		wxTextCtrl* m_In13Name;
		wxPanel* m_In13Panel;
		wxCheckBox* m_In13CheckBox;
		wxChoice* m_In13Choice;
		wxTextCtrl* m_In14Name;
		wxPanel* m_In14Panel;
		wxCheckBox* m_In14CheckBox;
		wxChoice* m_In14Choice;
		wxTextCtrl* m_In15Name;
		wxPanel* m_In15Panel;
		wxCheckBox* m_In15CheckBox;
		wxChoice* m_In15Choice;
		wxTextCtrl* m_In16Name;
		wxPanel* m_In16Panel;
		wxCheckBox* m_In16CheckBox;
		wxChoice* m_In16Choice;
		wxTextCtrl* m_In17Name;
		wxPanel* m_In17Panel;
		wxCheckBox* m_In17CheckBox;
		wxChoice* m_In17Choice;
		wxTextCtrl* m_In18Name;
		wxPanel* m_In18Panel;
		wxCheckBox* m_In18CheckBox;
		wxChoice* m_In18Choice;
		wxTextCtrl* m_In19Name;
		wxPanel* m_In19Panel;
		wxCheckBox* m_In19CheckBox;
		wxChoice* m_In19Choice;
		wxTextCtrl* m_In20Name;
		wxPanel* m_In20Panel;
		wxCheckBox* m_In20CheckBox;
		wxChoice* m_In20Choice;
		wxTextCtrl* m_In21Name;
		wxPanel* m_In21Panel;
		wxCheckBox* m_In21CheckBox;
		wxChoice* m_In21Choice;
		wxTextCtrl* m_In22Name;
		wxPanel* m_In22Panel;
		wxCheckBox* m_In22CheckBox;
		wxChoice* m_In22Choice;
		wxTextCtrl* m_In23Name;
		wxPanel* m_In23Panel;
		wxCheckBox* m_In23CheckBox;
		wxChoice* m_In23Choice;
		wxTextCtrl* m_In24Name;
		wxPanel* m_In24Panel;
		wxCheckBox* m_In24CheckBox;
		wxChoice* m_In24Choice;
		wxTextCtrl* m_In25Name;
		wxPanel* m_In25Panel;
		wxCheckBox* m_In25CheckBox;
		wxChoice* m_In25Choice;
		wxTextCtrl* m_In26Name;
		wxPanel* m_In26Panel;
		wxCheckBox* m_In26CheckBox;
		wxChoice* m_In26Choice;
		wxTextCtrl* m_In27Name;
		wxPanel* m_In27Panel;
		wxCheckBox* m_In27CheckBox;
		wxChoice* m_In27Choice;
		wxTextCtrl* m_In28Name;
		wxPanel* m_In28Panel;
		wxCheckBox* m_In28CheckBox;
		wxChoice* m_In28Choice;
		wxTextCtrl* m_In29Name;
		wxPanel* m_In29Panel;
		wxCheckBox* m_In29CheckBox;
		wxChoice* m_In29Choice;
		wxTextCtrl* m_In30Name;
		wxPanel* m_In30Panel;
		wxCheckBox* m_In30CheckBox;
		wxChoice* m_In30Choice;
		wxTextCtrl* m_BackName;
		wxPanel* m_BackPanel;
		wxCheckBox* m_BackCheckBox;
		wxChoice* m_BackChoice;
		wxStaticText* m_MaskBackName;
		wxPanel* m_MaskBackPanel;
		wxCheckBox* m_MaskBackCheckBox;
		wxStaticText* m_MaskBackStaticText;
		wxStaticText* m_SilkSBackName;
		wxPanel* m_SilkSBackPanel;
		wxCheckBox* m_SilkSBackCheckBox;
		wxStaticText* m_SilkSBackStaticText;
		wxStaticText* m_SoldPBackName;
		wxPanel* m_SoldPBackPanel;
		wxCheckBox* m_SoldPBackCheckBox;
		wxStaticText* m_SoldPBackStaticText;
		wxStaticText* m_AdhesBackName;
		wxPanel* m_AdhesBackPanel;
		wxCheckBox* m_AdhesBackCheckBox;
		wxStaticText* m_AdhesBackStaticText;
		wxStaticText* m_FabBackName;
		wxPanel* m_FabBackPanel;
		wxCheckBox* m_FabBackCheckBox;
		wxStaticText* m_FabBackStaticText;
		wxStaticText* m_CrtYdBackName;
		wxPanel* m_CrtYdBackPanel;
		wxCheckBox* m_CrtYdBackCheckBox;
		wxStaticText* m_CrtYdBackStaticText;
		wxStaticText* m_PCBEdgesName;
		wxPanel* m_PCBEdgesPanel;
		wxCheckBox* m_PCBEdgesCheckBox;
		wxStaticText* m_PCBEdgesStaticText;
		wxStaticText* m_MarginName;
		wxPanel* m_MarginPanel;
		wxCheckBox* m_MarginCheckBox;
		wxStaticText* m_MarginStaticText;
		wxStaticText* m_Eco1Name;
		wxPanel* m_Eco1Panel;
		wxCheckBox* m_Eco1CheckBox;
		wxStaticText* m_Eco1StaticText;
		wxStaticText* m_Eco2Name;
		wxPanel* m_Eco2Panel;
		wxCheckBox* m_Eco2CheckBox;
		wxStaticText* m_Eco2StaticText;
		wxStaticText* m_CommentsName;
		wxPanel* m_CommentsPanel;
		wxCheckBox* m_CommentsCheckBox;
		wxStaticText* m_CommentsStaticText;
		wxStaticText* m_DrawingsName;
		wxPanel* m_DrawingsPanel;
		wxCheckBox* m_DrawingsCheckBox;
		wxStaticText* m_DrawingsStaticText;
		wxStdDialogButtonSizer* m_sdbSizer2;
		wxButton* m_sdbSizer2OK;
		wxButton* m_sdbSizer2Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnPresetsChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCopperLayersChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void DenyChangeCheckBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkButtonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_LAYERS_SETUP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Layer Setup"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 550,1580 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_LAYERS_SETUP_BASE();
	
};

#endif //__DIALOG_LAYERS_SETUP_BASE_H__
