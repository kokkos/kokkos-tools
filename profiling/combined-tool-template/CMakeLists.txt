add_subdirectory(example-tools)

# Creating the combined tool library
kp_add_library(kp_combined_tool 
    combined-tool-template.cpp
)

# Adding the example tools to the combined tool library
target_link_libraries(kp_combined_tool PUBLIC kp_combined_simple kp_combined_daemon_timer)