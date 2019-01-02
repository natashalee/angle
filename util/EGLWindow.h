//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UTIL_EGLWINDOW_H_
#define UTIL_EGLWINDOW_H_

#include <stdint.h>
#include <list>
#include <memory>
#include <string>

#include "common/Optional.h"
#include "common/angleutils.h"
#include "util/EGLPlatformParameters.h"
#include "util/util_export.h"
#include "util/util_gl.h"

class OSWindow;

namespace angle
{
class Library;
struct PlatformMethods;
}  // namespace angle

class ANGLE_UTIL_EXPORT EGLWindow : angle::NonCopyable
{
  public:
    EGLWindow(EGLint glesMajorVersion,
              EGLint glesMinorVersion,
              const EGLPlatformParameters &platform);

    ~EGLWindow();

    void setConfigRedBits(int bits) { mRedBits = bits; }
    void setConfigGreenBits(int bits) { mGreenBits = bits; }
    void setConfigBlueBits(int bits) { mBlueBits = bits; }
    void setConfigAlphaBits(int bits) { mAlphaBits = bits; }
    void setConfigDepthBits(int bits) { mDepthBits = bits; }
    void setConfigStencilBits(int bits) { mStencilBits = bits; }
    void setConfigComponentType(EGLenum componentType) { mComponentType = componentType; }
    void setMultisample(bool multisample) { mMultisample = multisample; }
    void setSamples(EGLint samples) { mSamples = samples; }
    void setDebugEnabled(bool debug) { mDebug = debug; }
    void setNoErrorEnabled(bool noError) { mNoError = noError; }
    void setWebGLCompatibilityEnabled(bool webglCompatibility)
    {
        mWebGLCompatibility = webglCompatibility;
    }
    void setExtensionsEnabled(bool extensionsEnabled) { mExtensionsEnabled = extensionsEnabled; }
    void setBindGeneratesResource(bool bindGeneratesResource)
    {
        mBindGeneratesResource = bindGeneratesResource;
    }
    void setDebugLayersEnabled(bool enabled) { mDebugLayersEnabled = enabled; }
    void setClientArraysEnabled(bool enabled) { mClientArraysEnabled = enabled; }
    void setRobustAccess(bool enabled) { mRobustAccess = enabled; }
    void setRobustResourceInit(bool enabled) { mRobustResourceInit = enabled; }
    void setSwapInterval(EGLint swapInterval) { mSwapInterval = swapInterval; }
    void setPlatformMethods(angle::PlatformMethods *platformMethods)
    {
        mPlatformMethods = platformMethods;
    }
    void setContextProgramCacheEnabled(bool enabled) { mContextProgramCacheEnabled = enabled; }
    void setContextVirtualization(bool enabled) { mContextVirtualization = enabled; }

    static EGLBoolean FindEGLConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *config);

    void swap();

    EGLint getClientMajorVersion() const { return mClientMajorVersion; }
    EGLint getClientMinorVersion() const { return mClientMinorVersion; }
    const EGLPlatformParameters &getPlatform() const { return mPlatform; }
    EGLConfig getConfig() const;
    EGLDisplay getDisplay() const;
    EGLSurface getSurface() const;
    EGLContext getContext() const;
    int getConfigRedBits() const { return mRedBits; }
    int getConfigGreenBits() const { return mGreenBits; }
    int getConfigBlueBits() const { return mBlueBits; }
    int getConfigAlphaBits() const { return mAlphaBits; }
    int getConfigDepthBits() const { return mDepthBits; }
    int getConfigStencilBits() const { return mStencilBits; }
    bool isMultisample() const { return mMultisample; }
    bool isDebugEnabled() const { return mDebug; }
    EGLint getSwapInterval() const { return mSwapInterval; }
    const angle::PlatformMethods *getPlatformMethods() const { return mPlatformMethods; }

    // Internally initializes the Display, Surface and Context.
    bool initializeGL(OSWindow *osWindow, angle::Library *eglLibrary);

    // Only initializes the Display and Surface.
    bool initializeDisplayAndSurface(OSWindow *osWindow, angle::Library *eglLibrary);

    // Create an EGL context with this window's configuration
    EGLContext createContext(EGLContext share) const;

    // Only initializes the Context.
    bool initializeContext();

    void destroyGL();
    bool isGLInitialized() const;

    void makeCurrent();

    static bool ClientExtensionEnabled(const std::string &extName);

  private:
    EGLConfig mConfig;
    EGLDisplay mDisplay;
    EGLSurface mSurface;
    EGLContext mContext;

    EGLint mClientMajorVersion;
    EGLint mClientMinorVersion;
    EGLint mEGLMajorVersion;
    EGLint mEGLMinorVersion;
    EGLPlatformParameters mPlatform;
    int mRedBits;
    int mGreenBits;
    int mBlueBits;
    int mAlphaBits;
    int mDepthBits;
    int mStencilBits;
    EGLenum mComponentType;
    bool mMultisample;
    bool mDebug;
    bool mNoError;
    bool mWebGLCompatibility;
    Optional<bool> mExtensionsEnabled;
    bool mBindGeneratesResource;
    bool mClientArraysEnabled;
    bool mRobustAccess;
    Optional<bool> mRobustResourceInit;
    EGLint mSwapInterval;
    EGLint mSamples;
    Optional<bool> mDebugLayersEnabled;
    Optional<bool> mContextProgramCacheEnabled;
    Optional<bool> mContextVirtualization;
    angle::PlatformMethods *mPlatformMethods;
};

ANGLE_UTIL_EXPORT bool CheckExtensionExists(const char *allExtensions, const std::string &extName);

#endif  // UTIL_EGLWINDOW_H_
