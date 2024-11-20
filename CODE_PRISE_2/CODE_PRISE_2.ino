#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

const char* wifi_ssid = "YouTube : DOUBLE_T";
const char* wifi_password = "Thib123456";
const char* mqtt_server = "192.168.215.42"; //Adresse du serveur MQTT (à changer en fonction de l'endroit Maison ou IUT, ne pas oublier de changer au niveau du broker)
const int mqtt_port = 1883;
const char* mqtt_user = "toto";
const char* mqtt_password = "toto";
const char* mqtt_send_topic = "SAE301/PrisesSend";
const char* mqtt_receive_topic = "SAE301/PrisesBackup2"; // Nouveau topic

WiFiClient espClient;
PubSubClient mqttClient(espClient);

#define BUTTON_PIN 16    // GPIO 2 pour le bouton (modifiable selon votre câblage)       // Broche du bouton-poussoir (connectée en pull-down)
 
bool ledAllumée = false;        // État de la LED
bool dernierÉtatBouton = LOW;  // Dernier état du bouton
bool envoieNecessaire = true; // Indique si un envoi est nécessaire
unsigned long previousMillis = 0;

void setup() {

  // Connexion au réseau Wi-Fi
  WiFiManager wifiManager;
  wifiManager.autoConnect("Prise2", "toto123456");

  // Connexion au serveur MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
  reconnect();

  pinMode(BUTTON_PIN, INPUT_PULLUP); // Configuration du bouton comme une entrée
  pinMode(LED_BUILTIN, OUTPUT);   // Configuration de la LED intégrée comme une sortie
  Serial.begin(115200); // Initialisation du port série
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnect();
    }
    mqttClient.loop();

    int boutonÉtat = digitalRead(BUTTON_PIN);

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
      String data_prise2 = generateData(2);

      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_send_topic, data_prise2.c_str());
      }
    }
  }

  int boutonÉtat = digitalRead(BUTTON_PIN);
  
  if (boutonÉtat != dernierÉtatBouton) {
    // Bouton appuyé, basculez l'état de la LED
    ledAllumée = !ledAllumée;
    digitalWrite(LED_BUILTIN, ledAllumée ? HIGH : LOW);
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

  if (message == "1") {
    ledAllumée = false;
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("État de la LED : OFF");
  } else if (message == "0") {
    ledAllumée = true;
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("État de la LED : ON");
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

  String état = ledAllumée ? "OFF" : "ON";

  String data = "id_prise=" + String(id_prise) + ";etat=" + état;
  return data;
}
