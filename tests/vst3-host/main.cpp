#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    try {
        juce::ScopedJuceInitialiser_GUI initialise;
        const auto path = juce::SystemStats::getEnvironmentVariable("TVFDN_VST3_PATH", "");
        require(juce::File(path).exists(), "Set TVFDN_VST3_PATH to the built plugin bundle");
        juce::VST3PluginFormatHeadless format;
        juce::OwnedArray<juce::PluginDescription> descriptions;
        format.findAllTypesForFile(descriptions, path);
        require(descriptions.size() == 1, "Expected one VST3 plugin");
        auto create = [&] {
            juce::String error;
            auto plugin = format.createInstanceFromDescription(*descriptions[0], 48000, 512, error);
            if (!plugin) throw std::runtime_error(error.toStdString());
            return plugin;
        };
        auto plugin = create();
        auto layout = plugin->getBusesLayout();
        layout.inputBuses.set(0, juce::AudioChannelSet::discreteChannels(64));
        layout.outputBuses.set(0, juce::AudioChannelSet::discreteChannels(64));
        require(plugin->setBusesLayout(layout), "64-channel layout rejected");
        auto stereo = layout;
        stereo.inputBuses.set(0, juce::AudioChannelSet::stereo());
        stereo.outputBuses.set(0, juce::AudioChannelSet::stereo());
        require(!plugin->setBusesLayout(stereo), "Unsafe stereo layout accepted");
        plugin->setRateAndBufferSizeDetails(48000, 512);
        plugin->prepareToPlay(48000, 512);
        juce::AudioBuffer<float> audio(64, 512);
        juce::MidiBuffer midi;
        double energy = 0;
        for (int block = 0; block < 100; ++block) {
            audio.clear();
            if (block == 0) audio.setSample(63, 0, 0.1f);
            plugin->processBlock(audio, midi);
            for (int channel = 0; channel < 64; ++channel)
                for (int sample = 0; sample < 512; ++sample) {
                    const auto value = audio.getSample(channel, sample);
                    require(std::isfinite(value), "Nonfinite VST3 audio");
                    energy += double(value) * value;
                }
        }
        require(energy > 0, "Silent channel-64 impulse response");
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
        audio.clear();
        plugin->processBlock(audio, midi); // Deliver VST3 parameter changes to the processor.
        juce::MemoryBlock state;
        plugin->getStateInformation(state);
        require(state.getSize() > 0, "Empty VST3 state");
        plugin->releaseResources();
        plugin.reset();
        auto restored = create();
        restored->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        for (const auto& item : expected)
            require(std::abs(restored->getParameters()[item.first]->getValue() - item.second) < 0.0001f,
                "VST3 parameter not restored in fresh instance");
        std::printf("PASS: VST3 64-in/64-out; stereo rejected; channel-64 impulse energy %.9g; eight parameters restored\n", energy);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
