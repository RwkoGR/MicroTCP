# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project"

# Include any dependencies generated for this target.
include test/CMakeFiles/traffic_generator.dir/depend.make

# Include the progress variables for this target.
include test/CMakeFiles/traffic_generator.dir/progress.make

# Include the compile flags for this target's objects.
include test/CMakeFiles/traffic_generator.dir/flags.make

test/CMakeFiles/traffic_generator.dir/traffic_generator.cpp.o: test/CMakeFiles/traffic_generator.dir/flags.make
test/CMakeFiles/traffic_generator.dir/traffic_generator.cpp.o: test/traffic_generator.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/CMakeFiles/traffic_generator.dir/traffic_generator.cpp.o"
	cd "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/test" && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/traffic_generator.dir/traffic_generator.cpp.o -c "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/test/traffic_generator.cpp"

test/CMakeFiles/traffic_generator.dir/traffic_generator.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/traffic_generator.dir/traffic_generator.cpp.i"
	cd "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/test" && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/test/traffic_generator.cpp" > CMakeFiles/traffic_generator.dir/traffic_generator.cpp.i

test/CMakeFiles/traffic_generator.dir/traffic_generator.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/traffic_generator.dir/traffic_generator.cpp.s"
	cd "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/test" && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/test/traffic_generator.cpp" -o CMakeFiles/traffic_generator.dir/traffic_generator.cpp.s

# Object files for target traffic_generator
traffic_generator_OBJECTS = \
"CMakeFiles/traffic_generator.dir/traffic_generator.cpp.o"

# External object files for target traffic_generator
traffic_generator_EXTERNAL_OBJECTS =

test/traffic_generator: test/CMakeFiles/traffic_generator.dir/traffic_generator.cpp.o
test/traffic_generator: test/CMakeFiles/traffic_generator.dir/build.make
test/traffic_generator: lib/libmicrotcp.so
test/traffic_generator: test/CMakeFiles/traffic_generator.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable traffic_generator"
	cd "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/test" && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/traffic_generator.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/CMakeFiles/traffic_generator.dir/build: test/traffic_generator

.PHONY : test/CMakeFiles/traffic_generator.dir/build

test/CMakeFiles/traffic_generator.dir/clean:
	cd "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/test" && $(CMAKE_COMMAND) -P CMakeFiles/traffic_generator.dir/cmake_clean.cmake
.PHONY : test/CMakeFiles/traffic_generator.dir/clean

test/CMakeFiles/traffic_generator.dir/depend:
	cd "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project" "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/test" "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project" "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/test" "/mnt/c/Users/Rwko/Documents/MEGA/MEGAsync/CSD-SEM5/HY335-Computer Networks/Project_phase_A/HY335a_Project/test/CMakeFiles/traffic_generator.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : test/CMakeFiles/traffic_generator.dir/depend

