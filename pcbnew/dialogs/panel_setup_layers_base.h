///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gdicmn.h>
#include <wx/button.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CRTYDFRONTCHECKBOX 1000
#define ID_CRTYDFRONTCHOICE 1001
#define ID_FABFRONTCHECKBOX 1002
#define ID_FABFRONTCHOICE 1003
#define ID_ADHESFRONTCHECKBOX 1004
#define ID_ADHESFRONTCHOICE 1005
#define ID_SOLDPFRONTCHECKBOX 1006
#define ID_SOLDPFRONTCHOICE 1007
#define ID_SILKSFRONTCHECKBOX 1008
#define ID_SILKSFRONTCHOICE 1009
#define ID_MASKFRONTCHECKBOX 1010
#define ID_MASKFRONTCHOICE 1011
#define ID_FRONTCHECKBOX 1012
#define ID_FRONTNAME 1013
#define ID_FRONTCHOICE 1014
#define ID_IN1CHECKBOX 1015
#define ID_IN1NAME 1016
#define ID_IN1CHOICE 1017
#define ID_IN2CHECKBOX 1018
#define ID_IN2NAME 1019
#define ID_IN2CHOICE 1020
#define ID_IN3CHECKBOX 1021
#define ID_IN3NAME 1022
#define ID_IN3CHOICE 1023
#define ID_IN4CHECKBOX 1024
#define ID_IN4NAME 1025
#define ID_IN4CHOICE 1026
#define ID_IN5CHECKBOX 1027
#define ID_IN5NAME 1028
#define ID_IN5CHOICE 1029
#define ID_IN6CHECKBOX 1030
#define ID_IN6NAME 1031
#define ID_IN6CHOICE 1032
#define ID_IN7CHECKBOX 1033
#define ID_IN7NAME 1034
#define ID_IN7CHOICE 1035
#define ID_IN8CHECKBOX 1036
#define ID_IN8NAME 1037
#define ID_IN8CHOICE 1038
#define ID_IN9CHECKBOX 1039
#define ID_IN9NAME 1040
#define ID_IN9CHOICE 1041
#define ID_IN10CHECKBOX 1042
#define ID_IN10NAME 1043
#define ID_IN10CHOICE 1044
#define ID_IN11CHECKBOX 1045
#define ID_IN11NAME 1046
#define ID_IN11CHOICE 1047
#define ID_IN12CHECKBOX 1048
#define ID_IN12NAME 1049
#define ID_IN12CHOICE 1050
#define ID_IN13CHECKBOX 1051
#define ID_IN13NAME 1052
#define ID_IN13CHOICE 1053
#define ID_IN14CHECKBOX 1054
#define ID_IN14NAME 1055
#define ID_IN14CHOICE 1056
#define ID_IN15CHECKBOX 1057
#define ID_IN15NAME 1058
#define ID_IN15CHOICE 1059
#define ID_IN16CHECKBOX 1060
#define ID_IN16NAME 1061
#define ID_IN16CHOICE 1062
#define ID_IN17CHECKBOX 1063
#define ID_IN17NAME 1064
#define ID_IN17CHOICE 1065
#define ID_IN18CHECKBOX 1066
#define ID_IN18NAME 1067
#define ID_IN18CHOICE 1068
#define ID_IN19CHECKBOX 1069
#define ID_IN19NAME 1070
#define ID_IN19CHOICE 1071
#define ID_IN20CHECKBOX 1072
#define ID_IN20NAME 1073
#define ID_IN20CHOICE 1074
#define ID_IN21CHECKBOX 1075
#define ID_IN21NAME 1076
#define ID_IN21CHOICE 1077
#define ID_IN22CHECKBOX 1078
#define ID_IN22NAME 1079
#define ID_IN22CHOICE 1080
#define ID_IN23CHECKBOX 1081
#define ID_IN23NAME 1082
#define ID_IN24CHECKBOX 1083
#define ID_IN24NAME 1084
#define ID_IN24CHOICE 1085
#define ID_IN25CHECKBOX 1086
#define ID_IN25NAME 1087
#define ID_IN25CHOICE 1088
#define ID_IN26CHECKBOX 1089
#define ID_IN26NAME 1090
#define ID_IN26CHOICE 1091
#define ID_IN27CHECKBOX 1092
#define ID_IN27NAME 1093
#define ID_IN27CHOICE 1094
#define ID_IN28CHECKBOX 1095
#define ID_IN28NAME 1096
#define ID_IN28CHOICE 1097
#define ID_IN29CHECKBOX 1098
#define ID_IN29NAME 1099
#define ID_IN29CHOICE 1100
#define ID_IN30CHECKBOX 1101
#define ID_IN30NAME 1102
#define ID_IN30CHOICE 1103
#define ID_BACKCHECKBOX 1104
#define ID_BACKNAME 1105
#define ID_BACKCHOICE 1106
#define ID_MASKBACKCHECKBOX 1107
#define ID_MASKBACKCHOICE 1108
#define ID_SILKSBACKCHECKBOX 1109
#define ID_SILKSBACKCHOICE 1110
#define ID_SOLDPBACKCHECKBOX 1111
#define ID_SOLDPBACKCHOICE 1112
#define ID_ADHESBACKCHECKBOX 1113
#define ID_ADHESBACKCHOICE 1114
#define ID_FABBACKCHECKBOX 1115
#define ID_FABBACKCHOICE 1116
#define ID_CRTYDBACKCHECKBOX 1117
#define ID_CRTYDBACKCHOICE 1118
#define ID_PCBEDGESCHECKBOX 1119
#define ID_PCBEDGESCHOICE 1120
#define ID_MARGINCHECKBOX 1121
#define ID_ECO2CHOICE 1122
#define ID_ECO2CHECKBOX 1123
#define ID_ECO1CHECKBOX 1124
#define ID_ECO1CHOICE 1125
#define ID_COMMENTSCHECKBOX 1126
#define ID_COMMENTSCHOICE 1127
#define ID_DRAWINGSCHECKBOX 1128
#define ID_DRAWINGSCHOICE 1129

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_LAYERS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_LAYERS_BASE : public wxPanel
{
	private:

	protected:
		wxButton* m_addUserDefinedLayerButton;
		wxStaticLine* m_staticline2;
		wxScrolledWindow* m_LayersListPanel;
		wxFlexGridSizer* m_LayerListFlexGridSizer;
		wxCheckBox* m_CrtYdFrontCheckBox;
		wxTextCtrl* m_CrtYdFrontName;
		wxStaticText* m_CrtYdFrontStaticText;
		wxCheckBox* m_FabFrontCheckBox;
		wxTextCtrl* m_FabFrontName;
		wxStaticText* m_FabFrontStaticText;
		wxCheckBox* m_AdhesFrontCheckBox;
		wxTextCtrl* m_AdhesFrontName;
		wxStaticText* m_AdhesFrontStaticText;
		wxCheckBox* m_SoldPFrontCheckBox;
		wxTextCtrl* m_SoldPFrontName;
		wxStaticText* m_SoldPFrontStaticText;
		wxCheckBox* m_SilkSFrontCheckBox;
		wxTextCtrl* m_SilkSFrontName;
		wxStaticText* m_SilkSFrontStaticText;
		wxCheckBox* m_MaskFrontCheckBox;
		wxTextCtrl* m_MaskFrontName;
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
		wxTextCtrl* m_MaskBackName;
		wxStaticText* m_MaskBackStaticText;
		wxCheckBox* m_SilkSBackCheckBox;
		wxTextCtrl* m_SilkSBackName;
		wxStaticText* m_SilkSBackStaticText;
		wxCheckBox* m_SoldPBackCheckBox;
		wxTextCtrl* m_SoldPBackName;
		wxStaticText* m_SoldPBackStaticText;
		wxCheckBox* m_AdhesBackCheckBox;
		wxTextCtrl* m_AdhesBackName;
		wxStaticText* m_AdhesBackStaticText;
		wxCheckBox* m_FabBackCheckBox;
		wxTextCtrl* m_FabBackName;
		wxStaticText* m_FabBackStaticText;
		wxCheckBox* m_CrtYdBackCheckBox;
		wxTextCtrl* m_CrtYdBackName;
		wxStaticText* m_CrtYdBackStaticText;
		wxCheckBox* m_PCBEdgesCheckBox;
		wxTextCtrl* m_PCBEdgesName;
		wxStaticText* m_PCBEdgesStaticText;
		wxCheckBox* m_MarginCheckBox;
		wxTextCtrl* m_MarginName;
		wxStaticText* m_MarginStaticText;
		wxCheckBox* m_Eco1CheckBox;
		wxTextCtrl* m_Eco1Name;
		wxStaticText* m_Eco1StaticText;
		wxCheckBox* m_Eco2CheckBox;
		wxTextCtrl* m_Eco2Name;
		wxStaticText* m_Eco2StaticText;
		wxCheckBox* m_CommentsCheckBox;
		wxTextCtrl* m_CommentsName;
		wxStaticText* m_CommentsStaticText;
		wxCheckBox* m_DrawingsCheckBox;
		wxTextCtrl* m_DrawingsName;
		wxStaticText* m_DrawingsStaticText;
		wxCheckBox* m_User1CheckBox;
		wxTextCtrl* m_User1Name;
		wxStaticText* m_User1StaticText;
		wxCheckBox* m_User2CheckBox;
		wxTextCtrl* m_User2Name;
		wxStaticText* m_User2StaticText;
		wxCheckBox* m_User3CheckBox;
		wxTextCtrl* m_User3Name;
		wxStaticText* m_User3StaticText;
		wxCheckBox* m_User4CheckBox;
		wxTextCtrl* m_User4Name;
		wxStaticText* m_User4StaticText;
		wxCheckBox* m_User5CheckBox;
		wxTextCtrl* m_User5Name;
		wxStaticText* m_User5StaticText;
		wxCheckBox* m_User6CheckBox;
		wxTextCtrl* m_User6Name;
		wxStaticText* m_User6StaticText;
		wxCheckBox* m_User7CheckBox;
		wxTextCtrl* m_User7Name;
		wxStaticText* m_User7StaticText;
		wxCheckBox* m_User8CheckBox;
		wxTextCtrl* m_User8Name;
		wxStaticText* m_User8StaticText;
		wxCheckBox* m_User9CheckBox;
		wxTextCtrl* m_User9Name;
		wxStaticText* m_User9StaticText;

		// Virtual event handlers, overide them in your derived class
		virtual void addUserDefinedLayer( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdateAddUserDefinedLayer( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void DenyChangeCheckBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckBox( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_LAYERS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_SETUP_LAYERS_BASE();

};

