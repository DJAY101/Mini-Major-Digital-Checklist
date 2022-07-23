#include <Arduino.h>

#include <Arduino_GFX_Library.h>
#include <CFRotaryEncoder.h>

#include <WiFi.h>

#include <vector>


#include <Task.h>
#include <TaskRenderer.h>

//Wifi Name & Password
const char* ssid     = "Digi-Checklist";
const char* password = "123456789";
String hostname = "CheckList";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

//Encoder Definitions
const int ROT_PIN_OUTPUT_A = 21;
const int ROT_PIN_OUTPUT_B = 19;
const int ROT_PIN_PUSH_BUT = 22;

CFRotaryEncoder rotaryEncoder(ROT_PIN_OUTPUT_A, ROT_PIN_OUTPUT_B, ROT_PIN_PUSH_BUT);                        // Rotary Endoder.


#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */
#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else /* !defined(DISPLAY_DEV_KIT) */

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = new Arduino_ESP32SPI(17 /* DC */, 5 /* CS */, 18 /* SCK */, 23 /* MOSI */, -1 /* MISO */, VSPI /* spi_num */);

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, 4 /* RST */, 3 /* rotation */, false /* IPS */);

#endif /* !defined(DISPLAY_DEV_KIT) */

/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/


TaskRenderer* taskRenderer;
std::vector<Task*>* tasks;

void encoderChanged() {

  if (rotaryEncoder.getValue() != rotaryEncoder.getLastValue()) {
    int value = rotaryEncoder.getValue();
    if (value == -1) {
      rotaryEncoder.setValue(tasks->size()-1);
    }
    else if (value > tasks->size()-1) {
      rotaryEncoder.setValue(0);
    }
 
  taskRenderer->updateRender(rotaryEncoder.getValue());

  }
}

void buttonClicked() {
  Serial.print("Finished");
  tasks->at(rotaryEncoder.getValue())->setComplete(!tasks->at(rotaryEncoder.getValue())->getComplete());
  taskRenderer->taskChangedRender(rotaryEncoder.getValue(), true);
}




void setup() {
  // put your setup code here, to run once:
  Serial.begin(112500);
  gfx->begin();
  gfx->setTextSize(5, 5, 1);

  rotaryEncoder.setAfterRotaryChangeValueCallback(encoderChanged);
  rotaryEncoder.setPushButtonOnPressCallback(buttonClicked);


  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str()); //define hostname
  
  server.begin();


  tasks = new std::vector<Task*>;
  Task* task1 = new Task("Task 1");
  Task* task2 = new Task("Task 2");
  Task* task3 = new Task("Task 3");
  Task* task4 = new Task("Task 4");
  Task* task5 = new Task("Task 5");
  Task* task6 = new Task("Task 6");
  
  task3->setComplete(true);
  tasks->emplace_back(task1);
  tasks->emplace_back(task2);
  tasks->emplace_back(task3);
  tasks->emplace_back(task4);
  tasks->emplace_back(task5);
  tasks->emplace_back(task6);

  taskRenderer = new TaskRenderer(gfx, tasks);
  taskRenderer->init();
}


struct wifiData{
  std::vector<Task*>* tasks;
  WiFiClient* client;
  TaskRenderer* taskRenderer;

};

std::vector<String> splitString(String text, String splitCharacter) {
  std::vector<String> returnVal;

  String parts;
  for (int i = 0; i < text.length(); i++) {
    if (text[i] == splitCharacter[0] && parts.length() > 0) {
      returnVal.emplace_back(parts);
      parts = "";
    } else if(i == text.length()-1) {
      parts += text[i];
      returnVal.emplace_back(parts);
    } 
    else if(text[i] != splitCharacter[0]) {
      parts += text[i];
    }
  }
  return returnVal;

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
              
              m_client.println("<!DOCTYPE html><html>");
              m_client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              m_client.println("<link rel=\"icon\" href=\"data:,\">");
              m_client.println("<div style=\"background-color: rgb(76,57,235);\"><h style=\"color:white;\"><center> DIGI-DoIT </center></h></div>");
              m_client.println("<a href=\"/open\">Hello World!</a>");
              for (int i = 0; i < m_tasks->size(); i++) {
                m_client.println("<h3><a href=/" + String(i) + ">" + m_tasks->at(i)->getTask() +"</a></h3>");
              }
              
              if (GetRequest.length() > 1) {tasks->at(splitString(GetRequest, "/").at(0).toInt())->setComplete(true); taskRenderer->refreshTable(); m_client.println("<h1>YOU OPENNED" + tasks->at(splitString(GetRequest, "/").at(0).toInt())->getTask() + "</h1>");}

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


