/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2010-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 *  show_3d_xpm;      // 3D icon
 *  module_xpm;
 *  icon_kicad_xpm;   // Icon of the application
 */
#include <bitmaps.h>
#include <build_version.h>
#include <common.h>
#include <kiplatform/app.h>
#include <pgm_base.h>
#include <eda_base_frame.h>

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
    aInfo.SetCopyright( "(C) 1992-2021 KiCad Developers Team" );

    /* KiCad build version */
    wxString version;
    version << ( KIPLATFORM::APP::IsOperatingSystemUnsupported() ? "(UNSUPPORTED)"
                                                                 : GetBuildVersion() )
#ifdef DEBUG
            << ", debug"
#else
            << ", release"
#endif
            << " build";

    aInfo.SetBuildVersion( version );
    aInfo.SetBuildDate( GetBuildDate() );

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

    libVersion << "Platform: " << wxGetOsDescription() << ", "
// TODO (ISM): Readd conditional once our wx fork and flatpaks are running released 3.1.5
#if 0 && wxCHECK_VERSION( 3, 1, 5 )
    << platformInfo.GetBitnessName();
#else
    << platformInfo.GetArchName();
#endif

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
                << _( "The official KiCad website - " )
                << HtmlHyperlink( "http://www.kicad.org" )
                << "</li>";
    description << "<li>"
                << _( "Developer website - " )
                << HtmlHyperlink( "https://go.kicad.org/dev" )
                << "</li>";

    description << "<li>"
                << _("Official KiCad library repositories - " )
                << HtmlHyperlink( "https://go.kicad.org/libraries" )
                << "</li>";

    description << "</ul></p>";

    description << "<p><b><u>"
                << _( "Bug tracker" )
                << "</u></b>"; // bold & underlined font caption

    // bullet-ed list with some http links
    description << "<ul>";
    description << "<li>"
                << _( "Report or examine bugs - " )
                << HtmlHyperlink( "https://go.kicad.org/bugs" )
                << "</li>";
    description << "</ul></p>";

    description << "<p><b><u>"
                << _( "KiCad users group and community" )
                << "</u></b>"; // bold & underlined font caption

    description << "<ul>";
    description << "<li>"
                << _( "KiCad forum - " )
                << HtmlHyperlink( "https://go.kicad.org/forum" )
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
#define LEAD_DEV _( "Lead Development Team" )
#define FORMER_DEV _( "Lead Development Alumni" )
#define CONTRIB_DEV _( "Additional Contributions By")
    aInfo.AddDeveloper( new CONTRIBUTOR( "Jean-Pierre Charras", LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Wayne Stambaugh", LEAD_DEV, nullptr ) );

    // Alphabetical after the first two
    aInfo.AddDeveloper( new CONTRIBUTOR( "Jon Evans", LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Seth Hillbrand", LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Ian McInerney", LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Orson (Maciej Sumiński)" ), LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Mark Roszko", LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Thomas Pointhuber", LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Tomasz Wlostowski", LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( "Jeff Young", LEAD_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "John Beard" ), FORMER_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dick Hollenbeck" ), FORMER_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexis Lockwood" ), FORMER_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Brian Sidebotham" ), FORMER_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Martin Aberg" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Rohan Agrawal" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Johannes Agricola" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Nabeel Ahmad" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Werner Almesberger" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Shawn Anastasio" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Collin Anderson" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Tom Andrews" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mikael Arguedas" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Lachlan Audas" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jean-Noel Avila" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Pascal Baerten" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Konstantin Baranovskiy" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Roman Bashkov" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Roberto Fernandez Bautista" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michael Beardsworth" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Matthew Beckler" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Konrad Beckmann" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "David Beinder" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Frank Bennett" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Roman Beranek" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Gustav Bergquist" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Cirilo Bernardo" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Joël Bertrand" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andreas Beutling" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Brian F. G. Bidulock" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Anton Blanchard" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Blair Bonnett" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Stefan Brüns" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andreas Buhr" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ryan Bunch" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Emery Burhan" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Phinitnan Chanasabaeng" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Shivpratap Chauhan" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kevin Cozens" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Joseph Y. Chen" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexey Chernov" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marco Ciampa" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marcus Comstedt" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Diogo Condeco" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Colin Cooper" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Fabien Corona" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Garth Corral" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kevin Cozens" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dan Cross" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jonas Diemer" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andrew D'Addesio" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Martin d'Allens" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Camille Delbegue" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ruben De Smet" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ben Dooks" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andrew Downing" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jan Dubiec" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Gerd Egidy" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jean Philippe Eimer" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ben Ellis" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Oleg Endo" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Damien Espitallier" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Paul Ewing" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andrey Fedorushkov" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Julian Fellinger" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Joe Ferner" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Thomas Figueroa" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Drew Fustini" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ronnie Gaensli" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Christian Gagneraud" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ben Gamari" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ashutosh Gangwar" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alessandro Gatti" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Hal Gentz" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michael Geselbracht" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexander Golubev" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Angus Gratton" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Element Green" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mathias Grimmberger" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Hildo Guillardi Júnior" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Niki Guldbrand" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jonathan Haas" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Stefan Hamminga" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ben Harris" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Lukas F. Hartmann" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Aylons Hazzud" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Stefan Helmert" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Hartmut Henkel" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Brian Henning" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Paulo Henrique Silva" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Hans Henry von Tresckow" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Diego Herranz" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marco Hess" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mario Hros" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Matt Huszagh" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Torsten Hüter" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "José Ignacio Romero" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marco Inacio" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kinichiro Inoguchi" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Fabián Inostroza" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Vlad Ivanov" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jerry Jacobs" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Christian Jacobsen" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michal Jahelka" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Martin Janitschke" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jonathan Jara-Almonte" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Gilbert J.M. Forkel" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "José Jorge Enríquez" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Franck Jullien" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Eeli Kaikkonen" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Lajos Kamocsay" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Povilas Kanapickas" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mikhail Karpenko" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kerusey Karyu" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michael Kavanagh" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Graham Keeth" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Yury Khalyavin" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Eldar Khayrullin" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ingo Kletti" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Sylwester Kocjan" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Clemens Koller" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jakub Kozdon" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Simon Kueppers" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Martijn Kuipers" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Robbert Lagerweij" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dimitris Lampridis" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kevin Lannen" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ludovic Léau-mercier" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Paul LeoNerd Evens" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jonatan Liljedahl" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexander Lunev" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mario Luzeiro" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Johannes Maibaum" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mateusz Majchrzycki" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Daniel Majewski" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Lorenzo Marcantonio" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marco Mattila" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Maui" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kirill Mavreshko" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Miles McCoo" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Charles McDowell" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Moses McKnight" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Martin McNamara" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ievgenii Meshcheriakov" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ashley Mills" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Peter Montgomery" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alejandro García Montoro" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Felix Morgner" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jan Mrázek" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michael Narigon" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jon Neal" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Bastian Neumann" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kristian Nielsen" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Henrik Nyberg" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kristoffer Ödmark" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Russell Oliver" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jason Oster" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Miguel Angel Ajo Pelayo" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Patrick Pereira" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jacobo Aragunde Perez" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Matthew Petroff" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Johannes Pfister" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Brian Piccioni" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Nicolas Planel" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Carl Poirier" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Reece Pollack" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alain Portal" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andrei Pozolotin" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Antia Puentes" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Heikki Pulkkinen" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Morgan Quigley" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Urja Rannikko" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Joshua Redstone" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michele Renda" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jean-Samuel Reynaud" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dmitry Rezvanov" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Simon Richter" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Christoph Riehl" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Thiadmer Riemersma" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Gregor Riepl" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Lubomir Rintel" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Érico Rolim" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Heiko Rosemann" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Fabio Rossi" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ian Roth" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "J. Morio Sakaguchi" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ross Schlaikjer" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Julius Schmidt" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marvin Schmidt" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Carsten Schoenert" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Simon Schubert" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Adrian Scripca" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marco Serantoni" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Frank Severinsen" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Cheng Sheng" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexander Shuklin" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marco Serantoni" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Guillaume Simard" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Martin Sivak" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mateusz Skowroński" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Blake Smith" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Rafael Sokolowski" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Vesa Solonen" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ronald Sousa" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Craig Southeren" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Thomas Spindler" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Seppe Stas" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Bernhard Stegmaier" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michael Steinberg" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marco Sterbik" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexander Stock" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Martin Stoilov" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Hiroki Suenaga" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Karl Thorén" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Vladimir Ur" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Matthias Urlichs" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Vladimir Uryvaev" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Henri Valta" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dave Vandenbout" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Edwin van den Oetelaar" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mark van Doesburg" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Fabio Varesano" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Benjamin Vernoux" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Frank Villaro-Dixon" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Forrest Voight" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Tormod Volden" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Johannes Wågen" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Oliver Walters" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jonathan Warner" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dan Weatherill" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Simon Wells" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dominik Wernberger" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mikołaj Wielgus" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Nick Winters" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Adam Wolf" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andrzej Wolski" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Damian Wrobel" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andrew Wygle" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jiaxun Yang" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Robert Yates" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Yegor Yefremov" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kenta Yonekura" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexander Zakamaldin" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Henner Zeller" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andrew Zonenberg" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Karl Zeilhofer" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kevin Zheng" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Nick Østergaard" ), CONTRIB_DEV, nullptr ) );

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
                                          wxEmptyString,
                                          "Catalan (CA)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Martin Kratoška" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Czech (CZ)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Jerry Jacobs",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Dutch (NL)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Vesa Solonen",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Finnish (FI)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Jean-Pierre Charras",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "French (FR)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Mateusz Skowroński" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Polish (PL)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Kerusey Karyu",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Polish (PL)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Renie Marquet",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Portuguese (PT)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Igor Plyatov",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Russian (RU)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Andrey Fedorushkov",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Russian (RU)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Eldar Khayrullin",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Russian (RU)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Konstantin Baranovskiy",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Russian (RU)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Pedro Martin del Valle",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Spanish (ES)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Iñigo Zuluaga" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Spanish (ES)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Iñigo Figuero" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Spanish (ES)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Jonathan Haas",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "German (DE)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Rafael Sokolowski",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "German (DE)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Hiroshi Tokita",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Japanese (JA)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Kenta Yonekura",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Japanese (JA)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Manolis Stefanis",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Greek (el_GR)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Athanasios Vlastos",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Greek (el_GR)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Milonas Kostas",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Greek (el_GR)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Michail Misirlis",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Greek (el_GR)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Massimo Cioce",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Italian (IT)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Marco Ciampa",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Italian (IT)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Evgeniy Ivanov",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Bulgarian (BG)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Liu Guang",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Simplified Chinese (zh_CN)" ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Taotieren",
                                          wxEmptyString,
                                          wxEmptyString,
                                          "Simplified Chinese (zh_CN)" ) );

    // Maintainer who helper in translations, but not in a specific translation
    #define OTHERS_IN_TRANSLATION _( "Others" )
    aInfo.AddTranslator( new CONTRIBUTOR( "Remy Halvick",
                                          wxEmptyString,
                                          wxEmptyString,
                                          OTHERS_IN_TRANSLATION ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "David J S Briscoe",
                                          wxEmptyString,
                                          wxEmptyString,
                                          OTHERS_IN_TRANSLATION ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Dominique Laigle",
                                          wxEmptyString,
                                          wxEmptyString,
                                          OTHERS_IN_TRANSLATION ) );
    aInfo.AddTranslator( new CONTRIBUTOR( "Paul Burke",
                                          wxEmptyString,
                                          wxEmptyString,
                                          OTHERS_IN_TRANSLATION ) );


    // Program credits for 3d models
    #define LIBRARIANS _( "KiCad Librarian Team" )
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Christian Schlüter"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Rene Poeschl"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Antonio Vázquez Blanco "), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "cpresser"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Daniel Giesbrecht"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Otavio Augusto Gomes"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "herostrat"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Diego Herranz"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Joel Guittet"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Aristeidis Kimirtzis"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Chris Morgan"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Thomas Pointhuber"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Evan Shultz"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );

    #define MODELS_3D_CONTRIBUTION _( "3D models by" )
    aInfo.AddLibrarian( new CONTRIBUTOR( "Scripts by Maui",
                                      "https://github.com/easyw",
                                      "https://gitlab.com/kicad/libraries/kicad-packages3D-generator",
                                      MODELS_3D_CONTRIBUTION,
                                      aInfo.CreateKiBitmap( BITMAPS::three_d ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( "GitLab contributors",
                                      wxEmptyString,
                                      "https://gitlab.com/kicad/libraries/kicad-packages3D/-/graphs/master",
                                      MODELS_3D_CONTRIBUTION,
                                      aInfo.CreateKiBitmap( BITMAPS::three_d ) ) );

    #define SYMBOL_LIB_CONTRIBUTION _( "Symbols by" )
    aInfo.AddLibrarian( new CONTRIBUTOR( "GitLab contributors",
                                      wxEmptyString,
                                      "https://gitlab.com/kicad/libraries/kicad-symbols/-/graphs/master",
                                      SYMBOL_LIB_CONTRIBUTION,
                                      aInfo.CreateKiBitmap( BITMAPS::add_component ) ) );

    #define FOOTPRINT_LIB_CONTRIBUTION _( "Footprints by" )
    aInfo.AddLibrarian( new CONTRIBUTOR( "Scripts by Thomas Pointhuber",
                                      wxEmptyString,
                                      "https://gitlab.com/kicad/libraries/kicad-footprint-generator",
                                      FOOTPRINT_LIB_CONTRIBUTION,
                                      aInfo.CreateKiBitmap( BITMAPS::module ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( "GitLab contributors",
                                      wxEmptyString,
                                      "https://gitlab.com/kicad/libraries/kicad-footprints/-/graphs/master",
                                      FOOTPRINT_LIB_CONTRIBUTION,
                                      aInfo.CreateKiBitmap( BITMAPS::module ) ) );

    // Program credits for icons
    #define ICON_CONTRIBUTION _( "Icons by" )
    aInfo.AddArtist( new CONTRIBUTOR( wxT( "Aleksandr Zyrianov" ),
                                      wxEmptyString,
                                      wxEmptyString,
                                      ICON_CONTRIBUTION ) );
    aInfo.AddArtist( new CONTRIBUTOR( wxT( "Iñigo Zuluaga" ),
                                      wxEmptyString,
                                      wxEmptyString,
                                      ICON_CONTRIBUTION ) );
    aInfo.AddArtist( new CONTRIBUTOR( "Fabrizio Tappero",
                                      wxEmptyString,
                                      wxEmptyString,
                                      ICON_CONTRIBUTION ) );

    // Program credits for package developers.
    aInfo.AddPackager( new CONTRIBUTOR( "Steven Falco" ) );
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
