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

struct TimeDT
{
  int Hour;
  int Minute;
  int Second;
  int Day;
  int Month;
  int Year;
};
struct NtpDT
{
  String server;
  bool state;
  String timeZone;
};
struct FtpDT
{
  String Username;
  String Password;
  bool state;
};

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
struct AccountDT
{
  String Username;
  String Password;
};

struct ReleInfoDT
{
  int Rele;
  bool Active;
};

struct AlarmInfoDT
{
  bool TimerIsActive;
  int period;
  int hour;
  int minute;
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

NtpDT ntp;
FtpDT ftp;
TimeDT NowTime;
NetWorkDT network;
AccountDT Account;
ReleInfoDT RelesInfo[4];
AlarmInfoDT AlarmsInfo[8];

FtpServer ftpSrv;
ESP8266WebServer server(80);
RtcDS3231<TwoWire> Rtc(Wire);
SNTPtime NTPch("ntp.day.ir");
ESP8266HTTPUpdateServer httpUpdater;

int ReturnRelePin(int ID)
{
  switch (ID)
  {
  case 0:
    return 12;
  case 1:
    return 13;
  case 2:
    return 14;
  case 3:
    return 0;
  }
}
void SetDefultReleInfo()
{
  for (int i = 0; i < 4; i++)
  {
    int pin = ReturnRelePin(i);
    RelesInfo[i].Rele = pin;
    RelesInfo[i].Active = false;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 1);
  }
}

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

  if (!SPIFFS.exists(Return_file_confing_path(account_file_path)))
  {
    Serial.println("file " + Return_file_confing_path(account_file_path) + "is not exist.");
    Account.Username = "admin";
    Account.Password = "@dmin";
    write_account_config();
  }
  else
  {
    Serial.println("file " + Return_file_confing_path(account_file_path) + "is exist.");
  }
  if (!SPIFFS.exists(Return_file_confing_path(ntp_file_path)))
  {
    Serial.println("file " + Return_file_confing_path(ntp_file_path) + "is not exist.");
    ntp.server = "ntp.day.ir";
    ntp.state = false;
    ntp.timeZone = "3.5";
    write_ntp_config();
  }
  else
  {
    Serial.println("file " + Return_file_confing_path(ntp_file_path) + "is exist.");
  }

  if (!SPIFFS.exists(Return_file_confing_path(ftp_file_path)))
  {
    Serial.println("file " + Return_file_confing_path(ftp_file_path) + "is not exist.");
    ftp.Username = "admin";
    ftp.Password = "admin123";
    ftp.state = false;
    write_ftp_config();
  }
  else
  {
    Serial.println("file " + Return_file_confing_path(ftp_file_path) + "is exist.");
  }
  if (!SPIFFS.exists(Return_file_confing_path(alarm_file_path)))
  {
    Serial.println("file " + Return_file_confing_path(alarm_file_path) + "is not exist.");
    for (int i = 0; i < 8; i++)
    {
      AlarmsInfo[i].hour = 0;
      AlarmsInfo[i].minute = 0;
      AlarmsInfo[i].period = 0;
      AlarmsInfo[i].TimerIsActive = false;
    }
    write_alarm_config();
  }
  else
  {
    Serial.println("file " + Return_file_confing_path(alarm_file_path) + "is exist.");
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("Wellcome");
  Serial.println("Booting Sketch...");
  Serial.println("VERSION 2");

  SetDefultReleInfo();

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

  read_All_config();

  RTCConfig();

  NTPConfig();

  TimeDT now = GetTime();
  Serial.println(String(now.Year) + "/" + String(now.Month) + "/" + String(now.Day) + " - " +
                 String(now.Hour) + ":" + String(now.Minute) + ":" + String(now.Second));
  List_of_file();
  //wifi_config();
  Run_station();
  WebServerConfig();
}

void loop()
{
  server.handleClient();
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

void read_All_config()
{
  read_wifi_config();
  read_account_config();
  read_ntp_config();
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
void read_account_config()
{
  String Data = Json_Read_File(Return_file_confing_path(account_file_path));
  if (Data != "")
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(Data);
    if (json.success())
    {
      Account.Username = json["Username"].as<String>();
      Account.Password = json["Password"].as<String>();
    }
    json.end();
  }
}

void write_account_config()
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();

  json["Username"] = Account.Username;
  json["Password"] = Account.Password;

  Json_Write_File(Return_file_confing_path(account_file_path), json);
  json.end();
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
    json.end();
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
  json.end();
}

void read_ntp_config()
{
  String Data = Json_Read_File(Return_file_confing_path(ntp_file_path));
  if (Data != "")
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(Data);
    if (json.success())
    {
      ntp.server = json["Server"].as<String>();
      ntp.state = String_to_bool(json["state"].as<String>());
      ntp.timeZone = json["timeZone"].as<String>();
    }
  }
}

void write_ntp_config()
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();

  json["Server"] = ntp.server;
  json["state"] = bool_to_String(ntp.state);
  json["timeZone"] = ntp.timeZone;

  Json_Write_File(Return_file_confing_path(ntp_file_path), json);
}
void read_ftp_config()
{
  String Data = Json_Read_File(Return_file_confing_path(ftp_file_path));
  if (Data != "")
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(Data);
    if (json.success())
    {
      ftp.Username = json["Username"].as<String>();
      ftp.Password = json["Password"].as<String>();
      ftp.state = String_to_bool(json["state"].as<String>());
    }
  }
}

void write_ftp_config()
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();

  json["Username"] = ftp.Username;
  json["Password"] = ftp.Password;
  json["state"] = bool_to_String(ftp.state);

  Json_Write_File(Return_file_confing_path(ftp_file_path), json);
}

void read_alarm_config()
{
  String Data = Json_Read_File(Return_file_confing_path(alarm_file_path));
  if (Data != "")
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(Data);
    if (json.success())
    {
      for (int i = 0; i < 8; i++)
      {
        String objkey = "alarm " + String(i + 1);
        AlarmsInfo[i].hour = json[objkey]["hour"].as<String>().toInt();
        AlarmsInfo[i].minute = json[objkey]["minute"].as<String>().toInt();
        AlarmsInfo[i].period = json[objkey]["period"].as<String>().toInt();
        AlarmsInfo[i].TimerIsActive = String_to_bool(json[objkey]["TimerIsActive"].as<String>());
        Serial.println("get timer " + String(i + 1) + " is set : " + String(AlarmsInfo[i].hour) + " : " + String(AlarmsInfo[i].minute) + " for " + String(AlarmsInfo[i].period) + "minute");
      }
    }
  }
}

void write_alarm_config()
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();

  for (int i = 0; i < 8; i++)
  {
    String objkey = "alarm " + String(i + 1);
    JsonObject &_json = json.createNestedObject("objkey");
    json["hour"] = String(AlarmsInfo[i].hour);
    json["minute"] = String(AlarmsInfo[i].minute);
    json["period"] = String(AlarmsInfo[i].period);
    json["TimerIsActive"] = bool_to_String(AlarmsInfo[i].TimerIsActive);
    Serial.println("set timer " + String(i + 1) + " is set : " + String(AlarmsInfo[i].hour) + " : " + String(AlarmsInfo[i].minute) + " for " + String(AlarmsInfo[i].period) + "minute");
  }
  Json_Write_File(Return_file_confing_path(alarm_file_path), json);
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
  WiFi.setAutoConnect(true);
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

bool RTCConfig()
{
  Rtc.Begin();
  if (!Rtc.IsDateTimeValid())
  {
    if (Rtc.LastError() != 0)
    {
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    }
    else
    {
      Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(RtcDateTime(2019, 1, 1, 0, 0, 0));
    }
    return false;
  }
  else
  {
    Serial.println("rtc run good");
    return true;
  }
}

bool NTPConfig()
{
  int timeout = 0;
  boolean temp;
  Serial.println("SYNC TIME");
  do
  {
    Serial.print(".");
    timeout++;
    temp = !NTPch.setSNTPtime();
  } while (temp || timeout == 60);

  return temp;
}
TimeDT GetTimeFormRTC()
{
  RtcDateTime datetime = Rtc.GetDateTime();
  TimeDT temp;
  temp.Year = datetime.Year();
  temp.Month = datetime.Month();
  temp.Day = datetime.Day();
  temp.Hour = datetime.Hour();
  temp.Minute = datetime.Minute();
  temp.Second = datetime.Second();

  return temp;
}
void setTimeFormRTC()
{
  Rtc.SetDateTime(RtcDateTime(NowTime.Year, NowTime.Month, NowTime.Day, NowTime.Hour, NowTime.Minute, Rtc.GetDateTime().Second()));
}

int ConvertMiladitoShamsi(int _Month)
{
  int temp = _Month + 10;
  if (temp >= 13)
    temp = temp - 12;
  return temp;
}

bool IssummerTime(int _Month)
{
  if (ConvertMiladitoShamsi(_Month) <= 6)
    return true;
  else
    return false;
}

TimeDT GetTimeFormNTP()
{
  strDateTime datetime;
  double zone = 0;
  if (IssummerTime)
    zone = ntp.timeZone.toDouble() + 1;
  else
    zone = ntp.timeZone.toDouble();
  datetime = NTPch.getTime(zone, 0);
  TimeDT temp;
  temp.Year = datetime.year;
  temp.Month = datetime.month;
  temp.Day = datetime.day;
  temp.Hour = datetime.hour;
  temp.Minute = datetime.minute;
  temp.Second = datetime.second;

  return temp;
}

TimeDT GetTime()
{
  if (ntp.state)
    return GetTimeFormNTP();
  else
    return GetTimeFormRTC();
}
void UpdateTime()
{
  NowTime = GetTime();
}

int ReturnReleInfoID(int ID)
{
  switch (ID)
  {
  case 0:
    return 0;
  case 1:
    return 0;
  case 2:
    return 1;
  case 3:
    return 1;
  case 4:
    return 2;
  case 5:
    return 2;
  case 6:
    return 3;
  case 7:
    return 3;
  }
}

int TimeConvertToInt(int hour, int minutes)
{
  return (hour * 60) + minutes;
}

int *PeriodConvertToTime(int period, int hour, int minutes)
{
  int h = period / 60;
  int m = period % 60;
  int tempM = minutes + m;
  int tempH = hour + h;
  if (tempM > 59)
  {
    tempH += tempM / 60;
    tempM = tempM % 60;
  }
  if (tempH > 23)
    tempH = tempH - 24;
  int Mytime[] = {tempH, tempM};
  return Mytime;
}
bool IsEqualTimeToNow(int hour, int minute)
{
  TimeDT timeNow = GetTime();
  if (hour == timeNow.Hour && minute == timeNow.Minute)
    return true;
  else
    return false;
}
bool IsStartAlarm(AlarmInfoDT alarm)
{
  return IsEqualTimeToNow(alarm.hour, alarm.minute);
}
bool IsEndAlarm(AlarmInfoDT alarm)
{
  int *temptime = PeriodConvertToTime(alarm.period, alarm.hour, alarm.minute);
  return IsEqualTimeToNow(temptime[0], temptime[1]);
}

void PriodAlarm()
{
  for (int i = 0; i < 8; i++)
  {
    AlarmInfoDT _AlarmInfo = AlarmsInfo[i];
    if (_AlarmInfo.TimerIsActive)
    {
      ReleInfoDT _ReleInfo = RelesInfo[ReturnReleInfoID(i)];
      if (_ReleInfo.Active && IsEndAlarm(_AlarmInfo))
      {
        Serial.println("timer " + String(i + 1) + " is stop now");
        digitalWrite(_ReleInfo.Rele, 1);
        _ReleInfo.Active = false;
      }
    }
  }
}

void Alerm()
{
  for (int i = 0; i < 8; i++)
  {
    AlarmInfoDT _AlarmInfo = AlarmsInfo[i];
    if (_AlarmInfo.TimerIsActive)
      if (IsStartAlarm(_AlarmInfo))
      {
        Serial.println("timer " + String(i + 1) + " is start now");
        digitalWrite(RelesInfo[ReturnReleInfoID(i)].Rele, 0);
        RelesInfo[ReturnReleInfoID(i)].Active = true;
      }
  }
}

String return_Css()
{
  String Css = "body { background: #ebc9a2; font-family: 'Tahoma'; direction: rtl; }";
  Css += ".login { margin: 50px auto; padding: 18px 20px; width: 200px; background: #3f65b7; background-clip: padding-box; border: 1px solid #172b4e; border-bottom-color: #142647; border-radius: 5px; box-shadow: inset 0 1px rgba(255, 255, 255, 0.3), inset 0 0 1px 1px rgba(255, 255, 255, 0.1), 0 2px 10px rgba(0, 0, 0, 0.5);}";
  Css += ".login>h1 { margin-bottom: 20px; font-size: 16px; font-weight: bold; color: white; text-align: center; text-shadow: 0 -1px rgba(0, 0, 0, 0.4);}";
  Css += ".login-input { display: block; width: 100%; height: 37px; margin-bottom: 20px; font-family: 'Arial'; font-size: 26px; text-align: center; padding: 0 9px; color: white; text-shadow: 0 1px black; background: #2b3e5d; box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.3), 0 0 4px 1px rgba(255, 255, 255, 0.6);}";
  Css += ".login-submit { display: block; width: 100%; height: 37px; margin-bottom: 15px; font-size: 14px; font-weight: bold; color: #294779; text-align: center; text-shadow: 0 1px rgba(255, 255, 255, 0.3);}";
  Css += ".login-dropdown { display: block; width: 100%; height: 37px; margin-bottom: 20px; font-size: 26px; text-align: center; color: white; text-shadow: 0 1px black; background: #2b3e5d;box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.3), 0 0 4px 1px rgba(255, 255, 255, 0.6);}";
  Css += ".login-label { display: block; margin-bottom: 10px; font-size: 18px; text-align: center;color: white;}";
  Css += ".topnav { background-color: #3f65b7; background-clip: padding-box; border: 1px solid #172b4e; overflow: hidden; border-bottom-color: #142647; border-radius: 5px; box-shadow: inset 0 1px rgba(255, 255, 255, 0.3), inset 0 0 1px 1px rgba(255, 255, 255, 0.1), 0 2px 10px rgba(0, 0, 0, 0.5); padding: 10px 0px 10px 0px;text-align: center;}";
  Css += ".topnav a { color: #f2f2f2; text-align: center; padding: 90px 16px; text-decoration: none; font-size: 17px;}";
  Css += ".topnav a:hover { background-color: #ddd; color: black;}";
  Css += ".topnav a.active { background-color: #4CAF50; color: white;}";
  Css += ".switch { position: relative; display: inline-block; width: 60px; height: 34px; text-align: center;}";
  Css += ".switch input { opacity: 0; width: 0; height: 0; }";
  Css += ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #E04141; -webkit-transition: .4s; transition: .4s;}";
  Css += ".slider:before { position: absolute;content: '';height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; -webkit-transition: .4s; transition: .4s; }";
  Css += "input:checked+.slider { background-color: #34C258;}";
  Css += "input:focus+.slider { box-shadow: 0 0 1px #2196F3;}";
  Css += "input:checked+.slider:before { -webkit-transform: translateX(26px); -ms-transform: translateX(26px); transform: translateX(26px);}";
  Css += ".slider.round { border-radius: 34px; }";
  Css += ".slider.round:before { border-radius: 50%;}";
  Css += ".onoffsw { margin: auto; text-align: center; }";
  Css += ".labelsswitch { padding-left: 10px; padding-right: 10px;}";

  return Css;
}

String Tophtml(String title = "")
{
  String top = "<html><head><title>";
  if (title == "")
    top += "IOT Web Service";
  else
    top += title;
  top += "</title><meta charset='utf-8'>";
  top += "<style type='text/css'>";
  top += return_Css();
  top += "</style>";
  top += "</head>";
  return top;
}
String NavBar()
{
  String nav = "<div class='topnav'>";
  nav += "<a href='/'>خانه</a>";
  nav += "<a href='/RTCtime'>ساعت</a>";
  nav += "<a href='setalerm'>آلارم</a>";
  nav += "<a href='/adslmodem'>مودم</a>";
  nav += "<a href='/settingip'>شبکه</a>";
  nav += "<a href='/chkpass'>تغییر پسورد</a>";
  nav += "<a href='/login?DISCONNECT=YES'>خروج</a>";
  nav += "</div>";

  return nav;
}

String return_javaScript()
{
  String js = "var reqwifi = new XMLHttpRequest();";
  js += "var reqnet = new XMLHttpRequest();";
  js += "var reqauth = new XMLHttpRequest();";
  js += "var reqscan = new XMLHttpRequest();";
  js += "var reqReleStatus = new XMLHttpRequest();";
  js += "var reqChangeReleStatus = new XMLHttpRequest();";

  js += "function responseStatus() {if (this.readyState == 4 && this.status == 200) {var myobj = JSON.parse(this.responseText); if (myobj.rele1 == 'on') document.getElementById('rele1').checked = true; else document.getElementById('rele1').checked = false; if (myobj.rele2 == 'on') document.getElementById('rele2').checked = true; else document.getElementById('rele2').checked = false; if (myobj.rele3 == 'on') document.getElementById('rele3').checked = true; else document.getElementById('rele3').checked = false; if (myobj.rele4 == 'on') document.getElementById('rele4').checked = true; else document.getElementById('rele4').checked = false; } }";

  js += "function ProcessStatus() { reqReleStatus.open('Get', '/releStatus', true); reqReleStatus.onreadystatechange = responseStatus; reqReleStatus.send(); setTimeout(ProcessStatus, 1000); }";

  js += "function responseChangeStatus(parm) { if (this.readyState == 4 && this.status == 200) { alert('عملیات با موفقیت انجام شد.');} }";

  js += "function ReleChangeStatus(parm) { reqReleStatus.open('Get', '/releChangeStatus?rele=' + parm, true); reqReleStatus.onreadystatechange = responseChangeStatus; reqReleStatus.send();}";

  js += "function rele1Function() { ReleChangeStatus('rele1');}";
  js += "function rele2Function() { ReleChangeStatus('rele2');}";
  js += "function rele3Function() { ReleChangeStatus('rele3');}";
  js += "function rele4Function() { ReleChangeStatus('rele4');}";
  js += "function chkpassFunction() { var x = document.forms['chkForm']['OLDPASSWORD'].value; var y = document.forms['chkForm']['NEWPASSWORD'].value; var z = document.forms['chkForm']['RNPASSWORD'].value; if (x == "
        ") { alert('کلمه عبور وارد کنید . '); return false; } else if (y != z) { alert('تکرار کلمه عبور نادرست می باشد . '); return false;}";

  return js;
}
String BotomHtml()
{
  String contaxt;
  contaxt += "<script>" + return_javaScript() + "</script>";
  contaxt += "</html>";
  return contaxt;
}

void Send_rele_status()
{
  if (!is_authentified())
  {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();
  for (int i = 0; i < 4; i++)
  {
    String key = "rele" + String(i + 1);
    if (digitalRead(RelesInfo[i].Rele) == 1)
      json[key] = "off";
    else
      json[key] = "on";
  }
  String releObj = "";
  json.printTo(releObj);
  server.send(200, "text/plain", releObj);
}
void ReleChangeState(int id)
{

  if (digitalRead(RelesInfo[id].Rele) == 1)
  {
    digitalWrite(RelesInfo[id].Rele, 0);
    RelesInfo[id].Active = true;
    Serial.println("Pin" + String(RelesInfo[id].Rele) + " -> 0");
  }
  else
  {
    digitalWrite(RelesInfo[id].Rele, 1);
    RelesInfo[id].Active = false;
    Serial.println("Pin" + String(RelesInfo[id].Rele) + " -> 1");
  }
}
void handle_ReleChangeState()
{
  Serial.println("enter rchs");
  if (!is_authentified())
  {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  if (server.hasArg("rele"))
  {
    Serial.println(server.arg("rele"));
    if (server.arg("rele") == "rele1")
      ReleChangeState(0);
    if (server.arg("rele") == "rele2")
      ReleChangeState(1);
    if (server.arg("rele") == "rele3")
      ReleChangeState(2);
    if (server.arg("rele") == "rele4")
      ReleChangeState(3);
  }
  Serial.println(server.arg("end"));
  server.send(200, "text/plain", "Rele status is change");
}

bool is_authentified()
{
  Serial.println("Enter is_authentified");
  if (server.hasHeader("Cookie"))
  {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if (cookie.indexOf("ESPSESSIONID=1") != -1)
    {
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;
}

void handleLogin()
{
  String msg;
  if (server.hasHeader("Cookie"))
  {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
  }

  if (server.hasArg("DISCONNECT"))
  {
    Serial.println("Disconnection");
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
    server.send(301);
    return;
  }
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD"))
  {
    if (server.arg("USERNAME") == Account.Username && server.arg("PASSWORD") == Account.Password)
    {
      server.sendHeader("Location", "/");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
      server.send(301);
      Serial.println("Log in Successful");
      return;
    }
    msg = "Wrong username/password! try again.";
    Serial.println("Log in Failed");
  }
  String content = Tophtml("Login");
  content += "<body>";
  content += "<form method='POST' class='login' action='/login'><h1>ورود</h1><input type='user' name='USERNAME' class='login-input' placeholder='نام کاربری'>";
  content += "<input type='pass' name='PASSWORD' class='login-input' placeholder='کلمه عبور'><input type='submit' name='SUBMIT' value='ورود' class='login-submit'></form>";
  content += "</body></html>";
  server.send(200, "text/html", content);
}
void handleRoot()
{
  Serial.println("Enter handleRoot");
  String header;
  if (!is_authentified())
  {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  String contaxt = Tophtml();
  contaxt += "<body onload='ProcessStatus()'>";
  contaxt += NavBar();
  contaxt += "<div class='login'>";
  for (int i = 0; i < 4; i++)
  {
    contaxt += "<div class='onoffsw'><label class='labelsswitch'>";
    contaxt += " رله" + String(i + 1);
    contaxt += "</label><label class='switch'>";
    contaxt += "<input type='checkbox'  id='";
    contaxt += "rele" + String(i + 1) + "' ";
    contaxt += "onchange='rele" + String(i + 1) + "Function()'/>";
    contaxt += "<span class='slider round'></span></label></div>";
    if (i < 3)
      contaxt += "<br/>";
  }
  contaxt += "</div></body>";
  contaxt += BotomHtml();
  server.send(200, "text/html", contaxt);
}

void handleRTCtime()
{
  if (!is_authentified())
  {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  UpdateTime();
  if (server.hasArg("Hour") && server.hasArg("MIN") && server.hasArg("YEAR") && server.hasArg("MONTH") && server.hasArg("DAY"))
  {
    NowTime.Hour = server.arg("Hour").toInt();
    NowTime.Minute = server.arg("MIN").toInt();
    NowTime.Year = server.arg("YEAR").toInt();
    NowTime.Month = server.arg("MONTH").toInt();
    NowTime.Day = server.arg("DAY").toInt();
    setTimeFormRTC();
  }
  TimeDT rtctime = GetTimeFormRTC();
  String content = Tophtml();
  content += "<body>";
  content += NavBar();
  content += "<form method='POST' class='login' action='/settime'><h1>تنظیم ساعت</h1>";
  content += "<input type='text' name='Hour'  disabled class='login-input' placeholder='ساعت' value=" + String(rtctime.Hour) + ">";
  content += "<input type='text' name='MIN'  disabled class='login-input' placeholder='دقیقه'value=" + String(rtctime.Minute) + ">";
  content += "<br>";
  content += "<form method='POST' class='login' action='/login'><h1>تنظیم تاریخ</h1>";
  content += "<input type='text' name='YEAR' disabled class='login-input' placeholder='سال' value=" + String(rtctime.Year) + ">";
  content += "<input type='text' name='MONTH' disabled class='login-input' placeholder='ماه' value=" + String(rtctime.Month) + ">";
  content += "<input type='text' name='DAY' disabled class='login-input' placeholder='روز' value=" + String(rtctime.Day) + ">";
  content += "<input type='submit' name='SUBMIT' value='تغییر' class='login-submit'></form>";
  content += "</body>";
  content += BotomHtml();
  server.send(200, "text/html", content);
}

void handleChangePass()
{
  if (!is_authentified())
  {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  if (server.hasArg("OLDPASSWORD") && server.hasArg("NEWPASSWORD") && server.hasArg("RNPASSWORD"))
  {
    String Old = server.arg("OLDPASSWORD");
    String NEW = server.arg("NEWPASSWORD");
    String RNEW = server.arg("RNPASSWORD");
    Serial.print("Old : ");
    Serial.println(Old);
    Serial.print("NEW : ");
    Serial.println(NEW);
    Serial.print("RNEW : ");
    Serial.println(RNEW);

    if (Old == Account.Password)
      if (NEW == RNEW)
      {
        Account.Password = NEW;
        write_account_config();
      }
  }
  String content = Tophtml("تغییر پسورد");

  content += "<body>";
  content += NavBar();
  content += "<form name='chkForm' action='#' onsubmit='return chkpassFunction()' method='post' class='login'>";
  content += "<h1>تغییر پسورد</h1>";
  content += "<input type='text' name='OLDPASSWORD' id='OLDPASSWORD' class='login-input' placeholder='کلمه عبور قبلی '>";
  content += "<input type='password' name='RNPASSWORD' id='RNPASSWORD' class='login-input' placeholder='تکرار کلمه عبور'>";
  content += "<<input type='password' name='RNPASSWORD' id='RNPASSWORD' class='login-input' placeholder='تکرار کلمه عبور '>";
  content += "<input type='submit' value='تغییر' class='login-submit'></form>";
  content += "</body>";
  content += BotomHtml();
  server.send(200, "text/html", content);
}

void WebServerConfig()
{
  Serial.println("Init HTTP server");
  server.on("/", handleRoot);
  server.on("/releStatus", Send_rele_status);
  server.on("/releChangeStatus", handle_ReleChangeState);
  server.on("/login", handleLogin);
  server.on("/RTCtime", handleRTCtime);

  server.on("/chkpass", handleChangePass);

  server.on("/inline", []() { server.send(200, "text/plain", "this works without need of authentification"); });
  const char *headerkeys[] = {"User-Agent", "Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
  Serial.println("HTTP server started");
}