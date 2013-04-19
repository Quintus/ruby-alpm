#include "package.h"

/***************************************
 * Variables, etc
 ***************************************/

VALUE Package;

/***************************************
 * Methods
 ***************************************/

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

/***************************************
 * Binding
 ***************************************/

void Init_package()
{
  Package = rb_define_class_under(Alpm, "Package", rb_cObject);

  rb_define_method(Package, "filename", filename, 0);
  rb_define_method(Package, "name", name, 0);
  rb_define_method(Package, "version", version, 0);
}
