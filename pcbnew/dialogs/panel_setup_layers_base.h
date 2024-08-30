///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/button.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CRTYDFRONTCHOICE 6000
#define ID_FABFRONTCHECKBOX 6001
#define ID_FABFRONTCHOICE 6002
#define ID_ADHESFRONTCHECKBOX 6003
#define ID_ADHESFRONTCHOICE 6004
#define ID_SOLDPFRONTCHECKBOX 6005
#define ID_SOLDPFRONTCHOICE 6006
#define ID_SILKSFRONTCHECKBOX 6007
#define ID_SILKSFRONTCHOICE 6008
#define ID_MASKFRONTCHECKBOX 6009
#define ID_MASKFRONTCHOICE 6010
#define ID_FRONTCHECKBOX 6011
#define ID_FRONTNAME 6012
#define ID_FRONTCHOICE 6013
#define ID_IN1CHECKBOX 6014
#define ID_IN1NAME 6015
#define ID_IN1CHOICE 6016
#define ID_IN2CHECKBOX 6017
#define ID_IN2NAME 6018
#define ID_IN2CHOICE 6019
#define ID_IN3CHECKBOX 6020
#define ID_IN3NAME 6021
#define ID_IN3CHOICE 6022
#define ID_IN4CHECKBOX 6023
#define ID_IN4NAME 6024
#define ID_IN4CHOICE 6025
#define ID_IN5CHECKBOX 6026
#define ID_IN5NAME 6027
#define ID_IN5CHOICE 6028
#define ID_IN6CHECKBOX 6029
#define ID_IN6NAME 6030
#define ID_IN6CHOICE 6031
#define ID_IN7CHECKBOX 6032
#define ID_IN7NAME 6033
#define ID_IN7CHOICE 6034
#define ID_IN8CHECKBOX 6035
#define ID_IN8NAME 6036
#define ID_IN8CHOICE 6037
#define ID_IN9CHECKBOX 6038
#define ID_IN9NAME 6039
#define ID_IN9CHOICE 6040
#define ID_IN10CHECKBOX 6041
#define ID_IN10NAME 6042
#define ID_IN10CHOICE 6043
#define ID_IN11CHECKBOX 6044
#define ID_IN11NAME 6045
#define ID_IN11CHOICE 6046
#define ID_IN12CHECKBOX 6047
#define ID_IN12NAME 6048
#define ID_IN12CHOICE 6049
#define ID_IN13CHECKBOX 6050
#define ID_IN13NAME 6051
#define ID_IN13CHOICE 6052
#define ID_IN14CHECKBOX 6053
#define ID_IN14NAME 6054
#define ID_IN14CHOICE 6055
#define ID_IN15CHECKBOX 6056
#define ID_IN15NAME 6057
#define ID_IN15CHOICE 6058
#define ID_IN16CHECKBOX 6059
#define ID_IN16NAME 6060
#define ID_IN16CHOICE 6061
#define ID_IN17CHECKBOX 6062
#define ID_IN17NAME 6063
#define ID_IN17CHOICE 6064
#define ID_IN18CHECKBOX 6065
#define ID_IN18NAME 6066
#define ID_IN18CHOICE 6067
#define ID_IN19CHECKBOX 6068
#define ID_IN19NAME 6069
#define ID_IN19CHOICE 6070
#define ID_IN20CHECKBOX 6071
#define ID_IN20NAME 6072
#define ID_IN20CHOICE 6073
#define ID_IN21CHECKBOX 6074
#define ID_IN21NAME 6075
#define ID_IN21CHOICE 6076
#define ID_IN22CHECKBOX 6077
#define ID_IN22NAME 6078
#define ID_IN22CHOICE 6079
#define ID_IN23CHECKBOX 6080
#define ID_IN23NAME 6081
#define ID_IN24CHECKBOX 6082
#define ID_IN24NAME 6083
#define ID_IN24CHOICE 6084
#define ID_IN25CHECKBOX 6085
#define ID_IN25NAME 6086
#define ID_IN25CHOICE 6087
#define ID_IN26CHECKBOX 6088
#define ID_IN26NAME 6089
#define ID_IN26CHOICE 6090
#define ID_IN27CHECKBOX 6091
#define ID_IN27NAME 6092
#define ID_IN27CHOICE 6093
#define ID_IN28CHECKBOX 6094
#define ID_IN28NAME 6095
#define ID_IN28CHOICE 6096
#define ID_IN29CHECKBOX 6097
#define ID_IN29NAME 6098
#define ID_IN29CHOICE 6099
#define ID_IN30CHECKBOX 6100
#define ID_IN30NAME 6101
#define ID_IN30CHOICE 6102
#define ID_BACKCHECKBOX 6103
#define ID_BACKNAME 6104
#define ID_BACKCHOICE 6105
#define ID_MASKBACKCHECKBOX 6106
#define ID_MASKBACKCHOICE 6107
#define ID_SILKSBACKCHECKBOX 6108
#define ID_SILKSBACKCHOICE 6109
#define ID_SOLDPBACKCHECKBOX 6110
#define ID_SOLDPBACKCHOICE 6111
#define ID_ADHESBACKCHECKBOX 6112
#define ID_ADHESBACKCHOICE 6113
#define ID_FABBACKCHECKBOX 6114
#define ID_FABBACKCHOICE 6115
#define ID_CRTYDBACKCHOICE 6116
#define ID_PCBEDGESCHOICE 6117
#define ID_MARGINCHECKBOX 6118
#define ID_ECO2CHOICE 6119
#define ID_ECO2CHECKBOX 6120
#define ID_ECO1CHECKBOX 6121
#define ID_ECO1CHOICE 6122
#define ID_COMMENTSCHECKBOX 6123
#define ID_COMMENTSCHOICE 6124
#define ID_DRAWINGSCHECKBOX 6125
#define ID_DRAWINGSCHOICE 6126

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
		wxTextCtrl* m_CrtYdBackName;
		wxStaticText* m_CrtYdBackStaticText;
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
		wxChoice* m_User1Type;
		wxCheckBox* m_User2CheckBox;
		wxTextCtrl* m_User2Name;
		wxChoice* m_User2Type;
		wxCheckBox* m_User3CheckBox;
		wxTextCtrl* m_User3Name;
		wxChoice* m_User3Type;
		wxCheckBox* m_User4CheckBox;
		wxTextCtrl* m_User4Name;
		wxChoice* m_User4Type;
		wxCheckBox* m_User5CheckBox;
		wxTextCtrl* m_User5Name;
		wxChoice* m_User5Type;
		wxCheckBox* m_User6CheckBox;
		wxTextCtrl* m_User6Name;
		wxChoice* m_User6Type;
		wxCheckBox* m_User7CheckBox;
		wxTextCtrl* m_User7Name;
		wxChoice* m_User7Type;
		wxCheckBox* m_User8CheckBox;
		wxTextCtrl* m_User8Name;
		wxChoice* m_User8Type;
		wxCheckBox* m_User9CheckBox;
		wxTextCtrl* m_User9Name;
		wxChoice* m_User9Type;

		// Virtual event handlers, override them in your derived class
		virtual void addUserDefinedLayer( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void DenyChangeCheckBox( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_LAYERS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_LAYERS_BASE();

};

