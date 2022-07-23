#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>


class Task {
    public:
    Task(String task);
    
    String getTask() {return m_task;}
    bool getComplete() {return m_complete;}

    void setTask(String newTask) {m_task = newTask;}
    void setComplete(bool complete) {m_complete = complete;}

    private:
    String m_task;
    bool m_complete{false};
};