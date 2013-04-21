#include "database.h"

/***************************************
 * Variables
 ***************************************/

VALUE rb_cAlpm_Database;

/** Retrieves the associated Ruby Alpm instance from the given Package
 * instance, reads the C alpm_handle_t pointer from it and returns that
 * one. */
static alpm_handle_t* get_alpm_from_db(VALUE db)
{
  alpm_handle_t* p_handle = NULL;
  VALUE rb_alpm = rb_iv_get(db, "@alpm");

  Data_Get_Struct(rb_alpm, alpm_handle_t, p_handle);
  return p_handle;
}


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
  alpm_list_t* servers = NULL;
  int i;
  Data_Get_Struct(self, alpm_db_t, p_db);

  if (!RTEST(ary = rb_check_array_type(ary))) { /* Single = intended */
    rb_raise(rb_eTypeError, "Argument is no array (#to_ary)");
      return Qnil;
  }

  for(i=0; i < RARRAY_LEN(ary); i++) {
    VALUE url = rb_ary_entry(ary, i);
    servers = alpm_list_add(servers, StringValuePtr(url));
  }

  alpm_db_set_servers(p_db, servers);
  alpm_list_free(servers);

  return ary;
}

/**
 * call-seq:
 *   search(*queries, ...) → an_array
 *
 * Search the database with POSIX regular expressions for packages.
 * === Parameters
 * [*queries (splat)]
 *   A list of strings interpreted as POSIX regular expressions.
 *   For a package to be found, it must match _all_ queries terms,
 *   not just a single one. Each query is matched against both
 *   the package name and the package description, where only
 *   one needs to match for the package to be considered.
 *
 *   Note that the match is not performed by Ruby or even Oniguruma/Onigmo,
 *   but directly in libalpm via the +regexp+ library in C.
 *
 * === Return value
 * An array of Package instances whose names matched _all_ regular expressions.
 */
static VALUE search(int argc, VALUE argv[], VALUE self)
{
  alpm_db_t* p_db = NULL;
  alpm_list_t* targets = NULL;
  alpm_list_t* packages = NULL;
  alpm_list_t* item = NULL;
  VALUE result = rb_ary_new();
  int i;

  Data_Get_Struct(self, alpm_db_t, p_db);

  /* Convert our Ruby array to an alpm_list with C strings */
  for(i=0; i < argc; i++) {
    VALUE term = rb_check_string_type(argv[i]);
    if (!RTEST(term)) {
      rb_raise(rb_eTypeError, "Argument is not a string (#to_str)");
      return Qnil;
    }

    targets = alpm_list_add(targets, StringValuePtr(term));
  }

  /* Perform the query */
  packages = alpm_db_search(p_db, targets);
  if (!packages)
    return result;

  for(item=packages; item; item = alpm_list_next(item))
    rb_ary_push(result, Data_Wrap_Struct(rb_cAlpm_Package, NULL, NULL, item->data));

  alpm_list_free(targets);
  return result;
}

/**
 * call-seq:
 *   unregister()
 *
 * Unregister this database from libalpm. This method invalidates
 * +self+, so please don’t use it anymore after you called this
 * method.
 */
static VALUE unregister(VALUE self)
{
  alpm_db_t* p_db = NULL;
  Data_Get_Struct(self, alpm_db_t, p_db);

  if (alpm_db_unregister(p_db) < 0) {
    raise_last_alpm_error(get_alpm_from_db(self));
    return Qnil;
  }

  DATA_PTR(self) = NULL; /* This object is now invalid */
  return Qnil;
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
  rb_define_method(rb_cAlpm_Database, "search", RUBY_METHOD_FUNC(search), -1);
  rb_define_method(rb_cAlpm_Database, "unregister", RUBY_METHOD_FUNC(unregister), 0);
}
