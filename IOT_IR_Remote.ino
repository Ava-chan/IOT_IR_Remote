/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

/* Set these to your desired credentials. */
const char *ssid = "IOT_Remote";
const int wlan_mode = 0; //0 is AP mode, 1 is Stationary mode;

const String html_header = "<html><head>";
const String html_header_end = "<title>IOT_Remote</title></head><body>";
const String html_button_rec = "<a href=\"/rec\"><button>Record</button></a><br />";
const String html_button_send = "<a href=\"/send\"><button>Send</button></a>";
const String html_footer = "</body></html>";
const String html_meta_iphone = "<meta name = \"viewport\" content = \"width = device-width\"><meta name = \"viewport\" content = \"width = 320\">";

bool last_record[10000];

int LEDPIN=0;
int IRINPUTPIN=2;

ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
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
 
void handleRoot() {
  String html = "";
  if (wlan_mode == 0) {
    //AP_mode
    html += html_header;
    html += html_meta_iphone;
    html += html_header_end;
    html += "<br />";
    html += html_button_rec;
    html += html_button_send;
    html += html_footer;

    //html += printAccessPoints();
  } else {
    //Stationary_mode // TODO
    html += html_header;
    html += html_meta_iphone;
    html += html_header_end;
    html += "THIS IS TODO";
    html += html_footer;
  }  
	server.send(200, "text/html", html);
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
}

void showRecord() {
  String html = "";
  html += html_header;
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
  html += html_header_end;
  html += "Here comes the record: <br />";
  //TODO SHOW RECORD
  for (int i = 0; i < 10000; i+=4) {
    html += last_record[i] == true ? "1" : "0";
  }
  html += html_footer;
  server.send(200, "text/html", html);  
}

void sendRecord() {
  String html = "";
  html += html_header;
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
  html += html_header_end;
  html += "Send record<br />";
  html += html_footer;
  server.send(200, "text/html", html);
  for (int k = 0; k < 5; k++)
  for (int i = 0; i < 10000; i++) {
    if (last_record[i] == true) {
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
  digitalWrite(LEDPIN, LOW);
}

void handleRecord() {
  String html = "";
  html += html_header;
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
  html += "<meta http-equiv=\"refresh\" content=\"4; URL=/\">"; //Back to start page after 3 sec
  html += html_header_end;
  html += "Record now for 3 seconds";
  html += html_footer;
  server.send(200, "text/html", html);
  //TODO RECORD
  int found_start = 0;
  int current_millis = millis();
  while (found_start == 0 && (current_millis + 3000) >= millis()) {
    found_start = (digitalRead(IRINPUTPIN) == LOW) ? 1 : 0;
  }
  if (found_start == 1) {
    for (int i = 0; i < 10000; i++) {
      last_record[i] = (digitalRead(IRINPUTPIN) == LOW) ? true : false;
      delayMicroseconds(13); //26 for 36 kHz realized with two times 13
      digitalRead(IRINPUTPIN);
      delayMicroseconds(13);
    }
  }
}

void setup() {
	/* You can remove the password parameter if you want the AP to be open. */
  /*WiFi.mode(WIFI_STA);
  WiFi.disconnect();*/
  delay(1000);
  WiFi.mode(WIFI_AP);
	WiFi.softAP(ssid);

	IPAddress myIP = WiFi.softAPIP();
 
	server.on("/", handleRoot);
  server.on("/rec", handleRecord); 
  server.on("/srec", showRecord);
  server.on("/send", sendRecord);
  server.onNotFound(handleNotFound);
	server.begin();
	//Serial.println("HTTP server started");
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  pinMode(IRINPUTPIN, INPUT);
  //Serial.println("GPIO High");
}

int led_on = HIGH;

void loop() {
	server.handleClient();
  /*if (current_millis < millis()) {
    current_millis = millis() + 500;
    if (led_on == HIGH) led_on = LOW; else led_on = HIGH;
    //digitalWrite(LEDPIN, led_on);
    digitalWrite(LEDPIN, digitalRead(IRINPUTPIN));
    //if (led_on == HIGH) Serial.println("0HIGH"); else Serial.println("0LOW");
    //if ( == HIGH) Serial.println("1HIGH"); else Serial.println("1LOW");


  }*/
  digitalWrite(LEDPIN, digitalRead(IRINPUTPIN));
  
}
