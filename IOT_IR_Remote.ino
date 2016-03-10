/* Create a WiFi access point and provide a web server on it. */


#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <vector>

//Addres in local wireless network: iot_remote.local !
MDNSResponder mDNS; //Apple service for easy providing url name , eventually not supported
//https://de.wikipedia.org/wiki/Zeroconf#Multicast_DNS
//Supported from: Linux, Windows (Apple services must be installed), Apple (Iphone?)
//not supported for Android(S5mini!?)
ESP8266WebServer server(80);

//AccessPoint IP: 192.168.4.1
#define ssidAP "IOT_Remote"

const String html_header = "<html><head>";
const String html_header_end = "<title>IOT_Remote</title><link rel=\"stylesheet\" type=\"text/css\" href=\"phone.css\"><meta name=\"viewport\" content=\"user-scalable-no, width=device-width\"></head><body><center>";

//<link rel=\"stylesheet\" type=\"text/css\" href=\"standard.css\" media=\"screen and (min-width:481px)\">

const String html_footer = "</cener></body></html>";
const String html_meta_iphone = "<meta name = \"viewport\" content = \"width = device-width\"><meta name = \"viewport\" content = \"width = 320\">";

const int LEDPIN=0;
const int IRINPUTPIN=2;
const String srec_jumpback = "";

std::vector<int> vec;

String convertIPtoString(IPAddress adr){
     return (String(adr[0]) +"."+String(adr[1])+"."+String(adr[2]) +"."+String(adr[3]));
}
 
void handleRoot() {
    String html = html_header + html_header_end
    + "<div id=\"header\"> <h1>IOT_Remote</h1></div><br/>"
    + "Network IP: " + convertIPtoString(WiFi.localIP()) + "<br/>"
    + "AccessPoint IP: " + convertIPtoString(WiFi.softAPIP()) + "<br/>"
    + "<br /><br /><div id=\"menu\"><ul><li><a href=\"/rec\">Record</a></li><li><a href=\"/send\">Send</a></li></ul><ul><li><a href=\"/configWireless\">Settings</a></li></ul>"
    + html_footer;
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
  String html = html_header
  + "<meta http-equiv=\"refresh\" content=\"3; URL=/index.html\">"
  + html_header_end
  + "Here comes the record: <br />";
  //TODO SHOW RECORD
  for (int i = 0; i < vec.size(); i++) {
    html += String(vec[i]) + ", " + (i % 10 == 0 ? "<br />" : "");
  }
  html += html_footer;
  server.send(200, "text/html", html);
  return;  
}

void sendRecord() {
  String html = "";
  html += html_header;
  html += "<meta http-equiv=\"refresh\" content=\"3; URL=/index.html\">";
  html += html_header_end;
  html += "Send record<br />";
  html += html_footer;
  server.send(200, "text/html", html);
  for (int k = 0; k < 5; k++){
    int v = 0;
    bool value = false;
    for (int i = 0; i < 10000; i++) {
      if (i == vec[v]) {
        v++;
        value = !value;
      }
      if (value == true) {
        digitalWrite(LEDPIN, HIGH);
        delayMicroseconds(13);  //1M/38K/2
        digitalWrite(LEDPIN, LOW);
        delayMicroseconds(13);  //1M/38K/2
      }
      else {
        digitalWrite(LEDPIN, HIGH);
        delayMicroseconds(13);
        digitalWrite(LEDPIN, HIGH);
        delayMicroseconds(13);
      }
    }
  }
  digitalWrite(LEDPIN, HIGH);
  return;
}

int found_start = -1;

void handleRecord() {
  String html = "";
  html += html_header
  + "<meta http-equiv=\"refresh\" content=\"6; URL=/srec\">" //Back to start page after 5 sec
  + html_header_end
  + "<div id=\"header\"> <h1>IOT_Remote</h1></div></br>Record now for 5 seconds"
  + html_footer;
  found_start = 0;
  server.send(200, "text/html", html);
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
  String ap_ssids[ap_ssids_count];
  int k = 0;
  for (int i = 0; i < ap_ssids_count; i++){
    ap_ssids[k] = WiFi.SSID(i);
    for (int j = 0; j < k; j++) {
       if (ap_ssids[j] == ap_ssids[k]) {
            k--; 
            break;
       }
    }
    k++;
  }
  
  ap_ssids_count = k;

  for (int i = 0; i < ap_ssids_count; i++){
    option += "<option>";
    option += ap_ssids[i];
    option += "</option>";
  }
    
  String html = "";
  html += html_header + html_header_end
  + "<div id=\"header\"><h1>Settings</h1></div>"
  + "<form action=\"/submitWireless\" method=\"POST\" id=\"wform\"><div id=\"menu\"><ul>"
  + "<li><a align=\"left\">SSID:<br/><div class=\"select_style\"/><select name=\"SSIDs\">" +option + "</select></div></a></li>"
  + "<li><a align=\"left\">Password: <br/><input type=\"password\" style=\"width:100%;border:1px solid gray\" name=\"PASSWORD\" autofocus length=64 /></li></a></ul>"
  + "<ul><li><a href=\"#\" onclick=\"document.getElementById('wform').submit();\">Submit</a></li><li><a href=\"index.html\">Back</a></li></ul></div></form>"
  + html_footer;
  server.send(200, "text/html", html);
  
  return;
}

void sendPhoneCSS () {
  String css = "body { background-color: #ddd;  color: #222;  font-family: Helvetica;  font-size: 14px;  margin: 0;  padding: 0;}\n #header h1 {  margin: 0;  padding: 0;}\n #header h1 a {  background-color:#ccc;  border-bottom: 1px solid #666;  color: #222;  display: block;  font-size: 20px;  font-weight: bold;  padding: 10px 0;  text-align: center;  text-decoration: none;}\n#menu ul {  list-style: none;  margin: 10px;  padding: 0;}\n#menu ul li a{  background-color:#FFF;  border: 1px solid #999;  color: #222;  display: block;  font-size: 17px;  font-weight: bold;  margin-bottom: -1px;  padding: 12px 10px;  text-decoration: none;}\n#content, \n#sidebar {  padding: 12px;}\n#hinweis {  display: none;}";
  css += ".select_style {overflow:hidden;}\n.select_style select {-webkit-appearance: none;appearance:none;width:100%;background:none;background:transparent;border:1px solid gray;outline:1px;padding:7px 10px;}";
  server.send(200, "text/html", css);
}

bool testWifi() {
 
    for(int i = 0; i < 20;i++) {
        if (WiFi.status() == WL_CONNECTED){
            //Start Apple DNS Service to provide iot_remote.local adress in wireless network
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
    //HTML Pages
    server.on("/", handleRoot);
    server.on("/index.html", handleRoot);
    server.on("/configWireless",handleConfigWireless);
    server.on("/submitWireless",handleSubmitWireless);
    server.on("/rec", handleRecord); 
    server.on("/srec", showRecord);
    server.on("/send", sendRecord);
    //CSS Files
    server.on("/phone.css", sendPhoneCSS);
    //STD Handles
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

unsigned long current_millis;

void checkAndDoRecord() {
  if (found_start == 0) {
    current_millis = millis() + 3000;
    vec.clear();
    found_start = 1;
  }
  if (found_start == 1) {
    found_start == -1;
    while (current_millis > millis()) {
      if (digitalRead(IRINPUTPIN) == LOW) {
        found_start = 2;
        break;
      }
    }
  } 
  if (found_start == 2) {
    bool change_check = true;
    vec.push_back(0);
    
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
    if (vec.size() < 20) {
      found_start = 1; 
      vec.clear();
    } else found_start = -1;
  }
}

void loop() {
  server.handleClient();
  checkAndDoRecord();
}
