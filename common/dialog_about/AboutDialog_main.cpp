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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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


enum DEV_CATEGORY
{
    LEAD_DEV,
    FORMER_DEV,
    CONTRIB_DEV,
    DEV_CATEGORY_COUNT
};


enum LIBRARIAN_CATEGORY
{
    LIBRARIAN_TEAM,
    MODELS_3D_CONTRIBUTION,
    SYMBOL_LIB_CONTRIBUTION,
    FOOTPRINT_LIB_CONTRIBUTION,
    LIBRARIAN_CATEGORY_COUNT
};


struct DEVELOPER
{
    const wxChar* m_Name;
    DEV_CATEGORY  m_Category;
};


struct TRANSLATOR
{
    const wxChar* m_Name;
    const wxChar* m_Language;
};


struct LIBRARIAN
{
    const wxChar*      m_Name;
    LIBRARIAN_CATEGORY m_Category;
    const wxChar*      m_Url = nullptr;
};


static const DEVELOPER s_developers[] = {
    { wxT( "Jean-Pierre Charras" ), LEAD_DEV },
    { wxT( "Wayne Stambaugh" ), LEAD_DEV },

    // Alphabetical after the first two
    { wxT( "John Beard" ), LEAD_DEV },
    { wxT( "Jon Evans" ), LEAD_DEV },
    { wxT( "Roberto Fernandez Bautista" ), LEAD_DEV },
    { wxT( "Ethan Chien" ), LEAD_DEV },
    { wxT( "Fabien Corona" ), LEAD_DEV },
    { wxT( "Seth Hillbrand" ), LEAD_DEV },
    { wxT( "James Jackson" ), LEAD_DEV },
    { wxT( "Ian McInerney" ), LEAD_DEV },
    { wxT( "Mark Roszko" ), LEAD_DEV },
    { wxT( "Alex Shvartzkop" ), LEAD_DEV },
    { wxT( "Mike Williams" ), LEAD_DEV },
    { wxT( "Tomasz Wlostowski" ), LEAD_DEV },
    { wxT( "Jeff Young" ), LEAD_DEV },
    { wxT( "Eric Zhuang" ), LEAD_DEV },

    { wxT( "Dick Hollenbeck" ), FORMER_DEV },
    { wxT( "Alexis Lockwood" ), FORMER_DEV },
    { wxT( "Thomas Pointhuber" ), FORMER_DEV },
    { wxT( "Brian Sidebotham" ), FORMER_DEV },
    { wxT( "Orson (Maciej Sumiński)" ), FORMER_DEV },
    { wxT( "Mikolaj Wielgus" ), FORMER_DEV },

    { wxT( "Martin Aberg" ), CONTRIB_DEV },
    { wxT( "Yüksel Açikgöz" ), CONTRIB_DEV },
    { wxT( "Rohan Agrawal" ), CONTRIB_DEV },
    { wxT( "Johannes Agricola" ), CONTRIB_DEV },
    { wxT( "Erik Agsjö" ), CONTRIB_DEV },
    { wxT( "Nabeel Ahmad" ), CONTRIB_DEV },
    { wxT( "Christopher Alexander" ), CONTRIB_DEV },
    { wxT( "Werner Almesberger" ), CONTRIB_DEV },
    { wxT( "Shawn Anastasio" ), CONTRIB_DEV },
    { wxT( "Collin Anderson" ), CONTRIB_DEV },
    { wxT( "Tom Andrews" ), CONTRIB_DEV },
    { wxT( "Subaru Arai" ), CONTRIB_DEV },
    { wxT( "Mikael Arguedas" ), CONTRIB_DEV },
    { wxT( "Lachlan Audas" ), CONTRIB_DEV },
    { wxT( "Jean-Noel Avila" ), CONTRIB_DEV },

    { wxT( "Pascal Baerten" ), CONTRIB_DEV },
    { wxT( "Konstantin Baranovskiy" ), CONTRIB_DEV },
    { wxT( "Roman Bashkov" ), CONTRIB_DEV },
    { wxT( "Michael Beardsworth" ), CONTRIB_DEV },
    { wxT( "Markus Becker" ), CONTRIB_DEV },
    { wxT( "Matthew Beckler" ), CONTRIB_DEV },
    { wxT( "Konrad Beckmann" ), CONTRIB_DEV },
    { wxT( "Eduardo Behr" ), CONTRIB_DEV },
    { wxT( "David Beinder" ), CONTRIB_DEV },
    { wxT( "Frank Bennett" ), CONTRIB_DEV },
    { wxT( "Roman Beranek" ), CONTRIB_DEV },
    { wxT( "Francois Berder" ), CONTRIB_DEV },
    { wxT( "Martin Berglund" ), CONTRIB_DEV },
    { wxT( "Gustav Bergquist" ), CONTRIB_DEV },
    { wxT( "Cirilo Bernardo" ), CONTRIB_DEV },
    { wxT( "Joël Bertrand" ), CONTRIB_DEV },
    { wxT( "Harry Best" ), CONTRIB_DEV },
    { wxT( "Andreas Beutling" ), CONTRIB_DEV },
    { wxT( "Brian F. G. Bidulock" ), CONTRIB_DEV },
    { wxT( "Anton Blanchard" ), CONTRIB_DEV },
    { wxT( "Alexander Boehm" ), CONTRIB_DEV },
    { wxT( "Steve Bollinger" ), CONTRIB_DEV },
    { wxT( "Markus Bonk" ), CONTRIB_DEV },
    { wxT( "Blair Bonnett" ), CONTRIB_DEV },
    { wxT( "Franck Bourdonnec" ), CONTRIB_DEV },
    { wxT( "Kevin Bralten" ), CONTRIB_DEV },
    { wxT( "Carlo Bramini" ), CONTRIB_DEV },
    { wxT( "Matthias Breithaupt" ), CONTRIB_DEV },
    { wxT( "Stefan Brüns" ), CONTRIB_DEV },
    { wxT( "Andreas Buhr" ), CONTRIB_DEV },
    { wxT( "Ryan Bunch" ), CONTRIB_DEV },
    { wxT( "Emery Burhan" ), CONTRIB_DEV },

    { wxT( "Matt Campbell" ), CONTRIB_DEV },
    { wxT( "Scott Candey" ), CONTRIB_DEV },
    { wxT( "Phinitnan Chanasabaeng" ), CONTRIB_DEV },
    { wxT( "Shivpratap Chauhan" ), CONTRIB_DEV },
    { wxT( "Joseph Y. Chen" ), CONTRIB_DEV },
    { wxT( "Alexey Chernov" ), CONTRIB_DEV },
    { wxT( "Marco Ciampa" ), CONTRIB_DEV },
    { wxT( "Marcus Comstedt" ), CONTRIB_DEV },
    { wxT( "Diogo Condeco" ), CONTRIB_DEV },
    { wxT( "Colin Cooper" ), CONTRIB_DEV },
    { wxT( "Emile Cormier" ), CONTRIB_DEV },
    { wxT( "Garth Corral" ), CONTRIB_DEV },
    { wxT( "Sergio Costas" ), CONTRIB_DEV },
    { wxT( "Kevin Cozens" ), CONTRIB_DEV },
    { wxT( "Dan Cross" ), CONTRIB_DEV },

    { wxT( "Andrew D'Addesio" ), CONTRIB_DEV },
    { wxT( "Martin d'Allens" ), CONTRIB_DEV },
    { wxT( "Greg Davill" ), CONTRIB_DEV },
    { wxT( "Camille Delbegue" ), CONTRIB_DEV },
    { wxT( "Okan Demir" ), CONTRIB_DEV },
    { wxT( "Albin Dennevi" ), CONTRIB_DEV },
    { wxT( "Troy Denton" ), CONTRIB_DEV },
    { wxT( "Alexander Dewing" ), CONTRIB_DEV },
    { wxT( "Jonas Diemer" ), CONTRIB_DEV },
    { wxT( "Ben Dooks" ), CONTRIB_DEV },
    { wxT( "Jan Dorniak" ), CONTRIB_DEV },
    { wxT( "Pavel Dovgalyuk" ), CONTRIB_DEV },
    { wxT( "Andrew Downing" ), CONTRIB_DEV },
    { wxT( "Jan Dubiec" ), CONTRIB_DEV },
    { wxT( "Lucas Dumont" ), CONTRIB_DEV },
    { wxT( "Ruben De Smet" ), CONTRIB_DEV },

    { wxT( "Gerd Egidy" ), CONTRIB_DEV },
    { wxT( "Jean Philippe Eimer" ), CONTRIB_DEV },
    { wxT( "Ben Ellis" ), CONTRIB_DEV },
    { wxT( "Oleg Endo" ), CONTRIB_DEV },
    { wxT( "Damien Espitallier" ), CONTRIB_DEV },
    { wxT( "Paul Ewing" ), CONTRIB_DEV },

    { wxT( "Steven A. Falco" ), CONTRIB_DEV },
    { wxT( "Andrey Fedorushkov" ), CONTRIB_DEV },
    { wxT( "Julian Fellinger" ), CONTRIB_DEV },
    { wxT( "Joe Ferner" ), CONTRIB_DEV },
    { wxT( "Brian Fiete" ), CONTRIB_DEV },
    { wxT( "Thomas Figueroa" ), CONTRIB_DEV },
    { wxT( "Gilbert J.M. Forkel" ), CONTRIB_DEV },
    { wxT( "Vincenzo Fortunato" ), CONTRIB_DEV },
    { wxT( "Quentin Freimanis" ), CONTRIB_DEV },
    { wxT( "Dominique Fuchs" ), CONTRIB_DEV },
    { wxT( "Drew Fustini" ), CONTRIB_DEV },

    { wxT( "Ronnie Gaensli" ), CONTRIB_DEV },
    { wxT( "Christian Gagneraud" ), CONTRIB_DEV },
    { wxT( "Kamil Galik" ), CONTRIB_DEV },
    { wxT( "Ben Gamari" ), CONTRIB_DEV },
    { wxT( "Thomas Gambier" ), CONTRIB_DEV },
    { wxT( "Ashutosh Gangwar" ), CONTRIB_DEV },
    { wxT( "Adrián García" ), CONTRIB_DEV },
    { wxT( "Alessandro Gatti" ), CONTRIB_DEV },
    { wxT( "Zenn Geeraerts" ), CONTRIB_DEV },
    { wxT( "Hal Gentz" ), CONTRIB_DEV },
    { wxT( "Lucas Gerads" ), CONTRIB_DEV },
    { wxT( "Davide Gerhard" ), CONTRIB_DEV },
    { wxT( "Michael Geselbracht" ), CONTRIB_DEV },
    { wxT( "Giulio Girardi" ), CONTRIB_DEV },
    { wxT( "Jeff Glass" ), CONTRIB_DEV },
    { wxT( "Alexander Golubev" ), CONTRIB_DEV },
    { wxT( "Paweł Gorgoń" ), CONTRIB_DEV },
    { wxT( "Connor Goss" ), CONTRIB_DEV },
    { wxT( "Angus Gratton" ), CONTRIB_DEV },
    { wxT( "Andrea Greco" ), CONTRIB_DEV },
    { wxT( "Element Green" ), CONTRIB_DEV },
    { wxT( "Mathias Grimmberger" ), CONTRIB_DEV },
    { wxT( "Johan Grip" ), CONTRIB_DEV },
    { wxT( "Michal Grzegorzek" ), CONTRIB_DEV },
    { wxT( "Niki Guldbrand" ), CONTRIB_DEV },
    { wxT( "Tanay Gupta" ), CONTRIB_DEV },
    { wxT( "Alexander Guy" ), CONTRIB_DEV },
    { wxT( "Zoltan Gyarmati" ), CONTRIB_DEV },
    { wxT( "Hildo Guillardi Júnior" ), CONTRIB_DEV },

    { wxT( "Jonathan Haas" ), CONTRIB_DEV },
    { wxT( "Mark Hämmerling" ), CONTRIB_DEV },
    { wxT( "Stefan Hamminga" ), CONTRIB_DEV },
    { wxT( "Ma Han" ), CONTRIB_DEV },
    { wxT( "Scott Hanson" ), CONTRIB_DEV },
    { wxT( "Ben Harris" ), CONTRIB_DEV },
    { wxT( "Lukas F. Hartmann" ), CONTRIB_DEV },
    { wxT( "Jakob Haufe" ), CONTRIB_DEV },
    { wxT( "Aylons Hazzud" ), CONTRIB_DEV },
    { wxT( "Stefan Helmert" ), CONTRIB_DEV },
    { wxT( "Hartmut Henkel" ), CONTRIB_DEV },
    { wxT( "Brian Henning" ), CONTRIB_DEV },
    { wxT( "Diego Herranz" ), CONTRIB_DEV },
    { wxT( "Marco Hess" ), CONTRIB_DEV },
    { wxT( "Brendan Hickey" ), CONTRIB_DEV },
    { wxT( "Petri Hodju" ), CONTRIB_DEV },
    { wxT( "David Holdeman" ), CONTRIB_DEV },
    { wxT( "Laurens Holst" ), CONTRIB_DEV },
    { wxT( "Yang Hongbo" ), CONTRIB_DEV },
    { wxT( "Mario Hros" ), CONTRIB_DEV },
    { wxT( "Josue Huaroto" ), CONTRIB_DEV },
    { wxT( "Eli Hughes" ), CONTRIB_DEV },
    { wxT( "Matt Huszagh" ), CONTRIB_DEV },
    { wxT( "Torsten Hüter" ), CONTRIB_DEV },
    { wxT( "Paulo Henrique Silva" ), CONTRIB_DEV },
    { wxT( "Hans Henry von Tresckow" ), CONTRIB_DEV },

    { wxT( "Marco Inacio" ), CONTRIB_DEV },
    { wxT( "Kinichiro Inoguchi" ), CONTRIB_DEV },
    { wxT( "Fabián Inostroza" ), CONTRIB_DEV },
    { wxT( "Vlad Ivanov" ), CONTRIB_DEV },
    { wxT( "Andre Iwers" ), CONTRIB_DEV },
    { wxT( "José Ignacio Romero" ), CONTRIB_DEV },

    { wxT( "José Jorge Enríquez" ), CONTRIB_DEV },
    { wxT( "Hasan Jaafar" ), CONTRIB_DEV },
    { wxT( "Jerry Jacobs" ), CONTRIB_DEV },
    { wxT( "Christian Jacobsen" ), CONTRIB_DEV },
    { wxT( "Michal Jahelka" ), CONTRIB_DEV },
    { wxT( "Martin Janitschke" ), CONTRIB_DEV },
    { wxT( "Jonathan Jara-Almonte" ), CONTRIB_DEV },
    { wxT( "Zhuang Jiezhi" ), CONTRIB_DEV },
    { wxT( "Franck Jullien" ), CONTRIB_DEV },

    { wxT( "Eeli Kaikkonen" ), CONTRIB_DEV },
    { wxT( "Lajos Kamocsay" ), CONTRIB_DEV },
    { wxT( "Povilas Kanapickas" ), CONTRIB_DEV },
    { wxT( "Mikhail Karpenko" ), CONTRIB_DEV },
    { wxT( "Kerusey Karyu" ), CONTRIB_DEV },
    { wxT( "Michael Kavanagh" ), CONTRIB_DEV },
    { wxT( "Tom Keddie" ), CONTRIB_DEV },
    { wxT( "Graham Keeth" ), CONTRIB_DEV },
    { wxT( "Yury Khalyavin" ), CONTRIB_DEV },
    { wxT( "Eldar Khayrullin" ), CONTRIB_DEV },
    { wxT( "Lenny Khazan" ), CONTRIB_DEV },
    { wxT( "Georges Khaznadar" ), CONTRIB_DEV },
    { wxT( "Gary Kim" ), CONTRIB_DEV },
    { wxT( "Aristeidis Kimirtzis" ), CONTRIB_DEV },
    { wxT( "Bernhard Kirchen" ), CONTRIB_DEV },
    { wxT( "Ingo Kletti" ), CONTRIB_DEV },
    { wxT( "Kliment" ), CONTRIB_DEV },
    { wxT( "Sylwester Kocjan" ), CONTRIB_DEV },
    { wxT( "Uli Köhler" ), CONTRIB_DEV },
    { wxT( "Clemens Koller" ), CONTRIB_DEV },
    { wxT( "Asuki Kono" ), CONTRIB_DEV },
    { wxT( "Matt Kosman" ), CONTRIB_DEV },
    { wxT( "Jakub Kozdon" ), CONTRIB_DEV },
    { wxT( "Hajo Nils Krabbenhöft" ), CONTRIB_DEV },
    { wxT( "Andrej Krpic" ), CONTRIB_DEV },
    { wxT( "Simon Kueppers" ), CONTRIB_DEV },
    { wxT( "Martijn Kuipers" ), CONTRIB_DEV },
    { wxT( "Dhinesh Kumar" ), CONTRIB_DEV },
    { wxT( "Eric Kuzmenko" ), CONTRIB_DEV },

    { wxT( "Paul LeoNerd Evens" ), CONTRIB_DEV },
    { wxT( "Robbert Lagerweij" ), CONTRIB_DEV },
    { wxT( "Mika Laitio" ), CONTRIB_DEV },
    { wxT( "Floris Lambrechts" ), CONTRIB_DEV },
    { wxT( "Dimitris Lampridis" ), CONTRIB_DEV },
    { wxT( "Marco Langer" ), CONTRIB_DEV },
    { wxT( "Kevin Lannen" ), CONTRIB_DEV },
    { wxT( "lê văn lập" ), CONTRIB_DEV },
    { wxT( "Denis Latyshev" ), CONTRIB_DEV },
    { wxT( "Anton Lazarev" ), CONTRIB_DEV },
    { wxT( "Ludovic Léau-mercier" ), CONTRIB_DEV },
    { wxT( "Dag Lem" ), CONTRIB_DEV },
    { wxT( "Jonatan Liljedahl" ), CONTRIB_DEV },
    { wxT( "Huanyin Liu" ), CONTRIB_DEV },
    { wxT( "Brian Lu" ), CONTRIB_DEV },
    { wxT( "Magnus Lundmark" ), CONTRIB_DEV },
    { wxT( "Alexander Lunev" ), CONTRIB_DEV },
    { wxT( "Andrew Lutsenko" ), CONTRIB_DEV },
    { wxT( "Mario Luzeiro" ), CONTRIB_DEV },

    { wxT( "Mojca Miklavec Groenhuis" ), CONTRIB_DEV },
    { wxT( "Johannes Maibaum" ), CONTRIB_DEV },
    { wxT( "Philippe Maire" ), CONTRIB_DEV },
    { wxT( "Mateusz Majchrzycki" ), CONTRIB_DEV },
    { wxT( "Daniel Majewski" ), CONTRIB_DEV },
    { wxT( "Rachel Mant" ), CONTRIB_DEV },
    { wxT( "Lorenzo Marcantonio" ), CONTRIB_DEV },
    { wxT( "Miklós Márton" ), CONTRIB_DEV },
    { wxT( "Marco Mattila" ), CONTRIB_DEV },
    { wxT( "Steffen Mauch" ), CONTRIB_DEV },
    { wxT( "Maui" ), CONTRIB_DEV },
    { wxT( "Kirill Mavreshko" ), CONTRIB_DEV },
    { wxT( "Brian Mayton" ), CONTRIB_DEV },
    { wxT( "Miles McCoo" ), CONTRIB_DEV },
    { wxT( "Charles McDowell" ), CONTRIB_DEV },
    { wxT( "Ian McKernan" ), CONTRIB_DEV },
    { wxT( "Moses McKnight" ), CONTRIB_DEV },
    { wxT( "Martin McNamara" ), CONTRIB_DEV },
    { wxT( "Cameron McQuinn" ), CONTRIB_DEV },
    { wxT( "Ievgenii Meshcheriakov" ), CONTRIB_DEV },
    { wxT( "Yiannis Michael" ), CONTRIB_DEV },
    { wxT( "Ashley Mills" ), CONTRIB_DEV },
    { wxT( "Christoph Moench-Tegeder" ), CONTRIB_DEV },
    { wxT( "Sean Mollet" ), CONTRIB_DEV },
    { wxT( "Peter Montgomery" ), CONTRIB_DEV },
    { wxT( "Alejandro García Montoro" ), CONTRIB_DEV },
    { wxT( "Chris Morgan" ), CONTRIB_DEV },
    { wxT( "Felix Morgner" ), CONTRIB_DEV },
    { wxT( "Jan Mrázek" ), CONTRIB_DEV },
    { wxT( "Frank Muenstermann" ), CONTRIB_DEV },

    { wxT( "Michael Narigon" ), CONTRIB_DEV },
    { wxT( "Jon Neal" ), CONTRIB_DEV },
    { wxT( "Bastian Neumann" ), CONTRIB_DEV },
    { wxT( "Kristian Nielsen" ), CONTRIB_DEV },
    { wxT( "Daniil Nikolaev" ), CONTRIB_DEV },
    { wxT( "Érico Nogueira" ), CONTRIB_DEV },
    { wxT( "Allan Nordhøy" ), CONTRIB_DEV },
    { wxT( "Henrik Nyberg" ), CONTRIB_DEV },

    { wxT( "Kristoffer Ödmark" ), CONTRIB_DEV },
    { wxT( "Russell Oliver" ), CONTRIB_DEV },
    { wxT( "Jason Oster" ), CONTRIB_DEV },
    { wxT( "Juho Ovaska" ), CONTRIB_DEV },

    { wxT( "Frank Palazzolo" ), CONTRIB_DEV },
    { wxT( "Sven Pauli" ), CONTRIB_DEV },
    { wxT( "Matus Pavelek" ), CONTRIB_DEV },
    { wxT( "luz paz" ), CONTRIB_DEV },
    { wxT( "Miguel Angel Ajo Pelayo" ), CONTRIB_DEV },
    { wxT( "Patrick Pereira" ), CONTRIB_DEV },
    { wxT( "Jacobo Aragunde Perez" ), CONTRIB_DEV },
    { wxT( "Matthew Petroff" ), CONTRIB_DEV },
    { wxT( "Johannes Pfister" ), CONTRIB_DEV },
    { wxT( "Fabian Pflug" ), CONTRIB_DEV },
    { wxT( "Christian Pfluger" ), CONTRIB_DEV },
    { wxT( "Brian Piccioni" ), CONTRIB_DEV },
    { wxT( "Mathieu Pilato" ), CONTRIB_DEV },
    { wxT( "Nicolas Planel" ), CONTRIB_DEV },
    { wxT( "Carl Poirier" ), CONTRIB_DEV },
    { wxT( "Reece Pollack" ), CONTRIB_DEV },
    { wxT( "Alain Portal" ), CONTRIB_DEV },
    { wxT( "Andrei Pozolotin" ), CONTRIB_DEV },
    { wxT( "Damjan Prerad" ), CONTRIB_DEV },
    { wxT( "Antia Puentes" ), CONTRIB_DEV },
    { wxT( "Heikki Pulkkinen" ), CONTRIB_DEV },
    { wxT( "Zoltan Puskas" ), CONTRIB_DEV },
    { wxT( "Paweł Płóciennik" ), CONTRIB_DEV },

    { wxT( "Morgan Quigley" ), CONTRIB_DEV },

    { wxT( "Zlatan Radovanovic" ), CONTRIB_DEV },
    { wxT( "Barabas Raffai" ), CONTRIB_DEV },
    { wxT( "Urja Rannikko" ), CONTRIB_DEV },
    { wxT( "Alexander Rauth" ), CONTRIB_DEV },
    { wxT( "Hendrik v. Raven" ), CONTRIB_DEV },
    { wxT( "Joshua Redstone" ), CONTRIB_DEV },
    { wxT( "David Rees" ), CONTRIB_DEV },
    { wxT( "Michele Renda" ), CONTRIB_DEV },
    { wxT( "Jean-Samuel Reynaud" ), CONTRIB_DEV },
    { wxT( "Dmitry Rezvanov" ), CONTRIB_DEV },
    { wxT( "Simon Richter" ), CONTRIB_DEV },
    { wxT( "Christoph Riehl" ), CONTRIB_DEV },
    { wxT( "Thiadmer Riemersma" ), CONTRIB_DEV },
    { wxT( "Gregor Riepl" ), CONTRIB_DEV },
    { wxT( "RigoLigoRLC" ), CONTRIB_DEV },
    { wxT( "Ola Rinta-Koski" ), CONTRIB_DEV },
    { wxT( "Lubomir Rintel" ), CONTRIB_DEV },
    { wxT( "Érico Rolim" ), CONTRIB_DEV },
    { wxT( "Marcus A. Romer" ), CONTRIB_DEV },
    { wxT( "Heiko Rosemann" ), CONTRIB_DEV },
    { wxT( "Fabio Rossi" ), CONTRIB_DEV },
    { wxT( "Ian Roth" ), CONTRIB_DEV },
    { wxT( "Huang Rui" ), CONTRIB_DEV },

    { wxT( "Clément Saccoccio" ), CONTRIB_DEV },
    { wxT( "J. Morio Sakaguchi" ), CONTRIB_DEV },
    { wxT( "Simon Schaak" ), CONTRIB_DEV },
    { wxT( "Olliver Schinagl" ), CONTRIB_DEV },
    { wxT( "Ross Schlaikjer" ), CONTRIB_DEV },
    { wxT( "Julius Schmidt" ), CONTRIB_DEV },
    { wxT( "Marvin Schmidt" ), CONTRIB_DEV },
    { wxT( "Felix Schneider" ), CONTRIB_DEV },
    { wxT( "David Schneider" ), CONTRIB_DEV },
    { wxT( "Carsten Schoenert" ), CONTRIB_DEV },
    { wxT( "Armin Schoisswohl" ), CONTRIB_DEV },
    { wxT( "Simon Schubert" ), CONTRIB_DEV },
    { wxT( "Michal Schulz" ), CONTRIB_DEV },
    { wxT( "Adrian Scripca" ), CONTRIB_DEV },
    { wxT( "Pradeepa Senanayake" ), CONTRIB_DEV },
    { wxT( "Alihossein Sepahvand" ), CONTRIB_DEV },
    { wxT( "Marco Serantoni" ), CONTRIB_DEV },
    { wxT( "Julien Serin" ), CONTRIB_DEV },
    { wxT( "Frank Severinsen" ), CONTRIB_DEV },
    { wxT( "Cheng Sheng" ), CONTRIB_DEV },
    { wxT( "Yang Sheng" ), CONTRIB_DEV },
    { wxT( "Chetan Shinde" ), CONTRIB_DEV },
    { wxT( "Alexander Shuklin" ), CONTRIB_DEV },
    { wxT( "Guillaume Simard" ), CONTRIB_DEV },
    { wxT( "Adam Simpkins" ), CONTRIB_DEV },
    { wxT( "Slawomir Siudym" ), CONTRIB_DEV },
    { wxT( "Martin Sivak" ), CONTRIB_DEV },
    { wxT( "Mateusz Skowroński" ), CONTRIB_DEV },
    { wxT( "Dominik Sliwa" ), CONTRIB_DEV },
    { wxT( "Blake Smith" ), CONTRIB_DEV },
    { wxT( "Ikoma So" ), CONTRIB_DEV },
    { wxT( "Michal Sojka" ), CONTRIB_DEV },
    { wxT( "Rafael Sokolowski" ), CONTRIB_DEV },
    { wxT( "Vesa Solonen" ), CONTRIB_DEV },
    { wxT( "Ronald Sousa" ), CONTRIB_DEV },
    { wxT( "Craig Southeren" ), CONTRIB_DEV },
    { wxT( "Thomas Spindler" ), CONTRIB_DEV },
    { wxT( "Seppe Stas" ), CONTRIB_DEV },
    { wxT( "Bernhard Stegmaier" ), CONTRIB_DEV },
    { wxT( "Michael Steinberg" ), CONTRIB_DEV },
    { wxT( "Marco Sterbik" ), CONTRIB_DEV },
    { wxT( "Alexander Stock" ), CONTRIB_DEV },
    { wxT( "Martin Stoilov" ), CONTRIB_DEV },
    { wxT( "Michal Suchánek" ), CONTRIB_DEV },
    { wxT( "Hiroki Suenaga" ), CONTRIB_DEV },
    { wxT( "Kuba Sunderland-Ober" ), CONTRIB_DEV },
    { wxT( "Kacper Słomiński" ), CONTRIB_DEV },

    { wxT( "Nimish Telang" ), CONTRIB_DEV },
    { wxT( "Martin Thierer" ), CONTRIB_DEV },
    { wxT( "Karl Thorén" ), CONTRIB_DEV },
    { wxT( "Hiroshi Tokita" ), CONTRIB_DEV },
    { wxT( "Daniel Treffenstädt" ), CONTRIB_DEV },
    { wxT( "Salvador E. Tropea" ), CONTRIB_DEV },

    { wxT( "Vladimir Ur" ), CONTRIB_DEV },
    { wxT( "Yon Uriarte" ), CONTRIB_DEV },
    { wxT( "Matthias Urlichs" ), CONTRIB_DEV },
    { wxT( "Vladimir Uryvaev" ), CONTRIB_DEV },

    { wxT( "Mark van Doesburg" ), CONTRIB_DEV },
    { wxT( "Edwin van den Oetelaar" ), CONTRIB_DEV },
    { wxT( "Julie Vairai" ), CONTRIB_DEV },
    { wxT( "Andrej Valek" ), CONTRIB_DEV },
    { wxT( "Henri Valta" ), CONTRIB_DEV },
    { wxT( "Dave Vandenbout" ), CONTRIB_DEV },
    { wxT( "Raman Varabets" ), CONTRIB_DEV },
    { wxT( "Fabio Varesano" ), CONTRIB_DEV },
    { wxT( "Akhil Velagapudi" ), CONTRIB_DEV },
    { wxT( "Emmanuel Vera" ), CONTRIB_DEV },
    { wxT( "Benjamin Vernoux" ), CONTRIB_DEV },
    { wxT( "Frank Villaro-Dixon" ), CONTRIB_DEV },
    { wxT( "Mark Visser" ), CONTRIB_DEV },
    { wxT( "Forrest Voight" ), CONTRIB_DEV },
    { wxT( "Tormod Volden" ), CONTRIB_DEV },
    { wxT( "Nils van Zuijlen" ), CONTRIB_DEV },

    { wxT( "Bartek Wacławik" ), CONTRIB_DEV },
    { wxT( "Johannes Wågen" ), CONTRIB_DEV },
    { wxT( "Oliver Walters" ), CONTRIB_DEV },
    { wxT( "Jonathan Warner" ), CONTRIB_DEV },
    { wxT( "Dan Weatherill" ), CONTRIB_DEV },
    { wxT( "Stefan Weber" ), CONTRIB_DEV },
    { wxT( "Christian Weickhmann" ), CONTRIB_DEV },
    { wxT( "Bevan Weiss" ), CONTRIB_DEV },
    { wxT( "Simon Wells" ), CONTRIB_DEV },
    { wxT( "Dominik Wernberger" ), CONTRIB_DEV },
    { wxT( "Martin Whitaker" ), CONTRIB_DEV },
    { wxT( "Addo White" ), CONTRIB_DEV },
    { wxT( "Jan Wichmann" ), CONTRIB_DEV },
    { wxT( "Bernhard M. Wiedemann" ), CONTRIB_DEV },
    { wxT( "Nick Winters" ), CONTRIB_DEV },
    { wxT( "Adam Wolf" ), CONTRIB_DEV },
    { wxT( "Andrzej Wolski" ), CONTRIB_DEV },
    { wxT( "Céleste Wouters" ), CONTRIB_DEV },
    { wxT( "Damian Wrobel" ), CONTRIB_DEV },
    { wxT( "Andrew Wygle" ), CONTRIB_DEV },
    { wxT( "Adam Wysocki" ), CONTRIB_DEV },

    { wxT( "xx" ), CONTRIB_DEV },

    { wxT( "Jiaxun Yang" ), CONTRIB_DEV },
    { wxT( "Robert Yates" ), CONTRIB_DEV },
    { wxT( "Yegor Yefremov" ), CONTRIB_DEV },
    { wxT( "Kenta Yonekura" ), CONTRIB_DEV },

    { wxT( "Alexander Zakamaldin" ), CONTRIB_DEV },
    { wxT( "Frank Zeeman" ), CONTRIB_DEV },
    { wxT( "Karl Zeilhofer" ), CONTRIB_DEV },
    { wxT( "Henner Zeller" ), CONTRIB_DEV },
    { wxT( "Kevin Zheng" ), CONTRIB_DEV },
    { wxT( "Andrew Zonenberg" ), CONTRIB_DEV },

    { wxT( "wh201906" ), CONTRIB_DEV },
    { wxT( "Nick Østergaard" ), CONTRIB_DEV },
    { wxT( "木 王" ), CONTRIB_DEV },
};


static const wxChar* const s_docWriters[] = {
    wxT( "Scott Candey" ),
    wxS( "Jean-Pierre Charras" ),
    wxS( "Marco Ciampa" ),
    wxS( "Jon Evans" ),
    wxS( "Dick Hollenbeck" ),
    wxS( "James Jackson" ),
    wxS( "Graham Keeth" ),
    wxS( "Igor Plyatov" ),
    wxS( "Wayne Stambaugh" ),
    wxS( "Fabrizio Tappero" ),
    wxS( "taotieren" ),
};


static const TRANSLATOR s_translators[] = {
    { wxT( "Radovan Blažek" ), wxS( "Czech (CS)" ) },
    { wxT( "Ondřej Čertík" ), wxS( "Czech (CS)" ) },
    { wxT( "Martin Kratoška" ), wxS( "Czech (CS)" ) },
    { wxT( "Michal Kundrát" ), wxS( "Czech (CS)" ) },
    { wxT( "Radek Kuznik" ), wxS( "Czech (CS)" ) },
    { wxT( "Roman Ondráček" ), wxS( "Czech (CS)" ) },
    { wxT( "Petr Pazourek" ), wxS( "Czech (CS)" ) },
    { wxT( "René Široký" ), wxS( "Czech (CS)" ) },
    { wxT( "Hynek Štětina" ), wxS( "Czech (CS)" ) },
    { wxT( "Jan Straka" ), wxS( "Czech (CS)" ) },
    { wxT( "Andrej Valek" ), wxS( "Czech (CS)" ) },
    { wxT( "Jan Vykydal" ), wxS( "Czech (CS)" ) },

    { wxS( "Mads Dyrmann" ), wxS( "Danish (DA)" ) },
    { wxS( "Henrik Kauhanen" ), wxS( "Danish (DA)" ) },
    { wxS( "Nick Østergaard" ), wxS( "Danish (DA)" ) },

    { wxS( "Ettore Atalan" ), wxS( "German (DE)" ) },
    { wxS( "Ivan Chuba" ), wxS( "German (DE)" ) },
    { wxS( "Julian Daube" ), wxS( "German (DE)" ) },
    { wxS( "Benedikt Freisen" ), wxS( "German (DE)" ) },
    { wxS( "Jonathan Haas" ), wxS( "German (DE)" ) },
    { wxT( "Mark Hämmerling" ), wxS( "German (DE)" ) },
    { wxT( "Henrik Kauhanen" ), wxS( "German (DE)" ) },
    { wxT( "Johannes Maibaum" ), wxS( "German (DE)" ) },
    { wxT( "Mathias Neumann" ), wxS( "German (DE)" ) },
    { wxT( "Ken Ovo" ), wxS( "German (DE)" ) },
    { wxT( "Christian Schlüter" ), wxS( "German (DE)" ) },
    { wxT( "Karl Schuh" ), wxS( "German (DE)" ) },
    { wxT( "Frank Sonnenberg" ), wxS( "German (DE)" ) },
    { wxT( "Lauritz Tieste" ), wxS( "German (DE)" ) },
    { wxT( "Dominik Wernberger" ), wxS( "German (DE)" ) },

    { wxT( "Theodoros Asimakopoulos" ), wxS( "Greek (el_GR)" ) },
    { wxS( "Aristeidis Kimirtzis" ), wxS( "Greek (el_GR)" ) },
    { wxS( "Milonas Kostas" ), wxS( "Greek (el_GR)" ) },
    { wxS( "Michail Misirlis" ), wxS( "Greek (el_GR)" ) },
    { wxS( "Manolis Stefanis" ), wxS( "Greek (el_GR)" ) },
    { wxS( "Athanasios Vlastos" ), wxS( "Greek (el_GR)" ) },

    { wxT( "Adolfo Jayme Barrientos" ), wxS( "Spanish (ES)" ) },
    { wxT( "Roberto Fernandez Bautista" ), wxS( "Spanish (ES)" ) },
    { wxT( "Pablo Bianchi" ), wxS( "Spanish (ES)" ) },
    { wxT( "Echedey" ), wxS( "Spanish (ES)" ) },
    { wxT( "Iñigo Figuero" ), wxS( "Spanish (ES)" ) },
    { wxT( "Augusto Fraga Giachero" ), wxS( "Spanish (ES)" ) },
    { wxT( "Ulices Avila Hernandez" ), wxS( "Spanish (ES)" ) },
    { wxS( "Gabriel Martinez" ), wxS( "Spanish (ES)" ) },
    { wxT( "Tomás Mora" ), wxS( "Spanish (ES)" ) },
    { wxT( "Gallego Novato" ), wxS( "Spanish (ES)" ) },
    { wxT( "Jose Perez" ), wxS( "Spanish (ES)" ) },
    { wxT( "Francisco Jose Rey" ), wxS( "Spanish (ES)" ) },
    { wxT( "Gaston Schelotto" ), wxS( "Spanish (ES)" ) },
    { wxT( "uLe" ), wxS( "Spanish (ES)" ) },
    { wxS( "Pedro Martin del Valle" ), wxS( "Spanish (ES)" ) },
    { wxT( "VicSanRoPe" ), wxS( "Spanish (ES)" ) },
    { wxT( "Iñigo Zuluaga" ), wxS( "Spanish (ES)" ) },

    { wxT( "Ulices Avila Hernandez" ), wxS( "Spanish - Latin American (ES)" ) },
    { wxT( "lylythechosenone" ), wxS( "Spanish - Latin American (ES)" ) },
    { wxT( "uLe" ), wxS( "Spanish - Latin American (ES)" ) },
    { wxT( "VicSanRoPe" ), wxS( "Spanish - Latin American (ES)" ) },

    { wxT( "Alex Gellen" ), wxS( "Finnish (FI)" ) },
    { wxT( "Henrik Kauhanen" ), wxS( "Finnish (FI)" ) },
    { wxT( "Purkka Koodari" ), wxS( "Finnish (FI)" ) },
    { wxT( "Toni Laiho" ), wxS( "Finnish (FI)" ) },
    { wxT( "J. Lavoie" ), wxS( "Finnish (FI)" ) },
    { wxT( "Simo Mattila" ), wxS( "Finnish (FI)" ) },
    { wxT( "Petri Niemelä" ), wxS( "Finnish (FI)" ) },
    { wxT( "Ola Rinta-Koski" ), wxS( "Finnish (FI)" ) },
    { wxT( "Vesa Solonen" ), wxS( "Finnish (FI)" ) },
    { wxT( "Ricky Tigg" ), wxS( "Finnish (FI)" ) },
    { wxT( "Riku Viitanen" ), wxS( "Finnish (FI)" ) },

    { wxT( "Jean-Pierre Charras" ), wxS( "French (FR)" ) },

    { wxT( "Boromyr" ), wxS( "Italian (IT)" ) },
    { wxT( "Marco Ciampa" ), wxS( "Italian (IT)" ) },
    { wxT( "Luca Mattii" ), wxS( "Italian (IT)" ) },

    { wxT( "2tama3" ), wxS( "Japanese (JA)" ) },
    { wxT( "Subaru Arai" ), wxS( "Japanese (JA)" ) },
    { wxT( "Ji Yoon Choi" ), wxS( "Japanese (JA)" ) },
    { wxT( "Hidemichi Gotou" ), wxS( "Japanese (JA)" ) },
    { wxT( "Kinichiro Inoguchi" ), wxS( "Japanese (JA)" ) },
    { wxT( "co8 j" ), wxS( "Japanese (JA)" ) },
    { wxT( "Keisuke Nakao" ), wxS( "Japanese (JA)" ) },
    { wxT( "starfort-jp" ), wxS( "Japanese (JA)" ) },
    { wxT( "Norio Suzuki" ), wxS( "Japanese (JA)" ) },
    { wxT( "Hiroshi Tokita" ), wxS( "Japanese (JA)" ) },
    { wxT( "Yutaro Urata" ), wxS( "Japanese (JA)" ) },
    { wxT( "Kenta Yonekura" ), wxS( "Japanese (JA)" ) },
    { wxT( "Kaoru Zenyouji" ), wxS( "Japanese (JA)" ) },

    { wxT( "Minsu Kim (0xGabriel)" ), wxS( "Korean (KO)" ) },
    { wxT( "Ji Yoon Choi" ), wxS( "Korean (KO)" ) },
    { wxT( "DevAny" ), wxS( "Korean (KO)" ) },
    { wxT( "hokim" ), wxS( "Korean (KO)" ) },
    { wxT( "jehunseo" ), wxS( "Korean (KO)" ) },
    { wxT( "jeong-sangwon" ), wxS( "Korean (KO)" ) },
    { wxT( "jeongsuAn" ), wxS( "Korean (KO)" ) },
    { wxT( "Uibeom Jung" ), wxS( "Korean (KO)" ) },
    { wxT( "kmn4555" ), wxS( "Korean (KO)" ) },
    { wxT( "KwonHyeokbeom" ), wxS( "Korean (KO)" ) },
    { wxT( "Pedro Moreira" ), wxS( "Korean (KO)" ) },
    { wxT( "Jason Son" ), wxS( "Korean (KO)" ) },
    { wxT( "YunJiSang" ), wxS( "Korean (KO)" ) },
    { wxT( "강명구" ), wxS( "Korean (KO)" ) },
    { wxT( "김낙환" ), wxS( "Korean (KO)" ) },
    { wxT( "김랑기" ), wxS( "Korean (KO)" ) },
    { wxT( "김세영" ), wxS( "Korean (KO)" ) },
    { wxT( "김용재" ), wxS( "Korean (KO)" ) },
    { wxT( "김유진" ), wxS( "Korean (KO)" ) },
    { wxT( "김인수" ), wxS( "Korean (KO)" ) },
    { wxT( "김호진" ), wxS( "Korean (KO)" ) },
    { wxT( "남우근" ), wxS( "Korean (KO)" ) },
    { wxT( "박기정" ), wxS( "Korean (KO)" ) },
    { wxT( "박세훈" ), wxS( "Korean (KO)" ) },
    { wxT( "박준언" ), wxS( "Korean (KO)" ) },
    { wxT( "방준영" ), wxS( "Korean (KO)" ) },
    { wxT( "서범기" ), wxS( "Korean (KO)" ) },
    { wxT( "이기형" ), wxS( "Korean (KO)" ) },
    { wxT( "이상수" ), wxS( "Korean (KO)" ) },
    { wxT( "이윤성" ), wxS( "Korean (KO)" ) },
    { wxT( "킴슨김랑기" ), wxS( "Korean (KO)" ) },

    { wxT( "Ignas Brašiškis" ), wxS( "Lithuanian (LT)" ) },
    { wxT( "Henrik Kauhanen" ), wxS( "Lithuanian (LT)" ) },
    { wxT( "Dainius Mazuika" ), wxS( "Lithuanian (LT)" ) },
    { wxT( "WhiteChairFromIkea" ), wxS( "Lithuanian (LT)" ) },

    { wxT( "Arend-Jan van Hilten" ), wxS( "Dutch (NL)" ) },
    { wxT( "CJ van der Hoeven" ), wxS( "Dutch (NL)" ) },
    { wxT( "Laurens Holst" ), wxS( "Dutch (NL)" ) },
    { wxT( "Pim Jansen" ), wxS( "Dutch (NL)" ) },
    { wxT( "Robin Janssens" ), wxS( "Dutch (NL)" ) },
    { wxT( "johanneswilkens" ), wxS( "Dutch (NL)" ) },
    { wxT( "Henrik Kauhanen" ), wxS( "Dutch (NL)" ) },
    { wxT( "Tom Niesse" ), wxS( "Dutch (NL)" ) },
    { wxT( "Christiaan Nieuwlaat" ), wxS( "Dutch (NL)" ) },
    { wxT( "Stefan De Raedemaeker" ), wxS( "Dutch (NL)" ) },
    { wxT( "Ranforingus" ), wxS( "Dutch (NL)" ) },
    { wxT( "Herman van der Vaart" ), wxS( "Dutch (NL)" ) },
    { wxT( "Bas Wijnen" ), wxS( "Dutch (NL)" ) },

    { wxT( "Jarl Gjessing" ), wxS( "Norwegian (NO)" ) },
    { wxT( "Henrik Kauhanen" ), wxS( "Norwegian (NO)" ) },
    { wxT( "Stian Kristensen" ), wxS( "Norwegian (NO)" ) },
    { wxT( "Allan Nordhøy" ), wxS( "Norwegian (NO)" ) },
    { wxT( "Petter Reinholdtsen" ), wxS( "Norwegian (NO)" ) },
    { wxT( "Håvard Syslak" ), wxS( "Norwegian (NO)" ) },

    { wxT( "Ivan Chuba" ), wxS( "Polish (PL)" ) },
    { wxT( "Czam Ciał" ), wxS( "Polish (PL)" ) },
    { wxT( "Kerusey Karyu" ), wxS( "Polish (PL)" ) },
    { wxT( "Krzysztof Kawa" ), wxS( "Polish (PL)" ) },
    { wxT( "J Kolod" ), wxS( "Polish (PL)" ) },
    { wxT( "maksz42" ), wxS( "Polish (PL)" ) },
    { wxT( "Eryk Michalak" ), wxS( "Polish (PL)" ) },
    { wxT( "Filip Piękoś" ), wxS( "Polish (PL)" ) },
    { wxT( "Pomian" ), wxS( "Polish (PL)" ) },
    { wxT( "Mark Roszko" ), wxS( "Polish (PL)" ) },
    { wxT( "Mateusz Skowroński" ), wxS( "Polish (PL)" ) },
    { wxT( "Jan Sobków" ), wxS( "Polish (PL)" ) },
    { wxT( "szumsky" ), wxS( "Polish (PL)" ) },
    { wxT( "Grzegorz Szymaszek" ), wxS( "Polish (PL)" ) },
    { wxT( "ZbeeGin" ), wxS( "Polish (PL)" ) },

    { wxT( "brunofaus" ), wxS( "Brazilian Portuguese (PT_BR)" ) },
    { wxT( "Augusto Fraga Giachero" ), wxS( "Brazilian Portuguese (PT_BR)" ) },
    { wxT( "Hildo Guillardi Júnior" ), wxS( "Brazilian Portuguese (PT_BR)" ) },
    { wxT( "Pedro Moreira" ), wxS( "Brazilian Portuguese (PT_BR)" ) },
    { wxT( "soldado-do-wolfenstein" ), wxS( "Brazilian Portuguese (PT_BR)" ) },
    { wxT( "Wellington Terumi Uemura" ), wxS( "Brazilian Portuguese (PT_BR)" ) },

    { wxT( "Julio Dias" ), wxS( "Portuguese (PT)" ) },
    { wxT( "Augusto Fraga Giachero" ), wxS( "Portuguese (PT)" ) },
    { wxT( "Hildo Guillardi Júnior" ), wxS( "Portuguese (PT)" ) },
    { wxT( "leonardokr" ), wxS( "Portuguese (PT)" ) },
    { wxT( "Renie Marquet" ), wxS( "Portuguese (PT)" ) },
    { wxT( "Rafael Silva" ), wxS( "Portuguese (PT)" ) },
    { wxT( "Manuela Silva" ), wxS( "Portuguese (PT)" ) },
    { wxT( "ssantos" ), wxS( "Portuguese (PT)" ) },

    { wxT( "Konstantin Baranovskiy" ), wxS( "Russian (RU)" ) },
    { wxT( "Ivan Chuba" ), wxS( "Russian (RU)" ) },
    { wxT( "Andrey Fedorushkov" ), wxS( "Russian (RU)" ) },
    { wxT( "Free_squire" ), wxS( "Russian (RU)" ) },
    { wxT( "Alevtina Karashokova" ), wxS( "Russian (RU)" ) },
    { wxT( "Eldar Khayrullin" ), wxS( "Russian (RU)" ) },
    { wxT( "Alex Life" ), wxS( "Russian (RU)" ) },
    { wxT( "Dmitry Mikhirev" ), wxS( "Russian (RU)" ) },
    { wxT( "Igor Plyatov" ), wxS( "Russian (RU)" ) },
    { wxT( "sergio" ), wxS( "Russian (RU)" ) },
    { wxT( "xXx" ), wxS( "Russian (RU)" ) },
    { wxT( "Дмитрий Дёмин" ), wxS( "Russian (RU)" ) },
    { wxT( "МАН69К" ), wxS( "Russian (RU)" ) },

    { wxT( "Hanna Breisand" ), wxS( "Swedish (SV)" ) },
    { wxT( "Stefan Bjornelund the Gnome" ), wxS( "Swedish (SV)" ) },
    { wxT( "Johan Heikkilä" ), wxS( "Swedish (SV)" ) },
    { wxT( "Axel Henriksson"  ), wxS( "Swedish (SV)" ) },
    { wxT( "Richard Jonsson" ), wxS( "Swedish (SV)" ) },
    { wxT( "Henrik Kauhanen" ), wxS( "Swedish (SV)" ) },
    { wxT( "Joakim Lundborg" ), wxS( "Swedish (SV)" ) },
    { wxT( "Allan Nordhøy" ), wxS( "Swedish (SV)" ) },
    { wxT( "Elias Sjögreen" ), wxS( "Swedish (SV)" ) },

    { wxT( "Boonchai Kingrungped" ), wxS( "Thai (TH)" ) },

    { wxT( "Artem" ), wxS( "Ukrainian (UK)" ) },
    { wxT( "Ivan Chuba" ), wxS( "Ukrainian (UK)" ) },
    { wxT( "Stanislav Kaliuk" ), wxS( "Ukrainian (UK)" ) },
    { wxT( "Alexsandr Kuzemko" ), wxS( "Ukrainian (UK)" ) },
    { wxT( "Andrii Shelestov" ), wxS( "Ukrainian (UK)" ) },
    { wxT( "Максим Горпиніч" ), wxS( "Ukrainian (UK)" ) },

    { wxT( "CharlieYu" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "David Chen" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Dingzhong Chen" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "CloverGit" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Eric" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Liu Guang" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "HalfSweet" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Hubert Hu" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "aisuneko icecat" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Pinpang Liao" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Rigo Ligo" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Huanyin Liu" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Zhen Sun" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Jason Tan" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Taotieren" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "yangyangdaji" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Li Yi" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Li Yidong" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Tian Yunhao" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "Lao Zhu" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "yanzhen zhu" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "zly20129" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "向阳阳" ), wxS( "Simplified Chinese (zh_CN)" ) },
    { wxT( "欠陥電気" ), wxS( "Simplified Chinese (zh_CN)" ) },

    { wxT( "David Chen" ), wxS( "Traditional Chinese (zh_TW)" ) },
    { wxT( "kai chiao chuang" ), wxS( "Traditional Chinese (zh_TW)" ) },
    { wxT( "pon dahai" ), wxS( "Traditional Chinese (zh_TW)" ) },
    { wxT( "Shuwn Hsu" ), wxS( "Traditional Chinese (zh_TW)" ) },
    { wxT( "Poming Lee" ), wxS( "Traditional Chinese (zh_TW)" ) },
    { wxT( "William Lin" ), wxS( "Traditional Chinese (zh_TW)" ) },
    { wxT( "Oliver0804" ), wxS( "Traditional Chinese (zh_TW)" ) },
    { wxT( "reimu105" ), wxS( "Traditional Chinese (zh_TW)" ) },
    { wxT( "Che-Hsien Su" ), wxS( "Traditional Chinese (zh_TW)" ) },
    { wxT( "Taotieren" ), wxS( "Traditional Chinese (zh_TW)" ) },
    { wxT( "Li Yidong" ), wxS( "Traditional Chinese (zh_TW)" ) },
    { wxT( "撒景賢" ), wxS( "Traditional Chinese (zh_TW)" ) },

    { wxT( "Hesham Eina Abdalla" ), wxS( "Arabic (AR)" ) },
    { wxT( "Djamel Dellaa" ), wxS( "Arabic (AR)" ) },
    { wxT( "Ahmed Elswah" ), wxS( "Arabic (AR)" ) },
    { wxT( "HADJAISSA" ), wxS( "Arabic (AR)" ) },
    { wxT( "Morad Tamer" ), wxS( "Arabic (AR)" ) },

    { wxT( "Adolfo Jayme Barrientos" ), wxS( "Catalan (CA)" ) },
    { wxT( "Marc de Miguel" ), wxS( "Catalan (CA)" ) },
    { wxT( "Rafael Serrano" ), wxS( "Catalan (CA)" ) },
    { wxT( "Arnau Llovet Vidal" ), wxS( "Catalan (CA)" ) },

    { wxT( "Ivan Chuba" ), wxS( "Estonian (ET)" ) },

    { wxT( "Temuri Doghonadze" ), wxS( "Georgian (KA)" ) },

    { wxT( "Viktor Döme" ), wxS( "Hungarian (HU)" ) },
    { wxT( "István Farkas" ), wxS( "Hungarian (HU)" ) },
    { wxT( "Flórián Fuszkó" ), wxS( "Hungarian (HU)" ) },
    { wxT( "Sárkány Lőrinc" ), wxS( "Hungarian (HU)" ) },
    { wxT( "Miklós Márton" ), wxS( "Hungarian (HU)" ) },
    { wxT( "Balázs Meskó" ), wxS( "Hungarian (HU)" ) },
    { wxT( "Hajdu Norbert" ), wxS( "Hungarian (HU)" ) },
    { wxT( "Elek Zoltán" ), wxS( "Hungarian (HU)" ) },

    { wxT( "Reza Almanda" ), wxS( "Indonesian (ID)" ) },
    { wxT( "Jacque Fresco" ), wxS( "Indonesian (ID)" ) },
    { wxT( "Neko Nekowazarashi" ), wxS( "Indonesian (ID)" ) },
    { wxT( "Triyan W. Nugroho" ), wxS( "Indonesian (ID)" ) },

    { wxT( "Rihards Skuja" ), wxS( "Latvian (LV)" ) },

    { wxT( "Mahdi Ahmadzadeh" ), wxS( "Persian (FA)" ) },

    { wxT( "Nicoara Alex" ), wxS( "Romanian (RO)" ) },
    { wxT( "Tatu Bogdan" ), wxS( "Romanian (RO)" ) },
    { wxT( "Alex Gellen" ), wxS( "Romanian (RO)" ) },
    { wxT( "Adrian Scripcă" ), wxS( "Romanian (RO)" ) },

    { wxT( "Luka Borkovic" ), wxS( "Serbian (SR)" ) },

    { wxT( "David Chorváth" ), wxS( "Slovak (SK)" ) },
    { wxT( "Marcel Hecko" ), wxS( "Slovak (SK)" ) },
    { wxT( "Jakub Janek" ), wxS( "Slovak (SK)" ) },
    { wxT( "Andrej Valek" ), wxS( "Slovak (SK)" ) },

    { wxT( "Sašo Domadenik" ), wxS( "Slovenian (SI)" ) },
    { wxT( "Vitan Košpenda" ), wxS( "Slovenian (SI)" ) },
    { wxT( "Mitja Nemec" ), wxS( "Slovenian (SI)" ) },

    { wxT( "தமிழ்நேரம்" ), wxS( "Tamil (TA)" ) },

    { wxT( "YÜKSEL AÇIKGÖZ" ), wxS( "Turkish (TR)" ) },
    { wxT( "Argeolog" ), wxS( "Turkish (TR)" ) },
    { wxT( "Mahsum Aslan" ), wxS( "Turkish (TR)" ) },
    { wxT( "Tevfik Bagcivan" ), wxS( "Turkish (TR)" ) },
    { wxT( "Bahtiyar Bayram" ), wxS( "Turkish (TR)" ) },
    { wxT( "Marine Biologist" ), wxS( "Turkish (TR)" ) },
    { wxT( "Mustafa Selçuk ÇAVDAR" ), wxS( "Turkish (TR)" ) },
    { wxT( "dogukansahil" ), wxS( "Turkish (TR)" ) },
    { wxT( "Erkan" ), wxS( "Turkish (TR)" ) },
    { wxT( "Oğuz Ersen" ), wxS( "Turkish (TR)" ) },
    { wxT( "İclal Gör" ), wxS( "Turkish (TR)" ) },
    { wxT( "Mert Gülsoy" ), wxS( "Turkish (TR)" ) },
    { wxT( "Mert Kalkancı" ), wxS( "Turkish (TR)" ) },
    { wxT( "metin kiruc" ), wxS( "Turkish (TR)" ) },
    { wxT( "Gökhan Koçmarlı" ), wxS( "Turkish (TR)" ) },
    { wxT( "Niyazi" ), wxS( "Turkish (TR)" ) },
    { wxT( "Ahmet Saygın ÖĞÜLMÜŞ" ), wxS( "Turkish (TR)" ) },
    { wxT( "Ertuğrul Reisoğlu" ), wxS( "Turkish (TR)" ) },
    { wxT( "Murat Ursavaş" ), wxS( "Turkish (TR)" ) },
    { wxT( "VEDAT YAMAN" ), wxS( "Turkish (TR)" ) },

    { wxT( "Nguyen Van Dien" ), wxS( "Vietnamese (VI)" ) },
    { wxT( "Trần Phi Hải" ), wxS( "Vietnamese (VI)" ) },
    { wxT( "Nguyễn Ngọc Khánh" ), wxS( "Vietnamese (VI)" ) },
    { wxT( "lê văn lập" ), wxS( "Vietnamese (VI)" ) },
    { wxT( "Bế Trọng Nghĩa" ), wxS( "Vietnamese (VI)" ) },
    { wxT( "An Nguyen" ), wxS( "Vietnamese (VI)" ) },
    { wxT( "Phạm Minh Tấn" ), wxS( "Vietnamese (VI)" ) },

    { wxS( "David J S Briscoe" ), wxS( "Other" ) },
    { wxS( "Paul Burke" ), wxS( "Other" ) },
    { wxT( "Remy Halvick" ), wxS( "Other" ) },
    { wxS( "Dominique Laigle" ), wxS( "Other" ) },
};


static const LIBRARIAN s_librarians[] = {
    // Lead librarians
    { wxT( "Carsten Presser"), LIBRARIAN_TEAM },
    // Librarian trainining/recruiting
    { wxT( "Kliment Yanev" ), LIBRARIAN_TEAM },

    // Active librarians (last 2 years)
    { wxT( "Geries AbuAkel" ), LIBRARIAN_TEAM },
    { wxT( "Patrick Baus" ), LIBRARIAN_TEAM },
    { wxT( "John Beard" ), LIBRARIAN_TEAM },
    { wxT( "Jeremy Boynes" ), LIBRARIAN_TEAM },
    { wxT( "Greg Cormier" ), LIBRARIAN_TEAM },
    { wxT( "Tobias Falk" ), LIBRARIAN_TEAM },
    { wxT( "Simon Fivat" ), LIBRARIAN_TEAM },
    { wxT( "Ferrum" ), LIBRARIAN_TEAM },
    { wxT( "Jan Sebastian Götte (jaseg)" ), LIBRARIAN_TEAM },
    { wxT( "Petr Hodina" ), LIBRARIAN_TEAM },
    { wxT( "Mikkel Jeppesen" ), LIBRARIAN_TEAM },
    { wxT( "McDowell Johnson" ), LIBRARIAN_TEAM },
    { wxT( "Graham Keeth" ), LIBRARIAN_TEAM },
    { wxT( "Aristeidis Kimirtzis" ), LIBRARIAN_TEAM },
    { wxT( "Brandon Kirisaki" ), LIBRARIAN_TEAM },
    { wxT( "Thea Krug" ), LIBRARIAN_TEAM },
    { wxT( "Uli Köhler" ), LIBRARIAN_TEAM },
    { wxT( "Andrew Lutsenko" ), LIBRARIAN_TEAM },
    { wxT( "Mojca Miklavec Groenhuis" ), LIBRARIAN_TEAM },
    { wxT( "Peniel Mubita" ), LIBRARIAN_TEAM },
    { wxT( "Jorge Neiva" ), LIBRARIAN_TEAM },
    { wxT( "Carlos Nieves Ónega" ), LIBRARIAN_TEAM },
    { wxT( "Lynn Ochs" ), LIBRARIAN_TEAM },
    { wxT( "Ed Peguillan" ), LIBRARIAN_TEAM },
    { wxT( "Dash Peters" ), LIBRARIAN_TEAM },
    { wxT( "Sergio Rocha" ), LIBRARIAN_TEAM },
    { wxT( "Benjamin Reynier" ), LIBRARIAN_TEAM },
    { wxT( "Armin Schoisswohl" ), LIBRARIAN_TEAM },
    { wxT( "Joel Schulz-Andres" ), LIBRARIAN_TEAM },
    { wxT( "Frank Severinsen" ), LIBRARIAN_TEAM },
    { wxT( "Martin Sotirov" ), LIBRARIAN_TEAM },
    { wxT( "Philipp Swoboda" ), LIBRARIAN_TEAM },
    { wxT( "Christoph Werner" ), LIBRARIAN_TEAM },

    // Previously active librarians
    { wxT( "Christian Schlüter" ), LIBRARIAN_TEAM },
    { wxT( "Rene Poeschl" ), LIBRARIAN_TEAM },
    { wxT( "Antonio Vázquez Blanco " ), LIBRARIAN_TEAM },
    { wxT( "Daniel Giesbrecht" ), LIBRARIAN_TEAM },
    { wxT( "Otavio Augusto Gomes" ), LIBRARIAN_TEAM },
    { wxT( "herostrat" ), LIBRARIAN_TEAM },
    { wxT( "Diego Herranz" ), LIBRARIAN_TEAM },
    { wxT( "Joel Guittet" ), LIBRARIAN_TEAM },
    { wxT( "Chris Morgan" ), LIBRARIAN_TEAM },
    { wxT( "Thomas Pointhuber" ), LIBRARIAN_TEAM },
    { wxT( "Evan Shultz" ), LIBRARIAN_TEAM },
    { wxT( "Bob Cousins" ), LIBRARIAN_TEAM },
    { wxT( "Nick Østergaard" ), LIBRARIAN_TEAM },
    { wxT( "Oliver Walters" ), LIBRARIAN_TEAM },

    { wxS( "Scripts by Maui" ), MODELS_3D_CONTRIBUTION, wxS( "https://github.com/easyw" ) },
    { wxS( "Hasan Yavuz Özderya" ), MODELS_3D_CONTRIBUTION,
      wxS( "https://bitbucket.org/hyOzd/freecad-macros/src/master/" ) },
    { wxS( "GitHub contributors" ), MODELS_3D_CONTRIBUTION,
      wxS( "https://github.com/easyw/kicad-3d-models-in-freecad/graphs/contributors" ) },
    { wxS( "GitLab contributors" ), MODELS_3D_CONTRIBUTION,
      wxS( "https://gitlab.com/kicad/libraries/kicad-packages3D/-/graphs/master" ) },

    { wxS( "GitLab contributors" ), SYMBOL_LIB_CONTRIBUTION,
      wxS( "https://gitlab.com/kicad/libraries/kicad-symbols/-/graphs/master" ) },

    { wxS( "Scripts by Thomas Pointhuber" ), FOOTPRINT_LIB_CONTRIBUTION,
      wxS( "https://gitlab.com/kicad/libraries/kicad-footprint-generator" ) },
    { wxS( "GitLab contributors" ), FOOTPRINT_LIB_CONTRIBUTION,
      wxS( "https://gitlab.com/kicad/libraries/kicad-footprints/-/graphs/master" ) },
};


static const wxChar* const s_artists[] = {
    wxT( "Aleksandr Zyrianov" ),
    wxT( "Anda Subero" ),
    wxT( "Iñigo Zuluaga" ),
    wxS( "Fabrizio Tappero" ),
};


static const wxChar* const s_packagers[] = {
    wxS( "Steven Falco" ),
    wxS( "Johannes Maibaum" ),
    wxS( "Jean-Samuel Reynaud" ),
    wxS( "Bernhard Stegmaier" ),
    wxS( "Adam Wolf" ),
    wxT( "Nick Østergaard" ),
};


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
    const wxString devCategories[DEV_CATEGORY_COUNT] = { _( "Lead Development Team" ),
                                                         _( "Lead Development Alumni" ),
                                                         _( "Additional Contributions By" ) };

    for( const DEVELOPER& dev : s_developers )
        aInfo.AddDeveloper( new CONTRIBUTOR( dev.m_Name, devCategories[dev.m_Category] ) );

    // The document writers
    const wxString docTeam = _( "Documentation Team" );

    for( const wxChar* name : s_docWriters )
        aInfo.AddDocWriter( new CONTRIBUTOR( name, docTeam ) );

    // The translators
    for( const TRANSLATOR& translator : s_translators )
        aInfo.AddTranslator( new CONTRIBUTOR( translator.m_Name, translator.m_Language ) );

    // Program credits for library team
    const wxString librarianCategories[LIBRARIAN_CATEGORY_COUNT] = { _( "Librarian Team" ),
                                                                     _( "3D models" ),
                                                                     _( "Symbols" ),
                                                                     _( "Footprints" ) };

    for( const LIBRARIAN& librarian : s_librarians )
    {
        aInfo.AddLibrarian( new CONTRIBUTOR( librarian.m_Name,
                                             librarianCategories[librarian.m_Category],
                                             librarian.m_Url ? librarian.m_Url : wxT( "" ) ) );
    }

    // Program credits for icons
    const wxString iconContribution = _( "Icons" );

    for( const wxChar* name : s_artists )
        aInfo.AddArtist( new CONTRIBUTOR( name, iconContribution ) );

    // Program credits for package developers.
    const wxString packageDevs = _( "Package Developers" );

    for( const wxChar* name : s_packagers )
        aInfo.AddPackager( new CONTRIBUTOR( name, packageDevs ) );
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
