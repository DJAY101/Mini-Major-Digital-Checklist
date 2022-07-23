#include <TaskRenderer.h>



CellRender::CellRender(Arduino_GFX* gfx, u_int16_t x, u_int16_t y, String content, int height, int width, bool selected, bool completed) {
    m_gfx = gfx;
    m_x = x;
    m_y = y;
    m_selected = selected;
    m_completed = completed;
    m_content = content;
    m_renderHeight = height;
    m_renderWidth = width;
}

void CellRender::setSelected(bool selected) {
    m_selected = selected;
    if (m_selected) {
        drawThickRectangle(m_x, m_y, m_renderWidth, m_renderHeight, m_selectedBorderColour, borderThickness);
        drawArrow(m_renderWidth-40, m_y+(m_renderHeight/2), 50, m_arrowColour);
    } else {
        drawThickRectangle(m_x, m_y, m_renderWidth, m_renderHeight, m_borderColour, borderThickness);
        drawArrow(m_renderWidth-40, m_y+(m_renderHeight/2), 50, (m_completed) ? m_completedColour : m_backgroundColour);
    }
}

void CellRender::render() {
    if (m_completed) {
        m_gfx->fillRect(m_x, m_y, m_renderWidth, m_renderHeight, m_completedColour);
    }
    if (m_selected) {
        drawThickRectangle(m_x, m_y, m_renderWidth, m_renderHeight, m_selectedBorderColour, borderThickness);
        drawArrow(m_renderWidth-40, m_y+(m_renderHeight/2), 50, m_arrowColour);
    } else {
        drawThickRectangle(m_x, m_y, m_renderWidth, m_renderHeight, m_borderColour, borderThickness);
        drawArrow(m_renderWidth-40, m_y+(m_renderHeight/2), 50, (m_completed) ? m_completedColour : m_backgroundColour);
    }
    m_gfx->setCursor(10, m_y + (m_renderHeight/4));
    m_gfx->println(m_content);
}


void CellRender::drawThickRectangle(u_int16_t x, u_int16_t y, u_int16_t width, u_int16_t height, u_int16_t colour, u_int16_t thickness) {

    for (int i = 0; i < thickness; i++) {
        m_gfx->drawRect(x+i, y+i, width-(2*i), height-(2*i), colour);
    }

}

void CellRender::drawArrow(u_int16_t x, u_int16_t y, u_int16_t size, u_int16_t colour) {
    m_gfx->fillRect(x-size, y-(size/4), size, size/2, colour);
    m_gfx->fillTriangle(x-size, y-(size/2), x-(size), y+(size/2), x-(1.8*size), y, colour);
}




TaskRenderer::TaskRenderer(Arduino_GFX* gfx, std::vector<Task*>* tasks){
    m_gfx = gfx;
    m_tasks = tasks;
}

void TaskRenderer::init() {
    m_gfx->fillScreen(BLACK);
    for(int i = 0; i < (m_displayHeight/m_taskRenderHeight); i++) {
        if (i < m_tasks->size()){
            CellRender* temp = new CellRender(m_gfx, 0, m_taskRenderHeight*i, m_tasks->at(i)->getTask(), m_taskRenderHeight, m_displayWidth, (i==0) ? true : false, m_tasks->at(i)->getComplete());
            temp->render();
            m_cells.emplace_back(temp);
            m_currentElementsRendered.emplace_back(i);
        } else break;
    }
}

void TaskRenderer::taskChangedRender(int taskElement, bool selected) {

    int element = std::find(m_currentElementsRendered.begin(), m_currentElementsRendered.end(), taskElement) - m_currentElementsRendered.begin();
    m_cells.at(element)->clear();
    m_cells.at(element)->Set(m_tasks->at(taskElement)->getTask(), m_tasks->at(taskElement)->getComplete(), selected);
    m_cells.at(element)->render();

}

void TaskRenderer::refreshTable() { 
    //updates the current table with new values or state
    m_currentElementsRendered.clear();
    int tempCounter{0};
    for (auto cell : m_cells) {
        taskChangedRender(m_scrollOffset+tempCounter, (m_scrollOffset+tempCounter == m_selectedElement)?true:false);
        m_currentElementsRendered.emplace_back(m_scrollOffset+tempCounter);
        tempCounter++;
    }
}

void TaskRenderer::updateRender(int selectedElement) {

    if (std::find(m_currentElementsRendered.begin(), m_currentElementsRendered.end(), selectedElement) != m_currentElementsRendered.end()) {
        int element;
        //remove old arrow
        element = std::find(m_currentElementsRendered.begin(), m_currentElementsRendered.end(), m_selectedElement) - m_currentElementsRendered.begin();
        m_cells.at(element)->setSelected(false);
        //add new selected arrow
        element = std::find(m_currentElementsRendered.begin(), m_currentElementsRendered.end(), selectedElement) - m_currentElementsRendered.begin();
        m_cells.at(element)->setSelected(true);

        m_selectedElement = selectedElement;
        return;
    } 
    //one steps update
    if (abs(selectedElement-m_selectedElement)==1) {
        m_scrollOffset += selectedElement-m_selectedElement;
        int tempCounter{0};
        m_currentElementsRendered.clear();
        for (auto cell : m_cells) {

            taskChangedRender(m_scrollOffset+tempCounter, (selectedElement-m_selectedElement == 1) ? ((tempCounter==m_cells.size()-1)?true:false) :((tempCounter==0)?true:false));
            m_currentElementsRendered.emplace_back(m_scrollOffset+tempCounter);
            tempCounter++;
        }
        m_selectedElement = selectedElement;
        return;
    }
    //jumping from bottom to top vice versa
    if (abs(selectedElement-m_selectedElement) > 1) {
        m_scrollOffset = (selectedElement-m_selectedElement > 0) ? m_tasks->size()-m_cells.size() : 0;
        int tempCounter{0};
        m_currentElementsRendered.clear();
        for (auto cell : m_cells) {
            taskChangedRender(m_scrollOffset+tempCounter, (selectedElement-m_selectedElement > 0) ? ((tempCounter==m_cells.size()-1)?true:false) : ((tempCounter==0)?true:false));
            m_currentElementsRendered.emplace_back(m_scrollOffset+tempCounter);
            tempCounter++;
        }
        m_selectedElement = selectedElement;
        return;
    }


}


