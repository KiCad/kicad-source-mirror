/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * Handle special data (items attributes) during plot.
 *
 * Used in Gerber plotter to generate auxiliary data during plot (for instance info associated
 * to apertures and flashed pads)
 *
 * @file gbr_metadata.h
 */

#ifndef GBR_METADATA_H
#define GBR_METADATA_H

#include <gbr_netlist_metadata.h>

/**
 * Create a gerber TF.CreationDate attribute.
 *
 * The attribute value must conform to the full version of the ISO 8601 date and time format,
 * including time and time zone.
 *
 * Example of structured comment (compatible X1 gerber)
 *  G04 #@! TF.CreationDate,2018-11-21T08:49:16+01:00* (example of X1 attribute)
 *
 * Example NC drill files
 *  ; #@! TF.CreationDate,2018-11-21T08:49:16+01:00*    (example of NC drill comment)
 *
 * Example of X2 attribute:
 *  %TF.CreationDate,2018-11-06T08:25:24+01:00*%
 *
 * @note This is the date the Gerber file is effectively created, not the time the project
 *       PCB was started.
 *
 * @param aFormat string compatibility: X1, X2, GBRJOB or NC drill syntax.
 */
enum GBR_NC_STRING_FORMAT       // Options for string format in some attribute strings
{
    GBR_NC_STRING_FORMAT_X1,
    GBR_NC_STRING_FORMAT_X2,
    GBR_NC_STRING_FORMAT_GBRJOB,
    GBR_NC_STRING_FORMAT_NCDRILL
};

wxString GbrMakeCreationDateAttributeString( GBR_NC_STRING_FORMAT aFormat );


/**
 * Build a project GUID using format RFC4122 Version 1 or 4 from the project name, because
 * a KiCad project has no specific GUID.
 *
 * RFC4122 is used mainly for its syntax, because fields have no meaning for Gerber files
 * and therefore the GUID generated has no meaning because it do not use any time and time
 * stamp specific to the project, just a random pattern (random is here a pattern specific
 * to a project).
 *
 * See en.wikipedia.org/wiki/Universally_unique_identifier
 */
wxString GbrMakeProjectGUIDfromString( const wxString& aText );


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
        GBR_APERTURE_ATTRIB_NONE,      ///< uninitialized attribute.
        GBR_APERTURE_ATTRIB_ETCHEDCMP, ///< Aperture used for etched components.

        /// Aperture used for connected items like tracks (not vias).
        GBR_APERTURE_ATTRIB_CONDUCTOR,
        GBR_APERTURE_ATTRIB_EDGECUT, ///< Aperture used for board cutout,

        /// Aperture used for not connected items (texts, outlines on copper).
        GBR_APERTURE_ATTRIB_NONCONDUCTOR,
        GBR_APERTURE_ATTRIB_VIAPAD, ///< Aperture used for vias.

        /// Aperture used for through hole component on outer layer.
        GBR_APERTURE_ATTRIB_COMPONENTPAD,

        /// Aperture used for SMD pad. Excluded BGA pads which have their own type.
        GBR_APERTURE_ATTRIB_SMDPAD_SMDEF,

        /// Aperture used for SMD pad with a solder mask defined by the solder mask.
        GBR_APERTURE_ATTRIB_SMDPAD_CUDEF,

        /// Aperture used for BGA pads with a solder mask defined by the copper shape.
        GBR_APERTURE_ATTRIB_BGAPAD_SMDEF,

        /// Aperture used for BGA pad with a solder mask defined by the solder mask.
        GBR_APERTURE_ATTRIB_BGAPAD_CUDEF,

        /// Aperture used for edge connector pad (outer layers).
        GBR_APERTURE_ATTRIB_CONNECTORPAD,
        GBR_APERTURE_ATTRIB_WASHERPAD, ///< Aperture used for mechanical pads (NPTH).
        GBR_APERTURE_ATTRIB_TESTPOINT, ///< Aperture used for test point pad (outer layers).

        /// Aperture used for fiducial pad (outer layers), at board level.
        GBR_APERTURE_ATTRIB_FIDUCIAL_GLBL,

        /// Aperture used for fiducial pad (outer layers), at footprint level.
        GBR_APERTURE_ATTRIB_FIDUCIAL_LOCAL,

        /// Aperture used for heat sink pad (typically for SMDs).
        GBR_APERTURE_ATTRIB_HEATSINKPAD,

        /// Aperture used for castellated pads in copper layer files.
        GBR_APERTURE_ATTRIB_CASTELLATEDPAD,

        /// Aperture used for castellated pads in drill files.
        GBR_APERTURE_ATTRIB_CASTELLATEDDRILL,

        /// Aperture used for pressfit pads in drill files.
        /// this is similar to GBR_APERTURE_ATTRIB_COMPONENTPAD with optional PressFit field
        GBR_APERTURE_ATTRIB_PRESSFITDRILL,

        ///< Aperture used for via holes in drill files.
        GBR_APERTURE_ATTRIB_VIADRILL,

        ///< Aperture used for backdrill holes in drill files.
        GBR_APERTURE_ATTRIB_BACKDRILL,

        ///< Aperture used for pad holes in drill files.
        GBR_APERTURE_ATTRIB_CMP_DRILL,

        /// Aperture used for pads oblong holes in drill files.
        GBR_APERTURE_ATTRIB_CMP_OBLONG_DRILL,

        /// Aperture used for flashed cmp position in placement files.
        GBR_APERTURE_ATTRIB_CMP_POSITION,

        /// Aperture used for flashed pin 1 (or A1 or AA1) position in placement files.
        GBR_APERTURE_ATTRIB_PAD1_POS,

        /// Aperture used for flashed pads position in placement files.
        GBR_APERTURE_ATTRIB_PADOTHER_POS,

        /// Aperture used to draw component physical body outline without pins in placement files.
        GBR_APERTURE_ATTRIB_CMP_BODY,

        /// Aperture used to draw component physical body outline with pins in placement files.
        GBR_APERTURE_ATTRIB_CMP_LEAD2LEAD,

        /// Aperture used to draw component footprint bounding box in placement files.
        GBR_APERTURE_ATTRIB_CMP_FOOTPRINT,

        /// Aperture used to draw component outline courtyard in placement files.
        GBR_APERTURE_ATTRIB_CMP_COURTYARD,

        ///< aperture used for other purposes. Requires a text description of this feature.
        GBR_APERTURE_ATTRIB_OTHER,
        GBR_APERTURE_ATTRIB_END ///< sentinel: max value
    };

    GBR_APERTURE_METADATA() : m_ApertAttribute( GBR_APERTURE_ATTRIB_NONE ), m_CustomAttribute( "" )
    {}

    /**
     * @return the string corresponding to the aperture attribute.
     */
    static std::string GetAttributeName( GBR_APERTURE_ATTRIB aAttribute );
    std::string GetAttributeName()
    {
        return GetAttributeName( m_ApertAttribute );
    }

    /**
     * @param aUseX1StructuredComment false in X2 mode and true in X1 mode to add the net
     *                                attribute inside a compatible X1 structured comment
     *                                starting by "G04 #@! "
     * @return the full command string corresponding to the aperture attribute
     *         like "%TA.AperFunction,<function>*%"
     */
    static std::string FormatAttribute( GBR_APERTURE_ATTRIB aAttribute,
                                        bool                aUseX1StructuredComment,
                                        const std::string&  aCustomAttribute );

    std::string FormatAttribute( bool aUseX1StructuredComment )
    {
        return FormatAttribute( m_ApertAttribute, aUseX1StructuredComment, m_CustomAttribute );
    }

    // The id of the aperture attribute
    GBR_APERTURE_ATTRIB m_ApertAttribute;

    std::string m_CustomAttribute;
};


/**
 * Metadata which can be added in a gerber file as attribute in X2 format.
 */
class GBR_METADATA
{
public:
    GBR_METADATA(): m_isCopper( false)  {}

    void SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB aApertAttribute )
    {
        m_ApertureMetadata.m_ApertAttribute = aApertAttribute;
    }

    void SetApertureAttrib( std::string aCustomAttribute )
    {
        m_ApertureMetadata.m_ApertAttribute = GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_OTHER;
        m_ApertureMetadata.m_CustomAttribute = aCustomAttribute;
    }

    GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB GetApertureAttrib()
    {
        return m_ApertureMetadata.m_ApertAttribute;
    }

    std::string GetCustomAttribute() { return m_ApertureMetadata.m_CustomAttribute; }

    void SetNetAttribType( int aNetAttribType )
    {
        m_NetlistMetadata.m_NetAttribType = aNetAttribType;
    }

    int GetNetAttribType() const
    {
        return m_NetlistMetadata.m_NetAttribType;
    }

    void SetNetName( const wxString& aNetname ) { m_NetlistMetadata.m_Netname = aNetname; }

    void SetPadName( const wxString& aPadname, bool aUseUTF8 = false, bool aEscapeString = false )
    {
        m_NetlistMetadata.m_Padname.SetField( aPadname, aUseUTF8, aEscapeString );
    }

    void SetPadPinFunction( const wxString& aPadPinFunction, bool aUseUTF8, bool aEscapeString )
    {
        m_NetlistMetadata.m_PadPinFunction.SetField( aPadPinFunction, aUseUTF8, aEscapeString );
    }

    void SetCmpReference( const wxString& aComponentRef )
    {
        m_NetlistMetadata.m_Cmpref = aComponentRef;
    }

    /**
     * Allowed attributes are not the same on board copper layers and on other layers.
     *
     * A flag can be set or reset when attributes can be depending on layers
     */
    bool IsCopper() { return m_isCopper; }
    void SetCopper( bool aValue ) { m_isCopper = aValue; }

    /**
     * An item to handle aperture attribute.
     */
    GBR_APERTURE_METADATA m_ApertureMetadata;

    /**
     * An item to handle object attribute.
     */
    GBR_NETLIST_METADATA m_NetlistMetadata;

private:
    /**
     * If the metadata is relative to a copper layer or not, this flag which can be set/reset
     * when an attribute for a given item depends on whether a copper layer or a non copper
     * layer is plotted.  The initial state i false.
     */
    bool m_isCopper;
};


/**
 * Normalize \a aString and convert it to a Gerber std::string.
 *
 * Normalization means convert any code > 0x7F and unauthorized code to a hexadecimal
 * 16 bit sequence Unicode.  Illegal characters are ',' '*' '%' '\'.
 *
 * @param aString the string to convert.
 * @return an ASCII7 coded compliant gerber string.
 */
std::string FormatStringToGerber( const wxString& aString );


/**
 * Normalize \a aString and convert it to a Gerber compatible wxString.
 *
 * Normalization means convert to a hexadecimal 16 bit sequence Unicode and on request
 * convert any code > 0x7F.  Illegal characters are ',' '*' '%' '\'.
 *
 * @param aString the string to convert.
 * @param aAllowUtf8Chars false to convert non ASCII7 values to Unicode sequence.
 * @param aQuoteString  true to double quote the returned string.
 * @return a without illegal chars (and converted non ASCII7 chars on request)
 */
wxString ConvertNotAllowedCharsInGerber( const wxString& aString, bool aAllowUtf8Chars,
                                         bool aQuoteString );

/**
 * Convert a gerber string into a 16 bit Unicode string.
 *
 * @param aString the gerber string to format.
 * @return a 16 bit Unicode string.
 */
wxString FormatStringFromGerber( const wxString& aString );

/**
 * Generate the string to set a net attribute for a graphic object to print to a gerber file.
 *
 * @param aPrintedText is the string to print.
 * @param aLastNetAttributes is the current full set of attributes.
 * @param aData is the #GBR_NETLIST_METADATA associated to the graphic object (can be NULL
 *              if no associated metadata, and aClearPreviousAttributes will be set to false)
 * @param aClearPreviousAttributes returns true if the full set of attributes must be deleted
 *                                 from file before adding new attribute (happens when a previous
 *                                 attribute no longer exists).
 * @param aUseX1StructuredComment false in X2 mode and true in X1 mode to add the net attribute
 *                                in compatible X1 structured comment (i.e. prefixed by "G04 #@! ")
 * @return false if nothing can be done (GBR_NETLIST_METADATA has GBR_APERTURE_ATTRIB_NONE,
 *         and true if OK. If the new attribute(s) is the same as current attribute(s),
 *         \a aPrintedText will be empty.
 */
bool FormatNetAttribute( std::string& aPrintedText, std::string& aLastNetAttributes,
                         const GBR_NETLIST_METADATA* aData, bool& aClearPreviousAttributes,
                         bool aUseX1StructuredComment );

#endif      // GBR_METADATA_H
