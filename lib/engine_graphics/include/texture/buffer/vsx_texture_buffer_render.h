#pragma once

#include "vsx_texture_buffer_base.h"

class vsx_texture_buffer_render
    : public vsx_texture_buffer_base
{
public:

  // init an offscreen feedback possible buffer
  void init
  (
    vsx_texture<>* texture,
    int width, // width in pixels
    int height, // height in pixels
    bool float_texture, // use floating point channels (8-bit is default)
    bool alpha, // support alpha channel or not
    bool multisample, // enable anti-aliasing
    bool linear_filter, // linear min/mag texture filter
    GLuint existing_depth_texture_id // depth buffer from other buffer, 0 to ignore
  )
  {
    VSX_UNUSED(existing_depth_texture_id);

    int i_width = width;
    int i_height = height;

    if ( !has_buffer_support() )
      VSX_ERROR_RETURN("No FBO support");

    int prev_buf_l;
    prev_buf_l = vsx_gl_state::get_instance()->framebuffer_bind_get();

    // MIN / MAG filter
    GLint min_mag = linear_filter?GL_LINEAR:GL_NEAREST;


    // create fbo for multi sampled content and attach depth and color buffers to it
    glGenFramebuffersEXT(1, &frame_buffer_handle);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frame_buffer_handle);





    // color render buffer
    glGenRenderbuffersEXT(1, &render_buffer_color_handle);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, render_buffer_color_handle);


    if(multisample)
    {
      if (float_texture)
      {
        glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, 4, alpha?GL_RGBA16F_ARB:GL_RGB16F_ARB, width, height);
      }
      else
      {
        glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, 4, alpha?GL_RGBA8:GL_RGB8, width, height);
      }
    }
    else
    {
      if (float_texture)
      {
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, alpha?GL_RGBA16F_ARB:GL_RGB16F_ARB, width, height);
      }
      else
      {
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, alpha?GL_RGBA8:GL_RGB8, width, height);
      }
    }


    // depth render buffer
    glGenRenderbuffersEXT( 1, &render_buffer_depth_handle);
    glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, render_buffer_depth_handle);

    if(multisample && GLEW_EXT_framebuffer_multisample)
    {
      glRenderbufferStorageMultisampleEXT(
        GL_RENDERBUFFER_EXT,
        4,
        GL_DEPTH_COMPONENT,
        width,
        height
      );
    }
    else
    {
      glRenderbufferStorageEXT(
        GL_RENDERBUFFER_EXT,
        GL_DEPTH_COMPONENT,
        width,
        height
      );
    }

    // attach the render buffers to the FBO handle
    glFramebufferRenderbufferEXT(
      GL_FRAMEBUFFER_EXT,
      GL_COLOR_ATTACHMENT0_EXT,
      GL_RENDERBUFFER_EXT,
      render_buffer_color_handle
    );
    glFramebufferRenderbufferEXT(
      GL_FRAMEBUFFER_EXT,
      GL_DEPTH_ATTACHMENT_EXT,
      GL_RENDERBUFFER_EXT,
      render_buffer_depth_handle
    );



    // Texture and FBO to blit into


    // create texture for blitting into
    glGenTextures(1, &frame_buffer_blit_color_texture);
    glBindTexture(GL_TEXTURE_2D, frame_buffer_blit_color_texture);
    if (float_texture)
    {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, i_width, i_height, 0, GL_RGBA, GL_FLOAT, NULL);
    }
    else
    {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, i_width, i_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,0);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)min_mag);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)min_mag);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set your texture parameters here if required ...



    // create a normal fbo and attach texture to it
    glGenFramebuffersEXT(1, &frame_buffer_blit_handle);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frame_buffer_blit_handle);

    // attach the texture to the FBO
    glFramebufferTexture2DEXT(
      GL_FRAMEBUFFER_EXT,
      GL_COLOR_ATTACHMENT0_EXT,
      GL_TEXTURE_2D,
      frame_buffer_blit_color_texture,
      0
    );

    texture->texture->gl_id = frame_buffer_blit_color_texture;
    texture->texture->gl_type = GL_TEXTURE_2D;
    texture->texture->uploaded_to_gl = true;
    this->width = width;
    this->height = height;

    // restore eventual previous buffer
    vsx_gl_state::get_instance()->framebuffer_bind(prev_buf_l);

    valid_fbo = true; // fbo valid for capturing
  }




  // run in stop/start or when changing resolution
  void reinit
  (
    vsx_texture<>* texture,
    int width, // width in pixels
    int height, // height in pixels
    bool float_texture, // use floating point channels (8-bit is default)
    bool alpha, // support alpha channel or not
    bool multisample, // enable anti-aliasing
    bool linear_filter, // linear min/mag texture filter
    GLuint existing_depth_texture_id
  )
  {
    deinit(texture);
    init
    (
      texture,
      width,
      height,
      float_texture,
      alpha,
      multisample,
      linear_filter,
      existing_depth_texture_id
    );
  }



  void deinit(vsx_texture<>* texture)
  {
    if (!render_buffer_color_handle)
      return;

    glDeleteRenderbuffersEXT(1,&render_buffer_color_handle);
    glDeleteRenderbuffersEXT(1,&render_buffer_depth_handle);
    glDeleteTextures(1,&frame_buffer_blit_color_texture);
    glDeleteFramebuffersEXT(1, &frame_buffer_handle);
    glDeleteFramebuffersEXT(1, &frame_buffer_blit_handle);
    valid_fbo = false;
    texture->texture->gl_id = 0;
    texture->texture->gl_type = 0;
    texture->texture->uploaded_to_gl = false;
  }

  void begin_capture_to_buffer()
  {
    if (!valid_fbo)
      VSX_ERROR_RETURN("trying to capture to an invalid buffer");

    if (capturing_to_buffer)
      VSX_ERROR_RETURN("trying to capture to a buffer already capturing");

    vsx_gl_state::get_instance()->viewport_get( &viewport_size[0] );

    prev_buf = vsx_gl_state::get_instance()->framebuffer_bind_get();
    glPushAttrib(GL_ALL_ATTRIB_BITS );
    vsx_gl_state::get_instance()->matrix_get_v( VSX_GL_PROJECTION_MATRIX, buffer_save_matrix[0].m );
    vsx_gl_state::get_instance()->matrix_get_v( VSX_GL_MODELVIEW_MATRIX, buffer_save_matrix[1].m );
    vsx_gl_state::get_instance()->matrix_get_v( VSX_GL_TEXTURE_MATRIX, buffer_save_matrix[2].m );


    vsx_gl_state::get_instance()->matrix_mode( VSX_GL_PROJECTION_MATRIX );
    vsx_gl_state::get_instance()->matrix_load_identity();
    vsx_gl_state::get_instance()->matrix_mode( VSX_GL_MODELVIEW_MATRIX );
    vsx_gl_state::get_instance()->matrix_load_identity();
    vsx_gl_state::get_instance()->matrix_mode( VSX_GL_TEXTURE_MATRIX );
    vsx_gl_state::get_instance()->matrix_load_identity();

    buffer_save_blend = vsx_gl_state::get_instance()->blend_get();
    vsx_gl_state::get_instance()->blend_set(1);

    vsx_gl_state::get_instance()->framebuffer_bind(frame_buffer_handle);
    vsx_gl_state::get_instance()->viewport_set(0, 0, width, height);

    capturing_to_buffer = true;
  }


  void end_capture_to_buffer()
  {
    if (!valid_fbo)
      VSX_ERROR_RETURN("trying to capture to an invalid buffer");

    if (!capturing_to_buffer)
      VSX_ERROR_RETURN("trying to stop capturing to a buffer never capturing");

    glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, frame_buffer_handle);
    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, frame_buffer_blit_handle);
    glBlitFramebufferEXT(
      0,
      0,
      width - 1,
      height - 1,
      0,
      0,
      width - 1,
      height - 1,
      GL_COLOR_BUFFER_BIT,
      GL_NEAREST
    );

    vsx_gl_state::get_instance()->framebuffer_bind(prev_buf);
    vsx_gl_state::get_instance()->matrix_mode( VSX_GL_PROJECTION_MATRIX );
    vsx_gl_state::get_instance()->matrix_load_identity();
    vsx_gl_state::get_instance()->matrix_mult_f( buffer_save_matrix[0].m );
    vsx_gl_state::get_instance()->matrix_mode( VSX_GL_MODELVIEW_MATRIX );
    vsx_gl_state::get_instance()->matrix_load_identity();
    vsx_gl_state::get_instance()->matrix_mult_f( buffer_save_matrix[1].m );
    vsx_gl_state::get_instance()->matrix_mode( VSX_GL_TEXTURE_MATRIX );
    vsx_gl_state::get_instance()->matrix_load_identity();
    vsx_gl_state::get_instance()->matrix_mult_f( buffer_save_matrix[2].m );
    vsx_gl_state::get_instance()->blend_set(buffer_save_blend);

    glPopAttrib();

    vsx_gl_state::get_instance()->viewport_set( &viewport_size[0] );

    capturing_to_buffer = false;
  }


};
