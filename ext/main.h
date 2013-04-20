#ifndef RUBY_ALPM_MAIN_H
#define RUBY_ALPM_MAIN_H
#include <ruby.h>
#include <alpm.h>
#include <alpm_list.h>

// Directly creates a Ruby Symbol from a C string.
#define STR2SYM(str) ID2SYM(rb_intern(str))
// Directly creates a C string from a Ruby symbol.
#define SYM2STR(sym) rb_id2name(SYM2ID(sym))

extern VALUE rb_cAlpm;
extern VALUE rb_eAlpm_Error;

VALUE raise_last_alpm_error(alpm_handle_t* p_handle);
void Init_alpm();

#endif
