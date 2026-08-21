/**
  @file uemf_print.c

  @brief Functions for printing EMF records
  */

/*
File:      uemf_print.c
Version:   0.0.15
Date:      17-OCT-2013
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2013 David Mathog and California Institute of Technology (Caltech)
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include "emf2svg_private.h"
#include "emf2svg_print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "pmf2svg_print.h"

//! \cond
#define UNUSED(x) (void)(x)

#define IF_MEM_UNSAFE_PRINT_AND_RETURN(A, B, C)                                \
    if (IS_MEM_UNSAFE(A, B, C)) {                                              \
        verbose_printf("   record corruption HERE\n");                         \
        return;                                                                \
    }

/**
  \brief save the current device context on the stack.
  \param states drawingStates object
  */

void fill_print(drawingStates *states) {
    switch (states->currentDeviceContext.fill_polymode) {
    case (U_ALTERNATE):
        verbose_printf("   Fill Rule:      U_ALTERNATE\n");
        break;
    case (U_WINDING):
        verbose_printf("   Fill Rule:      U_WINDING\n");
        break;
    case (0):
        verbose_printf("   Fill Rule:      UNSET\n");
        break;
    default:
        verbose_printf("   Fill Rule:      UNKNOWN\n");
        break;
    }
    switch (states->currentDeviceContext.fill_mode) {
    case U_BS_SOLID:
        verbose_printf(
            "   Fill Mode:      BS_SOLID          Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        verbose_printf("   Fill Color:     #%02X%02X%02X\n",
                       states->currentDeviceContext.fill_red,
                       states->currentDeviceContext.fill_green,
                       states->currentDeviceContext.fill_blue);
        break;
    case U_BS_NULL:
        verbose_printf(
            "   Fill Mode:      BS_NULL           Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        break;
    case U_BS_HATCHED:
        verbose_printf(
            "   Fill Mode:      BS_HATCHED        Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_BS_PATTERN:
        verbose_printf(
            "   Fill Mode:      BS_PATTERN        Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_BS_INDEXED:
        verbose_printf(
            "   Fill Mode:      BS_INDEXED        Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_BS_DIBPATTERN:
        verbose_printf(
            "   Fill Mode:      BS_DIBPATTERN     Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_BS_DIBPATTERNPT:
        verbose_printf(
            "   Fill Mode:      BS_DIBPATTERNPT   Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_BS_PATTERN8X8:
        verbose_printf(
            "   Fill Mode:      BS_PATTERN8X8     Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_BS_DIBPATTERN8X8:
        verbose_printf(
            "   Fill Mode:      BS_DIBPATTERN8X8  Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_BS_MONOPATTERN:
        verbose_printf(
            "   Fill Mode:      BS_MONOPATTERN    Status: %sSUPPORTED%s\n",
            KRED, KNRM);
        break;
    default:
        verbose_printf("   Fill Mode:      %x     %sUNKNOWN%s\n",
                       states->currentDeviceContext.stroke_mode, KRED, KNRM);
        break;
    }
    return;
}

void stroke_print(drawingStates *states) {
    verbose_printf("   Stroke Mode:    0x%8.8X\n",
                   states->currentDeviceContext.stroke_mode);

    verbose_printf("   Stroke Color:   #%02X%02X%02X\n",
                   states->currentDeviceContext.stroke_red,
                   states->currentDeviceContext.stroke_green,
                   states->currentDeviceContext.stroke_blue);
    verbose_printf("   Stroke Width:   %f\n",
                   states->currentDeviceContext.stroke_width);
    // pen type
    switch (states->currentDeviceContext.stroke_mode & 0x000F0000) {
    case U_PS_COSMETIC:
        verbose_printf(
            "   Pen Type:       PS_COSMETIC       Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        break;
    case U_PS_GEOMETRIC:
        verbose_printf(
            "   Pen Type:       PS_GEOMETRIC      Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        break;
    default:
        verbose_printf("   Pen Type:       0x%X     %sUNKNOWN%s\n",
                       states->currentDeviceContext.stroke_mode & 0x000F0000,
                       KRED, KNRM);
        break;
    }
    // line style.
    switch (states->currentDeviceContext.stroke_mode & 0x000000FF) {
    case U_PS_SOLID:
        verbose_printf(
            "   Line Mode:      PS_SOLID          Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        break;
    case U_PS_DASH:
        verbose_printf(
            "   Line Mode:      PS_DASH           Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_PS_DOT:
        verbose_printf(
            "   Line Mode:      PS_DOT            Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_PS_DASHDOT:
        verbose_printf(
            "   Line Mode:      PS_DASHDOT        Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_PS_DASHDOTDOT:
        verbose_printf(
            "   Line Mode:      PS_DASHDOTDOT     Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_PS_NULL:
        verbose_printf(
            "   Line Mode:      PS_NULL           Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        break;
    case U_PS_INSIDEFRAME:
        verbose_printf(
            "   Line Mode:      PS_INSIDEFRAME    Status: %sPARTIAL%s\n", KYEL,
            KNRM);
        break;
    case U_PS_USERSTYLE:
        verbose_printf(
            "   Line Mode:      PS_USERSTYLE      Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    case U_PS_ALTERNATE:
        verbose_printf(
            "   Line Mode:      PS_ALTERNATE      Status: %sUNSUPPORTED%s\n",
            KRED, KNRM);
        break;
    default:
        verbose_printf("   Line Mode:      0x%X     %sUNKNOWN%s\n",
                       states->currentDeviceContext.stroke_mode & 0x000000FF,
                       KRED, KNRM);

        break;
    }
    // line cap.
    switch (states->currentDeviceContext.stroke_mode & 0x00000F00) {
    case U_PS_ENDCAP_ROUND:
        verbose_printf(
            "   Line Cap:       PS_ENDCAP_ROUND   Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        break;
    case U_PS_ENDCAP_SQUARE:
        verbose_printf(
            "   Line Cap:       PS_ENDCAP_SQUARE  Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        break;
    case U_PS_ENDCAP_FLAT:
        verbose_printf(
            "   Line Cap:       PS_ENDCAP_FLAT    Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        break;
    default:
        verbose_printf("   Line Cap:       0x%X     %sUNKNOWN%s\n",
                       states->currentDeviceContext.stroke_mode & 0x00000F00,
                       KRED, KNRM);

        break;
    }
    // line join.
    switch (states->currentDeviceContext.stroke_mode & 0x0000F000) {
    case U_PS_JOIN_ROUND:
        verbose_printf(
            "   Line Join:      U_PS_JOIN_ROUND   Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        break;
    case U_PS_JOIN_BEVEL:
        verbose_printf(
            "   Line Join:      PS_JOIN_BEVEL     Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        break;
    case U_PS_JOIN_MITER:
        verbose_printf(
            "   Line Join:      PS_JOIN_MITER     Status: %sSUPPORTED%s\n",
            KGRN, KNRM);
        break;
    default:
        verbose_printf("   Line Join:      0x%X     %sUNKNOWN%s\n",
                       states->currentDeviceContext.stroke_mode & 0x0000F000,
                       KRED, KNRM);
        break;
    }
}

/* one needed prototype */
void U_swap4(void *ul, unsigned int count);
//! \endcond

/**
  \brief Print some number of hex bytes
  \param buf pointer to the first byte
  \param num number of bytes
  */
void hexbytes_print(drawingStates *states, uint8_t *buf, unsigned int num) {
    for (; num; num--, buf++) {
        verbose_printf("%2.2X", *buf);
    }
}

/* **********************************************************************************************
   These functions print standard objects used in the EMR records.
   The low level ones do not append EOL.
 ***********************************************************************************************
 */

/**
  \brief Print a U_COLORREF object.
  \param color  U_COLORREF object
  */
void colorref_print(drawingStates *states, U_COLORREF color) {
    verbose_printf("{%u,%u,%u} ", color.Red, color.Green, color.Blue);
}

/**
  \brief Print a U_RGBQUAD object.
  \param color  U_RGBQUAD object
  */
void rgbquad_print(drawingStates *states, U_RGBQUAD color) {
    verbose_printf("{%u,%u,%u,%u} ", color.Blue, color.Green, color.Red,
                   color.Reserved);
}

/**
  \brief Print rect and rectl objects from Upper Left and Lower Right corner
  points.
  \param rect U_RECTL object
  */
void rectl_print(drawingStates *states, U_RECTL rect) {
    verbose_printf("{%d,%d,%d,%d} ", rect.left, rect.top, rect.right,
                   rect.bottom);
}

/**
  \brief Print a U_SIZEL object.
  \param sz U_SizeL object
  */
void sizel_print(drawingStates *states, U_SIZEL sz) {
    verbose_printf("{%d,%d} ", sz.cx, sz.cy);
}

/**
  \brief Print a U_POINTL object
  \param pt U_POINTL object
  */
void pointl_print(drawingStates *states, U_POINTL pt) {
    verbose_printf("{%d,%d} ", pt.x, pt.y);
}

/**
  \brief Print a pointer to a U_POINT16 object
  \param pt pointer to a U_POINT16 object
  Warning - WMF data may contain unaligned U_POINT16, do not call
  this routine with a pointer to such data!
  */
void point16_print(drawingStates *states, U_POINT16 pt) {
    verbose_printf("{%d,%d} ", pt.x, pt.y);
}

/**
  \brief Print a U_LCS_GAMMA object
  \param lg U_LCS_GAMMA object
  */
void lcs_gamma_print(drawingStates *states, U_LCS_GAMMA lg) {
    uint8_t tmp;
    tmp = lg.ignoreHi;
    verbose_printf("ignoreHi:%u ", tmp);
    tmp = lg.intPart;
    verbose_printf("intPart :%u ", tmp);
    tmp = lg.fracPart;
    verbose_printf("fracPart:%u ", tmp);
    tmp = lg.ignoreLo;
    verbose_printf("ignoreLo:%u ", tmp);
}

/**
  \brief Print a U_LCS_GAMMARGB object
  \param lgr U_LCS_GAMMARGB object
  */
void lcs_gammargb_print(drawingStates *states, U_LCS_GAMMARGB lgr) {
    verbose_printf("lcsGammaRed:");
    lcs_gamma_print(states, lgr.lcsGammaRed);
    verbose_printf("lcsGammaGreen:");
    lcs_gamma_print(states, lgr.lcsGammaGreen);
    verbose_printf("lcsGammaBlue:");
    lcs_gamma_print(states, lgr.lcsGammaBlue);
}

/**
  \brief Print a U_TRIVERTEX object.
  \param tv U_TRIVERTEX object.
  */
void trivertex_print(drawingStates *states, U_TRIVERTEX tv) {
    verbose_printf("{{%d,%d},{%u,%u,%u,%u}} ", tv.x, tv.y, tv.Red, tv.Green,
                   tv.Blue, tv.Alpha);
}

/**
  \brief Print a U_GRADIENT3 object.
  \param g3 U_GRADIENT3 object.
  */
void gradient3_print(drawingStates *states, U_GRADIENT3 g3) {
    verbose_printf("{%u,%u,%u} ", g3.Vertex1, g3.Vertex2, g3.Vertex3);
}

/**
  \brief Print a U_GRADIENT4 object.
  \param g4 U_GRADIENT4 object.
  */
void gradient4_print(drawingStates *states, U_GRADIENT4 g4) {
    verbose_printf("{%u,%u} ", g4.UpperLeft, g4.LowerRight);
}

/**
  \brief Print a U_LOGBRUSH object.
  \param lb U_LOGBRUSH object.
  */
void logbrush_print(drawingStates *states, U_LOGBRUSH lb) {
    verbose_printf("lbStyle:0x%8.8X ", lb.lbStyle);
    verbose_printf("lbColor:");
    colorref_print(states, lb.lbColor);
    verbose_printf("lbHatch:0x%8.8X ", lb.lbHatch);
}

/**
  \brief Print a U_XFORM object.
  \param xform U_XFORM object
  */
void xform_print(drawingStates *states, U_XFORM xform) {
    verbose_printf("{%f,%f.%f,%f,%f,%f} ", xform.eM11, xform.eM12, xform.eM21,
                   xform.eM22, xform.eDx, xform.eDy);
}

/**
  \brief Print a U_CIEXYZ object
  \param ciexyz U_CIEXYZ object
  */
void ciexyz_print(drawingStates *states, U_CIEXYZ ciexyz) {
    verbose_printf("{%d,%d.%d} ", ciexyz.ciexyzX, ciexyz.ciexyzY,
                   ciexyz.ciexyzZ);
}

/**
  \brief Print a U_CIEXYZTRIPLE object
  \param cie3 U_CIEXYZTRIPLE object
  */
void ciexyztriple_print(drawingStates *states, U_CIEXYZTRIPLE cie3) {
    verbose_printf("{Red:");
    ciexyz_print(states, cie3.ciexyzRed);
    verbose_printf(", Green:");
    ciexyz_print(states, cie3.ciexyzGreen);
    verbose_printf(", Blue:");
    ciexyz_print(states, cie3.ciexyzBlue);
    verbose_printf("} ");
}
/**
  \brief Print a U_LOGCOLORSPACEA object.
  \param lcsa     U_LOGCOLORSPACEA object
  */
void logcolorspacea_print(drawingStates *states, U_LOGCOLORSPACEA lcsa) {
    verbose_printf("lcsSignature:%u ", lcsa.lcsSignature);
    verbose_printf("lcsVersion:%u ", lcsa.lcsVersion);
    verbose_printf("lcsSize:%u ", lcsa.lcsSize);
    verbose_printf("lcsCSType:%d ", lcsa.lcsCSType);
    verbose_printf("lcsIntent:%d ", lcsa.lcsIntent);
    verbose_printf("lcsEndpoints:");
    ciexyztriple_print(states, lcsa.lcsEndpoints);
    verbose_printf("lcsGammaRGB: ");
    lcs_gammargb_print(states, lcsa.lcsGammaRGB);
    verbose_printf("filename:%s ", lcsa.lcsFilename);
}

/**

  \brief Print a U_LOGCOLORSPACEW object.
  \param lcsa U_LOGCOLORSPACEW object
  */
void logcolorspacew_print(drawingStates *states, U_LOGCOLORSPACEW lcsa) {
    char *string;
    verbose_printf("lcsSignature:%d ", lcsa.lcsSignature);
    verbose_printf("lcsVersion:%d ", lcsa.lcsVersion);
    verbose_printf("lcsSize:%d ", lcsa.lcsSize);
    verbose_printf("lcsCSType:%d ", lcsa.lcsCSType);
    verbose_printf("lcsIntent:%d ", lcsa.lcsIntent);
    verbose_printf("lcsEndpoints:");
    ciexyztriple_print(states, lcsa.lcsEndpoints);
    verbose_printf("lcsGammaRGB: ");
    lcs_gammargb_print(states, lcsa.lcsGammaRGB);
    string = U_Utf16leToUtf8(lcsa.lcsFilename, U_MAX_PATH, NULL);
    verbose_printf("filename:%s ", string);
    free(string);
}

/**
  \brief Print a U_PANOSE object.
  \param panose U_PANOSE object
  */
void panose_print(drawingStates *states, U_PANOSE panose) {
    verbose_printf("bFamilyType:%u ", panose.bFamilyType);
    verbose_printf("bSerifStyle:%u ", panose.bSerifStyle);
    verbose_printf("bWeight:%u ", panose.bWeight);
    verbose_printf("bProportion:%u ", panose.bProportion);
    verbose_printf("bContrast:%u ", panose.bContrast);
    verbose_printf("bStrokeVariation:%u ", panose.bStrokeVariation);
    verbose_printf("bArmStyle:%u ", panose.bArmStyle);
    verbose_printf("bLetterform:%u ", panose.bLetterform);
    verbose_printf("bMidline:%u ", panose.bMidline);
    verbose_printf("bXHeight:%u ", panose.bXHeight);
}

/**
  \brief Print a U_LOGFONT object.
  \param lf   U_LOGFONT object
  */
void logfont_print(drawingStates *states, U_LOGFONT lf) {
    char *string;
    verbose_printf("lfHeight:%d ", lf.lfHeight);
    verbose_printf("lfWidth:%d ", lf.lfWidth);
    verbose_printf("lfEscapement:%d ", lf.lfEscapement);
    verbose_printf("lfOrientation:%d ", lf.lfOrientation);
    verbose_printf("lfWeight:%d ", lf.lfWeight);
    verbose_printf("lfItalic:0x%2.2X ", lf.lfItalic);
    verbose_printf("lfUnderline:0x%2.2X ", lf.lfUnderline);
    verbose_printf("lfStrikeOut:0x%2.2X ", lf.lfStrikeOut);
    switch (lf.lfCharSet) {
    case U_ANSI_CHARSET:
        verbose_printf("lfCharSet:ANSI_CHARSET ");
        break;
    case U_DEFAULT_CHARSET:
        verbose_printf("lfCharSet:DEFAULT_CHARSET ");
        break;
    case U_SYMBOL_CHARSET:
        verbose_printf("lfCharSet:SYMBOL_CHARSET ");
        break;
    case U_SHIFTJIS_CHARSET:
        verbose_printf("lfCharSet:SHIFTJIS_CHARSET ");
        break;
    case U_HANGUL_CHARSET:
        verbose_printf("lfCharSet:HANGUL_CHARSET ");
        break;
    case U_GB2312_CHARSET:
        verbose_printf("lfCharSet:GB2312_CHARSET ");
        break;
    case U_CHINESEBIG5_CHARSET:
        verbose_printf("lfCharSet:CHINESEBIG5_CHARSET ");
        break;
    case U_GREEK_CHARSET:
        verbose_printf("lfCharSet:GREEK_CHARSET ");
        break;
    case U_TURKISH_CHARSET:
        verbose_printf("lfCharSet:TURKISH_CHARSET ");
        break;
    case U_HEBREW_CHARSET:
        verbose_printf("lfCharSet:HEBREW_CHARSET ");
        break;
    case U_ARABIC_CHARSET:
        verbose_printf("lfCharSet:ARABIC_CHARSET ");
        break;
    case U_BALTIC_CHARSET:
        verbose_printf("lfCharSet:BALTIC_CHARSET ");
        break;
    case U_RUSSIAN_CHARSET:
        verbose_printf("lfCharSet:RUSSIAN_CHARSET ");
        break;
    case U_EASTEUROPE_CHARSET:
        verbose_printf("lfCharSet:EASTEUROPE_CHARSET ");
        break;
    case U_THAI_CHARSET:
        verbose_printf("lfCharSet:THAI_CHARSET ");
        break;
    case U_JOHAB_CHARSET:
        verbose_printf("lfCharSet:JOHAB_CHARSET ");
        break;
    case U_MAC_CHARSET:
        verbose_printf("lfCharSet:MAC_CHARSET ");
        break;
    case U_OEM_CHARSET:
        verbose_printf("lfCharSet:OEM_CHARSET ");
        break;
    case U_VISCII_CHARSET:
        verbose_printf("lfCharSet:VISCII_CHARSET ");
        break;
    case U_TCVN_CHARSET:
        verbose_printf("lfCharSet:TCVN_CHARSET ");
        break;
    case U_KOI8_CHARSET:
        verbose_printf("lfCharSet:KOI8_CHARSET ");
        break;
    case U_ISO3_CHARSET:
        verbose_printf("lfCharSet:ISO3_CHARSET ");
        break;
    case U_ISO4_CHARSET:
        verbose_printf("lfCharSet:ISO4_CHARSET ");
        break;
    case U_ISO10_CHARSET:
        verbose_printf("lfCharSet:ISO10_CHARSET ");
        break;
    case U_CELTIC_CHARSET:
        verbose_printf("lfCharSet:CELTIC_CHARSET ");
        break;
    default:
        verbose_printf("lfCharSet:<Unknown charset [0x%X] >",
                       states->currentDeviceContext.font_charset);
        break;
    }
    verbose_printf("lfOutPrecision:0x%2.2X ", lf.lfOutPrecision);
    verbose_printf("lfClipPrecision:0x%2.2X ", lf.lfClipPrecision);
    verbose_printf("lfQuality:0x%2.2X ", lf.lfQuality);
    verbose_printf("lfPitchAndFamily:0x%2.2X ", lf.lfPitchAndFamily);
    string = U_Utf16leToUtf8(lf.lfFaceName, U_LF_FACESIZE, NULL);
    verbose_printf("lfFaceName:%s ", string);
    free(string);
}

/**
  \brief Print a U_LOGFONT_PANOSE object.
  \return U_LOGFONT_PANOSE object
  */
void logfont_panose_print(drawingStates *states, U_LOGFONT_PANOSE lfp) {
    char *string;
    verbose_printf("elfLogFont:");
    logfont_print(states, lfp.elfLogFont);
    string = U_Utf16leToUtf8(lfp.elfFullName, U_LF_FULLFACESIZE, NULL);
    verbose_printf("elfFullName:%s ", string);
    free(string);
    string = U_Utf16leToUtf8(lfp.elfStyle, U_LF_FACESIZE, NULL);
    verbose_printf("elfStyle:%s ", string);
    free(string);
    verbose_printf("elfVersion:%u ", lfp.elfVersion);
    verbose_printf("elfStyleSize:%u ", lfp.elfStyleSize);
    verbose_printf("elfMatch:%u ", lfp.elfMatch);
    verbose_printf("elfReserved:%u ", lfp.elfReserved);
    verbose_printf("elfVendorId:");
    hexbytes_print(states, (uint8_t *)lfp.elfVendorId, U_ELF_VENDOR_SIZE);
    verbose_printf(" ");
    verbose_printf("elfCulture:%u ", lfp.elfCulture);
    verbose_printf("elfPanose:");
    panose_print(states, lfp.elfPanose);
}

/**
  \brief Print a pointer to U_BITMAPINFOHEADER object.

  This may be called indirectly from WMF _print routines, where problems could
  occur
  if the data was passed as the struct or a pointer to the struct, as the struct
  may not
  be aligned in memory.

  \returns Actual number of color table entries.
  \param Bmih pointer to a U_BITMAPINFOHEADER object
  */
int bitmapinfoheader_print(drawingStates *states, const char *Bmih) {
    uint32_t utmp4;
    int32_t tmp4;
    int16_t tmp2;
    int Colors, BitCount, Width, Height, RealColors;

    /* DIB from a WMF may not be properly aligned on a 4 byte boundary, will be
     * aligned on a 2 byte boundary */

    memcpy(&utmp4, Bmih + offsetof(U_BITMAPINFOHEADER, biSize), 4);
    verbose_printf("biSize:%u ", utmp4);
    memcpy(&tmp4, Bmih + offsetof(U_BITMAPINFOHEADER, biWidth), 4);
    verbose_printf("biWidth:%d ", tmp4);
    Width = tmp4;
    memcpy(&tmp4, Bmih + offsetof(U_BITMAPINFOHEADER, biHeight), 4);
    verbose_printf("biHeight:%d ", tmp4);
    Height = tmp4;
    memcpy(&tmp2, Bmih + offsetof(U_BITMAPINFOHEADER, biPlanes), 2);
    verbose_printf("biPlanes:%u ", tmp2);
    memcpy(&tmp2, Bmih + offsetof(U_BITMAPINFOHEADER, biBitCount), 2);
    verbose_printf("biBitCount:%u ", tmp2);
    BitCount = tmp2;
    memcpy(&utmp4, Bmih + offsetof(U_BITMAPINFOHEADER, biCompression), 4);
    verbose_printf("biCompression:%u ", utmp4);
    memcpy(&utmp4, Bmih + offsetof(U_BITMAPINFOHEADER, biSizeImage), 4);
    verbose_printf("biSizeImage:%u ", utmp4);
    memcpy(&tmp4, Bmih + offsetof(U_BITMAPINFOHEADER, biXPelsPerMeter), 4);
    verbose_printf("biXPelsPerMeter:%d ", tmp4);
    memcpy(&tmp4, Bmih + offsetof(U_BITMAPINFOHEADER, biYPelsPerMeter), 4);
    verbose_printf("biYPelsPerMeter:%d ", tmp4);
    memcpy(&utmp4, Bmih + offsetof(U_BITMAPINFOHEADER, biClrUsed), 4);
    verbose_printf("biClrUsed:%u ", utmp4);
    Colors = utmp4;
    memcpy(&utmp4, Bmih + offsetof(U_BITMAPINFOHEADER, biClrImportant), 4);
    verbose_printf("biClrImportant:%u ", utmp4);
    RealColors = get_real_color_icount(Colors, BitCount, Width, Height);
    verbose_printf("ColorEntries:%d ", RealColors);
    return (RealColors);
}

/**
  \brief Print a Pointer to a U_BITMAPINFO object.
  \param Bmi Pointer to a U_BITMAPINFO object
  This may be called from WMF _print routines, where problems could occur
  if the data was passed as the struct or a pointer to the struct, as the struct
  may not
  be aligned in memory.
  */
void bitmapinfo_print(drawingStates *states, const char *Bmi,
                      const char *blimit) {
    int i, k;
    int ClrUsed;
    U_RGBQUAD BmiColor;
    verbose_printf("BmiHeader: ");
    IF_MEM_UNSAFE_PRINT_AND_RETURN(
        Bmi, offsetof(U_BITMAPINFO, bmiHeader) + sizeof(U_BITMAPINFOHEADER),
        blimit);
    ClrUsed =
        bitmapinfoheader_print(states, Bmi + offsetof(U_BITMAPINFO, bmiHeader));
    if (ClrUsed) {
        k = offsetof(U_BITMAPINFO, bmiColors);
        IF_MEM_UNSAFE_PRINT_AND_RETURN(Bmi,
                                       offsetof(U_BITMAPINFO, bmiColors) +
                                           ClrUsed * sizeof(U_RGBQUAD),
                                       blimit);
        for (i = 0; i < ClrUsed; i++, k += sizeof(U_RGBQUAD)) {
            memcpy(&BmiColor, Bmi + k, sizeof(U_RGBQUAD));
            verbose_printf("%d:", i);
            rgbquad_print(states, BmiColor);
        }
    }
}

/**
  \brief Print a U_BLEND object.
  \param blend a U_BLEND object
  */
void blend_print(drawingStates *states, U_BLEND blend) {
    verbose_printf("Operation:%u ", blend.Operation);
    verbose_printf("Flags:%u ", blend.Flags);
    verbose_printf("Global:%u ", blend.Global);
    verbose_printf("Op:%u ", blend.Op);
}

/**
  \brief Print a pointer to a U_EXTLOGPEN object.
  \param elp   PU_EXTLOGPEN object
  */
void extlogpen_print(drawingStates *states, PU_EXTLOGPEN elp) {
    unsigned int i;
    U_STYLEENTRY *elpStyleEntry;
    verbose_printf("elpPenStyle:0x%8.8X ", elp->elpPenStyle);
    verbose_printf("elpWidth:%u ", elp->elpWidth);
    verbose_printf("elpBrushStyle:0x%8.8X ", elp->elpBrushStyle);
    verbose_printf("elpColor");
    colorref_print(states, elp->elpColor);
    verbose_printf("elpHatch:%d ", elp->elpHatch);
    verbose_printf("elpNumEntries:%u ", elp->elpNumEntries);
    if (elp->elpNumEntries) {
        verbose_printf("elpStyleEntry:");
        elpStyleEntry = (uint32_t *)elp->elpStyleEntry;
        for (i = 0; i < elp->elpNumEntries; i++) {
            verbose_printf("%d:%u ", i, elpStyleEntry[i]);
        }
    }
}

/**
  \brief Print a U_LOGPEN object.
  \param lp  U_LOGPEN object

*/
void logpen_print(drawingStates *states, U_LOGPEN lp) {
    verbose_printf("lopnStyle:0x%8.8X ", lp.lopnStyle);
    verbose_printf("lopnWidth:");
    pointl_print(states, lp.lopnWidth);
    verbose_printf("lopnColor:");
    colorref_print(states, lp.lopnColor);
}

/**
  \brief Print a U_LOGPLTNTRY object.
  \param lpny Ignore U_LOGPLTNTRY object.
  */
void logpltntry_print(drawingStates *states, U_LOGPLTNTRY lpny) {
    verbose_printf("peReserved:%u ", lpny.peReserved);
    verbose_printf("peRed:%u ", lpny.peRed);
    verbose_printf("peGreen:%u ", lpny.peGreen);
    verbose_printf("peBlue:%u ", lpny.peBlue);
}

/**
  \brief Print a pointer to a U_LOGPALETTE object.
  \param lp  Pointer to a U_LOGPALETTE object.
  */
void logpalette_print(drawingStates *states, PU_LOGPALETTE lp) {
    int i;
    PU_LOGPLTNTRY palPalEntry;
    verbose_printf("palVersion:%u ", lp->palVersion);
    verbose_printf("palNumEntries:%u ", lp->palNumEntries);
    if (lp->palNumEntries) {
        palPalEntry = (PU_LOGPLTNTRY) & (lp->palPalEntry);
        for (i = 0; i < lp->palNumEntries; i++) {
            verbose_printf("%d:", i);
            logpltntry_print(states, palPalEntry[i]);
        }
    }
}

/**
  \brief Print a U_RGNDATAHEADER object.
  \param rdh  U_RGNDATAHEADER object
  */
void rgndataheader_print(drawingStates *states, U_RGNDATAHEADER rdh) {
    verbose_printf("dwSize:%u ", rdh.dwSize);
    verbose_printf("iType:%u ", rdh.iType);
    verbose_printf("nCount:%u ", rdh.nCount);
    verbose_printf("nRgnSize:%u ", rdh.nRgnSize);
    verbose_printf("rclBounds:");
    rectl_print(states, rdh.rclBounds);
}

/**
  \brief Print a pointer to a U_RGNDATA object.
  \param rd  pointer to a U_RGNDATA object.

  */
void rgndata_print(drawingStates *states, PU_RGNDATA rd, const char *blimit) {
    unsigned int i;
    PU_RECTL rects;
    IF_MEM_UNSAFE_PRINT_AND_RETURN(rd, sizeof(U_RGNDATAHEADER), blimit);
    verbose_printf("rdh:");
    rgndataheader_print(states, rd->rdh);
    verbose_printf(" rects: ");
    if (rd->rdh.nCount) {
        rects = (PU_RECTL) & (rd->Buffer);
        IF_MEM_UNSAFE_PRINT_AND_RETURN(rects, rd->rdh.nCount * sizeof(U_RECTL),
                                       blimit);
        for (i = 0; i < rd->rdh.nCount; i++) {
            verbose_printf("%d:", i);
            rectl_print(states, rects[i]);
        }
    }
}

/**
  \brief Print a U_COLORADJUSTMENT object.
  \param ca U_COLORADJUSTMENT object.
  */
void coloradjustment_print(drawingStates *states, U_COLORADJUSTMENT ca) {
    verbose_printf("caSize:%u ", ca.caSize);
    verbose_printf("caFlags:0x%4.4X ", ca.caFlags);
    verbose_printf("caIlluminantIndex:%u ", ca.caIlluminantIndex);
    verbose_printf("caRedGamma:%u ", ca.caRedGamma);
    verbose_printf("caGreenGamma:%u ", ca.caGreenGamma);
    verbose_printf("caBlueGamma:%u ", ca.caBlueGamma);
    verbose_printf("caReferenceBlack:%u ", ca.caReferenceBlack);
    verbose_printf("caReferenceWhite:%u ", ca.caReferenceWhite);
    verbose_printf("caContrast:%d ", ca.caContrast);
    verbose_printf("caBrightness:%d ", ca.caBrightness);
    verbose_printf("caColorfulness:%d ", ca.caColorfulness);
    verbose_printf("caRedGreenTint:%d ", ca.caRedGreenTint);
}

/**
  \brief Print a U_PIXELFORMATDESCRIPTOR object.
  \param pfd  U_PIXELFORMATDESCRIPTOR object
  */
void pixelformatdescriptor_print(drawingStates *states,
                                 U_PIXELFORMATDESCRIPTOR pfd) {
    verbose_printf("nSize:%u ", pfd.nSize);
    verbose_printf("nVersion:%u ", pfd.nVersion);
    verbose_printf("dwFlags:0x%8.8X ", pfd.dwFlags);
    verbose_printf("iPixelType:%u ", pfd.iPixelType);
    verbose_printf("cColorBits:%u ", pfd.cColorBits);
    verbose_printf("cRedBits:%u ", pfd.cRedBits);
    verbose_printf("cRedShift:%u ", pfd.cRedShift);
    verbose_printf("cGreenBits:%u ", pfd.cGreenBits);
    verbose_printf("cGreenShift:%u ", pfd.cGreenShift);
    verbose_printf("cBlueBits:%u ", pfd.cBlueBits);
    verbose_printf("cBlueShift:%u ", pfd.cBlueShift);
    verbose_printf("cAlphaBits:%u ", pfd.cAlphaBits);
    verbose_printf("cAlphaShift:%u ", pfd.cAlphaShift);
    verbose_printf("cAccumBits:%u ", pfd.cAccumBits);
    verbose_printf("cAccumRedBits:%u ", pfd.cAccumRedBits);
    verbose_printf("cAccumGreenBits:%u ", pfd.cAccumGreenBits);
    verbose_printf("cAccumBlueBits:%u ", pfd.cAccumBlueBits);
    verbose_printf("cAccumAlphaBits:%u ", pfd.cAccumAlphaBits);
    verbose_printf("cDepthBits:%u ", pfd.cDepthBits);
    verbose_printf("cStencilBits:%u ", pfd.cStencilBits);
    verbose_printf("cAuxBuffers:%u ", pfd.cAuxBuffers);
    verbose_printf("iLayerType:%u ", pfd.iLayerType);
    verbose_printf("bReserved:%u ", pfd.bReserved);
    verbose_printf("dwLayerMask:%u ", pfd.dwLayerMask);
    verbose_printf("dwVisibleMask:%u ", pfd.dwVisibleMask);
    verbose_printf("dwDamageMask:%u ", pfd.dwDamageMask);
}

/**
  \brief Print a Pointer to a U_EMRTEXT record
  \param emt      Pointer to a U_EMRTEXT record
  \param record   Pointer to the start of the record which contains this
  U_ERMTEXT
  \param type     0 for 8 bit character, anything else for 16
  */

void emrtext_print(drawingStates *states, const char *emt, const char *record,
                   const char *blimit, int type) {
    unsigned int i, off;
    char *string;
    PU_EMRTEXT pemt = (PU_EMRTEXT)emt;
    // constant part
    verbose_printf("ptlReference:");
    pointl_print(states, pemt->ptlReference);
    verbose_printf("nChars:%u ", pemt->nChars);
    verbose_printf("offString:%u ", pemt->offString);
    if (pemt->offString) {
        if (!type) {
            returnOutOfEmf((intptr_t)(record + pemt->offString) +
                           (intptr_t)pemt->nChars);
            IF_MEM_UNSAFE_PRINT_AND_RETURN(
                record, pemt->offString + pemt->nChars * sizeof(char), blimit);
            verbose_printf("string8:<%s> ", record + pemt->offString);
            verbose_printf("hexa:<");
            hexbytes_print(states, (uint8_t *)(record + pemt->offString),
                           pemt->nChars * sizeof(char));
            verbose_printf("> ");
        } else {
            returnOutOfEmf((intptr_t)(record + pemt->offString) +
                           2 * (intptr_t)pemt->nChars);
            IF_MEM_UNSAFE_PRINT_AND_RETURN(
                record, pemt->offString + pemt->nChars * 2 * sizeof(char),
                blimit);
            string = U_Utf16leToUtf8((uint16_t *)(record + pemt->offString),
                                     pemt->nChars, NULL);
            verbose_printf("string16:<%s> ", string);
            verbose_printf("hexa:<");
            hexbytes_print(states, (uint8_t *)(record + pemt->offString),
                           pemt->nChars * 2 * sizeof(char));
            verbose_printf("> ");
            free(string);
        }
    }
    verbose_printf("fOptions:0x%8.8X ", pemt->fOptions);
    off = sizeof(U_EMRTEXT);
    if (!(pemt->fOptions & U_ETO_NO_RECT)) {
        verbose_printf("rcl");
        rectl_print(states, *((U_RECTL *)(emt + off)));
        off += sizeof(U_RECTL);
    }
    verbose_printf("offDx:%u ", *((U_OFFDX *)(emt + off)));
    off = *(U_OFFDX *)(emt + off);
    verbose_printf("Dx:");
    for (i = 0; i < pemt->nChars; i++, off += sizeof(uint32_t)) {
        verbose_printf("%d:", *((uint32_t *)(record + off)));
    }
}

// hide these from Doxygen
//! @cond
/* **********************************************************************************************
   These functions contain shared code used by various U_EMR*_print functions.
 These should NEVER be called
   by end user code and to further that end prototypes are NOT provided and they
 are hidden from Doxygen.


   These are (mostly) ordered by U_EMR_* index number.

   The exceptions:
   void core3_print(const char *name, const char *label, const char *contents,
 drawingStates *states)
   void core7_print(const char *name, const char *field1, const char *field2,
 const char *contents, drawingStates *states)
   void core8_print(const char *name, const char *contents, drawingStates
 *states, int type)


 ***********************************************************************************************
 */

// Functions with the same form starting with U_EMRPOLYBEZIER_print
void core1_print(const char *name, const char *contents,
                 drawingStates *states) {
    unsigned int i;
    UNUSED(name);
    PU_EMRPOLYLINETO pEmr = (PU_EMRPOLYLINETO)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRPOLYBEZIER)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   cptl:           %d\n", pEmr->cptl);
    verbose_printf("   Points:         ");
    IF_MEM_UNSAFE_PRINT_AND_RETURN(pEmr->aptl, pEmr->cptl * sizeof(U_POINTL),
                                   blimit);
    for (i = 0; i < pEmr->cptl; i++) {
        verbose_printf("[%d]:", i);
        pointl_print(states, pEmr->aptl[i]);
    }
    verbose_printf("\n");
}

// Functions with the same form starting with U_EMRPOLYPOLYLINE_print
void core2_print(const char *name, const char *contents,
                 drawingStates *states) {
    unsigned int i;
    UNUSED(name);
    PU_EMRPOLYPOLYGON pEmr = (PU_EMRPOLYPOLYGON)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRPOLYPOLYGON)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   nPolys:         %d\n", pEmr->nPolys);
    verbose_printf("   cptl:           %d\n", pEmr->cptl);
    verbose_printf("   Counts:         ");
    IF_MEM_UNSAFE_PRINT_AND_RETURN(pEmr->aPolyCounts,
                                   pEmr->nPolys * sizeof(U_POLYCOUNTS), blimit);
    for (i = 0; i < pEmr->nPolys; i++) {
        verbose_printf(" [%d]:%d ", i, pEmr->aPolyCounts[i]);
    }
    verbose_printf("\n");
    PU_POINTL paptl = (PU_POINTL)((char *)pEmr->aPolyCounts +
                                  sizeof(uint32_t) * pEmr->nPolys);
    verbose_printf("   Points:         ");
    IF_MEM_UNSAFE_PRINT_AND_RETURN(paptl, pEmr->cptl * sizeof(U_POINTL),
                                   blimit);
    for (i = 0; i < pEmr->cptl; i++) {
        verbose_printf("[%d]:", i);
        pointl_print(states, paptl[i]);
    }
    verbose_printf("\n");
}

// Functions with the same form starting with U_EMRSETMAPMODE_print
void core3_print(const char *name, const char *label, const char *contents,
                 drawingStates *states) {
    UNUSED(name);
    /* access violation is impossible for these because there are no counts or
     * offsets */
    PU_EMRSETMAPMODE pEmr = (PU_EMRSETMAPMODE)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRSETMAPMODE)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    if (!strcmp(label, "crColor:")) {
        verbose_printf("   %-15s ", label);
        colorref_print(states, *(U_COLORREF *)&(pEmr->iMode));
        verbose_printf("\n");
    } else if (!strcmp(label, "iMode:")) {
        verbose_printf("   %-15s 0x%8.8X\n", label, pEmr->iMode);
    } else {
        verbose_printf("   %-15s %d\n", label, pEmr->iMode);
    }
}

// Functions taking a single U_RECT or U_RECTL, starting with
// U_EMRELLIPSE_print, also U_EMRFILLPATH_print,
void core4_print(const char *name, const char *contents,
                 drawingStates *states) {
    UNUSED(name);
    PU_EMRELLIPSE pEmr = (PU_EMRELLIPSE)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRELLIPSE)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   rclBox:         ");
    rectl_print(states, pEmr->rclBox);
    verbose_printf("\n");
}

// Functions with the same form starting with U_EMRPOLYBEZIER16_print
void core6_print(const char *name, const char *contents,
                 drawingStates *states) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYBEZIER16 pEmr = (PU_EMRPOLYBEZIER16)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRPOLYBEZIER16)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   cpts:           %d\n", pEmr->cpts);
    verbose_printf("   Points:         ");
    PU_POINT16 papts = (PU_POINT16)(&(pEmr->apts));
    IF_MEM_UNSAFE_PRINT_AND_RETURN(papts, pEmr->cpts * sizeof(U_POINT16),
                                   blimit);
    for (i = 0; i < pEmr->cpts; i++) {
        verbose_printf("[%d]:", i);
        point16_print(states, papts[i]);
    }
    verbose_printf("\n");
}

// Records with the same form starting with U_EMRSETWINDOWEXTEX_print
// CAREFUL, in the _set equivalents all functions with two uint32_t values are
// mapped here, and member names differ, consequently
//   print routines must supply the names of the two arguments.  These cannot be
//   null.  If the second one is
//   empty the values are printed as a pair {x,y}, otherwise each is printed
//   with its own label on a separate line.
void core7_print(const char *name, const char *field1, const char *field2,
                 const char *contents, drawingStates *states) {
    UNUSED(name);
    PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRGENERICPAIR)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    if (*field2) {
        verbose_printf("   %-15s %d\n", field1, pEmr->pair.x);
        verbose_printf("   %-15s %d\n", field2, pEmr->pair.y);
    } else {
        verbose_printf("   %-15s {%d,%d}\n", field1, pEmr->pair.x,
                       pEmr->pair.y);
    }
}

// For U_EMREXTTEXTOUTA and U_EMREXTTEXTOUTW, type=0 for the first one
void core8_print(const char *name, const char *contents, drawingStates *states,
                 int type) {
    UNUSED(name);
    PU_EMREXTTEXTOUTA pEmr = (PU_EMREXTTEXTOUTA)(contents);
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   iGraphicsMode:  %u\n", pEmr->iGraphicsMode);
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   exScale:        %f\n", pEmr->exScale);
    verbose_printf("   eyScale:        %f\n", pEmr->eyScale);
    verbose_printf("   emrtext:        ");
    emrtext_print(states,
                  contents + sizeof(U_EMREXTTEXTOUTA) - sizeof(U_EMRTEXT),
                  contents, blimit, type);
    verbose_printf("\n");
}

// Functions that take a rect and a pair of points, starting with U_EMRARC_print
void core9_print(const char *name, const char *contents,
                 drawingStates *states) {
    UNUSED(name);
    PU_EMRARC pEmr = (PU_EMRARC)(contents);
    verbose_printf("   rclBox:         ");
    rectl_print(states, pEmr->rclBox);
    verbose_printf("\n");
    verbose_printf("   ptlStart:       ");
    pointl_print(states, pEmr->ptlStart);
    verbose_printf("\n");
    verbose_printf("   ptlEnd:         ");
    pointl_print(states, pEmr->ptlEnd);
    verbose_printf("\n");
}

// Functions with the same form starting with U_EMRPOLYPOLYLINE16_print
void core10_print(const char *name, const char *contents,
                  drawingStates *states) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYPOLYLINE16 pEmr = (PU_EMRPOLYPOLYLINE16)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRPOLYPOLYLINE16)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   nPolys:         %d\n", pEmr->nPolys);
    verbose_printf("   cpts:           %d\n", pEmr->cpts);
    verbose_printf("   Counts:         ");
    IF_MEM_UNSAFE_PRINT_AND_RETURN(&(pEmr->aPolyCounts),
                                   pEmr->nPolys * sizeof(U_POLYCOUNTS), blimit);
    for (i = 0; i < pEmr->nPolys; i++) {
        verbose_printf(" [%d]:%d ", i, pEmr->aPolyCounts[i]);
    }
    verbose_printf("\n");
    verbose_printf("   Points:         ");
    PU_POINT16 papts = (PU_POINT16)((char *)pEmr->aPolyCounts +
                                    sizeof(uint32_t) * pEmr->nPolys);
    IF_MEM_UNSAFE_PRINT_AND_RETURN(papts, pEmr->cpts * sizeof(U_POINT16),
                                   blimit);
    for (i = 0; i < pEmr->cpts; i++) {
        verbose_printf("[%d]:", i);
        point16_print(states, papts[i]);
    }
    verbose_printf("\n");
}

// Functions with the same form starting with  U_EMRINVERTRGN_print and
// U_EMRPAINTRGN_print,
void core11_print(const char *name, const char *contents,
                  drawingStates *states) {
    UNUSED(name);
    PU_EMRINVERTRGN pEmr = (PU_EMRINVERTRGN)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRINVERTRGN)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   cbRgnData:      %d\n", pEmr->cbRgnData);
    verbose_printf("   RegionData:");
    const char *minptr =
        MAKE_MIN_PTR(((const char *)&pEmr->RgnData + pEmr->cbRgnData), blimit);
    rgndata_print(states, pEmr->RgnData, minptr);
    verbose_printf("\n");
}

// common code for U_EMRCREATEMONOBRUSH_print and
// U_EMRCREATEDIBPATTERNBRUSHPT_print,
void core12_print(const char *name, const char *contents,
                  drawingStates *states) {
    UNUSED(name);
    PU_EMRCREATEMONOBRUSH pEmr = (PU_EMRCREATEMONOBRUSH)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRCREATEMONOBRUSH)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   ihBrush:      %u\n", pEmr->ihBrush);
    verbose_printf("   iUsage :      %u\n", pEmr->iUsage);
    verbose_printf("   offBmi :      %u\n", pEmr->offBmi);
    verbose_printf("   cbBmi  :      %u\n", pEmr->cbBmi);
    if (pEmr->cbBmi) {
        verbose_printf("      bitmap:");
        bitmapinfo_print(states, contents + pEmr->offBmi, blimit);
        verbose_printf("\n");
    }
    verbose_printf("   offBits:      %u\n", pEmr->offBits);
    verbose_printf("   cbBits :      %u\n", pEmr->cbBits);
}

// common code for U_EMRALPHABLEND_print and U_EMRTRANSPARENTBLT_print,
void core13_print(const char *name, const char *contents,
                  drawingStates *states) {
    UNUSED(name);
    PU_EMRALPHABLEND pEmr = (PU_EMRALPHABLEND)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRALPHABLEND)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   Dest:           ");
    pointl_print(states, pEmr->Dest);
    verbose_printf("\n");
    verbose_printf("   cDest:          ");
    pointl_print(states, pEmr->cDest);
    verbose_printf("\n");
    verbose_printf("   Blend:          ");
    blend_print(states, pEmr->Blend);
    verbose_printf("\n");
    verbose_printf("   Src:            ");
    pointl_print(states, pEmr->Src);
    verbose_printf("\n");
    verbose_printf("   xformSrc:       ");
    xform_print(states, pEmr->xformSrc);
    verbose_printf("\n");
    verbose_printf("   crBkColorSrc:   ");
    colorref_print(states, pEmr->crBkColorSrc);
    verbose_printf("\n");
    verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc);
    verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc);
    verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc);
    if (pEmr->cbBmiSrc) {
        verbose_printf("      bitmap:");
        bitmapinfo_print(states, contents + pEmr->offBmiSrc, blimit);
        verbose_printf("\n");
    }
    verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc);
    verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc);
}
//! @endcond

/* **********************************************************************************************
   These are the core EMR functions, each creates a particular type of record.
   All return these records via a char* pointer, which is NULL if the call
 failed.
   They are listed in order by the corresponding U_EMR_* index number.
 ***********************************************************************************************
 */

/**
  \brief Print a pointer to a U_EMR_whatever record which has not been
  implemented.
  \param name       name of this type of record
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRNOTIMPLEMENTED_print(const char *name, const char *contents,
                               drawingStates *states) {
    UNUSED(name);
    UNUSED(contents);
    verbose_printf("   Not Implemented!\n");
}

// U_EMRHEADER                1
/**
  \brief Print a pointer to a U_EMR_HEADER record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRHEADER_print(const char *contents, drawingStates *states) {
    char *string;
    int p1len;

    PU_EMRHEADER pEmr = (PU_EMRHEADER)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRHEADER)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   rclFrame:       ");
    rectl_print(states, pEmr->rclFrame);
    verbose_printf("\n");
    verbose_printf("   dSignature:     0x%8.8X\n", pEmr->dSignature);
    verbose_printf("   nVersion:       0x%8.8X\n", pEmr->nVersion);
    verbose_printf("   nBytes:         %d\n", pEmr->nBytes);
    verbose_printf("   nRecords:       %d\n", pEmr->nRecords);
    verbose_printf("   nHandles:       %d\n", pEmr->nHandles);
    verbose_printf("   sReserved:      %d\n", pEmr->sReserved);
    verbose_printf("   nDescription:   %d\n", pEmr->nDescription);
    verbose_printf("   offDescription: %d\n", pEmr->offDescription);
    if (pEmr->offDescription) {
        IF_MEM_UNSAFE_PRINT_AND_RETURN(
            contents,
            pEmr->offDescription + pEmr->nDescription * 2 * sizeof(char),
            blimit);
        string =
            U_Utf16leToUtf8((uint16_t *)((char *)pEmr + pEmr->offDescription),
                            pEmr->nDescription, NULL);
        verbose_printf("      Desc. A:  %s\n", string);
        free(string);
        p1len =
            2 +
            2 * wchar16len((uint16_t *)((char *)pEmr + pEmr->offDescription));
        string = U_Utf16leToUtf8(
            (uint16_t *)((char *)pEmr + pEmr->offDescription + p1len),
            pEmr->nDescription, NULL);
        verbose_printf("      Desc. B:  %s\n", string);
        free(string);
    }
    verbose_printf("   nPalEntries:    %d\n", pEmr->nPalEntries);
    verbose_printf("   szlDevice:      {%d,%d} \n", pEmr->szlDevice.cx,
                   pEmr->szlDevice.cy);
    verbose_printf("   szlMillimeters: {%d,%d} \n", pEmr->szlMillimeters.cx,
                   pEmr->szlMillimeters.cy);
    if ((pEmr->nDescription && (pEmr->offDescription >= 100)) ||
        (!pEmr->offDescription && pEmr->emr.nSize >= 100)) {
        verbose_printf("   cbPixelFormat:  %d\n", pEmr->cbPixelFormat);
        verbose_printf("   offPixelFormat: %d\n", pEmr->offPixelFormat);
        if (pEmr->cbPixelFormat) {
            verbose_printf("      PFD:");
            IF_MEM_UNSAFE_PRINT_AND_RETURN(
                contents,
                pEmr->offPixelFormat + sizeof(U_PIXELFORMATDESCRIPTOR), blimit);
            pixelformatdescriptor_print(
                states,
                *(PU_PIXELFORMATDESCRIPTOR)(contents + pEmr->offPixelFormat));
            verbose_printf("\n");
        }
        verbose_printf("   bOpenGL:        %d\n", pEmr->bOpenGL);
        if ((pEmr->nDescription && (pEmr->offDescription >= 108)) ||
            (pEmr->cbPixelFormat && (pEmr->offPixelFormat >= 108)) ||
            (!pEmr->offDescription && !pEmr->cbPixelFormat &&
             pEmr->emr.nSize >= 108)) {
            verbose_printf("   szlMicrometers: {%d,%d} \n",
                           pEmr->szlMicrometers.cx, pEmr->szlMicrometers.cy);
        }
    }
}

// U_EMRPOLYBEZIER                       2
/**
  \brief Print a pointer to a U_EMR_POLYBEZIER record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYBEZIER_print(const char *contents, drawingStates *states) {
    core1_print("U_EMRPOLYBEZIER", contents, states);
}

// U_EMRPOLYGON                          3
/**
  \brief Print a pointer to a U_EMR_POLYGON record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYGON_print(const char *contents, drawingStates *states) {
    core1_print("U_EMRPOLYGON", contents, states);
}

// U_EMRPOLYLINE              4
/**
  \brief Print a pointer to a U_EMR_POLYLINE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYLINE_print(const char *contents, drawingStates *states) {
    core1_print("U_EMRPOLYLINE", contents, states);
}

// U_EMRPOLYBEZIERTO          5
/**
  \brief Print a pointer to a U_EMR_POLYBEZIERTO record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYBEZIERTO_print(const char *contents, drawingStates *states) {
    core1_print("U_EMRPOLYBEZIERTO", contents, states);
}

// U_EMRPOLYLINETO            6
/**
  \brief Print a pointer to a U_EMR_POLYLINETO record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYLINETO_print(const char *contents, drawingStates *states) {
    core1_print("U_EMRPOLYLINETO", contents, states);
}

// U_EMRPOLYPOLYLINE          7
/**
  \brief Print a pointer to a U_EMR_POLYPOLYLINE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYPOLYLINE_print(const char *contents, drawingStates *states) {
    core2_print("U_EMRPOLYPOLYLINE", contents, states);
}

// U_EMRPOLYPOLYGON           8
/**
  \brief Print a pointer to a U_EMR_POLYPOLYGON record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYPOLYGON_print(const char *contents, drawingStates *states) {
    core2_print("U_EMRPOLYPOLYGON", contents, states);
}

// U_EMRSETWINDOWEXTEX        9
/**
  \brief Print a pointer to a U_EMR_SETWINDOWEXTEX record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETWINDOWEXTEX_print(const char *contents, drawingStates *states) {
    core7_print("U_EMRSETWINDOWEXTEX", "szlExtent:", "", contents, states);
}

// U_EMRSETWINDOWORGEX       10
/**
  \brief Print a pointer to a U_EMR_SETWINDOWORGEX record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETWINDOWORGEX_print(const char *contents, drawingStates *states) {
    core7_print("U_EMRSETWINDOWORGEX", "ptlOrigin:", "", contents, states);
}

// U_EMRSETVIEWPORTEXTEX     11
/**
  \brief Print a pointer to a U_EMR_SETVIEWPORTEXTEX record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETVIEWPORTEXTEX_print(const char *contents, drawingStates *states) {
    core7_print("U_EMRSETVIEWPORTEXTEX", "szlExtent:", "", contents, states);
}

// U_EMRSETVIEWPORTORGEX     12
/**
  \brief Print a pointer to a U_EMR_SETVIEWPORTORGEX record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETVIEWPORTORGEX_print(const char *contents, drawingStates *states) {
    core7_print("U_EMRSETVIEWPORTORGEX", "ptlOrigin:", "", contents, states);
}

// U_EMRSETBRUSHORGEX        13
/**
  \brief Print a pointer to a U_EMR_SETBRUSHORGEX record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETBRUSHORGEX_print(const char *contents, drawingStates *states) {
    core7_print("U_EMRSETBRUSHORGEX", "ptlOrigin:", "", contents, states);
}

// U_EMREOF                  14
/**
  \brief Print a pointer to a U_EMR_EOF record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMREOF_print(const char *contents, drawingStates *states) {
    PU_EMREOF pEmr = (PU_EMREOF)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMREOF)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   cbPalEntries:   %u\n", pEmr->cbPalEntries);
    verbose_printf("   offPalEntries:  %u\n", pEmr->offPalEntries);
    if (pEmr->cbPalEntries) {
        verbose_printf("      PE:");
        logpalette_print(states,
                         (PU_LOGPALETTE)(contents + pEmr->offPalEntries));
        verbose_printf("\n");
    }
}

// U_EMRSETPIXELV            15
/**
  \brief Print a pointer to a U_EMR_SETPIXELV record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETPIXELV_print(const char *contents, drawingStates *states) {
    PU_EMRSETPIXELV pEmr = (PU_EMRSETPIXELV)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRSETPIXELV)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   ptlPixel:       ");
    pointl_print(states, pEmr->ptlPixel);
    verbose_printf("\n");
    verbose_printf("   crColor:        ");
    colorref_print(states, pEmr->crColor);
    verbose_printf("\n");
}

// U_EMRSETMAPPERFLAGS       16
/**
  \brief Print a pointer to a U_EMR_SETMAPPERFLAGS record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETMAPPERFLAGS_print(const char *contents, drawingStates *states) {
    PU_EMRSETMAPPERFLAGS pEmr = (PU_EMRSETMAPPERFLAGS)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRSETMAPPERFLAGS)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   dwFlags:        0x%8.8X\n", pEmr->dwFlags);
}

// U_EMRSETMAPMODE           17
/**
  \brief Print a pointer to a U_EMR_SETMAPMODE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETMAPMODE_print(const char *contents, drawingStates *states) {
    char *name = "U_EMRSETMAPMODE";
    char *label = "iMode:";
    UNUSED(name);
    PU_EMRSETMAPMODE pEmr = (PU_EMRSETMAPMODE)(contents);
    switch (pEmr->iMode) {
    case U_MM_TEXT:
        verbose_printf("   %-15s TEXT\n", label);
        break;
    case U_MM_LOMETRIC:
        verbose_printf("   %-15s LOMETRIC\n", label);
        break;
    case U_MM_HIMETRIC:
        verbose_printf("   %-15s HIMETRIC\n", label);
        break;
    case U_MM_LOENGLISH:
        verbose_printf("   %-15s LOENGLISH\n", label);
        break;
    case U_MM_HIENGLISH:
        verbose_printf("   %-15s HIENGLISH\n", label);
        break;
    case U_MM_TWIPS:
        verbose_printf("   %-15s TWIPS\n", label);
        break;
    case U_MM_ISOTROPIC:
        verbose_printf("   %-15s ISOTROPIC\n", label);
        break;
    case U_MM_ANISOTROPIC:
        verbose_printf("   %-15s ANISOTROPIC\n", label);
        break;
    default:
        break;
        verbose_printf("   %-15s 0x%8.8X\n", label, pEmr->iMode);
    }
}

// U_EMRSETBKMODE            18
/**
  \brief Print a pointer to a U_EMR_SETBKMODE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETBKMODE_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETBKMODE", "iMode:", contents, states);
}

// U_EMRSETPOLYFILLMODE      19
/**
  \brief Print a pointer to a U_EMR_SETPOLYFILLMODE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETPOLYFILLMODE_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETPOLYFILLMODE", "iMode:", contents, states);
}

// U_EMRSETROP2              20
/**
  \brief Print a pointer to a U_EMR_SETROP2 record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETROP2_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETROP2", "dwRop:", contents, states);
}

// U_EMRSETSTRETCHBLTMODE    21
/**
  \brief Print a pointer to a U_EMR_SETSTRETCHBLTMODE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETSTRETCHBLTMODE_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETSTRETCHBLTMODE", "iMode:", contents, states);
}

// U_EMRSETTEXTALIGN         22
/**
  \brief Print a pointer to a U_EMR_SETTEXTALIGN record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETTEXTALIGN_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETTEXTALIGN", "iMode:", contents, states);
}

// U_EMRSETCOLORADJUSTMENT   23
/**
  \brief Print a pointer to a U_EMR_SETCOLORADJUSTMENT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETCOLORADJUSTMENT_print(const char *contents,
                                   drawingStates *states) {
    PU_EMRSETCOLORADJUSTMENT pEmr = (PU_EMRSETCOLORADJUSTMENT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRSETCOLORADJUSTMENT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   ColorAdjustment:");
    coloradjustment_print(states, pEmr->ColorAdjustment);
    verbose_printf("\n");
}

// U_EMRSETTEXTCOLOR         24
/**
  \brief Print a pointer to a U_EMR_SETTEXTCOLOR record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETTEXTCOLOR_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETTEXTCOLOR", "crColor:", contents, states);
}

// U_EMRSETBKCOLOR           25
/**
  \brief Print a pointer to a U_EMR_SETBKCOLOR record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETBKCOLOR_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETBKCOLOR", "crColor:", contents, states);
}

// U_EMROFFSETCLIPRGN        26
/**
  \brief Print a pointer to a U_EMR_OFFSETCLIPRGN record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMROFFSETCLIPRGN_print(const char *contents, drawingStates *states) {
    core7_print("U_EMROFFSETCLIPRGN", "ptl:", "", contents, states);
}

// U_EMRMOVETOEX             27
/**
  \brief Print a pointer to a U_EMR_MOVETOEX record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRMOVETOEX_print(const char *contents, drawingStates *states) {
    core7_print("U_EMRMOVETOEX", "ptl:", "", contents, states);
}

// U_EMRSETMETARGN           28
/**
  \brief Print a pointer to a U_EMR_SETMETARGN record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETMETARGN_print(const char *contents, drawingStates *states) {
    UNUSED(contents);
}

// U_EMREXCLUDECLIPRECT      29
/**
  \brief Print a pointer to a U_EMR_EXCLUDECLIPRECT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMREXCLUDECLIPRECT_print(const char *contents, drawingStates *states) {
    core4_print("U_EMREXCLUDECLIPRECT", contents, states);
}

// U_EMRINTERSECTCLIPRECT    30
/**
  \brief Print a pointer to a U_EMR_INTERSECTCLIPRECT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRINTERSECTCLIPRECT_print(const char *contents, drawingStates *states) {
    core4_print("U_EMRINTERSECTCLIPRECT", contents, states);
}

// U_EMRSCALEVIEWPORTEXTEX   31
/**
  \brief Print a pointer to a U_EMR_SCALEVIEWPORTEXTEX record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSCALEVIEWPORTEXTEX_print(const char *contents,
                                   drawingStates *states) {
    core4_print("U_EMRSCALEVIEWPORTEXTEX", contents, states);
}

// U_EMRSCALEWINDOWEXTEX     32
/**
  \brief Print a pointer to a U_EMR_SCALEWINDOWEXTEX record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSCALEWINDOWEXTEX_print(const char *contents, drawingStates *states) {
    core4_print("U_EMRSCALEWINDOWEXTEX", contents, states);
}

// U_EMRSAVEDC               33
/**
  \brief Print a pointer to a U_EMR_SAVEDC record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSAVEDC_print(const char *contents, drawingStates *states) {
    saveDeviceContext(states);
    UNUSED(contents);
}

// U_EMRRESTOREDC            34
/**
  \brief Print a pointer to a U_EMR_RESTOREDC record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRRESTOREDC_print(const char *contents, drawingStates *states) {
    PU_EMRSETMAPMODE pEmr = (PU_EMRSETMAPMODE)(contents);
    restoreDeviceContext(states, pEmr->iMode);
    core3_print("U_EMRRESTOREDC", "iRelative:", contents, states);
}

// U_EMRSETWORLDTRANSFORM    35
/**
  \brief Print a pointer to a U_EMR_SETWORLDTRANSFORM record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETWORLDTRANSFORM_print(const char *contents, drawingStates *states) {
    PU_EMRSETWORLDTRANSFORM pEmr = (PU_EMRSETWORLDTRANSFORM)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRSETWORLDTRANSFORM)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   xform:");
    xform_print(states, pEmr->xform);
    verbose_printf("\n");
}

// U_EMRMODIFYWORLDTRANSFORM 36
/**
  \brief Print a pointer to a U_EMR_MODIFYWORLDTRANSFORM record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRMODIFYWORLDTRANSFORM_print(const char *contents,
                                     drawingStates *states) {
    PU_EMRMODIFYWORLDTRANSFORM pEmr = (PU_EMRMODIFYWORLDTRANSFORM)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRMODIFYWORLDTRANSFORM)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   xform:          ");
    xform_print(states, pEmr->xform);
    verbose_printf("\n");

    switch (pEmr->iMode) {
    case U_MWT_IDENTITY: {
        verbose_printf("   iMode:          U_MWT_IDENTITY\n");
        break;
    }
    case U_MWT_LEFTMULTIPLY: {
        verbose_printf("   iMode:          U_MWT_LEFTMULTIPLY\n");
        break;
    }
    case U_MWT_RIGHTMULTIPLY: {
        verbose_printf("   iMode:          U_MWT_RIGHTMULTIPLY\n");
        break;
    }
    case U_MWT_SET: {
        verbose_printf("   iMode:          U_MWT_SET\n");
        break;
    }
    default:
        break;
    }
}

// U_EMRSELECTOBJECT         37
/**
  \brief Print a pointer to a U_EMR_SELECTOBJECT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSELECTOBJECT_print(const char *contents, drawingStates *states) {
    PU_EMRSELECTOBJECT pEmr = (PU_EMRSELECTOBJECT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRSELECTOBJECT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    uint32_t index = pEmr->ihObject;
    if (index & U_STOCK_OBJECT) {
        switch (index) {
        case (U_WHITE_BRUSH):
            verbose_printf("   StockObject:    WHITE_BRUSH\n");
            break;
        case (U_LTGRAY_BRUSH):
            verbose_printf("   StockObject:    LTGRAY_BRUSH\n");
            break;
        case (U_GRAY_BRUSH):
            verbose_printf("   StockObject:    GRAY_BRUSH\n");
            break;
        case (U_DKGRAY_BRUSH):
            verbose_printf("   StockObject:    DKGRAY_BRUSH\n");
            break;
        case (U_BLACK_BRUSH):
            verbose_printf("   StockObject:    BLACK_BRUSH\n");
            break;
        case (U_NULL_BRUSH):
            verbose_printf("   StockObject:    NULL_BRUSH\n");
            break;
        case (U_WHITE_PEN):
            verbose_printf("   StockObject:    WHITE_PEN\n");
            break;
        case (U_BLACK_PEN):
            verbose_printf("   StockObject:    BLACK_PEN\n");
            break;
        case (U_NULL_PEN):
            verbose_printf("   StockObject:    NULL_PEN\n");
            break;
        case (U_OEM_FIXED_FONT):
            verbose_printf("   StockObject:    OEM_FIXED_FONT\n");
            break;
        case (U_ANSI_FIXED_FONT):
            verbose_printf("   StockObject:    ANSI_FIXED_FONT\n");
            break;
        case (U_ANSI_VAR_FONT):
            verbose_printf("   StockObject:    ANSI_VAR_FONT\n");
            break;
        case (U_SYSTEM_FONT):
            verbose_printf("   StockObject:    SYSTEM_FONT\n");
            break;
        case (U_DEVICE_DEFAULT_FONT):
            verbose_printf("   StockObject:    DEFAULT_FONT\n");
            break;
        case (U_DEFAULT_PALETTE):
            verbose_printf("   StockObject:    DEFAULT_PALETTE\n");
            break;
        case (U_SYSTEM_FIXED_FONT):
            verbose_printf("   StockObject:    DEFAULT_FIXED_FONT\n");
            break;
        case (U_DEFAULT_GUI_FONT):
            verbose_printf("   StockObject:    DEFAULT_GUI_FONT\n");
            break;
        default:
            verbose_printf("   StockObject:    0x%8.8X\n", pEmr->ihObject);
            break;
        }
    } else {
        verbose_printf("   ihObject:       %u\n", pEmr->ihObject);
    }
}

// U_EMRCREATEPEN            38
/**
  \brief Print a pointer to a U_EMR_CREATEPEN record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRCREATEPEN_print(const char *contents, drawingStates *states) {
    PU_EMRCREATEPEN pEmr = (PU_EMRCREATEPEN)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRCREATEPEN)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   ihPen:          %u\n", pEmr->ihPen);
    verbose_printf("   lopn:           ");
    logpen_print(states, pEmr->lopn);
    verbose_printf("\n");
}

// U_EMRCREATEBRUSHINDIRECT  39
/**
  \brief Print a pointer to a U_EMR_CREATEBRUSHINDIRECT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRCREATEBRUSHINDIRECT_print(const char *contents,
                                    drawingStates *states) {
    PU_EMRCREATEBRUSHINDIRECT pEmr = (PU_EMRCREATEBRUSHINDIRECT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRCREATEBRUSHINDIRECT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   ihBrush:        %u\n", pEmr->ihBrush);
    verbose_printf("   lb:             ");
    logbrush_print(states, pEmr->lb);
    verbose_printf("\n");

    if (pEmr->lb.lbStyle == U_BS_SOLID) {
    } else if (pEmr->lb.lbStyle == U_BS_HATCHED) {
    }
}

// U_EMRDELETEOBJECT         40
/**
  \brief Print a pointer to a U_EMR_DELETEOBJECT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRDELETEOBJECT_print(const char *contents, drawingStates *states) {
    PU_EMRDELETEOBJECT pEmr = (PU_EMRDELETEOBJECT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRDELETEOBJECT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   ihObject:       %u\n", pEmr->ihObject);
}

// U_EMRANGLEARC             41
/**
  \brief Print a pointer to a U_EMR_ANGLEARC record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRANGLEARC_print(const char *contents, drawingStates *states) {
    PU_EMRANGLEARC pEmr = (PU_EMRANGLEARC)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRANGLEARC)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   ptlCenter:      ");
    pointl_print(states, pEmr->ptlCenter);
    verbose_printf("\n");
    verbose_printf("   nRadius:        %u\n", pEmr->nRadius);
    verbose_printf("   eStartAngle:    %f\n", pEmr->eStartAngle);
    verbose_printf("   eSweepAngle:    %f\n", pEmr->eSweepAngle);
}

// U_EMRELLIPSE              42
/**
  \brief Print a pointer to a U_EMR_ELLIPSE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRELLIPSE_print(const char *contents, drawingStates *states) {
    core4_print("U_EMRELLIPSE", contents, states);
}

// U_EMRRECTANGLE            43
/**
  \brief Print a pointer to a U_EMR_RECTANGLE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRRECTANGLE_print(const char *contents, drawingStates *states) {
    core4_print("U_EMRRECTANGLE", contents, states);
}

// U_EMRROUNDRECT            44
/**
  \brief Print a pointer to a U_EMR_ROUNDRECT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRROUNDRECT_print(const char *contents, drawingStates *states) {
    PU_EMRROUNDRECT pEmr = (PU_EMRROUNDRECT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRROUNDRECT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   rclBox:         ");
    rectl_print(states, pEmr->rclBox);
    verbose_printf("\n");
    verbose_printf("   szlCorner:      ");
    sizel_print(states, pEmr->szlCorner);
    verbose_printf("\n");
}

// U_EMRARC                  45
/**
  \brief Print a pointer to a U_EMR_ARC record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRARC_print(const char *contents, drawingStates *states) {
    core9_print("U_EMRARC", contents, states);
}

// U_EMRCHORD                46
/**
  \brief Print a pointer to a U_EMR_CHORD record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRCHORD_print(const char *contents, drawingStates *states) {
    core9_print("U_EMRCHORD", contents, states);
}

// U_EMRPIE                  47
/**
  \brief Print a pointer to a U_EMR_PIE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPIE_print(const char *contents, drawingStates *states) {
    core9_print("U_EMRPIE", contents, states);
}

// U_EMRSELECTPALETTE        48
/**
  \brief Print a pointer to a U_EMR_SELECTPALETTE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSELECTPALETTE_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSELECTPALETTE", "ihPal:", contents, states);
}

// U_EMRCREATEPALETTE        49
/**
  \brief Print a pointer to a U_EMR_CREATEPALETTE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRCREATEPALETTE_print(const char *contents, drawingStates *states) {
    PU_EMRCREATEPALETTE pEmr = (PU_EMRCREATEPALETTE)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRCREATEPALETTE)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   ihPal:          %u\n", pEmr->ihPal);
    verbose_printf("   lgpl:           ");
    logpalette_print(states, (PU_LOGPALETTE) & (pEmr->lgpl));
    verbose_printf("\n");
}

// U_EMRSETPALETTEENTRIES    50
/**
  \brief Print a pointer to a U_EMR_SETPALETTEENTRIES record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETPALETTEENTRIES_print(const char *contents, drawingStates *states) {
    unsigned int i;
    PU_EMRSETPALETTEENTRIES pEmr = (PU_EMRSETPALETTEENTRIES)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRSETPALETTEENTRIES)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   ihPal:          %u\n", pEmr->ihPal);
    verbose_printf("   iStart:         %u\n", pEmr->iStart);
    verbose_printf("   cEntries:       %u\n", pEmr->cEntries);
    if (pEmr->cEntries) {
        verbose_printf("      PLTEntries:");
        PU_LOGPLTNTRY aPalEntries = (PU_LOGPLTNTRY) & (pEmr->aPalEntries);
        IF_MEM_UNSAFE_PRINT_AND_RETURN(
            aPalEntries, pEmr->cEntries * sizeof(U_LOGPLTNTRY), blimit);
        for (i = 0; i < pEmr->cEntries; i++) {
            verbose_printf("%d:", i);
            logpltntry_print(states, aPalEntries[i]);
        }
        verbose_printf("\n");
    }
}

// U_EMRRESIZEPALETTE        51
/**
  \brief Print a pointer to a U_EMR_RESIZEPALETTE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRRESIZEPALETTE_print(const char *contents, drawingStates *states) {
    core7_print("U_EMRRESIZEPALETTE", "ihPal:", "cEntries", contents, states);
}

// U_EMRREALIZEPALETTE       52
/**
  \brief Print a pointer to a U_EMR_REALIZEPALETTE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRREALIZEPALETTE_print(const char *contents, drawingStates *states) {
    UNUSED(contents);
}

// U_EMREXTFLOODFILL         53
/**
  \brief Print a pointer to a U_EMR_EXTFLOODFILL record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMREXTFLOODFILL_print(const char *contents, drawingStates *states) {
    PU_EMREXTFLOODFILL pEmr = (PU_EMREXTFLOODFILL)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMREXTFLOODFILL)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   ptlStart:       ");
    pointl_print(states, pEmr->ptlStart);
    verbose_printf("\n");
    verbose_printf("   crColor:        ");
    colorref_print(states, pEmr->crColor);
    verbose_printf("\n");
    verbose_printf("   iMode:          %u\n", pEmr->iMode);
}

// U_EMRLINETO               54
/**
  \brief Print a pointer to a U_EMR_LINETO record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRLINETO_print(const char *contents, drawingStates *states) {
    core7_print("U_EMRLINETO", "ptl:", "", contents, states);
}

// U_EMRARCTO                55
/**
  \brief Print a pointer to a U_EMR_ARCTO record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRARCTO_print(const char *contents, drawingStates *states) {
    core9_print("U_EMRARCTO", contents, states);
}

// U_EMRPOLYDRAW             56
/**
  \brief Print a pointer to a U_EMR_POLYDRAW record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYDRAW_print(const char *contents, drawingStates *states) {
    unsigned int i;
    PU_EMRPOLYDRAW pEmr = (PU_EMRPOLYDRAW)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRPOLYDRAW)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   cptl:           %d\n", pEmr->cptl);
    verbose_printf("   Points:         ");
    IF_MEM_UNSAFE_PRINT_AND_RETURN(pEmr->aptl, pEmr->cptl * sizeof(U_POINTL),
                                   blimit);
    for (i = 0; i < pEmr->cptl; i++) {
        verbose_printf("[%d]:", i);
        pointl_print(states, pEmr->aptl[i]);
    }
    verbose_printf("\n");
    verbose_printf("   Types:          ");
    const char *abTypes =
        (const char *)pEmr->aptl + pEmr->cptl * sizeof(U_POINTL);
    IF_MEM_UNSAFE_PRINT_AND_RETURN(abTypes, pEmr->cptl, blimit);
    for (i = 0; i < pEmr->cptl; i++) {
        verbose_printf(" [%d]:%u ", i, ((uint8_t *)abTypes)[i]);
    }
    verbose_printf("\n");
}

// U_EMRSETARCDIRECTION      57
/**
  \brief Print a pointer to a U_EMR_SETARCDIRECTION record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETARCDIRECTION_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETARCDIRECTION", "arcDirection:", contents, states);
}

// U_EMRSETMITERLIMIT        58
/**
  \brief Print a pointer to a U_EMR_SETMITERLIMIT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETMITERLIMIT_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETMITERLIMIT", "eMiterLimit:", contents, states);
}

// U_EMRBEGINPATH            59
/**
  \brief Print a pointer to a U_EMR_BEGINPATH record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRBEGINPATH_print(const char *contents, drawingStates *states) {
    UNUSED(contents);
}

// U_EMRENDPATH              60
/**
  \brief Print a pointer to a U_EMR_ENDPATH record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRENDPATH_print(const char *contents, drawingStates *states) {
    UNUSED(contents);
}

// U_EMRCLOSEFIGURE          61
/**
  \brief Print a pointer to a U_EMR_CLOSEFIGURE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRCLOSEFIGURE_print(const char *contents, drawingStates *states) {
    UNUSED(contents);
}

// U_EMRFILLPATH             62
/**
  \brief Print a pointer to a U_EMR_FILLPATH record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRFILLPATH_print(const char *contents, drawingStates *states) {
    // real work done in U_EMRENDPATH
    core4_print("U_EMRFILLPATH", contents, states);
}

// U_EMRSTROKEANDFILLPATH    63
/**
  \brief Print a pointer to a U_EMR_STROKEANDFILLPATH record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSTROKEANDFILLPATH_print(const char *contents, drawingStates *states) {
    // real work done in U_EMRENDPATH
    core4_print("U_EMRSTROKEANDFILLPATH", contents, states);
}

// U_EMRSTROKEPATH           64
/**
  \brief Print a pointer to a U_EMR_STROKEPATH record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSTROKEPATH_print(const char *contents, drawingStates *states) {
    // real work done in U_EMRENDPATH
    core4_print("U_EMRSTROKEPATH", contents, states);
}

// U_EMRFLATTENPATH          65
/**
  \brief Print a pointer to a U_EMR_FLATTENPATH record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRFLATTENPATH_print(const char *contents, drawingStates *states) {
    UNUSED(contents);
}

// U_EMRWIDENPATH            66
/**
  \brief Print a pointer to a U_EMR_WIDENPATH record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRWIDENPATH_print(const char *contents, drawingStates *states) {
    UNUSED(contents);
}

// U_EMRSELECTCLIPPATH       67
/**
  \brief Print a pointer to a U_EMR_SELECTCLIPPATH record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSELECTCLIPPATH_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSELECTCLIPPATH", "iMode:", contents, states);
}

// U_EMRABORTPATH            68
/**
  \brief Print a pointer to a U_EMR_ABORTPATH record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRABORTPATH_print(const char *contents, drawingStates *states) {
    UNUSED(contents);
}

// U_EMRUNDEF69                       69
#define U_EMRUNDEF69_print(A)                                                  \
    U_EMRNOTIMPLEMENTED_print("U_EMRUNDEF69", A) //!< Not implemented.

// U_EMRCOMMENT              70  Comment (any binary data, interpretation is
// program specific)
/**
  \brief Print a pointer to a U_EMR_COMMENT record.
  \param contents   pointer to a location in memory holding the comment record
  \param blimit     size in bytes of the comment record
  \param off        offset in bytes to the first byte in this record

  EMF+ records, if any, are stored in EMF comment records.
  */
void U_EMRCOMMENT_print(const char *contents, drawingStates *states,
                        const char *blimit, size_t off) {
    char *string;
    char *src;
    uint32_t cIdent, cIdent2, cbData;

    PU_EMRCOMMENT pEmr = (PU_EMRCOMMENT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRCOMMENT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    //    const char *blimit = contents + pEmr->emr.nSize;

    /* There are several different types of comments */

    IF_MEM_UNSAFE_PRINT_AND_RETURN(contents, sizeof(U_EMRCOMMENT), blimit);
    cbData = pEmr->cbData;
    verbose_printf("   cbData:         %d\n", cbData);
    IF_MEM_UNSAFE_PRINT_AND_RETURN(
        contents, sizeof(U_EMR) + sizeof(U_CBDATA) + cbData, blimit);
    src = (char *)&(pEmr->Data); // default
    if (cbData >= 4) {
        /* Since the comment is just a big bag of bytes the emf endian code
           cannot safely touch
           any of its payload.  This is the only record type with that
           limitation.  Try to determine
           what the contents are even if more byte swapping is required. */
        cIdent = *(uint32_t *)(src);
        if (U_BYTE_SWAP) {
            U_swap4(&(cIdent), 1);
        }
        if (cIdent == U_EMR_COMMENT_PUBLIC) {
            verbose_printf("   cIdent:         Public\n");
            PU_EMRCOMMENT_PUBLIC pEmrp = (PU_EMRCOMMENT_PUBLIC)pEmr;
            IF_MEM_UNSAFE_PRINT_AND_RETURN(contents,
                                           sizeof(U_EMRCOMMENT_PUBLIC), blimit);
            cIdent2 = pEmrp->pcIdent;
            if (U_BYTE_SWAP) {
                U_swap4(&(cIdent2), 1);
            }
            verbose_printf("   pcIdent:        0x%8.8x\n", cIdent2);
            src = (char *)&(pEmrp->Data);
            cbData -= 8;
        } else if (cIdent == U_EMR_COMMENT_SPOOL) {
            verbose_printf("   cIdent:         Spool\n");
            PU_EMRCOMMENT_SPOOL pEmrs = (PU_EMRCOMMENT_SPOOL)pEmr;
            IF_MEM_UNSAFE_PRINT_AND_RETURN(contents, sizeof(U_EMRCOMMENT_SPOOL),
                                           blimit);
            cIdent2 = pEmrs->esrIdent;
            if (U_BYTE_SWAP) {
                U_swap4(&(cIdent2), 1);
            }
            verbose_printf("   esrIdent:       0x%8.8x\n", cIdent2);
            src = (char *)&(pEmrs->Data);
            cbData -= 8;
        } else if (cIdent == U_EMR_COMMENT_EMFPLUSRECORD) {
            verbose_printf("   cIdent:         EMF+\n");
            PU_EMRCOMMENT_EMFPLUS pEmrpl = (PU_EMRCOMMENT_EMFPLUS)pEmr;
            IF_MEM_UNSAFE_PRINT_AND_RETURN(
                contents, sizeof(U_EMRCOMMENT_EMFPLUS), blimit);
            src = (char *)&(pEmrpl->Data);
            return;

        } else {
            verbose_printf(
                "   cIdent:         not (Public or Spool or EMF+)\n");
        }
    }
    if (cbData) { // The data may not be printable, but try it just in case
        string = (char *)malloc(cbData + 1);
        (void)strncpy(string, src, cbData);
        string[cbData] =
            '\0'; // it might not be terminated - it might not even be text!
        // verbose_printf("   Data:           <%s>\n",string);
        free(string);
    }
}

// U_EMRFILLRGN              71
/**
  \brief Print a pointer to a U_EMR_FILLRGN record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRFILLRGN_print(const char *contents, drawingStates *states) {
    PU_EMRFILLRGN pEmr = (PU_EMRFILLRGN)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRFILLRGN)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   cbRgnData:      %u\n", pEmr->cbRgnData);
    verbose_printf("   ihBrush:        %u\n", pEmr->ihBrush);
    const char *minptr =
        MAKE_MIN_PTR(((const char *)&pEmr->RgnData + pEmr->cbRgnData +
                      sizeof(U_RGNDATAHEADER)),
                     blimit);
    verbose_printf("   RegionData: ");
    rgndata_print(states, pEmr->RgnData, minptr);
    verbose_printf("\n");
}

// U_EMRFRAMERGN             72
/**
  \brief Print a pointer to a U_EMR_FRAMERGN record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRFRAMERGN_print(const char *contents, drawingStates *states) {
    PU_EMRFRAMERGN pEmr = (PU_EMRFRAMERGN)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRFRAMERGN)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   cbRgnData:      %u\n", pEmr->cbRgnData);
    verbose_printf("   ihBrush:        %u\n", pEmr->ihBrush);
    verbose_printf("   szlStroke:      ");
    sizel_print(states, pEmr->szlStroke);
    verbose_printf("\n");
    const char *minptr =
        MAKE_MIN_PTR(((const char *)&pEmr->RgnData + pEmr->cbRgnData), blimit);
    verbose_printf("   RegionData: ");
    rgndata_print(states, pEmr->RgnData, minptr);
    verbose_printf("\n");
}

// U_EMRINVERTRGN            73
/**
  \brief Print a pointer to a U_EMR_INVERTRGN record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRINVERTRGN_print(const char *contents, drawingStates *states) {
    core11_print("U_EMRINVERTRGN", contents, states);
}

// U_EMRPAINTRGN             74
/**
  \brief Print a pointer to a U_EMR_PAINTRGN record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPAINTRGN_print(const char *contents, drawingStates *states) {
    core11_print("U_EMRPAINTRGN", contents, states);
}

// U_EMREXTSELECTCLIPRGN     75
/**
  \brief Print a pointer to a U_EMR_EXTSELECTCLIPRGN record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMREXTSELECTCLIPRGN_print(const char *contents, drawingStates *states) {
    PU_EMREXTSELECTCLIPRGN pEmr = (PU_EMREXTSELECTCLIPRGN)(contents);
    if (pEmr->emr.nSize < U_SIZE_EMREXTSELECTCLIPRGN) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   cbRgnData:      %u\n", pEmr->cbRgnData);
    verbose_printf("   iMode:          %u\n", pEmr->iMode);
    if (pEmr->iMode == U_RGN_COPY && !pEmr->cbRgnData) {
        verbose_printf("   RegionData: none (Clip region becomes NULL)\n");
    } else {
        const char *minptr = MAKE_MIN_PTR(
            ((const char *)&pEmr->RgnData + pEmr->cbRgnData), blimit);
        verbose_printf("   RegionData: ");
        rgndata_print(states, pEmr->RgnData, minptr);
        verbose_printf("\n");
    }
}

// U_EMRBITBLT               76
/**
  \brief Print a pointer to a U_EMR_BITBLT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRBITBLT_print(const char *contents, drawingStates *states) {
    PU_EMRBITBLT pEmr = (PU_EMRBITBLT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRBITBLT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   Dest:           ");
    pointl_print(states, pEmr->Dest);
    verbose_printf("\n");
    verbose_printf("   cDest:          ");
    pointl_print(states, pEmr->cDest);
    verbose_printf("\n");
    verbose_printf("   dwRop :         0x%8.8X\n", pEmr->dwRop);
    verbose_printf("   Src:            ");
    pointl_print(states, pEmr->Src);
    verbose_printf("\n");
    verbose_printf("   xformSrc:       ");
    xform_print(states, pEmr->xformSrc);
    verbose_printf("\n");
    verbose_printf("   crBkColorSrc:   ");
    colorref_print(states, pEmr->crBkColorSrc);
    verbose_printf("\n");
    verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc);
    verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc);
    verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc);
    if (pEmr->cbBmiSrc) {
        verbose_printf("      bitmap:      ");
        bitmapinfo_print(states, contents + pEmr->offBmiSrc, blimit);
        verbose_printf("\n");
    }
    verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc);
    verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc);
}

// U_EMRSTRETCHBLT           77
/**
  \brief Print a pointer to a U_EMR_STRETCHBLT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSTRETCHBLT_print(const char *contents, drawingStates *states) {
    PU_EMRSTRETCHBLT pEmr = (PU_EMRSTRETCHBLT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRSTRETCHBLT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   Dest:           ");
    pointl_print(states, pEmr->Dest);
    verbose_printf("\n");
    verbose_printf("   cDest:          ");
    pointl_print(states, pEmr->cDest);
    verbose_printf("\n");
    verbose_printf("   dwRop :         0x%8.8X\n", pEmr->dwRop);
    verbose_printf("   Src:            ");
    pointl_print(states, pEmr->Src);
    verbose_printf("\n");
    verbose_printf("   xformSrc:       ");
    xform_print(states, pEmr->xformSrc);
    verbose_printf("\n");
    verbose_printf("   crBkColorSrc:   ");
    colorref_print(states, pEmr->crBkColorSrc);
    verbose_printf("\n");
    verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc);
    verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc);
    verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc);
    if (pEmr->cbBmiSrc) {
        verbose_printf("      bitmap:      ");
        bitmapinfo_print(states, contents + pEmr->offBmiSrc, blimit);
        verbose_printf("\n");
    }
    verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc);
    verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc);
    verbose_printf("   cSrc:           ");
    pointl_print(states, pEmr->cSrc);
    verbose_printf("\n");
}

// U_EMRMASKBLT              78
/**
  \brief Print a pointer to a U_EMR_MASKBLT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRMASKBLT_print(const char *contents, drawingStates *states) {
    PU_EMRMASKBLT pEmr = (PU_EMRMASKBLT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRMASKBLT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   Dest:           ");
    pointl_print(states, pEmr->Dest);
    verbose_printf("\n");
    verbose_printf("   cDest:          ");
    pointl_print(states, pEmr->cDest);
    verbose_printf("\n");
    verbose_printf("   dwRop :         0x%8.8X\n", pEmr->dwRop);
    verbose_printf("   Src:            ");
    pointl_print(states, pEmr->Src);
    verbose_printf("\n");
    verbose_printf("   xformSrc:       ");
    xform_print(states, pEmr->xformSrc);
    verbose_printf("\n");
    verbose_printf("   crBkColorSrc:   ");
    colorref_print(states, pEmr->crBkColorSrc);
    verbose_printf("\n");
    verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc);
    verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc);
    verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc);
    if (pEmr->cbBmiSrc) {
        verbose_printf("      Src bitmap:  ");
        bitmapinfo_print(states, contents + pEmr->offBmiSrc, blimit);
        verbose_printf("\n");
    }
    verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc);
    verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc);
    verbose_printf("   Mask:           ");
    pointl_print(states, pEmr->Mask);
    verbose_printf("\n");
    verbose_printf("   iUsageMask:     %u\n", pEmr->iUsageMask);
    verbose_printf("   offBmiMask:     %u\n", pEmr->offBmiMask);
    verbose_printf("   cbBmiMask:      %u\n", pEmr->cbBmiMask);
    if (pEmr->cbBmiMask) {
        verbose_printf("      Mask bitmap: ");
        bitmapinfo_print(states, contents + pEmr->offBmiMask, blimit);
        verbose_printf("\n");
    }
    verbose_printf("   offBitsMask:    %u\n", pEmr->offBitsMask);
    verbose_printf("   cbBitsMask:     %u\n", pEmr->cbBitsMask);
}

// U_EMRPLGBLT               79
/**
  \brief Print a pointer to a U_EMR_PLGBLT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPLGBLT_print(const char *contents, drawingStates *states) {
    PU_EMRPLGBLT pEmr = (PU_EMRPLGBLT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRPLGBLT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   aptlDst(UL):    ");
    pointl_print(states, pEmr->aptlDst[0]);
    verbose_printf("\n");
    verbose_printf("   aptlDst(UR):    ");
    pointl_print(states, pEmr->aptlDst[1]);
    verbose_printf("\n");
    verbose_printf("   aptlDst(LL):    ");
    pointl_print(states, pEmr->aptlDst[2]);
    verbose_printf("\n");
    verbose_printf("   Src:            ");
    pointl_print(states, pEmr->Src);
    verbose_printf("\n");
    verbose_printf("   cSrc:           ");
    pointl_print(states, pEmr->cSrc);
    verbose_printf("\n");
    verbose_printf("   xformSrc:       ");
    xform_print(states, pEmr->xformSrc);
    verbose_printf("\n");
    verbose_printf("   crBkColorSrc:   ");
    colorref_print(states, pEmr->crBkColorSrc);
    verbose_printf("\n");
    verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc);
    verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc);
    verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc);
    if (pEmr->cbBmiSrc) {
        verbose_printf("      Src bitmap:  ");
        bitmapinfo_print(states, contents + pEmr->offBmiSrc, blimit);
        verbose_printf("\n");
    }
    verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc);
    verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc);
    verbose_printf("   Mask:           ");
    pointl_print(states, pEmr->Mask);
    verbose_printf("\n");
    verbose_printf("   iUsageMsk:      %u\n", pEmr->iUsageMask);
    verbose_printf("   offBmiMask:     %u\n", pEmr->offBmiMask);
    verbose_printf("   cbBmiMask:      %u\n", pEmr->cbBmiMask);
    if (pEmr->cbBmiMask) {
        verbose_printf("      Mask bitmap: ");
        bitmapinfo_print(states, contents + pEmr->offBmiMask, blimit);
        verbose_printf("\n");
    }
    verbose_printf("   offBitsMask:    %u\n", pEmr->offBitsMask);
    verbose_printf("   cbBitsMask:     %u\n", pEmr->cbBitsMask);
}

// U_EMRSETDIBITSTODEVICE    80
/**
  \brief Print a pointer to a U_EMRSETDIBITSTODEVICE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETDIBITSTODEVICE_print(const char *contents, drawingStates *states) {
    PU_EMRSETDIBITSTODEVICE pEmr = (PU_EMRSETDIBITSTODEVICE)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRSETDIBITSTODEVICE)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   Dest:           ");
    pointl_print(states, pEmr->Dest);
    verbose_printf("\n");
    verbose_printf("   Src:            ");
    pointl_print(states, pEmr->Src);
    verbose_printf("\n");
    verbose_printf("   cSrc:           ");
    pointl_print(states, pEmr->cSrc);
    verbose_printf("\n");
    verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc);
    verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc);
    if (pEmr->cbBmiSrc) {
        verbose_printf("      Src bitmap:  ");
        bitmapinfo_print(states, contents + pEmr->offBmiSrc, blimit);
        verbose_printf("\n");
    }
    verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc);
    verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc);
    verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc);
    verbose_printf("   iStartScan:     %u\n", pEmr->iStartScan);
    verbose_printf("   cScans :        %u\n", pEmr->cScans);
}

// U_EMRSTRETCHDIBITS        81
/**
  \brief Print a pointer to a U_EMR_STRETCHDIBITS record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSTRETCHDIBITS_print(const char *contents, drawingStates *states) {
    PU_EMRSTRETCHDIBITS pEmr = (PU_EMRSTRETCHDIBITS)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRSTRETCHDIBITS)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   Dest:           ");
    pointl_print(states, pEmr->Dest);
    verbose_printf("\n");
    verbose_printf("   Src:            ");
    pointl_print(states, pEmr->Src);
    verbose_printf("\n");
    verbose_printf("   cSrc:           ");
    pointl_print(states, pEmr->cSrc);
    verbose_printf("\n");
    verbose_printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc);
    verbose_printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc);
    if (pEmr->cbBmiSrc) {
        verbose_printf("      Src bitmap:  ");
        bitmapinfo_print(states, contents + pEmr->offBmiSrc, blimit);
        verbose_printf("\n");
    }
    verbose_printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc);
    verbose_printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc);
    verbose_printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc);
    verbose_printf("   dwRop :         0x%8.8X\n", pEmr->dwRop);
    verbose_printf("   cDest:          ");
    pointl_print(states, pEmr->cDest);
    verbose_printf("\n");
}

// U_EMREXTCREATEFONTINDIRECTW_print    82
/**
  \brief Print a pointer to a U_EMR_EXTCREATEFONTINDIRECTW record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMREXTCREATEFONTINDIRECTW_print(const char *contents,
                                       drawingStates *states) {
    PU_EMREXTCREATEFONTINDIRECTW pEmr =
        (PU_EMREXTCREATEFONTINDIRECTW)(contents);
    if (pEmr->emr.nSize <
        U_SIZE_EMREXTCREATEFONTINDIRECTW_LOGFONT) { // smallest variant
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   ihFont:         %u\n", pEmr->ihFont);
    verbose_printf("   Font:           ");
    if (pEmr->emr.nSize ==
        U_SIZE_EMREXTCREATEFONTINDIRECTW_LOGFONT_PANOSE) { // holds
                                                           // logfont_panose
        IF_MEM_UNSAFE_PRINT_AND_RETURN(&(pEmr->elfw), sizeof(U_PANOSE), blimit);
        logfont_panose_print(states, pEmr->elfw);
    } else { // holds logfont or logfontExDv.  The latter isn't supported but it
             // starts with logfont, so use that
        IF_MEM_UNSAFE_PRINT_AND_RETURN(&(pEmr->elfw), sizeof(U_LOGFONT),
                                       blimit);
        logfont_print(states, *(PU_LOGFONT) & (pEmr->elfw));
    }
    verbose_printf("\n");
}

// U_EMREXTTEXTOUTA          83
/**
  \brief Print a pointer to a U_EMR_EXTTEXTOUTA record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMREXTTEXTOUTA_print(const char *contents, drawingStates *states) {
    core8_print("U_EMREXTTEXTOUTA", contents, states, 0);
}

// U_EMREXTTEXTOUTW          84
/**
  \brief Print a pointer to a U_EMR_EXTTEXTOUTW record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMREXTTEXTOUTW_print(const char *contents, drawingStates *states) {
    core8_print("U_EMREXTTEXTOUTW", contents, states, 1);
}

// U_EMRPOLYBEZIER16         85
/**
  \brief Print a pointer to a U_EMR_POLYBEZIER16 record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYBEZIER16_print(const char *contents, drawingStates *states) {
    core6_print("U_EMRPOLYBEZIER16", contents, states);
}

// U_EMRPOLYGON16            86
/**
  \brief Print a pointer to a U_EMR_POLYGON16 record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYGON16_print(const char *contents, drawingStates *states) {
    core6_print("U_EMRPOLYGON16", contents, states);
}

// U_EMRPOLYLINE16           87
/**
  \brief Print a pointer to a U_EMR_POLYLINE16 record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYLINE16_print(const char *contents, drawingStates *states) {
    core6_print("U_EMRPOLYLINE16", contents, states);
}

// U_EMRPOLYBEZIERTO16       88
/**
  \brief Print a pointer to a U_EMR_POLYBEZIERTO16 record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYBEZIERTO16_print(const char *contents, drawingStates *states) {
    core6_print("U_EMRPOLYBEZIERTO16", contents, states);
}

// U_EMRPOLYLINETO16         89
/**
  \brief Print a pointer to a U_EMR_POLYLINETO16 record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYLINETO16_print(const char *contents, drawingStates *states) {
    core6_print("U_EMRPOLYLINETO16", contents, states);
}

// U_EMRPOLYPOLYLINE16       90
/**
  \brief Print a pointer to a U_EMR_POLYPOLYLINE16 record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYPOLYLINE16_print(const char *contents, drawingStates *states) {
    core10_print("U_EMRPOLYPOLYLINE16", contents, states);
}

// U_EMRPOLYPOLYGON16        91
/**
  \brief Print a pointer to a U_EMR_POLYPOLYGON16 record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYPOLYGON16_print(const char *contents, drawingStates *states) {
    core10_print("U_EMRPOLYPOLYLINE16", contents, states);
}

// U_EMRPOLYDRAW16           92
/**
  \brief Print a pointer to a U_EMR_POLYDRAW16 record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPOLYDRAW16_print(const char *contents, drawingStates *states) {
    unsigned int i;
    PU_EMRPOLYDRAW16 pEmr = (PU_EMRPOLYDRAW16)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRPOLYDRAW16)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   cpts:           %d\n", pEmr->cpts);
    verbose_printf("   Points:         ");
    IF_MEM_UNSAFE_PRINT_AND_RETURN(pEmr->apts, pEmr->cpts * sizeof(U_POINT16),
                                   blimit);
    for (i = 0; i < pEmr->cpts; i++) {
        verbose_printf("[%d]:", i);
        point16_print(states, pEmr->apts[i]);
    }
    verbose_printf("\n");
    verbose_printf("   Types:          ");
    const char *abTypes =
        (const char *)pEmr->apts + pEmr->cpts * sizeof(U_POINT16);
    IF_MEM_UNSAFE_PRINT_AND_RETURN(abTypes, pEmr->cpts, blimit);
    for (i = 0; i < pEmr->cpts; i++) {
        verbose_printf(" [%d]:%u ", i, ((uint8_t *)abTypes)[i]);
    }
    verbose_printf("\n");
}

// U_EMRCREATEMONOBRUSH      93
/**
  \brief Print a pointer to a U_EMR_CREATEMONOBRUSH record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRCREATEMONOBRUSH_print(const char *contents, drawingStates *states) {
    core12_print("U_EMRCREATEMONOBRUSH", contents, states);
}

// U_EMRCREATEDIBPATTERNBRUSHPT_print   94
/**
  \brief Print a pointer to a U_EMR_CREATEDIBPATTERNBRUSHPT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRCREATEDIBPATTERNBRUSHPT_print(const char *contents,
                                        drawingStates *states) {
    core12_print("U_EMRCREATEDIBPATTERNBRUSHPT", contents, states);
}

// U_EMREXTCREATEPEN         95
/**
  \brief Print a pointer to a U_EMR_EXTCREATEPEN record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMREXTCREATEPEN_print(const char *contents, drawingStates *states) {
    PU_EMREXTCREATEPEN pEmr = (PU_EMREXTCREATEPEN)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMREXTCREATEPEN)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   ihPen:          %u\n", pEmr->ihPen);
    verbose_printf("   offBmi:         %u\n", pEmr->offBmi);
    verbose_printf("   cbBmi:          %u\n", pEmr->cbBmi);
    if (pEmr->cbBmi) {
        verbose_printf("      bitmap:      ");
        bitmapinfo_print(states, contents + pEmr->offBmi, blimit);
        verbose_printf("\n");
    }
    verbose_printf("   offBits:        %u\n", pEmr->offBits);
    verbose_printf("   cbBits:         %u\n", pEmr->cbBits);
    verbose_printf("   elp:            ");
    extlogpen_print(states, (PU_EXTLOGPEN) & (pEmr->elp));
    verbose_printf("\n");
}

// U_EMRPOLYTEXTOUTA         96 NOT IMPLEMENTED, denigrated after Windows NT
#define U_EMRPOLYTEXTOUTA_print(A)                                             \
    U_EMRNOTIMPLEMENTED_print("U_EMRPOLYTEXTOUTA", A) //!< Not implemented.
// U_EMRPOLYTEXTOUTW         97 NOT IMPLEMENTED, denigrated after Windows NT
#define U_EMRPOLYTEXTOUTW_print(A)                                             \
    U_EMRNOTIMPLEMENTED_print("U_EMRPOLYTEXTOUTW", A) //!< Not implemented.

// U_EMRSETICMMODE           98
/**
  \brief Print a pointer to a U_EMR_SETICMMODE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETICMMODE_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETICMMODE", "iMode:", contents, states);
}

// U_EMRCREATECOLORSPACE     99
/**
  \brief Print a pointer to a U_EMR_CREATECOLORSPACE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRCREATECOLORSPACE_print(const char *contents, drawingStates *states) {
    PU_EMRCREATECOLORSPACE pEmr = (PU_EMRCREATECOLORSPACE)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRCREATECOLORSPACE)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    verbose_printf("   ihCS:           %u\n", pEmr->ihCS);
    verbose_printf("   ColorSpace:     ");
    logcolorspacea_print(states, pEmr->lcs);
    verbose_printf("\n");
}

// U_EMRSETCOLORSPACE       100
/**
  \brief Print a pointer to a U_EMR_SETCOLORSPACE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETCOLORSPACE_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETCOLORSPACE", "ihCS:", contents, states);
}

// U_EMRDELETECOLORSPACE    101
/**
  \brief Print a pointer to a U_EMR_DELETECOLORSPACE record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRDELETECOLORSPACE_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRDELETECOLORSPACE", "ihCS:", contents, states);
}

// U_EMRGLSRECORD           102  Not implemented
#define U_EMRGLSRECORD_print(A)                                                \
    U_EMRNOTIMPLEMENTED_print("U_EMRGLSRECORD", A) //!< Not implemented.
// U_EMRGLSBOUNDEDRECORD    103  Not implemented
#define U_EMRGLSBOUNDEDRECORD_print(A)                                         \
    U_EMRNOTIMPLEMENTED_print("U_EMRGLSBOUNDEDRECORD", A) //!< Not implemented.

// U_EMRPIXELFORMAT         104
/**
  \brief Print a pointer to a U_EMR_PIXELFORMAT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRPIXELFORMAT_print(const char *contents, drawingStates *states) {
    PU_EMRPIXELFORMAT pEmr = (PU_EMRPIXELFORMAT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRPIXELFORMAT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    IF_MEM_UNSAFE_PRINT_AND_RETURN(&(pEmr->pfd),
                                   sizeof(U_PIXELFORMATDESCRIPTOR), blimit);
    verbose_printf("   Pfd:            ");
    pixelformatdescriptor_print(states, pEmr->pfd);
    verbose_printf("\n");
}

// U_EMRDRAWESCAPE          105  Not implemented
#define U_EMRDRAWESCAPE_print(A)                                               \
    U_EMRNOTIMPLEMENTED_print("U_EMRDRAWESCAPE", A) //!< Not implemented.
// U_EMREXTESCAPE           106  Not implemented
#define U_EMREXTESCAPE_print(A)                                                \
    U_EMRNOTIMPLEMENTED_print("U_EMREXTESCAPE", A) //!< Not implemented.
// U_EMRUNDEF107            107  Not implemented
#define U_EMRUNDEF107_print(A)                                                 \
    U_EMRNOTIMPLEMENTED_print("U_EMRUNDEF107", A) //!< Not implemented.

// U_EMRSMALLTEXTOUT        108
/**
  \brief Print a pointer to a U_EMR_SMALLTEXTOUT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSMALLTEXTOUT_print(const char *contents, drawingStates *states) {
    int roff;
    char *string;
    PU_EMRSMALLTEXTOUT pEmr = (PU_EMRSMALLTEXTOUT)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRSMALLTEXTOUT)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   Dest:           ");
    pointl_print(states, pEmr->Dest);
    verbose_printf("\n");
    verbose_printf("   cChars:         %u\n", pEmr->cChars);
    verbose_printf("   fuOptions:      0x%8.8X\n", pEmr->fuOptions);
    verbose_printf("   iGraphicsMode:  0x%8.8X\n", pEmr->iGraphicsMode);
    verbose_printf("   exScale:        %f\n", pEmr->exScale);
    verbose_printf("   eyScale:        %f\n", pEmr->eyScale);
    roff =
        sizeof(U_EMRSMALLTEXTOUT); // offset to the start of the variable fields
    if (!(pEmr->fuOptions & U_ETO_NO_RECT)) {
        IF_MEM_UNSAFE_PRINT_AND_RETURN(contents, roff, blimit);
        verbose_printf("   rclBounds:      ");
        rectl_print(states, *(PU_RECTL)(contents + roff));
        verbose_printf("\n");
        roff += sizeof(U_RECTL);
    }
    if (pEmr->fuOptions & U_ETO_SMALL_CHARS) {
        IF_MEM_UNSAFE_PRINT_AND_RETURN(
            contents, roff + pEmr->cChars * sizeof(char), blimit);
        verbose_printf("   Text8:          <%.*s>\n", pEmr->cChars,
                       contents + roff); /* May not be null terminated */
    } else {
        string =
            U_Utf16leToUtf8((uint16_t *)(contents + roff), pEmr->cChars, NULL);
        IF_MEM_UNSAFE_PRINT_AND_RETURN(
            contents, roff + pEmr->cChars * 2 * sizeof(char), blimit);
        verbose_printf("   Text16:         <%s>\n", contents + roff);
        free(string);
    }
}

// U_EMRFORCEUFIMAPPING     109  Not implemented
#define U_EMRFORCEUFIMAPPING_print(A)                                          \
    U_EMRNOTIMPLEMENTED_print("U_EMRFORCEUFIMAPPING", A) //!< Not implemented.
// U_EMRNAMEDESCAPE         110  Not implemented
#define U_EMRNAMEDESCAPE_print(A)                                              \
    U_EMRNOTIMPLEMENTED_print("U_EMRNAMEDESCAPE", A) //!< Not implemented.
// U_EMRCOLORCORRECTPALETTE 111  Not implemented
#define U_EMRCOLORCORRECTPALETTE_print(A)                                      \
    U_EMRNOTIMPLEMENTED_print("U_EMRCOLORCORRECTPALETTE",                      \
                              A) //!< Not implemented.
// U_EMRSETICMPROFILEA      112  Not implemented
#define U_EMRSETICMPROFILEA_print(A)                                           \
    U_EMRNOTIMPLEMENTED_print("U_EMRSETICMPROFILEA", A) //!< Not implemented.
// U_EMRSETICMPROFILEW      113  Not implemented
#define U_EMRSETICMPROFILEW_print(A)                                           \
    U_EMRNOTIMPLEMENTED_print("U_EMRSETICMPROFILEW", A) //!< Not implemented.

// U_EMRALPHABLEND          114
/**
  \brief Print a pointer to a U_EMR_ALPHABLEND record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRALPHABLEND_print(const char *contents, drawingStates *states) {
    core13_print("U_EMRALPHABLEND", contents, states);
}

// U_EMRSETLAYOUT           115
/**
  \brief Print a pointer to a U_EMR_SETLAYOUT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRSETLAYOUT_print(const char *contents, drawingStates *states) {
    core3_print("U_EMRSETLAYOUT", "iMode:", contents, states);
}

// U_EMRTRANSPARENTBLT      116
/**
  \brief Print a pointer to a U_EMR_TRANSPARENTBLT record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRTRANSPARENTBLT_print(const char *contents, drawingStates *states) {
    core13_print("U_EMRTRANSPARENTBLT", contents, states);
}

// U_EMRUNDEF117            117  Not implemented
#define U_EMRUNDEF117_print(A)                                                 \
    U_EMRNOTIMPLEMENTED_print("U_EMRUNDEF117", A) //!< Not implemented.
// U_EMRGRADIENTFILL        118
/**
  \brief Print a pointer to a U_EMR_GRADIENTFILL record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRGRADIENTFILL_print(const char *contents, drawingStates *states) {
    unsigned int i;
    PU_EMRGRADIENTFILL pEmr = (PU_EMRGRADIENTFILL)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRGRADIENTFILL)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   rclBounds:      ");
    rectl_print(states, pEmr->rclBounds);
    verbose_printf("\n");
    verbose_printf("   nTriVert:       %u\n", pEmr->nTriVert);
    verbose_printf("   nGradObj:       %u\n", pEmr->nGradObj);
    verbose_printf("   ulMode:         %u\n", pEmr->ulMode);
    contents += sizeof(U_EMRGRADIENTFILL);
    if (pEmr->nTriVert) {
        IF_MEM_UNSAFE_PRINT_AND_RETURN(
            contents, pEmr->nTriVert * sizeof(U_TRIVERTEX), blimit);
        verbose_printf("   TriVert:        ");
        for (i = 0; i < pEmr->nTriVert; i++, contents += sizeof(U_TRIVERTEX)) {
            trivertex_print(states, *(PU_TRIVERTEX)(contents));
        }
        verbose_printf("\n");
    }
    if (pEmr->nGradObj) {
        verbose_printf("   GradObj:        ");
        if (pEmr->ulMode == U_GRADIENT_FILL_TRIANGLE) {
            IF_MEM_UNSAFE_PRINT_AND_RETURN(
                contents, pEmr->nGradObj * sizeof(U_GRADIENT3), blimit);
            for (i = 0; i < pEmr->nGradObj;
                 i++, contents += sizeof(U_GRADIENT3)) {
                gradient3_print(states, *(PU_GRADIENT3)(contents));
            }
        } else if (pEmr->ulMode == U_GRADIENT_FILL_RECT_H ||
                   pEmr->ulMode == U_GRADIENT_FILL_RECT_V) {
            IF_MEM_UNSAFE_PRINT_AND_RETURN(
                contents, pEmr->nGradObj * sizeof(U_GRADIENT4), blimit);
            for (i = 0; i < pEmr->nGradObj;
                 i++, contents += sizeof(U_GRADIENT4)) {
                gradient4_print(states, *(PU_GRADIENT4)(contents));
            }
        } else {
            verbose_printf("invalid ulMode value!");
        }
        verbose_printf("\n");
    }
}

// U_EMRSETLINKEDUFIS       119  Not implemented
#define U_EMRSETLINKEDUFIS_print(A)                                            \
    U_EMRNOTIMPLEMENTED_print("U_EMR_SETLINKEDUFIS", A) //!< Not implemented.
// U_EMRSETTEXTJUSTIFICATION120  Not implemented (denigrated)
#define U_EMRSETTEXTJUSTIFICATION_print(A)                                     \
    U_EMRNOTIMPLEMENTED_print("U_EMR_SETTEXTJUSTIFICATION",                    \
                              A) //!< Not implemented.
// U_EMRCOLORMATCHTOTARGETW 121  Not implemented
#define U_EMRCOLORMATCHTOTARGETW_print(A)                                      \
    U_EMRNOTIMPLEMENTED_print("U_EMR_COLORMATCHTOTARGETW",                     \
                              A) //!< Not implemented.

// U_EMRCREATECOLORSPACEW   122
/**
  \brief Print a pointer to a U_EMR_CREATECOLORSPACEW record.
  \param contents   pointer to a buffer holding all EMR records
  */
void U_EMRCREATECOLORSPACEW_print(const char *contents, drawingStates *states) {
    unsigned int i;
    PU_EMRCREATECOLORSPACEW pEmr = (PU_EMRCREATECOLORSPACEW)(contents);
    if (pEmr->emr.nSize < sizeof(U_EMRCREATECOLORSPACEW)) {
        verbose_printf("   record corruption HERE\n");
        return;
    }
    const char *blimit = contents + pEmr->emr.nSize;
    verbose_printf("   ihCS:           %u\n", pEmr->ihCS);
    verbose_printf("   ColorSpace:     ");
    logcolorspacew_print(states, pEmr->lcs);
    verbose_printf("\n");
    verbose_printf("   dwFlags:        0x%8.8X\n", pEmr->dwFlags);
    verbose_printf("   cbData:         %u\n", pEmr->cbData);
    verbose_printf("   Data(hexvalues):");
    if (pEmr->dwFlags & 1) {
        IF_MEM_UNSAFE_PRINT_AND_RETURN(contents, pEmr->cbData, blimit);
        for (i = 0; i < pEmr->cbData; i++) {
            verbose_printf("[%d]:%2.2X ", i, pEmr->Data[i]);
        }
    }
    verbose_printf("\n");
}

void U_emf_onerec_print(const char *contents, const char *blimit, int recnum,
                        size_t off, drawingStates *states) {
    PU_ENHMETARECORD lpEMFR = (PU_ENHMETARECORD)(contents + off);
    FLAG_RESET;
    verbose_printf("\n%-30srecord:%5d type:%-4d offset:%8d rsize:%8d\n",
                   U_emr_names(lpEMFR->iType), recnum, lpEMFR->iType, (int)off,
                   lpEMFR->nSize);
}
#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
