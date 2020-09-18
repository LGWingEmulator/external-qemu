#include "PostWorker.h"

#include "ColorBuffer.h"
#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "emugl/common/misc.h"

#define POST_DEBUG 0
#if POST_DEBUG >= 1
#define DD(fmt, ...) \
    fprintf(stderr, "%s:%d| " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(fmt, ...) (void)0
#endif


PostWorker::PostWorker(PostWorker::BindSubwinCallback&& cb) :
    mFb(FrameBuffer::getFB()),
    mBindSubwin(cb) {}

void PostWorker::fillMultiDisplayPostStruct(ComposeLayer* l, int idx, ColorBuffer* cb) {
    l->composeMode = HWC2_COMPOSITION_DEVICE;
    l->blendMode = HWC2_BLEND_MODE_NONE;
    l->transform = (hwc_transform_t)0;
    l->alpha = 1.0;
    /*
     * to support multi display rotation & skins 
    l->displayFrame.left = mFb->m_displays[idx].pos_x;
    l->displayFrame.top = mFb->m_displays[idx].pos_y;
    l->displayFrame.right = mFb->m_displays[idx].pos_x + mFb->m_displays[idx].width;
    l->displayFrame.bottom =  mFb->m_displays[idx].pos_y + mFb->m_displays[idx].height;
    *
    */
    l->displayFrame.left = w_displays[idx].pos_x;
    l->displayFrame.top = w_displays[idx].pos_y;
    l->displayFrame.right = w_displays[idx].pos_x + w_displays[idx].width;
    l->displayFrame.bottom =  w_displays[idx].pos_y + w_displays[idx].height;
    l->crop.left = 0.0;
    l->crop.top = (float)cb->getHeight();
    l->crop.right = (float)cb->getWidth();
    l->crop.bottom = 0.0;
}

void PostWorker::recomputeFrameLayout(int rot) {
	/*
    for (const auto& iter : w_displays) {
	printf("w_disp[%d] w,h[%d %d]\n",iter.first,iter.second.width,iter.second.height);
    }
    int i;
    for(i=0;i<4;i++)
        printf("w_displays[%d] x,y[%d %d] w,y[%d %d] %d\n",i,
			w_displays[i].pos_x,w_displays[i].pos_y,
			w_displays[i].width,w_displays[i].height,
			w_displays[i].rotation);
			*/
    int loc;

    if ( rot == 270 ) {
        w_displays[0].pos_x = 0;
        w_displays[0].pos_y = 0;
        loc = w_main_h-w_sub_w;
        if ( loc > 0 ) w_displays[1].pos_x = (loc)/2;
        else w_displays[1].pos_x = 0;
        w_displays[1].pos_y = w_main_w + w_gap;

        w_displays[0].width = w_main_h;
        w_displays[0].height = w_main_w;
        w_displays[1].width = w_sub_w;
        w_displays[1].height = w_sub_h;
        w_displays[1].rotation = w_rot;
    }
    else if ( rot == 180 ) {
        w_displays[0].pos_x = w_sub_h + w_gap ;
        w_displays[0].pos_y = 0;

        w_displays[1].pos_x = 0;
        loc = w_main_h-w_sub_w;
        if ( loc > 0 ) w_displays[1].pos_y = (loc)/2;
        else w_displays[1].pos_y = 0;

        w_displays[0].width = w_main_w;
        w_displays[0].height = w_main_h;
        w_displays[1].width = w_sub_h;
        w_displays[1].height = w_sub_w;
        w_displays[1].rotation = w_rot;
    }
    else if ( rot == 90 ) {
        w_displays[0].pos_x = 0;
        w_displays[0].pos_y = w_sub_h + w_gap;
        loc = w_main_h-w_sub_w;
        if ( loc > 0 ) w_displays[1].pos_x = (loc)/2;
        else w_displays[1].pos_x = 0;
        w_displays[1].pos_y = 0 ;

        w_displays[0].width = w_main_h;
        w_displays[0].height = w_main_w;
        w_displays[1].width = w_sub_w;
        w_displays[1].height = w_sub_h;
        w_displays[1].rotation = w_rot;
    }
    else {
        w_displays[0].pos_x = 0;
        w_displays[0].pos_y = 0;
        w_displays[1].pos_x = w_main_w + w_gap ;
        loc = w_main_h-w_sub_w;
        if ( loc > 0 ) w_displays[1].pos_y = (loc)/2;
        else w_displays[1].pos_y = 0;

        w_displays[0].width = w_main_w;
        w_displays[0].height = w_main_h;
        w_displays[1].width = w_sub_h;
        w_displays[1].height = w_sub_w;
        w_displays[1].rotation = w_rot;
    }
//    printf("main w,h[%d %d] sub w,h[%d %d]\n",main_w,main_h,sub_w,sub_h);
//    printf("[0] x,y[%d %d] w,h[%d %d]\n",w_displays[0].pos_x,w_displays[0].pos_y,w_displays[0].width,w_displays[0].height);
//    printf("[1] x,y[%d %d] w,h[%d %d]\n",w_displays[1].pos_x,w_displays[1].pos_y,w_displays[1].width,w_displays[1].height);
}

void PostWorker::post(ColorBuffer* cb) {
    // bind the subwindow eglSurface
    if (!m_initialized) {
        m_initialized = mBindSubwin();
    }
    if ( ! w_displays_init ) {
	w_displays.emplace(0, DualInfo( 0,0,0,0,0));
	w_displays.emplace(1, DualInfo( 0,0,0,0,0));
	w_displays.emplace(2, DualInfo( 0,0,0,0,0));
	w_displays.emplace(3, DualInfo( 0,0,0,0,0));
	w_displays_init = true;
        mFb->getDualSize(&w_main_w, &w_main_h, &w_sub_w, &w_sub_h, &w_gap, &w_rot);
    }

    float dpr = mFb->getDpr();
    int windowWidth = mFb->windowWidth();
    int windowHeight = mFb->windowHeight();
    float px = mFb->getPx();
    float py = mFb->getPy();
    int zRot = mFb->getZrot();

    cb->waitSync();

    // Find the x and y values at the origin when "fully scrolled."
    // Multiply by 2 because the texture goes from -1 to 1, not 0 to 1.
    // Multiply the windowing coordinates by DPR because they ignore
    // DPR, but the viewport includes DPR.
    float fx = 2.f * (m_viewportWidth  - windowWidth  * dpr) / (float)m_viewportWidth;
    float fy = 2.f * (m_viewportHeight - windowHeight * dpr) / (float)m_viewportHeight;

    // finally, compute translation values
    float dx = px * fx;
    float dy = py * fy;

    if (mFb->m_displays.size() > 1 ) {
        int combinedW, combinedH;
	// to support multidisplay rotation & skins
	if ( zRot == 0 || zRot == 180 )
            mFb->getCombinedDisplaySize(&combinedW, &combinedH);
	else
            mFb->getCombinedDisplaySize(&combinedH, &combinedW);
//printf("PostWorker w,h[%d %d]\n",combinedW, combinedH);

        mFb->getTextureDraw()->prepareForDrawLayer();
	// to support multi display rotation & skins
        for (const auto& iter : mFb->m_displays) {
            if (iter.first != 0) {
		recomputeFrameLayout(zRot);
		break;
	    }
        }
        for (const auto& iter : mFb->m_displays) {
            if ((iter.first != 0) &&
                (iter.second.width == 0 || iter.second.height == 0 || iter.second.cb == 0)) {
                continue;
            }
            // don't render 2nd cb to primary display
	    if ( cb->getDisplay() != 0 )
                printf("Second cb[%d]\n",cb->getDisplay());
            if (iter.first == 0 && cb->getDisplay() != 0) {
                continue;
            }
            ColorBuffer* multiDisplayCb = iter.first == 0 ? cb :
                mFb->findColorBuffer(iter.second.cb).get();
            if (multiDisplayCb == nullptr) {
                ERR("fail to find cb %d\n", iter.second.cb);
                continue;
            }

            ComposeLayer l;
// Overlay mask
            if ( iter.first ) {
                l.displayFrame.left = 0;
                l.displayFrame.top = 0;
                l.displayFrame.right = combinedW;
                l.displayFrame.bottom =  combinedH;
	        if ( zRot == 90 ) l.transform = (hwc_transform_t)HAL_TRANSFORM_ROT_270;
                else if ( zRot == 180 ) l.transform = (hwc_transform_t)HAL_TRANSFORM_ROT_180;
                else if ( zRot == 270 ) l.transform = (hwc_transform_t)HAL_TRANSFORM_ROT_90;
                else	                l.transform = (hwc_transform_t)0;
                multiDisplayCb->postLayerMask(&l, combinedW, combinedH);
	    }

            fillMultiDisplayPostStruct(&l, iter.first, multiDisplayCb);
//second requred rotate 270 more
//because wing screen default is 270
            int rotation = zRot;
            if ( iter.first ) {
                rotation += w_displays[iter.first].rotation * 90;
                if ( rotation >= 360 ) rotation -= 360;
            }
	    if ( rotation == 90 ) l.transform = (hwc_transform_t)HAL_TRANSFORM_ROT_270;
	    else if ( rotation == 180 ) l.transform = (hwc_transform_t)HAL_TRANSFORM_ROT_180;
	    else if ( rotation == 270 ) l.transform = (hwc_transform_t)HAL_TRANSFORM_ROT_90;
            else	            l.transform = (hwc_transform_t)0;

	    if ( !iter.first ) {
                multiDisplayCb->postLayer(&l, combinedW, combinedH);
	    }
	    else if ( emugl::get_emugl_window_operations().isSwiveled() ) {
                multiDisplayCb->postLayer(&l, combinedW, combinedH);
	    }
        }
    } else {
        // render the color buffer to the window and apply the overlay
        GLuint tex = cb->scale();
//        cb->postWithOverlay(tex, zRot, dx, dy);
        cb->post(tex, zRot, dx, dy);
    }

    s_egl.eglSwapBuffers(mFb->getDisplay(), mFb->getWindowSurface());
}

// Called whenever the subwindow needs a refresh (FrameBuffer::setupSubWindow).
// This rebinds the subwindow context (to account for
// when the refresh is a display change, for instance)
// and resets the posting viewport.
void PostWorker::viewport(int width, int height) {
    // rebind the subwindow eglSurface unconditionally---
    // this could be from a display change
    m_initialized = mBindSubwin();

    float dpr = mFb->getDpr();
    m_viewportWidth = width * dpr;
    m_viewportHeight = height * dpr;
    s_gles2.glViewport(0, 0, m_viewportWidth, m_viewportHeight);
}

// Called when the subwindow refreshes, but there is no
// last posted color buffer to show to the user. Instead of
// displaying whatever happens to be in the back buffer,
// clear() is useful for outputting consistent colors.
void PostWorker::clear() {
    s_gles2.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                    GL_STENCIL_BUFFER_BIT);
    s_egl.eglSwapBuffers(mFb->getDisplay(), mFb->getWindowSurface());
}

void PostWorker::compose(ComposeDevice* p) {
    // bind the subwindow eglSurface
    if (!m_initialized) {
        m_initialized = mBindSubwin();
    }

    ComposeLayer* l = (ComposeLayer*)p->layer;
    GLint vport[4] = { 0, };
    s_gles2.glGetIntegerv(GL_VIEWPORT, vport);
    s_gles2.glViewport(0, 0, mFb->getWidth(),mFb->getHeight());
    if (!m_composeFbo) {
        s_gles2.glGenFramebuffers(1, &m_composeFbo);
    }
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, m_composeFbo);
    s_gles2.glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0_OES,
                                   GL_TEXTURE_2D,
                                   mFb->findColorBuffer(p->targetHandle)->getTexture(),
                                   0);

    DD("worker compose %d layers\n", p->numLayers);
    mFb->getTextureDraw()->prepareForDrawLayer();
    for (int i = 0; i < p->numLayers; i++, l++) {
        DD("\tcomposeMode %d color %d %d %d %d blendMode "
               "%d alpha %f transform %d %d %d %d %d "
               "%f %f %f %f\n",
               l->composeMode, l->color.r, l->color.g, l->color.b,
               l->color.a, l->blendMode, l->alpha, l->transform,
               l->displayFrame.left, l->displayFrame.top,
               l->displayFrame.right, l->displayFrame.bottom,
               l->crop.left, l->crop.top, l->crop.right,
               l->crop.bottom);
        composeLayer(l);
    }

    mFb->findColorBuffer(p->targetHandle)->setSync();
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s_gles2.glViewport(vport[0], vport[1], vport[2], vport[3]);
    mFb->getTextureDraw()->cleanupForDrawLayer();
}

void PostWorker::compose(ComposeDevice_v2* p) {
    // bind the subwindow eglSurface
    if (!m_initialized) {
        m_initialized = mBindSubwin();
    }

    ComposeLayer* l = (ComposeLayer*)p->layer;
    GLint vport[4] = { 0, };
    s_gles2.glGetIntegerv(GL_VIEWPORT, vport);
    s_gles2.glViewport(0, 0, mFb->getWidth(),mFb->getHeight());
    if (!m_composeFbo) {
        s_gles2.glGenFramebuffers(1, &m_composeFbo);
    }
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, m_composeFbo);
    s_gles2.glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0_OES,
                                   GL_TEXTURE_2D,
                                   mFb->findColorBuffer(p->targetHandle)->getTexture(),
                                   0);

    DD("worker compose %d layers\n", p->numLayers);
    mFb->getTextureDraw()->prepareForDrawLayer();
    for (int i = 0; i < p->numLayers; i++, l++) {
        DD("\tcomposeMode %d color %d %d %d %d blendMode "
               "%d alpha %f transform %d %d %d %d %d "
               "%f %f %f %f\n",
               l->composeMode, l->color.r, l->color.g, l->color.b,
               l->color.a, l->blendMode, l->alpha, l->transform,
               l->displayFrame.left, l->displayFrame.top,
               l->displayFrame.right, l->displayFrame.bottom,
               l->crop.left, l->crop.top, l->crop.right,
               l->crop.bottom);
        composeLayer(l);
    }

    mFb->findColorBuffer(p->targetHandle)->setSync();
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s_gles2.glViewport(vport[0], vport[1], vport[2], vport[3]);
    mFb->getTextureDraw()->cleanupForDrawLayer();
}

void PostWorker::composeLayer(ComposeLayer* l) {
    if (l->composeMode == HWC2_COMPOSITION_DEVICE) {
        ColorBufferPtr cb = mFb->findColorBuffer(l->cbHandle);
        if (cb == nullptr) {
            // bad colorbuffer handle
            ERR("%s: fail to find colorbuffer %d\n", __FUNCTION__, l->cbHandle);
            return;
        }
        cb->postLayer(l, mFb->getWidth(), mFb->getHeight());
    }
    else {
        // no Colorbuffer associated with SOLID_COLOR mode
        mFb->getTextureDraw()->drawLayer(l, mFb->getWidth(), mFb->getHeight(),
                                         1, 1, 0);
    }
}

void PostWorker::screenshot(
    ColorBuffer* cb,
    int width,
    int height,
    GLenum format,
    GLenum type,
    SkinRotation rotation,
    void* pixels) {
    cb->readPixelsScaled(
        width, height, format, type, rotation, pixels);
}

PostWorker::~PostWorker() {
    if (mFb->getDisplay() != EGL_NO_DISPLAY) {
        s_egl.eglMakeCurrent(mFb->getDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE,
                             EGL_NO_CONTEXT);
    }
}
