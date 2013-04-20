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

/**
 * call-seq:
 *   add_server( url )
 *
 * Add a server to sync from to this database.
 *
 * === Parameters
 * [url]
 *   The remote URL for this server.
 */
static VALUE add_server(VALUE self, VALUE url)
{
  alpm_db_t* p_db = NULL;
  Data_Get_Struct(self, alpm_db_t, p_db);

  alpm_db_add_server(p_db, StringValuePtr(url));
  return Qnil;
}

/**
 * call-seq:
 *   remove_server( url )
 *
 * Remove a sync server from this database.
 *
 * === Parameters
 * [url]
 *   The remote URL for the server.
 */
static VALUE remove_server(VALUE self, VALUE url)
{
  alpm_db_t* p_db = NULL;
  Data_Get_Struct(self, alpm_db_t, p_db);

  alpm_db_remove_server(p_db, StringValuePtr(url));
  return Qnil;
}

/**
 * call-seq:
 *   servers() → an_array
 *
 * Returns an array of all server URLs for this database.
 */
static VALUE get_servers(VALUE self)
{
  alpm_db_t* p_db = NULL;
  alpm_list_t* p_servers = NULL;
  alpm_list_t* p_item = NULL;
  VALUE result = rb_ary_new();
  Data_Get_Struct(self, alpm_db_t, p_db);

  p_servers = alpm_db_get_servers(p_db);
  if (!p_servers)
    return result;

  for(p_item = p_servers; p_item; p_item = alpm_list_next(p_item)) {
    rb_ary_push(result, rb_str_new2((char*) p_item->data));
  }

  return rb_obj_freeze(result); /* Modifying this in Ruby land would be nonsense */
}

/**
 * call-seq:
 *   servers=( ary )
 *
 * Replace the list of servers for this database with the
 * given one.
 *
 * === Parameters
 * [servers]
 *   An array of URLs.
 */
static VALUE set_servers(VALUE self, VALUE ary)
{
  alpm_db_t* p_db = NULL;
  alpm_list_t servers;
  int i;
  Data_Get_Struct(self, alpm_db_t, p_db);

  if (!RTEST(ary = rb_check_array_type(ary))) { /* Single = intended */
    rb_raise(rb_eTypeError, "Argument is no array (#to_ary)");
      return Qnil;
  }

  memset(&servers, '\0', sizeof(servers));

  for(i=0; i < RARRAY_LEN(ary); i++) {
    VALUE url = rb_ary_entry(ary, i);
    alpm_list_add(&servers, StringValuePtr(url));
  }

  alpm_db_set_servers(p_db, &servers);
  return ary;
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
  rb_define_method(rb_cAlpm_Database, "add_server", RUBY_METHOD_FUNC(add_server), 1);
  rb_define_method(rb_cAlpm_Database, "remove_server", RUBY_METHOD_FUNC(remove_server), 1);
  rb_define_method(rb_cAlpm_Database, "servers", RUBY_METHOD_FUNC(get_servers), 0);
  rb_define_method(rb_cAlpm_Database, "servers=", RUBY_METHOD_FUNC(set_servers), 1);
}
