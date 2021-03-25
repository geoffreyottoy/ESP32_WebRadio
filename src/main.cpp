    /////////////////////////////////////////////////////////////////
   //         ESP32 Internet Radio Project     v1.1               //
  //       Get the latest version of the code here:              //
 //          http://educ8s.tv/esp32-internet-radio              //
/////////////////////////////////////////////////////////////////

#include <Arduino.h>

#include <VS1053.h>  //https://github.com/baldram/ESP_VS1053_Library
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_wifi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>


#define NR_STATIONS 2

#define VS1053_CS   32 
#define VS1053_DCS  33  
#define VS1053_DREQ 35 
#define VS1053_SCK  18
#define VS1053_MOSI 23
#define VS1053_MISO 19

#define BTN_NEXT    12
#define BTN_PREV    13

#define VOLUME  60 // volume level 0-100

TaskHandle_t PlayerLoop_th;
TaskHandle_t Task2;

int radioStation = 0;

char ssid[] = "Techtile";            //  your network SSID (name) 
char pass[] = "Techtile229";       // your network password

// Few Radio Stations
String host[NR_STATIONS] = { "icecast.vrtcdn.be", "icecast.vrtcdn.be" };
String path[NR_STATIONS] = { "/radio1-high.mp3", "/stubru_bruut-high.mp3" };
int   port[NR_STATIONS] = { 80, 80 };

WiFiClient client;
uint8_t mp3buff[32];   // vs1053 likes 32 bytes at a time

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);

bool changeStation = false;

// LED pins
const int led1 = 2;
const int led2 = 4;

bool connectToStation( int station_no );
void connectToWIFI(bool reconnect);
void initMP3Decoder();

void drawRadioStationName(int id);

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

String outputState(){
  if((radioStation < NR_STATIONS) && (radioStation > -1)){
    return host[radioStation] ;
  }
  return "";
}

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<h4>Output - GPIO 2 - State <span id=\"outputState\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

// MP3 Stream loop
void PlayerLoop( void * pvParameters ){
  Serial.print("PlayerLoop running on core ");
  Serial.println(xPortGetCoreID());

  initMP3Decoder();

  connectToWIFI(false);

  while(true){
    if(changeStation){
      Serial.println("Changing station...");
      changeStation = false;
      client.stop();
    }

    if(!client.connected()){
      connectToWIFI(true);
      connectToStation(radioStation);
    }

    if(!client.connected()){
      vTaskDelay(5000);
    }
      
    if(client.available() > 0){
      uint8_t bytesread = client.read(mp3buff, 32);
      player.playChunk(mp3buff, bytesread);
    }

    vTaskDelay(1);
  }
}

//Task2code: blinks an LED every 700 ms
void Task2code( void * pvParameters ){
  connectToWIFI(false);

  if(WiFi.status() != WL_CONNECTED){
    Serial.println("troubles");
    while(1) vTaskDelay(10);
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
      radioStation = inputMessage.toInt();
      changeStation = true;
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
  SPI.begin(VS1053_SCK, VS1053_MISO, VS1053_MOSI);

  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BTN_PREV), previousButtonInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_NEXT), nextButtonInterrupt, FALLING);

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    PlayerLoop,     /* Task function. */
                    "PlayerLoop",   /* name of task. */
                    10000,          /* Stack size of task */
                    NULL,           /* parameter of the task */
                    5,              /* priority of the task */
                    &PlayerLoop_th, /* Task handle to keep track of created task */
                    0);             /* pin task to core 0 */                  
  delay(500); 

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    2,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
}

void loop() {
  vTaskDelete(NULL);
}

bool connectToStation (int station_no ) {
  if(station_no > NR_STATIONS || station_no < 0){
    return false;
  }

  if(WiFi.status() != WL_CONNECTED){
    return false;
  }

  char currentHost[64];
  host[station_no].toCharArray(currentHost, 64);
  if(client.connect(currentHost, port[station_no])){
    Serial.println("Connected now"); 
    client.print(String("GET ") + path[station_no] + " HTTP/1.1\r\n" +
              "Host: " + host[station_no] + "\r\n" + 
              "Connection: close\r\n\r\n");
    return true;
  }

  return false;
}

void connectToWIFI(bool reconnect){
  if(reconnect){
    if(WiFi.status() != WL_CONNECTED){
      WiFi.reconnect();
    }
  }
  else{
    WiFi.begin(ssid, pass);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
}

void initMP3Decoder()
{
  player.begin();
  player.switchToMp3Mode(); // optional, some boards require this
  player.setVolume(VOLUME);
  if(player.isChipConnected()){
    Serial.println("Connected to codec");
  }
  else{
    Serial.println("Could not connect to codec");
  }
}


void drawRadioStationName(int id)
{
  String command;

}

