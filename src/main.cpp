    /////////////////////////////////////////////////////////////////
   //         ESP32 Internet Radio Project     v1.1               //
  //       Get the latest version of the code here:              //
 //          http://educ8s.tv/esp32-internet-radio              //
/////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <VUMeter.h>
#include <WebPlayer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define BTN_NEXT    12
#define BTN_PREV    13

TaskHandle_t Player_th;
TaskHandle_t WebUI_th;
TaskHandle_t VUMeter_th;

char ssid[] = "Techtile";            //  your network SSID (name) 
char pass[] = "Techtile229";       // your network password

WebPlayer player(ssid, pass);

void IRAM_ATTR previousButtonInterrupt() {

  /*static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
 
 if (interrupt_time - last_interrupt_time > 200) 
 {
   if(radioStation>0)
    radioStation--;
    else
    radioStation = 3;
 }
 last_interrupt_time = interrupt_time;*/
}

void IRAM_ATTR nextButtonInterrupt() {

  /*static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
 
 if (interrupt_time - last_interrupt_time > 200) 
 {
   if(radioStation<4)
    radioStation++;
    else
    radioStation = 0;
 }
 last_interrupt_time = interrupt_time;*/
}

const char* PARAM_INPUT_1 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "On";
      }
      else { 
        inputChecked = false;
        outputStateM = "Off";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = player.getCurrentStation();
    buttons+= "<h4>Output - GPIO 2 - State <span id=\"outputState\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

// MP3 Stream loop
void PlayerLoop( void * pvParameters ){
  Serial.print("PlayerLoop running on core ");
  Serial.println(xPortGetCoreID());

  player.begin();

  while(true){
    player.loop();
    yield();
  }
}

// VU Meter loop
void VUMeterLoop( void * pvParameters ){
  Serial.print("VUMeterLoop running on core ");
  Serial.println(xPortGetCoreID());

  analogSetWidth(9);

  while(true){
    vuMeter.loop();
    yield();
  }
}

//WebUIcode: blinks an LED every 700 ms
void WebUILoop( void * pvParameters ){
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500);
    Serial.print(".");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      player.setStation(inputMessage.toInt());
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    int radioStation;
    player.getCurrentStation(&radioStation);
    request->send(200, "text/plain", String(radioStation).c_str());
  });

  // Start server
  server.begin();

  while(true) {
    vTaskDelay(1);
    yield();
  }
}

void setup() {
  Serial.begin(115200); 

  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BTN_PREV), previousButtonInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_NEXT), nextButtonInterrupt, FALLING);

  WiFi.begin(ssid, pass);

  //create a task that will be executed in the PlayerLoop() function, with priority 5 and executed on core 0
  xTaskCreatePinnedToCore(
                    PlayerLoop,     /* Task function. */
                    "PlayerLoop",   /* name of task. */
                    10000,          /* Stack size of task */
                    NULL,           /* parameter of the task */
                    5,              /* priority of the task */
                    &Player_th, /* Task handle to keep track of created task */
                    0);             /* pin task to core 0 */                  
  delay(500); 

  //create a task that will be executed in the VUMeterLoop() function, with priority 3 and executed on core 1
  xTaskCreatePinnedToCore(
                    VUMeterLoop,   /* Task function. */
                    "VUMeter",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    3,           /* priority of the task */
                    &VUMeter_th,   /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
  delay(500); 

  //create a task that will be executed in the Task2code() function, with priority 2 and executed on core 1
  xTaskCreatePinnedToCore(
                    WebUILoop,   /* Task function. */
                    "WebUI",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    2,           /* priority of the task */
                    &WebUI_th,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
  delay(500); 
}


void loop() {
  // delete loop task (because we don't use it)
  vTaskDelete(NULL);
}

