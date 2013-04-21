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

/**
 * call-seq:
 *   description() → a_string
 *   desc()        → a_string
 *
 * Returns the description for this package.
 */
static VALUE description(VALUE self)
{
  alpm_pkg_t* p_pkg = NULL;
  Data_Get_Struct(self, alpm_pkg_t, p_pkg);

  return rb_str_new2(alpm_pkg_get_desc(p_pkg));
}

/**
 * call-seq:
 *   url() → a_string
 *
 * Returns the homepage for this package.
 */
static VALUE url(VALUE self)
{
  alpm_pkg_t* p_pkg = NULL;
  Data_Get_Struct(self, alpm_pkg_t, p_pkg);

  return rb_str_new2(alpm_pkg_get_url(p_pkg));
}

/**
 * call-seq:
 *   packager() → a_string
 *
 * The packager’s name.
 */
static VALUE packager(VALUE self)
{
  alpm_pkg_t* p_pkg = NULL;
  Data_Get_Struct(self, alpm_pkg_t, p_pkg);

  return rb_str_new2(alpm_pkg_get_packager(p_pkg));
}

/**
 * call-seq:
 *   md5sum() → a_string
 *
 * The package’s MD5 checksum
 */
static VALUE md5sum(VALUE self)
{
  alpm_pkg_t* p_pkg = NULL;
  Data_Get_Struct(self, alpm_pkg_t, p_pkg);

  return rb_str_new2(alpm_pkg_get_md5sum(p_pkg));
}

/**
 * call-seq:
 *   sha256sum() → a_string
 *
 * The package’s SHA256 checksum
 */
static VALUE sha256sum(VALUE self)
{
  alpm_pkg_t* p_pkg = NULL;
  Data_Get_Struct(self, alpm_pkg_t, p_pkg);

  return rb_str_new2(alpm_pkg_get_sha256sum(p_pkg));
}

/**
 * call-seq:
 *   size() → an_integer
 *
 * The size of the package, in bytes. Only available for sync
 * databases and package files, not for packages from
 * the +local+ database.
 */
static VALUE size(VALUE self)
{
  alpm_pkg_t* p_pkg = NULL;
  Data_Get_Struct(self, alpm_pkg_t, p_pkg);

  return LONG2NUM(alpm_pkg_get_size(p_pkg));
}

/**
 * call-seq:
 *   installed_size() → an_integer
 *   isize() → an_integer
 *
 * The installed size of the package, in bytes.
 */
static VALUE installed_size(VALUE self)
{
  alpm_pkg_t* p_pkg = NULL;
  Data_Get_Struct(self, alpm_pkg_t, p_pkg);

  return LONG2NUM(alpm_pkg_get_isize(p_pkg));
}

/**
 * call-seq:
 *   self <=> other → nil, -1, 0, or 1
 *
 * Compares two packages and checks which one is newer based
 * on the version numbers.
 *
 * === Parameter
 * [other]
 *   Another Package instance.
 *
 * === Return value
 * If +other+ isn’t a Package instance, returns +nil+. If
 * +other+ has another #name, compares the names using the
 * current C locale (+LC_COLLATE+) and returns -1 if +self+
 * comes alphabetically first, 0 if the names are equal, and
 * 1 if +other+ comes alphabetically first.
 *
 * Otherwise, compares the version numbers of both packages and
 * returns -1 if if +self+ is newer, 0 if both are equal, and 1
 * if +other+ is newer. Note that the version check includes
 * checks on the package _epoch_ and the _pkgrel_, so the
 * result may not be the way you expect it on the first glance.
 * Quoting from the libalpm source in <tt>version.c</tt>:
 *
 * “Different epoch values for version strings will override any further
 * comparison. If no epoch is provided, 0 is assumed.
 *
 * Keep in mind that the pkgrel is only compared if it is available
 * on both versions handed to this function. For example, comparing
 * 1.5-1 and 1.5 will yield 0; comparing 1.5-1 and 1.5-2 will yield
 * -1 as expected. This is mainly for supporting versioned dependencies
 * that do not include the pkgrel.”
 */
static VALUE compare(VALUE self, VALUE other)
{
  alpm_pkg_t* p_pkg1 = NULL;
  alpm_pkg_t* p_pkg2 = NULL;
  int result;

  /* Don’t compare apples with pears */
  if (!RTEST(rb_obj_is_kind_of(other, rb_cAlpm_Package)))
    return Qnil;

  Data_Get_Struct(self, alpm_pkg_t, p_pkg1);
  Data_Get_Struct(other, alpm_pkg_t, p_pkg2);

  /* First compare names. If they’re different, sort alphabetically. */
  result = strcoll(alpm_pkg_get_name(p_pkg1), alpm_pkg_get_name(p_pkg2));
  if (result == 0) {
    /* OK, names are equal. Compare the version numbers. */
    result = alpm_pkg_vercmp(alpm_pkg_get_version(p_pkg1), alpm_pkg_get_version(p_pkg2));
    return INT2NUM(result);
  }
  else
    return INT2NUM(result);
}

/***************************************
 * Binding
 ***************************************/

void Init_package()
{
  rb_cAlpm_Package = rb_define_class_under(rb_cAlpm, "Package", rb_cObject);
  rb_include_module(rb_cAlpm_Package, rb_mComparable);

  rb_define_method(rb_cAlpm_Package, "initialize", RUBY_METHOD_FUNC(initialize), 0);
  rb_define_method(rb_cAlpm_Package, "filename", filename, 0);
  rb_define_method(rb_cAlpm_Package, "name", name, 0);
  rb_define_method(rb_cAlpm_Package, "version", version, 0);
  rb_define_method(rb_cAlpm_Package, "inspect", RUBY_METHOD_FUNC(inspect), 0);
  rb_define_method(rb_cAlpm_Package, "description", RUBY_METHOD_FUNC(description), 0);
  rb_define_method(rb_cAlpm_Package, "url", RUBY_METHOD_FUNC(url), 0);
  rb_define_method(rb_cAlpm_Package, "md5sum", RUBY_METHOD_FUNC(md5sum), 0);
  rb_define_method(rb_cAlpm_Package, "sha256sum", RUBY_METHOD_FUNC(sha256sum), 0);
  rb_define_method(rb_cAlpm_Package, "size", RUBY_METHOD_FUNC(size), 0);
  rb_define_method(rb_cAlpm_Package, "installed_size", RUBY_METHOD_FUNC(installed_size), 0);
  rb_define_method(rb_cAlpm_Package, "packager", RUBY_METHOD_FUNC(packager), 0);
  rb_define_method(rb_cAlpm_Package, "<=>", RUBY_METHOD_FUNC(compare), 1);

  rb_define_alias(rb_cAlpm_Package, "desc", "description");
  rb_define_alias(rb_cAlpm_Package, "isize", "installed_size");
}
