# TVFDN-plugin
### Real-time audio plugin implementing a Time-Varying Feedback Delay Network.

---

The compiled versions (.dll and .vst) can be downloaded in the *Releases* section.

The source code is available in the `./source/` folder.

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
