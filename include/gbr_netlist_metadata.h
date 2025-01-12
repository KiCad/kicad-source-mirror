/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef GBR_NETLIST_METADATA_H
#define GBR_NETLIST_METADATA_H

/**
 * Information which can be added in a gerber P&P file as attribute of a component.
 *
 * This is only applicable to objects having the TA.AperFunction attribute "ComponentMain"
 * There are specific attributes defined attached to the component by the %TO command
 * %TO.CRot,<angle> The rotation angle of the component.
 *   The rotation angle is consistent with the one for graphics objects.
 *   Positive rotation is counter- clockwise as viewed from the top side, even if
 *   the component is on the board side.
 *   The base orientation of component - no rotation - on the top side is as in IPC-7351.
 *   Components on the bottom side are of course mirrored.
 *   The base orientation on the bottom side is the one on the top side
 *   mirrored around the X axis.
 *
 * %TO.CMfr,<string>     Manufacturer
 * %TO.CMPN,<string>     Manufacturer part number
 * %TO.Cpkg,<string>     Package, as per IPC-7351
 * %TO.CVal,<string>     Value, a string. E.g. 220nF
 * %TO.CMnt,<string>     Mount type: (SMD|TH|Other)
 * %TO.CFtp,<string>     Footprint name, a string. E.g. LQFP-100_14x14mm_P0.5mm
                         This is the footprint name coming from the CAD tool libraries.
 * %TO.CPgN,<string>     Package name, like the JEDEC JEP95 standard.
 * %TO.CPgD,<string>     Package description.
 * %TO.CHgt,<string>     Height, a decimal, in the unit of the file.
 * %TO.CLbN,<string>     Library name.
 * %TO.CLbD,<string>     Library description.
 * %TO.Sup,<SN>,<SPN>    SN is a field with the supplier name.
 *                       SPN is a field with the supplier part name.
 */
class GBR_CMP_PNP_METADATA
{
public:
    enum MOUNT_TYPE
    {
        MOUNT_TYPE_UNSPECIFIED,
        MOUNT_TYPE_SMD,
        MOUNT_TYPE_TH
    };

    GBR_CMP_PNP_METADATA() :
        m_Orientation( 0.0 ),
        m_MountType( MOUNT_TYPE_UNSPECIFIED )
    {}

    void ClearData();           // Clear all strings

    /**
     * One line by non empty data the orientation (.CRot) and mount type (.CMnt) are always
     * generated.
     *
     * @return a string containing the formatted metadata in X2 syntax.
     */
    wxString FormatCmpPnPMetadata();


    double m_Orientation;       // orientation in degree
    wxString m_Manufacturer;    // Manufacturer name
    wxString m_MPN;             // Manufacturer part number
    wxString m_Package;         // Package, as per IPC-7351
    wxString m_Footprint;       // Footprint name, from library
    wxString m_LibraryName;     // Library name, containing the footprint
    wxString m_LibraryDescr;    // Library description
    wxString m_Value;           // Component value
    MOUNT_TYPE m_MountType;     // SMD|TH|Other
};

/**
 * A Gerber data field.
 *
 * This is a Unicode string with some chars converted in escaped hexadecimal sequence
 * when creating the file.  The following characters are  always escaped because they
 * are separator in Gerber files: * , \ %.  Non ASCII7 characters can be converted to
 * UTF8 or escaped.
 */
class GBR_DATA_FIELD
{
public:
    GBR_DATA_FIELD() : m_useUTF8( false ), m_escapeString( false )
    {}

    void clear()
    {
        m_field.clear();
        m_useUTF8 = false;
        m_escapeString = false;
    }

    void Clear() { clear(); }

    const wxString& GetValue() const { return m_field; }

    void SetField( const wxString& aField, bool aUseUTF8, bool aEscapeString )
    {
        m_field = aField;
        m_useUTF8 = aUseUTF8;
        m_escapeString = aEscapeString;
    }

    bool IsEmpty() const { return m_field.IsEmpty(); }

    std::string GetGerberString() const;

private:
    wxString m_field;       ///< the Unicode text to print in Gbr file
                            ///< (after escape and quoting)
    bool m_useUTF8;         ///< true to use UTF8, false to escape non ASCII7 chars
    bool m_escapeString;    ///< true to quote the field in gbr file
};


/**
 * Information which can be added in a gerber file as attribute of an object.
 *
 * The #GBR_INFO_TYPE types can be OR'ed to add 2 (or more) attributes.  There are only 3
 * net attributes defined attached to an object by the %TO command:
 *  - %TO.P
 *  - %TO.N
 *  - %TO.C
 *
 * The .P attribute can be used only for flashed pads (using the D03 command) and only for
 * external copper layers, if the component is on a external copper layer for other copper
 * layer items (pads on internal layers, tracks ... ), only .N and .C can be used.
 */
class GBR_NETLIST_METADATA
{
public:
    // This enum enables the different net attributes attached to the object
    // the values can be ORed for items which can have more than one attribute
    // (A flashed pad has all allowed attributes)
    enum GBR_NETINFO_TYPE
    {
        GBR_NETINFO_UNSPECIFIED,    ///< idle command (no command)
        GBR_NETINFO_PAD = 1,        ///< print info associated to a flashed pad (TO.P attribute)
        GBR_NETINFO_NET = 2,        ///< print info associated to a net (TO.N attribute)
        GBR_NETINFO_CMP = 4         ///< print info associated to a component (TO.C attribute)
    };

    GBR_NETLIST_METADATA(): m_NetAttribType( GBR_NETINFO_UNSPECIFIED ),
            m_NotInNet( false ), m_TryKeepPreviousAttributes( false )
    {
    }

    /**
     * Clear the extra data string printed at end of net attributes.
     */
    void ClearExtraData()
    {
        m_ExtraData.Clear();
    }

    /**
     * Set the extra data string printed at end of net attributes
     */
    void SetExtraData( const wxString& aExtraData)
    {
        m_ExtraData = aExtraData;
    }

    /**
     * Remove the net attribute specified by \a aName.
     *
     * If aName == NULL or empty, remove all attributes.
     *
     * @param aName is the name (.CN, .P .N or .C) of the attribute to remove.
     */
    void ClearAttribute( const wxString* aName )
    {
        if( m_NetAttribType == GBR_NETINFO_UNSPECIFIED )
        {
            m_Padname.clear();
            m_PadPinFunction.clear();
            m_Cmpref.clear();
            m_Netname.clear();
            return;
        }

        if( !aName || aName->IsEmpty() || *aName == wxT( ".CN" ) )
        {
            m_NetAttribType = GBR_NETINFO_UNSPECIFIED;
            m_Padname.clear();
            m_PadPinFunction.clear();
            m_Cmpref.clear();
            m_Netname.clear();
            return;
        }

        if( *aName == wxT( ".C" ) )
        {
            m_NetAttribType &= ~GBR_NETINFO_CMP;
            m_Cmpref.clear();
            return;
        }

        if( *aName == wxT( ".N" ) )
        {
            m_NetAttribType &= ~GBR_NETINFO_NET;
            m_Netname.clear();
            return;
        }

        if( *aName == wxT( ".P" ) )
        {
            m_NetAttribType &= ~GBR_NETINFO_PAD;
            m_Padname.clear();
            m_PadPinFunction.clear();
            return;
        }
    }

    // these members are used in the %TO object attributes command.
    int      m_NetAttribType;   ///< the type of net info
                                ///< (used to define the gerber string to create)
    bool     m_NotInNet;        ///< true if a pad of a footprint cannot be connected
                                ///< (for instance a mechanical NPTH, ot a not named pad)
                                ///< in this case the pad net name is empty in gerber file
    GBR_DATA_FIELD m_Padname;   ///< for a flashed pad: the pad name ((TO.P attribute)
    GBR_DATA_FIELD m_PadPinFunction;  ///< for a pad: the pin function (defined in schematic)
    wxString m_Cmpref;    ///< the component reference parent of the data
    wxString m_Netname;   ///< for items associated to a net: the netname

    wxString m_ExtraData;       ///< a string to print after %TO object attributes, if not empty
                                ///< it is printed "as this"
    /**
     * If true, do not clear all attributes when a attribute has changed.  This is useful
     * when some attributes need to be persistent.   If false, attributes will be cleared
     * if only one attribute cleared.  This is a more secure way to set attributes, when
     * all attribute changes are not safely managed.
     */
    bool     m_TryKeepPreviousAttributes;
};


// Flashed pads use the full attribute set: this is a helper for flashed pads
#define GBR_NETINFO_ALL                                                               \
    ( GBR_NETLIST_METADATA::GBR_NETINFO_PAD | GBR_NETLIST_METADATA::GBR_NETINFO_NET   \
      | GBR_NETLIST_METADATA::GBR_NETINFO_CMP )

#endif      // GBR_NETLIST_METADATA_H
