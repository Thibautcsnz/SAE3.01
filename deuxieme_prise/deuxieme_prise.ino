#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

const char* wifi_ssid = "YouTube : DOUBLE_T";
const char* wifi_password = "Thib123456";
const char* mqtt_server = "pnoutlet.site";
const int mqtt_port = 1883;
const char* mqtt_user = "Thib";
const char* mqtt_password = "toto";
const char* mqtt_send_topic = "SAE301/MQTT_S";
const char* mqtt_receive_topic = "SAE301/MQTT_R"; // Nouveau topic

WiFiClient espClient;
PubSubClient mqttClient(espClient);

#define BOUTON 16       // Broche du bouton-poussoir (connectée en pull-down)
#define INTERNAL_LED 2  // Broche de la LED intégrée

bool ledAllumée = false;        // État de la LED
bool dernierÉtatBouton = LOW;  // Dernier état du bouton
bool envoieNecessaire = true; // Indique si un envoi est nécessaire

void setup() {

  // Connexion au réseau Wi-Fi
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP", "toto123456");

  // Connexion au serveur MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
  reconnect();

  pinMode(BOUTON, INPUT); // Configuration du bouton comme une entrée
  pinMode(INTERNAL_LED, OUTPUT);   // Configuration de la LED intégrée comme une sortie
  Serial.begin(115200); // Initialisation du port série
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
    digitalWrite(INTERNAL_LED, ledAllumée ? HIGH : LOW);
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

  Serial.println("Contenu du message : " + message); // Affichez le contenu du message

  if (message == "OFF") {
    // Éteignez la LED
    ledAllumée = false;
    digitalWrite(INTERNAL_LED, HIGH); // Contrôle de la LED
  } else if (message == "ON") {
    // Allumez la LED
    ledAllumée = true;
    digitalWrite(INTERNAL_LED, LOW); // Contrôle de la LED
  }
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.println("Tentative de connexion au serveur MQTT...");
    if (mqttClient.connect("Prise2", mqtt_user, mqtt_password)) {
      Serial.println("Connecté au serveur MQTT");
      mqttClient.subscribe(mqtt_receive_topic); // Abonnez-vous au topic de sauvegarde
    } else {
      Serial.print("Échec de la connexion au serveur MQTT. Réessai dans 5 secondes...");
      delay(5000);
    }
  }
}

String generateData(int id_prise) {

  String état = ledAllumée ? "ON" : "OFF";

  String data = "id_prise=" + String(id_prise) + ";etat=" + état;
  return data;
}
