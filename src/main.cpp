#include <Arduino.h>

#include <Arduino_GFX_Library.h>
#include <CFRotaryEncoder.h>

#include <WiFi.h>
#include <Preferences.h>

#include <vector>


#include <Task.h>
#include <TaskRenderer.h>

#include <brightnessController.h>

//Wifi Name & Password
const char* ssid     = "Digi-Checklist";
const char* password = "123456789";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

//Encoder Definitions
const int ROT_PIN_OUTPUT_A = 21;
const int ROT_PIN_OUTPUT_B = 19;
const int ROT_PIN_PUSH_BUT = 22;

CFRotaryEncoder rotaryEncoder(ROT_PIN_OUTPUT_A, ROT_PIN_OUTPUT_B, ROT_PIN_PUSH_BUT);                        // Rotary Endoder.

const int displayBrightnessPin = 33;

#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */
#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else /* !defined(DISPLAY_DEV_KIT) */

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = new Arduino_ESP32SPI(17 /* DC */, 5 /* CS */, 18 /* SCK */, 23 /* MOSI */, -1 /* MISO */, VSPI /* spi_num */);

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, 4 /* RST */, 1 /* rotation */, false /* IPS */);

#endif /* !defined(DISPLAY_DEV_KIT) */

/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

BrightnessController brightnessController(displayBrightnessPin, 10000);

TaskRenderer* taskRenderer;
std::vector<Task*>* tasks;

void encoderChanged() {

  if (rotaryEncoder.getValue() != rotaryEncoder.getLastValue()) {
    brightnessController.interacted();
    int value = rotaryEncoder.getValue();
    if (value <= -1) {
      taskRenderer->renderNetworkInfo(password, ssid);
      rotaryEncoder.setValue(-1);
    }
    else if (value > tasks->size()-1) {
      rotaryEncoder.setValue(tasks->size()-1);
      taskRenderer->updateRender(rotaryEncoder.getValue());

    } else if (rotaryEncoder.getValue() == 0 && rotaryEncoder.getLastValue()== -1) {
      taskRenderer->refreshTable();
    }
     else {
      taskRenderer->updateRender(rotaryEncoder.getValue());
    }
  }
}

void buttonClicked() {
  Serial.print("Finished");
  // tasks->at(rotaryEncoder.getValue())->setComplete(!tasks->at(rotaryEncoder.getValue())->getComplete());
  // taskRenderer->taskChangedRender(rotaryEncoder.getValue(), true);
  taskRenderer->allTaskCompletedAnim();
  rotaryEncoder.setValue(0);
}


std::vector<String> splitString(String text, String splitCharacter) {
  std::vector<String> returnVal;

  String parts;
  for (int i = 0; i < text.length(); i++) {
    if (text[i] == splitCharacter[0] && parts.length() > 0) {
      returnVal.emplace_back(parts);
      parts = "";
    } else if(i == text.length()-1 && text[i] != splitCharacter[0]) {
      parts += text[i];
      returnVal.emplace_back(parts);
    } 
    else if(text[i] != splitCharacter[0]) {
      parts += text[i];
    }
  }
  //if there is not splits found then return -1
  if (returnVal.size() == 0) {returnVal.emplace_back("-1");}
  return returnVal;

}


Preferences preferences;
IPAddress IP;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(112500);
  gfx->begin();
  gfx->setTextSize(3, 3, 1);
  
  rotaryEncoder.setAfterRotaryChangeValueCallback(encoderChanged);
  rotaryEncoder.setPushButtonOnPressCallback(buttonClicked);
  rotaryEncoder.setEncoderInvert(true);
  //pinMode(displayBrightnessPin, OUTPUT);
  //digitalWrite(displayBrightnessPin, HIGH);


  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);
  
  IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    
  server.begin();

  preferences.begin("savedData", false); //setup the database to read locally


  tasks = new std::vector<Task*>;

  //grab saved data
  String data = preferences.getString("Data", "");
  //load saved data into vector for tasks
  std::vector<String> dataSplit = splitString(data, ",");
  if (dataSplit[0] != "-1") {
    for (String taskName: dataSplit){
      Task* temp = new Task(taskName);
      tasks->emplace_back(temp);
    }
  }

  taskRenderer = new TaskRenderer(gfx, tasks);
  taskRenderer->init();
}


struct wifiData{
  std::vector<Task*>* tasks;
  WiFiClient* client;
  TaskRenderer* taskRenderer;

};



void decodeHTMLString(String* text) {
  text->replace("%20", " ");
}

void WifiLoopCode(void * pvParameters) {

  
  WiFiClient m_client = *(((wifiData*)pvParameters)->client);
  std::vector<Task*> *m_tasks = ((wifiData*)pvParameters)->tasks;
  TaskRenderer* m_taskRenderer = ((wifiData*)pvParameters)->taskRenderer;

  String GetRequest;
  for(;;) {
      String currentLine;
    while(m_client.connected()) {
      if(m_client.available() > 0) {
          char c = m_client.read();
          header += c;
          Serial.print(c);

          if (c == '\n') {

            if(currentLine.indexOf("GET") != -1) {
              int loopCounter = 0;
              bool notFound = true;
              while(notFound) {
                if (currentLine.substring(4+loopCounter, 5+loopCounter) != " ") {
                  GetRequest += currentLine[4+loopCounter];
                } else {
                  notFound = false;
                }
                loopCounter++;
              }
              Serial.print("Get Request: ");
              Serial.println(GetRequest);
            }

            //after receiving HTML Request and data from client then send new HTML data
            if (currentLine.length() == 0) {
              m_client.println("HTTP/1.1 200 OK");
              m_client.println("Content-type:text/html");
              m_client.println("Connection: close");
              m_client.println();



              decodeHTMLString(&GetRequest);
              std::vector<String> getRequestSplit = splitString(GetRequest, "/");
              if(getRequestSplit.at(0) == "edit") {
                std::vector<String> parts = splitString(GetRequest, "/");

                String currentData = preferences.getString("Data");
                preferences.clear();
                std::vector<String> splitData = splitString(currentData, ",");
                splitData.at(parts.at(1).toInt()) = parts.at(2);
                currentData = "";
                for (auto taskName : splitData) {
                  currentData += "," + taskName;
                }
                preferences.putString("Data", currentData);

                tasks->at(parts.at(1).toInt())->setTask(parts.at(2));
                tasks->at(parts.at(1).toInt())->setComplete((parts.at(3)=="true")? true:false);
                taskRenderer->refreshTable();
              } else if (getRequestSplit.at(0) == "create") {
                String currentData = preferences.getString("Data");

                preferences.clear();
                currentData = currentData + "," + getRequestSplit.at(1);
                preferences.putString("Data", currentData);

                Task* temp = new Task(getRequestSplit.at(1));
                tasks->emplace_back(temp);
                taskRenderer->refreshTable();

              } else if (getRequestSplit.at(0) == "delete") {

                String currentData = preferences.getString("Data");
                preferences.clear();
                std::vector<String> splitData = splitString(currentData, ",");
                splitData.erase(splitData.begin()+getRequestSplit.at(1).toInt());
                currentData="";
                for (auto taskName : splitData) {
                  currentData += "," + taskName;
                }
                preferences.putString("Data", currentData);

                Task* toBeDeletedTask = tasks->at(getRequestSplit.at(1).toInt());
                tasks->erase(tasks->begin()+getRequestSplit.at(1).toInt());
                delete toBeDeletedTask;

                taskRenderer->refreshTable();

              }


              
              m_client.println("<!DOCTYPE html><html>");
              m_client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              m_client.println("<link rel=\"icon\" href=\"data:,\">");
              m_client.println("<div style=\"background-color: rgb(76,57,235);\"><h style=\"color:white;font-size:30px;\"><center> DIGI-DoIT </center></h></div>");
              if(getRequestSplit.at(0) == "-1"){
                Serial.println("Get request is size zero!");
                m_client.println("<script type=\"text/javascript\">");
                m_client.println("location.replace(\"/homePage\");");
                m_client.println("</script>");
              }else if(getRequestSplit.at(0) == "editPage") {
                m_client.println("<a href=\"/homePage\"><h1>Back home!</h1></a>");
                m_client.println("<h1>TASK ID:" + getRequestSplit.at(1) + "</h1>");
                m_client.println("<h1>New task detail</h1>");
                m_client.println("<input id=\"taskInput\" value=\"" + tasks->at(getRequestSplit.at(1).toInt())->getTask() + "\">");
                m_client.println("<input type=\"checkbox\" id=\"taskComplete\" name=\"check1\">");
                m_client.println("<label for=\"check1\">Task Completed</label><br>");
                m_client.println("<h3>Submit</h3>");
                m_client.println("<button id=\"submit\" style=\"font-size = 20px;\">CHANGE</button>");
                m_client.println("<button id=\"deleteB\" style=\"font-size = 20px;\">DELETE</button>");

                //JavaScript
                m_client.println("<script type=\"text/javascript\">");

                m_client.println("function clickAction() {if(taskComplete.checked) {fetch(\"/edit/" + getRequestSplit.at(1) + "/\"+taskInput.value+\"/true\").then(res=>{location.replace(\"/homePage\");})} else {fetch(\"/edit/" + getRequestSplit.at(1) + "/\"+taskInput.value+\"/false\").then(res=>{location.replace(\"/homePage\");})};}");
                m_client.println("submit.onclick = clickAction;");
                m_client.println("function deleteAction() { fetch(\"/delete/"+ getRequestSplit.at(1) +"\").then(res=>{location.replace(\"/homePage\");}); }");
                m_client.println("deleteB.onclick = deleteAction;");

                m_client.println("</script>");

              } else if (getRequestSplit.at(0) == "homePage") {
                m_client.println("<h1>Current Editable Tasks:</h1>");
                for (int i = 0; i < m_tasks->size(); i++) {
                  m_client.println("<h2><a href=\"/editPage/"+ String(i) +"\">" + m_tasks->at(i)->getTask() +"</a></h2>");
                }
                m_client.println("<h1><a href=\"createPage\">Add new task!</a></h1>");

              } else if (getRequestSplit.at(0) == "createPage") {
                m_client.println("<a href=\"/homePage\"><h1>Back home!</h1></a>");
                m_client.println("<h1>Create a new task!</h1>");
                m_client.println("<h2>Enter task details: </h2>");
                m_client.println("<input id=\"taskInput\">");
                m_client.println("<button id=\"submit\" style=\"font-size = 20px;\">Create</button>");

                m_client.println("<script type=\"text/javascript\">");

                m_client.println("function clickAction() {if(taskInput.value.length > 1) {fetch(\"/create/\"+taskInput.value).then(res => {location.replace(\"/homePage\");});} } ");
                m_client.println("submit.onclick = clickAction;");
                m_client.println("</script>");

              }


              // m_client.println("<h1>TASK ID</h1>");
              // m_client.println("<input id=\"taskID\">");
              // m_client.println("<h1>New task detail</h1>");
              // m_client.println("<input id=\"taskInput\">");
              // m_client.println("<h3>Submit</h3>");
              // m_client.println("<button id=\"submit\" style=\"font-size = 20px;\">CHANGE</button>");

              // //JavaScript
              // m_client.println("<script type=\"text/javascript\">");
              // m_client.println("function clickAction() {fetch(\"/edit/\"+taskID.value+\"/\"+taskInput.value).then(res=>{location.replace(\"/Complete!\");})}");
              // m_client.println("submit.onclick = clickAction;");


              // m_client.println("</script>");

              m_client.println();
              Serial.println("Sent HTML Data");



              m_client.stop();
              vTaskDelete(NULL);
            } else {
            currentLine = "";
            }
          } else if (c != '\r') {
            currentLine += c;
          }
      }

    }

    vTaskDelete(NULL);

  }
}




WiFiClient client;
void loop() {
  // put your main code here, to run repeatedly:
  rotaryEncoder.loop();
  brightnessController.update();

  if (xTaskGetHandle("Wifi") == NULL){
     client = server.available();
    if(client && client.connected()) {
      
      Serial.println("Task Called");
      wifiData *m_wifiData = new wifiData;
      m_wifiData->client = &client;
      m_wifiData->tasks = tasks;
      m_wifiData->taskRenderer = taskRenderer;
      xTaskCreate(WifiLoopCode, "Wifi", 10000, (void*)m_wifiData, 1, NULL);
    } 
  }
}


