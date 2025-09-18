#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

// GPIO pins for the 4 pump relays (GPIO 2 is the built-in LED)
const int PUMP_PINS[4] = {23, 22, 2, 19};

// Pump state tracking
bool pumpStates[4] = {false, false, false, false};

// Web server and WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Function declarations
void setPumpState(int pumpId, bool state);
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                     AwsEventType type, void *arg, uint8_t *data, size_t len);
void setupWiFiAP();
String getMainPage();

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Pool Controller Starting...");
    
    // Initialize pump relay pins
    for (int i = 0; i < 4; i++) {
        pinMode(PUMP_PINS[i], OUTPUT);
        digitalWrite(PUMP_PINS[i], LOW);  // Start with all pumps OFF
        Serial.printf("Initialized pump %d on GPIO %d\n", i, PUMP_PINS[i]);
    }
    
    // Setup WiFi Access Point
    setupWiFiAP();
    
    // Setup WebSocket
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);
    
    // Captive Portal - redirect all requests to our main page
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->host() != "192.168.4.1") {
            request->redirect("http://192.168.4.1/");
            return;
        }
        request->send(200, "text/html", getMainPage());
    });
    
    // Main page with enhanced interface
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", getMainPage());
    });
    
    // DNS responses for captive portal
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://192.168.4.1/");
    });
    
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://192.168.4.1/");
    });
    
    // Start server
    server.begin();
    Serial.println("Web server started on http://192.168.4.1");
}

void loop() {
    // Clean up WebSocket connections
    ws.cleanupClients();
    
    // Main loop - other tasks will be added here later
    delay(10);
}

void setupWiFiAP() {
    const char* ssid = "Quinta-dos-Britos";
    const char* password = "poolcontrol123";
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Access Point Started. IP: ");
    Serial.println(IP);
    Serial.printf("SSID: %s\n", ssid);
    Serial.printf("Password: %s\n", password);
}

void setPumpState(int pumpId, bool state) {
    // Validate pump ID
    if (pumpId < 0 || pumpId >= 4) {
        Serial.printf("ERROR: Invalid pump ID %d. Must be 0-3.\n", pumpId);
        return;
    }
    
    // Update pump state
    pumpStates[pumpId] = state;
    digitalWrite(PUMP_PINS[pumpId], state ? HIGH : LOW);
    
    Serial.printf("Pump %d set to %s (GPIO %d)\n", 
                  pumpId, state ? "ON" : "OFF", PUMP_PINS[pumpId]);
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                     AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", 
                         client->id(), client->remoteIP().toString().c_str());
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
            
        case WS_EVT_DATA: {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                // Parse incoming JSON message
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, data, len);
                
                if (error) {
                    Serial.printf("JSON parsing failed: %s\n", error.c_str());
                    return;
                }
                
                // Handle pump control commands
                if (doc["action"] == "set_pump") {
                    if (!doc["pump_id"].isNull() && !doc["state"].isNull()) {
                        int pumpId = doc["pump_id"];
                        bool pumpState = doc["state"];
                        
                        Serial.printf("Received command: pump %d -> %s\n", 
                                     pumpId, pumpState ? "ON" : "OFF");
                        
                        // Execute pump control
                        setPumpState(pumpId, pumpState);
                        
                        // Broadcast confirmation to all clients
                        JsonDocument response;
                        response["action"] = "pump_status";
                        response["pump_id"] = pumpId;
                        response["state"] = pumpState;
                        response["success"] = true;
                        
                        String responseStr;
                        serializeJson(response, responseStr);
                        ws.textAll(responseStr);
                        
                    } else {
                        Serial.println("ERROR: Missing pump_id or state in command");
                    }
                } else {
                    Serial.printf("Unknown action: %s\n", doc["action"].as<const char*>());
                }
            }
            break;
        }
        
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

String getMainPage() {
    return R"(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Quinta dos Britos - Piscina</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh; padding: 20px; color: white;
        }
        .container { max-width: 600px; margin: 0 auto; }
        .header { text-align: center; margin-bottom: 30px; }
        .header h1 { font-size: 2.5em; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }
        .header p { opacity: 0.9; font-size: 1.1em; }
        .status { 
            background: rgba(255,255,255,0.15); border-radius: 15px; 
            padding: 20px; margin-bottom: 20px; backdrop-filter: blur(10px);
        }
        .pump-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 20px; }
        .pump-card { 
            background: rgba(255,255,255,0.2); border-radius: 12px; 
            padding: 20px; text-align: center; transition: all 0.3s ease;
            backdrop-filter: blur(10px); border: 1px solid rgba(255,255,255,0.3);
        }
        .pump-card:hover { transform: translateY(-2px); background: rgba(255,255,255,0.25); }
        .pump-card h3 { margin-bottom: 15px; font-size: 1.2em; }
        .switch { 
            position: relative; display: inline-block; width: 60px; height: 34px;
            margin: 10px 0;
        }
        .switch input { opacity: 0; width: 0; height: 0; }
        .slider { 
            position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0;
            background-color: rgba(255,255,255,0.3); transition: .4s; border-radius: 34px;
        }
        .slider:before { 
            position: absolute; content: ""; height: 26px; width: 26px; left: 4px; bottom: 4px;
            background-color: white; transition: .4s; border-radius: 50%;
        }
        input:checked + .slider { background-color: #4CAF50; }
        input:checked + .slider:before { transform: translateX(26px); }
        .connection-status { 
            text-align: center; padding: 15px; margin-top: 20px;
            border-radius: 10px; font-weight: bold;
        }
        .connected { background: rgba(76, 175, 80, 0.3); }
        .disconnected { background: rgba(244, 67, 54, 0.3); }
        .logs { 
            background: rgba(0,0,0,0.3); border-radius: 10px; padding: 15px;
            margin-top: 20px; font-family: monospace; font-size: 0.9em;
            max-height: 200px; overflow-y: auto;
        }
        
        /* Animações para simular funcionamento */
        .pump-animation {
            position: absolute; top: 10px; right: 10px; width: 20px; height: 20px;
            border-radius: 50%; opacity: 0; transition: all 0.3s ease;
        }
        
        .pump-active .pump-animation {
            opacity: 1; animation: pumpWorking 1.5s infinite ease-in-out;
        }
        
        .circulation .pump-animation { background: #2196F3; }
        .filtration .pump-animation { background: #4CAF50; }
        .heating .pump-animation { background: #FF5722; }
        .border .pump-animation { background: #9C27B0; }
        
        @keyframes pumpWorking {
            0%, 100% { transform: scale(1); opacity: 0.7; }
            50% { transform: scale(1.3); opacity: 1; }
        }
        
        /* Animação de ondas para simular água */
        .water-animation {
            position: absolute; bottom: 0; left: 0; right: 0; height: 4px;
            background: linear-gradient(90deg, transparent, rgba(33, 150, 243, 0.5), transparent);
            border-radius: 0 0 12px 12px; opacity: 0; transition: all 0.3s ease;
        }
        
        .pump-active .water-animation {
            opacity: 1; animation: waterFlow 2s infinite linear;
        }
        
        @keyframes waterFlow {
            0% { transform: translateX(-100%); }
            100% { transform: translateX(100%); }
        }
        
        /* Melhoria visual para estado ativo */
        .pump-card { position: relative; overflow: hidden; }
        .pump-active { 
            box-shadow: 0 0 20px rgba(76, 175, 80, 0.4);
            border-color: rgba(76, 175, 80, 0.6) !important;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🏛️ Quinta dos Britos</h1>
            <p>🏊 Sistema de Automação da Piscina</p>
        </div>
        
        <div class="status">
            <h2>Status do Sistema</h2>
            <p>MAC: e4:65:b8:79:42:6c</p>
            <p>IP: 192.168.4.1</p>
            <p>Uptime: <span id="uptime">--</span></p>
        </div>
        
        <div class="pump-grid">
            <div class="pump-card circulation" id="card0">
                <div class="pump-animation"></div>
                <div class="water-animation"></div>
                <h3>💧 Circulação</h3>
                <label class="switch">
                    <input type="checkbox" id="pump0">
                    <span class="slider"></span>
                </label>
                <p>GPIO 23</p>
            </div>
            <div class="pump-card filtration" id="card1">
                <div class="pump-animation"></div>
                <div class="water-animation"></div>
                <h3>🔧 Filtragem</h3>
                <label class="switch">
                    <input type="checkbox" id="pump1">
                    <span class="slider"></span>
                </label>
                <p>GPIO 22</p>
            </div>
            <div class="pump-card heating" id="card2">
                <div class="pump-animation"></div>
                <div class="water-animation"></div>
                <h3>🔥 Aquecimento</h3>
                <label class="switch">
                    <input type="checkbox" id="pump2">
                    <span class="slider"></span>
                </label>
                <p>GPIO 2 (LED)</p>
            </div>
            <div class="pump-card border" id="card3">
                <div class="pump-animation"></div>
                <div class="water-animation"></div>
                <h3>🌊 Borda Infinita</h3>
                <label class="switch">
                    <input type="checkbox" id="pump3">
                    <span class="slider"></span>
                </label>
                <p>GPIO 19</p>
            </div>
        </div>
        
        <div id="connectionStatus" class="connection-status disconnected">
            🔴 Desconectado do WebSocket
        </div>
        
        <div class="logs">
            <strong>Logs do Sistema:</strong>
            <div id="logs"></div>
        </div>
    </div>

    <script>
        let ws;
        let startTime = Date.now();
        
        function connectWebSocket() {
            ws = new WebSocket('ws://192.168.4.1/ws');
            
            ws.onopen = function() {
                document.getElementById('connectionStatus').textContent = '🟢 Conectado ao WebSocket';
                document.getElementById('connectionStatus').className = 'connection-status connected';
                addLog('WebSocket conectado');
            };
            
            ws.onclose = function() {
                document.getElementById('connectionStatus').textContent = '🔴 Desconectado do WebSocket';
                document.getElementById('connectionStatus').className = 'connection-status disconnected';
                addLog('WebSocket desconectado - tentando reconectar...');
                setTimeout(connectWebSocket, 3000);
            };
            
            ws.onmessage = function(event) {
                const data = JSON.parse(event.data);
                addLog('Recebido: ' + JSON.stringify(data));
                
                if (data.action === 'pump_status') {
                    document.getElementById('pump' + data.pump_id).checked = data.state;
                    updatePumpAnimation(data.pump_id, data.state);
                }
            };
        }
        
        function addLog(message) {
            const logs = document.getElementById('logs');
            const time = new Date().toLocaleTimeString();
            logs.innerHTML += `<div>${time}: ${message}</div>`;
            logs.scrollTop = logs.scrollHeight;
        }
        
        function updateUptime() {
            const uptime = Math.floor((Date.now() - startTime) / 1000);
            const minutes = Math.floor(uptime / 60);
            const seconds = uptime % 60;
            document.getElementById('uptime').textContent = `${minutes}m ${seconds}s`;
        }
        
        function updatePumpAnimation(pumpId, isActive) {
            const card = document.getElementById('card' + pumpId);
            if (isActive) {
                card.classList.add('pump-active');
                addLog(`💧 Bomba ${pumpId} LIGADA - Animação iniciada`);
            } else {
                card.classList.remove('pump-active');
                addLog(`⏹️ Bomba ${pumpId} DESLIGADA - Animação parada`);
            }
        }
        
        // Setup pump controls
        for (let i = 0; i < 4; i++) {
            document.getElementById('pump' + i).addEventListener('change', function() {
                const command = {
                    action: 'set_pump',
                    pump_id: i,
                    state: this.checked
                };
                
                if (ws && ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify(command));
                    addLog('Enviado: ' + JSON.stringify(command));
                    updatePumpAnimation(i, this.checked);
                } else {
                    addLog('Erro: WebSocket não conectado');
                    this.checked = !this.checked; // Revert
                }
            });
        }
        
        // Initialize
        connectWebSocket();
        setInterval(updateUptime, 1000);
        addLog('Interface carregada');
    </script>
</body>
</html>
)";
}
