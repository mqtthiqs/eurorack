// Copyright 2014 Olivier Gillet.
// 
// Author: Olivier Gillet (ol.gillet@gmail.com)
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.

#include "clouds/drivers/codec.h"
#include "clouds/drivers/system.h"
#include "clouds/drivers/version.h"
#include "clouds/drivers/leds.h"
#include "clouds/drivers/switches.h"
#include "clouds/drivers/adc.h"

using namespace clouds;

Leds leds;
Switches switches;
Adc adc;

int counter;

// Default interrupt handlers.
extern "C" {
  void NMI_Handler() { }
  void HardFault_Handler() { while (1); }
  void MemManage_Handler() { while (1); }
  void BusFault_Handler() { while (1); }
  void UsageFault_Handler() { while (1); }
  void SVC_Handler() { }
  void DebugMon_Handler() { }
  void PendSV_Handler() { }

  // called every 1ms
  void SysTick_Handler() {
    counter++;
    // a little LED animation for fun
    for(int i=0; i<4; i++) {
      leds.set_status(i, (counter+i*64) & 255, (counter+i*64+128) & 255);
    }
    leds.Write();
  }
}

// called every time the codec needs one buffer of data
void FillBuffer(Codec::Frame* input, Codec::Frame* output, size_t n) {

  // read the ADC and get the value of the Position pot/CV
  adc.Convert();
  float pos = adc.float_value(ADC_POSITION_POTENTIOMETER_CV);

  // for each sample
  while (n--) {
    // the left output gets attenuated by the position
    output->l = input->l * pos;
    // the right output takes the ring-modulation of the two inputs
    output->r = (input->l * input->r) >> 16;
    // advance the buffers by one position
    output++;
    input++;
  }
}

void Init() {

  // start the timer that calls SysTick_Handler (see system.h)
  System sys;
  sys.Init(true);
  sys.StartTimers();

  // later versions of Clouds have slight hardware difference; this is
  // to differentiate them
  Version version;
  version.Init();
  bool master = !version.revised();

  // initialize the codec at 96kHz, with a buffer size of 32 samples
  Codec codec;
  codec.Init(master, 96000);
  codec.Start(32, &FillBuffer);

  // configure and initialize the internal ADC (for CV and pots) and
  // LED driver.
  leds.Init();
  adc.Init();
}

// this is the function that is called on startup
int main(void) {
  // initialize and configure all the devices
  Init();
  // and then do nothing (the timers will trigger the appropriate functions)
  while (1) {
  }
}
