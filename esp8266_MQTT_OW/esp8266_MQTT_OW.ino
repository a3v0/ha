/*
 Basic ESP8266 MQTT example with one wire temperature added from compostThermo

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#define TRUE 0
#define FALSE 1

const char * NODE_LOC = "shop";
const char * PING = "shop/ping";
const char * T0 = "shop/t0";
const char * T1 = "shop/t1";
const char * IN_TOPIC = "shop/inTopic";

const unsigned int PING_T = 10;

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 5

// function to print a device address
void printAddress(DeviceAddress deviceAddress);

// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Update these with values suitable for your network.
const char* ssid = "xray";
const char* password = "Z3eTB8PrdXkXTCcAtici";
const char* mqtt_server = "192.168.1.67";

WiFiClient espClient;
PubSubClient client(espClient);
long lastTemp = 0;
long lastMsg = 0;
boolean toggle = false;
char msg[50];
int value = 0;

// for oneWire code
float temp0 = 0;
float temp1 = 0;
const int inPin = 5;
int deviceCount;
// arrays to hold device addresses
DeviceAddress insideThermometer, outsideThermometer;


// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		// zero pad the address if necessary
		if (deviceAddress[i] < 16) Serial.print("0");
		Serial.print(deviceAddress[i], HEX);
	}
	Serial.println();
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", NODE_LOC);
      // ... and resubscribe
      client.subscribe(IN_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_oneWire()
{
	deviceCount = sensors.getDeviceCount();
	Serial.print("DeviceCount:");
	Serial.println(deviceCount);

	if (sensors.getAddress(insideThermometer, 0))
		printAddress(insideThermometer);
	else
		Serial.println("Unable to find address for Device 0");

	if (sensors.getAddress(outsideThermometer, 1))
		printAddress(outsideThermometer);
	else
		Serial.println("Unable to find address for Device 1");

	sensors.setResolution(12);
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(inPin, INPUT);
  sensors.begin();
  setup_oneWire();
  lastTemp = 0;
  lastMsg = 0;
  toggle = false;
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  long now = millis();
  /*
  if (now - lastMsg > (PING_T*1000)) {
	  lastMsg = now;
	  ++value;
	  snprintf(msg, 75, PING, " #%ld", value);
	  Serial.print("Publish message: ");
	  Serial.println(msg);
	  client.publish(PING, msg); 
	  toggle = !toggle;
	  digitalWrite(BUILTIN_LED, toggle);
  }
  */
  
  /// oneWire Temperature code 
  if (now - lastTemp > (20*1000))  // 20 seconds
  { 
	  lastTemp = now;

	  sensors.requestTemperatures(); // Send the command to get temperatures
	  temp0 = sensors.getTempCByIndex(0);
	  temp1 = sensors.getTempCByIndex(1);
	  //Serial.println(temp0);
	  //Serial.println(temp1);

	  // If temps are reasonable publish them 

	 // if ((temp > -20) && (temp < 60)) 
	  {
		  for (int t = 0; t < 5; t++)
		  {
			  Serial.print("Publish ");
			  Serial.print("try: ");
			  Serial.print(t);
        Serial.print(" ");
			  Serial.print(NODE_LOC);
			  Serial.print("/_temperature0 ");
			  Serial.print(temp0);

			  if (client.publish(T0, String(temp0).c_str(), TRUE))
        {
				  Serial.println();
          break;
        }
			  Serial.println("Failed");
		  }

       // if ((temp > -20) && (temp < 60)) 
    {
      for (int t = 0; t < 5; t++)
      {
        Serial.print("Publish ");
        Serial.print("try: ");
        Serial.print(t);
        Serial.print(" ");
        Serial.print(NODE_LOC);
        Serial.print("/_temperature1 ");
        Serial.print(temp1);

        if (client.publish(T1, String(temp1).c_str(), TRUE))
        {
          Serial.println();
          break;
        }
        Serial.println("Failed");
      } 
    }

    digitalWrite(BUILTIN_LED, LOW);
    delay(1000);
    digitalWrite(BUILTIN_LED, HIGH );
		  //go to sleep
      /*
		  Serial.println("Sleep\n");
		  //delay(1000);
		  //ESP.deepSleep(900 * 1000000, WAKE_RF_DEFAULT); // 15 minutes
		  ESP.deepSleep(45 * 1000000, WAKE_RF_DEFAULT); // 45 seconds 
      */
	  }
  }

}
