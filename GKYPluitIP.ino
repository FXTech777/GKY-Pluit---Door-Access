#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <time.h>
#include <ESP8266HTTPUpdateServer.h>


const char* ssid = "Staff - GKY Pluit";
const char* password = "pluit1974";

ESP8266WebServer server(80);
const int ledPin = D4;      // Led Indikator
const int pushPin = D1;     // Push Button
const int buzzPin = D2;     // Buzzer


ESP8266HTTPUpdateServer httpUpdater;


const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

void initTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("Syncing time");
  while (time(nullptr) < 100000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWaktu tersinkron.");
}

String getDateID() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[11];
  strftime(buffer, sizeof(buffer), "%d-%m-%Y", timeinfo); // Format Indonesia
  return String(buffer);
}

String getTimeNow() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[9];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
  return String(buffer);
}

void logAccess(String username, String clientIP) {
  File file = SPIFFS.open("/access_log.csv", "a");
  if (!file) {
    Serial.println("Gagal membuka file log!");
    return;
  }
  String logEntry = getDateID() + "," + getTimeNow() + "," + username + "," + clientIP + "\n";
  file.print(logEntry);
  file.close();
  Serial.print("Log disimpan: ");
  Serial.println(logEntry);
}

void handleOpen() {
  String username = server.hasArg("username") ? server.arg("username") : "UNKNOWN";
  String clientIP = server.client().remoteIP().toString();

  digitalWrite(pushPin, HIGH);  // LED ON
  delay(500);
  digitalWrite(pushPin, LOW); // LED OFF

  logAccess(username, clientIP);
  server.send(200, "text/plain", "Access Granted to " + username);
}

void ledBlink() {
  digitalWrite(ledPin, LOW);  // LED ON
  delay(50);
  digitalWrite(ledPin, HIGH); // LED OFF  
}

void startUpBuzz() {
  digitalWrite(buzzPin, HIGH);
  delay(1500);
  digitalWrite(buzzPin, LOW);
}

void readyBuzz() {
  digitalWrite(buzzPin, HIGH);
  delay(150);
  digitalWrite(buzzPin, LOW);
  delay(50);
  digitalWrite(buzzPin, HIGH);
  delay(150);
  digitalWrite(buzzPin, LOW);
}


void handleRoot() {
  String html = "<html><head><title>GKY Pluit - Ruang TU</title>";
  html += "<style>table{border-collapse:collapse;width:100%;}th,td{border:1px solid #aaa;padding:8px;text-align:left;}th{background:#eee}</style>";
  html += "</head><body><h2>GKY Pluit - Ruang TU</h2>";
  html += "<table><tr><th>Tanggal</th><th>Jam</th><th>Nama</th><th>IP Address</th></tr>";


  File file = SPIFFS.open("/access_log.csv", "r");
  if (!file) {
    html += "<tr><td colspan='4'>Gagal membuka file log</td></tr>";
  } else {
    while (file.available()) {
      String line = file.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;

      // Pisahkan data berdasarkan koma
      int idx1 = line.indexOf(',');
      int idx2 = line.indexOf(',', idx1 + 1);
      int idx3 = line.indexOf(',', idx2 + 1);

      if (idx1 < 0 || idx2 < 0 || idx3 < 0) continue;

      String tanggal = line.substring(0, idx1);
      String jam = line.substring(idx1 + 1, idx2);
      String nama = line.substring(idx2 + 1, idx3);
      String ip = line.substring(idx3 + 1);

      html += "<tr><td>" + tanggal + "</td><td>" + jam + "</td><td>" + nama + "</td><td>" + ip + "</td></tr>";
    }
    file.close();
  }

html += "</table><br>";
html += "<a href=\"/access_log.csv\" download><button>Download CSV</button></a> ";
html += "<a href=\"/update\"><button>Upgrade Firmware</button></a>";
html += "</body></html>";

  server.send(200, "text/html", html);
}


void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(pushPin, OUTPUT);
  pinMode(buzzPin, OUTPUT);
  digitalWrite(ledPin, HIGH); // LED OFF

  startUpBuzz();

  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    ledBlink();
  }
  Serial.println();
  digitalWrite(ledPin, HIGH); // LED OFF  
  Serial.print("Terhubung! IP: ");
  Serial.println(WiFi.localIP());
  readyBuzz();
  if (!SPIFFS.begin()) {
    Serial.println("Gagal mount SPIFFS");
    return;
  }

  initTime();

  server.on("/open", handleOpen);
  server.begin();
  httpUpdater.setup(&server);
  Serial.println("Web server berjalan.");

  server.on("/", handleRoot);
  server.serveStatic("/access_log.csv", SPIFFS, "/access_log.csv");

}

void loop() {
  server.handleClient();
}