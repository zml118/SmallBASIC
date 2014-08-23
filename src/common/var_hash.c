// This file is part of SmallBASIC
//
// Support for hash variables
// eg: dim foo: key="blah": foo(key) = "something": ? foo(key)
//
// This program is distributed under the terms of the GPL v2.0 or later
// Download the GNU Public License (GPL) from www.gnu.org
//
// Copyright(C) 2010-2014 Chris Warren-Smith. [http://tinyurl.com/ja2ss]

#include "common/sys.h"
#include "common/var.h"
#include "common/smbas.h"
#include "common/var_hash.h"
#include "common/search.h"

/**
 * Globals for callback access
 */
typedef struct CallbackData {
  int method;
  int handle;
  int index;
  int count;
  int firstElement;
  void* hash;
  var_p_t var;
} CallbackData;

CallbackData cb;

/**
 * Our internal tree element node
 */
typedef struct Element {
  var_p_t key;
  var_p_t value;
} Element;

/**
 * Return value from search routines
 */
typedef struct Node {
  Element *element;
} Node;

/**
 * Returns a new Element
 */
Element *create_element(var_p_t key) {
  Element *element = (Element *) tmp_alloc(sizeof (Element));
  element->key = v_new();
  v_set(element->key, key);
  element->value = NULL;
  return element;
}

/**
 * Returns a new Element using the integer based key
 */
Element *create_int_element(int key) {
  Element *element = (Element *) tmp_alloc(sizeof (Element));
  element->key = v_new();
  v_setint(element->key, key);
  element->value = NULL;
  return element;
}

/**
 * cleanup the given element
 */
void delete_element(Element *element) {
  // cleanup v_new
  v_free(element->key);
  tmp_free(element->key);

  // cleanup v_new
  if (element->value) {
    v_free(element->value);
    tmp_free(element->value);
  }

  // cleanup create_element()
  tmp_free(element);
}

/**
 * Callback to compare Element's
 */
int cmp_fn(const void *a, const void *b) {
  Element *el_a = (Element *)a;
  Element *el_b = (Element *)b;

  int result;
  if (el_a->key->type == V_STR && el_b->key->type == V_STR) {
    result = strcasecmp(el_a->key->v.p.ptr, el_b->key->v.p.ptr);
  } else {
    result = v_compare(el_a->key, el_b->key);
  }
  return result;
}

/**
 * Compare one HASH to another. see v_compare comments for return spec.
 */
int hash_compare(const var_p_t var_a, const var_p_t var_b) {
  return 0;
}

/**
 * Return true if the structure is empty
 */
int hash_is_empty(const var_p_t var_p) {
  return (var_p->v.hash == NULL);
}

/**
 * Return the contents of the structure as an integer
 */
int hash_to_int(const var_p_t var_p) {
  return hash_length(var_p);
}

/**
 * Helper for hash_length
 */
void hash_length_cb(const void *nodep, VISIT value, int level) {
  if (value == leaf || value == endorder) {
    cb.count++;
  }
}

/**
 * Return the number of elements
 */
int hash_length(const var_p_t var_p) {
  cb.count = 0;
  if (var_p->type == V_HASH) {
    twalk(var_p->v.hash, hash_length_cb);
  }
  return cb.count;
}

/**
 * Helper for hash_length
 */
void hash_elem_cb(const void *nodep, VISIT value, int level) {
  if (value == leaf || value == endorder) {
    if (cb.count++ == cb.index) {
      Element *element = ((Node *) nodep)->element;
      cb.var = element->key;
    }
  }
}

/**
 * return the element at the nth position
 */
var_p_t hash_elem(const var_p_t var_p, int index) {
  cb.count = 0;
  cb.index = index;
  cb.var = NULL;
  if (var_p->type == V_HASH) {
    twalk(var_p->v.hash, hash_elem_cb);
  }
  return cb.var;
}

/**
 * Empty struct values
 */
void hash_clear(const var_p_t var) {
  hash_free_var(var);
}

/**
 * Helper for hash_free_var
 */
void hash_free_cb(void *nodep) {
  Element *element = (Element *) nodep;
  delete_element(element);
}

/**
 * Delete the given structure
 */
void hash_free_var(var_p_t var_p) {
  if (var_p->type == V_HASH) {
    tdestroy(var_p->v.hash, hash_free_cb);
    var_p->v.hash = NULL;
  }
}

/**
 * Inserts the variable into the b-tree, set result to the key value
 */
void hash_insert_key(var_p_t base, var_p_t var_key, var_p_t *result) {
  Element *key = create_element(var_key);
  Node *node = tfind(key, &(base->v.hash), cmp_fn);
  if (node != NULL) {
    // item already exists
    *result = node->element->value;
    delete_element(key);
  }
  else {
    key->value = *result = v_new();
    tsearch(key, &(base->v.hash), cmp_fn);
  }
}

/**
 * Returns the final element eg z in foo.x.y.z
 *
 * Scan byte code for node kwTYPE_UDS_EL and attach as field elements
 * if they don't already exist.
 */
var_p_t hash_resolve_fields(const var_p_t base) {
  var_p_t field = NULL;
  if (code_peek() == kwTYPE_UDS_EL) {
    code_skipnext();
    if (code_peek() != kwTYPE_STR) {
      err_stackmess();
      return NULL;
    } else {
      code_skipnext();
    }

    if (base->type != V_HASH) {
      if (v_is_nonzero(base)) {
        err_typemismatch();
        return NULL;
      } else {
        v_free(base);
        base->type = V_HASH;
        base->v.hash = NULL;
      }
    }

    // evaluate the variable 'key' name
    var_t key;
    v_eval_str(&key);
    hash_insert_key(base, &key, &field);
    v_free(&key);

    // evaluate the next sub-element
    field = hash_resolve_fields(field);
  } else {
    field = base;
  }
  return field;
}

/**
 * Return the variable in base keyed by key, if not found then creates
 * an empty variable that will be returned in a further call
 */
void hash_get_value(var_p_t base, var_p_t var_key, var_p_t *result) {
  if (base->type == V_ARRAY && base->v.a.size) {
    // convert the non-empty array to a hash
    int i;
    var_t *clone = v_clone(base);

    v_free(base);
    base->type = V_HASH;
    base->v.hash = NULL;

    for (i = 0; i < clone->v.a.size; i++) {
      const var_t *element = (var_t *)(clone->v.a.ptr + (sizeof(var_t) * i));
      Element *key = create_int_element(i);
      key->value = v_new();
      v_set(key->value, element);
      tsearch(key, &(base->v.hash), cmp_fn);
    }

    // free the clone
    v_free(clone);
    tmp_free(clone);
  } else if (base->type != V_HASH) {
    if (v_is_nonzero(base)) {
      err_typemismatch();
      return;
    } else {
      // initialise as hash
      v_free(base);
      base->type = V_HASH;
      base->v.hash = NULL;
    }
  }

  hash_insert_key(base, var_key, result);
}

/**
 * Callback for hash_set()
 */
void hash_set_cb(const void *nodep, VISIT value, int level) {
  if (value == leaf || value == endorder) {
    // copy the element and insert it into the destination
    Element *element = ((Node *) nodep)->element;
    Element *key = create_element(element->key);
    key->value = v_new();
    v_set(key->value, element->value);
    tsearch(key, &(cb.hash), cmp_fn);
  }
}

/**
 * copy values from one structure to another
 */
void hash_set(var_p_t dest, const var_p_t src) {
  if (dest != src) {
    v_free(dest);
    cb.hash = NULL;

    if (src->type == V_HASH) {
      twalk(src->v.hash, hash_set_cb);
    }

    dest->type = V_HASH;
    dest->v.hash = cb.hash;
  }
}

/**
 * Return the contents of the structure as a string
 */
void hash_to_str(const var_p_t var_p, char *out, int max_len) {
  sprintf(out, "UDS:%d", hash_to_int(var_p));
}

/**
 * Callback for hash_write()
 */
void hash_write_cb(const void *nodep, VISIT value, int level) {
  if (value == leaf || value == endorder) {
    Element *element = ((Node *) nodep)->element;
    if (!cb.firstElement) {
      pv_write(",", cb.method, cb.handle);
    }
    cb.firstElement = 0;
    pv_writevar(element->key, cb.method, cb.handle);
    pv_write("=", cb.method, cb.handle);
    pv_writevar(element->value, cb.method, cb.handle);
  }
}

/**
 * Print the contents of the structure
 */
void hash_write(const var_p_t var_p, int method, int handle) {
  if (var_p->type == V_HASH) {
    cb.method = method;
    cb.handle = handle;
    cb.firstElement = 1;
    pv_write("[", method, handle);
    twalk(var_p->v.hash, hash_write_cb);
    pv_write("]", method, handle);
  }
}

