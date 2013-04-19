#include "main.h"

VALUE Alpm;

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
 *
 * root() → a_string
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
 *
 * dbpath() → a_string
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

void Init_alpm()
{
  Alpm = rb_define_class("Alpm", rb_cObject);
  rb_define_alloc_func(Alpm, allocate);

  rb_define_method(Alpm, "initialize", RUBY_METHOD_FUNC(initialize), 2);
  rb_define_method(Alpm, "inspect", RUBY_METHOD_FUNC(inspect), 0);
  rb_define_method(Alpm, "root", RUBY_METHOD_FUNC(root), 0);
  rb_define_method(Alpm, "dbpath", RUBY_METHOD_FUNC(dbpath), 0);
}
