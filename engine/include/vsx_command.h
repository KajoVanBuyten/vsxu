/**
* Project: VSXu Engine: Realtime modular visual programming engine.
*
* This file is part of Vovoid VSXu Engine.
*
* @author Jonatan Wallmander, Robert Wenzel, Vovoid Media Technologies AB Copyright (C) 2003-2013
* @see The GNU Lesser General Public License (LGPL)
*
* VSXu Engine is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU Lesser General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef VSX_COMMAND_H
#define VSX_COMMAND_H

#include <vsx_platform.h>
#include <map>
#include <list>
#include <vector>
#include "vsxfst.h"
#include <vsx_string_helper.h>
#include "vsx_engine_dllimport.h"

#ifdef VSXU_DEBUG
  #include "debug/vsx_error.h"
#endif


// different types of commands
#define VSX_COMMAND_PARAM_SET 1
#define VSX_COMMAND_MENU 2
#define VSX_COMMAND 3

#define VSX_COMMAND_DELETE_ITERATIONS 50
#define VSX_COMMAND_GARBAGE_COLLECT true
// command specification (container class)
// this is used within the whole system. that is both the client, who has both a vsx_engine and an instance of the gui
// widget system, and in the server.
// it works somewhat the way windows messages works, like "do this"

// Each instance of vsxu program running has a global command processing queue.
// The vsx_command_list is there for this, it acts as a FIFO buffer for commands as well as reading and managing
// possible commands that appear in menus etc.

// cut'n'paste examples:
// commands.adds(VSX_COMMAND_MENU, "Connect to server", "desktop.server.connect","");
//            ^-- adds sys item ^-- type menu

//**********************************************************************************************************************
class vsx_command_s;
ENGINE_DLLIMPORT extern std::vector<vsx_command_s*> vsx_command_garbage_list; // the parts of the command
ENGINE_DLLIMPORT void vsx_command_process_garbage();
ENGINE_DLLIMPORT void vsx_command_process_garbage_exit();


ENGINE_DLLIMPORT class vsx_command_s
{
public:
  ENGINE_DLLIMPORT static int id;
  int iterations;
  #ifdef VSXU_DEBUG
    bool garbage_collected;
  #endif
  bool parsed;
  int owner; // for color-coding this command
  int type; // type of command
  vsx_string<>title; // Title - for internal GUI stuff like menus and stuff
  vsx_string<>cmd; // the first part of the command, the actual command
  vsx_string<>cmd_data; // the second parameter (for simple commands)
  vsx_avector<char> cmd_data_bin; // the binary part of the command
  vsx_string<>raw; // the unparsed command, empty when binary command
  std::vector< vsx_string<> > parts; // the parts of the command

  vsx_command_s() {
    iterations = 0;
    #ifdef VSXU_DEBUG
      garbage_collected = false;
    #endif
    parsed = false;
    type = 0;
    ++id;
  }

  void copy (vsx_command_s *t) {
    owner = t->owner;
    type = t->type;
    title = t->title;
    cmd = t->cmd;
    cmd_data = t->cmd_data;
    raw = t->raw;
    parts = t->parts;
  }

  vsx_string<>str() {
    if (raw.size())
    return raw; else
    return cmd + " " + cmd_data;
  }
  ENGINE_DLLIMPORT void parse();

  void gc()
  {
    #ifdef VSXU_DEBUG
      if (garbage_collected)
        VSX_ERROR_RETURN(L"double garbage collection");
    #endif
    vsx_command_garbage_list.push_back(this);
    #ifdef VSXU_DEBUG
      garbage_collected = true;
    #endif
  }

  // returns a string like "part1 part2 part3" if start was 1 and end was 3
  ENGINE_DLLIMPORT vsx_string<>get_parts(int start = 0, int end = -1);

  void dump_to_stdout()
  {
  	for (size_t i = 0; i < parts.size(); i++)
  	printf("%s ",parts[i].c_str());
  	printf("\n");
  }
};


template<class T>
T* vsx_command_parse(vsx_string<>& cmd_raw, bool garbage_collect = false)
{
  std::vector< vsx_string<> > cmdps;
  T *t = new T;
  if (garbage_collect)
    t->gc();

  t->raw = cmd_raw;
  //printf("parsing raw: %s\n",cmd_raw.c_str());
  vsx_string<>deli = " ";
  split_string(cmd_raw, deli, cmdps);
  //printf("parsing2: %s\n",cmd_raw.c_str());
  t->cmd = cmdps[0];
  if (cmdps.size() > 1)
  {
    t->cmd_data = cmdps[1];
  }
  t->parts = cmdps;
  t->parsed = true;
  return t;
}



#endif
