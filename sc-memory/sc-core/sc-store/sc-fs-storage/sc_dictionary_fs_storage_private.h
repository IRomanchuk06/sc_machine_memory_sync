/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef _sc_dictionary_fs_storage_private_h_
#define _sc_dictionary_fs_storage_private_h_

#include "../sc_types.h"
#include "../sc-base/sc_allocator.h"
#include "../sc-container/sc-string/sc_string.h"

#include "stdio.h"

#define SC_FS_EXT ".scdb";

sc_uint8 _sc_dictionary_children_size()
{
  sc_uint8 const max_sc_char = 255;
  sc_uint8 const min_sc_char = 1;

  return max_sc_char - min_sc_char + 1;
}

void _sc_char_to_sc_int(sc_char ch, sc_uint8 * ch_num, sc_uint8 const * mask)
{
  *ch_num = 128 + (sc_uint8)ch;
}

void _sc_init_db_path(sc_char const * path, sc_char const * postfix, sc_char ** out_path)
{
  sc_uint32 size = sc_str_len(path) + sc_str_len(postfix) + 2;
  *out_path = sc_mem_new(sc_char, size + 1);
  sc_str_printf(*out_path, size, "%s/%s", path, postfix);
}

sc_bool _sc_dictionary_fs_storage_node_destroy(sc_dictionary_node * node, void ** args)
{
  (void)args;

  if (node->data != null_ptr)
  {
    sc_mem_free(((sc_list *)node->data)->begin->data);
    sc_list_destroy(node->data);
  }
  node->data = null_ptr;

  sc_mem_free(node->next);
  node->next = null_ptr;

  sc_mem_free(node->offset);
  node->offset = null_ptr;
  node->offset_size = 0;

  sc_mem_free(node);

  return SC_TRUE;
}

sc_list * _sc_dictionary_fs_storage_get_string_terms(sc_char const * string)
{
  static const sc_char delim[] = " ,.-\0()[]_";

  sc_uint32 const size = sc_str_len(string);
  sc_char copied_string[size + 1];
  sc_mem_cpy(copied_string, string, size);
  copied_string[size] = '\0';

  sc_char * term = strtok(copied_string, delim);
  sc_list * terms;
  sc_list_init(&terms);
  sc_dictionary * unique_terms;
  sc_dictionary_initialize(&unique_terms, _sc_dictionary_children_size(), _sc_char_to_sc_int);
  while (term != null_ptr)
  {
    sc_uint64 const term_length = sc_str_len(term);
    sc_char * term_copy;
    sc_str_cpy(term_copy, term, term_length);

    if (sc_dictionary_is_in(unique_terms, term_copy, term_length))
      sc_mem_free(term_copy);
    else
    {
      sc_list_push_back(terms, term_copy);
      sc_dictionary_append(unique_terms, term_copy, term_length, null_ptr);
    }

    term = strtok(null_ptr, delim);
  }
  sc_dictionary_destroy(unique_terms, sc_dictionary_node_destroy);

  return terms;
}

#endif
