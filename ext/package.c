#include "package.h"

/***************************************
 * Variables, etc
 ***************************************/

VALUE rb_cAlpm_Package;

/***************************************
 * Methods
 ***************************************/

static VALUE initialize(VALUE self)
{
  rb_raise(rb_eNotImpError, "Can't create new packages with this library.");
  return self;
}

/**
 * call-seq:
 *   filename() → a_string
 *
 * Filename of the package file.
 */
static VALUE filename(VALUE self)
{
  alpm_pkg_t* p_pkg = NULL;
  Data_Get_Struct(self, alpm_pkg_t, p_pkg);

  return rb_str_new2(alpm_pkg_get_filename(p_pkg));
}

/**
 * call-seq:
 *   name() → a_string
 *
 * Name of the package.
 */
static VALUE name(VALUE self)
{
  alpm_pkg_t* p_pkg = NULL;
  Data_Get_Struct(self, alpm_pkg_t, p_pkg);

  return rb_str_new2(alpm_pkg_get_name(p_pkg));
}

/**
 * call-seq:
 *   version() → a_string
 *
 * Version number of the package.
 */
static VALUE version(VALUE self)
{
  alpm_pkg_t* p_pkg = NULL;
  Data_Get_Struct(self, alpm_pkg_t, p_pkg);

  return rb_str_new2(alpm_pkg_get_version(p_pkg));
}

/**
 * call-seq:
 *   inspect() → a_string
 *
 * Human-readable description.
 */
static VALUE inspect(VALUE self)
{
  alpm_pkg_t* p_pkg = NULL;
  char buf[256];
  int len;
  Data_Get_Struct(self, alpm_pkg_t, p_pkg);

  len = sprintf(buf, "#<%s %s (%s)>",
                rb_obj_classname(self),
                alpm_pkg_get_name(p_pkg),
                alpm_pkg_get_version(p_pkg));

  return rb_str_new(buf, len);
}

/***************************************
 * Binding
 ***************************************/

void Init_package()
{
  rb_cAlpm_Package = rb_define_class_under(rb_cAlpm, "Package", rb_cObject);

  rb_define_method(rb_cAlpm_Package, "initialize", RUBY_METHOD_FUNC(initialize), 0);
  rb_define_method(rb_cAlpm_Package, "filename", filename, 0);
  rb_define_method(rb_cAlpm_Package, "name", name, 0);
  rb_define_method(rb_cAlpm_Package, "version", version, 0);
  rb_define_method(rb_cAlpm_Package, "inspect", RUBY_METHOD_FUNC(inspect), 0);
}
