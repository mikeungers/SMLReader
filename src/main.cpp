#include <list>
#include "config.h"
#include "debug.h"
#include <sml/sml_file.h>
#include "Sensor.h"
#include <IotWebConf.h>
#include "MqttPublisher.h"
#include "EEPROM.h"
#include <ESP8266WiFi.h>

std::list<Sensor*> *sensors = new std::list<Sensor*>();

void wifiConnected();
void configSaved();

DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;
WiFiClient net;

MqttConfig mqttConfig;
MqttPublisher publisher;

IotWebConf iotWebConf(WIFI_AP_SSID, &dnsServer, &server, WIFI_AP_DEFAULT_PASSWORD, CONFIG_VERSION);
IotWebConfParameter params[] = {
	IotWebConfParameter("MQTT server", "mqttServer", mqttConfig.server, sizeof(mqttConfig.server), "text", NULL, mqttConfig.server, NULL, true),
	IotWebConfParameter("MQTT port", "mqttPort", mqttConfig.port, sizeof(mqttConfig.port), "text", NULL, mqttConfig.port, NULL, true),
	IotWebConfParameter("MQTT username", "mqttUsername", mqttConfig.username, sizeof(mqttConfig.username), "text", NULL, mqttConfig.username, NULL, true),
	IotWebConfParameter("MQTT password", "mqttPassword", mqttConfig.password, sizeof(mqttConfig.password), "password", NULL, mqttConfig.password, NULL, true),
	IotWebConfParameter("MQTT topic", "mqttTopic", mqttConfig.topic, sizeof(mqttConfig.topic), "text", NULL, mqttConfig.topic, NULL, true)};

boolean needReset = false;
boolean connected = false;


void process_message(byte *buffer, size_t len, Sensor *sensor)
{
	// Parse
	sml_file *file = sml_file_parse(buffer + 8, len - 16);

	DEBUG_SML_FILE(file);

	if (connected) {
		publisher.publish(sensor, file);
	}

	// free the malloc'd memory
	sml_file_free(file);
}

void setup()
{
	// Setup debugging stuff
	SERIAL_DEBUG_SETUP(115200);

#ifdef DEBUG
	// Delay for getting a serial console attached in time
	delay(2000);
#endif

	// Setup reading heads
	DEBUG("Setting up %d configured sensors...", NUM_OF_SENSORS);
	const SensorConfig *config  = SENSOR_CONFIGS;
	for (uint8_t i = 0; i < NUM_OF_SENSORS; i++, config++)
	{
		Sensor *sensor = new Sensor(config, process_message);
		if (DEEP_SLEEP_ENABLED) {
			sensor->enableSleepMode();
		}
		sensors->push_back(sensor);
	}
	DEBUG("Sensor setup done.");

	// Initialize publisher
	// Setup WiFi and config stuff
	DEBUG("Setting up WiFi and config stuff.");

	for (uint8_t i = 0; i < sizeof(params) / sizeof(params[0]); i++)
	{
		DEBUG("Adding parameter %s.", params[i].label);
		iotWebConf.addParameter(&params[i]);
	}
	iotWebConf.setConfigSavedCallback(&configSaved);
	iotWebConf.setWifiConnectionCallback(&wifiConnected);
	iotWebConf.setupUpdateServer(&httpUpdater);

	boolean validConfig = iotWebConf.init();
	if (!validConfig)
	{
		DEBUG("Missing or invalid config. MQTT publisher disabled.");
		MqttConfig defaults;
		// Resetting to default values
		strcpy(mqttConfig.server, defaults.server);
		strcpy(mqttConfig.port, defaults.port);
		strcpy(mqttConfig.username, defaults.username);
		strcpy(mqttConfig.password, defaults.password);
		strcpy(mqttConfig.topic, defaults.topic);
	}
	else
	{
		// Setup MQTT publisher
		publisher.setup(mqttConfig);
	}

	server.on("/", [] { iotWebConf.handleConfig(); });
	server.onNotFound([]() { iotWebConf.handleNotFound(); });

	DEBUG("Setup done.");
}

bool canSleep() {
	// Check whether all sensors are sleeping
	for (std::list<Sensor*>::iterator it = sensors->begin(); it != sensors->end(); ++it){
		if (!(*it)->isSleeping()) {
			return false;
		};
	}
	return true;
}

void loop()
{
	// Publisher
	if (connected) {
		publisher.loop();
		yield();
	}

	if (needReset)
	{
		// Doing a chip reset caused by config changes
		DEBUG("Rebooting after 1 second.");
		delay(1000);
		ESP.restart();
	}

	// Execute sensor state machines
	for (std::list<Sensor*>::iterator it = sensors->begin(); it != sensors->end(); ++it){
		(*it)->loop();
	}
	iotWebConf.doLoop();
	yield();

	// Sleep if deep sleep is enabled
	if (DEEP_SLEEP_ENABLED) {
		if (canSleep()) {
			ESP.deepSleep(min(DEEP_SLEEP_DURATION,3600) * 1e6);
		};
	}
}

void configSaved()
{
	DEBUG("Configuration was updated.");
	needReset = true;
}

void wifiConnected()
{
	DEBUG("WiFi connection established.");
	connected = true;
	publisher.connect();
}
