#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include <vector>

#include "Task.h"
#include <cmath>

#include <bitmap.h>


class CellRender {

    public:
    CellRender(Arduino_GFX* gfx, u_int16_t x, u_int16_t y, String content, int height, int width, bool selected, bool completed);
    void render();
    void clear() {m_gfx->fillRect(m_x, m_y, m_renderWidth, m_renderHeight, m_backgroundColour);};
    void setSelected(bool selected);
    void Set(String content, bool completed, bool selected) {m_content = content; m_completed = completed; m_selected = selected;}



    void drawThickRectangle(u_int16_t x, u_int16_t y, u_int16_t width, u_int16_t height, u_int16_t colour, u_int16_t thickness);
    void drawArrow(u_int16_t x, u_int16_t y, u_int16_t size, u_int16_t colour);
    private:
        Arduino_GFX* m_gfx;
        u_int16_t m_x;
        u_int16_t m_y;
        bool m_selected;
        bool m_completed;
        String m_content;
        int m_renderHeight;
        int m_renderWidth;

        int borderThickness{3};


        u_int16_t m_borderColour{WHITE};
        u_int16_t m_completedColour{GREEN};
        u_int16_t m_selectedBorderColour{RED};
        u_int16_t m_arrowColour{RED};
        u_int16_t m_backgroundColour{BLACK};


};

class TaskRenderer {

    public:
    TaskRenderer(Arduino_GFX* gfx, std::vector<Task*>* tasks);
    
    void init();
    
    void updateRender(int selectedElement);
    void taskChangedRender(int taskElement, bool selected);

    void checkPreviousTask();
    bool allTasksComplete();
    void allTaskCompletedAnim();

    void refreshTable();

    private:

    int m_selectedElement{0};
    Arduino_GFX* m_gfx{nullptr};

    std::vector<Task*>* m_tasks{nullptr};
    std::vector<int> m_currentElementsRendered;
    std::vector<CellRender*> m_cells;

    uint16_t m_borderColour{WHITE};
    uint16_t m_completedColour{GREEN};
    uint16_t m_selectedBorderColour{RED};

    int m_taskRenderHeight{105};
    int m_displayHeight{320};
    int m_displayWidth{480};

    int m_scrollOffset{0};

    float m_animDeltaTime{0};

    // 'TickIcon', 480x320px

};
