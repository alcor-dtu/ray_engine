/*
 * Copyright (c) 2008 - 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and proprietary
 * rights in and to this software, related documentation and any modifications thereto.
 * Any use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation is strictly
 * prohibited.
 *
 * TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED *AS IS*
 * AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS BE LIABLE FOR ANY
 * SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT
 * LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF
 * BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR
 * INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES
 */

#include <glfw_display.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <sample_scene.h>
#include <mouse.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <iostream>
#include <cstdio> //sprintf
#include <sstream>
#include "logger.h"
#include "camera_host.h"

 // #define NVTX_ENABLE enables the nvToolsExt stuff from Nsight in NsightHelper.h
 //#define NVTX_ENABLE

 //-----------------------------------------------------------------------------
 // 
 // GLFWDisplay class implementation 
 //-----------------------------------------------------------------------------

Mouse*         GLFWDisplay::mMouse = nullptr;
SampleScene*   GLFWDisplay::mScene = nullptr;
GLFWwindow*    GLFWDisplay::mWindow = nullptr;
bool           GLFWDisplay::mDisplayedFrames = true;

bool           GLFWDisplay::mIssRGBSupported = false;
bool           GLFWDisplay::mUsesRGB = false;

bool           GLFWDisplay::mInitialized = false;
std::string    GLFWDisplay::mTitle = "";

bool            GLFWDisplay::mRequiresDisplay = true;


void GLFWDisplay::init(int& argc, char** argv)
{
    mInitialized = true;

    if (mRequiresDisplay)
    {
        if (!glfwInit())
        {
            Logger::error << "Error initializing GLFW " << std::endl;
        }
    }
}

void GLFWDisplay::run(const std::string& title, SampleScene* scene)
{
    if (!mInitialized) {
        std::cerr << "ERROR - GLFWDisplay::run() called before GLFWDisplay::init()" << std::endl;
        exit(2);
    }
    mScene = scene;
    mTitle = title;

    if (mRequiresDisplay)
    {
        mWindow = glfwCreateWindow(1, 1, mTitle.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(mWindow);

        glewInit();
        GLint glewInitResult = glewInit();
        if (GLEW_OK != glewInitResult)
        {
            printf("ERROR: %s\n", glewGetErrorString(glewInitResult));
            exit(EXIT_FAILURE);
        }

        if (glewIsSupported("GL_EXT_texture_sRGB GL_EXT_framebuffer_sRGB")) {
            mIssRGBSupported = true;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        if (!mWindow)
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        mWindow = nullptr;
    }

    int buffer_width;
    int buffer_height;
    try {
        // Set mUp scene
        mScene->initialize_scene(mWindow);

        // Initialize camera according to scene params
        optix::Buffer buffer = mScene->get_output_buffer();
        RTsize buffer_width_rts, buffer_height_rts;
        buffer->getSize(buffer_width_rts, buffer_height_rts);
        buffer_width = static_cast<int>(buffer_width_rts);
        buffer_height = static_cast<int>(buffer_height_rts);
        mMouse = new Mouse(mScene->get_camera(), buffer_width, buffer_height);
        mMouse->handleMouseFunc(0, 0, -1, GLFW_PRESS, 0);
    }
    catch (optix::Exception& e) {
        Logger::error << (e.getErrorString().c_str());
        exit(2);
    }

    mScene->scene_initialized();

    if (mRequiresDisplay)
    {

        glfwSetWindowSize(mWindow, buffer_width, buffer_height);
        // Initialize state
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, 1, 0, 1, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glViewport(0, 0, buffer_width, buffer_height);

        glfwSetKeyCallback(mWindow, key_pressed);
        glfwSetMouseButtonCallback(mWindow, mouse_button);
        glfwSetCursorPosCallback(mWindow, mouse_moving);
        glfwSetWindowSizeCallback(mWindow, resize);

        while (!glfwWindowShouldClose(mWindow))
        {
            glfwPollEvents();
            display();
        }
    }
    else
    {
        while (true)
        {
            display();
        }
    }
}


void GLFWDisplay::resize(GLFWwindow * window, int width, int height)
{
    // disallow size 0
    width = optix::max(1, width);
    height = optix::max(1, height);
    mScene->get_camera()->set_aspect_ratio(width / (float)height);

    mScene->signalCameraChanged();
    mMouse->handleResize(width, height);

    glfwSetWindowSize(window, width, height);

    try {
        mScene->resize(width, height);
    }
    catch (optix::Exception& e) {
        Logger::error << (e.getErrorString().c_str());
        exit(2);
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glViewport(0, 0, width, height);
}

void GLFWDisplay::display_frame()
{
    GLboolean sRGB = GL_FALSE;
    if (mUsesRGB && mIssRGBSupported) {
        glGetBooleanv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &sRGB);
        if (sRGB) {
            glEnable(GL_FRAMEBUFFER_SRGB_EXT);
        }
    }

    // Draw the resulting image
    optix::Buffer buffer = mScene->get_output_buffer();
    RTsize buffer_width_rts, buffer_height_rts;
    buffer->getSize(buffer_width_rts, buffer_height_rts);
    const int buffer_width = static_cast<int>(buffer_width_rts);
    const int buffer_height = static_cast<int>(buffer_height_rts);
    const RTformat buffer_format = buffer->getFormat();
    const unsigned int vboId = buffer->getGLBOId();

    if (vboId)
    {
        static GLuint texture = 0;
        if (!texture)
        {
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            // Change these to GL_LINEAR for super- or sub-sampling
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

            // GL_CLAMP_TO_EDGE for linear filtering, not relevant for nearest.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBindTexture(GL_TEXTURE_2D, texture);
        // send pbo to texture
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, vboId);

        const RTsize elementSize = buffer->getElementSize();
        if ((elementSize % 8) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
        else if ((elementSize % 4) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        else if ((elementSize % 2) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
        else                             glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (buffer_format == RT_FORMAT_UNSIGNED_BYTE4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, buffer_width, buffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        }
        else if (buffer_format == RT_FORMAT_FLOAT4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, buffer_width, buffer_height, 0, GL_RGBA, GL_FLOAT, 0);
        }
        else if (buffer_format == RT_FORMAT_FLOAT3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F_ARB, buffer_width, buffer_height, 0, GL_RGB, GL_FLOAT, 0);
        }
        else if (buffer_format == RT_FORMAT_FLOAT) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, buffer_width, buffer_height, 0, GL_LUMINANCE, GL_FLOAT, 0);
        }
        else {
            assert(0 && "Unknown buffer format");
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glEnable(GL_TEXTURE_2D);

        // Initialize offsets to pixel center sampling.

        float u = 0.5f / buffer_width;
        float v = 0.5f / buffer_height;

        glBegin(GL_QUADS);
        glTexCoord2f(u, v);
        glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, v);
        glVertex2f(1.0f, 0.0f);
        glTexCoord2f(1.0f - u, 1.0f - v);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(u, 1.0f - v);
        glVertex2f(0.0f, 1.0f);
        glEnd();

        glDisable(GL_TEXTURE_2D);
    }
    else {
        GLvoid* imageData = buffer->map();
        assert(imageData);

        GLenum gl_data_type = GL_FALSE;
        GLenum gl_format = GL_FALSE;

        switch (buffer_format) {
        case RT_FORMAT_UNSIGNED_BYTE4:
            gl_data_type = GL_UNSIGNED_BYTE;
            gl_format = GL_RGBA;
            break;

        case RT_FORMAT_FLOAT:
            gl_data_type = GL_FLOAT;
            gl_format = GL_LUMINANCE;
            break;

        case RT_FORMAT_FLOAT3:
            gl_data_type = GL_FLOAT;
            gl_format = GL_RGB;
            break;

        case RT_FORMAT_FLOAT4:
            gl_data_type = GL_FLOAT;
            gl_format = GL_RGBA;
            break;

        default:
            fprintf(stderr, "Unrecognized buffer data type or format.\n");
            exit(2);
            break;
        }

        const RTsize elementSize = buffer->getElementSize();
        int align = 1;
        if ((elementSize % 8) == 0) align = 8;
        else if ((elementSize % 4) == 0) align = 4;
        else if ((elementSize % 2) == 0) align = 2;
        glPixelStorei(GL_UNPACK_ALIGNMENT, align);

        glDrawPixels(static_cast<GLsizei>(buffer_width), static_cast<GLsizei>(buffer_height),
            gl_format, gl_data_type, imageData);

        buffer->unmap();

    }
    if (mUsesRGB && mIssRGBSupported && sRGB) {
        glDisable(GL_FRAMEBUFFER_SRGB_EXT);
    }
}

void GLFWDisplay::display()
{

    bool display_requested = mRequiresDisplay;
    mMouse->setCamera(mScene->get_camera());

    try 
    {
        mScene->trace();

        if (display_requested && mDisplayedFrames) {
            // Only enable for debugging
            // glClearColor(1.0, 0.0, 0.0, 0.0);
            // glClear(GL_COLOR_BUFFER_BIT);

            display_frame();
        }
    }
    catch (optix::Exception& e) {
        std::cout << (e.getErrorString().c_str());
        exit(2);
    }

    mScene->post_draw_callback();

    std::string debug;

    if (display_requested && mDisplayedFrames) {
        // Swap buffers
        glfwSwapBuffers(mWindow);
    }
}

void GLFWDisplay::key_pressed(GLFWwindow * window, int key, int scancode, int action, int modifier)
{
    try {
        if (mScene->key_pressed(key, action, modifier)) {
            return;
        }
    }
    catch (optix::Exception& e) {
        Logger::error << (e.getErrorString().c_str());
        exit(2);
    }

    switch (key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        quit();
        break;
    default:
        return;
    }
}

void GLFWDisplay::mouse_button(GLFWwindow * window, int button, int action, int modifiers)
{
    double xd, yd;
    glfwGetCursorPos(window, &xd, &yd);
    int x = static_cast<int>(xd);
    int y = static_cast<int>(yd);
    if (!mScene->mouse_pressed(x, y, button, action, modifiers))
    {
        mMouse->handleMouseFunc(x, y, button, action, modifiers);
        mScene->signalCameraChanged();
    }
}

void GLFWDisplay::mouse_moving(GLFWwindow * window, double xd, double yd)
{
    int x = static_cast<int>(xd);
    int y = static_cast<int>(yd);
    mMouse->handleMoveFunc(x, y);
    if (mMouse->handleMoveFunc(x, y))
    {
        mScene->signalCameraChanged();
    }
}


void GLFWDisplay::quit(int return_code)
{
    try {
        if (mScene)
        {
            mScene->clean_up();
            if (mScene->get_context().get() != 0)
            {
                Logger::error << ("Derived scene class failed to call SampleScene::clean_up()");
                exit(2);
            }
        }
        exit(return_code);
    }
    catch (optix::Exception& e) {
        Logger::error << (e.getErrorString().c_str());
        exit(2);
    }
}
