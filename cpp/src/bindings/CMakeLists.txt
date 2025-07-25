# Python bindings for claude_draw C++ optimization layer
pybind11_add_module(_claude_draw_cpp MODULE
    # Main binding file
    module.cpp
    
    # Core type bindings
    core_types.cpp
    
    # Shape bindings (to be added)
    # shape_bindings.cpp
    
    # Container bindings (to be added)
    # container_bindings.cpp
    
    # Utilities (to be added)
    # conversion_utils.cpp
)

# Link with core library
target_link_libraries(_claude_draw_cpp PRIVATE claude_draw_core)

# Set properties
set_target_properties(_claude_draw_cpp PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    INTERPROCEDURAL_OPTIMIZATION TRUE
    PREFIX "${PYTHON_MODULE_PREFIX}"
    SUFFIX "${PYTHON_MODULE_EXTENSION}"
)

# Optimized type conversion policies
target_compile_definitions(_claude_draw_cpp PRIVATE
    PYBIND11_DETAILED_ERROR_MESSAGES
)

# Install the Python module
install(TARGETS _claude_draw_cpp 
    LIBRARY DESTINATION ${Python3_SITEARCH}/claude_draw
)