#include "database.h"

/***************************************
 * Variables
 ***************************************/

VALUE Database;

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
    return Data_Wrap_Struct(Package, NULL, NULL, p_pkg);
  else
    return Qnil;
}

/***************************************
 * Binding
 ***************************************/


/**
 * Document-class: Alpm::Database
 *
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
  Database = rb_define_class_under(Alpm, "Database", rb_cObject);

  rb_define_singleton_method(Database, "get", RUBY_METHOD_FUNC(get), 1);

  rb_define_method(Database, "initialize", RUBY_METHOD_FUNC(initialize), 0);
  rb_define_method(Database, "name", RUBY_METHOD_FUNC(name), 0);
}
