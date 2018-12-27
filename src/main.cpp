#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#include "FS.h"

ESP8266WebServer server(80); //settaggio serrver sulla porta 80

#define G D1
#define R D2
#define B D3
#define BT D4

U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, D5, D6, D7, D8, D0);

extern void oled_display_update(char *s_1, char *s_2, char *s_3, char *s_4, char *s_5, char *s_6, int mode, U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI oled_display);
void save_json();
bool request = 0;

//Dichiarazione di variabili globali

const char *www_username = "LEST";
const char *www_password = "LESTermostater";

const char *net_ssid = "";
const char *net_pswd = "";
const char *api_url = "";
const char *id_device = "";
String id_lest = "";

int delay_time = 0;
bool static_config = 0; //static or DHCP

int ip[4], dns[4], default_gw[4], subnet_m[4];

String getData, Link, file_config;

HTTPClient http;

void setup()
{

  pinMode(R, OUTPUT); //Red
  pinMode(G, OUTPUT); //Green
  pinMode(B, OUTPUT); //Blue

  Serial.begin(115200); //Inizializzo la seriale

  u8g2.begin();

  oled_display_update("", "", "", "", "", "", 2, u8g2);

  WiFi.mode(WIFI_AP_STA); //Abilito la possibilità di avere access point e client attivi

  //Monto il mio SPIFFS File System

  if (!SPIFFS.begin())
  {
    Serial.println("File system non montato ");
    oled_display_update("Errore X(", "config.js", "non importato", "", "", "", 1, u8g2);
  }
  else
  {
    Serial.println("File system montato");
  }

  // Dichiaro  che il file da aprire è il config.json

  File config_json_file = SPIFFS.open("/config.json", "r");

  file_config = config_json_file.readStringUntil('\n'); //questo è il nostro config.js salvato su stringa

  //Creo buffer JSON per la lettura del file config.json

  DynamicJsonBuffer jsonBuffer;
  JsonObject &config_json = jsonBuffer.parseObject(file_config); //leggo la configurazione

  if (!config_json.success())
  {
    Serial.println("Impossibile leggere la configurazione");
    oled_display_update("Errore X(", "config.js", "non parsato", "", "", "", 1, u8g2);
  }
  else
  {
    Serial.println("Configuazione correttamente caricata");
    config_json.printTo(Serial);
    Serial.println("");
  }

  net_ssid = config_json["net_ssid"];
  net_pswd = config_json["net_pswd"];
  api_url = config_json["api_url"];
  id_device = config_json["id_device"];
  id_lest = String(id_device);
  delay_time = config_json["delay_time"];

  static_config = config_json["net_static"];

  ip[0] = config_json["net_ip_0"];
  ip[1] = config_json["net_ip_1"];
  ip[2] = config_json["net_ip_2"];
  ip[3] = config_json["net_ip_3"];

  default_gw[0] = config_json["net_dfgw_0"];
  default_gw[1] = config_json["net_dfgw_1"];
  default_gw[2] = config_json["net_dfgw_2"];
  default_gw[3] = config_json["net_dfgw_3"];

  subnet_m[0] = config_json["net_sm_0"];
  subnet_m[1] = config_json["net_sm_1"];
  subnet_m[2] = config_json["net_sm_2"];
  subnet_m[3] = config_json["net_sm_3"];

  dns[0] = config_json["net_dns_0"];
  dns[1] = config_json["net_dns_1"];
  dns[2] = config_json["net_dns_2"];
  dns[3] = config_json["net_dns_3"];

  if (static_config)
  {
    Serial.println("Configurazione statica....");
    IPAddress ip_addr(ip[0], ip[1], ip[2], ip[3]);
    IPAddress dns_addr(dns[0], dns[1], dns[2], dns[3]);
    IPAddress gw_addr(default_gw[0], default_gw[1], default_gw[2], default_gw[3]);
    IPAddress sm_addr(subnet_m[0], subnet_m[1], subnet_m[2], subnet_m[3]);

    Serial.println(ip_addr);
    Serial.println(gw_addr);
    Serial.println(sm_addr);
    Serial.println(dns_addr);

    if (!WiFi.config(ip_addr, gw_addr, sm_addr, dns_addr))
    { //Configurazione statica del web-server in caso di Client
      Serial.println("Errore configurazione statica");
    }
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(net_ssid, net_pswd); // Provo a eseguire una connessione con le credenziali che ho

  Serial.print("Connecting...");

  long start_c = millis();
  long counter = 0;
  long soglia = 300000; //soglia di controllo per passare il AP (default 25s)

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    counter += millis() - start_c;
    Serial.println(counter);
    if (counter < soglia)
    {
      delay(1000);
      Serial.print(".");
    }
    else
    {
      break;
    }
  }
  Serial.println("");
  Serial.println(WiFi.macAddress());

  if (WiFi.status() == WL_CONNECTED)
  {
    //Connessione stabilita
    Serial.println(WiFi.localIP().toString().c_str());
    oled_display_update("Connesso", "bella", ":P", "", "", "", 0, u8g2);

    analogWrite(R, 255);
    analogWrite(G, 0);
    analogWrite(B, 0);

    //Setto http sull'indirizzo del mio server

    String http_address = String(api_url);

    Serial.print("Richiesta settata su: "); //Stampo l'indirizzo
    Serial.println(http_address);

    request = 1; //Abilito l'invio del dato

    http.begin(http_address); //configuro e avio htt sul'url precedentemente dichiarato

    // Dichiaro la struttura del mio filesystem in modo da caricare i file archiviati con SPIFFS
    server.serveStatic("/js", SPIFFS, "/js");
    server.on("/info", []() {
      return server.send(200, "text/plain", id_lest);
    });
    server.on("/save", save_json);
    server.serveStatic("/img", SPIFFS, "/img");
    server.serveStatic("/css", SPIFFS, "/css");
    server.serveStatic("/", SPIFFS, "/index.html");
    server.begin(); //Faccio partire il server

    // Mostro l'avvenuto successo della connesione su display e do informazioni utili all'utente
  }
  else
  {

    //Problemi di connesione avvio Access Point

    WiFi.disconnect(true); //Disconnetto la wifi

    WiFi.mode(WIFI_AP);
    WiFi.softAP("LEST", "LESTermostater"); // ìdichiaro i parametri del mio access point

    Serial.println("Access Point Mode");
    Serial.println(WiFi.softAPIP());

    analogWrite(R, 120);
    analogWrite(G, 255);
    analogWrite(B, 0);

    //update_display(1, "AP SOMMM", "192.168.4.1"); //192.168.4.1 default

    //Avviso con modalità 1 il display che ho creato un access point

    server.serveStatic("/js", SPIFFS, "/js");
    server.on("/info", []() {
      return server.send(200, "text/plain", id_lest);
    });
    server.on("/save", save_json);
    server.serveStatic("/img", SPIFFS, "/img");
    server.serveStatic("/css", SPIFFS, "/css");
    server.serveStatic("/", SPIFFS, "/index.html");
    server.begin(); //Faccio partire il server
  }
}

void loop()
{
  server.handleClient();
}


void save_json()
{

  if (!SPIFFS.begin()) //controllo di aver accesso al filesystem
  {
    // Se vine visualizzato c'è un problema al filesystem
    server.send(500, "text/plain", "Impossibile leggere la configurazione attualmente memorizzata"); //messaggio di callback per client web
    Serial.println("SPIFFS2 Mount failed");
  }
  else
  { //File sistem correttamente caricato
    Serial.println("SPIFFS2 Mount succesfull");
  }

  File file_conf_saved = SPIFFS.open("/config.json", "r"); //Apro il file in modalità lettura

  DynamicJsonBuffer jsonBuffer_local; //creo secondo buffer
  String conf_read_file = file_conf_saved.readStringUntil('\n');
  JsonObject &json = jsonBuffer_local.parseObject(conf_read_file);

  if (!json.success())
  {
    server.send(500, "text/plain", "Impossibile leggere la configurazione attualmente memorizzata"); //messaggio di callback per client web
    Serial.println("Impossibile leggere la configurazione");
    oled_display_update("Errore X(", "config.js", "non importato", "", "", "", 1, u8g2);
  }
  else
  {
    Serial.println("Configurazione correttamente caricata");
    json.printTo(Serial);
    Serial.println("");
  }

  //#######################################################
  //controllo e salvataggio dei dati in caso di cambiamento
  //#######################################################

  if (server.arg("net_ssid") != "")
  { //ssid
    json.set("net_ssid", server.arg("net_ssid"));
  }

  if (server.arg("net_pswd") != "")
  { //password
    json.set("net_pswd", server.arg("net_pswd"));
  }

  if (server.arg("api_url") != "")
  { //codice dispositivo
    json.set("api_url", server.arg("api_url"));
  }

  if (server.arg("id_device") != "")
  { //url server con api
    json.set("id_device", server.arg("id_device"));
  }

  if (server.arg("net_static") != "")
  { // 1-0 abilita configurazione statica
    json.set("net_static", server.arg("net_static"));
  }

  if (server.arg("net_ip_0") != "")
  { // ip[0]
    json.set("net_ip_0", server.arg("net_ip_0"));
  }

  if (server.arg("net_ip_1") != "")
  { //ip[1]
    json.set("net_ip_1", server.arg("net_ip_1"));
  }

  if (server.arg("net_ip_2") != "")
  { //ip[2]
    json.set("net_ip_2", server.arg("net_ip_2"));
  }

  if (server.arg("net_ip_3") != "")
  { //ip[3]
    json.set("net_ip_3", server.arg("net_ip_3"));
  }

  if (server.arg("net_dns_0") != "")
  { //dns[0]
    json.set("net_dns_0", server.arg("net_dns_0"));
  }

  if (server.arg("net_dns_1") != "")
  { //dns[1]
    json.set("net_dns_1", server.arg("net_dns_1"));
  }

  if (server.arg("net_dns_2") != "")
  { //dns[2]
    json.set("net_dns_2", server.arg("net_dns_2"));
  }

  if (server.arg("net_dns_3") != "")
  { //dns[3]
    json.set("net_dns_3", server.arg("net_dns_3"));
  }

  request = 0;

  delay(100); //aspetto che tutto sia correttamente settato e poi scrivo

  if (!server.authenticate(www_username, www_password))
  {
    return server.requestAuthentication();
  }

  File save = SPIFFS.open("/config.json", "w"); //Apro il file in modalità scrittura

  delay(200);
  json.printTo(save);   //salvo la nuova configurazione
  json.printTo(Serial); // stampo la nuova configurazione

  server.send(200, "text/plain", "Salvataggio efettuato correttamente. Riavvia SOMMM appena led rosso spento"); //messaggio di callback per client web
  Serial.println("Riavvia il dispositivo appena led rosso spento");
  oled_display_update("Savaltaggio eseguito", "Riavviare", "il Dispositivo", "", "", "", 0, u8g2);


  //Riavvio dispositivo
}
