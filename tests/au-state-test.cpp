#include <AudioToolbox/AudioToolbox.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static void check(OSStatus result, const char* operation)
{
    if (result != noErr) {
        std::fprintf(stderr, "%s failed: %d\n", operation, (int) result);
        std::exit(1);
    }
}

int main(int argc, char** argv)
{
    AudioComponentDescription description {'aufx', 'Tv64', 'Tvfn', 0, 0};
    AudioComponent component = nullptr;
    CFBundleRef bundle = nullptr;
    if (argc == 2) {
        auto url = CFURLCreateFromFileSystemRepresentation(nullptr,
            reinterpret_cast<const UInt8*>(argv[1]), std::strlen(argv[1]), true);
        bundle = CFBundleCreate(nullptr, url);
        CFRelease(url);
        if (!bundle || !CFBundleLoadExecutable(bundle)) return 1;
        auto factory = reinterpret_cast<AudioComponentFactoryFunction>(
            CFBundleGetFunctionPointerForName(bundle, CFSTR("TVFDN_64AUFactory")));
        if (!factory) return 1;
        component = AudioComponentRegister(&description, CFSTR("TVFDN state test"), 0x10000, factory);
    } else {
        component = AudioComponentFindNext(nullptr, &description);
    }
    if (!component) return std::fprintf(stderr, "AU unavailable\n"), 1;
    AudioUnit writer = nullptr;
    check(AudioComponentInstanceNew(component, &writer), "create writer");
    UInt32 bytes = 0;
    check(AudioUnitGetPropertyInfo(writer, kAudioUnitProperty_ParameterList,
        kAudioUnitScope_Global, 0, &bytes, nullptr), "parameter list size");
    std::vector<AudioUnitParameterID> ids(bytes / sizeof(AudioUnitParameterID));
    check(AudioUnitGetProperty(writer, kAudioUnitProperty_ParameterList,
        kAudioUnitScope_Global, 0, ids.data(), &bytes), "parameter list");
    if (ids.size() != 8) return std::fprintf(stderr, "Expected eight parameters\n"), 1;
    std::vector<AudioUnitParameterValue> expected;
    for (auto id : ids) {
        AudioUnitParameterInfo info {};
        bytes = sizeof(info);
        check(AudioUnitGetProperty(writer, kAudioUnitProperty_ParameterInfo,
            kAudioUnitScope_Global, id, &info, &bytes), "parameter info");
        auto value = info.defaultValue < (info.minValue + info.maxValue) / 2
            ? info.maxValue : info.minValue;
        check(AudioUnitSetParameter(writer, id, kAudioUnitScope_Global, 0, value, 0), "set parameter");
        expected.push_back(value);
    }
    CFPropertyListRef state = nullptr;
    bytes = sizeof(state);
    check(AudioUnitGetProperty(writer, kAudioUnitProperty_ClassInfo,
        kAudioUnitScope_Global, 0, &state, &bytes), "save AU state");
    auto serialized = CFPropertyListCreateData(nullptr, state, kCFPropertyListBinaryFormat_v1_0, 0, nullptr);
    CFRelease(state);
    check(AudioComponentInstanceDispose(writer), "destroy writer");
    if (!serialized) return 1;
    state = CFPropertyListCreateWithData(nullptr, serialized, kCFPropertyListImmutable, nullptr, nullptr);
    CFRelease(serialized);
    if (!state) return 1;
    AudioUnit reader = nullptr;
    check(AudioComponentInstanceNew(component, &reader), "create fresh reader");
    check(AudioUnitSetProperty(reader, kAudioUnitProperty_ClassInfo,
        kAudioUnitScope_Global, 0, &state, sizeof(state)), "restore AU state");
    CFRelease(state);
    for (size_t i = 0; i < ids.size(); ++i) {
        AudioUnitParameterValue actual = 0;
        check(AudioUnitGetParameter(reader, ids[i], kAudioUnitScope_Global, 0, &actual), "read restored parameter");
        if (std::abs(actual - expected[i]) > 0.0001f) {
            std::fprintf(stderr, "FAIL: parameter %u: expected %g, got %g\n", ids[i], expected[i], actual);
            return 1;
        }
    }
    check(AudioComponentInstanceDispose(reader), "destroy reader");
    if (bundle) CFRelease(bundle);
    std::printf("PASS: all eight parameters restored after binary state serialization and fresh AU instantiation\n");
}
