#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

static std::vector<float> render(juce::AudioPluginInstance& plugin, int channels,
                                 bool stereoInput, double& energy)
{
    plugin.setRateAndBufferSizeDetails(48000, 512);
    plugin.prepareToPlay(48000, 512);
    juce::AudioBuffer<float> audio(channels, 512);
    juce::MidiBuffer midi;
    std::vector<float> firstTwo;
    for (int block = 0; block < 150; ++block) {
        const int frames = block % 3 == 0 ? 512 : (block % 3 == 1 ? 64 : 137);
        audio.setSize(channels, frames, false, false, true);
        audio.clear();
        if (stereoInput) {
            if (block == 0) {
                audio.setSample(0, 0, 0.1f);
                audio.setSample(1, 127, -0.05f);
            }
            if (block == 20) audio.setSample(1, 0, 0.07f);
        } else if (block == 0) {
            audio.setSample(63, 0, 0.1f);
        }
        plugin.processBlock(audio, midi);
        for (int sample = 0; sample < frames; ++sample)
            for (int channel = 0; channel < channels; ++channel) {
                const auto value = audio.getSample(channel, sample);
                require(std::isfinite(value), "Nonfinite VST3 audio");
                energy += double(value) * value;
                if (channel < 2) firstTwo.push_back(value);
            }
    }
    require(energy > 0, "Silent impulse response");
    plugin.releaseResources();
    return firstTwo;
}

int main()
{
    try {
        juce::ScopedJuceInitialiser_GUI initialise;
        const auto path = juce::SystemStats::getEnvironmentVariable("TVFDN_VST3_PATH", "");
        const auto referencePath = juce::SystemStats::getEnvironmentVariable("TVFDN_VST3_REFERENCE_PATH", "");
        juce::VST3PluginFormatHeadless format;
        auto create = [&] (const juce::String& bundle) {
            require(juce::File(bundle).exists(), "Plugin bundle not found");
            juce::OwnedArray<juce::PluginDescription> descriptions;
            format.findAllTypesForFile(descriptions, bundle);
            require(descriptions.size() == 1, "Expected one VST3 plugin");
            juce::String error;
            auto plugin = format.createInstanceFromDescription(*descriptions[0], 48000, 512, error);
            if (!plugin) throw std::runtime_error(error.toStdString());
            return plugin;
        };
        auto plugin = create(path);
        const bool stereo = plugin->getName() == "TVFDN Stereo";
        const int channels = stereo ? 2 : 64;
        auto layout = plugin->getBusesLayout();
        const auto channelSet = stereo ? juce::AudioChannelSet::stereo() : juce::AudioChannelSet::discreteChannels(64);
        layout.inputBuses.set(0, channelSet);
        layout.outputBuses.set(0, channelSet);
        require(plugin->setBusesLayout(layout), "Expected channel layout rejected");
        auto unsupported = layout;
        const auto otherChannels = stereo ? juce::AudioChannelSet::discreteChannels(64) : juce::AudioChannelSet::stereo();
        unsupported.inputBuses.set(0, otherChannels);
        unsupported.outputBuses.set(0, otherChannels);
        require(!plugin->setBusesLayout(unsupported), "Unsupported layout accepted");
        unsupported.inputBuses.set(0, juce::AudioChannelSet::mono());
        unsupported.outputBuses.set(0, juce::AudioChannelSet::mono());
        require(!plugin->setBusesLayout(unsupported), "Mono layout accepted");
        unsupported = layout;
        unsupported.outputBuses.set(0, juce::AudioChannelSet::mono());
        require(!plugin->setBusesLayout(unsupported), "Mismatched layout accepted");
        double energy = 0;
        render(*plugin, channels, stereo, energy);

        const juce::StringArray names {"RT_DC", "RT_NY", "RT_CrossOverFrequency", "Osc_Frequency",
            "Delay_Factor", "Frequency Spread", "TV Bypassed", "Absorption Bypassed"};
        std::vector<std::pair<int, float>> expected;
        const auto& parameters = plugin->getParameters();
        for (int index = 0; index < parameters.size(); ++index)
            if (names.contains(parameters[index]->getName(128))) {
                const auto value = parameters[index]->getValue() < 0.5f ? 1.0f : 0.0f;
                parameters[index]->setValueNotifyingHost(value);
                expected.emplace_back(index, value);
            }
        require(expected.size() == 8, "Expected eight processor parameters");
        juce::AudioBuffer<float> audio(channels, 64);
        juce::MidiBuffer midi;
        audio.clear();
        plugin->processBlock(audio, midi); // Deliver VST3 parameter changes to the processor.
        juce::MemoryBlock state;
        plugin->getStateInformation(state);
        require(state.getSize() > 0, "Empty VST3 state");
        plugin.reset();
        auto restored = create(path);
        restored->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        for (const auto& item : expected)
            require(std::abs(restored->getParameters()[item.first]->getValue() - item.second) < 0.0001f,
                "VST3 parameter not restored in fresh instance");
        restored.reset();

        if (stereo) {
            require(!referencePath.isEmpty(), "Set TVFDN_VST3_REFERENCE_PATH to TVFDN 64");
            for (bool bypassed : {false, true}) {
                auto candidate = create(path);
                auto reference = create(referencePath);
                require(reference->getTotalNumInputChannels() == 64, "Reference must have 64 inputs");
                for (auto* instance : {candidate.get(), reference.get()})
                    for (auto* parameter : instance->getParameters())
                        if (parameter->getName(128) == "TV Bypassed")
                            parameter->setValueNotifyingHost(bypassed ? 1.0f : 0.0f);
                double referenceEnergy = 0, stereoEnergy = 0;
                const auto actual = render(*candidate, 2, true, stereoEnergy);
                const auto full = render(*reference, 64, true, referenceEnergy);
                require(actual.size() == full.size(), "Stereo response length mismatch");
                for (size_t i = 0; i < actual.size(); ++i)
                    require(std::abs(actual[i] - full[i]) < 0.000001f,
                            "Stereo differs from channels 1/2 of the 64-channel FDN");
            }
            std::puts("PASS: stereo samples match FDN outputs 1/2 with only inputs 1/2 driven, TV on/off");
        }
        std::printf("PASS: VST3 %d-in/%d-out; unsupported layouts rejected; variable-block impulse energy %.9g; eight parameters restored\n",
                    channels, channels, energy);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
