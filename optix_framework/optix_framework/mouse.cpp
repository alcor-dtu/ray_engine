
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

#include "mouse.h"
#include <optixu/optixu_matrix_namespace.h>
#include <iostream>
#include "math_utils.h"
#include <GLFW/glfw3.h>
#include <cmath>
#define ISFINITE std::isfinite

#define GLFW_MOTION -1
#include "logger.h"
#include "camera_host.h"

//-----------------------------------------------------------------------------
// 
// Helpers
//
//-----------------------------------------------------------------------------

namespace {

  float3 projectToSphere( float x, float y, float radius )
  {
    x /= radius;
    y /= radius;
    float rad2 = x*x+y*y;
    if(rad2 > 1.0f) {
      float rad = sqrt(rad2);
      x /= rad;
      y /= rad;
      return optix::make_float3( x, y, 0.0f );
    } else {
      float z = sqrt(1-rad2);
      return optix::make_float3( x, y, z );
    }
  }

  optix::Matrix4x4 rotationMatrix( const optix::float3& _to, const optix::float3& _from )
  {
    float3 from = normalize( _from );
    float3 to   = normalize( _to );

    float3 v = cross(from, to);
    float  e = dot(from, to);
    if ( e > 1.0f-1.e-9f ) {
      return optix::Matrix4x4::identity();
    } else {
      float h = 1.0f/(1.0f + e);
      float mtx[16];
      mtx[0] = e + h * v.x * v.x;
      mtx[1] = h * v.x * v.y + v.z;
      mtx[2] = h * v.x * v.z - v.y;
      mtx[3] = 0.0f;

      mtx[4] = h * v.x * v.y - v.z;
      mtx[5] = e + h * v.y * v.y;
      mtx[6] = h * v.y * v.z + v.x;
      mtx[7] = 0.0f; 

      mtx[8] = h * v.x * v.z + v.y;
      mtx[9] = h * v.y * v.z - v.x;
      mtx[10] = e + h * v.z * v.z;
      mtx[11] = 0.0f; 
      
      mtx[12] = 0.0f; 
      mtx[13] = 0.0f; 
      mtx[14] = 0.0f; 
      mtx[15] = 1.0f; 

      return optix::Matrix4x4( mtx );
    }
  }
}
  
  
//-----------------------------------------------------------------------------
// 
// Mouse definition 
//
//-----------------------------------------------------------------------------

Mouse::Mouse(Camera* camera, int xres, int yres)
  : camera(camera)
  , xres(xres)
  , yres(yres)
  , fov_speed(2)
  , dolly_speed(5)
  , translate_speed(33)
{
}

bool Mouse::handleMouseFunc(int x, int y, int button, int action, int modifier)
{
  switch(action) {
  case GLFW_PRESS:
    current_interaction = InteractionState(modifier, button, GLFW_PRESS);
    return call_func(x,y);
    break;
  case GLFW_RELEASE:
	  current_interaction.state = GLFW_RELEASE;
    break;
  }
  return false;
}

bool Mouse::handleMoveFunc(int x, int y)
{
	if (current_interaction.state == GLFW_PRESS || current_interaction.state == GLFW_MOTION)
	{
		current_interaction.state = GLFW_MOTION;
		return call_func(x, y);
	}
	return false;
}

void Mouse::handlePassiveMotionFunc(int x, int y)
{
}

void Mouse::handleResize(int new_xres, int new_yres)
{
  xres = new_xres;
  yres = new_yres;
  camera->set_aspect_ratio(static_cast<float>(xres)/yres);
}

bool Mouse::call_func(int x, int y)
{
  int modifier = current_interaction.modifier;
  int button   = current_interaction.button;
  bool camera_changed = false;
  if (modifier == 0 && button == GLFW_MOUSE_BUTTON_LEFT)
  {
	  rotate(x, y); camera_changed = true;
  }
  if (modifier == 0 && button == GLFW_MOUSE_BUTTON_MIDDLE)
  {
	  translate(x, y); camera_changed = true;
  }
  if (modifier == GLFW_MOD_SHIFT    && button == GLFW_MOUSE_BUTTON_RIGHT)
  {
	  fov(x, y); camera_changed = true;
  }
  if (modifier == 0 && button == GLFW_MOUSE_BUTTON_RIGHT)
  {
	  dolly(x, y); camera_changed = true;
  }
  return camera_changed;
}

void Mouse::fov(int x, int y)
{
  if(current_interaction.state == GLFW_MOTION) {
    float xmotion = (current_interaction.last_x - x)/static_cast<float>(xres);
    float ymotion = (current_interaction.last_y - y)/static_cast<float>(yres);
    float scale;
    if(fabsf(xmotion) > fabsf(ymotion))
      scale = xmotion;
    else
      scale = ymotion;
    scale *= fov_speed;

    if (scale < 0.0f)
      scale = 1.0f/(1.0f-scale);
    else
      scale += 1.0f;

    // Manipulate Camera
    camera->scale_fov(scale);
  }
  current_interaction.last_x = x;
  current_interaction.last_y = y;
}


void Mouse::translate(int x, int y)
{
  if(current_interaction.state == GLFW_MOTION) {
    float xmotion =  float(current_interaction.last_x - x)/xres;
    float ymotion = -float(current_interaction.last_y - y)/yres;
    float2 translation = optix::make_float2(xmotion, ymotion) * translate_speed;

    camera->translate(translation);
  }
  current_interaction.last_x = x;
  current_interaction.last_y = y;
}


void Mouse::dolly(int x, int y)
{
  if(current_interaction.state == GLFW_MOTION) {
    float xmotion = -float(current_interaction.last_x - x)/xres;
    float ymotion = -float(current_interaction.last_y - y)/yres;

    float scale;
    if(fabsf(xmotion) > fabsf(ymotion))
      scale = xmotion;
    else
      scale = ymotion;
    scale *= dolly_speed;

    camera->dolly(scale);
  }
  current_interaction.last_x = x;
  current_interaction.last_y = y;
}

void Mouse::rotate(int x, int y)
{

  float xpos = 2.0f*static_cast<float>(x)/static_cast<float>(xres) - 1.0f;
  float ypos = 1.0f - 2.0f*static_cast<float>(y)/static_cast<float>(yres);

  if ( current_interaction.state == GLFW_PRESS ) {
    
    current_interaction.rotate_from = projectToSphere( xpos, ypos, 0.8f );

  } else if(current_interaction.state == GLFW_MOTION) {

    float3 to( projectToSphere( xpos, ypos, 0.8f ) );
    float3 from( current_interaction.rotate_from );
  
	optix::Matrix4x4 m = rotationMatrix( to, from);
    current_interaction.rotate_from = to;
    camera->transform( m ); 
  }
  current_interaction.last_x = x;
  current_interaction.last_y = y;

}

void Mouse::track_and_pan(int x, int y)
{
}

void Mouse::track(int x, int y)
{
}

void Mouse::pan(int x, int y)
{
}

void Mouse::transform( const optix::Matrix4x4& trans )
{
  camera->transform(trans);
}

//-----------------------------------------------------------------------------
// 
// PinholeCamera definition 
//
//-----------------------------------------------------------------------------

