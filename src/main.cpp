#include <ESP8266MQTTMesh.h>
#include <FS.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Publish dummy temp/humidity values instead of reading from sensor
#define DUMMY_VALUES 1

/*
* Misc constants
*/
// DHT21 sensor pin
const uint8_t PIN_SENSOR = 14;

// Delay between sensor reads
const uint16_t DELAY_SENSOR_READ = 2000;

/*
* FW Config
*/
#define FW_ID           0x01
#define FW_VERSION      "0.1"

/*
* Network/mesh config
*/
const wifi_conn NETWORKS[] = { WIFI_CONN("AgileGateway", "agilegateway", NULL, 0),
                              NULL};
const char*  MESH_PASS = "agileiot_sensor_mesh";

/*
* MQTT config
*/
const char* MQTT_BROKER     = "10.42.0.1";
const int MQTT_PORT         = 1883;
const char* MQTT_TOPIC_OUT  = "sensor_node_out/";
const char* MQTT_TOPIC_IN   = "sensor_node_in/";

/*
* Globals
*/
String DEVICE_ID  = String(ESP.getChipId());

// Called when a MQTT message is received
void mqtt_callback(const char *topic, const char *msg);

// Init mesh object
ESP8266MQTTMesh mesh = ESP8266MQTTMesh::Builder(NETWORKS, MQTT_BROKER, MQTT_PORT)
                       .setVersion(FW_VERSION, FW_ID)
                       .setMeshPassword(MESH_PASS)
                       .setTopic(MQTT_TOPIC_IN, MQTT_TOPIC_OUT)
                       .build();

// Temp/Hum Sensor
DHT_Unified dht(PIN_SENSOR, DHT21);

/**
* Setup
*/
void setup()
{
    Serial.begin(115200);

    Serial.println("----- NEW FW ------");

    // Initialize sensor
    dht.begin();
    delay(1000);

    Serial.println("Starting mesh");

    // Configure mesh and start
    mesh.setCallback(mqtt_callback);
    mesh.begin();
}

/**
* Main loop
*/
void loop()
{
    if(!mesh.connected())
        return;

    long currMillis = millis();

    /*
    * Read DHT21 sensor and publish temp/hum to broker
    */
    #if !DUMMY_VALUES

    static long prevMillisSensor = millis();
    if(millis() - prevMillisSensor >= DELAY_SENSOR_READ)
    {
        sensors_event_t event;
        String temp, hum;

        // Get temperature
        dht.temperature().getEvent(&event);
        if (isnan(event.temperature))
            Serial.println("Error reading temperature!");
        else
            temp = event.temperature;

        // Get humidity
        dht.humidity().getEvent(&event);
        if (isnan(event.relative_humidity))
            Serial.println("Error reading humidity!");
        else
            hum = event.relative_humidity;

        // If both values acquired successfully, prepare json and publish
        if(temp.length() && hum.length())
        {
            String msg = "{ humidity: \"" + hum + "\", temperature: \"" + temp  + "\" }";
            mesh.publish("telemetry/", msg.c_str());
        }

        prevMillisSensor = currMillis;
    }

    #endif

    /*
    * Publish dummy data for testing purposes
    */
    #if DUMMY_VALUES

    static uint8_t
            dummy_temp[] = {23, 24, 26, 27, 26, 24, 25, 23, 22},
            dummy_hum[] = {45, 40, 37, 35, 38, 41, 44, 47, 46};
    static int count = 0;
    static long prevMillisDummy = millis();
    if(millis() - prevMillisDummy >= DELAY_SENSOR_READ)
    {
        if(count == sizeof(dummy_temp))
            count = 0;

        String msg = "{ humidity: \"" + String(dummy_hum[count]) + "\", temperature: \"" + dummy_temp[count]  + "\" }";

        mesh.publish("telemetry/", msg.c_str());
        prevMillisDummy = currMillis;
        count++;
    }

    #endif

    // Blink
    // static long previousMillisBlink = millis();
    // if(millis() - previousMillisBlink >= 800)
    // {
    //     if(digitalRead(LED_BUILTIN))
    //         digitalWrite(LED_BUILTIN, LOW);
    //     else
    //         digitalWrite(LED_BUILTIN, HIGH);
    //
    //     previousMillisBlink = currentMillis;
    // }
}

/**
* Called when a MQTT message is received
*/
void mqtt_callback(const char *topic, const char *msg)
{
    Serial.println("Received MQTT on topic " + String(topic) + ": " + String(msg));
}
