/**
  @file upmf_draw.c

  @brief Functions for printing EMF records
  */

/*
File:      upmf_draw.c
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
#include "pmf2svg.h"
#include <stdio.h>

//! \cond

#define UNUSED(x)                                                              \
    (void)(x) //! Please ignore - Doxygen simply insisted on including this

/*
   this function is not visible in the API.  Print "data" for one of the many
   records that has none.
   */
int U_PMR_NODATAREC_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    return (1);
}

/*
   this function is not visible in the API.  Common routine used by many
   functions that draw points.
   */
void U_PMF_VARPOINTS_draw(const char **contents, int Flags, uint32_t Elements,
                          FILE *out, drawingStates *states) {
    return;
}

/*
   this function is not visible in the API.  Common routine used by many
   functions that draw points.
   */
void U_PMF_VARPOINTF_S_draw(U_PMF_POINTF *Points, uint32_t Elements, FILE *out,
                            drawingStates *states) {
    return;
}

/*
   this function is not visible in the API.  Common routine used by many
   functions that draw rectangles.
   */
int U_PMF_VARRECTF_S_draw(U_PMF_RECTF *Rects, uint32_t Elements, FILE *out,
                          drawingStates *states) {
    return (1);
}

/*
   this function is not visible in the API.  Common routine used by many
   functions.
   */
int U_PMF_VARBRUSHID_draw(int btype, uint32_t BrushID, FILE *out,
                          drawingStates *states) {
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
int U_pmf_onerec_draw(const char *contents, const char *blimit, int recnum,
                      int off, FILE *out, drawingStates *states) {
    int status;
    static U_OBJ_ACCUM ObjCont = {
        NULL, 0, 0, 0,
        0}; /* for keeping track of object continuation. These may
               be split across multiple EMF Comment records */
    U_PMF_CMN_HDR Header;
    const char *contemp = contents;
    if (!U_PMF_CMN_HDR_get(&contemp, &Header)) {
        return (-1);
    }

    int type = Header.Type & U_PMR_TYPE_MASK; /* strip the U_PMR_RECFLAG bit,
                                                 leaving the indexable part */
    if (type < U_PMR_MIN || type > U_PMR_MAX)
        return (-1); /* unknown EMF+ record type */
    status =
        U_PMF_CMN_HDR_draw(Header, recnum, off, out, states); /* EMF+ part */

    /* Buggy EMF+ can set the continue bit and then do something else. In that
       case, force out the pending
       Object.  Side effect - clears the pending object. */
    if ((type != U_PMR_OBJECT) && (ObjCont.used > 0)) {
        U_PMR_OBJECT_draw(contents, blimit, &ObjCont, 1, out, states);
    }

    switch (type) {
    case (U_PMR_HEADER):
        U_PMR_HEADER_draw(contents, out, states);
        break;
    case (U_PMR_ENDOFFILE):
        U_PMR_ENDOFFILE_draw(contents, out, states);
        U_OA_release(&ObjCont);
        break;
    case (U_PMR_COMMENT):
        U_PMR_COMMENT_draw(contents, out, states);
        break;
    case (U_PMR_GETDC):
        U_PMR_GETDC_draw(contents, out, states);
        break;
    case (U_PMR_MULTIFORMATSTART):
        U_PMR_MULTIFORMATSTART_draw(contents, out, states);
        break;
    case (U_PMR_MULTIFORMATSECTION):
        U_PMR_MULTIFORMATSECTION_draw(contents, out, states);
        break;
    case (U_PMR_MULTIFORMATEND):
        U_PMR_MULTIFORMATEND_draw(contents, out, states);
        break;
    case (U_PMR_OBJECT):
        U_PMR_OBJECT_draw(contents, blimit, &ObjCont, 0, out, states);
        break;
    case (U_PMR_CLEAR):
        U_PMR_CLEAR_draw(contents, out, states);
        break;
    case (U_PMR_FILLRECTS):
        U_PMR_FILLRECTS_draw(contents, blimit, out, states);
        break;
    case (U_PMR_DRAWRECTS):
        U_PMR_DRAWRECTS_draw(contents, blimit, out, states);
        break;
    case (U_PMR_FILLPOLYGON):
        U_PMR_FILLPOLYGON_draw(contents, out, states);
        break;
    case (U_PMR_DRAWLINES):
        U_PMR_DRAWLINES_draw(contents, out, states);
        break;
    case (U_PMR_FILLELLIPSE):
        U_PMR_FILLELLIPSE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWELLIPSE):
        U_PMR_DRAWELLIPSE_draw(contents, out, states);
        break;
    case (U_PMR_FILLPIE):
        U_PMR_FILLPIE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWPIE):
        U_PMR_DRAWPIE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWARC):
        U_PMR_DRAWARC_draw(contents, out, states);
        break;
    case (U_PMR_FILLREGION):
        U_PMR_FILLREGION_draw(contents, out, states);
        break;
    case (U_PMR_FILLPATH):
        U_PMR_FILLPATH_draw(contents, out, states);
        break;
    case (U_PMR_DRAWPATH):
        U_PMR_DRAWPATH_draw(contents, out, states);
        break;
    case (U_PMR_FILLCLOSEDCURVE):
        U_PMR_FILLCLOSEDCURVE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWCLOSEDCURVE):
        U_PMR_DRAWCLOSEDCURVE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWCURVE):
        U_PMR_DRAWCURVE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWBEZIERS):
        U_PMR_DRAWBEZIERS_draw(contents, out, states);
        break;
    case (U_PMR_DRAWIMAGE):
        U_PMR_DRAWIMAGE_draw(contents, out, states);
        break;
    case (U_PMR_DRAWIMAGEPOINTS):
        U_PMR_DRAWIMAGEPOINTS_draw(contents, out, states);
        break;
    case (U_PMR_DRAWSTRING):
        U_PMR_DRAWSTRING_draw(contents, out, states);
        break;
    case (U_PMR_SETRENDERINGORIGIN):
        U_PMR_SETRENDERINGORIGIN_draw(contents, out, states);
        break;
    case (U_PMR_SETANTIALIASMODE):
        U_PMR_SETANTIALIASMODE_draw(contents, out, states);
        break;
    case (U_PMR_SETTEXTRENDERINGHINT):
        U_PMR_SETTEXTRENDERINGHINT_draw(contents, out, states);
        break;
    case (U_PMR_SETTEXTCONTRAST):
        U_PMR_SETTEXTCONTRAST_draw(contents, out, states);
        break;
    case (U_PMR_SETINTERPOLATIONMODE):
        U_PMR_SETINTERPOLATIONMODE_draw(contents, out, states);
        break;
    case (U_PMR_SETPIXELOFFSETMODE):
        U_PMR_SETPIXELOFFSETMODE_draw(contents, out, states);
        break;
    case (U_PMR_SETCOMPOSITINGMODE):
        U_PMR_SETCOMPOSITINGMODE_draw(contents, out, states);
        break;
    case (U_PMR_SETCOMPOSITINGQUALITY):
        U_PMR_SETCOMPOSITINGQUALITY_draw(contents, out, states);
        break;
    case (U_PMR_SAVE):
        U_PMR_SAVE_draw(contents, out, states);
        break;
    case (U_PMR_RESTORE):
        U_PMR_RESTORE_draw(contents, out, states);
        break;
    case (U_PMR_BEGINCONTAINER):
        U_PMR_BEGINCONTAINER_draw(contents, out, states);
        break;
    case (U_PMR_BEGINCONTAINERNOPARAMS):
        U_PMR_BEGINCONTAINERNOPARAMS_draw(contents, out, states);
        break;
    case (U_PMR_ENDCONTAINER):
        U_PMR_ENDCONTAINER_draw(contents, out, states);
        break;
    case (U_PMR_SETWORLDTRANSFORM):
        U_PMR_SETWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_RESETWORLDTRANSFORM):
        U_PMR_RESETWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_MULTIPLYWORLDTRANSFORM):
        U_PMR_MULTIPLYWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_TRANSLATEWORLDTRANSFORM):
        U_PMR_TRANSLATEWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_SCALEWORLDTRANSFORM):
        U_PMR_SCALEWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_ROTATEWORLDTRANSFORM):
        U_PMR_ROTATEWORLDTRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_SETPAGETRANSFORM):
        U_PMR_SETPAGETRANSFORM_draw(contents, out, states);
        break;
    case (U_PMR_RESETCLIP):
        U_PMR_RESETCLIP_draw(contents, out, states);
        break;
    case (U_PMR_SETCLIPRECT):
        U_PMR_SETCLIPRECT_draw(contents, out, states);
        break;
    case (U_PMR_SETCLIPPATH):
        U_PMR_SETCLIPPATH_draw(contents, out, states);
        break;
    case (U_PMR_SETCLIPREGION):
        U_PMR_SETCLIPREGION_draw(contents, out, states);
        break;
    case (U_PMR_OFFSETCLIP):
        U_PMR_OFFSETCLIP_draw(contents, out, states);
        break;
    case (U_PMR_DRAWDRIVERSTRING):
        U_PMR_DRAWDRIVERSTRING_draw(contents, out, states);
        break;
    case (U_PMR_STROKEFILLPATH):
        U_PMR_STROKEFILLPATH_draw(contents, out, states);
        break;
    case (U_PMR_SERIALIZABLEOBJECT):
        U_PMR_SERIALIZABLEOBJECT_draw(contents, out, states);
        break;
    case (U_PMR_SETTSGRAPHICS):
        U_PMR_SETTSGRAPHICS_draw(contents, out, states);
        break;
    case (U_PMR_SETTSCLIP):
        U_PMR_SETTSCLIP_draw(contents, out, states);
        break;
    }
    if (states->Error) {
        U_OA_release(&ObjCont);
    }
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
int U_PMF_CMN_HDR_draw(U_PMF_CMN_HDR Header, int precnum, int off, FILE *out,
                       drawingStates *states) {
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
int U_PMF_UINT8_ARRAY_draw(const char *Start, const uint8_t *Array,
                           int Elements, char *End, FILE *out,
                           drawingStates *states) {
    return (1);
}

/**
  \brief Print value of an BrushType Enumeration
  \returns record 1 on sucess, 0 on error
  \param otype    Value to print.
  EMF+ manual 2.1.1.3, Microsoft name: BrushType Enumeration
  */
int U_PMF_BRUSHTYPEENUMERATION_draw(int otype, FILE *out,
                                    drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print value of an BrushType Enumeration
  \returns record 1 on sucess, 0 on error
  \param otype    Value to print.
  EMF+ manual 2.1.1.4, Microsoft name: BrushType Enumeration
  */
int U_PMF_COMBINEMODEENUMERATION_draw(int otype, FILE *out,
                                      drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print value of a HatchStyle Enumeration
  \returns record 1 on sucess, 0 on error
  \param hstype    Value to print.
  EMF+ manual 2.1.1.13, Microsoft name: HatchStyle Enumeration
  */
int U_PMF_HATCHSTYLEENUMERATION_draw(int hstype, FILE *out,
                                     drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print value of an ObjectType Enumeration
  \returns record 1 on sucess, 0 on error
  \param otype    Value to print.
  EMF+ manual 2.1.1.22, Microsoft name: ObjectType Enumeration
  */
int U_PMF_OBJECTTYPEENUMERATION_draw(int otype, FILE *out,
                                     drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print value of a  U_PMF_PATHPOINTTYPE_ENUM object
  \return 1
  \param  Type   Value to print
  EMF+ manual 2.1.1.23, Microsoft name: PathPointType Enumeration
  */
int U_PMF_PATHPOINTTYPE_ENUM_draw(int Type, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a PixelFormat Enumeration value
  \return 1 always
  \param  pfe   A PixelFormat Enumeration value
  EMF+ manual 2.1.1.25, Microsoft name: PixelFormat Enumeration (U_PF_*)
  */
int U_PMF_PX_FMT_ENUM_draw(int pfe, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print as text a RegionNodeDataType Enumeration
  \return 1
  \param  Type   RegionNodeDataType Enumeration
  EMF+ manual 2.1.1.27, Microsoft name: RegionNodeDataType Enumeration
  (U_RNDT_*)
  */
int U_PMF_NODETYPE_draw(int Type, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_BRUSH object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.1, Microsoft name: EmfPlusBrush Object
  */
int U_PMF_BRUSH_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAP object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  Which      A string which is either "Start" or "End".
  EMF+ manual 2.2.1.2, Microsoft name: EmfPlusCustomLineCap Object
  */
int U_PMF_CUSTOMLINECAP_draw(const char *contents, const char *Which, FILE *out,
                             drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_FONT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.3, Microsoft name: EmfPlusFont Object
  */
int U_PMF_FONT_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IMAGE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.4, Microsoft name: EmfPlusImage Object
  */
int U_PMF_IMAGE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IMAGEATTRIBUTES object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.5, Microsoft name: EmfPlusImageAttributes Object
  */
int U_PMF_IMAGEATTRIBUTES_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATH object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.6, Microsoft name: EmfPlusPath Object
  */
int U_PMF_PATH_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PEN object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.7, Microsoft name: EmfPlusPen Object
  */
int U_PMF_PEN_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_REGION object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.8, Microsoft name: EmfPlusRegion Object
  */
int U_PMF_REGION_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_STRINGFORMAT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.1.9, Microsoft name: EmfPlusStringFormat Object
  */
int U_PMF_STRINGFORMAT_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_ARGB object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.1, Microsoft name: EmfPlusARGB Object
  */
int U_PMF_ARGB_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BITMAP object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.2, Microsoft name: EmfPlusBitmap Object
  */
int U_PMF_BITMAP_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BITMAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.3, Microsoft name: EmfPlusBitmapData Object
  */
int U_PMF_BITMAPDATA_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BLENDCOLORS object
  \return size in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.4, Microsoft name: EmfPlusBlendColors Object
  */
int U_PMF_BLENDCOLORS_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BLENDFACTORS object
  \return size on success, 0 on error
  \param  type       Type of BlendFactors, usually H or V
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.5, Microsoft name: EmfPlusBlendFactors Object
  */
int U_PMF_BLENDFACTORS_draw(const char *contents, const char *type, FILE *out,
                            drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BOUNDARYPATHDATA object
  \return size on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.6, Microsoft name: EmfPlusBoundaryPathData Object
  */
int U_PMF_BOUNDARYPATHDATA_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_BOUNDARYPOINTDATA object
  \return size on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.7, Microsoft name: EmfPlusBoundaryPointData Object
  */
int U_PMF_BOUNDARYPOINTDATA_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CHARACTERRANGE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.8, Microsoft name: EmfPlusCharacterRange Object
  */
int U_PMF_CHARACTERRANGE_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_DASHEDLINEDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.9, Microsoft name: EmfPlusCompoundLineData Object
  */
int U_PMF_COMPOUNDLINEDATA_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
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
int U_PMF_COMPRESSEDIMAGE_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMENDCAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.11, Microsoft name: EmfPlusCustomEndCapData Object
  */
int U_PMF_CUSTOMENDCAPDATA_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAPARROWDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.12, Microsoft name: EmfPlusCustomLineCapArrowData Object
  */
int U_PMF_CUSTOMLINECAPARROWDATA_draw(const char *contents, FILE *out,
                                      drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.13, Microsoft name: EmfPlusCustomLineCapData Object
  */
int U_PMF_CUSTOMLINECAPDATA_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMLINECAPOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  Flags      CustomLineCapData Flags

  EMF+ manual 2.2.2.14, Microsoft name: EmfPlusCustomLineCapOptionalData Object
  */
int U_PMF_CUSTOMLINECAPOPTIONALDATA_draw(const char *contents, uint32_t Flags,
                                         FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_CUSTOMSTARTCAPDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.15, Microsoft name: EmfPlusCustomStartCapData Object
  */
int U_PMF_CUSTOMSTARTCAPDATA_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_DASHEDLINEDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.16, Microsoft name: EmfPlusDashedLineData Object
  */
int U_PMF_DASHEDLINEDATA_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_FILLPATHOBJ object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.17, Microsoft name: EmfPlusFillPath Object
  */
int U_PMF_FILLPATHOBJ_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_FOCUSSCALEDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.18, Microsoft name: EmfPlusFocusScaleData Object
  */
int U_PMF_FOCUSSCALEDATA_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_GRAPHICSVERSION_draw object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.19, Microsoft name: EmfPlusGraphicsVersion Object
  */
int U_PMF_GRAPHICSVERSION_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_HATCHBRUSHDATA_draw object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.20, Microsoft name: EmfPlusHatchBrushData Object
  */
int U_PMF_HATCHBRUSHDATA_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_LANGUAGEIDENTIFIER object
  \return 1 on success, 0 on error
  \param  LId   Record from which to print data
  EMF+ manual 2.2.2.23, Microsoft name: EmfPlusLanguageIdentifier Object
  */
int U_PMF_LANGUAGEIDENTIFIER_draw(U_PMF_LANGUAGEIDENTIFIER LId, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_LINEARGRADIENTBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.24, Microsoft name: EmfPlusLinearGradientBrushData Object
  */
int U_PMF_LINEARGRADIENTBRUSHDATA_draw(const char *contents, FILE *out,
                                       drawingStates *states) {
    int status = 1;
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
int U_PMF_LINEARGRADIENTBRUSHOPTIONALDATA_draw(const char *contents, int BDFlag,
                                               FILE *out,
                                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_LINEPATH object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.26, Microsoft name: EmfPlusLinePath Object
  */
int U_PMF_LINEPATH_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_METAFILE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.27, Microsoft name: EmfPlusMetafile Object
  */
int U_PMF_METAFILE_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PALETTE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.28, Microsoft name: EmfPlusPalette Object
  */
int U_PMF_PALETTE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATHGRADIENTBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.29, Microsoft name: EmfPlusPathGradientBrushData Object
  */
int U_PMF_PATHGRADIENTBRUSHDATA_draw(const char *contents, FILE *out,
                                     drawingStates *states) {
    int status = 1;
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
int U_PMF_PATHGRADIENTBRUSHOPTIONALDATA_draw(const char *contents, int BDFlag,
                                             FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_PATHPOINTTYPE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.31, Microsoft name: EmfPlusPathPointType Object
  */
int U_PMF_PATHPOINTTYPE_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATHPOINTTYPERLE object
  \return Number of elements in the run, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.32, Microsoft name: EmfPlusPathPointTypeRLE Object
  */
int U_PMF_PATHPOINTTYPERLE_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_PATHPOINTTYPERLE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.33, Microsoft name: EmfPlusPenData Object
  */
int U_PMF_PENDATA_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
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
int U_PMF_PENOPTIONALDATA_draw(const char *contents, int Flags, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}
/**
  \brief Print data from a  U_PMF_POINT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.35, Microsoft name: EmfPlusPoint Object
  */
int U_PMF_POINT_draw(const char **contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_POINT Structure
  \return 1 on success, 0 on error
  \param  Point   U_PMF_POINT Structure to print
  EMF+ manual 2.2.2.35, Microsoft name: EmfPlusPoint Object
  */
int U_PMF_POINT_S_draw(U_PMF_POINT *Point, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_POINTF object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.36, Microsoft name: EmfPlusPointF Object
  */
int U_PMF_POINTF_draw(const char **contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_POINTF Structure
  \return 1 on success, 0 on error
  \param  Point   U_PMF_POINTF Structure to print
  EMF+ manual 2.2.2.36, Microsoft name: EmfPlusPointF Object
  */
int U_PMF_POINTF_S_draw(U_PMF_POINTF *Point, FILE *out, drawingStates *states) {
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
int U_PMF_POINTR_draw(const char **contents, U_FLOAT *Xpos, U_FLOAT *Ypos,
                      FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_RECT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.38, Microsoft name: EmfPlusRect Object
  */
int U_PMF_RECT_draw(const char **contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_RECT Structure
  \return 1 on success, 0 on error
  \param  Rect   U_PMF_RECT structure
  EMF+ manual 2.2.2.39, Microsoft name: EmfPlusRectF Object
  */
int U_PMF_RECT_S_draw(U_PMF_RECT *Rect, FILE *out, drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_RECTF object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.39, Microsoft name: EmfPlusRectF Object
  */
int U_PMF_RECTF_draw(const char **contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_RECTF Structure
  \return 1 on success, 0 on error
  \param  Rect   U_PMF_RECTF Structure
  EMF+ manual 2.2.2.39, Microsoft name: EmfPlusRectF Object
  */
int U_PMF_RECTF_S_draw(U_PMF_RECTF *Rect, FILE *out, drawingStates *states) {
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
int U_PMF_REGIONNODE_draw(const char *contents, int Level, FILE *out,
                          drawingStates *states) {
    int status = 1;
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
int U_PMF_REGIONNODECHILDNODES_draw(const char *contents, int Level, FILE *out,
                                    drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_REGIONNODEPATH object
  \return Size of data on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.42, Microsoft name: EmfPlusRegionNodePath Object
  */
int U_PMF_REGIONNODEPATH_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_SOLIDBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.43, Microsoft name: EmfPlusSolidBrushData Object
  */
int U_PMF_SOLIDBRUSHDATA_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
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
int U_PMF_STRINGFORMATDATA_draw(const char *contents, uint32_t TabStopCount,
                                uint32_t RangeCount, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_TEXTUREBRUSHDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.45, Microsoft name: EmfPlusTextureBrushData Object
  */
int U_PMF_TEXTUREBRUSHDATA_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_TEXTUREBRUSHOPTIONALDATA object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  \param  HasImage   True if the record contains an image.

  EMF+ manual 2.2.2.46, Microsoft name: EmfPlusTextureBrushOptionalData Object
  */
int U_PMF_TEXTUREBRUSHOPTIONALDATA_draw(const char *contents, int HasImage,
                                        FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_TRANSFORMMATRIX object stored in file byte
  order.
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.2.47, Microsoft name: EmfPlusTransformMatrix Object
  */
int U_PMF_TRANSFORMMATRIX_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_TRANSFORMMATRIX structure
  \return 1 on success, 0 on error
  \param  Tm  U_PMF_TRANSFORMMATRIX structure
  EMF+ manual 2.2.2.47, Microsoft name: EmfPlusTransformMatrix Object
  */
int U_PMF_TRANSFORMMATRIX2_draw(U_PMF_TRANSFORMMATRIX *Tm, FILE *out,
                                drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_ROTMATRIX object
  \return 1 on success, 0 on error
  \param  Rm   U_PMF_ROTMATRIX object
  NOT DOCUMENTED, like EMF+ manual 2.2.2.47, Microsoft name:
  EmfPlusTransformMatrix Object, but missing offset values
  */
int U_PMF_ROTMATRIX2_draw(U_PMF_ROTMATRIX *Rm, FILE *out,
                          drawingStates *states) {
    return (1);
}

/**
  \brief Print data from a  U_PMF_IE_BLUR object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.1, Microsoft name: BlurEffect Object
  */
int U_PMF_IE_BLUR_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_BRIGHTNESSCONTRAST object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.2, Microsoft name: BrightnessContrastEffect Object
  */
int U_PMF_IE_BRIGHTNESSCONTRAST_draw(const char *contents, FILE *out,
                                     drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORBALANCE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.3, Microsoft name: ColorBalanceEffect Object
  */
int U_PMF_IE_COLORBALANCE_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORCURVE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.4, Microsoft name: ColorCurveEffect Object
  */
int U_PMF_IE_COLORCURVE_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORLOOKUPTABLE object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.5, Microsoft name: ColorLookupTableEffect Object
  */
int U_PMF_IE_COLORLOOKUPTABLE_draw(const char *contents, FILE *out,
                                   drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_COLORMATRIX object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.6, Microsoft name: ColorMatrixEffect Object
  */
int U_PMF_IE_COLORMATRIX_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_HUESATURATIONLIGHTNESS object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.7, Microsoft name: HueSaturationLightnessEffect Object
  */
int U_PMF_IE_HUESATURATIONLIGHTNESS_draw(const char *contents, FILE *out,
                                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_LEVELS object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.8, Microsoft name: LevelsEffect Object
  */
int U_PMF_IE_LEVELS_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_REDEYECORRECTION object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.9, Microsoft name: RedEyeCorrectionEffect Object
  */
int U_PMF_IE_REDEYECORRECTION_draw(const char *contents, FILE *out,
                                   drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_SHARPEN object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.10, Microsoft name: SharpenEffect Object
  */
int U_PMF_IE_SHARPEN_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMF_IE_TINT object
  \return 1 on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.2.3.11, Microsoft name: TintEffect Object
  */
int U_PMF_IE_TINT_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
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
int U_PMR_OFFSETCLIP_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_OFFSETCLIP record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.2, Microsoft name: EmfPlusResetClip Record, Index 0x31
  */
int U_PMR_RESETCLIP_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_SETCLIPPATH record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.3, Microsoft name: EmfPlusSetClipPath Record, Index 0x33
  */
int U_PMR_SETCLIPPATH_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCLIPRECT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.4, Microsoft name: EmfPlusSetClipRect Record, Index 0x32
  */
int U_PMR_SETCLIPRECT_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCLIPREGION record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.1.5, Microsoft name: EmfPlusSetClipRegion Record, Index 0x34
  */
int U_PMR_SETCLIPREGION_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_COMMENT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.2.1, Microsoft name: EmfPlusComment Record, Index 0x03
  */
int U_PMR_COMMENT_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_ENDOFFILE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.3.1, Microsoft name: EmfPlusEndOfFile Record, Index 0x02
  */
int U_PMR_ENDOFFILE_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_ENDOFFILE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.3.2, Microsoft name: EmfPlusGetDC Record, Index 0x04
  */
int U_PMR_GETDC_draw(const char *contents, FILE *out, drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_HEADER record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.3.3, Microsoft name: EmfPlusHeader Record, Index 0x01
  */
int U_PMR_HEADER_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_CLEAR record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.1, Microsoft name: EmfPlusClear Record, Index 0x09
  */
int U_PMR_CLEAR_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWARC record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.2, Microsoft name: EmfPlusDrawArc Record, Index 0x12
  */
int U_PMR_DRAWARC_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWBEZIERS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.3, Microsoft name: EmfPlusDrawBeziers Record, Index 0x19
  */
int U_PMR_DRAWBEZIERS_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
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
int U_PMR_DRAWCLOSEDCURVE_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
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
int U_PMR_DRAWCURVE_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWDRIVERSTRING record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.6, Microsoft name: EmfPlusDrawDriverString Record, Index
  0x36
  */
int U_PMR_DRAWDRIVERSTRING_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWELLIPSE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.7, Microsoft name: EmfPlusDrawEllipse Record, Index 0x0F
  */
int U_PMR_DRAWELLIPSE_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWIMAGE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.8, Microsoft name: EmfPlusDrawImage Record, Index 0x1A
  */
int U_PMR_DRAWIMAGE_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWIMAGEPOINTS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.9, Microsoft name: EmfPlusDrawImagePoints Record, Index 0x1B
  */
int U_PMR_DRAWIMAGEPOINTS_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWLINES record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.10, Microsoft name: EmfPlusDrawLines Record, Index 0x0D
  */
int U_PMR_DRAWLINES_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWPATH record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.11, Microsoft name: EmfPlusDrawPath Record, Index 0x15
  */
int U_PMR_DRAWPATH_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWPIE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.12, Microsoft name: EmfPlusDrawPie Record, Index 0x0D
  */
int U_PMR_DRAWPIE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWRECTS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  \param blimit      One byte past the last record in memory.
  EMF+ manual 2.3.4.13, Microsoft name: EmfPlusDrawRects Record, Index 0x0B
  */
int U_PMR_DRAWRECTS_draw(const char *contents, const char *blimit, FILE *out,
                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_DRAWSTRING record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.14, Microsoft name: EmfPlusDrawString Record, Index 0x1C
  */
int U_PMR_DRAWSTRING_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLCLOSEDCURVE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.15, Microsoft name: EmfPlusFillClosedCurve Record, Index
  0x16
  */
int U_PMR_FILLCLOSEDCURVE_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLELLIPSE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.16, Microsoft name: EmfPlusFillEllipse Record, Index 0x0E
  */
int U_PMR_FILLELLIPSE_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLPATH record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.17, Microsoft name: EmfPlusFillPath Record, Index 0x14
  */
int U_PMR_FILLPATH_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLPIE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.18, Microsoft name: EmfPlusFillPie Record, Index 0x10
  */
int U_PMR_FILLPIE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLPOLYGON record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.19, Microsoft name: EmfPlusFillPolygon Record, Index 0x0C
  */
int U_PMR_FILLPOLYGON_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLRECTS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  \param blimit      One byte past the last record in memory.
  EMF+ manual 2.3.4.20, Microsoft name: EmfPlusFillRects Record, Index 0x0A
  */
int U_PMR_FILLRECTS_draw(const char *contents, const char *blimit, FILE *out,
                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_FILLREGION record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.4.21, Microsoft name: EmfPlusFillRegion Record, Index 0x13
  */
int U_PMR_FILLREGION_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    int status = 1;
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
int U_PMR_OBJECT_draw(const char *contents, const char *blimit,
                      U_OBJ_ACCUM *ObjCont, int term, FILE *out,
                      drawingStates *states) {
    U_PMF_CMN_HDR Header;
    uint32_t ObjID;
    int otype, ntype;
    uint32_t TSize;
    const char *Data;
    int ttype, status;

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
        U_PMF_OBJECTTYPEENUMERATION_draw(ObjCont->Type, out, states);
        ttype = ObjCont->Type & 0x3F;
        status = 1;
    } else {
        status = U_PMR_OBJECT_get(contents, &Header, &ObjID, &otype, &ntype,
                                  &TSize, &Data);
        /* In a corrupt EMF+ file we might hit a new type of record before all
           the continuation records
           expected have been found.  If that happens terminate whatever we have
           accumulated so far, and then go on
           to emit the new (unexpected) record. */
        if (contents + Header.Size >= blimit)
            return (0);
        if (!status)
            return (status);
        if ((ObjCont->used > 0) &&
            (U_OA_append(ObjCont, NULL, 0, otype, ObjID) < 0)) {
            U_PMR_OBJECT_draw(contents, blimit, ObjCont, 1, out, states);
        }
        U_PMF_OBJECTTYPEENUMERATION_draw(otype, out, states);
        if (ntype) {
            if (checkOutOfEMF(states,
                              (uintptr_t)((uintptr_t)Data +
                                         (uintptr_t)Header.DataSize - 4)) ||
                ((int64_t)Header.DataSize - 4) < 0) {
                status = 0;
            } else {
                U_OA_append(
                    ObjCont, Data, Header.DataSize - 4, otype,
                    ObjID); // The total byte count is not added to the object
            }
        } else {
            if (checkOutOfEMF(states,
                              (uintptr_t)Data + (uintptr_t)Header.DataSize)) {
                status = 0;
            } else {
                U_OA_append(
                    ObjCont, Data, Header.DataSize, otype,
                    ObjID); // The total byte count is not added to the object
            }
        }
        if (ntype && ObjCont->used < TSize)
            return (status);
        /* preceding terminates any continued series for >= accumulated bytes */
        ttype = otype;
    }
    if (status) {
        switch (ttype) {
        case U_OT_Brush:
            (void)U_PMF_BRUSH_draw(ObjCont->accum, out, states);
            break;
        case U_OT_Pen:
            (void)U_PMF_PEN_draw(ObjCont->accum, out, states);
            break;
        case U_OT_Path:
            (void)U_PMF_PATH_draw(ObjCont->accum, out, states);
            break;
        case U_OT_Region:
            (void)U_PMF_REGION_draw(ObjCont->accum, out, states);
            break;
        case U_OT_Image:
            (void)U_PMF_IMAGE_draw(ObjCont->accum, out, states);
            break;
        case U_OT_Font:
            (void)U_PMF_FONT_draw(ObjCont->accum, out, states);
            break;
        case U_OT_StringFormat:
            (void)U_PMF_STRINGFORMAT_draw(ObjCont->accum, out, states);
            break;
        case U_OT_ImageAttributes:
            (void)U_PMF_IMAGEATTRIBUTES_draw(ObjCont->accum, out, states);
            break;
        case U_OT_CustomLineCap:
            (void)U_PMF_CUSTOMLINECAP_draw(ObjCont->accum, "", out, states);
            break;
        case U_OT_Invalid:
        default:
            break;
        }
        U_OA_clear(ObjCont);
    }
    return (status);
}

/**
  \brief Print data from a  U_PMR_SERIALIZABLEOBJECT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.5.2, Microsoft name: EmfPlusSerializableObject Record, Index
  0x38
  */
int U_PMR_SERIALIZABLEOBJECT_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETANTIALIASMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.1, Microsoft name: EmfPlusSetAntiAliasMode Record, Index
  0x1E
  */
int U_PMR_SETANTIALIASMODE_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCOMPOSITINGMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.2, Microsoft name: EmfPlusSetCompositingMode Record, Index
  0x23
  */
int U_PMR_SETCOMPOSITINGMODE_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETCOMPOSITINGQUALITY record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.3, Microsoft name: EmfPlusSetCompositingQuality Record,
  Index 0x24
  */
int U_PMR_SETCOMPOSITINGQUALITY_draw(const char *contents, FILE *out,
                                     drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETINTERPOLATIONMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.4, Microsoft name: EmfPlusSetInterpolationMode Record, Index
  0x21
  */
int U_PMR_SETINTERPOLATIONMODE_draw(const char *contents, FILE *out,
                                    drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETPIXELOFFSETMODE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.5, Microsoft name: EmfPlusSetPixelOffsetMode Record, Index
  0x22
  */
int U_PMR_SETPIXELOFFSETMODE_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETRENDERINGORIGIN record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.6, Microsoft name: EmfPlusSetRenderingOrigin Record, Index
  0x1D
  */
int U_PMR_SETRENDERINGORIGIN_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTEXTCONTRAST record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.7, Microsoft name: EmfPlusSetTextContrast Record, Index 0x20
  */
int U_PMR_SETTEXTCONTRAST_draw(const char *contents, FILE *out,
                               drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTEXTRENDERINGHINT record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.6.8, Microsoft name: EmfPlusSetTextRenderingHint Record, Index
  0x1F
  */
int U_PMR_SETTEXTRENDERINGHINT_draw(const char *contents, FILE *out,
                                    drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_BEGINCONTAINER record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.1, Microsoft name: EmfPlusBeginContainer Record, Index 0x27
  */
int U_PMR_BEGINCONTAINER_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_BEGINCONTAINERNOPARAMS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.2, Microsoft name: EmfPlusBeginContainerNoParams Record,
  Index 0x28
  */
int U_PMR_BEGINCONTAINERNOPARAMS_draw(const char *contents, FILE *out,
                                      drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_ENDCONTAINER record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.3, Microsoft name: EmfPlusEndContainer Record, Index 0x29
  */
int U_PMR_ENDCONTAINER_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_RESTORE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.4, Microsoft name: EmfPlusRestore Record, Index 0x26
  */
int U_PMR_RESTORE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SAVE record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.7.5, Microsoft name: EmfPlusSave Record, Index 0x25
  */
int U_PMR_SAVE_draw(const char *contents, FILE *out, drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTSCLIP record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.8.1, Microsoft name: EmfPlusSetTSClip Record, Index 0x3A
  */
int U_PMR_SETTSCLIP_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETTSGRAPHICS record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.8.2, Microsoft name: EmfPlusSetTSGraphics Record, Index 0x39
  */
int U_PMR_SETTSGRAPHICS_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_MULTIPLYWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.1, Microsoft name: EmfPlusMultiplyWorldTransform Record,
  Index 0x2C
  */
int U_PMR_MULTIPLYWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                      drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_RESETWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.2, Microsoft name: EmfPlusResetWorldTransform Record, Index
  0x2B
  */
int U_PMR_RESETWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                   drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_ROTATEWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.3, Microsoft name: EmfPlusRotateWorldTransform Record, Index
  0x2F
  */
int U_PMR_ROTATEWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                    drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SCALEWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.4, Microsoft name: EmfPlusScaleWorldTransform Record, Index
  0x2E
  */
int U_PMR_SCALEWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                   drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETPAGETRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.5, Microsoft name: EmfPlusSetPageTransform Record, Index
  0x30
  */
int U_PMR_SETPAGETRANSFORM_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_SETWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.6, Microsoft name: EmfPlusSetWorldTransform Record, Index
  0x2A
  */
int U_PMR_SETWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_TRANSLATEWORLDTRANSFORM record
  \return size of record in bytes on success, 0 on error
  \param  contents   Record from which to print data
  EMF+ manual 2.3.9.7, Microsoft name: EmfPlusTranslateWorldTransform Record,
  Index 0x2D
  */
int U_PMR_TRANSLATEWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                       drawingStates *states) {
    int status = 1;
    return (status);
}

/**
  \brief Print data from a  U_PMR_STROKEFILLPATH record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  */
int U_PMR_STROKEFILLPATH_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_MULTIFORMATSTART record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  EMF+ manual mentioned in 2.1.1.1, reserved, not otherwise documented,
  Microsoft name: EmfPlusMultiFormatStart Record, Index 0x05
  */
int U_PMR_MULTIFORMATSTART_draw(const char *contents, FILE *out,
                                drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_MULTIFORMATSECTION record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  EMF+ manual mentioned in 2.1.1.1, reserved, not otherwise documented,
  Microsoft name: EmfPlusMultiFormatSection Record, Index 0x06
  */
int U_PMR_MULTIFORMATSECTION_draw(const char *contents, FILE *out,
                                  drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

/**
  \brief Print data from a  U_PMR_MULTIFORMATEND record
  \return 1 on success, 0 on error
  \param  contents    Record from which to print data
  EMF+ manual mentioned in 2.1.1.1, reserved, not otherwise documented,
  Microsoft name: EmfPlusMultiFormatEnd Record, Index 0x06
  */
int U_PMR_MULTIFORMATEND_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    return (U_PMR_NODATAREC_draw(contents, out, states));
}

#ifdef __cplusplus
}
#endif

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
