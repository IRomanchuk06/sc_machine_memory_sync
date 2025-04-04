/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_memory_ext.h"

#include <gmodule.h>

#include "sc-core/sc-base/sc_allocator.h"

#include "sc-store/sc-base/sc_message.h"
#include "sc-store/sc-fs-memory/sc_file_system.h"

GList * modules_priority_list = null_ptr;

//! Type of module function
typedef sc_result (*fModuleInitializeFunc)(sc_addr);
typedef sc_result (*fModuleShutdownFunc)();
typedef sc_uint32 (*fModulePriorityFunc)();

typedef struct
{
  GModule * ptr;
  sc_char * path;
  sc_uint32 priority;
  fModuleInitializeFunc init_func;
  fModuleShutdownFunc shut_func;
} sc_module_info;

void sc_module_info_free(sc_pointer mi)
{
  sc_module_info * info = (sc_module_info *)mi;

  if (info->path)
    sc_mem_free(info->path);
  if (info->ptr)
    g_module_close(info->ptr);
  sc_mem_free(info);
}

gint sc_priority_less(gconstpointer a, gconstpointer b)
{
  sc_module_info * ma = (sc_module_info *)a;
  sc_module_info * mb = (sc_module_info *)b;

  if (ma->priority < mb->priority)
    return -1;
  if (ma->priority > mb->priority)
    return 1;

  return strcmp(ma->path, mb->path);
}

gint sc_priority_great(gconstpointer a, gconstpointer b)
{
  return sc_priority_less(b, a);
}

sc_char * _sc_module_build_path(sc_char const * directory, sc_char const * module_name)
{
  return g_strconcat(directory, "/", module_name, null_ptr);
}

sc_result _sc_ext_collect_extensions_from_directory(sc_char const * extension_directory_path)
{
  GDir * extension_directory = null_ptr;
  sc_char const * file_name = null_ptr;
  fModuleInitializeFunc initialize_func = null_ptr;
  fModuleInitializeFunc shutdown_func = null_ptr;

  sc_message("Collect extensions from %s", extension_directory_path);

  // check if specified directory exist
  if (sc_fs_is_directory(extension_directory_path) == SC_FALSE)
  {
    sc_warning("Specified extensions directory %s doesn't exist", extension_directory_path);
    return SC_RESULT_ERROR_INVALID_PARAMS;
  }

  // get list of files in extension directory
  extension_directory = g_dir_open(extension_directory_path, 0, null_ptr);
  if (extension_directory == null_ptr)
  {
    sc_warning("Specified extensions directory %s can't be opened", extension_directory_path);
    return SC_RESULT_ERROR;
  }

  // list all files in directory and try to load them
  file_name = g_dir_read_name(extension_directory);
  while (file_name != null_ptr)
  {
    sc_module_info * mi = sc_mem_new(sc_module_info, 1);
    mi->path = _sc_module_build_path(extension_directory_path, file_name);

    // open module
    mi->ptr = g_module_open(mi->path, G_MODULE_BIND_LOCAL);
    if (mi->ptr == null_ptr)
    {
      sc_warning("Can't open module %s: %s", mi->path, g_module_error());
      goto clean;
    }

    if (g_module_symbol(
            mi->ptr, "sc_module_initialize_with_init_memory_generated_structure", (sc_pointer *)&initialize_func)
        == SC_FALSE)
    {
      if (g_module_symbol(mi->ptr, "sc_module_initialize", (sc_pointer *)&initialize_func) == SC_FALSE)
      {
        sc_warning("Can't find any 'sc_module_initialize' symbol in module: %s", mi->path);
        goto clean;
      }
    }
    mi->init_func = initialize_func;

    if (g_module_symbol(mi->ptr, "sc_module_shutdown", (sc_pointer *)&shutdown_func) == SC_FALSE)
    {
      sc_warning("Can't find 'sc_module_shutdown' symbol in module: %s", mi->path);
      goto clean;
    }
    mi->shut_func = shutdown_func;

    fModulePriorityFunc module_priority_func;
    if (g_module_symbol(mi->ptr, "sc_module_load_priority", (sc_pointer *)&module_priority_func) == SC_FALSE)
      mi->priority = SC_MAXUINT32;
    else
      mi->priority = module_priority_func();

    modules_priority_list = g_list_insert_sorted(modules_priority_list, (sc_pointer)mi, sc_priority_less);
    goto next;

  clean:
  {
    sc_module_info_free(mi);
  }

  next:
  {
    file_name = g_dir_read_name(extension_directory);
  }
  }

  g_dir_close(extension_directory);

  return SC_RESULT_OK;
}

sc_result sc_ext_initialize(
    sc_char const ** extension_directories,
    sc_uint32 const extension_directories_count,
    sc_char const ** enabled_list,
    sc_addr const init_memory_generated_structure_addr)
{
  sc_unused(&enabled_list);

  // doesn't need to initialize extensions
  if (extension_directories == null_ptr)
    return SC_RESULT_OK;

  // check if modules supported on this platform
  if (g_module_supported() == SC_FALSE)
  {
    sc_warning("Modules not supported on this platform");
    return SC_RESULT_ERROR;
  }

  for (sc_uint32 i = 0; i < extension_directories_count; ++i)
    _sc_ext_collect_extensions_from_directory(extension_directories[i]);

  // initialize modules
  GList * item = modules_priority_list;
  sc_result init_result;
  while (item != null_ptr)
  {
    sc_module_info * module = (sc_module_info *)item->data;
    sc_message("Initialize module: %s", module->path);
    init_result = module->init_func(init_memory_generated_structure_addr);

    if (init_result != SC_RESULT_OK)
    {
      sc_warning("Something happened during module initialization: %s", module->path);
      module->shut_func();
      g_module_close(module->ptr);
      module->ptr = null_ptr;
    }

    item = item->next;
  }

  return SC_RESULT_OK;
}

void sc_ext_shutdown()
{
  modules_priority_list = g_list_sort(modules_priority_list, sc_priority_great);
  GList * item = modules_priority_list;
  while (item != null_ptr)
  {
    sc_module_info * module = (sc_module_info *)item->data;
    sc_message("Shutdown module: %s", module->path);
    if (module->ptr != null_ptr)
    {
      if (module->shut_func() != SC_RESULT_OK)
        sc_warning("Something happened during module shutdown: %s", module->path);
    }

    item = item->next;
  }

  g_list_free_full(modules_priority_list, sc_module_info_free);
  modules_priority_list = null_ptr;
}
