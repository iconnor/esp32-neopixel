#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h>
WiFiManager wifiManager;

#ifdef wokwi
#define NEOPIXEL_DIN 15
#define NUMPIXELS 10
#else
#define NEOPIXEL_DIN 2  // Note: D4 on NodeMCU is mapped to GPIO2
#define NUMPIXELS 250 + 52
#endif

#define LED_STRIP "LED Strip"
#include <fauxmoESP.h>

fauxmoESP fauxmo;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_DIN, NEO_GRB + NEO_KHZ800);
char device_name[35];
bool lightState = false;
int loop_delay = 25;

char* get_device_name() {
    if (strlen(device_name) > 0) {
        return device_name;
    }
    uint8_t mac_address[8];
    WiFi.macAddress(mac_address);
    sprintf(device_name, "IMC%02X%02X%02X%02X%02X%02X", mac_address[0], mac_address[1], mac_address[2], mac_address[3],  mac_address[4], mac_address[5], mac_address[6]);
    return device_name;
}

void setupFauxmo() {
  // By default, fauxmoESP creates it's own webserver on the defined port
    // The TCP port must be 80 for gen3 devices (default is 1901)
    // This has to be done before the call to enable()

    fauxmo.addDevice(get_device_name());
    
    fauxmo.setPort(80); // required for gen3 devices
    fauxmo.enable(true);

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
        lightState = state;
        loop_delay = value;
    });
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("Happy Halloween!");

  // Initialize NeoPixel strip
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // Seed random number generator
  randomSeed(analogRead(0));

  #ifdef wokwi
  wifiManager.preloadWiFi("Wokwi-GUEST", "");
  #else
  wifiManager.autoConnect(get_device_name());
  #endif
  Serial.println("Connected to Wifi");
  setupFauxmo();
  ESP.wdtEnable(5000);
}

void loop() {
  fauxmo.handle();
  if (lightState) {
    int flicker;
    uint8_t myRed, myGrn;
    float dimmer = 1.0;  // Change this value to set the overall brightness (0.0 to 1.0)

    for (int i = 0; i < NUMPIXELS; i++) {
      flicker = random(0, 30);  // Flicker intensity, you can change the range
      myRed = max((int)((255 - flicker) * dimmer), 0);
      myGrn = max((int)((30 - flicker) * dimmer), 0);

      // Set pixel color here.
      strip.setPixelColor(i, strip.Color(myRed, myGrn, 0));
    }

    // Refresh the strip
    strip.show();
  } else {
    for (int i = 0; i < NUMPIXELS; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
  }
  delay(loop_delay);  // Delay for effect
  if (fauxmo.getDeviceId(get_device_name()) > -1) {
    // Is there a health check? Maybe restart to work around unresponsive devices?
    ESP.wdtFeed();
  }
  
}