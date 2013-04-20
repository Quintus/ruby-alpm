#include "database.h"

/***************************************
 * Variables
 ***************************************/

VALUE rb_cAlpm_Database;

/***************************************
 * Methods
 ***************************************/

static VALUE initialize(VALUE self)
{
  rb_raise(rb_eNotImpError, "Can't create new databases with this library.");
  return self;
}

/**
 * call-seq:
 *   name() → a_string
 *
 * Returns the name of the package database.
 */
static VALUE name(VALUE self)
{
  alpm_db_t* p_db = NULL;
  Data_Get_Struct(self, alpm_db_t, p_db);

  return rb_str_new2(alpm_db_get_name(p_db));
}

/**
 * call-seq:
 *   get( name ) → a_package
 *
 * Find a Package by name in the database.
 *
 * === Parameters
 * [name]
 *   The name of the package. Must match exactly.
 *
 * === Return value
 * A Package instance representing the package.
 */
static VALUE get(VALUE self, VALUE name)
{
  alpm_db_t* p_db = NULL;
  alpm_pkg_t* p_pkg = NULL;
  Data_Get_Struct(self, alpm_db_t, p_db);

  p_pkg = alpm_db_get_pkg(p_db, StringValuePtr(name));

  if (p_pkg)
    return Data_Wrap_Struct(rb_cAlpm_Package, NULL, NULL, p_pkg);
  else
    return Qnil;
}

/**
 * call-seq:
 *   inspect() → a_string
 *
 * Human-readable description.
 */
static VALUE inspect(VALUE self)
{
  alpm_db_t* p_db = NULL;
  int len;
  char buf[256];
  Data_Get_Struct(self, alpm_db_t, p_db);

  len = sprintf(buf, "#<%s %s>", rb_obj_classname(self), alpm_db_get_name(p_db));
  return rb_str_new(buf, len);
}

/**
 * call-seq:
 *   valid?() → true or false
 *
 * Checks if the database is in a valid state (mostly useful
 * for verifying signature status). If this returns false,
 * check out Alpm::errno for the reason.
 */
static VALUE valid(VALUE self)
{
  alpm_db_t* p_db = NULL;
  Data_Get_Struct(self, alpm_db_t, p_db);

  return alpm_db_get_valid(p_db) == 0 ? Qtrue : Qfalse;
}

/***************************************
 * Binding
 ***************************************/


/**
 * A Database is the list of packages in a repository, where the notion
 * of the "repository" is an abstract one. It may be an actual remote
 * repository, or just represent the current system state. It does not
 * make any difference in treating instances of this class.
 *
 * The database listings are always kept local, so interacting with this
 * class is possible while being offline.
 */
void Init_database()
{
  rb_cAlpm_Database = rb_define_class_under(rb_cAlpm, "Database", rb_cObject);

  rb_define_method(rb_cAlpm_Database, "initialize", RUBY_METHOD_FUNC(initialize), 0);
  rb_define_method(rb_cAlpm_Database, "get", RUBY_METHOD_FUNC(get), 1);
  rb_define_method(rb_cAlpm_Database, "name", RUBY_METHOD_FUNC(name), 0);
  rb_define_method(rb_cAlpm_Database, "inspect", RUBY_METHOD_FUNC(inspect), 0);
  rb_define_method(rb_cAlpm_Database, "valid?", RUBY_METHOD_FUNC(valid), 0);
}
