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

enum WifiMode : int
{
  Station = 1,
  AccessPoint = 2,
  both = 3
};

enum File_Path_name
{
  wifi_file_path,
  ntp_file_path,
  ftp_file_path,
  alarm_file_path,
  account_file_path,
};

IPAddress IP(192, 168, 1, 200);     //ESP static ip
IPAddress gateway(192, 168, 1, 1);  //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0); //Subnet mask
IPAddress dns(8, 8, 8, 8);

IPAddress IPa(192, 168, 2, 200);     //ESP static ip
IPAddress gatewaya(192, 168, 2, 1);  //IP Address of your WiFi Router (Gateway)
IPAddress subneta(255, 255, 255, 0); //Subnet mask

NetWorkDT network;

void sead()
{

  if (!SPIFFS.exists(Return_file_confing_path(wifi_file_path)))
  {
    Serial.println("file " + Return_file_confing_path(wifi_file_path) + "is not exist.");
    network.AS_IP = IPAddress(192, 168, 1, 200);
    network.AS_Gateway = IPAddress(192, 168, 1, 1);
    network.AS_Subnet = IPAddress(255, 255, 255, 0);
    network.AS_Dns = IPAddress(8, 8, 8, 8);
    network.AS_Ssid = "D-Link";
    network.AS_Password = "SL2580617418@sh";
    network.AS_Dhcp = false;

    network.AP_IP = IPAddress(192, 168, 1, 250);
    network.AP_Gateway = IPAddress(192, 168, 1, 250);
    network.AP_Subnet = IPAddress(255, 255, 255, 0);
    network.AP_Ssid = "ESPIOTAccessPoint";
    network.AP_Password = "@dminIOT";
    network.mode = both;

    write_wifi_config();
  }
  else
  {
    Serial.println("file " + Return_file_confing_path(wifi_file_path) + "is exist.");
  }
}

void setup()
{

  Serial.begin(115200);
  delay(1000);
  Serial.println("Wellcome");
  Serial.println("Booting Sketch...");
  Serial.println("VERSION 2");

  if (SPIFFS.begin())
  {
    Serial.println("SPIFFS Initialize....ok");
  }
  else
  {
    Serial.println("SPIFFS Initialization...failed");
  }

  Serial.println("cheaking  confing file");
  sead();
  Serial.println("cheaked   confing file");

  /*network.AS_IP = IP;
  network.AS_Gateway = gateway;
  network.AS_Subnet = subnet;
  network.AS_Dns = dns;
  network.AS_Dhcp = false;

  network.AS_Ssid = "D-Link";
  network.AS_Password = "SL2580617418@sh";
  network.mode = both;

  network.AP_IP = IPa;
  network.AP_Gateway = gatewaya;
  network.AP_Subnet = subneta;

  network.AP_Ssid = "Link";*/
  read_All_config();

  wifi_config();

  List_of_file();
}

void loop()
{
}

void List_of_file()
{
  Dir dir = SPIFFS.openDir("/");
  if (dir.isDirectory())
    Serial.println("Directory exist");
  else
    Serial.println("Directory not exist");
  while (dir.next())
  {
    String str = "";
    str += dir.fileName();
    str += " / ";
    str += dir.fileSize();
    str += "\r\n";
    Serial.println(str);
  }
}

void read_All_config(){
  read_wifi_config();
}
String Return_file_confing_path(File_Path_name key)
{
  switch (key)
  {
  case wifi_file_path:
    return "/wifi_json_config.txt";
  case ntp_file_path:
    return "/ntp_json_config.txt";
  case ftp_file_path:
    return "/ftp_json_config.txt";
  case alarm_file_path:
    return "/alarm_json_config.txt";
  case account_file_path:
    return "/account_json_config.txt";
  }
}

void Json_Write_File(String Path, JsonObject &Data)
{
  File file;
  if (SPIFFS.exists(Path))
    file = SPIFFS.open(Path, "w");
  else
    file = SPIFFS.open(Path, "w+");

  if (file)
  {
    Data.printTo(file);
    file.close();
    Serial.println("File was written  -> " + Path);
  }
  else
  {
    file.close();
    Serial.println("error to write to file  -> " + Path);
  }
}
String Json_Read_File(String Path)
{
  File file = SPIFFS.open(Path, "r");
  if (file)
  {
    String Data = file.readString();
    file.close();
    Serial.println("File was readed -> " + Path);
    return Data;
  }
  else
  {
    file.close();
    Serial.println("error to reading file -> " + Path);
    return "";
  }
}

void read_wifi_config()
{
  String Data = Json_Read_File(Return_file_confing_path(wifi_file_path));
  if (Data != "")
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(Data);
    if (json.success())
    {
      network.AS_IP = String_to_IP(json["Station"]["IP"].as<String>());
      network.AS_Gateway = String_to_IP(json["Station"]["Gateway"].as<String>());
      network.AS_Subnet = String_to_IP(json["Station"]["Subnet"].as<String>());
      network.AS_Dns = String_to_IP(json["Station"]["Dns"].as<String>());
      network.AS_Ssid = json["Station"]["Ssid"].as<String>();
      network.AS_Password = json["Station"]["Password"].as<String>();
      network.AS_Dhcp = String_to_bool(json["Station"]["Dhcp"].as<String>());

      network.AP_IP = String_to_IP(json["AccessPoint"]["IP"].as<String>());
      network.AP_Gateway = String_to_IP(json["AccessPoint"]["Gateway"].as<String>());
      network.AP_Subnet = String_to_IP(json["AccessPoint"]["Subnet"].as<String>());
      network.AP_Ssid = json["AccessPoint"]["Ssid"].as<String>();
      network.AP_Password = json["AccessPoint"]["Password"].as<String>();

      network.mode = json["mode"].as<String>().toInt();
    }
  }
}

void write_wifi_config()
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();

  json["mode"] = network.mode;

  JsonObject &Station_json = json.createNestedObject("Station");
  Station_json["IP"] = network.AS_IP.toString();
  Station_json["Gateway"] = network.AS_Gateway.toString();
  Station_json["Subnet"] = network.AS_Subnet.toString();
  Station_json["Dns"] = network.AS_Dns.toString();
  Station_json["Ssid"] = network.AS_Ssid;
  Station_json["Password"] = network.AS_Password;
  Station_json["Dhcp"] = bool_to_String(network.AS_Dhcp);

  JsonObject &AccessPoint_json = json.createNestedObject("AccessPoint");
  AccessPoint_json["IP"] = network.AP_IP.toString();
  AccessPoint_json["Gateway"] = network.AP_Gateway.toString();
  AccessPoint_json["Subnet"] = network.AP_Subnet.toString();
  AccessPoint_json["Ssid"] = network.AP_Ssid;
  AccessPoint_json["Password"] = network.AP_Password;

  Json_Write_File(Return_file_confing_path(wifi_file_path), json);
}

void wifi_config()
{
  switch (network.mode)
  {
  case Station:
    Run_station();
    break;
  case AccessPoint:
    Run_AccessPoint();
    break;
  case both:
    Run_Multi();
    break;
  }
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
  Run_station();
  Run_AccessPoint();
}

IPAddress String_to_IP(String str)
{
  IPAddress IP_addr;
  int c1 = str.indexOf('.');
  int c2 = str.indexOf('.', c1 + 1);
  int c3 = str.indexOf('.', c2 + 1);
  int ln = str.length();
  IP_addr[0] = str.substring(0, c1).toInt();
  IP_addr[1] = str.substring(c1 + 1, c2).toInt();
  IP_addr[2] = str.substring(c2 + 1, c3).toInt();
  IP_addr[3] = str.substring(c3 + 1, ln).toInt();
  return IP_addr;
}

bool String_to_bool(String str)
{
  if (str == "false" || str == "no" || str == "0")
    return false;
  if (str == "true" || str == "yes" || str == "1")
    return true;
}

String bool_to_String(bool b)
{
  if (b)
    return "true";
  else
    return "false";
}