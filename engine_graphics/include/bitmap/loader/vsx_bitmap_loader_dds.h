#ifndef VSX_BITMAP_LOADER_DDS_H
#define VSX_BITMAP_LOADER_DDS_H

#include "vsx_bitmap_loader_base.h"
#include <bitmap/vsx_bitmap_transform.h>
#include <vsx_singleton_counter.h>

// original example code kindly supplied by Jon Watte

//  little-endian, of course
#define DDS_MAGIC 0x20534444

//  DDS_header.dwFlags
#define DDSD_CAPS                   0x00000001
#define DDSD_HEIGHT                 0x00000002
#define DDSD_WIDTH                  0x00000004
#define DDSD_PITCH                  0x00000008
#define DDSD_PIXELFORMAT            0x00001000
#define DDSD_MIPMAPCOUNT            0x00020000
#define DDSD_LINEARSIZE             0x00080000
#define DDSD_DEPTH                  0x00800000

//  DDS_header.sPixelFormat.dwFlags
#define DDPF_ALPHAPIXELS            0x00000001
#define DDPF_FOURCC                 0x00000004
#define DDPF_INDEXED                0x00000020
#define DDPF_RGB                    0x00000040

//  DDS_header.sCaps.dwCaps1
#define DDSCAPS_COMPLEX             0x00000008
#define DDSCAPS_TEXTURE             0x00001000
#define DDSCAPS_MIPMAP              0x00400000

//  DDS_header.sCaps.dwCaps2
#define DDSCAPS2_CUBEMAP            0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000
#define DDSCAPS2_VOLUME             0x00200000

#define D3DFMT_DXT1 0x31545844
#define D3DFMT_DXT3 0x33545844
#define D3DFMT_DXT5 0x35545844

#define PF_IS_DXT1(pf) \
  ((pf.flags & DDPF_FOURCC) && \
   (pf.four_cc == D3DFMT_DXT1))

#define PF_IS_DXT3(pf) \
  ((pf.flags & DDPF_FOURCC) && \
   (pf.four_cc == D3DFMT_DXT3))

#define PF_IS_DXT5(pf) \
  ((pf.flags & DDPF_FOURCC) && \
   (pf.four_cc == D3DFMT_DXT5))


class vsx_bitmap_loader_dds
    : public vsx_bitmap_loader_base
{

  struct dds_header
  {
    struct
    {
      uint32_t    magic;
      uint32_t    size;
      uint32_t    flags;
      uint32_t    height;
      uint32_t    width;
      uint32_t    pitch_or_linear_size;
      uint32_t    depth;
      uint32_t    mip_map_count;
      uint32_t    reserved_1[ 11 ];

      struct
      {
        uint32_t    size;
        uint32_t    flags;
        uint32_t    four_cc;
        uint32_t    rgb_bit_count;
        uint32_t    r_bit_mask;
        uint32_t    g_bit_mask;
        uint32_t    b_bit_mask;
        uint32_t    alpha_bit_mask;
      } pixel_format;

      struct
      {
        uint32_t    caps_1;
        uint32_t    caps_2;
        uint32_t    dds_x;
        uint32_t    reserved;
      } caps;
      uint32_t    reserved_2;
    };
    char data[ 128 ];
  } __attribute__((packed));


  static void* worker(void *ptr)
  {
    vsx_texture_loader_thread_info* thread_info = ((vsx_texture_loader_thread_info*)ptr);

    vsxf* filesystem = thread_info->filesystem;
    vsx_bitmap* bitmap = thread_info->bitmap;

    vsxf_handle* file_handle = filesystem->f_open(thread_info->filename.c_str(), "rb");
    dds_header header;
    filesystem->f_read( &header, sizeof(dds_header), file_handle );

    req_error_goto(header.magic == DDS_MAGIC, "File does not start with \"DDS \"", end);
    req_error_goto(header.size == 124, "Wrong header size", end);

    bitmap->compression = PF_IS_DXT1(header.pixel_format) ? vsx_bitmap::compression_dxt1 : vsx_bitmap::compression_none;
    bitmap->compression = PF_IS_DXT3(header.pixel_format) ? vsx_bitmap::compression_dxt3 : vsx_bitmap::compression_none;
    bitmap->compression = PF_IS_DXT5(header.pixel_format) ? vsx_bitmap::compression_dxt5 : vsx_bitmap::compression_none;

    req_error_goto( bitmap->compression != vsx_bitmap::compression_none, "DDS loader only supports DXT1 or DXT3 or DXT5 compressed formats.", end);

    thread_info->bitmap->filename = thread_info->filename;
    thread_info->bitmap->width = header.width;
    thread_info->bitmap->height = header.height;
    thread_info->bitmap->channels = 4;

    if (thread_info->hint & vsx_bitmap::flip_vertical_hint)
      vsx_bitmap_transform::get_instance()->flip_vertically(thread_info->bitmap);

    if (thread_info->hint & vsx_bitmap::cubemap_split_6_1_hint)
      vsx_bitmap_transform::get_instance()->split_into_cubemap(thread_info->bitmap);

    thread_info->bitmap->timestamp = vsx_singleton_counter::get();
    __sync_fetch_and_add( &(thread_info->bitmap->data_ready), 1 );

  end:
    delete thread_info;
    return 0;
  }

  void load_internal(vsx_string<> filename, vsxf* filesystem, vsx_bitmap* bitmap, bool thread, vsx_texture_loader_thread_info* thread_info)
  {
    if (!thread)
      return (void)worker((void*)thread_info);

    new vsx_bitmap_loader_thread(worker, thread_info);
  }

public:

  static vsx_bitmap_loader_dds* get_instance()
  {
    static vsx_bitmap_loader_dds vtlj;
    return &vtlj;
  }

};

#endif