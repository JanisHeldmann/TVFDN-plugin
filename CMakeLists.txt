cmake_minimum_required(VERSION 3.22)
project(TVFDN VERSION 1.2.0 LANGUAGES C CXX)

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

function(add_tvfdn_plugin target product_name plugin_code bundle_id)
    juce_add_plugin(${target}
        COMPANY_NAME "TVFDN"
        BUNDLE_ID "${bundle_id}"
        PLUGIN_MANUFACTURER_CODE Tvfn
        PLUGIN_CODE ${plugin_code}
        FORMATS ${TVFDN_FORMATS}
        PRODUCT_NAME "${product_name}"
        COPY_PLUGIN_AFTER_BUILD FALSE
        AU_MAIN_TYPE kAudioUnitType_Effect
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE)
    juce_generate_juce_header(${target})
    target_sources(${target} PRIVATE source/PluginProcessor.cpp source/PluginEditor.cpp)
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_compile_definitions(${target} PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(${target} PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)
endfunction()

add_tvfdn_plugin(TVFDN "TVFDN 64" Tv64 "org.tvfdn.plugin64")
add_tvfdn_plugin(TVFDNStereo "TVFDN Stereo" TvSt "org.tvfdn.pluginstereo")
target_compile_definitions(TVFDNStereo PUBLIC TVFDN_STEREO=1)

include(CTest)
if(BUILD_TESTING AND APPLE AND "AU" IN_LIST TVFDN_FORMATS)
    add_executable(TVFDNAUStateTest tests/au-state-test.cpp)
    target_compile_features(TVFDNAUStateTest PRIVATE cxx_std_17)
    target_link_libraries(TVFDNAUStateTest PRIVATE "-framework AudioToolbox" "-framework CoreFoundation")
    add_dependencies(TVFDNAUStateTest TVFDN_AU TVFDNStereo_AU)
    add_test(NAME AUParameterStateRoundTrip
        COMMAND TVFDNAUStateTest "$<TARGET_BUNDLE_DIR:TVFDN_AU>")
    add_test(NAME AUStereoParameterStateRoundTrip
        COMMAND TVFDNAUStateTest "$<TARGET_BUNDLE_DIR:TVFDNStereo_AU>" stereo)
endif()
