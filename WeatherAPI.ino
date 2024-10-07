/*
 * by: D de Vries
 * sources: Emmanuel Odunlade 
 * To use this code you will need to create an account and get an api key
 * Open a browser and go to OpenWeatherMap
 * Press the "Sign up" button and create a free account
 * Once your account is created, you’ll be presented with a dashboard that contains several tabs 
 * Select the API Keys tab and copy your unique Key
 * line 21 and 22 change to your hotspot name and pass (enable compatibility mode on your phone)
 * On line 29 choose your city and country code
 */

////INCLUDE LIBRARIES AND SETTINGS HERE ////

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// Replace with your SSID and password details
char ssid[] = "<Name of Wifi/Hotspot Here>";
char pass[] = "<Password of Wifi/Hotspot Here>";

WiFiClient client;

// Open Weather Map API server name
const char server[] = "api.openweathermap.org";

// Replace the next line to match your city and 2 letter country code
String nameOfCity = "Amsterdam,NL";  //your City here

// Replace the next line with your API Key
String apiKey = "<Add API Key Here>";

String text;

int jsonend = 0;
boolean startJson = false;
int status = WL_IDLE_STATUS;

#define JSON_BUFF_DIMENSION 2500

unsigned long lastConnectionTime = 10 * 60 * 1000;  // last time you connected to the server, in milliseconds
const unsigned long postInterval = 10L * 1000L;     // * 60 * 1000;  // posting interval of 10 minutes (10L * 1000L; 10 seconds delay for testing)

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN D1  // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 16  // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Delay between updates to control the speed of the wave
#define DELAYVAL 16  // Lower value for a faster wave

//// RUN THIS CODE ONLY ONCE ////
void setup() {
  Serial.begin(9600);
  pixels.begin();

  WiFi.begin(ssid, pass);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected");
  printWiFiStatus();
}

////RUN THIS CODE CONTINUOUSLY
void loop() {
  if (millis() - lastConnectionTime > postInterval) {
    lastConnectionTime = millis();
    makehttpRequest();
  }
}

////ADDITIONAL FUNCTIONS THAT CAN BE CALLED UPON
void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void makehttpRequest() {
  Serial.println("Making HTTP request...");
  client.stop();
  if (client.connect(server, 80)) {
    Serial.println("Client connected to API");
    client.println("GET /data/2.5/forecast?q=" + nameOfCity + "&APPID=" + apiKey + "&mode=json&units=metric&cnt=2 HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();

    unsigned long timeout = millis();
    while (client.available() == 0) {
      Serial.println("Client unavailable");
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }

    char c = 0;
    while (client.available()) {
      c = client.read();
      if (c == '{') {
        startJson = true;
        jsonend++;
      }
      if (c == '}') {
        jsonend--;
      }
      if (startJson == true) {
        text += c;
      }
      if (jsonend == 0 && startJson == true) {
        parseJson(text.c_str());
        text = "";
        startJson = false;
      }
    }
  } else {
    Serial.println("Connection failed");
    return;
  }
}

void parseJson(const char* jsonString) {
  Serial.println("Unpacking JSON");
  DynamicJsonDocument doc(4096);  // Adjust buffer size as needed

  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
    Serial.println("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  JsonArray list = doc["list"];
  JsonObject nowT = list[0];
  JsonObject later = list[1];

  String weatherNow = nowT["weather"][0]["description"];
  String weatherLater = later["weather"][0]["description"];

  diffDataAction(weatherNow, weatherLater);
}

void diffDataAction(String nowT, String later) {
  Serial.println("Communicating weather to user");
  Serial.println("Current weather: " + nowT);
  Serial.println("Later weather: " + later);



  // Use a switch statement based on the weather description
  String weatherType;
  if (nowT.indexOf("rain") != -1) {
    weatherType = "rain";
  } else if (nowT.indexOf("snow") != -1) {
    weatherType = "snow";
  } else if (nowT.indexOf("clear") != -1 || nowT.indexOf("sunny") != -1) {
    weatherType = "clear";
  } else if (nowT.indexOf("hail") != -1) {
    weatherType = "hail";
  } else if (nowT.indexOf("clouds") != -1) {
    weatherType = "clouds";
  } else {
    weatherType = "unknown";  // Handle any other cases
  }

  // Handle later weather effects similarly
  String laterWeatherType;
  if (later.indexOf("rain") != -1) {
    laterWeatherType = "rain";
  } else if (later.indexOf("snow") != -1) {
    laterWeatherType = "snow";
  } else if (later.indexOf("clear") != -1 || later.indexOf("sunny") != -1) {
    laterWeatherType = "clear";
  } else if (later.indexOf("hail") != -1) {
    laterWeatherType = "hail";
  } else if (later.indexOf("clouds") != -1) {
    laterWeatherType = "clouds";
  } else {
    laterWeatherType = "unknown";  // Handle any other cases
  }

  // Determine action based on current and later weather
  if (weatherType != "unknown" && laterWeatherType != "unknown") {
    if (weatherType == laterWeatherType) {
      Serial.println("Weather remains the same.");
    } else {
      Serial.println("Weather is changing from " + weatherType + " to " + laterWeatherType);
    }
  }

  Serial.println(laterWeatherType.c_str()[0]);
  // Execute the appropriate light show effect based on the later weather type
  switch (laterWeatherType.c_str()[0]) {
    case 'r':  // Rain
      rainEffect();
      break;
    case 's':  // Snow
      snowEffect();
      break;
    case 'h':  // Hail
      hailEffect();
      break;
    case 'b':  // Hail
      sunnyEffect();
      break;
    case 'c':  // Clear or Clouds
      if (laterWeatherType.indexOf("clear") != -1 || laterWeatherType.indexOf("sunny") != -1) {
        sunnyEffect();
      } else {
        cloudsEffect();
      }
      break;
    default:
      atmosphereEffect();  // For any unknown or unhandled weather
      break;
  }

  pixels.show();
}

void rainEffect() {
  // Blue raindrops falling effect
  uint32_t rainColor = pixels.Color(0, 0, 255);  // Blue
  for (int i = 0; i < NUMPIXELS; i++) {
    if (i % 5 == 0) {  // Raindrop every 5 pixels
      pixels.setPixelColor(i, rainColor);
    } else {
      pixels.setPixelColor(i, 0);  // Turn off the rest
    }
    delay(100);  // Slow falling effect
    pixels.show();
  }
}

void snowEffect() {
  // White sparkling snowflakes effect
  uint32_t snowColor = pixels.Color(255, 255, 255);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(random(NUMPIXELS), snowColor);
    // Random snowflake sparkles
    delay(200);
    // Slower flicker effect for snow
    pixels.show();
  }
}

void hailEffect() {                                  // Bright white fast flashing effect for hail
  uint32_t hailColor = pixels.Color(255, 255, 255);  // White
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, hailColor);  // Flash all pixels
  }
  delay(50);       // Fast flash
  pixels.clear();  // Clear all pixels
  delay(50);       // Delay for the flash effect
  pixels.show();
}

void sunnyEffect() {                                // Bright yellow-orange glow for sunny weather
  uint32_t sunnyColor = pixels.Color(255, 223, 0);  // Yellow-orange
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, sunnyColor);  // Set all pixels to yellow
  }
  delay(100);  // Slight glow delay
  pixels.show();
}

void cloudsEffect() {                    // Soft, pulsing white or gray light to represent varying cloud cover
  for (int i = 0; i < 256; i++) {        // Fading in and out for clouds
    pixels.fill(pixels.Color(i, i, i));  // Shades of gray/white
    pixels.show();
    delay(10);  // Slow pulsing
  }
  for (int i = 255; i >= 0; i--) {
    pixels.fill(pixels.Color(i, i, i));  // Fading back out
    pixels.show();
    delay(10);
  }
}

void atmosphereEffect() {
  pixels.fill(pixels.Color(100, 100, 150));
  pixels.show();
  delay(1000);
}
