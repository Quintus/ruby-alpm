#ifndef RUBY_ALPM_MAIN_H
#define RUBY_ALPM_MAIN_H
#include <ruby.h>
#include <alpm.h>
#include <alpm_list.h>

// Directly creates a Ruby Symbol from a C string.
#define STR2SYM(str) ID2SYM(rb_intern(str))

extern VALUE Alpm;
extern VALUE AlpmError;

VALUE raise_last_alpm_error(alpm_handle_t* p_handle);
void Init_alpm();

#endif
