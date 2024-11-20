#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

const char* wifi_ssid = "Prise1";
const char* wifi_password = "toto";
const char* mqtt_server = "10.128.200.4";
const int mqtt_port = 1883;
const char* mqtt_user = "Thib";
const char* mqtt_password = "toto";
const char* mqtt_send_topic = "SAE301/pnoutletSend";
const char* mqtt_receive_topic = "SAE301/pnoutletBackup";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

#define BOUTON 2  
#define LED 13     
#define DS18B20_PIN 10

bool ledAllumée = false;        
bool dernierÉtatBouton = LOW;  
float derniereTemperature = 0.0; 
bool envoieNecessaire = true; 
unsigned long previousMillis = 0;
const long interval = 3000; // Intervalle de lecture de la température (5 secondes)

OneWire oneWire(DS18B20_PIN);      
DallasTemperature sensors(&oneWire);  

void setup() {
  
  // Connexion au réseau Wi-Fi
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP", "toto123456");

  // Connexion au serveur MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
  reconnect();

  pinMode(BOUTON, INPUT); 
  pinMode(LED, OUTPUT);   
  Serial.begin(115200); 

  sensors.begin();  
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnect();
    }
    mqttClient.loop();

    int boutonÉtat = digitalRead(BOUTON);

    if (boutonÉtat != dernierÉtatBouton) {
      dernierÉtatBouton = boutonÉtat;

      // Envoi de la valeur sur MQTT
      if (mqttClient.connected()) {
        if (boutonÉtat == HIGH) {
          mqttClient.publish(mqtt_send_topic, "0");
        } else {
          mqttClient.publish(mqtt_send_topic, "1");
        }
      }
      
      envoieNecessaire = true;
    }

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      float temperatureCelsius = lireTemperature();

      if (envoieNecessaire) {
        envoieNecessaire = false;

        // Générer les données pour les prises 1 et 2
        String data_prise1 = generateData(1);

        if (mqttClient.connected()) {
          mqttClient.publish(mqtt_send_topic, data_prise1.c_str());
        }
      }
    }
  }
  
  // Autre code et logique ici

  int boutonÉtat = digitalRead(BOUTON);
  float temperatureCelsius = lireTemperature();

  if (boutonÉtat != digitalRead(BOUTON)) {
    // Bouton appuyé, basculez l'état de la LED
    ledAllumée = !ledAllumée;
    digitalWrite(LED, ledAllumée ? HIGH : LOW);
    envoieNecessaire = true;
  }

  if (temperatureCelsius != derniereTemperature) {
    // Faites quelque chose ici si nécessaire
    // Par exemple, vous pouvez envoyer un message MQTT.
    envoieNecessaire = true;
  }

  // Mettez à jour la température précédente
  derniereTemperature = temperatureCelsius;
}

float lireTemperature() {
  sensors.requestTemperatures();
  float temperatureCelsius = sensors.getTempCByIndex(0);

  return temperatureCelsius;
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Gestion des messages MQTT reçus ici
  Serial.println("Message MQTT reçu:");
  Serial.write(payload, length);
  Serial.println();
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("Contenu du message : " + message); 

  if (message == "1") {
    ledAllumée = true;
    digitalWrite(LED, HIGH);
    Serial.println("État de la LED : ON");
  } else if (message == "0") {
    ledAllumée = false;
    digitalWrite(LED, LOW);
    Serial.println("État de la LED : OFF");
  }
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.println("Tentative de connexion au serveur MQTT...");
    if (mqttClient.connect("prise1", mqtt_user, mqtt_password)) {
      Serial.println("Connecté au serveur MQTT");
      mqttClient.subscribe(mqtt_receive_topic); 
    } else {
      Serial.print("Échec de la connexion au serveur MQTT. Réessai dans 5 secondes...");
      delay(5000);
    }
  }
}

String generateData(int id_prise) {
  float temperatureCelsius = lireTemperature();

  // Formatez la température avec deux décimales
  String temperatureFormatted = String(temperatureCelsius, 2);

  String état = ledAllumée ? "ON" : "OFF";

  String data = "id_prise=" + String(id_prise) + ";temp=" + temperatureFormatted + ";etat=" + état;
  return data;
}
