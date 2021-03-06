#include "main.h"
#include "package.h"
#include "transaction.h"
#include "database.h"

/***************************************
 * Variables, etc
 ***************************************/

VALUE rb_cAlpm;
VALUE rb_eAlpm_Error;

/** Raises the last libalpm error as a Ruby exception of
 * class Alpm::AlpmError. */
VALUE raise_last_alpm_error(alpm_handle_t* p_handle)
{
  const char* msg = alpm_strerror(alpm_errno(p_handle));
  rb_raise(rb_eAlpm_Error, msg);
  return Qnil;
}

/** Takes a Ruby array of Ruby Symbols and computes the C
 * alpm_siglevel_t from it. Raises an exception if `ary'
 * doesn’t respond to #to_ary. */
alpm_siglevel_t siglevel_from_ruby(VALUE ary)
{
  alpm_siglevel_t level = 0;

  if (!(RTEST(ary = rb_check_array_type(ary)))) { /*  Single = intended */
    VALUE str = rb_inspect(level);
    rb_raise(rb_eTypeError, "Not an array (#to_ary): %s", StringValuePtr(str));
    return Qnil;
  }

  if (rb_ary_includes(ary, STR2SYM("package")))
    level |= ALPM_SIG_PACKAGE;
  if (rb_ary_includes(ary, STR2SYM("package_optional")))
    level |= ALPM_SIG_PACKAGE_OPTIONAL;
  if (rb_ary_includes(ary, STR2SYM("package_marginal_ok")))
    level |= ALPM_SIG_PACKAGE_MARGINAL_OK;
  if (rb_ary_includes(ary, STR2SYM("package_unknown_ok")))
    level |= ALPM_SIG_PACKAGE_UNKNOWN_OK;
  if (rb_ary_includes(ary, STR2SYM("database")))
    level |= ALPM_SIG_DATABASE;
  if (rb_ary_includes(ary, STR2SYM("database_optional")))
    level |= ALPM_SIG_DATABASE_OPTIONAL;
  if (rb_ary_includes(ary, STR2SYM("database_marginal_ok")))
    level |= ALPM_SIG_DATABASE_MARGINAL_OK;
  if (rb_ary_includes(ary, STR2SYM("database_unknown_ok")))
    level |= ALPM_SIG_DATABASE_UNKNOWN_OK;
  if (rb_ary_includes(ary, STR2SYM("package_set")))
    level |= ALPM_SIG_PACKAGE_SET;
  if (rb_ary_includes(ary, STR2SYM("package_trust_set")))
    level |= ALPM_SIG_PACKAGE_TRUST_SET;
  if (rb_ary_includes(ary, STR2SYM("use_default")))
    level |= ALPM_SIG_USE_DEFAULT;

  return level;
}

/** Frees an alpm package loaded via alpm_pkg_load().
 * This is the only case where we have to keep track
 * of package memory. */
static void free_loaded_pkg(void* ptr)
{
  alpm_pkg_t* p_pkg = (alpm_pkg_t*) ptr;
  alpm_pkg_free(p_pkg);
}

void log_callback(alpm_loglevel_t level, const char* msg, ...)
{
  VALUE levelsym;

  switch(level){
  case ALPM_LOG_ERROR:
    levelsym = ID2SYM(rb_intern("error"));
    break;
  case ALPM_LOG_WARNING:
    levelsym = ID2SYM(rb_intern("warning"));
    break;
  case ALPM_LOG_DEBUG:
    levelsym = ID2SYM(rb_intern("debug"));
    break;
  case ALPM_LOG_FUNCTION:
    levelsym = ID2SYM(rb_intern("function"));
    break;
  default:
    rb_warn("[ruby-alpm] Ignoring unknown logging level '%d' (message: %s)", level, msg);
    return;
  }

  // TODO
}

/***************************************
 * Methods
 ***************************************/

static void deallocate(void* ptr)
{
  if (ptr)
    alpm_release(ptr);
}

static VALUE allocate(VALUE klass)
{
  alpm_handle_t* p_alpm = NULL;
  VALUE obj = Data_Wrap_Struct(klass, 0, deallocate, p_alpm);
  return obj;
}

/**
 * call-seq:
 *   new( rootpath , dbpath ) → an_alpm
 *
 * Creates a new Alpm instance configured for the given directories.
 *
 * === Parameters
 * [rootpath]
 *   File system root directory to install packages under.
 * [dbpath]
 *   Directory used for permanent storage of things like the
 *   list of currently installed packages.
 *
 * === Return value
 * The newly created instance.
 */
static VALUE initialize(VALUE self, VALUE root, VALUE dbpath)
{
  alpm_handle_t* p_alpm = NULL;
  alpm_errno_t err;

  p_alpm = alpm_initialize(StringValuePtr(root), StringValuePtr(dbpath), &err);
  if (!p_alpm)
    rb_raise(rb_eRuntimeError, "Initializing alpm library failed: %s", alpm_strerror(err));

  DATA_PTR(self) = p_alpm;
  return self;
}

/**
 * call-seq:
 *   inspect()  → a_string
 *
 * Human-readable description.
 */
static VALUE inspect(VALUE self)
{
  alpm_handle_t* p_alpm = NULL;
  char buf[256];
  int len;

  Data_Get_Struct(self, alpm_handle_t, p_alpm);

  len = sprintf(buf, "#<%s target=%s db=%s>",
                rb_obj_classname(self),
                alpm_option_get_root(p_alpm),
                alpm_option_get_dbpath(p_alpm));

  return rb_str_new(buf, len);
}

/**
 * call-seq:
 *   root() → a_string
 *
 * Returns the target path to install packages under.
 */
static VALUE root(VALUE self)
{
  alpm_handle_t* p_alpm = NULL;
  Data_Get_Struct(self, alpm_handle_t, p_alpm);

  return rb_str_new2(alpm_option_get_root(p_alpm));
}

/**
 * call-seq:
 *   dbpath() → a_string
 *
 * Returns the path under which permanent information like
 * the list of installed packages is stored.
 */
static VALUE dbpath(VALUE self)
{
  alpm_handle_t* p_alpm = NULL;
  Data_Get_Struct(self, alpm_handle_t, p_alpm);

  return rb_str_new2(alpm_option_get_dbpath(p_alpm));
}

/**
 * call-seq:
 *   log(){|level, message|...}
 *
 * Defines a callback to use when something needs to be logged.
 *
 * == Parameters
 * [level]
 *   Log level. One of :function, :debug, :warning, :error.
 * [message]
 *   The message to log.
 */
static VALUE set_logcb(VALUE self)
{
  alpm_handle_t* p_alpm = NULL;
  Data_Get_Struct(self, alpm_handle_t, p_alpm);

  rb_iv_set(self, "logcb", rb_block_proc());
  alpm_option_set_logcb(p_alpm, log_callback);

  return Qnil;
}

/**
 * call-seq:
 *   gpgdir() → a_string
 *
 * The directory where the package keyring is stored.
 */
static VALUE get_gpgdir(VALUE self)
{
  alpm_handle_t* p_alpm = NULL;
  Data_Get_Struct(self, alpm_handle_t, p_alpm);

  return rb_str_new2(alpm_option_get_gpgdir(p_alpm));
}

/**
 * call-seq:
 *   gpgdir=( path )
 *
 * Set the directory to store the package keyring in.
 */
static VALUE set_gpgdir(VALUE self, VALUE gpgdir)
{
  alpm_handle_t* p_alpm = NULL;
  Data_Get_Struct(self, alpm_handle_t, p_alpm);

  alpm_option_set_gpgdir(p_alpm, StringValuePtr(gpgdir));
  return gpgdir;
}

/**
 * call-seq:
 *   arch() → a_symbol
 *
 * Returns the architecture to download packages for.
 */
static VALUE get_arch(VALUE self)
{
  alpm_handle_t* p_alpm = NULL;
  Data_Get_Struct(self, alpm_handle_t, p_alpm);

  return ID2SYM(rb_intern(alpm_option_get_arch(p_alpm)));
}

/**
 * call-seq:
 *   arch=( sym )
 *
 * Defines the processor architecture to download packages for.
 *
 * == Parameters
 * [sym]
 *   A symbol like <tt>:i386</tt>, <tt>:x86_64</tt>, <tt>:armv7l</tt>, ...
 */
static VALUE set_arch(VALUE self, VALUE arch)
{
  alpm_handle_t* p_alpm = NULL;
  const char* archstr = rb_id2name(SYM2ID(arch));

  Data_Get_Struct(self, alpm_handle_t, p_alpm);
  alpm_option_set_arch(p_alpm, archstr);

  return arch;
}

/**
 * call-seq:
 *   transaction( [ flags ] ){|transaction|...} → an_object
 *
 * Puts libalpm into transaction mode, i.e. allows you to add
 * and remove packaages by means of a transaction. The block
 * gets called with an instance of the (otherwise uninstanciatable,
 * this is a libalpm restriction) Transaction class, which you can
 * freely modify for your operations. When you added all packages
 * you want to add/remove to/from the system, call Transaction#prepare
 * in order to have libalpm resolve dependencies and other stuff.
 * You can then call Transaction#commit to execute your transaction.
 *
 * === Parameters
 * [flags ({})]
 *   A hash with the following keys:
 *   [:nodeps]
 *     Ignore dependency checks.
 *   [:force]
 *     Ignore file conflicts and overwrite files.
 *   [:nosave]
 *     Delete files even if they are tagged as backup.
 *   [:nodepversion]
 *     Ignore version numbers when checking dependencies.
 *   [:cascade]
 *     Remove also any packages depending on a package being removed.
 *   [:recurse]
 *     Remove packages and their unneeded deps (not explicitely installed).
 *   [:dbonly]
 *     Modify database but do not commit changes to the filesystem.
 *   [:alldeps]
 *     Use ALPM_REASON_DEPEND when installing packages.
 *   [:downloadonly]
 *     Only download packages and do not actually install.
 *   [:noscriptlet]
 *     Do not execute install scriptlets after installing.
 *   [:noconflicts]
 *     Ignore dependency conflicts.
 *   [:needed]
 *     Do not install a package if it is already installed and up to date.
 *   [:allexplicit]
 *     Use ALPM_PKG_REASON_EXPLICIT when installing packages.
 *   [:unneeded]
 *     Do not remove a package if it is needed by another one.
 *   [:recurseall]
 *     Remove also explicitely installed unneeded deps (use with :recurse).
 *   [:nolock]
 *     Do not lock the database during the operation.
 *
 * === Return value
 * The result of the block’s last expression.
 *
 * === Remarks
 * Do not store the Transaction instance somewhere; this will give
 * you grief, because it is a transient object always referring to
 * the currently active transaction or bomb if there is none.
 */
static VALUE transaction(int argc, VALUE argv[], VALUE self)
{
  VALUE transaction;
  VALUE result;
  alpm_handle_t* p_alpm = NULL;
  alpm_transflag_t flags = 0;

  Data_Get_Struct(self, alpm_handle_t, p_alpm);

  if (argc == 1) {
    if (TYPE(argv[0]) != T_HASH)
      rb_raise(rb_eTypeError, "Argument is not a hash.");

    if (rb_hash_aref(flags, STR2SYM("nodeps")))
      flags |= ALPM_TRANS_FLAG_NODEPS;
    if (rb_hash_aref(flags, STR2SYM("force")))
      flags |= ALPM_TRANS_FLAG_FORCE;
    if (rb_hash_aref(flags, STR2SYM("nosave")))
      flags |= ALPM_TRANS_FLAG_NOSAVE;
    if (rb_hash_aref(flags, STR2SYM("nodepversion")))
      flags |= ALPM_TRANS_FLAG_NODEPVERSION;
    if (rb_hash_aref(flags, STR2SYM("cascade")))
      flags |= ALPM_TRANS_FLAG_CASCADE;
    if (rb_hash_aref(flags, STR2SYM("recurse")))
      flags |= ALPM_TRANS_FLAG_RECURSE;
    if (rb_hash_aref(flags, STR2SYM("dbonly")))
      flags |= ALPM_TRANS_FLAG_DBONLY;
    if (rb_hash_aref(flags, STR2SYM("alldeps")))
      flags |= ALPM_TRANS_FLAG_ALLDEPS;
    if (rb_hash_aref(flags, STR2SYM("downloadonly")))
      flags |= ALPM_TRANS_FLAG_DOWNLOADONLY;
    if (rb_hash_aref(flags, STR2SYM("noscriptlet")))
      flags |= ALPM_TRANS_FLAG_NOSCRIPTLET;
    if (rb_hash_aref(flags, STR2SYM("noconflicts")))
      flags |= ALPM_TRANS_FLAG_NOCONFLICTS;
    if (rb_hash_aref(flags, STR2SYM("needed")))
      flags |= ALPM_TRANS_FLAG_NEEDED;
    if (rb_hash_aref(flags, STR2SYM("allexplicit")))
      flags |= ALPM_TRANS_FLAG_ALLEXPLICIT;
    if (rb_hash_aref(flags, STR2SYM("unneeded")))
      flags |= ALPM_TRANS_FLAG_UNNEEDED;
    if (rb_hash_aref(flags, STR2SYM("recurseall")))
      flags |= ALPM_TRANS_FLAG_RECURSEALL;
    if (rb_hash_aref(flags, STR2SYM("nolock")))
      flags |= ALPM_TRANS_FLAG_NOLOCK;
  }
  else {
    rb_raise(rb_eArgError, "Wrong number of arguments, expected 0..1, got %d.", argc);
    return Qnil;
  }

  /* Create the transaction */
  if (alpm_trans_init(p_alpm, flags) < 0)
    return raise_last_alpm_error(p_alpm);

  /* Create an instance of Transaction. Note that alpm forces
   * you to only have *one* single Transaction instance, hence
   * there is no other way to instanciate this class apart from
   * this method. The user now modify and exute this sole
   * transaction. */
  transaction = rb_obj_alloc(rb_cAlpm_Transaction);
  rb_iv_set(transaction, "@alpm", self);
  result = rb_yield(transaction);

  /* When we get here we assume the user is done with
   * his stuff. Clean up. */
  if (alpm_trans_release(p_alpm) < 0)
    return raise_last_alpm_error(p_alpm);

  /* Return the last value from the block */
  return result;
}

/**
 * call-seq:
 *   local_db() → a_database
 *
 * Returns the database of locally installed packages.
 * An instance of Database.
 */
static VALUE local_db(VALUE self)
{
  alpm_handle_t* p_alpm = NULL;
  alpm_db_t* p_db = NULL;
  VALUE obj;
  Data_Get_Struct(self, alpm_handle_t, p_alpm);

  p_db = alpm_get_localdb(p_alpm);
  if (!p_db) {
    rb_raise(rb_eAlpm_Error, "Failed to retrieve local DB from libalpm.");
    return Qnil;
  }

  obj = Data_Wrap_Struct(rb_cAlpm_Database, NULL, NULL, p_db);
  rb_iv_set(obj, "@alpm", self);
  return obj;
}

/**
 * call-seq:
 *   sync_dbs() → an_array
 *
 * Returns an array of Database instances, each representing a single
 * sync database. You must register your remote databases previously
 * using #register_syncdb.
 */
static VALUE sync_dbs(VALUE self)
{
  alpm_handle_t* p_alpm = NULL;
  alpm_list_t* p_dbs = NULL;
  VALUE result;
  unsigned int i;
  Data_Get_Struct(self, alpm_handle_t, p_alpm);


  /* Get the list of all DBs */
  p_dbs = alpm_get_syncdbs(p_alpm);
  if (!p_dbs) {
    rb_raise(rb_eAlpm_Error, "Failed to retrieve sync DBs from libalpm.");
    return Qnil;
  }

  /* Transform them into a Ruby array of Database instances */
  result = rb_ary_new();
  for(i=0; i < alpm_list_count(p_dbs); i++) {
    VALUE db = Data_Wrap_Struct(rb_cAlpm_Database, NULL, NULL, alpm_list_nth(p_dbs, i));
    rb_iv_set(db, "@alpm", self);
    rb_ary_push(result, db);
  }

  /* Return that array */
  return result;
}

/**
 * call-seq:
 *   register_syncdb( reponame , siglevel ) → a_database
 *
 * Registers a remote synchronisation database with libalpm.
 *
 * === Parameters
 * [reponame]
 *   Name of the database/repository. That is, the name of the directory
 *   on the synchronisation server.
 * [siglevel]
 *   An array of one or more of the following values, denoting the security
 *   level of the packages.
 *   * :package
 *   * :package_optional
 *   * :package_marginal_ok
 *   * :package_unknown_ok
 *   * :database
 *   * :database_optional
 *   * :database_marginal_ok
 *   * :database_unknown_ok
 *   * :package_set
 *   * :package_trust_set
 *   * :use_default
 *
 * === Return value
 * The newly created Database instance.
 */
static VALUE register_syncdb(VALUE self, VALUE reponame, VALUE ary)
{
  alpm_handle_t* p_alpm = NULL;
  alpm_siglevel_t level;
  alpm_db_t* p_db = NULL;
  VALUE obj;

  Data_Get_Struct(self, alpm_handle_t, p_alpm);
  level = siglevel_from_ruby(ary);

  p_db = alpm_register_syncdb(p_alpm, StringValuePtr(reponame), level);
  if (!p_db) {
    rb_raise(rb_eAlpm_Error, "Failed to register sync db with libalpm");
    return Qnil;
  }

  obj = Data_Wrap_Struct(rb_cAlpm_Database, NULL, NULL, p_db);
  rb_iv_set(obj, "@alpm", self);
  return obj;
}

/**
 * call-seq:
 *   load_package( path , siglevel [, full ] ) → a_package
 *
 * Loads a Package from a file.
 *
 * === Parameters
 * [path]
 *   The path to the file to load.
 * [siglevel]
 *   The PGP signature level for the package. See #register_syncdb
 *   for the possible values in this array.
 * [full = false]
 *   If unset (the default), stop loading the package after the
 *   metadata.
 *
 * === Return value
 * An instance of Alpm::Package.
 */
static VALUE load_package(int argc, VALUE argv[], VALUE self)
{
  VALUE rpath, rlevel, rfull;
  int full = 0;
  alpm_handle_t* p_alpm = NULL;
  alpm_pkg_t* p_pkg = NULL;

  Data_Get_Struct(self, alpm_handle_t, p_alpm);
  rb_scan_args(argc, argv, "21", &rpath, &rlevel, &rfull);
  full = RTEST(rfull) ? 1 : 0;

  if (alpm_pkg_load(p_alpm, StringValuePtr(rpath), full, siglevel_from_ruby(rlevel), &p_pkg) < 0) {
    raise_last_alpm_error(p_alpm);
    return Qnil;
  }

  return Data_Wrap_Struct(rb_cAlpm_Package, NULL, free_loaded_pkg, p_pkg);
}

/**
 * call-seq:
 *   errno() → an_integer
 *
 * Error code of the last encountered error. See libalpm source
 * for the exact possible values. See #strerror for getting a
 * human-readable description from an error code.
 *
 * Beware this method returns nonsense if there was no error
 * enountered.
 */
static VALUE rberrno(VALUE self)
{
  alpm_handle_t* p_alpm = NULL;
  Data_Get_Struct(self, alpm_handle_t, p_alpm);

  return INT2NUM(alpm_errno(p_alpm));
}

/**
 * call-seq:
 *   strerror( code ) → a_string
 *
 * Takes a libalpm error code and returns a human-readable
 * description for it. The last encountered error’s code
 * can be obtained via #errno.
 */
static VALUE rbstrerror(VALUE self, VALUE errcode)
{
  return rb_str_new2(alpm_strerror(NUM2INT(errcode)));
}

/***************************************
 * Binding
 ***************************************/

/**
 * Document-class: Alpm::Error
 *
 * Exception class for errors in this library.
 */

/**
 * Document-class: Alpm
 *
 * Main class for interacting with Archlinux’ package management system.
 * Each instance operates on a _root_ directory (where packages are installed
 * under) and a _db_ directory (where permanent information like the list of
 * installed packages is kept). Additionally, it needs a #gpgdir to save the
 * package maintainer’s keyring to and an #arch, the CPU architecture to
 * download packages for.
 *
 * For a normal Archlinux system, the values are as follows:
 *
 * [root]
 *   /
 * [dbdir]
 *   /var/lib/pacman
 * [gpgdir]
 *   /etc/pacman.d/gnupg
 * [arch]
 *   <tt>x86_64</tt>
 */
void Init_alpm()
{
  rb_cAlpm = rb_define_class("Alpm", rb_cObject);
  rb_eAlpm_Error = rb_define_class_under(rb_cAlpm, "AlpmError", rb_eStandardError);
  rb_define_alloc_func(rb_cAlpm, allocate);

  rb_define_method(rb_cAlpm, "initialize", RUBY_METHOD_FUNC(initialize), 2);
  rb_define_method(rb_cAlpm, "inspect", RUBY_METHOD_FUNC(inspect), 0);
  rb_define_method(rb_cAlpm, "root", RUBY_METHOD_FUNC(root), 0);
  rb_define_method(rb_cAlpm, "dbpath", RUBY_METHOD_FUNC(dbpath), 0);
  rb_define_method(rb_cAlpm, "log", RUBY_METHOD_FUNC(set_logcb), 0);
  rb_define_method(rb_cAlpm, "gpgdir", RUBY_METHOD_FUNC(get_gpgdir), 0);
  rb_define_method(rb_cAlpm, "gpgdir=", RUBY_METHOD_FUNC(set_gpgdir), 1);
  rb_define_method(rb_cAlpm, "arch", RUBY_METHOD_FUNC(get_arch), 0);
  rb_define_method(rb_cAlpm, "arch=", RUBY_METHOD_FUNC(set_arch), 1);
  rb_define_method(rb_cAlpm, "transaction", RUBY_METHOD_FUNC(transaction), -1);
  rb_define_method(rb_cAlpm, "local_db", RUBY_METHOD_FUNC(local_db), 0);
  rb_define_method(rb_cAlpm, "sync_dbs", RUBY_METHOD_FUNC(sync_dbs), 0);
  rb_define_method(rb_cAlpm, "register_syncdb", RUBY_METHOD_FUNC(register_syncdb), 2);
  rb_define_method(rb_cAlpm, "load_package", RUBY_METHOD_FUNC(load_package), -1);
  rb_define_method(rb_cAlpm, "errno", RUBY_METHOD_FUNC(rberrno), 0);
  rb_define_method(rb_cAlpm, "strerror", RUBY_METHOD_FUNC(rbstrerror), 1);

  Init_database();
  Init_transaction();
  Init_package();
}
