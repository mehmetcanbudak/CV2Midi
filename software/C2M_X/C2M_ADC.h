#ifndef C2M_ADC_H_
#define C2M_ADC_H_

#include "src/drivers/ADC/C2M_util_ADC.h"
#include "C2M_config.h"

#include <stdint.h>
#include <string.h>

enum ADC_CHANNEL {
  ADC_CHANNEL_1_1,
  ADC_CHANNEL_2_1,
  ADC_CHANNEL_3_1,
  ADC_CHANNEL_4_1,
  ADC_CHANNEL_5_1,
  ADC_CHANNEL_1_2,
  ADC_CHANNEL_2_2,
  ADC_CHANNEL_3_2,
  ADC_CHANNEL_4_2,
  ADC_CHANNEL_5_2,
  ADC_CHANNEL_LAST,
  ADC_CHANNEL_NUM = ADC_CHANNEL_1_2
};

#define DMA_BUF_SIZE 16
#define DMA_NUM_CH 16 // 16 because we need some power of 2 (see SCA_CHANNEL_ID[] below)

namespace C2M {

class ADC {
public:

  static constexpr uint8_t kAdcResolution = 12;
  static constexpr uint32_t kAdcSmoothing = 4;
  static constexpr uint32_t kAdcSmoothBits = 8; // fractional bits for smoothing
  static constexpr uint16_t kDefaultPitchCVScale = SEMITONES << 7;

  // 16 bit has best-case 13 bits useable, but we only want 12 so we discard 4 anyway
  static constexpr uint8_t kAdcScanResolution = 16;
  static constexpr uint8_t kAdcScanAverages = 4; // ... doesn't like to be increased
  static constexpr uint8_t kAdcSamplingSpeed = ADC_HIGH_SPEED_16BITS;
  static constexpr uint8_t kAdcConversionSpeed = ADC_HIGH_SPEED;
  static constexpr uint32_t kAdcValueShift = kAdcSmoothBits;

  struct CalibrationData {
    
    uint16_t offset[ADC_CHANNEL_LAST];
    uint16_t pitch_cv_scale;
  };

  static void Init(CalibrationData *calibration_data);
  static void Init_DMA();
  static void DMA_ISR();
  static void Scan_DMA();

  template <ADC_CHANNEL channel>
  static int32_t value() {
    return calibration_data_->offset[channel] - (smoothed_[channel] >> kAdcValueShift);
  }

  static int32_t value(ADC_CHANNEL channel) {
    return calibration_data_->offset[channel] - (smoothed_[channel] >> kAdcValueShift);
  }

  static uint32_t raw_value(ADC_CHANNEL channel) {
    return raw_[channel] >> kAdcValueShift;
  }

  static uint32_t smoothed_raw_value(ADC_CHANNEL channel) {
    return smoothed_[channel] >> kAdcValueShift;
  }

  static int32_t pitch_value(ADC_CHANNEL channel) {
    return (value(channel) * calibration_data_->pitch_cv_scale) >> 12;
  }

  static int32_t raw_pitch_value(ADC_CHANNEL channel) {
    int32_t value = calibration_data_->offset[channel] - raw_value(channel);
    return (value * calibration_data_->pitch_cv_scale) >> 12;
  }
  
  static void CalibratePitch(int32_t c2, int32_t c4);

private:

  template <ADC_CHANNEL channel>
  static void update(uint32_t value) {
    value = (value  >> (kAdcScanResolution - kAdcResolution)) << kAdcSmoothBits;
    raw_[channel] = value;
    // division should be shift if kAdcSmoothing is power-of-two
    value = (smoothed_[channel] * (kAdcSmoothing - 1) + value) / kAdcSmoothing;
    smoothed_[channel] = value;
  }

  static ::ADC adc_;
  static volatile bool ready_;
  static CalibrationData *calibration_data_;
  
  static uint32_t raw_[ADC_CHANNEL_LAST];
  static uint32_t smoothed_[ADC_CHANNEL_LAST];
  
  /*  
   *   below: channel ids for the ADCx_SCA register: we have 10 inputs, which isn't ideal ... so we pad the array
   *   pitch inputs 1-5: A5 = 0x4C; A4 = 0x4D; A3 = 0x49; A2 = 0x48; A0 = 0x45 
   *   velocity inputs 1-5: A9 = 0x44; A8 = 0x4F; A7 = 0x47; A6 = 0x46; A1 = 0x4E
  */
  
  static constexpr uint16_t SCA_CHANNEL_ID[DMA_NUM_CH] = { 0x4C, 0x4D, 0x49, 0x48, 0x45, 0x44, 0x4F, 0x47, 0x46, 0x4E, 0x4C, 0x4D, 0x49, 0x48, 0x45, 0x44 }; 
};

};

#endif // C2M_ADC_H_
