#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/"])
env.Append(LIBS=["input", "zinnia"])
sources = Glob("src/*.cpp")

libname = "libtoasterutils"

library = env.SharedLibrary(
    "bin/" + libname + env["suffix"] + env["SHLIBSUFFIX"],
    source=sources,
)

Default(library)
