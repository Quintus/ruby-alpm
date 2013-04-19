require "mkmf"

$CFLAGS += " -Wall"

dir_config "alpm"

unless find_header("alpm.h", "/usr/include")
  abort "Could not find alpm.h"
end

unless find_library("alpm", "alpm_initialize", "/usr/lib")
  abort "Could not find alpm_initialize() in libalpm"
end

create_makefile "alpm"
