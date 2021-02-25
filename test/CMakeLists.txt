cet_enable_asserts()

# Make sure tests have correct environment settings.
include(CetTest)
# If you explicitly include CetTest in a subdirectory, you will need to
# re-initialize the test environment.
cet_test_env("FHICL_FILE_PATH=.")

add_subdirectory(Binaries)
add_subdirectory(Destinations)
add_subdirectory(Extensions)
add_subdirectory(Receivers)
