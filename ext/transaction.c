#include "transaction.h"

/***************************************
 * Variables, etc
 ***************************************/

VALUE rb_cAlpm_Transaction;

/** Retrieves the associated Ruby Alpm instance from the given Package
 * instance, reads the C alpm_handle_t pointer from it and returns that
 * one. */
static alpm_handle_t* get_alpm_from_trans(VALUE trans)
{
  alpm_handle_t* p_handle = NULL;
  VALUE rb_alpm = rb_iv_get(trans, "@alpm");

  Data_Get_Struct(rb_alpm, alpm_handle_t, p_handle);
  return p_handle;
}

/***************************************
 * Methods
 ***************************************/

static VALUE initialize(VALUE self)
{
  rb_raise(rb_eNotImpError, "This class can't be instanciated. Use Alpm#transaction to create transactions.");
  return self;
}

/**
 * Set the associated Alpm instance. Private, do not call
 * from the outside.
 */
static VALUE set_alpm(VALUE self, VALUE alpm)
{
  rb_iv_set(self, "@alpm", alpm);
  return alpm;
}

/**
 * call-seq:
 *   add_package( pkg )
 *
 * Add a package to this transaction, marking it as to be installed.
 *
 * === Parameters
 * [package]
 *   An instance of Alpm::Package. This package will be installed
 *   into the Alpm root when you #commit this transaction.
 */
static VALUE add_package(VALUE self, VALUE package)
{
  alpm_handle_t* p_alpm = NULL;
  alpm_pkg_t* p_pkg = NULL;

  Data_Get_Struct(package, alpm_pkg_t, p_pkg);
  p_alpm = get_alpm_from_trans(self);

  alpm_add_pkg(p_alpm, p_pkg);

  return package;
}

/**
 * call-seq:
 *   self << pkg → self
 *
 * Like #add_package, but returns +self+ for method chaining.
 */
static VALUE add_package2(VALUE self, VALUE package)
{
  add_package(self, package);
  return self;
}

/**
 * call-seq:
 *   each_added_package{|pkg| ...}
 *   each_added_package() → an_enumerator
 *
 * Iterates over all packages (instances of Package) that would
 * be added by this transaction. If called without a block,
 * an enumerator is returned.
 *
 * === Parameters
 * [pkg (Block)]
 *   The currently iterated Package.
 *
 * === Return value
 * Undefined, if a block is given. An Enumerator otherwise.
 */
static VALUE each_added_package(VALUE self)
{
  alpm_list_t* p_pkgs = NULL;
  unsigned int i;

  p_pkgs = alpm_trans_get_add(get_alpm_from_trans(self));

  RETURN_ENUMERATOR(self, 0, NULL);
  for(i=0; i < alpm_list_count(p_pkgs); i++){
    alpm_pkg_t* p_pkg = (alpm_pkg_t*) alpm_list_nth(p_pkgs, i);
    rb_yield(Data_Wrap_Struct(rb_cAlpm_Package, NULL, NULL, p_pkg));
  }

  return Qnil;
}

/**
 * call-seq:
 *   each_removed_package{|pkg| ...}
 *   each_removed_package() → an_enumerator
 *
 * Iterates over all packages (instances of Package) that would
 * be removed by this transaction. If called without a block,
 * an enumerator is returned.
 *
 * === Parameters
 * [pkg (Block)]
 *   The currently iterated Package.
 *
 * === Return value
 * Undefined, if a block is given. An Enumerator otherwise.
 */
static VALUE each_removed_package(VALUE self)
{
  alpm_list_t* p_pkgs = NULL;
  unsigned int i;

  p_pkgs = alpm_trans_get_remove(get_alpm_from_trans(self));

  RETURN_ENUMERATOR(self, 0, NULL);
  for(i=0; i < alpm_list_count(p_pkgs); i++){
    alpm_pkg_t* p_pkg = (alpm_pkg_t*) alpm_list_nth(p_pkgs, i);
    rb_yield(Data_Wrap_Struct(rb_cAlpm_Package, NULL, NULL, p_pkg));
  }

  return Qnil;
}

/***************************************
 * Binding
 ***************************************/

/**
 * Document-class: Alpm::Transaction
 *
 * Transactions are the interesting part when working with Archlinux’
 * package management. Each operation, be it sync, upgrade, query, or
 * remove, is essentially a transaction, represented by an instance of
 * this class. They must first be constructed, then prepared, and finally
 * committed before they take any effect.
 *
 * As libalpm only allows a single active transaction at a time, you can’t
 * simply instanciate this class. Instead, call Alpm#transaction, which
 * puts libalpm into transaction mode and creates an instance of Transaction
 * for you which you can operate on. Make your adjustments, then call #prepare
 * to have libalpm prepare the transaction by e.g. resolving dependencies. Then,
 * call #commit to modify both the system and the database. When the original
 * block you passed to Alpm#transaction ends, libalpm is instructed to leave
 * transaction mode. Don’t save the Transaction instance passed to the block,
 * is is useless after the block has finished.
 */
void Init_transaction()
{
  rb_cAlpm_Transaction = rb_define_class_under(rb_cAlpm, "Transaction", rb_cObject);

  rb_define_method(rb_cAlpm_Transaction, "initialize", RUBY_METHOD_FUNC(initialize), 0);
  rb_define_method(rb_cAlpm_Transaction, "add_package", RUBY_METHOD_FUNC(add_package), 1);
  rb_define_method(rb_cAlpm_Transaction, "<<", RUBY_METHOD_FUNC(add_package2), 1);
  rb_define_private_method(rb_cAlpm_Transaction, "alpm=", RUBY_METHOD_FUNC(set_alpm), 1);
  rb_define_method(rb_cAlpm_Transaction, "each_added_package", RUBY_METHOD_FUNC(each_added_package), 0);
  rb_define_method(rb_cAlpm_Transaction, "each_removed_package", RUBY_METHOD_FUNC(each_removed_package), 0);
}
