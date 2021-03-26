#include <WebPlayer.h>

#define MAX_VOLUME      100
#define DEFAULT_VOLUME  70  // volume level 0-100

#define VS1053_CS       14 
#define VS1053_DCS      12  
#define VS1053_DREQ     27 
#define VS1053_SCK      18
#define VS1053_MOSI     23
#define VS1053_MISO     19

WebPlayer::WebPlayer(const char SSID[], const char passKey[]){
    SPI.begin(VS1053_SCK, VS1053_MISO, VS1053_MOSI);

    this->ssid = (char *)malloc(strlen(SSID));
    this->pass = (char *)malloc(strlen(passKey));
    strcpy(this->ssid, SSID);
    strcpy(this->pass, passKey);

    volume = DEFAULT_VOLUME;

    this->player = new VS1053(VS1053_CS, VS1053_DCS, VS1053_DREQ);
}

void WebPlayer::begin(void){
    this->player->begin();
    this->player->switchToMp3Mode(); // optional, some boards require this
    this->player->setVolume(DEFAULT_VOLUME);
    if(this->player->isChipConnected()){
        Serial.println("MP3 codec initialized.");
    }
    else{
        Serial.println("Could not connect to codec!");
    }
}

void WebPlayer::loop(void){

    // disconnect from station (when changing the station)
    if(this->changeStation){
        Serial.println("Changing station...");
        changeStation = false;
        client.stop();
        vTaskDelay(10);
    }

    // (re)connect
    if(!client.connected()){
        if(!connectToStation()){
            vTaskDelay(100);
        }
    }

    // if we get here and we're still not connected, something went seriously wrong
    // -> take a break, we'll try again later
    if(!client.connected()){
        vTaskDelay(5000);
    }
    
    // read data and send to codec
    if(client.available() > 0){
        uint8_t bytesread = client.read(this->mp3buff, 32);
        player->playChunk(this->mp3buff, bytesread);
        player->setVolume((uint8_t) this->volume);
    }
    else{
        vTaskDelay(100);
        this->errors++;
    }

    if(this->errors > 100000 ){
        ESP.restart();
    }

    // give other threads time to do stuff
    vTaskDelay(1);
}

bool WebPlayer::connectToStation(void){
    if(WiFi.status() != WL_CONNECTED){
        Serial.println("Not connected to WLAN");
        return false;
    }

    char currentHost[64];
    hosts[this->radioStation].toCharArray(currentHost, 64);
    if(client.connect(currentHost, this->ports[this->radioStation])){
        Serial.print(F("Connected to "));
        Serial.println(this->names[this->radioStation]);
        this->client.print(String("GET ") + paths[this->radioStation] + " HTTP/1.1\r\n" +
                "Host: " + this->hosts[this->radioStation] + "\r\n" + 
                "Connection: close\r\n\r\n");
        return true;
    }

    return false;
}

bool WebPlayer::setStation(uint8_t station){
    if((station > NR_STATIONS) || (station < 0)){
        this->radioStation = 0;
        return false;
    }
    this->radioStation = station;
    this->changeStation = true;
    return true;
}

void WebPlayer::nextStation(void){
    this->radioStation++;
    if(this->radioStation > NR_STATIONS){
        this->radioStation = 0;
    }
    this->changeStation = true;
}

void WebPlayer::prevStation(void){
    this->radioStation--;
    if(this->radioStation < 0){
        this->radioStation = NR_STATIONS-1;
    }
    this->changeStation = true;
}

String WebPlayer::getCurrentStation(int * stationNr){
    if((this->radioStation > NR_STATIONS) || (this->radioStation < 0)){
        if(stationNr != NULL){
            * stationNr = -1;
        }
        return "";
    }
    if(stationNr != NULL){
        * stationNr = this->radioStation;
    }
    return this->names[this->radioStation];
}

uint8_t WebPlayer::getVolume(void){
    return (uint8_t)this->volume;
}

void WebPlayer::volumeUp(uint8_t step){
    this->volume += step;
    if(this->volume > MAX_VOLUME){
        this->volume = MAX_VOLUME;
    }
    this->setVolume((uint8_t)this->volume);
}

void WebPlayer::volumeDown(uint8_t step){
    this->volume -= step;
    if(this->volume < 0){
        this->volume = 0;
    }
    this->setVolume((uint8_t)this->volume);
}

void WebPlayer::setVolume(uint8_t vol){
    this->player->setVolume(vol);
}