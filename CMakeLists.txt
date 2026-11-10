cmake_minimum_required(VERSION 3.22)
project(TVFDN VERSION 1.1.0 LANGUAGES C CXX)

set(JUCE_SOURCE_DIR "" CACHE PATH "Path to a JUCE checkout (tested with 8.0.17-security)")
if(NOT EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Set JUCE_SOURCE_DIR to a JUCE checkout")
endif()
add_subdirectory("${JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")

set(default_formats VST3)
if(APPLE)
    list(APPEND default_formats AU)
endif()
set(TVFDN_FORMATS "${default_formats}" CACHE STRING "Plugin formats to build")

juce_add_plugin(TVFDN
    COMPANY_NAME "TVFDN"
    BUNDLE_ID "org.tvfdn.plugin64"
    PLUGIN_MANUFACTURER_CODE Tvfn
    PLUGIN_CODE Tv64
    FORMATS ${TVFDN_FORMATS}
    PRODUCT_NAME "TVFDN 64"
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE)
juce_generate_juce_header(TVFDN)
target_sources(TVFDN PRIVATE source/PluginProcessor.cpp source/PluginEditor.cpp)
target_compile_features(TVFDN PRIVATE cxx_std_17)
target_compile_definitions(TVFDN PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(TVFDN PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags)

include(CTest)
if(BUILD_TESTING AND APPLE AND "AU" IN_LIST TVFDN_FORMATS)
    add_executable(TVFDNAUStateTest tests/au-state-test.cpp)
    target_compile_features(TVFDNAUStateTest PRIVATE cxx_std_17)
    target_link_libraries(TVFDNAUStateTest PRIVATE "-framework AudioToolbox" "-framework CoreFoundation")
    add_dependencies(TVFDNAUStateTest TVFDN_AU)
    add_test(NAME AUParameterStateRoundTrip
        COMMAND TVFDNAUStateTest "$<TARGET_BUNDLE_DIR:TVFDN_AU>")
endif()
