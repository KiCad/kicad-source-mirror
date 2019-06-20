/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * a class to handle special data (items attributes) during plot.
 * used in Gerber plotter to generate auxiliary data during plot
 * (for instance info associated to apertures and flashed pads)
 *
 * @file gbr_metadata.h
 */

#ifndef GBR_METADATA_H
#define GBR_METADATA_H

#include <gbr_netlist_metadata.h>

/** creates the TF.CreationDate attribute:
 * The attribute value must conform to the full version of the ISO 8601
 * date and time format, including time and time zone. Note that this is
 * the date the Gerber file is effectively created,
 * not the time the project of PCB was started
 * @param aFormat = string compatibility: X1, X2, GBRJOB or NC drill synthax.
 * exemple of structured comment (compatible X1 gerber)
 *  G04 #@! TF.CreationDate,2018-11-21T08:49:16+01:00* (exemple of X1 attribute)
 * exemple NC drill files
 *  ; #@! TF.CreationDate,2018-11-21T08:49:16+01:00*    (exemple of NC drill comment)
 * exemple of X2 attribute:
 *  %TF.CreationDate,2018-11-06T08:25:24+01:00*%
 */
enum GBR_NC_STRING_FORMAT       // Options for string format in some attribute strings
{
    GBR_NC_STRING_FORMAT_X1,
    GBR_NC_STRING_FORMAT_X2,
    GBR_NC_STRING_FORMAT_GBRJOB,
    GBR_NC_STRING_FORMAT_NCDRILL
};
wxString GbrMakeCreationDateAttributeString( GBR_NC_STRING_FORMAT aFormat );


/** A helper function to build a project GUID using format RFC4122 Version 1 or 4
 * from the project name, because a kicad project has no specific GUID
 * RFC4122 is used mainly for its syntax, because fields have no meaning for Gerber files
 * and therefore the GUID generated has no meaning because it do not use any time and time stamp
 * specific to the project, just a random pattern (random is here a pattern specific to a project).
 *
 * See en.wikipedia.org/wiki/Universally_unique_identifier
 */
wxString GbrMakeProjectGUIDfromString( wxString& aText );


// this class handle info which can be added in a gerber file as attribute
// of an aperture, by the %TA.AperFunction command
// This attribute is added when creating a new aperture (command %ADDxx)
// Only one aperture attribute can be added to a given aperture
//
class GBR_APERTURE_METADATA
{
public:
    enum GBR_APERTURE_ATTRIB
    {
        GBR_APERTURE_ATTRIB_NONE,           ///< uninitialized attribute
        GBR_APERTURE_ATTRIB_ETCHEDCMP,      ///< aperture used for etched components
        GBR_APERTURE_ATTRIB_CONDUCTOR,      ///< aperture used for connected items like tracks (not vias)
        GBR_APERTURE_ATTRIB_CUTOUT,         ///< aperture used for board cutout
        GBR_APERTURE_ATTRIB_NONCONDUCTOR,   ///< aperture used for not connected items (texts, outlines on copper)
        GBR_APERTURE_ATTRIB_VIAPAD,         ///< aperture used for vias
        GBR_APERTURE_ATTRIB_COMPONENTPAD,   ///< aperture used for through hole component on outer layer
        GBR_APERTURE_ATTRIB_SMDPAD_SMDEF,   ///< aperture used for SMD pad. Excluded BGA pads which have their own type
        GBR_APERTURE_ATTRIB_SMDPAD_CUDEF,   ///< aperture used for SMD pad with a solder mask defined by the solder mask
        GBR_APERTURE_ATTRIB_BGAPAD_SMDEF,   ///< aperture used for BGA pads with a solder mask defined by the copper shape
        GBR_APERTURE_ATTRIB_BGAPAD_CUDEF,   ///< aperture used for BGA pad with a solder mask defined by the solder mask
        GBR_APERTURE_ATTRIB_CONNECTORPAD,   ///< aperture used for edge connecto pad (outer layers)
        GBR_APERTURE_ATTRIB_WASHERPAD,      ///< aperture used for mechanical pads (NPTH)
        GBR_APERTURE_ATTRIB_HEATSINKPAD,    ///< aperture used for heat sink pad (typically for SMDs)
        GBR_APERTURE_ATTRIB_VIADRILL,       ///< aperture used for via holes in drill files
        GBR_APERTURE_ATTRIB_COMPONENTDRILL, ///< aperture used for pad holes in drill files
        GBR_APERTURE_ATTRIB_COMPONENTOBLONGDRILL, ///< aperture used for pads oblong holes in drill files
        GBR_APERTURE_ATTRIB_END             ///< sentinel: max value
    };

    GBR_APERTURE_METADATA()
        :m_ApertAttribute( GBR_APERTURE_ATTRIB_NONE )
    {}

    /**
     * @return the string corresponding to the aperture attribute
     */
    static std::string GetAttributeName( GBR_APERTURE_ATTRIB aAttribute );
    std::string GetAttributeName()
    {
        return GetAttributeName( m_ApertAttribute );
    }

    /**
     * @return the full command string corresponding to the aperture attribute
     * like "%TA.AperFunction,<function>*%"
     * @param aUseX1StructuredComment = false in X2 mode,
     * and true in X1 mode to add the net attribut
     * inside a compatible X1 structured comment starting by "G04 #@! "
     * like "G04 #@! TA.AperFunction,<function>*"
     */
    static std::string FormatAttribute( GBR_APERTURE_ATTRIB aAttribute,
                                        bool aUseX1StructuredComment );

    std::string FormatAttribute( bool aUseX1StructuredComment )
    {
        return FormatAttribute( m_ApertAttribute, aUseX1StructuredComment );
    }

    // The id of the aperture attribute
    GBR_APERTURE_ATTRIB m_ApertAttribute;
};

// this class handle metadata which can be added in a gerber file as attribute
// in X2 format
class  GBR_METADATA
{
public:
    GBR_METADATA(): m_isCopper( false)  {}

    void SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB aApertAttribute )
    {
        m_ApertureMetadata.m_ApertAttribute = aApertAttribute;
    }

    GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB GetApertureAttrib()
    {
        return m_ApertureMetadata.m_ApertAttribute;
    }

    void SetNetAttribType( int aNetAttribType )
    {
        m_NetlistMetadata.m_NetAttribType = aNetAttribType;
    }

    int GetNetAttribType() const
    {
        return m_NetlistMetadata.m_NetAttribType;
    }

    void SetNetName( const wxString& aNetname ) { m_NetlistMetadata.m_Netname = aNetname; }
    void SetPadName( const wxString& aPadname ) { m_NetlistMetadata.m_Padname = aPadname; }
    void SetCmpReference( const wxString& aComponentRef ) { m_NetlistMetadata.m_Cmpref = aComponentRef; }

    /**
     * Allowed attributes are not the same on board copper layers and on other layers
     * Therefore a flag can be set or reset when attributes can be depending on layers
     */
    bool IsCopper() { return m_isCopper; }
    void SetCopper( bool aValue ) { m_isCopper = aValue; }

    /**
     * a item to handle aperture attribute:
     */
    GBR_APERTURE_METADATA m_ApertureMetadata;

    /**
     * a item to handle object attribute:
     */
    GBR_NETLIST_METADATA m_NetlistMetadata;

private:
    /**
     * if the metadata is relative to a copper layer or not. This is a flag
     * which can be set/reset when an attribute for a given item depends on the fact
     * a copper layer or a non copper layer is plotted.
     * The initial state in false.
     */
    bool m_isCopper;
};


/**
 * This helper function "normalize" aString and convert it to a Gerber std::string
 * Normalisation means convert any code > 0x7F and unautorized code to a hexadecimal
 * 16 bits sequence unicode
 * unautorized codes are ',' '*' '%' '\'
 * @param aString = the wxString to convert
 * @return a std::string (ASCII7 coded) compliant with a gerber string
 */
std::string formatStringToGerber( const wxString& aString );

/**
 * This helper function make the inverse conversion of formatStringToGerber()
 * It converts a "normalized" gerber string and convert it to a 16 bits sequence unicode
 * @param aString = the wxString compliant with a gerber string format
 * @return a wxString (unicode 16) from the gerber string
 */
wxString FormatStringFromGerber( const wxString& aString );

/**
 * Generates the string to print to a gerber file, to set a net attribute
 * for a graphic object.
 * @param aPrintedText is the string to print
 * @param aLastNetAttributes is the current full set of attributes.
 * @param aData is the GBR_NETLIST_METADATA associated to the graphic object (can be NULL
 * if no associated metadata, and aClearPreviousAttributes will be set to false)
 * @param aClearPreviousAttributes returns true if the full set of attributes
 * must be deleted from file before adding new attribute (happens when a previous
 * attribute does not exist no more).
 * @param aUseX1StructuredComment = false in X2 mode, and true in X1 mode to add the net attribut
 * in compatible X1 structured comment (i.e. prefixed by "G04 #@! ")
 * @return false if nothing can be done (GBR_NETLIST_METADATA has GBR_APERTURE_ATTRIB_NONE,
 * and true if OK
 * if the new attribute(s) is the same as current attribute(s), aPrintedText
 * will be empty
 */
bool FormatNetAttribute( std::string& aPrintedText, std::string& aLastNetAttributes,
                         GBR_NETLIST_METADATA* aData, bool& aClearPreviousAttributes,
                         bool aUseX1StructuredComment );

#endif      // GBR_METADATA_H
