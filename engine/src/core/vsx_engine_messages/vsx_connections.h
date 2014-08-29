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

if (cmd == "param_connect")
{
  // syntax:
  //       0            1          2          3          4
  //  param_connect [in-comp] [in-param] [out-comp] [out-param]
  if (c->parts.size() >= 5)
  {
    vsx_comp* dest = get_component_by_name(c->parts[1]);
    //if (dest->get_params_in()->get_by_name
    vsx_comp* src  = get_component_by_name(c->parts[3]);
    if (dest && src)
    {
      vsx_engine_param* dest_param = dest->get_params_in()->get_by_name(c->parts[2]);
      vsx_engine_param* src_param = src->get_params_out()->get_by_name(c->parts[4]);
      if (dest_param && src_param)
      {
        if (!dest_param->sequence)
        {
          int order = dest_param->connect(src_param);
          // connect the first param to the second, let the parameter class handle wether or not it's an alias, to set up a channel etc.
          if (order != -1)
          {
            if (c->parts.size() != 6)
            {
              cmd_out->add_raw("param_connect_volatile "+c->parts[1]+" "+c->parts[2]+" "+c->parts[3]+" "+c->parts[4]+" "+vsx_string_helper::i2s(order), VSX_COMMAND_GARBAGE_COLLECT);
            }
          }
        }
        else
          cmd_out->add_raw("alert_fail "+base64_encode(c->raw)+" Error "+base64_encode("- Can not connect -| This parameter is sequenced!"));
      }
      else
      {
        if (!src_param) cmd_out->add_raw("alert_fail "+base64_encode(c->raw)+" Error "+base64_encode("- Can not connect -| Source parameter declared in parameter specification but not bound in module!|Contact module author of|"+c->parts[3]+"::"+c->parts[4])+"!");
        if (!dest_param) cmd_out->add_raw("alert_fail "+base64_encode(c->raw)+" Error "+base64_encode("- Can not connect -| Destination parameter declared in parameter specification but not bound in module!|Contact module author of |"+c->parts[1]+"::"+c->parts[2])+"!");
      }
    }
    else
    {
      if (!dest)
      cmd_out->add_raw("alert_fail "+base64_encode(c->raw)+" Error "+base64_encode("Contact module author:|destination module param \""+c->parts[2]+" lacks implementation!"));
    }
  }
  else cmd_out->add_raw("alert_fail "+base64_encode(c->raw)+" Error "+base64_encode("Can not connect:| Neither source or dest component exists."));
  goto process_message_queue_end;
}




if (cmd == "param_disconnect")
{
  // syntax:
  //  param_disconnect [in-comp] [in-param] [out-comp] [out-param]
  if (c->parts.size() == 5)
  {
    vsx_comp* dest = get_component_by_name(c->parts[1]);
    vsx_comp* src  = get_component_by_name(c->parts[3]);
    if (dest && src) {
      vsx_engine_param* dest_param = dest->get_params_in()->get_by_name(c->parts[2]);
      vsx_engine_param* src_param = src->get_params_out()->get_by_name(c->parts[4]);
      if (dest_param->disconnect(src_param) != -1) {
        cmd_out->add_raw("param_disconnect_ok "+c->parts[1]+" "+c->parts[2]+" "+c->parts[3]+" "+c->parts[4]);
      }
      else
      cmd_out->add_raw("alert_fail "+base64_encode(c->raw)+" Error "+base64_encode("Can not disconnect, failed. You shouldn't see this, did you type the command manually?"));
    }
  }
  goto process_message_queue_end;
}






if (cmd == "param_alias")
{
  // syntax:
  //        0          1           2            3           4              5                 6
  //   param_alias [p_def] [-1=in / 1=out] [component] [parameter] [source_component] [source_parameter]
  if (c->parts.size() >= 7) {
    // the source here is the param to be aliased, the dest is the destination component that is to own the new alias
    vsx_comp* src  = get_component_by_name(c->parts[5]);
    vsx_comp* dest = get_component_by_name(c->parts[3]);
    if (dest && src) {
      vsx_engine_param_list* src_l;
      vsx_engine_param_list* dest_l;
      if (c->parts[2] == "-1")
      {
        src_l = src->get_params_in();
        dest_l = dest->get_params_in();
      }
      else
      {
        src_l = src->get_params_out();
        dest_l = dest->get_params_out();
      }

      vsx_engine_param* src_param = src_l->get_by_name(c->parts[6]);
      if (src_param) {
        // alias the parameter into the paramlist of the destination component
        vsx_string new_name = dest_l->alias_get_unique_name(c->parts[4]);
        int order = dest_l->alias(src_param, new_name);

        // compute new name for c->parts[1]
        std::vector<vsx_string> parts;
        vsx_string deli = ":";
        explode(c->parts[1], deli, parts, 2);
        parts[0] = new_name;
        c->parts[1] = implode(parts, deli);

        cmd_out->add_raw("param_alias_ok "+c->parts[1]+" "+c->parts[2]+" "+c->parts[3]+" "+new_name+" "+c->parts[5]+" "+c->parts[6]+" "+vsx_string_helper::i2s(order));

      }
    }
  }
  goto process_message_queue_end;
}





if (cmd == "param_unalias")
{
  // syntax:
  //   param_unalias [-1/1] [component] [param_name]
  vsx_comp* dest = get_component_by_name(c->parts[2]);
  if (dest) {
    //vsx_engine_param* param;
    bool result;
    if (c->parts[1] == "-1") {
      result = dest->get_params_in()->unalias(c->parts[3]);
    } else
    {
      result = dest->get_params_out()->unalias(c->parts[3]);
    }
    if (result)
    cmd_out->add_raw("param_unalias_ok "+c->parts[1]+" "+c->parts[2]+" "+c->parts[3]);
    else
    cmd_out->add_raw("alert_fail "+base64_encode(c->raw)+" Error "+base64_encode("Could not unalias, this is a possible bug."));
  }
  goto process_message_queue_end;
}





if (cmd == "connections_order")
{
  //syntax:
  //  connections_order_ok [component] [param] [specification]
  vsx_comp* dest = get_component_by_name(c->parts[1]);
  if (dest) {
    if (dest->get_params_in()->order(c->parts[2],c->parts[3]) > 0)
    cmd_out->add_raw("connections_order_ok "+c->parts[1]+" "+c->parts[2]+" "+c->parts[3]);
    else
    cmd_out->add_raw("alert_fail "+base64_encode(c->raw)+" Error "+base64_encode("Could not order connections."));
  }
  goto process_message_queue_end;
}

