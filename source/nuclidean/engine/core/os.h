// Project Nuclidean Source File
#pragma once

#include <types.h>

namespace nc::os
{

// Assigns a name for the thread running the command. Both on debug and ship builds
void set_current_thread_name(cstr name);

}
