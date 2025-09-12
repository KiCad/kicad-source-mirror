///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-75-g9786507b-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "panel_common_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_COMMON_SETTINGS_BASE::PANEL_COMMON_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText20 = new wxStaticText( this, wxID_ANY, _("Rendering Engine"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText20->Wrap( -1 );
	bLeftSizer->Add( m_staticText20, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftSizer->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );


	bLeftSizer->Add( 0, 3, 0, 0, 5 );

	m_renderingSizer = new wxFlexGridSizer( 0, 1, 4, 0 );
	m_renderingSizer->SetFlexibleDirection( wxBOTH );
	m_renderingSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rbAccelerated = new wxRadioButton( this, wxID_ANY, _("Accelerated Graphics"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbAccelerated->SetToolTip( _("Hardware-accelerated graphics (recommended)") );

	m_renderingSizer->Add( m_rbAccelerated, 0, wxLEFT, 5 );

	m_rbFallback = new wxRadioButton( this, wxID_ANY, _("Fallback Graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbFallback->SetToolTip( _("Software graphics (for computers which do not support KiCad's hardware acceleration requirements)") );

	m_renderingSizer->Add( m_rbFallback, 0, wxLEFT, 5 );


	bLeftSizer->Add( m_renderingSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bLeftSizer->Add( 0, 3, 0, 0, 5 );

	wxGridBagSizer* gbSizer11;
	gbSizer11 = new wxGridBagSizer( 0, 6 );
	gbSizer11->SetFlexibleDirection( wxBOTH );
	gbSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer11->SetEmptyCellSize( wxSize( -1,2 ) );

	wxStaticText* antialiasingLabel;
	antialiasingLabel = new wxStaticText( this, wxID_ANY, _("Antialiasing:"), wxDefaultPosition, wxDefaultSize, 0 );
	antialiasingLabel->Wrap( -1 );
	gbSizer11->Add( antialiasingLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_antialiasingChoices[] = { _("No Antialiasing"), _("Fast Antialiasing"), _("High Quality Antialiasing") };
	int m_antialiasingNChoices = sizeof( m_antialiasingChoices ) / sizeof( wxString );
	m_antialiasing = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_antialiasingNChoices, m_antialiasingChoices, 0 );
	m_antialiasing->SetSelection( 0 );
	gbSizer11->Add( m_antialiasing, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	gbSizer11->AddGrowableCol( 1 );

	bLeftSizer->Add( gbSizer11, 0, wxLEFT, 5 );

	wxBoxSizer* bAntialiasingSizer;
	bAntialiasingSizer = new wxBoxSizer( wxVERTICAL );


	bLeftSizer->Add( bAntialiasingSizer, 0, wxTOP|wxLEFT|wxEXPAND, 5 );


	bLeftSizer->Add( 0, 15, 0, 0, 5 );

	m_staticText21 = new wxStaticText( this, wxID_ANY, _("Helper Applications"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	bLeftSizer->Add( m_staticText21, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftSizer->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bHelperAppsSizer;
	bHelperAppsSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* textEditorLabel;
	textEditorLabel = new wxStaticText( this, wxID_ANY, _("Text editor:"), wxDefaultPosition, wxDefaultSize, 0 );
	textEditorLabel->Wrap( -1 );
	bSizer61->Add( textEditorLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_textEditorPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer61->Add( m_textEditorPath, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_textEditorBtn = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer61->Add( m_textEditorBtn, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bHelperAppsSizer->Add( bSizer61, 0, wxTOP|wxRIGHT|wxEXPAND, 5 );


	bHelperAppsSizer->Add( 0, 4, 0, wxEXPAND, 5 );

	bSizerFileManager = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextFileManager = new wxStaticText( this, wxID_ANY, _("File manager:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFileManager->Wrap( -1 );
	bSizerFileManager->Add( m_staticTextFileManager, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_textCtrlFileManager = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerFileManager->Add( m_textCtrlFileManager, 1, wxALIGN_CENTER_VERTICAL, 5 );


	bHelperAppsSizer->Add( bSizerFileManager, 0, wxEXPAND|wxRIGHT|wxTOP, 5 );


	bHelperAppsSizer->Add( 0, 12, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_defaultPDFViewer = new wxRadioButton( this, wxID_ANY, _("System default PDF viewer"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer8->Add( m_defaultPDFViewer, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bHelperAppsSizer->Add( bSizer8, 0, wxBOTTOM|wxEXPAND, 2 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_otherPDFViewer = new wxRadioButton( this, wxID_ANY, _("Other:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_otherPDFViewer, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_PDFViewerPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_PDFViewerPath, 1, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_pdfViewerBtn = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer7->Add( m_pdfViewerBtn, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bHelperAppsSizer->Add( bSizer7, 0, wxBOTTOM|wxRIGHT|wxEXPAND, 5 );


	bLeftSizer->Add( bHelperAppsSizer, 0, wxTOP|wxLEFT|wxEXPAND, 5 );


	bLeftSizer->Add( 0, 15, 0, 0, 5 );

	m_staticText22 = new wxStaticText( this, wxID_ANY, _("User Interface"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	bLeftSizer->Add( m_staticText22, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bUserInterfaceSizer;
	bUserInterfaceSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );

	m_checkBoxIconsInMenus = new wxCheckBox( this, wxID_ANY, _("Show icons in menus"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer14->Add( m_checkBoxIconsInMenus, 0, wxALL, 5 );

	m_showScrollbars = new wxCheckBox( this, wxID_ANY, _("Show scrollbars in editors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showScrollbars->SetValue(true);
	m_showScrollbars->SetToolTip( _("This change takes effect when relaunching the editor.") );

	bSizer14->Add( m_showScrollbars, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_focusFollowSchPcb = new wxCheckBox( this, wxID_ANY, _("Focus follows mouse between schematic and PCB editors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_focusFollowSchPcb->SetToolTip( _("If the mouse cursor is moved over the canvas of a schematic or PCB editor window, that window is raised.") );

	bSizer14->Add( m_focusFollowSchPcb, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_hotkeyFeedback = new wxCheckBox( this, wxID_ANY, _("Show popup indicator when toggling settings with hotkeys"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hotkeyFeedback->SetToolTip( _("When enabled, certain hotkeys that cycle between settings will show a popup indicator briefly to indicate the change in settings.") );

	bSizer14->Add( m_hotkeyFeedback, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_gridStriping = new wxCheckBox( this, wxID_ANY, _("Use alternating row colors in tables"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gridStriping->SetToolTip( _("When enabled, use a different color for every other table row") );

	bSizer14->Add( m_gridStriping, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bUserInterfaceSizer->Add( bSizer14, 0, wxEXPAND, 5 );

	m_disableCustomCursors = new wxCheckBox( this, wxID_ANY, _("Disable custom cursors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_disableCustomCursors->SetToolTip( _("When enabled, KiCad will use default system cursors instead of custom ones") );

	bUserInterfaceSizer->Add( m_disableCustomCursors, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizerIconsTheme;
	bSizerIconsTheme = new wxBoxSizer( wxHORIZONTAL );

	m_stIconTheme = new wxStaticText( this, wxID_ANY, _("Icon theme:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stIconTheme->Wrap( -1 );
	bSizerIconsTheme->Add( m_stIconTheme, 0, wxALL, 5 );

	m_rbIconThemeLight = new wxRadioButton( this, wxID_ANY, _("Light"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbIconThemeLight->SetToolTip( _("Use icons designed for light window backgrounds") );

	bSizerIconsTheme->Add( m_rbIconThemeLight, 0, wxALL, 5 );

	m_rbIconThemeDark = new wxRadioButton( this, wxID_ANY, _("Dark"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbIconThemeDark->SetToolTip( _("Use icons designed for dark window backgrounds") );

	bSizerIconsTheme->Add( m_rbIconThemeDark, 0, wxALL, 5 );

	m_rbIconThemeAuto = new wxRadioButton( this, wxID_ANY, _("Automatic"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbIconThemeAuto->SetValue( true );
	m_rbIconThemeAuto->SetToolTip( _("Automatically choose light or dark icons based on the system color theme") );

	bSizerIconsTheme->Add( m_rbIconThemeAuto, 0, wxALL, 5 );


	bUserInterfaceSizer->Add( bSizerIconsTheme, 0, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizerToolbarSize;
	bSizerToolbarSize = new wxBoxSizer( wxHORIZONTAL );

	m_stToolbarIconSize = new wxStaticText( this, wxID_ANY, _("Toolbar icon size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stToolbarIconSize->Wrap( -1 );
	bSizerToolbarSize->Add( m_stToolbarIconSize, 0, wxALL, 5 );

	m_rbIconSizeSmall = new wxRadioButton( this, wxID_ANY, _("Small"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbIconSizeSmall->SetToolTip( _("Use compact icons in the toolbars") );

	bSizerToolbarSize->Add( m_rbIconSizeSmall, 0, wxALL, 5 );

	m_rbIconSizeNormal = new wxRadioButton( this, wxID_ANY, _("Normal"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbIconSizeNormal->SetToolTip( _("Use the default KiCad icon size in the toolbars") );

	bSizerToolbarSize->Add( m_rbIconSizeNormal, 0, wxALL, 5 );

	m_rbIconSizeLarge = new wxRadioButton( this, wxID_ANY, _("Large"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbIconSizeLarge->SetToolTip( _("Use larger icons in the toolbars") );

	bSizerToolbarSize->Add( m_rbIconSizeLarge, 0, wxALL, 5 );


	bUserInterfaceSizer->Add( bSizerToolbarSize, 0, wxEXPAND, 5 );

	m_scaleFonts = new wxCheckBox( this, wxID_ANY, _("Apply icon scaling to fonts"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserInterfaceSizer->Add( m_scaleFonts, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_fontScalingHelp = new wxStaticText( this, wxID_ANY, _("(This workaround will improve some GTK HiDPI font scaling issues.)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontScalingHelp->Wrap( -1 );
	bUserInterfaceSizer->Add( m_fontScalingHelp, 0, wxBOTTOM|wxLEFT, 8 );

	m_gbUserInterface = new wxGridBagSizer( 5, 0 );
	m_gbUserInterface->SetFlexibleDirection( wxVERTICAL );
	m_gbUserInterface->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	m_staticTextCanvasScale = new wxStaticText( this, wxID_ANY, _("Canvas scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCanvasScale->Wrap( -1 );
	m_gbUserInterface->Add( m_staticTextCanvasScale, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_canvasScaleCtrl = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0, 1 );
	m_canvasScaleCtrl->SetDigits( 0 );
	m_gbUserInterface->Add( m_canvasScaleCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_canvasScaleAuto = new wxCheckBox( this, wxID_ANY, _("Automatic"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gbUserInterface->Add( m_canvasScaleAuto, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );


	bUserInterfaceSizer->Add( m_gbUserInterface, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxBoxSizer* bSizerHighContrast;
	bSizerHighContrast = new wxBoxSizer( wxHORIZONTAL );

	m_highContrastLabel = new wxStaticText( this, wxID_ANY, _("High-contrast mode dimming factor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_highContrastLabel->Wrap( -1 );
	bSizerHighContrast->Add( m_highContrastLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_highContrastCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerHighContrast->Add( m_highContrastCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_highContrastUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_highContrastUnits->Wrap( -1 );
	bSizerHighContrast->Add( m_highContrastUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bUserInterfaceSizer->Add( bSizerHighContrast, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bLeftSizer->Add( bUserInterfaceSizer, 0, wxTOP|wxLEFT|wxEXPAND, 5 );


	bLeftSizer->Add( 0, 0, 0, 0, 5 );

	m_staticText251 = new wxStaticText( this, wxID_ANY, _("Scaling"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText251->Wrap( -1 );
	bLeftSizer->Add( m_staticText251, 0, wxLEFT|wxRIGHT|wxTOP, 15 );

	m_staticline7 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftSizer->Add( m_staticline7, 0, wxEXPAND | wxALL, 5 );


	bPanelSizer->Add( bLeftSizer, 0, wxRIGHT, 35 );

	wxBoxSizer* rightSizer;
	rightSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText23 = new wxStaticText( this, wxID_ANY, _("Editing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	rightSizer->Add( m_staticText23, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline6 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	rightSizer->Add( m_staticline6, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bEditingSizer;
	bEditingSizer = new wxBoxSizer( wxVERTICAL );

	m_warpMouseOnMove = new wxCheckBox( this, wxID_ANY, _("Warp mouse to anchor of moved object"), wxDefaultPosition, wxDefaultSize, 0 );
	m_warpMouseOnMove->SetValue(true);
	bEditingSizer->Add( m_warpMouseOnMove, 0, wxALL, 5 );

	m_NonImmediateActions = new wxCheckBox( this, wxID_ANY, _("First hotkey selects tool"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NonImmediateActions->SetToolTip( _("If not checked, hotkeys will immediately perform an action even if the relevant tool was not previously selected.") );

	bEditingSizer->Add( m_NonImmediateActions, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	rightSizer->Add( bEditingSizer, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	rightSizer->Add( 0, 15, 0, wxEXPAND, 5 );

	m_staticText24 = new wxStaticText( this, wxID_ANY, _("Session"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	rightSizer->Add( m_staticText24, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline5 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	rightSizer->Add( m_staticline5, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSessionSizer;
	bSessionSizer = new wxBoxSizer( wxVERTICAL );

	m_cbRememberOpenFiles = new wxCheckBox( this, wxID_ANY, _("Remember open files for next project launch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRememberOpenFiles->SetValue(true);
	m_cbRememberOpenFiles->SetToolTip( _("If checked, launching a project will also launch tools such as the schematic and board editors with previously open files") );

	bSessionSizer->Add( m_cbRememberOpenFiles, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bSessionSizer->Add( 0, 5, 0, 0, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 5, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,2 ) );

	m_staticTextFileHistorySize = new wxStaticText( this, wxID_ANY, _("File history size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFileHistorySize->Wrap( -1 );
	gbSizer1->Add( m_staticTextFileHistorySize, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_fileHistorySize = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 0 );
	gbSizer1->Add( m_fileHistorySize, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	gbSizer1->AddGrowableCol( 1 );

	bSessionSizer->Add( gbSizer1, 0, wxALL, 5 );


	rightSizer->Add( bSessionSizer, 0, wxTOP|wxLEFT|wxEXPAND, 5 );


	rightSizer->Add( 0, 15, 0, wxEXPAND, 5 );

	m_staticText25 = new wxStaticText( this, wxID_ANY, _("Project Backup"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	rightSizer->Add( m_staticText25, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline4 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	rightSizer->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bProjectBackupSizer;
	bProjectBackupSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer3;
	gbSizer3 = new wxGridBagSizer( 2, 0 );
	gbSizer3->SetFlexibleDirection( wxBOTH );
	gbSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbBackupEnabled = new wxCheckBox( this, wxID_ANY, _("Automatically backup projects"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbBackupEnabled->SetToolTip( _("Automatically create backup archives of the current project when saving files") );

	gbSizer3->Add( m_cbBackupEnabled, wxGBPosition( 0, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT, 5 );

	m_staticText16 = new wxStaticText( this, wxID_ANY, _("Maximum total backup size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	m_staticText16->SetToolTip( _("If the total size of backup files grows above this limit, old backups will be deleted (set to 0 for no limit)") );

	gbSizer3->Add( m_staticText16, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 3 );

	m_backupLimitTotalSize = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000, 0 );
	gbSizer3->Add( m_backupLimitTotalSize, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 3 );

	m_staticText17 = new wxStaticText( this, wxID_ANY, _("MB"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	gbSizer3->Add( m_staticText17, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );


	gbSizer3->AddGrowableCol( 1 );

	bProjectBackupSizer->Add( gbSizer3, 0, wxALL, 5 );


	rightSizer->Add( bProjectBackupSizer, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bPanelSizer->Add( rightSizer, 0, wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_textEditorBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnTextEditorClick ), NULL, this );
	m_defaultPDFViewer->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnRadioButtonPdfViewer ), NULL, this );
	m_otherPDFViewer->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnRadioButtonPdfViewer ), NULL, this );
	m_pdfViewerBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnPDFViewerClick ), NULL, this );
	m_canvasScaleAuto->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnCanvasScaleAuto ), NULL, this );
}

PANEL_COMMON_SETTINGS_BASE::~PANEL_COMMON_SETTINGS_BASE()
{
	// Disconnect Events
	m_textEditorBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnTextEditorClick ), NULL, this );
	m_defaultPDFViewer->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnRadioButtonPdfViewer ), NULL, this );
	m_otherPDFViewer->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnRadioButtonPdfViewer ), NULL, this );
	m_pdfViewerBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnPDFViewerClick ), NULL, this );
	m_canvasScaleAuto->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnCanvasScaleAuto ), NULL, this );

}
