/* Create a WiFi access point and provide a web server on it. */


#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <vector>

//Addres in local wireless network: esp8266.local !
MDNSResponder mDNS; //Apple service for easy providing url name , eventually not supported
//https://de.wikipedia.org/wiki/Zeroconf#Multicast_DNS
//Supported from: Linux, Windows (Apple services must be installed), Apple (Iphone?)
//not supported for Android(S5mini!?)
ESP8266WebServer server(80);

//AccessPoint IP: 192.168.4.1
#define ssidAP "IOT_Remote"

const String html_header = "<html><head>";
const String html_header_end = "<title>IOT_Remote</title></head><body><center>";
const String html_button_rec = "<a href=\"/rec\"><button>Record</button></a><br />";
const String html_button_send = "<a href=\"/send\"><button>Send</button></a><br/>";
const String html_footer = "</cener></body></html>";
const String html_meta_iphone = "<meta name = \"viewport\" content = \"width = device-width\"><meta name = \"viewport\" content = \"width = 320\">";

const int LEDPIN=0;
const int IRINPUTPIN=2;

std::vector<int> vec;

String printAccessPoints() {
  String html = "found ";

  int ap_ssids_count = WiFi.scanNetworks();
  String ap_ssids[ap_ssids_count];
  int k = 0;
  for (int i = 0; i < ap_ssids_count; ++i)
  {
    // Print SSID and RSSI for each network found
    delay(10);
    ap_ssids[k] = "<tr><td>";
    if (WiFi.encryptionType(i) == ENC_TYPE_NONE) {
      // not encrypted
      ap_ssids[k] += "NONE";
    } else if (WiFi.encryptionType(i) == ENC_TYPE_TKIP || WiFi.encryptionType(i) == ENC_TYPE_CCMP) {
      ap_ssids[k] += "WPA";
    } else if (WiFi.encryptionType(i) == ENC_TYPE_WEP) {
      ap_ssids[k] += "WEP";
    } else if (WiFi.encryptionType(i) == ENC_TYPE_AUTO) {
      ap_ssids[k] += "AUTO";
    } else {
      
      // encrypted
      ap_ssids[k] += "***";
    }
    ap_ssids[k] += "</td><td>";
    ap_ssids[k] += WiFi.SSID(i);
    ap_ssids[k] += "</td></tr>";
    for (int j = 0; j < k; j++) {
       if (ap_ssids[j] == ap_ssids[k]) {
        k--; 
        break;
       }
    }
    k++;
  }
  ap_ssids_count = k;
  html += ap_ssids_count;
  html += " APs: <br /><br />";
  html += "<table>";    
  for (int i = 0; i < ap_ssids_count; i++) {
    html += ap_ssids[i];
  }
  html += "</table>";
  return html;
}

String convertIPtoString(IPAddress adr){
     return (String(adr[0]) +"."+String(adr[1])+"."+String(adr[2]) +"."+String(adr[3]));
}
 
void handleRoot() {
    String html = html_header;
    html += html_meta_iphone;
    html += html_header_end;
    html += "Network IP: " + convertIPtoString(WiFi.localIP()) + "<br/>";
    html += "AccessPoint IP: " + convertIPtoString(WiFi.softAPIP()) + "<br/>";
    html += "<br />";
    html += html_button_rec;
    html += html_button_send;
    html += "<a href=\"/configWireless\"><button>Config Network</button></a><br/>";
    html += html_footer;

    //html += printAccessPoints();

	server.send(200, "text/html", html);
    return;
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  return;
}

void showRecord() {
  String html = "";
  html += html_header;
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
  html += html_header_end;
  html += "Here comes the record: <br />";
  //TODO SHOW RECORD
  for (int i = 0; i < vec.size(); i++) {
    html += String(vec[i]);
    html += ", ";
    if (i % 10 == 0) html += "<br />";
    //html += last_record[i] == true ? "1" : "0";
  }
  //html += "v";
  html += html_footer;
  server.send(200, "text/html", html);
  return;  
}

void sendRecord() {
  String html = "";
  html += html_header;
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
  html += "<meta http-equiv=\"refresh\" content=\"4; URL=/srec\">"; //Back to start page after 3 sec
  html += html_header_end;
  html += "Send record<br />";
  html += html_footer;
  server.send(200, "text/html", html);
  for (int k = 0; k < 5; k++){
    int v = 0;
    bool value = true;
    for (int i = 0; i < 10000; i++) {
      if (i == vec[v]) {
        v++;
        value = !value;
      }
      if (value == true) {
        digitalWrite(LEDPIN, LOW);
        delayMicroseconds(13);  //1M/38K/2
        digitalWrite(LEDPIN, HIGH);
        delayMicroseconds(13);  //1M/38K/2
      }
      else {
        digitalWrite(LEDPIN, LOW);
        delayMicroseconds((27));
      }
    }
  }
  digitalWrite(LEDPIN, LOW);
  return;
}

void handleRecord() {
  String html = "";
  html += html_header;
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
  html += "<meta http-equiv=\"refresh\" content=\"6; URL=/srec\">"; //Back to start page after 5 sec
  html += html_header_end;
  html += "Record now for 5 seconds";
  html += html_footer;
  server.send(200, "text/html", html);
  //TODO RECORD
  int found_start = 0;
  int current_millis = millis();
  while (found_start == 0 && (current_millis + 5000) >= millis()) {
    found_start = (digitalRead(IRINPUTPIN) == LOW) ? 1 : 0;
  }
  //bool last_record[10000];
  if (found_start == 1) {
    vec.clear();
    bool change_check = true;
    vec.push_back(0);
    
    delayMicroseconds(27);
    for (int i = 0; i < 10000; i++) {
      bool newone = (digitalRead(IRINPUTPIN) == LOW) ? true : false;
      if (change_check != newone) {
        change_check = newone;
        vec.push_back(i);
      }
      //last_record[i] = (digitalRead(IRINPUTPIN) == LOW) ? true : false;
      delayMicroseconds(13); //26 for 36 kHz
      digitalRead(IRINPUTPIN);
      delayMicroseconds(13);
    }
  }
  return;
}

void handleSubmitWireless(){
    
    String response = "Something went wrong, try again.";
    if (server.args() > 0 ) {
        //clearing eeprom
        for(uint8_t i = 0; i < 96; ++i){ 
            EEPROM.write(i, 0); 
        }
        //write ssid and password in eeprom
        for(uint8_t i = 0; i < server.args(); i++ ){
            if(i==0){
                String ssid = server.arg(i);
                //Only write data, if everything seems correct!
                if(ssid.length() > 0 && ssid.length() <=32){
                    response= "SSID: " + ssid + " configured.<br/>Please restart to log into wireless network.<br/>";
                    for (uint8_t j = 0; j < ssid.length(); ++j){
                        EEPROM.write(j, ssid[j]);
                    }
                }
            }else if(i==1){
                String pass = server.arg(i);
                //Only write data, if everything seems correct!
                if(pass.length() > 0 && pass.length() <= 64){
                    for (uint8_t j = 0; j < pass.length(); ++j){
                        EEPROM.write(32+j, pass[j]);
                    }
                }
            }else{
                //Wrong arg count, sth went wrong?
                break;
            }            
        }
        EEPROM.commit();
    }
    
    String html = html_header;
    html += "<meta http-equiv=\"refresh\" content=\"10; URL=/\">"; //Back to start page after 10 sec
    html += html_meta_iphone;
    html += html_header_end;
    html += response;
    html += html_footer;
    server.send(200, "text/html", html);

    return;
}

void handleConfigWireless(){
    
  String option;
  int ap_ssids_count = WiFi.scanNetworks();
  int k = 0;
  for (int i = 0; i < ap_ssids_count; ++i){
    option += "<option>";
    option += WiFi.SSID(i);
    option += "</option>";
  }
    
  String html = "";
  html += html_header;
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
  html += html_header_end;
  html += "<form action=\"/submitWireless\" method=\"POST\">";
  html+=  "<label>SSID: </label><select name=\"SSIDs\">" +option + "</select><br/>";
  html += "<label>Password: </label><input type=\"password\" name=\"PASSWORD\" autofocus length=64><br/><br/>";
  html += "<input type=\"submit\" value=\"Submit\"></form>";
  html += html_footer;
  server.send(200, "text/html", html);
  
  return;
}


bool testWifi() {
 
    for(int i = 0; i < 20;i++) {
        if (WiFi.status() == WL_CONNECTED){
            //Start Apple DNS Service to provide esp8266.local adress in wireless network
            mDNS.begin("iot_remote");
            mDNS.addService("http","tcp",80);
            return(true); 
        } 
        delay(500); //Try some connects
    }
    return(false);
} 

void setupAP(){
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.softAP(ssidAP);

    return;
}

void launchWebsite() {

    server.on("/", handleRoot);
    server.on("/configWireless",handleConfigWireless);
    server.on("/submitWireless",handleSubmitWireless);
    server.on("/rec", handleRecord); 
    server.on("/srec", showRecord);
    server.on("/send", sendRecord);
    server.onNotFound(handleNotFound);
    server.begin();

    return;
}

void setup() { 

    //configure pins
    pinMode(LEDPIN, OUTPUT);
    digitalWrite(LEDPIN, LOW);
    pinMode(IRINPUTPIN, INPUT);
     
    EEPROM.begin(512);
    delay(10);

    //Read saved ssid from eeprom!
    String eSSID;
    for(int i = 0; i <32;++i){
        eSSID+= char(EEPROM.read(i));
    }
    //Read saved password from eeprom!
    String ePassword;
    for(int i = 32; i < 96;++i){
        ePassword+= char(EEPROM.read(i));
    }
    
    if(eSSID.length() > 1 ) {
      //test wlan-connection
        WiFi.begin(eSSID.c_str(), ePassword.c_str());
        if (testWifi()){ 
            launchWebsite();
            return;
        }
    }
    //Connection with network failed, open an AP
    setupAP(); 
    launchWebsite();
 
    return;
}

void loop() {
	  server.handleClient();
    digitalWrite(LEDPIN, digitalRead(IRINPUTPIN));
    //wait for connects, maybe set here a small delay?
    // modem sleep and low sleep are automatically used, so no sleep is necessary, deep sleep cant be used be cause the power off wifi!
}
