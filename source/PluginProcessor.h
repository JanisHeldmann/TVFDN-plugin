/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */

#pragma once

#include <JuceHeader.h>
#include <typeinfo>
#include "Matrices64.h"

using namespace juce;
using namespace std::complex_literals;

//==============================================================================
/**
 */

class AbsorptionFilters
{
public:
    
    size_t N = 64;
    float fs{48000};
    
    float RT_DC{1.f};
    float RT_NY{1.f};
    float delayFactor{1.f};
    float crossover_frequency{1000.f};
    
    dsp::Matrix<float> DELAYS{N,1};
        
    std::vector<dsp::IIR::Filter<float>> iir_filters;
     
    std::array<float, 4> coeffs{1.f,0.f,1.f,0.f};
    dsp::Matrix<float> filtOutput{1,N};
 
    AbsorptionFilters(dsp::Matrix<float> _DELAYS)
    {
        DELAYS = _DELAYS;
        
        // IIR::Filter cannot be copied
        std::vector<dsp::IIR::Filter<float>> tmp(N);
        iir_filters.swap(tmp);
    };
    
    float RT602slope(float RT60,float fs){
        return -60.f/(RT60*fs);
    }
    
    float db2mag(float ydb){
        return pow(10.f,ydb/20.f);
    }
    
    void updateFirstOrderFilter(float _RT_DC, float _RT_NY, float _crossover_frequency, float _delayFactor){
        if ( _RT_DC != RT_DC || _RT_NY != RT_NY || _crossover_frequency != crossover_frequency || _delayFactor != delayFactor){
            
            RT_DC = _RT_DC;
            RT_NY = _RT_NY;
            delayFactor = _delayFactor;
            crossover_frequency = _crossover_frequency;
            
            // too high cross-over frequency leads to instable filter; fs/4 is the limit
            if(crossover_frequency > fs/5){
                crossover_frequency = fs/5;
            }
            if(crossover_frequency < 500.f){
                crossover_frequency = 500.f;
            }
            
            float omega = crossover_frequency / fs * 2* MathConstants<float>::pi;
            
            for(int j = 0; j < N ; j++){
                float HDc = db2mag( delayFactor * DELAYS(j,0) * RT602slope( RT_DC, fs ) );
                float HNyq = db2mag( delayFactor * DELAYS(j,0) * RT602slope( RT_NY, fs ) );
            
                float t = tan(omega);
                float k = sqrt(HDc / HNyq);
                        
                float b0 = (t * k + 1) * HNyq;
                float b1  = (t * k - 1) * HNyq ;
                float a0  = t / k + 1;
                float a1  = t / k - 1;
                
                coeffs[0] = b0;
                coeffs[1] = b1;
                coeffs[2] = a0;
                coeffs[3] = a1;
                *iir_filters[j].coefficients = coeffs;
            }
        }
    }
    
    
    void prepare(const dsp::ProcessSpec& filterSpec){
        
        fs = filterSpec.sampleRate;
        
        updateFirstOrderFilter(2.f,2.f,1000.f,1.f); // dummy call
        
        for (int i = 0; i < N; i++){
            iir_filters[i].reset(); // reset after filter order changed
            iir_filters[i].prepare(filterSpec);
        }
    };
    
    dsp::Matrix<float> filt(dsp::Matrix<float> filtInput)
    {
        for (int i = 0; i < N; i++){
            filtOutput(0,i) = iir_filters[i].processSample(filtInput(0,i));
        }
        return filtOutput;
    };
};


class Delays
{
public:
    size_t N = 64;
    
    dsp::Matrix<float> DELAYS{N,1};
    std::vector< dsp::DelayLine<float, dsp::DelayLineInterpolationTypes::None> > DelayLines {};
    dsp::Matrix<float> delayOutput{1,N};
    float delayFactor{1};
    
    Delays(dsp::Matrix<float> _DELAYS)
    {
        DELAYS = _DELAYS;
        
        // IIR::DelayLine cannot be copied
        std::vector< dsp::DelayLine<float, dsp::DelayLineInterpolationTypes::None> > tmp(N);
        DelayLines.swap(tmp);
        
        for(int j = 0; j < N;j++){
            DelayLines[j].setMaximumDelayInSamples(192000);
        }
    }
    
    
    dsp::Matrix<float> popSamples()
    {
        for(int j = 0; j < N;j++)
        {
            delayOutput(0,j) =  DelayLines[j].popSample(0);
        }
        return delayOutput;
    }
    
    void pushSamples(  dsp::Matrix<float> _InDelays)
    {
        for(int j = 0; j < N;j++)
        {
            DelayLines[j].pushSample(0, _InDelays(0,j));
        }
    }
    
    void prepare(const dsp::ProcessSpec& Spec){
        for(int j = 0; j < N; j++)
        {
            DelayLines[j].reset();
            DelayLines[j].prepare(Spec);
            DelayLines[j].setDelay(DELAYS(j,0));
        }
    }
    
    void updateDelayFactor(float _delayFactor){
        if ( _delayFactor != delayFactor )
        {
            delayFactor = _delayFactor;
            for(int j = 0; j < N; j++)
            {
                DelayLines[j].setDelay(delayFactor * DELAYS(j,0));
            }
        }
    }
};



class TVmatrix
{
public:
    
    size_t N = 64;
    size_t numberOfOsc = 32;
    float fs{48000};
    
    dsp::FFT fft{6};
    std::vector<float> fftInputOutput;
    dsp::Matrix<float> outputFrame{1,N};
    float real{};
    float imag{};
    double modSaw{};
    float osc_frequency{1.0f};
    float osc_spread{0.1f};
    float E1{1.f};
    float E2{0.f};

    std::vector<dsp::Oscillator< double >> oscArray;
    
    std::vector<float> randSpread { // SEB: I added a 32nd osc
        -0.480259 , 0.600137 , -0.137172 , 0.821295 , -0.636306 , -0.472394 , -0.708922 , -0.727863 , 0.738584 , 0.159409 , 0.099720 , -0.710090 , 0.706062 , 0.244110 , -0.298095 , 0.026499 , -0.196384 , -0.848067 , -0.520168 , -0.753362 , -0.632184 , -0.520095 , -0.165466 , -0.900691 , 0.805432 , 0.889574 , -0.018272 , -0.021495 , -0.324561 , 0.800108 ,  -0.261506,  -0.161506
    };
    
    TVmatrix(size_t _N)
    {
        N = _N;
        numberOfOsc = N/2; // _N/2-1
        fftInputOutput.resize(2*N);
        std::vector<dsp::Oscillator< double >> tmp(numberOfOsc);
        oscArray.swap(tmp);
        
    }
    
    void updateOscFrequency(float _osc_frequency, float _osc_spread){
        if ( _osc_frequency != osc_frequency || _osc_spread != osc_spread ){
            osc_frequency = _osc_frequency;
            osc_spread = _osc_spread;
            
            for (int i = 0; i < numberOfOsc ; i++)
            {
                oscArray[i].setFrequency((randSpread[i]*osc_spread+1)*osc_frequency);
            }
        }
    }
    
    
    void  prepare(const dsp::ProcessSpec& Spec){
        
        fs = Spec.sampleRate;
        
        for (int i = 0; i < numberOfOsc ; i++){
            oscArray[i].prepare(Spec);
            oscArray[i].initialise([](double x) {return x / (2 * MathConstants<float>::pi);});
        }
        
        updateOscFrequency(0.1f,0.1f); // dummy call
    }
    
    //############ oscillation ###################
    
    dsp::Matrix<float> filt( dsp::Matrix<float> inputFrame){
        
        for(int m = 0; m < N; m++){
            fftInputOutput[m]= inputFrame(0,m);
        }
        
        fft.performRealOnlyForwardTransform(fftInputOutput.data(),true);
        

        
        for(int m = 0; m < N + 0; m = m + 2){ // used to be N+2
            E1= 1.f;
            E2= 0.f;
            if (m > 1) {
                modSaw = MathConstants<float>::twoPi * oscArray[m-m/2].processSample(0.5);
                E1=std::cos(modSaw);
                E2=std::sin(modSaw);
            }
            real = fftInputOutput[m]*E1-fftInputOutput[m+1]*E2;
            imag = fftInputOutput[m]*E2+fftInputOutput[m+1]*E1;
            
            fftInputOutput[m] = real;
            fftInputOutput[m+1]= imag;
        }
        
        fft.performRealOnlyInverseTransform(fftInputOutput.data());
        for(int m = 0; m < N; m++){
            outputFrame(0,m) = fftInputOutput[m];
        }
        return outputFrame;
        
    }
    
};


class FDN
{
public:
    
    bool TVBypassed{false};
    bool AbsorptionBypassed{false};
    
    float fs{48000.f};
    size_t N = 64;
    size_t  MyNumberOfInputs = N;
    size_t  MyNumberOfOutputs = N;
    size_t  BufferSize = 1;
    
    float RT_DC{1.5f};
    float RT_NY{0.5f};
    float RT_CrossOverFrequency{1000.f};
    float osc_frequency{1.f};
    float spread{0.5f};
    float delayFactor{1.f};

//    signal frames
    dsp::Matrix<float> InDelays{1,N};
    dsp::Matrix<float> InSamples{1,MyNumberOfInputs};
    dsp::Matrix<float> OutSamples{1,MyNumberOfOutputs};
    dsp::Matrix<float> DelayOutput{1,N};
    dsp::Matrix<float> filtInput{1,N};
    dsp::Matrix<float> filtOutput{1,N};
    dsp::Matrix<float> DelayOutputFilt{1,N};
    dsp::Matrix<float> feedback{1,N};
    dsp::Matrix<float> feedbackTV{1,N};

    // ###############  FDN parameters ###############
    
    ImportedMatrices Matrices;
    dsp::Matrix<float> DELAYS {N,1,Matrices.delays.getRawDataPointer()};
    dsp::Matrix<float> Directs{MyNumberOfInputs,MyNumberOfOutputs,Matrices.directs.getRawDataPointer()};
    dsp::Matrix<float> InGains{MyNumberOfInputs,N,Matrices.inGains.getRawDataPointer()};
    dsp::Matrix<float> OutGains{N,MyNumberOfOutputs,Matrices.outGains.getRawDataPointer()};
    dsp::Matrix<float> feedbackMatrix{N,N,Matrices.feedbackMatrixValues.getRawDataPointer()};
    dsp::Matrix<float> feedbackMatrixTransposed{N,N,Matrices.feedbackMatrixValuesTransposed.getRawDataPointer()};
    
    Delays delays;
    AbsorptionFilters absorptionFilters;
    TVmatrix tvMatrix;
   
    
    //################### METHODS ##################
    FDN() : delays(DELAYS) , absorptionFilters(DELAYS) , tvMatrix(N)
    {
        
    };

    //    ################## PREPARE FUNCTION ##################
    
    void prepare(const dsp::ProcessSpec& Spec,const dsp::ProcessSpec& filterSpec  )
    {
        fs = Spec.sampleRate;
        
        InDelays.clear();
        InSamples.clear();
        OutSamples.clear();
        DelayOutput.clear();
        
        delays.prepare(Spec);
        
        absorptionFilters.prepare(filterSpec);
        
        tvMatrix.prepare(Spec);
    }
    

    //    ################# PROCESS FUNCTION ###################
    
    void process(dsp::AudioBlock<float> block)
    {


        delays.updateDelayFactor(delayFactor);
        
        tvMatrix.updateOscFrequency(osc_frequency,spread);
        
        absorptionFilters.updateFirstOrderFilter(RT_DC,RT_NY,RT_CrossOverFrequency,delayFactor);
        
        // PROCESS
        for(int i = 0; i < block.getNumSamples(); i++) // block version with fix 256 size
        {
            for (int IN = 0; IN < MyNumberOfInputs; ++IN)
            {
                InSamples(0,IN) = block.getSample(IN, i);
            }
            
            //InDelays = InSamples*InGains;
            InDelays = InSamples;
            DelayOutput = delays.popSamples();
            
            if(AbsorptionBypassed == true)
            {
                DelayOutputFilt = DelayOutput;
            }
            else
            {
                DelayOutputFilt = absorptionFilters.filt(DelayOutput);
            };
            
            
            feedback = DelayOutputFilt;
            
            if(TVBypassed == true)
            {
                //feedback = DelayOutputFilt*feedbackMatrixTransposed;
                feedbackTV = feedback*feedbackMatrixTransposed;
            }
            else
            {
                feedbackTV = tvMatrix.filt(feedback);
            }
            
            InDelays = InDelays+feedbackTV;
            delays.pushSamples(InDelays);
                        
            //OutSamples = InSamples*Directs; //todo take away
            //OutSamples =  OutSamples+(DelayOutput*(OutGains));
            OutSamples = feedbackTV;
            
            for (int OUT = 0; OUT < MyNumberOfOutputs; ++OUT)
            {
                block.setSample(OUT, i, OutSamples(0,OUT));
            }
        }
    }
};


class GlivelabPlugin64AudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    GlivelabPlugin64AudioProcessor();
    ~GlivelabPlugin64AudioProcessor() override;
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif
    
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
    
    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    
    //==============================================================================
    const String getName() const override;
    
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    
    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;
    
    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()};
    
    
private:
    
    int count = 0;
    FDN fdn{};
    
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlivelabPlugin64AudioProcessor)
};

