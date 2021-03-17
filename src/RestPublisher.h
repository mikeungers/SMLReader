#ifndef REST_PUBLISHER_H
#define REST_PUBLISHER_H

#include "config.h"
#include "debug.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <string.h>
#include <sml/sml_file.h>
#include "MqttPublisher.h"

struct RestConfig
{
  char server[256] = "mosquitto";
  char port[8] = "1883";
  char username[128] = "";
  char password[128] = "";
};

class RestPublisher
{
public:
  void setup(MqttConfig _config)
  {
    DEBUG("Setting up REST publisher.");
    config = _config;
  }

  void connect() {
    http.begin(net, "http://ptsv2.com/t/ilh63-1616009767/post"); //HTTP
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST("{\"hello\":\"world\"}");
    http.end();
  }

  void publish(Sensor *sensor, sml_file *file)
  {

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
                    publish(entryTopic + "value", buffer);
                }
                else if (!sensor->config->numeric_only) {
                  if (entry->value->type == SML_TYPE_OCTET_STRING)
                  {
                      char *value;
                      sml_value_to_strhex(entry->value, &value, true);
                      publish(entryTopic + "value", value);
                      free(value);
                  }
                  else if (entry->value->type == SML_TYPE_BOOLEAN)
                  {
                      publish(entryTopic + "value",  entry->value->data.boolean ? "true" : "false");
                  }
                }
            }
        }
    }
  }

private:
  MqttConfig config;
  WiFiClient net;
  HTTPClient http;

  void publish(const String &topic, const String &payload)
  {
    publish(topic.c_str(), payload.c_str());
  }
  void publish(String &topic, const char *payload)
  {
    publish(topic.c_str(), payload);
  }
  void publish(const char *topic, const String &payload)
  {
    publish(topic, payload.c_str());
  }
  void publish(const char *topic, const char *payload)
  {
    http.begin(net, "http://ptsv2.com/t/ilh63-1616009767/post"); //HTTP
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST("{\"hello\":\"world\"}");
    http.end();
  }
};

#endif