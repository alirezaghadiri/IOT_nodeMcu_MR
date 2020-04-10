#include "FS.h"
#include <Wire.h>
#include <SNTPtime.h>
#include <RtcDS3231.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <ESP8266FtpServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

struct NetWorkDT
{
  IPAddress AS_IP;
  IPAddress AS_Gateway;
  IPAddress AS_Subnet;
  IPAddress AS_Dns;
  String AS_Ssid;
  String AS_Password;
  bool AS_Dhcp;
  String AP_Ssid;
  String AP_Password;
  IPAddress AP_IP;
  IPAddress AP_Gateway;
  IPAddress AP_Subnet;
  int mode;
};

IPAddress IP(192, 168, 1, 200);     //ESP static ip
IPAddress gateway(192, 168, 1, 1);  //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0); //Subnet mask
IPAddress dns(8, 8, 8, 8);

IPAddress IPa(192, 168, 2, 200);     //ESP static ip
IPAddress gatewaya(192, 168, 2, 1);  //IP Address of your WiFi Router (Gateway)
IPAddress subneta(255, 255, 255, 0); //Subnet mask

NetWorkDT network;

void setup()
{

  Serial.begin(115200);
  delay(1000);
  Serial.println("Wellcome");
  Serial.println("Booting Sketch...");
  Serial.println("VERSION 2");
  network.AS_IP = IP;
  network.AS_Gateway = gateway;
  network.AS_Subnet = subnet;
  network.AS_Dns = dns;
  network.AS_Dhcp = false;

  network.AS_Ssid = "D-Link";
  network.AS_Password = "SL2580617418@sh";

  network.AP_IP = IPa;
  network.AP_Gateway = gatewaya;
  network.AP_Subnet = subneta;

  network.AP_Ssid = "Link";

  Run_Multi();
}

void loop()
{
}

bool Run_station()
{
  digitalWrite(2, 0);
  Serial.println("run the wifi to mode Station");
  ESP.eraseConfig();
  WiFi.setAutoConnect(false);
  WiFi.disconnect(true);
  WiFi.hostname("IOT");
  WiFi.mode(WIFI_STA);
  if (network.AS_Password == NULL)
    WiFi.begin(network.AS_Ssid.c_str());
  else
    WiFi.begin(network.AS_Ssid.c_str(), network.AS_Password.c_str());
  if (!network.AS_Dhcp)
    WiFi.config(network.AS_IP, network.AS_Dns, network.AS_Gateway, network.AS_Subnet);
  Serial.print("Conecting to Modem");
  int timeout = 0;
  bool state = true;

  while (WiFi.status() != WL_CONNECTED || timeout == 30)
  {
    Serial.print(".");
    if (WiFi.status() == WL_NO_SSID_AVAIL)
    {
      Serial.println("WL_NO_SSID_AVAIL");
      state = false;
      break;
    }
    if (WiFi.status() == WL_CONNECT_FAILED)
    {
      Serial.println("WL_CONNECT_FAILED");
      state = false;
      break;
    }
    if (WiFi.status() == WL_NO_SHIELD)
    {
      Serial.println("WL_NO_SHIELD");
      state = false;
      break;
    }
    delay(500);
    timeout++;
  }

  if (state)
  {
    Serial.println();
    network.AS_IP = WiFi.localIP();
    network.AS_Subnet = WiFi.subnetMask();
    network.AS_Gateway = WiFi.gatewayIP();
    network.AS_Dns = WiFi.dnsIP();

    Serial.println("Ip Address : " + network.AS_IP.toString());
    Serial.println("Subnet : " + network.AS_Subnet.toString());
    Serial.println("Gateway : " + network.AS_Gateway.toString());
    Serial.println("Dns : " + network.AS_Dns.toString());
  }
  return state;
}
void Run_AccessPoint()
{
  digitalWrite(2, 0);
  Serial.println("run the wifi to mode AccessPoint");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(network.AP_IP, network.AP_Gateway, network.AP_Subnet);
  if (network.AP_Password != NULL)
    WiFi.softAP(network.AP_Ssid.c_str(), network.AP_Password.c_str());
  else
    WiFi.softAP(network.AP_Ssid.c_str());

  Serial.println("Access Point is Run");
  Serial.println("SSid : " + network.AP_Ssid);
  Serial.print("IP address : ");
  Serial.println(network.AP_IP);
  digitalWrite(2, 1);
}
void Run_Multi()
{
  Run_AccessPoint();
  Run_station();
}