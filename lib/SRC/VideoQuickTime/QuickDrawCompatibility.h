/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef QuickDrawCompatibility_h
#define QuickDrawCompatibility_h

#ifndef __LP64__

#import <Carbon/Carbon.h>

#if defined(QD_HEADERS_ARE_PRIVATE) && QD_HEADERS_ARE_PRIVATE

#ifdef __cplusplus
extern "C" {
#endif

#define MacSetRect SetRect
#define MacOffsetRect OffsetRect
#define MacSetRectRgn SetRectRgn
#define MacUnionRgn UnionRgn

extern Boolean EmptyRgn(RgnHandle);
extern OSStatus CreateCGContextForPort(CGrafPtr, CGContextRef*);
extern OSStatus SyncCGContextOriginWithPort(CGContextRef, CGrafPtr);
extern PixMapHandle GetPortPixMap(CGrafPtr);
extern QDErr NewGWorldFromPtr(GWorldPtr*, UInt32, const Rect*, CTabHandle, GDHandle, GWorldFlags, Ptr, SInt32);
extern QDErr NewGWorld(GWorldPtr*, short, const Rect*, CTabHandle, GDHandle, GWorldFlags);
extern PixMapHandle GetGWorldPixMap(GWorldPtr);
extern Boolean LockPixels(PixMapHandle);
extern void UnlockPixels(PixMapHandle);
extern Rect* GetPortBounds(CGrafPtr, Rect*);
extern Rect* GetRegionBounds(RgnHandle, Rect*);
extern RgnHandle GetPortClipRegion(CGrafPtr, RgnHandle);
extern RgnHandle GetPortVisibleRegion(CGrafPtr, RgnHandle);
extern RgnHandle NewRgn();
extern void BackColor(long);
extern void RGBBackColor(const RGBColor *color);
extern void DisposeGWorld(GWorldPtr);
extern void DisposeRgn(RgnHandle);
extern void ForeColor(long);
extern void RGBForeColor(const RGBColor *color);
extern void GetGWorld(CGrafPtr*, GDHandle*);
extern void GetPort(GrafPtr*);
extern void GlobalToLocal(Point*);
extern void SetRect(Rect*, short, short, short, short);
extern void SetRectRgn(RgnHandle, short, short, short, short);
extern void UnionRgn(RgnHandle, RgnHandle, RgnHandle);
extern void MovePortTo(short, short);
extern void OffsetRect(Rect*, short, short);
extern void OffsetRgn(RgnHandle, short, short);
extern void PaintRect(const Rect*);
extern void PenNormal();
extern void PortSize(short, short);
extern void RectRgn(RgnHandle, const Rect*);
extern void SectRgn(RgnHandle, RgnHandle, RgnHandle);
extern void SetGWorld(CGrafPtr, GDHandle);
extern void SetOrigin(short, short);
extern void SetPort(GrafPtr);
extern void SetPortClipRegion(CGrafPtr, RgnHandle);
extern void SetPortVisibleRegion(CGrafPtr, RgnHandle);
extern OSStatus QDBeginCGContext(CGrafPtr, CGContextRef*);
extern OSStatus QDEndCGContext(CGrafPtr, CGContextRef*);

enum {
    blackColor = 33,
    whiteColor = 30,
    redColor = 205,
    greenColor = 341,
    blueColor = 409,
    cyanColor = 273,
    magentaColor = 137,
    yellowColor = 69
};

enum {
    // Graphics Transfer Modes.
    // Boolean modes
    // src modes are used with bitmaps and text;
    // pat modes are used with lines and shapes
    //srcCopy               =0, // "Copy"
    srcOr                 =1, // "Or" (Deprecated)
    srcXor                =2, // "Xor" (Deprecated)
    srcBic                =3, // "Replace Black" (Deprecated)
    notSrcCopy            =4, // "Inverse Copy" (Deprecated)
    notSrcOr              =5, // "Inverse Or" (Deprecated)
    notSrcXor             =6, // "Inverse Xor" (Deprecated)
    notSrcBic             =7, // "Inverse Replace Black" (Deprecated)
    patCopy               =8, // "Pattern Copy" (Deprecated)
    patOr                 =9, // "Pattern Or" (Deprecated)
    patXor                =10, // "Pattern Xor" (Deprecated)
    patBic                =11, // "Pattern Replace Black" (Deprecated)
    notPatCopy            =12, // "Inverse Pattern Copy" (Deprecated)
    notPatOr              =13, // "Inverse Pattern Or" (Deprecated)
    notPatXor             =14, // "Inverse Pattern Xor" (Deprecated)
    notPatBic             =15, // "Inverse Pattern Replace Black" (Deprecated)
    // Text dimming
    grayishTextOr         =49, // "Grayish Text Or" (Deprecated)
    // Highlighting
    hilite                =50, // "Hilite" (Takes OpColor, Deprecated)
    hilitetransfermode    =50,
    // Arithmetic modes
    blend                 =32, // "Blend" (Transparent, Takes OpColor)
    addPin                =33, // "Add Pin" (Takes OpColor, Deprecated)
    addOver               =34, // "Add Over" (Deprecated)
    subPin                =35, // "Subtract Pin" (Takes OpColor, Deprecated)
    addMax                =37, // "Add Max" (Deprecated)
    adMax                 =37,
    subOver               =38, // "Subtract Over" (Deprecated)
    adMin                 =39, // "Add Min" (Deprecated)
    ditherCopy            =64, // "Dither Copy"
    // Transparent mode
    transparent           =36, // "Transparent" (Transparent, Takes OpColor, Deprecated)
    // Other modes
    //graphicsModeStraightAlpha     = 256, // "Straight Alpha"
    //graphicsModePreWhiteAlpha     = 257, // "Premultiplied White Alpha"
    //graphicsModePreBlackAlpha     = 258, // "Premultiplied Black Alpha"
    //graphicsModeComposition       = 259, // "Composition"
    //graphicsModeStraightAlphaBlend = 260, // "Straight Alpha Blend" (Transparent, Takes OpColor)
    //graphicsModePreMulColorAlpha  = 261, // "Premultiplied Color Alpha" (Takes OpColor, Deprecated)
    //graphicsModePerComponentAlpha = 272 // "Per Component Alpha" (Deprecated)
};

#ifdef __cplusplus
}
#endif

#endif // defined(QD_HEADERS_ARE_PRIVATE) && QD_HEADERS_ARE_PRIVATE

#endif // __LP64__

#endif // QuickDrawCompatibility_h
