/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2010-2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 *  lang_xx_xpm[];      // Icons of various national flags
 *  show_3d_xpm[];      // 3D icon
 *  edit_module_xpm[];
 *  icon_kicad_xpm[];   // Icon of the application
 */
#include <bitmaps.h>
#include <build_version.h>
#include <common.h>
#include <pgm_base.h>
#include <wxstruct.h>

#include "aboutinfo.h"
#include "dialog_about.h"


WX_DEFINE_OBJARRAY( Contributors )

// Helper functions:
static wxString HtmlHyperlink( const wxString& url, const wxString& description = wxEmptyString );
static wxString HtmlNewline( const unsigned int amount = 1 );


/**
 * Initializes the <code>AboutAppInfo</code> object with application specific information.
 *
 * This the object which holds all information about the application
 */
static void InitKiCadAboutNew( AboutAppInfo& info )
{
    // Set application specific icon
    const wxTopLevelWindow* const tlw = wxDynamicCast( Pgm().App().GetTopWindow(),
                                                       wxTopLevelWindow );

    if( tlw )
        info.SetIcon( tlw->GetIcon() );
    else
    {
        wxBitmap    bitmap = KiBitmap( icon_kicad_xpm  );
        wxIcon      icon;

        icon.CopyFromBitmap( bitmap );

        info.SetIcon( icon );
    }

    /* Set title */
    info.SetAppName( Pgm().App().GetAppName() );

    /* Copyright information */
    info.SetCopyright( wxT( "(C) 1992-2016 KiCad Developers Team" ) );

    /* KiCad build version */
    wxString version;
    version << GetBuildVersion()
#ifdef DEBUG
            << wxT( ", debug" )
#else
            << wxT( ", release" )
#endif
            << wxT( " build" );

    info.SetBuildVersion( version );

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
    libVersion << wxT( "and Boost " ) << ( BOOST_VERSION / 100000 ) << wxT( "." )
               << ( BOOST_VERSION / 100 % 1000 ) << wxT( "." ) << ( BOOST_VERSION % 100 )
               << wxT( "\n" );

    // Operating System Information

    wxPlatformInfo platformInfo;

    libVersion << wxT( "Platform: " ) << wxGetOsDescription() << wxT( ", " )
               << platformInfo.GetArchName();

    info.SetLibVersion( libVersion );


    /* info/description part HTML formatted */

    wxString description;

    /* short description */
    description << wxT( "<p>" );
    description << wxT( "<b><u>" )
                << _( "Description" )
                << wxT( "</u></b>" ); // bold & underlined font for caption

    description << wxT( "<p>" )
                << _( "The KiCad EDA Suite is a set of open source applications for the "
                      "creation of electronic schematics and to design printed circuit boards." )
                << wxT( "</p>" );

    description << wxT( "</p>" );

    /* websites */
    description << wxT( "<p><b><u>" )
                << _( "KiCad on the web" )
                << wxT( "</u></b>" ); // bold & underlined font for caption

    // bullet-ed list with some http links
    description << wxT( "<ul>" );
    description << wxT( "<li>" )
                << HtmlHyperlink( wxT( "http://www.kicad-pcb.org" ),
                                  _( "The official KiCad site" ) )
                << wxT( "</li>" );
    description << wxT( "<li>" )
                << HtmlHyperlink( wxT( "https://launchpad.net/kicad" ),
                                  _( "Developer's website on Launchpad" ) )
                << wxT("</li>" );
    description << wxT( "<li>" )
                << HtmlHyperlink( wxT( "https://github.com/KiCad/" ),
                                  _( "Our official Repository for component and footprint libraries" ) )
                << wxT( "</li>" );

    description << wxT( "<p><u>" )
                << _( "Non official repositories" )
                << wxT( "</u>" );
    description << wxT( "<li>" )
                << HtmlHyperlink( wxT( "http://www.kicadlib.org" ),
                                  _( "Additional component libraries repository (kicadlib)" ) )
                << wxT( "</li>" );
    description << wxT( "<li>" )
                << HtmlHyperlink( wxT( "http://smisioto.no-ip.org/elettronica/kicad/kicad-en.htm" ),
                                  _( "Additional component libraries repository (smisioto)" ) )
                << wxT( "</li>" );
    description << wxT( "</ul>" );
    description << wxT( "</p>" );

    description << wxT( "<p><b><u>" )
                << _( "Bug tracker" )
                << wxT( "</u></b>" ); // bold & underlined font caption

    // bullet-ed list with some http links
    description << wxT( "<ul>" );
    description << wxT( "<li>" )
                <<HtmlHyperlink( wxT( "https://bugs.launchpad.net/kicad/+bugs?orderby=-id&start=0" ),
                                 _( "Report or examine bugs" ) )
                << wxT( "</li>" );
    description << wxT( "</ul></p>" );

    description << wxT( "<p><b><u>" )
                << _( "KiCad user group and community" )
                << wxT( "</u></b>" ); // bold & underlined font caption

    description << wxT( "<ul>" );
    description << wxT( "<li>" )
                << HtmlHyperlink( wxT( "https://groups.yahoo.com/neo/groups/kicad-users/info" ),
                                  _( "KiCad user group" ) )
                << wxT( "</li>" );

    description << wxT( "<li>" )
                << HtmlHyperlink( wxT( "https://forum.kicad.info" ),
                                  _( "KiCad forum" ) )
                << wxT( "</li>" );

    description << wxT( "</ul></p>" );

    info.SetDescription( description );


    // License information also HTML formatted:
    wxString license;
    license
        << wxT( "<div align='center'>" )
        << HtmlNewline( 4 )
        << _( "The complete KiCad EDA Suite is released under the" ) << HtmlNewline( 2 )
        << HtmlHyperlink( wxT( "http://www.gnu.org/licenses" ),
                          _( "GNU General Public License (GPL) version 3 or any later version" ) )
        << wxT( "</div>" );

    info.SetLicense( license );


    /* A contributor consists of the following information:
     * Mandatory:
     * - Name
     * - EMail address
     * Optional:
     * - Category
     * - Category specific icon
     *
     * All contributors of the same category will be enumerated under this category
     * which should be represented by the same icon.
     */

    // The core developers
    info.AddDeveloper( new Contributor( wxT( "Jean-Pierre Charras" ),
                                        wxT( "jp.charras@wanadoo.fr" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Dick Hollenbeck" ),
                                        wxT( "dick@softplc.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Wayne Stambaugh" ),
                                        wxT( "stambaughw@gmail.com" ) ) );

    // alphabetically by last name after main 3 above:
    info.AddDeveloper( new Contributor( wxT( "Frank Bennett" ),
                                        wxT( "bennett78@lpbroadband.net" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Cirilo Bernardo" ),
                                        wxT( "cirilo_bernardo@yahoo.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Jonas Diemer" ),
                                        wxT( "diemer@gmx.de" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Torsten Hüter" ),
                                        wxT( "torstenhtr@gmx.de" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Jerry Jacobs" ),
                                        wxT( "xor.gate.engineering@gmail.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Mario Luzeiro" ),
                                        wxT( "mrluzeiro@ua.pt" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Daniel Majewski" ),
                                        wxT( "lordblick@gmail.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Lorenzo Marcantonio" ),
                                        wxT( "lomarcan@tin.it" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Marco Mattila" ),
                                        wxT( "marcom99@gmail.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Chris Pavlina" ),
                                        wxT( "pavlina.chris@gmail.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Miguel Angel Ajo Pelayo" ),
                                        wxT( "miguelangel@nbee.es" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Jacobo Aragunde Perez" ),
                                        wxT( "jaragunde@igalia.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Simon Richter" ),
                                        wxT( "Simon.Richter@hogyros.de" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Mark Roszko" ),
                                        wxT( "mark.roszko@gmail.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Marco Serantoni" ),
                                        wxT( "marco.serantoni@gmail.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Brian Sidebotham" ),
                                        wxT( "brian.sidebotham@gmail.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Mateusz Skowroński" ),
                                        wxT( "skowri@gmail.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Rafael Sokolowski" ),
                                        wxT( "rafael.sokolowski@web.de" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Vesa Solonen" ),
                                        wxT( "vesa.solonen@hut.fi" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Bernhard Stegmaier" ),
                                        wxT( "stegmaier@sw-systems.de" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Orson (Maciej Sumiński)" ),
                                        wxT( "maciej.suminski@cern.ch" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Tomasz Wlostowski" ),
                                        wxT( "tomasz.wlostowski@cern.ch" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Adam Wolf" ),
                                        wxT( "adamwolf@feelslikeburning.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Alexander Zakamaldin" ),
                                        wxT( "zaka62@mail.ru" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Henner Zeller" ),
                                        wxT( "h.zeller@acm.org" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Andrew Zonenberg" ),
                                        wxT( "azonenberg@drawersteak.com" ) ) );
    info.AddDeveloper( new Contributor( wxT( "Nick Østergaard" ),
                                        wxT( "oe.nick@gmail.com" ) ) );

    // The document writers
    info.AddDocWriter( new Contributor( wxT( "Jean-Pierre Charras" ),
                                        wxT( "jp.charras@wanadoo.fr" ) ) );
    info.AddDocWriter( new Contributor( wxT( "Marco Ciampa" ),
                                        wxT( "ciampix@libero.it" ) ) );
    info.AddDocWriter( new Contributor( wxT( "Dick Hollenbeck" ),
                                        wxT( "dick@softplc.com" ) ) );
    info.AddDocWriter( new Contributor( wxT( "Igor Plyatov" ),
                                        wxT( "plyatov@gmail.com" ) ) );
    info.AddDocWriter( new Contributor( wxT( "Wayne Stambaugh" ),
                                        wxT( "stambaughw@gmail.com" ) ) );
    info.AddDocWriter( new Contributor( wxT( "Fabrizio Tappero" ),
                                        wxT( "fabrizio.tappero@gmail.com" ) ) );

    /* The translators
     * As category the language to which the translation was done is used
     * and as icon the national flag of the corresponding country.
     */
    info.AddTranslator( new Contributor( wxT( "Robert Buj" ),
                                         wxT( "rbuj@fedoraproject.org" ),
                                         wxT( "Catalan (CA)" ),
                                         KiBitmapNew( lang_catalan_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Martin Kratoška" ),
                                         wxT( "martin@ok1rr.com" ),
                                         wxT( "Czech (CZ)" ),
                                         KiBitmapNew( lang_cs_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Jerry Jacobs" ),
                                         wxT( "xor.gate.engineering@gmail.com" ),
                                         wxT( "Dutch (NL)" ),
                                         KiBitmapNew( lang_nl_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Vesa Solonen" ),
                                         wxT( "vesa.solonen@hut.fi" ),
                                         wxT( "Finnish (FI)" ),
                                         KiBitmapNew( lang_fi_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Jean-Pierre Charras" ),
                                         wxT( "jp.charras@wanadoo.fr" ),
                                         wxT( "French (FR)" ),
                                         KiBitmapNew( lang_fr_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Mateusz Skowroński" ),
                                         wxT( "skowri@gmail.com" ),
                                         wxT( "Polish (PL)" ),
                                         KiBitmapNew( lang_pl_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Kerusey Karyu" ),
                                         wxT( "keruseykaryu@o2.pl" ),
                                         wxT( "Polish (PL)" ),
                                         KiBitmapNew( lang_pl_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Renie Marquet" ),
                                         wxT( "reniemarquet@uol.com.br" ),
                                         wxT( "Portuguese (PT)" ),
                                         KiBitmapNew( lang_pt_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Igor Plyatov" ),
                                         wxT( "plyatov@gmail.com" ),
                                         wxT( "Russian (RU)" ),
                                         KiBitmapNew( lang_ru_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Andrey Fedorushkov" ),
                                         wxT( "andrf@mail.ru" ),
                                         wxT( "Russian (RU)" ),
                                         KiBitmapNew( lang_ru_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Eldar Khayrullin" ),
                                         wxT( "eldar.khayrullin@mail.ru" ),
                                         wxT( "Russian (RU)" ),
                                         KiBitmapNew( lang_ru_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Pedro Martin del Valle" ),
                                         wxT( "pkicad@yahoo.es" ),
                                         wxT( "Spanish (ES)" ),
                                         KiBitmapNew( lang_es_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Iñigo Zuluaga" ),
                                         wxT( "inigo_zuluaga@yahoo.es" ),
                                         wxT( "Spanish (ES)" ),
                                         KiBitmapNew( lang_es_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Iñigo Figuero" ),
                                         wxT( "ifs@elektroquark.com" ),
                                         wxT( "Spanish (ES)" ),
                                         KiBitmapNew( lang_es_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Rafael Sokolowski" ),
                                         wxT( "rafael.sokolowski@web.de" ),
                                         wxT( "German (DE)" ),
                                         KiBitmapNew( lang_de_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Kenta Yonekura" ),
                                         wxT( "yoneken@kicad.jp" ),
                                         wxT( "Japanese (JA)" ),
                                         KiBitmapNew( lang_jp_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Manolis Stefanis" ),
                                         wxT( "" ),
                                         wxT( "Greek (el_GR)" ),
                                         KiBitmapNew( lang_gr_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Athanasios Vlastos" ),
                                         wxT( "" ),
                                         wxT( "Greek (el_GR)" ),
                                         KiBitmapNew( lang_gr_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Milonas Kostas" ),
                                         wxT( "milonas.ko@gmail.com" ),
                                         wxT( "Greek (el_GR)" ),
                                         KiBitmapNew( lang_gr_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Michail Misirlis" ),
                                         wxT( "mmisirlis@gmail.com" ),
                                         wxT( "Greek (el_GR)" ),
                                         KiBitmapNew( lang_gr_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Massimo Cioce" ),
                                         wxT( "ciocemax@alice.it" ),
                                         wxT( "Italian (IT)" ),
                                         KiBitmapNew( lang_it_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Marco Ciampa" ),
                                         wxT( "ciampix@libero.it" ),
                                         wxT( "Italian (IT)" ),
                                         KiBitmapNew( lang_it_xpm ) ) );
    info.AddTranslator( new Contributor( wxT( "Evgeniy Ivanov" ),
                                         wxT( "evgeniy_p_ivanov@yahoo.ca" ),
                                         wxT( "Bulgarian (BG)" ),
                                         KiBitmapNew( lang_bg_xpm ) ) );

    // Maintainer who helper in translations, but not in a specific translation
    #define OTHERS_IN_TRANSLATION _( "Others" )
    info.AddTranslator( new Contributor( wxT( "Remy Halvick" ),
                                         wxEmptyString,
                                         OTHERS_IN_TRANSLATION ) );
    info.AddTranslator( new Contributor( wxT( "David Briscoe" ),
                                         wxEmptyString,
                                         OTHERS_IN_TRANSLATION ) );
    info.AddTranslator( new Contributor( wxT( "Dominique Laigle" ),
                                         wxEmptyString,
                                         OTHERS_IN_TRANSLATION ) );
    info.AddTranslator( new Contributor( wxT( "Paul Burke" ),
                                         wxEmptyString,
                                         OTHERS_IN_TRANSLATION ) );

    // Programm credits for icons
    #define ICON_CONTRIBUTION _( "Icons by" )
    info.AddArtist( new Contributor( wxT( "Iñigo Zuluaga" ),
                                     wxT( "inigo_zuluaga@yahoo.es" ),
                                     ICON_CONTRIBUTION,
                                     KiBitmapNew( edit_module_xpm ) ) );
    info.AddArtist( new Contributor( wxT( "Konstantin Baranovskiy" ),
                                     wxT( "baranovskiykonstantin@gmail.com" ),
                                     ICON_CONTRIBUTION,
                                     KiBitmapNew( edit_module_xpm ) ) );
    info.AddArtist( new Contributor( wxT( "Fabrizio Tappero" ),
                                     wxT( "fabrizio.tappero@gmail.com" ),
                                     ICON_CONTRIBUTION,
                                     KiBitmapNew( edit_module_xpm ) ) );

    // Programm credits for 3d models
    #define MODELS_3D_CONTRIBUTION _( "3D models by" )
    info.AddArtist( new Contributor( wxT( "Christophe Boschat" ),
                                     wxT( "nox454@hotmail.fr" ),
                                     MODELS_3D_CONTRIBUTION,
                                     KiBitmapNew( three_d_xpm ) ) );
    info.AddArtist( new Contributor( wxT( "Renie Marquet" ),
                                     wxT( "reniemarquet@uol.com.br" ),
                                     MODELS_3D_CONTRIBUTION,
                                     KiBitmapNew( three_d_xpm ) ) );

    // Programm credits for package developers.
    info.AddPackager( new Contributor( wxT( "Jean-Samuel Reynaud" ),
                                       wxT( "js.reynaud@gmail.com" ) ) );
    info.AddPackager( new Contributor( wxT( "Bernhard Stegmaier" ),
                                       wxT( "stegmaier@sw-systems.de" ) ) );
    info.AddPackager( new Contributor( wxT( "Adam Wolf" ),
                                       wxT( "adamwolf@feelslikeburning.com" ) ) );
    info.AddPackager( new Contributor( wxT( "Nick Østergaard" ),
                                       wxT( "oe.nick@gmail.com" ) ) );
}


bool ShowAboutDialog( wxWindow* parent )
{
    AboutAppInfo info;

    InitKiCadAboutNew( info );

    dialog_about* dlg = new dialog_about( parent, info );
    dlg->SetIcon( info.GetIcon() );
    dlg->Show();

    return true;
}


///////////////////////////////////////////////////////////////////////////////
/// Helper functions
///////////////////////////////////////////////////////////////////////////////

/**
 * Function HtmlHyperlink
 *
 * wraps \a aUrl with a HTML anchor tag containing a hyperlink text reference
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
        hyperlink << wxT( "<a href='" ) << aUrl << wxT( "'>" ) << aUrl << wxT( "</a>" );
    else
        hyperlink << wxT( "<a href='" ) << aUrl << wxT( "'>" ) << aDescription << wxT( "</a>" );

    return hyperlink;
}


/**
 * Function HtmlNewline
 *
 * creates an HTML newline character sequence of \a aCount.
 *
 * @param aCount the number of HTML newline tags to concatenate, default is to return just
 *               one <br> tag.
 * @return the concatenated amount of HTML newline tag(s) <br>
 */
static wxString HtmlNewline( const unsigned int aCount )
{
    wxString newlineTags = wxEmptyString;

    for( size_t i = 0; i<aCount; ++i )
        newlineTags << wxT( "<br>" );

    return newlineTags;
}
