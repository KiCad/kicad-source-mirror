/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2010-2017 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/version.hpp>
#include <wx/aboutdlg.h>
#include <wx/arrimpl.cpp>
#include <wx/textctrl.h>
#include <wx/utils.h>

/* Used icons:
 *  lang_xx_xpm;      // Icons of various national flags
 *  show_3d_xpm;      // 3D icon
 *  edit_module_xpm;
 *  icon_kicad_xpm;   // Icon of the application
 */
#include <bitmaps.h>
#include <build_version.h>
#include <common.h>
#include <pgm_base.h>
#include <wxstruct.h>

#include "aboutinfo.h"
#include "dialog_about.h"


WX_DEFINE_OBJARRAY( CONTRIBUTORS )

// Helper functions:
static wxString HtmlHyperlink( const wxString& url, const wxString& description = wxEmptyString );
static wxString HtmlNewline( const unsigned int amount = 1 );


/**
 * Initializes the <code>ABOUT_APP_INFO</code> object with application specific information.
 * This is the object which holds all information about the application
 */
static void buildKicadAboutBanner( EDA_BASE_FRAME* aParent, ABOUT_APP_INFO& aInfo )
{
    // Set application specific icon
    aInfo.SetAppIcon( aParent->GetIcon() );

    /* Set title */
    aInfo.SetAppName( Pgm().App().GetAppName() );

    /* Copyright information */
    aInfo.SetCopyright( "(C) 1992-2017 KiCad Developers Team" );

    /* KiCad build version */
    wxString version;
    version << GetBuildVersion()
#ifdef DEBUG
            << ", debug"
#else
            << ", release"
#endif
            << " build";

    aInfo.SetBuildVersion( version );

    /* wxWidgets version */
    wxString libVersion;
    libVersion << wxGetLibraryVersionInfo().GetVersionString();

    /* Unicode or ANSI version */
#if wxUSE_UNICODE
    libVersion << wxT( " Unicode " );
#else
    libVersion << wxT( " ANSI " );
#endif

    // Just in case someone builds KiCad with the platform native of Boost instead of
    // the version included with the KiCad source.
    libVersion << "and Boost " << ( BOOST_VERSION / 100000 ) << "."
               << ( BOOST_VERSION / 100 % 1000 ) << "." << ( BOOST_VERSION % 100 )
               << "\n";

    // Operating System Information

    wxPlatformInfo platformInfo;

    libVersion << "Platform: " << wxGetOsDescription() << ", " << platformInfo.GetArchName();

    aInfo.SetLibVersion( libVersion );


    /* info/description part HTML formatted */

    wxString description;

    /* short description */
    description << "<p>";
    description << "<b><u>"
                << _( "Description" )
                << "</u></b>"; // bold & underlined font for caption

    description << "<p>"
                << _( "The KiCad EDA Suite is a set of open source applications for the "
                      "creation of electronic schematics and printed circuit boards." )
                << "</p>";

    description << "</p>";

    /* websites */
    description << "<p><b><u>"
                << _( "KiCad on the web" )
                << "</u></b>"; // bold & underlined font for caption

    // bullet-ed list with some http links
    description << "<ul>";
    description << "<li>"
                << "The official KiCad website - "
                << HtmlHyperlink( "http://www.kicad-pcb.org" )
                << "</li>";
    description << "<li>"
                << "Developer website on Launchpad - "
                << HtmlHyperlink( "https://launchpad.net/kicad" )
                << "</li>";

    description << "<li>"
                << "Official KiCad library repositories - "
                << HtmlHyperlink( "https://github.com/KiCad/" )
                << "</li>";

    description << "</ul></p>";

    description << "<p><b><u>"
                << _( "Bug tracker" )
                << "</u></b>"; // bold & underlined font caption

    // bullet-ed list with some http links
    description << "<ul>";
    description << "<li>"
                << "Report or examine bugs - "
                << HtmlHyperlink( "https://bugs.launchpad.net/kicad/+bugs?orderby=-id&start=0",
                                  "https://bugs.launchpad.net/kicad" )
                << "</li>";
    description << "</ul></p>";

    description << "<p><b><u>"
                << _( "KiCad user's groups and community" )
                << "</u></b>"; // bold & underlined font caption

    description << "<ul>";
    description << "<li>"
                << "KiCad forum - "
                << HtmlHyperlink( "https://forum.kicad.info" )
                << "</li>";

    description << "<li>"
                << "KiCad user's group - "
                << HtmlHyperlink( "https://groups.yahoo.com/neo/groups/kicad-users/info" )
                << "</li>";

    description << "</ul></p>";

    aInfo.SetDescription( description );


    // License information also HTML formatted:
    wxString license;
    license
        << "<div align='center'>"
        << HtmlNewline( 4 )
        << _( "The complete KiCad EDA Suite is released under the" ) << HtmlNewline( 2 )
        << HtmlHyperlink( "http://www.gnu.org/licenses",
                          _( "GNU General Public License (GPL) version 3 or any later version" ) )
        << "</div>";

    aInfo.SetLicense( license );


    /* A contributor consists of the following information:
     * Mandatory:
     * - Name
     * Optional:
     * - EMail address
     * - Category
     * - Category specific icon
     *
     * All contributors of the same category will be enumerated under this category
     * which should be represented by the same icon.
     */

    // The core developers
    aInfo.AddDeveloper( new CONTRIBUTOR( "Jean-Pierre Charras" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Dick Hollenbeck" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Wayne Stambaugh" ) );

    // alphabetically by last name after main 3 above:
    aInfo.AddDeveloper( new CONTRIBUTOR( "Frank Bennett" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Cirilo Bernardo" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Jonas Diemer" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Jon Evans" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Seth Hillbrand" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Torsten Hüter" ) ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Jerry Jacobs" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Mario Luzeiro" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Daniel Majewski" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Lorenzo Marcantonio" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Marco Mattila" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Russell Oliver" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Chris Pavlina" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Miguel Angel Ajo Pelayo" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Jacobo Aragunde Perez" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Simon Richter" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Mark Roszko" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Marco Serantoni" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Brian Sidebotham" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mateusz Skowroński" ) ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Rafael Sokolowski" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Vesa Solonen" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Bernhard Stegmaier" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Orson (Maciej Sumiński)" ) ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Oliver Walters" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Tomasz Wlostowski" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Adam Wolf" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Alexander Zakamaldin" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Henner Zeller" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Andrew Zonenberg" ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Nick Østergaard" ) ) );

    // The document writers
    aInfo.AddDocWriter( new CONTRIBUTOR( "Jean-Pierre Charras" ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( "Marco Ciampa" ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( "Dick Hollenbeck" ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( "Igor Plyatov" ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( "Wayne Stambaugh" ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( "Fabrizio Tappero" ) );

    /* The translators
     * As category the language to which the translation was done is used
     * and as icon the national flag of the corresponding country.
     */
    aInfo.AddTranslator( new CONTRIBUTOR( "Robert Buj",
                                          wxEmptyString,
                                          "Catalan (CA)",
                                          KiBitmapNew( lang_catalan_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Martin Kratoška" ),
                                          wxEmptyString,
                                          "Czech (CZ)",
                                          KiBitmapNew( lang_cs_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Jerry Jacobs",
                                          wxEmptyString,
                                          "Dutch (NL)",
                                          KiBitmapNew( lang_nl_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Vesa Solonen",
                                          wxEmptyString,
                                          "Finnish (FI)",
                                          KiBitmapNew( lang_fi_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Jean-Pierre Charras",
                                          wxEmptyString,
                                          "French (FR)",
                                          KiBitmapNew( lang_fr_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Mateusz Skowroński" ),
                                          wxEmptyString,
                                          "Polish (PL)",
                                          KiBitmapNew( lang_pl_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Kerusey Karyu",
                                          wxEmptyString,
                                          "Polish (PL)",
                                          KiBitmapNew( lang_pl_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Renie Marquet",
                                          wxEmptyString,
                                          "Portuguese (PT)",
                                          KiBitmapNew( lang_pt_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Igor Plyatov",
                                          wxEmptyString,
                                          "Russian (RU)",
                                          KiBitmapNew( lang_ru_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Andrey Fedorushkov",
                                          wxEmptyString,
                                          "Russian (RU)",
                                          KiBitmapNew( lang_ru_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Eldar Khayrullin",
                                          wxEmptyString,
                                          "Russian (RU)",
                                         KiBitmapNew( lang_ru_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Pedro Martin del Valle",
                                          wxEmptyString,
                                          "Spanish (ES)",
                                          KiBitmapNew( lang_es_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Iñigo Zuluaga" ),
                                          wxEmptyString,
                                          "Spanish (ES)",
                                          KiBitmapNew( lang_es_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Iñigo Figuero" ),
                                          wxEmptyString,
                                          "Spanish (ES)",
                                          KiBitmapNew( lang_es_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Rafael Sokolowski",
                                          wxEmptyString,
                                          "German (DE)",
                                          KiBitmapNew( lang_de_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Kenta Yonekura",
                                          wxEmptyString,
                                          "Japanese (JA)",
                                          KiBitmapNew( lang_jp_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Manolis Stefanis",
                                          wxEmptyString,
                                          "Greek (el_GR)",
                                          KiBitmapNew( lang_gr_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Athanasios Vlastos",
                                          wxEmptyString,
                                          "Greek (el_GR)",
                                          KiBitmapNew( lang_gr_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Milonas Kostas",
                                          wxEmptyString,
                                          "Greek (el_GR)",
                                          KiBitmapNew( lang_gr_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Michail Misirlis",
                                          wxEmptyString,
                                          "Greek (el_GR)",
                                          KiBitmapNew( lang_gr_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Massimo Cioce",
                                          wxEmptyString,
                                          "Italian (IT)",
                                          KiBitmapNew( lang_it_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Marco Ciampa",
                                          wxEmptyString,
                                          "Italian (IT)",
                                          KiBitmapNew( lang_it_xpm ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Evgeniy Ivanov",
                                          wxEmptyString,
                                          "Bulgarian (BG)",
                                          KiBitmapNew( lang_bg_xpm ) ) );

    // Maintainer who helper in translations, but not in a specific translation
    #define OTHERS_IN_TRANSLATION _( "Others" )
    aInfo.AddTranslator( new CONTRIBUTOR( "Remy Halvick",
                                          wxEmptyString,
                                          OTHERS_IN_TRANSLATION ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "David Briscoe",
                                          wxEmptyString,
                                          OTHERS_IN_TRANSLATION ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Dominique Laigle",
                                          wxEmptyString,
                                          OTHERS_IN_TRANSLATION ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Paul Burke",
                                          wxEmptyString,
                                          OTHERS_IN_TRANSLATION ) );

    // Programm credits for icons
    #define ICON_CONTRIBUTION _( "Icons by" )
    aInfo.AddArtist( new CONTRIBUTOR( wxT( "Iñigo Zuluaga" ),
                                      wxEmptyString,
                                      ICON_CONTRIBUTION,
                                      KiBitmapNew( svg_file_xpm ) ) );
    aInfo.AddArtist( new CONTRIBUTOR( "Konstantin Baranovskiy",
                                      wxEmptyString,
                                      ICON_CONTRIBUTION,
                                      KiBitmapNew( svg_file_xpm ) ) );
    aInfo.AddArtist( new CONTRIBUTOR( "Fabrizio Tappero",
                                      wxEmptyString,
                                      ICON_CONTRIBUTION,
                                      KiBitmapNew( svg_file_xpm ) ) );

    // Program credits for 3d models
    #define MODELS_3D_CONTRIBUTION _( "3D models by" )
    aInfo.AddArtist( new CONTRIBUTOR( "Christophe Boschat",
                                      wxEmptyString,
                                      MODELS_3D_CONTRIBUTION,
                                      KiBitmapNew( three_d_xpm ) ) );
    aInfo.AddArtist( new CONTRIBUTOR( "Renie Marquet",
                                      wxEmptyString,
                                      MODELS_3D_CONTRIBUTION,
                                      KiBitmapNew( three_d_xpm ) ) );

    #define SYMBOL_LIB_CONTRIBUTION _( "Symbols by" )
    aInfo.AddArtist( new CONTRIBUTOR( "Leet Hacker1",
                                      wxEmptyString,
                                      SYMBOL_LIB_CONTRIBUTION,
                                      KiBitmapNew( edit_component_xpm ) ) );

    #define FOOTPRINT_LIB_CONTRIBUTION _( "Footprints by" )
    aInfo.AddArtist( new CONTRIBUTOR( "Leet Hacker2",
                                      wxEmptyString,
                                      FOOTPRINT_LIB_CONTRIBUTION,
                                      KiBitmapNew( edit_module_xpm ) ) );

    // Program credits for package developers.
    aInfo.AddPackager( new CONTRIBUTOR( "Jean-Samuel Reynaud" ) );
    aInfo.AddPackager( new CONTRIBUTOR( "Bernhard Stegmaier" ) );
    aInfo.AddPackager( new CONTRIBUTOR( "Adam Wolf" ) );
    aInfo.AddPackager( new CONTRIBUTOR( wxT( "Nick Østergaard" ) ) );
}


void ShowAboutDialog( EDA_BASE_FRAME* aParent )
{
    ABOUT_APP_INFO info;
    buildKicadAboutBanner( aParent, info );

    DIALOG_ABOUT dlg( aParent, info );
    dlg.ShowModal();
}


///////////////////////////////////////////////////////////////////////////////
/// Helper functions
///////////////////////////////////////////////////////////////////////////////

/**
 * Wrap \a aUrl with a HTML anchor tag containing a hyperlink text reference
 * to form a HTML hyperlink.
 *
 * @param aUrl the url that will be embedded in an anchor tag containing a hyperlink reference
 * @param aDescription the optional describing text that will be represented as a hyperlink.
 *  If not specified the url will be used as hyperlink.
 * @return a HTML conform hyperlink like <a href='url'>description</a>
 */
static wxString HtmlHyperlink( const wxString& aUrl, const wxString& aDescription )
{
    wxString hyperlink = wxEmptyString;

    if( aDescription.IsEmpty() )
        hyperlink << "<a href='" << aUrl << "'>" << aUrl << "</a>";
    else
        hyperlink << "<a href='" << aUrl << "'>" << aDescription << "</a>";

    return hyperlink;
}


/**
 * Create an HTML newline character sequence of \a aCount.
 *
 * @param aCount the number of HTML newline tags to concatenate, default is to return just
 *               one <br> tag.
 * @return the concatenated amount of HTML newline tag(s) <br>
 */
static wxString HtmlNewline( const unsigned int aCount )
{
    wxString newlineTags = wxEmptyString;

    for( size_t i = 0; i<aCount; ++i )
        newlineTags << "<br>";

    return newlineTags;
}
