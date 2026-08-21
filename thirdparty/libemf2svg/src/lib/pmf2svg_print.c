/**
  @file upmf_print.c

  @brief Functions for printing EMF records
  */

/*
File:      upmf_print.c
Version:   0.0.3
Date:      24-MAR-2014
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2014 David Mathog and California Institute of Technology (Caltech)
*/

/* compiler options:

   -DNOBRUSH causes brush objects to be treated as pen objects.  PowerPoint 2003
   and 2010 define pen objects
   as brush objects, and this is one way to see their structure even though they
   are misidentified.
   This option should only be used for tiny test files, consisting of just line
   objects.
   */

#ifdef __cplusplus
extern "C" {
#endif

#include "emf2svg_private.h"
#include "emf2svg_print.h"
#include "pmf2svg.h"
#include "pmf2svg_print.h"
#include <stdio.h>
#include <stdlib.h>

//! \cond

#define UNUSED(x)                                                              \
    (void)(x) //! Please ignore - Doxygen simply insisted on including this

/*
   this function is not visible in the API.  Print "data" for one of the many
   records that has none.
   */
int U_PMR_NODATAREC_print(const char *contents, FILE *out,
                          drawingStates *states) {
    U_PMF_CMN_HDR Header;
    int status =
        U_PMR_RESETCLIP_get(contents, &Header); /* One of many possibilities */
    if (status)
        status = Header.Size;
    return (status);
}

/*
   this function is not visible in the API.  Common routine used by many
   functions that draw points.
   */
void U_PMF_VARPOINTS_print(const char **contents, int Flags, uint32_t Elements,
                           const char *blimit, FILE *out,
                           drawingStates *states) {
    unsigned int i;
    U_FLOAT Xpos, Ypos;

    if (Flags & U_PPF_P) {
        verbose_printf("   +  Points(Relative):");
    } else if (Flags & U_PPF_C) {
        verbose_printf("   +  Points(Int16):");
    } else {
        verbose_printf("   +  Points(Float):");
    }
    for (Xpos = Ypos = i = 0; i < Elements; i++) {
        verbose_printf(" %d:", i);
        if (Flags & U_PPF_P) {
            (void)U_PMF_POINTR_print(contents, &Xpos, &Ypos, blimit, out,
                                     states);
        } else if (Flags & U_PPF_C) {
            (void)U_PMF_POINT_print(contents, blimit, out, states);
        } else {
            (void)U_PMF_POINTF_print(contents, blimit, out, states);
        }
    }
#if 0
        int residual;
        uintptr_t holdptr = (uintptr_t) *contents;
        residual = holdptr & 0x3;
        if(residual){ *contents += (4-residual); }
        verbose_printf("DEBUG U_PMF_VARPOINTS_print residual:%d *contents:%p\n",residual,*contents);fflush(stdout);
#endif
    verbose_printf("\n");
}

/*
   this function is not visible in the API.  Common routine used by many
   functions that draw points.
   */
void U_PMF_VARPOINTF_S_print(U_PMF_POINTF *Points, uint32_t Elements, FILE *out,
                             drawingStates *states) {
    unsigned int i;
    verbose_printf("   +  Points:");
    for (i = 0; i < Elements; i++, Points++) {
        verbose_printf(" %d:", i);
        (void)U_PMF_POINTF_S_print(Points, out, states);
    }
    verbose_printf("\n");
}

/*
   this function is not visible in the API.  Common routine used by many
   functions that draw rectangles.
   */
int U_PMF_VARRECTF_S_print(U_PMF_RECTF *Rects, uint32_t Elements, FILE *out,
                           drawingStates *states) {
    if (!Elements)
        return (0);
    if (Elements == 1) {
        verbose_printf(" Rect(Float):");
    } else {
        verbose_printf(" Rects(Float):");
    }
    while (1) {
        U_PMF_RECTF_S_print(Rects++, out, states);
        Elements--;
        if (!Elements)
            break;
        verbose_printf(" ");
    }
    return (1);
}

/*
   this function is not visible in the API.  Common routine used by many
   functions.
   */
int U_PMF_VARBRUSHID_print(int btype, uint32_t BrushID, FILE *out,
                           drawingStates *states) {
    if (btype) {
        verbose_printf(" Color:");
        (void)U_PMF_ARGB_print((char *)&(BrushID), out, states);
    } else {
        verbose_printf(" BrushID:%u", BrushID);
    }
    return (1);
}
//! \endcond

/**
  \brief Print any EMF+ record
  \returns record length for a normal record, 0 for EMREOF or , -1 for a bad
  record
  \param contents   pointer to a buffer holding this EMF+ record
  \param blimit     one byte past the end of data of this EMF+ record
  \param recnum     EMF number of this record in contents
  \param off        Offset from the beginning of the EMF+ file.
  */
int U_pmf_onerec_print(const char *contents, const char *blimit, int recnum,
                       int off, FILE *out, drawingStates *states) {
    int status;
    int rstatus;
    static U_OBJ_ACCUM ObjCont = {
        NULL, 0, 0, 0,
        0}; /* for keeping track of object continuation. These may
               be split across multiple EMF Comment records */
    U_PMF_CMN_HDR Header;
    const char *contemp = contents;

    /* Check that COMMON header data in record can be touched without an access
       violation.  If it cannot be
        this is either a corrupt EMF or one engineered to cause a buffer
       overflow.  Pointer math
        could wrap so check both sides of the range.
    */
    if (IS_MEM_UNSAFE(contents, sizeof(U_PMF_CMN_HDR), blimit))
        return (-1);
    if (!U_PMF_CMN_HDR_get(&contemp, &Header)) {
        return (-1);
    }

    int type = Header.Type & U_PMR_TYPE_MASK; /* strip the U_PMR_RECFLAG bit,
                                                 leaving the indexable part */
    if (type < U_PMR_MIN || type > U_PMR_MAX)
        return (-1); /* unknown EMF+ record type */
    status = U_PMF_CMN_HDR_print(contents, Header, recnum, off, out,
                                 states); /* EMF+ part */

    /* Buggy EMF+ can set the continue bit and then do something else. In that
       case, force out the pending
       Object.  Side effect - clears the pending object. */
    if ((type != U_PMR_OBJECT) && (ObjCont.used > 0)) {
        U_PMR_OBJECT_print(contents, blimit, &ObjCont, 1, out, states);
    }

    switch (type) {
    case (U_PMR_HEADER):
        rstatus = U_PMR_HEADER_print(contents, out, states);
        break;
    case (U_PMR_ENDOFFILE):
        rstatus = U_PMR_ENDOFFILE_print(contents, out, states);
        U_OA_release(&ObjCont);
        break;
    case (U_PMR_COMMENT):
        rstatus = U_PMR_COMMENT_print(contents, out, states);
        break;
    case (U_PMR_GETDC):
        rstatus = U_PMR_GETDC_print(contents, out, states);
        break;
    case (U_PMR_MULTIFORMATSTART):
        rstatus = U_PMR_MULTIFORMATSTART_print(contents, out, states);
        break;
    case (U_PMR_MULTIFORMATSECTION):
        rstatus = U_PMR_MULTIFORMATSECTION_print(contents, out, states);
        break;
    case (U_PMR_MULTIFORMATEND):
        rstatus = U_PMR_MULTIFORMATEND_print(contents, out, states);
        break;
    case (U_PMR_OBJECT):
        rstatus =
            U_PMR_OBJECT_print(contents, blimit, &ObjCont, 0, out, states);
        break;
    case (U_PMR_CLEAR):
        rstatus = U_PMR_CLEAR_print(contents, out, states);
        break;
    case (U_PMR_FILLRECTS):
        rstatus = U_PMR_FILLRECTS_print(contents, out, states);
        break;
    case (U_PMR_DRAWRECTS):
        rstatus = U_PMR_DRAWRECTS_print(contents, out, states);
        break;
    case (U_PMR_FILLPOLYGON):
        rstatus = U_PMR_FILLPOLYGON_print(contents, out, states);
        break;
    case (U_PMR_DRAWLINES):
        rstatus = U_PMR_DRAWLINES_print(contents, out, states);
        break;
    case (U_PMR_FILLELLIPSE):
        rstatus = U_PMR_FILLELLIPSE_print(contents, out, states);
        break;
    case (U_PMR_DRAWELLIPSE):
        rstatus = U_PMR_DRAWELLIPSE_print(contents, out, states);
        break;
    case (U_PMR_FILLPIE):
        rstatus = U_PMR_FILLPIE_print(contents, out, states);
        break;
    case (U_PMR_DRAWPIE):
        rstatus = U_PMR_DRAWPIE_print(contents, out, states);
        break;
    case (U_PMR_DRAWARC):
        rstatus = U_PMR_DRAWARC_print(contents, out, states);
        break;
    case (U_PMR_FILLREGION):
        rstatus = U_PMR_FILLREGION_print(contents, out, states);
        break;
    case (U_PMR_FILLPATH):
        rstatus = U_PMR_FILLPATH_print(contents, out, states);
        break;
    case (U_PMR_DRAWPATH):
        rstatus = U_PMR_DRAWPATH_print(contents, out, states);
        break;
    case (U_PMR_FILLCLOSEDCURVE):
        rstatus = U_PMR_FILLCLOSEDCURVE_print(contents, out, states);
        break;
    case (U_PMR_DRAWCLOSEDCURVE):
        rstatus = U_PMR_DRAWCLOSEDCURVE_print(contents, out, states);
        break;
    case (U_PMR_DRAWCURVE):
        rstatus = U_PMR_DRAWCURVE_print(contents, out, states);
        break;
    case (U_PMR_DRAWBEZIERS):
        rstatus = U_PMR_DRAWBEZIERS_print(contents, out, states);
        break;
    case (U_PMR_DRAWIMAGE):
        rstatus = U_PMR_DRAWIMAGE_print(contents, out, states);
        break;
    case (U_PMR_DRAWIMAGEPOINTS):
        rstatus = U_PMR_DRAWIMAGEPOINTS_print(contents, out, states);
        break;
    case (U_PMR_DRAWSTRING):
        rstatus = U_PMR_DRAWSTRING_print(contents, out, states);
        break;
    case (U_PMR_SETRENDERINGORIGIN):
        rstatus = U_PMR_SETRENDERINGORIGIN_print(contents, out, states);
        break;
    case (U_PMR_SETANTIALIASMODE):
        rstatus = U_PMR_SETANTIALIASMODE_print(contents, out, states);
        break;
    case (U_PMR_SETTEXTRENDERINGHINT):
        rstatus = U_PMR_SETTEXTRENDERINGHINT_print(contents, out, states);
        break;
    case (U_PMR_SETTEXTCONTRAST):
        rstatus = U_PMR_SETTEXTCONTRAST_print(contents, out, states);
        break;
    case (U_PMR_SETINTERPOLATIONMODE):
        rstatus = U_PMR_SETINTERPOLATIONMODE_print(contents, out, states);
        break;
    case (U_PMR_SETPIXELOFFSETMODE):
        rstatus = U_PMR_SETPIXELOFFSETMODE_print(contents, out, states);
        break;
    case (U_PMR_SETCOMPOSITINGMODE):
        rstatus = U_PMR_SETCOMPOSITINGMODE_print(contents, out, states);
        break;
    case (U_PMR_SETCOMPOSITINGQUALITY):
        rstatus = U_PMR_SETCOMPOSITINGQUALITY_print(contents, out, states);
        break;
    case (U_PMR_SAVE):
        rstatus = U_PMR_SAVE_print(contents, out, states);
        break;
    case (U_PMR_RESTORE):
        rstatus = U_PMR_RESTORE_print(contents, out, states);
        break;
    case (U_PMR_BEGINCONTAINER):
        rstatus = U_PMR_BEGINCONTAINER_print(contents, out, states);
        break;
    case (U_PMR_BEGINCONTAINERNOPARAMS):
        rstatus = U_PMR_BEGINCONTAINERNOPARAMS_print(contents, out, states);
        break;
    case (U_PMR_ENDCONTAINER):
        rstatus = U_PMR_ENDCONTAINER_print(contents, out, states);
        break;
    case (U_PMR_SETWORLDTRANSFORM):
        rstatus = U_PMR_SETWORLDTRANSFORM_print(contents, out, states);
        break;
    case (U_PMR_RESETWORLDTRANSFORM):
        rstatus = U_PMR_RESETWORLDTRANSFORM_print(contents, out, states);
        break;
    case (U_PMR_MULTIPLYWORLDTRANSFORM):
        rstatus = U_PMR_MULTIPLYWORLDTRANSFORM_print(contents, out, states);
        break;
    case (U_PMR_TRANSLATEWORLDTRANSFORM):
        rstatus = U_PMR_TRANSLATEWORLDTRANSFORM_print(contents, out, states);
        break;
    case (U_PMR_SCALEWORLDTRANSFORM):
        rstatus = U_PMR_SCALEWORLDTRANSFORM_print(contents, out, states);
        break;
    case (U_PMR_ROTATEWORLDTRANSFORM):
        rstatus = U_PMR_ROTATEWORLDTRANSFORM_print(contents, out, states);
        break;
    case (U_PMR_SETPAGETRANSFORM):
        rstatus = U_PMR_SETPAGETRANSFORM_print(contents, out, states);
        break;
    case (U_PMR_RESETCLIP):
        rstatus = U_PMR_RESETCLIP_print(contents, out, states);
        break;
    case (U_PMR_SETCLIPRECT):
        rstatus = U_PMR_SETCLIPRECT_print(contents, out, states);
        break;
    case (U_PMR_SETCLIPPATH):
        rstatus = U_PMR_SETCLIPPATH_print(contents, out, states);
        break;
    case (U_PMR_SETCLIPREGION):
        rstatus = U_PMR_SETCLIPREGION_print(contents, out, states);
        break;
    case (U_PMR_OFFSETCLIP):
        rstatus = U_PMR_OFFSETCLIP_print(contents, out, states);
        break;
    case (U_PMR_DRAWDRIVERSTRING):
        rstatus = U_PMR_DRAWDRIVERSTRING_print(contents, out, states);
        break;
    case (U_PMR_STROKEFILLPATH):
        rstatus = U_PMR_STROKEFILLPATH_print(contents, out, states);
        break;
    case (U_PMR_SERIALIZABLEOBJECT):
        rstatus = U_PMR_SERIALIZABLEOBJECT_print(contents, out, states);
        break;
    case (U_PMR_SETTSGRAPHICS):
        rstatus = U_PMR_SETTSGRAPHICS_print(contents, out, states);
        break;
    case (U_PMR_SETTSCLIP):
        rstatus = U_PMR_SETTSCLIP_print(contents, out, states);
        break;
    }
    if (!rstatus)
        status = -1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CMN_HDR object
  \return number of bytes in record, 0 on error
  \param  Header     Header of the record
  \param  precnum    EMF+ record number in file.
  \param  off        Offset in file to the start of this EMF+ record.
  common structure present at the beginning of all(*) EMF+ records
  */

int U_PMF_CMN_HDR_print(const char *contents, U_PMF_CMN_HDR Header, int precnum,
                        int off, FILE *out, drawingStates *states) {
    verbose_printf(
        "   %-29srec+:%5d type:%X offset:%8d rsize:%8u dsize:%8u flags:%4.4X\n",
        U_pmr_names(Header.Type & U_PMR_TYPE_MASK), precnum, Header.Type, off,
        Header.Size, Header.DataSize, Header.Flags);
    return ((int)Header.Size);
}

/**
  \brief Print data from a an array of uint8_t values
  \return 1
  \param  Start      Text to lead array data
  \param  Array      uint8_t array of data passed as char *
  \param  Elements   Number of elements in Array
  \param  End        Text to follow array data
  */
int U_PMF_UINT8_ARRAY_print(const char *Start, const uint8_t *Array,
                            int Elements, char *End, FILE *out,
                            drawingStates *states) {
    if (Start)
        verbose_printf("%s", Start);
    for (; Elements--; Array++) {
        verbose_printf(" %u", *Array);
    }
    if (End)
        verbose_printf("%s", End);
    return (1);
}

/**
  \brief Print value of an BrushType Enumeration
  \returns record 1 on sucess, 0 on error
  \param otype    Value to print.
  EMF+ manual 2.1.1.3, Microsoft name: BrushType Enumeration
  */
int U_PMF_BRUSHTYPEENUMERATION_print(int otype, FILE *out,
                                     drawingStates *states) {
    int status = 1;
    switch (otype) {
    case U_BT_SolidColor:
        verbose_printf("SolidColor");
        break;
    case U_BT_HatchFill:
        verbose_printf("HatchFill");
        break;
    case U_BT_TextureFill:
        verbose_printf("TextureFill");
        break;
    case U_BT_PathGradient:
        verbose_printf("PathGradient");
        break;
    case U_BT_LinearGradient:
        verbose_printf("LinearGradient");
        break;
    default:
        status = 0;
        verbose_printf("INVALID(%d)", otype);
        break;
    }
    return (status);
}

/**
  \brief Print value of an BrushType Enumeration
  \returns record 1 on sucess, 0 on error
  \param otype    Value to print.
  EMF+ manual 2.1.1.4, Microsoft name: BrushType Enumeration
  */
int U_PMF_COMBINEMODEENUMERATION_print(int otype, FILE *out,
                                       drawingStates *states) {
    int status = 1;
    switch (otype) {
    case U_CM_Replace:
        verbose_printf("Replace");
        break;
    case U_CM_Intersect:
        verbose_printf("Intersect");
        break;
    case U_CM_Union:
        verbose_printf("Union");
        break;
    case U_CM_XOR:
        verbose_printf("XOR");
        break;
    case U_CM_Exclude:
        verbose_printf("Exclude");
        break;
    case U_CM_Complement:
        verbose_printf("Complement");
        break;
    default:
        status = 0;
        verbose_printf("INVALID(%d)", otype);
        break;
    }
    return (status);
}

/**
  \brief Print value of a HatchStyle Enumeration
  \returns record 1 on sucess, 0 on error
  \param hstype    Value to print.
  EMF+ manual 2.1.1.13, Microsoft name: HatchStyle Enumeration
  */
int U_PMF_HATCHSTYLEENUMERATION_print(int hstype, FILE *out,
                                      drawingStates *states) {
    int status = 1;
    switch (hstype) {
    case U_HSP_Horizontal:
        verbose_printf("Horizontal");
        break;
    case U_HSP_Vertical:
        verbose_printf("Vertical");
        break;
    case U_HSP_ForwardDiagonal:
        verbose_printf("ForwardDiagonal");
        break;
    case U_HSP_BackwardDiagonal:
        verbose_printf("BackwardDiagonal");
        break;
    case U_HSP_LargeGrid:
        verbose_printf("LargeGrid");
        break;
    case U_HSP_DiagonalCross:
        verbose_printf("DiagonalCross");
        break;
    case U_HSP_05Percent:
        verbose_printf("05Percent");
        break;
    case U_HSP_10Percent:
        verbose_printf("10Percent");
        break;
    case U_HSP_20Percent:
        verbose_printf("20Percent");
        break;
    case U_HSP_25Percent:
        verbose_printf("25Percent");
        break;
    case U_HSP_30Percent:
        verbose_printf("30Percent");
        break;
    case U_HSP_40Percent:
        verbose_printf("40Percent");
        break;
    case U_HSP_50Percent:
        verbose_printf("50Percent");
        break;
    case U_HSP_60Percent:
        verbose_printf("60Percent");
        break;
    case U_HSP_70Percent:
        verbose_printf("70Percent");
        break;
    case U_HSP_75Percent:
        verbose_printf("75Percent");
        break;
    case U_HSP_80Percent:
        verbose_printf("80Percent");
        break;
    case U_HSP_90Percent:
        verbose_printf("90Percent");
        break;
    case U_HSP_LightDownwardDiagonal:
        verbose_printf("LightDownwardDiagonal");
        break;
    case U_HSP_LightUpwardDiagonal:
        verbose_printf("LightUpwardDiagonal");
        break;
    case U_HSP_DarkDownwardDiagonal:
        verbose_printf("DarkDownwardDiagonal");
        break;
    case U_HSP_DarkUpwardDiagonal:
        verbose_printf("DarkUpwardDiagonal");
        break;
    case U_HSP_WideDownwardDiagonal:
        verbose_printf("WideDownwardDiagonal");
        break;
    case U_HSP_WideUpwardDiagonal:
        verbose_printf("WideUpwardDiagonal");
        break;
    case U_HSP_LightVertical:
        verbose_printf("LightVertical");
        break;
    case U_HSP_LightHorizontal:
        verbose_printf("LightHorizontal");
        break;
    case U_HSP_NarrowVertical:
        verbose_printf("NarrowVertical");
        break;
    case U_HSP_NarrowHorizontal:
        verbose_printf("NarrowHorizontal");
        break;
    case U_HSP_DarkVertical:
        verbose_printf("DarkVertical");
        break;
    case U_HSP_DarkHorizontal:
        verbose_printf("DarkHorizontal");
        break;
    case U_HSP_DashedDownwardDiagonal:
        verbose_printf("DashedDownwardDiagonal");
        break;
    case U_HSP_DashedUpwardDiagonal:
        verbose_printf("DashedUpwardDiagonal");
        break;
    case U_HSP_DashedHorizontal:
        verbose_printf("DashedHorizontal");
        break;
    case U_HSP_DashedVertical:
        verbose_printf("DashedVertical");
        break;
    case U_HSP_SmallConfetti:
        verbose_printf("SmallConfetti");
        break;
    case U_HSP_LargeConfetti:
        verbose_printf("LargeConfetti");
        break;
    case U_HSP_ZigZag:
        verbose_printf("ZigZag");
        break;
    case U_HSP_Wave:
        verbose_printf("Wave");
        break;
    case U_HSP_DiagonalBrick:
        verbose_printf("DiagonalBrick");
        break;
    case U_HSP_HorizontalBrick:
        verbose_printf("HorizontalBrick");
        break;
    case U_HSP_Weave:
        verbose_printf("Weave");
        break;
    case U_HSP_Plaid:
        verbose_printf("Plaid");
        break;
    case U_HSP_Divot:
        verbose_printf("Divot");
        break;
    case U_HSP_DottedGrid:
        verbose_printf("DottedGrid");
        break;
    case U_HSP_DottedDiamond:
        verbose_printf("DottedDiamond");
        break;
    case U_HSP_Shingle:
        verbose_printf("Shingle");
        break;
    case U_HSP_Trellis:
        verbose_printf("Trellis");
        break;
    case U_HSP_Sphere:
        verbose_printf("Sphere");
        break;
    case U_HSP_SmallGrid:
        verbose_printf("SmallGrid");
        break;
    case U_HSP_SmallCheckerBoard:
        verbose_printf("SmallCheckerBoard");
        break;
    case U_HSP_LargeCheckerBoard:
        verbose_printf("LargeCheckerBoard");
        break;
    case U_HSP_OutlinedDiamond:
        verbose_printf("OutlinedDiamond");
        break;
    case U_HSP_SolidDiamond:
        verbose_printf("SolidDiamond");
        break;
    default:
        status = 0;
        verbose_printf("INVALID(%d)", hstype);
        break;
    }
    return (status);
}

/**
  \brief Print value of an ObjectType Enumeration
  \returns record 1 on sucess, 0 on error
  \param otype    Value to print.
  EMF+ manual 2.1.1.22, Microsoft name: ObjectType Enumeration
  */
int U_PMF_OBJECTTYPEENUMERATION_print(int otype, FILE *out,
                                      drawingStates *states) {
    int status = 1;
    switch (otype) {
    case U_OT_Invalid:
        verbose_printf("Invalid");
        break;
    case U_OT_Brush:
        verbose_printf("Brush");
        break;
    case U_OT_Pen:
        verbose_printf("Pen");
        break;
    case U_OT_Path:
        verbose_printf("Path");
        break;
    case U_OT_Region:
        verbose_printf("Region");
        break;
    case U_OT_Image:
        verbose_printf("Image");
        break;
    case U_OT_Font:
        verbose_printf("Font");
        break;
    case U_OT_StringFormat:
        verbose_printf("StringFormat");
        break;
    case U_OT_ImageAttributes:
        verbose_printf("ImageAttributes");
        break;
    case U_OT_CustomLineCap:
        verbose_printf("CustomLineCap");
        break;
    default:
        status = 0;
        verbose_printf("INVALID(%d)", otype);
        break;
    }
    return (status);
}

/**
  \brief Print value of a  U_PMF_PATHPOINTTYPE_ENUM object
  \return 1
  \param  Type   Value to print
  EMF+ manual 2.1.1.23, Microsoft name: PathPointType Enumeration
  */
int U_PMF_PATHPOINTTYPE_ENUM_print(int Type, FILE *out, drawingStates *states) {
    switch (Type & U_PPT_MASK) {
    case U_PPT_Start:
        verbose_printf("Start");
        break;
    case U_PPT_Line:
        verbose_printf("Line");
        break;
    case U_PPT_Bezier:
        verbose_printf("Bezier");
        break;
    default:
        verbose_printf("INVALID(%d)", Type);
        break;
    }
    return (1);
}

/**
  \brief Print data from a PixelFormat Enumeration value
  \return 1 always
  \param  pfe   A PixelFormat Enumeration value
  EMF+ manual 2.1.1.25, Microsoft name: PixelFormat Enumeration (U_PF_*)
  */
int U_PMF_PX_FMT_ENUM_print(int pfe, FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    uint8_t idx;
    verbose_printf("   +  PxFmtEnum: ");
    verbose_printf(" 32Bit:%c", (pfe & 1 << 9 ? 'Y' : 'N'));
    verbose_printf(" 16Bit:%c", (pfe & 1 << 10 ? 'Y' : 'N'));
    verbose_printf(" PreAlpha:%c", (pfe & 1 << 11 ? 'Y' : 'N'));
    verbose_printf(" Alpha:%c", (pfe & 1 << 12 ? 'Y' : 'N'));
    verbose_printf(" GDI:%c", (pfe & 1 << 13 ? 'Y' : 'N'));
    verbose_printf(" LUT:%c", (pfe & 1 << 14 ? 'Y' : 'N'));
    verbose_printf(" BitsPerPx:%u", (pfe >> 16) & 0xFF);
    idx = pfe >> 24;
    verbose_printf(" Type:%u(", idx);
    switch (idx) {
    case 0:
        verbose_printf("undefined");
        break;
    case 1:
        verbose_printf("monochrome with LUT");
        break;
    case 2:
        verbose_printf("4 bit with LUT");
        break;
    case 3:
        verbose_printf("8 bit with LUT");
        break;
    case 4:
        verbose_printf("16 bits grey values");
        break;
    case 5:
        verbose_printf("16 bit RGB values (5,5,5,(1 ignored))");
        break;
    case 6:
        verbose_printf("16 bit RGB values (5,6,5)");
        break;
    case 7:
        verbose_printf("16 bit ARGB values (1 alpha, 5,5,5 colors)");
        break;
    case 8:
        verbose_printf("24 bit RGB values (8,8.8)");
        break;
    case 9:
        verbose_printf("32 bit RGB value  (8,8,8,(8 ignored))");
        break;
    case 10:
        verbose_printf("32 bit ARGB values (8 alpha,8,8,8)");
        break;
    case 11:
        verbose_printf(
            "32 bit PARGB values (8,8,8,8, but RGB already multiplied by A)");
        break;
    case 12:
        verbose_printf("48 bit RGB (16,16,16)");
        break;
    case 13:
        verbose_printf("64 bit ARGB (16 alpha, 16,16,16)");
        break;
    case 14:
        verbose_printf(
            "64 bit PARGB (16,16,16,16, but RGB already multiplied by A)");
        break;
    default:
        verbose_printf("INVALID(%d)", idx);
        break;
    }
    verbose_printf(")");
    return (1);
}

/**
  \brief Print as text a RegionNodeDataType Enumeration
  \return 1
  \param  Type   RegionNodeDataType Enumeration
  EMF+ manual 2.1.1.27, Microsoft name: RegionNodeDataType Enumeration
  (U_RNDT_*)
  */
int U_PMF_NODETYPE_print(int Type, FILE *out, drawingStates *states) {
    if (Type == U_RNDT_And) {
        verbose_printf("And");
    } else if (Type == U_RNDT_Or) {
        verbose_printf("Or");
    } else if (Type == U_RNDT_Xor) {
        verbose_printf("Xor");
    } else if (Type == U_RNDT_Exclude) {
        verbose_printf("Exclude");
    } else if (Type == U_RNDT_Complement) {
        verbose_printf("Complement");
    } else if (Type == U_RNDT_Rect) {
        verbose_printf("Rect");
    } else if (Type == U_RNDT_Path) {
        verbose_printf("Path");
    } else if (Type == U_RNDT_Empty) {
        verbose_printf("Empty");
    } else if (Type == U_RNDT_Infinite) {
        verbose_printf("Infinite");
    } else {
        verbose_printf("Undefined");
        return (0);
    }
    return (1);
}

/**
  \brief Print data from a  U_PMF_BRUSH object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.1, Microsoft name: EmfPlusBrush Object
  */
int U_PMF_BRUSH_print(const char *contents, const char *blimit, FILE *out,
                      drawingStates *states) {
    FLAG_IGNORED;
    uint32_t Version, Type;
    const char *Data;
    int status = U_PMF_BRUSH_get(contents, &Version, &Type, &Data, blimit);
    if (status) {
        verbose_printf("   +  Brush:");
        (void)U_PMF_GRAPHICSVERSION_memsafe_print((char *)&Version, out,
                                                  states);
        verbose_printf(" Type:%X(", Type);
        (void)U_PMF_BRUSHTYPEENUMERATION_print(Type, out, states);
        verbose_printf(")");
        switch (Type) {
        case U_BT_SolidColor:
            status = U_PMF_ARGB_print(Data, out, states);
            break;
        case U_BT_HatchFill:
            verbose_printf("\n");
            status = U_PMF_HATCHBRUSHDATA_print(Data, blimit, out, states);
            break;
        case U_BT_TextureFill:
            verbose_printf("\n");
            status = U_PMF_TEXTUREBRUSHDATA_print(Data, blimit, out, states);
            break;
        case U_BT_PathGradient:
            verbose_printf("\n");
            status =
                U_PMF_PATHGRADIENTBRUSHDATA_print(Data, blimit, out, states);
            break;
        case U_BT_LinearGradient:
            verbose_printf("\n");
            status =
                U_PMF_LINEARGRADIENTBRUSHDATA_print(Data, blimit, out, states);
            break;
        default:
            status = 0;
        }
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAP object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  Which      A string which is either "Start" or "End".
  EMF+ manual 2.2.1.2, Microsoft name: EmfPlusCustomLineCap Object
  */
int U_PMF_CUSTOMLINECAP_print(const char *contents, const char *Which,
                              const char *blimit, FILE *out,
                              drawingStates *states) {
    FLAG_IGNORED;
    uint32_t Version, Type;
    const char *Data;
    int status =
        U_PMF_CUSTOMLINECAP_get(contents, &Version, &Type, &Data, blimit);

    if (status) {
        verbose_printf("   +  %sLineCap:", Which);
        (void)U_PMF_GRAPHICSVERSION_memsafe_print((char *)&Version, out,
                                                  states);
        verbose_printf(", Type %X\n", Type);
        switch (Type) {
        case U_CLCDT_Default:
            status = U_PMF_CUSTOMLINECAPDATA_print(Data, blimit, out, states);
            break;
        case U_CLCDT_AdjustableArrow:
            status =
                U_PMF_CUSTOMLINECAPARROWDATA_print(Data, blimit, out, states);
            break;
        default:
            status = 0;
        }
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_FONT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.3, Microsoft name: EmfPlusFont Object
  */
int U_PMF_FONT_print(const char *contents, const char *blimit, FILE *out,
                     drawingStates *states) {
    FLAG_IGNORED;
    uint32_t Version, SizeUnit, Length;
    U_FLOAT EmSize;
    int32_t FSFlags;
    const char *Data;
    char *string;
    int status = U_PMF_FONT_get(contents, &Version, &EmSize, &SizeUnit,
                                &FSFlags, &Length, &Data, blimit);
    if (status) {
        verbose_printf("   +  Font:");
        (void)U_PMF_GRAPHICSVERSION_memsafe_print((char *)&Version, out,
                                                  states);
        verbose_printf(" EmSize:%f ", EmSize);
        verbose_printf(" SizeUnit:%d ", SizeUnit);
        verbose_printf(" FSFlags:%d ", FSFlags);
        verbose_printf(" Length:%d", Length);
        string = U_Utf16leToUtf8((uint16_t *)Data, Length, NULL);
        if (string) {
            verbose_printf(" Family:<%s>\n", string);
            free(string);
        } else {
            verbose_printf(" Family:<>\n");
        }
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IMAGE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.4, Microsoft name: EmfPlusImage Object
  */
int U_PMF_IMAGE_print(const char *contents, const char *blimit, FILE *out,
                      drawingStates *states) {
    FLAG_IGNORED;
    uint32_t Version, Type;
    const char *Data;
    int status = U_PMF_IMAGE_get(contents, &Version, &Type, &Data, blimit);
    if (status) {
        verbose_printf("   +  Image:");
        (void)U_PMF_GRAPHICSVERSION_memsafe_print((char *)&Version, out,
                                                  states);
        verbose_printf(" Type:%X\n", Type);
        switch (Type) {
        case U_IDT_Unknown:
            verbose_printf("   +  Unknown Image Type\n");
            break;
        case U_IDT_Bitmap:
            status = U_PMF_BITMAP_print(Data, blimit, out, states);
            break;
        case U_IDT_Metafile:
            status = U_PMF_METAFILE_print(Data, blimit, out, states);
            break;
        default:
            status = 0;
        }
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IMAGEATTRIBUTES object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.5, Microsoft name: EmfPlusImageAttributes Object
  */
int U_PMF_IMAGEATTRIBUTES_print(const char *contents, const char *blimit,
                                FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    uint32_t Version, WrapMode, ClampColor, ObjectClamp;
    int status = U_PMF_IMAGEATTRIBUTES_get(contents, &Version, &WrapMode,
                                           &ClampColor, &ObjectClamp, blimit);

    if (status) {
        verbose_printf("   +  Image Attributes: ");
        (void)U_PMF_GRAPHICSVERSION_memsafe_print((char *)&Version, out,
                                                  states);
        verbose_printf(" WrapMode:%X", WrapMode);
        verbose_printf(" ClampColor:%X", ClampColor);
        verbose_printf(" ObjectClamp:%X\n", ObjectClamp);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATH object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.6, Microsoft name: EmfPlusPath Object
  */
int U_PMF_PATH_print(const char *contents, const char *blimit, FILE *out,
                     drawingStates *states) {
    FLAG_IGNORED;
    unsigned int i, pos;
    uint32_t Version, Count;
    uint16_t Flags;
    const char *Points;
    const char *Types;
    int status = U_PMF_PATH_get(contents, &Version, &Count, &Flags, &Points,
                                &Types, blimit);
    if (status) {
        verbose_printf("   +  Path: Version:%X Count:%d Flags:%X\n", Version,
                       Count, Flags);

        /* Points part */
        U_PMF_VARPOINTS_print(&Points, Flags, Count, blimit, out, states);

        /* Types part */
        verbose_printf("   +  Types:");
        pos = 0;
        for (i = 0; i < Count; i++) {
            /* EMF+ manual says that the first of these two cases can actually
               contain either type
               of PATHPOINT, but it does not say how the program is supposed to
               figure out which record
               is which type. */
            if (Flags & U_PPF_R) {
                verbose_printf(" %u:", pos);
                pos += U_PMF_PATHPOINTTYPERLE_print(Types, blimit, out, states);
                Types += 2;
            } else {
                verbose_printf(" %d:", i);
                (void)U_PMF_PATHPOINTTYPE_print(Types, blimit, out, states);
                Types++;
            }
        }
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_PEN object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.7, Microsoft name: EmfPlusPen Object
  */
int U_PMF_PEN_print(const char *contents, const char *blimit, FILE *out,
                    drawingStates *states) {
    FLAG_IGNORED;
    uint32_t Version, Type;
    const char *PenData;
    const char *Brush;
    int status =
        U_PMF_PEN_get(contents, &Version, &Type, &PenData, &Brush, blimit);
    if (status) {
        verbose_printf("   +  Pen: Version:%X Type:%d\n", Version, Type);
        (void)U_PMF_PENDATA_print(PenData, blimit, out, states);
        (void)U_PMF_BRUSH_print(Brush, blimit, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_REGION object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.8, Microsoft name: EmfPlusRegion Object
  */
int U_PMF_REGION_print(const char *contents, const char *blimit, FILE *out,
                       drawingStates *states) {
    FLAG_IGNORED;
    uint32_t Version, Count;
    const char *Nodes;
    int status = U_PMF_REGION_get(contents, &Version, &Count, &Nodes, blimit);
    if (status) {
        verbose_printf("   + ");
        (void)U_PMF_GRAPHICSVERSION_memsafe_print((char *)&Version, out,
                                                  states);

        verbose_printf(" ChildNodes:%d", Count);
        (void)U_PMF_REGIONNODE_print(Nodes, 1, blimit, out,
                                     states); /* 1 == top level*/
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_STRINGFORMAT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.9, Microsoft name: EmfPlusStringFormat Object
  */
int U_PMF_STRINGFORMAT_print(const char *contents, const char *blimit,
                             FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    U_PMF_STRINGFORMAT Sfs;
    const char *Data;
    int status = U_PMF_STRINGFORMAT_get(contents, &Sfs, &Data, blimit);
    if (status) {
        verbose_printf("   +  StringFormat: ");
        verbose_printf(" Version:%X", Sfs.Version);
        verbose_printf(" Flags:%X", Sfs.Flags);
        verbose_printf(" Language");
        (void)U_PMF_LANGUAGEIDENTIFIER_print(Sfs.Language, out, states);
        verbose_printf(" StringAlignment:%X", Sfs.StringAlignment);
        verbose_printf(" LineAlign:%X", Sfs.LineAlign);
        verbose_printf(" DigitSubstitution:%X", Sfs.DigitSubstitution);
        verbose_printf(" DigitLanguage");
        (void)U_PMF_LANGUAGEIDENTIFIER_print(Sfs.DigitLanguage, out, states);
        verbose_printf(" FirstTabOffset:%f", Sfs.FirstTabOffset);
        verbose_printf(" HotkeyPrefix:%d", Sfs.HotkeyPrefix);
        verbose_printf(" LeadingMargin:%f", Sfs.LeadingMargin);
        verbose_printf(" TrailingMargin:%f", Sfs.TrailingMargin);
        verbose_printf(" Tracking:%f", Sfs.Tracking);
        verbose_printf(" Trimming:%X", Sfs.Trimming);
        verbose_printf(" TabStopCount:%u", Sfs.TabStopCount);
        verbose_printf(" RangeCount:%u", Sfs.RangeCount);
        (void)U_PMF_STRINGFORMATDATA_print(Data, Sfs.TabStopCount,
                                           Sfs.RangeCount, blimit, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_ARGB object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.1, Microsoft name: EmfPlusARGB Object
  */
int U_PMF_ARGB_print(const char *contents, FILE *out, drawingStates *states) {
    uint8_t Blue, Green, Red, Alpha;
    int status = U_PMF_ARGB_get(contents, &Blue, &Green, &Red, &Alpha,
                                contents + sizeof(U_RGBQUAD));
    if (status) {
        verbose_printf(" RGBA{%2.2X,%2.2X,%2.2X,%2.2X}", Red, Green, Blue,
                       Alpha);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_BITMAP object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.2, Microsoft name: EmfPlusBitmap Object
  */
int U_PMF_BITMAP_print(const char *contents, const char *blimit, FILE *out,
                       drawingStates *states) {
    FLAG_IGNORED;
    U_PMF_BITMAP Bs;
    const char *Data;
    int status = U_PMF_BITMAP_get(contents, &Bs, &Data, blimit);
    if (status) {
        verbose_printf("   +  Bitmap: Width:%d Height:%d Stride:%d\n", Bs.Width,
                       Bs.Height, Bs.Stride);
        U_PMF_PX_FMT_ENUM_print(Bs.PxFormat, out, states);
        switch (Bs.Type) {
        case 0:
            verbose_printf(" Type:MSBitmap\n");
            break;
        case 1:
            verbose_printf(" Type:(PNG|JPG|GIF|EXIF|TIFF)\n");
            break;
        default:
            verbose_printf(" Type:INVALID(%d)\n", Bs.Type);
            break;
        }
        /* Pixel data is never shown - it could easily swamp the output for even
         * a smallish picture */
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_BITMAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.3, Microsoft name: EmfPlusBitmapData Object
  */
int U_PMF_BITMAPDATA_print(const char *contents, const char *blimit, FILE *out,
                           drawingStates *states) {
    unsigned int i;
    U_PMF_PALETTE Ps;
    const char *Colors;
    const char *Data;
    int status = U_PMF_BITMAPDATA_get(contents, &Ps, &Colors, &Data, blimit);
    if (status) {
        status = 0;
        verbose_printf(" BMData: Flags:%X, Elements:%u Colors:", Ps.Flags,
                       Ps.Elements);
        for (i = 0; i < Ps.Elements; i++, Colors += sizeof(U_PMF_ARGB)) {
            (void)U_PMF_ARGB_print(Colors, out, states);
        }
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_BLENDCOLORS object
  \return size in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.4, Microsoft name: EmfPlusBlendColors Object
  */
int U_PMF_BLENDCOLORS_print(const char *contents, const char *blimit, FILE *out,
                            drawingStates *states) {
    FLAG_IGNORED;
    unsigned int i;
    uint32_t Elements;
    U_FLOAT *Positions;
    const char *Colors;
    int status =
        U_PMF_BLENDCOLORS_get(contents, &Elements, &Positions, &Colors, blimit);
    if (status) {
        verbose_printf("   +  BlendColors:  Entries:%d (entry,pos,color): ",
                       Elements);
        for (i = 0; i < Elements; i++) {
            verbose_printf(" (%d,%f,", i, Positions[i]);
            (void)U_PMF_ARGB_print(Colors, out, states);
            Colors += sizeof(U_PMF_ARGB);
            verbose_printf(")");
        }
        status = sizeof(uint32_t) + Elements * sizeof(U_FLOAT) +
                 Elements * sizeof(U_PMF_ARGB);
        free(Positions);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_BLENDFACTORS object
  \return size on success, 0 on error
  \param  type       Type of BlendFactors, usually H or V
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.5, Microsoft name: EmfPlusBlendFactors Object
  */
int U_PMF_BLENDFACTORS_print(const char *contents, const char *type,
                             const char *blimit, FILE *out,
                             drawingStates *states) {
    FLAG_IGNORED;
    unsigned int i;
    uint32_t Elements;
    U_FLOAT *Positions;
    U_FLOAT *Factors;
    int status = U_PMF_BLENDFACTORS_get(contents, &Elements, &Positions,
                                        &Factors, blimit);
    if (status) {
        verbose_printf("   +  BlendFactors%s: Entries:%d (entry,pos,factor): ",
                       type, Elements);
        for (i = 0; i < Elements; i++) {
            verbose_printf(" (%d,%f,%f)", i, Positions[i], Factors[i]);
        }
        status = sizeof(uint32_t) + Elements * 2 * sizeof(U_FLOAT);
        free(Positions);
        free(Factors);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_BOUNDARYPATHDATA object
  \return size on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.6, Microsoft name: EmfPlusBoundaryPathData Object
  */
int U_PMF_BOUNDARYPATHDATA_print(const char *contents, const char *blimit,
                                 FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    int32_t Size;
    const char *Data;
    int status = U_PMF_BOUNDARYPATHDATA_get(contents, &Size, &Data, blimit);
    if (status) {
        verbose_printf("   +  BoundaryPathData: Size:%d\n", Size);
        (void)U_PMF_PATH_print(Data, blimit, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_BOUNDARYPOINTDATA object
  \return size on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.7, Microsoft name: EmfPlusBoundaryPointData Object
  */
int U_PMF_BOUNDARYPOINTDATA_print(const char *contents, const char *blimit,
                                  FILE *out, drawingStates *states) {
    int32_t Elements;
    U_PMF_POINTF *Points;
    int status =
        U_PMF_BOUNDARYPOINTDATA_get(contents, &Elements, &Points, blimit);
    if (status) {
        verbose_printf("   +  BoundaryPointData: Elements:%u\n", Elements);
        U_PMF_VARPOINTF_S_print(Points, Elements, out, states);
        free(Points);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_CHARACTERRANGE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.8, Microsoft name: EmfPlusCharacterRange Object
  */
int U_PMF_CHARACTERRANGE_print(const char *contents, const char *blimit,
                               FILE *out, drawingStates *states) {
    int32_t First, Length;
    int status = U_PMF_CHARACTERRANGE_get(contents, &First, &Length, blimit);
    if (status) {
        verbose_printf(" {%d,%d}", First, Length);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_DASHEDLINEDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.9, Microsoft name: EmfPlusCompoundLineData Object
  */
int U_PMF_COMPOUNDLINEDATA_print(const char *contents, const char *blimit,
                                 FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    int32_t Elements;
    U_FLOAT *Widths;
    U_FLOAT *hold;
    int status =
        U_PMF_COMPOUNDLINEDATA_get(contents, &Elements, &Widths, blimit);
    if (status) {
        verbose_printf("   +  CompoundLineData: Elements:%u {", Elements);
        Elements--;
        for (hold = Widths; Elements; Elements--, Widths++) {
            verbose_printf("%f, ", *Widths);
        }
        verbose_printf("%f}", *Widths);
        free(hold);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_COMPRESSEDIMAGE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.10, Microsoft name: EmfPlusCompressedImage Object

  This function does not do anything useful, but it is included so that all
  objects have a corresponding _get().
  */
int U_PMF_COMPRESSEDIMAGE_print(const char *contents, const char *blimit,
                                FILE *out, drawingStates *states) {
    const char *Data;
    int status = U_PMF_COMPRESSEDIMAGE_get(contents, &Data, blimit);
    if (status) {
        verbose_printf("CompressedImage:\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMENDCAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.11, Microsoft name: EmfPlusCustomEndCapData Object
  */
int U_PMF_CUSTOMENDCAPDATA_print(const char *contents, const char *blimit,
                                 FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    int32_t Size;
    const char *Data;
    int status = U_PMF_CUSTOMENDCAPDATA_get(contents, &Size, &Data, blimit);
    if (status) {
        verbose_printf("   +  CustomEndCap: Size:%d\n", Size);
        (void)U_PMF_CUSTOMLINECAP_print(Data, "End", blimit, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAPARROWDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.12, Microsoft name: EmfPlusCustomLineCapArrowData Object
  */
int U_PMF_CUSTOMLINECAPARROWDATA_print(const char *contents, const char *blimit,
                                       FILE *out, drawingStates *states) {
    U_PMF_CUSTOMLINECAPARROWDATA Ccad;
    int status = U_PMF_CUSTOMLINECAPARROWDATA_get(contents, &Ccad, blimit);
    if (status) {
        verbose_printf("CustomLineCapArrowData: ");
        verbose_printf(" Width:%f", Ccad.Width);
        verbose_printf(" Height:%f", Ccad.Height);
        verbose_printf(" MiddleInset:%f", Ccad.MiddleInset);
        verbose_printf(" FillState:%u", Ccad.FillState);
        verbose_printf(" StartCap:%X", Ccad.StartCap);
        verbose_printf(" EndCap:%X", Ccad.EndCap);
        verbose_printf(" Join:%X", Ccad.Join);
        verbose_printf(" MiterLimit:%f", Ccad.MiterLimit);
        verbose_printf(" WidthScale:%f", Ccad.WidthScale);
        verbose_printf(" FillHotSpot:{%f,%f}", Ccad.FillHotSpot[0],
                       Ccad.FillHotSpot[1]);
        verbose_printf(" LineHotSpot:{%f,%f}", Ccad.LineHotSpot[0],
                       Ccad.LineHotSpot[1]);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.13, Microsoft name: EmfPlusCustomLineCapData Object
  */
int U_PMF_CUSTOMLINECAPDATA_print(const char *contents, const char *blimit,
                                  FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    U_PMF_CUSTOMLINECAPDATA Clcd;
    const char *Data;
    int status = U_PMF_CUSTOMLINECAPDATA_get(contents, &Clcd, &Data, blimit);
    if (status) {
        verbose_printf("   +  CustomLineCapData: ");
        verbose_printf(" Flags:%X", Clcd.Flags);
        verbose_printf(" Cap:%X", Clcd.Cap);
        verbose_printf(" Inset:%f", Clcd.Inset);
        verbose_printf(" StartCap:%X", Clcd.StartCap);
        verbose_printf(" EndCap:%X", Clcd.EndCap);
        verbose_printf(" Join:%X", Clcd.Join);
        verbose_printf(" MiterLimit:%f", Clcd.MiterLimit);
        verbose_printf(" WidthScale:%f", Clcd.WidthScale);
        verbose_printf(" FillHotSpot:{%f,%f}", Clcd.FillHotSpot[0],
                       Clcd.FillHotSpot[1]);
        verbose_printf(" LineHotSpot:{%f,%f}\n", Clcd.LineHotSpot[0],
                       Clcd.LineHotSpot[1]);
        (void)U_PMF_CUSTOMLINECAPOPTIONALDATA_print(Data, Clcd.Flags, blimit,
                                                    out, states);
        /* preceding line always emits an EOL */
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAPOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  Flags      CustomLineCapData Flags

  EMF+ manual 2.2.2.14, Microsoft name: EmfPlusCustomLineCapOptionalData Object
  */
int U_PMF_CUSTOMLINECAPOPTIONALDATA_print(const char *contents, uint32_t Flags,
                                          const char *blimit, FILE *out,
                                          drawingStates *states) {
    FLAG_IGNORED;
    const char *FillData;
    const char *LineData;
    int status = U_PMF_CUSTOMLINECAPOPTIONALDATA_get(contents, Flags, &FillData,
                                                     &LineData, blimit);
    if (status) { /* True even if there is nothing in it! */
        verbose_printf("   +  CustomLineCapOptionalData:");
        if (FillData || LineData) {
            if (FillData) {
                (void)U_PMF_FILLPATHOBJ_print(FillData, blimit, out, states);
            }
            if (LineData) {
                (void)U_PMF_LINEPATH_print(LineData, blimit, out, states);
            }
        } else {
            verbose_printf("None");
        }
    }
    if (status <= 1) {
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMSTARTCAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.15, Microsoft name: EmfPlusCustomStartCapData Object
  */
int U_PMF_CUSTOMSTARTCAPDATA_print(const char *contents, const char *blimit,
                                   FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    int32_t Size;
    const char *Data;
    int status = U_PMF_CUSTOMSTARTCAPDATA_get(contents, &Size, &Data, blimit);
    if (status) {
        verbose_printf("   +  CustomStartCap: Size:%d ", Size);
        (void)U_PMF_CUSTOMLINECAP_print(Data, "Start", blimit, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_DASHEDLINEDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.16, Microsoft name: EmfPlusDashedLineData Object
  */
int U_PMF_DASHEDLINEDATA_print(const char *contents, const char *blimit,
                               FILE *out, drawingStates *states) {
    int32_t Elements;
    U_FLOAT *Lengths;
    U_FLOAT *hold;
    int status =
        U_PMF_DASHEDLINEDATA_get(contents, &Elements, &Lengths, blimit);
    if (status) {
        verbose_printf(" DashedLineData: Elements:%u {", Elements);
        Elements--;
        for (hold = Lengths; Elements; Elements--, Lengths++) {
            verbose_printf("%f, ", *Lengths);
        }
        verbose_printf("%f}", *Lengths);
        free(hold);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_FILLPATHOBJ object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.17, Microsoft name: EmfPlusFillPath Object
  */
int U_PMF_FILLPATHOBJ_print(const char *contents, const char *blimit, FILE *out,
                            drawingStates *states) {
    int32_t Size;
    const char *Data;
    int status = U_PMF_FILLPATHOBJ_get(contents, &Size, &Data, blimit);
    if (status) {
        verbose_printf(" FillPathObj: Size:%d\n", Size);
        if (Size) {
            (void)U_PMF_PATH_print(Data, blimit, out, states);
        }
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_FOCUSSCALEDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.18, Microsoft name: EmfPlusFocusScaleData Object
  */
int U_PMF_FOCUSSCALEDATA_print(const char *contents, const char *blimit,
                               FILE *out, drawingStates *states) {
    uint32_t Count;
    U_FLOAT ScaleX, ScaleY;
    int status =
        U_PMF_FOCUSSCALEDATA_get(contents, &Count, &ScaleX, &ScaleY, blimit);
    if (status) {
        verbose_printf(" FocusScaleData: Count:%d ScaleX:%f ScaleY:%f ", Count,
                       ScaleX, ScaleY);
    }
    return (status);
}

/**
    \brief Print data from a  U_PMF_GRAPHICSVERSION object already known to be
   memory safe
    \return 1 on success, 0 on error
    \param  contents   Record from which to print data
    EMF+ manual 2.2.2.19, Microsoft name: EmfPlusGraphicsVersion Object

    In this module the only time a U_PMF_GRAPHICSVERSION is printed is after it
    has already been copied into a safe memory structure.  This routine fakes up
    a blimit for the general routine.

*/
int U_PMF_GRAPHICSVERSION_memsafe_print(const char *contents, FILE *out,
                                        drawingStates *states) {
    const char *blimit = contents + sizeof(U_PMF_GRAPHICSVERSION);
    return (U_PMF_GRAPHICSVERSION_print(contents, blimit, out, states));
}

/**
    \brief Print data from a  U_PMF_GRAPHICSVERSION object
    \return 1 on success, 0 on error
    \param  contents   Record from which to print data
    \param  blimit     one byte past the end of data
    EMF+ manual 2.2.2.19, Microsoft name: EmfPlusGraphicsVersion Object
*/
int U_PMF_GRAPHICSVERSION_print(const char *contents, const char *blimit,
                                FILE *out, drawingStates *states) {
    int Signature, GrfVersion;
    int status =
        U_PMF_GRAPHICSVERSION_get(contents, &Signature, &GrfVersion, blimit);
    if (status) {
        verbose_printf(" MetaFileSig:%X", Signature);
        verbose_printf(" GraphicsVersion:%X", GrfVersion);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_HATCHBRUSHDATA_print object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.20, Microsoft name: EmfPlusHatchBrushData Object
  */
int U_PMF_HATCHBRUSHDATA_print(const char *contents, const char *blimit,
                               FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    uint32_t Style;
    U_PMF_ARGB Foreground, Background;
    int status = U_PMF_HATCHBRUSHDATA_get(contents, &Style, &Foreground,
                                          &Background, blimit);
    if (status) {
        verbose_printf("   +  HBdata: Style:%u(", Style);
        U_PMF_HATCHSTYLEENUMERATION_print(Style, out, states);
        verbose_printf(") FG:{");
        (void)U_PMF_ARGB_print((char *)&Foreground, out, states);
        verbose_printf("} BG:{");
        (void)U_PMF_ARGB_print((char *)&Background, out, states);
        verbose_printf("}");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_LANGUAGEIDENTIFIER object
  \return 1 on success, 0 on error
  \param  LId   Record from which to print data
  EMF+ manual 2.2.2.23, Microsoft name: EmfPlusLanguageIdentifier Object
  */
int U_PMF_LANGUAGEIDENTIFIER_print(U_PMF_LANGUAGEIDENTIFIER LId, FILE *out,
                                   drawingStates *states) {
    FLAG_IGNORED;
    int SubLId, PriLId;
    int status = U_PMF_LANGUAGEIDENTIFIER_get(LId, &SubLId, &PriLId);
    if (status) { /* do it the hard way just to verify that the preceding call
                     works, OK to just print LId directly */
        verbose_printf("{%4.4X}",
                       U_PMF_LANGUAGEIDENTIFIEROBJ_set(SubLId, PriLId));
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_LINEARGRADIENTBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.24, Microsoft name: EmfPlusLinearGradientBrushData Object
  */
int U_PMF_LINEARGRADIENTBRUSHDATA_print(const char *contents,
                                        const char *blimit, FILE *out,
                                        drawingStates *states) {
    U_PMF_LINEARGRADIENTBRUSHDATA Lgbd;
    const char *Data;
    int status =
        U_PMF_LINEARGRADIENTBRUSHDATA_get(contents, &Lgbd, &Data, blimit);
    if (status) {
        verbose_printf(
            "   +  LinearGradientBrushData: Flags:%X WrapMode:%d Rect:",
            Lgbd.Flags, Lgbd.WrapMode);
        (void)U_PMF_RECTF_S_print(&(Lgbd.RectF), out, states);
        verbose_printf(" StartColor:");
        (void)U_PMF_ARGB_print((char *)&(Lgbd.StartColor), out, states);
        verbose_printf(" EndColor:");
        (void)U_PMF_ARGB_print((char *)&(Lgbd.EndColor), out, states);
        /* Technically these are to be ignored, in practice they must be colors
         * with the same value as the preceding 2*/
        verbose_printf(" Reserved1:");
        (void)U_PMF_ARGB_print((char *)&(Lgbd.Reserved1), out, states);
        verbose_printf(" Reserved2:");
        (void)U_PMF_ARGB_print((char *)&(Lgbd.Reserved2), out, states);
        verbose_printf("\n");
        (void)U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA_print(Data, Lgbd.Flags,
                                                          blimit, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  BDFlag     Describes optional values in contents
  EMF+ manual 2.2.2.25, Microsoft name: EmfPlusLinearGradientBrushOptionalData
  Object
  */
int U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA_print(const char *contents,
                                                int BDFlag, const char *blimit,
                                                FILE *out,
                                                drawingStates *states) {
    FLAG_IGNORED;
    U_PMF_TRANSFORMMATRIX Tm;
    const char *Bc;
    const char *BfH;
    const char *BfV;
    int None = 1;
    verbose_printf("   +  LinearGradientBrushOptionalData: ");
    int status = U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA_get(
        contents, BDFlag, &Tm, &Bc, &BfH, &BfV, blimit);
    if (status) {
        if (BDFlag & U_BD_Transform) {
            U_PMF_TRANSFORMMATRIX2_print(&Tm, out, states);
            None = 0;
        }
        if (Bc) {
            verbose_printf("\n");
            (void)U_PMF_BLENDCOLORS_print(Bc, blimit, out, states);
            None = 0;
        }
        if (BfH) {
            verbose_printf("\n");
            (void)U_PMF_BLENDFACTORS_print(BfH, "H", blimit, out, states);
            None = 0;
        }
        if (BfV) {
            verbose_printf("\n");
            (void)U_PMF_BLENDFACTORS_print(BfV, "V", blimit, out, states);
            None = 0;
        }
        if (None) {
            verbose_printf("(none)");
        }
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_LINEPATH object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.26, Microsoft name: EmfPlusLinePath Object
  */
int U_PMF_LINEPATH_print(const char *contents, const char *blimit, FILE *out,
                         drawingStates *states) {
    int32_t Size;
    const char *Data;
    int status = U_PMF_LINEPATH_get(contents, &Size, &Data, blimit);
    if (status) {
        verbose_printf(" LinePath: Size:%d\n", Size);
        (void)U_PMF_PATH_print(Data, blimit, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_METAFILE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.27, Microsoft name: EmfPlusMetafile Object
  */
int U_PMF_METAFILE_print(const char *contents, const char *blimit, FILE *out,
                         drawingStates *states) {
    uint32_t Type;
    uint32_t Size;
    const char *Data;
    int status = U_PMF_METAFILE_get(contents, &Type, &Size, &Data, blimit);
    if (status) {
        verbose_printf(" MetaFile: Type:%X Size:%d", Type, Size);
        /* embedded metafiles are not handled beyond this*/
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_PALETTE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.28, Microsoft name: EmfPlusPalette Object
  */
int U_PMF_PALETTE_print(const char *contents, const char *blimit, FILE *out,
                        drawingStates *states) {
    unsigned int i;
    uint32_t Flags;
    uint32_t Elements;
    const char *Data;
    int status = U_PMF_PALETTE_get(contents, &Flags, &Elements, &Data, blimit);
    if (status) {
        verbose_printf(" Palette: Flags:%X Elements:%u Colors:", Flags,
                       Elements);
        for (i = 0; i < Elements; i++) {
            (void)U_PMF_ARGB_print(Data, out, states);
            Data += sizeof(U_PMF_ARGB);
        }
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATHGRADIENTBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.29, Microsoft name: EmfPlusPathGradientBrushData Object
  */
int U_PMF_PATHGRADIENTBRUSHDATA_print(const char *contents, const char *blimit,
                                      FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    U_PMF_PATHGRADIENTBRUSHDATA Pgbd;
    const char *Gradient;
    const char *Boundary;
    const char *Data = NULL;
    unsigned int i;
    int status = U_PMF_PATHGRADIENTBRUSHDATA_get(contents, &Pgbd, &Gradient,
                                                 &Boundary, &Data, blimit);
    if (status) {
        verbose_printf(
            "   +  PathGradientBrushData: Flags:%X WrapMode:%d, CenterColor:",
            Pgbd.Flags, Pgbd.WrapMode);
        (void)U_PMF_ARGB_print((char *)&(Pgbd.CenterColor), out, states);
        verbose_printf(" Center:");
        (void)U_PMF_POINTF_S_print(&(Pgbd.Center), out, states);
        verbose_printf(" Elements:%u\n", Pgbd.Elements);
        if (Pgbd.Elements) {
            verbose_printf("   +  SurroundingColor: ");
            for (i = Pgbd.Elements; i; i--, Gradient += 4) {
                (void)U_PMF_ARGB_print(Gradient, out, states);
            }
            verbose_printf("\n");
        }
        if (Pgbd.Flags & U_BD_Path) {
            (void)U_PMF_BOUNDARYPATHDATA_print(Boundary, blimit, out, states);
        } else {
            (void)U_PMF_BOUNDARYPOINTDATA_print(Boundary, blimit, out, states);
        }
        (void)U_PMF_PATHGRADIENTBRUSHOPTIONALDATA_print(Data, Pgbd.Flags,
                                                        blimit, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATHGRADIENTBRUSHOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  BDFlag     Describes optional values in contents
  EMF+ manual 2.2.2.30, Microsoft name: EmfPlusPathGradientBrushOptionalData
  Object
  */
int U_PMF_PATHGRADIENTBRUSHOPTIONALDATA_print(const char *contents, int BDFlag,
                                              const char *blimit, FILE *out,
                                              drawingStates *states) {
    FLAG_IGNORED;
    if (BDFlag & (U_BD_Transform | U_BD_PresetColors | U_BD_BlendFactorsH |
                  U_BD_FocusScales)) {
        verbose_printf("   +  PathGradientBrushOptionalData: ");
    }
    if (BDFlag & U_BD_Transform) {
        U_PMF_TRANSFORMMATRIX_print(contents, blimit, out, states);
        contents += sizeof(U_PMF_TRANSFORMMATRIX);
    }
    if (BDFlag &
        U_BD_PresetColors) { /* If this is present, BlendFactorsH will not be */
        contents += U_PMF_BLENDCOLORS_print(contents, blimit, out, states);
    }
    if (BDFlag & U_BD_BlendFactorsH) { /* If this is present, U_BD_PresetColors
                                          will not be */
        contents += U_PMF_BLENDFACTORS_print(contents, "", blimit, out, states);
    }
    if (BDFlag & U_BD_FocusScales) {
        contents += U_PMF_BLENDFACTORS_print(contents, "", blimit, out, states);
        U_PMF_FOCUSSCALEDATA_print(contents, blimit, out, states);
    }
    return (1);
}

/**
  \brief Print data from a  U_PMF_PATHPOINTTYPE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.31, Microsoft name: EmfPlusPathPointType Object
  */
int U_PMF_PATHPOINTTYPE_print(const char *contents, const char *blimit,
                              FILE *out, drawingStates *states) {
    int Flags, Type;
    int status = U_PMF_PATHPOINTTYPE_get(contents, &Flags, &Type, blimit);
    if (status) {
        verbose_printf("{Flags:%X Type:", Flags);
        (void)U_PMF_PATHPOINTTYPE_ENUM_print(Type, out, states);
        verbose_printf("}");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATHPOINTTYPERLE object
  \return Number of elements in the run, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.32, Microsoft name: EmfPlusPathPointTypeRLE Object
  */
int U_PMF_PATHPOINTTYPERLE_print(const char *contents, const char *blimit,
                                 FILE *out, drawingStates *states) {
    int Bezier, Elements, Type;
    int status =
        U_PMF_PATHPOINTTYPERLE_get(contents, &Bezier, &Elements, &Type, blimit);
    if (status) {
        status = Elements;
        verbose_printf(" PathPointTypeRLE: Bezier:%c Elements:%u, Type: ",
                       (Bezier ? 'Y' : 'N'), Elements);
        (void)U_PMF_PATHPOINTTYPE_ENUM_print(Type, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATHPOINTTYPERLE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.33, Microsoft name: EmfPlusPenData Object
  */
int U_PMF_PENDATA_print(const char *contents, const char *blimit, FILE *out,
                        drawingStates *states) {
    FLAG_IGNORED;
    uint32_t Flags, Unit;
    U_FLOAT Width;
    const char *Data;
    int status =
        U_PMF_PENDATA_get(contents, &Flags, &Unit, &Width, &Data, blimit);
    if (status) {
        verbose_printf("   +  Pendata: Flags:%X Unit:%X Width:%f", Flags, Unit,
                       Width);
        (void)U_PMF_PENOPTIONALDATA_print(
            Data, Flags, blimit, out, states); /* prints a new line at end */
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_PENOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  Flags      PenData Flags that determine which optionaldata fields are
  present in the record.

  EMF+ manual 2.2.2.34, Microsoft name: EmfPlusPenOptionalData Object
  */
int U_PMF_PENOPTIONALDATA_print(const char *contents, int Flags,
                                const char *blimit, FILE *out,
                                drawingStates *states) {
    U_PMF_TRANSFORMMATRIX Matrix;
    int32_t StartCap;
    int32_t EndCap;
    uint32_t Join;
    U_FLOAT MiterLimit;
    int32_t Style;
    int32_t DLCap;
    U_FLOAT DLOffset;
    const char *DLData;
    int32_t Alignment;
    const char *CmpndLineData;
    const char *CSCapData;
    const char *CECapData;
    int status = U_PMF_PENOPTIONALDATA_get(
        contents,
        Flags, // determines which fields are filled
        &Matrix, &StartCap, &EndCap, &Join, &MiterLimit, &Style, &DLCap,
        &DLOffset, &DLData, &Alignment, &CmpndLineData, &CSCapData, &CECapData,
        blimit);
    if (status) {
        if (Flags & U_PD_Transform) {
            (void)U_PMF_TRANSFORMMATRIX2_print(&Matrix, out, states);
        }
        if (Flags & U_PD_StartCap) {
            verbose_printf(" StartCap:%d", StartCap);
        }
        if (Flags & U_PD_EndCap) {
            verbose_printf(" EndCap:%d", EndCap);
        }
        if (Flags & U_PD_Join) {
            verbose_printf(" Join:%X", Join);
        }
        if (Flags & U_PD_MiterLimit) {
            verbose_printf(" MiterLimit:%f", MiterLimit);
        }
        if (Flags & U_PD_LineStyle) {
            verbose_printf(" Style:%X", Style);
        }
        if (Flags & U_PD_DLCap) {
            verbose_printf(" DLCap:%X", DLCap);
        }
        if (Flags & U_PD_DLOffset) {
            verbose_printf(" DLOffset:%f", DLOffset);
        }
        if (Flags & U_PD_DLData) {
            (void)U_PMF_DASHEDLINEDATA_print(DLData, blimit, out, states);
        }
        if (Flags & U_PD_NonCenter) {
            verbose_printf(" Alignment:%d", Alignment);
        }
        if (Flags & (U_PD_Transform | U_PD_StartCap | U_PD_EndCap | U_PD_Join |
                     U_PD_MiterLimit | U_PD_LineStyle | U_PD_DLCap |
                     U_PD_DLOffset | U_PD_DLData | U_PD_NonCenter)) {
            verbose_printf("\n");
        }
        if (Flags & U_PD_CLData) {
            (void)U_PMF_COMPOUNDLINEDATA_print(CmpndLineData, blimit, out,
                                               states);
        }
        if (Flags & U_PD_CustomStartCap) {
            (void)U_PMF_CUSTOMSTARTCAPDATA_print(CSCapData, blimit, out,
                                                 states);
        }
        if (Flags & U_PD_CustomEndCap) {
            (void)U_PMF_CUSTOMENDCAPDATA_print(CECapData, blimit, out, states);
        }
    }
    return (status);
}
/**
  \brief Print data from a  U_PMF_POINT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.35, Microsoft name: EmfPlusPoint Object
  */
int U_PMF_POINT_print(const char **contents, const char *blimit, FILE *out,
                      drawingStates *states) {
    U_FLOAT X, Y;
    int status = U_PMF_POINT_get(contents, &X, &Y, blimit);
    if (status) {
        verbose_printf("{%f,%f}", X, Y);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_POINT Structure
  \return 1 on success, 0 on error
  \param  Point   U_PMF_POINT Structure to print
  EMF+ manual 2.2.2.35, Microsoft name: EmfPlusPoint Object
  */
int U_PMF_POINT_S_print(U_PMF_POINT *Point, FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    if (!Point) {
        return (0);
    }
    verbose_printf("{%d,%d}", Point->X, Point->Y);
    return (1);
}

/**
  \brief Print data from a  U_PMF_POINTF object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.36, Microsoft name: EmfPlusPointF Object
  */
int U_PMF_POINTF_print(const char **contents, const char *blimit, FILE *out,
                       drawingStates *states) {
    U_FLOAT X, Y;
    int status = U_PMF_POINTF_get(contents, &X, &Y, blimit);
    if (status) {
        verbose_printf("{%f,%f}", X, Y);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_POINTF Structure
  \return 1 on success, 0 on error
  \param  Point   U_PMF_POINTF Structure to print
  EMF+ manual 2.2.2.36, Microsoft name: EmfPlusPointF Object
  */
int U_PMF_POINTF_S_print(U_PMF_POINTF *Point, FILE *out,
                         drawingStates *states) {
    if (!Point) {
        return (0);
    }
    verbose_printf("{%f,%f}", Point->X, Point->Y);
    return (1);
}

/**
  \brief Print data from a  U_PMF_POINTR object
  \return bytes traversed on success, 0 on error
  \param  contents   Pointer to next data to print
  \param  Xpos       X coordinate for current point
  \param  Ypos       Y coordinate for current point

  On each call the next relative offset is extracted, the current
  coordinates are modified with that offset, and the pointer is
  advanced to the next data point.

  EMF+ manual 2.2.2.37, Microsoft name: EmfPlusPointR Object
  */
int U_PMF_POINTR_print(const char **contents, U_FLOAT *Xpos, U_FLOAT *Ypos,
                       const char *blimit, FILE *out, drawingStates *states) {
    U_FLOAT X, Y;
    int status = U_PMF_POINTR_get(contents, &X, &Y, blimit);
    *Xpos += X;
    *Ypos += Y;
    if (status) {
        verbose_printf("{%f,%f(%f,%f)}", *Xpos, *Ypos, X, Y);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_RECT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.38, Microsoft name: EmfPlusRect Object
  */
int U_PMF_RECT_print(const char **contents, const char *blimit, FILE *out,
                     drawingStates *states) {
    int16_t X, Y, Width, Height;
    int status = U_PMF_RECT_get(contents, &X, &Y, &Width, &Height, blimit);
    if (status) {
        verbose_printf("{UL{%d,%d},WH{%d,%d}}", X, Y, Width, Height);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_RECT Structure
  \return 1 on success, 0 on error
  \param  Rect   U_PMF_RECT structure
  EMF+ manual 2.2.2.39, Microsoft name: EmfPlusRectF Object
  */
int U_PMF_RECT_S_print(U_PMF_RECT *Rect, FILE *out, drawingStates *states) {
    verbose_printf("{UL{%d,%d},WH{%d,%d}}", Rect->X, Rect->Y, Rect->Width,
                   Rect->Height);
    return (1);
}

/**
  \brief Print data from a  U_PMF_RECTF object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.39, Microsoft name: EmfPlusRectF Object
  */
int U_PMF_RECTF_print(const char **contents, const char *blimit, FILE *out,
                      drawingStates *states) {
    U_FLOAT X, Y, Width, Height;
    int status = U_PMF_RECTF_get(contents, &X, &Y, &Width, &Height, blimit);
    if (status) {
        verbose_printf("{UL{%f,%f},WH{%f,%f}}", X, Y, Width, Height);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_RECTF Structure
  \return 1 on success, 0 on error
  \param  Rect   U_PMF_RECTF Structure
  EMF+ manual 2.2.2.39, Microsoft name: EmfPlusRectF Object
  */
int U_PMF_RECTF_S_print(U_PMF_RECTF *Rect, FILE *out, drawingStates *states) {
    verbose_printf("{UL{%f,%f},WH{%f,%f}}", Rect->X, Rect->Y, Rect->Width,
                   Rect->Height);
    return (1);
}

/**
  \brief Print data from a  U_PMF_REGIONNODE object
  \return size on success, 0 on error
  \param  contents   Record from which to print data
  \param  Level      Tree level.  This routine is recursive and could go down
  many levels. 1 is the top, >1 are child nodes.
  EMF+ manual 2.2.2.40, Microsoft name: EmfPlusRegionNode Object
  */
int U_PMF_REGIONNODE_print(const char *contents, int Level, const char *blimit,
                           FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    int len = 4; /* Type will always be present */
    uint32_t Type;
    const char *Data;
    int status = U_PMF_REGIONNODE_get(contents, &Type, &Data, blimit);
    if (status) {
        verbose_printf("\n   +  RegionNode(Level:%d) { Type:%X(", Level, Type);
        U_PMF_NODETYPE_print(Type, out, states);
        verbose_printf(")");
        if (Type >= U_RNDT_And && Type <= U_RNDT_Complement) {
            len += U_PMF_REGIONNODECHILDNODES_print(Data, Level + 1, blimit,
                                                    out, states);
        } else if (Type == U_RNDT_Rect) {
            len += sizeof(U_PMF_RECTF);
            (void)U_PMF_RECTF_print(&Data, blimit, out, states);
            verbose_printf("\n");
        } else if (Type == U_RNDT_Path) {
            len += U_PMF_REGIONNODEPATH_print(Data, blimit, out, states);
        }
        /* U_RNDT_Empty and  U_RNDT_Infinite do not change the length */
        else if (Type == U_RNDT_Empty) {
            verbose_printf(" Empty");
        } else if (Type == U_RNDT_Infinite) {
            verbose_printf(" Infinite");
        }
        verbose_printf("   +  RegionNode(Level:%d) }", Level);
        status = len; /* length of data + length of type */
    }
    verbose_printf("\n");
    return (status);
}

/**
  \brief Print data from a  U_PMF_REGIONNODECHILDNODES object
  \return size on success, 0 on error
  \param  contents   Record from which to print data
  \param  Level      Tree level.  This routine is recursive and could go down
  many levels. 1 is the top, >1 are child nodes.
  EMF+ manual 2.2.2.41, Microsoft name: EmfPlusRegionNodeChildNodes Object
  */
int U_PMF_REGIONNODECHILDNODES_print(const char *contents, int Level,
                                     const char *blimit, FILE *out,
                                     drawingStates *states) {
    uint32_t size, rsize;
    verbose_printf(" RegionNodeChildNodes:\n");
    verbose_printf("   +  RNCN__Left(Level:%d) {", Level);
    size = U_PMF_REGIONNODE_print(contents, Level, blimit, out, states);
    verbose_printf("   +  RNCN__Left(Level:%d) },\n", Level);
    if (size) {
        contents += size;
        verbose_printf("   +  RNCN_Right(Level:%d) {", Level);
        rsize = U_PMF_REGIONNODE_print(contents, Level, blimit, out, states);
        size += rsize;
        verbose_printf("   +  RNCN_Right(Level:%d) },\n", Level);
    }
    return (size);
}

/**
  \brief Print data from a  U_PMF_REGIONNODEPATH object
  \return Size of data on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.42, Microsoft name: EmfPlusRegionNodePath Object
  */
int U_PMF_REGIONNODEPATH_print(const char *contents, const char *blimit,
                               FILE *out, drawingStates *states) {
    int32_t Size;
    const char *Data;
    int status = U_PMF_REGIONNODEPATH_get(contents, &Size, &Data, blimit);
    if (status) {
        verbose_printf(" RegionNodePath: \n");
        (void)U_PMF_PATH_print(Data, blimit, out, states);
        status = Size + 4; /* data sizee + the 4 bytes encoding the size */
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_SOLIDBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.43, Microsoft name: EmfPlusSolidBrushData Object
  */
int U_PMF_SOLIDBRUSHDATA_print(const char *contents, const char *blimit,
                               FILE *out, drawingStates *states) {
    U_PMF_ARGB Color;
    int status = U_PMF_SOLIDBRUSHDATA_get(contents, &Color, blimit);
    if (status) {
        verbose_printf(" SolidBrushData: ");
        (void)U_PMF_ARGB_print((char *)&Color, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_STRINGFORMATDATA object
  \return 1 on success, 0 on error
  \param  contents      Record from which to print data
  \param  TabStopCount  Entries in TabStop array
  \param  RangeCount    Entries in CharRange array
  EMF+ manual 2.2.2.44, Microsoft name: EmfPlusStringFormatData Object
  */
int U_PMF_STRINGFORMATDATA_print(const char *contents, uint32_t TabStopCount,
                                 uint32_t RangeCount, const char *blimit,
                                 FILE *out, drawingStates *states) {
    const U_FLOAT *TabStops;
    const U_PMF_CHARACTERRANGE *CharRange;
    int status = U_PMF_STRINGFORMATDATA_get(contents, TabStopCount, RangeCount,
                                            &TabStops, &CharRange, blimit);
    if (status) {
        verbose_printf(" SFdata: TabStopCount:%u RangeCount:%u\n", TabStopCount,
                       RangeCount);

        verbose_printf("  Tabstops:");
        for (; TabStopCount; TabStopCount--, TabStops++) {
            verbose_printf(" %f", *TabStops);
        }
        verbose_printf("\n");

        verbose_printf("  CharRange:");
        for (; RangeCount; RangeCount--, CharRange++) {
            verbose_printf(" {%d,%d}", CharRange->First, CharRange->Length);
        }
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_TEXTUREBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.45, Microsoft name: EmfPlusTextureBrushData Object
  */
int U_PMF_TEXTUREBRUSHDATA_print(const char *contents, const char *blimit,
                                 FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    uint32_t Flags;
    int32_t WrapMode;
    const char *Data;
    int status =
        U_PMF_TEXTUREBRUSHDATA_get(contents, &Flags, &WrapMode, &Data, blimit);
    if (status) {
        verbose_printf("   +  TBdata: Flags:%X WrapMode:%d", Flags, WrapMode);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_TEXTUREBRUSHOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  HasImage   True if the record contains an image.

  EMF+ manual 2.2.2.46, Microsoft name: EmfPlusTextureBrushOptionalData Object
  */
int U_PMF_TEXTUREBRUSHOPTIONALDATA_print(const char *contents, int HasMatrix,
                                         int HasImage, const char *blimit,
                                         FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    U_PMF_TRANSFORMMATRIX Matrix;
    U_PMF_TRANSFORMMATRIX *pMatrix;
    const char *Image;
    if (HasMatrix) {
        pMatrix = &Matrix;
    } else {
        pMatrix = NULL;
    }
    int status = U_PMF_TEXTUREBRUSHOPTIONALDATA_get(contents, HasImage, pMatrix,
                                                    &Image, blimit);
    if (status) {
        verbose_printf("   +  TBOptdata: Image:%c", (HasImage ? 'Y' : 'N'));
        (void)U_PMF_TRANSFORMMATRIX2_print(&Matrix, out, states);
        (void)U_PMF_IMAGE_print(Image, blimit, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_TRANSFORMMATRIX object stored in file byte
  order.
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.47, Microsoft name: EmfPlusTransformMatrix Object
  */
int U_PMF_TRANSFORMMATRIX_print(const char *contents, const char *blimit,
                                FILE *out, drawingStates *states) {
    U_PMF_TRANSFORMMATRIX Tm;
    int status = U_PMF_TRANSFORMMATRIX_get(contents, &Tm, blimit);
    if (status) {
        U_PMF_TRANSFORMMATRIX2_print(&Tm, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_TRANSFORMMATRIX structure
  \return 1 on success, 0 on error
  \param  Tm  U_PMF_TRANSFORMMATRIX structure
  EMF+ manual 2.2.2.47, Microsoft name: EmfPlusTransformMatrix Object
  */
int U_PMF_TRANSFORMMATRIX2_print(U_PMF_TRANSFORMMATRIX *Tm, FILE *out,
                                 drawingStates *states) {
    if (Tm) {
        verbose_printf(" Matrix:{%f,%f,%f,%f,%f,%f}", Tm->m11, Tm->m12, Tm->m21,
                       Tm->m22, Tm->dX, Tm->dY);
    } else {
        verbose_printf(" Matrix:(None)");
    }
    return (1);
}

/**
  \brief Print data from a  U_PMF_ROTMATRIX object
  \return 1 on success, 0 on error
  \param  Rm   U_PMF_ROTMATRIX object
  NOT DOCUMENTED, like EMF+ manual 2.2.2.47, Microsoft name:
  EmfPlusTransformMatrix Object, but missing offset values
  */
int U_PMF_ROTMATRIX2_print(U_PMF_ROTMATRIX *Rm, FILE *out,
                           drawingStates *states) {
    verbose_printf(" Matrix:{%f,%f,%f,%f}", Rm->m11, Rm->m12, Rm->m21, Rm->m22);
    return (1);
}

/**
  \brief Print data from a  U_PMF_IE_BLUR object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.1, Microsoft name: BlurEffect Object
  */
int U_PMF_IE_BLUR_print(const char *contents, const char *blimit, FILE *out,
                        drawingStates *states) {
    U_FLOAT Radius;
    uint32_t ExpandEdge;
    int status = U_PMF_IE_BLUR_get(contents, &Radius, &ExpandEdge, blimit);
    if (status) {
        verbose_printf("BlurEffect Radius:%f ExpandEdge:%u\n", Radius,
                       ExpandEdge);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_BRIGHTNESSCONTRAST object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.2, Microsoft name: BrightnessContrastEffect Object
  */
int U_PMF_IE_BRIGHTNESSCONTRAST_print(const char *contents, const char *blimit,
                                      FILE *out, drawingStates *states) {
    int32_t Brightness, Contrast;
    int status = U_PMF_IE_BRIGHTNESSCONTRAST_get(contents, &Brightness,
                                                 &Contrast, blimit);
    if (status) {
        verbose_printf("BrightnessContrastEffect Brightness:%d Contrast:%d\n",
                       Brightness, Contrast);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORBALANCE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.3, Microsoft name: ColorBalanceEffect Object
  */
int U_PMF_IE_COLORBALANCE_print(const char *contents, const char *blimit,
                                FILE *out, drawingStates *states) {
    int32_t CyanRed, MagentaGreen, YellowBlue;
    int status = U_PMF_IE_COLORBALANCE_get(contents, &CyanRed, &MagentaGreen,
                                           &YellowBlue, blimit);
    if (status) {
        verbose_printf(
            "ColorBalanceEffect CyanRed:%d MagentaGreen:%d YellowBlue:%d\n",
            CyanRed, MagentaGreen, YellowBlue);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORCURVE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.4, Microsoft name: ColorCurveEffect Object
  */
int U_PMF_IE_COLORCURVE_print(const char *contents, const char *blimit,
                              FILE *out, drawingStates *states) {
    uint32_t Adjust, Channel;
    int32_t Intensity;
    int status = U_PMF_IE_COLORCURVE_get(contents, &Adjust, &Channel,
                                         &Intensity, blimit);
    if (status) {
        verbose_printf("ColorBalanceEffect Adjust:%u Channel:%u Intensity:%d\n",
                       Adjust, Channel, Intensity);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORLOOKUPTABLE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.5, Microsoft name: ColorLookupTableEffect Object
  */
int U_PMF_IE_COLORLOOKUPTABLE_print(const char *contents, const char *blimit,
                                    FILE *out, drawingStates *states) {
    const uint8_t *BLUT, *GLUT, *RLUT, *ALUT;
    int status = U_PMF_IE_COLORLOOKUPTABLE_get(contents, &BLUT, &GLUT, &RLUT,
                                               &ALUT, blimit);
    if (status) {
        verbose_printf("ColorLookupTableEffect \n");
        // U_PMF_UINT8_ARRAY_print(" BLUT:", BLUT, 256, "\n");
        // U_PMF_UINT8_ARRAY_print(" GLUT:", GLUT, 256, "\n");
        // U_PMF_UINT8_ARRAY_print(" RLUT:", RLUT, 256, "\n");
        // U_PMF_UINT8_ARRAY_print(" ALUT:", ALUT, 256, "\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORMATRIX object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.6, Microsoft name: ColorMatrixEffect Object
  */
int U_PMF_IE_COLORMATRIX_print(const char *contents, const char *blimit,
                               FILE *out, drawingStates *states) {
    U_PMF_IE_COLORMATRIX Matrix;
    int i, j;
    int status = U_PMF_IE_COLORMATRIX_get(contents, &Matrix, blimit);
    if (status) {
        verbose_printf("ColorMatrixEffect\n");
        for (i = 0; i < 5; i++) {
            verbose_printf(" {");
            for (j = 0; j < 4; j++) {
                verbose_printf("%f,", Matrix.M[i][j]);
            }
            verbose_printf("%f}", Matrix.M[i][j]);
        }
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_HUESATURATIONLIGHTNESS object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.7, Microsoft name: HueSaturationLightnessEffect Object
  */
int U_PMF_IE_HUESATURATIONLIGHTNESS_print(const char *contents,
                                          const char *blimit, FILE *out,
                                          drawingStates *states) {
    int32_t Hue, Saturation, Lightness;
    int status = U_PMF_IE_HUESATURATIONLIGHTNESS_get(
        contents, &Hue, &Saturation, &Lightness, blimit);
    if (status) {
        verbose_printf(
            "HueSaturationLightnessEffect Hue:%d Saturation:%d Lightness:%d\n",
            Hue, Saturation, Lightness);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_LEVELS object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.8, Microsoft name: LevelsEffect Object
  */
int U_PMF_IE_LEVELS_print(const char *contents, const char *blimit, FILE *out,
                          drawingStates *states) {
    int32_t Highlight, Midtone, Shadow;
    int status =
        U_PMF_IE_LEVELS_get(contents, &Highlight, &Midtone, &Shadow, blimit);
    if (status) {
        verbose_printf("LevelEffect Highlight:%d Midtone:%d Shadow:%d\n",
                       Highlight, Midtone, Shadow);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_REDEYECORRECTION object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.9, Microsoft name: RedEyeCorrectionEffect Object
  */
int U_PMF_IE_REDEYECORRECTION_print(const char *contents, const char *blimit,
                                    FILE *out, drawingStates *states) {
    int32_t Elements;
    U_RECTL *Rects;
    int status =
        U_PMF_IE_REDEYECORRECTION_get(contents, &Elements, &Rects, blimit);
    if (status) {
        verbose_printf("RedEyeCorrectionEffect Elements:%u", Elements);
        for (; Elements; Elements--, Rects++) {
            verbose_printf(" ");
            rectl_print(states, *Rects);
        }
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_SHARPEN object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.10, Microsoft name: SharpenEffect Object
  */
int U_PMF_IE_SHARPEN_print(const char *contents, const char *blimit, FILE *out,
                           drawingStates *states) {
    U_FLOAT Radius;
    int32_t Sharpen;
    int status = U_PMF_IE_SHARPEN_get(contents, &Radius, &Sharpen, blimit);
    if (status) {
        verbose_printf("SharpenEffect Radius:%f Sharpen:%u\n", Radius, Sharpen);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_TINT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.11, Microsoft name: TintEffect Object
  */
int U_PMF_IE_TINT_print(const char *contents, const char *blimit, FILE *out,
                        drawingStates *states) {
    int32_t Hue, Amount;
    int status = U_PMF_IE_TINT_get(contents, &Hue, &Amount, blimit);
    if (status) {
        verbose_printf("TintEffect Hue:%d Amount:%d\n", Hue, Amount);
    }
    return (status);
}

/* *****************************************************************************************
 */
/* EMF+ records, the EMF+ record header is printed separately, these print the
 * contents only */
/* *****************************************************************************************
 */

/**
  \brief Print data from a  U_PMR_OFFSETCLIP record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.1, Microsoft name: EmfPlusOffsetClip Record,  Index 0x35
  */
int U_PMR_OFFSETCLIP_print(const char *contents, FILE *out,
                           drawingStates *states) {
    FLAG_IGNORED;
    U_PMF_CMN_HDR Header;
    U_FLOAT dX, dY;
    int status = U_PMR_OFFSETCLIP_get(contents, &Header, &dX, &dY);
    if (status) {
        verbose_printf("   +  dx:%f dy:%f\n", dX, dY);
    } else {
        verbose_printf("   corrupt record\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_OFFSETCLIP record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.2, Microsoft name: EmfPlusResetClip Record, Index 0x31
  */
int U_PMR_RESETCLIP_print(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_IGNORED;
    return (U_PMR_NODATAREC_print(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_SETCLIPPATH record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.3, Microsoft name: EmfPlusSetClipPath Record, Index 0x33
  */
int U_PMR_SETCLIPPATH_print(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_IGNORED;
    int CMenum;
    uint32_t PathID;
    int status = U_PMR_SETCLIPPATH_get(contents, NULL, &PathID, &CMenum);
    if (status) {
        verbose_printf("   +  PathID:%u CMenum:%d\n", PathID, CMenum);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCLIPRECT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.4, Microsoft name: EmfPlusSetClipRect Record, Index 0x32
  */
int U_PMR_SETCLIPRECT_print(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_IGNORED;
    int CMenum;
    U_PMF_RECTF Rect;
    int status = U_PMR_SETCLIPRECT_get(contents, NULL, &CMenum, &Rect);
    if (status) {
        verbose_printf("   +  CMenum:%d(", CMenum);
        U_PMF_COMBINEMODEENUMERATION_print(CMenum, out, states);
        verbose_printf(") Rect:");
        U_PMF_RECTF_S_print(&Rect, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCLIPREGION record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.5, Microsoft name: EmfPlusSetClipRegion Record, Index 0x34
  */
int U_PMR_SETCLIPREGION_print(const char *contents, FILE *out,
                              drawingStates *states) {
    FLAG_IGNORED;
    int CMenum;
    uint32_t PathID;
    int status = U_PMR_SETCLIPREGION_get(contents, NULL, &PathID, &CMenum);
    if (status) {
        verbose_printf("   +  PathID:%u CMenum:%d(", PathID, CMenum);
        U_PMF_COMBINEMODEENUMERATION_print(CMenum, out, states);
        verbose_printf(")\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_COMMENT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.2.1, Microsoft name: EmfPlusComment Record, Index 0x03
  */
int U_PMR_COMMENT_print(const char *contents, FILE *out,
                        drawingStates *states) {
    FLAG_IGNORED;
    U_PMF_CMN_HDR Header;
    const char *Data;
    unsigned int i = 0;
    int status = U_PMR_COMMENT_get(contents, &Header, &Data);
    if (status) {
        /* try to print it, but only ASCII, bail on anything that is not ASCII
         */
        verbose_printf("   +  Data:");
        for (i = 0; i < Header.DataSize; i++, Data++) {
            if (!*Data)
                break;
            if (*(unsigned const char *)Data < 128) {
                verbose_printf("%c", *Data);
            } else {
                break;
            }
        }
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_ENDOFFILE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.3.1, Microsoft name: EmfPlusEndOfFile Record, Index 0x02
  */
int U_PMR_ENDOFFILE_print(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_IGNORED;
    return (U_PMR_NODATAREC_print(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_ENDOFFILE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.3.2, Microsoft name: EmfPlusGetDC Record, Index 0x04
  */
int U_PMR_GETDC_print(const char *contents, FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    return (U_PMR_NODATAREC_print(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_HEADER record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.3.3, Microsoft name: EmfPlusHeader Record, Index 0x01
  */
int U_PMR_HEADER_print(const char *contents, FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    U_PMF_GRAPHICSVERSION Version;
    int IsDual, IsVideo;
    uint32_t LogicalDpiX, LogicalDpiY;
    int status = U_PMR_HEADER_get(contents, NULL, &Version, &IsDual, &IsVideo,
                                  &LogicalDpiX, &LogicalDpiY);
    if (status) {
        /* try to print it, but only ASCII, bail on anything that is not ASCII
         */
        verbose_printf("   + ");
        (void)U_PMF_GRAPHICSVERSION_memsafe_print((char *)&Version, out,
                                                  states);
        verbose_printf(" IsDual:%c IsVideo:%d LogicalDpiX,y:{%u,%u}\n",
                       (IsDual ? 'Y' : 'N'), IsVideo, LogicalDpiX, LogicalDpiY);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_CLEAR record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.1, Microsoft name: EmfPlusClear Record, Index 0x09
  */
int U_PMR_CLEAR_print(const char *contents, FILE *out, drawingStates *states) {
    U_PMF_ARGB Color;
    int status = U_PMR_CLEAR_get(contents, NULL, &Color);
    if (status) {
        /* try to print it, but only ASCII, bail on anything that is not ASCII
         */
        verbose_printf("   +  Color:");
        (void)U_PMF_ARGB_print((char *)&Color, out, states);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWARC record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.2, Microsoft name: EmfPlusDrawArc Record, Index 0x12
  */
int U_PMR_DRAWARC_print(const char *contents, FILE *out,
                        drawingStates *states) {
    FLAG_IGNORED;
    uint32_t PenID;
    int ctype;
    U_FLOAT Start, Sweep;
    U_PMF_RECTF Rect;
    int status = U_PMR_DRAWARC_get(contents, NULL, &PenID, &ctype, &Start,
                                   &Sweep, &Rect);
    if (status) {
        verbose_printf("   +  PenID:%u ctype:%d Start:%f Sweep:%f Rect:", PenID,
                       ctype, Start, Sweep);
        (void)U_PMF_VARRECTF_S_print(&Rect, 1, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWBEZIERS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.3, Microsoft name: EmfPlusDrawBeziers Record, Index 0x19
  */
int U_PMR_DRAWBEZIERS_print(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_IGNORED;
    uint32_t PenID;
    int ctype, RelAbs;
    uint32_t Elements;
    U_PMF_POINTF *Points;
    int status = U_PMR_DRAWBEZIERS_get(contents, NULL, &PenID, &ctype, &RelAbs,
                                       &Elements, &Points);
    if (status) {
        verbose_printf("   +  PenIdx:%u ctype:%d RelAbs:%d Elements:%u\n",
                       PenID, ctype, RelAbs, Elements);
        U_PMF_VARPOINTF_S_print(Points, Elements, out, states);
        free(Points);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWCLOSEDCURVE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data

  Curve is a cardinal spline.
  References sent by MS support:
http://alvyray.com/Memos/CG/Pixar/spline77.pdf
http://msdn.microsoft.com/en-us/library/4cf6we5y(v=vs.110).aspx

EMF+ manual 2.3.4.4, Microsoft name: EmfPlusDrawClosedCurve Record, Index 0x17
*/
int U_PMR_DRAWCLOSEDCURVE_print(const char *contents, FILE *out,
                                drawingStates *states) {
    FLAG_IGNORED;
    uint32_t PenID;
    int ctype, RelAbs;
    U_FLOAT Tension;
    uint32_t Elements;
    U_PMF_POINTF *Points;
    int status = U_PMR_DRAWCLOSEDCURVE_get(
        contents, NULL, &PenID, &ctype, &RelAbs, &Tension, &Elements, &Points);
    if (status) {
        verbose_printf("   +  PenID:%u ctype:%d RelAbs:%d Tension:%f\n", PenID,
                       ctype, RelAbs, Tension);
        U_PMF_VARPOINTF_S_print(Points, Elements, out, states);
        free(Points);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWCURVE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data

  Curve is a cardinal spline, using doubled terminator points to generate curves
for the terminal segments.
  References sent by MS support:
http://alvyray.com/Memos/CG/Pixar/spline77.pdf
http://msdn.microsoft.com/en-us/library/4cf6we5y(v=vs.110).aspx

EMF+ manual 2.3.4.5, Microsoft name: EmfPlusDrawCurve Record, Index 0x18
*/
int U_PMR_DRAWCURVE_print(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_IGNORED;
    uint32_t PenID;
    int ctype;
    U_FLOAT Tension;
    uint32_t Offset, NSegs, Elements;
    U_PMF_POINTF *Points;
    int status = U_PMR_DRAWCURVE_get(contents, NULL, &PenID, &ctype, &Tension,
                                     &Offset, &NSegs, &Elements, &Points);
    if (status) {
        verbose_printf("   +  PenID:%u ctype:%d Tension:%f Offset:%u NSegs:%u "
                       "Elements:%u\n",
                       PenID, ctype, Tension, Offset, NSegs, Elements);
        U_PMF_VARPOINTF_S_print(Points, Elements, out, states);
        free(Points);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWDRIVERSTRING record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.6, Microsoft name: EmfPlusDrawDriverString Record, Index
  0x36
  */
int U_PMR_DRAWDRIVERSTRING_print(const char *contents, FILE *out,
                                 drawingStates *states) {
    FLAG_IGNORED;
    unsigned int i;
    uint32_t FontID;
    int btype;
    uint32_t BrushID, DSOFlags, HasMatrix, Elements;
    uint16_t *Glyphs;
    uint16_t *GlyphsIter;
    U_PMF_POINTF *Points;
    U_PMF_TRANSFORMMATRIX *Matrix;
    int status = U_PMR_DRAWDRIVERSTRING_get(
        contents, NULL, &FontID, &btype, &BrushID, &DSOFlags, &HasMatrix,
        &Elements, &Glyphs, &Points, &Matrix);
    if (status) {
        verbose_printf(
            "   +  FontID:%u btype:%d BrushID:%u DSOFlags:%X Elements:%u\n",
            FontID, btype, BrushID, DSOFlags, Elements);

        verbose_printf("   +  Glyphs:");
        if (*Glyphs) {
            for (GlyphsIter = Glyphs, i = 0; i < Elements; i++, GlyphsIter++) {
                verbose_printf(" %u", *GlyphsIter);
            }
            free(Glyphs);
        } else {
            verbose_printf("(none)");
        }
        verbose_printf("\n");

        verbose_printf("   +  Positions:\n");
        if (Points) {
            U_PMF_VARPOINTF_S_print(Points, Elements, out, states);
            free(Points);
        } else {
            verbose_printf("(none)\n");
        }

        if (Matrix) {
            verbose_printf("   + ");
            U_PMF_TRANSFORMMATRIX2_print(Matrix, out, states);
            free(Matrix);
            verbose_printf("\n");
        }
    } else {
        verbose_printf("   corrupt record\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWELLIPSE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.7, Microsoft name: EmfPlusDrawEllipse Record, Index 0x0F
  */
int U_PMR_DRAWELLIPSE_print(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_IGNORED;
    uint32_t PenID;
    int ctype;
    U_PMF_RECTF Rect;
    int status = U_PMR_DRAWELLIPSE_get(contents, NULL, &PenID, &ctype, &Rect);
    if (status) {
        verbose_printf("   +  PenID:%u ctype:%d", PenID, ctype);
        (void)U_PMF_VARRECTF_S_print(&Rect, 1, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWIMAGE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.8, Microsoft name: EmfPlusDrawImage Record, Index 0x1A
  */
int U_PMR_DRAWIMAGE_print(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_IGNORED;
    uint32_t ImgID, ImgAttrID;
    int ctype;
    int32_t SrcUnit;
    U_PMF_RECTF SrcRect;
    U_PMF_RECTF DstRect;
    int status = U_PMR_DRAWIMAGE_get(contents, NULL, &ImgID, &ctype, &ImgAttrID,
                                     &SrcUnit, &SrcRect, &DstRect);
    if (status) {
        verbose_printf(
            "   +  ImgID:%u ctype:%d ImgAttrID:%u SrcUnit:%d SrcRect:", ImgID,
            ctype, ImgAttrID, SrcUnit);
        (void)U_PMF_RECTF_S_print(&SrcRect, out, states);
        verbose_printf(" DstRect:");
        (void)U_PMF_RECTF_S_print(&DstRect, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWIMAGEPOINTS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.9, Microsoft name: EmfPlusDrawImagePoints Record, Index 0x1B
  */
int U_PMR_DRAWIMAGEPOINTS_print(const char *contents, FILE *out,
                                drawingStates *states) {
    FLAG_IGNORED;
    uint32_t ImgID, ImgAttrID;
    int ctype, etype, RelAbs;
    int32_t SrcUnit;
    U_PMF_RECTF SrcRect;
    uint32_t Elements;
    U_PMF_POINTF *Points;
    int status = U_PMR_DRAWIMAGEPOINTS_get(
        contents, NULL, &ImgID, &ctype, &etype, &RelAbs, &ImgAttrID, &SrcUnit,
        &SrcRect, &Elements, &Points);
    if (status) {
        verbose_printf("   +  ImgID:%u ctype:%d etype:%d ImgAttrID:%u "
                       "SrcUnit:%d Elements:%u SrcRect:",
                       ImgID, ctype, etype, ImgAttrID, SrcUnit, Elements);
        (void)U_PMF_RECTF_S_print(&SrcRect, out, states);
        verbose_printf("\n");
        U_PMF_VARPOINTF_S_print(Points, Elements, out, states);
        free(Points);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWLINES record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.10, Microsoft name: EmfPlusDrawLines Record, Index 0x0D
  */
int U_PMR_DRAWLINES_print(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_IGNORED;
    uint32_t PenIdx;
    int ctype, dtype, RelAbs;
    uint32_t Elements;
    U_PMF_POINTF *Points;
    int status = U_PMR_DRAWLINES_get(contents, NULL, &PenIdx, &ctype, &dtype,
                                     &RelAbs, &Elements, &Points);
    if (status) {
        verbose_printf("   +  PenIdx:%d ctype:%d dtype:%d RelAbs:%d\n", PenIdx,
                       ctype, dtype, RelAbs);
        U_PMF_VARPOINTF_S_print(Points, Elements, out, states);
        free(Points);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWPATH record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.11, Microsoft name: EmfPlusDrawPath Record, Index 0x15
  */
int U_PMR_DRAWPATH_print(const char *contents, FILE *out,
                         drawingStates *states) {
    FLAG_IGNORED;
    uint32_t PathIdx, PenIdx;
    int status = U_PMR_DRAWPATH_get(contents, NULL, &PathIdx, &PenIdx);
    if (status) {
        verbose_printf("   +  PathIdx:%d PenIdx:%d\n", PathIdx, PenIdx);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWPIE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.12, Microsoft name: EmfPlusDrawPie Record, Index 0x0D
  */
int U_PMR_DRAWPIE_print(const char *contents, FILE *out,
                        drawingStates *states) {
    FLAG_IGNORED;
    uint32_t PenID;
    int ctype;
    U_FLOAT Start, Sweep;
    U_PMF_RECTF Rect;
    int status = U_PMR_DRAWPIE_get(contents, NULL, &PenID, &ctype, &Start,
                                   &Sweep, &Rect);
    if (status) {
        verbose_printf("   +  PenID:%u ctype:%d Start:%f Sweep:%f Rect:", PenID,
                       ctype, Start, Sweep);
        (void)U_PMF_VARRECTF_S_print(&Rect, 1, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWRECTS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  \param blimit      One byte past the last record in memory.
  EMF+ manual 2.3.4.13, Microsoft name: EmfPlusDrawRects Record, Index 0x0B
  */
int U_PMR_DRAWRECTS_print(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_IGNORED;
    uint32_t PenID;
    int ctype;
    uint32_t Elements;
    U_PMF_RECTF *Rects = NULL;
    U_PMF_CMN_HDR hdr;
    int status =
        U_PMR_DRAWRECTS_get(contents, &hdr, &PenID, &ctype, &Elements, &Rects);
    if (status) {
        verbose_printf("   +  PenID:%u ctype:%d Elements:%u Rect:", PenID,
                       ctype, Elements);
        (void)U_PMF_VARRECTF_S_print(Rects, Elements, out, states);
        verbose_printf("\n");
    } else {
        verbose_printf("   corrupt record\n");
    }
    if (Rects)
        free(Rects);
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWSTRING record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.14, Microsoft name: EmfPlusDrawString Record, Index 0x1C
  */
int U_PMR_DRAWSTRING_print(const char *contents, FILE *out,
                           drawingStates *states) {
    FLAG_IGNORED;
    char *String8 = NULL;
    uint32_t FontID, BrushID, FormatID, Length;
    int btype;
    U_PMF_RECTF Rect;
    uint16_t *String16;
    int status = U_PMR_DRAWSTRING_get(contents, NULL, &FontID, &btype, &BrushID,
                                      &FormatID, &Length, &Rect, &String16);
    if (status) {
        verbose_printf(
            "   +  FontID:%u StringFormatID:%u btype:%d Length:%u Rect:",
            FontID, FormatID, btype, Length);
        (void)U_PMF_RECTF_S_print(&Rect, out, states);
        (void)U_PMF_VARBRUSHID_print(btype, BrushID, out, states);
        if (String16) {
            String8 = U_Utf16leToUtf8(String16, Length, NULL);
            free(String16);
            if (String8) {
                verbose_printf(" String(as_UTF8):<%s>\n", String8);
                free(String8);
            }
        } else {
            verbose_printf(" String(as_UTF8):(none)\n");
        }
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLCLOSEDCURVE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.15, Microsoft name: EmfPlusFillClosedCurve Record, Index
  0x16
  */
int U_PMR_FILLCLOSEDCURVE_print(const char *contents, FILE *out,
                                drawingStates *states) {
    FLAG_IGNORED;
    uint32_t BrushID;
    int btype, ctype, ftype, RelAbs;
    U_FLOAT Tension;
    uint32_t Elements;
    U_PMF_POINTF *Points;
    int status = U_PMR_FILLCLOSEDCURVE_get(contents, NULL, &btype, &ctype,
                                           &ftype, &RelAbs, &BrushID, &Tension,
                                           &Elements, &Points);
    if (status) {
        verbose_printf("   +  btype:%d ctype:%d ftype:%d RelAbs:%d Elements:%u",
                       btype, ctype, ftype, RelAbs, Elements);
        (void)U_PMF_VARBRUSHID_print(btype, BrushID, out, states);
        verbose_printf("\n");
        U_PMF_VARPOINTF_S_print(Points, Elements, out, states);
        free(Points);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLELLIPSE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.16, Microsoft name: EmfPlusFillEllipse Record, Index 0x0E
  */
int U_PMR_FILLELLIPSE_print(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_IGNORED;
    int btype, ctype;
    uint32_t BrushID;
    U_PMF_RECTF Rect;
    int status =
        U_PMR_FILLELLIPSE_get(contents, NULL, &btype, &ctype, &BrushID, &Rect);
    if (status) {
        verbose_printf("   +  btype:%d ctype:%d", btype, ctype);
        (void)U_PMF_VARBRUSHID_print(btype, BrushID, out, states);
        (void)U_PMF_VARRECTF_S_print(&Rect, 1, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLPATH record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.17, Microsoft name: EmfPlusFillPath Record, Index 0x14
  */
int U_PMR_FILLPATH_print(const char *contents, FILE *out,
                         drawingStates *states) {
    FLAG_IGNORED;
    int btype;
    uint32_t PathID, BrushID;
    int status = U_PMR_FILLPATH_get(contents, NULL, &PathID, &btype, &BrushID);
    if (status) {
        verbose_printf("   +  PathID:%u btype:%d", PathID, btype);
        (void)U_PMF_VARBRUSHID_print(btype, BrushID, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLPIE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.18, Microsoft name: EmfPlusFillPie Record, Index 0x10
  */
int U_PMR_FILLPIE_print(const char *contents, FILE *out,
                        drawingStates *states) {
    FLAG_IGNORED;
    int btype, ctype;
    U_FLOAT Start, Sweep;
    uint32_t BrushID;
    U_PMF_RECTF Rect;
    int status = U_PMR_FILLPIE_get(contents, NULL, &btype, &ctype, &BrushID,
                                   &Start, &Sweep, &Rect);
    if (status) {
        verbose_printf("   +  btype:%d ctype:%d", btype, ctype);
        (void)U_PMF_VARBRUSHID_print(btype, BrushID, out, states);
        (void)U_PMF_VARRECTF_S_print(&Rect, 1, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLPOLYGON record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.19, Microsoft name: EmfPlusFillPolygon Record, Index 0x0C
  */
int U_PMR_FILLPOLYGON_print(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_IGNORED;
    int btype, ctype, RelAbs;
    uint32_t BrushID, Elements;
    U_PMF_POINTF *Points;
    int status = U_PMR_FILLPOLYGON_get(contents, NULL, &btype, &ctype, &RelAbs,
                                       &BrushID, &Elements, &Points);
    if (status) {
        verbose_printf("   +  btype:%d ctype:%d RelAbs:%d Elements:%u", btype,
                       ctype, RelAbs, Elements);
        (void)U_PMF_VARBRUSHID_print(btype, BrushID, out, states);
        verbose_printf("\n");
        U_PMF_VARPOINTF_S_print(Points, Elements, out, states);
        free(Points);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLRECTS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  \param blimit      One byte past the last record in memory.
  EMF+ manual 2.3.4.20, Microsoft name: EmfPlusFillRects Record, Index 0x0A
  */
int U_PMR_FILLRECTS_print(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_IGNORED;
    int btype, ctype;
    uint32_t BrushID, Elements;
    U_PMF_RECTF *Rects;
    U_PMF_CMN_HDR hdr;
    int status = U_PMR_FILLRECTS_get(contents, &hdr, &btype, &ctype, &BrushID,
                                     &Elements, &Rects);
    if (status) {
        verbose_printf("   +  btype:%d ctype:%d Elements:%u", btype, ctype,
                       Elements);
        (void)U_PMF_VARBRUSHID_print(btype, BrushID, out, states);
        (void)U_PMF_VARRECTF_S_print(Rects, Elements, out, states);
        free(Rects);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLREGION record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.21, Microsoft name: EmfPlusFillRegion Record, Index 0x13
  */
int U_PMR_FILLREGION_print(const char *contents, FILE *out,
                           drawingStates *states) {
    FLAG_IGNORED;
    uint32_t RgnID, BrushID;
    int btype, ctype;
    int status =
        U_PMR_FILLREGION_get(contents, NULL, &RgnID, &btype, &ctype, &BrushID);
    if (status) {
        verbose_printf("   +  RgnID:%u btype:%d ctype:%d", RgnID, btype, ctype);
        (void)U_PMF_VARBRUSHID_print(btype, BrushID, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_OBJECT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  \param  blimit     One byte past the last record in memory.
  \param  ObjCont    Structure that holds accumulated object.
  \param  term       Flag used when an abnormal termination of a series of
  continuation records is encountered.
  EMF+ manual 2.3.5.1, Microsoft name: EmfPlusObject Record, Index 0x13
  */
int U_PMR_OBJECT_print(const char *contents, const char *blimit,
                       U_OBJ_ACCUM *ObjCont, int term, FILE *out,
                       drawingStates *states) {
    U_PMF_CMN_HDR Header;
    uint32_t ObjID;
    int otype, ntype;
    uint32_t TSize;
    const char *Data;
    int ttype, status;

    // int k; const char *cptr; for(cptr=contents, k=0; k<608;k++,cptr++){
    // verbose_printf("%3.3d %2.2X\n",k,*(uint8_t*)cptr); }; fflush(stdout);

    /* Continued records are a pain. Each contains the total size of the
       continued object in the first 4 bytes
       of data.  When the total hits that then then the record is complete, even
       though the continuation bit will
       still be set on that last record.  Check for this and then print the
       terminated continued series.
       */

    if (term) { /* mode for handling unexpected end of accumulated object */
        if (ObjCont->used == 0)
            return (0); /* no continued object pending */
        verbose_printf("   +  START Forced Termination of Accumulating object "
                       "Bytes:%u ObjID:%u DeclaredType:%d(",
                       ObjCont->used, ObjCont->Id, ObjCont->Type);
        U_PMF_OBJECTTYPEENUMERATION_print(ObjCont->Type, out, states);
        ttype = ObjCont->Type & 0x3F;
        verbose_printf(")\n");
        status = 1;
    } else {
        status = U_PMR_OBJECT_get(contents, &Header, &ObjID, &otype, &ntype,
                                  &TSize, &Data);
        /* In a corrupt EMF+ file we might hit a new type of record before all
           the continuation records
           expected have been found.  If that happens terminate whatever we have
           accumulated so far, and then go on
           to emit the new (unexpected) record. */
        if (IS_MEM_UNSAFE(contents, Header.Size, blimit))
            return (0);
        if (!status) {
            verbose_printf("   corrupt record\n");
            return (status);
        }
        if ((ObjCont->used > 0) &&
            (U_OA_append(ObjCont, NULL, 0, otype, ObjID) < 0)) {
            U_PMR_OBJECT_print(contents, blimit, ObjCont, 1, out, states);
        }
        verbose_printf("   +  ObjID:%u ObjType:%d(", ObjID, otype);
        U_PMF_OBJECTTYPEENUMERATION_print(otype, out, states);
        verbose_printf(") ntype:%d", ntype);
        verbose_printf(" ContinueD:%c", (ObjCont->used ? 'Y' : 'N'));
        verbose_printf(" ContinueB:%c", (ntype ? 'Y' : 'N'));
        if (ntype) {
            if (checkOutOfEMF(states,
                              (uintptr_t)((uintptr_t)Data +
                                         (uintptr_t)Header.DataSize - 4)) ||
                ((int64_t)Header.DataSize - 4) < 0) {
                status = 0;
                verbose_printf("   corrupt record\n");
            } else {
                U_OA_append(ObjCont, Data, Header.DataSize - 4, otype,
                            ObjID); // The total byte count is not added to
                                    // the object
                verbose_printf(" TotalSize:%u", TSize);
                verbose_printf(" Accumulated:%u", ObjCont->used);
            }
        } else {
            if (checkOutOfEMF(states,
                              (uintptr_t)Data + (uintptr_t)Header.DataSize)) {
                status = 0;
                verbose_printf("   corrupt record\n");
            } else {
                U_OA_append(
                    ObjCont, Data, Header.DataSize, otype,
                    ObjID); // The total byte count is not added to the object
            }
        }
        verbose_printf("\n");
        if (ntype && ObjCont->used < TSize)
            return (status);
        /* preceding terminates any continued series for >= accumulated bytes */
        ttype = otype;
    }
    if (status) {
        blimit =
            ObjCont->accum +
            ObjCont->used; /* more restrictive blimit, just to end of object */
        switch (ttype) {
        case U_OT_Brush:
            (void)U_PMF_BRUSH_print(ObjCont->accum, blimit, out, states);
            break;
        case U_OT_Pen:
            (void)U_PMF_PEN_print(ObjCont->accum, blimit, out, states);
            break;
        case U_OT_Path:
            (void)U_PMF_PATH_print(ObjCont->accum, blimit, out, states);
            break;
        case U_OT_Region:
            (void)U_PMF_REGION_print(ObjCont->accum, blimit, out, states);
            break;
        case U_OT_Image:
            (void)U_PMF_IMAGE_print(ObjCont->accum, blimit, out, states);
            break;
        case U_OT_Font:
            (void)U_PMF_FONT_print(ObjCont->accum, blimit, out, states);
            break;
        case U_OT_StringFormat:
            (void)U_PMF_STRINGFORMAT_print(ObjCont->accum, blimit, out, states);
            break;
        case U_OT_ImageAttributes:
            (void)U_PMF_IMAGEATTRIBUTES_print(ObjCont->accum, blimit, out,
                                              states);
            break;
        case U_OT_CustomLineCap:
            (void)U_PMF_CUSTOMLINECAP_print(ObjCont->accum, "", blimit, out,
                                            states);
            break;
        case U_OT_Invalid:
        default:
            verbose_printf("INVALID OBJECT TYPE!!!!\n");
            break;
        }
        U_OA_clear(ObjCont);
    }
    if (term)
        verbose_printf(
            "   +  END   Forced Termination of Accumulating object\n");
    return (status);
}

/**
  \brief Print data from a  U_PMR_SERIALIZABLEOBJECT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.5.2, Microsoft name: EmfPlusSerializableObject Record, Index
  0x38
  */
int U_PMR_SERIALIZABLEOBJECT_print(const char *contents, FILE *out,
                                   drawingStates *states) {
    FLAG_IGNORED;
    uint8_t GUID[16];
    uint32_t Size;
    const char *Data;
    char *string = NULL;
    int iee;
    int status =
        U_PMR_SERIALIZABLEOBJECT_get(contents, NULL, &GUID[0], &Size, &Data);
    if (status) {
        string = U_PMF_CURLYGUID_set(&GUID[0]);
        if (string) {
            verbose_printf("   +  GUID:%s Size:%u", string, Size);
            iee = U_PMF_KNOWNCURLYGUID_set(
                string); /* overwrites string with short text form */
            verbose_printf("\n   +  Effect:");
            free(string);
            switch (iee) {
            case U_IEE_Unknown:
                verbose_printf("(undefined)\n");
                break;
            case U_IEE_BlurEffectGuid:
                U_PMF_IE_BLUR_print(Data, Data + sizeof(U_PMF_IE_BLUR), out,
                                    states);
                break;
            case U_IEE_BrightnessContrastEffectGuid:
                U_PMF_IE_BRIGHTNESSCONTRAST_print(
                    Data, Data + sizeof(U_PMF_IE_BRIGHTNESSCONTRAST), out,
                    states);
                break;
            case U_IEE_ColorBalanceEffectGuid:
                U_PMF_IE_COLORBALANCE_print(
                    Data, Data + sizeof(U_PMF_IE_COLORBALANCE), out, states);
                break;
            case U_IEE_ColorCurveEffectGuid:
                U_PMF_IE_COLORCURVE_print(
                    Data, Data + sizeof(U_PMF_IE_COLORCURVE), out, states);
                break;
            case U_IEE_ColorLookupTableEffectGuid:
                U_PMF_IE_COLORLOOKUPTABLE_print(
                    Data, Data + sizeof(U_PMF_IE_COLORLOOKUPTABLE), out,
                    states);
                break;
            case U_IEE_ColorMatrixEffectGuid:
                U_PMF_IE_COLORMATRIX_print(
                    Data, Data + sizeof(U_PMF_IE_COLORMATRIX), out, states);
                break;
            case U_IEE_HueSaturationLightnessEffectGuid:
                U_PMF_IE_HUESATURATIONLIGHTNESS_print(
                    Data, Data + sizeof(U_PMF_IE_HUESATURATIONLIGHTNESS), out,
                    states);
                break;
            case U_IEE_LevelsEffectGuid:
                U_PMF_IE_LEVELS_print(Data, Data + sizeof(U_PMF_IE_LEVELS), out,
                                      states);
                break;
            case U_IEE_RedEyeCorrectionEffectGuid:
                U_PMF_IE_REDEYECORRECTION_print(
                    Data, Data + sizeof(U_PMF_IE_REDEYECORRECTION), out,
                    states);
                break;
            case U_IEE_SharpenEffectGuid:
                U_PMF_IE_SHARPEN_print(Data, Data + sizeof(U_PMF_IE_SHARPEN),
                                       out, states);
                break;
            case U_IEE_TintEffectGuid:
                U_PMF_IE_TINT_print(Data, Data + sizeof(U_PMF_IE_TINT), out,
                                    states);
                break;
            }
        } else {
            verbose_printf("   +  GUID:ERROR Size:%u\n", Size);
        }
    } else {
        verbose_printf("   corrupt record\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETANTIALIASMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.1, Microsoft name: EmfPlusSetAntiAliasMode Record, Index
  0x1E
  */
int U_PMR_SETANTIALIASMODE_print(const char *contents, FILE *out,
                                 drawingStates *states) {
    FLAG_IGNORED;
    int SMenum, aatype;
    int status = U_PMR_SETANTIALIASMODE_get(contents, NULL, &SMenum, &aatype);
    if (status) {
        verbose_printf("   +  SMenum:%d AntiAlias:%c\n", SMenum,
                       (aatype ? 'Y' : 'N'));
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCOMPOSITINGMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.2, Microsoft name: EmfPlusSetCompositingMode Record, Index
  0x23
  */
int U_PMR_SETCOMPOSITINGMODE_print(const char *contents, FILE *out,
                                   drawingStates *states) {
    FLAG_IGNORED;
    int CMenum;
    int status = U_PMR_SETCOMPOSITINGMODE_get(contents, NULL, &CMenum);
    if (status) {
        verbose_printf("   +  CMenum:%d\n", CMenum);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCOMPOSITINGQUALITY record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.3, Microsoft name: EmfPlusSetCompositingQuality Record,
  Index 0x24
  */
int U_PMR_SETCOMPOSITINGQUALITY_print(const char *contents, FILE *out,
                                      drawingStates *states) {
    FLAG_IGNORED;
    int CQenum;
    int status = U_PMR_SETCOMPOSITINGQUALITY_get(contents, NULL, &CQenum);
    if (status) {
        verbose_printf("   +  CQenum:%d\n", CQenum);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETINTERPOLATIONMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.4, Microsoft name: EmfPlusSetInterpolationMode Record, Index
  0x21
  */
int U_PMR_SETINTERPOLATIONMODE_print(const char *contents, FILE *out,
                                     drawingStates *states) {
    FLAG_IGNORED;
    int IMenum;
    int status = U_PMR_SETINTERPOLATIONMODE_get(contents, NULL, &IMenum);
    if (status) {
        verbose_printf("   +  IMenum:%d\n", IMenum);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETPIXELOFFSETMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.5, Microsoft name: EmfPlusSetPixelOffsetMode Record, Index
  0x22
  */
int U_PMR_SETPIXELOFFSETMODE_print(const char *contents, FILE *out,
                                   drawingStates *states) {
    FLAG_IGNORED;
    int POMenum;
    int status = U_PMR_SETPIXELOFFSETMODE_get(contents, NULL, &POMenum);
    if (status) {
        verbose_printf("   +  POMenum:%d\n", POMenum);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETRENDERINGORIGIN record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.6, Microsoft name: EmfPlusSetRenderingOrigin Record, Index
  0x1D
  */
int U_PMR_SETRENDERINGORIGIN_print(const char *contents, FILE *out,
                                   drawingStates *states) {
    FLAG_IGNORED;
    int32_t X, Y;
    int status = U_PMR_SETRENDERINGORIGIN_get(contents, NULL, &X, &Y);
    if (status) {
        verbose_printf("   +  X:%d Y:%d\n", X, Y);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTEXTCONTRAST record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.7, Microsoft name: EmfPlusSetTextContrast Record, Index 0x20
  */
int U_PMR_SETTEXTCONTRAST_print(const char *contents, FILE *out,
                                drawingStates *states) {
    FLAG_IGNORED;
    int GC;
    int status = U_PMR_SETTEXTCONTRAST_get(contents, NULL, &GC);
    if (status) {
        verbose_printf("   +  GC:%d\n", GC);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTEXTRENDERINGHINT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.8, Microsoft name: EmfPlusSetTextRenderingHint Record, Index
  0x1F
  */
int U_PMR_SETTEXTRENDERINGHINT_print(const char *contents, FILE *out,
                                     drawingStates *states) {
    FLAG_IGNORED;
    int TRHenum;
    int status = U_PMR_SETTEXTRENDERINGHINT_get(contents, NULL, &TRHenum);
    if (status) {
        verbose_printf("   +  TRHenum:%d\n", TRHenum);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_BEGINCONTAINER record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.1, Microsoft name: EmfPlusBeginContainer Record, Index 0x27
  */
int U_PMR_BEGINCONTAINER_print(const char *contents, FILE *out,
                               drawingStates *states) {
    FLAG_IGNORED;
    int UTenum;
    U_PMF_RECTF DstRect, SrcRect;
    uint32_t StackID;
    int status = U_PMR_BEGINCONTAINER_get(contents, NULL, &UTenum, &DstRect,
                                          &SrcRect, &StackID);
    if (status) {
        verbose_printf("   +  UTenum:%d", UTenum);
        verbose_printf(" DstRect:");
        (void)U_PMF_RECTF_S_print(&DstRect, out, states);
        verbose_printf(" SrcRect:");
        (void)U_PMF_RECTF_S_print(&SrcRect, out, states);
        verbose_printf(" StackID:%u\n", StackID);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_BEGINCONTAINERNOPARAMS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.2, Microsoft name: EmfPlusBeginContainerNoParams Record,
  Index 0x28
  */
int U_PMR_BEGINCONTAINERNOPARAMS_print(const char *contents, FILE *out,
                                       drawingStates *states) {
    FLAG_IGNORED;
    uint32_t StackID;
    int status = U_PMR_BEGINCONTAINERNOPARAMS_get(contents, NULL, &StackID);
    if (status) {
        verbose_printf("   +  StackID:%u\n", StackID);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_ENDCONTAINER record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.3, Microsoft name: EmfPlusEndContainer Record, Index 0x29
  */
int U_PMR_ENDCONTAINER_print(const char *contents, FILE *out,
                             drawingStates *states) {
    FLAG_IGNORED;
    uint32_t StackID;
    int status = U_PMR_ENDCONTAINER_get(contents, NULL, &StackID);
    if (status) {
        verbose_printf("   +  StackID:%u\n", StackID);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_RESTORE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.4, Microsoft name: EmfPlusRestore Record, Index 0x26
  */
int U_PMR_RESTORE_print(const char *contents, FILE *out,
                        drawingStates *states) {
    FLAG_IGNORED;
    uint32_t StackID;
    int status = U_PMR_RESTORE_get(contents, NULL, &StackID);
    if (status) {
        verbose_printf("   +  StackID:%u\n", StackID);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SAVE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.5, Microsoft name: EmfPlusSave Record, Index 0x25
  */
int U_PMR_SAVE_print(const char *contents, FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    uint32_t StackID;
    int status = U_PMR_SAVE_get(contents, NULL, &StackID);
    if (status) {
        verbose_printf("   +  StackID:%u\n", StackID);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTSCLIP record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.8.1, Microsoft name: EmfPlusSetTSClip Record, Index 0x3A
  */
int U_PMR_SETTSCLIP_print(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_IGNORED;
    int ctype;
    uint32_t Elements;
    U_PMF_RECTF *Rects;
    int status = U_PMR_SETTSCLIP_get(contents, NULL, &ctype, &Elements, &Rects);
    if (status) {
        verbose_printf("   +  ctype:%d Elements:%u", ctype, Elements);
        (void)U_PMF_VARRECTF_S_print(Rects, Elements, out, states);
        free(Rects);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTSGRAPHICS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.8.2, Microsoft name: EmfPlusSetTSGraphics Record, Index 0x39
  */
int U_PMR_SETTSGRAPHICS_print(const char *contents, FILE *out,
                              drawingStates *states) {
    FLAG_IGNORED;
    int vgatype, pptype;
    uint8_t AntiAliasMode, TextRenderHint, CompositingMode, CompositingQuality,
        FilterType, PixelOffset;
    int16_t RenderOriginX, RenderOriginY;
    uint16_t TextContrast;
    U_PMF_TRANSFORMMATRIX WorldToDevice;
    const char *Data;
    int status = U_PMR_SETTSGRAPHICS_get(
        contents, NULL, &vgatype, &pptype, &AntiAliasMode, &TextRenderHint,
        &CompositingMode, &CompositingQuality, &RenderOriginX, &RenderOriginY,
        &TextContrast, &FilterType, &PixelOffset, &WorldToDevice, &Data);
    if (status) {
        const char *blimit = contents + status;
        verbose_printf("   +  vgatype:%d pptype:%u", vgatype, pptype);
        verbose_printf(" AntiAliasMode:%u TextRenderHint:%u CompositingMode:%u "
                       "CompositingQuality:%u",
                       AntiAliasMode, TextRenderHint, CompositingMode,
                       CompositingQuality);
        verbose_printf(" RenderOriginX:%d RenderOriginY:%d", RenderOriginX,
                       RenderOriginY);
        verbose_printf(" TextContrast:%u", TextContrast);
        verbose_printf(" WorldToDevice:");
        U_PMF_TRANSFORMMATRIX2_print(&WorldToDevice, out, states);
        if (pptype && !U_PMF_PALETTE_print(Data, blimit, out, states))
            return (0);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_MULTIPLYWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.1, Microsoft name: EmfPlusMultiplyWorldTransform Record,
  Index 0x2C
  */
int U_PMR_MULTIPLYWORLDTRANSFORM_print(const char *contents, FILE *out,
                                       drawingStates *states) {
    FLAG_IGNORED;
    int xmtype;
    U_PMF_TRANSFORMMATRIX Matrix;
    int status =
        U_PMR_MULTIPLYWORLDTRANSFORM_get(contents, NULL, &xmtype, &Matrix);
    if (status) {
        verbose_printf("   +  xmtype:%d Multiply:%s", xmtype,
                       (xmtype ? "Post" : "Pre"));
        U_PMF_TRANSFORMMATRIX2_print(&Matrix, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_RESETWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.2, Microsoft name: EmfPlusResetWorldTransform Record, Index
  0x2B
  */
int U_PMR_RESETWORLDTRANSFORM_print(const char *contents, FILE *out,
                                    drawingStates *states) {
    FLAG_IGNORED;
    return (U_PMR_NODATAREC_print(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_ROTATEWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.3, Microsoft name: EmfPlusRotateWorldTransform Record, Index
  0x2F
  */
int U_PMR_ROTATEWORLDTRANSFORM_print(const char *contents, FILE *out,
                                     drawingStates *states) {
    FLAG_IGNORED;
    int xmtype;
    U_FLOAT Angle;
    int status =
        U_PMR_ROTATEWORLDTRANSFORM_get(contents, NULL, &xmtype, &Angle);
    if (status) {
        verbose_printf("   +  xmtype:%d Multiply:%s Angle:%f\n", xmtype,
                       (xmtype ? "Post" : "Pre"), Angle);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SCALEWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.4, Microsoft name: EmfPlusScaleWorldTransform Record, Index
  0x2E
  */
int U_PMR_SCALEWORLDTRANSFORM_print(const char *contents, FILE *out,
                                    drawingStates *states) {
    FLAG_IGNORED;
    int xmtype;
    U_FLOAT Sx, Sy;
    int status =
        U_PMR_SCALEWORLDTRANSFORM_get(contents, NULL, &xmtype, &Sx, &Sy);
    if (status) {
        verbose_printf("   +  xmtype:%d Multiply:%s ScaleX:%f ScaleY:%f\n",
                       xmtype, (xmtype ? "Post" : "Pre"), Sx, Sy);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETPAGETRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.5, Microsoft name: EmfPlusSetPageTransform Record, Index
  0x30
  */
int U_PMR_SETPAGETRANSFORM_print(const char *contents, FILE *out,
                                 drawingStates *states) {
    FLAG_IGNORED;
    int UTenum;
    U_FLOAT Scale;
    int status = U_PMR_SETPAGETRANSFORM_get(contents, NULL, &UTenum, &Scale);
    if (status) {
        verbose_printf("   +  UTenum:%d Scale:%f\n", UTenum, Scale);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.6, Microsoft name: EmfPlusSetWorldTransform Record, Index
  0x2A
  */
int U_PMR_SETWORLDTRANSFORM_print(const char *contents, FILE *out,
                                  drawingStates *states) {
    FLAG_IGNORED;
    U_PMF_TRANSFORMMATRIX Matrix;
    int status = U_PMR_SETWORLDTRANSFORM_get(contents, NULL, &Matrix);
    if (status) {
        verbose_printf("   + ");
        U_PMF_TRANSFORMMATRIX2_print(&Matrix, out, states);
        verbose_printf("\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_TRANSLATEWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.7, Microsoft name: EmfPlusTranslateWorldTransform Record,
  Index 0x2D
  */
int U_PMR_TRANSLATEWORLDTRANSFORM_print(const char *contents, FILE *out,
                                        drawingStates *states) {
    FLAG_IGNORED;
    int xmtype;
    U_FLOAT Dx, Dy;
    int status =
        U_PMR_TRANSLATEWORLDTRANSFORM_get(contents, NULL, &xmtype, &Dx, &Dy);
    if (status) {
        verbose_printf(
            "   +  xmtype:%d Multiply:%s TranslateX:%f TranlateY:%f\n", xmtype,
            (xmtype ? "Post" : "Pre"), Dx, Dy);
    } else {
        verbose_printf("   corrupt record\n");
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_STROKEFILLPATH record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  */
int U_PMR_STROKEFILLPATH_print(const char *contents, FILE *out,
                               drawingStates *states) {
    FLAG_IGNORED;
    return (U_PMR_NODATAREC_print(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_MULTIFORMATSTART record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  EMF+ manual mentioned in 2.1.1.1, reserved, not otherwise documented,
  Microsoft name: EmfPlusMultiFormatStart Record, Index 0x05
  */
int U_PMR_MULTIFORMATSTART_print(const char *contents, FILE *out,
                                 drawingStates *states) {
    FLAG_IGNORED;
    return (U_PMR_NODATAREC_print(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_MULTIFORMATSECTION record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  EMF+ manual mentioned in 2.1.1.1, reserved, not otherwise documented,
  Microsoft name: EmfPlusMultiFormatSection Record, Index 0x06
  */
int U_PMR_MULTIFORMATSECTION_print(const char *contents, FILE *out,
                                   drawingStates *states) {
    FLAG_IGNORED;
    return (U_PMR_NODATAREC_print(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_MULTIFORMATEND record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  EMF+ manual mentioned in 2.1.1.1, reserved, not otherwise documented,
  Microsoft name: EmfPlusMultiFormatEnd Record, Index 0x06
  */
int U_PMR_MULTIFORMATEND_print(const char *contents, FILE *out,
                               drawingStates *states) {
    FLAG_IGNORED;
    return (U_PMR_NODATAREC_print(contents, out, states));
}

#ifdef __cplusplus
}
#endif

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
