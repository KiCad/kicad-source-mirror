/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2010-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
    aInfo.SetCopyright( wxT( "(C) 1992-2023 KiCad Developers Team" ) );

    /* KiCad build version */
    wxString version;
    version << ( KIPLATFORM::APP::IsOperatingSystemUnsupported() ? wxString( wxS( "(UNSUPPORTED)" ) )
                                                                 : GetBuildVersion() )
#ifdef DEBUG
            << wxT( ", debug" )
#else
            << wxT( ", release" )
#endif
            << wxT( " build" );

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
    libVersion << wxT( "and Boost " ) << ( BOOST_VERSION / 100000 ) << wxT( "." )
               << ( BOOST_VERSION / 100 % 1000 ) << wxT( "." ) << ( BOOST_VERSION % 100 )
               << wxT( "\n" );

    // Operating System Information

    wxPlatformInfo platformInfo;

    libVersion << wxT( "Platform: " ) << wxGetOsDescription() << wxT( ", " )
               << GetPlatformGetBitnessName();

    aInfo.SetLibVersion( libVersion );

    // info/description part HTML formatted:
    wxString description;

    /* short description */
    description << wxT( "<p>" );
    description << wxT( "<b><u>" )
                << _( "Description" )
                << wxT( "</u></b>" ); // bold & underlined font for caption

    description << wxT( "<p>" )
                << _( "The KiCad EDA Suite is a set of open source applications for the "
                      "creation of electronic schematics and printed circuit boards." )
                << wxT( "</p>" );

    description << wxT( "</p>" );

    /* websites */
    description << wxT( "<p><b><u>" )
                << _( "KiCad on the web" )
                << wxT( "</u></b>" ); // bold & underlined font for caption

    // bullet-ed list with some http links
    description << wxT( "<ul>" );
    description << wxT( "<li>" )
                << _( "The official KiCad website - " )
                << HtmlHyperlink( wxS( "http://www.kicad.org" ) )
                << wxT( "</li>" );
    description << wxT( "<li>" )
                << _( "Developer website - " )
                << HtmlHyperlink( wxS( "https://go.kicad.org/dev" ) )
                << wxT( "</li>" );

    description << wxT( "<li>" )
                << _("Official KiCad library repositories - " )
                << HtmlHyperlink( wxS( "https://go.kicad.org/libraries" ) )
                << wxT( "</li>" );

    description << wxT( "</ul></p>" );

    description << wxT( "<p><b><u>" )
                << _( "Bug tracker" )
                << wxT( "</u></b>" ); // bold & underlined font caption

    // bullet-ed list with some http links
    description << wxT( "<ul>" );
    description << wxT( "<li>" )
                << _( "Report or examine bugs - " )
                << HtmlHyperlink( wxS( "https://go.kicad.org/bugs" ) )
                << wxT( "</li>" );
    description << wxT( "</ul></p>" );

    description << wxT( "<p><b><u>" )
                << _( "KiCad users group and community" )
                << wxT( "</u></b>" ); // bold & underlined font caption

    description << wxT( "<ul>" );
    description << wxT( "<li>" )
                << _( "KiCad forum - " )
                << HtmlHyperlink( wxS( "https://go.kicad.org/forum" ) )
                << wxT( "</li>" );

    description << wxT( "</ul></p>" );

    aInfo.SetDescription( description );


    // License information also HTML formatted:
    wxString license;
    license
        << wxT( "<div align='center'>" )
        << HtmlNewline( 4 )
        << _( "The complete KiCad EDA Suite is released under the" ) << HtmlNewline( 2 )
        << HtmlHyperlink( wxS( "http://www.gnu.org/licenses" ),
                          _( "GNU General Public License (GPL) version 3 or any later version" ) )
        << wxT( "</div>" );

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
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jean-Pierre Charras" ), LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Wayne Stambaugh" ), LEAD_DEV, nullptr ) );

    // Alphabetical after the first two
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jon Evans" ), LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Roberto Fernandez Bautista" ), LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Seth Hillbrand" ), LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ian McInerney" ), LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mark Roszko" ), LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Thomas Pointhuber" ), LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alex Shvartzkop" ), LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mike Williams" ), LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Tomasz Wlostowski" ), LEAD_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jeff Young" ), LEAD_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "John Beard" ), FORMER_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dick Hollenbeck" ), FORMER_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexis Lockwood" ), FORMER_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Brian Sidebotham" ), FORMER_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Orson (Maciej Sumiński)" ), FORMER_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mikolaj Wielgus" ), FORMER_DEV, nullptr ) );

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
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michael Beardsworth" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Matthew Beckler" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Konrad Beckmann" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Eduardo Behr" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "David Beinder" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Frank Bennett" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Roman Beranek" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Francois Berder" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Gustav Bergquist" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Cirilo Bernardo" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Joël Bertrand" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andreas Beutling" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Brian F. G. Bidulock" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Anton Blanchard" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Blair Bonnett" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Markus Bonk" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Franck Bourdonnec" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Carlo Bramini" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Stefan Brüns" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andreas Buhr" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ryan Bunch" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Emery Burhan" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Scott Candey" ), CONTRIB_DEV, nullptr ) );
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

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andrew D'Addesio" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Martin d'Allens" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Greg Davill" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Camille Delbegue" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Albin Dennevi" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ruben De Smet" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexander Dewing" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jonas Diemer" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ben Dooks" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Pavel Dovgalyuk" ), CONTRIB_DEV, nullptr ) );
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
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Brian Fiete" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Thomas Figueroa" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Vincenzo Fortunato" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Drew Fustini" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ronnie Gaensli" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Christian Gagneraud" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ben Gamari" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ashutosh Gangwar" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alessandro Gatti" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Hal Gentz" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Davide Gerhard" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michael Geselbracht" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Giulio Girardi" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jeff Glass" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexander Golubev" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Angus Gratton" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Element Green" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mathias Grimmberger" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Johan Grip" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Hildo Guillardi Júnior" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Niki Guldbrand" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Tanay Gupta" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexander Guy" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Zoltan Gyarmati" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jonathan Haas" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mark Hämmerling" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Stefan Hamminga" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Scott Hanson" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ben Harris" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Lukas F. Hartmann" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jakob Haufe" ), CONTRIB_DEV, nullptr ) );
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

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "James Jackson" ), CONTRIB_DEV, nullptr ) );
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
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Tom Keddie" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Graham Keeth" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Yury Khalyavin" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Eldar Khayrullin" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Gary Kim" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ingo Kletti" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kliment" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Sylwester Kocjan" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Clemens Koller" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Asuki Kono" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jakub Kozdon" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Hajo Nils Krabbenhöft" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Simon Kueppers" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Martijn Kuipers" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Robbert Lagerweij" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dimitris Lampridis" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marco Langer" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kevin Lannen" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mika Laitio" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Floris Lambrechts" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "lê văn lập" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Anton Lazarev" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dag Lem" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ludovic Léau-mercier" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Paul LeoNerd Evens" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jonatan Liljedahl" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Huanyin Liu" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexander Lunev" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andrew Lutsenko" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mario Luzeiro" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Johannes Maibaum" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mateusz Majchrzycki" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Daniel Majewski" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Rachel Mant" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Lorenzo Marcantonio" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Miklós Márton" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marco Mattila" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Steffen Mauch" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Brian Mayton" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Maui" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kirill Mavreshko" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Miles McCoo" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Charles McDowell" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Moses McKnight" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Martin McNamara" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Cameron McQuinn" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ievgenii Meshcheriakov" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ashley Mills" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Christoph Moench-Tegeder" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Peter Montgomery" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alejandro García Montoro" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Chris Morgan" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Felix Morgner" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jan Mrázek" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michael Narigon" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jon Neal" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Bastian Neumann" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Daniil Nikolaev" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kristian Nielsen" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Érico Nogueira" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Allan Nordhøy" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Henrik Nyberg" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kristoffer Ödmark" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Russell Oliver" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jason Oster" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Frank Palazzolo" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "luz paz" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Miguel Angel Ajo Pelayo" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Patrick Pereira" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jacobo Aragunde Perez" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Matthew Petroff" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Johannes Pfister" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Christian Pfluger" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Brian Piccioni" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Nicolas Planel" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Carl Poirier" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Reece Pollack" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alain Portal" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andrei Pozolotin" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Antia Puentes" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Zoltan Puskas" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Heikki Pulkkinen" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Morgan Quigley" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Barabas Raffai" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Urja Rannikko" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Joshua Redstone" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michele Renda" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Jean-Samuel Reynaud" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dmitry Rezvanov" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Simon Richter" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Christoph Riehl" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Thiadmer Riemersma" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Gregor Riepl" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "RigoLigoRLC" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ola Rinta-Koski" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Lubomir Rintel" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Érico Rolim" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marcus A. Romer" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Heiko Rosemann" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Fabio Rossi" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ian Roth" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "J. Morio Sakaguchi" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Simon Schaak" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Ross Schlaikjer" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Julius Schmidt" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Felix Schneider" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marvin Schmidt" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Carsten Schoenert" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Simon Schubert" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michal Schulz" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Adrian Scripca" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Pradeepa Senanayake" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marco Serantoni" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Frank Severinsen" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Cheng Sheng" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Yang Sheng" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Chetan Shinde" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Alexander Shuklin" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Slawomir Siudym" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Marco Serantoni" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Guillaume Simard" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Martin Sivak" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Mateusz Skowroński" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dominik Sliwa" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Blake Smith" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Michal Sojka" ), CONTRIB_DEV, nullptr ) );
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
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Hiroshi Tokita" ), CONTRIB_DEV, nullptr ) );

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
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Stefan Weber" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Bevan Weiss" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Simon Wells" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Dominik Wernberger" ), CONTRIB_DEV, nullptr ) );
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
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Frank Zeeman" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Henner Zeller" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Andrew Zonenberg" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Karl Zeilhofer" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Kevin Zheng" ), CONTRIB_DEV, nullptr ) );

    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "Nick Østergaard" ), CONTRIB_DEV, nullptr ) );
    aInfo.AddDeveloper( new CONTRIBUTOR( wxT( "木 王" ), CONTRIB_DEV, nullptr ) );

    // The document writers
    aInfo.AddDocWriter( new CONTRIBUTOR( wxS( "Jean-Pierre Charras" ) ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( wxS( "Marco Ciampa" ) ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( wxS( "Jon Evans" ) ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( wxS( "Dick Hollenbeck" ) ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( wxS( "Graham Keeth" ) ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( wxS( "Igor Plyatov" ) ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( wxS( "Wayne Stambaugh" ) ) );
    aInfo.AddDocWriter( new CONTRIBUTOR( wxS( "Fabrizio Tappero" ) ) );

    /* The translators
     * As category the language to which the translation was done is used
     */
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Ondřej Čertík" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Czech (CS)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Martin Kratoška" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Czech (CS)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Radek Kuznik" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Czech (CS)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Roman Ondráček" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Czech (CS)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "René Široký" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Czech (CS)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Jan Straka" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Czech (CS)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Jan Vykydal" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Czech (CS)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Mads Dyrmann" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Danish (DA)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Henrik Kauhanen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Danish (DA)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Nick Østergaard" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Danish (DA)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Ivan Chuba" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "German (DE)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Benedikt Freisen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "German (DE)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Jonathan Haas" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "German (DE)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Mark Hämmerling" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "German (DE)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Henrik Kauhanen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "German (DE)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Mathias Neumann" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "German (DE)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Dominik Wernberger" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "German (DE)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Manolis Stefanis" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Greek (el_GR)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Athanasios Vlastos" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Greek (el_GR)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Milonas Kostas" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Greek (el_GR)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Michail Misirlis" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Greek (el_GR)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Aristeidis Kimirtzis" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Greek (el_GR)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Adolfo Jayme Barrientos" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Spanish (ES)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Roberto Fernandez Bautista" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Spanish (ES)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Iñigo Figuero" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Spanish (ES)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Augusto Fraga Giachero" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Spanish (ES)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Pedro Martin del Valle" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Spanish (ES)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Jose Perez" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Spanish (ES)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Iñigo Zuluaga" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Spanish (ES)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "VicSanRoPe" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Spanish - Latin American (ES)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Ulices Avila Hernandez" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Spanish - Latin American (ES)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Vesa Solonen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Finnish (FI)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Alex Gellen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Finnish (FI)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Toni Laiho" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Finnish (FI)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Henrik Kauhanen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Finnish (FI)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "J. Lavoie" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Finnish (FI)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Purkka Koodari" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Finnish (FI)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Simo Mattila" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Finnish (FI)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Petri Niemelä" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Finnish (FI)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Ola Rinta-Koski" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Finnish (FI)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Riku Viitanen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Finnish (FI)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Jean-Pierre Charras" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "French (FR)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Marco Ciampa" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Italian (IT)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Ji Yoon Choi" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Japanese (JA)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Hidemichi Gotou" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Japanese (JA)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Kinichiro Inoguchi" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Japanese (JA)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Keisuke Nakao" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Japanese (JA)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Norio Suzuki" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Japanese (JA)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "starfort-jp" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Japanese (JA)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Hiroshi Tokita" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Japanese (JA)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Kenta Yonekura" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Japanese (JA)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Kaoru Zenyouji" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Japanese (JA)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "킴슨김랑기" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Ji Yoon Choi" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "jeongsuAn" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "이상수" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "jehunseo" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Uibeom Jung" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "박준언" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "방준영" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "박기정" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "hokim" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "김낙환" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "이기형" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "남우근" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "강명구" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "김용재" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "서범기" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "김세영" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "이윤성" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "KwonHyeokbeom" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Minsu Kim (0xGabriel)" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Korean (KO)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Henrik Kauhanen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Lithuanian (LT)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Dainius Mazuika" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Lithuanian (LT)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Arend-Jan van Hilten" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Dutch (NL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "CJ van der Hoeven" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Dutch (NL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Pim Jansen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Dutch (NL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Henrik Kauhanen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Dutch (NL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Bas Wijnen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Dutch (NL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Tom Niesse" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Dutch (NL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Christiaan Nieuwlaat" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Dutch (NL)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Jarl Gjessing" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Norwegian (NO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Henrik Kauhanen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Norwegian (NO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Allan Nordhøy" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Norwegian (NO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Petter Reinholdtsen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Norwegian (NO)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Håvard Syslak" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Norwegian (NO)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Ivan Chuba" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Polish (PL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Kerusey Karyu" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Polish (PL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Krzysztof Kawa" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Polish (PL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Mark Roszko" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Polish (PL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Mateusz Skowroński" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Polish (PL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Grzegorz Szymaszek" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Polish (PL)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "ZbeeGin" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Polish (PL)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Augusto Fraga Giachero" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Brazilian Portuguese (PT_BR)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Wellington Terumi Uemura" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Brazilian Portuguese (PT_BR)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Augusto Fraga Giachero" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Portuguese (PT)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Renie Marquet" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Portuguese (PT)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Rafael Silva" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Portuguese (PT)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "ssantos" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Portuguese (PT)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Igor Plyatov" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Russian (RU)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Дмитрий Дёмин" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Russian (RU)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Andrey Fedorushkov" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Russian (RU)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Eldar Khayrullin" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Russian (RU)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Konstantin Baranovskiy" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Russian (RU)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Hanna Breisand" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Swedish (SV)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Axel Henriksson"  ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Swedish (SV)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Richard Jonsson" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Swedish (SV)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Henrik Kauhanen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Swedish (SV)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Allan Nordhøy" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Swedish (SV)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Boonchai Kingrungped" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Thai (TH)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Artem" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Ukrainian (UK)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Ivan Chuba" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Ukrainian (UK)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Stanislav Kaliuk" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Ukrainian (UK)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Alexsandr Kuzemko" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Ukrainian (UK)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Andrii Shelestov" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Ukrainian (UK)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Liu Guang" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Simplified Chinese (zh_CN)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Taotieren" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Simplified Chinese (zh_CN)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Dingzhong Chen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Simplified Chinese (zh_CN)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "David Chen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Simplified Chinese (zh_CN)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Eric" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Simplified Chinese (zh_CN)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Rigo Ligo" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Simplified Chinese (zh_CN)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Huanyin Liu" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Simplified Chinese (zh_CN)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "yangyangdaji" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Simplified Chinese (zh_CN)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Tian Yunhao" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Simplified Chinese (zh_CN)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "David Chen" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Traditional Chinese (zh_TW)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "pon dahai" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Traditional Chinese (zh_TW)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "kai chiao chuang" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Traditional Chinese (zh_TW)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Taotieren" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Traditional Chinese (zh_TW)" ) ) );
    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "william" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Traditional Chinese (zh_TW)" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxT( "Remy Halvick" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Other" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "David J S Briscoe" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Other" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Dominique Laigle" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Other" ) ) );

    aInfo.AddTranslator( new CONTRIBUTOR( wxS( "Paul Burke" ),
                                          wxEmptyString,
                                          wxEmptyString,
                                          wxS( "Other" ) ) );


    // Program credits for library team
    #define LIBRARIANS _( "KiCad Librarian Team" )
    // Lead librarians
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "cpresser"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );

    // Active librarians
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Aristeidis Kimirtzis"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "apo"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Armin Schoisswohl"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Carlos Nieves Ónega"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Dash Peters"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Jan Sebastian Götte (jaseg)"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Greg Cormier"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Jeremy Boynes"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Jorge Neiva"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Kliment"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Andrew Lutsenko"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );

    // Former librarians
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Christian Schlüter"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Rene Poeschl"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Antonio Vázquez Blanco "), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Daniel Giesbrecht"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Otavio Augusto Gomes"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "herostrat"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Diego Herranz"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Joel Guittet"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Chris Morgan"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Thomas Pointhuber"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Evan Shultz"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Bob Cousins"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "nickoe"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxT( "Oliver Walters"), LIBRARIANS, aInfo.CreateKiBitmap( BITMAPS::library ) ) );

    #define MODELS_3D_CONTRIBUTION _( "3D models by" )
    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "Scripts by Maui" ),
                                      wxS( "https://github.com/easyw" ),
                                      wxS( "https://gitlab.com/kicad/libraries/kicad-packages3D-generator" ),
                                      MODELS_3D_CONTRIBUTION,
                                      aInfo.CreateKiBitmap( BITMAPS::three_d ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "GitLab contributors" ),
                                      wxEmptyString,
                                      wxS( "https://gitlab.com/kicad/libraries/kicad-packages3D/-/graphs/master" ),
                                      MODELS_3D_CONTRIBUTION,
                                      aInfo.CreateKiBitmap( BITMAPS::three_d ) ) );

    #define SYMBOL_LIB_CONTRIBUTION _( "Symbols by" )
    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "GitLab contributors" ),
                                      wxEmptyString,
                                      wxS( "https://gitlab.com/kicad/libraries/kicad-symbols/-/graphs/master" ),
                                      SYMBOL_LIB_CONTRIBUTION,
                                      aInfo.CreateKiBitmap( BITMAPS::add_component ) ) );

    #define FOOTPRINT_LIB_CONTRIBUTION _( "Footprints by" )
    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "Scripts by Thomas Pointhuber" ),
                                      wxEmptyString,
                                      wxS( "https://gitlab.com/kicad/libraries/kicad-footprint-generator" ),
                                      FOOTPRINT_LIB_CONTRIBUTION,
                                      aInfo.CreateKiBitmap( BITMAPS::module ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "GitLab contributors" ),
                                      wxEmptyString,
                                      wxS( "https://gitlab.com/kicad/libraries/kicad-footprints/-/graphs/master" ),
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
    aInfo.AddArtist( new CONTRIBUTOR( wxS( "Fabrizio Tappero" ),
                                      wxEmptyString,
                                      wxEmptyString,
                                      ICON_CONTRIBUTION ) );

    // Program credits for package developers.
    aInfo.AddPackager( new CONTRIBUTOR( wxS( "Steven Falco" ) ) );
    aInfo.AddPackager( new CONTRIBUTOR( wxS( "Jean-Samuel Reynaud" ) ) );
    aInfo.AddPackager( new CONTRIBUTOR( wxS( "Bernhard Stegmaier" ) ) );
    aInfo.AddPackager( new CONTRIBUTOR( wxS( "Adam Wolf" ) ) );
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
        hyperlink << wxS( "<a href='" ) << aUrl << wxS( "'>" ) << aUrl << wxS( "</a>" );
    else
        hyperlink << wxS( "<a href='" )<< aUrl << wxS( "'>" ) << aDescription << wxS( "</a>" );

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
        newlineTags << wxS( "<br>" );

    return newlineTags;
}
