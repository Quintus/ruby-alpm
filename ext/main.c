#include "main.h"

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

/***************************************
 * Variables, etc
 ***************************************/

VALUE Alpm;

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

/***************************************
 * Binding
 ***************************************/

void Init_alpm()
{
  Alpm = rb_define_class("Alpm", rb_cObject);
  rb_define_alloc_func(Alpm, allocate);

  rb_define_method(Alpm, "initialize", RUBY_METHOD_FUNC(initialize), 2);
  rb_define_method(Alpm, "inspect", RUBY_METHOD_FUNC(inspect), 0);
  rb_define_method(Alpm, "root", RUBY_METHOD_FUNC(root), 0);
  rb_define_method(Alpm, "dbpath", RUBY_METHOD_FUNC(dbpath), 0);
  rb_define_method(Alpm, "log", RUBY_METHOD_FUNC(set_logcb), 0);
  rb_define_method(Alpm, "gpgdir", RUBY_METHOD_FUNC(get_gpgdir), 0);
  rb_define_method(Alpm, "gpgdir=", RUBY_METHOD_FUNC(set_gpgdir), 1);
  rb_define_method(Alpm, "arch", RUBY_METHOD_FUNC(get_arch), 0);
  rb_define_method(Alpm, "arch=", RUBY_METHOD_FUNC(set_arch), 1);
}
