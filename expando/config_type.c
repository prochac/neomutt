/**
 * @file
 * Type representing an Expando
 *
 * @authors
 * Copyright (C) 2023-2024 Tóth János <gomba007@gmail.com>
 * Copyright (C) 2023-2024 Richard Russon <rich@flatcap.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page expando_config_type Type: Expando
 *
 * Config type representing an Expando.
 */

#include "config.h"
#include <stddef.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "expando.h"
#include "validation.h"

/**
 * expando_destroy - Destroy an Expando object - Implements ConfigSetType::destroy() - @ingroup cfg_type_destroy
 */
static void expando_destroy(const struct ConfigSet *cs, void *var, const struct ConfigDef *cdef)
{
  struct Expando **a = var;
  if (!*a)
    return;

  expando_free(a);
}

/**
 * expando_string_set - Set an Expando by string - Implements ConfigSetType::string_set() - @ingroup cfg_type_string_set
 */
static int expando_string_set(const struct ConfigSet *cs, void *var, struct ConfigDef *cdef,
                              const char *value, struct Buffer *err)
{
  if (!cs || !cdef)
    return CSR_ERR_CODE; /* LCOV_EXCL_LINE */

  /* Store empty string list as NULL */
  if (value && (value[0] == '\0'))
    value = NULL;

  if (!value && (cdef->type & D_NOT_EMPTY))
  {
    buf_printf(err, _("Option %s may not be empty"), cdef->name);
    return CSR_ERR_INVALID | CSR_INV_VALIDATOR;
  }

  int rc = CSR_SUCCESS;

  if (var)
  {
    const struct ExpandoDefinition *defs = (const struct ExpandoDefinition *)
                                               cdef->data;
    struct Expando *exp = expando_parse(value, defs, err);

    if (expando_equal(exp, *(struct Expando **) var))
    {
      expando_free(&exp);
      return CSR_SUCCESS | CSR_SUC_NO_CHANGE;
    }

    if (startup_only(cdef, err))
    {
      expando_free(&exp);
      return CSR_ERR_INVALID | CSR_INV_VALIDATOR;
    }

    if (cdef->validator)
    {
      rc = cdef->validator(cs, cdef, (intptr_t) exp, err);

      if (CSR_RESULT(rc) != CSR_SUCCESS)
      {
        expando_free(&exp);
        return rc | CSR_INV_VALIDATOR;
      }
    }

    expando_destroy(cs, var, cdef);

    *(struct Expando **) var = exp;

    if (!exp)
      rc |= CSR_SUC_EMPTY;
  }
  else
  {
    if (cdef->type & D_INTERNAL_INITIAL_SET)
      FREE(&cdef->initial);

    cdef->type |= D_INTERNAL_INITIAL_SET;
    cdef->initial = (intptr_t) mutt_str_dup(value);
  }

  return rc;
}

/**
 * expando_string_get - Get an Expando as a string - Implements ConfigSetType::string_get() - @ingroup cfg_type_string_get
 */
static int expando_string_get(const struct ConfigSet *cs, void *var,
                              const struct ConfigDef *cdef, struct Buffer *result)
{
  const char *str = NULL;

  if (var)
  {
    struct Expando *exp = *(struct Expando **) var;
    if (exp)
      str = exp->string;
  }
  else
  {
    str = (char *) cdef->initial;
  }

  if (!str)
    return CSR_SUCCESS | CSR_SUC_EMPTY; /* empty string */

  buf_addstr(result, str);
  return CSR_SUCCESS;
}

/**
 * expando_native_set - Set an Expando object from an Expando config item - Implements ConfigSetType::native_get() - @ingroup cfg_type_native_get
 */
static int expando_native_set(const struct ConfigSet *cs, void *var,
                              const struct ConfigDef *cdef, intptr_t value,
                              struct Buffer *err)
{
  int rc;

  struct Expando *exp_value = (struct Expando *) value;
  if (!exp_value && (cdef->type & D_NOT_EMPTY))
  {
    buf_printf(err, _("Option %s may not be empty"), cdef->name);
    return CSR_ERR_INVALID | CSR_INV_VALIDATOR;
  }

  struct Expando *exp_old = *(struct Expando **) var;
  if (expando_equal(exp_value, exp_old))
    return CSR_SUCCESS | CSR_SUC_NO_CHANGE;

  if (startup_only(cdef, err))
    return CSR_ERR_INVALID | CSR_INV_VALIDATOR;

  if (cdef->validator)
  {
    rc = cdef->validator(cs, cdef, value, err);

    if (CSR_RESULT(rc) != CSR_SUCCESS)
      return rc | CSR_INV_VALIDATOR;
  }

  expando_free(var);

  struct Expando *exp_copy = NULL;

  if (exp_value)
  {
    const struct ExpandoDefinition *defs = (const struct ExpandoDefinition *)
                                               cdef->data;
    exp_copy = expando_parse(exp_value->string, defs, err);
  }

  rc = CSR_SUCCESS;
  if (!exp_copy)
    rc |= CSR_SUC_EMPTY;

  *(struct Expando **) var = exp_copy;
  return rc;
}

/**
 * expando_native_get - Get an Expando object from an Expando config item - Implements ConfigSetType::native_get() - @ingroup cfg_type_native_get
 */
static intptr_t expando_native_get(const struct ConfigSet *cs, void *var,
                                   const struct ConfigDef *cdef, struct Buffer *err)
{
  if (!cs || !var || !cdef)
    return INT_MIN; /* LCOV_EXCL_LINE */

  struct Expando *exp = *(struct Expando **) var;

  return (intptr_t) exp;
}

/**
 * expando_reset - Reset an Expando to its initial value - Implements ConfigSetType::reset() - @ingroup cfg_type_reset
 */
static int expando_reset(const struct ConfigSet *cs, void *var,
                         const struct ConfigDef *cdef, struct Buffer *err)
{
  if (!cs || !var || !cdef)
    return CSR_ERR_CODE; /* LCOV_EXCL_LINE */

  struct Expando *exp = NULL;
  const char *initial = (const char *) cdef->initial;

  if (initial)
  {
    const struct ExpandoDefinition *defs = (const struct ExpandoDefinition *)
                                               cdef->data;
    exp = expando_parse(initial, defs, err);
  }

  if (expando_equal(exp, *(struct Expando **) var))
  {
    expando_free(&exp);
    return CSR_SUCCESS | CSR_SUC_NO_CHANGE;
  }

  if (startup_only(cdef, err))
  {
    expando_free(&exp);
    return CSR_ERR_INVALID | CSR_INV_VALIDATOR;
  }

  int rc = CSR_SUCCESS;

  if (cdef->validator)
  {
    rc = cdef->validator(cs, cdef, (intptr_t) exp, err);

    if (CSR_RESULT(rc) != CSR_SUCCESS)
    {
      expando_destroy(cs, &exp, cdef);
      return rc | CSR_INV_VALIDATOR;
    }
  }

  if (!exp)
    rc |= CSR_SUC_EMPTY;

  expando_destroy(cs, var, cdef);

  *(struct Expando **) var = exp;
  return rc;
}

/**
 * CstExpando - Config type representing an Expando
 */
const struct ConfigSetType CstExpando = {
  DT_EXPANDO,
  "expando",
  expando_string_set,
  expando_string_get,
  expando_native_set,
  expando_native_get,
  NULL, // string_plus_equals
  NULL, // string_minus_equals
  expando_reset,
  expando_destroy,
};

/**
 * cs_subset_expando - Get an Expando config item by name
 * @param sub   Config Subset
 * @param name  Name of config item
 * @retval ptr  Expando
 * @retval NULL Empty Expando
 */
const struct Expando *cs_subset_expando(const struct ConfigSubset *sub, const char *name)
{
  assert(sub && name);

  struct HashElem *he = cs_subset_create_inheritance(sub, name);
  assert(he);

#ifndef NDEBUG
  struct HashElem *he_base = cs_get_base(he);
  assert(DTYPE(he_base->type) == DT_EXPANDO);
#endif

  intptr_t value = cs_subset_he_native_get(sub, he, NULL);
  assert(value != INT_MIN);

  return (const struct Expando *) value;
}
