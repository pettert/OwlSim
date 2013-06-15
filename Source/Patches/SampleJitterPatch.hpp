//
//  SampleJitterPatch.hpp
//
//  A distortion effect: delays incoming samples by random amounts of time.
//  (Strictly speaking: play back samples taken from a randomly selected point
//  in the near past. With variable near/far selection bias.)
//
//  Created by martind on 13/06/2013.
//  http://github.com/dekstop/OwlSim
//
//  Parameter assignment:
//  - A: max sample delay time
//  - B: near/far sample selection bias
//  - C:
//  - D: dry/wet mix
//  - Push-button:
//
//  TODO:
//  - ...
//

#ifndef OwlSim_SampleJitterPatch_hpp
#define OwlSim_SampleJitterPatch_hpp

#include "StompBox.h"
//#include "juce_Random.h"

class SampleJitterPatch : public Patch {
  
  const float MIN_DELAY = 0.00001; // in seconds
  const float MAX_DELAY = 0.02;
  const float MIN_BIAS = 0.1; // bias acts as exponent on a random delay time, t = rnd^bias
  const float MAX_BIAS = 6;
  
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

    float p1 = getRampedParameterValue(PARAMETER_A);
    float p2 = getRampedParameterValue(PARAMETER_B);
//    float p3 = getRampedParameterValue(PARAMETER_C);
    float p4 = getRampedParameterValue(PARAMETER_D);

    unsigned int maxSampleDelay = rate * (MIN_DELAY + p1*p1 * (MAX_DELAY-MIN_DELAY));
    float bias = MIN_BIAS + p2*p2 * (MAX_BIAS-MIN_BIAS);
    // float cutoff = p3;
    float dryWetMix = p4;
    
    int size = input.getSize();
    float* buf = input.getSamples();
    Random r;
    for (int i=0; i<size; ++i)
    {
      int offset = round(maxSampleDelay * pow(r.nextFloat(), bias));
      int readIdx = writeIdx - offset;
      while (readIdx<0)
        readIdx += bufferSize;

      circularBuffer[writeIdx] = buf[i];
      buf[i] =
        circularBuffer[readIdx] * dryWetMix +
        buf[i] * (1 - dryWetMix);

      writeIdx = (++writeIdx) % bufferSize;
    }
    output.setSamples(buf);
  }
  
  ~SampleJitterPatch(){
    free(circularBuffer);
  }

  
private:
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

#endif // OwlSim_SampleJitterPatch_hpp