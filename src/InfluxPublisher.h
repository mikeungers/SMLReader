#ifndef REST_PUBLISHER_H
#define REST_PUBLISHER_H

#include "config.h"
#include "debug.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <string.h>
#include <sml/sml_file.h>
#include "MqttPublisher.h"

#include <InfluxDbClient.h>

#define INFLUXDB_URL "influxdb-url"
#define INFLUXDB_TOKEN "toked-id"
#define INFLUXDB_ORG "org"
#define INFLUXDB_BUCKET "bucket"

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point point("meter");

class InfluxPublisher
{
public:
  
  void setup(MqttConfig _config)
  {
    DEBUG("Setting up Influx publisher.");
    config = _config;
  }

  void connect() {
    client.validateConnection();
  }

  void publish(Sensor *sensor, sml_file *file)
  {
    /*doc["userKey"] = "0imfnc8mVLWwsAawjYr4Rx-Af50DDqtlx";
    doc["sensorKey"] = "0imfnc8mVLWwsAawjYr4Rx-Af50DDqtlx";*/
    point.clearFields();
    point.setTime(timeClient.getEpochTime());

    for (int i = 0; i < file->messages_len; i++)
    {
        sml_message *message = file->messages[i];
        if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE)
        {
            sml_list *entry;
            sml_get_list_response *body;
            body = (sml_get_list_response *)message->message_body->data;
            for (entry = body->val_list; entry != NULL; entry = entry->next)
            {
                if (!entry->value)
                {   // do not crash on null value
                    continue;
                }

                char obisIdentifier[32];
                char buffer[255];

                sprintf(obisIdentifier, "%d-%d:%d.%d.%d/%d",
                        entry->obj_name->str[0], entry->obj_name->str[1],
                        entry->obj_name->str[2], entry->obj_name->str[3],
                        entry->obj_name->str[4], entry->obj_name->str[5]);

                String entryTopic = "";
                
                if (((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_INTEGER) ||
                         ((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_UNSIGNED))
                {
                    double value = sml_value_to_double(entry->value);
                    int scaler = (entry->scaler) ? *entry->scaler : 0;
                    int prec = -scaler;
                    if (prec < 0)
                        prec = 0;
                    value = value * pow(10, scaler);
                    sprintf(buffer, "%.*f", prec, value);
                    point.addField(obisIdentifier, buffer);
                }
                else if (!sensor->config->numeric_only) {
                  if (entry->value->type == SML_TYPE_OCTET_STRING)
                  {
                      char *value;
                      sml_value_to_strhex(entry->value, &value, true);
                      point.addField(obisIdentifier, buffer);
                      free(value);
                  }
                  else if (entry->value->type == SML_TYPE_BOOLEAN)
                  {
                      point.addField(obisIdentifier, entry->value->data.boolean ? "true" : "false");
                  }
                }
            }
        }
    }
     if (!client.writePoint(point)) {
        // handle error
        /*Serial.print("InfluxDB write failed: ");
        Serial.println(client.getLastErrorMessage());*/
    }
  }

private:
  MqttConfig config;
  WiFiClient net;
  HTTPClient http;
};

#endif