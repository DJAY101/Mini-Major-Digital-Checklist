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
    //initialise the table renderer
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
    //when a task is changed or a cell needs to be re rendered this gets called
    int element = std::find(m_currentElementsRendered.begin(), m_currentElementsRendered.end(), taskElement) - m_currentElementsRendered.begin();
    m_cells.at(element)->clear();
    if (taskElement < m_tasks->size()){
    m_cells.at(element)->Set(m_tasks->at(taskElement)->getTask(), m_tasks->at(taskElement)->getComplete(), selected);
    m_cells.at(element)->render();
    }

}

void TaskRenderer::refreshTable() { 
    //updates the current table with new values or state
    m_currentElementsRendered.clear();
    int tempCounter{0};
    for (auto cell : m_cells) {
        taskChangedRender(m_scrollOffset*3+tempCounter, (m_scrollOffset*3+tempCounter == m_selectedElement)?true:false);
        m_currentElementsRendered.emplace_back(m_scrollOffset*3+tempCounter);
        tempCounter++;
    }
}

void TaskRenderer::checkPreviousTask() {
    //completes the task previously selected
    if(!m_tasks->at(m_selectedElement)->getComplete()) {
        m_tasks->at(m_selectedElement)->setComplete(true);
        taskChangedRender(m_selectedElement, false);
    }

}

bool TaskRenderer::allTasksComplete() {
    //check if all the tasks are compelted
    for (auto task : *m_tasks){
        if (!task->getComplete()) {
            return false;
        }
    }
    return true;
}

void TaskRenderer::allTaskCompletedAnim() {

    //run the finished all tasks aniamtion
    if (!allTasksComplete()) return;

    m_gfx->fillScreen(BLACK);
    bool animationRunning{true};
    int x_center = m_displayWidth/2;
    int y_center = m_displayHeight/2;
    int outerCircleRad = 120;
    int innerCircleRad = 90;
    float angle = -90;
    float targetAngle = 270;

    float v1{0.469};
    float v2{0.484};
    float v3{0.044};
    float v4{0.03};

    while (animationRunning) {
        m_gfx->drawLine(innerCircleRad*cos(angle*M_PI/180) + x_center, innerCircleRad*sin(angle*M_PI/180) + y_center, outerCircleRad*cos(angle*M_PI/180) + x_center, outerCircleRad*sin(angle*M_PI/180) + y_center, WHITE);
        if (angle < targetAngle) {
            float PC = (angle+abs(angle))/(270+abs(angle)); //percentage complete for bezier curve
            float bezierAnimVal = pow(1-PC, 3)*v1 + 3*PC*(1-PC, 2)*v2 + 3*pow(PC, 2)*(1-PC)*v3 + pow(PC, 3)*v4; //bezier curve for animation time completion
            angle += 0.1*m_animDeltaTime*0.00005;
        } else {animationRunning = false;}
        m_animDeltaTime = millis() - m_animDeltaTime;

    }
    m_gfx->drawBitmap(0, 0, tickIcon, 480, 320, GREEN);

    for (auto task:*m_tasks) {
        task->setComplete(false);
    }

    m_selectedElement = 0;
    m_scrollOffset = 0;
    delay(1000);
    refreshTable();

}



void TaskRenderer::updateRender(int selectedElement) {
    //render a new page
    if (std::find(m_currentElementsRendered.begin(), m_currentElementsRendered.end(), selectedElement) != m_currentElementsRendered.end()) {
        checkPreviousTask();
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
    //one steps update when no new page is required to be rendered
    if (abs(selectedElement-m_selectedElement)==1) {
        checkPreviousTask();
        m_scrollOffset += selectedElement-m_selectedElement;
        int tempCounter{0};
        m_currentElementsRendered.clear();
        for (auto cell : m_cells) {
            
            taskChangedRender(m_scrollOffset*3+tempCounter, (selectedElement-m_selectedElement == -1) ? ((tempCounter==m_cells.size()-1)?true:false) :((tempCounter==0)?true:false));
            m_currentElementsRendered.emplace_back(m_scrollOffset*3+tempCounter);
            tempCounter++;
        }
        m_selectedElement = selectedElement;
        return;
    }
    //jumping from bottom to top vice versa
    // if (abs(selectedElement-m_selectedElement) > 1) {
    //     m_scrollOffset = (selectedElement-m_selectedElement > 0) ? m_tasks->size()-m_cells.size() : 0;
    //     int tempCounter{0};
    //     m_currentElementsRendered.clear();
    //     for (auto cell : m_cells) {
    //         taskChangedRender(m_scrollOffset+tempCounter, (selectedElement-m_selectedElement > 0) ? ((tempCounter==m_cells.size()-1)?true:false) : ((tempCounter==0)?true:false));
    //         m_currentElementsRendered.emplace_back(m_scrollOffset+tempCounter);
    //         tempCounter++;
    //     }
    //     m_selectedElement = selectedElement;
    //     return;
    // }


}


