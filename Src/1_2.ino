#include <WiFi.h>
#include "secrets.h"



const char* ssid;
const char* password;

IPAddress local_IP(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);

const int ledPin = 2;
const int potPin = 34;

const String correctUsername = "admin";
const String correctPassword = "1234";

bool loggedIn = false;
bool darkMode = false;

unsigned long startTime = 0;
struct DataPoint {
unsigned long time;
int value;
};

#define MAX_DATA_POINTS 50
DataPoint chartPoints[MAX_DATA_POINTS];
int chartIndex = 0;
unsigned long lastUpdate = 0;

void setup() {
Serial.begin(115200);
pinMode(ledPin, OUTPUT);
digitalWrite(ledPin, LOW);

WiFi.softAPConfig(local_IP, gateway, subnet);
WiFi.softAP(ssid, password);

Serial.println("Access Point Started");
Serial.print("IP Address: ");
Serial.println(WiFi.softAPIP());

server.begin();
startTime = millis();
}

void loop() {
if (millis() - lastUpdate >= 1000) {
lastUpdate = millis();
int potValue = analogRead(potPin);
unsigned long secondsPassed = (millis() - startTime) / 1000;
chartPoints[chartIndex % MAX_DATA_POINTS] = {secondsPassed, potValue};
chartIndex++;
}

WiFiClient client = server.available();
if (client) {
String request = "";
unsigned long timeout = millis();

while (client.connected() && millis() - timeout < 100) {
  while (client.available()) {
    char c = client.read();
    request += c;
    timeout = millis();
  }
}

Serial.println("Request:");
Serial.println(request);

if (request.indexOf("POST /login") != -1) {
  int bodyStart = request.indexOf("\r\n\r\n");
  if (bodyStart != -1) {
    String body = request.substring(bodyStart + 4);
    int usernameStart = body.indexOf("username=") + 9;
    int usernameEnd = body.indexOf("&", usernameStart);
    String receivedUsername = body.substring(usernameStart, usernameEnd);
    int passwordStart = body.indexOf("password=") + 9;
    String receivedPassword = body.substring(passwordStart);

    if (receivedUsername == correctUsername && receivedPassword == correctPassword) {
      loggedIn = true;
    }
  }
}

if (request.indexOf("GET /led/on") != -1 && loggedIn) {
  digitalWrite(ledPin, HIGH);
}

if (request.indexOf("GET /led/off") != -1 && loggedIn) {
  digitalWrite(ledPin, LOW);
}

if (request.indexOf("GET /logout") != -1) {
  loggedIn = false;
  digitalWrite(ledPin, LOW);
}

if (request.indexOf("GET /dark") != -1 && loggedIn) {
  darkMode = !darkMode;
}

if (request.indexOf("GET /data.json") != -1 && loggedIn) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println();
  client.print("[");

  int startIdx = chartIndex > MAX_DATA_POINTS ? chartIndex - MAX_DATA_POINTS : 0;
  for (int i = 0; i < MAX_DATA_POINTS && i < chartIndex; i++) {
    int idx = (startIdx + i) % MAX_DATA_POINTS;
    client.print("{\"time\":");
    client.print(chartPoints[idx].time);
    client.print(",\"value\":");
    client.print(chartPoints[idx].value);
    client.print("}");
    if (i < MAX_DATA_POINTS - 1 && i < chartIndex - 1) client.print(",");
  }
  client.println("]");
  client.stop();
  return;
}

if (request.indexOf("GET /add-value?value=") != -1 && loggedIn) {
  int valueStart = request.indexOf("value=") + 6;
  int valueEnd = request.indexOf(" ", valueStart);
  if (valueEnd == -1) valueEnd = request.length();
  String valStr = request.substring(valueStart, valueEnd);
  int val = valStr.toInt();
  unsigned long secondsPassed = (millis() - startTime) / 1000;
  chartPoints[chartIndex % MAX_DATA_POINTS] = {secondsPassed, val};
  chartIndex++;
}

client.println("HTTP/1.1 200 OK");
client.println("Content-Type: text/html");
client.println();
client.println("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Control Panel</title>");
client.println("<style>");
client.print("body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; ");
client.print(darkMode ? "background-color: #222; color: white;" : "background: linear-gradient(135deg, #f6d365 0%, #fda085 100%); color: black;");
client.println(" }");
client.println(".container { background-color: #ffffffcc; padding: 40px; border-radius: 15px; box-shadow: 0 8px 16px rgba(0,0,0,0.3); text-align: center; max-height: 90vh; overflow-y: auto; width: 90%; max-width: 600px; }");
client.println("h1 { margin-bottom: 24px; }");
client.println("form { display: flex; flex-direction: column; }");
client.println("input[type='text'], input[type='password'] { padding: 12px; margin: 8px 0; border: 1px solid #ccc; border-radius: 8px; font-size: 16px; }");
client.println("input[type='submit'], button { padding: 12px; margin-top: 16px; background-color: #4CAF50; color: white; border: none; border-radius: 8px; font-size: 16px; cursor: pointer; transition: background-color 0.3s ease; }");
client.println("input[type='submit']:hover, button:hover { background-color: #45a049; }");
client.println("a { text-decoration: none; }");
client.println("table { width: 100%; margin-top: 20px; border-collapse: collapse; }");
client.println("th, td { border: 1px solid #ddd; padding: 8px; text-align: center; }");
client.println("th { background-color: #f2f2f2; }");
client.println(".fullscreen-btn { margin-top: 16px; background-color: #2196F3; }");
client.println(".small-btn { padding: 6px 10px; font-size: 12px; }");
client.println("</style>");
client.println("<script>function goFullscreen() { document.documentElement.requestFullscreen(); }</script>");
client.println("</head><body>");
client.println("<div class='container'>");

if (!loggedIn) {
  client.println("<h1>Login to System</h1>");
  client.println("<form action='/login' method='POST'>");
  client.println("<input type='text' name='username' placeholder='Username'>");
  client.println("<input type='password' name='password' placeholder='Password'>");
  client.println("<input type='submit' value='Login'>");
  client.println("</form>");
} else {
  client.println("<h1>Control LED</h1>");
  client.println("<a href='/led/on'><button>Turn On LED</button></a>");
  client.println("<a href='/led/off'><button>Turn Off LED</button></a>");

  client.println("<h2>-------Next Section-------</h2>");
  client.println("<h2>Potentiometer Readings</h2>");
  client.println("<table id='dataTable'><thead><tr><th>Time (s)</th><th>Value</th></tr></thead><tbody></tbody></table>");

  client.println("<br><br><canvas id='myChart' width='500' height='300' style='margin-top:20px;'></canvas>");
  client.println("<script>");
  client.println("const canvas = document.getElementById('myChart');");
  client.println("const ctx = canvas.getContext('2d');");
  client.println("function drawChart(data) {");
  client.println("  ctx.clearRect(0, 0, canvas.width, canvas.height);");
  client.println("  if (data.length === 0) return;");
  client.println("  let maxVal = Math.max(...data.map(p => p.value));");
  client.println("  let minVal = Math.min(...data.map(p => p.value));");
  client.println("  let scaleX = (canvas.width - 50) / (data.length - 1 || 1);");
  client.println("  let scaleY = maxVal !== minVal ? (canvas.height - 40) / (maxVal - minVal) : 1;");

  client.println("  ctx.beginPath();");
  client.println("  ctx.moveTo(40, 0);");
  client.println("  ctx.lineTo(40, canvas.height - 20);");
  client.println("  ctx.lineTo(canvas.width, canvas.height - 20);");
  client.println("  ctx.strokeStyle = 'gray'; ctx.lineWidth = 1; ctx.stroke();");

  client.println("  ctx.fillStyle = 'black'; ctx.font = '12px sans-serif';");
  client.println("  for (let i = 0; i <= 5; i++) {");
  client.println("    let val = minVal + (maxVal - minVal) * (i / 5);");
  client.println("    let y = canvas.height - 20 - i * (canvas.height - 40) / 5;");
  client.println("    ctx.fillText(val.toFixed(0), 5, y + 3);");
  client.println("    ctx.beginPath(); ctx.moveTo(40, y); ctx.lineTo(canvas.width, y);");
  client.println("    ctx.strokeStyle = '#ddd'; ctx.stroke();");
  client.println("  }");

  client.println("  ctx.beginPath();");
  client.println("  ctx.moveTo(40, canvas.height - 20 - (data[0].value - minVal) * scaleY);");
  client.println("  for (let i = 1; i < data.length; i++) {");
  client.println("    ctx.lineTo(40 + i * scaleX, canvas.height - 20 - (data[i].value - minVal) * scaleY);");
  client.println("  }");
  client.println("  ctx.strokeStyle = 'blue'; ctx.lineWidth = 2; ctx.stroke();");

  client.println("  ctx.fillText('Time (s)', canvas.width / 2, canvas.height - 2);");
  client.println("  ctx.save(); ctx.translate(12, canvas.height / 2);");
  client.println("  ctx.rotate(-Math.PI / 2); ctx.fillText('Value', 0, 0); ctx.restore();");
  client.println("}");

  client.println("function updateTable(data) {");
  client.println("  const tableBody = document.querySelector('#dataTable tbody');");
  client.println("  tableBody.innerHTML = ''; // clear previous rows");
  client.println("  data.forEach(point => {");
  client.println("    const row = document.createElement('tr');");
  client.println("    const timeCell = document.createElement('td');");
  client.println("    timeCell.textContent = point.time;");
  client.println("    const valueCell = document.createElement('td');");
  client.println("    valueCell.textContent = point.value;");
  client.println("    row.appendChild(timeCell);");
  client.println("    row.appendChild(valueCell);");
  client.println("    tableBody.appendChild(row);");
  client.println("  });");
  client.println("}");

  client.println("async function fetchData() {");
  client.println("  const res = await fetch('/data.json');");
  client.println("  const data = await res.json();");
  client.println("  drawChart(data);");
  client.println("  updateTable(data);");
  client.println("}");
  client.println("setInterval(fetchData, 1000);");
  client.println("fetchData();");
  client.println("</script>");

  client.println("<br><a href='/logout'><button>Logout</button></a>");
  client.println("<br><br><a href='/dark'><button class='small-btn'>Dark Mode</button></a>");
  client.println("<button class='fullscreen-btn small-btn' onclick='goFullscreen()'>Fullscreen</button>");
}

client.println("</div>");
client.println("</body></html>");

client.stop();

}
}
