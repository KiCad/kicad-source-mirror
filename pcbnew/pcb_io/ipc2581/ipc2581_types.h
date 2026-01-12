/**
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright The KiCad Developers, see AUTHORS.txt for contributors.
*
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef IPC2581_TYPES_H
#define IPC2581_TYPES_H

#include <map>
#include <string>

enum class bomCategoryType
{
    ELECTRICAL,
    PROGRAMMABLE,
    MECHANICAL,
    MATERIAL,
    DOCUMENT
};

enum class boardTechnologyType
{
    RIGID,
    RIGID_FLEX,
    FLEX,
    HDI,
    EMBEDDED_COMPONENT,
    OTHER
};

enum class butterflyShapeType
{
    ROUND,
    SQUARE
};

enum class cadPinType
{
    THRU,
    BLIND,
    SURFACE
};

enum class auxLayerType
{
    COVERING,
    PLUGGING,
    TENTING,
    FILLING,
    CAPPING,
};

enum class certificationCategoryType
{
    ASSEMBLYDRAWING,
    ASSEMBLYFIXTUREGENERATION,
    ASSEMBLYPANEL,
    ASSEMBLYPREPTOOLS,
    ASSEMBLYTESTFIXTUREGENERATION,
    ASSEMBLYTESTGENERATION,
    BOARDFABRICATION,
    BOARDFIXTUREGENERATION,
    BOARDPANEL,
    BOARDTESTGENERATION,
    COMPONENTPLACEMENT,
    DETAILEDDRAWING,
    FABRICATIONDRAWING,
    GENERALASSEMBLY,
    GLUEDOT,
    MECHANICALHARDWARE,
    MULTIBOARDPARTSLIST,
    PHOTOTOOLS,
    SCHEMATICDRAWINGS,
    SINGLEBOARDPARTSLIST,
    SOLDERSTENCILPASTE,
    SPECSOURCECONTROLDRAWING,
    EMBEDDEDCOMPONENT,
    OTHER
};

enum class certificationStatusType
{
    ALPHA,
    BETA,
    CERTIFIED,
    SELFTEST
};

enum class complianceListType
{
    ROHS,
    CONFLICT_MINERALS,
    WEEE,
    REACH,
    HALOGEN_FREE,
    OTHER
};

enum class conductorListType
{
    CONDUCTIVITY,
    SURFACE_ROUGHNESS_UPFACING,
    SURFACE_ROUGHNESS_DOWNFACING,
    SURFACE_ROUGHNESS_TREATED,
    ETCH_FACTOR,
    FINISHED_HEIGHT,
    OTHER
};

enum class colorListType
{
    BLACK,
    WHITE,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    BROWN,
    ORANGE,
    PINK,
    PURPLE,
    GRAY,
    OTHER
};

enum class contextType
{
    BOARD,
    BOARDPANEL,
    ASSEMBLY,
    ASSEMBLYPALLET,
    DOCUMENTATION,
    TOOLING,
    COUPON,
    MISCELLANEOUS
};

enum class dfxCategoryType
{
    COMPONENT,
    BOARDFAB,
    ASSEMBLY,
    TESTING,
    DATAQUALITY
};

enum class dielectricListType
{
    DIELECTRIC_CONSTANT,
    LOSS_TANGENT,
    GLASS_TYPE,
    GLASS_STYLE,
    RESIN_CONTENT,
    PROCESSABILITY_TEMP,
    OTHER
};

enum class donutShapeType
{
    ROUND,
    SQUARE,
    HEXAGON,
    OCTAGON
};

enum class toolListType
{
    CARBIDE,
    ROUTER,
    LASER,
    FLATNOSE,
    EXTENSION,
    V_CUTTER
};

enum class toolPropertyListType
{
    DRILL_SIZE,
    FINISHED_SIZE,
    BIT_ANGLE,
    OTHER
};

enum class enterpriseCodeType
{
    DUNNS,
    CAGE
};

enum class exposureType
{
    EXPOSED,
    COVERED_PRIMARY,
    COVERED_SECONDARY,
    COVERED
};

enum class floorLifeType
{
    UNLIMITED,
    _1_YEAR,
    _4_WEEKS,
    _168_HOURS,
    _72_HOURS,
    _48_HOURS,
    _24_HOURS,
    BAKE
};

enum class geometryUsageType
{
    THIEVING,
    THERMAL_RELIEF,
    NONE
};

enum class generalListType
{
    ELECTRICAL,
    THERMAL,
    MATERIAL,
    INSTRUCTION,
    STANDARD,
    CONFIGURATION,
    OTHER
};

enum class impedanceListType
{
    IMPEDANCE,
    LINEWIDTH,
    SPACING,
    REF_PLANE_LAYER_ID,
    COPLANAR_GROUND_SPACING,
    OTHER
};

enum class isoCodeType
{
    AD,
    AE,
    AF,
    AG,
    AI,
    AL,
    AM,
    AN,
    AO,
    AQ,
    AR,
    AS,
    AT,
    AU,
    AW,
    AZ,
    BA,
    BB,
    BD,
    BE,
    BF,
    BG,
    BH,
    BI,
    BJ,
    BM,
    BN,
    BO,
    BR,
    BS,
    BT,
    BV,
    BW,
    BY,
    BZ,
    CA,
    CC,
    CF,
    CG,
    CH,
    CI,
    CK,
    CL,
    CM,
    CN,
    CO,
    CR,
    CU,
    CV,
    CX,
    CY,
    CZ,
    DE,
    DJ,
    DK,
    DM,
    DO,
    DZ,
    EC,
    EE,
    EG,
    EH,
    ER,
    ES,
    ET,
    FI,
    FJ,
    FK,
    FM,
    FO,
    FR,
    FX,
    GA,
    GB,
    GD,
    GE,
    GF,
    GH,
    GI,
    GL,
    GM,
    GN,
    GP,
    GQ,
    GR,
    GS,
    GT,
    GU,
    GW,
    GY,
    HK,
    HM,
    HN,
    HR,
    HT,
    HU,
    ID,
    IE,
    IL,
    IND, // iso code IN conflicts header, just use the A-3 type
    IO,
    IQ,
    IR,
    IS,
    IT,
    JM,
    JO,
    JP,
    KE,
    KG,
    KH,
    KI,
    KM,
    KN,
    KP,
    KR,
    KW,
    KY,
    KZ,
    LA,
    LB,
    LC,
    LI,
    LK,
    LR,
    LS,
    LT,
    LU,
    LV,
    LY,
    MA,
    MC,
    MD,
    MG,
    MH,
    MK,
    ML,
    MM,
    MN,
    MO,
    MP,
    MQ,
    MR,
    MS,
    MT,
    MU,
    MV,
    MW,
    MX,
    MY,
    MZ,
    NA,
    NC,
    NE,
    NF,
    NG,
    NI,
    NL,
    NO,
    NP,
    NR,
    NU,
    NZ,
    OM,
    PA,
    PE,
    PF,
    PG,
    PH,
    PK,
    PL,
    PM,
    PN,
    PR,
    PT,
    PW,
    PY,
    QA,
    RE,
    RO,
    RU,
    RW,
    SA,
    SB,
    SC,
    SD,
    SE,
    SG,
    SH,
    SI,
    SJ,
    SK,
    SL,
    SM,
    SN,
    SO,
    SR,
    ST,
    SV,
    SY,
    SZ,
    TC,
    TD,
    TF,
    TG,
    TH,
    TJ,
    TK,
    TM,
    TN,
    TO,
    TP,
    TR,
    TT,
    TV,
    TW,
    TZ,
    UA,
    UG,
    UM,
    US,
    UY,
    UZ,
    VA,
    VC,
    VE,
    VG,
    VI,
    VN,
    VU,
    WF,
    WS,
    YE,
    YT,
    YU,
    ZA,
    ZM,
    ZR,
    ZW
};

enum class lineEndType
{
    NONE,
    ROUND,
    SQUARE
};

enum class fillPropertyType
{
    HOLLOW,
    HATCH,
    MESH,
    FILL,
    VOIDFILL
};

enum class linePropertyType
{
    SOLID,
    DOTTED,
    DASHED,
    CENTER,
    PHANTOM,
    ERASE
};

enum class markingUsageType
{
    REFDES,
    PARTNAME,
    TARGET,
    POLARITY_MARKING,
    ATTRIBUTE_GRAPHICS,
    PIN_ONE,
    NONE
};

enum class mountType
{
    SMT,
    THMT,
    OTHER
};

enum class netClassType
{
    CLK,
    FIXED,
    GROUND,
    SIGNAL,
    POWER,
    UNUSED
};

enum class netPointType
{
    END,
    MIDDLE
};

enum class packageTypeType
{
    AXIAL_LEADED,
    BARE_DIE,
    CERAMIC_BGA,
    CERAMIC_DIP,
    CERAMIC_FLATPACK,
    CERAMIC_QUAD_FLATPACK,
    CERAMIC_SIP,
    CHIP,
    CHIP_SCALE,
    CHOKE_SWITCH_SM,
    COIL,
    CONNECTOR_SM,
    CONNECTOR_TH,
    EMBEDDED,
    FLIPCHIP,
    HERMETIC_HYBRED,
    LEADLESS_CERAMIC_CHIP_CARRIER,
    MCM,
    MELF,
    FINEPITCH_BGA,
    MOLDED,
    NETWORK,
    PGA,
    PLASTIC_BGA,
    PLASTIC_CHIP_CARRIER,
    PLASTIC_DIP,
    PLASTIC_SIP,
    POWER_TRANSISTOR,
    RADIAL_LEADED,
    RECTANGULAR_QUAD_FLATPACK,
    RELAY_SM,
    RELAY_TH,
    SOD123,
    SOIC,
    SOJ,
    SOPIC,
    SOT143,
    SOT23,
    SOT52,
    SOT89,
    SQUARE_QUAD_FLATPACK,
    SSOIC,
    SWITCH_TH,
    TANTALUM,
    TO_TYPE,
    TRANSFORMER,
    TRIMPOT_SM,
    TRIMPOT_TH,
    OTHER
};

enum class padUsageType
{
    TERMINATION,
    VIA,
    PLANE,
    MASK,
    TOOLING_HOLE,
    THIEVING,
    THERMAL_RELIEF,
    FIDUCIAL,
    NONE
};

enum class padUseType
{
    REGULAR,
    ANTIPAD,
    THERMAL,
    OTHER
};

enum class pinElectricalType
{
    ELECTRICAL,
    MECHANICAL,
    UNDEFINED
};

enum class pinMountType
{
    SURFACE_MOUNT_PIN,
    SURFACE_MOUNT_PAD,
    THROUGH_HOLE_PIN,
    THROUGH_HOLE_HOLE,
    PRESSFIT,
    NONBOARD,
    HOLE,
    UNDEFINED
};

enum class pinOneOrientationType
{
    LOWER_LEFT,
    LEFT,
    LEFT_CENTER,
    UPPER_LEFT,
    UPPER_CENTER,
    UPPER_RIGHT,
    RIGHT,
    RIGHT_CENTER,
    LOWER_RIGHT,
    LOWER_CENTER,
    CENTER,
    OTHER
};

enum class polarityType
{
    POSITIVE,
    NEGATIVE
};

enum class propertyUnitType
{
    MM,
    INCH,
    MICRON,
    OHMS,
    MHO_CM,
    SIEMENS_M,
    CELCIUS,
    FARANHEIT,
    PERCENT,
    Hz,
    DEGREES,
    RMAX,
    RZ,
    RMS,
    SECTION,
    CLASS,
    ITEM,
    GAUGE,
    OTHER
};

enum class roleFunctionType
{
    SENDER,
    OWNER,
    RECEIVER,
    DESIGNER,
    ENGINEER,
    BUYER,
    CUSTOMERSERVICE,
    DELIVERTO,
    BILLTO,
    OTHER
};

enum class platingStatusType
{
    PLATED,
    NONPLATED,
    VIA
};

enum class standardPrimitive
{
    BUTTERFLY,
    CIRCLE,
    CONTOUR,
    DIAMOND,
    DONUT,
    ELLIPSE,
    HEXAGON,
    MOIRE,
    OCTAGON,
    OVAL,
    RECTCENTER,
    RECTCHAM,
    RECTCORNER,
    RECTROUND,
    THERMAL,
    TRIANGLE
};

enum class structureListType
{
    STRIPLINE,
    PLANE_LESS_STRIPLINE,
    MICROSTRIP_EMBEDDED,
    MICROSTRIP_NO_MASK,
    MICROSTRIP_MASK_COVERED,
    MICROSTRIP_DUAL_MASKED_COVERED,
    COPLANAR_WAVEGUIDE_STRIPLINE,
    COPLANAR_WAVEGUIDE_EMBEDDED,
    COPLANAR_WAVEGUIDE_NO_MASK,
    COPLANAR_WAVEGUIDE_MASK_COVERED,
    COPLANAR_WAVEGUIDE_DUAL_MASKED_COVERED,
    OTHER
};

enum class technologyListType
{
    RIGID,
    RIGID_FLEX,
    FLEX,
    HDI,
    EMBEDDED_COMPONENT,
    OTHER
};

enum class temperatureListType
{
    THERMAL_DELAMINATION,
    EXPANSION_Z_AXIS,
    EXPANSION_X_Y_AXIS,
    OTHER
};

enum class thermalShapeType
{
    ROUND,
    SQUARE,
    HEXAGON,
    OCTAGON
};

enum class thievingListType
{
    KEEP_IN,
    KEEP_OUT
};

enum class transmissionListType
{
    SINGLE_ENDED,
    EDGE_COUPLED,
    BROADSIDE_COUPLED,
    OTHER
};

enum class unitModeType
{
    DISTANCE,
    AREA,
    RESISTANCE,
    CAPACITANCE,
    IMPEDANCE,
    PERCENTAGE,
    SIZE,
    NONE
};

enum class unitsType
{
    MILLIMETER,
    MICRON,
    INCH
};

enum class vCutListType
{
    ANGLE,
    THICKNESS_REMAINING,
    OFFSET,
    OTHER
};

enum class edgeChamferListType
{
    ANGLE,
    WIDTH,
    SIDE
};

enum class whereMeasuredType
{
    LAMINATE,
    METAL,
    MASK,
    OTHER
};

/**
 * IPC-6012 surface finish types from Table 3-3 "Final Finish and Coating Requirements".
 * Used in IPC-2581C Section 8.1.1.16 SurfaceFinish specification.
 *
 * ENIG/ENEPIG suffixes: -N = Normal (soldering), -G = Gold wire bonding (thicker gold)
 */
enum class surfaceFinishType
{
    NONE,     ///< No surface finish / not specified - skip coating layer generation
    ENIG_N,   ///< ENIG for soldering (normal gold thickness)
    ENEPIG_N, ///< ENEPIG for soldering (normal gold thickness)
    OSP,      ///< Organic Solderability Preservative
    HT_OSP,   ///< High Temperature OSP
    IAG,      ///< Immersion Silver
    ISN,      ///< Immersion Tin
    G,        ///< Gold (hard gold)
    N,        ///< Nickel
    DIG,      ///< Direct Immersion Gold
    S,        ///< Solder (HASL/SMOBC)
    OTHER     ///< Non-standard finish
};

#endif // IPC2581_TYPES_H