#ifndef SC_TRANSACTION_BUFFER_H
#define SC_TRANSACTION_BUFFER_H

#include <sc-store/sc-transaction/sc_element_version.h>
#include <sc-core/sc-container/sc_list.h>
#include <sc-core/sc_stream.h>
#include <sc-core/sc-base/sc_monitor.h>

typedef struct sc_transaction_buffer
{
  sc_list * new_elements;
  sc_list * modified_elements;
  sc_list * deleted_elements;
  sc_list * content_changes;
  sc_monitor_table * monitor_table;
  sc_uint64 transaction_id;
} sc_transaction_buffer;

sc_bool sc_transaction_buffer_initialize(sc_transaction_buffer * transaction_buffer, sc_uint64 txn_id);

sc_bool sc_transaction_buffer_created_add(sc_transaction_buffer const * buffer, sc_addr const * addr);
sc_bool sc_transaction_buffer_modified_add(
    sc_transaction_buffer const * buffer,
    sc_addr const * addr,
    sc_element const * element,
    SC_ELEMENT_MODIFIED_FLAGS flags);
sc_bool sc_transaction_buffer_removed_add(sc_transaction_buffer const * buffer, sc_addr const * addr);
sc_bool sc_transaction_buffer_content_set(
    sc_transaction_buffer const * buffer,
    sc_addr const * addr,
    sc_stream const * content);

sc_bool sc_transaction_buffer_contains_created(sc_transaction_buffer const * buffer, sc_addr const * addr);

#endif
