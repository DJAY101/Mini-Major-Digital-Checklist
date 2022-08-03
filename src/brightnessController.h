#pragma once
#include <Arduino.h>
class BrightnessController{
    public:

        BrightnessController(int LEDpin, float duration) : m_lengthTillDim{duration}, m_ledPin{LEDpin} {m_timerH = timerBegin(0, 80, true); ledcAttachPin(LEDpin, 0); ledcSetup(0, 1000, 8);};
        void update() {updateBrightness(); };
        void updateBrightness() {if (timerReadMilis(m_timerH) > m_lengthTillDim) {ledcWrite(0, 25);} else {ledcWrite(0, 255);}}
        void interacted() {timerWrite(m_timerH, 0);} //called when it has been interacted to reset the timer

    private:

        float m_lengthTillDim;
        int m_ledPin;
        float m_timer{0};
        float deltaTime{0};

        hw_timer_t* m_timerH;



};