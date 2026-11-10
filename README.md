# TVFDN-plugin
### Real-time audio plugin implementing a Time-Varying Feedback Delay Network.

---

Download the macOS AU and VST3 builds from [Releases](https://github.com/JanisHeldmann/TVFDN-plugin/releases). Legacy VST2 downloads remain available in the v1.0.0-beta release.

See [CHANGELOG.md](CHANGELOG.md) for the version history.

## Automatic builds and releases

[Build plugins](https://github.com/JanisHeldmann/TVFDN-plugin/actions/workflows/plugins.yml) builds Windows x64 VST3 and universal macOS AU/VST3 on pushes to `main` and pull requests. Downloads are available as workflow artifacts for 30 days. Both platforms run offline VST3 layout, channel-64 impulse, and eight-parameter state tests; macOS also runs the AU tests and Apple's validator. This does not replace testing in a Windows DAW or with the physical audio system.

To publish a release, update the CMake version and changelog, commit them, and push a matching stable tag such as `v1.2.0`. After both platforms pass, Actions creates/updates the GitHub release with all three plugin ZIPs, checksums, and notes. The tag must match the CMake version and the exact source commit used for both builds. No release is published from a pull request or an ordinary branch push. Automatic publishing updates the release notes and makes that version the latest stable release.

For an existing release, choose **Actions → Build plugins → Run workflow**, leave the workflow branch as `main`, enter the version tag in **ref**, and select **publish_release**. Workflow tooling is checked out separately so old source tags can be rebuilt without changing the tags. Leaving **publish_release** off produces downloadable artifacts only. Build jobs have read-only repository access; release-write access is limited to the publishing job. macOS downloads remain ad-hoc signed rather than Developer ID signed/notarized.

The source code is available in the `./source/` folder.

## Building VST3 and Audio Unit

The CMake project builds VST3 by default and also AU on macOS. It requires CMake 3.22 or newer, a C++17 toolchain, and a JUCE checkout. JUCE `8.0.17-security` was used for the macOS validation below.

```sh
git clone --depth 1 --branch 8.0.17-security https://github.com/juce-framework/JUCE.git ../JUCE
cmake -S . -B build -DJUCE_SOURCE_DIR=../JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

On macOS, Xcode's command-line tools must be selected. Add `'-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0` to the configure command for a universal build. On Windows, use a Visual Studio C++ developer environment. Windows x64 compilation and offline VST3 checks are covered by GitHub Actions; DAW playback still needs a Windows host check. To select formats explicitly, pass e.g. `-DTVFDN_FORMATS=VST3` at configure time.

Plugins are written under `build/TVFDN_artefacts/Release/`; they are not installed automatically. For local macOS use, ad-hoc sign each bundle with `codesign --force --sign - "path/to/bundle"`, then copy the `.vst3` to `~/Library/Audio/Plug-Ins/VST3/` and the `.component` to `~/Library/Audio/Plug-Ins/Components/`. Restart/rescan the host after installation. Distribution signing/notarization is separate.

These are new VST3/AU builds; the existing VST2 release assets are unchanged. The new plugin identity is **TVFDN 64** (`Tv64` / `Tvfn`), and VST2 replacement is disabled. Existing projects using the legacy VST2 must keep that plugin installed.

### Channels and project state

The processor accepts **64 discrete inputs and 64 discrete outputs only**. Its DSP assumes 64 channels, so mono/stereo and other layouts are rejected. In REAPER, set the track channel count to 64 and check the plugin pin mapping. Stereo monitoring requires explicit output routing/downmixing.

All eight processor parameters are saved/restored with plugin state. The ringing audio tail is not serialized, and projects saved with earlier builds cannot recover parameter values that those builds did not store. The existing DSP preparation runs only on the first `prepareToPlay` call; sample-rate changes within one instance remain outside this change.

### Validation

On macOS, the CMake build includes a native AudioToolbox regression test. It loads the built AU directly without installing it, changes all eight parameters, serializes the AU state, destroys the instance, and verifies the values in a fresh instance:

```sh
ctest --test-dir build -C Release --output-on-failure
```

After installing the AU, Apple's validator and the channel-64 impulse smoke test can be run with:

```sh
auval -v aufx Tv64 Tvfn -strict
clang++ -std=c++17 tests/au-smoke.cpp -framework AudioToolbox -o /tmp/tvfdn-au-smoke
/tmp/tvfdn-au-smoke
```

The smoke test checks finite, nonzero output at 44.1, 48, and 96 kHz. These checks do not validate physical audio routing.

`tests/reaper-state-test.lua` tests VST3 state through a real REAPER project save and a fresh process reload. Run it only in an isolated REAPER instance/configuration, with `TVFDN_TEST_DIR` set to an existing scratch directory and `TVFDN_TEST_PHASE=write` or `read`. The write phase requires an empty project, creates a 64-channel track, changes all eight parameters, and saves `state-test.rpp` plus `expected.tsv`; the read phase must be launched with that project. Both phases write their result to `<phase>.log` and quit the test instance. The isolated configuration must include the built VST3 directory in the platform-specific VST search-path setting (`vstpath_arm64` on Apple Silicon, `vstpath64` on Windows x64). For example, on macOS (substitute absolute paths):

```sh
TVFDN_TEST_DIR=/tmp/tvfdn-test TVFDN_TEST_PHASE=write /Applications/REAPER.app/Contents/MacOS/REAPER -newinst -cfgfile /tmp/tvfdn-test/reaper.ini -new /path/to/TVFDN-plugin/tests/reaper-state-test.lua
TVFDN_TEST_DIR=/tmp/tvfdn-test TVFDN_TEST_PHASE=read /Applications/REAPER.app/Contents/MacOS/REAPER -newinst -cfgfile /tmp/tvfdn-test/reaper.ini /tmp/tvfdn-test/state-test.rpp /path/to/TVFDN-plugin/tests/reaper-state-test.lua
```

Both logs must report `PASS`. The script must not be run in your normal REAPER instance.

---

## Description

The plugin is a Time-Varying Feedback Delay Network (TVFDN) for real-time multichannel audio signal processing[1].

<img src="imgs/digital-circuit.png" alt="digital circuit" width="700"/>

The TVFDN has the following features:
- Order 64 &rarr; it receives 64 input channels and it produces 64 output channels
- Time-varying feedback matrix
- Delay lines, absorption filters, and time variance are parametrized

The time variation of the feedback matrix produces an oscillation of the TVFDN poles[2].

The implemention is done with the use of the [JUCE](https://github.com/juce-framework/JUCE.git) framework.

---

## Interface

<img src="imgs/interface.png" alt="plugin interface" width="600"/>

|       Parameter       |                         Description                        |
|:---------------------:|------------------------------------------------------------|
| RT_DC                 | Reverberation time (in seconds) at direct component. The direct component is 0 Hz. This value is valid for all frequencies from 0 Hz to the crossover frequency |
| RT_NY                 | Reverberation time (in seconds) at Nyquist frequency. The Nyquist frequency is half of the sampling frequency. This value is valid for all frequencies from the crossover frequency to the Nyquist frequency |
| RT_CrossOverFrequency | Crossover frequency (in Hz) for the reverberation time profile over frequency |
| Osc_Frequency         | It is the frequency offset (in Hz) with which the eigenvalues of the feedback matrix of the FDN oscillate in time |
| Delay_Factor          | Scalar value applied to the length of the delay lines of the FDN. Changing it changes the perceived volume of the room that the reverberator is emulating |
| Frequency Spread      | Adds randomization to how the oscillation of the eigenvalues of the feedback matrix of the FDN change in time |
| TV Bypassed           | It activates the bypass of the time variation inside the FDN |
| Absorption            | It activates the bypass of the absorption filters in the FDN. **When toggled on, the FDN becomes lossless**<sup>*</sup>|

<sup>*</sup> The reverberation of a lossless FDN will not decay in time. Please be careful when using this function.

---

## Scope

The plugin was designed as a digital signal processing solution for reverberation enhancement systems[3].

---

## For MacOS users
 
MacOS might block the .vst file with the message that it cannot verify it is free of malware. You can manually allow the use of the .vst file with **Gatekeeper Override**.

However, for this to work reliably, MacOS must attempt to **open the file with an associated application**. A VST plugin is not a standalone app, so you need to temporarily associate it with one.

Follow these steps carefully:

#### **Step 1 — Do NOT Move It to Trash**
When the warning appears, click **“Done”** (not “Move to Trash”).

#### **Step 2 — Assign an Application to the .vst File**

1. Open **Finder**.
2. Navigate to the plugin file
    (typically /Library/Audio/Plug-Ins/VST/ or /Library/Audio/Plug-Ins/VST3/)
3. Right-click the .vst file → **Get Info**
4. In the **Open with:** section:
    - Choose any regular application (for example, a text editor such as TextEdit or VS Code)
    - Do **not** use Terminal
5. Click **Change All…** (optional, but helps ensure the association is applied)
This step ensures that macOS has an application it can attempt to launch when you press “Open Anyway.”

#### **Step 3 — Allow the Plugin in Security Settings**

1. Open **System Settings → Privacy & Security**    
2. Scroll to the **Security** section
3. You should see:
    > “GlivelabPlugin64.vst was blocked because it is not from an identified developer.”
4. Click **“Open Anyway”**
    
Because the file now has an associated application, macOS will actually attempt to open it.

#### **Step 4 — Confirm the Second Dialog**

A new dialog appears asking if you are sure you want to open it.
Click **Open**.

At this point:
- macOS removes the quarantine restriction for this file.
- The plugin is permanently allowed.

#### **Step 5 — Reopen Your DAW**

Now launch your DAW normally.
The plugin should load without further Gatekeeper warnings.

### **Why This Works**

Gatekeeper only completes the override when it performs an actual open attempt.

Since a .vst file is not an executable app bundle, macOS needs a registered application to trigger that process. Assigning any normal application (such as a text editor) enables that verification step.

---

## License

This project is provided under the GPLv3 License - see the [LICENSE](LICENSE) file for details.

---

## References

[1] Schlecht, Sebastian J., and Emanuël AP Habets. "Time-varying feedback matrices in feedback delay networks and their application in artificial reverberation." The Journal of the Acoustical Society of America 138, no. 3 (2015): 1389-1398.

[2] Schlecht, Sebastian J., and Emanuël AP Habets. "Practical considerations of time-varying feedback delay networks." In Audio Engineering Society Convention 138. Audio Engineering Society, 2015.

[3] Schlecht, Sebastian J., and Emanuël AP Habets. "Reverberation enhancement systems with time-varying mixing matrices." In 59th International Conference of the Audio Engineering Society. 2015.
