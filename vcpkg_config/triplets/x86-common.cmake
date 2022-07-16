# Included by platform-specific triplets to configure third party dependencies.
# Do not use as a triplet directly.

# Default to static linkage for all dependencies.
set(VCPKG_LIBRARY_LINKAGE static)

# Override linkage to dynamic only for these.
if(PORT MATCHES "openal-soft")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()

set(VCPKG_C_FLAGS "")
set(VCPKG_CXX_FLAGS "-std=c++20")
