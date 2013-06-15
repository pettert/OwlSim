//
//  BiasedDelayPatch.hpp
//
//  A basic delay effect with sample bias modulation.
//  This can produce great nonlinear saturation effects, however
//  its behaviour is highly dependent on the audio source.
//
//  Created by martind on 15/06/2013.
//  http://github.com/dekstop/OwlSim
//
//  Parameter assignment:
//  - A: delay time
//  - B: feedback
//  - C: bias: low..high
//  - D: dry/wet mix
//  - Push-button:
//
//  TODO:
//  - see SimpleDelayPatch.hpp
//

#ifndef OwlSim_BiasedDelayPatch_hpp
#define OwlSim_BiasedDelayPatch_hpp

#include "StompBox.h"

class BiasedDelayPatch : public Patch {
  
  const float MIN_DELAY = 0.01; // in seconds
  const float MAX_DELAY = 4;
  
  const float MIN_BIAS = 0.5;
  const float MED_BIAS = 1;
  const float MAX_BIAS = 3;

  float* circularBuffer;
  unsigned int bufferSize;
  unsigned int writeIdx;
  
public:
  void processAudio(AudioInputBuffer &input, AudioOutputBuffer &output){

    double rate = getSampleRate();
    
    if (circularBuffer==NULL)
    {
      bufferSize = MAX_DELAY * rate;
      circularBuffer = new float[bufferSize];
      memset(circularBuffer, 0, bufferSize*sizeof(float));
      writeIdx = 0;
    }

    unsigned int sampleDelay = getSampleDelayParameter(getRampedParameterValue(PARAMETER_A), rate);
    float feedback = getRampedParameterValue(PARAMETER_B);
    float bias = getBias(1 - getRampedParameterValue(PARAMETER_C));
    float dryWetMix = getRampedParameterValue(PARAMETER_D);
    
    int size = input.getSize();
    float* buf = input.getSamples();
    for (int i=0; i<size; ++i)
    {
      float delaySample = circularBuffer[writeIdx];
      float v = buf[i] + circularBuffer[writeIdx] * feedback;
      circularBuffer[writeIdx] = fminf(1, fmaxf(0, applySampleBias(v, bias))); // Guard: hard range limits.
      buf[i] = linearBlend(buf[i], delaySample, dryWetMix);

      writeIdx = (++writeIdx) % sampleDelay;
    }
    output.setSamples(buf);
  }
  
  ~BiasedDelayPatch(){
    free(circularBuffer);
  }
  
private:
  
  unsigned int getSampleDelayParameter(float p1, float rate){
    return (MIN_DELAY + p1 * (MAX_DELAY-MIN_DELAY)) * rate;
  }

  // Mapping p1 parameter ranges so that:
  // - full-left (0) is "low bias"
  // - centre (0.5) is "no bias"
  // - full-right (1.0) is "high bias"
  float getBias(float p1){
    if (p1 < 0.5)
    { // min .. med
      p1 = p1 * 2; // [0..1] range
      return p1*p1 * (MED_BIAS-MIN_BIAS) + MIN_BIAS;
    } else
    { // med .. max
      p1 = (p1 - 0.5) * 2; // [0..1] range
      return p1*p1 * (MAX_BIAS-MED_BIAS) + MED_BIAS;
    }
  }
  
  float applySampleBias(float v, float bias){
    return
      powf(fabs(v), bias) * // bias
      (v < 0 ? -1 : 1);    // sign
  }
  
  // Fade from a to b, over mix range [0..1]
  float linearBlend(float a, float b, float mix){
    return a * (1 - mix) + b * mix;
  }

  // Parameter ramping to reduce clicks.
  
  float oldVal[4] = {0, 0, 0, 0};
  float ramp = 0.1; // 0..1
  
  float getRampedParameterValue(PatchParameterId id){
    float val = getParameterValue(id);
    float result = val * ramp + oldVal[id] * (1-ramp);
    oldVal[id] = val;
    return result;
  }
};

#endif // OwlSim_SimpleBiasedDelayPatch_hpp