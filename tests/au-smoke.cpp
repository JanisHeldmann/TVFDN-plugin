#include <AudioToolbox/AudioToolbox.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(OSStatus result, const char* operation)
{
    if (result != noErr) {
        std::fprintf(stderr, "%s failed: %d\n", operation, (int) result);
        std::exit(1);
    }
}

struct Input {
    std::vector<float> samples = std::vector<float>(64 * 512);
    bool impulse = true;
};

static OSStatus input(void* context, AudioUnitRenderActionFlags*, const AudioTimeStamp*,
                      UInt32, UInt32 frames, AudioBufferList* buffers)
{
    auto& data = *static_cast<Input*>(context);
    std::fill(data.samples.begin(), data.samples.end(), 0.0f);
    if (data.impulse) data.samples[63 * 512] = 0.1f;
    data.impulse = false;
    for (UInt32 c = 0; c < buffers->mNumberBuffers; ++c) {
        buffers->mBuffers[c] = {1, static_cast<UInt32>(frames * sizeof(float)), data.samples.data() + c * 512};
    }
    return noErr;
}

int main()
{
    AudioComponentDescription description {'aufx', 'Tv64', 'Tvfn', 0, 0};
    auto component = AudioComponentFindNext(nullptr, &description);
    if (component == nullptr) return std::fprintf(stderr, "AU not registered\n"), 1;
    AudioUnit unit = nullptr;
    check(AudioComponentInstanceNew(component, &unit), "instantiate");
    Input source;
    AURenderCallbackStruct callback {input, &source};
    check(AudioUnitSetProperty(unit, kAudioUnitProperty_SetRenderCallback,
                              kAudioUnitScope_Input, 0, &callback, sizeof(callback)), "input callback");
    UInt32 maximumFrames = 512;
    check(AudioUnitSetProperty(unit, kAudioUnitProperty_MaximumFramesPerSlice,
                              kAudioUnitScope_Global, 0, &maximumFrames, sizeof(maximumFrames)), "block size");
    std::vector<unsigned char> storage(offsetof(AudioBufferList, mBuffers) + 64 * sizeof(AudioBuffer));
    auto* buffers = reinterpret_cast<AudioBufferList*>(storage.data());
    std::vector<float> output(64 * 512);
    for (double rate : {44100.0, 48000.0, 96000.0}) {
        AudioStreamBasicDescription format {rate, kAudioFormatLinearPCM,
            kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved,
            sizeof(float), 1, sizeof(float), 64, 32, 0};
        check(AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Input, 0, &format, sizeof(format)), "64-channel input");
        check(AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Output, 0, &format, sizeof(format)), "64-channel output");
        check(AudioUnitInitialize(unit), "initialize");
        source.impulse = true;
        double energy = 0;
        for (int block = 0; block < 100; ++block) {
            buffers->mNumberBuffers = 64;
            for (int c = 0; c < 64; ++c)
                buffers->mBuffers[c] = {1, 512 * sizeof(float), output.data() + c * 512};
            AudioTimeStamp time {};
            time.mFlags = kAudioTimeStampSampleTimeValid;
            time.mSampleTime = block * 512;
            AudioUnitRenderActionFlags flags = 0;
            check(AudioUnitRender(unit, &flags, &time, 0, 512, buffers), "render");
            for (float value : output) {
                if (!std::isfinite(value)) return std::fprintf(stderr, "Nonfinite audio\n"), 1;
                energy += double(value) * value;
            }
        }
        if (energy <= 0) return std::fprintf(stderr, "Silent impulse response\n"), 1;
        std::printf("PASS: 64 channels, %.0f Hz, channel-64 impulse, finite nonzero response (energy %.8g)\n", rate, energy);
        check(AudioUnitUninitialize(unit), "uninitialize");
    }
    check(AudioComponentInstanceDispose(unit), "dispose");
}
