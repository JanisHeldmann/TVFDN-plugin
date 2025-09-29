# TVFDN-plugin
### Real-time audio plugin implementing a Time-Varying Feedback Delay Network.

Please, read carefully the *Interface* section for a description of the graphical user interface and to avoid undesired/dangerous behaviour of the plugin.

---

## Description

The plugin is an implementation of a time-varying feedback delay network (TVFDN) for real-time multichannel audio processing[1].

![](imgs/digital_circuit.png)

The TVFDN has the following features:
- Order 64 &rarr; it receives 64 input channels and it produces 64 output channels
- Time-varying feedback matrix
- Delay lines, absorption filters, and time variance are parametrized

The time variation of the feedback matrix produces an oscillation of the TVFDN poles[2].

The implemention is done with the use of the [JUCE](https://github.com/juce-framework/JUCE.git) framework.

---

## Interface

![](imgs/interface.png)

The graphical user interface includes the following sliders and buttons, which affect the TVFDN behaviour:

- RT_DC: reverberation time at 0 Hz
- RT_NY: reverberation time at Nyquist frequency
- RT_CrossOverFrequency: crossover frequency of the reverberation time profile defining the chance between the RT_CD value and the RT_NY value
- Osc_Frequency: affects the frequency with which the poles of the TVFDN oscillate
- Delay_Factor: scalar multiplier applied to the delay lines
- Frequency Spread: affects the oscillation functions defining the oscillation of the poles of the TVFDN, it adds randomness to prevent the synchronization of the poles' movement
- TV Bypassed: when toggled ON, the time variance is bypassed, and the feedback matrix of the FDN is static, thus the FDN itself is static
- Absorption: when toggled ON, the absorption filters are bypassed, thus the TVFDN is lossless

*Please, be careful when toggling the TV Bypassed or the Absorption buttons.*

Toggling the TV Bypassed button ON might induce the reverberation enhancement system into instability.

Toggling the Absorption button ON will make the TVFDN lossless, and thus it will induce the reverberation enhancement system into instability.

---

## Scope

The plugin was designed as a digital signal processing solution for reverberation enhancement systems[3].

---

## License

This project is provided under the GPLv3 License - see the [LICENSE](LICENSE) file for details.

---

## References

[1] Schlecht, Sebastian J., and Emanuël AP Habets. "Time-varying feedback matrices in feedback delay networks and their application in artificial reverberation." The Journal of the Acoustical Society of America 138, no. 3 (2015): 1389-1398.

[2] Schlecht, Sebastian J., and Emanuël AP Habets. "Practical considerations of time-varying feedback delay networks." In Audio Engineering Society Convention 138. Audio Engineering Society, 2015.

[3] Schlecht, Sebastian J., and Emanuël AP Habets. "Reverberation enhancement systems with time-varying mixing matrices." In 59th International Conference of the Audio Engineering Society. 2015.
