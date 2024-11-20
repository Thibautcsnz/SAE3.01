#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

const char* wifi_ssid = "YouTube : DOUBLE_T";
const char* wifi_password = "Thib123456";
const char* mqtt_server = "192.168.79.42"; //Adresse du serveur MQTT (à changer en fonction de l'endroit Maison ou IUT, ne pas oublier de changer au niveau du broker)
const int mqtt_port = 1883;
const char* mqtt_user = "toto";
const char* mqtt_password = "toto";
const char* mqtt_send_topic = "SAE301/PrisesSend";
const char* mqtt_receive_topic = "SAE301/PrisesBackup";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

#define BOUTON 16  
#define LED 15      

bool ledAllumée = false;        
bool dernierÉtatBouton = LOW;  
bool envoieNecessaire = true; 
unsigned long previousMillis = 0;

void setup() {
  
  // Connexion au réseau Wi-Fi
  WiFiManager wifiManager;
  wifiManager.autoConnect("Prise1", "toto123456");

  // Connexion au serveur MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
  reconnect();
  pinMode(LED, OUTPUT);   
  Serial.begin(115200); 
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
          mqttClient.publish(mqtt_send_topic, "OFF");
        } else {
          mqttClient.publish(mqtt_send_topic, "ON");
        }
      }
      
      envoieNecessaire = true;
    }

    unsigned long currentMillis = millis();

    if (envoieNecessaire) {
      envoieNecessaire = false;

      // Générer les données pour les prises 1 et 2
      String data_prise1 = generateData(1);

      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_send_topic, data_prise1.c_str());
      }
    }
  }

  int boutonÉtat = digitalRead(BOUTON);
  
  if (boutonÉtat != dernierÉtatBouton) {
    // Bouton appuyé, basculez l'état de la LED
    ledAllumée = !ledAllumée;
    digitalWrite(LED, ledAllumée ? HIGH : LOW);
    envoieNecessaire = true;
  }
}  // Fin de la fonction loop


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

  if (message == "ON") {
    ledAllumée = true;
    digitalWrite(LED, HIGH);
    Serial.println("État de la LED : ON");
  } else if (message == "OFF") {
    ledAllumée = false;
    digitalWrite(LED, LOW);
    Serial.println("État de la LED : OFF");
  }
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.println("Tentative de connexion au serveur MQTT...");
    if (mqttClient.connect("Prise1", mqtt_user, mqtt_password)) {
      Serial.println("Connecté au serveur MQTT");
      mqttClient.subscribe(mqtt_receive_topic); 
    } else {
      Serial.print("Échec de la connexion au serveur MQTT. Réessai dans 5 secondes...");
      delay(5000);
    }
  }
}

String generateData(int id_prise) {
  String état = ledAllumée ? "ON" : "OFF";

  // Modifiez le format des données pour être en JSON
  String data = "{\"id_prise\":" + String(id_prise) + ",\"etat\":\"" + état + "\"}";
  return data;
}