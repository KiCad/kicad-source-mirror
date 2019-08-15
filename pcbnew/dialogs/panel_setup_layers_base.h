///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/choice.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CRTYDFRONTCHECKBOX 1000
#define ID_CRTYDFRONTNAME 1001
#define ID_CRTYDFRONTCHOICE 1002
#define ID_FABFRONTCHECKBOX 1003
#define ID_FABFRONTNAME 1004
#define ID_FABFRONTCHOICE 1005
#define ID_ADHESFRONTCHECKBOX 1006
#define ID_ADHESFRONTNAME 1007
#define ID_ADHESFRONTCHOICE 1008
#define ID_SOLDPFRONTCHECKBOX 1009
#define ID_SOLDPFRONTNAME 1010
#define ID_SOLDPFRONTCHOICE 1011
#define ID_SILKSFRONTCHECKBOX 1012
#define ID_SILKSFRONTNAME 1013
#define ID_SILKSFRONTCHOICE 1014
#define ID_MASKFRONTCHECKBOX 1015
#define ID_MASKFRONTNAME 1016
#define ID_MASKFRONTCHOICE 1017
#define ID_FRONTCHECKBOX 1018
#define ID_FRONTNAME 1019
#define ID_FRONTCHOICE 1020
#define ID_IN1CHECKBOX 1021
#define ID_IN1NAME 1022
#define ID_IN1CHOICE 1023
#define ID_IN2CHECKBOX 1024
#define ID_IN2NAME 1025
#define ID_IN2CHOICE 1026
#define ID_IN3CHECKBOX 1027
#define ID_IN3NAME 1028
#define ID_IN3CHOICE 1029
#define ID_IN4CHECKBOX 1030
#define ID_IN4NAME 1031
#define ID_IN4CHOICE 1032
#define ID_IN5CHECKBOX 1033
#define ID_IN5NAME 1034
#define ID_IN5CHOICE 1035
#define ID_IN6CHECKBOX 1036
#define ID_IN6NAME 1037
#define ID_IN6CHOICE 1038
#define ID_IN7CHECKBOX 1039
#define ID_IN7NAME 1040
#define ID_IN7CHOICE 1041
#define ID_IN8CHECKBOX 1042
#define ID_IN8NAME 1043
#define ID_IN8CHOICE 1044
#define ID_IN9CHECKBOX 1045
#define ID_IN9NAME 1046
#define ID_IN9CHOICE 1047
#define ID_IN10CHECKBOX 1048
#define ID_IN10NAME 1049
#define ID_IN10CHOICE 1050
#define ID_IN11CHECKBOX 1051
#define ID_IN11NAME 1052
#define ID_IN11CHOICE 1053
#define ID_IN12CHECKBOX 1054
#define ID_IN12NAME 1055
#define ID_IN12CHOICE 1056
#define ID_IN13CHECKBOX 1057
#define ID_IN13NAME 1058
#define ID_IN13CHOICE 1059
#define ID_IN14CHECKBOX 1060
#define ID_IN14NAME 1061
#define ID_IN14CHOICE 1062
#define ID_IN15CHECKBOX 1063
#define ID_IN15NAME 1064
#define ID_IN15CHOICE 1065
#define ID_IN16CHECKBOX 1066
#define ID_IN16NAME 1067
#define ID_IN16CHOICE 1068
#define ID_IN17CHECKBOX 1069
#define ID_IN17NAME 1070
#define ID_IN17CHOICE 1071
#define ID_IN18CHECKBOX 1072
#define ID_IN18NAME 1073
#define ID_IN18CHOICE 1074
#define ID_IN19CHECKBOX 1075
#define ID_IN19NAME 1076
#define ID_IN19CHOICE 1077
#define ID_IN20CHECKBOX 1078
#define ID_IN20NAME 1079
#define ID_IN20CHOICE 1080
#define ID_IN21CHECKBOX 1081
#define ID_IN21NAME 1082
#define ID_IN21CHOICE 1083
#define ID_IN22CHECKBOX 1084
#define ID_IN22NAME 1085
#define ID_IN22CHOICE 1086
#define ID_IN23CHECKBOX 1087
#define ID_IN23NAME 1088
#define ID_IN24CHECKBOX 1089
#define ID_IN24NAME 1090
#define ID_IN24CHOICE 1091
#define ID_IN25CHECKBOX 1092
#define ID_IN25NAME 1093
#define ID_IN25CHOICE 1094
#define ID_IN26CHECKBOX 1095
#define ID_IN26NAME 1096
#define ID_IN26CHOICE 1097
#define ID_IN27CHECKBOX 1098
#define ID_IN27NAME 1099
#define ID_IN27CHOICE 1100
#define ID_IN28CHECKBOX 1101
#define ID_IN28NAME 1102
#define ID_IN28CHOICE 1103
#define ID_IN29CHECKBOX 1104
#define ID_IN29NAME 1105
#define ID_IN29CHOICE 1106
#define ID_IN30CHECKBOX 1107
#define ID_IN30NAME 1108
#define ID_IN30CHOICE 1109
#define ID_BACKCHECKBOX 1110
#define ID_BACKNAME 1111
#define ID_BACKCHOICE 1112
#define ID_MASKBACKCHECKBOX 1113
#define ID_MASKBACKNAME 1114
#define ID_MASKBACKCHOICE 1115
#define ID_SILKSBACKCHECKBOX 1116
#define ID_SILKSBACKNAME 1117
#define ID_SILKSBACKCHOICE 1118
#define ID_SOLDPBACKCHECKBOX 1119
#define ID_SOLDPBACKNAME 1120
#define ID_SOLDPBACKCHOICE 1121
#define ID_ADHESBACKCHECKBOX 1122
#define ID_ADHESBACKNAME 1123
#define ID_ADHESBACKCHOICE 1124
#define ID_FABBACKCHECKBOX 1125
#define ID_FABBACKNAME 1126
#define ID_FABBACKCHOICE 1127
#define ID_CRTYDBACKCHECKBOX 1128
#define ID_CRTYDBACKNAME 1129
#define ID_CRTYDBACKCHOICE 1130
#define ID_PCBEDGESCHECKBOX 1131
#define ID_PCBEDGESNAME 1132
#define ID_PCBEDGESCHOICE 1133
#define ID_MARGINCHECKBOX 1134
#define ID_MARGINNAME 1135
#define ID_ECO2CHOICE 1136
#define ID_ECO2CHECKBOX 1137
#define ID_ECO2NAME 1138
#define ID_ECO1CHECKBOX 1139
#define ID_ECO1NAME 1140
#define ID_ECO1CHOICE 1141
#define ID_COMMENTSCHECKBOX 1142
#define ID_COMMENTSNAME 1143
#define ID_COMMENTSCHOICE 1144
#define ID_DRAWINGSCHECKBOX 1145
#define ID_DRAWINGSNAME 1146
#define ID_DRAWINGSCHOICE 1147

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_LAYERS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_LAYERS_BASE : public wxPanel
{
	private:

	protected:
		wxChoice* m_PresetsChoice;
		wxStaticText* m_staticTextCopperLayers;
		wxChoice* m_CopperLayersChoice;
		wxStaticText* m_thicknessLabel;
		wxTextCtrl* m_thicknessCtrl;
		wxStaticText* m_thicknessUnits;
		wxStaticLine* m_staticline2;
		wxScrolledWindow* m_LayersListPanel;
		wxFlexGridSizer* m_LayerListFlexGridSizer;
		wxCheckBox* m_CrtYdFrontCheckBox;
		wxStaticText* m_CrtYdFrontName;
		wxStaticText* m_CrtYdFrontStaticText;
		wxCheckBox* m_FabFrontCheckBox;
		wxStaticText* m_FabFrontName;
		wxStaticText* m_FabFrontStaticText;
		wxCheckBox* m_AdhesFrontCheckBox;
		wxStaticText* m_AdhesFrontName;
		wxStaticText* m_AdhesFrontStaticText;
		wxCheckBox* m_SoldPFrontCheckBox;
		wxStaticText* m_SoldPFrontName;
		wxStaticText* m_SoldPFrontStaticText;
		wxCheckBox* m_SilkSFrontCheckBox;
		wxStaticText* m_SilkSFrontName;
		wxStaticText* m_SilkSFrontStaticText;
		wxCheckBox* m_MaskFrontCheckBox;
		wxStaticText* m_MaskFrontName;
		wxStaticText* m_MaskFrontStaticText;
		wxCheckBox* m_FrontCheckBox;
		wxTextCtrl* m_FrontName;
		wxChoice* m_FrontChoice;
		wxCheckBox* m_In1CheckBox;
		wxTextCtrl* m_In1Name;
		wxChoice* m_In1Choice;
		wxCheckBox* m_In2CheckBox;
		wxTextCtrl* m_In2Name;
		wxChoice* m_In2Choice;
		wxCheckBox* m_In3CheckBox;
		wxTextCtrl* m_In3Name;
		wxChoice* m_In3Choice;
		wxCheckBox* m_In4CheckBox;
		wxTextCtrl* m_In4Name;
		wxChoice* m_In4Choice;
		wxCheckBox* m_In5CheckBox;
		wxTextCtrl* m_In5Name;
		wxChoice* m_In5Choice;
		wxCheckBox* m_In6CheckBox;
		wxTextCtrl* m_In6Name;
		wxChoice* m_In6Choice;
		wxCheckBox* m_In7CheckBox;
		wxTextCtrl* m_In7Name;
		wxChoice* m_In7Choice;
		wxCheckBox* m_In8CheckBox;
		wxTextCtrl* m_In8Name;
		wxChoice* m_In8Choice;
		wxCheckBox* m_In9CheckBox;
		wxTextCtrl* m_In9Name;
		wxChoice* m_In9Choice;
		wxCheckBox* m_In10CheckBox;
		wxTextCtrl* m_In10Name;
		wxChoice* m_In10Choice;
		wxCheckBox* m_In11CheckBox;
		wxTextCtrl* m_In11Name;
		wxChoice* m_In11Choice;
		wxCheckBox* m_In12CheckBox;
		wxTextCtrl* m_In12Name;
		wxChoice* m_In12Choice;
		wxCheckBox* m_In13CheckBox;
		wxTextCtrl* m_In13Name;
		wxChoice* m_In13Choice;
		wxCheckBox* m_In14CheckBox;
		wxTextCtrl* m_In14Name;
		wxChoice* m_In14Choice;
		wxCheckBox* m_In15CheckBox;
		wxTextCtrl* m_In15Name;
		wxChoice* m_In15Choice;
		wxCheckBox* m_In16CheckBox;
		wxTextCtrl* m_In16Name;
		wxChoice* m_In16Choice;
		wxCheckBox* m_In17CheckBox;
		wxTextCtrl* m_In17Name;
		wxChoice* m_In17Choice;
		wxCheckBox* m_In18CheckBox;
		wxTextCtrl* m_In18Name;
		wxChoice* m_In18Choice;
		wxCheckBox* m_In19CheckBox;
		wxTextCtrl* m_In19Name;
		wxChoice* m_In19Choice;
		wxCheckBox* m_In20CheckBox;
		wxTextCtrl* m_In20Name;
		wxChoice* m_In20Choice;
		wxCheckBox* m_In21CheckBox;
		wxTextCtrl* m_In21Name;
		wxChoice* m_In21Choice;
		wxCheckBox* m_In22CheckBox;
		wxTextCtrl* m_In22Name;
		wxChoice* m_In22Choice;
		wxCheckBox* m_In23CheckBox;
		wxTextCtrl* m_In23Name;
		wxChoice* m_In23Choice;
		wxCheckBox* m_In24CheckBox;
		wxTextCtrl* m_In24Name;
		wxChoice* m_In24Choice;
		wxCheckBox* m_In25CheckBox;
		wxTextCtrl* m_In25Name;
		wxChoice* m_In25Choice;
		wxCheckBox* m_In26CheckBox;
		wxTextCtrl* m_In26Name;
		wxChoice* m_In26Choice;
		wxCheckBox* m_In27CheckBox;
		wxTextCtrl* m_In27Name;
		wxChoice* m_In27Choice;
		wxCheckBox* m_In28CheckBox;
		wxTextCtrl* m_In28Name;
		wxChoice* m_In28Choice;
		wxCheckBox* m_In29CheckBox;
		wxTextCtrl* m_In29Name;
		wxChoice* m_In29Choice;
		wxCheckBox* m_In30CheckBox;
		wxTextCtrl* m_In30Name;
		wxChoice* m_In30Choice;
		wxCheckBox* m_BackCheckBox;
		wxTextCtrl* m_BackName;
		wxChoice* m_BackChoice;
		wxCheckBox* m_MaskBackCheckBox;
		wxStaticText* m_MaskBackName;
		wxStaticText* m_MaskBackStaticText;
		wxCheckBox* m_SilkSBackCheckBox;
		wxStaticText* m_SilkSBackName;
		wxStaticText* m_SilkSBackStaticText;
		wxCheckBox* m_SoldPBackCheckBox;
		wxStaticText* m_SoldPBackName;
		wxStaticText* m_SoldPBackStaticText;
		wxCheckBox* m_AdhesBackCheckBox;
		wxStaticText* m_AdhesBackName;
		wxStaticText* m_AdhesBackStaticText;
		wxCheckBox* m_FabBackCheckBox;
		wxStaticText* m_FabBackName;
		wxStaticText* m_FabBackStaticText;
		wxCheckBox* m_CrtYdBackCheckBox;
		wxStaticText* m_CrtYdBackName;
		wxStaticText* m_CrtYdBackStaticText;
		wxCheckBox* m_PCBEdgesCheckBox;
		wxStaticText* m_PCBEdgesName;
		wxStaticText* m_PCBEdgesStaticText;
		wxCheckBox* m_MarginCheckBox;
		wxStaticText* m_MarginName;
		wxStaticText* m_MarginStaticText;
		wxCheckBox* m_Eco1CheckBox;
		wxStaticText* m_Eco1Name;
		wxStaticText* m_Eco1StaticText;
		wxCheckBox* m_Eco2CheckBox;
		wxStaticText* m_Eco2Name;
		wxStaticText* m_Eco2StaticText;
		wxCheckBox* m_CommentsCheckBox;
		wxStaticText* m_CommentsName;
		wxStaticText* m_CommentsStaticText;
		wxCheckBox* m_DrawingsCheckBox;
		wxStaticText* m_DrawingsName;
		wxStaticText* m_DrawingsStaticText;

		// Virtual event handlers, overide them in your derived class
		virtual void OnPresetsChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCopperLayersChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void DenyChangeCheckBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckBox( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_LAYERS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 621,545 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_SETUP_LAYERS_BASE();

};

