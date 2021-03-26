#ifndef __WEB_PLAYER__
#define __WEB_PLAYER__

#include <Arduino.h>
#include <VS1053.h>  //https://github.com/baldram/ESP_VS1053_Library
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_wifi.h>

#define NR_STATIONS 2

class WebPlayer {
private:
    // Few Radio Stations
    String names[NR_STATIONS] = { "RADIO 1", "STUBRU BRUUT" };
    String hosts[NR_STATIONS] = { "icecast.vrtcdn.be", "icecast.vrtcdn.be" };
    String paths[NR_STATIONS] = { "/radio1-high.mp3", "/stubru_bruut-high.mp3" };
    int ports[NR_STATIONS] = { 80, 80 };

    char * ssid;            //  your network SSID (name) 
    char * pass;            // your network password

    int errors = 0;

    VS1053 * player;
    WiFiClient client;
    uint8_t mp3buff[32];    // vs1053 likes 32 bytes at a time

    bool changeStation = false;
    int radioStation = 0;
    int volume;

    void setVolume(uint8_t volume);

public:
    // Constructor.  Only sets pin values.  Doesn't touch the chip.  Be sure to call begin()!
    WebPlayer(const char ssid[], const char passkey[]);

    bool connectToStation(void);
    void connectToWIFI(bool reconnect);
    void begin(void);
    bool setStation(uint8_t station=0);
    void nextStation(void);
    void prevStation(void);
    uint8_t getVolume(void);
    void volumeUp(uint8_t step=10);
    void volumeDown(uint8_t step=10);
    String getCurrentStation(int * stationNr = NULL);
    void loop(void);
};


#endif /* __WEB_PLAYER__ */
