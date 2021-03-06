/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
  * ***** BEGIN LICENSE BLOCK *****
  * Version: MPL 1.1/GPL 2.0/LGPL 2.1
  *
  * The contents of this file are subject to the Mozilla Public License Version
  * 1.1 (the "License"); you may not use this file except in compliance with
  * the License. You may obtain a copy of the License at
  * http://www.mozilla.org/MPL/
  *
  * Software distributed under the License is distributed on an "AS IS" basis,
  * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
  * for the specific language governing rights and limitations under the
  * License.
  *
  * The Original Code is Mozilla Corporation code.
  *
  * The Initial Developer of the Original Code is Mozilla Foundation.
  * Portions created by the Initial Developer are Copyright (C) 2011
  * the Initial Developer. All Rights Reserved.
  *
  * Contributor(s):
  *   Bas Schouten <bschouten@mozilla.com>
  *
  * Alternatively, the contents of this file may be used under the terms of
  * either the GNU General Public License Version 2 or later (the "GPL"), or
  * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
  * in which case the provisions of the GPL or the LGPL are applicable instead
  * of those above. If you wish to allow use of your version of this file only
  * under the terms of either the GPL or the LGPL, and not to allow others to
  * use your version of this file under the terms of the MPL, indicate your
  * decision by deleting the provisions above and replace them with the notice
  * and other provisions required by the GPL or the LGPL. If you do not delete
  * the provisions above, a recipient may use your version of this file under
  * the terms of any one of the MPL, the GPL or the LGPL.
  *
  * ***** END LICENSE BLOCK ***** */
     
#ifndef MOZILLA_GFX_DRAWTARGETDUAL_H_
#define MOZILLA_GFX_DRAWTARGETDUAL_H_
     
#include <vector>
#include <sstream>

#include "SourceSurfaceDual.h"
     
#include "2D.h"
     
namespace mozilla {
namespace gfx {
     
#define FORWARD_FUNCTION(funcName) \
  virtual void funcName() { mA->funcName(); mB->funcName(); }
#define FORWARD_FUNCTION1(funcName, var1Type, var1Name) \
  virtual void funcName(var1Type var1Name) { mA->funcName(var1Name); mB->funcName(var1Name); }

/* This is a special type of DrawTarget. It duplicates all drawing calls
 * accross two drawtargets. An exception to this is when a snapshot of another
 * dual DrawTarget is used as the source for any surface data. In this case
 * the snapshot of the first source DrawTarget is used as a source for the call
 * to the first destination DrawTarget (mA) and the snapshot of the second
 * source DrawTarget is used at the source for the second destination
 * DrawTarget (mB). This class facilitates black-background/white-background
 * drawing for per-component alpha extraction for backends which do not support
 * native component alpha.
 */
class DrawTargetDual : public DrawTarget
{
public:
  DrawTargetDual(DrawTarget *aA, DrawTarget *aB)
    : mA(aA)
    , mB(aB)
  { 
    mFormat = aA->GetFormat();
  }
     
  virtual BackendType GetType() const { return mA->GetType(); }
  virtual TemporaryRef<SourceSurface> Snapshot() { return new SourceSurfaceDual(mA, mB); }
  virtual IntSize GetSize() { return mA->GetSize(); }
     
  FORWARD_FUNCTION(Flush)
  FORWARD_FUNCTION1(PushClip, const Path *, aPath)
  FORWARD_FUNCTION1(PushClipRect, const Rect &, aRect)
  FORWARD_FUNCTION(PopClip)
  FORWARD_FUNCTION1(ClearRect, const Rect &, aRect)

  virtual void SetTransform(const Matrix &aTransform) {
    mTransform = aTransform;
    mA->SetTransform(aTransform);
    mB->SetTransform(aTransform);
  }

  virtual void DrawSurface(SourceSurface *aSurface, const Rect &aDest, const Rect & aSource,
                           const DrawSurfaceOptions &aSurfOptions, const DrawOptions &aOptions);
  
  virtual void DrawSurfaceWithShadow(SourceSurface *aSurface, const Point &aDest,
                                     const Color &aColor, const Point &aOffset,
                                     Float aSigma, CompositionOp aOp);

  virtual void CopySurface(SourceSurface *aSurface, const IntRect &aSourceRect,
                           const IntPoint &aDestination);

  virtual void FillRect(const Rect &aRect, const Pattern &aPattern, const DrawOptions &aOptions);

  virtual void StrokeRect(const Rect &aRect, const Pattern &aPattern,
                          const StrokeOptions &aStrokeOptions, const DrawOptions &aOptions);

  virtual void StrokeLine(const Point &aStart, const Point &aEnd, const Pattern &aPattern,
                          const StrokeOptions &aStrokeOptions, const DrawOptions &aOptions);

  virtual void Stroke(const Path *aPath, const Pattern &aPattern,
                      const StrokeOptions &aStrokeOptions, const DrawOptions &aOptions);

  virtual void Fill(const Path *aPath, const Pattern &aPattern, const DrawOptions &aOptions);

  virtual void FillGlyphs(ScaledFont *aScaledFont, const GlyphBuffer &aBuffer,
                          const Pattern &aPattern, const DrawOptions &aOptions,
                          const GlyphRenderingOptions *aRenderingOptions);
  
  virtual void Mask(const Pattern &aSource, const Pattern &aMask, const DrawOptions &aOptions);
     
  virtual TemporaryRef<SourceSurface>
    CreateSourceSurfaceFromData(unsigned char *aData,
                                const IntSize &aSize,
                                int32_t aStride,
                                SurfaceFormat aFormat) const
  {
    return mA->CreateSourceSurfaceFromData(aData, aSize, aStride, aFormat);
  }
     
  virtual TemporaryRef<SourceSurface> OptimizeSourceSurface(SourceSurface *aSurface) const
  {
    return mA->OptimizeSourceSurface(aSurface);
  }
     
  virtual TemporaryRef<SourceSurface>
    CreateSourceSurfaceFromNativeSurface(const NativeSurface &aSurface) const
  {
    return mA->CreateSourceSurfaceFromNativeSurface(aSurface);
  }
     
  virtual TemporaryRef<DrawTarget>
    CreateSimilarDrawTarget(const IntSize &aSize, SurfaceFormat aFormat) const;
     
  virtual TemporaryRef<PathBuilder> CreatePathBuilder(FillRule aFillRule = FILL_WINDING) const
  {
    return mA->CreatePathBuilder(aFillRule);
  }
     
  virtual TemporaryRef<GradientStops>
    CreateGradientStops(GradientStop *aStops,
                        uint32_t aNumStops,
                        ExtendMode aExtendMode = EXTEND_CLAMP) const
  {
    return mA->CreateGradientStops(aStops, aNumStops, aExtendMode);
  }
     
  virtual void *GetNativeSurface(NativeSurfaceType aType)
  {
    return NULL;
  }
     
private:
  RefPtr<DrawTarget> mA;
  RefPtr<DrawTarget> mB;
};
     
}
}
     
#endif /* MOZILLA_GFX_DRAWTARGETDUAL_H_ */ 