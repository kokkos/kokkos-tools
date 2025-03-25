# How to Build

# With Cmake

1. Create your build directory and go to it (in Kokkos Tools e.g. type `mkdir myBuild; cd myBuild`)

2. Type `cmake .. -DCMAKE_INSTALL_PREFIX=${YOUR_KOKKOS_TOOLS_INSTALL_DIR}`  and change any options, including tools you want turned on  (some are by default off).  (Optional)

3. Type `make`

# With Makefile (recommended)

1. Go into the directory of the particular tool, e.g., `cd debugging/kernel_logger`

2. Type `make`

3. This generates the shared library within that subdirectory.


### Run

Given your installed tool shared library `lib<name_of_tool_shared_lib>.so` and an application executable called yourApplication.exe, type:

`export KOKKOS_TOOLS_LIBS=${YOUR_KOKKOS_TOOLS_INSTALL_DIR}/lib<name_of_tool_shared_lib>.so; ./yourApplication.exe`
