#pragma once
#if defined(ARDUINO_ARCH_RENESAS)  
#include "AudioPWM/PWMAudioBase.h"
#include "AudioTimer/AudioTimer.h"
#include "pwm.h"


namespace audio_tools {

// forward declaration
class PWMDriverRenesas;
/**
 * @typedef  DriverPWMBase
 * @brief Please use DriverPWMBase!
 */
using PWMDriver = PWMDriverRenesas;


/**
 * @brief Audio output to PWM pins for Renesas based Arduino implementations
 * @ingroup platform
 * @author Phil Schatzmann
 * @copyright GPLv3
 */

class PWMDriverRenesas : public DriverPWMBase {

    public:

        PWMDriverRenesas(){
            LOGD("PWMDriverRenesas");
        }

        virtual PWMConfig defaultConfig() {
            PWMConfig cfg;
            Pins pwm_pins;
            // add default pins
            for (int j=2;j<13;j+=2)   pwm_pins.push_back(j);
            cfg.setPins(pwm_pins);
            return cfg;
        }

        // Ends the output
        virtual void end() override {
            TRACED();
            ticker.end(); // it does not hurt to call this even if it has not been started
            is_timer_started = false;

            // stop and release pins
            for (int j=0;j<audio_config.channels;j++){
                if (pins[j]!=nullptr){
                    pins[j]->suspend();
                    pins[j]->end(); 
                    delete pins[j];
                    pins[j] = nullptr;
                }
            }
            pins.clear();
            //pins.shrink_to_fit();
        }


    protected:
        Vector<PwmOut*> pins;      
        TimerAlarmRepeating ticker; // calls a callback repeatedly with a timeout

        /// when we get the first write -> we activate the timer to start with the output of data
        virtual void startTimer() override {
            TRACED();
            if (!is_timer_started){
                TRACED();
                ticker.setCallbackParameter(this);
                ticker.begin(defaultPWMAudioOutputCallback, audio_config.sample_rate, HZ);
                is_timer_started = true;
            }
        }

        /// Setup PWM Pins
        virtual void setupPWM(){
            TRACED();
            pins.resize(audio_config.channels);
            for (int j=0;j<audio_config.channels;j++){
                LOGD("Processing channel %d", j);
                auto gpio = audio_config.pins()[j];
                PwmOut* pin = new PwmOut(gpio);
                LOGI("PWM Pin: %d", gpio);
                pin->begin(50, 0);  // 50: 20000hz at 0%
                pins[j] = pin;
            }
        }

        /// not used -> see startTimer();
        virtual void setupTimer() {
        } 

        virtual int maxChannels() {
            return PIN_PWM_COUNT;
        };

        /// provides the max value for the configured resulution
        virtual int maxOutputValue(){
            return 100; // percent
        }
        
        /// write a pwm value to the indicated channel. The max value depends on the resolution
        virtual void pwmWrite(int channel, int value){
            pins[channel]->pulse_perc(value);
            //char buffer[80];
            //sprintf(buffer,"channel %d - value: %d", channel, value);
            //LOGD("%s", buffer);
        }

        /// timer callback: write the next frame to the pins
        static void  defaultPWMAudioOutputCallback(void *obj) {
            PWMDriverRenesas* accessAudioPWM = (PWMDriverRenesas*) obj;
            if (accessAudioPWM!=nullptr){
                accessAudioPWM->playNextFrame();
            }
        }

};


} // Namespace


#endif
