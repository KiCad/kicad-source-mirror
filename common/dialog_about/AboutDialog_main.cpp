/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#define ADD_DEV( name, category ) aInfo.AddDeveloper( new CONTRIBUTOR( name, category ) )
#define LEAD_DEV _( "Lead Development Team" )
#define FORMER_DEV _( "Lead Development Alumni" )
#define CONTRIB_DEV _( "Additional Contributions By")
    ADD_DEV( wxT( "Jean-Pierre Charras" ), LEAD_DEV );
    ADD_DEV( wxT( "Wayne Stambaugh" ), LEAD_DEV );

    // Alphabetical after the first two
    ADD_DEV( wxT( "John Beard" ), LEAD_DEV );
    ADD_DEV( wxT( "Jon Evans" ), LEAD_DEV );
    ADD_DEV( wxT( "Roberto Fernandez Bautista" ), LEAD_DEV );
    ADD_DEV( wxT( "Ethan Chien" ), LEAD_DEV );
    ADD_DEV( wxT( "Fabien Corona" ), LEAD_DEV );
    ADD_DEV( wxT( "Seth Hillbrand" ), LEAD_DEV );
    ADD_DEV( wxT( "James Jackson" ), LEAD_DEV );
    ADD_DEV( wxT( "Ian McInerney" ), LEAD_DEV );
    ADD_DEV( wxT( "Mark Roszko" ), LEAD_DEV );
    ADD_DEV( wxT( "Alex Shvartzkop" ), LEAD_DEV );
    ADD_DEV( wxT( "Mike Williams" ), LEAD_DEV );
    ADD_DEV( wxT( "Tomasz Wlostowski" ), LEAD_DEV );
    ADD_DEV( wxT( "Jeff Young" ), LEAD_DEV );
    ADD_DEV( wxT( "Eric Zhuang" ), LEAD_DEV );

    ADD_DEV( wxT( "Dick Hollenbeck" ), FORMER_DEV );
    ADD_DEV( wxT( "Alexis Lockwood" ), FORMER_DEV );
    ADD_DEV( wxT( "Thomas Pointhuber" ), FORMER_DEV );
    ADD_DEV( wxT( "Brian Sidebotham" ), FORMER_DEV );
    ADD_DEV( wxT( "Orson (Maciej Sumiński)" ), FORMER_DEV );
    ADD_DEV( wxT( "Mikolaj Wielgus" ), FORMER_DEV );

    ADD_DEV( wxT( "Martin Aberg" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Yüksel Açikgöz" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Rohan Agrawal" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Johannes Agricola" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Erik Agsjö" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Nabeel Ahmad" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Werner Almesberger" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Shawn Anastasio" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Collin Anderson" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Tom Andrews" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Subaru Arai" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mikael Arguedas" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Lachlan Audas" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jean-Noel Avila" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Pascal Baerten" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Konstantin Baranovskiy" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Roman Bashkov" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Michael Beardsworth" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Matthew Beckler" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Markus Becker" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Konrad Beckmann" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Eduardo Behr" ), CONTRIB_DEV );
    ADD_DEV( wxT( "David Beinder" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Frank Bennett" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Roman Beranek" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Francois Berder" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Martin Berglund" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Gustav Bergquist" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Cirilo Bernardo" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Joël Bertrand" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Harry Best" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Andreas Beutling" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Brian F. G. Bidulock" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Anton Blanchard" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Steve Bollinger" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Blair Bonnett" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Markus Bonk" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Franck Bourdonnec" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Carlo Bramini" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Matthias Breithaupt" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Stefan Brüns" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Andreas Buhr" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ryan Bunch" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Emery Burhan" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Scott Candey" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Phinitnan Chanasabaeng" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Shivpratap Chauhan" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kevin Cozens" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Joseph Y. Chen" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Alexey Chernov" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Marco Ciampa" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Marcus Comstedt" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Diogo Condeco" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Colin Cooper" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Emile Cormier" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Garth Corral" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Sergio Costas" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kevin Cozens" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Dan Cross" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Andrew D'Addesio" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Martin d'Allens" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Greg Davill" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Camille Delbegue" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Albin Dennevi" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ruben De Smet" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Alexander Dewing" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Okan Demir" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Troy Denton" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jonas Diemer" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ben Dooks" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Pavel Dovgalyuk" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Andrew Downing" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jan Dubiec" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Lucas Dumont" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Gerd Egidy" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jean Philippe Eimer" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ben Ellis" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Oleg Endo" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Damien Espitallier" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Paul Ewing" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Andrey Fedorushkov" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Julian Fellinger" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Joe Ferner" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Brian Fiete" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Thomas Figueroa" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Gilbert J.M. Forkel" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Vincenzo Fortunato" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Drew Fustini" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Ronnie Gaensli" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Christian Gagneraud" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ben Gamari" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ashutosh Gangwar" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kamil Galik" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Alessandro Gatti" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Thomas Gambier" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Hal Gentz" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Lucas Gerads" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Davide Gerhard" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Michael Geselbracht" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Giulio Girardi" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jeff Glass" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Alexander Golubev" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Angus Gratton" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Andrea Greco" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Element Green" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mathias Grimmberger" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Johan Grip" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Michal Grzegorzek" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Hildo Guillardi Júnior" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Niki Guldbrand" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Tanay Gupta" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Alexander Guy" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Zoltan Gyarmati" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Jonathan Haas" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mark Hämmerling" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Stefan Hamminga" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ma Han" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Scott Hanson" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ben Harris" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Lukas F. Hartmann" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jakob Haufe" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Aylons Hazzud" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Stefan Helmert" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Hartmut Henkel" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Brian Henning" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Paulo Henrique Silva" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Hans Henry von Tresckow" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Diego Herranz" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Marco Hess" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Petri Hodju" ), CONTRIB_DEV );
    ADD_DEV( wxT( "David Holdeman" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Laurens Holst" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Yang Hongbo" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mario Hros" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Josue Huaroto" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Matt Huszagh" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Torsten Hüter" ), CONTRIB_DEV );

    ADD_DEV( wxT( "José Ignacio Romero" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Marco Inacio" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kinichiro Inoguchi" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Fabián Inostroza" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Vlad Ivanov" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Andre Iwers" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Hasan Jaafar" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jerry Jacobs" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Christian Jacobsen" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Michal Jahelka" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Martin Janitschke" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jonathan Jara-Almonte" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Zhuang Jiezhi" ), CONTRIB_DEV );
    ADD_DEV( wxT( "José Jorge Enríquez" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Franck Jullien" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Eeli Kaikkonen" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Lajos Kamocsay" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Povilas Kanapickas" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mikhail Karpenko" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kerusey Karyu" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Michael Kavanagh" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Tom Keddie" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Graham Keeth" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Yury Khalyavin" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Eldar Khayrullin" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Georges Khaznadar" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Gary Kim" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Aristeidis Kimirtzis" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ingo Kletti" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kliment" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Sylwester Kocjan" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Uli Köhler" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Clemens Koller" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Asuki Kono" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jakub Kozdon" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Andrej Krpic" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Hajo Nils Krabbenhöft" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Simon Kueppers" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Martijn Kuipers" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Dhinesh Kumar" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Eric Kuzmenko" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Robbert Lagerweij" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Dimitris Lampridis" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Marco Langer" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kevin Lannen" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mika Laitio" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Floris Lambrechts" ), CONTRIB_DEV );
    ADD_DEV( wxT( "lê văn lập" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Denis Latyshev" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Anton Lazarev" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Dag Lem" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ludovic Léau-mercier" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Paul LeoNerd Evens" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jonatan Liljedahl" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Huanyin Liu" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Alexander Lunev" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Andrew Lutsenko" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mario Luzeiro" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Johannes Maibaum" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mateusz Majchrzycki" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Daniel Majewski" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Rachel Mant" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Lorenzo Marcantonio" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Miklós Márton" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Marco Mattila" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Steffen Mauch" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Brian Mayton" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Maui" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kirill Mavreshko" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Miles McCoo" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Charles McDowell" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Moses McKnight" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Martin McNamara" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Cameron McQuinn" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ievgenii Meshcheriakov" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mojca Miklavec Groenhuis" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ashley Mills" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Christoph Moench-Tegeder" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Peter Montgomery" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Alejandro García Montoro" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Chris Morgan" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Felix Morgner" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jan Mrázek" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Frank Muenstermann" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Michael Narigon" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jon Neal" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Bastian Neumann" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Daniil Nikolaev" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kristian Nielsen" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Érico Nogueira" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Allan Nordhøy" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Henrik Nyberg" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Kristoffer Ödmark" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Russell Oliver" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jason Oster" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Frank Palazzolo" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Sven Pauli" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Matus Pavelek" ), CONTRIB_DEV );
    ADD_DEV( wxT( "luz paz" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Miguel Angel Ajo Pelayo" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Patrick Pereira" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jacobo Aragunde Perez" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Matthew Petroff" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Johannes Pfister" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Fabian Pflug" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Christian Pfluger" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Brian Piccioni" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mathieu Pilato" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Nicolas Planel" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Paweł Płóciennik" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Carl Poirier" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Reece Pollack" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Alain Portal" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Andrei Pozolotin" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Antia Puentes" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Zoltan Puskas" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Heikki Pulkkinen" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Morgan Quigley" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Barabas Raffai" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Urja Rannikko" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Alexander Rauth" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Hendrik v. Raven" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Joshua Redstone" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Michele Renda" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jean-Samuel Reynaud" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Dmitry Rezvanov" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Simon Richter" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Christoph Riehl" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Thiadmer Riemersma" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Gregor Riepl" ), CONTRIB_DEV );
    ADD_DEV( wxT( "RigoLigoRLC" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ola Rinta-Koski" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Lubomir Rintel" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Érico Rolim" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Marcus A. Romer" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Heiko Rosemann" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Fabio Rossi" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ian Roth" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Huang Rui" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Clément Saccoccio" ), CONTRIB_DEV );
    ADD_DEV( wxT( "J. Morio Sakaguchi" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Simon Schaak" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ross Schlaikjer" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Julius Schmidt" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Felix Schneider" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Marvin Schmidt" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Carsten Schoenert" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Armin Schoisswohl" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Simon Schubert" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Michal Schulz" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Adrian Scripca" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Pradeepa Senanayake" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Marco Serantoni" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Frank Severinsen" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Cheng Sheng" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Yang Sheng" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Chetan Shinde" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Alexander Shuklin" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Adam Simpkins" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Slawomir Siudym" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Marco Serantoni" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Guillaume Simard" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Martin Sivak" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mateusz Skowroński" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Dominik Sliwa" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kacper Słomiński" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Blake Smith" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ikoma So" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Michal Sojka" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Rafael Sokolowski" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Vesa Solonen" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Ronald Sousa" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Craig Southeren" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Thomas Spindler" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Seppe Stas" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Bernhard Stegmaier" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Michael Steinberg" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Marco Sterbik" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Alexander Stock" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Martin Stoilov" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Hiroki Suenaga" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kuba Sunderland-Ober" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Nimish Telang" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Martin Thierer" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Karl Thorén" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Hiroshi Tokita" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Daniel Treffenstädt" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Vladimir Ur" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Yon Uriarte" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Matthias Urlichs" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Vladimir Uryvaev" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Julie Vairai" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Henri Valta" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Dave Vandenbout" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Edwin van den Oetelaar" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mark van Doesburg" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Nils van Zuijlen" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Fabio Varesano" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Benjamin Vernoux" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Frank Villaro-Dixon" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Mark Visser" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Forrest Voight" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Tormod Volden" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Bartek Wacławik" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Johannes Wågen" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Oliver Walters" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jonathan Warner" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Dan Weatherill" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Stefan Weber" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Christian Weickhmann" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Bernhard M. Wiedemann" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Bevan Weiss" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Simon Wells" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Dominik Wernberger" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Jan Wichmann" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Nick Winters" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Adam Wolf" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Andrzej Wolski" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Céleste Wouters" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Damian Wrobel" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Andrew Wygle" ), CONTRIB_DEV );

    ADD_DEV( wxT( "xx" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Jiaxun Yang" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Robert Yates" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Yegor Yefremov" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kenta Yonekura" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Alexander Zakamaldin" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Frank Zeeman" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Henner Zeller" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Andrew Zonenberg" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Karl Zeilhofer" ), CONTRIB_DEV );
    ADD_DEV( wxT( "Kevin Zheng" ), CONTRIB_DEV );

    ADD_DEV( wxT( "Nick Østergaard" ), CONTRIB_DEV );
    ADD_DEV( wxT( "木 王" ), CONTRIB_DEV );
    ADD_DEV( wxT( "wh201906" ), CONTRIB_DEV );

    // The document writers
#define DOC_TEAM _( "Documentation Team" )
#define ADD_WRITER( name, category ) aInfo.AddDocWriter( new CONTRIBUTOR( name, category ) )
    ADD_WRITER( wxT( "Scott Candey" ), DOC_TEAM );
    ADD_WRITER( wxS( "Jean-Pierre Charras" ), DOC_TEAM );
    ADD_WRITER( wxS( "Marco Ciampa" ), DOC_TEAM );
    ADD_WRITER( wxS( "Jon Evans" ), DOC_TEAM );
    ADD_WRITER( wxS( "Dick Hollenbeck" ), DOC_TEAM );
    ADD_WRITER( wxS( "James Jackson" ), DOC_TEAM );
    ADD_WRITER( wxS( "Graham Keeth" ), DOC_TEAM );
    ADD_WRITER( wxS( "Igor Plyatov" ), DOC_TEAM );
    ADD_WRITER( wxS( "Wayne Stambaugh" ), DOC_TEAM );
    ADD_WRITER( wxS( "Fabrizio Tappero" ), DOC_TEAM );
    ADD_WRITER( wxS( "taotieren" ), DOC_TEAM );

    /* The translators
     * As category the language to which the translation was done is used
     */
#define ADD_TRANSLATOR( name, category ) aInfo.AddTranslator( new CONTRIBUTOR( name, category ) )
    ADD_TRANSLATOR( wxT( "Radovan Blažek" ), wxS( "Czech (CS)" ) );
    ADD_TRANSLATOR( wxT( "Ondřej Čertík" ), wxS( "Czech (CS)" ) );
    ADD_TRANSLATOR( wxT( "Martin Kratoška" ), wxS( "Czech (CS)" ) );
    ADD_TRANSLATOR( wxT( "Radek Kuznik" ), wxS( "Czech (CS)" ) );
    ADD_TRANSLATOR( wxT( "Roman Ondráček" ), wxS( "Czech (CS)" ) );
    ADD_TRANSLATOR( wxT( "Petr Pazourek" ), wxS( "Czech (CS)" ) );
    ADD_TRANSLATOR( wxT( "René Široký" ), wxS( "Czech (CS)" ) );
    ADD_TRANSLATOR( wxT( "Jan Straka" ), wxS( "Czech (CS)" ) );
    ADD_TRANSLATOR( wxT( "Andrej Valek" ), wxS( "Czech (CS)" ) );
    ADD_TRANSLATOR( wxT( "Jan Vykydal" ), wxS( "Czech (CS)" ) );

    ADD_TRANSLATOR( wxS( "Mads Dyrmann" ), wxS( "Danish (DA)" ) );
    ADD_TRANSLATOR( wxS( "Henrik Kauhanen" ), wxS( "Danish (DA)" ) );
    ADD_TRANSLATOR( wxS( "Nick Østergaard" ), wxS( "Danish (DA)" ) );

    ADD_TRANSLATOR( wxS( "Ettore Atalan" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxS( "Ivan Chuba" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxS( "Julian Daube" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxS( "Benedikt Freisen" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxS( "Jonathan Haas" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxT( "Mark Hämmerling" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxT( "Johannes Maibaum" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxT( "Henrik Kauhanen" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxT( "Mathias Neumann" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxT( "Ken Ovo" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxT( "Karl Schuh" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxT( "Frank Sonnenberg" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxT( "Lauritz Tieste" ), wxS( "German (DE)" ) );
    ADD_TRANSLATOR( wxT( "Dominik Wernberger" ), wxS( "German (DE)" ) );

    ADD_TRANSLATOR( wxS( "Manolis Stefanis" ), wxS( "Greek (el_GR)" ) );
    ADD_TRANSLATOR( wxS( "Athanasios Vlastos" ), wxS( "Greek (el_GR)" ) );
    ADD_TRANSLATOR( wxS( "Milonas Kostas" ), wxS( "Greek (el_GR)" ) );
    ADD_TRANSLATOR( wxS( "Michail Misirlis" ), wxS( "Greek (el_GR)" ) );
    ADD_TRANSLATOR( wxS( "Aristeidis Kimirtzis" ), wxS( "Greek (el_GR)" ) );

    ADD_TRANSLATOR( wxT( "Adolfo Jayme Barrientos" ), wxS( "Spanish (ES)" ) );
    ADD_TRANSLATOR( wxT( "Roberto Fernandez Bautista" ), wxS( "Spanish (ES)" ) );
    ADD_TRANSLATOR( wxT( "Pablo Bianchi" ), wxS( "Spanish (ES)" ) );
    ADD_TRANSLATOR( wxT( "Iñigo Figuero" ), wxS( "Spanish (ES)" ) );
    ADD_TRANSLATOR( wxT( "Augusto Fraga Giachero" ), wxS( "Spanish (ES)" ) );
    ADD_TRANSLATOR( wxS( "Pedro Martin del Valle" ), wxS( "Spanish (ES)" ) );
    ADD_TRANSLATOR( wxS( "Gabriel Martinez" ), wxS( "Spanish (ES)" ) );
    ADD_TRANSLATOR( wxT( "Gallego Novato" ), wxS( "Spanish (ES)" ) );
    ADD_TRANSLATOR( wxT( "Jose Perez" ), wxS( "Spanish (ES)" ) );
    ADD_TRANSLATOR( wxT( "Iñigo Zuluaga" ), wxS( "Spanish (ES)" ) );
    ADD_TRANSLATOR( wxT( "Ulices Avila Hernandez" ), wxS( "Spanish (ES)" ) );
    ADD_TRANSLATOR( wxT( "VicSanRoPe" ), wxS( "Spanish (ES)" ) );

    ADD_TRANSLATOR( wxT( "VicSanRoPe" ), wxS( "Spanish - Latin American (ES)" ) );
    ADD_TRANSLATOR( wxT( "Ulices Avila Hernandez" ), wxS( "Spanish - Latin American (ES)" ) );

    ADD_TRANSLATOR( wxT( "Vesa Solonen" ), wxS( "Finnish (FI)" ) );
    ADD_TRANSLATOR( wxT( "Alex Gellen" ), wxS( "Finnish (FI)" ) );
    ADD_TRANSLATOR( wxT( "Toni Laiho" ), wxS( "Finnish (FI)" ) );
    ADD_TRANSLATOR( wxT( "Henrik Kauhanen" ), wxS( "Finnish (FI)" ) );
    ADD_TRANSLATOR( wxT( "Toni Laiho" ), wxS( "Finnish (FI)" ) );
    ADD_TRANSLATOR( wxT( "J. Lavoie" ), wxS( "Finnish (FI)" ) );
    ADD_TRANSLATOR( wxT( "Purkka Koodari" ), wxS( "Finnish (FI)" ) );
    ADD_TRANSLATOR( wxT( "Simo Mattila" ), wxS( "Finnish (FI)" ) );
    ADD_TRANSLATOR( wxT( "Petri Niemelä" ), wxS( "Finnish (FI)" ) );
    ADD_TRANSLATOR( wxT( "Ola Rinta-Koski" ), wxS( "Finnish (FI)" ) );
    ADD_TRANSLATOR( wxT( "Riku Viitanen" ), wxS( "Finnish (FI)" ) );

    ADD_TRANSLATOR( wxT( "Jean-Pierre Charras" ), wxS( "French (FR)" ) );

    ADD_TRANSLATOR( wxT( "Boromyr" ), wxS( "Italian (IT)" ) );
    ADD_TRANSLATOR( wxT( "Marco Ciampa" ), wxS( "Italian (IT)" ) );

    ADD_TRANSLATOR( wxT( "Subaru Arai" ), wxS( "Japanese (JA)" ) );
    ADD_TRANSLATOR( wxT( "Ji Yoon Choi" ), wxS( "Japanese (JA)" ) );
    ADD_TRANSLATOR( wxT( "Hidemichi Gotou" ), wxS( "Japanese (JA)" ) );
    ADD_TRANSLATOR( wxT( "Kinichiro Inoguchi" ), wxS( "Japanese (JA)" ) );
    ADD_TRANSLATOR( wxT( "Keisuke Nakao" ), wxS( "Japanese (JA)" ) );
    ADD_TRANSLATOR( wxT( "Norio Suzuki" ), wxS( "Japanese (JA)" ) );
    ADD_TRANSLATOR( wxT( "starfort-jp" ), wxS( "Japanese (JA)" ) );
    ADD_TRANSLATOR( wxT( "Hiroshi Tokita" ), wxS( "Japanese (JA)" ) );
    ADD_TRANSLATOR( wxT( "Yutaro Urata" ), wxS( "Japanese (JA)" ) );
    ADD_TRANSLATOR( wxT( "Kenta Yonekura" ), wxS( "Japanese (JA)" ) );
    ADD_TRANSLATOR( wxT( "Kaoru Zenyouji" ), wxS( "Japanese (JA)" ) );

    ADD_TRANSLATOR( wxT( "킴슨김랑기" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "Ji Yoon Choi" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "jeongsuAn" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "이상수" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "jehunseo" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "Uibeom Jung" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "박준언" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "방준영" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "박기정" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "hokim" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "김낙환" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "이기형" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "남우근" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "강명구" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "김용재" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "서범기" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "김세영" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "이윤성" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "김랑기" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "KwonHyeokbeom" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "Minsu Kim (0xGabriel)" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "Pedro Moreira" ), wxS( "Korean (KO)" ) );
    ADD_TRANSLATOR( wxT( "jeong-sangwon" ), wxS( "Korean (KO)" ) );

    ADD_TRANSLATOR( wxT( "Ignas Brašiškis" ), wxS( "Lithuanian (LT)" ) );
    ADD_TRANSLATOR( wxT( "Henrik Kauhanen" ), wxS( "Lithuanian (LT)" ) );
    ADD_TRANSLATOR( wxT( "Dainius Mazuika" ), wxS( "Lithuanian (LT)" ) );

    ADD_TRANSLATOR( wxT( "Stefan De Raedemaeker" ), wxS( "Dutch (NL)" ) );
    ADD_TRANSLATOR( wxT( "Laurens Holst" ), wxS( "Dutch (NL)" ) );
    ADD_TRANSLATOR( wxT( "Pim Jansen" ), wxS( "Dutch (NL)" ) );
    ADD_TRANSLATOR( wxT( "Henrik Kauhanen" ), wxS( "Dutch (NL)" ) );
    ADD_TRANSLATOR( wxT( "Tom Niesse" ), wxS( "Dutch (NL)" ) );
    ADD_TRANSLATOR( wxT( "Christiaan Nieuwlaat" ), wxS( "Dutch (NL)" ) );
    ADD_TRANSLATOR( wxT( "Ranforingus" ), wxS( "Dutch (NL)" ) );
    ADD_TRANSLATOR( wxT( "CJ van der Hoeven" ), wxS( "Dutch (NL)" ) );
    ADD_TRANSLATOR( wxT( "Arend-Jan van Hilten" ), wxS( "Dutch (NL)" ) );
    ADD_TRANSLATOR( wxT( "Bas Wijnen" ), wxS( "Dutch (NL)" ) );

    ADD_TRANSLATOR( wxT( "Jarl Gjessing" ), wxS( "Norwegian (NO)" ) );
    ADD_TRANSLATOR( wxT( "Henrik Kauhanen" ), wxS( "Norwegian (NO)" ) );
    ADD_TRANSLATOR( wxT( "Allan Nordhøy" ), wxS( "Norwegian (NO)" ) );
    ADD_TRANSLATOR( wxT( "Petter Reinholdtsen" ), wxS( "Norwegian (NO)" ) );
    ADD_TRANSLATOR( wxT( "Håvard Syslak" ), wxS( "Norwegian (NO)" ) );

    ADD_TRANSLATOR( wxT( "Ivan Chuba" ), wxS( "Polish (PL)" ) );
    ADD_TRANSLATOR( wxT( "Kerusey Karyu" ), wxS( "Polish (PL)" ) );
    ADD_TRANSLATOR( wxT( "Krzysztof Kawa" ), wxS( "Polish (PL)" ) );
    ADD_TRANSLATOR( wxT( "J Kolod" ), wxS( "Polish (PL)" ) );
    ADD_TRANSLATOR( wxT( "Eryk Michalak" ), wxS( "Polish (PL)" ) );
    ADD_TRANSLATOR( wxT( "Filip Piękoś" ), wxS( "Polish (PL)" ) );
    ADD_TRANSLATOR( wxT( "Mark Roszko" ), wxS( "Polish (PL)" ) );
    ADD_TRANSLATOR( wxT( "Mateusz Skowroński" ), wxS( "Polish (PL)" ) );
    ADD_TRANSLATOR( wxT( "Jan Sobków" ), wxS( "Polish (PL)" ) );
    ADD_TRANSLATOR( wxT( "Grzegorz Szymaszek" ), wxS( "Polish (PL)" ) );
    ADD_TRANSLATOR( wxT( "ZbeeGin" ), wxS( "Polish (PL)" ) );

    ADD_TRANSLATOR( wxT( "Augusto Fraga Giachero" ), wxS( "Brazilian Portuguese (PT_BR)" ) );
    ADD_TRANSLATOR( wxT( "Pedro Moreira" ), wxS( "Brazilian Portuguese (PT_BR)" ) );
    ADD_TRANSLATOR( wxT( "Wellington Terumi Uemura" ), wxS( "Brazilian Portuguese (PT_BR)" ) );

    ADD_TRANSLATOR( wxT( "Julio Dias" ), wxS( "Portuguese (PT)" ) );
    ADD_TRANSLATOR( wxT( "Augusto Fraga Giachero" ), wxS( "Portuguese (PT)" ) );
    ADD_TRANSLATOR( wxT( "Renie Marquet" ), wxS( "Portuguese (PT)" ) );
    ADD_TRANSLATOR( wxT( "Rafael Silva" ), wxS( "Portuguese (PT)" ) );
    ADD_TRANSLATOR( wxT( "ssantos" ), wxS( "Portuguese (PT)" ) );

    ADD_TRANSLATOR( wxT( "Konstantin Baranovskiy" ), wxS( "Russian (RU)" ) );
    ADD_TRANSLATOR( wxT( "Дмитрий Дёмин" ), wxS( "Russian (RU)" ) );
    ADD_TRANSLATOR( wxT( "Andrey Fedorushkov" ), wxS( "Russian (RU)" ) );
    ADD_TRANSLATOR( wxT( "Alevtina Karashokova" ), wxS( "Russian (RU)" ) );
    ADD_TRANSLATOR( wxT( "Eldar Khayrullin" ), wxS( "Russian (RU)" ) );
    ADD_TRANSLATOR( wxT( "Alex Life" ), wxS( "Russian (RU)" ) );
    ADD_TRANSLATOR( wxT( "Dmitry Mikhirev" ), wxS( "Russian (RU)" ) );
    ADD_TRANSLATOR( wxT( "Igor Plyatov" ), wxS( "Russian (RU)" ) );

    ADD_TRANSLATOR( wxT( "Hanna Breisand" ), wxS( "Swedish (SV)" ) );
    ADD_TRANSLATOR( wxT( "Axel Henriksson"  ), wxS( "Swedish (SV)" ) );
    ADD_TRANSLATOR( wxT( "Richard Jonsson" ), wxS( "Swedish (SV)" ) );
    ADD_TRANSLATOR( wxT( "Henrik Kauhanen" ), wxS( "Swedish (SV)" ) );
    ADD_TRANSLATOR( wxT( "Allan Nordhøy" ), wxS( "Swedish (SV)" ) );
    ADD_TRANSLATOR( wxT( "Elias Sjögreen" ), wxS( "Swedish (SV)" ) );

    ADD_TRANSLATOR( wxT( "Boonchai Kingrungped" ), wxS( "Thai (TH)" ) );

    ADD_TRANSLATOR( wxT( "Artem" ), wxS( "Ukrainian (UK)" ) );
    ADD_TRANSLATOR( wxT( "Ivan Chuba" ), wxS( "Ukrainian (UK)" ) );
    ADD_TRANSLATOR( wxT( "Stanislav Kaliuk" ), wxS( "Ukrainian (UK)" ) );
    ADD_TRANSLATOR( wxT( "Alexsandr Kuzemko" ), wxS( "Ukrainian (UK)" ) );
    ADD_TRANSLATOR( wxT( "Andrii Shelestov" ), wxS( "Ukrainian (UK)" ) );

    ADD_TRANSLATOR( wxT( "欠陥電気" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "向阳阳" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "David Chen" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Dingzhong Chen" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Eric" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Hubert Hu" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Liu Guang" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "HalfSweet" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Huanyin Liu" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Pinpang Liao" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Rigo Ligo" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Zhen Sun" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Jason Tan" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Taotieren" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Tian Yunhao" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Li Yidong" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "yangyangdaji" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "Lao Zhu" ), wxS( "Simplified Chinese (zh_CN)" ) );
    ADD_TRANSLATOR( wxT( "zly20129" ), wxS( "Simplified Chinese (zh_CN)" ) );

    ADD_TRANSLATOR( wxT( "撒景賢" ), wxS( "Traditional Chinese (zh_TW)" ) );
    ADD_TRANSLATOR( wxT( "David Chen" ), wxS( "Traditional Chinese (zh_TW)" ) );
    ADD_TRANSLATOR( wxT( "pon dahai" ), wxS( "Traditional Chinese (zh_TW)" ) );
    ADD_TRANSLATOR( wxT( "kai chiao chuang" ), wxS( "Traditional Chinese (zh_TW)" ) );
    ADD_TRANSLATOR( wxT( "Poming Lee" ), wxS( "Traditional Chinese (zh_TW)" ) );
    ADD_TRANSLATOR( wxT( "William Lin" ), wxS( "Traditional Chinese (zh_TW)" ) );
    ADD_TRANSLATOR( wxT( "Taotieren" ), wxS( "Traditional Chinese (zh_TW)" ) );
    ADD_TRANSLATOR( wxT( "Li Yidong" ), wxS( "Traditional Chinese (zh_TW)" ) );

    ADD_TRANSLATOR( wxT( "Remy Halvick" ), wxS( "Other" ) );
    ADD_TRANSLATOR( wxS( "David J S Briscoe" ), wxS( "Other" ) );
    ADD_TRANSLATOR( wxS( "Dominique Laigle" ), wxS( "Other" ) );
    ADD_TRANSLATOR( wxS( "Paul Burke" ), wxS( "Other" ) );


    // Program credits for library team
#define LIBRARIANS _( "Librarian Team" )
#define ADD_LIBRARIAN( name ) aInfo.AddLibrarian( new CONTRIBUTOR( name, LIBRARIANS ) )

    // Lead librarians
    ADD_LIBRARIAN( wxT( "Carsten Presser") );

    // Active librarians (last 2 years)
    ADD_LIBRARIAN( wxT( "Geries AbuAkel" ) );
    ADD_LIBRARIAN( wxT( "Patrick Baus" ) );
    ADD_LIBRARIAN( wxT( "John Beard" ) );
    ADD_LIBRARIAN( wxT( "Jeremy Boynes" ) );
    ADD_LIBRARIAN( wxT( "Greg Cormier" ) );
    ADD_LIBRARIAN( wxT( "Tobias Falk" ) );
    ADD_LIBRARIAN( wxT( "Simon Fivat" ) );
    ADD_LIBRARIAN( wxT( "Jan Sebastian Götte (jaseg)" ) );
    ADD_LIBRARIAN( wxT( "Petr Hodina" ) );
    ADD_LIBRARIAN( wxT( "Mikkel Jeppesen" ) );
    ADD_LIBRARIAN( wxT( "Aristeidis Kimirtzis" ) );
    ADD_LIBRARIAN( wxT( "Brandon Kirisaki" ) );
    ADD_LIBRARIAN( wxT( "Uli Köhler" ) );
    ADD_LIBRARIAN( wxT( "Graham Keeth" ) );
    ADD_LIBRARIAN( wxT( "Andrew Lutsenko" ) );
    ADD_LIBRARIAN( wxT( "Mojca Miklavec Groenhuis" ) );
    ADD_LIBRARIAN( wxT( "Jorge Neiva" ) );
    ADD_LIBRARIAN( wxT( "Carlos Nieves Ónega" ) );
    ADD_LIBRARIAN( wxT( "Lynn Ochs" ) );
    ADD_LIBRARIAN( wxT( "Ed Peguillan" ) );
    ADD_LIBRARIAN( wxT( "Dash Peters" ) );
    ADD_LIBRARIAN( wxT( "Sergio Rocha" ) );
    ADD_LIBRARIAN( wxT( "Benjamin Reynier" ) );
    ADD_LIBRARIAN( wxT( "Armin Schoisswohl" ) );
    ADD_LIBRARIAN( wxT( "Joel Schulz-Andres" ) );
    ADD_LIBRARIAN( wxT( "Frank Severinsen" ) );
    ADD_LIBRARIAN( wxT( "Martin Sotirov" ) );
    ADD_LIBRARIAN( wxT( "Philipp Swoboda" ) );
    ADD_LIBRARIAN( wxT( "Kliment Yanev" ) );

    // Previously active librarians
    ADD_LIBRARIAN( wxT( "Christian Schlüter" ) );
    ADD_LIBRARIAN( wxT( "Rene Poeschl" ) );
    ADD_LIBRARIAN( wxT( "Antonio Vázquez Blanco " ) );
    ADD_LIBRARIAN( wxT( "Daniel Giesbrecht" ) );
    ADD_LIBRARIAN( wxT( "Otavio Augusto Gomes" ) );
    ADD_LIBRARIAN( wxT( "herostrat" ) );
    ADD_LIBRARIAN( wxT( "Diego Herranz" ) );
    ADD_LIBRARIAN( wxT( "Joel Guittet" ) );
    ADD_LIBRARIAN( wxT( "Chris Morgan" ) );
    ADD_LIBRARIAN( wxT( "Thomas Pointhuber" ) );
    ADD_LIBRARIAN( wxT( "Evan Shultz" ) );
    ADD_LIBRARIAN( wxT( "Bob Cousins" ) );
    ADD_LIBRARIAN( wxT( "Nick Østergaard" ) );
    ADD_LIBRARIAN( wxT( "Oliver Walters" ) );

#define MODELS_3D_CONTRIBUTION _( "3D models" )
#define SYMBOL_LIB_CONTRIBUTION _( "Symbols" )
#define FOOTPRINT_LIB_CONTRIBUTION _( "Footprints" )
    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "Scripts by Maui" ),
                                         MODELS_3D_CONTRIBUTION,
                                         wxS( "https://github.com/easyw" ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "Hasan Yavuz Özderya" ),
                                         MODELS_3D_CONTRIBUTION,
                                         wxS( "https://bitbucket.org/hyOzd/freecad-macros/src/master/" ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "GitHub contributors" ),
                                         MODELS_3D_CONTRIBUTION,
                                         wxS( "https://github.com/easyw/kicad-3d-models-in-freecad/graphs/contributors" ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "GitLab contributors" ),
                                         MODELS_3D_CONTRIBUTION,
                                         wxS( "https://gitlab.com/kicad/libraries/kicad-packages3D/-/graphs/master" ) ) );

    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "GitLab contributors" ),
                                         SYMBOL_LIB_CONTRIBUTION,
                                         wxS( "https://gitlab.com/kicad/libraries/kicad-symbols/-/graphs/master" ) ) );

    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "Scripts by Thomas Pointhuber" ),
                                         FOOTPRINT_LIB_CONTRIBUTION,
                                         wxS( "https://gitlab.com/kicad/libraries/kicad-footprint-generator" ) ) );
    aInfo.AddLibrarian( new CONTRIBUTOR( wxS( "GitLab contributors" ),
                                         FOOTPRINT_LIB_CONTRIBUTION,
                                         wxS( "https://gitlab.com/kicad/libraries/kicad-footprints/-/graphs/master" ) ) );

    // Program credits for icons
#define ICON_CONTRIBUTION _( "Icons" )
    aInfo.AddArtist( new CONTRIBUTOR( wxT( "Aleksandr Zyrianov" ), ICON_CONTRIBUTION ) );
    aInfo.AddArtist( new CONTRIBUTOR( wxT( "Anda Subero" ), ICON_CONTRIBUTION ) );
    aInfo.AddArtist( new CONTRIBUTOR( wxT( "Iñigo Zuluaga" ), ICON_CONTRIBUTION ) );
    aInfo.AddArtist( new CONTRIBUTOR( wxS( "Fabrizio Tappero" ), ICON_CONTRIBUTION ) );

    // Program credits for package developers.
#define PACKAGE_DEVS _( "Package Developers" )

    aInfo.AddPackager( new CONTRIBUTOR( wxS( "Steven Falco" ), PACKAGE_DEVS ) );
    aInfo.AddPackager( new CONTRIBUTOR( wxS( "Johannes Maibaum" ), PACKAGE_DEVS ) );
    aInfo.AddPackager( new CONTRIBUTOR( wxS( "Jean-Samuel Reynaud" ), PACKAGE_DEVS ) );
    aInfo.AddPackager( new CONTRIBUTOR( wxS( "Bernhard Stegmaier" ), PACKAGE_DEVS ) );
    aInfo.AddPackager( new CONTRIBUTOR( wxS( "Adam Wolf" ) , PACKAGE_DEVS ) );
    aInfo.AddPackager( new CONTRIBUTOR( wxT( "Nick Østergaard" ), PACKAGE_DEVS ) );
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
