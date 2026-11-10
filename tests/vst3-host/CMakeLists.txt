cmake_minimum_required(VERSION 3.22)
project(TVFDNVST3Tests LANGUAGES C CXX)

set(JUCE_SOURCE_DIR "" CACHE PATH "Path to the JUCE checkout used by the plugin build")
add_subdirectory("${JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
add_executable(TVFDNVST3Test main.cpp)
target_compile_features(TVFDNVST3Test PRIVATE cxx_std_17)
target_compile_definitions(TVFDNVST3Test PRIVATE
    JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_AU=0 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(TVFDNVST3Test PRIVATE juce::juce_audio_processors_headless
    juce::juce_recommended_config_flags)
enable_testing()
add_test(NAME VST3StateAndRender COMMAND TVFDNVST3Test)
set_tests_properties(VST3StateAndRender PROPERTIES TIMEOUT 180)
