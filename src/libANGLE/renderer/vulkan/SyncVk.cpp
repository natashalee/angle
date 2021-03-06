//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SyncVk.cpp:
//    Implements the class methods for SyncVk.
//

#include "libANGLE/renderer/vulkan/SyncVk.h"

#include "common/debug.h"
#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/DisplayVk.h"

namespace rx
{
FenceSyncVk::FenceSyncVk() {}

FenceSyncVk::~FenceSyncVk() {}

void FenceSyncVk::onDestroy(RendererVk *renderer)
{
    if (mEvent.valid())
    {
        renderer->releaseObject(renderer->getCurrentQueueSerial(), &mEvent);
    }
}

angle::Result FenceSyncVk::initialize(vk::Context *context)
{
    ASSERT(!mEvent.valid());

    RendererVk *renderer = context->getRenderer();
    VkDevice device      = renderer->getDevice();

    VkEventCreateInfo eventCreateInfo = {};
    eventCreateInfo.sType             = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    eventCreateInfo.flags             = 0;

    vk::Scoped<vk::Event> event(device);
    ANGLE_VK_TRY(context, event.get().init(device, eventCreateInfo));

    mEvent        = event.release();
    mSignalSerial = renderer->getCurrentQueueSerial();

    renderer->getCommandGraph()->setFenceSync(mEvent);
    return angle::Result::Continue;
}

angle::Result FenceSyncVk::clientWait(vk::Context *context,
                                      bool flushCommands,
                                      uint64_t timeout,
                                      VkResult *outResult)
{
    RendererVk *renderer = context->getRenderer();

    // If the event is already set, don't wait
    bool alreadySignaled = false;
    ANGLE_TRY(getStatus(context, &alreadySignaled));
    if (alreadySignaled)
    {
        *outResult = VK_EVENT_SET;
        return angle::Result::Continue;
    }

    // If timeout is zero, there's no need to wait, so return timeout already.
    if (timeout == 0)
    {
        *outResult = VK_TIMEOUT;
        return angle::Result::Continue;
    }

    // If asked to flush, only do so if the queue serial hasn't changed (as otherwise the event
    // signal is already flushed). If not asked to flush, do the flush anyway!  This is because
    // there's no cpu-side wait on the event and there's no fence yet inserted to wait on.  We could
    // test the event in a loop with a sleep, which can only ever not timeout if another thread
    // performs the flush.  Instead, we perform the flush for simplicity.
    if (hasPendingWork(renderer))
    {
        ANGLE_TRY(renderer->flush(context));
    }

    // Wait on the fence that's implicitly inserted at the end of every submission.
    bool timedOut = false;
    angle::Result result =
        renderer->finishToSerialOrTimeout(context, mSignalSerial, timeout, &timedOut);
    ANGLE_TRY(result);

    *outResult = timedOut ? VK_TIMEOUT : VK_SUCCESS;
    return angle::Result::Continue;
}

angle::Result FenceSyncVk::serverWait(vk::Context *context)
{
    context->getRenderer()->getCommandGraph()->waitFenceSync(mEvent);
    return angle::Result::Continue;
}

angle::Result FenceSyncVk::getStatus(vk::Context *context, bool *signaled)
{
    VkResult result = mEvent.getStatus(context->getRenderer()->getDevice());
    if (result != VK_EVENT_SET && result != VK_EVENT_RESET)
    {
        ANGLE_VK_TRY(context, result);
    }
    *signaled = result == VK_EVENT_SET;
    return angle::Result::Continue;
}

bool FenceSyncVk::hasPendingWork(RendererVk *renderer)
{
    return mSignalSerial == renderer->getCurrentQueueSerial();
}

SyncVk::SyncVk() : SyncImpl() {}

SyncVk::~SyncVk() {}

void SyncVk::onDestroy(const gl::Context *context)
{
    mFenceSync.onDestroy(vk::GetImpl(context)->getRenderer());
}

angle::Result SyncVk::set(const gl::Context *context, GLenum condition, GLbitfield flags)
{
    ASSERT(condition == GL_SYNC_GPU_COMMANDS_COMPLETE);
    ASSERT(flags == 0);

    return mFenceSync.initialize(vk::GetImpl(context));
}

angle::Result SyncVk::clientWait(const gl::Context *context,
                                 GLbitfield flags,
                                 GLuint64 timeout,
                                 GLenum *outResult)
{
    ContextVk *contextVk = vk::GetImpl(context);

    ASSERT((flags & ~GL_SYNC_FLUSH_COMMANDS_BIT) == 0);

    bool flush = (flags & GL_SYNC_FLUSH_COMMANDS_BIT) != 0;
    VkResult result;

    ANGLE_TRY(mFenceSync.clientWait(contextVk, flush, static_cast<uint64_t>(timeout), &result));

    switch (result)
    {
        case VK_EVENT_SET:
            *outResult = GL_ALREADY_SIGNALED;
            return angle::Result::Continue;

        case VK_SUCCESS:
            *outResult = GL_CONDITION_SATISFIED;
            return angle::Result::Continue;

        case VK_TIMEOUT:
            *outResult = GL_TIMEOUT_EXPIRED;
            return angle::Result::Incomplete;

        default:
            UNREACHABLE();
            *outResult = GL_WAIT_FAILED;
            return angle::Result::Stop;
    }
}

angle::Result SyncVk::serverWait(const gl::Context *context, GLbitfield flags, GLuint64 timeout)
{
    ASSERT(flags == 0);
    ASSERT(timeout == GL_TIMEOUT_IGNORED);

    return mFenceSync.serverWait(vk::GetImpl(context));
}

angle::Result SyncVk::getStatus(const gl::Context *context, GLint *outResult)
{
    bool signaled = false;
    ANGLE_TRY(mFenceSync.getStatus(vk::GetImpl(context), &signaled));

    *outResult = signaled ? GL_SIGNALED : GL_UNSIGNALED;
    return angle::Result::Continue;
}

EGLSyncVk::EGLSyncVk(const egl::AttributeMap &attribs) : EGLSyncImpl()
{
    ASSERT(attribs.isEmpty());
}

EGLSyncVk::~EGLSyncVk() {}

void EGLSyncVk::onDestroy(const egl::Display *display)
{
    mFenceSync.onDestroy(vk::GetImpl(display)->getRenderer());
}

egl::Error EGLSyncVk::initialize(const egl::Display *display, EGLenum type)
{
    ASSERT(type == EGL_SYNC_FENCE_KHR);

    if (mFenceSync.initialize(vk::GetImpl(display)) == angle::Result::Stop)
    {
        return egl::Error(EGL_BAD_ALLOC, "eglCreateSyncKHR failed to create sync object");
    }

    return egl::NoError();
}

egl::Error EGLSyncVk::clientWait(const egl::Display *display,
                                 EGLint flags,
                                 EGLTime timeout,
                                 EGLint *outResult)
{
    ASSERT((flags & ~EGL_SYNC_FLUSH_COMMANDS_BIT_KHR) == 0);

    bool flush = (flags & EGL_SYNC_FLUSH_COMMANDS_BIT_KHR) != 0;
    VkResult result;

    if (mFenceSync.clientWait(vk::GetImpl(display), flush, static_cast<uint64_t>(timeout),
                              &result) == angle::Result::Stop)
    {
        return egl::Error(EGL_BAD_ALLOC);
    }

    switch (result)
    {
        case VK_EVENT_SET:
            // fall through.  EGL doesn't differentiate between event being already set, or set
            // before timeout.
        case VK_SUCCESS:
            *outResult = EGL_CONDITION_SATISFIED_KHR;
            return egl::NoError();

        case VK_TIMEOUT:
            *outResult = EGL_TIMEOUT_EXPIRED_KHR;
            return egl::NoError();

        default:
            UNREACHABLE();
            *outResult = EGL_FALSE;
            return egl::Error(EGL_BAD_ALLOC);
    };
}

egl::Error EGLSyncVk::serverWait(const egl::Display *display, EGLint flags)
{
    ASSERT(flags == 0);
    if (mFenceSync.serverWait(vk::GetImpl(display)) == angle::Result::Stop)
    {
        return egl::Error(EGL_BAD_ALLOC);
    }
    return egl::NoError();
}

egl::Error EGLSyncVk::getStatus(const egl::Display *display, EGLint *outStatus)
{
    bool signaled = false;
    if (mFenceSync.getStatus(vk::GetImpl(display), &signaled) == angle::Result::Stop)
    {
        return egl::Error(EGL_BAD_ALLOC);
    }

    *outStatus = signaled ? EGL_SIGNALED_KHR : EGL_UNSIGNALED_KHR;
    return egl::NoError();
}

}  // namespace rx
