#pragma once

#include <vsx_common_dllimport.h>
#include <container/vsx_nw_vector.h>
#include <string/vsx_string.h>
#include <tools/vsx_lock.h>

#include <filesystem/vsx_file.h>
#include <filesystem/vsx_filesystem_archive.h>
#include <filesystem/vsx_filesystem_archive_info.h>
#include <filesystem/vsx_filesystem_archive_base.h>

#define VSXF_NUM_ADD_THREADS 8
#define VSXF_WORK_CHUNK_MAX_SIZE 1024*1024*5

namespace vsx
{
  class COMMON_DLLIMPORT filesystem_archive
  {
  public:
    enum archive_type_t
    {
      archive_none,
      archive_vsx,
      archive_vsxz
    };
  private:

    filesystem_archive_base* archive = 0x0;
    archive_type_t archive_type = archive_none;

  public:

    void create(const char* filename, archive_type_t type);
    int load(const char* archive_filename, bool load_data_multithreaded);
    void close();

    int file_add(vsx_string<> filename, vsx_string<> disk_filename, bool deferred_multithreaded);

    bool is_archive();
    bool is_archive_populated();
    bool is_file(vsx_string<> filename);

    vsx_nw_vector<filesystem_archive_info>* files_get();

    void file_open(const char* filename, const char* mode, file* &handle);
    void file_close(file* handle);
  };
}
