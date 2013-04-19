#include "transaction.h"

/***************************************
 * Variables, etc
 ***************************************/

VALUE Transaction;

/** Retrieves the associated Ruby Alpm instance from the given Package
 * instance, reads the C alpm_handle_t pointer from it and returns that
 * one. */
static alpm_handle_t* get_alpm_from_pkg(VALUE package)
{
  alpm_handle_t* p_handle = NULL;
  VALUE rb_alpm = rb_iv_get(package, "@alpm");

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
  p_alpm = get_alpm_from_pkg(package);

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
  Transaction = rb_define_class_under(Alpm, "Transaction", rb_cObject);

  rb_define_method(Transaction, "initialize", RUBY_METHOD_FUNC(initialize), 0); /** :nodoc: */
  rb_define_method(Transaction, "add_package", RUBY_METHOD_FUNC(add_package), 1);
  rb_define_method(Transaction, "<<", RUBY_METHOD_FUNC(add_package2), 1);
  rb_define_private_method(Transaction, "alpm=", RUBY_METHOD_FUNC(set_alpm), 1);
}